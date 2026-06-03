// src/devices/uart/uart_win.cpp

#include "platform/platform.hpp"

#ifdef PLATFORM_WINDOWS

#include "devices/uart.hpp"
#include "windows.h"
#include "conio.h"
#include <atomic>
#include <chrono>
#include <thread>

static HANDLE g_stdin_handle = INVALID_HANDLE_VALUE;
static DWORD g_old_mode = 0;
static std::atomic<bool> g_have_old_mode{false};

static void restore_console_mode() {
    if (g_have_old_mode && g_stdin_handle != INVALID_HANDLE_VALUE)
        SetConsoleMode(g_stdin_handle, g_old_mode);
}

static BOOL WINAPI console_ctrl_handler(DWORD type) {
    if (type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT ||
        type == CTRL_CLOSE_EVENT || type == CTRL_LOGOFF_EVENT ||
        type == CTRL_SHUTDOWN_EVENT) {
        restore_console_mode();
        return FALSE;
    }
    return FALSE;
}

void UART::set_raw_mode() {
    g_stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
    if (g_stdin_handle == INVALID_HANDLE_VALUE)
        return;

    if (GetConsoleMode(g_stdin_handle, &g_old_mode)) {
        g_have_old_mode = true;
        DWORD new_mode = g_old_mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
        new_mode |= ENABLE_PROCESSED_INPUT;
        SetConsoleMode(g_stdin_handle, new_mode);
        SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
    }
}

void UART::restore_terminal() {
    restore_console_mode();
    SetConsoleCtrlHandler(console_ctrl_handler, FALSE);
}

bool UART::read_char(uint8_t& ch) {
    if (!_kbhit()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return false;
    }

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

