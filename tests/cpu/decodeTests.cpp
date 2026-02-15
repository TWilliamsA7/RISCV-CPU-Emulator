// tests/cpu/decodeTests.cpp

#include <gtest/gtest.h>
#include "cpu/decode.hpp"

TEST(Decode, DecodeAdd) {
    uint32_t  instr = 0x003100B3;

    auto d = decode(instr);

    EXPECT_EQ(d.kind, InstrKind::ADD);
    EXPECT_EQ(d.rd, 1);
    EXPECT_EQ(d.rs1, 2);
    EXPECT_EQ(d.rs2, 3);
    EXPECT_EQ(d.imm, 0);
}

TEST(Decode, DecodeSub) {
    uint32_t  instr = 0x407303B3;

    auto d = decode(instr);

    EXPECT_EQ(d.kind, InstrKind::SUB);
    EXPECT_EQ(d.rd, 7);
    EXPECT_EQ(d.rs1, 6);
    EXPECT_EQ(d.rs2, 7);
    EXPECT_EQ(d.imm, 0);
}

TEST(Decode, DecodeAddI) {
    uint32_t  instr = 0xFFF10093;

    auto d = decode(instr);

    EXPECT_EQ(d.kind, InstrKind::ADDI);
    EXPECT_EQ(d.rd, 1);
    EXPECT_EQ(d.rs1, 2);
    EXPECT_EQ(d.imm, -1);
}

TEST(Decode, DecodeLW) {
    uint32_t  instr = 0x00822183;

    auto d = decode(instr);

    EXPECT_EQ(d.kind, InstrKind::LW);
    EXPECT_EQ(d.rd, 3);
    EXPECT_EQ(d.rs1, 4);
    EXPECT_EQ(d.imm, 8);
}

TEST(Decode, DecodeSW) {
    uint32_t  instr = 0x00322423;

    auto d = decode(instr);

    EXPECT_EQ(d.kind, InstrKind::SW);
    EXPECT_EQ(d.rd, 0);
    EXPECT_EQ(d.rs1, 4);
    EXPECT_EQ(d.rs2, 3);
    EXPECT_EQ(d.imm, 8);
}

TEST(Decode, DecodeBEQ) {
    uint32_t  instr = 0x00208863;

    auto d = decode(instr);

    EXPECT_EQ(d.kind, InstrKind::BEQ);
    EXPECT_EQ(d.rs1, 1);
    EXPECT_EQ(d.rs2, 2);
    EXPECT_EQ(d.imm, 16);
}

TEST(Decode, DecodeLUI) {
    uint32_t  instr = 0x123452B7;

    auto d = decode(instr);

    EXPECT_EQ(d.kind, InstrKind::LUI);
    EXPECT_EQ(d.rd, 5);
    EXPECT_EQ(d.imm, 0x12345000);
}

TEST(Decode, DecodeJAL) {
    uint32_t  instr = 0x020000EF;

    auto d = decode(instr);

    EXPECT_EQ(d.kind, InstrKind::JAL);
    EXPECT_EQ(d.rd, 1);
    EXPECT_EQ(d.imm, 32);
}

TEST(Decode, DecodeInvalid) {
    uint32_t  instr = 0x0;

    auto d = decode(instr);

    EXPECT_EQ(d.kind, InstrKind::INVALID);
}