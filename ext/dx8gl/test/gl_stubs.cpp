// OpenGL function stubs for testing purposes
#include "../src/gl3_headers.h"

namespace dx8gl {
    // Stub functions
    static void stub_UseProgram(GLuint program) { 
        // Stub implementation for testing
    }
    
    static void stub_UniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) { 
        // Stub implementation for testing
    }
    
    static void stub_Uniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) { 
        // Stub implementation for testing
    }
    
    static void stub_Uniform1f(GLint location, GLfloat v0) { 
        // Stub implementation for testing
    }
    
    static void stub_Uniform1i(GLint location, GLint v0) { 
        // Stub implementation for testing
    }
    
    // Function pointers (initialized to stubs)
    PFNGLUSEPROGRAMPROC gl_UseProgram = stub_UseProgram;
    PFNGLUNIFORMMATRIX4FVPROC gl_UniformMatrix4fv = stub_UniformMatrix4fv;
    PFNGLUNIFORM4FPROC gl_Uniform4f = stub_Uniform4f;
    PFNGLUNIFORM1FPROC gl_Uniform1f = stub_Uniform1f;
    PFNGLUNIFORM1IPROC gl_Uniform1i = stub_Uniform1i;
}