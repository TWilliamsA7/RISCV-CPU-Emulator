add_library(elf
    elf.cpp
)

target_include_directories(elf PUBLIC
    ${PROJECT_SOURCE_DIR}/inc
)