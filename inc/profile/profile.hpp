// inc/profile/profile.hpp

#pragma once

#include "isa/isa.hpp"
#include <string>

enum class Platform {
    BARE_METAL,
    XV6,
    LINUX
};

struct Profile {
    std::string name = "Bare Metal";
    std::string source_path = "none";
    Platform platform = Platform::BARE_METAL;

    uint32_t starting_pc = 0x80000000;
    uint32_t dram_start = 0x80000000;
    uint32_t dram_size = 128 * 1000 * 1000;

    Extensions extensions;

    std::string elf_path;
    std::string disk_path;
    bool verbose;
};

Profile loadProfile(std::string profile_path);