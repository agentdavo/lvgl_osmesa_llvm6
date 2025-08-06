#include "d3d8_surface.h"
#include "d3d8_device.h"
#include "d3d8_texture.h"
#include "osmesa_gl_loader.h"
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <cstring>
#include <algorithm>

#ifdef __EMSCRIPTEN__
// WebGL extension for packed depth stencil
#ifndef GL_DEPTH_STENCIL
#define GL_DEPTH_STENCIL 0x84F9
#endif
#ifndef GL_UNSIGNED_INT_24_8
#define GL_UNSIGNED_INT_24_8 0x84FA
#endif
#endif

namespace dx8gl {

// Constructor for standalone surface
Direct3DSurface8::Direct3DSurface8(Direct3DDevice8* device, UINT width, UINT height,
                                   D3DFORMAT format, DWORD usage, D3DPOOL pool,
                                   D3DMULTISAMPLE_TYPE multisample)
    : ref_count_(1)
    , device_(device)
    , parent_texture_(nullptr)
    , width_(width)
    , height_(height)
    , format_(format)
    , usage_(usage)
    , level_(0)
    , pool_(pool)
    , multisample_type_(multisample)
    , texture_(0)
    , renderbuffer_(0)
    , framebuffer_(0)
    , locked_(false)
    , lock_buffer_(nullptr)
    , lock_rect_()
    , lock_flags_(0)
    , pitch_(0) {
    
    // Calculate pitch
    pitch_ = width_ * get_format_size(format_);
    
    // Add reference to device
    device_->AddRef();
    
    DX8GL_DEBUG("Direct3DSurface8 created: %ux%u format=%d usage=0x%08x",
                width, height, format, usage);
}

// Constructor for texture surface
Direct3DSurface8::Direct3DSurface8(Direct3DTexture8* texture, UINT level, UINT width, 
                                   UINT height, D3DFORMAT format, DWORD usage, D3DPOOL pool)
    : ref_count_(1)
    , device_(nullptr)
    , parent_texture_(texture)
    , width_(width)
    , height_(height)
    , format_(format)
    , usage_(usage)
    , level_(level)
    , pool_(pool)
    , multisample_type_(D3DMULTISAMPLE_NONE)  // Textures don't have multisampling
    , texture_(0)
    , renderbuffer_(0)
    , framebuffer_(0)
    , locked_(false)
    , lock_buffer_(nullptr)
    , lock_rect_()
    , lock_flags_(0)
    , pitch_(0) {
    
    // Calculate pitch
    pitch_ = width_ * get_format_size(format_);
    
    // Add reference to parent texture
    parent_texture_->AddRef();
    
    DX8GL_DEBUG("Direct3DSurface8 created for texture: level=%u %ux%u format=%d",
                level, width, height, format);
}

Direct3DSurface8::~Direct3DSurface8() {
    DX8GL_DEBUG("Direct3DSurface8 destructor");
    
    // Clean up OpenGL resources
    if (framebuffer_) {
        glDeleteFramebuffers(1, &framebuffer_);
    }
    if (renderbuffer_) {
        glDeleteRenderbuffers(1, &renderbuffer_);
    }
    if (texture_ && !parent_texture_) {  // Only delete if we own it
        glDeleteTextures(1, &texture_);
    }
    
    // Clean up lock buffer
    if (lock_buffer_) {
        free(lock_buffer_);
    }
    
    // Release references
    if (device_) {
        device_->Release();
    }
    if (parent_texture_) {
        parent_texture_->Release();
    }
}

bool Direct3DSurface8::initialize() {
    // For texture surfaces, the texture is managed by parent
    if (parent_texture_) {
        return true;
    }
    
    // OSMesa context is always current
    
    // Convert format
    GLenum internal_format, format, type;
    if (!get_gl_format(format_, internal_format, format, type)) {
        DX8GL_ERROR("Unsupported surface format: %d", format_);
        return false;
    }
    
    // Create appropriate OpenGL object based on usage
    if (is_depth_stencil()) {
        // Check if renderbuffers are supported using modern method
        bool has_renderbuffers = has_extension("GL_ARB_framebuffer_object");
        
        if (has_renderbuffers) {
            // Create renderbuffer for depth/stencil
            glGenRenderbuffers(1, &renderbuffer_);
            if (!renderbuffer_) {
                DX8GL_WARNING("Renderbuffers not available, using default depth buffer");
                renderbuffer_ = 0;
                return true; // Continue without explicit depth buffer - use default
            }
        } else {
            // OSMesa or legacy OpenGL - use default depth buffer
            renderbuffer_ = 0;
            DX8GL_DEBUG("Using default depth buffer for OSMesa rendering");
            return true;
        }
        
        if (renderbuffer_) {
            glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer_);
#ifdef __EMSCRIPTEN__
        // WebGL requires specific formats for renderbuffers
        GLenum rb_format = internal_format;
        if (format_ == D3DFMT_D16) {
            rb_format = GL_DEPTH_COMPONENT16;
        } else if (format_ == D3DFMT_D24S8 || format_ == D3DFMT_D24X8) {
            rb_format = GL_DEPTH_STENCIL;  // WebGL OES_packed_depth_stencil
        } else if (format_ == D3DFMT_D32) {
            rb_format = GL_DEPTH_COMPONENT16;  // WebGL doesn't support 32-bit depth
        }
        glRenderbufferStorage(GL_RENDERBUFFER, rb_format, width_, height_);
#else
        glRenderbufferStorage(GL_RENDERBUFFER, internal_format, width_, height_);
#endif
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
            
            DX8GL_DEBUG("Created depth/stencil renderbuffer %u", renderbuffer_);
        }
    } else {
        // Create texture for color surfaces
        glGenTextures(1, &texture_);
        GLenum error = glGetError();
        if (error != GL_NO_ERROR || !texture_) {
            DX8GL_ERROR("Failed to generate texture: GL error 0x%04x", error);
            return false;
        }
        
        glBindTexture(GL_TEXTURE_2D, texture_);
        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width_, height_, 0,
                     format, type, nullptr);
        
        // Set default texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        glBindTexture(GL_TEXTURE_2D, 0);
        
        DX8GL_DEBUG("Created color texture %u", texture_);
    }
    
    // Create framebuffer if this is a render target
    if (is_render_target() && texture_) {
        // Check if framebuffer objects are supported using modern method
        bool has_fbo = has_extension("GL_ARB_framebuffer_object");
        
        // Check if we're using OSMesa - if so, always use default framebuffer
        const char* renderer = (const char*)glGetString(GL_RENDERER);
        bool is_osmesa = renderer && (strstr(renderer, "llvmpipe") || strstr(renderer, "softpipe") || 
                                      strstr(renderer, "osmesa") || strstr(renderer, "OSMesa"));
        
        if (has_fbo && !is_osmesa) {
            glGenFramebuffers(1, &framebuffer_);
            if (!framebuffer_) {
                DX8GL_WARNING("Framebuffer objects not available, using default framebuffer");
                framebuffer_ = 0;
            } else {
                // Bind framebuffer and attach the texture
                glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                      GL_TEXTURE_2D, texture_, 0);
                
                // Check framebuffer completeness
                GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                if (status != GL_FRAMEBUFFER_COMPLETE) {
                    DX8GL_WARNING("Framebuffer incomplete: 0x%x, falling back to default framebuffer", status);
                    glDeleteFramebuffers(1, &framebuffer_);
                    framebuffer_ = 0;
                }
                
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                DX8GL_DEBUG("Created framebuffer %u with color attachment", framebuffer_);
            }
        } else {
            // OSMesa or legacy OpenGL - use default framebuffer (0)
            framebuffer_ = 0;
            DX8GL_DEBUG("Using default framebuffer for OSMesa rendering");
        }
    }
    
    return true;
}

// IUnknown methods
HRESULT Direct3DSurface8::QueryInterface(REFIID riid, void** ppvObj) {
    if (!ppvObj) {
        return E_POINTER;
    }
    
    if (IsEqualGUID(*riid, IID_IUnknown) || IsEqualGUID(*riid, IID_IDirect3DSurface8)) {
        *ppvObj = static_cast<IDirect3DSurface8*>(this);
        AddRef();
        return S_OK;
    }
    
    *ppvObj = nullptr;
    return E_NOINTERFACE;
}

ULONG Direct3DSurface8::AddRef() {
    LONG ref = ++ref_count_;
    DX8GL_TRACE("Direct3DSurface8::AddRef() -> %ld", ref);
    return ref;
}

ULONG Direct3DSurface8::Release() {
    LONG ref = --ref_count_;
    DX8GL_TRACE("Direct3DSurface8::Release() -> %ld", ref);
    
    if (ref == 0) {
        delete this;
    }
    
    return ref;
}

// IDirect3DSurface8 methods
HRESULT Direct3DSurface8::GetDevice(IDirect3DDevice8** ppDevice) {
    if (!ppDevice) {
        return D3DERR_INVALIDCALL;
    }
    
    if (parent_texture_) {
        // Get device from parent texture
        return parent_texture_->GetDevice(ppDevice);
    } else if (device_) {
        *ppDevice = device_;
        device_->AddRef();
        return D3D_OK;
    }
    
    return D3DERR_INVALIDCALL;
}

HRESULT Direct3DSurface8::SetPrivateData(REFGUID refguid, const void* pData,
                                         DWORD SizeOfData, DWORD Flags) {
    return private_data_manager_.SetPrivateData(refguid, pData, SizeOfData, Flags);
}

HRESULT Direct3DSurface8::GetPrivateData(REFGUID refguid, void* pData,
                                         DWORD* pSizeOfData) {
    return private_data_manager_.GetPrivateData(refguid, pData, pSizeOfData);
}

HRESULT Direct3DSurface8::FreePrivateData(REFGUID refguid) {
    return private_data_manager_.FreePrivateData(refguid);
}

HRESULT Direct3DSurface8::GetContainer(REFIID riid, void** ppContainer) {
    if (!ppContainer) {
        return D3DERR_INVALIDCALL;
    }
    
    if (parent_texture_) {
        return parent_texture_->QueryInterface(riid, ppContainer);
    }
    
    return E_NOINTERFACE;
}

HRESULT Direct3DSurface8::GetDesc(D3DSURFACE_DESC* pDesc) {
    if (!pDesc) {
        return D3DERR_INVALIDCALL;
    }
    
    pDesc->Format = format_;
    pDesc->Type = D3DRTYPE_SURFACE;
    pDesc->Usage = usage_;
    pDesc->Pool = pool_;
    pDesc->Size = pitch_ * height_;
    pDesc->MultiSampleType = multisample_type_;
    pDesc->Width = width_;
    pDesc->Height = height_;
    
    return D3D_OK;
}

HRESULT Direct3DSurface8::LockRect(D3DLOCKED_RECT* pLockedRect, const RECT* pRect,
                                   DWORD Flags) {
    DX8GL_INFO("LockRect called on surface %p, format=%d, size=%ux%u", this, format_, width_, height_);
    
    if (!pLockedRect) {
        return D3DERR_INVALIDCALL;
    }
    
    // Update device statistics for texture locks
    // Note: This includes all surface locks from textures, render targets, etc.
    if (device_) {
        device_->current_stats_.texture_locks++;
    }
    
    std::lock_guard<std::mutex> lock(lock_mutex_);
    
    if (locked_) {
        DX8GL_ERROR("Surface already locked");
        return D3DERR_INVALIDCALL;
    }
    
    // Determine lock rectangle
    if (pRect) {
        lock_rect_ = *pRect;
        // Validate rect
        if (lock_rect_.left < 0 || lock_rect_.top < 0 ||
            lock_rect_.right > static_cast<LONG>(width_) || 
            lock_rect_.bottom > static_cast<LONG>(height_) ||
            lock_rect_.left >= lock_rect_.right ||
            lock_rect_.top >= lock_rect_.bottom) {
            return D3DERR_INVALIDCALL;
        }
    } else {
        lock_rect_.left = 0;
        lock_rect_.top = 0;
        lock_rect_.right = width_;
        lock_rect_.bottom = height_;
    }
    
    UINT lock_width = lock_rect_.right - lock_rect_.left;
    UINT lock_height = lock_rect_.bottom - lock_rect_.top;
    UINT lock_size = lock_width * lock_height * get_format_size(format_);
    
    DX8GL_TRACE("Lock surface: rect=(%d,%d,%d,%d) flags=0x%08x",
                lock_rect_.left, lock_rect_.top, lock_rect_.right, lock_rect_.bottom, Flags);
    
    // Allocate lock buffer
    if (!lock_buffer_) {
        lock_buffer_ = malloc(pitch_ * height_);
        if (!lock_buffer_) {
            return E_OUTOFMEMORY;
        }
    }
    
    // Read existing data if not discarding
    if (!(Flags & D3DLOCK_DISCARD)) {
        if (texture_) {
            // Read from texture
            glBindTexture(GL_TEXTURE_2D, texture_);
            
            // Create temporary FBO for reading
            GLuint read_fbo = 0;
            glGenFramebuffers(1, &read_fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, read_fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                  GL_TEXTURE_2D, texture_, 0);
            
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
                // Read pixels
                GLenum internal_format, format, type;
                get_gl_format(format_, internal_format, format, type);
                glReadPixels(0, 0, width_, height_, format, type, lock_buffer_);
            }
            
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &read_fbo);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
    
    // Return locked pointer
    pLockedRect->Pitch = pitch_;
    pLockedRect->pBits = static_cast<BYTE*>(lock_buffer_) + 
                        (lock_rect_.top * pitch_) + 
                        (lock_rect_.left * get_format_size(format_));
    
    locked_ = true;
    lock_flags_ = Flags;
    
    return D3D_OK;
}

HRESULT Direct3DSurface8::UnlockRect() {
    std::lock_guard<std::mutex> lock(lock_mutex_);
    
    if (!locked_) {
        DX8GL_ERROR("Surface not locked");
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_INFO("UnlockRect called on surface %p, format=%d, lock_flags=0x%x", this, format_, lock_flags_);
    
    // Upload data if modified
    DX8GL_INFO("UnlockRect: lock_flags_=0x%x, D3DLOCK_READONLY=0x%x, texture_=%u", 
               lock_flags_, D3DLOCK_READONLY, texture_);
    
    if (!(lock_flags_ & D3DLOCK_READONLY)) {
        GLuint gl_texture = texture_;
        
        // If this is a texture surface, get the GL texture from parent
        if (!gl_texture && parent_texture_) {
            gl_texture = parent_texture_->get_gl_texture();
            DX8GL_INFO("Using parent texture GL object: %u", gl_texture);
        }
        
        if (gl_texture) {
            DX8GL_INFO("Uploading texture data to GL texture %u", gl_texture);
            // Upload to texture
            glBindTexture(GL_TEXTURE_2D, gl_texture);
            
            GLenum internal_format, format, type;
            get_gl_format(format_, internal_format, format, type);
            DX8GL_INFO("Texture format conversion: D3D format=%d -> GL internal=%d, format=%d, type=%d", 
                       format_, internal_format, format, type);
            
            if (lock_rect_.left == 0 && lock_rect_.top == 0 &&
                lock_rect_.right == static_cast<LONG>(width_) && 
                lock_rect_.bottom == static_cast<LONG>(height_)) {
                // Full surface update
                DX8GL_INFO("Full surface update: format_=%d, GL format=%d", format_, format);
#ifdef __EMSCRIPTEN__
                // For WebGL, convert ARGB to RGBA if needed
                DX8GL_INFO("Checking conversion: format_=%d (A8R8G8B8=%d, X8R8G8B8=%d), GL format=%d (GL_RGBA=%d)", 
                           format_, D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8, format, GL_RGBA);
                if ((format_ == D3DFMT_A8R8G8B8 || format_ == D3DFMT_X8R8G8B8) && format == GL_RGBA) {
                    DX8GL_INFO("Converting ARGB to RGBA for WebGL");
                    void* converted_data = malloc(width_ * height_ * 4);
                    if (converted_data) {
                        BYTE* src_bytes = static_cast<BYTE*>(lock_buffer_);
                        BYTE* dst_bytes = static_cast<BYTE*>(converted_data);
                        for (UINT i = 0; i < width_ * height_; i++) {
                            // DirectX: ARGB (A=src[3], R=src[2], G=src[1], B=src[0] on little-endian)
                            // WebGL:   RGBA (R=dst[0], G=dst[1], B=dst[2], A=dst[3])
                            dst_bytes[i*4 + 0] = src_bytes[i*4 + 2]; // R
                            dst_bytes[i*4 + 1] = src_bytes[i*4 + 1]; // G  
                            dst_bytes[i*4 + 2] = src_bytes[i*4 + 0]; // B
                            dst_bytes[i*4 + 3] = src_bytes[i*4 + 3]; // A
                        }
                        DX8GL_INFO("Converted texture data: first pixel ARGB 0x%02x%02x%02x%02x -> RGBA 0x%02x%02x%02x%02x",
                                   src_bytes[3], src_bytes[2], src_bytes[1], src_bytes[0],
                                   dst_bytes[0], dst_bytes[1], dst_bytes[2], dst_bytes[3]);
                        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width_, height_, 0,
                                    format, type, converted_data);
                        free(converted_data);
                    }
                } else {
                    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width_, height_, 0,
                                format, type, lock_buffer_);
                }
#else
                glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width_, height_, 0,
                            format, type, lock_buffer_);
#endif
            } else {
                // Partial update
                UINT lock_width = lock_rect_.right - lock_rect_.left;
                UINT lock_height = lock_rect_.bottom - lock_rect_.top;
                
                // Create temporary buffer for sub-region
                UINT pixel_size = get_format_size(format_);
                void* sub_data = malloc(lock_width * lock_height * pixel_size);
                if (sub_data) {
                    // Copy sub-region to temporary buffer
                    BYTE* src = static_cast<BYTE*>(lock_buffer_) + 
                               (lock_rect_.top * pitch_) + 
                               (lock_rect_.left * pixel_size);
                    BYTE* dst = static_cast<BYTE*>(sub_data);
                    
                    for (UINT y = 0; y < lock_height; y++) {
                        memcpy(dst, src, lock_width * pixel_size);
                        src += pitch_;
                        dst += lock_width * pixel_size;
                    }
                    
                    glTexSubImage2D(GL_TEXTURE_2D, 0, lock_rect_.left, lock_rect_.top,
                                   lock_width, lock_height, format, type, sub_data);
                    
                    free(sub_data);
                }
            }
            
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
    
    locked_ = false;
    lock_flags_ = 0;
    
    return D3D_OK;
}

// Internal methods
bool Direct3DSurface8::copy_from(Direct3DSurface8* source, const RECT* src_rect, 
                                const POINT* dest_point) {
    if (!source) {
        return false;
    }
    
    DX8GL_INFO("copy_from: source format=%d, dest format=%d", source->format_, format_);
    
    // Determine source rectangle
    RECT src;
    if (src_rect) {
        src = *src_rect;
        // Validate source rect
        if (src.left < 0 || src.top < 0 ||
            src.right > static_cast<LONG>(source->width_) || 
            src.bottom > static_cast<LONG>(source->height_) ||
            src.left >= src.right || src.top >= src.bottom) {
            DX8GL_ERROR("Invalid source rectangle");
            return false;
        }
    } else {
        src.left = 0;
        src.top = 0;
        src.right = source->width_;
        src.bottom = source->height_;
    }
    
    // Determine destination point
    POINT dest;
    if (dest_point) {
        dest = *dest_point;
    } else {
        dest.x = 0;
        dest.y = 0;
    }
    
    UINT copy_width = src.right - src.left;
    UINT copy_height = src.bottom - src.top;
    
    // Validate destination
    if (dest.x < 0 || dest.y < 0 ||
        dest.x + copy_width > width_ || dest.y + copy_height > height_) {
        DX8GL_ERROR("Copy would exceed destination bounds");
        return false;
    }
    
    // Check if this is a depth/stencil copy
    bool is_depth_copy = is_depth_stencil() && source->is_depth_stencil();
    
    // ES 2.0-compatible implementation using glReadPixels + glTexSubImage2D
    if (!is_depth_copy && (source->texture_ || source->parent_texture_) && 
        (texture_ || parent_texture_)) {
        
        // Get source GL texture
        GLuint src_texture = source->texture_;
        if (!src_texture && source->parent_texture_) {
            src_texture = source->parent_texture_->get_gl_texture();
        }
        
        // Get destination GL texture
        GLuint dst_texture = texture_;
        if (!dst_texture && parent_texture_) {
            dst_texture = parent_texture_->get_gl_texture();
        }
        
        if (!src_texture || !dst_texture) {
            DX8GL_ERROR("Missing texture for surface copy");
            return false;
        }
        
        // Allocate temporary buffer for pixel data
        UINT pixel_size = get_format_size(source->format_);
        UINT buffer_size = copy_width * copy_height * pixel_size;
        void* temp_buffer = malloc(buffer_size);
        if (!temp_buffer) {
            DX8GL_ERROR("Failed to allocate temporary buffer for copy");
            return false;
        }
        
        // Create FBO for reading from source
        GLuint read_fbo = 0;
        glGenFramebuffers(1, &read_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, read_fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_TEXTURE_2D, src_texture, source->level_);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            DX8GL_ERROR("Failed to create read framebuffer");
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &read_fbo);
            free(temp_buffer);
            return false;
        }
        
        // Read pixels from source
        GLenum src_internal_format, src_format, src_type;
        get_gl_format(source->format_, src_internal_format, src_format, src_type);
        
        glReadPixels(src.left, src.top, copy_width, copy_height,
                     src_format, src_type, temp_buffer);
        
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            DX8GL_ERROR("glReadPixels failed: 0x%04x", error);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &read_fbo);
            free(temp_buffer);
            return false;
        }
        
        // Cleanup read FBO
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &read_fbo);
        
        // Handle format conversion if needed
        void* write_buffer = temp_buffer;
        void* converted_buffer = nullptr;
        
        if (source->format_ != format_) {
            // Need format conversion
            converted_buffer = malloc(buffer_size);
            if (converted_buffer) {
                if (!convert_format(temp_buffer, converted_buffer, 
                                   source->format_, format_, 
                                   copy_width * copy_height)) {
                    DX8GL_WARNING("Format conversion failed, using source format");
                    free(converted_buffer);
                    converted_buffer = nullptr;
                } else {
                    write_buffer = converted_buffer;
                }
            }
        }
        
        // Write pixels to destination
        GLenum dst_internal_format, dst_format, dst_type;
        get_gl_format(format_, dst_internal_format, dst_format, dst_type);
        
        glBindTexture(GL_TEXTURE_2D, dst_texture);
        glTexSubImage2D(GL_TEXTURE_2D, level_, dest.x, dest.y,
                       copy_width, copy_height, dst_format, dst_type, write_buffer);
        
        error = glGetError();
        if (error != GL_NO_ERROR) {
            DX8GL_ERROR("glTexSubImage2D failed: 0x%04x", error);
        }
        
        glBindTexture(GL_TEXTURE_2D, 0);
        
        // Cleanup
        free(temp_buffer);
        if (converted_buffer) {
            free(converted_buffer);
        }
        
        // Mark destination as dirty if it's a managed texture
        if (parent_texture_ && parent_texture_->get_pool() == D3DPOOL_MANAGED) {
            RECT dirty_rect = { dest.x, dest.y, 
                               dest.x + static_cast<LONG>(copy_width), 
                               dest.y + static_cast<LONG>(copy_height) };
            parent_texture_->mark_level_dirty(level_, &dirty_rect);
        }
        
        DX8GL_DEBUG("Surface copy completed: %ux%u from (%d,%d) to (%d,%d)",
                    copy_width, copy_height, src.left, src.top, dest.x, dest.y);
        
        return true;
    }
    
    // Depth/stencil copies are not supported in ES 2.0
    if (is_depth_copy) {
        DX8GL_WARNING("Depth/stencil surface copies not supported in ES 2.0");
        return false;
    }
    
    DX8GL_ERROR("Unsupported surface copy configuration");
    return false;
}

// Static helper methods
bool Direct3DSurface8::get_gl_format(D3DFORMAT d3d_format, GLenum& internal_format,
                                     GLenum& format, GLenum& type) {
    switch (d3d_format) {
        // Color formats
        case D3DFMT_R8G8B8:
            internal_format = GL_RGB;
            format = GL_RGB;
            type = GL_UNSIGNED_BYTE;
            return true;
            
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
#ifdef __EMSCRIPTEN__
            // WebGL doesn't support GL_BGRA, use RGBA and handle swizzling elsewhere
            internal_format = GL_RGBA;
            format = GL_RGBA;
            type = GL_UNSIGNED_BYTE;
#else
            // DirectX uses XRGB/ARGB byte order, OpenGL BGRA matches this
            internal_format = GL_RGBA;
            format = GL_BGRA;
            type = GL_UNSIGNED_BYTE;
#endif
            return true;
            
        case D3DFMT_R5G6B5:
            internal_format = GL_RGB;
            format = GL_RGB;
            type = GL_UNSIGNED_SHORT_5_6_5;
            return true;
            
        case D3DFMT_A4R4G4B4:
        case D3DFMT_X4R4G4B4:
            internal_format = GL_RGBA;
            format = GL_RGBA;
            type = GL_UNSIGNED_SHORT_4_4_4_4;
            return true;
            
        case D3DFMT_A1R5G5B5:
        case D3DFMT_X1R5G5B5:
            internal_format = GL_RGBA;
            format = GL_RGBA;
            type = GL_UNSIGNED_SHORT_5_5_5_1;
            return true;
            
        // Depth/stencil formats
        case D3DFMT_D16:
            internal_format = GL_DEPTH_COMPONENT16;
            format = GL_DEPTH_COMPONENT;
            type = GL_UNSIGNED_SHORT;
            return true;
            
        case D3DFMT_D24S8:
        case D3DFMT_D24X8:
#ifdef __EMSCRIPTEN__
            // WebGL with OES_packed_depth_stencil extension
            internal_format = GL_DEPTH_STENCIL;
            format = GL_DEPTH_STENCIL;
            type = GL_UNSIGNED_INT_24_8;
#else
            // Use ES 2.0 compatible formats
            internal_format = GL_DEPTH_COMPONENT;
            format = GL_DEPTH_COMPONENT;
            type = GL_UNSIGNED_SHORT;
#endif
            return true;
            
        case D3DFMT_D32:
            internal_format = GL_DEPTH_COMPONENT;
            format = GL_DEPTH_COMPONENT;
            type = GL_UNSIGNED_INT;
            return true;
            
        // Luminance formats
        case D3DFMT_L8:
            internal_format = GL_LUMINANCE;
            format = GL_LUMINANCE;
            type = GL_UNSIGNED_BYTE;
            return true;
            
        case D3DFMT_A8L8:
            internal_format = GL_LUMINANCE_ALPHA;
            format = GL_LUMINANCE_ALPHA;
            type = GL_UNSIGNED_BYTE;
            return true;
            
        // Alpha format
        case D3DFMT_A8:
            internal_format = GL_ALPHA;
            format = GL_ALPHA;
            type = GL_UNSIGNED_BYTE;
            return true;
            
        default:
            DX8GL_ERROR("Unsupported D3D format: %d", d3d_format);
            return false;
    }
}

UINT Direct3DSurface8::get_format_size(D3DFORMAT format) {
    switch (format) {
        case D3DFMT_R8G8B8:
            return 3;
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
        case D3DFMT_D24S8:
        case D3DFMT_D24X8:
        case D3DFMT_D32:
            return 4;
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_A4R4G4B4:
        case D3DFMT_X4R4G4B4:
        case D3DFMT_D16:
        case D3DFMT_A8L8:
            return 2;
        case D3DFMT_L8:
        case D3DFMT_A8:
            return 1;
        default:
            return 4;  // Default to 4 bytes
    }
}

bool Direct3DSurface8::convert_format(const void* src, void* dst, D3DFORMAT src_format,
                                     D3DFORMAT dst_format, UINT pixel_count) {
    if (!src || !dst || src_format == dst_format) {
        return false;
    }
    
    const BYTE* src_bytes = static_cast<const BYTE*>(src);
    BYTE* dst_bytes = static_cast<BYTE*>(dst);
    
    // Handle common conversions
    if (src_format == D3DFMT_A8R8G8B8 && dst_format == D3DFMT_X8R8G8B8) {
        // ARGB to XRGB - just set alpha to 255
        const DWORD* src_pixels = reinterpret_cast<const DWORD*>(src_bytes);
        DWORD* dst_pixels = reinterpret_cast<DWORD*>(dst_bytes);
        for (UINT i = 0; i < pixel_count; i++) {
            dst_pixels[i] = src_pixels[i] | 0xFF000000;
        }
        return true;
    }
    else if (src_format == D3DFMT_X8R8G8B8 && dst_format == D3DFMT_A8R8G8B8) {
        // XRGB to ARGB - alpha is already 255 in X format
        memcpy(dst_bytes, src_bytes, pixel_count * 4);
        return true;
    }
    else if (src_format == D3DFMT_A8R8G8B8 && dst_format == D3DFMT_R5G6B5) {
        // ARGB32 to RGB565
        const DWORD* src_pixels = reinterpret_cast<const DWORD*>(src_bytes);
        WORD* dst_pixels = reinterpret_cast<WORD*>(dst_bytes);
        for (UINT i = 0; i < pixel_count; i++) {
            DWORD pixel = src_pixels[i];
            BYTE r = (pixel >> 16) & 0xFF;
            BYTE g = (pixel >> 8) & 0xFF;
            BYTE b = pixel & 0xFF;
            dst_pixels[i] = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
        }
        return true;
    }
    else if (src_format == D3DFMT_R5G6B5 && dst_format == D3DFMT_A8R8G8B8) {
        // RGB565 to ARGB32
        const WORD* src_pixels = reinterpret_cast<const WORD*>(src_bytes);
        DWORD* dst_pixels = reinterpret_cast<DWORD*>(dst_bytes);
        for (UINT i = 0; i < pixel_count; i++) {
            WORD pixel = src_pixels[i];
            BYTE r = ((pixel >> 11) & 0x1F) << 3;
            BYTE g = ((pixel >> 5) & 0x3F) << 2;
            BYTE b = (pixel & 0x1F) << 3;
            // Replicate high bits to low bits for better color accuracy
            r |= r >> 5;
            g |= g >> 6;
            b |= b >> 5;
            dst_pixels[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
        return true;
    }
    else if (src_format == D3DFMT_R5G6B5 && dst_format == D3DFMT_X8R8G8B8) {
        // RGB565 to XRGB32 - same as above
        const WORD* src_pixels = reinterpret_cast<const WORD*>(src_bytes);
        DWORD* dst_pixels = reinterpret_cast<DWORD*>(dst_bytes);
        for (UINT i = 0; i < pixel_count; i++) {
            WORD pixel = src_pixels[i];
            BYTE r = ((pixel >> 11) & 0x1F) << 3;
            BYTE g = ((pixel >> 5) & 0x3F) << 2;
            BYTE b = (pixel & 0x1F) << 3;
            r |= r >> 5;
            g |= g >> 6;
            b |= b >> 5;
            dst_pixels[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
        return true;
    }
    else if (src_format == D3DFMT_L8 && dst_format == D3DFMT_A8R8G8B8) {
        // Luminance to ARGB - replicate luminance to RGB
        for (UINT i = 0; i < pixel_count; i++) {
            BYTE l = src_bytes[i];
            DWORD* dst_pixel = reinterpret_cast<DWORD*>(dst_bytes + i * 4);
            *dst_pixel = 0xFF000000 | (l << 16) | (l << 8) | l;
        }
        return true;
    }
    else if (src_format == D3DFMT_A8L8 && dst_format == D3DFMT_A8R8G8B8) {
        // Alpha-luminance to ARGB
        for (UINT i = 0; i < pixel_count; i++) {
            BYTE l = src_bytes[i * 2];
            BYTE a = src_bytes[i * 2 + 1];
            DWORD* dst_pixel = reinterpret_cast<DWORD*>(dst_bytes + i * 4);
            *dst_pixel = (a << 24) | (l << 16) | (l << 8) | l;
        }
        return true;
    }
    
    DX8GL_WARNING("Unsupported format conversion: %d to %d", src_format, dst_format);
    return false;
}

} // namespace dx8gl