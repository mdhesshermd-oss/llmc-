; syscalls.asm (Updated to include VMCALL)
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

; InternalVMCALL(key, code, arg1, arg2)
InternalVMCALL proc
    ; ecx = key, edx = code, r8 = arg1, r9 = arg2
    vmcall
    ret
InternalVMCALL endp

end
