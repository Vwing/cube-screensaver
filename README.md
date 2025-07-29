# Bouncing Cube Screensaver

A Windows screensaver featuring a bouncing cube that celebrates when it hits a corner!

## Features

- 3D cube with realistic physics
- Multi-monitor support - each monitor gets its own cube
- Corner detection with celebration animation
- Smooth OpenGL rendering
- Random colors and speeds for each cube

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

### Method 2: System Installation
1. Copy `BouncingCube.scr` to `C:\Windows\System32\`
2. Go to Windows Settings > Personalization > Lock screen
3. Click "Screen saver settings"
4. Select "BouncingCube" from the dropdown

## How It Works

- Each monitor displays its own bouncing cube
- The cube bounces off the edges of the screen
- When the cube hits a corner (within a small threshold), it triggers a celebration:
  - The cube pulses in size
  - The colors brighten
  - The effect lasts for about 1 second

## Technical Details

- Written in C++ using Win32 API and OpenGL
- Uses the Windows screensaver library (scrnsave.lib)
- Implements required screensaver exports: ScreenSaverProc, ScreenSaverConfigureDialog
- Multi-monitor support via EnumDisplayMonitors
- Each monitor gets its own OpenGL context

## Troubleshooting

If the build fails:
- Ensure Visual Studio is installed with C++ development tools
- Check that CMake is in your PATH
- Try building with Visual Studio 2019 instead of 2022 by modifying the CMake generator in build.bat