#include "emulator.hpp"
#include <iostream>
#include <unistd.h>

int main(int argc, char** argv) {

    EmulatorConfig config;

    int opt;

    while ((opt = getopt(argc, argv, "bmcva")) != -1) {
        switch (opt) {
            case 'm': config.cpu_config.extensions.m = true; break;
            case 'c': config.cpu_config.extensions.c = true; break;
            case 'a': config.cpu_config.extensions.a = true; break;
            case 'v': config.verbose = true; break;
            case 'b': config.cpu_config.mode = ExecutionMode::BARE_METAL; break;
            default:
                std::cerr << "Usage: " << argv[0] << " [-m] [-c] [-b] [-a] <elf_file> <disk>" << std::endl;
                return 1;
        }
    }

    if (optind >= argc) {
        std::cerr << "Expected ELF file after options" << std::endl;
        return 1;
    }

    config.elf_path = argv[optind];
    config.disk_path = argv[optind + 1];

    try {
        Emulator emulator(config);
        return emulator.emulate();
    } catch (std::runtime_error& e) {
        std::cerr << e.what();
        return 1;
    }
}
