// tests/fixtures/cpuTest.hpp

#include <gtest/gtest.h>
#include "cpu/cpu.hpp"

class CPUTest : public ::testing::Test {
    protected:
        Memory mem{100};
        CPU cpu{mem, true};

        void loadProgram(const std::vector<uint8_t>& program) {
            mem.loadBinary(program, 0x0);
        }

};