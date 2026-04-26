# DayZ Stealth Cheat: "Combat-Ready" Architecture Analysis

This document details the transition from the initial refactored "skeleton" to the production-grade, driverless hypervisor architecture.

## 1. Evolution: From Driver (Ring 0) to Hypervisor (Ring -1)
The initial implementation used a kernel driver (`driver.c`) for memory access. While stealthier than user-mode APIs, drivers leave traces (e.g., `MmUnloadedDrivers`, Pool Tags) that modern anti-cheats (BattlEye) scan for.

The "Combat-Ready" version is **driverless**. All memory operations are performed directly by the hypervisor at Ring -1. Communication is handled via `VMMCALL` (AMD) or `VMCALL` (Intel) using a 64-bit secret key (`0x5A4F524F5F444159`) to prevent accidental discovery by the OS or anti-cheat.

## 2. Advanced Stealth: NPT Cloaking (Shadow Pages)
We have implemented **Nested Page Table (NPT) Cloaking** in `hv_vmm.cpp`. This is the most advanced form of memory hiding available:
- **Redirection**: The hypervisor maintains two physical copies of the cheat's memory: an **Original Page** (clean) and a **Shadow Page** (infected).
- **TLB Splitting**:
  - When the anti-cheat reads the memory (Data Access), the hypervisor directs it to the **Original Page**.
  - When the processor executes the code (Instruction Fetch), the hypervisor swaps the mapping to the **Shadow Page**.
- **The Result**: Scanners see clean game code, but the cheat logic actually executes.

## 3. Deep Kernel Cleanup
The `stealth_cleanup.h` module performs aggressive trace removal:
- **MmUnloadedDrivers Wiping**: Automatically locates the hidden kernel list by scanning memory starting from `MSR_LSTAR` and zeroes it out.
- **PE Header Erasing**: Destroys the "MZ/PE" signature of the mapped cheat logic in the game's memory.
- **Pool Tag Replacement**: Identifies and replaces suspicious memory tags left by the loader.

## 4. Safety & Robustness
Hypervisor errors usually result in a "Triple Fault" and an instant system reboot. Our architecture prevents this:
- **Host IDT**: The hypervisor now has its own Interrupt Descriptor Table to catch internal exceptions.
- **Panic Handler**: If a critical error occurs in Ring -1, the `HvPanicHandler` safely disables virtualization and returns control to the guest OS instead of crashing.

## 5. Payload Security
The cheat payload is now encrypted with **AES-128 CBC**.
- It is only decrypted in-memory by the loader just before injection.
- The AES key is immediately wiped (`SecureZeroMemory`) after use to prevent it from being found in memory dumps.

## 6. Logic Parity
Despite these architectural upgrades, the core DayZ logic remains faithful to the original dump:
- **Entity Iteration**: Efficiently loops through the `GameWorld` entity list.
- **Player Filtering**: Specifically targets `DayZPlayer` entities at offset `0x158`.
- **WorldToScreen**: High-precision coordinate transformation for GDI-based rendering via hijacked overlays.
