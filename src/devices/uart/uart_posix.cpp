// src/devices/uart/uart_posix.cpp

#include "platform/platform.hpp"

#ifdef PLATFORM_POSIX

#include "devices/uart.hpp"
#include <unistd.h>
#include <termios.h>

static struct termios g_old_termios;

void UART::set_raw_mode() {
    struct termios t;
    tcgetattr(STDIN_FILENO, &g_old_termios);
    t = g_old_termios;
    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN]  = 1;
    t.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void UART::restore_terminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &g_old_termios);
}

bool UART::read_char(uint8_t& ch) {
    int n = read(STDIN_FILENO, &ch, 1);
    return n == 1;
}

#endif

