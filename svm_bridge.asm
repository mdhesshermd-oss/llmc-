; --- svm_bridge.asm ---
; Low-level SVM bridge for AMD virtualization transitions.
; Targets MASM (ml64.exe) for Visual Studio 2022.

.code

; External C++ handler defined in hv_vmm.cpp
extern HandleVmExit : proc

; SvmLaunch(uint64_t vmcb_pa, uint64_t hsave_pa, void* context)
; RCX = VMCB_PA, RDX = HSAVE_PA, R8 = Context VA
SvmLaunch proc
    mov r10, rcx        ; Save VMCB_PA
    mov r11, rdx        ; Save HSAVE_PA

    ; Setup Host GS Base for fast context access in Ring -1
    mov ecx, 0C0000102h ; MSR_KERNEL_GS_BASE
    mov rax, r8         ; Context VA (low 32)
    mov rdx, r8
    shr rdx, 32         ; Context VA (high 32)
    wrmsr

svm_loop:
    mov rax, r11        ; HSAVE_PA for vmsave/vmload
    vmsave rax

    ; Save extended processor state (AVX/SSE)
    sub rsp, 4096
    and rsp, -64
    xor rax, rax
    mov rcx, 0
    xgetbv
    xsave [rsp]

    ; --- ENTER GUEST MODE ---
    mov rax, r10
    vmrun rax

    ; --- VMEXIT JUMPS HERE (Host RIP in VMCB) ---
SvmVmExitHandler label qword
    vmload r11          ; Restore host state

    ; Save Guest GPRs to stack (matches GuestRegisters struct)
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

    mov rcx, r10        ; Param 1: VMCB_PA
    mov rdx, rsp        ; Param 2: GuestRegisters*

    sub rsp, 32
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

    xrstor [rsp]
    add rsp, 4096
    jmp svm_loop

svm_exit_final:
    ; Cleanup and return to caller
    add rsp, 120        ; Discard pushed registers
    xrstor [rsp]
    add rsp, 4096
    ret
SvmLaunch endp

; Export the label for use in C++ (VMCB setup)
public SvmVmExitHandler

end
