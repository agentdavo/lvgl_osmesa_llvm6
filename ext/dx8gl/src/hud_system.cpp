// HUD System implementation for dx8gl demos
// Provides on-screen display of FPS, debug info, and controls

#include "hud_system.h"
#include "d3d8_device.h"
#include <cstring>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace dx8gl {

// Simple 8x8 bitmap font data (ASCII 32-126)
// Each character is 8 bytes, each byte represents one row
static const unsigned char g_fontData[] = {
    // Space (32)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // ! (33)
    0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00,
    // " (34)
    0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // # (35)
    0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00,
    // $ (36)
    0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00,
    // % (37)
    0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00,
    // & (38)
    0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00,
    // ' (39)
    0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
    // ( (40)
    0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00,
    // ) (41)
    0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00,
    // * (42)
    0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00,
    // + (43)
    0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00,
    // , (44)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x06,
    // - (45)
    0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00,
    // . (46)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00,
    // / (47)
    0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00,
    // 0-9 (48-57)
    0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00, // 0
    0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00, // 1
    0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00, // 2
    0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00, // 3
    0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00, // 4
    0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00, // 5
    0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00, // 6
    0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00, // 7
    0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00, // 8
    0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00, // 9
    // : (58)
    0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00,
    // ; (59)
    0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x06,
    // < (60)
    0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00,
    // = (61)
    0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00,
    // > (62)
    0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00,
    // ? (63)
    0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00,
    // @ (64)
    0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00,
    // A-Z (65-90)
    0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00, // A
    0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00, // B
    0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00, // C
    0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00, // D
    0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00, // E
    0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00, // F
    0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00, // G
    0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00, // H
    0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00, // I
    0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00, // J
    0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00, // K
    0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00, // L
    0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00, // M
    0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00, // N
    0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00, // O
    0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00, // P
    0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00, // Q
    0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00, // R
    0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00, // S
    0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00, // T
    0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00, // U
    0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00, // V
    0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00, // W
    0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00, // X
    0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00, // Y
    0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00, // Z
    // [ (91)
    0x1E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x00,
    // \ (92)
    0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00,
    // ] (93)
    0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00,
    // ^ (94)
    0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00,
    // _ (95)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
    // ` (96)
    0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
    // a-z (97-122)
    0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00, // a
    0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00, // b
    0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00, // c
    0x38, 0x30, 0x30, 0x3e, 0x33, 0x33, 0x6E, 0x00, // d
    0x00, 0x00, 0x1E, 0x33, 0x3f, 0x03, 0x1E, 0x00, // e
    0x1C, 0x36, 0x06, 0x0f, 0x06, 0x06, 0x0F, 0x00, // f
    0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x1F, // g
    0x07, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x67, 0x00, // h
    0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00, // i
    0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, // j
    0x07, 0x06, 0x66, 0x36, 0x1E, 0x36, 0x67, 0x00, // k
    0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00, // l
    0x00, 0x00, 0x33, 0x7F, 0x7F, 0x6B, 0x63, 0x00, // m
    0x00, 0x00, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x00, // n
    0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00, // o
    0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F, // p
    0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78, // q
    0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x0F, 0x00, // r
    0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00, // s
    0x08, 0x0C, 0x3E, 0x0C, 0x0C, 0x2C, 0x18, 0x00, // t
    0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00, // u
    0x00, 0x00, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00, // v
    0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00, // w
    0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00, // x
    0x00, 0x00, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x1F, // y
    0x00, 0x00, 0x3F, 0x19, 0x0C, 0x26, 0x3F, 0x00, // z
    // { (123)
    0x38, 0x0C, 0x0C, 0x07, 0x0C, 0x0C, 0x38, 0x00,
    // | (124)
    0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00,
    // } (125)
    0x07, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0x07, 0x00,
    // ~ (126)
    0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// Vertex structure for HUD rendering
struct HUDVertex {
    float x, y, z, rhw;
    D3DCOLOR color;
    float u, v;
};

#define D3DFVF_HUDVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)

// Static instance for global access
HUDSystem* HUD::s_instance = nullptr;

HUDSystem::HUDSystem(IDirect3DDevice8* device)
    : m_device(device)
    , m_fontTexture(nullptr)
    , m_flags(HUD_SHOW_FPS)
    , m_fps(0.0f)
    , m_frameTime(0.0f)
    , m_frameCount(0)
    , m_fpsUpdateTimer(0.0f)
    , m_lastFrameTime(std::chrono::high_resolution_clock::now())
{
    if (m_device) {
        m_device->AddRef();
    }
}

HUDSystem::~HUDSystem() {
    if (m_fontTexture) {
        m_fontTexture->Release();
    }
    if (m_device) {
        m_device->Release();
    }
}

bool HUDSystem::Initialize() {
    CreateFontTexture();
    
    // Set default control text
    std::vector<std::string> defaultControls = {
        "F1 - Toggle FPS",
        "F2 - Toggle Debug",
        "F3 - Toggle Controls",
        "F4 - Toggle Stats"
    };
    SetControlText(defaultControls);
    
    return true;
}

void HUDSystem::Update(float deltaTime) {
    // Update FPS counter
    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> frameDuration = currentTime - m_lastFrameTime;
    m_lastFrameTime = currentTime;
    
    m_frameTime = frameDuration.count();
    m_frameCount++;
    m_fpsUpdateTimer += m_frameTime;
    
    // Update FPS display every 0.5 seconds
    if (m_fpsUpdateTimer >= 0.5f) {
        m_fps = m_frameCount / m_fpsUpdateTimer;
        m_frameCount = 0;
        m_fpsUpdateTimer = 0.0f;
    }
    
    // Update device statistics if stats are being shown
    if (m_flags & HUD_SHOW_STATS) {
        UpdateDeviceStatistics();
    }
}

void HUDSystem::Render() {
    if (!m_device || !m_fontTexture) {
        return;
    }
    
    // Save render states
    DWORD oldZEnable, oldLighting, oldCullMode, oldAlphaBlendEnable;
    DWORD oldSrcBlend, oldDestBlend, oldAlphaTestEnable;
    DWORD oldColorOp, oldColorArg1, oldColorArg2;
    DWORD oldAlphaOp, oldAlphaArg1, oldAlphaArg2;
    IDirect3DBaseTexture8* oldTexture = nullptr;
    
    m_device->GetRenderState(D3DRS_ZENABLE, &oldZEnable);
    m_device->GetRenderState(D3DRS_LIGHTING, &oldLighting);
    m_device->GetRenderState(D3DRS_CULLMODE, &oldCullMode);
    m_device->GetRenderState(D3DRS_ALPHABLENDENABLE, &oldAlphaBlendEnable);
    m_device->GetRenderState(D3DRS_SRCBLEND, &oldSrcBlend);
    m_device->GetRenderState(D3DRS_DESTBLEND, &oldDestBlend);
    m_device->GetRenderState(D3DRS_ALPHATESTENABLE, &oldAlphaTestEnable);
    m_device->GetTextureStageState(0, D3DTSS_COLOROP, &oldColorOp);
    m_device->GetTextureStageState(0, D3DTSS_COLORARG1, &oldColorArg1);
    m_device->GetTextureStageState(0, D3DTSS_COLORARG2, &oldColorArg2);
    m_device->GetTextureStageState(0, D3DTSS_ALPHAOP, &oldAlphaOp);
    m_device->GetTextureStageState(0, D3DTSS_ALPHAARG1, &oldAlphaArg1);
    m_device->GetTextureStageState(0, D3DTSS_ALPHAARG2, &oldAlphaArg2);
    m_device->GetTexture(0, &oldTexture);
    
    // Set HUD render states
    m_device->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_device->SetRenderState(D3DRS_LIGHTING, FALSE);
    m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    m_device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    m_device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    m_device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    m_device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    m_device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    m_device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    
    m_device->SetTexture(0, m_fontTexture);
    m_device->SetVertexShader(D3DFVF_HUDVERTEX);
    
    int yOffset = HUD_PADDING;
    
    // Render FPS
    if (m_flags & HUD_SHOW_FPS) {
        std::stringstream ss;
        ss << "FPS: " << std::fixed << std::setprecision(1) << m_fps;
        ss << " (" << std::fixed << std::setprecision(2) << (m_frameTime * 1000.0f) << "ms)";
        
        D3DCOLOR fpsColor = m_colors.fps_good;
        if (m_fps < 30.0f) {
            fpsColor = m_colors.fps_bad;
        } else if (m_fps < 60.0f) {
            fpsColor = m_colors.fps_medium;
        }
        
        int textWidth = ss.str().length() * (HUD_CHAR_WIDTH + HUD_CHAR_SPACING);
        RenderBox(HUD_PADDING - 5, yOffset - 2, textWidth + 10, HUD_LINE_HEIGHT + 4, m_colors.background);
        RenderText(ss.str(), HUD_PADDING, yOffset, fpsColor);
        yOffset += HUD_LINE_HEIGHT + 5;
    }
    
    // Render Debug Info
    if (m_flags & HUD_SHOW_DEBUG && (!m_debugText.empty() || !m_debugLines.empty())) {
        RenderText("DEBUG INFO:", HUD_PADDING, yOffset, m_colors.header);
        yOffset += HUD_LINE_HEIGHT;
        
        if (!m_debugText.empty()) {
            RenderText(m_debugText, HUD_PADDING, yOffset, m_colors.text);
            yOffset += HUD_LINE_HEIGHT;
        }
        
        for (const auto& line : m_debugLines) {
            RenderText(line, HUD_PADDING, yOffset, m_colors.text);
            yOffset += HUD_LINE_HEIGHT;
        }
        yOffset += 5;
    }
    
    // Render Stats
    if (m_flags & HUD_SHOW_STATS && !m_stats.empty()) {
        RenderText("STATISTICS:", HUD_PADDING, yOffset, m_colors.header);
        yOffset += HUD_LINE_HEIGHT;
        
        for (const auto& stat : m_stats) {
            std::string statLine = stat.first + ": " + stat.second;
            RenderText(statLine, HUD_PADDING, yOffset, m_colors.text);
            yOffset += HUD_LINE_HEIGHT;
        }
        yOffset += 5;
    }
    
    // Render Controls (bottom right)
    if (m_flags & HUD_SHOW_CONTROLS && !m_controlText.empty()) {
        D3DVIEWPORT8 viewport;
        m_device->GetViewport(&viewport);
        
        int maxWidth = 0;
        for (const auto& control : m_controlText) {
            int width = control.length() * (HUD_CHAR_WIDTH + HUD_CHAR_SPACING);
            maxWidth = std::max(maxWidth, width);
        }
        
        int xPos = viewport.Width - maxWidth - HUD_PADDING;
        int yPos = viewport.Height - (m_controlText.size() * HUD_LINE_HEIGHT) - HUD_PADDING - HUD_LINE_HEIGHT;
        
        RenderBox(xPos - 5, yPos - 2, maxWidth + 10, 
                  (m_controlText.size() + 1) * HUD_LINE_HEIGHT + 4, m_colors.background);
        
        RenderText("CONTROLS:", xPos, yPos, m_colors.header);
        yPos += HUD_LINE_HEIGHT;
        
        for (const auto& control : m_controlText) {
            RenderText(control, xPos, yPos, m_colors.text);
            yPos += HUD_LINE_HEIGHT;
        }
    }
    
    // Restore render states
    m_device->SetRenderState(D3DRS_ZENABLE, oldZEnable);
    m_device->SetRenderState(D3DRS_LIGHTING, oldLighting);
    m_device->SetRenderState(D3DRS_CULLMODE, oldCullMode);
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, oldAlphaBlendEnable);
    m_device->SetRenderState(D3DRS_SRCBLEND, oldSrcBlend);
    m_device->SetRenderState(D3DRS_DESTBLEND, oldDestBlend);
    m_device->SetRenderState(D3DRS_ALPHATESTENABLE, oldAlphaTestEnable);
    m_device->SetTextureStageState(0, D3DTSS_COLOROP, oldColorOp);
    m_device->SetTextureStageState(0, D3DTSS_COLORARG1, oldColorArg1);
    m_device->SetTextureStageState(0, D3DTSS_COLORARG2, oldColorArg2);
    m_device->SetTextureStageState(0, D3DTSS_ALPHAOP, oldAlphaOp);
    m_device->SetTextureStageState(0, D3DTSS_ALPHAARG1, oldAlphaArg1);
    m_device->SetTextureStageState(0, D3DTSS_ALPHAARG2, oldAlphaArg2);
    
    if (oldTexture) {
        m_device->SetTexture(0, oldTexture);
        oldTexture->Release();
    } else {
        m_device->SetTexture(0, nullptr);
    }
}

void HUDSystem::AddDebugLine(const std::string& line) {
    m_debugLines.push_back(line);
    // Keep only last 10 lines
    if (m_debugLines.size() > 10) {
        m_debugLines.erase(m_debugLines.begin());
    }
}

void HUDSystem::SetStatValue(const std::string& name, const std::string& value) {
    auto it = std::find_if(m_stats.begin(), m_stats.end(),
        [&name](const auto& pair) { return pair.first == name; });
    
    if (it != m_stats.end()) {
        it->second = value;
    } else {
        m_stats.push_back({name, value});
    }
}

void HUDSystem::UpdateDeviceStatistics() {
    if (!m_device) return;
    
    // Cast to our Direct3DDevice8 implementation
    Direct3DDevice8* dx8Device = static_cast<Direct3DDevice8*>(m_device);
    
    // Get statistics from device
    SetStatValue("Matrix Changes", std::to_string(dx8Device->get_matrix_changes()));
    SetStatValue("Render State Changes", std::to_string(dx8Device->get_render_state_changes()));
    SetStatValue("Texture State Changes", std::to_string(dx8Device->get_texture_state_changes()));
    SetStatValue("Texture Changes", std::to_string(dx8Device->get_texture_changes()));
    SetStatValue("Draw Calls", std::to_string(dx8Device->get_draw_calls()));
    SetStatValue("Triangles", std::to_string(dx8Device->get_triangles_drawn()));
    SetStatValue("Vertices", std::to_string(dx8Device->get_vertices_processed()));
    SetStatValue("Clear Calls", std::to_string(dx8Device->get_clear_calls()));
    SetStatValue("Shader Changes", std::to_string(dx8Device->get_shader_changes()));
}

void HUDSystem::RenderText(const std::string& text, int x, int y, D3DCOLOR color) {
    for (size_t i = 0; i < text.length(); ++i) {
        int charIndex = (unsigned char)text[i] - 32;
        if (charIndex < 0 || charIndex >= 95) continue;
        
        float u1 = (charIndex % 16) * (1.0f / 16.0f);
        float v1 = (charIndex / 16) * (1.0f / 6.0f);
        float u2 = u1 + (1.0f / 16.0f);
        float v2 = v1 + (1.0f / 6.0f);
        
        HUDVertex vertices[4];
        vertices[0] = { (float)x, (float)y, 0.5f, 1.0f, color, u1, v1 };
        vertices[1] = { (float)(x + HUD_CHAR_WIDTH), (float)y, 0.5f, 1.0f, color, u2, v1 };
        vertices[2] = { (float)x, (float)(y + HUD_CHAR_HEIGHT), 0.5f, 1.0f, color, u1, v2 };
        vertices[3] = { (float)(x + HUD_CHAR_WIDTH), (float)(y + HUD_CHAR_HEIGHT), 0.5f, 1.0f, color, u2, v2 };
        
        // Use DrawPrimitiveUP to go through the command buffer system
        m_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(HUDVertex));
        
        x += HUD_CHAR_WIDTH + HUD_CHAR_SPACING;
    }
}

void HUDSystem::RenderBox(int x, int y, int width, int height, D3DCOLOR color) {
    m_device->SetTexture(0, nullptr);
    
    HUDVertex vertices[4];
    vertices[0] = { (float)x, (float)y, 0.5f, 1.0f, color, 0, 0 };
    vertices[1] = { (float)(x + width), (float)y, 0.5f, 1.0f, color, 0, 0 };
    vertices[2] = { (float)x, (float)(y + height), 0.5f, 1.0f, color, 0, 0 };
    vertices[3] = { (float)(x + width), (float)(y + height), 0.5f, 1.0f, color, 0, 0 };
    
    // Use DrawPrimitiveUP to go through the command buffer system
    m_device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(HUDVertex));
    
    m_device->SetTexture(0, m_fontTexture);
}

void HUDSystem::CreateFontTexture() {
    if (m_fontTexture) {
        return;
    }
    
    // Create a 128x48 texture (16x6 characters)
    if (FAILED(m_device->CreateTexture(128, 48, 1, 0, D3DFMT_A8R8G8B8, 
                                       D3DPOOL_MANAGED, &m_fontTexture))) {
        return;
    }
    
    D3DLOCKED_RECT lockedRect;
    if (SUCCEEDED(m_fontTexture->LockRect(0, &lockedRect, nullptr, 0))) {
        unsigned char* pDest = (unsigned char*)lockedRect.pBits;
        
        // Clear texture
        memset(pDest, 0, 128 * 48 * 4);
        
        // Render font characters
        for (int charIndex = 0; charIndex < 95; ++charIndex) {
            int charX = (charIndex % 16) * 8;
            int charY = (charIndex / 16) * 8;
            
            for (int y = 0; y < 8; ++y) {
                unsigned char rowData = g_fontData[charIndex * 8 + y];
                for (int x = 0; x < 8; ++x) {
                    if (rowData & (1 << (7 - x))) {
                        int pixelOffset = ((charY + y) * lockedRect.Pitch) + ((charX + x) * 4);
                        pDest[pixelOffset + 0] = 255; // B
                        pDest[pixelOffset + 1] = 255; // G
                        pDest[pixelOffset + 2] = 255; // R
                        pDest[pixelOffset + 3] = 255; // A
                    }
                }
            }
        }
        
        m_fontTexture->UnlockRect(0);
    }
}


// Global HUD helper implementation
void HUD::Create(IDirect3DDevice8* device) {
    if (!s_instance) {
        s_instance = new HUDSystem(device);
        s_instance->Initialize();
    }
}

void HUD::Destroy() {
    delete s_instance;
    s_instance = nullptr;
}

} // namespace dx8gl