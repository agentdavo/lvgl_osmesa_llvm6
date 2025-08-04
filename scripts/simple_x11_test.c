#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    Display *display;
    Window window;
    XEvent event;
    int screen;
    
    // Open connection to X server
    display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }
    
    screen = DefaultScreen(display);
    
    // Create simple window
    window = XCreateSimpleWindow(display, RootWindow(display, screen), 
                                10, 10, 400, 400, 1,
                                BlackPixel(display, screen), 
                                WhitePixel(display, screen));
    
    // Select input events
    XSelectInput(display, window, ExposureMask | KeyPressMask);
    
    // Map window
    XMapWindow(display, window);
    
    printf("X11 window created successfully\n");
    printf("Window should be visible now\n");
    printf("Press any key in the window to exit...\n");
    
    // Event loop
    while (1) {
        XNextEvent(display, &event);
        if (event.type == Expose) {
            printf("Window exposed\n");
        }
        if (event.type == KeyPress) {
            printf("Key pressed, exiting\n");
            break;
        }
    }
    
    // Close display
    XCloseDisplay(display);
    
    return 0;
}