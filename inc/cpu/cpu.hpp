// inc/cpu/cpu.hpp

#pragma once
#include <cstdint>

#include "memory/memory.hpp"

struct StepResult {
    uint32_t pc_before;
    uint32_t pc_after;
    uint32_t instruction;
    bool trap;
};

struct ArchitecturalState {
    uint32_t pc;
    uint32_t x[32];
    Memory memory;
};

class CPU {
    public:
        StepResult step();
        const ArchitecturalState& state() const;

    private:
        ArchitecturalState _state;
};