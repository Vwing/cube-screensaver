# Bouncing Cube Screensaver

A Windows screensaver featuring a 3D cube that bounces across multiple monitors with smooth rotation and configurable settings.

## Features

- **Single cube across multiple monitors**: Cube travels seamlessly between all connected displays
- **Smooth 3D rotation**: Uses rotation matrices for clean motion without visual jumps
- **Configurable cube size**: Settings dialog with slider (Small/Medium/Large)
- **Corner celebration**: Cube pulses and changes color when hitting screen corners
- **Perspective projection**: Proper 3D rendering with lighting and depth
- **Persistent settings**: Cube size preference saved to Windows registry

## Building

### Prerequisites
- Visual Studio 2019 or 2022 with C++ development tools
- CMake 3.10 or higher

### Build Instructions

1. Open a command prompt in the project directory
2. Run: `build.bat`

The screensaver will be built as `build\Release\BouncingCube.scr`

### Alternative: Manual Build with Visual Studio

If you prefer to build manually:

```bash
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
```

Then open the generated `.sln` file in Visual Studio and build in Release mode.

## Installation

### Method 1: Quick Install
1. Right-click on `BouncingCube.scr`
2. Select "Install"
3. Configure cube size in Screen Saver settings

### Method 2: System Installation
1. Copy `BouncingCube.scr` to `C:\Windows\System32\`
2. Go to Windows Settings > Personalization > Lock screen
3. Click "Screen saver settings"
4. Select "BouncingCube" from the dropdown
5. Click "Settings" to adjust cube size

## How It Works

- A single 3D cube travels across all connected monitors seamlessly
- The cube uses single-axis rotation for smooth, predictable motion
- Bouncing off screen edges generates new random rotation axes while preserving orientation
- Corner detection triggers celebration effects:
  - The cube pulses in size
  - The colors brighten and cycle
  - The effect lasts for about 1 second
- Cube size can be adjusted from Small to Large in the settings dialog

## Technical Details

- Written in C++ using Win32 API and OpenGL
- Uses perspective projection for proper 3D depth perception
- Rotation matrices prevent visual jumps and gimbal lock issues
- Multi-monitor support via EnumDisplayMonitors with shared cube state
- Settings stored in Windows registry for persistence
- Uses common controls (trackbar) for configuration dialog
- Implements required screensaver exports: ScreenSaverProc, ScreenSaverConfigureDialog

## Troubleshooting

If the build fails:
- Ensure Visual Studio is installed with C++ development tools
- Check that CMake is in your PATH
- Try building with Visual Studio 2019 instead of 2022 by modifying the CMake generator in build.bat