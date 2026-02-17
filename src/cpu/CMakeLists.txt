add_library(cpu
    cpu.cpp
    execute.cpp
    decode.cpp
)

target_include_directories(cpu PUBLIC
    ${PROJECT_SOURCE_DIR}/inc
)

target_link_libraries(cpu PUBLIC
    core 
    isa
)