add_library(devices
    uart/uart.cpp
    uart/uart_win.cpp
    uart/uart_posix.cpp
    virtio_blk.cpp
    virtio_net/virtio_net.cpp
    virtio_net/virtio_net_win.cpp
    virtio_net/virtio_net_posix.cpp
)

target_include_directories(devices PUBLIC
    ${PROJECT_SOURCE_DIR}/inc
)
