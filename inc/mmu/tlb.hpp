// inc/mmu/tlb.hpp

#pragma once

#include <cstdint>
#include <cstring>

// ----------------------------------------------------------------------------
// RV32 Sv32 TLB — 64-entry, 4-way set-associative, 16 sets
//
// Address split (Sv32):
//   VA[31:22] = VPN[1]  (10 bits)
//   VA[21:12] = VPN[0]  (10 bits)
//   VA[11:0]  = page offset
//
// TLB geometry:
//   sets      = 16   (indexed by vpn[3:0])
//   ways      = 4
//   total     = 64 entries
//   tag       = vpn[19:4] | asid  (16 + 9 = 25 bits, ignoring asid for globals)
//   replace   = 3-bit pseudo-LRU tree per set
// ----------------------------------------------------------------------------

static constexpr int TLB_WAYS  = 4;
static constexpr int TLB_SETS  = 16;

// PTE permission/status flags mirrored from the walked PTE.
// Matches RISC-V PTE layout: D A G U X W R V (bits 7..0).
struct TLBFlags {
    bool read;      // PTE.R
    bool write;     // PTE.W
    bool execute;   // PTE.X
    bool user;      // PTE.U — page accessible in U-mode
    bool global;    // PTE.G — ASID-independent; only flushed by sfence.vma x0,x0
    bool accessed;  // PTE.A — set by walker before fill
    bool dirty;     // PTE.D — set by walker before fill (on stores)
};

struct TLBEntry {
    uint32_t vpn;        // full 20-bit VPN (va[31:12]), zero-extended to 32 bits
    uint16_t asid;       // 9-bit ASID captured from satp at fill time
    uint32_t ppn;        // 22-bit PPN (pa[33:12] in Sv32)
    TLBFlags flags;
    bool     valid;      // slot is occupied
    bool     superpage;  // true → 4 MB superpage; only vpn[19:10] was matched
};


struct PLRUState {
    uint8_t bits; // only low 3 bits used
};

struct TLB {
    TLBEntry  entries[TLB_SETS][TLB_WAYS];
    PLRUState plru[TLB_SETS];

    // Construct zeroed, all entries invalid.
    TLB() { reset(); }

    void reset() {
        std::memset(entries, 0, sizeof(entries));
        std::memset(plru,    0, sizeof(plru));
    }
};

// ----------------------------------------------------------------------------
// Public API
// ----------------------------------------------------------------------------

// Look up a virtual page number + ASID.
// Returns a pointer to the matching entry on hit, nullptr on miss.
// Updates PLRU on hit.
// `asid`  — current satp.ASID
// `vpn`   — va[31:12]
TLBEntry* tlb_lookup(TLB& tlb, uint32_t vpn, uint16_t asid);

// Insert (or replace) a mapping.  Called after a successful page-table walk.
// `ppn`        — physical page number (pa[33:12])
// `flags`      — permissions/status from the leaf PTE
// `superpage`  — true if the walk terminated at level 1 (4 MB page)
void tlb_fill(TLB& tlb, uint32_t vpn, uint16_t asid,
              uint32_t ppn, TLBFlags flags, bool superpage = false);

// sfence.vma variants --------------------------------------------------------

// sfence.vma x0, x0 — flush everything (including globals)
void tlb_flush_all(TLB& tlb);

// sfence.vma x0, rs2 — flush all entries matching `asid` (non-globals only)
void tlb_flush_asid(TLB& tlb, uint16_t asid);

// sfence.vma rs1, x0 — flush the single entry matching `va` (any ASID)
void tlb_flush_va(TLB& tlb, uint32_t va);

// sfence.vma rs1, rs2 — flush the entry matching both `va` and `asid`
void tlb_flush_va_asid(TLB& tlb, uint32_t va, uint16_t asid);