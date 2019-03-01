#pragma once

#include <GL/glew.h>
#ifdef _WIN32
#include <GL/wglew.h>
#elif defined(__APPLE__) && !defined(GLEW_APPLE_GLX)
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLTypes.h>
#include <Opengl/glext.h>
#include <opengl/gl3.h>
#else
#include <GL/eglew.h>
#endif
#include <string>

namespace mmcv
{

    typedef struct GLContextStruct
    {
#ifdef _WIN32
        HWND wnd;
        HDC dc;
        HGLRC rc;
        std::string cn;
#elif defined(__APPLE__) && !defined(GLEW_APPLE_GLX)
        CGLContextObj ctx, octx;
#else
        EGLDisplay display;
        EGLContext ctx;
#endif
    } GLContext;

    void InitContext(GLContext* ctx);

    GLboolean CreateContext(GLContext* ctx, int max_gpus, int gpu_index);

    void DestroyContext(GLContext* ctx);

}
