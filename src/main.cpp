#include "cpu/cpu.hpp"
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: ./emulator {program.bin}\n";
        return 1;
    }

    Memory mem{4096};
    CPU cpu{mem, true};

    mem.loadBinary(argv[1], 0);

    cpu.run(1024);
    return 0;
}
