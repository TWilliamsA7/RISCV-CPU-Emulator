// inc/bus/bus.hpp

#pragma once

#include <cstdint>
#include <vector>
#include "clint/clint.hpp"
#include "plic/plic.hpp"
#include "devices/uart.hpp"
#include "devices/virtio_blk.hpp"
#include <mutex>
#include <functional>
#include <string>

class Emulator;
class CPU;

// Interface for reading and writing to perhiperals
class Bus {
    public:

        Bus(Emulator& sys);

        // Starting point of DRAM addresses
        static constexpr uint32_t DRAM_BASE = 0x80000000;

        // Size of avaiable DRAM
        static constexpr uint32_t DRAM_SIZE = 1024 * 1024 * 128;

        // Read 32 bit value located at addr 
        uint32_t read32(uint32_t addr);

        // Read 16 bit value located at addr 
        uint16_t read16(uint32_t addr);

        // Read 8 bit value located at addr
        uint8_t read8(uint32_t addr);

        uint8_t* phys_ptr(uint32_t phys_addr);

        // Write 32 bit value to addr
        void write32(uint32_t addr, uint32_t val);

        // Write 16 bit value to addr
        void write16(uint32_t addr, uint16_t val);

        // Write 32 bit value to addr
        void write8(uint32_t addr, uint8_t val);

        // Register CPU for notification
        void register_cpu(CPU* cpu);

        // Queue host-provided bytes for deterministic UART tests
        void inject_uart_input(const std::string& input);
        void defer_uart_input_until_wfi(const std::string& input);
        void release_deferred_uart_input();

        bool is_mmio(uint32_t addr) const;

        // Atomic Read-Modify-Write
        uint32_t atomic_rmw_w(uint32_t addr, std::function<uint32_t(uint32_t)> operation);

        void load_binary(std::vector<uint8_t>::const_iterator bin_start, std::vector<uint8_t>::const_iterator bin_end, uint32_t addr);

        void write8_unlocked(uint32_t addr, uint8_t val);
        void write16_unlocked(uint32_t addr, uint16_t val);
        void write32_unlocked(uint32_t addr, uint32_t val);

    private:

        Emulator& sys_;

        friend class VirtioBlk;

        // Dynamic Random Access Memory
        std::vector<uint8_t> dram_;

        // UART: Universal Asynchronous Reciever/Transmitter
        UART uart_;

        VirtioBlk virtio_blk_;

        std::mutex mem_mutex_;

        CPU* cpu_ptr_;

        std::string deferred_uart_input_;

        
};
