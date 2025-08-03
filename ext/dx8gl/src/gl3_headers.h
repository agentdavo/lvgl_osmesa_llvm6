/**
 * @file gl3_headers.h
 * @brief Centralized OpenGL ES 3.0 / OpenGL 3.3 Core header management for dx8gl
 * 
 * This file provides a unified interface for OpenGL ES 3.0 and OpenGL 3.3 Core
 * headers across different platforms and configurations.
 */

#ifndef DX8GL_GL3_HEADERS_H
#define DX8GL_GL3_HEADERS_H

// Platform-specific GL headers
#ifdef __APPLE__
    // macOS uses different headers for OpenGL
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
    #include <OpenGL/gl3ext.h>
    
    // Define ES 3.0 specific constants that might be missing
    #ifndef GL_ES_VERSION_3_0
        #define GL_ES_VERSION_3_0 1
    #endif
#else
    #ifdef DX8GL_USE_OPENGL_ES
        // OpenGL ES 3.0 headers
        #include <GLES3/gl3.h>
        #include <GLES3/gl3ext.h>
    #else
        // Desktop OpenGL 3.3 Core headers
        #define GL_GLEXT_PROTOTYPES
        #include <GL/gl.h>
        #include <GL/glext.h>
        
        // Define GL 3.3 Core specific version
        #ifndef GL_VERSION_3_3
            #define GL_VERSION_3_3 1
        #endif
    #endif
#endif

// Common OpenGL constants that might be missing
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

// Vertex Array Object functions
#ifndef GL_VERTEX_ARRAY_BINDING
#define GL_VERTEX_ARRAY_BINDING 0x85B5
#endif

// DirectX 8 compatible constants
#ifndef DX8GL_MAX_LIGHTS
#define DX8GL_MAX_LIGHTS 8
#endif

#ifndef DX8GL_MAX_TEXTURE_UNITS
#define DX8GL_MAX_TEXTURE_UNITS 8
#endif

// Utility macro for maximum
#ifndef DX8GL_MAX
#define DX8GL_MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

// Vertex attribute locations for dx8gl
enum {
    DX8GL_ATTRIB_POSITION = 0,
    DX8GL_ATTRIB_NORMAL = 1,
    DX8GL_ATTRIB_COLOR = 2,
    DX8GL_ATTRIB_TEXCOORD0 = 3,
    DX8GL_ATTRIB_TEXCOORD1 = 4,
    DX8GL_ATTRIB_TEXCOORD2 = 5,
    DX8GL_ATTRIB_TEXCOORD3 = 6,
    DX8GL_ATTRIB_TEXCOORD4 = 7,
    DX8GL_ATTRIB_TEXCOORD5 = 8,
    DX8GL_ATTRIB_TEXCOORD6 = 9,
    DX8GL_ATTRIB_TEXCOORD7 = 10,
    DX8GL_ATTRIB_MAX = 11
};

// Uniform locations for common matrices and state
enum {
    DX8GL_UNIFORM_MVP_MATRIX = 0,
    DX8GL_UNIFORM_MODEL_MATRIX = 1,
    DX8GL_UNIFORM_VIEW_MATRIX = 2,
    DX8GL_UNIFORM_PROJ_MATRIX = 3,
    DX8GL_UNIFORM_NORMAL_MATRIX = 4,
    DX8GL_UNIFORM_TEXTURE_MATRIX = 5,
    DX8GL_UNIFORM_FOG_PARAMS = 6,
    DX8GL_UNIFORM_MATERIAL = 7,
    DX8GL_UNIFORM_LIGHTS = 8,
    DX8GL_UNIFORM_ALPHA_REF = 9,
    DX8GL_UNIFORM_TEXTURE0 = 10,
    DX8GL_UNIFORM_TEXTURE1 = 11,
    DX8GL_UNIFORM_TEXTURE2 = 12,
    DX8GL_UNIFORM_TEXTURE3 = 13,
    DX8GL_UNIFORM_TEXTURE4 = 14,
    DX8GL_UNIFORM_TEXTURE5 = 15,
    DX8GL_UNIFORM_TEXTURE6 = 16,
    DX8GL_UNIFORM_TEXTURE7 = 17,
    DX8GL_UNIFORM_MAX = 18
};

// Helper macros for GL error checking
#ifdef DEBUG
#define DX8GL_CHECK_GL_ERROR() do { \
    GLenum err = glGetError(); \
    if (err != GL_NO_ERROR) { \
        fprintf(stderr, "GL Error: 0x%04x at %s:%d\n", err, __FILE__, __LINE__); \
    } \
} while(0)
#else
#define DX8GL_CHECK_GL_ERROR() ((void)0)
#endif

// Initialize GL 3.3 Core / ES 3.0 extensions if needed
void dx8gl_init_gl3_extensions(void);

#endif /* DX8GL_GL3_HEADERS_H */