// src/cpu/execute.cpp

#include "cpu/cpu.hpp"
#include <iostream>

const std::array<CPU::ExecFn, static_cast<size_t>(InstrKind::COUNT)> CPU::dispatch_ = {
    &CPU::execADD,
    &CPU::execSUB,
    &CPU::execAND,
    &CPU::execOR,
    &CPU::execXOR,
    &CPU::execSLL,
    &CPU::execSRL,
    &CPU::execSRA,
    &CPU::execSLT,
    &CPU::execSLTU,
    &CPU::execADDI,
    &CPU::execANDI,
    &CPU::execORI,
    &CPU::execXORI,
    &CPU::execSLTI,
    &CPU::execSLTIU,
    &CPU::execSLLI,
    &CPU::execSRLI,
    &CPU::execSRAI,
    &CPU::execLW,
    &CPU::execLH,
    &CPU::execLHU,
    &CPU::execLB,
    &CPU::execLBU,
    &CPU::execSW,
    &CPU::execSH,
    &CPU::execSB,
    &CPU::execBEQ,
    &CPU::execBNE,
    &CPU::execBLT,
    &CPU::execBGE,
    &CPU::execBLTU,
    &CPU::execBGEU,
    &CPU::execJAL,
    &CPU::execJALR,
    &CPU::execLUI,
    &CPU::execAUIPC,
    &CPU::execECALL,
    &CPU::execEBREAK,
    &CPU::execFENCE,
    &CPU::execCSRRW,
    &CPU::execCSRRS,
    &CPU::execCSRRC,
    &CPU::execCSRRWI,
    &CPU::execCSRRSI,
    &CPU::execCSRRCI,
    &CPU::execINVALID,
};


void CPU::execute(const DecodedInstr& i) {
    auto idx = static_cast<size_t>(i.kind);
    (this->*dispatch_[idx])(i);
}

void CPU::execADD(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    int32_t b = static_cast<int32_t>(regs_[i.rs2]);
    int32_t result = a + b;
    writeReg(i.rd, static_cast<uint32_t>(result));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    pc_ += 4;
}

void CPU::execSUB(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    int32_t b = static_cast<int32_t>(regs_[i.rs2]);
    int32_t result = a - b;
    writeReg(i.rd, static_cast<uint32_t>(result));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    pc_ += 4;
}

void CPU::execAND(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    int32_t b = static_cast<int32_t>(regs_[i.rs2]);
    int32_t result = a & b;
    writeReg(i.rd, static_cast<uint32_t>(result));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    pc_ += 4;
}

void CPU::execOR(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    int32_t b = static_cast<int32_t>(regs_[i.rs2]);
    int32_t result = a | b;
    writeReg(i.rd, static_cast<uint32_t>(result));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    pc_ += 4;
}

void CPU::execXOR(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    int32_t b = static_cast<int32_t>(regs_[i.rs2]);
    int32_t result = a ^ b;
    writeReg(i.rd, static_cast<uint32_t>(result));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    pc_ += 4;
}

void CPU::execSLL(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    int32_t b = static_cast<int32_t>(regs_[i.rs2]);
    int32_t result = a << b;
    writeReg(i.rd, static_cast<uint32_t>(result));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    pc_ += 4;
}

void CPU::execSRL(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    uint32_t a = regs_[i.rs1];
    uint32_t b = regs_[i.rs2];
    uint32_t result = a >> b;
    writeReg(i.rd, static_cast<uint32_t>(result));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    pc_ += 4;
}

void CPU::execSRA(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    int32_t b = static_cast<int32_t>(regs_[i.rs2]);
    int32_t result = a >> b;
    writeReg(i.rd, static_cast<uint32_t>(result));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    pc_ += 4;
}

void CPU::execSLT(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    int32_t b = static_cast<int32_t>(regs_[i.rs2]);
    writeReg(i.rd, static_cast<uint32_t>(a < b ? 1 : 0));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    pc_ += 4;
}

void CPU::execSLTU(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    uint32_t a = regs_[i.rs1];
    uint32_t b = regs_[i.rs2];
    writeReg(i.rd, static_cast<uint32_t>(a < b ? 1 : 0));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    pc_ += 4;
}

void CPU::execADDI(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    writeReg(i.rd, static_cast<uint32_t>(a + i.imm));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    pc_ += 4;
}

void CPU::execANDI(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    writeReg(i.rd, static_cast<uint32_t>(a & i.imm));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    pc_ += 4;
}

void CPU::execORI(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    writeReg(i.rd, static_cast<uint32_t>(a | i.imm));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    pc_ += 4;
}

void CPU::execXORI(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    writeReg(i.rd, static_cast<uint32_t>(a ^ i.imm));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    pc_ += 4;
}

void CPU::execSLTI(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    writeReg(i.rd, static_cast<uint32_t>(a < i.imm ? 1 : 0));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    pc_ += 4;
}

void CPU::execSLTIU(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    uint32_t a = regs_[i.rs1];
    writeReg(i.rd, static_cast<uint32_t>(a < i.imm ? 1 : 0));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    pc_ += 4;
}

void CPU::execSLLI(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    writeReg(i.rd, static_cast<uint32_t>(a << i.imm));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    pc_ += 4;
}

void CPU::execSRLI(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    uint32_t a = regs_[i.rs1];
    writeReg(i.rd, static_cast<uint32_t>(a >> i.imm));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    pc_ += 4;
}

void CPU::execSRAI(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    writeReg(i.rd, static_cast<uint32_t>(a >> i.imm));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    pc_ += 4;
}

void CPU::execLW(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    uint32_t a = regs_[i.rs1] + i.imm;
    writeReg(i.rd, bus_.read32(a));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    pc_ += 4;
}

void CPU::execLH(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    uint32_t a = regs_[i.rs1]  + i.imm;
    int16_t half = bus_.read16(a);       
    int32_t s = static_cast<int32_t>(half);
    writeReg(i.rd, static_cast<uint32_t>(s));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    pc_ += 4;
}

void CPU::execLHU(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    uint32_t a = regs_[i.rs1]  + i.imm;
    uint32_t u = static_cast<uint32_t>(bus_.read16(a));
    writeReg(i.rd, u);
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    pc_ += 4;
}

void CPU::execLB(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    uint32_t a = regs_[i.rs1]  + i.imm;
    int8_t byte = bus_.read8(a);       
    int32_t s = static_cast<int32_t>(byte);
    writeReg(i.rd, static_cast<uint32_t>(s));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    pc_ += 4;
}

void CPU::execLBU(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    uint32_t a = regs_[i.rs1] + i.imm;
    uint32_t u = static_cast<uint32_t>(bus_.read8(a));
    writeReg(i.rd, u);
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    pc_ += 4;
}

void CPU::execSW(const DecodedInstr& i) {
    uint32_t a = regs_[i.rs1] + i.imm;
    uint32_t o = bus_.read32(a);
    bus_.write32(a, regs_[i.rs2]);
    sr.mem_write = MemWrite{ a, o, regs_[i.rs2], 4};
    pc_ += 4;
}

void CPU::execSH(const DecodedInstr& i) {
    uint32_t a = regs_[i.rs1] + i.imm;
    uint32_t o = static_cast<uint32_t>(bus_.read16(a));
    bus_.write16(a, static_cast<uint16_t>(regs_[i.rs2]));
    sr.mem_write = MemWrite{ a, o, regs_[i.rs2], 2};
    pc_ += 4;
}

void CPU::execSB(const DecodedInstr& i) {
    uint32_t a = regs_[i.rs1] + i.imm;
    uint32_t o = static_cast<uint32_t>(bus_.read8(a));
    bus_.write8(a, static_cast<uint8_t>(regs_[i.rs2]));
    sr.mem_write = MemWrite{ a, o, regs_[i.rs2], 1};
    pc_ += 4;
}

void CPU::execBEQ(const DecodedInstr& i) {
    if (regs_[i.rs1] == regs_[i.rs2])
        pc_ += i.imm;
    else
        pc_ += 4;
}

void CPU::execBNE(const DecodedInstr& i) {
    if (regs_[i.rs1] != regs_[i.rs2])
        pc_ += i.imm;
    else
        pc_ += 4;
}

void CPU::execBLT(const DecodedInstr& i) {
    int32_t r1 = static_cast<int32_t>(regs_[i.rs1]);
    int32_t r2 = static_cast<int32_t>(regs_[i.rs2]);
    if (r1 < r2)
        pc_ += i.imm;
    else
        pc_ += 4;
}

void CPU::execBGE(const DecodedInstr& i) {
    int32_t r1 = static_cast<int32_t>(regs_[i.rs1]);
    int32_t r2 = static_cast<int32_t>(regs_[i.rs2]);
    if (r1 >= r2)
        pc_ += i.imm;
    else
        pc_ += 4;
}

void CPU::execBLTU(const DecodedInstr& i) {
    if (regs_[i.rs1] < regs_[i.rs2])
        pc_ += i.imm;
    else
        pc_ += 4;
}

void CPU::execBGEU(const DecodedInstr& i) {
    if (regs_[i.rs1] >= regs_[i.rs2])
        pc_ += i.imm;
    else
        pc_ += 4;
}

void CPU::execJAL(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    writeReg(i.rd, pc_ + 4);
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd] };
    pc_ += i.imm;
}

void CPU::execJALR(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    writeReg(i.rd, pc_ + 4);
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd] };
    pc_ = (regs_[i.rs1] + i.imm) &~ 1;
}

void CPU::execLUI(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    writeReg(i.rd, static_cast<uint32_t>(i.imm));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd] };
    pc_ += 4;
}

void CPU::execAUIPC(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    writeReg(i.rd, static_cast<uint32_t>(pc_ + i.imm));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd] };
    pc_ += 4;
}

void CPU::execECALL(const DecodedInstr& i) {
    uint32_t syscall_id = regs_[17]; // a7
    if (syscall_id == 93) { // SYS_exit
        std::cout << "Program exited via ECALL with code: " << regs_[10] << std::endl;
        halted = true;
    } else {
        std::cout << "Unhandled ECALL: " << syscall_id << std::endl;
    }
    pc_ += 4;
}

void CPU::execEBREAK(const DecodedInstr& i) {
    pc_ += 4;
}

void CPU::execFENCE(const DecodedInstr& i) {
    pc_ += 4;
}

void CPU::execCSRRW(const DecodedInstr& i) {
    uint32_t csr_addr = i.imm;
    uint32_t old_val = csrs_[csr_addr];
    
    csrs_[csr_addr] = regs_[i.rs1];
    writeReg(i.rd, old_val);
    pc_ += 4;
}

void CPU::execCSRRS(const DecodedInstr& i) {
    pc_ += 4;
}

void CPU::execCSRRC(const DecodedInstr& i) {
    pc_ += 4;
}

void CPU::execCSRRWI(const DecodedInstr& i) {
    pc_ += 4;
}

void CPU::execCSRRSI(const DecodedInstr& i) {
    pc_ += 4;
}

void CPU::execCSRRCI(const DecodedInstr& i) {
    pc_ += 4;
}

void CPU::execINVALID(const DecodedInstr& i) {
    pc_ += 4;
}
