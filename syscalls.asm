; syscalls.asm (Updated for Intel/AMD Multi-Vendor Support)
.code

InternalSyscall proc
    mov eax, ecx
    mov r10, rdx
    mov rdx, r8
    mov r8, r9
    mov r9, [rsp + 40]
    mov rax, [rsp + 48]
    mov [rsp + 40], rax
    mov rax, [rsp + 56]
    mov [rsp + 48], rax
    syscall
    ret
InternalSyscall endp

; InternalVMCALL(key, code, arg1, arg2) - INTEL
InternalVMCALL proc
    vmcall
    ret
InternalVMCALL endp

; InternalVMMCALL(key, code, arg1, arg2) - AMD
InternalVMMCALL proc
    vmmcall
    ret
InternalVMMCALL endp

end
