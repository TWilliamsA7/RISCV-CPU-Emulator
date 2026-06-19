add_library("cache"
    icache.cpp
)

target_include_directories("cache" PUBLIC
    ${PROJECT_SOURCE_DIR}/inc
)
