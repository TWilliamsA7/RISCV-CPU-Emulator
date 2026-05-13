// inc/cpu/cpu.hpp

#pragma once

#include <cstdint>
#include <unordered_map>
#include <array>
#include <string>
#include <Optional>
#include "isa/isa.hpp"
#include "bus/bus.hpp"
#include "core/state.hpp"
#include "mmu/mmu.hpp"

enum class ExecutionMode {
    BARE_METAL,
    SYSTEM,
};

struct CPUConfig {
    bool extension_m = false;
    bool extension_c = false;
    bool verbose = false;
    ExecutionMode mode = ExecutionMode::SYSTEM;
};

class CPU {
    public:
        CPU(CPUConfig config, Bus& bus, Clint& clint);

        void run();
        StepResult step();
        
        void setPC(uint32_t pc);
        uint32_t pc() const;
        uint32_t reg(size_t idx) const;


        bool isHalted() const;

        // === Trace Functions === //

        void printTrace() const;
        void dumpRegisters() const;

        friend class MMU;

    private:
        uint32_t pc_;
        std::optional<uint32_t> next_pc_;
        std::array<uint32_t, 32> regs_;
        std::unordered_map<uint16_t, uint32_t> csrs_;

        enum PrivilegeLevel {
            USER = 0,
            SUPERVISOR = 1,
            MACHINE = 3,
        };

        PrivilegeLevel privilege_level_;

        enum CPUState {
            IDLE,
            WAITING_FOR_INTERRUPT,
            ACTIVE,
            HALTED,
        };

        CPUState state_;

        Bus& bus_;
        Clint& clint_;
        CPUConfig config_;
        uint32_t ADDRESS_MISALIGNMENT_MASK;

        StepResult sr;
        void clearStep();



        // === Write Operations === //

        void writeReg(uint8_t rd, uint32_t value);
        bool writeCSR(uint16_t addr, uint32_t val);
        std::optional<uint32_t> readCSR(uint16_t addr);

        void updateCycle();

        // === Trap == //

        bool trap_occurred_ = false;
        void checkInterrupts();

        bool sModeInterruptsEnabled();
        bool mModeInterruptsEnabled();

        void trap(uint32_t cause, uint32_t tval, bool is_interrupt, PrivilegeLevel target_level = PrivilegeLevel::MACHINE);
    
        
        // === Execute and Decode === //

        static DecodedInstr decode(uint32_t instr);
        static uint32_t decompress(uint16_t i);

        void execute(const DecodedInstr& i);
        
        // === Execution Dispath === //
        
        using ExecFn = void (CPU::*)(const DecodedInstr&);
        static const std::array<ExecFn, static_cast<size_t>(InstrKind::COUNT)> dispatch_;

        // === Execution Helper Functions === //

        void execADD(const DecodedInstr& i);
        void execSUB(const DecodedInstr& i);
        void execAND(const DecodedInstr& i);
        void execOR(const DecodedInstr& i);
        void execXOR(const DecodedInstr& i);
        void execSLL(const DecodedInstr& i);
        void execSRL(const DecodedInstr& i);
        void execSRA(const DecodedInstr& i);
        void execSLT(const DecodedInstr& i);
        void execSLTU(const DecodedInstr& i);
        void execADDI(const DecodedInstr& i);
        void execANDI(const DecodedInstr& i);
        void execORI(const DecodedInstr& i);
        void execXORI(const DecodedInstr& i);
        void execSLTI(const DecodedInstr& i);
        void execSLTIU(const DecodedInstr& i);
        void execSLLI(const DecodedInstr& i);
        void execSRLI(const DecodedInstr& i);
        void execSRAI(const DecodedInstr& i);
        void execLW(const DecodedInstr& i);
        void execLH(const DecodedInstr& i);
        void execLHU(const DecodedInstr& i);
        void execLB(const DecodedInstr& i);
        void execLBU(const DecodedInstr& i);
        void execSW(const DecodedInstr& i);
        void execSH(const DecodedInstr& i);
        void execSB(const DecodedInstr& i);
        void execBEQ(const DecodedInstr& i);
        void execBNE(const DecodedInstr& i);
        void execBLT(const DecodedInstr& i);
        void execBGE(const DecodedInstr& i);
        void execBLTU(const DecodedInstr& i);
        void execBGEU(const DecodedInstr& i);
        void execJAL(const DecodedInstr& i);
        void execJALR(const DecodedInstr& i);
        void execLUI(const DecodedInstr& i);
        void execAUIPC(const DecodedInstr& i);
        void execECALL(const DecodedInstr& i);
        void execEBREAK(const DecodedInstr& i);
        void execFENCE(const DecodedInstr& i);
        void execCSRRW(const DecodedInstr& i);
        void execCSRRS(const DecodedInstr& i);
        void execCSRRC(const DecodedInstr& i);
        void execCSRRWI(const DecodedInstr& i);
        void execCSRRSI(const DecodedInstr& i);
        void execCSRRCI(const DecodedInstr& i);
        void execMUL(const DecodedInstr& i);
        void execMULH(const DecodedInstr& i);
        void execMULHSU(const DecodedInstr& i);
        void execMULHU(const DecodedInstr& i);
        void execDIV(const DecodedInstr& i);
        void execDIVU(const DecodedInstr& i);
        void execREM(const DecodedInstr& i);
        void execREMU(const DecodedInstr& i);
        void execMRET(const DecodedInstr& i);
        void execSRET(const DecodedInstr& i);
        void execWFI(const DecodedInstr& i);
        void execSFENCE_VMA(const DecodedInstr& i);
        void execINVALID(const DecodedInstr& i);

        // === Miscellaneous Helper functions === //
        std::string hex32(uint32_t v) const;

        enum CSR {
            MISA = 0x301,
            MTVEC = 0x305,
            MSTATUS = 0x300,
            MIDELEG = 0x303,
            MEDELEG = 0x302,
            MCAUSE = 0x342,
            MEPC = 0x341,
            MTVAL = 0x343,
            MCYCLE = 0xB00,
            MCYCLEH = 0xB80,
            CYCLE = 0xC00,
            CYCLEH = 0xC80,
            MINSTRET = 0xB02,
            MINSTRETH = 0xB82,
            MVENDORID = 0xF11,
            MIP = 0x344,
            MIE = 0x304,
            SSTATUS = 0x100,
            SIE = 0x104,
            STVEC = 0x105,
            SSCRATCH = 0x140,
            SEPC = 0x141,
            SCAUSE = 0x142,
            STVAL = 0x143,
            SIP = 0x144,
            SATP = 0x180
        };

        // === Trace === //
        std::string disasm(const DecodedInstr& di) const;
        std::string csrName(uint16_t csr) const;

        enum ExceptionCause {
            MISALIGNED_INSTRUCTION = 0,
            INSTRUCTION_ACCESS_FAULT = 1,
            ILLEGAL_INSTRUCTION = 2,
            BREAKPOINT = 3,
            MISALIGNED_LOAD_ADDRESS = 4,
            LOAD_ACCESS_FAULT = 5,
            MISALIGNED_STORE_ADDRESS = 6,
            STORE_ACCESS_FAULT = 7,
            U_MODE_ENVIRONMENT_CALL = 8,
            S_MODE_ENVIRONMENT_CALL = 9,
            M_MODE_ENVIRONMENT_CALL = 11,
            INSTRUCTION_PAGE_FAULT = 12,
            LOAD_PAGE_FAULT = 13,
            STORE_PAGE_FAULT = 15
        };
};