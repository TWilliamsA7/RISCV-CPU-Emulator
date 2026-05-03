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
    &CPU::execSLT,
    &CPU::execSLTU,
    &CPU::execSRL,
    &CPU::execSRA,
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
    &CPU::execMUL,
    &CPU::execMULH,
    &CPU::execMULHSU,
    &CPU::execMULHU,
    &CPU::execDIV,
    &CPU::execDIVU,
    &CPU::execREM,
    &CPU::execREMU,
    &CPU::execMRET,
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
    
}

void CPU::execSUB(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    int32_t b = static_cast<int32_t>(regs_[i.rs2]);
    int32_t result = a - b;
    writeReg(i.rd, static_cast<uint32_t>(result));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execAND(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    int32_t b = static_cast<int32_t>(regs_[i.rs2]);
    int32_t result = a & b;
    writeReg(i.rd, static_cast<uint32_t>(result));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execOR(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    int32_t b = static_cast<int32_t>(regs_[i.rs2]);
    int32_t result = a | b;
    writeReg(i.rd, static_cast<uint32_t>(result));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execXOR(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    int32_t b = static_cast<int32_t>(regs_[i.rs2]);
    int32_t result = a ^ b;
    writeReg(i.rd, static_cast<uint32_t>(result));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execSLL(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    int32_t b = static_cast<int32_t>(regs_[i.rs2]);
    int32_t result = a << b;
    writeReg(i.rd, static_cast<uint32_t>(result));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execSRL(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    uint32_t a = regs_[i.rs1];
    uint32_t b = regs_[i.rs2];
    uint32_t result = a >> b;
    writeReg(i.rd, static_cast<uint32_t>(result));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execSRA(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    int32_t b = static_cast<int32_t>(regs_[i.rs2]);
    int32_t result = a >> b;
    writeReg(i.rd, static_cast<uint32_t>(result));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execSLT(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    int32_t b = static_cast<int32_t>(regs_[i.rs2]);
    writeReg(i.rd, static_cast<uint32_t>(a < b ? 1 : 0));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execSLTU(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    uint32_t a = regs_[i.rs1];
    uint32_t b = regs_[i.rs2];
    writeReg(i.rd, static_cast<uint32_t>(a < b ? 1 : 0));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execADDI(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    writeReg(i.rd, static_cast<uint32_t>(a + i.imm));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execANDI(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    writeReg(i.rd, static_cast<uint32_t>(a & i.imm));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execORI(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    writeReg(i.rd, static_cast<uint32_t>(a | i.imm));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execXORI(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    writeReg(i.rd, static_cast<uint32_t>(a ^ i.imm));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execSLTI(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    writeReg(i.rd, static_cast<uint32_t>(a < i.imm ? 1 : 0));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execSLTIU(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    uint32_t a = regs_[i.rs1];
    writeReg(i.rd, static_cast<uint32_t>(a < i.imm ? 1 : 0));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execSLLI(const DecodedInstr& i) {
    if ((i.imm >> 5) != 0) {
        trap(2, i.raw, false);
        return;
    }

    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    writeReg(i.rd, static_cast<uint32_t>(a << i.imm));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execSRLI(const DecodedInstr& i) {
     if ((i.imm >> 5) != 0) {
        trap(2, i.raw, false);
        return;
    }

    uint32_t o = regs_[i.rd];
    uint32_t a = regs_[i.rs1];
    writeReg(i.rd, static_cast<uint32_t>(a >> i.imm));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execSRAI(const DecodedInstr& i) {
    if ((i.imm >> 5) != 0x20) {
        trap(2, i.raw, false);
        return;
    }

    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    writeReg(i.rd, static_cast<uint32_t>(a >> i.imm));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execLW(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    uint32_t a = regs_[i.rs1] + i.imm;
    writeReg(i.rd, bus_.read32(a));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execLH(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    uint32_t a = regs_[i.rs1]  + i.imm;
    int16_t half = bus_.read16(a);       
    int32_t s = static_cast<int32_t>(half);
    writeReg(i.rd, static_cast<uint32_t>(s));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execLHU(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    uint32_t a = regs_[i.rs1]  + i.imm;
    uint32_t u = static_cast<uint32_t>(bus_.read16(a));
    writeReg(i.rd, u);
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execLB(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    uint32_t a = regs_[i.rs1]  + i.imm;
    int8_t byte = static_cast<int8_t>(bus_.read8(a));
    writeReg(i.rd, static_cast<uint32_t>(static_cast<int32_t>(byte)));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execLBU(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    uint32_t a = regs_[i.rs1] + i.imm;
    uint32_t u = static_cast<uint32_t>(bus_.read8(a));
    writeReg(i.rd, u);
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execSW(const DecodedInstr& i) {
    uint32_t a = regs_[i.rs1] + i.imm;
    uint32_t o = bus_.read32(a);
    bus_.write32(a, regs_[i.rs2]);
    sr.mem_write = MemWrite{ a, o, regs_[i.rs2], 4};
    
}

void CPU::execSH(const DecodedInstr& i) {
    uint32_t a = regs_[i.rs1] + i.imm;
    uint32_t o = static_cast<uint32_t>(bus_.read16(a));
    bus_.write16(a, static_cast<uint16_t>(regs_[i.rs2]));
    sr.mem_write = MemWrite{ a, o, regs_[i.rs2], 2};
    
}

void CPU::execSB(const DecodedInstr& i) {
    uint32_t a = regs_[i.rs1] + i.imm;
    uint32_t o = static_cast<uint32_t>(bus_.read8(a));
    bus_.write8(a, static_cast<uint8_t>(regs_[i.rs2]));
    sr.mem_write = MemWrite{ a, o, regs_[i.rs2], 1};
    
}

void CPU::execBEQ(const DecodedInstr& i) {
    if (regs_[i.rs1] == regs_[i.rs2]) {
        uint32_t target = pc_ + i.imm;

        if (target & ADDRESS_MISALIGNMENT_MASK) {
            trap(0, target, false);
            return;
        }

        next_pc_ = target;
    }
}

void CPU::execBNE(const DecodedInstr& i) {
    if (regs_[i.rs1] != regs_[i.rs2]) {
        uint32_t target = pc_ + i.imm;

        if (target & ADDRESS_MISALIGNMENT_MASK) {
            trap(0, target, false);
            return;
        }

        next_pc_ = target;
    }
}

void CPU::execBLT(const DecodedInstr& i) {
    int32_t r1 = static_cast<int32_t>(regs_[i.rs1]);
    int32_t r2 = static_cast<int32_t>(regs_[i.rs2]);
    if (r1 < r2) {
        uint32_t target = pc_ + i.imm;

        if (target & ADDRESS_MISALIGNMENT_MASK) {
            trap(0, target, false);
            return;
        }

        next_pc_ = target;
    }
}

void CPU::execBGE(const DecodedInstr& i) {
    int32_t r1 = static_cast<int32_t>(regs_[i.rs1]);
    int32_t r2 = static_cast<int32_t>(regs_[i.rs2]);
    if (r1 >= r2) {
        uint32_t target = pc_ + i.imm;

        if (target & ADDRESS_MISALIGNMENT_MASK) {
            trap(0, target, false);
            return;
        }

        next_pc_ = target;

    }
}

void CPU::execBLTU(const DecodedInstr& i) {
    if (regs_[i.rs1] < regs_[i.rs2]) {
        uint32_t target = pc_ + i.imm;

        if (target & ADDRESS_MISALIGNMENT_MASK) {
            trap(0, target, false);
            return;
        }

        next_pc_ = target;
    }
}

void CPU::execBGEU(const DecodedInstr& i) {
    if (regs_[i.rs1] >= regs_[i.rs2]) {
        uint32_t target = pc_ + i.imm;

        if (target & ADDRESS_MISALIGNMENT_MASK) {
            trap(0, target, false);
            return;
        }

        next_pc_ = target;
    }
}

void CPU::execJAL(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    uint32_t target = pc_ + i.imm;

    if (target & ADDRESS_MISALIGNMENT_MASK) {
        trap(0, target, false);
        return;
    }

    writeReg(i.rd, pc_ + 4);
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd] };
    next_pc_ = target;
}

void CPU::execJALR(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    uint32_t next_pc = (regs_[i.rs1] + i.imm) &~ 1;

    if (next_pc & ADDRESS_MISALIGNMENT_MASK) {
        trap(0, next_pc, false);
        return;
    }

    writeReg(i.rd, pc_ + 4);
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd] };
    next_pc_ = next_pc;
}

void CPU::execLUI(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    writeReg(i.rd, static_cast<uint32_t>(i.imm));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd] };
    
}

void CPU::execAUIPC(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    writeReg(i.rd, static_cast<uint32_t>(pc_ + i.imm));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd] };
    
}

void CPU::execECALL(const DecodedInstr& i) {
    if (regs_[17] == 93) { 
        std::cout << "Program exit via ECALL with code: " << regs_[10] << std::endl;
        halted = true;
        return;
    }

    // 2. If it's not a syscall, or a different syscall, check for a trap handler
    uint32_t cause = 11; // Machine Mode ECALL
    
    if (csrs_[CSR::MTVEC] != 0) {
        trap(cause, 0, false);
    } else {
        std::cout << "Unhandled ECALL: a7=" << regs_[17] << " @ PC=" << std::hex << pc_ << std::endl;
        halted = true; 
    }
}

void CPU::execEBREAK(const DecodedInstr& i) {
    trap(3, 0, false);
}

void CPU::execFENCE(const DecodedInstr& i) {
    
}

void CPU::execCSRRW(const DecodedInstr& i) {
    uint16_t csr_addr = i.csr & 0xFFF;
    uint32_t old_val = readCSR(csr_addr);
    uint32_t o = regs_[i.rd];
    
    writeCSR(csr_addr, regs_[i.rs1]);
    writeReg(i.rd, old_val);
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd] };
    sr.csr_write = CsrWrite{ csr_addr, old_val,  csrs_[csr_addr] };
    
}

void CPU::execCSRRS(const DecodedInstr& i) {
    uint16_t csr_addr = i.csr & 0xFFF;
    uint32_t old_val = readCSR(csr_addr);
    uint32_t o = regs_[i.rd];
    
    if (i.rs1 != 0) {
        writeCSR(csr_addr, old_val | regs_[i.rs1]);
    }
    writeReg(i.rd, old_val);
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd] };
    sr.csr_write = CsrWrite{ csr_addr, old_val,  csrs_[csr_addr] };
    
}

void CPU::execCSRRC(const DecodedInstr& i) {
    uint16_t csr_addr = i.csr & 0xFFF;
    uint32_t old_val = readCSR(csr_addr);
    uint32_t o = regs_[i.rd];

    if (i.rs1 != 0) {
        writeCSR(csr_addr, old_val & ~regs_[i.rs1]);
    }
    writeReg(i.rd, old_val);
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd] };
    sr.csr_write = CsrWrite{ csr_addr, old_val,  csrs_[csr_addr] };
    
}

void CPU::execCSRRWI(const DecodedInstr& i) {
    uint16_t csr_addr = i.csr & 0xFFF;
    uint32_t old_val = readCSR(csr_addr);
    uint32_t uimm = i.rs1;
    uint32_t o = regs_[i.rd];

    writeReg(i.rd, old_val);
    writeCSR(csr_addr, uimm);

    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd] };
    sr.csr_write = CsrWrite{ csr_addr, old_val,  csrs_[csr_addr] };
    
    
}

void CPU::execCSRRSI(const DecodedInstr& i) {
    uint16_t csr_addr = i.csr & 0xFFF;
    uint32_t old_val = readCSR(csr_addr);
    uint32_t uimm = i.rs1;
    uint32_t o = regs_[i.rd];

    writeReg(i.rd, old_val);

    
    if (uimm != 0) {
        writeCSR(csr_addr, old_val | uimm);
    }

    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd] };
    sr.csr_write = CsrWrite{ csr_addr, old_val,  csrs_[csr_addr] };
    
    
}

void CPU::execCSRRCI(const DecodedInstr& i) {
    uint16_t csr_addr = i.csr & 0xFFF;
    uint32_t old_val = readCSR(csr_addr);
    uint32_t uimm = i.rs1;
    uint32_t o = regs_[i.rd];

    writeReg(i.rd, old_val);

    
    if (uimm != 0) {
        writeCSR(csr_addr, old_val & ~uimm);
    }
    
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd] };
    sr.csr_write = CsrWrite{ csr_addr, old_val,  csrs_[csr_addr] };
    
    
}

void CPU::execMUL(const DecodedInstr& i) {
    uint32_t o = (uint32_t)regs_[i.rd];
    int64_t res = (int64_t)(int32_t)regs_[i.rs1] * (int64_t)(int32_t)regs_[i.rs2];
    writeReg(i.rd, (uint32_t)(res & 0xFFFFFFFF));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execMULH(const DecodedInstr& i) {
    uint32_t o = (uint32_t)regs_[i.rd];
    int64_t res = (int64_t)(int32_t)regs_[i.rs1] * (int64_t)(int32_t)regs_[i.rs2];
    writeReg(i.rd, (uint32_t)(res >> 32));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execMULHSU(const DecodedInstr& i) {
    uint32_t o = (uint32_t)regs_[i.rd];

    int64_t s1 = (int32_t) regs_[i.rs1];
    uint64_t s2 = (uint32_t) regs_[i.rs2];

    uint64_t res = s1 * (int64_t) s2;
    writeReg(i.rd, (uint32_t)(res >> 32));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execMULHU(const DecodedInstr& i) {
    uint32_t o = (uint32_t)regs_[i.rd];
    uint64_t res = (uint64_t)regs_[i.rs1] * (uint64_t)regs_[i.rs2];
    writeReg(i.rd, (uint32_t)(res >> 32));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execDIV(const DecodedInstr& i) {
    int32_t dividend = (int32_t)regs_[i.rs1];
    int32_t divisor = (int32_t)regs_[i.rs2];
    uint32_t o = (uint32_t)regs_[i.rd];

    if (divisor == 0) {
        writeReg(i.rd, 0xFFFFFFFF);
    } else if (dividend == INT32_MIN && divisor == -1) {
        writeReg(i.rd, dividend);
    } else {
        writeReg(i.rd, (uint32_t)(dividend / divisor));
    }
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execDIVU(const DecodedInstr& i) {
    uint32_t dividend = (uint32_t)regs_[i.rs1];
    uint32_t divisor = (uint32_t)regs_[i.rs2];
    uint32_t o = (uint32_t)regs_[i.rd];

    if (divisor == 0) {
        writeReg(i.rd, 0xFFFFFFFF);
    } else {
        writeReg(i.rd, (uint32_t)(dividend / divisor));
    }
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execREM(const DecodedInstr& i) {
    int32_t dividend = (int32_t)regs_[i.rs1];
    int32_t divisor = (int32_t)regs_[i.rs2];
    uint32_t o = (uint32_t)regs_[i.rd];

    if (divisor == 0) {
        writeReg(i.rd, dividend);
    } else if (dividend == INT32_MIN && divisor == -1) {
        writeReg(i.rd, 0);
    } else {
        writeReg(i.rd, (uint32_t)(dividend % divisor));
    }
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execREMU(const DecodedInstr& i) {
    uint32_t dividend = (uint32_t)regs_[i.rs1];
    uint32_t divisor = (uint32_t)regs_[i.rs2];
    uint32_t o = (uint32_t)regs_[i.rd];

    if (divisor == 0) {
        writeReg(i.rd, dividend);
    } else {
        writeReg(i.rd, (uint32_t)(dividend % divisor));
    }
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
}


void CPU::execMRET(const DecodedInstr& i) {
    next_pc_ = readCSR(CSR::MEPC);

    // 2. Handle mstatus bit manipulation
    uint32_t mstatus = readCSR(CSR::MSTATUS);
    uint32_t mpie = (mstatus >> 7) & 1;
    // Set MIE to MPIE, then set MPIE to 1
    mstatus = (mstatus & ~(1 << 3)) | (mpie << 3);
    mstatus |= (1 << 7);

    sr.csr_write = CsrWrite{ 0x300, readCSR(CSR::MSTATUS),  mstatus };
    csrs_[CSR::MSTATUS] = mstatus;
}


void CPU::execINVALID(const DecodedInstr& i) {
    trap(2, sr.instruction, false);
}
