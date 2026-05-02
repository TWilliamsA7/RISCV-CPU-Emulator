add_library(clint
    clint.cpp
)

target_include_directories(clint PUBLIC
    ${PROJECT_SOURCE_DIR}/inc
)
