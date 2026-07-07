find_package(PkgConfig REQUIRED)
pkg_check_modules(SLIRP REQUIRED slirp)
pkg_check_modules(GLIB  REQUIRED glib-2.0)
pkg_check_modules(SDL2 REQUIRED sdl2)

add_library(devices
    uart/uart.cpp
    uart/uart_win.cpp
    uart/uart_posix.cpp
    virtio_blk.cpp
    virtio_net/virtio_net.cpp
    virtio_net/virtio_net_slirp.cpp
    framebuffer/framebuffer.cpp
    framebuffer/framebuffer_sdl.cpp
)

target_include_directories(devices PUBLIC
    ${PROJECT_SOURCE_DIR}/inc
    ${SLIRP_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(devices PUBLIC
    ${SLIRP_LINK_LIBRARIES}
    ${GLIB_LINK_LIBRARIES}
    ${SDL2_LINK_LIBRARIES}
)

if(WIN32)
    target_link_libraries(devices PUBLIC ws2_32)
endif()