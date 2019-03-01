#include <stdint.h>
#include <OpenglContext.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex> 

using namespace std;

GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
		case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

int main()
{

#if defined(GLEW_EGL)
	typedef const GLubyte* (GLAPIENTRY * PFNGLGETSTRINGPROC) (GLenum name);
	PFNGLGETSTRINGPROC getString;
#endif

	mmcv::GLContext ctx;
	if (mmcv::CreateContext(&ctx, 1, 0) != 0)
	{
		std::cout << "Error: CreateContext failed" << std::endl;
		DestroyContext(&ctx);
	}
	glewInit();

#if defined(GLEW_EGL)
	getString = (PFNGLGETSTRINGPROC)eglGetProcAddress("glGetString");
	if (!getString)
	{
		fprintf(stderr, "Error: eglGetProcAddress failed to fetch glGetString\n");
		DestroyContext(&ctx);
		return 1;
	}
#endif

	glCheckError();
	const GLubyte* glstring = glGetString(GL_VERSION);
	std::cout << "opengl version: " << glstring << std::endl;
	glCheckError();
	std::cout << "finish" << std::endl;
    return 0;
}