cmake_minimum_required(VERSION 3.11)
project(trackmania_clone)

set(CMAKE_CXX_STANDARD 11)

# Check if we are building for Android
if (ANDROID)
    # Android specific settings
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

    # We expect Raylib to be provided externally or fetched
    # For simplicity in CI, we will assume raylib is available or fetch it
    include(FetchContent)
    FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
    )
    FetchContent_MakeAvailable(raylib)

    add_library(trackmania_clone SHARED src/main.cpp)
    target_link_libraries(trackmania_clone raylib android log)
else()
    # Desktop build
    find_package(raylib REQUIRED)
    add_executable(trackmania_clone src/main.cpp)
    target_link_libraries(trackmania_clone raylib m)
endif()
