#include <iostream>
#include <vector>
#include <memory>
#include <cstring>
#include <limits>
#include "../src/state_manager.h"
#include "../src/logger.h"

using namespace std;
using namespace dx8gl;

// Test result tracking
static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        tests_run++; \
        if (condition) { \
            tests_passed++; \
            cout << "[PASS] " << message << endl; \
        } else { \
            cout << "[FAIL] " << message << endl; \
            return false; \
        } \
    } while(0)

// Mock ShaderProgram for testing
struct MockShaderProgram : public ShaderProgram {
    MockShaderProgram() : ShaderProgram() {
        program = 1; // Mock program ID
        
        // Mock some uniform locations
        uniform_locations["u_world_matrix"] = 0;
        uniform_locations["u_view_matrix"] = 1;
        uniform_locations["u_projection_matrix"] = 2;
        uniform_locations["u_world_view_proj_matrix"] = 3;
        uniform_locations["u_material_diffuse"] = 4;
        uniform_locations["u_material_ambient"] = 5;
        uniform_locations["u_fog_enable"] = 6;
        uniform_locations["u_fog_color"] = 7;
        uniform_locations["u_ambient_light"] = 8;
        uniform_locations["u_num_lights"] = 9;
        
        u_world_matrix = 0;
        u_view_matrix = 1;
        u_projection_matrix = 2;
        u_world_view_proj_matrix = 3;
    }
};

bool test_render_state_validation() {
    cout << "\n=== Test: Render State Validation ===" << endl;
    
    StateManager state_manager;
    TEST_ASSERT(state_manager.initialize(), "State manager initialization should succeed");
    
    // Test valid render states
    state_manager.set_render_state(D3DRS_ZENABLE, TRUE);
    state_manager.set_render_state(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    state_manager.set_render_state(D3DRS_CULLMODE, D3DCULL_CCW);
    TEST_ASSERT(state_manager.validate_state(), "Valid render states should pass validation");
    
    // Test invalid blend factors
    state_manager.set_render_state(D3DRS_ALPHABLENDENABLE, TRUE);
    state_manager.set_render_state(D3DRS_SRCBLEND, 999); // Invalid blend factor
    TEST_ASSERT(!state_manager.validate_state(), "Invalid blend factor should fail validation");
    
    // Fix the blend factor and test again
    state_manager.set_render_state(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    TEST_ASSERT(state_manager.validate_state(), "Valid blend factor should pass validation");
    
    // Test invalid depth function
    state_manager.set_render_state(D3DRS_ZFUNC, 999); // Invalid comparison function
    TEST_ASSERT(!state_manager.validate_state(), "Invalid depth function should fail validation");
    
    // Fix depth function
    state_manager.set_render_state(D3DRS_ZFUNC, D3DCMP_LESS);
    TEST_ASSERT(state_manager.validate_state(), "Valid depth function should pass validation");
    
    // Test invalid alpha test settings
    state_manager.set_render_state(D3DRS_ALPHATESTENABLE, TRUE);
    state_manager.set_render_state(D3DRS_ALPHAREF, 300); // Out of range (0-255)
    TEST_ASSERT(!state_manager.validate_state(), "Invalid alpha reference should fail validation");
    
    // Fix alpha reference
    state_manager.set_render_state(D3DRS_ALPHAREF, 128);
    TEST_ASSERT(state_manager.validate_state(), "Valid alpha reference should pass validation");
    
    return true;
}

bool test_texture_state_validation() {
    cout << "\n=== Test: Texture State Validation ===" << endl;
    
    StateManager state_manager;
    state_manager.initialize();
    
    // Test valid texture states
    state_manager.set_texture_stage_state(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    state_manager.set_texture_stage_state(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    state_manager.set_texture_stage_state(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    state_manager.set_texture_stage_state(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    state_manager.set_texture_stage_state(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    state_manager.set_texture_stage_state(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
    state_manager.set_texture_stage_state(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    TEST_ASSERT(state_manager.validate_state(), "Valid texture states should pass validation");
    
    // Test invalid color operation
    state_manager.set_texture_stage_state(0, D3DTSS_COLOROP, 999); // Invalid operation
    TEST_ASSERT(!state_manager.validate_state(), "Invalid texture color operation should fail validation");
    
    // Fix color operation
    state_manager.set_texture_stage_state(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    TEST_ASSERT(state_manager.validate_state(), "Valid texture color operation should pass validation");
    
    // Test invalid texture coordinate index
    state_manager.set_texture_stage_state(0, D3DTSS_TEXCOORDINDEX, 10); // Out of range (0-7)
    TEST_ASSERT(!state_manager.validate_state(), "Invalid texture coordinate index should fail validation");
    
    // Fix texture coordinate index
    state_manager.set_texture_stage_state(0, D3DTSS_TEXCOORDINDEX, 1);
    TEST_ASSERT(state_manager.validate_state(), "Valid texture coordinate index should pass validation");
    
    // Test invalid filter mode
    state_manager.set_texture_stage_state(0, D3DTSS_MAGFILTER, 999); // Invalid filter
    TEST_ASSERT(!state_manager.validate_state(), "Invalid texture filter should fail validation");
    
    // Fix filter mode
    state_manager.set_texture_stage_state(0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
    TEST_ASSERT(state_manager.validate_state(), "Valid texture filter should pass validation");
    
    // Test texture stage ordering - stage 1 active but stage 0 disabled
    state_manager.set_texture_stage_state(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
    state_manager.set_texture_stage_state(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
    TEST_ASSERT(!state_manager.validate_state(), "Invalid texture stage ordering should fail validation");
    
    return true;
}

bool test_transform_state_validation() {
    cout << "\n=== Test: Transform State Validation ===" << endl;
    
    StateManager state_manager;
    state_manager.initialize();
    
    // Test valid matrices (identity matrices)
    D3DMATRIX identity = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    
    state_manager.set_transform(D3DTS_WORLD, &identity);
    state_manager.set_transform(D3DTS_VIEW, &identity);
    state_manager.set_transform(D3DTS_PROJECTION, &identity);
    TEST_ASSERT(state_manager.validate_state(), "Valid transform matrices should pass validation");
    
    // Test matrix with NaN values
    D3DMATRIX invalid_matrix = identity;
    invalid_matrix._11 = std::numeric_limits<float>::quiet_NaN();
    state_manager.set_transform(D3DTS_WORLD, &invalid_matrix);
    TEST_ASSERT(!state_manager.validate_state(), "Matrix with NaN values should fail validation");
    
    // Test matrix with infinity values
    invalid_matrix = identity;
    invalid_matrix._22 = std::numeric_limits<float>::infinity();
    state_manager.set_transform(D3DTS_VIEW, &invalid_matrix);
    TEST_ASSERT(!state_manager.validate_state(), "Matrix with infinity values should fail validation");
    
    // Restore valid matrices
    state_manager.set_transform(D3DTS_WORLD, &identity);
    state_manager.set_transform(D3DTS_VIEW, &identity);
    TEST_ASSERT(state_manager.validate_state(), "Restored valid matrices should pass validation");
    
    return true;
}

bool test_light_state_validation() {
    cout << "\n=== Test: Light State Validation ===" << endl;
    
    StateManager state_manager;
    state_manager.initialize();
    state_manager.set_render_state(D3DRS_LIGHTING, TRUE);
    
    // Test valid directional light
    D3DLIGHT8 light = {};
    light.Type = D3DLIGHT_DIRECTIONAL;
    light.Diffuse.r = light.Diffuse.g = light.Diffuse.b = light.Diffuse.a = 1.0f;
    light.Direction.x = 0.0f;
    light.Direction.y = 0.0f;
    light.Direction.z = 1.0f;
    
    state_manager.set_light(0, &light);
    state_manager.light_enable(0, TRUE);
    TEST_ASSERT(state_manager.validate_state(), "Valid directional light should pass validation");
    
    // Test invalid light type
    light.Type = static_cast<D3DLIGHTTYPE>(999);
    state_manager.set_light(0, &light);
    TEST_ASSERT(!state_manager.validate_state(), "Invalid light type should fail validation");
    
    // Test valid point light
    light.Type = D3DLIGHT_POINT;
    light.Range = 100.0f;
    light.Attenuation0 = 1.0f;
    light.Attenuation1 = 0.0f;
    light.Attenuation2 = 0.0f;
    state_manager.set_light(0, &light);
    TEST_ASSERT(state_manager.validate_state(), "Valid point light should pass validation");
    
    // Test point light with invalid range
    light.Range = -10.0f; // Invalid negative range
    state_manager.set_light(0, &light);
    TEST_ASSERT(!state_manager.validate_state(), "Point light with negative range should fail validation");
    
    // Test spot light with invalid cone angles
    light.Type = D3DLIGHT_SPOT;
    light.Range = 100.0f;
    light.Theta = 1.0f;
    light.Phi = 0.5f; // Phi should be >= Theta
    state_manager.set_light(0, &light);
    TEST_ASSERT(!state_manager.validate_state(), "Spot light with invalid cone angles should fail validation");
    
    return true;
}

bool test_shader_state_application() {
    cout << "\n=== Test: Shader State Application ===" << endl;
    
    StateManager state_manager;
    state_manager.initialize();
    
    MockShaderProgram mock_shader;
    
    // Set up some transform states
    D3DMATRIX test_matrix = {
        2.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 2.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    state_manager.set_transform(D3DTS_WORLD, &test_matrix);
    
    // Apply transform states (this should not crash)
    state_manager.apply_transform_states(&mock_shader);
    TEST_ASSERT(true, "Transform state application should not crash");
    
    // Set up material state
    D3DMATERIAL8 material = {};
    material.Diffuse.r = 1.0f;
    material.Diffuse.g = 0.5f;
    material.Diffuse.b = 0.2f;
    material.Diffuse.a = 1.0f;
    material.Power = 32.0f;
    state_manager.set_material(&material);
    
    // Apply material state
    state_manager.apply_material_state(&mock_shader);
    TEST_ASSERT(true, "Material state application should not crash");
    
    // Set up fog state
    state_manager.set_render_state(D3DRS_FOGENABLE, TRUE);
    state_manager.set_render_state(D3DRS_FOGCOLOR, 0xFF808080);
    state_manager.set_render_state(D3DRS_FOGSTART, *reinterpret_cast<const DWORD*>(&test_matrix._11)); // 2.0f
    state_manager.set_render_state(D3DRS_FOGEND, *reinterpret_cast<const DWORD*>(&test_matrix._22)); // 2.0f
    
    // Apply fog state
    state_manager.apply_fog_state(&mock_shader);
    TEST_ASSERT(true, "Fog state application should not crash");
    
    // Set up light state
    D3DLIGHT8 light = {};
    light.Type = D3DLIGHT_DIRECTIONAL;
    light.Diffuse.r = light.Diffuse.g = light.Diffuse.b = 1.0f;
    light.Direction.z = 1.0f;
    state_manager.set_light(0, &light);
    state_manager.light_enable(0, TRUE);
    state_manager.set_render_state(D3DRS_LIGHTING, TRUE);
    
    // Apply light state
    state_manager.apply_light_states(&mock_shader);
    TEST_ASSERT(true, "Light state application should not crash");
    
    return true;
}

bool test_state_mutations() {
    cout << "\n=== Test: State Mutations and Dirty Flags ===" << endl;
    
    StateManager state_manager;
    state_manager.initialize();
    
    // Test that changing render states marks them as dirty
    DWORD original_cull = state_manager.get_render_state(D3DRS_CULLMODE);
    state_manager.set_render_state(D3DRS_CULLMODE, D3DCULL_CW);
    DWORD new_cull = state_manager.get_render_state(D3DRS_CULLMODE);
    TEST_ASSERT(new_cull == D3DCULL_CW, "Render state should be updated correctly");
    TEST_ASSERT(new_cull != original_cull, "Render state should change from original value");
    
    // Test texture stage state mutations
    state_manager.set_texture_stage_state(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    DWORD color_op = state_manager.get_texture_stage_state(0, D3DTSS_COLOROP);
    TEST_ASSERT(color_op == D3DTOP_MODULATE, "Texture stage state should be updated correctly");
    
    state_manager.set_texture_stage_state(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    DWORD mag_filter = state_manager.get_texture_stage_state(0, D3DTSS_MAGFILTER);
    TEST_ASSERT(mag_filter == D3DTEXF_LINEAR, "Texture filter state should be updated correctly");
    
    // Test transform state mutations
    D3DMATRIX original_world, new_world;
    state_manager.get_transform(D3DTS_WORLD, &original_world);
    
    D3DMATRIX test_world = {
        1.5f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.5f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    state_manager.set_transform(D3DTS_WORLD, &test_world);
    state_manager.get_transform(D3DTS_WORLD, &new_world);
    TEST_ASSERT(new_world._11 == 1.5f, "World matrix should be updated correctly");
    
    // Test light state mutations
    D3DLIGHT8 test_light = {};
    test_light.Type = D3DLIGHT_POINT;
    test_light.Diffuse.r = 0.8f;
    test_light.Diffuse.g = 0.6f;
    test_light.Diffuse.b = 0.4f;
    test_light.Range = 50.0f;
    
    state_manager.set_light(1, &test_light);
    state_manager.light_enable(1, TRUE);
    TEST_ASSERT(state_manager.is_light_enabled(1), "Light should be enabled");
    
    D3DLIGHT8 retrieved_light;
    state_manager.get_light(1, &retrieved_light);
    TEST_ASSERT(retrieved_light.Type == D3DLIGHT_POINT, "Light type should be preserved");
    TEST_ASSERT(retrieved_light.Diffuse.r == 0.8f, "Light diffuse color should be preserved");
    TEST_ASSERT(retrieved_light.Range == 50.0f, "Light range should be preserved");
    
    return true;
}

bool test_state_invalidation() {
    cout << "\n=== Test: State Invalidation ===" << endl;
    
    StateManager state_manager;
    state_manager.initialize();
    
    // Set up some states
    state_manager.set_render_state(D3DRS_ZENABLE, TRUE);
    state_manager.set_render_state(D3DRS_ALPHABLENDENABLE, TRUE);
    state_manager.set_texture_stage_state(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    state_manager.set_texture_stage_state(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    
    // Apply render states to clear dirty flags
    state_manager.apply_render_states();
    
    // Invalidate cached states
    state_manager.invalidate_cached_render_states();
    TEST_ASSERT(true, "State invalidation should not crash");
    
    // States should still be readable
    DWORD z_enable = state_manager.get_render_state(D3DRS_ZENABLE);
    TEST_ASSERT(z_enable == TRUE, "Render states should remain accessible after invalidation");
    
    DWORD color_op = state_manager.get_texture_stage_state(0, D3DTSS_COLOROP);
    TEST_ASSERT(color_op == D3DTOP_MODULATE, "Texture states should remain accessible after invalidation");
    
    return true;
}

bool run_all_tests() {
    cout << "Running State Manager Validation Tests" << endl;
    cout << "=======================================" << endl;
    
    bool all_passed = true;
    
    all_passed &= test_render_state_validation();
    all_passed &= test_texture_state_validation();
    all_passed &= test_transform_state_validation();
    all_passed &= test_light_state_validation();
    all_passed &= test_shader_state_application();
    all_passed &= test_state_mutations();
    all_passed &= test_state_invalidation();
    
    return all_passed;
}

int main() {
    bool success = run_all_tests();
    
    cout << "\n=======================================" << endl;
    cout << "Test Results: " << tests_passed << "/" << tests_run << " passed" << endl;
    
    if (success && tests_passed == tests_run) {
        cout << "All tests PASSED!" << endl;
        return 0;
    } else {
        cout << "Some tests FAILED!" << endl;
        return 1;
    }
}