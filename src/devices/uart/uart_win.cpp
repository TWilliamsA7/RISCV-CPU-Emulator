// src/devices/uart/uart_win.cpp

#include "platform/platform.hpp"

#ifdef PLATFORM_WINDOWS

#include "devices/uart.hpp"
#include "windows.h"
#include "conio.h"

static HANDLE g_stdin_handle = INVALID_HANDLE_VALUE;
static DWORD g_old_mode = 0;

void UART::set_raw_mode() {
    g_stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(g_stdin_handle, &g_old_mode);
    DWORD new_mode = g_old_mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    SetConsoleMode(g_stdin_handle, new_mode);
}

void UART::restore_terminal() {
    if (g_stdin_handle != INVALID_HANDLE_VALUE)
        SetConsoleMode(g_stdin_handle, g_old_mode);
}

bool UART::read_char(uint8_t& ch) {
    int c = _getch();
    if (c == EOF) return false;
    if (c == 0 || c == 0xE0) {
        _getch();
        return false;
    }
    ch = static_cast<uint8_t>(c);
    return true;
}

#endif

