// inc/cache/icache.hpp

#pragma once

#include <cstdint>
#include <array>

class Bus;

class ICache {
    public:
        ICache(Bus& bus);

        static constexpr uint32_t PAGE_SIZE = 4096;
        static constexpr uint32_t NUM_PAGES = 64;

        uint8_t* fetch_page(uint32_t phys_addr);

    private:
        struct Entry {
            uint32_t phys_page = 0;
            uint8_t* data = nullptr;
            bool valid = false;
        };

        Bus& bus_;

        std::array<Entry, NUM_PAGES> cache_;
};