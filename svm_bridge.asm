; --- svm_bridge.asm ---
; Gbhv-style low-level SVM transition logic (Final Combat-Ready).
; Targets MASM (ml64.exe) for Visual Studio 2022.

.code

extern GbhvHandleVmExit : proc

; GbhvSvmLaunch(uint64_t VmcbPa, uint64_t HsavePa, void* Context)
GbhvSvmLaunch proc
    ; Capture Guest State for continuation
    mov rax, [rsp]          ; Return RIP
    mov [r8], rax           ; Context->GuestRip
    mov [r8 + 8], rsp       ; Context->GuestRsp
    pushfq
    pop rax
    mov [r8 + 16], rax      ; Context->GuestRflags

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
    mov r12, r8         ; GbhvContext
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

    ; --- Extended State Preservation (AVX/SSE) ---
    mov rbp, rsp        ; Save stack pointer for alignment
    sub rsp, 4096       ; Buffer for xsave
    and rsp, -64        ; 64-byte alignment requirement
    xor rax, rax
    mov rcx, 0
    xgetbv
    xsave [rsp]

    mov rax, r13
    vmrun rax           ; --- RUN GUEST ---

    ; --- VMEXIT OCCURRED ---
    mov rax, r14
    vmload rax          ; Restore host state from HSAVE

SvmVmExitHandler label qword
    ; 4. Save Guest GPRs (15 registers = 120 bytes)
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

    ; 5. Call C++ handler: GbhvHandleVmExit(VmcbPa, Registers)
    mov rcx, r13
    mov rdx, rsp
    sub rsp, 32
    call GbhvHandleVmExit
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

    xrstor [rsp]        ; Restore AVX/SSE
    mov rsp, rbp        ; Restore stack pointer

    jmp svm_loop
GbhvSvmLaunch endp

public SvmVmExitHandler

end
