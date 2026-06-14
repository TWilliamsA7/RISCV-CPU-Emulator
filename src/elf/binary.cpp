#include "elf/elf.hpp"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <stdexcept>

void load_binary(const std::string& filename, Bus& bus, uint32_t addr) {
     std::ifstream inputFile(filename, std::ios::binary);

    // Check if the file opened successfully
    if (!inputFile.is_open()) {
        throw new std::runtime_error("Error: Unable to open binary");
    }

    std::size_t fileSize = std::filesystem::file_size(filename);
    std::vector<std::uint8_t> buffer(fileSize);
    inputFile.read(reinterpret_cast<char*>(buffer.data()), fileSize);

    // Close the file.
    inputFile.close();

    bus.load_binary(buffer.begin(), buffer.end(), addr);
}

