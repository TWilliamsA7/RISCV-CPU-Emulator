// inc/core/state.hpp

#pragma once

#include "isa/isa.hpp"
#include <cstdint>
#include <optional>

struct RegWrite {
    uint8_t rd;
    uint32_t old_val;
    uint32_t new_val;
};

struct MemWrite {
    uint32_t addr;
    uint32_t old_val;
    uint32_t new_val;
    uint8_t size; // 1, 2, or 4 bytes
};

struct CsrWrite {
    uint16_t addr;
    uint32_t old_val;
    uint32_t new_val;
};

struct StepResult {
    uint32_t pc_before;
    uint32_t instruction;
    DecodedInstr dInstr;
    uint32_t pc_after;

    std::optional<RegWrite> reg_write;
    std::optional<MemWrite> mem_write;
    std::optional<CsrWrite> csr_write;
};
