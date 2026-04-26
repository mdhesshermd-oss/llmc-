; --- svm_bridge.asm ---
; Gbhv-style low-level SVM transition logic.
; Targets MASM (ml64.exe) for Visual Studio 2022.

.code

extern HandleVmExit : proc

; GbhvSvmLaunch(uint64_t VmcbPa, uint64_t HsavePa, void* Context)
GbhvSvmLaunch proc
    ; 1. Save host context
    push rbx
    push rbp
    push rdi
    push rsi
    push r12
    push r13
    push r14
    push r15

    ; Persistent storage in non-volatile registers
    mov r12, r8         ; GbhvContext (PVMM_PROCESSOR_CONTEXT)
    mov r13, rcx        ; VmcbPhysical
    mov r14, rdx        ; HostSavePhysical

    ; 2. Setup Host GS Base for fast access to context
    mov ecx, 0C0000102h ; MSR_KERNEL_GS_BASE
    mov rax, r12
    mov rdx, r12
    shr rdx, 32
    wrmsr

svm_loop:
    ; 3. Transition to Guest Mode
    mov rax, r14
    vmsave rax          ; Save current host state to HSAVE

    mov rax, r13
    vmrun rax           ; --- RUN GUEST ---

    ; --- VMEXIT OCCURRED ---
    mov rax, r14
    vmload rax          ; Restore host state from HSAVE

SvmVmExitHandler label qword
    ; 4. Save Guest state (15 registers = 120 bytes)
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rdi
    push rsi
    push rbp
    push rbx
    push rdx
    push rcx
    push rax

    ; 5. Call C++ handler: HandleVmExit(VmcbPa, Registers)
    mov rcx, r13
    mov rdx, rsp
    sub rsp, 32
    call HandleVmExit
    add rsp, 32

    ; 6. Return to Guest
    pop rax
    pop rcx
    pop rdx
    pop rbx
    pop rbp
    pop rsi
    pop rdi
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    jmp svm_loop
GbhvSvmLaunch endp

public SvmVmExitHandler

end
