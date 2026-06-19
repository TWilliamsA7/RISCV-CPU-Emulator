// src/emulator.cpp

#include "emulator.hpp"

#include <iostream>
#include "elf/elf.hpp"

Emulator::Emulator(EmulatorConfig config) : 
    config_(config), 
    clint(), plic(), 
    bus(*this), cpu(*this), mmu(*this) {

    if (config_.elf_path == nullptr) {
        throw std::runtime_error("[ERROR] Missing ELF path...\n");
    }

    if (config_.cpu_config.mode == ExecutionMode::SYSTEM && config_.disk_path == nullptr) {
        throw std::runtime_error("[ERROR] Missing disk path for system level execution...\n");
    }

    std::cout << "[INFO] Initializing Emulator...\n";

    load_elf(config_.elf_path, bus, cpu);

    std::cout << "[INFO] Emulator Ready\n";
}

int Emulator::emulate() {
    try {
        std::cout << "[INFO] Starting Emulator\n";
        cpu.run();
    } catch (const ProgramExit& e) {
        return e.code;
    }

    std::cout << "\n[INFO] Exiting Emulator...\n";

    return 0;
}