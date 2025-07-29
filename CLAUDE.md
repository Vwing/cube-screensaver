# 3D Bouncing Cube Screensaver

## Overview
A Windows screensaver featuring a 3D cube that bounces across multiple monitors with smooth rotation and configurable settings.

## Key Features
- **Multi-monitor support**: Cube travels seamlessly across all connected displays
- **Smooth 3D rotation**: Uses rotation matrices to prevent visual jumps when bouncing
- **Corner celebration**: Cube pulses and changes color when hitting screen corners
- **Configurable cube size**: Settings dialog with slider (Small/Medium/Large)
- **Perspective projection**: Proper 3D rendering with lighting

## Technical Implementation

### Rotation System
- Uses 4x4 rotation matrices instead of Euler angles to prevent gimbal lock
- Single-axis rotation for clean motion (no complex tumbling)
- Matrix multiplication preserves orientation between axis changes
- New random rotation axis generated on each bounce

### Coordinate System
- Screen coordinates in pixels for physics/collision detection
- Conversion to normalized coordinates for OpenGL rendering
- Aspect ratio correction for proper perspective projection

### Settings Storage
- Registry storage: `HKEY_CURRENT_USER\Software\BouncingCubeScreensaver`
- Cube size range: 0.05 to 0.6 scale units
- Automatic settings load on screensaver start

### Key Functions
- `UpdateCube()`: Physics and collision detection at main.cpp:267
- `DrawCube()`: 3D rendering with matrices at main.cpp:187
- `CreateRotationMatrix()`: Matrix generation at main.cpp:61
- `ScreenSaverConfigureDialog()`: Settings UI at main.cpp:543

### Build System
- CMake configuration in CMakeLists.txt
- Outputs .scr file for Windows screensaver installation
- Requires OpenGL, scrnsave.lib, and common controls

## Installation
1. Build with `./build.bat`
2. Right-click `BouncingCube.scr` → Install
3. Access settings via Screen Saver settings in Windows

## Recent Fixes
- Fixed "weird tumbling" by implementing single-axis rotation
- Eliminated visual rotation jumps using rotation matrices
- Fixed aspect ratio handling for full-width bouncing
- Added configurable cube size with persistent settings