// src/memory/memory.cpp

#include "memory/memory.hpp"
#include <stdexcept>
#include <filesystem>
#include <iostream>
#include <fstream>

Memory::Memory(size_t size) : data_(size, 0) {}

uint8_t Memory::read8(uint32_t addr) const {
    checkAddress(addr, 1);
    return data_[addr];
}

uint16_t Memory::read16(uint32_t addr) const {
    checkAddress(addr, 2);
    return data_[addr] |
           (data_[addr + 1] << 8);
}

uint32_t Memory::read32(uint32_t addr) const {
    checkAddress(addr, 4);
    return data_[addr] |
           (data_[addr + 1] << 8) |
           (data_[addr + 2] << 16) |
           (data_[addr + 3] << 24);
}

void Memory::write8(uint32_t addr, uint8_t val) {
    checkAddress(addr, 1);
    data_[addr] = val;
}

void Memory::write16(uint32_t addr, uint16_t val) {
    checkAddress(addr, 2);
    data_[addr] = val & 0xFF;
    data_[addr + 1] = (val >> 8) & 0xFF;
}

void Memory::write32(uint32_t addr, uint32_t val) {
    checkAddress(addr, 4);
    data_[addr] = val & 0xFF;
    data_[addr + 1] = (val >> 8) & 0xFF;
    data_[addr + 2] = (val >> 16) & 0xFF;
    data_[addr + 3] = (val >> 24) & 0xFF;
}

size_t Memory::size() const { return data_.size(); }

void Memory::loadBinary(const std::string& filePath, uint32_t startAddr) {
    std::ifstream inputFile(filePath, std::ios::binary);

    // Check if the file opened successfully
    if (!inputFile.is_open()) {
        std::cerr << "Error: Unable to open file " << filePath << std::endl;
        return;
    }

    // 2. Get the size of the file using std::filesystem::file_size (C++17).
    // An alternative (pre-C++17) is to use seekg and tellg.
    std::size_t fileSize = std::filesystem::file_size(filePath);

    // 3. Create a vector of std::byte with the exact size of the file.
    std::vector<std::uint8_t> buffer(fileSize);

    // 4. Read the entire file content into the vector's data buffer.
    // We must reinterpret_cast<char*> because std::ifstream::read takes a char* buffer.
    inputFile.read(reinterpret_cast<char*>(buffer.data()), fileSize);

    // 5. Close the file.
    inputFile.close();

    loadBinary(buffer, startAddr);
}

void Memory::loadBinary(const std::vector<uint8_t>& program, uint32_t startAddr) {
    checkAddress(startAddr, program.size());
    std::copy(program.begin(), program.end(), data_.begin() + startAddr);
}

void Memory::reset() {
    std::fill(data_.begin(), data_.end(), 0);
}

void Memory::checkAddress(uint32_t addr, size_t width) const {
    if (addr + width > data_.size()) {
        throw std::out_of_range("Memory access out of bounds");
    }
}

