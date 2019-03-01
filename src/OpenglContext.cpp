#include "OpenglContext.hpp"
#include <thread>
#include <mutex>
#include <sstream>

namespace mmcv
{

    static std::mutex opengl_ctx_mutex;

	static int64_t get_thread_id()
	{
		std::stringstream ss;
		ss << std::this_thread::get_id();
		return std::stoull(ss.str());
	}

#ifdef _WIN32

    void InitContext(GLContext* ctx)
    {
        ctx->wnd = NULL;
        ctx->dc = NULL;
        ctx->rc = NULL;
    }

    GLboolean CreateContext(GLContext* ctx, int max_gpus, int gpu_index)
    {
        std::lock_guard<std::mutex> lock(opengl_ctx_mutex);

        int visual = -1;

        WNDCLASS wc;
        PIXELFORMATDESCRIPTOR pfd;
        /* check for input */
        if (NULL == ctx) return GL_TRUE;
        /* register window class */
        ZeroMemory(&wc, sizeof(WNDCLASS));
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpfnWndProc = DefWindowProc;
        std::stringstream ss;
        ss << "GLEW_" <<  get_thread_id();
        ctx->cn = ss.str();
        wc.lpszClassName = ctx->cn.c_str();
        if (0 == RegisterClass(&wc)) return GL_TRUE;
        /* create window */
        ctx->wnd = CreateWindow(ctx->cn.c_str(), ctx->cn.c_str(), 0, 0, 0,
            512, 512, NULL, NULL,
            GetModuleHandle(NULL), NULL);
        if (NULL == ctx->wnd) return GL_TRUE;
        /* get the device context */
        ctx->dc = GetDC(ctx->wnd);
        if (NULL == ctx->dc) return GL_TRUE;
        /* find pixel format */
        ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
        if (visual == -1) /* find default */
        {
            pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
            pfd.nVersion = 1;
            pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
            visual = ChoosePixelFormat(ctx->dc, &pfd);
            if (0 == visual) return GL_TRUE;
        }
        /* set the pixel format for the dc */
        if (FALSE == SetPixelFormat(ctx->dc, visual, &pfd)) return GL_TRUE;
        /* create rendering context */
        ctx->rc = wglCreateContext(ctx->dc);
        if (NULL == ctx->rc) return GL_TRUE;
        if (FALSE == wglMakeCurrent(ctx->dc, ctx->rc)) return GL_TRUE;
        return GL_FALSE;
    }

    void DestroyContext(GLContext* ctx)
    {
        //std::lock_guard<std::mutex> lock(opengl_ctx_mutex);

        if (NULL == ctx) return;
        if (NULL != ctx->rc) wglMakeCurrent(NULL, NULL);
        if (NULL != ctx->rc) wglDeleteContext(ctx->rc);
        if (NULL != ctx->wnd && NULL != ctx->dc) ReleaseDC(ctx->wnd, ctx->dc);
        if (NULL != ctx->wnd) DestroyWindow(ctx->wnd);
        UnregisterClass(ctx->cn.c_str(), GetModuleHandle(NULL));
    }

#elif defined(__APPLE__) && !defined(GLEW_APPLE_GLX)

    void InitContext(GLContext* ctx)
    {
        ctx->ctx = NULL;
        ctx->octx = NULL;
    }

    GLboolean CreateContext(GLContext* ctx, int max_gpus, int gpu_index)
    {
        std::lock_guard<std::mutex> lock(opengl_ctx_mutex);

        CGLContextObj context;
        CGLPixelFormatAttribute attributes[13] = {
            kCGLPFAOpenGLProfile,
            (CGLPixelFormatAttribute) kCGLOGLPVersion_3_2_Core,
            kCGLPFAAccelerated,
            kCGLPFAColorSize, (CGLPixelFormatAttribute)24,
            kCGLPFAAlphaSize, (CGLPixelFormatAttribute)8,
            kCGLPFADoubleBuffer,
            kCGLPFASampleBuffers, (CGLPixelFormatAttribute)1,
            kCGLPFASamples,  (CGLPixelFormatAttribute)4,
            (CGLPixelFormatAttribute)0
        };
        CGLPixelFormatObj pix;
        CGLError errorCode;
        GLint num;
        errorCode = CGLChoosePixelFormat( attributes, &pix, &num );
        errorCode = CGLCreateContext(pix, NULL, &ctx->ctx);
        CGLDestroyPixelFormat( pix );
        ctx->octx = CGLGetCurrentContext();
        errorCode = CGLSetCurrentContext(ctx->ctx);
    }

    void DestroyContext(GLContext* ctx)
    {
        std::lock_guard<std::mutex> lock(opengl_ctx_mutex);

        if (NULL == ctx) return;
        CGLSetCurrentContext(ctx->octx);
        if (NULL != ctx->ctx) CGLReleaseContext(ctx->ctx);
    }

#else

    void InitContext(GLContext* ctx)
    {
        ctx->ctx = NULL;
    }

    GLboolean CreateContext(GLContext* ctx, int max_gpus, int gpu_index)
    {
        std::lock_guard<std::mutex> lock(opengl_ctx_mutex);

        EGLDeviceEXT devices[10];
        EGLint numDevices;
        EGLSurface  surface;
        EGLint majorVersion, minorVersion;
        EGLint configAttribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, 1,
            EGL_GREEN_SIZE, 1,
            EGL_BLUE_SIZE, 1,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
            EGL_NONE
        };
        static const EGLint contextAttribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };
        static const EGLint pBufferAttribs[] = {
            EGL_WIDTH,  512,
            EGL_HEIGHT, 512,
            EGL_NONE
        };
        EGLConfig config;
        EGLint numConfig;
        EGLBoolean pBuffer;

        PFNEGLQUERYDEVICESEXTPROC       queryDevices = NULL;
        PFNEGLGETPLATFORMDISPLAYEXTPROC getPlatformDisplay = NULL;
        PFNEGLGETERRORPROC              getError = NULL;
        PFNEGLGETDISPLAYPROC            getDisplay = NULL;
        PFNEGLINITIALIZEPROC            initialize = NULL;
        PFNEGLBINDAPIPROC               bindAPI = NULL;
        PFNEGLCHOOSECONFIGPROC          chooseConfig = NULL;
        PFNEGLCREATEWINDOWSURFACEPROC   createWindowSurface = NULL;
        PFNEGLCREATECONTEXTPROC         createContext = NULL;
        PFNEGLMAKECURRENTPROC           makeCurrent = NULL;
        PFNEGLCREATEPBUFFERSURFACEPROC  createPbufferSurface = NULL;

        /* Load necessary entry points */
        queryDevices = (PFNEGLQUERYDEVICESEXTPROC)eglGetProcAddress("eglQueryDevicesEXT");
        getPlatformDisplay = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
        getError = (PFNEGLGETERRORPROC)eglGetProcAddress("eglGetError");
        getDisplay = (PFNEGLGETDISPLAYPROC)eglGetProcAddress("eglGetDisplay");
        initialize = (PFNEGLINITIALIZEPROC)eglGetProcAddress("eglInitialize");
        bindAPI = (PFNEGLBINDAPIPROC)eglGetProcAddress("eglBindAPI");
        chooseConfig = (PFNEGLCHOOSECONFIGPROC)eglGetProcAddress("eglChooseConfig");
        createWindowSurface = (PFNEGLCREATEWINDOWSURFACEPROC)eglGetProcAddress("eglCreateWindowSurface");
        createPbufferSurface = (PFNEGLCREATEPBUFFERSURFACEPROC)eglGetProcAddress("eglCreatePbufferSurface");
        createContext = (PFNEGLCREATECONTEXTPROC)eglGetProcAddress("eglCreateContext");
        makeCurrent = (PFNEGLMAKECURRENTPROC)eglGetProcAddress("eglMakeCurrent");
        if (!getError || !getDisplay || !initialize || !bindAPI || !chooseConfig || !createWindowSurface || !createContext || !makeCurrent)
            return GL_TRUE;

        pBuffer = 0;
        ctx->display = EGL_NO_DISPLAY;
        if (queryDevices && getPlatformDisplay)
        {
            queryDevices(max_gpus, devices, &numDevices);
            if (numDevices == max_gpus)
            {
                /* Nvidia EGL doesn't need X11 for p-buffer surface */
                ctx->display = getPlatformDisplay(EGL_PLATFORM_DEVICE_EXT, devices[gpu_index], 0);
                configAttribs[1] = EGL_PBUFFER_BIT;
                pBuffer = 1;
            }
        }
        if (ctx->display == EGL_NO_DISPLAY)
        {
            /* Fall-back to X11 surface, works on Mesa */
            ctx->display = getDisplay(EGL_DEFAULT_DISPLAY);
        }
        if (ctx->display == EGL_NO_DISPLAY)
            return GL_TRUE;

        eglewInit(ctx->display);

        if (bindAPI(EGL_OPENGL_API) != EGL_TRUE)
            return GL_TRUE;

        if (chooseConfig(ctx->display, configAttribs, &config, 1, &numConfig) != EGL_TRUE || (numConfig != 1))
            return GL_TRUE;

        ctx->ctx = createContext(ctx->display, config, EGL_NO_CONTEXT, pBuffer ? contextAttribs : NULL);
        if (NULL == ctx->ctx)
            return GL_TRUE;

        surface = EGL_NO_SURFACE;
        /* Create a p-buffer surface if possible */
        if (pBuffer && createPbufferSurface)
        {
            surface = createPbufferSurface(ctx->display, config, pBufferAttribs);
        }
        /* Create a generic surface without a native window, if necessary */
        if (surface == EGL_NO_SURFACE)
        {
            surface = createWindowSurface(ctx->display, config, (EGLNativeWindowType)NULL, NULL);
        }
#if 0
        if (surface == EGL_NO_SURFACE)
            return GL_TRUE;
#endif

        if (makeCurrent(ctx->display, surface, surface, ctx->ctx) != EGL_TRUE)
            return GL_TRUE;

        return GL_FALSE;
    }

    void DestroyContext(GLContext* ctx)
    {
        std::lock_guard<std::mutex> lock(opengl_ctx_mutex);

        if (NULL != ctx->ctx) eglDestroyContext(ctx->display, ctx->ctx);
    }

#endif

}
