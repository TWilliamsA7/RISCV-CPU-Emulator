// src/cpu/cpu.cpp

#include "cpu/cpu.hpp"
#include "memory/memory.hpp"

#include <algorithm>
#include <cassert>

CPU::CPU(Memory& mem) : pc_(0), memory_(mem) {
    std::fill(std::begin(x_), std::end(x_), 0);
}


StepResult CPU::step() {

}


uint32_t CPU::pc() const { return pc_; }

uint32_t CPU::reg(size_t idx) const {
    assert(idx < 32);
    return x_[idx];
}


