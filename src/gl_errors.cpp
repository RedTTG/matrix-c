#include "gl_errors.h"

#include <iostream>
#include <ostream>
#include <string>

#ifdef __ANDROID__
#include <GLES3/gl3.h>
#else
#include "glad.h"
#endif

void checkGLError(const char *call, const char *file, int line) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (err) {
            case GL_INVALID_ENUM:                  error = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             error = "GL_INVALID_OPERATION"; break;
#ifndef __ANDROID__
            case GL_STACK_OVERFLOW:                error = "GL_STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW:               error = "GL_STACK_UNDERFLOW"; break;
#endif
            case GL_OUT_OF_MEMORY:                 error = "GL_OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            default:                               error = "UNKNOWN_ERROR"; break;
        }
        std::cerr << "OpenGL Error: " << error
                  << " in call: " << call
                  << " at " << file << ":" << line << std::endl;
    }
}
