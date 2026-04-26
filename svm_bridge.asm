; --- svm_bridge.asm ---
; Low-level SVM bridge for AMD virtualization transitions.
; Targets MASM (ml64.exe) for Visual Studio 2022.

.code

; External C++ handler defined in hv_vmm.cpp
extern HandleVmExit : proc

; SvmLaunch(uint64_t vmcb_pa, uint64_t hsave_pa, void* context)
; RCX = VMCB_PA, RDX = HSAVE_PA, R8 = Context VA
SvmLaunch proc
    ; Save non-volatile registers
    push rbx
    push rbp
    push rdi
    push rsi
    push r12
    push r13
    push r14
    push r15

    ; Use non-volatile registers for persistent data across Guest execution
    mov r12, r8         ; r12 = Context (PerCoreData*)
    mov r13, rcx        ; r13 = VMCB_PA
    mov r14, rdx        ; r14 = HSAVE_PA

    ; Setup Host GS Base for fast context access in Ring -1
    mov ecx, 0C0000102h ; MSR_KERNEL_GS_BASE
    mov rax, r12
    mov rdx, r12
    shr rdx, 32
    wrmsr

svm_loop:
    mov rax, r14
    vmsave rax          ; Save Host State

    ; Save extended processor state (AVX/SSE) with proper alignment
    mov rbp, rsp        ; Save stack pointer for alignment
    sub rsp, 4096       ; Buffer for xsave
    and rsp, -64        ; 64-byte alignment requirement
    xor rax, rax
    mov rcx, 0
    xgetbv
    xsave [rsp]

    ; --- ENTER GUEST MODE ---
    mov rax, r13
    vmrun rax

    ; --- VMEXIT JUMPS HERE (Host RIP in VMCB) ---
SvmVmExitHandler label qword
    mov rax, r14
    vmload rax          ; Restore Host State

    ; Save Guest GPRs to stack (15 registers = 120 bytes)
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

    mov rcx, r13        ; Param 1: VMCB_PA
    mov rdx, rsp        ; Param 2: GuestRegisters*

    sub rsp, 32         ; Shadow space
    call HandleVmExit
    add rsp, 32

    ; Check for Unload Status (0xC0000600)
    cmp eax, 0C0000600h
    je svm_exit_final

    ; Restore Guest GPRs
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

    xrstor [rsp]        ; Restore AVX/SSE state
    mov rsp, rbp        ; Restore original stack pointer
    jmp svm_loop

svm_exit_final:
    ; Cleanup and return to caller
    add rsp, 120        ; Discard Guest GPRs (15 * 8)
    xrstor [rsp]
    mov rsp, rbp

    pop r15
    pop r14
    pop r13
    pop r12
    pop rsi
    pop rdi
    pop rbp
    pop rbx
    ret
SvmLaunch endp

; Export the label for use in C++ (VMCB setup)
public SvmVmExitHandler

end
