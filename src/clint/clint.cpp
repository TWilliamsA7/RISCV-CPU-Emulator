#include "clint/clint.hpp"

Clint::Clint() : start_time_(std::chrono::high_resolution_clock::now()) {}

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

    //printf("CLINT Write: offset =0x%x, val=0x%x\n", offset, val);

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

void Clint::updateMtime() {
    std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
    uint64_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time_).count();
    mtime = (elapsed * frequency_)  / 1000000;
}