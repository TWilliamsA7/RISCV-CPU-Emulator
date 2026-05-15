#include "cpu/cpu.hpp"
#include "elf/elf.hpp"
#include <iostream>
#include <unistd.h>

int main(int argc, char** argv) {

    CPUConfig config;
    int opt;

    while ((opt = getopt(argc, argv, "bmcva")) != -1) {
        switch (opt) {
            case 'm': config.extension_m = true; break;
            case 'c': config.extension_c = true; break;
            case 'a': config.extension_a = true; break;
            case 'v': config.verbose = true; break;
            case 'b': config.mode = ExecutionMode::BARE_METAL; break;
            default:
                std::cerr << "Usage: " << argv[0] << " [-m] [-c] [-b] [-a] <elf_file>" << std::endl;
                return 1;
        }
    }

    if (optind >= argc) {
        std::cerr << "Expected ELF file after options" << std::endl;
        return 1;
    }

    char* elf_path = argv[optind];

    Clint clint;
    PLIC plic;
    Bus bus = Bus(clint, plic);
    CPU cpu(config, bus, clint, plic);

    load_elf(elf_path, bus, cpu);

    cpu.run();

    return 0;
}
