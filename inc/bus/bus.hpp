// inc/bus/bus.hpp

#pragma once

#include <cstdint>
#include <vector>
#include "clint/clint.hpp"
#include "plic/plic.hpp"
#include <mutex>
#include <functional>

class CPU;

// Interface for reading and writing to perhiperals
class Bus {
    public:

        Bus(Clint& clint, PLIC& plic);

        static constexpr uint32_t UART = 0x10000000;

        // Starting point of DRAM addresses
        static constexpr uint32_t DRAM_BASE = 0x80000000;

        // Size of avaiable DRAM
        static constexpr uint32_t DRAM_SIZE = 1024 * 1024 * 128;

        // Read 32 bit value located at addr 
        uint32_t read32(uint32_t addr) const;

        // Read 16 bit value located at addr 
        uint16_t read16(uint32_t addr) const;

        // Read 8 bit value located at addr
        uint8_t read8(uint32_t addr) const;

        // Write 32 bit value to addr
        void write32(uint32_t addr, uint32_t val);

        // Write 16 bit value to addr
        void write16(uint32_t addr, uint16_t val);

        // Write 32 bit value to addr
        void write8(uint32_t addr, uint8_t val);

        // Register CPU for notification
        void register_cpu(CPU* cpu);

        // Atomic Read-Modify-Write
        uint32_t atomic_rmw_w(uint32_t addr, std::function<uint32_t(uint32_t)> operation);

    private:

        // Dynamic Random Access Memory
        std::vector<uint8_t> dram_;

        // CLINT: Core Local Interruptor
        Clint& clint_;

        // PLIC: Platform-Level Interrupt Controller
        PLIC& plic_;

        std::mutex mem_mutex_;

        CPU* cpu_ptr_;

        void write32_unlocked(uint32_t addr, uint32_t val);
};