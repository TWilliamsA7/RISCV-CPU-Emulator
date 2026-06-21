// src/devices/uart.cpp

#include "devices/uart.hpp"
#include "emulator.hpp"
#include <iostream>
#include <cstdlib>

UART::UART(Emulator& sys, std::function<void(uint32_t)> set_pending_cb)
    : sys_(sys), set_pending_(set_pending_cb) {
    load_test_input();
    start_input_thread();  
}

UART::~UART() { stop_input_thread(); }

uint8_t UART::read_lsr() {
    std::lock_guard<std::mutex> lock(rx_mutex_);
    uint8_t lsr = LSR_THRE | LSR_TEMT; // TX always ready
    if (!rx_fifo_.empty()) lsr |= LSR_DR;
    return lsr;
}

uint8_t UART::read_iir() {
    bool fifo_enabled = (fcr_ & 0x01);
    uint8_t fifo_bits = fifo_enabled ? 0xC0 : 0x00; // bits 6-7 when FIFO enabled
    
    if ((ier_ & IER_RDI) && !rx_fifo_.empty())
        return fifo_bits | IIR_RDI;
    if (ier_ & IER_THRI)
        return fifo_bits | IIR_THRI;
    return fifo_bits | 0x01; // no interrupt pending, but report FIFO status
}

uint8_t UART::read8(uint32_t offset) {
    switch (offset) {
        case RBR: {
            if (lcr_ & 0x80) return 0;
            std::lock_guard<std::mutex> lock(rx_mutex_);
            if (!rx_fifo_.empty()) {
                uint8_t ch = rx_fifo_.front();
                rx_fifo_.pop();
                return ch;
            }
            return 0;
        }
        case IER: return (lcr_ & 0x80) ? 0 : ier_;
        case IIR: return read_iir();
        case LCR: return lcr_;
        case MCR: return mcr_;
        case LSR: return read_lsr();
        case MSR: return 0x00; // No modem signals
        case SCR: return scr_;
        default:  return 0;
    }
}

void UART::write8(uint32_t offset, uint8_t val) {
    switch (offset) {
        case THR:
            if (lcr_ & 0x80) break; // DLAB set — divisor latch, ignore
            putchar(val);
            fflush(stdout);
            // If TX interrupt enabled, assert it
            if (ier_ & IER_THRI)
                set_pending_(IRQ);
            break;
        case IER:
            if (!(lcr_ & 0x80)) {
                ier_ = val & 0x0F;
                // Assert interrupt if RX data waiting
                if ((ier_ & IER_RDI) && !rx_fifo_.empty())
                    set_pending_(IRQ);
                // Assert interrupt immediately if TX interrupt enabled — TX always ready
                if (ier_ & IER_THRI)
                    set_pending_(IRQ);
            }
            break;
        case FCR: {
            fcr_ = val;
            if (val & 0x02) { // RX FIFO reset
                std::lock_guard<std::mutex> lock(rx_mutex_);
                while (!rx_fifo_.empty()) rx_fifo_.pop();
            }
            // Trigger level from bits 6-7
            switch ((val >> 6) & 0x3) {
                case 0: fifo_trigger_ = 1;  break;
                case 1: fifo_trigger_ = 4;  break;
                case 2: fifo_trigger_ = 8;  break;
                case 3: fifo_trigger_ = 14; break;
            }
            break;    
        }
        case LCR: lcr_ = val; break;
        case MCR: mcr_ = val & 0x1F; break;
        case SCR: scr_ = val; break;
        default: break;
    }
}

void UART::rx_push(uint8_t ch) {
    {
        std::lock_guard<std::mutex> lock(rx_mutex_);
        rx_fifo_.push(ch);
    }
    if (ier_ & IER_RDI) {
        set_pending_(IRQ);
    }
}

void UART::load_test_input() {
    if (const char* input = std::getenv("RISCV_EMU_UART_INPUT")) {
        for (const char* p = input; *p != '\0'; ++p) {
            rx_push(static_cast<uint8_t>(*p));
        }
    }
}

void UART::start_input_thread() {
    set_raw_mode();
    running_ = true;
    input_thread_ = std::thread([this]() {
        while (running_) {
            uint8_t ch;
            if (read_char(ch))
                rx_push(ch);
        }
    });
}

void UART::stop_input_thread() {
    running_ = false;
    if (input_thread_.joinable())
        input_thread_.join();
    restore_terminal();
}
