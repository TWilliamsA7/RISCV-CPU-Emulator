// src/cache/icache.cpp

#include "cache/icache.hpp"
#include "bus/bus.hpp"

ICache::ICache(Bus& bus) : bus_(bus) { }

static inline uint32_t index_from_page(uint32_t phys_page) {
    return phys_page % ICache::NUM_PAGES;
}

uint8_t* ICache::fetch_page(uint32_t phys_addr)
{
    uint32_t phys_page = phys_addr >> 12;
    uint32_t idx = index_from_page(phys_page);

    Entry& e = cache_[idx];

    if (e.valid && e.phys_page == phys_page)
    {
        return e.data; // HIT
    }

    // MISS → allocate or map page
    if (!e.data)
        e.data = new uint8_t[PAGE_SIZE];

    // load from memory system (IMPORTANT)
    memcpy(e.data,
           bus_.phys_ptr(phys_page << 12), // or read loop
           PAGE_SIZE);

    e.phys_page = phys_page;
    e.valid = true;

    return e.data;
}