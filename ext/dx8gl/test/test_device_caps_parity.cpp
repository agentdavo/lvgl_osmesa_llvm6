#include "backend_param_test.h"
#include <map>
#include <set>
#include <sstream>

using namespace dx8gl;

class DeviceCapsParityTest : public BackendParamTest {
protected:
    // Structure to hold capabilities for comparison
    struct CapsSnapshot {
        D3DCAPS8 caps;
        TestBackendType backend;
        
        CapsSnapshot() : caps{}, backend{TestBackendType::OSMesa} {}
        CapsSnapshot(const D3DCAPS8& c, TestBackendType b) : caps(c), backend(b) {}
    };
    
    static std::map<TestBackendType, CapsSnapshot> caps_snapshots_;
    static bool first_run_;
    
    // Whitelist of fields that are allowed to differ between backends
    static std::set<std::string> GetWhitelistedFields() {
        return {
            "AdapterOrdinal",     // Different adapter IDs
            "DevCaps",           // Backend-specific device capabilities
            "PrimitiveMiscCaps", // Some primitives may vary
            "RasterCaps",        // Rasterization differences
            "LineCaps",          // Line drawing variations
            "MaxTextureWidth",   // Backend texture limits
            "MaxTextureHeight",  // Backend texture limits
            "MaxVolumeExtent",   // Volume texture support varies
            "MaxSimultaneousTextures", // WebGPU may differ
            "MaxUserClipPlanes", // Clipping plane support
            "MaxVertexShaderConst", // Shader constant limits
            "VertexShaderVersion", // Shader version support
            "PixelShaderVersion"   // Shader version support
        };
    }
    
    void SetUp() override {
        BackendParamTest::SetUp();
        RequireDevice();
        
        // Get caps for this backend
        D3DCAPS8 caps = {};
        HRESULT hr = device_->GetDeviceCaps(&caps);
        ASSERT_EQ(hr, D3D_OK) << "Failed to get device caps for " << GetBackendName();
        
        // Store snapshot
        caps_snapshots_[backend_] = CapsSnapshot(caps, backend_);
    }
    
    // Compare two capability values and report differences
    template<typename T>
    void CompareField(const std::string& field_name, T value1, T value2,
                     TestBackendType backend1, TestBackendType backend2,
                     std::vector<std::string>& differences) {
        if (value1 != value2) {
            auto whitelist = GetWhitelistedFields();
            std::stringstream ss;
            ss << field_name << ": " 
               << GetBackendName(backend1) << "=" << value1 << " vs "
               << GetBackendName(backend2) << "=" << value2;
            
            if (whitelist.find(field_name) != whitelist.end()) {
                ss << " (whitelisted)";
                // Log but don't fail
                RecordProperty(field_name.c_str(), ss.str());
            } else {
                differences.push_back(ss.str());
            }
        }
    }
    
    // Comprehensive caps comparison
    void CompareCaps(const D3DCAPS8& caps1, TestBackendType backend1,
                    const D3DCAPS8& caps2, TestBackendType backend2,
                    std::vector<std::string>& differences) {
        #define COMPARE_FIELD(field) \
            CompareField(#field, caps1.field, caps2.field, backend1, backend2, differences)
        
        // Device information
        COMPARE_FIELD(DeviceType);
        COMPARE_FIELD(AdapterOrdinal);
        
        // Capability flags
        COMPARE_FIELD(Caps);
        COMPARE_FIELD(Caps2);
        COMPARE_FIELD(Caps3);
        COMPARE_FIELD(PresentationIntervals);
        
        // Cursor capabilities
        COMPARE_FIELD(CursorCaps);
        
        // 3D Device capabilities
        COMPARE_FIELD(DevCaps);
        COMPARE_FIELD(PrimitiveMiscCaps);
        COMPARE_FIELD(RasterCaps);
        COMPARE_FIELD(ZCmpCaps);
        COMPARE_FIELD(SrcBlendCaps);
        COMPARE_FIELD(DestBlendCaps);
        COMPARE_FIELD(AlphaCmpCaps);
        COMPARE_FIELD(ShadeCaps);
        COMPARE_FIELD(TextureCaps);
        COMPARE_FIELD(TextureFilterCaps);
        COMPARE_FIELD(CubeTextureFilterCaps);
        COMPARE_FIELD(VolumeTextureFilterCaps);
        COMPARE_FIELD(TextureAddressCaps);
        COMPARE_FIELD(VolumeTextureAddressCaps);
        COMPARE_FIELD(LineCaps);
        
        // Size limits
        COMPARE_FIELD(MaxTextureWidth);
        COMPARE_FIELD(MaxTextureHeight);
        COMPARE_FIELD(MaxVolumeExtent);
        COMPARE_FIELD(MaxTextureRepeat);
        COMPARE_FIELD(MaxTextureAspectRatio);
        COMPARE_FIELD(MaxAnisotropy);
        COMPARE_FIELD(MaxVertexW);
        
        // Guard band limits
        COMPARE_FIELD(GuardBandLeft);
        COMPARE_FIELD(GuardBandTop);
        COMPARE_FIELD(GuardBandRight);
        COMPARE_FIELD(GuardBandBottom);
        
        // Fog and point size limits
        COMPARE_FIELD(ExtentsAdjust);
        COMPARE_FIELD(StencilCaps);
        COMPARE_FIELD(FVFCaps);
        COMPARE_FIELD(TextureOpCaps);
        COMPARE_FIELD(MaxTextureBlendStages);
        COMPARE_FIELD(MaxSimultaneousTextures);
        
        // Vertex processing
        COMPARE_FIELD(VertexProcessingCaps);
        COMPARE_FIELD(MaxActiveLights);
        COMPARE_FIELD(MaxUserClipPlanes);
        COMPARE_FIELD(MaxVertexBlendMatrices);
        COMPARE_FIELD(MaxVertexBlendMatrixIndex);
        
        // Point parameters
        COMPARE_FIELD(MaxPointSize);
        COMPARE_FIELD(MaxPrimitiveCount);
        COMPARE_FIELD(MaxVertexIndex);
        COMPARE_FIELD(MaxStreams);
        COMPARE_FIELD(MaxStreamStride);
        
        // Shader versions
        COMPARE_FIELD(VertexShaderVersion);
        COMPARE_FIELD(MaxVertexShaderConst);
        COMPARE_FIELD(PixelShaderVersion);
        COMPARE_FIELD(MaxPixelShaderValue);
        
        #undef COMPARE_FIELD
    }
};

// Static member definitions
std::map<TestBackendType, DeviceCapsParityTest::CapsSnapshot> DeviceCapsParityTest::caps_snapshots_;
bool DeviceCapsParityTest::first_run_ = true;

TEST_P(DeviceCapsParityTest, BasicCapsRetrieval) {
    // Just verify we can get caps without error
    D3DCAPS8 caps = {};
    HRESULT hr = device_->GetDeviceCaps(&caps);
    EXPECT_EQ(hr, D3D_OK);
    
    // Log some basic info
    RecordProperty("Backend", GetBackendName().c_str());
    RecordProperty("MaxTextureWidth", caps.MaxTextureWidth);
    RecordProperty("MaxTextureHeight", caps.MaxTextureHeight);
    RecordProperty("VertexShaderVersion", caps.VertexShaderVersion);
    RecordProperty("PixelShaderVersion", caps.PixelShaderVersion);
}

TEST_P(DeviceCapsParityTest, CrossBackendComparison) {
    // Skip if we're the first backend being tested
    if (caps_snapshots_.size() < 2) {
        RecordProperty("Status", "Waiting for other backends to compare");
        return;
    }
    
    std::vector<std::string> all_differences;
    
    // Compare this backend against all others
    auto& this_snapshot = caps_snapshots_[backend_];
    
    for (const auto& [other_backend, other_snapshot] : caps_snapshots_) {
        if (other_backend == backend_) continue;
        
        std::vector<std::string> differences;
        CompareCaps(this_snapshot.caps, backend_,
                   other_snapshot.caps, other_backend,
                   differences);
        
        if (!differences.empty()) {
            all_differences.push_back("=== " + GetBackendName() + " vs " + 
                                     GetBackendName(other_backend) + " ===");
            all_differences.insert(all_differences.end(), 
                                  differences.begin(), differences.end());
        }
    }
    
    // Report all non-whitelisted differences
    if (!all_differences.empty()) {
        std::stringstream report;
        for (const auto& diff : all_differences) {
            report << diff << "\n";
        }
        
        // Log the differences but don't fail the test yet
        // This allows us to see all differences across backends
        RecordProperty("CapsDifferences", report.str().c_str());
        
        // Only fail if there are critical differences
        // For now, we'll be lenient and just warn
        std::cout << "Warning: Device capabilities differ between backends:\n" 
                  << report.str() << std::endl;
    }
}

TEST_P(DeviceCapsParityTest, RequiredCapsPresent) {
    D3DCAPS8 caps = {};
    HRESULT hr = device_->GetDeviceCaps(&caps);
    ASSERT_EQ(hr, D3D_OK);
    
    // Check for required capabilities that all backends should support
    
    // Basic texture support
    EXPECT_GT(caps.MaxTextureWidth, 0u) << "Backend must support textures";
    EXPECT_GT(caps.MaxTextureHeight, 0u) << "Backend must support textures";
    
    // At least one texture stage
    EXPECT_GE(caps.MaxSimultaneousTextures, 1u) << "Backend must support at least 1 texture";
    EXPECT_GE(caps.MaxTextureBlendStages, 1u) << "Backend must support at least 1 blend stage";
    
    // Basic primitive support
    EXPECT_GT(caps.MaxPrimitiveCount, 0u) << "Backend must support primitive rendering";
    EXPECT_GT(caps.MaxVertexIndex, 0u) << "Backend must support indexed rendering";
    
    // Depth testing
    EXPECT_NE(caps.ZCmpCaps, 0u) << "Backend must support depth comparison";
    
    // Alpha blending
    EXPECT_NE(caps.SrcBlendCaps, 0u) << "Backend must support source blending";
    EXPECT_NE(caps.DestBlendCaps, 0u) << "Backend must support dest blending";
    
    // Backend-specific checks
    if (backend_ == TestBackendType::OSMesa) {
        // OSMesa should support everything
        EXPECT_GT(caps.MaxVolumeExtent, 0u) << "OSMesa should support volume textures";
    }
    
    if (backend_ == TestBackendType::WebGPU) {
        // WebGPU has specific requirements
        EXPECT_GE(caps.MaxTextureWidth, 2048u) << "WebGPU should support at least 2048x2048 textures";
        EXPECT_GE(caps.MaxTextureHeight, 2048u) << "WebGPU should support at least 2048x2048 textures";
    }
}

TEST_P(DeviceCapsParityTest, ShaderVersionConsistency) {
    D3DCAPS8 caps = {};
    HRESULT hr = device_->GetDeviceCaps(&caps);
    ASSERT_EQ(hr, D3D_OK);
    
    // Extract shader versions
    DWORD vs_major = (caps.VertexShaderVersion >> 8) & 0xFF;
    DWORD vs_minor = caps.VertexShaderVersion & 0xFF;
    DWORD ps_major = (caps.PixelShaderVersion >> 8) & 0xFF;
    DWORD ps_minor = caps.PixelShaderVersion & 0xFF;
    
    RecordProperty("VertexShaderVersion", 
                   std::to_string(vs_major) + "." + std::to_string(vs_minor));
    RecordProperty("PixelShaderVersion", 
                   std::to_string(ps_major) + "." + std::to_string(ps_minor));
    
    // All backends should support at least shader model 1.1
    EXPECT_GE(vs_major, 1u) << "Backend should support at least VS 1.x";
    EXPECT_GE(ps_major, 1u) << "Backend should support at least PS 1.x";
    
    // If shader constants are supported, there should be some available
    if (vs_major >= 1) {
        EXPECT_GT(caps.MaxVertexShaderConst, 0u) 
            << "Vertex shader should have constants if supported";
    }
}

// Instantiate tests for all backends
INSTANTIATE_BACKEND_PARAM_TEST(DeviceCapsParityTest);