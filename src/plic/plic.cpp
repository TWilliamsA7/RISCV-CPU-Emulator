// src/plic/plic.cpp

#include "plic/plic.hpp"

PLIC::PLIC()
    : pending_(0) {
    priority_.fill(0);
    enable_.fill(0);
    threshold_.fill(0);
    claimed_.fill(0);
}

uint32_t PLIC::read32(uint32_t offset) const {
    if (offset < 0x1000)  // Priority
        return priority_[(offset >> 2) & 0x1F];
    if (offset == 0x1000) // Pending
        return pending_;
    if (offset >= 0x2000 && offset < 0x2100) { // Enable
        uint32_t ctx = (offset - 0x2000) / 0x80;
        if (ctx < 2) return enable_[ctx];
    }
    if (offset >= 0x200000) { // Threshold / Claim
        uint32_t ctx = (offset - 0x200000) / 0x1000;
        uint32_t local = (offset - 0x200000) % 0x1000;
        if (ctx < 2) {
            if (local == 0) return threshold_[ctx];
            if (local == 4) return best_pending(ctx); // claim
        }
    }
    return 0;
}

void PLIC::write32(uint32_t offset, uint32_t val) {
    if (offset >= 0x4 && offset < 0x1000) // Priority (skip source 0)
        priority_[(offset >> 2) & 0x1F] = val & 0x7;
    else if (offset >= 0x2000 && offset < 0x2100) {
        uint32_t ctx = (offset - 0x2000) / 0x80;
        if (ctx < 2) enable_[ctx] = val & ~1u; // source 0 always disabled
    }
    else if (offset >= 0x200000) {
        uint32_t ctx = (offset - 0x200000) / 0x1000;
        uint32_t local = (offset - 0x200000) % 0x1000;
        if (ctx < 2) {
            if (local == 0) threshold_[ctx] = val & 0x7;
            if (local == 4) { // complete
                pending_ &= ~(1u << val);  // clear the pending bit
                claimed_[ctx] = 0;
            }
        }
    }
}

uint32_t PLIC::best_pending(uint32_t ctx) const {
    uint32_t active = pending_ & enable_[ctx];
    uint32_t best_src = 0;
    uint32_t best_pri = threshold_[ctx]; // must exceed threshold
    for (uint32_t src = 1; src < NUM_SOURCES; src++) {
        if ((active >> src) & 1) {
            if (priority_[src] > best_pri) {
                best_pri = priority_[src];
                best_src = src;
            }
        }
    }
    return best_src; // 0 means nothing to claim
}

void PLIC::set_pending(uint32_t source) {
    if (source > 0 && source < NUM_SOURCES) {
        pending_ |= (1U << source);
    }
}

void PLIC::clear_pending(uint32_t source) {
    if (source > 0 && source < NUM_SOURCES) {
        pending_ &= ~(1U << source);
    }
}

bool PLIC::m_interrupt_pending() const { return best_pending(CTX_MACHINE) != 0; }
bool PLIC::s_interrupt_pending() const { return best_pending(CTX_SUPERVISOR) != 0; }
