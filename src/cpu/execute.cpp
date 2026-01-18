// src/cpu/execute.cpp

#include "core/state.hpp"
#include "isa/rv32i.hpp"

namespace cpu {

    void execute_dummy(core::CpuState&) {
        static_assert(isa::rv32i::ISA_MAGIC == 32);
    }

}