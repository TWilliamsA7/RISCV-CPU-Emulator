// src/devices/uart/uart_posix.cpp

#include "platform/platform.hpp"

#ifdef PLATFORM_POSIX

#include "devices/uart.hpp"
#include "emulator.hpp"
#include <unistd.h>
#include <termios.h>
#include <csignal>
#include <sys/select.h>
#include "cpu/cpu.hpp"

UART* UART::s_instance = nullptr;
static struct termios g_old_termios;
static bool g_have_old_termios = false;

static void restore_termios() {
    if (g_have_old_termios)
        tcsetattr(STDIN_FILENO, TCSANOW, &g_old_termios);
}

void UART::signal_restore(int sig) {
    restore_termios();
    
    if (s_instance)
        s_instance->sys_.cpu.halt();
}

void UART::set_raw_mode() {
    struct termios t;
    if (tcgetattr(STDIN_FILENO, &g_old_termios) != 0)
        return;

    g_have_old_termios = true;
    s_instance = this;
    t = g_old_termios;
    t.c_lflag &= ~(ICANON | ECHO | ISIG); // add ISIG — disables Ctrl+C/Z signal generation
    t.c_cc[VMIN]  = 1;
    t.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
    // Keep SIGTERM for clean shutdown from external kill command
    std::signal(SIGTERM, signal_restore);
    // Remove SIGINT handler — Ctrl+C now goes to guest
}

void UART::restore_terminal() {
    restore_termios();
    std::signal(SIGINT, SIG_DFL);
    std::signal(SIGTERM, SIG_DFL);
}

bool UART::read_char(uint8_t& ch) {
    uint8_t byte;
    ssize_t n = read(STDIN_FILENO, &byte, 1);
    if (n <= 0) return false;

    static bool escape_mode = false;

    if (escape_mode) {
        escape_mode = false;
        if (byte == 'q' || byte == 'Q') {
            // Ctrl+A then Q — quit emulator
            restore_termios();
            if (s_instance)
                s_instance->sys_.cpu.halt();
            return false;
        } else if (byte == 0x01) {
            // Ctrl+A twice — send a literal Ctrl+A to the guest
            ch = 0x01;
            return true;
        } else {
            // Unrecognized sequence — send both bytes to guest
            // Send the Ctrl+A first, then fall through to send current byte
            if (s_instance)
                s_instance->rx_push(0x01);
            ch = byte;
            return true;
        }
    }

    if (byte == 0x01) { // Ctrl+A
        escape_mode = true;
        return false;
    }

    // Pass Ctrl+C and everything else straight to the guest
    ch = byte;
    return true;
}

#endif

