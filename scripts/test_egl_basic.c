#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <string.h>

int main() {
    printf("Testing basic EGL functionality...\n");
    
    // Initialize EGL
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        printf("Failed to get EGL display\n");
        return 1;
    }
    
    EGLint major, minor;
    if (!eglInitialize(display, &major, &minor)) {
        printf("Failed to initialize EGL\n");
        return 1;
    }
    
    printf("EGL version: %d.%d\n", major, minor);
    printf("EGL vendor: %s\n", eglQueryString(display, EGL_VENDOR));
    printf("EGL version string: %s\n", eglQueryString(display, EGL_VERSION));
    printf("EGL client APIs: %s\n", eglQueryString(display, EGL_CLIENT_APIS));
    printf("EGL extensions: %s\n", eglQueryString(display, EGL_EXTENSIONS));
    
    // Check for surfaceless context support
    const char* extensions = eglQueryString(display, EGL_EXTENSIONS);
    if (strstr(extensions, "EGL_KHR_surfaceless_context")) {
        printf("\nEGL_KHR_surfaceless_context is supported!\n");
    } else {
        printf("\nEGL_KHR_surfaceless_context is NOT supported\n");
    }
    
    // Try to choose a simple config
    const EGLint config_attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_NONE
    };
    
    EGLConfig config;
    EGLint num_configs;
    if (!eglChooseConfig(display, config_attribs, &config, 1, &num_configs)) {
        printf("Failed to choose config\n");
        eglTerminate(display);
        return 1;
    }
    
    printf("\nFound %d matching configs\n", num_configs);
    
    // Try to create a context
    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        printf("Failed to bind OpenGL ES API\n");
        eglTerminate(display);
        return 1;
    }
    
    const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribs);
    if (context == EGL_NO_CONTEXT) {
        printf("Failed to create context\n");
        eglTerminate(display);
        return 1;
    }
    
    printf("Successfully created EGL context\n");
    
    // Try to make context current without a surface (surfaceless)
    if (eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context)) {
        printf("Successfully made context current with EGL_NO_SURFACE (surfaceless)\n");
    } else {
        printf("Failed to make context current with EGL_NO_SURFACE\n");
        printf("EGL error: 0x%x\n", eglGetError());
    }
    
    // Cleanup
    eglDestroyContext(display, context);
    eglTerminate(display);
    
    return 0;
}