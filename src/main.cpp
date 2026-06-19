#include "emulator.hpp"
#include <iostream>


int main(int argc, char** argv) {

    EmulatorConfig config;

    if (argc < 2) {
        std::cerr << "[ERROR] Usage: ./riscv_emulator [profile.json]\n";
        return 1;
    } else if (argc > 2) {
        std::cout << "[WARN] Excess Arguments Detected\n";
    }

    Profile profile;

    try {
        profile = loadProfile(argv[1]);
    } catch (std::runtime_error& e) {
        std::cerr << e.what();
        return 1;
    }

    std::cout << "[INFO] Successfully loaded profile from: " << profile.source_path << std::endl;

    config.profile = profile;
    config.cpu_config.extensions = config.profile.extensions;
    config.cpu_config.mode = config.profile.platform == Platform::BARE_METAL ? ExecutionMode::BARE_METAL : ExecutionMode::SYSTEM;
    config.cpu_config.starting_pc = config.profile.starting_pc;

    try {
        Emulator emulator(config);
        emulator.initialize();
        return emulator.emulate();
    } catch (std::runtime_error& e) {
        std::cerr << e.what();
        return 1;
    }
}
