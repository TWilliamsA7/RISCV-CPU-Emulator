// tests/cpu/stepTests.cpp

#include <gtest/gtest.h>
#include "../tests/fixtures/cpuTest.hpp"

TEST_F(CPUTest, ImmutableX0) {
    // addi x0, x0, 5
    loadProgram({ 0x13, 0x00, 0x50, 0x00 });

    StepResult sr = cpu.step();
    EXPECT_EQ(sr.pc_before, 0);
    EXPECT_EQ(sr.pc_after, 4);
    EXPECT_EQ(sr.dInstr.kind, InstrKind::ADDI);
    EXPECT_EQ(cpu.reg(0x0), 0x0);
}

TEST_F(CPUTest, HandleADDI) {

    // addi x1, x0, 18
    loadProgram({ 0x93, 0x00, 0x20, 0x01 });

    StepResult sr = cpu.step();

    EXPECT_EQ(sr.pc_before, 0);
    EXPECT_EQ(sr.pc_after, 4);
    EXPECT_EQ(sr.dInstr.kind, InstrKind::ADDI);
    EXPECT_EQ(cpu.reg(0x1), 0x12);

}

TEST_F(CPUTest, HandleInvalid) {

    // addi x1, x0, 18
    loadProgram({ 0x00, 0x00, 0x00, 0x00 });

    StepResult sr = cpu.step();

    EXPECT_EQ(sr.pc_before, 0);
    EXPECT_EQ(sr.pc_after, 0);
    EXPECT_EQ(sr.dInstr.kind, InstrKind::INVALID);
    EXPECT_EQ(cpu.isHalted(), true);

}