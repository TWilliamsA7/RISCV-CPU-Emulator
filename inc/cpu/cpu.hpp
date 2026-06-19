// inc/cpu/cpu.hpp

#pragma once

#include <cstdint>
#include <unordered_map>
#include <array>
#include <string>
#include <atomic>
#include "isa/isa.hpp"
#include "core/state.hpp"
#include "bus/bus.hpp"
#include "mmu/mmu.hpp"
#include "mmu/tlb.hpp"
#include "types.hpp"
#include <cache/icache.hpp>
#include <cache/cache_entry.hpp>

enum class ExecutionMode {
    BARE_METAL,
    SYSTEM,
};

struct CPUConfig {
    ExecutionMode mode = ExecutionMode::SYSTEM;
    Extensions extensions;
    uint32_t starting_pc = 0x80000000;
};

class Emulator;

class CPU {
    public:
        // Constructor with configured devices
        CPU(Emulator& sys);

        // Execute loaded instructions
        void run();

        // Single step execution
        StepResult step();
        
        // Set Program Counter
        void setPC(uint32_t pc);

        // Get Program Counter
        uint32_t pc() const;

        // Get Register idx
        uint32_t reg(size_t idx) const;

        // Returns True if CPU is halted
        bool isHalted() const;

        void halt();


        // === Trace Functions === //

        // Prints a summary of the last instruction executed
        void printTrace() const;

        // Prints a dump of all current register values
        void dumpRegisters() const;


        // === Reseveration === //

        // Remove an existing reservation on the address addr
        void invalidateReservation(uint32_t addr);

        friend class MMU;
        friend class Bus;

        // Defines read, write, execute permissions


    private:

        Emulator& sys_;

        // Program Counter
        uint32_t pc_;
        // Next Program Counter
        bool set_next_pc_ = false;
        uint32_t next_pc_;

        // Registers
        std::array<uint32_t, 32> regs_;
        // Control Status Registers
        std::array<uint32_t, 4096> csrs_;

        // Reservation

        // True if a valid reservation exists
        bool reservation_valid_;
        // Address of the current reservation if it exists
        uint32_t reservation_addr_;



        // Current Execution Priviledge Level
        PrivilegeLevel privilege_level_;

        // CPU States
        enum CPUState {
            IDLE,
            WAITING_FOR_INTERRUPT,
            ACTIVE,
            HALTED,
        };

        // Current CPU State
        CPUState state_;

        // TLB
        TLB tlb_;

        ICache icache_;

        uint32_t current_epoch_ = 0;
        static constexpr uint16_t DECODE_CACHE_SIZE = 4096;
        CacheEntry decoded_cache_[DECODE_CACHE_SIZE];
        void clearDecodeCache();

        // Supported Extensions Mask
        static constexpr uint32_t SUPPORTED_EXTENSIONS_MASK = (1U << 30) | (1U << 8) | (1U << 18) | (1U << 20) | (1U << 12) | (1U << 2) | (1U << 0);

        // Required Extenions Mask
        static constexpr uint32_t REQUIRED_EXTENSIONS_MASK = (1U << 30) | (1U << 8);

        // Mask used to check instruction alignment
        uint32_t ADDRESS_MISALIGNMENT_MASK;

        // Output state of cpu after a step
        StepResult sr;

        // Clear the previous step
        void clearStep();

        // Update CLINT and timer variables
        void updateCycle();

        // === Write Operations === //

        // Write value to rd
        void writeReg(uint8_t rd, uint32_t value);

        // Write val to CSR addr
        bool writeCSR(uint16_t addr, uint32_t val);

        // Read val of CSR addr if it exists
        uint32_t readCSR(uint16_t addr);

        void write_pmpcfg(uint16_t addr, uint32_t val);
        void write_pmpaddr(uint16_t addr, uint32_t val);


        // === Trap == //

        // True if a trap occurred on the current step
        bool trap_occurred_ = false;

        // Check if there is a current pending interrupt and fire accordingly
        void checkInterrupts();

        // Execute trap sequence depending on the privilege and interrupts
        void trap(uint32_t cause, uint32_t tval, bool is_interrupt);
    
        // Determine Trap Level
        PrivilegeLevel getTrapTargetLevel(uint32_t cause, bool is_interrupt);



        const uint32_t fetchInstr(uint32_t pc);
        
        // === Execute and Decode === //

        // Decode a 32 bit instruction
        static DecodedInstr decode(uint32_t instr);

        inline const DecodedInstr& fetch_and_decode(uint32_t pc);

        // Decompress a 16 bit instruction into a 32 bit instruction
        static uint32_t decompress(uint16_t i);

        // Execute a decoded instruction
        void execute(const DecodedInstr& i);
        
        // === Execution Dispath === //
        
        using ExecFn = void (CPU::*)(const DecodedInstr&);

        // Instruction Execution Dispath Array
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
        void execLR_W(const DecodedInstr& i);
        void execSC_W(const DecodedInstr& i);
        void execAMOSWAP_W(const DecodedInstr& i);
        void execAMOADD_W(const DecodedInstr& i);
        void execAMOAND_W(const DecodedInstr& i);
        void execAMOOR_W(const DecodedInstr& i);
        void execAMOXOR_W(const DecodedInstr& i);
        void execAMOMAX_W(const DecodedInstr& i);
        void execAMOMAXU_W(const DecodedInstr& i);
        void execAMOMIN_W(const DecodedInstr& i);
        void execAMOMINU_W(const DecodedInstr& i);
        void execINVALID(const DecodedInstr& i);

        // Control Status Registers
        enum CSR {
            // ============================================================
            // Supervisor Trap Setup
            // ============================================================
            SSTATUS     = 0x100,
            SIE         = 0x104,
            STVEC       = 0x105,
            SCOUNTEREN  = 0x106,

            SENVCFG     = 0x10A,
            SSTATEEN0   = 0x10C,

            // ============================================================
            // Supervisor Trap Handling
            // ============================================================
            SSCRATCH    = 0x140,
            SEPC        = 0x141,
            SCAUSE      = 0x142,
            STVAL       = 0x143,
            SIP         = 0x144,

            // ============================================================
            // Supervisor Address Translation
            // ============================================================
            SATP        = 0x180,

            // ============================================================
            // Machine Information Registers (Read-Only)
            // ============================================================
            MVENDORID   = 0xF11,
            MARCHID     = 0xF12,
            MIMPID      = 0xF13,
            MHARTID     = 0xF14,

            // ============================================================
            // Machine Trap Setup
            // ============================================================
            MSTATUS     = 0x300,
            MISA        = 0x301,
            MEDELEG     = 0x302,
            MIDELEG     = 0x303,
            MIE         = 0x304,
            MTVEC       = 0x305,
            MCOUNTEREN  = 0x306,

            MENVCFG     = 0x30A,
            MSTATEEN0   = 0x30C,
            MSTATEEN1   = 0x30D,
            MSTATEEN2   = 0x30E,
            MSTATEEN3   = 0x30F,

            MSTATUSH    = 0x310,  
            MENVCFGH    = 0x31A,   

            MCOUNTINHIBIT = 0x320,

            // ============================================================
            // Machine Trap Handling
            // ============================================================
            MSCRATCH    = 0x340,
            MEPC        = 0x341,
            MCAUSE      = 0x342,
            MTVAL       = 0x343,
            MIP         = 0x344,

            // ============================================================
            // Physical Memory Protection (PMP)
            // ============================================================
            PMPCFG0     = 0x3A0,
            PMPCFG1     = 0x3A1,
            PMPCFG2     = 0x3A2,
            PMPCFG3     = 0x3A3,

            PMPADDR0    = 0x3B0,
            PMPADDR1    = 0x3B1,
            PMPADDR2    = 0x3B2,
            PMPADDR3    = 0x3B3,
            PMPADDR4    = 0x3B4,
            PMPADDR5    = 0x3B5,
            PMPADDR6    = 0x3B6,
            PMPADDR7    = 0x3B7,
            PMPADDR8    = 0x3B8,
            PMPADDR9    = 0x3B9,
            PMPADDR10   = 0x3BA,
            PMPADDR11   = 0x3BB,
            PMPADDR12   = 0x3BC,
            PMPADDR13   = 0x3BD,
            PMPADDR14   = 0x3BE,
            PMPADDR15   = 0x3BF,

            // ============================================================
            // Machine Counters
            // ============================================================
            MCYCLE      = 0xB00,
            MINSTRET    = 0xB02,

            MCYCLEH     = 0xB80,   
            MINSTRETH   = 0xB82,   

            // ============================================================
            // User Counter / Timer View
            // (Visible only if enabled through counteren CSRs)
            // ============================================================
            CYCLE       = 0xC00,
            TIME        = 0xC01,

            CYCLEH      = 0xC80,   
            TIMEH       = 0xC81,   

            // SSTC

            STIMECMP = 0x14D,
            STIMECMPH = 0x15D,

            // MSECC

            MSECCFG = 0x747,
            MSECCFGH = 0x757
        
        };

        // === Trace === //

        // Write a 32 bit integer in as hex string
        std::string hex32(uint32_t v) const;

        // Disassemble a decoded instruction to string format
        std::string disasm(const DecodedInstr& di) const;

        // Convert from CSR address to name if it is mapped
        std::string csrName(uint16_t csr) const;

        // Exception Causes
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

extern CPU* g_cpu;
