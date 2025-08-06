#!/usr/bin/env python3
"""
HTTP server for dx8gl WebGPU demo
Serves files with proper MIME types and CORS headers for WebGPU
"""

import http.server
import socketserver
import os
import sys
from pathlib import Path

PORT = 8090
DIRECTORY = Path(__file__).parent

class WebGPURequestHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(DIRECTORY), **kwargs)
    
    def end_headers(self):
        # Add CORS headers for WebGPU
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        super().end_headers()
    
    def guess_type(self, path):
        mimetype = super().guess_type(path)
        # Ensure WASM files are served with correct MIME type
        if path.endswith('.wasm'):
            return 'application/wasm'
        elif path.endswith('.js'):
            return 'application/javascript'
        elif path.endswith('.mjs'):
            return 'application/javascript'
        return mimetype
    
    def log_message(self, format, *args):
        # Custom logging format
        sys.stderr.write(f"[{self.log_date_time_string()}] {format % args}\n")

def main():
    os.chdir(DIRECTORY)
    
    print(f"Starting dx8gl WebGPU demo server...")
    print(f"Directory: {DIRECTORY}")
    print(f"URL: http://localhost:{PORT}/")
    print(f"Files available:")
    print(f"  - http://localhost:{PORT}/dx8gl_demo.html (dx8gl DirectX 8 demo)")
    print(f"  - http://localhost:{PORT}/index.html (Pure WebGPU demo)")
    print(f"  - http://localhost:{PORT}/animometer.html (Reference example)")
    print("\nPress Ctrl+C to stop the server")
    
    with socketserver.TCPServer(("", PORT), WebGPURequestHandler) as httpd:
        httpd.allow_reuse_address = True
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nShutting down server...")
            httpd.shutdown()

if __name__ == "__main__":
    main()