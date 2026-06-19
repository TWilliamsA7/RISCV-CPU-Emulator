// inc/cache/cache_entry.hpp

#include "isa/isa.hpp"

struct CacheEntry {
    uint32_t epoch = 0;
    uint32_t pc = 0xFFFFFFFF;
    DecodedInstr decoded;
};