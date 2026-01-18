add_library(platform
    # Insert .cpp files
)

target_include_directories(platform PUBLIC
    ${PROJECT_SOURCE_DIR}/inc
)

target_link_libraries(platform PUBLIC
    core 
    memory
)