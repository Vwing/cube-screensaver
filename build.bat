@echo off
echo Building Bouncing Cube Screensaver...

if not exist build mkdir build
cd build

cmake -G "Visual Studio 17 2022" -A x64 ..
if errorlevel 1 (
    echo CMake configuration failed. Trying with Visual Studio 16 2019...
    cmake -G "Visual Studio 16 2019" -A x64 ..
)

cmake --build . --config Release

if exist Release\BouncingCube.scr (
    echo.
    echo Build successful! Screensaver created: build\Release\BouncingCube.scr
    echo.
    echo To install:
    echo 1. Right-click on BouncingCube.scr
    echo 2. Select "Install"
    echo.
    echo Or copy it to C:\Windows\System32\ to make it available in Screen Saver settings.
) else (
    echo Build failed!
)

cd ..
pause