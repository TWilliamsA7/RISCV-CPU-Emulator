// inc/core/state.hpp

#pragma once

#include "isa/isa.hpp"
#include <cstdint>

struct RegWrite {
    bool valid = false;
    uint8_t rd;
    uint32_t old_val;
    uint32_t new_val;
};

struct MemWrite {
    bool valid = false;
    uint32_t addr;
    uint32_t old_val;
    uint32_t new_val;
    uint8_t size; // 1, 2, or 4 bytes
};

struct CsrWrite {
    bool valid = false;
    uint16_t addr;
    uint32_t old_val;
    uint32_t new_val;
};

struct StepResult {
    uint32_t pc_before;
    uint32_t instruction;
    DecodedInstr dInstr;
    uint32_t pc_after;

    RegWrite reg_write;
    MemWrite mem_write;
    CsrWrite csr_write;
};
