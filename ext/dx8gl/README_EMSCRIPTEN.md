# dx8gl Emscripten Support

This directory contains support for building and testing dx8gl samples in web browsers using Emscripten and WebGL.

## Prerequisites

1. Install Emscripten SDK:
```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh  # Add this to your .bashrc/.zshrc
```

2. Ensure you have Python 3 installed (for the test server)

## Building

Run the build script:
```bash
./build_emscripten.sh
```

This will:
- Create a `build-emscripten/` directory
- Configure CMake with Emscripten toolchain
- Build all dx8gl samples as HTML/JS/WASM files
- Generate WebGL-based test applications

## Testing

1. Start the test server:
```bash
./serve_emscripten.py
```

2. Open your browser to http://localhost:8000/samples/

3. Click on any of the sample HTML files to run them

## Available Samples

- **dx8gl_clear_test.html** - Basic clear color test
- **dx8gl_triangle.html** - Simple triangle rendering
- **dx8gl_textured_cube.html** - Textured 3D cube
- **dx8gl_lighting_test.html** - Lighting and shading
- **dx8gl_vertex_shader_test.html** - Vertex shader tests
- **dx8gl_pixel_shader_test.html** - Pixel shader tests
- **dx8gl_complex_vertex_shader_test.html** - Complex VS 1.1 shaders
- **dx8gl_shader_modifiers_test.html** - Shader modifiers (_sat, _x2, etc.)
- **dx8gl_bytecode_assembler_test.html** - Shader bytecode assembly

## WebGL Features

The Emscripten build automatically:
- Uses WebGL 2.0 when available (falls back to WebGL 1.0)
- Enables full OpenGL ES 2.0 emulation
- Provides shader compilation from DirectX 8 to GLSL ES
- Supports all DirectX 8 features via translation

## Debugging

Open the browser's Developer Console (F12) to see:
- Shader compilation logs
- WebGL errors and warnings
- dx8gl debug output
- Performance metrics

## Browser Compatibility

Tested and working on:
- Chrome 90+
- Firefox 88+
- Safari 14+
- Edge 90+

Mobile browsers with WebGL 2.0 support should also work.

## Troubleshooting

1. **"WebGL not available"**: Ensure your browser supports WebGL 2.0
2. **Shader compilation errors**: Check console for GLSL errors
3. **Black screen**: May indicate missing WebGL extensions
4. **Performance issues**: Try Chrome with hardware acceleration enabled

## Development

To add a new sample:
1. Create the sample .cpp file in `samples/`
2. Add it to `samples/CMakeLists.txt` using `add_dx8gl_sample()`
3. Rebuild with `./build_emscripten.sh`
4. Test in browser