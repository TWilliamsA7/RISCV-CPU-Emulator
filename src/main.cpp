#include "cpu/cpu.hpp"
#include "elf/elf.hpp"
#include "errors/errors.hpp"
#include <iostream>
#include <string>
#include <unistd.h>

int main(int argc, char** argv) {

    CPUConfig config;
    std::string uart_input;
    std::string wfi_uart_input;
    bool use_sbi = false;
    int opt;

    while ((opt = getopt(argc, argv, "bmcvaU:W:s")) != -1) {
        switch (opt) {
            case 'm': config.extension_m = true; break;
            case 'c': config.extension_c = true; break;
            case 'a': config.extension_a = true; break;
            case 'v': config.verbose = true; break;
            case 'b': config.mode = ExecutionMode::BARE_METAL; break;
            case 'U': uart_input = optarg; break;
            case 'W': wfi_uart_input = optarg; break;
            case 's': use_sbi = true; break;
            default:
                std::cerr << "Usage: " << argv[0] << " [-m] [-c] [-b] [-a] [-U uart_input] [-W wfi_uart_input] [-s] <elf_file> <bin>" << std::endl;
                return 1;
        }
    }

    if (optind >= argc) {
        std::cerr << "Expected ELF file after options" << std::endl;
        return 1;
    }

    char* elf_path = argv[optind];
    char* disk_path = argv[optind + 1];


    Clint clint;
    PLIC plic;
    Bus bus = Bus(clint, plic, disk_path);
    CPU cpu(config, bus, clint, plic);

    load_elf(elf_path, bus, &cpu);


    if (!uart_input.empty())
        bus.inject_uart_input(uart_input);
    if (!wfi_uart_input.empty())
        bus.defer_uart_input_until_wfi(wfi_uart_input);

    try {
        cpu.run();
    } catch (const ProgramExit& e) {
        return e.code;
    }

    return 0;
}
