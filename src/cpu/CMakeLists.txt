add_library(cpu
    # Insert .cpp files
)

target_include_directories(cpu PUBLIC
    ${PROJECT_SOURCE_DIR}/inc
)

target_link_libraries(cpu PUBLIC
    core 
    isa
)