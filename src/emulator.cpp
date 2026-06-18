// src/emulator.cpp

#include "emulator.hpp"

#include <iostream>

Emulator::Emulator(EmulatorConfig config) : 
    config_(config), 
    clint(*this), plic(*this), 
    bus(*this), cpu(*this), mmu(*this) {

    std::cout << "Initializing Emulator...\n";

}