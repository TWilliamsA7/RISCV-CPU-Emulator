// inc/cpu/cpu.hpp

#pragma once

#include <cstdint>
#include <array>
#include "cpu/isa.hpp"
#include "memory/memory.hpp"

struct StepResult {
    uint32_t pc_before;
    uint32_t pc_after;
    uint32_t instruction;
    DecodedInstr dInstr;
    bool trap;
};

class CPU {
    public:
        CPU(Memory& mem);
        StepResult step();
        
        uint32_t pc() const;
        uint32_t reg(size_t idx) const;
        bool isHalted() const;
        
        static DecodedInstr decode(uint32_t instr);

    private:
        uint32_t pc_;
        uint32_t r[32];

        Memory& memory_;

        bool halted = false;

        void inline writeReg(uint8_t rd, uint32_t value);
        
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
        void execINVALID(const DecodedInstr& i);

};