// inc/emulator.hpp

#include "cpu/cpu.hpp"

struct EmulatorConfig {
    char* elf_path = nullptr;
    char* disk_path = nullptr;

    CPUConfig cpu_config;
};


class Emulator {
    public:

        Emulator(EmulatorConfig config);

        Clint clint;
        PLIC plic;
        Bus bus;
        CPU cpu;
        MMU mmu;

        EmulatorConfig config_;
};