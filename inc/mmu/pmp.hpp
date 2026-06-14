// inc/mmu/pmp.hpp

#pragma once

#include "cpu/cpu.hpp"
#include "mmu.hpp"

inline void check_pmp(CPU& cpu, uint32_t pa, MMU::AccessType at) {
    (void)cpu; (void)pa; (void)at;
    // no-op
}