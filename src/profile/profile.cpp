// src/profile/profile.cpp

#include "profile/profile.hpp"
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

Profile loadProfile(std::string profile_path) {

    std::ifstream file(profile_path);
    json config;
    file >> config;

    Profile profile;

    profile.source_path = profile_path;

    if (config.contains("name"))
        profile.name = config["name"];
        
    if (config["platform"] == "bare-metal") {
        profile.platform = Platform::BARE_METAL;
    } else if (config["platform"] == "xv6") {
        profile.platform = Platform::XV6;
    } else if (config["platform"] == "linux") {
        profile.platform = Platform::LINUX;
    } else {
        throw new std::runtime_error("[ERROR] Invalid platform selected");
    }

    if (config.contains("dram-start")) {
        std::string ds = config["dram-start"];
        profile.dram_start = std::stoi(ds, nullptr, 16);
    }

    if (config.contains("dram-size")) {
        std::string ds = config["dram-size"];
        profile.dram_size = std::stoi(ds, nullptr, 16);
    }

    if (config.contains("starting-pc")) {
        std::string sp = config["starting-pc"];
        profile.starting_pc = std::stoi(sp, nullptr, 16);
    }

    if (config.contains("extensions") && config["extensions"].is_array()) {
        for (auto& arg : config["extensions"]) {
                std::string extension = arg.get<std::string>();
                if (extension == "m") {
                    profile.extensions.m = true;
                } else if (extension == "c") {
                    profile.extensions.c = true;
                } else if (extension == "a") {
                    profile.extensions.a = true;
            }
        }
    }

    profile.verbose = config.value("verbose", false);

    if (config.contains("elf-path")) {
        profile.elf_path = config["elf-path"];
    } else {
        throw new std::runtime_error("[ERROR] Missing Elf Path");
    }

    if (profile.platform == Platform::XV6 || profile.platform == Platform::LINUX) {
        if (config.contains("disk-path")) {
            profile.disk_path = config["disk-path"];
        } else {
            throw new std::runtime_error("[ERROR] Cannot Run Selected Platform Without Disk");
        }
    }

    return profile;
}

