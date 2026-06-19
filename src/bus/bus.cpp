// src/bus/bus.cpp


#include "bus/bus.hpp"
#include "emulator.hpp"
#include "errors/errors.hpp"
#include <iostream>

Bus::Bus(Emulator& sys) 
    : sys_(sys), 
    uart_(sys, [&sys](uint32_t irq){ sys.plic.set_pending(irq);}),
    virtio_blk_([&sys](uint32_t irq){ sys.plic.set_pending(irq); }, *this)
{
    dram_ = std::vector<uint8_t>(Bus::DRAM_SIZE, 0);
}

void Bus::init_devices(const std::string& disk_path) {
    if (disk_path.size())
        virtio_blk_.init(disk_path);
}

void Bus::register_cpu(CPU* cpu) { cpu_ptr_ = cpu; }

void Bus::inject_uart_input(const std::string& input) {
    for (unsigned char ch : input) {
        uart_.rx_push(ch);
    }
}

void Bus::defer_uart_input_until_wfi(const std::string& input) {
    deferred_uart_input_ += input;
}

void Bus::release_deferred_uart_input() {
    if (deferred_uart_input_.empty())
        return;

    std::string input;
    input.swap(deferred_uart_input_);
    inject_uart_input(input);
}

bool Bus::is_mmio(uint32_t addr) const {
    return (addr >= UART::BASE && addr < UART::BASE + UART::SIZE) ||
           (addr >= Clint::BASE && addr < Clint::BASE + Clint::SIZE) ||
           (addr >= PLIC::BASE && addr < PLIC::BASE + PLIC::SIZE);
}

uint8_t Bus::read8(uint32_t addr) {

    if (addr >= UART::BASE && addr < UART::BASE + UART::SIZE) {
        return uart_.read8(addr - UART::BASE);
    }

    if (addr >= Bus::DRAM_BASE && addr < DRAM_BASE + DRAM_SIZE) {
        uint32_t offset = addr - Bus::DRAM_BASE;
        return dram_[offset];
    }
    
    throw BusAccessError(std::to_string(addr) + " is outside of mapped range");
}

uint16_t Bus::read16(uint32_t addr) {
    if (addr >= Bus::DRAM_BASE && addr < DRAM_BASE + DRAM_SIZE) {
        uint32_t offset = addr - Bus::DRAM_BASE;
        uint32_t result;
        std::memcpy(&result, &dram_[offset], 2);
        return result;
    }
    
    throw BusAccessError(std::to_string(addr) + " is outside of mapped range");
}

uint32_t Bus::read32(uint32_t addr) {

    if (addr >= UART::BASE && addr < UART::BASE + UART::SIZE)
        return uart_.read8(addr - UART::BASE);

    if (addr >= Clint::BASE && addr < Clint::BASE + Clint::SIZE)
        return sys_.clint.read32(addr - Clint::BASE);
    if (addr >= Bus::DRAM_BASE && addr < DRAM_BASE + DRAM_SIZE) {
        uint32_t offset = addr - Bus::DRAM_BASE;
        uint32_t result;
        std::memcpy(&result, &dram_[offset], 4);
        return result;
    }
    if (addr >= PLIC::BASE && addr < PLIC::BASE + PLIC::SIZE) {
        return sys_.plic.read32(addr - PLIC::BASE);
    }

    if (addr >= VirtioBlk::BASE && addr < VirtioBlk::BASE + VirtioBlk::SIZE)
        return virtio_blk_.read32(addr - VirtioBlk::BASE);

    throw BusAccessError(std::to_string(addr) + " is outside of mapped range");
}

uint8_t* Bus::phys_ptr(uint32_t phys_addr) {
    if (phys_addr >= DRAM_BASE && phys_addr < DRAM_BASE + DRAM_SIZE) {
        return &dram_[phys_addr - DRAM_BASE];
    }
    throw BusAccessError("phys_ptr out of range: " + std::to_string(phys_addr));
}

void Bus::write8(uint32_t addr, uint8_t val) {
    // std::lock_guard<std::mutex> lock(mem_mutex_);

    if (addr >= UART::BASE && addr < UART::BASE + UART::SIZE) {
        uart_.write8(addr - UART::BASE, val);
        return;
    }

    if (addr >= Bus::DRAM_BASE && addr < DRAM_BASE + DRAM_SIZE) {
        uint32_t offset = addr - Bus::DRAM_BASE;
        dram_[offset] = val;

        if (cpu_ptr_) {
            cpu_ptr_->invalidateReservation(addr);
            cpu_ptr_->icache_.invalidate_page(addr);
            cpu_ptr_->clearDecodeCache();
        }
    }
}

void Bus::write16(uint32_t addr, uint16_t val) {
    // std::lock_guard<std::mutex> lock(mem_mutex_);

    if (addr >= Bus::DRAM_BASE && addr < DRAM_BASE + DRAM_SIZE) {
        uint32_t offset = addr - Bus::DRAM_BASE;
        std::memcpy(&dram_[offset], &val, 2);

        if (cpu_ptr_) {
            cpu_ptr_->invalidateReservation(addr);
            cpu_ptr_->icache_.invalidate_page(addr);
            cpu_ptr_->clearDecodeCache();
        }
    }
}

void Bus::write32(uint32_t addr, uint32_t val) {
    // std::lock_guard<std::mutex> lock(mem_mutex_);

    if (addr == 0x80001000 && val != 0) {
        if (val == 1U) {
            std::cout << "PASS: SUCCESSFUL WRITE TO HOST\n";
            throw ProgramExit(0);
        } else {
            std::cout << "FAIL: ERROR CODE " << val << " WRITTEN TO HOST\n";
            throw ProgramExit(1);
        }
    }

    if (addr >= Clint::BASE && addr < Clint::BASE + Clint::SIZE) {
        sys_.clint.write32(addr - Clint::BASE, val);
    } else if (addr >= PLIC::BASE && addr < PLIC::BASE + PLIC::SIZE) {
        sys_.plic.write32(addr - PLIC::BASE, val);
    } else if (addr >= Bus::DRAM_BASE && addr < DRAM_BASE + DRAM_SIZE) {
        uint32_t offset = addr - Bus::DRAM_BASE;
        std::memcpy(&dram_[offset], &val, 4);

        if (cpu_ptr_) {
            cpu_ptr_->invalidateReservation(addr);
            cpu_ptr_->icache_.invalidate_page(addr);
            cpu_ptr_->clearDecodeCache();
        }
    } else if (addr >= VirtioBlk::BASE && addr < VirtioBlk::BASE + VirtioBlk::SIZE)
        virtio_blk_.write32(addr - VirtioBlk::BASE, val);
}

uint32_t Bus::atomic_rmw_w(uint32_t addr, std::function<uint32_t(uint32_t)> operation) {
   std::lock_guard<std::mutex> lock(mem_mutex_);

    uint32_t old_val = read32(addr);

    uint32_t new_val = operation(old_val);

    write32_unlocked(addr, new_val);        

    return old_val;
}

void Bus::write32_unlocked(uint32_t addr, uint32_t val) {
    if (addr >= Bus::DRAM_BASE && addr < DRAM_BASE + DRAM_SIZE) {
        uint32_t offset = addr - Bus::DRAM_BASE;
        std::memcpy(&dram_[offset], &val, 4);

        if (cpu_ptr_) {
            cpu_ptr_->invalidateReservation(addr);
            cpu_ptr_->icache_.invalidate_page(addr);
            cpu_ptr_->clearDecodeCache();
        }
    }
}

void Bus::write8_unlocked(uint32_t addr, uint8_t val) {
    if (addr >= Bus::DRAM_BASE && addr < Bus::DRAM_BASE + Bus::DRAM_SIZE) {
        dram_[addr - Bus::DRAM_BASE] = val;
        if (cpu_ptr_) {
            cpu_ptr_->invalidateReservation(addr);
            cpu_ptr_->icache_.invalidate_page(addr);
            cpu_ptr_->clearDecodeCache();
        }
    }
}

void Bus::write16_unlocked(uint32_t addr, uint16_t val) {
    if (addr >= Bus::DRAM_BASE && addr < Bus::DRAM_BASE + Bus::DRAM_SIZE) {
        uint32_t offset = addr - Bus::DRAM_BASE;
        std::memcpy(&dram_[offset], &val, 2);
        if (cpu_ptr_) {
            cpu_ptr_->invalidateReservation(addr);
            cpu_ptr_->icache_.invalidate_page(addr);
            cpu_ptr_->clearDecodeCache();
        }
    }
}


void Bus::load_binary(std::vector<uint8_t>::const_iterator bin_start, std::vector<uint8_t>::const_iterator bin_end, uint32_t addr) {
    if (addr >= DRAM_BASE && addr <= (DRAM_BASE + DRAM_SIZE)) {
        std::copy(bin_start, bin_end, dram_.begin() + (addr - DRAM_BASE));
    } else {
        throw std::runtime_error("Cannot load binary outside of DRAM!");
    }
}