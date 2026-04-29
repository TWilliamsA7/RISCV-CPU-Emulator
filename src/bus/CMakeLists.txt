add_library(bus
    bus.cpp
)

target_include_directories(bus PUBLIC
    ${PROJECT_SOURCE_DIR}/inc
)
