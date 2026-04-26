; --- svm_launch.asm ---
; Low-level AMD SVM execution, exit handling, and Ring -1 exception safety.
; Targets MASM (ml64.exe) for Visual Studio 2022.

.code

; External C++ handlers defined in hv_vmm.cpp
extern HandleVmExit : proc
extern HvPanicHandler : proc

; --- SvmLaunch ---
; Prototype: extern "C" void SvmLaunch(void* vmcb_pa);
; RCX = Physical address of the VMCB
SvmLaunch proc
    push rbx
    push rbp
    push rdi
    push rsi
    push r12
    push r13
    push r14
    push r15

    mov rax, rcx
    vmrun rax

    ; --- VMEXIT OCCURRED ---
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

; --- SvmVmExitHandler ---
; Entry point from VMCB Host RIP.
SvmVmExitHandler proc
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

    mov rcx, rax        ; VMCB physical address (passed in RAX by CPU/Launch)
    mov rdx, rsp        ; GuestRegisters*

    sub rsp, 32
    call HandleVmExit
    add rsp, 32

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

    vmrun rax
    jmp SvmVmExitHandler
SvmVmExitHandler endp

; --- hv_exception_stub ---
; Catch-all for hypervisor host-mode exceptions.
hv_exception_stub proc
    cli
    pushfq
    push rax
    push rcx
    push rdx

    sub rsp, 32
    call HvPanicHandler
    add rsp, 32

    pop rdx
    pop rcx
    pop rax
    popfq
    sti
    iretq
hv_exception_stub endp

end
