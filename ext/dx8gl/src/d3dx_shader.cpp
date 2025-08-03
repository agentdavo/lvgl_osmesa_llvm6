// D3DX Shader utilities implementation
#include "d3dx_compat.h"
#include "logger.h"
#include "dx8_shader_translator.h"
#include <string.h>
#include <stdlib.h>

// Define IID_ID3DXBuffer if not already defined
#ifndef IID_ID3DXBuffer
static const GUID IID_ID3DXBuffer = {0x8ba5fb08, 0x5195, 0x40e2, {0xac, 0x58, 0x0d, 0x98, 0x9c, 0x3a, 0x01, 0x02}};
#endif

// Simple ID3DXBuffer implementation
class D3DXBuffer : public ID3DXBuffer {
private:
    ULONG ref_count_;
    void* buffer_;
    DWORD size_;
    
public:
    D3DXBuffer(DWORD size) : ref_count_(1), size_(size) {
        buffer_ = malloc(size);
    }
    
    virtual ~D3DXBuffer() {
        if (buffer_) {
            free(buffer_);
        }
    }
    
    // IUnknown methods
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) {
        UNUSED(riid);
        // For simplicity, just return this object for any interface request
        *ppvObject = this;
        AddRef();
        return S_OK;
    }
    
    virtual ULONG STDMETHODCALLTYPE AddRef() {
        return ++ref_count_;
    }
    
    virtual ULONG STDMETHODCALLTYPE Release() {
        ULONG count = --ref_count_;
        if (count == 0) {
            delete this;
        }
        return count;
    }
    
    // ID3DXBuffer methods
    virtual void* STDMETHODCALLTYPE GetBufferPointer() {
        return buffer_;
    }
    
    virtual DWORD STDMETHODCALLTYPE GetBufferSize() {
        return size_;
    }
};

extern "C" {

HRESULT D3DXAssembleShader(const char* pSrcData, DWORD SrcDataLen, DWORD Flags,
                          ID3DXBuffer** ppConstants, ID3DXBuffer** ppCompiledShader,
                          ID3DXBuffer** ppCompilationErrors) {
    UNUSED(Flags);
    
    DX8GL_INFO("D3DXAssembleShader called");
    
    // Create shader translator
    dx8gl::DX8ShaderTranslator translator;
    
    // Parse the shader
    std::string source;
    if (SrcDataLen == 0 || SrcDataLen == (DWORD)-1) {
        source = std::string(pSrcData);
    } else {
        source = std::string(pSrcData, SrcDataLen);
    }
    
    std::string error_msg;
    if (!translator.parse_shader(source, error_msg)) {
        DX8GL_ERROR("Failed to parse shader: %s", error_msg.c_str());
        
        if (ppCompilationErrors) {
            size_t error_len = error_msg.length() + 1;
            D3DXBuffer* error_buffer = new D3DXBuffer(error_len);
            memcpy(error_buffer->GetBufferPointer(), error_msg.c_str(), error_len);
            *ppCompilationErrors = error_buffer;
        }
        
        return D3DERR_INVALIDCALL;
    }
    
    // Generate GLSL for debugging
    std::string glsl = translator.generate_glsl();
    DX8GL_INFO("Generated GLSL:\n%s", glsl.c_str());
    
    // Get bytecode
    std::vector<DWORD> bytecode = translator.get_bytecode();
    
    // Create compiled shader buffer
    if (ppCompiledShader) {
        size_t bytecode_size = bytecode.size() * sizeof(DWORD);
        D3DXBuffer* shader_buffer = new D3DXBuffer(bytecode_size);
        memcpy(shader_buffer->GetBufferPointer(), bytecode.data(), bytecode_size);
        *ppCompiledShader = shader_buffer;
    }
    
    // Create constants buffer (if requested)
    if (ppConstants) {
        // For now, just return empty buffer
        D3DXBuffer* const_buffer = new D3DXBuffer(4);
        *ppConstants = const_buffer;
    }
    
    if (ppCompilationErrors) {
        *ppCompilationErrors = nullptr;
    }
    
    return S_OK;
}

} // extern "C"