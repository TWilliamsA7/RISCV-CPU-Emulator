add_library(mmu
    mmu.cpp
    tlb.cpp
)

target_include_directories(mmu PUBLIC
    ${PROJECT_SOURCE_DIR}/inc
)
