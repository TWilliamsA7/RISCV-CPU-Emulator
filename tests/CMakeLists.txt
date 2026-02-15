file(GLOB_RECURSE TEST_SOURCES "*.cpp")

add_executable(run_tests ${TEST_SOURCES})

target_link_libraries(run_tests
    cpu
    core
    memory
    gtest
    gtest_main
)

include(GoogleTest)
gtest_discover_tests(run_tests)
