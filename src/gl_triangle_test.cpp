#include <iostream>
#include <GL/gl.h>
#include <GL/osmesa.h>
#include <cstdio>
#include <cstdlib>

// Simple vertex shader source
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec3 aColor;
out vec3 vertexColor;
uniform mat4 mvp;
void main() {
    gl_Position = mvp * vec4(aPos, 1.0);
    vertexColor = aColor;
}
)";

// Simple fragment shader source  
const char* fragmentShaderSource = R"(
#version 330 core
in vec3 vertexColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vertexColor, 1.0);
}
)";

int main() {
    // Create OSMesa context
    const int attribs[] = {
        OSMESA_FORMAT, OSMESA_RGBA,
        OSMESA_DEPTH_BITS, 24,
        OSMESA_STENCIL_BITS, 8,
        OSMESA_PROFILE, OSMESA_CORE_PROFILE,
        OSMESA_CONTEXT_MAJOR_VERSION, 3,
        OSMESA_CONTEXT_MINOR_VERSION, 3,
        0
    };
    
    OSMesaContext ctx = OSMesaCreateContextAttribs(attribs, NULL);
    if (!ctx) {
        std::cerr << "Failed to create OSMesa context" << std::endl;
        return 1;
    }
    
    // Create framebuffer
    const int width = 400, height = 400;
    void* buffer = malloc(width * height * 4);
    
    if (!OSMesaMakeCurrent(ctx, buffer, GL_UNSIGNED_BYTE, width, height)) {
        std::cerr << "Failed to make OSMesa context current" << std::endl;
        return 1;
    }
    
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    
    // Load GL function pointers
    auto glCreateShader = (PFNGLCREATESHADERPROC)OSMesaGetProcAddress("glCreateShader");
    auto glShaderSource = (PFNGLSHADERSOURCEPROC)OSMesaGetProcAddress("glShaderSource");
    auto glCompileShader = (PFNGLCOMPILESHADERPROC)OSMesaGetProcAddress("glCompileShader");
    auto glGetShaderiv = (PFNGLGETSHADERIVPROC)OSMesaGetProcAddress("glGetShaderiv");
    auto glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)OSMesaGetProcAddress("glGetShaderInfoLog");
    auto glCreateProgram = (PFNGLCREATEPROGRAMPROC)OSMesaGetProcAddress("glCreateProgram");
    auto glAttachShader = (PFNGLATTACHSHADERPROC)OSMesaGetProcAddress("glAttachShader");
    auto glLinkProgram = (PFNGLLINKPROGRAMPROC)OSMesaGetProcAddress("glLinkProgram");
    auto glGetProgramiv = (PFNGLGETPROGRAMIVPROC)OSMesaGetProcAddress("glGetProgramiv");
    auto glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)OSMesaGetProcAddress("glGetProgramInfoLog");
    auto glUseProgram = (PFNGLUSEPROGRAMPROC)OSMesaGetProcAddress("glUseProgram");
    auto glDeleteShader = (PFNGLDELETESHADERPROC)OSMesaGetProcAddress("glDeleteShader");
    auto glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)OSMesaGetProcAddress("glGenVertexArrays");
    auto glGenBuffers = (PFNGLGENBUFFERSPROC)OSMesaGetProcAddress("glGenBuffers");
    auto glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)OSMesaGetProcAddress("glBindVertexArray");
    auto glBindBuffer = (PFNGLBINDBUFFERPROC)OSMesaGetProcAddress("glBindBuffer");
    auto glBufferData = (PFNGLBUFFERDATAPROC)OSMesaGetProcAddress("glBufferData");
    auto glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)OSMesaGetProcAddress("glVertexAttribPointer");
    auto glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)OSMesaGetProcAddress("glEnableVertexAttribArray");
    auto glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)OSMesaGetProcAddress("glGetUniformLocation");
    auto glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)OSMesaGetProcAddress("glUniformMatrix4fv");
    
    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
        return 1;
    }
    
    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
        return 1;
    }
    
    // Link shaders
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader linking failed: " << infoLog << std::endl;
        return 1;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // Set up vertex data - triangle with position and color
    float vertices[] = {
        // positions       // colors
         0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  // top - red
        -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  // bottom left - green
         0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f   // bottom right - blue
    };
    
    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Position attribute (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Color attribute (location = 2, matching dx8gl convention)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // Set up MVP matrix (identity for now)
    float mvp[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    
    // Clear and render
    glViewport(0, 0, width, height);
    glClearColor(0.25f, 0.25f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Draw triangle
    glUseProgram(shaderProgram);
    GLint mvpLoc = glGetUniformLocation(shaderProgram, "mvp");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvp);
    
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    glFinish();
    
    // Save as PPM
    FILE* fp = fopen("gl_triangle_test.ppm", "wb");
    if (fp) {
        fprintf(fp, "P6\n%d %d\n255\n", width, height);
        uint8_t* pixels = (uint8_t*)buffer;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = (y * width + x) * 4;
                uint8_t r = pixels[idx];
                uint8_t g = pixels[idx + 1];
                uint8_t b = pixels[idx + 2];
                fwrite(&r, 1, 1, fp);
                fwrite(&g, 1, 1, fp);
                fwrite(&b, 1, 1, fp);
            }
        }
        fclose(fp);
        std::cout << "Saved gl_triangle_test.ppm" << std::endl;
    }
    
    // Check center pixel
    uint8_t* pixels = (uint8_t*)buffer;
    int cx = 200, cy = 200;
    int idx = (cy * width + cx) * 4;
    std::cout << "Center pixel RGB: " << (int)pixels[idx] << ", " 
              << (int)pixels[idx+1] << ", " << (int)pixels[idx+2] << std::endl;
    
    // Cleanup
    free(buffer);
    OSMesaDestroyContext(ctx);
    
    return 0;
}