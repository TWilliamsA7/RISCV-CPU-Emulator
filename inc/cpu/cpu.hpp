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

class CPU {
    public:
        CPU(Memory& mem);
        StepResult step();
        
        uint32_t pc() const;
        uint32_t reg(size_t idx) const;

    private:
        uint32_t pc_;
        uint32_t x_[32];

        Memory& memory_;
};