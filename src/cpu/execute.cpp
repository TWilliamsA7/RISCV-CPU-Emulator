// src/cpu/execute.cpp

#include "core/state.h"
#include "isa/rv32i.h"

namespace cpu {

    void execute_dummy(core::CpuState&) {
        static_assert(isa::rv32i::ISA_MAGIC == 32);
    }

}