add_library(plic
    plic.cpp
)

target_include_directories(plic PUBLIC
    ${PROJECT_SOURCE_DIR}/inc
)
