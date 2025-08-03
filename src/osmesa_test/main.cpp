#include <GL/osmesa.h>
#include <GL/glu.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define WIDTH 400
#define HEIGHT 400

// Write a simple PPM file
void write_ppm(const char* filename, GLfloat* buffer, int width, int height) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        printf("Could not open %s for writing\n", filename);
        return;
    }
    
    fprintf(f, "P6\n%d %d\n255\n", width, height);
    
    for (int y = height - 1; y >= 0; y--) {  // Flip Y
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 4;  // RGBA
            unsigned char r = (unsigned char)(buffer[idx + 0] * 255.0f);
            unsigned char g = (unsigned char)(buffer[idx + 1] * 255.0f);
            unsigned char b = (unsigned char)(buffer[idx + 2] * 255.0f);
            fwrite(&r, 1, 1, f);
            fwrite(&g, 1, 1, f);
            fwrite(&b, 1, 1, f);
        }
    }
    
    fclose(f);
    printf("Wrote rendered image to %s\n", filename);
}

void render_scene() {
    GLfloat light_ambient[] = { 0.0, 0.0, 0.0, 1.0 };
    GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };
    GLfloat red_mat[]   = { 1.0, 0.2, 0.2, 1.0 };
    GLfloat green_mat[] = { 0.2, 1.0, 0.2, 0.5 };
    GLfloat blue_mat[]  = { 0.2, 0.2, 1.0, 1.0 };
    
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-2.5, 2.5, -2.5, 2.5, -10.0, 10.0);
    glMatrixMode(GL_MODELVIEW);

    glClearColor(0.4, 0.4, 0.4, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    glRotatef(20.0, 1.0, 0.0, 0.0);

    // Red square
    glPushMatrix();
    glTranslatef(0.0, -0.5, 0.0); 
    glRotatef(90, 1, 0.5, 0);
    glScalef(3, 3, 3);
    glDisable(GL_LIGHTING);
    glColor4f(1, 0, 0, 0.5);
    glBegin(GL_QUADS);
    glVertex2f(-1, -1);
    glVertex2f( 1, -1);
    glVertex2f( 1,  1);
    glVertex2f(-1,  1);
    glEnd();
    glEnable(GL_LIGHTING);
    glPopMatrix();

    // Green cylinder
    glPushMatrix();
    glTranslatef(-0.75, -0.5, 0.0); 
    glRotatef(270.0, 1.0, 0.0, 0.0);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, green_mat);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLUquadricObj *qobj = gluNewQuadric();
    gluCylinder(qobj, 1.0, 0.0, 2.0, 16, 1);
    gluDeleteQuadric(qobj);
    glDisable(GL_BLEND);
    glPopMatrix();

    // Blue sphere
    glPushMatrix();
    glTranslatef(0.75, 1.0, 1.0); 
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, blue_mat);
    qobj = gluNewQuadric();
    gluSphere(qobj, 1.0, 20, 20);
    gluDeleteQuadric(qobj);
    glPopMatrix();

    glPopMatrix();
    glFinish();
}

int main() {
    printf("OSMesa Rendering Test\n");
    printf("Width: %d, Height: %d\n", WIDTH, HEIGHT);
    
    // Create OSMesa context
    OSMesaContext ctx = OSMesaCreateContextExt(GL_RGBA, 16, 0, 0, NULL);
    if (!ctx) {
        printf("OSMesaCreateContext failed!\n");
        return 1;
    }
    
    // Allocate buffer for rendering (float RGBA)
    GLfloat* buffer = (GLfloat*)malloc(WIDTH * HEIGHT * 4 * sizeof(GLfloat));
    if (!buffer) {
        printf("Failed to allocate buffer\n");
        OSMesaDestroyContext(ctx);
        return 1;
    }
    
    // Make context current
    if (!OSMesaMakeCurrent(ctx, buffer, GL_FLOAT, WIDTH, HEIGHT)) {
        printf("OSMesaMakeCurrent failed!\n");
        free(buffer);
        OSMesaDestroyContext(ctx);
        return 1;
    }
    
    // Print OpenGL info
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("Version: %s\n", glGetString(GL_VERSION));
    printf("Vendor: %s\n", glGetString(GL_VENDOR));
    
    // Render scene
    render_scene();
    
    // Write output image
    write_ppm("osmesa_output.ppm", buffer, WIDTH, HEIGHT);
    
    // Cleanup
    free(buffer);
    OSMesaDestroyContext(ctx);
    
    printf("OSMesa test completed successfully!\n");
    return 0;
}