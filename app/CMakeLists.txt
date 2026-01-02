cmake_minimum_required(VERSION 3.22)
project(trackmaniaclone)

# Fetch Raylib
include(FetchContent)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
)
FetchContent_MakeAvailable(raylib)

add_library(main SHARED src/main/cpp/main.cpp)

# Link libraries
# -u ANativeActivity_onCreate is required to prevent the linker from stripping the entry point
target_link_libraries(main PRIVATE raylib android log EGL GLESv2 OpenSLES -u ANativeActivity_onCreate)
