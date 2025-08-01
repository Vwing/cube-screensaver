# Screensaver Multi-Monitor Rendering Issue

## Current Status
- **WORKING**: Cube physics are now constrained to primary monitor only (mirror mode)
- **NOT WORKING**: Cube only renders on one monitor, not both (despite mirror mode being enabled)

## Problem
The cube renders on only one monitor even though mirror mode is implemented. The rendering logic tries to render on all monitors with OpenGL contexts, but only one monitor actually shows the cube.

## Key Code Locations
- **Mirror mode rendering**: main.cpp:568-583 (WM_TIMER handler)
- **OpenGL context initialization**: main.cpp:507-510 (WM_CREATE handler) 
- **Monitor enumeration**: main.cpp:124-137 (MonitorEnumProc)
- **Mirror mode config**: g_MirrorMode variable (defaulted to true)

## Suspected Issues
1. **OpenGL Context Problem**: Each monitor window needs its own OpenGL context, but the association logic may be broken
2. **Window-Monitor Association**: Windows screensaver creates separate windows for each monitor, but our code may not be properly associating each window with its monitor's OpenGL context
3. **Context Switching**: The `wglMakeCurrent()` calls in `RenderScene()` might not be working correctly across different windows

## Architecture
- Each monitor gets its own screensaver window from Windows
- Each monitor should have its own OpenGL context (stored in `mon.hglrc`)
- In mirror mode, every timer tick should render the cube on ALL monitors with valid contexts
- Physics are constrained to primary monitor bounds only

## What to Investigate Next
1. Check if all monitors actually have valid OpenGL contexts (`mon.hglrc != NULL`)
2. Verify that `RenderScene()` is being called for each monitor
3. Ensure `wglMakeCurrent()` works correctly for each monitor's context
4. Consider that the window-monitor association in WM_CREATE might be the root cause

## Test Setup
- Dual monitor vertical arrangement (one above the other)
- Mirror mode enabled (g_MirrorMode = true)
- Cube should appear on both monitors simultaneously
- Physics constrained to primary monitor space