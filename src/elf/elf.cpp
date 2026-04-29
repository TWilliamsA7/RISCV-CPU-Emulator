// src/elf/elf.cpp

#include "elf/elf.hpp"

#include <fstream>

void load_elf(const std::string& filename, Bus& bus, CPU& cpu) {
    std::ifstream file(filename, std::ios::binary);
    Elf32_Ehdr header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    // Verify RISC-V
    if (header.e_machine != 0xF3) throw std::runtime_error("Not a RISC-V ELF");

    // Set initial PC
    cpu.setPC(header.e_entry);

    // Load segments
    file.seekg(header.e_phoff);
    for (int i = 0; i < header.e_phnum; ++i) {
        Elf32_Phdr phdr;
        file.read(reinterpret_cast<char*>(&phdr), sizeof(phdr));

        if (phdr.p_type == 1) { // PT_LOAD
            std::vector<uint8_t> buffer(phdr.p_filesz);
            auto current_pos = file.tellg();
            file.seekg(phdr.p_offset);
            file.read(reinterpret_cast<char*>(buffer.data()), phdr.p_filesz);
            
            for (uint32_t j = 0; j < phdr.p_filesz; ++j) {
                bus.write8(phdr.p_paddr + j, buffer[j]);
            }

            for (uint32_t j = phdr.p_filesz; j < phdr.p_memsz; ++j) {
                bus.write8(phdr.p_paddr + j, 0);
            }

            file.seekg(current_pos);
        }
    }
}