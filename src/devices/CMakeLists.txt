add_library(devices
    uart/uart.cpp
    uart/uart_win.cpp
    uart/uart_posix.cpp
)

target_include_directories(devices PUBLIC
    ${PROJECT_SOURCE_DIR}/inc
)
