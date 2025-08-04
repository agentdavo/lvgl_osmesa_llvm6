// HUD System for dx8gl demos
// Provides on-screen display of FPS, debug info, and controls

#ifndef DX8GL_HUD_SYSTEM_H
#define DX8GL_HUD_SYSTEM_H

#include "d3d8.h"
#include <string>
#include <vector>
#include <chrono>

namespace dx8gl {

// HUD display flags
enum HUDFlags {
    HUD_SHOW_FPS = 0x01,
    HUD_SHOW_DEBUG = 0x02,
    HUD_SHOW_CONTROLS = 0x04,
    HUD_SHOW_STATS = 0x08,
    HUD_SHOW_ALL = 0xFF
};

// HUD color scheme
struct HUDColors {
    D3DCOLOR background = D3DCOLOR_ARGB(180, 0, 0, 0);
    D3DCOLOR text = D3DCOLOR_ARGB(255, 255, 255, 255);
    D3DCOLOR fps_good = D3DCOLOR_ARGB(255, 0, 255, 0);
    D3DCOLOR fps_medium = D3DCOLOR_ARGB(255, 255, 255, 0);
    D3DCOLOR fps_bad = D3DCOLOR_ARGB(255, 255, 0, 0);
    D3DCOLOR header = D3DCOLOR_ARGB(255, 0, 255, 136);
    D3DCOLOR highlight = D3DCOLOR_ARGB(255, 255, 255, 0);
};

class HUDSystem {
public:
    HUDSystem(IDirect3DDevice8* device);
    ~HUDSystem();

    // Initialize HUD resources
    bool Initialize();
    
    // Update HUD data (call once per frame)
    void Update(float deltaTime);
    
    // Render HUD overlay
    void Render();
    
    // Toggle HUD elements
    void SetFlags(uint32_t flags) { m_flags = flags; }
    uint32_t GetFlags() const { return m_flags; }
    void ToggleFPS() { m_flags ^= HUD_SHOW_FPS; }
    void ToggleDebug() { m_flags ^= HUD_SHOW_DEBUG; }
    void ToggleControls() { m_flags ^= HUD_SHOW_CONTROLS; }
    void ToggleStats() { m_flags ^= HUD_SHOW_STATS; }
    
    // Set custom debug text
    void SetDebugText(const std::string& text) { m_debugText = text; }
    void AddDebugLine(const std::string& line);
    void ClearDebugLines() { m_debugLines.clear(); }
    
    // Set control instructions
    void SetControlText(const std::vector<std::string>& controls) { m_controlText = controls; }
    
    // Set statistics
    void SetStatValue(const std::string& name, const std::string& value);
    void ClearStats() { m_stats.clear(); }
    
    // Customize colors
    void SetColors(const HUDColors& colors) { m_colors = colors; }
    
private:
    IDirect3DDevice8* m_device;
    IDirect3DTexture8* m_fontTexture;
    
    uint32_t m_flags;
    HUDColors m_colors;
    
    // FPS tracking
    float m_fps;
    float m_frameTime;
    int m_frameCount;
    float m_fpsUpdateTimer;
    std::chrono::high_resolution_clock::time_point m_lastFrameTime;
    
    // Text content
    std::string m_debugText;
    std::vector<std::string> m_debugLines;
    std::vector<std::string> m_controlText;
    std::vector<std::pair<std::string, std::string>> m_stats;
    
    // Rendering helpers
    void RenderText(const std::string& text, int x, int y, D3DCOLOR color);
    void RenderBox(int x, int y, int width, int height, D3DCOLOR color);
    void CreateFontTexture();
    
    // Font metrics (assuming 8x8 bitmap font)
    static const int HUD_CHAR_WIDTH = 8;
    static const int HUD_CHAR_HEIGHT = 8;
    static const int HUD_CHAR_SPACING = 1;
    static const int HUD_LINE_HEIGHT = 10;
    static const int HUD_PADDING = 10;
};

// Global HUD instance helper
class HUD {
public:
    static void Create(IDirect3DDevice8* device);
    static void Destroy();
    static HUDSystem* Get() { return s_instance; }
    
private:
    static HUDSystem* s_instance;
};

} // namespace dx8gl

#endif // DX8GL_HUD_SYSTEM_H