# Project Definition
cmake_minimum_required(VERSSION 3.20)
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

# Setup GoogleTest
if(BUILD_TESTS AND NOT USE_SYSTEM_DEPENDENCIES)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0 
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

if(BUILD_TESTS)
    FetchContent_MakeAvailable(googletest)
endif()

add_subdirectory(src)
add_subdirectory(tests)

if(BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()