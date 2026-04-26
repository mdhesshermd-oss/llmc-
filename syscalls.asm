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
; Correctly handles 4+ arguments by managing the stack spill space.
InternalSyscall proc
    mov eax, ecx            ; Set Syscall ID
    mov r10, rdx            ; Set 1st arg (RCX)
    mov rdx, r8             ; Set 2nd arg (RDX)
    mov r8, r9              ; Set 3rd arg (R8)
    mov r9, [rsp + 40]      ; Set 4th arg (R9) from stack

    ; Note: If the syscall has 5th or 6th args, they must be at [rsp+48] and [rsp+56].
    ; However, the 'syscall' instruction only expects 4 GPR args; the rest remain on stack.

    syscall
    ret
InternalSyscall endp

end
