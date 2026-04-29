#include "cpu/cpu.hpp"
#include "elf/elf.hpp"
#include <iostream>

int main(int argc, char** argv) {

    if (argc < 2) {
        std::cout << "Usage: ./emulator {program.elf}\n";
        return 1;
    }

    Bus bus;
    CPU cpu(bus);

    load_elf(argv[1], bus, cpu);

    cpu.run();

    return 0;
}
