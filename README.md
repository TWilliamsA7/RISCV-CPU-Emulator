# RV32 System Emulator

A cycle-driven RISC-V (RV32) emulator implementing Machine/Supervisor/User privilege modes, Sv32 paging, and a virtio-based peripheral set sufficient to boot a Linux guest via OpenSBI. Built in C++20, targeting both Windows (MSVC/UCRT64) and Linux/WSL hosts.

---

## Features

### Core Architecture (RV32)

- **Base ISA**: RV32I, fetch/decode/execute pipeline with a direct dispatch table (`CPU::dispatch_`) mapping `InstrKind` → handler function.
- **Extensions**: `M` (integer multiply/divide), `A` (atomics: `LR.W`/`SC.W`, full AMO family), `C` (16-bit compressed instruction decompression into canonical 32-bit form).
- **Instruction Decode Cache**: A direct-mapped, epoch-tagged decode cache (`CacheEntry[4096]`) avoids re-decoding hot loops; invalidated via epoch bump on `SFENCE.VMA` or self-modifying writes.
- **Instruction Cache (`ICache`)**: Page-granularity (4 KB) physical-memory cache with 64 entries, invalidated on DRAM writes and `SFENCE.VMA`.
- **CSR File**: Full 4096-entry CSR array with machine/supervisor CSR read/write gating by privilege level, including PMP (`pmpcfgN`/`pmpaddrN`), counters (`mcycle`/`minstret` + high halves), and platform-conditional trapping of unsupported extensions (Zihpm, Sscofpmf, Smaia) when targeting Linux profiles.
- **SBI Layer**: In-CPU SBI call handling (`handleSBI`) supporting legacy (v0.1) extensions (`set_timer`, `console_putchar`, `console_getchar`, IPI, shutdown) plus Base, Timer (`0x54494D45`), HSM (`0x48534D`), and System Reset (`0x53525354`) SBI 2.0 extensions.

### Memory & Privileged Architecture

- **Privilege Levels**: Machine, Supervisor, and User modes (`PrivilegeLevel` enum), with `MRET`/`SRET` handling MPP/SPP restoration and MPRV clearing.
- **MMU**: Sv32 two-level page table walker (`MMU::translate_walk`) with PTE permission checks (R/W/X/U), accessed/dirty bit updates written back to memory, and superpage (4 MB) support.
- **TLB**: 64-entry, 4-way set-associative, 16-set TLB with per-set pseudo-LRU replacement, ASID tagging, and global-page handling. Supports selective flush variants matching `SFENCE.VMA` semantics (`x0,x0`; `x0,rs2`; `rs1,x0`; `rs1,rs2`).
- **PMP**: 16-entry Physical Memory Protection with TOR, NA4, and NAPOT address-matching modes, and lock-bit enforcement (including TOR-lock propagation to the preceding entry).
- **Trap/Interrupt Controller**: Cause-based trap delegation (`mideleg`/`medeleg`) with vectored (`mtvec`/`stvec` mode bit) and direct dispatch, full `mstatus`/`sstatus` field shadowing, and a priority-ordered interrupt scan (`checkInterrupts`).
- **CLINT**: Machine-mode timer (`mtime`/`mtimecmp`) and software interrupt (`msip`) device, wall-clock-driven via `std::chrono`.
- **PLIC**: 32-source platform-level interrupt controller with per-context (M/S) priority thresholds, enable masks, and atomic `pending_` bitmask safe for concurrent poll-thread/CPU-thread access.

### Peripherals / I/O

- **UART (16550-compatible)**: Full register set (RBR/THR/IER/IIR/FCR/LCR/MCR/LSR/MSR/SCR), background input thread with platform-specific raw terminal handling (POSIX termios / Windows console mode), and an escape-sequence (`Ctrl+A`) hotkey for emulator quit or literal byte injection.
- **VirtIO Block Device**: MMIO virtio-blk (device ID 2) with descriptor-chain request processing (`VIRTIO_BLK_T_IN`/`_OUT`), disk-image-backed storage, and used-ring/interrupt notification.
- **VirtIO Net Device**: MMIO virtio-net (device ID 1) backed by **libslirp**, running an isolated poll thread that owns all `slirp_*` calls. Supports DHCP (guest obtains `10.0.2.15`), gateway ARP/ICMP, and intra-subnet UDP. TX frames cross threads via a mutex-guarded queue + wakeup pipe; RX frames are injected directly into guest virtqueues from the poll thread.
- **Framebuffer + Input**: SDL2-backed linear framebuffer (640×400, ARGB8888) with a keyboard input queue (DOOM-style keymap), driven by a dedicated render thread.
- **Host-Exit Hook**: A magic MMIO write (`0x80001000`) signals pass/fail exit codes for automated test harnesses (`riscv-tests`-style convention).

---

## Architecture Overview

The emulator is a **single-hart, single-threaded core** (`CPU::run` → `CPU::step`) surrounded by asynchronous I/O threads (UART input, libslirp poll loop, SDL render loop) that communicate via mutex-protected queues and atomics — never by directly poking CPU/MMU state.

Each `CPU::step()` performs:

1. **Interrupt check** (`checkInterrupts`) — scans `mip & mie` in fixed priority order (11, 3, 7, 9, 1, 5) and traps if a delegated/enabled interrupt is pending.
2. **Fetch + Decode** (`fetch_and_decode`) — resolves the PC through the epoch-tagged decode cache; on miss, performs an MMU translation (`AccessType::FETCH`), pulls the physical page through the `ICache`, and decompresses 16-bit instructions on the fly if `C` is enabled.
3. **Execute** (`execute`) — dispatches through a static `std::array<ExecFn, InstrKind::COUNT>` function-pointer table (`CPU::dispatch_`), avoiding a giant `switch`.
4. **Retire** — updates `minstret`/`minstreth`, advances `pc_` (either sequential or via `next_pc_` for control-flow instructions), and optionally emits a structured `StepResult` trace (`RegWrite`/`MemWrite`/`CsrWrite`) for `--verbose` execution logging.

Memory accesses route through `Bus`, which owns DRAM and dispatches MMIO ranges to `UART`, `CLINT`, `PLIC`, `VirtioBlk`, `VirtioNet`, `FrameBuffer`, and `InputDevice` by address range. All cross-thread paths (virtio-net poll thread, UART input thread, PLIC `pending_`) use `std::atomic` or `std::mutex` explicitly — this was hardened after real data races were found and fixed during virtio-net bring-up (see `PLIC::pending_` and `SlirpState`).

---

## Prerequisites & Dependencies

| Component                                 | Purpose                                 | Notes                                                   |
| ----------------------------------------- | --------------------------------------- | ------------------------------------------------------- |
| **C++20 compiler**                        | Core build                              | GCC (MSYS2 UCRT64) or MSVC                              |
| **CMake ≥ 3.10** (presets require ≥ 3.24) | Build system                            | Uses `CMakePresets.json`                                |
| **Ninja**                                 | Generator                               | Required by all configure presets                       |
| **nlohmann/json**                         | Profile (config) parsing                | Fetched automatically via `FetchContent`                |
| **libslirp**                              | VirtIO-net user-mode networking backend | Linux: `pkg-config slirp`; required by `devices` target |
| **glib-2.0**                              | libslirp dependency                     | Required by `devices` target                            |
| **SDL2**                                  | Framebuffer + keyboard input device     | Required by `devices` target                            |
| **pthreads** (`Threads::Threads`)         | UNIX threading                          | Linked on `UNIX` builds                                 |
| **ws2_32 / iphlpapi**                     | Windows sockets/network APIs            | Linked on `WIN32` builds                                |

> On Windows, the project builds via **MSYS2 UCRT64** (VS Code preset `ucrt64-vscode`) or natively via **MSVC** (`msvc-vscode`), both driving Ninja.

---

## Installation & Building

### Clone

```bash
git clone <repo-url>
cd <repo-directory>
git checkout feat/virtio-net   # networking work lives on this branch
```

### Linux / WSL (GCC + Ninja)

```bash
# Install dependencies (Debian/Ubuntu example)
sudo apt install cmake ninja-build libslirp-dev libglib2.0-dev libsdl2-dev

cmake --preset ucrt64-ci
cmake --build --preset ci
```

### Windows — MSYS2 UCRT64 (VS Code)

```bash
cmake --preset ucrt64-vscode
cmake --build --preset vscode
```

### Windows — MSVC (Release)

```powershell
cmake --preset msvc-vscode
cmake --build --preset msvc-vscode
```

The resulting binary is `riscv_emulator` (or `riscv_emulator.exe`) inside the corresponding `build*/` directory.

---

## Usage Guide

The emulator is driven entirely by a **JSON profile file** passed as the sole positional argument:

```bash
./riscv_emulator profile.json
```

Passing zero or more-than-one argument triggers a usage error / warning respectively (`src/main.cpp`).

### Profile Schema (`Profile` / `loadProfile`)

| Key                        | Type         | Required                   | Description                                                              |
| -------------------------- | ------------ | -------------------------- | ------------------------------------------------------------------------ |
| `name`                     | string       | no                         | Human-readable label                                                     |
| `platform`                 | string       | **yes**                    | `"bare-metal"`, `"xv6"`, or `"linux"`                                    |
| `starting-pc`              | hex string   | no                         | Initial PC (default `0x80000000`)                                        |
| `extensions`               | string array | no                         | Any of `"m"`, `"c"`, `"a"`                                               |
| `verbose`                  | bool         | no                         | Enables per-instruction trace output                                     |
| `elf-path`                 | string       | one of elf/binary          | ELF image to load (parses `Elf32_Ehdr`/`Elf32_Phdr`, honors entry point) |
| `binary-path`              | string       | one of elf/binary          | Raw flat binary, loaded at `dram-start`/`kernel-address`                 |
| `disk-path`                | string       | required for `xv6`/`linux` | Backing file for virtio-blk                                              |
| `dtb-path` / `dtb-address` | string / hex | required for `linux`       | Device tree blob and its load address (also placed in `a1` at boot)      |
| `opensbi-path`             | string       | required for `linux`       | ELF firmware loaded first; entry point becomes initial PC                |
| `kernel-address`           | hex string   | required for `linux`       | Physical address the kernel binary is loaded at                          |
| `tap-name`                 | string       | optional                   | Non-empty enables `virtio-net` (libslirp) initialization                 |
| `open-sdl`                 | bool         | optional                   | Enables the SDL framebuffer + input device                               |

### Example: Booting a Linux Guest (OpenSBI → Kernel)

```json
{
  "name": "buildroot-linux",
  "platform": "linux",
  "extensions": ["m", "a", "c"],
  "opensbi-path": "firmware/fw_jump.elf",
  "binary-path": "images/Image",
  "kernel-address": "0x80400000",
  "dtb-path": "images/virt.dtb",
  "dtb-address": "0x82200000",
  "disk-path": "images/rootfs.ext2",
  "tap-name": "slirp0",
  "verbose": false
}
```

```bash
./riscv_emulator profile.json
```

Boot sequence: OpenSBI ELF is loaded and its entry point becomes the CPU's starting PC; the kernel binary and DTB are loaded at their configured physical addresses; `a1` is preset to `dtb-address` per the RISC-V Linux boot convention.

### Example: Bare-Metal ELF (No MMU, No Disk)

```json
{
  "platform": "bare-metal",
  "elf-path": "tests/hello.elf",
  "extensions": ["m"],
  "verbose": true
}
```

### Networking (virtio-net / libslirp)

Setting a non-empty `"tap-name"` initializes the virtio-net device against libslirp's built-in NAT stack (**not** a real host TAP device — the field is repurposed as an enable flag):

- Guest subnet: `10.0.2.0/24`
- Gateway/host: `10.0.2.2`
- DHCP-assigned guest address: `10.0.2.15`
- DNS: `10.0.2.3`

Inside the guest:

```sh
udhcpc -i eth0
ping 10.0.2.2
```

> **Known limitation**: libslirp's architecture in this integration supports intra-subnet UDP/ICMP but not TCP-to-gateway, DNS resolution, or external connectivity (`wget`/`curl` to real hosts will not succeed). This is an accepted architectural boundary, not an open bug.

### Deterministic UART Input (Testing)

Set the `RISCV_EMU_UART_INPUT` environment variable to pre-seed the UART RX FIFO before the input thread starts — useful for scripted/non-interactive test runs:

```bash
RISCV_EMU_UART_INPUT=$'root\n' ./riscv_emulator profile.json
```

---

## Future Roadmap

Based on current stubs and architectural notes in the codebase:

- **RV64 support** — the core is presently RV32-only (`Elf32_Ehdr`, 32-bit `pc_`/`regs_`/CSRs throughout).
- **Floating point (F/D extensions)** — not present in `Extensions` or `InstrKind`.
- **Full networking parity** — TAP/bridged networking was evaluated and explicitly deferred in favor of libslirp; external connectivity (DNS, WAN TCP, host↔guest file transfer) remains unimplemented by design.
- **Hart plurality** — `sbi_hart_start` unconditionally fails (`SBI_ERR_NOT_SUPPORTED`); the emulator is single-hart only.
- **Multi-instruction fence granularity** — `execFENCE` is currently a no-op; `SFENCE.VMA` handles TLB/ICache invalidation, but ordinary `FENCE`/`FENCE.I` semantics are not modeled.
- **PMP debug instrumentation cleanup** — remove the hardcoded diagnostic `printf` calls in `CPU::pmp_check` once PMP validation is finalized.
- **`virtio_net.hpp` MAC/feature negotiation** — currently a fixed MAC (`52:54:00:12:34:56`) and minimal feature bits (`VIRTIO_NET_F_MAC`, `VIRTIO_F_VERSION_1`); no `VIRTIO_NET_F_STATUS` link-change events beyond a hardcoded "link up".

---
