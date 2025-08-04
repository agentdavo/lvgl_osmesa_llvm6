# Device Reset Sample

This sample demonstrates proper handling of DirectX 8 device reset scenarios in dx8gl.

## Overview

The device reset sample shows:
- Proper handling of D3DPOOL_DEFAULT vs D3DPOOL_MANAGED resources
- Device reset triggered by window resize
- Manual reset triggered by spacebar
- Resource recreation after device reset
- Render state restoration

## Key Concepts

### Resource Pools

DirectX 8 has different memory pools for resources:

- **D3DPOOL_DEFAULT**: Resources lost on device reset, must be recreated
- **D3DPOOL_MANAGED**: Resources preserved across device resets
- **D3DPOOL_SYSTEMMEM**: System memory resources (not demonstrated here)

### Device Reset Flow

1. Detect device lost condition (window resize, alt-tab, etc.)
2. Release all D3DPOOL_DEFAULT resources
3. Call `IDirect3DDevice8::Reset()` with new parameters
4. Recreate D3DPOOL_DEFAULT resources
5. Restore render states (they're reset to defaults)

## Visual Demonstration

The sample displays two textured quads side-by-side:
- **Left quad**: Uses D3DPOOL_DEFAULT texture (red checkerboard initially)
- **Right quad**: Uses D3DPOOL_MANAGED texture (green checkerboard)

After reset:
- Left texture changes to blue (recreated with different color)
- Right texture remains green (preserved across reset)

## Controls

- **Resize window**: Triggers device reset
- **SPACE**: Manually trigger device reset
- **ESC**: Exit application

## Implementation Details

The sample implements:
- Proper COM reference counting for resources
- TestCooperativeLevel() checking for device state
- Resource tracking and cleanup
- Error handling for reset failures
- Visual feedback showing reset success

## Building

From the dx8gl root directory:
```bash
cmake -S . -B build
cmake --build build
```

The sample will be built as `build/samples/device_reset/dx8gl_device_reset`.