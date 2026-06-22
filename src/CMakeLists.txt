# 1. Declare and Fetch dependencies FIRST
include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/include.zip
    # REMOVED DOWNLOAD_EXTRACT_TIMESTAMP (Not supported on CMake 3.16)
)
FetchContent_MakeAvailable(nlohmann_json)

# Fallback setup for older FetchContent versions that don't make targets automatically
if(NOT TARGET nlohmann_json::nlohmann_json)
    add_library(nlohmann_json INTERFACE)
    target_include_directories(nlohmann_json INTERFACE "${nlohmann_json_SOURCE_DIR}/include")
    add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
endif()

# 2. Now include your local subdirectories
add_subdirectory(core)
add_subdirectory(cpu)
add_subdirectory(memory)
add_subdirectory(isa)
add_subdirectory(platform)
add_subdirectory(bus)
add_subdirectory(elf)
add_subdirectory(clint)
add_subdirectory(mmu)
add_subdirectory(plic)
add_subdirectory(devices)
add_subdirectory("cache")
add_subdirectory(profile)

# 3. Define your main target
add_executable(riscv_emulator main.cpp emulator.cpp)

# 4. Optimization Flag setups
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -fno-plt -funroll-loops")

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

if(ipo_supported)
    message(STATUS "IPO/LTO supported, enabling it.")
    set_target_properties(riscv_emulator PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO/LTO not supported: ${ipo_error}")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(riscv_emulator PRIVATE -flto)
        target_link_options(riscv_emulator PRIVATE -flto)
    endif()
endif()

# 5. Link EVERYTHING (including nlohmann_json)
if(UNIX)
    target_link_libraries(riscv_emulator PRIVATE 
        cpu core memory elf bus clint mmu plic devices "cache" profile Threads::Threads
        nlohmann_json::nlohmann_json
    )
else()
    target_link_libraries(riscv_emulator PRIVATE 
        cpu core memory elf bus clint mmu plic devices "cache" profile
        nlohmann_json::nlohmann_json
    )
endif()