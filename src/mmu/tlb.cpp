// src/mmu/tlb.cpp

#include "mmu/tlb.hpp"

static inline int set_of(uint32_t vpn) {
    return static_cast<int>(vpn & 0xF);
}

static int plru_victim(uint8_t state) {
    // bit2==0 → left (W0,W1) recently used → right subtree is victim
    // bit2==1 → right (W2,W3) recently used → left subtree is victim
    if ((state >> 2) & 1) {
        // left subtree is LRU; bit1 decides W0 vs W1
        return ((state >> 1) & 1) ? 0 : 1;
    } else {
        // right subtree is LRU; bit0 decides W2 vs W3
        return ((state >> 0) & 1) ? 2 : 3;
    }
}

static uint8_t plru_update(uint8_t state, int way) {
    switch (way) {
        case 0:
            // accessed left→left; mark bit2=0 (left recently used), bit1=0 (W0 recently used)
            state = (state & 0b001) | 0b000;
            break;
        case 1:
            // accessed left→right; mark bit2=0, bit1=1
            state = (state & 0b001) | 0b010;
            break;
        case 2:
            // accessed right→left; mark bit2=1, bit0=0
            state = (state & 0b010) | 0b100;
            break;
        case 3:
            // accessed right→right; mark bit2=1, bit0=1
            state = (state & 0b010) | 0b101;
            break;
        default:
            break;
    }
    return state;
}

TLBEntry* tlb_lookup(TLB& tlb, uint32_t vpn, uint16_t asid) {
    const int set = set_of(vpn);
 
    for (int way = 0; way < TLB_WAYS; ++way) {
        TLBEntry& e = tlb.entries[set][way];
 
        if (!e.valid)
            continue;
 
        // VPN match: for superpages, only the top 10 bits (VPN[1]) matter.
        bool vpn_match;
        if (e.superpage) {
            vpn_match = ((e.vpn >> 10) == (vpn >> 10));
        } else {
            vpn_match = (e.vpn == vpn);
        }
 
        if (!vpn_match)
            continue;
 
        // ASID match: global entries are always a match regardless of ASID.
        if (!e.flags.global && e.asid != asid)
            continue;
 
        // Hit — update PLRU and return the entry.
        tlb.plru[set].bits = plru_update(tlb.plru[set].bits, way);
        return &e;
    }
 
    return nullptr; // miss
}

void tlb_fill(TLB& tlb, uint32_t vpn, uint16_t asid,
              uint32_t ppn, TLBFlags flags, bool superpage) {
    const int set = set_of(vpn);
 
    // First pass: look for an existing entry for this (vpn, asid) to update
    // rather than evict an unrelated entry.
    for (int way = 0; way < TLB_WAYS; ++way) {
        TLBEntry& e = tlb.entries[set][way];
        if (e.valid && e.vpn == vpn &&
            (e.flags.global || e.asid == asid)) {
            e.ppn       = ppn;
            e.flags     = flags;
            e.asid      = asid;
            e.superpage = superpage;
            tlb.plru[set].bits = plru_update(tlb.plru[set].bits, way);
            return;
        }
    }
 
    // Second pass: look for an invalid (empty) slot.
    for (int way = 0; way < TLB_WAYS; ++way) {
        TLBEntry& e = tlb.entries[set][way];
        if (!e.valid) {
            e.vpn       = vpn;
            e.asid      = asid;
            e.ppn       = ppn;
            e.flags     = flags;
            e.superpage = superpage;
            e.valid     = true;
            tlb.plru[set].bits = plru_update(tlb.plru[set].bits, way);
            return;
        }
    }
 
    // All ways occupied — evict the PLRU victim.
    const int victim = plru_victim(tlb.plru[set].bits);
    TLBEntry& e = tlb.entries[set][victim];
    e.vpn       = vpn;
    e.asid      = asid;
    e.ppn       = ppn;
    e.flags     = flags;
    e.superpage = superpage;
    e.valid     = true;
    tlb.plru[set].bits = plru_update(tlb.plru[set].bits, victim);
}

void tlb_flush_all(TLB& tlb) {
    tlb.reset();
}
 
void tlb_flush_asid(TLB& tlb, uint16_t asid) {
    for (int set = 0; set < TLB_SETS; ++set) {
        for (int way = 0; way < TLB_WAYS; ++way) {
            TLBEntry& e = tlb.entries[set][way];
            if (e.valid && !e.flags.global && e.asid == asid) {
                e.valid = false;
            }
        }
    }
}

void tlb_flush_va(TLB& tlb, uint32_t va) {
    const uint32_t vpn = va >> 12;
    const int      set = set_of(vpn);
 
    for (int way = 0; way < TLB_WAYS; ++way) {
        TLBEntry& e = tlb.entries[set][way];
        if (!e.valid) continue;
 
        bool vpn_match = e.superpage
            ? ((e.vpn >> 10) == (vpn >> 10))
            : (e.vpn == vpn);
 
        if (vpn_match)
            e.valid = false;
    }
}

void tlb_flush_va_asid(TLB& tlb, uint32_t va, uint16_t asid) {
    const uint32_t vpn = va >> 12;
    const int      set = set_of(vpn);
 
    for (int way = 0; way < TLB_WAYS; ++way) {
        TLBEntry& e = tlb.entries[set][way];
        if (!e.valid) continue;
 
        bool vpn_match = e.superpage
            ? ((e.vpn >> 10) == (vpn >> 10))
            : (e.vpn == vpn);
 
        if (vpn_match && (e.flags.global || e.asid == asid))
            e.valid = false;
    }
}