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
; Supports up to 6 arguments with proper stack management.
InternalSyscall proc
    mov eax, ecx            ; Syscall ID
    mov r10, rdx            ; First arg (RCX for syscall)

    ; Setup args 2, 3, 4
    mov rdx, r8             ; RDX
    mov r8, r9              ; R8
    mov r9, [rsp + 40]      ; R9 (was pushed by caller as 5th arg)

    ; Shadow space is handled by the caller.
    ; If more than 4 args, they are already on stack above shadow space.

    syscall
    ret
InternalSyscall endp

end
