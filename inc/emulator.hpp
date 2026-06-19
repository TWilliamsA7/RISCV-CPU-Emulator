// inc/emulator.hpp

#include "cpu/cpu.hpp"

#pragma once

struct EmulatorConfig {
    char* elf_path = nullptr;
    char* disk_path = nullptr;

    CPUConfig cpu_config;
    bool verbose = false;
};


class Emulator {
    public:

        Emulator(EmulatorConfig config);

        EmulatorConfig config_;
        Clint clint;
        PLIC plic;
        Bus bus;
        CPU cpu;
        MMU mmu;


        int emulate();
};