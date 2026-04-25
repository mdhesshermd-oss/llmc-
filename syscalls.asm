; syscalls.asm
; MASM implementation for direct syscalls in x64
.code

InternalSyscall proc
    mov eax, ecx          ; eax = SSN (System Service Number)
    mov r10, rdx          ; r10 = 1st argument for syscall (originally 2nd for InternalSyscall)
    mov rdx, r8           ; rdx = 2nd argument
    mov r8, r9            ; r8 = 3rd argument
    mov r9, [rsp + 40]    ; r9 = 4th argument

    ; Shift remaining stack arguments if any (5th and 6th)
    mov rax, [rsp + 48]
    mov [rsp + 40], rax
    mov rax, [rsp + 56]
    mov [rsp + 48], rax

    syscall               ; Invoke the kernel
    ret
InternalSyscall endp

end
