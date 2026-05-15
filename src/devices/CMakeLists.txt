add_library(devices
    uart.cpp
)

target_include_directories(devices PUBLIC
    ${PROJECT_SOURCE_DIR}/inc
)
