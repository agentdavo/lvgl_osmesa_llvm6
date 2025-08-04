#ifndef DX8GL_GL_ERROR_CHECK_H
#define DX8GL_GL_ERROR_CHECK_H

#include "osmesa_gl_loader.h"
#include "logger.h"
#include <string>

namespace dx8gl {

// Convert GL error code to string
inline const char* gl_error_string(GLenum error) {
    switch (error) {
        case GL_NO_ERROR: return "GL_NO_ERROR";
        case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
        case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
        case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
        case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
        default: return "Unknown GL error";
    }
}

// Check and log OpenGL errors - returns true if error occurred
inline bool check_gl_error_safe(const char* operation) {
    bool had_error = false;
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        DX8GL_ERROR("OpenGL error after %s: %s (0x%04X)", 
                    operation, gl_error_string(error), error);
        had_error = true;
    }
    return had_error;
}

// Macro for checking GL errors in debug builds
#ifdef NDEBUG
    #define GL_CHECK(x) x
#else
    #define GL_CHECK(x) do { \
        x; \
        check_gl_error(#x); \
    } while(0)
#endif

// Check GL errors and return false on error
inline bool gl_call_safe(const char* operation) {
    return !check_gl_error_safe(operation);
}

} // namespace dx8gl

#endif // DX8GL_GL_ERROR_CHECK_H