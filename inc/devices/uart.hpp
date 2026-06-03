// inc/devices/uart.hpp

#pragma once

#include <cstdint>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>

class UART {
    public:
        static constexpr uint32_t BASE = 0x10000000;
        static constexpr uint32_t SIZE = 0x100;
        static constexpr uint32_t IRQ  = 10; // PLIC source 10

        // 16550 register offsets
        static constexpr uint32_t RBR = 0x0; // Receiver Buffer (read)
        static constexpr uint32_t THR = 0x0; // Transmit Holding (write)
        static constexpr uint32_t IER = 0x1; // Interrupt Enable
        static constexpr uint32_t IIR = 0x2; // Interrupt Identification (read)
        static constexpr uint32_t FCR = 0x2; // FIFO Control (write)
        static constexpr uint32_t LCR = 0x3; // Line Control
        static constexpr uint32_t MCR = 0x4; // Modem Control
        static constexpr uint32_t LSR = 0x5; // Line Status
        static constexpr uint32_t MSR = 0x6; // Modem Status
        static constexpr uint32_t SCR = 0x7; // Scratch

        // LSR bits
        static constexpr uint8_t LSR_DR   = 0x01; // Data Ready
        static constexpr uint8_t LSR_THRE = 0x20; // TX Holding Register Empty
        static constexpr uint8_t LSR_TEMT = 0x40; // TX Empty

        // IER bits
        static constexpr uint8_t IER_RDI  = 0x01; // RX Data Interrupt enable
        static constexpr uint8_t IER_THRI = 0x02; // TX Holding Register Empty interrupt enable

        // IIR values
        static constexpr uint8_t IIR_NO_INT = 0x01; // No interrupt pending
        static constexpr uint8_t IIR_THRI   = 0x02; // TX interrupt
        static constexpr uint8_t IIR_RDI    = 0x04; // RX interrupt

        explicit UART(std::function<void(uint32_t)> set_pending_cb);
        ~UART();

        uint8_t read8(uint32_t offset);
        void write8(uint32_t offset, uint8_t val);

        // Feed a character into the RX FIFO (called from input thread)
        void rx_push(uint8_t ch);

        void start_input_thread();
        void stop_input_thread();

        void load_test_input();

    private:
        uint8_t ier_ = 0;
        uint8_t lcr_ = 0;
        uint8_t mcr_ = 0;
        uint8_t scr_ = 0;

        std::queue<uint8_t> rx_fifo_;
        std::mutex          rx_mutex_;

        std::thread         input_thread_;
        std::atomic<bool>   running_{false};

        // Callback to assert PLIC interrupt
        std::function<void(uint32_t)> set_pending_;

        uint8_t read_lsr();
        uint8_t read_iir();

        // Platform-specific: save/restore terminal state

        void set_raw_mode();
        void restore_terminal();
        static bool read_char(uint8_t& ch);
};
