// src/devices/uart.cpp

#include "devices/uart.hpp"
#include <iostream>
#include <cstdlib>

UART::UART(std::function<void(uint32_t)> set_pending_cb)
    : set_pending_(set_pending_cb) {
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
    // RX interrupt takes priority over TX
    if ((ier_ & IER_RDI) && !rx_fifo_.empty())
        return IIR_RDI;
    if (ier_ & IER_THRI)
        return IIR_THRI;
    return IIR_NO_INT;
}

uint8_t UART::read8(uint32_t offset) {
    switch (offset) {
        case RBR: {
            // If DLAB set in LCR, this is divisor latch low — just return 0
            if (lcr_ & 0x80) return 0;
            std::lock_guard<std::mutex> lock(rx_mutex_);
            if (!rx_fifo_.empty()) {
                uint8_t ch = rx_fifo_.front();
                rx_fifo_.pop();
                // If FIFO now empty, interrupt clears naturally on next IIR read
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
                std::lock_guard<std::mutex> lock(rx_mutex_);
                if ((ier_ & IER_RDI) && !rx_fifo_.empty())
                    set_pending_(IRQ);
            }
            break;
        case FCR: break; // FIFO control — accept but ignore
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
    // Assert PLIC interrupt if RX interrupts enabled
    if (ier_ & IER_RDI)
        set_pending_(IRQ);
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
