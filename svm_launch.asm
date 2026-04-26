; --- svm_launch.asm ---
; Low-level AMD SVM (AMD-V) execution and exit handling for Ring -1.
; Targets MASM (ml64.exe) for Visual Studio 2022.

.code

; External C++ handler defined in hv_vmm.cpp
extern HandleVmExit : proc

; --- SvmLaunch ---
; Prototype: extern "C" void SvmLaunch(void* vmcb_pa);
; RCX = Physical address of the VMCB
SvmLaunch proc
    ; Save host state that isn't saved by VMCB
    push rbx
    push rbp
    push rdi
    push rsi
    push r12
    push r13
    push r14
    push r15

    ; The VMCB physical address is in RCX.
    ; AMD requires the VMCB address in RAX for VMRUN.
    mov rax, rcx

    ; Enter Guest Mode
    vmrun rax

    ; --- VMEXIT OCCURRED HERE ---
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
; This is the entry point from the VMCB's Host State RIP.
SvmVmExitHandler proc
    ; 1. Save all Guest registers
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

    ; 2. Prepare arguments for the C++ handler:
    mov rcx, rsp        ; GuestRegisters*

    sub rsp, 32
    call HandleVmExit
    add rsp, 32

    ; 4. Restore Guest registers
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

    ; 5. Re-run the guest (RAX should contain VMCB PA)
    vmrun rax
    jmp SvmVmExitHandler
SvmVmExitHandler endp

end
