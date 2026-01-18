file(GLOB TEST_SOURCES "*.cpp")

add_executable(run_tests ${TEST_SOURCES})

target_link_libraries(run_tests
    cpu
    core
    memory
    isa
    gtest
    gtest_main
)

add_test(NAME riscv_tests COMMAND run_tests)
