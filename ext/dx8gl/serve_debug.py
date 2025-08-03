#!/usr/bin/env python3
"""
Simple HTTP server for testing Emscripten-compiled WebGL applications (Debug builds).
Serves with proper MIME types and CORS headers for WebAssembly.
"""

import http.server
import socketserver
import os
import sys
from pathlib import Path

class MyHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        # Add headers needed for SharedArrayBuffer and WebAssembly
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', '*')
        super().end_headers()
    
    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()
    
    def guess_type(self, path):
        mimetype = super().guess_type(path)
        # Ensure proper MIME types for WebAssembly files
        if path.endswith('.wasm'):
            return 'application/wasm'
        if path.endswith('.js'):
            return 'application/javascript'
        return mimetype

def main():
    PORT = 8001  # Different port for debug server
    DIRECTORY = "build-emscripten-debug/Debug"
    
    # Change to build directory if it exists
    if os.path.exists(DIRECTORY):
        os.chdir(DIRECTORY)
        print(f"Serving DEBUG build from: {os.getcwd()}")
    else:
        print(f"Warning: {DIRECTORY} not found. Build debug version first!")
        print(f"Serving from current directory: {os.getcwd()}")
    
    Handler = MyHTTPRequestHandler
    
    with socketserver.TCPServer(("", PORT), Handler) as httpd:
        print(f"\n🌐 DEBUG Server running at http://localhost:{PORT}")
        print(f"📁 Available samples at http://localhost:{PORT}/")
        print("\nPress Ctrl+C to stop the server\n")
        
        # List available HTML files
        current_dir = Path(".")
        html_files = list(current_dir.glob("*.html"))
        if html_files:
            print("Available samples:")
            for f in sorted(html_files):
                print(f"  - http://localhost:{PORT}/{f.name}")
            print()
        
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n\nServer stopped.")
            sys.exit(0)

if __name__ == "__main__":
    main()