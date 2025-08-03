#ifndef DX8GL_OSMESA_GL_LOADER_H
#define DX8GL_OSMESA_GL_LOADER_H

#include <GL/osmesa.h>
#include <GL/gl.h>
#include <GL/glext.h>

namespace dx8gl {

// Function pointer types
extern PFNGLGENBUFFERSPROC gl_GenBuffers;
extern PFNGLBINDBUFFERPROC gl_BindBuffer;
extern PFNGLBUFFERDATAPROC gl_BufferData;
extern PFNGLBUFFERSUBDATAPROC gl_BufferSubData;
extern PFNGLDELETEBUFFERSPROC gl_DeleteBuffers;
extern PFNGLGENVERTEXARRAYSPROC gl_GenVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC gl_BindVertexArray;
extern PFNGLDELETEVERTEXARRAYSPROC gl_DeleteVertexArrays;
extern PFNGLCREATESHADERPROC gl_CreateShader;
extern PFNGLSHADERSOURCEPROC gl_ShaderSource;
extern PFNGLCOMPILESHADERPROC gl_CompileShader;
extern PFNGLDELETESHADERPROC gl_DeleteShader;
extern PFNGLGETSHADERIVPROC gl_GetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC gl_GetShaderInfoLog;
extern PFNGLCREATEPROGRAMPROC gl_CreateProgram;
extern PFNGLATTACHSHADERPROC gl_AttachShader;
extern PFNGLLINKPROGRAMPROC gl_LinkProgram;
extern PFNGLUSEPROGRAMPROC gl_UseProgram;
extern PFNGLDELETEPROGRAMPROC gl_DeleteProgram;
extern PFNGLGETPROGRAMIVPROC gl_GetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC gl_GetProgramInfoLog;
extern PFNGLGETUNIFORMLOCATIONPROC gl_GetUniformLocation;
extern PFNGLUNIFORM1IPROC gl_Uniform1i;
extern PFNGLUNIFORM1FPROC gl_Uniform1f;
extern PFNGLUNIFORM4FPROC gl_Uniform4f;
extern PFNGLUNIFORMMATRIX4FVPROC gl_UniformMatrix4fv;
extern PFNGLVERTEXATTRIBPOINTERPROC gl_VertexAttribPointer;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC gl_EnableVertexAttribArray;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC gl_DisableVertexAttribArray;
extern PFNGLGETATTRIBLOCATIONPROC gl_GetAttribLocation;

// Initialize function pointers via OSMesaGetProcAddress
bool InitializeOSMesaGL();

} // namespace dx8gl

// Wrapper macros that use the function pointers (must be outside namespace)
#ifdef DX8GL_HAS_OSMESA
#define glGenBuffers dx8gl::gl_GenBuffers
#define glBindBuffer dx8gl::gl_BindBuffer
#define glBufferData dx8gl::gl_BufferData
#define glBufferSubData dx8gl::gl_BufferSubData
#define glDeleteBuffers dx8gl::gl_DeleteBuffers
#define glGenVertexArrays dx8gl::gl_GenVertexArrays
#define glBindVertexArray dx8gl::gl_BindVertexArray
#define glDeleteVertexArrays dx8gl::gl_DeleteVertexArrays
#define glCreateShader dx8gl::gl_CreateShader
#define glShaderSource dx8gl::gl_ShaderSource
#define glCompileShader dx8gl::gl_CompileShader
#define glDeleteShader dx8gl::gl_DeleteShader
#define glGetShaderiv dx8gl::gl_GetShaderiv
#define glGetShaderInfoLog dx8gl::gl_GetShaderInfoLog
#define glCreateProgram dx8gl::gl_CreateProgram
#define glAttachShader dx8gl::gl_AttachShader
#define glLinkProgram dx8gl::gl_LinkProgram
#define glUseProgram dx8gl::gl_UseProgram
#define glDeleteProgram dx8gl::gl_DeleteProgram
#define glGetProgramiv dx8gl::gl_GetProgramiv
#define glGetProgramInfoLog dx8gl::gl_GetProgramInfoLog
#define glGetUniformLocation dx8gl::gl_GetUniformLocation
#define glUniform1i dx8gl::gl_Uniform1i
#define glUniform1f dx8gl::gl_Uniform1f
#define glUniform4f dx8gl::gl_Uniform4f
#define glUniformMatrix4fv dx8gl::gl_UniformMatrix4fv
#define glVertexAttribPointer dx8gl::gl_VertexAttribPointer
#define glEnableVertexAttribArray dx8gl::gl_EnableVertexAttribArray
#define glDisableVertexAttribArray dx8gl::gl_DisableVertexAttribArray
#define glGetAttribLocation dx8gl::gl_GetAttribLocation
#endif

#endif // DX8GL_OSMESA_GL_LOADER_H