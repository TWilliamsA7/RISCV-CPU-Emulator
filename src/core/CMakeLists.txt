add_library(core
    core.cpp
)

target_include_directories(core PUBLIC
    ${PROJECT_SOURCE_DIR}/inc
)

target_link_libraries(core PUBLIC
    isa
    memory
)