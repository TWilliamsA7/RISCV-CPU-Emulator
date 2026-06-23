// src/emulator.cpp

#include "emulator.hpp"

#include <iostream>
#include "elf/elf.hpp"

Emulator::Emulator(EmulatorConfig config) : 
    config_(config), 
    clint(), plic(), 
    bus(*this), cpu(*this), mmu(*this) {}

void Emulator::initialize() {
    if (config_.profile.elf_path.size() == 0 && config_.profile.bin_path.size() == 0) {
        throw std::runtime_error("[ERROR] Missing ELF/Bin path...\n");
    }

    if (config_.cpu_config.mode == ExecutionMode::SYSTEM && config_.profile.disk_path.size() == 0) {
        throw std::runtime_error("[ERROR] Missing disk path for system level execution...\n");
    }

    bus.init_devices(config_.profile.disk_path, config_.profile.tap_name);

    std::cout << "[INFO] Initializing Emulator...\n";

    if (config_.profile.opensbi_path.size() == 0) {
        if (config_.profile.elf_path.size() != 0) {
            load_elf(config_.profile.elf_path, bus, cpu);
        } else {
            load_binary(config_.profile.bin_path, bus, config_.profile.dram_start);
        }
    } else {
        load_elf(config_.profile.opensbi_path, bus, cpu);
        load_binary(config_.profile.bin_path, bus, config_.profile.kernel_address);
    }


    if (config_.profile.dtb_path.size() != 0) {
        load_binary(config_.profile.dtb_path, bus, config_.profile.dtb_address);
    }

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