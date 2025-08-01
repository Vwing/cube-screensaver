cmake_minimum_required(VERSION 3.10)
project(BouncingCubeScreensaver)

set(CMAKE_CXX_STANDARD 11)

# Build the modern OpenGL application (BouncingCubeApp.exe)
add_executable(BouncingCubeApp WIN32 BouncingCubeApp.cpp)

# Link required libraries for the app
target_link_libraries(BouncingCubeApp 
    opengl32
    glu32
    user32
    gdi32
)

# Set subsystem to WINDOWS for the app
set_target_properties(BouncingCubeApp PROPERTIES
    LINK_FLAGS "/SUBSYSTEM:WINDOWS"
)

# Build the thin screensaver wrapper (BouncingCube.scr)
add_executable(BouncingCube WIN32 ScreensaverWrapper.cpp screensaver.def screensaver.rc)

# Set output to .scr extension for the wrapper
set_target_properties(BouncingCube PROPERTIES
    SUFFIX ".scr"
)

# Link required libraries for the wrapper
target_link_libraries(BouncingCube 
    scrnsave
    user32
    gdi32
    comctl32
)

# Set subsystem to WINDOWS for the wrapper
set_target_properties(BouncingCube PROPERTIES
    LINK_FLAGS "/SUBSYSTEM:WINDOWS"
)

# Install both targets
install(TARGETS BouncingCube BouncingCubeApp DESTINATION ${CMAKE_INSTALL_PREFIX})