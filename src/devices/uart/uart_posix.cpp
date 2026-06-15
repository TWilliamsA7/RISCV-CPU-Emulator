// src/devices/uart/uart_posix.cpp

#include "platform/platform.hpp"

#ifdef PLATFORM_POSIX

#include "devices/uart.hpp"
#include <unistd.h>
#include <termios.h>
#include <csignal>
#include <sys/select.h>
#include "cpu/cpu.hpp"

static struct termios g_old_termios;
static bool g_have_old_termios = false;

static void restore_termios() {
    if (g_have_old_termios)
        tcsetattr(STDIN_FILENO, TCSANOW, &g_old_termios);
}

static void signal_restore(int sig) {
    restore_termios();
    
    if (g_cpu)
        g_cpu->halt();
}

void UART::set_raw_mode() {
    struct termios t;
    if (tcgetattr(STDIN_FILENO, &g_old_termios) != 0)
        return;

    g_have_old_termios = true;
    t = g_old_termios;
    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN]  = 1;
    t.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
    std::signal(SIGINT, signal_restore);
    std::signal(SIGTERM, signal_restore);
}

void UART::restore_terminal() {
    restore_termios();
    std::signal(SIGINT, SIG_DFL);
    std::signal(SIGTERM, SIG_DFL);
}

bool UART::read_char(uint8_t& ch) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    struct timeval tv {};
    tv.tv_sec = 0;
    tv.tv_usec = 10000;

    int ready = select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv);
    if (ready <= 0)
        return false;

    int n = read(STDIN_FILENO, &ch, 1);
    return n == 1;
}

#endif

