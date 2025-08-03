#include "lvgl_platform.h"
#include <lvgl.h>
#include <GL/osmesa.h>
#include <GL/glu.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define CANVAS_WIDTH 400
#define CANVAS_HEIGHT 400

static lv_obj_t *canvas;
static uint8_t *canvas_buf;
static GLfloat *osmesa_buffer;
static OSMesaContext osmesa_ctx;

static float rotation_angle = 0.0f;

static void render_osmesa_scene(void)
{
    // Make OSMesa context current
    if (!OSMesaMakeCurrent(osmesa_ctx, osmesa_buffer, GL_FLOAT, CANVAS_WIDTH, CANVAS_HEIGHT)) {
        printf("OSMesaMakeCurrent failed!\n");
        return;
    }
    
    // Set up viewport and projection
    glViewport(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (GLfloat)CANVAS_WIDTH / (GLfloat)CANVAS_HEIGHT, 0.1, 100.0);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -5.0f);
    
    // Clear with dark blue background
    glClearColor(0.1f, 0.1f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // Rotate the triangle
    glPushMatrix();
    glRotatef(rotation_angle, 0.0f, 1.0f, 0.0f);
    
    // Draw a colorful triangle
    glBegin(GL_TRIANGLES);
        // Red vertex
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(-1.0f, -1.0f, 0.0f);
        
        // Green vertex
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(1.0f, -1.0f, 0.0f);
        
        // Blue vertex
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
    
    glPopMatrix();
    
    glFinish();
}

static void update_canvas_from_osmesa()
{
    // Convert float RGBA to RGBA32 for LVGL canvas
    for (int y = 0; y < CANVAS_HEIGHT; y++) {
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            int src_idx = (y * CANVAS_WIDTH + x) * 4;
            int dst_idx = ((CANVAS_HEIGHT - 1 - y) * CANVAS_WIDTH + x) * 4; // Flip Y and RGBA32 is 4 bytes
            
            // Get float values and convert to 8-bit
            uint8_t r = (uint8_t)(osmesa_buffer[src_idx + 0] * 255.0f);
            uint8_t g = (uint8_t)(osmesa_buffer[src_idx + 1] * 255.0f);
            uint8_t b = (uint8_t)(osmesa_buffer[src_idx + 2] * 255.0f);
            uint8_t a = (uint8_t)(osmesa_buffer[src_idx + 3] * 255.0f);
            
            // Store as RGBA32 (native format)
            canvas_buf[dst_idx + 0] = r;
            canvas_buf[dst_idx + 1] = g;
            canvas_buf[dst_idx + 2] = b;
            canvas_buf[dst_idx + 3] = a;
        }
    }
    
    lv_obj_invalidate(canvas);
}

static void render_timer_cb(lv_timer_t *timer)
{
    // Update rotation angle
    rotation_angle += 2.0f;
    if (rotation_angle > 360.0f) {
        rotation_angle -= 360.0f;
    }
    
    // Render with OSMesa
    render_osmesa_scene();
    
    // Update LVGL canvas with rendered image
    update_canvas_from_osmesa();
}

int main()
{
    // Create LVGL window
    lv_display_t *disp = LvglPlatform::create_window(800, 600, "sdl");
    if (!disp) {
        printf("Failed to create LVGL window\n");
        return 1;
    }
    
    // Create OSMesa context
    osmesa_ctx = OSMesaCreateContextExt(GL_RGBA, 16, 0, 0, NULL);
    if (!osmesa_ctx) {
        printf("OSMesaCreateContext failed!\n");
        return 1;
    }
    
    // Allocate OSMesa buffer (float RGBA)
    osmesa_buffer = (GLfloat *)malloc(CANVAS_WIDTH * CANVAS_HEIGHT * 4 * sizeof(GLfloat));
    if (!osmesa_buffer) {
        printf("Failed to allocate OSMesa buffer\n");
        return 1;
    }
    
    // Create LVGL canvas
    canvas_buf = (uint8_t *)malloc(CANVAS_WIDTH * CANVAS_HEIGHT * 4); // RGBA32
    canvas = lv_canvas_create(lv_screen_active());
    lv_canvas_set_buffer(canvas, canvas_buf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_COLOR_FORMAT_NATIVE);
    lv_obj_center(canvas);
    
    // Add title
    lv_obj_t *title = lv_label_create(lv_screen_active());
    lv_label_set_text(title, "OSMesa rendering to LVGL Canvas");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Add info label
    lv_obj_t *info = lv_label_create(lv_screen_active());
    
    // Make OSMesa context current to get renderer info
    OSMesaMakeCurrent(osmesa_ctx, osmesa_buffer, GL_FLOAT, CANVAS_WIDTH, CANVAS_HEIGHT);
    const char *renderer = (const char *)glGetString(GL_RENDERER);
    const char *version = (const char *)glGetString(GL_VERSION);
    
    char info_text[256];
    snprintf(info_text, sizeof(info_text), "Renderer: %s\nVersion: %s", renderer, version);
    lv_label_set_text(info, info_text);
    lv_obj_align(info, LV_ALIGN_BOTTOM_MID, 0, -20);
    
    // Initial render
    render_osmesa_scene();
    update_canvas_from_osmesa();
    
    // Create timer for animation (30 FPS)
    lv_timer_create(render_timer_cb, 33, NULL);
    
    // Main loop
    while (true) {
        LvglPlatform::poll_events();
    }
    
    // Cleanup
    free(osmesa_buffer);
    free(canvas_buf);
    OSMesaDestroyContext(osmesa_ctx);
    
    return 0;
}