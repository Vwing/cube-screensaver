cmake_minimum_required(VERSION 3.10)
project(BouncingCubeScreensaver)

set(CMAKE_CXX_STANDARD 11)

# Set output to .scr extension
set(CMAKE_EXECUTABLE_SUFFIX ".scr")

# Add executable
add_executable(BouncingCube WIN32 main.cpp screensaver.def)

# Link required libraries
target_link_libraries(BouncingCube 
    scrnsave
    opengl32
    glu32
    user32
    gdi32
    comctl32
)

# Set subsystem to WINDOWS
set_target_properties(BouncingCube PROPERTIES
    LINK_FLAGS "/SUBSYSTEM:WINDOWS"
)

# Install target
install(TARGETS BouncingCube DESTINATION ${CMAKE_INSTALL_PREFIX})