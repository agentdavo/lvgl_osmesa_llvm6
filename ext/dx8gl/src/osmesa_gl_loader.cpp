#include "osmesa_gl_loader.h"
#include "logger.h"
#include <GL/osmesa.h>

namespace dx8gl {

// Define function pointers
PFNGLGENBUFFERSPROC gl_GenBuffers = nullptr;
PFNGLBINDBUFFERPROC gl_BindBuffer = nullptr;
PFNGLBUFFERDATAPROC gl_BufferData = nullptr;
PFNGLBUFFERSUBDATAPROC gl_BufferSubData = nullptr;
PFNGLDELETEBUFFERSPROC gl_DeleteBuffers = nullptr;
PFNGLGENVERTEXARRAYSPROC gl_GenVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYPROC gl_BindVertexArray = nullptr;
PFNGLDELETEVERTEXARRAYSPROC gl_DeleteVertexArrays = nullptr;
PFNGLCREATESHADERPROC gl_CreateShader = nullptr;
PFNGLSHADERSOURCEPROC gl_ShaderSource = nullptr;
PFNGLCOMPILESHADERPROC gl_CompileShader = nullptr;
PFNGLDELETESHADERPROC gl_DeleteShader = nullptr;
PFNGLGETSHADERIVPROC gl_GetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC gl_GetShaderInfoLog = nullptr;
PFNGLCREATEPROGRAMPROC gl_CreateProgram = nullptr;
PFNGLATTACHSHADERPROC gl_AttachShader = nullptr;
PFNGLLINKPROGRAMPROC gl_LinkProgram = nullptr;
PFNGLUSEPROGRAMPROC gl_UseProgram = nullptr;
PFNGLDELETEPROGRAMPROC gl_DeleteProgram = nullptr;
PFNGLGETPROGRAMIVPROC gl_GetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC gl_GetProgramInfoLog = nullptr;
PFNGLGETUNIFORMLOCATIONPROC gl_GetUniformLocation = nullptr;
PFNGLUNIFORM1IPROC gl_Uniform1i = nullptr;
PFNGLUNIFORM1FPROC gl_Uniform1f = nullptr;
PFNGLUNIFORM4FPROC gl_Uniform4f = nullptr;
PFNGLUNIFORMMATRIX4FVPROC gl_UniformMatrix4fv = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC gl_VertexAttribPointer = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC gl_EnableVertexAttribArray = nullptr;
PFNGLDISABLEVERTEXATTRIBARRAYPROC gl_DisableVertexAttribArray = nullptr;
PFNGLGETATTRIBLOCATIONPROC gl_GetAttribLocation = nullptr;

// Helper macro to load a function - note we pass the variable name but need to use the GL function name
#define LOAD_GL_FUNC(var_name, gl_name, type_name) \
    do { \
        var_name = (type_name)OSMesaGetProcAddress(gl_name); \
        if (!var_name) { \
            /* Try with ARB suffix */ \
            var_name = (type_name)OSMesaGetProcAddress(gl_name "ARB"); \
            if (!var_name) { \
                /* Try with EXT suffix */ \
                var_name = (type_name)OSMesaGetProcAddress(gl_name "EXT"); \
                if (!var_name) { \
                    DX8GL_ERROR("Failed to load %s (tried base, ARB, and EXT)", gl_name); \
                    success = false; \
                } \
            } \
        } \
    } while(0)

bool InitializeOSMesaGL() {
    DX8GL_INFO("Initializing OSMesa GL function pointers");
    
    bool success = true;
    
    // Buffer functions
    gl_GenBuffers = (PFNGLGENBUFFERSPROC)OSMesaGetProcAddress("glGenBuffers");
    if (!gl_GenBuffers) gl_GenBuffers = (PFNGLGENBUFFERSPROC)OSMesaGetProcAddress("glGenBuffersARB");
    if (!gl_GenBuffers) { DX8GL_ERROR("Failed to load glGenBuffers"); success = false; }
    
    gl_BindBuffer = (PFNGLBINDBUFFERPROC)OSMesaGetProcAddress("glBindBuffer");
    if (!gl_BindBuffer) gl_BindBuffer = (PFNGLBINDBUFFERPROC)OSMesaGetProcAddress("glBindBufferARB");
    if (!gl_BindBuffer) { DX8GL_ERROR("Failed to load glBindBuffer"); success = false; }
    
    gl_BufferData = (PFNGLBUFFERDATAPROC)OSMesaGetProcAddress("glBufferData");
    if (!gl_BufferData) gl_BufferData = (PFNGLBUFFERDATAPROC)OSMesaGetProcAddress("glBufferDataARB");
    if (!gl_BufferData) { DX8GL_ERROR("Failed to load glBufferData"); success = false; }
    
    gl_BufferSubData = (PFNGLBUFFERSUBDATAPROC)OSMesaGetProcAddress("glBufferSubData");
    if (!gl_BufferSubData) gl_BufferSubData = (PFNGLBUFFERSUBDATAPROC)OSMesaGetProcAddress("glBufferSubDataARB");
    if (!gl_BufferSubData) { DX8GL_ERROR("Failed to load glBufferSubData"); success = false; }
    
    gl_DeleteBuffers = (PFNGLDELETEBUFFERSPROC)OSMesaGetProcAddress("glDeleteBuffers");
    if (!gl_DeleteBuffers) gl_DeleteBuffers = (PFNGLDELETEBUFFERSPROC)OSMesaGetProcAddress("glDeleteBuffersARB");
    if (!gl_DeleteBuffers) { DX8GL_ERROR("Failed to load glDeleteBuffers"); success = false; }
    
    // VAO functions
    LOAD_GL_FUNC(gl_GenVertexArrays, "glGenVertexArrays", PFNGLGENVERTEXARRAYSPROC);
    LOAD_GL_FUNC(gl_BindVertexArray, "glBindVertexArray", PFNGLBINDVERTEXARRAYPROC);
    LOAD_GL_FUNC(gl_DeleteVertexArrays, "glDeleteVertexArrays", PFNGLDELETEVERTEXARRAYSPROC);
    
    // Shader functions
    LOAD_GL_FUNC(gl_CreateShader, "glCreateShader", PFNGLCREATESHADERPROC);
    LOAD_GL_FUNC(gl_ShaderSource, "glShaderSource", PFNGLSHADERSOURCEPROC);
    LOAD_GL_FUNC(gl_CompileShader, "glCompileShader", PFNGLCOMPILESHADERPROC);
    LOAD_GL_FUNC(gl_DeleteShader, "glDeleteShader", PFNGLDELETESHADERPROC);
    LOAD_GL_FUNC(gl_GetShaderiv, "glGetShaderiv", PFNGLGETSHADERIVPROC);
    LOAD_GL_FUNC(gl_GetShaderInfoLog, "glGetShaderInfoLog", PFNGLGETSHADERINFOLOGPROC);
    
    // Program functions
    LOAD_GL_FUNC(gl_CreateProgram, "glCreateProgram", PFNGLCREATEPROGRAMPROC);
    LOAD_GL_FUNC(gl_AttachShader, "glAttachShader", PFNGLATTACHSHADERPROC);
    LOAD_GL_FUNC(gl_LinkProgram, "glLinkProgram", PFNGLLINKPROGRAMPROC);
    LOAD_GL_FUNC(gl_UseProgram, "glUseProgram", PFNGLUSEPROGRAMPROC);
    LOAD_GL_FUNC(gl_DeleteProgram, "glDeleteProgram", PFNGLDELETEPROGRAMPROC);
    LOAD_GL_FUNC(gl_GetProgramiv, "glGetProgramiv", PFNGLGETPROGRAMIVPROC);
    LOAD_GL_FUNC(gl_GetProgramInfoLog, "glGetProgramInfoLog", PFNGLGETPROGRAMINFOLOGPROC);
    
    // Uniform functions
    LOAD_GL_FUNC(gl_GetUniformLocation, "glGetUniformLocation", PFNGLGETUNIFORMLOCATIONPROC);
    LOAD_GL_FUNC(gl_Uniform1i, "glUniform1i", PFNGLUNIFORM1IPROC);
    LOAD_GL_FUNC(gl_Uniform1f, "glUniform1f", PFNGLUNIFORM1FPROC);
    LOAD_GL_FUNC(gl_Uniform4f, "glUniform4f", PFNGLUNIFORM4FPROC);
    LOAD_GL_FUNC(gl_UniformMatrix4fv, "glUniformMatrix4fv", PFNGLUNIFORMMATRIX4FVPROC);
    
    // Vertex attribute functions
    LOAD_GL_FUNC(gl_VertexAttribPointer, "glVertexAttribPointer", PFNGLVERTEXATTRIBPOINTERPROC);
    LOAD_GL_FUNC(gl_EnableVertexAttribArray, "glEnableVertexAttribArray", PFNGLENABLEVERTEXATTRIBARRAYPROC);
    LOAD_GL_FUNC(gl_DisableVertexAttribArray, "glDisableVertexAttribArray", PFNGLDISABLEVERTEXATTRIBARRAYPROC);
    LOAD_GL_FUNC(gl_GetAttribLocation, "glGetAttribLocation", PFNGLGETATTRIBLOCATIONPROC);
    
    if (success) {
        DX8GL_INFO("Successfully loaded all OSMesa GL function pointers");
    } else {
        DX8GL_ERROR("Failed to load some OSMesa GL function pointers");
    }
    
    return success;
}

#undef LOAD_GL_FUNC

} // namespace dx8gl