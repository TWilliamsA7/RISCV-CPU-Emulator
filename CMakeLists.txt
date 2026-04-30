# Project Definition
cmake_minimum_required(VERSION 3.20)
project(riscv_emulator LANGUAGES CXX)

# Build Options
option(BUILD_TESTS "Build unit tests" ON)
option(USE_SYSTEM_DEPENDENCIES "Prefer system-installed libraries" OFF)

# Standard Settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Suppress errors from dependencies using old CMake versions
set(CMAKE_POLICY_VERSION_MINIMUM "3.5")

# Export compile commands for VSCode IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
include(FetchContent)

if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
endif()

add_subdirectory(src)