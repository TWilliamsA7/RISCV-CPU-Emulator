// src/cpu/execute.cpp

#include "cpu/cpu.hpp"

void CPU::execute(const DecodedInstr& i) {

}

void CPU::execADD(const DecodedInstr& i) {
    int32_t a = static_cast<int32_t>(r[i.rs1]);
    int32_t b = static_cast<int32_t>(r[i.rs2]);
    int32_t result = a + b;
    writeReg(i.rd, static_cast<uint32_t>(result));
    pc_ += 4;
}

void CPU::execSUB(const DecodedInstr& i) {
    int32_t a = static_cast<int32_t>(r[i.rs1]);
    int32_t b = static_cast<int32_t>(r[i.rs2]);
    int32_t result = a - b;
    writeReg(i.rd, static_cast<uint32_t>(result));
    pc_ += 4;
}

void CPU::execAND(const DecodedInstr& i) {
    int32_t a = static_cast<int32_t>(r[i.rs1]);
    int32_t b = static_cast<int32_t>(r[i.rs2]);
    int32_t result = a & b;
    writeReg(i.rd, static_cast<uint32_t>(result));
    pc_ += 4;
}

void CPU::execOR(const DecodedInstr& i) {
    int32_t a = static_cast<int32_t>(r[i.rs1]);
    int32_t b = static_cast<int32_t>(r[i.rs2]);
    int32_t result = a | b;
    writeReg(i.rd, static_cast<uint32_t>(result));
    pc_ += 4;
}

void CPU::execXOR(const DecodedInstr& i) {
    int32_t a = static_cast<int32_t>(r[i.rs1]);
    int32_t b = static_cast<int32_t>(r[i.rs2]);
    int32_t result = a ^ b;
    writeReg(i.rd, static_cast<uint32_t>(result));
    pc_ += 4;
}

void CPU::execSLL(const DecodedInstr& i) {
    int32_t a = static_cast<int32_t>(r[i.rs1]);
    int32_t b = static_cast<int32_t>(r[i.rs2]);
    int32_t result = a << b;
    writeReg(i.rd, static_cast<uint32_t>(result));
    pc_ += 4;
}

void CPU::execSRL(const DecodedInstr& i) {
    uint32_t a = r[i.rs1];
    uint32_t b = r[i.rs2];
    uint32_t result = a >> b;
    writeReg(i.rd, static_cast<uint32_t>(result));
    pc_ += 4;
}

void CPU::execSRA(const DecodedInstr& i) {
    int32_t a = static_cast<int32_t>(r[i.rs1]);
    int32_t b = static_cast<int32_t>(r[i.rs2]);
    int32_t result = a >> b;
    writeReg(i.rd, static_cast<uint32_t>(result));
    pc_ += 4;
}

void CPU::execSLT(const DecodedInstr& i) {
    int32_t a = static_cast<int32_t>(r[i.rs1]);
    int32_t b = static_cast<int32_t>(r[i.rs2]);
    writeReg(i.rd, static_cast<uint32_t>(a < b ? 1 : 0));
    pc_ += 4;
}

void CPU::execSLTU(const DecodedInstr& i) {
    uint32_t a = r[i.rs1];
    uint32_t b = r[i.rs2];
    writeReg(i.rd, static_cast<uint32_t>(a < b ? 1 : 0));
    pc_ += 4;
}

void CPU::execADDI(const DecodedInstr& i) {
    int32_t a = static_cast<int32_t>(r[i.rs1]);
    writeReg(i.rd, static_cast<uint32_t>(a + i.imm));
    pc_ += 4;
}

void CPU::execANDI(const DecodedInstr& i) {
    int32_t a = static_cast<int32_t>(r[i.rs1]);
    writeReg(i.rd, static_cast<uint32_t>(a & i.imm));
    pc_ += 4;
}

void CPU::execORI(const DecodedInstr& i) {
    int32_t a = static_cast<int32_t>(r[i.rs1]);
    writeReg(i.rd, static_cast<uint32_t>(a | i.imm));
    pc_ += 4;
}

void CPU::execXORI(const DecodedInstr& i) {
    int32_t a = static_cast<int32_t>(r[i.rs1]);
    writeReg(i.rd, static_cast<uint32_t>(a ^ i.imm));
    pc_ += 4;
}

void CPU::execSLTI(const DecodedInstr& i) {
    int32_t a = static_cast<int32_t>(r[i.rs1]);
    writeReg(i.rd, static_cast<uint32_t>(a < i.imm ? 1 : 0));
    pc_ += 4;
}

void CPU::execSLTIU(const DecodedInstr& i) {
    uint32_t a = r[i.rs1];
    writeReg(i.rd, static_cast<uint32_t>(a < i.imm ? 1 : 0));
    pc_ += 4;
}

void CPU::execSLLI(const DecodedInstr& i) {
    int32_t a = static_cast<int32_t>(r[i.rs1]);
    writeReg(i.rd, static_cast<uint32_t>(a << i.imm));
    pc_ += 4;
}

void CPU::execSRLI(const DecodedInstr& i) {
    uint32_t a = r[i.rs1];
    writeReg(i.rd, static_cast<uint32_t>(a >> i.imm));
    pc_ += 4;
}

void CPU::execSRAI(const DecodedInstr& i) {
    int32_t a = static_cast<int32_t>(r[i.rs1]);
    writeReg(i.rd, static_cast<uint32_t>(a >> i.imm));
    pc_ += 4;
}

void CPU::execLW(const DecodedInstr& i) {
    uint32_t a = r[i.rs1] + i.imm;
    writeReg(i.rd, memory_.read32(a));
    pc_ += 4;
}

void CPU::execLH(const DecodedInstr& i) {
    uint32_t a = r[i.rs1]  + i.imm;
    int16_t half = memory_.read16(a);       
    int32_t s = static_cast<int32_t>(half);
    writeReg(i.rd, static_cast<uint32_t>(s));
    pc_ += 4;
}

void CPU::execLHU(const DecodedInstr& i) {
    uint32_t a = r[i.rs1]  + i.imm;
    uint32_t u = static_cast<uint32_t>(memory_.read16(a));
    writeReg(i.rd, u);
    pc_ += 4;
}

void CPU::execLB(const DecodedInstr& i) {
    uint32_t a = r[i.rs1]  + i.imm;
    int8_t byte = memory_.read8(a);       
    int32_t s = static_cast<int32_t>(byte);
    writeReg(i.rd, static_cast<uint32_t>(s));
    pc_ += 4;
}

void CPU::execLBU(const DecodedInstr& i) {
    uint32_t a = r[i.rs1] + i.imm;
    uint32_t u = static_cast<uint32_t>(memory_.read8(a));
    writeReg(i.rd, u);
    pc_ += 4;
}

void CPU::execSW(const DecodedInstr& i) {
    uint32_t a = r[i.rs1] + i.imm;
    memory_.write32(a, r[i.rs2]);
    pc_ += 4;
}

void CPU::execSH(const DecodedInstr& i) {
    uint32_t a = r[i.rs1] + i.imm;
    memory_.write16(a, static_cast<uint16_t>(r[i.rs2]));
    pc_ += 4;
}

void CPU::execSB(const DecodedInstr& i) {
    uint32_t a = r[i.rs1] + i.imm;
    memory_.write8(a, static_cast<uint8_t>(r[i.rs2]));
    pc_ += 4;
}

void CPU::execBEQ(const DecodedInstr& i) {

}

void CPU::execBNE(const DecodedInstr& i) {

}

void CPU::execBLT(const DecodedInstr& i) {

}

void CPU::execBGE(const DecodedInstr& i) {

}

void CPU::execBLTU(const DecodedInstr& i) {

}

void CPU::execBGEU(const DecodedInstr& i) {

}

void CPU::execJAL(const DecodedInstr& i) {

}

void CPU::execJALR(const DecodedInstr& i) {

}

void CPU::execLUI(const DecodedInstr& i) {

}

void CPU::execAUIPC(const DecodedInstr& i) {

}
