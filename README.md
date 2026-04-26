# DayZ Stealth Project (Refactored from Memory Dump)

This repository contains a full-stack refactor and implementation of a DayZ stealth cheat, originally extracted from a memory dump of a Tauri-based Rust application.

## 🚀 Key Features

- **Ring -1 Hypervisor (AMD SVM)**: Professional-grade virtualization for stealth and bypassing Ring 0/Ring 3 hooks.
- **Driverless Operation**: Uses a kernel bridge only for initialization, then transitions all logic to the hypervisor.
- **NPT Cloaking (Shadow Pages)**: Swaps physical frames to hide cheat memory from anti-cheat scanners.
- **Thread Hijacking Injection**: Injects logic by redirecting existing game threads instead of creating new ones.
- **AES-128 CBC Security**: Encrypted logic payload with just-in-time in-memory decryption.
- **Survivor-Only ESP**: Refactored logic specifically filtering for Human Players with optimized Enfusion Engine math.
- **Overlay Hijacking**: Renders ESP via trusted overlays (AMD Radeon, NVIDIA, Discord) to evade visual detection.

## 📁 Project Structure

- `refactored_logic.cpp`: Core DayZ ESP logic (Entity iteration, WorldToScreen).
- `hv_vmm.cpp`: The VM-Exit handler and hypercall dispatcher.
- `manual_map.h`: Stealthy PE mapper with thread hijacking.
- `driver.c`: Kernel bridge for multi-core hypervisor initialization.
- `hypervisor_io.h`: Unified Ring 3 to Ring -1 communication interface.
- `stealth_cleanup.h`: Kernel trace removal and PE header wiping.

## 🛠️ Build & Usage

1. **Compilation**: Use Visual Studio 2022 with MASM (ml64.exe). See `BUILD.md` for project settings.
2. **Payload**: Encrypt your DLL using `encrypt_and_pack.py` and embed it as a resource (IDR_CHEAT_DLL).
3. **Execution**: Load the driver using a manual mapper (KDMapper) then run `refactored_loader.exe`.

## 📜 Documentation

- `ANALYSIS.md`: Detailed breakdown of the "Combat-Ready" architecture.
- `BUILD.md`: Step-by-step build instructions.
- `OFFSETS.md`: Reference for maintaining engine offsets.
- `DEBUGGING.md`: Guidelines for safe testing.
