// inc/elf/elf.hpp

#include "cpu/cpu.hpp"
#include "bus/bus.hpp"

#include <cstdint>
#include <string>

struct Elf32_Ehdr {
    unsigned char e_ident[16]; // Magic numbers and class info
    uint16_t      e_type;      // Object file type (Executable, DYN, etc.)
    uint16_t      e_machine;   // Architecture (0xF3 for RISC-V)
    uint32_t      e_version;   // Object file version
    uint32_t      e_entry;     // Entry point virtual address
    uint32_t      e_phoff;     // Program header table file offset
    uint32_t      e_shoff;     // Section header table file offset
    uint32_t      e_flags;     // Processor-specific flags
    uint16_t      e_ehsize;    // ELF header size
    uint16_t      e_phentsize; // Size of one program header table entry
    uint16_t      e_phnum;     // Number of program header table entries
    uint16_t      e_shentsize; // Size of one section header table entry
    uint16_t      e_shnum;     // Number of section header table entries
    uint16_t      e_shstrndx;  // Section header string table index
};

struct Elf32_Phdr {
    uint32_t p_type;
    uint32_t p_offset; // Location of data in the file
    uint32_t p_vaddr;  // Virtual address in RAM
    uint32_t p_paddr;  // Physical address in RAM
    uint32_t p_filesz; // Size of data in the file
    uint32_t p_memsz;  // Size of data in memory (can be larger for .bss)
    uint32_t p_flags;
    uint32_t p_align;
};

void load_elf(const std::string& filename, Bus& bus, CPU& cpu);