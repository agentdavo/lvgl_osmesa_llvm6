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
    
    // Read file
    std::ifstream file(pSrcFile, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return D3DERR_NOTFOUND;
    }
    
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> file_data(file_size);
    file.read(reinterpret_cast<char*>(file_data.data()), file_size);
    file.close();
    
    // For now, return not implemented
    // TODO: Implement D3DXLoadSurfaceFromMemory
    return E_NOTIMPL;
}

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
    
    DX8GL_INFO("D3DXLoadSurfaceFromMemory");
    
    // Get surface description
    D3DSURFACE_DESC desc;
    HRESULT hr = pDestSurface->GetDesc(&desc);
    if (FAILED(hr)) {
        return hr;
    }
    
    // Lock destination surface
    D3DLOCKED_RECT locked_rect;
    hr = pDestSurface->LockRect(&locked_rect, pDestRect, 0);
    if (FAILED(hr)) {
        return hr;
    }
    
    // Simple copy for now (assuming matching formats)
    if (SrcFormat == desc.Format && !pSrcRect && !pDestRect) {
        // Direct copy
        const uint8_t* src = static_cast<const uint8_t*>(pSrcMemory);
        uint8_t* dst = static_cast<uint8_t*>(locked_rect.pBits);
        
        UINT copy_pitch = std::min<UINT>(SrcPitch, locked_rect.Pitch);
        for (UINT y = 0; y < desc.Height; y++) {
            memcpy(dst + y * locked_rect.Pitch,
                   src + y * SrcPitch,
                   copy_pitch);
        }
    }
    
    pDestSurface->UnlockRect();
    return D3D_OK;
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
    
    DX8GL_WARNING("D3DXSaveSurfaceToFile not implemented");
    return E_NOTIMPL;
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