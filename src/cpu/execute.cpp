// src/cpu/execute.cpp

#include "cpu/cpu.hpp"
#include <iostream>
#include <errors/errors.hpp>

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
    &CPU::execSRET,
    &CPU::execWFI,
    &CPU::execSFENCE_VMA,
    &CPU::execLR_W,
    &CPU::execSC_W,
    &CPU::execAMOSWAP_W,
    &CPU::execAMOADD_W,
    &CPU::execAMOAND_W,
    &CPU::execAMOOR_W,
    &CPU::execAMOXOR_W,
    &CPU::execAMOMAX_W,
    &CPU::execAMOMAXU_W,
    &CPU::execAMOMIN_W,
    &CPU::execAMOMINU_W,
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
        trap(ExceptionCause::ILLEGAL_INSTRUCTION, i.raw, false);
        return;
    }

    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    writeReg(i.rd, static_cast<uint32_t>(a << i.imm));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execSRLI(const DecodedInstr& i) {
     if ((i.imm >> 5) != 0) {
        trap(ExceptionCause::ILLEGAL_INSTRUCTION, i.raw, false);
        return;
    }

    uint32_t o = regs_[i.rd];
    uint32_t a = regs_[i.rs1];
    writeReg(i.rd, static_cast<uint32_t>(a >> i.imm));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    
}

void CPU::execSRAI(const DecodedInstr& i) {
    if ((i.imm >> 5) != 0x20) {
        trap(ExceptionCause::ILLEGAL_INSTRUCTION, i.raw, false);
        return;
    }

    uint32_t o = regs_[i.rd];
    int32_t a = static_cast<int32_t>(regs_[i.rs1]);
    writeReg(i.rd, static_cast<uint32_t>(a >> (i.imm & 0x1F)));
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
}

void CPU::execLW(const DecodedInstr& i) {
    uint32_t a = regs_[i.rs1] + i.imm;
    
    try {
        uint32_t o = regs_[i.rd];

        uint32_t pa = mmu_.translate(a, MMU::AccessType::LOAD);

        writeReg(i.rd, bus_.read32(pa));
        sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    } catch (const BusAccessError&) {
        trap(ExceptionCause::LOAD_ACCESS_FAULT, a, false);
    } catch (const LoadPageError&) {
        trap(ExceptionCause::LOAD_PAGE_FAULT, a, false);
    }
}

void CPU::execLH(const DecodedInstr& i) {
    uint32_t a = regs_[i.rs1]  + i.imm;
    
    try {
        uint32_t o = regs_[i.rd];

        uint32_t pa = mmu_.translate(a, MMU::AccessType::LOAD);

        int16_t half = bus_.read16(pa);       
        int32_t s = static_cast<int32_t>(half);
        writeReg(i.rd, static_cast<uint32_t>(s));
        sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    } catch (const BusAccessError&) {
        trap(ExceptionCause::LOAD_ACCESS_FAULT, a, false);
    } catch (const LoadPageError&) {
        trap(ExceptionCause::LOAD_PAGE_FAULT, a, false);
    }
}

void CPU::execLHU(const DecodedInstr& i) { 
    uint32_t a = regs_[i.rs1]  + i.imm;
    
    try {
        uint32_t pa = mmu_.translate(a, MMU::AccessType::LOAD);
        uint32_t o = regs_[i.rd];
        uint32_t u = static_cast<uint32_t>(bus_.read16(pa));
        writeReg(i.rd, u);
        sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    } catch (const BusAccessError&) {
        trap(ExceptionCause::LOAD_ACCESS_FAULT, a, false);
    } catch (const LoadPageError&) {
        trap(ExceptionCause::LOAD_PAGE_FAULT, a, false);
    }

    
}

void CPU::execLB(const DecodedInstr& i) {
    uint32_t a = regs_[i.rs1]  + i.imm;

    try {
        uint32_t pa = mmu_.translate(a, MMU::AccessType::LOAD);
        uint32_t o = regs_[i.rd];
        int8_t byte = static_cast<int8_t>(bus_.read8(pa));
        writeReg(i.rd, static_cast<uint32_t>(static_cast<int32_t>(byte)));
        sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    } catch (const BusAccessError&) {
        trap(ExceptionCause::LOAD_ACCESS_FAULT, a, false);
    } catch (const LoadPageError&) {
        trap(ExceptionCause::LOAD_PAGE_FAULT, a, false);
    }
}

void CPU::execLBU(const DecodedInstr& i) {
    uint32_t a = regs_[i.rs1] + i.imm;

    try {
        uint32_t pa = mmu_.translate(a, MMU::AccessType::LOAD);
        uint32_t o = regs_[i.rd];
        uint32_t u = static_cast<uint32_t>(bus_.read8(pa));
        writeReg(i.rd, u);
        sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd]};
    } catch (const BusAccessError&) {
        trap(ExceptionCause::LOAD_ACCESS_FAULT, a, false);
    } catch (const LoadPageError&) {
        trap(ExceptionCause::LOAD_PAGE_FAULT, a, false);
    }
}

void CPU::execSW(const DecodedInstr& i) {
    uint32_t a = regs_[i.rs1] + i.imm;

    try {
        uint32_t pa = mmu_.translate(a, MMU::AccessType::STORE);
        uint32_t o = bus_.read32(pa);
        bus_.write32(pa, regs_[i.rs2]);
        sr.mem_write = MemWrite{ a, o, regs_[i.rs2], 4};
    } catch (const BusAccessError&) {
        trap(ExceptionCause::STORE_ACCESS_FAULT, a, false);
    } catch (const StorePageError&) {
        trap(ExceptionCause::STORE_PAGE_FAULT, a, false);
    }
    
}

void CPU::execSH(const DecodedInstr& i) {
    uint32_t a = regs_[i.rs1] + i.imm;

    try {
        uint32_t pa = mmu_.translate(a, MMU::AccessType::STORE);
        uint32_t o = static_cast<uint32_t>(bus_.read16(pa));
        bus_.write16(pa, static_cast<uint16_t>(regs_[i.rs2]));
        sr.mem_write = MemWrite{ a, o, regs_[i.rs2], 2};
    } catch (const BusAccessError&) {
        trap(ExceptionCause::STORE_ACCESS_FAULT, a, false);
    } catch (const StorePageError&) {
        trap(ExceptionCause::STORE_PAGE_FAULT, a, false);
    }
}

void CPU::execSB(const DecodedInstr& i) {
    uint32_t a = regs_[i.rs1] + i.imm; 

    try {
        uint32_t pa = mmu_.translate(a, MMU::AccessType::STORE);
        uint32_t o = static_cast<uint32_t>(bus_.read8(pa));
        bus_.write8(pa, static_cast<uint8_t>(regs_[i.rs2]));
        sr.mem_write = MemWrite{ a, o, regs_[i.rs2], 1};
    } catch (const BusAccessError&) {  
        trap(ExceptionCause::STORE_ACCESS_FAULT, a, false);
    } catch (const StorePageError&) {
        trap(ExceptionCause::STORE_PAGE_FAULT, a, false);
    }
}

void CPU::execBEQ(const DecodedInstr& i) {
    if (regs_[i.rs1] == regs_[i.rs2]) {
        uint32_t target = pc_ + i.imm;

        if (target & ADDRESS_MISALIGNMENT_MASK) {
            trap(ExceptionCause::MISALIGNED_INSTRUCTION, target, false);
            return;
        }

        next_pc_ = target;
    }
}

void CPU::execBNE(const DecodedInstr& i) {
    if (regs_[i.rs1] != regs_[i.rs2]) {
        uint32_t target = pc_ + i.imm;

        if (target & ADDRESS_MISALIGNMENT_MASK) {
            trap(ExceptionCause::MISALIGNED_INSTRUCTION, target, false);
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
            trap(ExceptionCause::MISALIGNED_INSTRUCTION, target, false);
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
            trap(ExceptionCause::MISALIGNED_INSTRUCTION, target, false);
            return;
        }

        next_pc_ = target;

    }
}

void CPU::execBLTU(const DecodedInstr& i) {
    if (regs_[i.rs1] < regs_[i.rs2]) {
        uint32_t target = pc_ + i.imm;

        if (target & ADDRESS_MISALIGNMENT_MASK) {
            trap(ExceptionCause::MISALIGNED_INSTRUCTION, target, false);
            return;
        }

        next_pc_ = target;
    }
}

void CPU::execBGEU(const DecodedInstr& i) {
    if (regs_[i.rs1] >= regs_[i.rs2]) {
        uint32_t target = pc_ + i.imm;

        if (target & ADDRESS_MISALIGNMENT_MASK) {
            trap(ExceptionCause::MISALIGNED_INSTRUCTION, target, false);
            return;
        }

        next_pc_ = target;
    }
}

void CPU::execJAL(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    uint32_t target = pc_ + i.imm;

    if (target & ADDRESS_MISALIGNMENT_MASK) {
        trap(ExceptionCause::MISALIGNED_INSTRUCTION, target, false);
        return;
    }

    writeReg(i.rd, pc_ + i.instr_len);
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd] };
    next_pc_ = target;
}

void CPU::execJALR(const DecodedInstr& i) {
    uint32_t o = regs_[i.rd];
    uint32_t next_pc = (regs_[i.rs1] + i.imm) &~ 1;

    if (next_pc & ADDRESS_MISALIGNMENT_MASK) {
        trap(ExceptionCause::MISALIGNED_INSTRUCTION, next_pc, false);
        return;
    }

    writeReg(i.rd, pc_ + i.instr_len);
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
    if (config_.mode == ExecutionMode::BARE_METAL) {
        if (regs_[17] == 93) {
            std::cout << "Program exit via ECALL with code: " << regs_[10] << std::endl;
            state_ = CPUState::HALTED;
            return;
        }
    }

    uint32_t cause;
    switch (privilege_level_) {
        case PrivilegeLevel::USER: cause = ExceptionCause::U_MODE_ENVIRONMENT_CALL; break;
        case PrivilegeLevel::SUPERVISOR: cause = ExceptionCause::S_MODE_ENVIRONMENT_CALL; break;
        case PrivilegeLevel::MACHINE: cause = ExceptionCause::M_MODE_ENVIRONMENT_CALL; break;
        default: cause = ExceptionCause::M_MODE_ENVIRONMENT_CALL; break;
    }

    bool delegate = (csrs_[CSR::MEDELEG] >> cause) & 1;
    PrivilegeLevel target = (delegate && privilege_level_ < PrivilegeLevel::MACHINE) ? PrivilegeLevel::SUPERVISOR : PrivilegeLevel::MACHINE;
    trap(cause, 0, false, target);
}

void CPU::execEBREAK(const DecodedInstr& i) {
    uint32_t cause = ExceptionCause::BREAKPOINT;
    bool delegate = (csrs_[CSR::MEDELEG] >> cause) & 1;
    PrivilegeLevel target = (delegate && privilege_level_ < PrivilegeLevel::MACHINE)
        ? PrivilegeLevel::SUPERVISOR
        : PrivilegeLevel::MACHINE;
    trap(cause, pc_, false, target);
}

void CPU::execFENCE(const DecodedInstr& i) {
    
}

void CPU::execCSRRW(const DecodedInstr& i) {
    uint16_t csr_addr = i.csr & 0xFFF;
    auto old_val = readCSR(csr_addr);
    if (!old_val.has_value()) return;


    uint32_t o = regs_[i.rd];
    
    if (!writeCSR(csr_addr, regs_[i.rs1])) return;

    writeReg(i.rd, old_val.value());
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd] };
    sr.csr_write = CsrWrite{ csr_addr, old_val.value(),  csrs_[csr_addr] };
    
}

void CPU::execCSRRS(const DecodedInstr& i) {
    uint16_t csr_addr = i.csr & 0xFFF;
    auto old_val = readCSR(csr_addr);
    if (!old_val.has_value()) return;
    uint32_t o = regs_[i.rd];
    
    if (i.rs1 != 0) {
        if (!writeCSR(csr_addr, old_val.value() | regs_[i.rs1])) return;
    }
    writeReg(i.rd, old_val.value());
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd] };
    sr.csr_write = CsrWrite{ csr_addr, old_val.value(),  csrs_[csr_addr] };
    
}

void CPU::execCSRRC(const DecodedInstr& i) {
    uint16_t csr_addr = i.csr & 0xFFF;
    auto old_val = readCSR(csr_addr);
    if (!old_val.has_value()) return;
    uint32_t o = regs_[i.rd];

    if (i.rs1 != 0) {
        if (!writeCSR(csr_addr, old_val.value() & ~regs_[i.rs1])) return;
    }
    writeReg(i.rd, old_val.value());
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd] };
    sr.csr_write = CsrWrite{ csr_addr, old_val.value(),  csrs_[csr_addr] };
    
}

void CPU::execCSRRWI(const DecodedInstr& i) {
    uint16_t csr_addr = i.csr & 0xFFF;
    auto old_val = readCSR(csr_addr);
    if (!old_val.has_value()) return;
    uint32_t uimm = i.rs1;
    uint32_t o = regs_[i.rd];

    if (!writeCSR(csr_addr, uimm)) return;
    writeReg(i.rd, old_val.value());

    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd] };
    sr.csr_write = CsrWrite{ csr_addr, old_val.value(),  csrs_[csr_addr] };
    
    
}

void CPU::execCSRRSI(const DecodedInstr& i) {
    uint16_t csr_addr = i.csr & 0xFFF;
    auto old_val = readCSR(csr_addr);
    if (!old_val.has_value()) return;
    uint32_t uimm = i.rs1;
    uint32_t o = regs_[i.rd];

    writeReg(i.rd, old_val.value());

    
    if (uimm != 0) {
        if (!writeCSR(csr_addr, old_val.value() | uimm)) return;
    }

    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd] };
    sr.csr_write = CsrWrite{ csr_addr, old_val.value(),  csrs_[csr_addr] };
    
    
}

void CPU::execCSRRCI(const DecodedInstr& i) {
    uint16_t csr_addr = i.csr & 0xFFF;
    auto old_val = readCSR(csr_addr);
    if (!old_val.has_value()) return;
    uint32_t uimm = i.rs1;
    uint32_t o = regs_[i.rd];

    writeReg(i.rd, old_val.value());

    
    if (uimm != 0) {
        if (!writeCSR(csr_addr, old_val.value() & ~uimm));
    }
    
    sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd] };
    sr.csr_write = CsrWrite{ csr_addr, old_val.value(),  csrs_[csr_addr] };
    
    
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
    if (privilege_level_ != PrivilegeLevel::MACHINE) {
        trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false);
        return;
    }
    
    next_pc_ = csrs_[CSR::MEPC];

    uint32_t mstatus = csrs_[CSR::MSTATUS];

    // Read MPP BEFORE clearing it
    uint32_t mpp = (mstatus >> 11) & 0x3;

    uint32_t mpie = (mstatus >> 7) & 1;
    // MIE = MPIE
    mstatus = (mstatus & ~(1 << 3)) | (mpie << 3);
    // MPIE = 1
    mstatus |= (1 << 7);
    // MPP = U (least privileged supported mode)
    mstatus &= ~(3 << 11);

    privilege_level_ = static_cast<CPU::PrivilegeLevel>(mpp);

    if (privilege_level_ != PrivilegeLevel::MACHINE) {
        mstatus &= ~(1 << 17);  // clear MPRV
    }

    sr.csr_write = CsrWrite{ CSR::MSTATUS, csrs_[CSR::MSTATUS], mstatus };
    csrs_[CSR::MSTATUS] = mstatus;
}

void CPU::execSRET(const DecodedInstr& i) {
    if (privilege_level_ == PrivilegeLevel::USER) {
        trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false);
        return;
    }

    if (privilege_level_ == PrivilegeLevel::SUPERVISOR &&
        ((csrs_[CSR::MSTATUS] >> 22) & 1)) {
        trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false);
        return;
    }
    
    uint32_t mstatus = csrs_[CSR::MSTATUS];

    PrivilegeLevel target_level = ((mstatus >> 8) & 1) 
        ? PrivilegeLevel::SUPERVISOR 
        : PrivilegeLevel::USER;

    uint32_t spie = (mstatus >> 5) & 1;
    mstatus = (mstatus & ~(1 << 1)) | (spie << 1);  // SIE = SPIE
    mstatus |= (1 << 5);                              // SPIE = 1
    mstatus &= ~(1 << 8);                             // SPP = 0 (U-mode)

    // Clear MPRV when leaving M-mode (SRET always leaves M-mode)
    mstatus &= ~(1 << 17);

    sr.csr_write = CsrWrite{ CSR::MSTATUS, csrs_[CSR::MSTATUS], mstatus };
    csrs_[CSR::MSTATUS] = mstatus;

    privilege_level_ = target_level;
    next_pc_ = csrs_[CSR::SEPC];
}

void CPU::execWFI(const DecodedInstr& i) {
    if (privilege_level_ == PrivilegeLevel::USER) {
        bool delegate = (csrs_[CSR::MEDELEG] >> ExceptionCause::ILLEGAL_INSTRUCTION) & 1;
        PrivilegeLevel target = (delegate && privilege_level_ < PrivilegeLevel::MACHINE)
            ? PrivilegeLevel::SUPERVISOR : PrivilegeLevel::MACHINE;
        trap(ExceptionCause::ILLEGAL_INSTRUCTION, i.raw, false, target);
        return;
    }
    if (privilege_level_ == PrivilegeLevel::SUPERVISOR) {
        bool mstatus_tw = (csrs_[CSR::MSTATUS] >> 21) & 1;
        if (mstatus_tw) {
            // TW illegal instruction always goes to M-mode per spec
            trap(ExceptionCause::ILLEGAL_INSTRUCTION, i.raw, false, PrivilegeLevel::MACHINE);
            return;
        }
    }
    state_ = CPUState::WAITING_FOR_INTERRUPT;
}

void CPU::execSFENCE_VMA(const DecodedInstr& i) {
    if (privilege_level_ == PrivilegeLevel::USER) {
        trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false);
        return;
    }
    
    // S-mode: illegal when TVM=1 (mstatus bit 20)
    if (privilege_level_ == PrivilegeLevel::SUPERVISOR) {
        if ((csrs_[CSR::MSTATUS] >> 20) & 1) {
            trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false);
            return;
        }
    }
}

void CPU::execLR_W(const DecodedInstr& i) {
    try {
        uint32_t paddr = mmu_.translate(regs_[i.rs1], MMU::AccessType::LOAD);
        uint32_t o = regs_[i.rd];

        writeReg(i.rd, bus_.read32(paddr));

        reservation_addr_ = paddr;
        reservation_valid_ = true;

        sr.reg_write = RegWrite{ i.rd, o, regs_[i.rd] };
    } catch (const BusAccessError&) {
        trap(ExceptionCause::LOAD_ACCESS_FAULT, regs_[i.rs1], false);
    } catch (const LoadPageError&) {
        trap(ExceptionCause::LOAD_PAGE_FAULT, regs_[i.rs1], false);
    }
}

void CPU::execSC_W(const DecodedInstr& i) {
    try {
        uint32_t paddr = mmu_.translate(regs_[i.rs1], MMU::AccessType::STORE);
        uint32_t oReg = regs_[i.rd];
        uint32_t oMem = bus_.read32(paddr);

        if (reservation_valid_ && reservation_addr_ == paddr) {
            bus_.write32(paddr, regs_[i.rs2]);
            writeReg(i.rd, 0);
            sr.mem_write = MemWrite{ regs_[i.rs1], oMem, regs_[i.rs2], 4 };
        } else {
            writeReg(i.rd, 1);
            sr.mem_write = MemWrite{ regs_[i.rs1], oMem, oMem, 4 };
        }
        reservation_valid_ = false;

        uint32_t newMem = bus_.read32(paddr);

        sr.reg_write = RegWrite{ i.rd, oReg, regs_[i.rd] };
        

    } catch (const BusAccessError&) {
        reservation_valid_ = false;
        trap(ExceptionCause::STORE_ACCESS_FAULT, regs_[i.rs1], false);
    } catch (const StorePageError&) {
        reservation_valid_ = false;
        trap(ExceptionCause::STORE_PAGE_FAULT, regs_[i.rs1], false);
    }
    reservation_valid_ = false;
}

void CPU::execAMOSWAP_W(const DecodedInstr& i) {
    try {
        uint32_t oReg = regs_[i.rd];
        uint32_t paddr = mmu_.translate(regs_[i.rs1], MMU::AccessType::STORE);

        if (paddr & 0x3) {
            trap(ExceptionCause::MISALIGNED_STORE_ADDRESS, paddr, false);
            return;
        }

        uint32_t operand = regs_[i.rs2];

        uint32_t oMem = bus_.atomic_rmw_w(paddr, [operand](uint32_t mem_val) { return operand; });

        writeReg(i.rd, oMem);

        uint32_t newMem = operand;

        sr.reg_write = RegWrite{ i.rd, oReg, regs_[i.rd] };
        sr.mem_write = MemWrite{ regs_[i.rs1], oMem, newMem, 4};

    } catch (const BusAccessError&) {
        trap(ExceptionCause::STORE_ACCESS_FAULT, regs_[i.rs1], false);
    } catch (const StorePageError&) {
        trap(ExceptionCause::STORE_PAGE_FAULT, regs_[i.rs1], false);
    }
}

void CPU::execAMOADD_W(const DecodedInstr& i) {
    try {
        uint32_t oReg = regs_[i.rd];
        uint32_t paddr = mmu_.translate(regs_[i.rs1], MMU::AccessType::STORE);

        if (paddr & 0x3) {
            trap(ExceptionCause::MISALIGNED_STORE_ADDRESS, paddr, false);
            return;
        }

        uint32_t operand = regs_[i.rs2];

        uint32_t oMem = bus_.atomic_rmw_w(paddr, [operand](uint32_t mem_val) { return mem_val + operand; });

        writeReg(i.rd, oMem);

        uint32_t newMem = oMem + operand;

        sr.reg_write = RegWrite{ i.rd, oReg, regs_[i.rd] };
        sr.mem_write = MemWrite{ regs_[i.rs1], oMem, newMem, 4};

    } catch (const BusAccessError&) {
        trap(ExceptionCause::STORE_ACCESS_FAULT, regs_[i.rs1], false);
    } catch (const StorePageError&) {
        trap(ExceptionCause::STORE_PAGE_FAULT, regs_[i.rs1], false);
    }
}

void CPU::execAMOAND_W(const DecodedInstr& i) {
    try {
        uint32_t oReg = regs_[i.rd];
        uint32_t paddr = mmu_.translate(regs_[i.rs1], MMU::AccessType::STORE);

        if (paddr & 0x3) {
            trap(ExceptionCause::MISALIGNED_STORE_ADDRESS, paddr, false);
            return;
        }

        uint32_t operand = regs_[i.rs2];

        uint32_t oMem = bus_.atomic_rmw_w(paddr, [operand](uint32_t mem_val) { return mem_val & operand; });

        writeReg(i.rd, oMem);

        uint32_t newMem = oMem & operand;

        sr.reg_write = RegWrite{ i.rd, oReg, regs_[i.rd] };
        sr.mem_write = MemWrite{ regs_[i.rs1], oMem, newMem, 4};

    } catch (const BusAccessError&) {
        trap(ExceptionCause::STORE_ACCESS_FAULT, regs_[i.rs1], false);
    } catch (const StorePageError&) {
        trap(ExceptionCause::STORE_PAGE_FAULT, regs_[i.rs1], false);
    }
}

void CPU::execAMOOR_W(const DecodedInstr& i) {
    try {
        uint32_t oReg = regs_[i.rd];
        uint32_t paddr = mmu_.translate(regs_[i.rs1], MMU::AccessType::STORE);

        if (paddr & 0x3) {
            trap(ExceptionCause::MISALIGNED_STORE_ADDRESS, paddr, false);
            return;
        }

        uint32_t operand = regs_[i.rs2];

        uint32_t oMem = bus_.atomic_rmw_w(paddr, [operand](uint32_t mem_val) { return mem_val | operand; });

        writeReg(i.rd, oMem);

        uint32_t newMem = oMem | operand;

        sr.reg_write = RegWrite{ i.rd, oReg, regs_[i.rd] };
        sr.mem_write = MemWrite{ regs_[i.rs1], oMem, newMem, 4};

    } catch (const BusAccessError&) {
        trap(ExceptionCause::STORE_ACCESS_FAULT, regs_[i.rs1], false);
    } catch (const StorePageError&) {
        trap(ExceptionCause::STORE_PAGE_FAULT, regs_[i.rs1], false);
    }
}

void CPU::execAMOXOR_W(const DecodedInstr& i) {
    try {
        uint32_t oReg = regs_[i.rd];
        uint32_t paddr = mmu_.translate(regs_[i.rs1], MMU::AccessType::STORE);

        if (paddr & 0x3) {
            trap(ExceptionCause::MISALIGNED_STORE_ADDRESS, paddr, false);
            return;
        }

        uint32_t operand = regs_[i.rs2];

        uint32_t oMem = bus_.atomic_rmw_w(paddr, [operand](uint32_t mem_val) { return mem_val ^ operand; });

        writeReg(i.rd, oMem);

        uint32_t newMem = oMem ^ operand;

        sr.reg_write = RegWrite{ i.rd, oReg, regs_[i.rd] };
        sr.mem_write = MemWrite{ regs_[i.rs1], oMem, newMem, 4};

    } catch (const BusAccessError&) {
        trap(ExceptionCause::STORE_ACCESS_FAULT, regs_[i.rs1], false);
    } catch (const StorePageError&) {
        trap(ExceptionCause::STORE_PAGE_FAULT, regs_[i.rs1], false);
    }
}

void CPU::execAMOMAX_W(const DecodedInstr& i) {
    try {
        uint32_t oReg = regs_[i.rd];
        uint32_t paddr = mmu_.translate(regs_[i.rs1], MMU::AccessType::STORE);

        if (paddr & 0x3) {
            trap(ExceptionCause::MISALIGNED_STORE_ADDRESS, paddr, false);
            return;
        }

        int32_t operand = static_cast<int32_t>(regs_[i.rs2]);

        uint32_t oMem = bus_.atomic_rmw_w(paddr, [operand](uint32_t mem_val) { return std::max(static_cast<int32_t>(mem_val), operand); });

        writeReg(i.rd, oMem);

        uint32_t newMem = static_cast<uint32_t>(std::max<int32_t>(static_cast<int32_t>(oMem), operand));
        sr.reg_write = RegWrite{ i.rd, oReg, regs_[i.rd] };
        sr.mem_write = MemWrite{ regs_[i.rs1], oMem, newMem, 4};

    } catch (const BusAccessError&) {
        trap(ExceptionCause::STORE_ACCESS_FAULT, regs_[i.rs1], false);
    } catch (const StorePageError&) {
        trap(ExceptionCause::STORE_PAGE_FAULT, regs_[i.rs1], false);
    }
}

void CPU::execAMOMAXU_W(const DecodedInstr& i) {
    try {
        uint32_t oReg = regs_[i.rd];
        uint32_t paddr = mmu_.translate(regs_[i.rs1], MMU::AccessType::STORE);

        if (paddr & 0x3) {
            trap(ExceptionCause::MISALIGNED_STORE_ADDRESS, paddr, false);
            return;
        }

        uint32_t operand = regs_[i.rs2];

        uint32_t oMem = bus_.atomic_rmw_w(paddr, [operand](uint32_t mem_val) { return std::max(mem_val, operand); });

        writeReg(i.rd, oMem);

        uint32_t newMem = static_cast<uint32_t>(std::max(oMem, operand));
        sr.reg_write = RegWrite{ i.rd, oReg, regs_[i.rd] };
        sr.mem_write = MemWrite{ regs_[i.rs1], oMem, newMem, 4};

    } catch (const BusAccessError&) {
        trap(ExceptionCause::STORE_ACCESS_FAULT, regs_[i.rs1], false);
    } catch (const StorePageError&) {
        trap(ExceptionCause::STORE_PAGE_FAULT, regs_[i.rs1], false);
    }
}

void CPU::execAMOMIN_W(const DecodedInstr& i) {
    try {
        uint32_t oReg = regs_[i.rd];
        uint32_t paddr = mmu_.translate(regs_[i.rs1], MMU::AccessType::STORE);

        if (paddr & 0x3) {
            trap(ExceptionCause::MISALIGNED_STORE_ADDRESS, paddr, false);
            return;
        }

        int32_t operand = static_cast<int32_t>(regs_[i.rs2]);

        uint32_t oMem = bus_.atomic_rmw_w(paddr, [operand](uint32_t mem_val) { return std::min(static_cast<int32_t>(mem_val), operand); });

        writeReg(i.rd, oMem);

        uint32_t newMem = static_cast<uint32_t>(std::min(static_cast<int32_t>(oMem), operand));
        sr.reg_write = RegWrite{ i.rd, oReg, regs_[i.rd] };
        sr.mem_write = MemWrite{ regs_[i.rs1], oMem, newMem, 4};

    } catch (const BusAccessError&) {
        trap(ExceptionCause::STORE_ACCESS_FAULT, regs_[i.rs1], false);
    } catch (const StorePageError&) {
        trap(ExceptionCause::STORE_PAGE_FAULT, regs_[i.rs1], false);
    }
}

void CPU::execAMOMINU_W(const DecodedInstr& i) {
    try {
        uint32_t oReg = regs_[i.rd];
        uint32_t paddr = mmu_.translate(regs_[i.rs1], MMU::AccessType::STORE);

        if (paddr & 0x3) {
            trap(ExceptionCause::MISALIGNED_STORE_ADDRESS, paddr, false);
            return;
        }

        uint32_t operand = regs_[i.rs2];

        uint32_t oMem = bus_.atomic_rmw_w(paddr, [operand](uint32_t mem_val) { return std::min(mem_val, operand); });

        writeReg(i.rd, oMem);

        uint32_t newMem = static_cast<uint32_t>(std::min(oMem, operand));
        sr.reg_write = RegWrite{ i.rd, oReg, regs_[i.rd] };
        sr.mem_write = MemWrite{ regs_[i.rs1], oMem, newMem, 4};

    } catch (const BusAccessError&) {
        trap(ExceptionCause::STORE_ACCESS_FAULT, regs_[i.rs1], false);
    } catch (const StorePageError&) {
        trap(ExceptionCause::STORE_PAGE_FAULT, regs_[i.rs1], false);
    }
}

void CPU::execINVALID(const DecodedInstr& i) {
    trap(ExceptionCause::ILLEGAL_INSTRUCTION, sr.instruction, false);
}
