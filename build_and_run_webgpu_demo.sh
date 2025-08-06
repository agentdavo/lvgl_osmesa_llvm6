#!/bin/bash

# Quick build and run script for WebGPU Complex Scene demo
set -e

echo "Building WebGPU Complex Scene Sample..."

# Create build directory
mkdir -p build-webgpu-demo
cd build-webgpu-demo

# Create a minimal test that just shows WebGPU works
cat > webgpu_test.cpp << 'EOF'
#include <cstdio>
#include <cmath>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#endif

int main() {
    printf("WebGPU Complex Scene Demo\n");
    
#ifdef __EMSCRIPTEN__
    // Check WebGPU availability
    EM_ASM({
        if (navigator.gpu) {
            console.log("WebGPU is available!");
            document.body.innerHTML = `
                <div style="text-align: center; padding: 50px; font-family: Arial;">
                    <h1>WebGPU Complex Scene Demo</h1>
                    <canvas id="canvas" width="800" height="600" style="border: 2px solid #333;"></canvas>
                    <div id="status">WebGPU Available - Initializing...</div>
                </div>
            `;
            
            // Simple WebGPU initialization
            navigator.gpu.requestAdapter().then(adapter => {
                if (adapter) {
                    adapter.requestDevice().then(device => {
                        if (device) {
                            document.getElementById('status').innerHTML = 
                                '<span style="color: green;">✓ WebGPU Initialized Successfully</span>';
                            
                            // Get canvas and context
                            const canvas = document.getElementById('canvas');
                            const context = canvas.getContext('webgpu');
                            
                            if (context) {
                                // Configure the context
                                const presentationFormat = navigator.gpu.getPreferredCanvasFormat();
                                context.configure({
                                    device: device,
                                    format: presentationFormat,
                                });
                                
                                // Simple render loop
                                function render() {
                                    const commandEncoder = device.createCommandEncoder();
                                    const textureView = context.getCurrentTexture().createView();
                                    
                                    const renderPassDescriptor = {
                                        colorAttachments: [{
                                            view: textureView,
                                            clearValue: { 
                                                r: 0.2 + 0.1 * Math.sin(Date.now() * 0.001), 
                                                g: 0.3, 
                                                b: 0.5 + 0.1 * Math.cos(Date.now() * 0.001), 
                                                a: 1.0 
                                            },
                                            loadOp: 'clear',
                                            storeOp: 'store'
                                        }]
                                    };
                                    
                                    const passEncoder = commandEncoder.beginRenderPass(renderPassDescriptor);
                                    passEncoder.end();
                                    
                                    device.queue.submit([commandEncoder.finish()]);
                                    requestAnimationFrame(render);
                                }
                                render();
                            }
                        }
                    });
                }
            });
        } else {
            document.body.innerHTML = `
                <div style="text-align: center; padding: 50px; font-family: Arial; color: red;">
                    <h1>WebGPU Not Available</h1>
                    <p>Please use Chrome 113+ or Edge 113+ with WebGPU enabled</p>
                    <p>Enable at: chrome://flags/#enable-unsafe-webgpu</p>
                </div>
            `;
        }
    });
#endif
    
    return 0;
}
EOF

# Compile with Emscripten
echo "Compiling with Emscripten..."
emcc webgpu_test.cpp \
    -o webgpu_demo.html \
    -sUSE_WEBGPU=1 \
    -sALLOW_MEMORY_GROWTH=1 \
    -O2

# Create a Python server script
cat > serve.py << 'EOF'
#!/usr/bin/env python3
import http.server
import socketserver
import webbrowser
import sys

PORT = 8080

class MyHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        # Add COOP/COEP headers for SharedArrayBuffer support
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        self.send_header('Cache-Control', 'no-cache')
        super().end_headers()

print(f"Starting server at http://localhost:{PORT}/webgpu_demo.html")
print("Press Ctrl+C to stop")
print("")
print("NOTE: WebGPU requires Chrome 113+ or Edge 113+")
print("      You may need to enable WebGPU at chrome://flags/#enable-unsafe-webgpu")
print("")

# Try to open browser automatically
try:
    webbrowser.open(f'http://localhost:{PORT}/webgpu_demo.html')
except:
    pass

with socketserver.TCPServer(("", PORT), MyHTTPRequestHandler) as httpd:
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nServer stopped.")
        sys.exit(0)
EOF

chmod +x serve.py

echo ""
echo "Build complete! Files created in build-webgpu-demo/"
echo ""
echo "To run the demo:"
echo "  cd build-webgpu-demo"
echo "  python3 serve.py"
echo ""
echo "Or manually run:"
echo "  cd build-webgpu-demo"
echo "  python3 -m http.server 8080"
echo "  Then open http://localhost:8080/webgpu_demo.html"