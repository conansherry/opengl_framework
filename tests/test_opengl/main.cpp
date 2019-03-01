#include <stdint.h>
#include <OpenglContext.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex> 

using namespace std;


EGLDisplay  display;
EGLContext  ctx;

/* See: http://stackoverflow.com/questions/12662227/opengl-es2-0-offscreen-context-for-fbo-rendering */

GLboolean glewCreateContext(struct createParams *params)
{
	EGLDeviceEXT devices[1];
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
		EGL_WIDTH,  128,
		EGL_HEIGHT, 128,
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
	display = EGL_NO_DISPLAY;
	if (queryDevices && getPlatformDisplay)
	{
		queryDevices(1, devices, &numDevices);
		if (numDevices == 1)
		{
			/* Nvidia EGL doesn't need X11 for p-buffer surface */
			display = getPlatformDisplay(EGL_PLATFORM_DEVICE_EXT, devices[0], 0);
			configAttribs[1] = EGL_PBUFFER_BIT;
			pBuffer = 1;
		}
	}
	if (display == EGL_NO_DISPLAY)
	{
		/* Fall-back to X11 surface, works on Mesa */
		display = getDisplay(EGL_DEFAULT_DISPLAY);
	}
	if (display == EGL_NO_DISPLAY)
		return GL_TRUE;

	eglewInit(display);

	if (bindAPI(EGL_OPENGL_API) != EGL_TRUE)
		return GL_TRUE;

	if (chooseConfig(display, configAttribs, &config, 1, &numConfig) != EGL_TRUE || (numConfig != 1))
		return GL_TRUE;

	ctx = createContext(display, config, EGL_NO_CONTEXT, pBuffer ? contextAttribs : NULL);
	if (NULL == ctx)
		return GL_TRUE;

	surface = EGL_NO_SURFACE;
	/* Create a p-buffer surface if possible */
	if (pBuffer && createPbufferSurface)
	{
		surface = createPbufferSurface(display, config, pBufferAttribs);
	}
	/* Create a generic surface without a native window, if necessary */
	if (surface == EGL_NO_SURFACE)
	{
		surface = createWindowSurface(display, config, (EGLNativeWindowType)NULL, NULL);
	}
#if 0
	if (surface == EGL_NO_SURFACE)
		return GL_TRUE;
#endif

	if (makeCurrent(display, surface, surface, ctx) != EGL_TRUE)
		return GL_TRUE;

	return GL_FALSE;
}

void glewDestroyContext()
{
	if (NULL != ctx) eglDestroyContext(display, ctx);
}

struct createParams
{
	int         major, minor;  /* GL context version number */

							   /* https://www.opengl.org/registry/specs/ARB/glx_create_context.txt */
	int         profile;       /* core = 1, compatibility = 2 */
	int         flags;         /* debug = 1, forward compatible = 2 */
};

int main()
{
	struct createParams params =
	{
		0,   /* major */
		0,   /* minor */
		0,   /* profile mask */
		0    /* flags */
	};
	if (GL_TRUE == glewCreateContext(&params))
	{
		fprintf(stderr, "Error: glewCreateContext failed\n");
		glewDestroyContext();
		return 1;
	}
	glewExperimental = GL_TRUE;
	int err = glewInit();
	if (GLEW_OK != err)
	{
		fprintf(stderr, "Error [main]: glewInit failed: %s\n", glewGetErrorString(err));
		glewDestroyContext();
		return 1;
	}

    return 0;
}