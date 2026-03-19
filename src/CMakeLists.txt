add_subdirectory(core)
add_subdirectory(cpu)
add_subdirectory(memory)
add_subdirectory(isa)
add_subdirectory(platform)

add_executable(riscv_emulator main.cpp)

target_link_libraries(riscv_emulator PRIVATE cpu core memory)