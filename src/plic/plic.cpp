// src/plic/plic.cpp

#include "plic/plic.hpp"

PLIC::PLIC() {
    priority_.fill(0);
    enable_.fill(0);
    threshold_.fill(0);
    claimed_.fill(0);
    // pending_ initialised to 0 by atomic constructor
}

uint32_t PLIC::read32(uint32_t offset) {
    uint32_t pending = pending_.load(std::memory_order_acquire);

    if (offset < 0x1000)
        return priority_[(offset >> 2) & 0x1F];
    if (offset == 0x1000)
        return pending;
    if (offset >= 0x2000 && offset < 0x2100) {
        uint32_t ctx = (offset - 0x2000) / 0x80;
        if (ctx < 2) return enable_[ctx];
    }
    if (offset >= 0x200000) {
        uint32_t ctx   = (offset - 0x200000) / 0x1000;
        uint32_t local = (offset - 0x200000) % 0x1000;
        if (ctx < 2) {
            if (local == 0) return threshold_[ctx];
            if (local == 4) {
                uint32_t src = best_pending(ctx);
                if (src != 0) {
                    claimed_[ctx] = src;
                    // Clear pending atomically.
                    pending_.fetch_and(~(1u << src), std::memory_order_acq_rel);
                }
                return src;
            }
        }
    }
    return 0;
}

void PLIC::write32(uint32_t offset, uint32_t val) {
    if (offset >= 0x4 && offset < 0x1000)
        priority_[(offset >> 2) & 0x1F] = val & 0x7;
    else if (offset >= 0x2000 && offset < 0x2100) {
        uint32_t ctx = (offset - 0x2000) / 0x80;
        if (ctx < 2) enable_[ctx] = val & ~1u;
    }
    else if (offset >= 0x200000) {
        uint32_t ctx   = (offset - 0x200000) / 0x1000;
        uint32_t local = (offset - 0x200000) % 0x1000;
        if (ctx < 2) {
            if (local == 0) threshold_[ctx] = val & 0x7;
            if (local == 4) {
                pending_.fetch_and(~(1u << val), std::memory_order_acq_rel);
                claimed_[ctx] = 0;
            }
        }
    }
}

uint32_t PLIC::best_pending(uint32_t ctx) const {
    uint32_t active = pending_.load(std::memory_order_acquire) & enable_[ctx];
    uint32_t best_src = 0;
    uint32_t best_pri = threshold_[ctx];
    for (uint32_t src = 1; src < NUM_SOURCES; src++) {
        if ((active >> src) & 1) {
            if (priority_[src] > best_pri) {
                best_pri = priority_[src];
                best_src = src;
            }
        }
    }
    return best_src;
}

void PLIC::set_pending(uint32_t source) {
    if (source > 0 && source < NUM_SOURCES)
        pending_.fetch_or(1u << source, std::memory_order_acq_rel);
}

void PLIC::clear_pending(uint32_t source) {
    if (source > 0 && source < NUM_SOURCES)
        pending_.fetch_and(~(1u << source), std::memory_order_acq_rel);
}

bool PLIC::m_interrupt_pending() const { return best_pending(CTX_MACHINE)    != 0; }
bool PLIC::s_interrupt_pending() const { return best_pending(CTX_SUPERVISOR) != 0; }