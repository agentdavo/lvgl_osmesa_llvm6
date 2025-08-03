/**
 * @file gles2_headers.h
 * @brief Centralized OpenGL ES 2.0 header management for dx8gl
 * 
 * This file provides a unified interface for OpenGL ES 2.0 headers across
 * different platforms and configurations. It replaces the scattered ES 1.1
 * includes throughout the codebase.
 */

#ifndef DX8GL_GLES2_HEADERS_H
#define DX8GL_GLES2_HEADERS_H

// ES 2.0 is now mandatory - no longer conditional

// Platform-specific GL headers
#ifdef __APPLE__
    // macOS uses different headers for OpenGL
    #include <OpenGL/gl.h>
    #include <OpenGL/glext.h>
    
    // Define ES 2.0 specific constants that might be missing
    #ifndef GL_ES_VERSION_2_0
        #define GL_ES_VERSION_2_0 1
    #endif
#else
    // Standard OpenGL ES 2.0 headers
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>
#endif

// Common OpenGL constants that might be missing or renamed
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

// ES 2.0 shader stages
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif

#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif

// Missing GL constants that were in ES 1.1 but needed for compatibility
#ifndef GL_SMOOTH
#define GL_SMOOTH 0x1D01
#endif

#ifndef GL_MODULATE
#define GL_MODULATE 0x2100
#endif

#ifndef GL_TEXTURE_ENV_MODE
#define GL_TEXTURE_ENV_MODE 0x2200
#endif

#ifndef GL_COMBINE_RGB
#define GL_COMBINE_RGB 0x8571
#endif

#ifndef GL_COMBINE_ALPHA
#define GL_COMBINE_ALPHA 0x8572
#endif

// Light parameters
#ifndef GL_POSITION
#define GL_POSITION 0x1203
#endif

#ifndef GL_DIFFUSE
#define GL_DIFFUSE 0x1201
#endif

#ifndef GL_AMBIENT
#define GL_AMBIENT 0x1200
#endif

#ifndef GL_SPECULAR
#define GL_SPECULAR 0x1202
#endif

#ifndef GL_SPOT_DIRECTION
#define GL_SPOT_DIRECTION 0x1204
#endif

#ifndef GL_SPOT_CUTOFF
#define GL_SPOT_CUTOFF 0x1206
#endif

#ifndef GL_CONSTANT_ATTENUATION
#define GL_CONSTANT_ATTENUATION 0x1207
#endif

#ifndef GL_LINEAR_ATTENUATION
#define GL_LINEAR_ATTENUATION 0x1208
#endif

#ifndef GL_QUADRATIC_ATTENUATION
#define GL_QUADRATIC_ATTENUATION 0x1209
#endif

// Material parameters
#ifndef GL_EMISSION
#define GL_EMISSION 0x1600
#endif

#ifndef GL_SHININESS
#define GL_SHININESS 0x1601
#endif

// Fog parameters
#ifndef GL_FOG_MODE
#define GL_FOG_MODE 0x0B65
#endif

#ifndef GL_FOG_COLOR
#define GL_FOG_COLOR 0x0B66
#endif

#ifndef GL_FOG_DENSITY
#define GL_FOG_DENSITY 0x0B62
#endif

#ifndef GL_FOG_START
#define GL_FOG_START 0x0B63
#endif

#ifndef GL_FOG_END
#define GL_FOG_END 0x0B64
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

// Function pointer types for GL ES 2.0 extensions we might need
// Only define if not already provided by system headers
#ifndef GL_OES_vertex_array_object
typedef void (GL_APIENTRYP PFNGLGENVERTEXARRAYSOESPROC) (GLsizei n, GLuint *arrays);
typedef void (GL_APIENTRYP PFNGLBINDVERTEXARRAYOESPROC) (GLuint array);
typedef void (GL_APIENTRYP PFNGLDELETEVERTEXARRAYSOESPROC) (GLsizei n, const GLuint *arrays);

// Extension function pointers (loaded at runtime)
extern PFNGLGENVERTEXARRAYSOESPROC glGenVertexArraysOES;
extern PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOES;
extern PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOES;
#endif

// Initialize GL ES 2.0 extensions
void dx8gl_init_gles2_extensions(void);

#endif /* DX8GL_GLES2_HEADERS_H */