// src/emulator.cpp

#include "emulator.hpp"

#include <iostream>
#include "elf/elf.hpp"

Emulator::Emulator(EmulatorConfig config) : 
    config_(config), 
    clint(), plic(), 
    bus(*this), cpu(*this), mmu(*this) {}

void Emulator::initialize() {
    if (config_.profile.elf_path.size() == 0) {
        throw std::runtime_error("[ERROR] Missing ELF path...\n");
    }

    if (config_.cpu_config.mode == ExecutionMode::SYSTEM && config_.profile.disk_path.size() == 0) {
        throw std::runtime_error("[ERROR] Missing disk path for system level execution...\n");
    }

    bus.init_devices(config_.profile.disk_path);

    std::cout << "[INFO] Initializing Emulator...\n";

    load_elf(config_.profile.elf_path, bus, cpu);

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