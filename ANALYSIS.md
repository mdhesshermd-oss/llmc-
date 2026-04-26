# DayZ Stealth Cheat: "Combat-Ready" Architecture Analysis

This document details the transition to the finalized, production-grade driverless hypervisor architecture.

## 1. Multi-Core Virtualization (Ring 0 -> Ring -1)
The cheat now utilizes a kernel-mode bridge (`driver.c`) to initialize the hypervisor on all CPU cores simultaneously via `KeGenericCallDpc`.
- **The Process**: The loader sends the `IO_LAUNCH_HV` command. The driver then executes the SVM/VT-x bootstrap on every core, virtualizing the entire system.
- **Independence**: Once initialized, the hypervisor operates independently of the driver, allowing the `.sys` file to be unloaded to reduce the detection surface.

## 2. Advanced Injection: Thread Hijacking
To avoid the heavily monitored `CreateRemoteThread` API, the injector now uses **Thread Hijacking**:
- **Suspension**: An existing game thread (e.g., the rendering thread) is suspended.
- **Redirection**: The thread's `RIP` (Instruction Pointer) is redirected to a stealthy shellcode area mapped by the hypervisor.
- **Execution**: The shellcode calls `LoadLibrary` (to resolve imports) and the cheat's entry point, then jumps back to the original `RIP` to resume normal game execution.

## 3. Memory Cloaking & Identity Mapping
The hypervisor implements **1:1 Identity Mapping** for guest physical memory using **2MB Huge Pages** in the Nested Page Tables (NPT).
- This ensures that the hypervisor can access any part of the game's RAM without expensive or detectable address translations.
- **Cloaking**: NPT Shadowing is used to hide the cheat's code. Scanners see original game bytes, while the CPU executes the modified logic.

## 4. Deep Kernel Sanitization
All traces of the loader and driver are wiped from the Ring -1 context:
- **MmUnloadedDrivers**: Cleared via hypercall to prevent BattlEye from finding traces of the manual-mapped driver.
- **Pool Tags**: Suspicious memory allocations are renamed or hidden.
- **PE Headers**: The "MZ" signatures of the injected logic are erased immediately after injection.

## 5. Security & Parity
- **AES-128 CBC**: The logic payload is encrypted with a secret key matching the hypercall authorization key.
- **ESP Logic**: The core refactored DayZ logic (`refactored_logic.cpp`) remains 100% faithful to the original dump, providing "Survivor-only" ESP via optimized Enfusion Engine math.
