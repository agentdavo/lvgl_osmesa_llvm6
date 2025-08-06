#include "d3dx_compat.h"
#include "d3d8_device.h"
#include "d3d8_texture.h"
#include "logger.h"
#include <cstring>
#include <fstream>
#include <vector>
#include <algorithm>

// Missing Windows type definitions for Emscripten
typedef const char* LPCSTR;
typedef const void* LPCVOID;

// D3DX types and constants
typedef struct _D3DXIMAGE_INFO {
    UINT Width;
    UINT Height;
    UINT Depth;
    UINT MipLevels;
    D3DFORMAT Format;
    D3DRESOURCETYPE ResourceType;
    DWORD ImageFileFormat;
} D3DXIMAGE_INFO;

typedef enum _D3DXIMAGE_FILEFORMAT {
    D3DXIFF_BMP = 0,
    D3DXIFF_JPG = 1,
    D3DXIFF_TGA = 2,
    D3DXIFF_PNG = 3,
    D3DXIFF_DDS = 4,
    D3DXIFF_PPM = 5,
    D3DXIFF_DIB = 6,
    D3DXIFF_HDR = 7,
    D3DXIFF_PFM = 8,
    D3DXIFF_FORCE_DWORD = 0x7fffffff
} D3DXIMAGE_FILEFORMAT;

// Simple image format detection
enum ImageFormat {
    FORMAT_UNKNOWN,
    FORMAT_BMP,
    FORMAT_TGA,
    FORMAT_DDS,
    FORMAT_PNG,
    FORMAT_JPG
};


static ImageFormat detect_image_format(const uint8_t* data, size_t size) {
    if (size < 4) return FORMAT_UNKNOWN;
    
    // BMP: "BM"
    if (data[0] == 'B' && data[1] == 'M') {
        return FORMAT_BMP;
    }
    
    // DDS: "DDS "
    if (data[0] == 'D' && data[1] == 'D' && data[2] == 'S' && data[3] == ' ') {
        return FORMAT_DDS;
    }
    
    // PNG: \x89PNG
    if (data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G') {
        return FORMAT_PNG;
    }
    
    // JPEG: 0xFFD8
    if (data[0] == 0xFF && data[1] == 0xD8) {
        return FORMAT_JPG;
    }
    
    // TGA has no magic number, try to detect by header
    if (size >= 18) {
        uint8_t id_length = data[0];
        uint8_t color_map_type = data[1];
        uint8_t image_type = data[2];
        
        if (color_map_type <= 1 && 
            (image_type == 1 || image_type == 2 || image_type == 3 ||
             image_type == 9 || image_type == 10 || image_type == 11)) {
            return FORMAT_TGA;
        }
    }
    
    return FORMAT_UNKNOWN;
}

// Simple BMP loader (supports 24-bit and 32-bit uncompressed BMPs)
static bool load_bmp(const uint8_t* data, size_t size, 
                    std::vector<uint8_t>& pixels, 
                    UINT& width, UINT& height, D3DFORMAT& format) {
    if (size < 54) return false;  // Minimum BMP header size
    
    // BMP header
    uint32_t file_size = *reinterpret_cast<const uint32_t*>(data + 2);
    uint32_t data_offset = *reinterpret_cast<const uint32_t*>(data + 10);
    uint32_t header_size = *reinterpret_cast<const uint32_t*>(data + 14);
    
    if (header_size < 40) return false;  // We only support BITMAPINFOHEADER
    
    int32_t bmp_width = *reinterpret_cast<const int32_t*>(data + 18);
    int32_t bmp_height = *reinterpret_cast<const int32_t*>(data + 22);
    uint16_t planes = *reinterpret_cast<const uint16_t*>(data + 26);
    uint16_t bits_per_pixel = *reinterpret_cast<const uint16_t*>(data + 28);
    uint32_t compression = *reinterpret_cast<const uint32_t*>(data + 30);
    
    if (planes != 1 || compression != 0) return false;  // Uncompressed only
    if (bits_per_pixel != 24 && bits_per_pixel != 32) return false;
    
    width = std::abs(bmp_width);
    height = std::abs(bmp_height);
    format = (bits_per_pixel == 32) ? D3DFMT_A8R8G8B8 : D3DFMT_X8R8G8B8;
    
    // Calculate row stride (BMP rows are padded to 4-byte boundary)
    size_t row_stride = ((bmp_width * bits_per_pixel + 31) / 32) * 4;
    size_t expected_size = data_offset + row_stride * height;
    if (size < expected_size) return false;
    
    // Allocate pixel buffer
    pixels.resize(width * height * 4);  // Always output as 32-bit
    
    // Copy pixels (BMP is bottom-up by default)
    const uint8_t* src = data + data_offset;
    bool bottom_up = bmp_height > 0;
    
    for (uint32_t y = 0; y < height; y++) {
        uint32_t src_y = bottom_up ? (height - 1 - y) : y;
        const uint8_t* src_row = src + src_y * row_stride;
        uint8_t* dst_row = pixels.data() + y * width * 4;
        
        if (bits_per_pixel == 24) {
            // Convert BGR to ARGB
            for (uint32_t x = 0; x < width; x++) {
                dst_row[x * 4 + 0] = src_row[x * 3 + 2];  // B
                dst_row[x * 4 + 1] = src_row[x * 3 + 1];  // G
                dst_row[x * 4 + 2] = src_row[x * 3 + 0];  // R
                dst_row[x * 4 + 3] = 0xFF;                // A
            }
        } else {
            // Copy BGRA to ARGB
            for (uint32_t x = 0; x < width; x++) {
                dst_row[x * 4 + 0] = src_row[x * 4 + 2];  // B
                dst_row[x * 4 + 1] = src_row[x * 4 + 1];  // G
                dst_row[x * 4 + 2] = src_row[x * 4 + 0];  // R
                dst_row[x * 4 + 3] = src_row[x * 4 + 3];  // A
            }
        }
    }
    
    return true;
}

// Simple TGA loader (supports 24-bit and 32-bit uncompressed TGAs)
static bool load_tga(const uint8_t* data, size_t size,
                    std::vector<uint8_t>& pixels,
                    UINT& width, UINT& height, D3DFORMAT& format) {
    if (size < 18) return false;  // Minimum TGA header size
    
    // TGA header
    uint8_t id_length = data[0];
    uint8_t color_map_type = data[1];
    uint8_t image_type = data[2];
    uint16_t color_map_first = *reinterpret_cast<const uint16_t*>(data + 3);
    uint16_t color_map_length = *reinterpret_cast<const uint16_t*>(data + 5);
    uint8_t color_map_entry_size = data[7];
    uint16_t x_origin = *reinterpret_cast<const uint16_t*>(data + 8);
    uint16_t y_origin = *reinterpret_cast<const uint16_t*>(data + 10);
    uint16_t tga_width = *reinterpret_cast<const uint16_t*>(data + 12);
    uint16_t tga_height = *reinterpret_cast<const uint16_t*>(data + 14);
    uint8_t pixel_depth = data[16];
    uint8_t image_descriptor = data[17];
    
    // We only support uncompressed true-color images
    if (image_type != 2 || color_map_type != 0) return false;
    if (pixel_depth != 24 && pixel_depth != 32) return false;
    
    width = tga_width;
    height = tga_height;
    format = (pixel_depth == 32) ? D3DFMT_A8R8G8B8 : D3DFMT_X8R8G8B8;
    
    // Skip ID field
    size_t data_offset = 18 + id_length;
    if (size < data_offset + width * height * (pixel_depth / 8)) return false;
    
    // Allocate pixel buffer
    pixels.resize(width * height * 4);  // Always output as 32-bit
    
    // Copy pixels
    const uint8_t* src = data + data_offset;
    bool bottom_up = (image_descriptor & 0x20) == 0;
    
    for (uint32_t y = 0; y < height; y++) {
        uint32_t src_y = bottom_up ? (height - 1 - y) : y;
        const uint8_t* src_row = src + src_y * width * (pixel_depth / 8);
        uint8_t* dst_row = pixels.data() + y * width * 4;
        
        if (pixel_depth == 24) {
            // Convert BGR to ARGB
            for (uint32_t x = 0; x < width; x++) {
                dst_row[x * 4 + 0] = src_row[x * 3 + 2];  // B
                dst_row[x * 4 + 1] = src_row[x * 3 + 1];  // G
                dst_row[x * 4 + 2] = src_row[x * 3 + 0];  // R
                dst_row[x * 4 + 3] = 0xFF;                // A
            }
        } else {
            // Copy BGRA to ARGB
            for (uint32_t x = 0; x < width; x++) {
                dst_row[x * 4 + 0] = src_row[x * 4 + 2];  // B
                dst_row[x * 4 + 1] = src_row[x * 4 + 1];  // G
                dst_row[x * 4 + 2] = src_row[x * 4 + 0];  // R
                dst_row[x * 4 + 3] = src_row[x * 4 + 3];  // A
            }
        }
    }
    
    return true;
}

// D3DX Implementation
extern "C" {

HRESULT WINAPI D3DXCreateTexture(
    IDirect3DDevice8* pDevice,
    UINT Width,
    UINT Height,
    UINT MipLevels,
    DWORD Usage,
    D3DFORMAT Format,
    D3DPOOL Pool,
    IDirect3DTexture8** ppTexture)
{
    if (!pDevice || !ppTexture) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_INFO("D3DXCreateTexture: %ux%u, MipLevels=%u, Format=%d, Pool=%d",
               Width, Height, MipLevels, Format, Pool);
    
    return pDevice->CreateTexture(Width, Height, MipLevels, Usage, Format, Pool, ppTexture);
}

HRESULT WINAPI D3DXCreateTextureFromFile(
    IDirect3DDevice8* pDevice,
    const char* pSrcFile,
    IDirect3DTexture8** ppTexture)
{
    return D3DXCreateTextureFromFileEx(pDevice, pSrcFile, 
                                      D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT,
                                      0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
                                      D3DX_DEFAULT, D3DX_DEFAULT, 0,
                                      nullptr, nullptr, ppTexture);
}

HRESULT WINAPI D3DXCreateTextureFromFileEx(
    IDirect3DDevice8* pDevice,
    const char* pSrcFile,
    UINT Width,
    UINT Height,
    UINT MipLevels,
    DWORD Usage,
    D3DFORMAT Format,
    D3DPOOL Pool,
    DWORD Filter,
    DWORD MipFilter,
    D3DCOLOR ColorKey,
    void* pSrcInfo,
    void* pPalette,
    IDirect3DTexture8** ppTexture)
{
    if (!pDevice || !pSrcFile || !ppTexture) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_INFO("D3DXCreateTextureFromFileEx: %s", pSrcFile);
    
    // Read file into memory
    std::ifstream file(pSrcFile, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        DX8GL_ERROR("Failed to open file: %s", pSrcFile);
        return D3DERR_NOTFOUND;
    }
    
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> file_data(file_size);
    file.read(reinterpret_cast<char*>(file_data.data()), file_size);
    file.close();
    
    // Load from memory
    return D3DXCreateTextureFromFileInMemoryEx(pDevice, file_data.data(), file_size,
                                               Width, Height, MipLevels, Usage,
                                               Format, Pool, Filter, MipFilter,
                                               ColorKey, pSrcInfo, pPalette, ppTexture);
}

HRESULT WINAPI D3DXCreateTextureFromFileInMemory(
    IDirect3DDevice8* pDevice,
    LPCVOID pSrcData,
    UINT SrcDataSize,
    IDirect3DTexture8** ppTexture)
{
    return D3DXCreateTextureFromFileInMemoryEx(pDevice, pSrcData, SrcDataSize,
                                               D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT,
                                               0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
                                               D3DX_DEFAULT, D3DX_DEFAULT, 0,
                                               nullptr, nullptr, ppTexture);
}

HRESULT WINAPI D3DXCreateTextureFromFileInMemoryEx(
    IDirect3DDevice8* pDevice,
    LPCVOID pSrcData,
    UINT SrcDataSize,
    UINT Width,
    UINT Height,
    UINT MipLevels,
    DWORD Usage,
    D3DFORMAT Format,
    D3DPOOL Pool,
    DWORD Filter,
    DWORD MipFilter,
    D3DCOLOR ColorKey,
    void* pSrcInfo,
    void* pPalette,
    IDirect3DTexture8** ppTexture)
{
    if (!pDevice || !pSrcData || !ppTexture || SrcDataSize == 0) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_INFO("D3DXCreateTextureFromFileInMemoryEx: %u bytes", SrcDataSize);
    
    // Detect image format
    const uint8_t* data = static_cast<const uint8_t*>(pSrcData);
    ImageFormat img_format = detect_image_format(data, SrcDataSize);
    
    std::vector<uint8_t> pixels;
    UINT img_width = 0, img_height = 0;
    D3DFORMAT img_format_d3d = D3DFMT_UNKNOWN;
    
    // Load image based on format
    bool loaded = false;
    switch (img_format) {
        case FORMAT_BMP:
            loaded = load_bmp(data, SrcDataSize, pixels, img_width, img_height, img_format_d3d);
            break;
        case FORMAT_TGA:
            loaded = load_tga(data, SrcDataSize, pixels, img_width, img_height, img_format_d3d);
            break;
        case FORMAT_DDS:
            // DDS is more complex, skip for now
            DX8GL_WARNING("DDS format not yet supported");
            break;
        case FORMAT_PNG:
            DX8GL_WARNING("PNG format not yet supported");
            break;
        case FORMAT_JPG:
            DX8GL_WARNING("JPEG format not yet supported");
            break;
        default:
            DX8GL_ERROR("Unknown image format");
            break;
    }
    
    if (!loaded) {
        return D3DERR_NOTFOUND;
    }
    
    // Fill source info if requested
    if (pSrcInfo) {
        D3DXIMAGE_INFO* info = static_cast<D3DXIMAGE_INFO*>(pSrcInfo);
        info->Width = img_width;
        info->Height = img_height;
        info->Depth = 1;
        info->MipLevels = 1;
        info->Format = img_format_d3d;
        info->ResourceType = D3DRTYPE_TEXTURE;
        info->ImageFileFormat = D3DXIFF_BMP;  // Simplification
    }
    
    // Determine final texture parameters
    UINT tex_width = (Width == D3DX_DEFAULT) ? img_width : Width;
    UINT tex_height = (Height == D3DX_DEFAULT) ? img_height : Height;
    D3DFORMAT tex_format = (Format == D3DFMT_UNKNOWN) ? img_format_d3d : Format;
    
    // Create texture
    IDirect3DTexture8* texture = nullptr;
    HRESULT hr = pDevice->CreateTexture(tex_width, tex_height, 
                                       (MipLevels == D3DX_DEFAULT) ? 0 : MipLevels,
                                       Usage, tex_format, Pool, &texture);
    if (FAILED(hr)) {
        return hr;
    }
    
    // Lock and fill texture
    D3DLOCKED_RECT locked_rect;
    hr = texture->LockRect(0, &locked_rect, nullptr, 0);
    if (FAILED(hr)) {
        texture->Release();
        return hr;
    }
    
    // Simple copy or resize
    if (tex_width == img_width && tex_height == img_height) {
        // Direct copy
        uint8_t* dst = static_cast<uint8_t*>(locked_rect.pBits);
        for (UINT y = 0; y < tex_height; y++) {
            memcpy(dst + y * locked_rect.Pitch,
                   pixels.data() + y * img_width * 4,
                   img_width * 4);
        }
    } else {
        // Simple point sampling resize
        uint8_t* dst = static_cast<uint8_t*>(locked_rect.pBits);
        for (UINT y = 0; y < tex_height; y++) {
            for (UINT x = 0; x < tex_width; x++) {
                UINT src_x = x * img_width / tex_width;
                UINT src_y = y * img_height / tex_height;
                uint32_t* src_pixel = reinterpret_cast<uint32_t*>(
                    pixels.data() + (src_y * img_width + src_x) * 4);
                uint32_t* dst_pixel = reinterpret_cast<uint32_t*>(
                    dst + y * locked_rect.Pitch + x * 4);
                *dst_pixel = *src_pixel;
            }
        }
    }
    
    texture->UnlockRect(0);
    
    // Generate mipmaps if requested
    if (MipLevels == D3DX_DEFAULT || MipLevels > 1) {
        D3DXFilterTexture(texture, nullptr, 0, D3DX_DEFAULT);
    }
    
    *ppTexture = texture;
    return D3D_OK;
}

HRESULT WINAPI D3DXFilterTexture(
    IDirect3DTexture8* pTexture,
    void* pPalette,
    DWORD SrcLevel,
    DWORD Filter)
{
    if (!pTexture) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_INFO("D3DXFilterTexture: SrcLevel=%u, Filter=0x%08x", SrcLevel, Filter);
    
    // Get texture description
    D3DSURFACE_DESC desc;
    HRESULT hr = pTexture->GetLevelDesc(SrcLevel, &desc);
    if (FAILED(hr)) {
        return hr;
    }
    
    // Get number of mip levels
    DWORD level_count = pTexture->GetLevelCount();
    if (SrcLevel >= level_count - 1) {
        return D3D_OK;  // Nothing to generate
    }
    
    // Simple box filter mipmap generation
    for (UINT level = SrcLevel + 1; level < level_count; level++) {
        D3DLOCKED_RECT src_rect, dst_rect;
        
        // Lock source level
        hr = pTexture->LockRect(level - 1, &src_rect, nullptr, D3DLOCK_READONLY);
        if (FAILED(hr)) continue;
        
        // Lock destination level
        hr = pTexture->LockRect(level, &dst_rect, nullptr, 0);
        if (FAILED(hr)) {
            pTexture->UnlockRect(level - 1);
            continue;
        }
        
        // Get level dimensions
        D3DSURFACE_DESC src_desc, dst_desc;
        pTexture->GetLevelDesc(level - 1, &src_desc);
        pTexture->GetLevelDesc(level, &dst_desc);
        
        // Simple 2x2 box filter
        uint32_t* src = static_cast<uint32_t*>(src_rect.pBits);
        uint32_t* dst = static_cast<uint32_t*>(dst_rect.pBits);
        
        for (UINT y = 0; y < dst_desc.Height; y++) {
            for (UINT x = 0; x < dst_desc.Width; x++) {
                // Sample 2x2 block from source
                UINT src_x = x * 2;
                UINT src_y = y * 2;
                
                uint32_t p00 = src[(src_y + 0) * (src_rect.Pitch / 4) + (src_x + 0)];
                uint32_t p01 = src[(src_y + 0) * (src_rect.Pitch / 4) + (src_x + 1)];
                uint32_t p10 = src[(src_y + 1) * (src_rect.Pitch / 4) + (src_x + 0)];
                uint32_t p11 = src[(src_y + 1) * (src_rect.Pitch / 4) + (src_x + 1)];
                
                // Average the colors
                uint32_t r = ((p00 >> 16) & 0xFF) + ((p01 >> 16) & 0xFF) + 
                            ((p10 >> 16) & 0xFF) + ((p11 >> 16) & 0xFF);
                uint32_t g = ((p00 >> 8) & 0xFF) + ((p01 >> 8) & 0xFF) + 
                            ((p10 >> 8) & 0xFF) + ((p11 >> 8) & 0xFF);
                uint32_t b = (p00 & 0xFF) + (p01 & 0xFF) + 
                            (p10 & 0xFF) + (p11 & 0xFF);
                uint32_t a = ((p00 >> 24) & 0xFF) + ((p01 >> 24) & 0xFF) + 
                            ((p10 >> 24) & 0xFF) + ((p11 >> 24) & 0xFF);
                
                dst[y * (dst_rect.Pitch / 4) + x] = 
                    ((a / 4) << 24) | ((r / 4) << 16) | ((g / 4) << 8) | (b / 4);
            }
        }
        
        pTexture->UnlockRect(level);
        pTexture->UnlockRect(level - 1);
    }
    
    return D3D_OK;
}

// Forward declarations for helper functions
static UINT get_bytes_per_pixel(D3DFORMAT format);
static uint32_t convert_pixel(uint32_t src_pixel, D3DFORMAT src_format, D3DFORMAT dst_format);
static bool matches_color_key(uint32_t pixel, D3DCOLOR color_key, D3DFORMAT format);

HRESULT WINAPI D3DXLoadSurfaceFromMemory(
    IDirect3DSurface8* pDestSurface,
    const PALETTEENTRY* pDestPalette,
    const RECT* pDestRect,
    LPCVOID pSrcMemory,
    D3DFORMAT SrcFormat,
    UINT SrcPitch,
    const PALETTEENTRY* pSrcPalette,
    const RECT* pSrcRect,
    DWORD Filter,
    D3DCOLOR ColorKey)
{
    if (!pDestSurface || !pSrcMemory) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_INFO("D3DXLoadSurfaceFromMemory: SrcFormat=%d, Filter=0x%08x, ColorKey=0x%08x", 
               SrcFormat, Filter, ColorKey);
    
    // Get surface description
    D3DSURFACE_DESC desc;
    HRESULT hr = pDestSurface->GetDesc(&desc);
    if (FAILED(hr)) {
        return hr;
    }
    
    // Determine source and destination rectangles
    RECT src_rect = { 0, 0, (LONG)(SrcPitch / get_bytes_per_pixel(SrcFormat)), 0 };
    if (pSrcRect) {
        src_rect = *pSrcRect;
    } else {
        // Try to infer source height from pitch and format
        src_rect.right = SrcPitch / get_bytes_per_pixel(SrcFormat);
        src_rect.bottom = desc.Height; // Assume same height as destination
    }
    
    RECT dest_rect = { 0, 0, (LONG)desc.Width, (LONG)desc.Height };
    if (pDestRect) {
        dest_rect = *pDestRect;
    }
    
    // Lock destination surface
    D3DLOCKED_RECT locked_rect;
    hr = pDestSurface->LockRect(&locked_rect, pDestRect, 0);
    if (FAILED(hr)) {
        return hr;
    }
    
    const uint8_t* src = static_cast<const uint8_t*>(pSrcMemory);
    uint8_t* dst = static_cast<uint8_t*>(locked_rect.pBits);
    
    UINT src_bpp = get_bytes_per_pixel(SrcFormat);
    UINT dst_bpp = get_bytes_per_pixel(desc.Format);
    
    UINT src_width = src_rect.right - src_rect.left;
    UINT src_height = src_rect.bottom - src_rect.top;
    UINT dst_width = dest_rect.right - dest_rect.left;
    UINT dst_height = dest_rect.bottom - dest_rect.top;
    
    // Copy/convert pixels
    for (UINT dst_y = 0; dst_y < dst_height; dst_y++) {
        for (UINT dst_x = 0; dst_x < dst_width; dst_x++) {
            // Calculate source coordinates (with scaling)
            UINT src_x = (dst_x * src_width) / dst_width;
            UINT src_y = (dst_y * src_height) / dst_height;
            
            // Bounds check
            if (src_x >= src_width || src_y >= src_height) continue;
            
            // Read source pixel
            const uint8_t* src_pixel_ptr = src + (src_rect.top + src_y) * SrcPitch + 
                                          (src_rect.left + src_x) * src_bpp;
            
            uint32_t src_pixel = 0;
            if (src_bpp == 4) {
                src_pixel = *reinterpret_cast<const uint32_t*>(src_pixel_ptr);
            } else if (src_bpp == 2) {
                src_pixel = *reinterpret_cast<const uint16_t*>(src_pixel_ptr);
            } else if (src_bpp == 3) {
                src_pixel = src_pixel_ptr[0] | (src_pixel_ptr[1] << 8) | (src_pixel_ptr[2] << 16);
            } else if (src_bpp == 1) {
                src_pixel = src_pixel_ptr[0];
            }
            
            // Check color key
            if (ColorKey && matches_color_key(src_pixel, ColorKey, SrcFormat)) {
                continue; // Skip transparent pixels
            }
            
            // Convert pixel format
            uint32_t dst_pixel = convert_pixel(src_pixel, SrcFormat, desc.Format);
            
            // Write destination pixel
            uint8_t* dst_pixel_ptr = dst + dst_y * locked_rect.Pitch + dst_x * dst_bpp;
            
            if (dst_bpp == 4) {
                *reinterpret_cast<uint32_t*>(dst_pixel_ptr) = dst_pixel;
            } else if (dst_bpp == 2) {
                *reinterpret_cast<uint16_t*>(dst_pixel_ptr) = static_cast<uint16_t>(dst_pixel);
            } else if (dst_bpp == 3) {
                dst_pixel_ptr[0] = static_cast<uint8_t>(dst_pixel);
                dst_pixel_ptr[1] = static_cast<uint8_t>(dst_pixel >> 8);
                dst_pixel_ptr[2] = static_cast<uint8_t>(dst_pixel >> 16);
            } else if (dst_bpp == 1) {
                dst_pixel_ptr[0] = static_cast<uint8_t>(dst_pixel);
            }
        }
    }
    
    pDestSurface->UnlockRect();
    
    DX8GL_INFO("D3DXLoadSurfaceFromMemory completed: %ux%u -> %ux%u", 
               src_width, src_height, dst_width, dst_height);
    
    return D3D_OK;
}

HRESULT WINAPI D3DXLoadSurfaceFromFile(
    IDirect3DSurface8* pDestSurface,
    const PALETTEENTRY* pDestPalette,
    const RECT* pDestRect,
    const char* pSrcFile,
    const RECT* pSrcRect,
    DWORD Filter,
    D3DCOLOR ColorKey,
    void* pSrcInfo)
{
    if (!pDestSurface || !pSrcFile) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_INFO("D3DXLoadSurfaceFromFile: %s", pSrcFile);
    
    // Read file
    std::ifstream file(pSrcFile, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        DX8GL_ERROR("Failed to open file: %s", pSrcFile);
        return D3DERR_NOTFOUND;
    }
    
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> file_data(file_size);
    file.read(reinterpret_cast<char*>(file_data.data()), file_size);
    file.close();
    
    // Detect image format
    ImageFormat img_format = detect_image_format(file_data.data(), file_size);
    
    std::vector<uint8_t> pixels;
    UINT img_width = 0, img_height = 0;
    D3DFORMAT img_format_d3d = D3DFMT_UNKNOWN;
    
    // Load image based on format
    bool loaded = false;
    switch (img_format) {
        case FORMAT_BMP:
            loaded = load_bmp(file_data.data(), file_size, pixels, img_width, img_height, img_format_d3d);
            break;
        case FORMAT_TGA:
            loaded = load_tga(file_data.data(), file_size, pixels, img_width, img_height, img_format_d3d);
            break;
        case FORMAT_DDS:
            DX8GL_WARNING("DDS format not yet supported");
            return D3DERR_NOTFOUND;
        case FORMAT_PNG:
            DX8GL_WARNING("PNG format not yet supported");
            return D3DERR_NOTFOUND;
        case FORMAT_JPG:
            DX8GL_WARNING("JPEG format not yet supported");
            return D3DERR_NOTFOUND;
        default:
            DX8GL_ERROR("Unknown image format");
            return D3DERR_NOTFOUND;
    }
    
    if (!loaded) {
        DX8GL_ERROR("Failed to load image: %s", pSrcFile);
        return D3DERR_NOTFOUND;
    }
    
    // Fill source info if requested
    if (pSrcInfo) {
        D3DXIMAGE_INFO* info = static_cast<D3DXIMAGE_INFO*>(pSrcInfo);
        info->Width = img_width;
        info->Height = img_height;
        info->Depth = 1;
        info->MipLevels = 1;
        info->Format = img_format_d3d;
        info->ResourceType = D3DRTYPE_SURFACE;
        info->ImageFileFormat = (img_format == FORMAT_BMP) ? D3DXIFF_BMP : D3DXIFF_TGA;
    }
    
    // Now load the decoded image data into the surface
    return D3DXLoadSurfaceFromMemory(pDestSurface, pDestPalette, pDestRect,
                                     pixels.data(), img_format_d3d, img_width * 4,
                                     nullptr, pSrcRect, Filter, ColorKey);
}

// Helper function to get bytes per pixel for a format
static UINT get_bytes_per_pixel(D3DFORMAT format) {
    switch (format) {
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
        // case D3DFMT_A8B8G8R8:  // Not available in D3D8
        // case D3DFMT_X8B8G8R8:  // Not available in D3D8
            return 4;
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_A4R4G4B4:
        case D3DFMT_X4R4G4B4:
            return 2;
        case D3DFMT_R8G8B8:
            return 3;
        case D3DFMT_A8:
        case D3DFMT_L8:
            return 1;
        default:
            return 4; // Default to 32-bit
    }
}

// Helper function to convert pixel from one format to another
static uint32_t convert_pixel(uint32_t src_pixel, D3DFORMAT src_format, D3DFORMAT dst_format) {
    // Extract ARGB components from source
    uint32_t src_a, src_r, src_g, src_b;
    
    switch (src_format) {
        case D3DFMT_A8R8G8B8:
            src_a = (src_pixel >> 24) & 0xFF;
            src_r = (src_pixel >> 16) & 0xFF;
            src_g = (src_pixel >> 8) & 0xFF;
            src_b = src_pixel & 0xFF;
            break;
        case D3DFMT_X8R8G8B8:
            src_a = 0xFF;
            src_r = (src_pixel >> 16) & 0xFF;
            src_g = (src_pixel >> 8) & 0xFF;
            src_b = src_pixel & 0xFF;
            break;
        case D3DFMT_R5G6B5:
            src_a = 0xFF;
            src_r = ((src_pixel >> 11) & 0x1F) * 255 / 31;
            src_g = ((src_pixel >> 5) & 0x3F) * 255 / 63;
            src_b = (src_pixel & 0x1F) * 255 / 31;
            break;
        default:
            // Default: assume ARGB8888
            src_a = (src_pixel >> 24) & 0xFF;
            src_r = (src_pixel >> 16) & 0xFF;
            src_g = (src_pixel >> 8) & 0xFF;
            src_b = src_pixel & 0xFF;
            break;
    }
    
    // Convert to destination format
    switch (dst_format) {
        case D3DFMT_A8R8G8B8:
            return (src_a << 24) | (src_r << 16) | (src_g << 8) | src_b;
        case D3DFMT_X8R8G8B8:
            return (0xFF << 24) | (src_r << 16) | (src_g << 8) | src_b;
        case D3DFMT_R5G6B5:
            return ((src_r * 31 / 255) << 11) | ((src_g * 63 / 255) << 5) | (src_b * 31 / 255);
        default:
            return (src_a << 24) | (src_r << 16) | (src_g << 8) | src_b;
    }
}

// Helper function to check if pixel matches color key
static bool matches_color_key(uint32_t pixel, D3DCOLOR color_key, D3DFORMAT format) {
    if (color_key == 0) return false; // No color key
    
    // Convert pixel to ARGB for comparison (ignoring alpha for color key)
    uint32_t pixel_rgb = convert_pixel(pixel, format, D3DFMT_X8R8G8B8) & 0x00FFFFFF;
    uint32_t key_rgb = color_key & 0x00FFFFFF;
    
    return pixel_rgb == key_rgb;
}


HRESULT WINAPI D3DXLoadSurfaceFromSurface(
    IDirect3DSurface8* pDestSurface,
    const PALETTEENTRY* pDestPalette,
    const RECT* pDestRect,
    IDirect3DSurface8* pSrcSurface,
    const PALETTEENTRY* pSrcPalette,
    const RECT* pSrcRect,
    DWORD Filter,
    D3DCOLOR ColorKey)
{
    if (!pDestSurface || !pSrcSurface) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_INFO("D3DXLoadSurfaceFromSurface");
    
    // Get surface descriptions
    D3DSURFACE_DESC src_desc, dst_desc;
    HRESULT hr = pSrcSurface->GetDesc(&src_desc);
    if (FAILED(hr)) return hr;
    
    hr = pDestSurface->GetDesc(&dst_desc);
    if (FAILED(hr)) return hr;
    
    // Lock both surfaces
    D3DLOCKED_RECT src_locked, dst_locked;
    hr = pSrcSurface->LockRect(&src_locked, pSrcRect, D3DLOCK_READONLY);
    if (FAILED(hr)) return hr;
    
    hr = pDestSurface->LockRect(&dst_locked, pDestRect, 0);
    if (FAILED(hr)) {
        pSrcSurface->UnlockRect();
        return hr;
    }
    
    // Simple copy for matching formats
    if (src_desc.Format == dst_desc.Format) {
        UINT width = std::min(src_desc.Width, dst_desc.Width);
        UINT height = std::min(src_desc.Height, dst_desc.Height);
        UINT bytes_per_pixel = 4;  // Assume 32-bit for now
        
        const uint8_t* src = static_cast<const uint8_t*>(src_locked.pBits);
        uint8_t* dst = static_cast<uint8_t*>(dst_locked.pBits);
        
        for (UINT y = 0; y < height; y++) {
            memcpy(dst + y * dst_locked.Pitch,
                   src + y * src_locked.Pitch,
                   width * bytes_per_pixel);
        }
    }
    
    pDestSurface->UnlockRect();
    pSrcSurface->UnlockRect();
    
    return D3D_OK;
}

// Helper function to write BMP file
static bool save_bmp(const char* filename, const uint8_t* pixels, UINT width, UINT height, D3DFORMAT format) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // BMP file header
    uint16_t bfType = 0x4D42; // "BM"
    uint32_t bfSize = 54 + width * height * 3; // File header + DIB header + pixel data
    uint16_t bfReserved1 = 0;
    uint16_t bfReserved2 = 0;
    uint32_t bfOffBits = 54;
    
    file.write(reinterpret_cast<const char*>(&bfType), 2);
    file.write(reinterpret_cast<const char*>(&bfSize), 4);
    file.write(reinterpret_cast<const char*>(&bfReserved1), 2);
    file.write(reinterpret_cast<const char*>(&bfReserved2), 2);
    file.write(reinterpret_cast<const char*>(&bfOffBits), 4);
    
    // DIB header (BITMAPINFOHEADER)
    uint32_t biSize = 40;
    int32_t biWidth = width;
    int32_t biHeight = height; // Positive = bottom-up
    uint16_t biPlanes = 1;
    uint16_t biBitCount = 24;
    uint32_t biCompression = 0; // BI_RGB
    uint32_t biSizeImage = 0;
    int32_t biXPelsPerMeter = 2835; // 72 DPI
    int32_t biYPelsPerMeter = 2835;
    uint32_t biClrUsed = 0;
    uint32_t biClrImportant = 0;
    
    file.write(reinterpret_cast<const char*>(&biSize), 4);
    file.write(reinterpret_cast<const char*>(&biWidth), 4);
    file.write(reinterpret_cast<const char*>(&biHeight), 4);
    file.write(reinterpret_cast<const char*>(&biPlanes), 2);
    file.write(reinterpret_cast<const char*>(&biBitCount), 2);
    file.write(reinterpret_cast<const char*>(&biCompression), 4);
    file.write(reinterpret_cast<const char*>(&biSizeImage), 4);
    file.write(reinterpret_cast<const char*>(&biXPelsPerMeter), 4);
    file.write(reinterpret_cast<const char*>(&biYPelsPerMeter), 4);
    file.write(reinterpret_cast<const char*>(&biClrUsed), 4);
    file.write(reinterpret_cast<const char*>(&biClrImportant), 4);
    
    // Write pixel data (bottom-up, BGR format)
    uint32_t row_stride = ((width * 3 + 3) / 4) * 4; // Align to 4 bytes
    std::vector<uint8_t> row_buffer(row_stride, 0);
    
    for (int y = height - 1; y >= 0; y--) {
        const uint8_t* src_row = pixels + y * width * 4;
        uint8_t* dst = row_buffer.data();
        
        for (UINT x = 0; x < width; x++) {
            // Convert ARGB to BGR
            dst[x * 3 + 0] = src_row[x * 4 + 0]; // B
            dst[x * 3 + 1] = src_row[x * 4 + 1]; // G
            dst[x * 3 + 2] = src_row[x * 4 + 2]; // R
        }
        
        file.write(reinterpret_cast<const char*>(row_buffer.data()), row_stride);
    }
    
    file.close();
    return true;
}

// Helper function to write TGA file
static bool save_tga(const char* filename, const uint8_t* pixels, UINT width, UINT height, D3DFORMAT format) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // TGA header
    uint8_t id_length = 0;
    uint8_t color_map_type = 0;
    uint8_t image_type = 2; // Uncompressed true-color
    uint16_t color_map_first = 0;
    uint16_t color_map_length = 0;
    uint8_t color_map_entry_size = 0;
    uint16_t x_origin = 0;
    uint16_t y_origin = 0;
    uint16_t tga_width = width;
    uint16_t tga_height = height;
    uint8_t pixel_depth = 32; // Always save as 32-bit
    uint8_t image_descriptor = 0x20; // Top-to-bottom
    
    file.write(reinterpret_cast<const char*>(&id_length), 1);
    file.write(reinterpret_cast<const char*>(&color_map_type), 1);
    file.write(reinterpret_cast<const char*>(&image_type), 1);
    file.write(reinterpret_cast<const char*>(&color_map_first), 2);
    file.write(reinterpret_cast<const char*>(&color_map_length), 2);
    file.write(reinterpret_cast<const char*>(&color_map_entry_size), 1);
    file.write(reinterpret_cast<const char*>(&x_origin), 2);
    file.write(reinterpret_cast<const char*>(&y_origin), 2);
    file.write(reinterpret_cast<const char*>(&tga_width), 2);
    file.write(reinterpret_cast<const char*>(&tga_height), 2);
    file.write(reinterpret_cast<const char*>(&pixel_depth), 1);
    file.write(reinterpret_cast<const char*>(&image_descriptor), 1);
    
    // Write pixel data (top-to-bottom, BGRA format)
    for (UINT y = 0; y < height; y++) {
        const uint8_t* src_row = pixels + y * width * 4;
        
        for (UINT x = 0; x < width; x++) {
            // Write as BGRA
            file.write(reinterpret_cast<const char*>(&src_row[x * 4 + 0]), 1); // B
            file.write(reinterpret_cast<const char*>(&src_row[x * 4 + 1]), 1); // G
            file.write(reinterpret_cast<const char*>(&src_row[x * 4 + 2]), 1); // R
            file.write(reinterpret_cast<const char*>(&src_row[x * 4 + 3]), 1); // A
        }
    }
    
    file.close();
    return true;
}

HRESULT WINAPI D3DXSaveSurfaceToFile(
    const char* pDestFile,
    DWORD DestFormat,
    IDirect3DSurface8* pSrcSurface,
    const PALETTEENTRY* pSrcPalette,
    const RECT* pSrcRect)
{
    if (!pDestFile || !pSrcSurface) {
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_INFO("D3DXSaveSurfaceToFile: %s, format=%d", pDestFile, DestFormat);
    
    // Get surface description
    D3DSURFACE_DESC desc;
    HRESULT hr = pSrcSurface->GetDesc(&desc);
    if (FAILED(hr)) {
        return hr;
    }
    
    // Determine source rectangle
    RECT src_rect = { 0, 0, static_cast<LONG>(desc.Width), static_cast<LONG>(desc.Height) };
    if (pSrcRect) {
        src_rect = *pSrcRect;
    }
    
    UINT width = src_rect.right - src_rect.left;
    UINT height = src_rect.bottom - src_rect.top;
    
    // Lock the surface
    D3DLOCKED_RECT locked_rect;
    hr = pSrcSurface->LockRect(&locked_rect, pSrcRect, D3DLOCK_READONLY);
    if (FAILED(hr)) {
        return hr;
    }
    
    // Allocate temporary buffer for pixel data (always convert to ARGB8888)
    std::vector<uint8_t> pixels(width * height * 4);
    
    // Copy and convert pixels to ARGB8888
    const uint8_t* src = static_cast<const uint8_t*>(locked_rect.pBits);
    uint8_t* dst = pixels.data();
    
    for (UINT y = 0; y < height; y++) {
        const uint8_t* src_row = src + y * locked_rect.Pitch;
        uint8_t* dst_row = dst + y * width * 4;
        
        for (UINT x = 0; x < width; x++) {
            uint32_t pixel = convert_pixel(
                *reinterpret_cast<const uint32_t*>(src_row + x * get_bytes_per_pixel(desc.Format)),
                desc.Format,
                D3DFMT_A8R8G8B8
            );
            
            // Store as BGRA (little-endian ARGB)
            dst_row[x * 4 + 0] = (pixel >> 0) & 0xFF;  // B
            dst_row[x * 4 + 1] = (pixel >> 8) & 0xFF;  // G
            dst_row[x * 4 + 2] = (pixel >> 16) & 0xFF; // R
            dst_row[x * 4 + 3] = (pixel >> 24) & 0xFF; // A
        }
    }
    
    pSrcSurface->UnlockRect();
    
    // Save based on format
    bool saved = false;
    D3DXIMAGE_FILEFORMAT format = static_cast<D3DXIMAGE_FILEFORMAT>(DestFormat);
    
    switch (format) {
        case D3DXIFF_BMP:
            saved = save_bmp(pDestFile, pixels.data(), width, height, D3DFMT_A8R8G8B8);
            break;
        case D3DXIFF_TGA:
            saved = save_tga(pDestFile, pixels.data(), width, height, D3DFMT_A8R8G8B8);
            break;
        case D3DXIFF_PNG:
            DX8GL_WARNING("PNG format not supported for saving");
            return D3DERR_INVALIDCALL;
        case D3DXIFF_JPG:
            DX8GL_WARNING("JPEG format not supported for saving");
            return D3DERR_INVALIDCALL;
        case D3DXIFF_DDS:
            DX8GL_WARNING("DDS format not supported for saving");
            return D3DERR_INVALIDCALL;
        default:
            DX8GL_ERROR("Unknown image format: %d", format);
            return D3DERR_INVALIDCALL;
    }
    
    if (!saved) {
        DX8GL_ERROR("Failed to save surface to file: %s", pDestFile);
        return D3DERR_INVALIDCALL;
    }
    
    DX8GL_INFO("Successfully saved surface to %s", pDestFile);
    return D3D_OK;
}

// ANSI versions - just aliases to the Unicode versions
HRESULT WINAPI D3DXCreateTextureFromFileExA(
    IDirect3DDevice8* pDevice,
    const char* pSrcFile,
    UINT Width,
    UINT Height,
    UINT MipLevels,
    DWORD Usage,
    D3DFORMAT Format,
    D3DPOOL Pool,
    DWORD Filter,
    DWORD MipFilter,
    D3DCOLOR ColorKey,
    void* pSrcInfo,
    void* pPalette,
    IDirect3DTexture8** ppTexture)
{
    return D3DXCreateTextureFromFileEx(pDevice, pSrcFile, Width, Height, MipLevels,
                                       Usage, Format, Pool, Filter, MipFilter,
                                       ColorKey, pSrcInfo, pPalette, ppTexture);
}

} // extern "C"