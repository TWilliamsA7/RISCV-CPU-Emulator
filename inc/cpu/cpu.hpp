// inc/cpu/cpu.hpp

#pragma once

#include <cstdint>
#include <unordered_set>
#include <array>
#include <string>
#include "isa/isa.hpp"
#include "bus/bus.hpp"
#include "core/state.hpp"

class CPU {
    public:
        CPU(Bus& bus);

        void run();
        void run(uint32_t count);
        StepResult step();
        
        void setPC(uint32_t pc);
        uint32_t pc() const;
        uint32_t reg(size_t idx) const;


        bool isHalted() const;

        // === Trace Functions === //

        void printTrace() const;
        void dumpRegisters() const;
        void addBreakpoint(uint32_t addr);
        void removeBreakpoint(uint32_t addr);
        
    private:
        uint32_t pc_;
        std::array<uint32_t, 32> regs_;
        std::array<uint32_t, 4096> csrs_;
        
        Bus& bus_;

        StepResult sr;
        void clearStep();
        
        bool trace_enabled_ = false;
        bool halted = false;
        std::unordered_set<uint32_t> breakpoints_;

        // === Write Operations === //

        void writeReg(uint8_t rd, uint32_t value);
        void writeCSR(uint16_t addr, uint32_t val);

        // === Trap == //

        void trap(uint32_t cause, uint32_t tval);
        
        // === Execute and Decode === //

        static DecodedInstr decode(uint32_t instr);
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
        void execINVALID(const DecodedInstr& i);

        // === Miscellaneous Helper functions === //
        std::string hex32(uint32_t v) const;

        enum CSR {
            MTVEC = 0x305,
            MSTATUS = 0x300,
            MCAUSE = 0x342,
            MEPC = 0x341,
            MTVAL = 0x343,
            MCYCLE = 0xB00,
            MCYCLEH = 0xB80,
            CYCLE = 0xC00,
            CYCLEH = 0xC80,
            MINSTRET = 0xB02,
            MINSTRETH = 0xB82,
        };
};

