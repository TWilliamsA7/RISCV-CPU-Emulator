// inc/emulator.hpp

#include "cpu/cpu.hpp"
#include "profile/profile.hpp"

#pragma once

struct EmulatorConfig {
    Profile profile;
    CPUConfig cpu_config;
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