; --- syscalls.asm ---
; External assembly procedures for direct system calls and hypercalls.
; Targets MASM (ml64.exe).

.code

; --- _hv_call ---
; Prototype: extern "C" uint64_t _hv_call(uint64_t key, Command cmd, void* args);
; RCX = key, RDX = cmd, R8 = args
_hv_call proc
    vmmcall
    ret
_hv_call endp

; --- InternalSyscall ---
; Prototype: extern "C" NTSTATUS InternalSyscall(uint32_t ssdt_id, ...);
; Correctly handles up to 6 arguments by managing the stack spill space.
InternalSyscall proc
    mov eax, ecx            ; Set Syscall ID
    mov r10, rdx            ; Set 1st arg (RCX)
    mov rdx, r8             ; Set 2nd arg (RDX)
    mov r8, r9              ; Set 3rd arg (R8)
    mov r9, [rsp + 40]      ; Set 4th arg (R9) from stack

    ; Handle 5th and 6th arguments for 64-bit kernel
    ; We must copy them from our caller's stack to the current stack
    ; for the 'syscall' instruction to find them if it looks at the stack.
    mov rax, [rsp + 48]
    mov [rsp + 40], rax     ; arg5
    mov rax, [rsp + 56]
    mov [rsp + 48], rax     ; arg6

    syscall
    ret
InternalSyscall endp

end
