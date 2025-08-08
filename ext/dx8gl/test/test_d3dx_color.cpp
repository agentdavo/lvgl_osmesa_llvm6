#include <gtest/gtest.h>
#include "d3dx_compat.h"
#include <cmath>

class D3DXColorTest : public ::testing::Test {
protected:
    // Helper to check if two colors are approximately equal
    bool ColorNear(const D3DXCOLOR& a, const D3DXCOLOR& b, float epsilon = 1e-5f) {
        return std::abs(a.r - b.r) < epsilon &&
               std::abs(a.g - b.g) < epsilon &&
               std::abs(a.b - b.b) < epsilon &&
               std::abs(a.a - b.a) < epsilon;
    }
};

TEST_F(D3DXColorTest, ColorAdjustSaturation) {
    D3DXCOLOR color, result;
    
    // Test full saturation (s=1, no change)
    color.r = 1.0f; color.g = 0.5f; color.b = 0.0f; color.a = 1.0f;
    D3DXColorAdjustSaturation(&result, &color, 1.0f);
    EXPECT_TRUE(ColorNear(result, color));
    
    // Test zero saturation (s=0, should be grayscale)
    color.r = 1.0f; color.g = 0.0f; color.b = 0.0f; color.a = 1.0f;
    D3DXColorAdjustSaturation(&result, &color, 0.0f);
    
    // Pure red should become gray based on luminance weights
    float expectedGray = 0.2125f;  // Red luminance weight
    EXPECT_NEAR(result.r, expectedGray, 1e-3f);
    EXPECT_NEAR(result.g, expectedGray, 1e-3f);
    EXPECT_NEAR(result.b, expectedGray, 1e-3f);
    EXPECT_FLOAT_EQ(result.a, 1.0f);
    
    // Test half saturation
    color.r = 1.0f; color.g = 0.5f; color.b = 0.0f; color.a = 0.5f;
    D3DXColorAdjustSaturation(&result, &color, 0.5f);
    
    // Should be halfway between original and grayscale
    float gray = 1.0f * 0.2125f + 0.5f * 0.7154f + 0.0f * 0.0721f;
    D3DXCOLOR expected;
    expected.r = gray + 0.5f * (1.0f - gray);
    expected.g = gray + 0.5f * (0.5f - gray);
    expected.b = gray + 0.5f * (0.0f - gray);
    expected.a = 0.5f;
    
    EXPECT_TRUE(ColorNear(result, expected, 1e-3f));
    
    // Test oversaturation (s=2, should increase saturation)
    color.r = 0.8f; color.g = 0.6f; color.b = 0.4f; color.a = 1.0f;
    D3DXColorAdjustSaturation(&result, &color, 2.0f);
    
    // Result should be clamped to [0, 1]
    EXPECT_GE(result.r, 0.0f);
    EXPECT_LE(result.r, 1.0f);
    EXPECT_GE(result.g, 0.0f);
    EXPECT_LE(result.g, 1.0f);
    EXPECT_GE(result.b, 0.0f);
    EXPECT_LE(result.b, 1.0f);
}

TEST_F(D3DXColorTest, ColorAdjustContrast) {
    D3DXCOLOR color, result;
    
    // Test normal contrast (c=1, no change)
    color.r = 0.3f; color.g = 0.5f; color.b = 0.7f; color.a = 1.0f;
    D3DXColorAdjustContrast(&result, &color, 1.0f);
    EXPECT_TRUE(ColorNear(result, color));
    
    // Test zero contrast (c=0, all should be 0.5)
    D3DXColorAdjustContrast(&result, &color, 0.0f);
    EXPECT_FLOAT_EQ(result.r, 0.5f);
    EXPECT_FLOAT_EQ(result.g, 0.5f);
    EXPECT_FLOAT_EQ(result.b, 0.5f);
    EXPECT_FLOAT_EQ(result.a, 1.0f);
    
    // Test increased contrast (c=2)
    color.r = 0.6f; color.g = 0.4f; color.b = 0.5f; color.a = 0.8f;
    D3DXColorAdjustContrast(&result, &color, 2.0f);
    
    // Formula: 0.5 + 2 * (value - 0.5)
    EXPECT_FLOAT_EQ(result.r, 0.7f);  // 0.5 + 2*(0.6-0.5) = 0.7
    EXPECT_FLOAT_EQ(result.g, 0.3f);  // 0.5 + 2*(0.4-0.5) = 0.3
    EXPECT_FLOAT_EQ(result.b, 0.5f);  // 0.5 + 2*(0.5-0.5) = 0.5
    EXPECT_FLOAT_EQ(result.a, 0.8f);  // Alpha unchanged
    
    // Test extreme contrast with clamping
    color.r = 0.8f; color.g = 0.2f; color.b = 0.5f; color.a = 1.0f;
    D3DXColorAdjustContrast(&result, &color, 10.0f);
    
    // Should be clamped to [0, 1]
    EXPECT_FLOAT_EQ(result.r, 1.0f);  // 0.5 + 10*(0.8-0.5) = 3.5 → clamped to 1
    EXPECT_FLOAT_EQ(result.g, 0.0f);  // 0.5 + 10*(0.2-0.5) = -2.5 → clamped to 0
    EXPECT_FLOAT_EQ(result.b, 0.5f);  // 0.5 + 10*(0.5-0.5) = 0.5
}

TEST_F(D3DXColorTest, ColorLerp) {
    D3DXCOLOR c1, c2, result;
    
    // Test t=0 (should return c1)
    c1.r = 0.2f; c1.g = 0.4f; c1.b = 0.6f; c1.a = 0.8f;
    c2.r = 0.8f; c2.g = 0.6f; c2.b = 0.4f; c2.a = 0.2f;
    
    D3DXColorLerp(&result, &c1, &c2, 0.0f);
    EXPECT_TRUE(ColorNear(result, c1));
    
    // Test t=1 (should return c2)
    D3DXColorLerp(&result, &c1, &c2, 1.0f);
    EXPECT_TRUE(ColorNear(result, c2));
    
    // Test t=0.5 (halfway between)
    D3DXColorLerp(&result, &c1, &c2, 0.5f);
    EXPECT_FLOAT_EQ(result.r, 0.5f);
    EXPECT_FLOAT_EQ(result.g, 0.5f);
    EXPECT_FLOAT_EQ(result.b, 0.5f);
    EXPECT_FLOAT_EQ(result.a, 0.5f);
    
    // Test t=0.25
    D3DXColorLerp(&result, &c1, &c2, 0.25f);
    EXPECT_FLOAT_EQ(result.r, 0.35f);  // 0.2 + 0.25*(0.8-0.2)
    EXPECT_FLOAT_EQ(result.g, 0.45f);  // 0.4 + 0.25*(0.6-0.4)
    EXPECT_FLOAT_EQ(result.b, 0.55f);  // 0.6 + 0.25*(0.4-0.6)
    EXPECT_FLOAT_EQ(result.a, 0.65f);  // 0.8 + 0.25*(0.2-0.8)
    
    // Test extrapolation (t > 1)
    D3DXColorLerp(&result, &c1, &c2, 1.5f);
    EXPECT_FLOAT_EQ(result.r, 1.1f);   // 0.2 + 1.5*(0.8-0.2) = 1.1
    EXPECT_FLOAT_EQ(result.g, 0.7f);   // 0.4 + 1.5*(0.6-0.4) = 0.7
    EXPECT_FLOAT_EQ(result.b, 0.3f);   // 0.6 + 1.5*(0.4-0.6) = 0.3
    EXPECT_FLOAT_EQ(result.a, -0.1f);  // 0.8 + 1.5*(0.2-0.8) = -0.1
}

TEST_F(D3DXColorTest, ColorModulate) {
    D3DXCOLOR c1, c2, result;
    
    // Test basic multiplication
    c1.r = 0.5f; c1.g = 0.8f; c1.b = 1.0f; c1.a = 0.9f;
    c2.r = 0.4f; c2.g = 0.5f; c2.b = 0.6f; c2.a = 0.7f;
    
    D3DXColorModulate(&result, &c1, &c2);
    EXPECT_FLOAT_EQ(result.r, 0.2f);   // 0.5 * 0.4
    EXPECT_FLOAT_EQ(result.g, 0.4f);   // 0.8 * 0.5
    EXPECT_FLOAT_EQ(result.b, 0.6f);   // 1.0 * 0.6
    EXPECT_FLOAT_EQ(result.a, 0.63f);  // 0.9 * 0.7
    
    // Test with zeros
    c1.r = 0.0f; c1.g = 0.5f; c1.b = 1.0f; c1.a = 1.0f;
    c2.r = 1.0f; c2.g = 0.0f; c2.b = 0.5f; c2.a = 0.0f;
    
    D3DXColorModulate(&result, &c1, &c2);
    EXPECT_FLOAT_EQ(result.r, 0.0f);
    EXPECT_FLOAT_EQ(result.g, 0.0f);
    EXPECT_FLOAT_EQ(result.b, 0.5f);
    EXPECT_FLOAT_EQ(result.a, 0.0f);
    
    // Test identity (multiply by white)
    c1.r = 0.3f; c1.g = 0.6f; c1.b = 0.9f; c1.a = 0.5f;
    c2.r = 1.0f; c2.g = 1.0f; c2.b = 1.0f; c2.a = 1.0f;
    
    D3DXColorModulate(&result, &c1, &c2);
    EXPECT_TRUE(ColorNear(result, c1));
}

TEST_F(D3DXColorTest, ColorNegative) {
    D3DXCOLOR color, result;
    
    // Test basic inversion
    color.r = 0.3f; color.g = 0.7f; color.b = 0.0f; color.a = 0.8f;
    D3DXColorNegative(&result, &color);
    
    EXPECT_FLOAT_EQ(result.r, 0.7f);  // 1 - 0.3
    EXPECT_FLOAT_EQ(result.g, 0.3f);  // 1 - 0.7
    EXPECT_FLOAT_EQ(result.b, 1.0f);  // 1 - 0
    EXPECT_FLOAT_EQ(result.a, 0.8f);  // Alpha unchanged
    
    // Test black to white
    color.r = 0.0f; color.g = 0.0f; color.b = 0.0f; color.a = 1.0f;
    D3DXColorNegative(&result, &color);
    
    EXPECT_FLOAT_EQ(result.r, 1.0f);
    EXPECT_FLOAT_EQ(result.g, 1.0f);
    EXPECT_FLOAT_EQ(result.b, 1.0f);
    EXPECT_FLOAT_EQ(result.a, 1.0f);
    
    // Test white to black
    color.r = 1.0f; color.g = 1.0f; color.b = 1.0f; color.a = 0.5f;
    D3DXColorNegative(&result, &color);
    
    EXPECT_FLOAT_EQ(result.r, 0.0f);
    EXPECT_FLOAT_EQ(result.g, 0.0f);
    EXPECT_FLOAT_EQ(result.b, 0.0f);
    EXPECT_FLOAT_EQ(result.a, 0.5f);
    
    // Test middle gray
    color.r = 0.5f; color.g = 0.5f; color.b = 0.5f; color.a = 0.2f;
    D3DXColorNegative(&result, &color);
    
    EXPECT_FLOAT_EQ(result.r, 0.5f);
    EXPECT_FLOAT_EQ(result.g, 0.5f);
    EXPECT_FLOAT_EQ(result.b, 0.5f);
    EXPECT_FLOAT_EQ(result.a, 0.2f);
}

TEST_F(D3DXColorTest, ColorScale) {
    D3DXCOLOR color, result;
    
    // Test scale by 1 (no change)
    color.r = 0.2f; color.g = 0.4f; color.b = 0.6f; color.a = 0.8f;
    D3DXColorScale(&result, &color, 1.0f);
    EXPECT_TRUE(ColorNear(result, color));
    
    // Test scale by 0 (all zeros)
    D3DXColorScale(&result, &color, 0.0f);
    EXPECT_FLOAT_EQ(result.r, 0.0f);
    EXPECT_FLOAT_EQ(result.g, 0.0f);
    EXPECT_FLOAT_EQ(result.b, 0.0f);
    EXPECT_FLOAT_EQ(result.a, 0.0f);
    
    // Test scale by 2
    color.r = 0.3f; color.g = 0.4f; color.b = 0.5f; color.a = 0.25f;
    D3DXColorScale(&result, &color, 2.0f);
    
    EXPECT_FLOAT_EQ(result.r, 0.6f);
    EXPECT_FLOAT_EQ(result.g, 0.8f);
    EXPECT_FLOAT_EQ(result.b, 1.0f);
    EXPECT_FLOAT_EQ(result.a, 0.5f);
    
    // Test scale by 0.5
    color.r = 0.8f; color.g = 0.6f; color.b = 0.4f; color.a = 1.0f;
    D3DXColorScale(&result, &color, 0.5f);
    
    EXPECT_FLOAT_EQ(result.r, 0.4f);
    EXPECT_FLOAT_EQ(result.g, 0.3f);
    EXPECT_FLOAT_EQ(result.b, 0.2f);
    EXPECT_FLOAT_EQ(result.a, 0.5f);
    
    // Test HDR scale (no clamping)
    color.r = 0.5f; color.g = 0.5f; color.b = 0.5f; color.a = 0.5f;
    D3DXColorScale(&result, &color, 3.0f);
    
    EXPECT_FLOAT_EQ(result.r, 1.5f);  // Not clamped
    EXPECT_FLOAT_EQ(result.g, 1.5f);
    EXPECT_FLOAT_EQ(result.b, 1.5f);
    EXPECT_FLOAT_EQ(result.a, 1.5f);
    
    // Test negative scale
    color.r = 0.3f; color.g = 0.6f; color.b = 0.9f; color.a = 0.5f;
    D3DXColorScale(&result, &color, -1.0f);
    
    EXPECT_FLOAT_EQ(result.r, -0.3f);
    EXPECT_FLOAT_EQ(result.g, -0.6f);
    EXPECT_FLOAT_EQ(result.b, -0.9f);
    EXPECT_FLOAT_EQ(result.a, -0.5f);
}

TEST_F(D3DXColorTest, EdgeCases) {
    D3DXCOLOR color, result;
    
    // Test saturation with pure white
    color.r = 1.0f; color.g = 1.0f; color.b = 1.0f; color.a = 1.0f;
    D3DXColorAdjustSaturation(&result, &color, 0.0f);
    
    // White to grayscale should stay white (luminance = 1)
    EXPECT_NEAR(result.r, 1.0f, 1e-3f);
    EXPECT_NEAR(result.g, 1.0f, 1e-3f);
    EXPECT_NEAR(result.b, 1.0f, 1e-3f);
    
    // Test saturation with pure black
    color.r = 0.0f; color.g = 0.0f; color.b = 0.0f; color.a = 1.0f;
    D3DXColorAdjustSaturation(&result, &color, 2.0f);
    
    // Black should stay black regardless of saturation
    EXPECT_FLOAT_EQ(result.r, 0.0f);
    EXPECT_FLOAT_EQ(result.g, 0.0f);
    EXPECT_FLOAT_EQ(result.b, 0.0f);
    
    // Test contrast with values at midpoint
    color.r = 0.5f; color.g = 0.5f; color.b = 0.5f; color.a = 0.5f;
    D3DXColorAdjustContrast(&result, &color, 100.0f);
    
    // Values at midpoint should stay at midpoint
    EXPECT_FLOAT_EQ(result.r, 0.5f);
    EXPECT_FLOAT_EQ(result.g, 0.5f);
    EXPECT_FLOAT_EQ(result.b, 0.5f);
}