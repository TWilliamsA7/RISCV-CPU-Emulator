#include "clint/clint.hpp"

uint32_t Clint::read32(uint32_t offset) {
    switch (offset) {
        case MSIP_OFFSET:  return msip ? 1 : 0;
        case MTIMECMP_LO:  return (uint32_t)(mtimecmp);
        case MTIMECMP_HI:  return (uint32_t)(mtimecmp >> 32);
        case MTIME_LO:     return (uint32_t)(mtime);
        case MTIME_HI:     return (uint32_t)(mtime >> 32);
        default:           return 0;
    }
}

void Clint::write32(uint32_t offset, uint32_t val) {
    switch (offset) {
        case MSIP_OFFSET:  msip = val & 1; break;
        case MTIMECMP_LO:
            mtimecmp = (mtimecmp & 0xFFFFFFFF00000000ULL) | val;
            break;
        case MTIMECMP_HI:
            mtimecmp = (mtimecmp & 0x00000000FFFFFFFFULL) | ((uint64_t)val << 32);
            break;
        // mtime is read-only from bus perspective in most implementations
        default: break;
    }
}

void Clint::tick() { mtime++; }