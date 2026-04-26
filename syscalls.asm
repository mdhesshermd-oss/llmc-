; --- syscalls.asm ---
; External assembly procedures for direct system calls and hypercalls.
; Targets MASM (ml64.exe).

.code

; --- _hv_call ---
; Prototype: extern "C" uint64_t _hv_call(uint64_t key, Command cmd, void* args);
; RCX = key, RDX = cmd, R8 = args
_hv_call proc
    ; Trigger AMD SVM VMMCALL (3 bytes: 0F 01 D9)
    ; For Intel VT-x, this would be VMCALL (0F 01 C1)
    ; The hypervisor will catch this and process based on registers.
    vmmcall
    ret
_hv_call endp

; --- _sys_allocate_virtual_memory ---
; Prototype: extern "C" NTSTATUS _sys_allocate_virtual_memory(HANDLE ProcessHandle, PVOID* BaseAddress, ULONG_PTR ZeroBits, PSIZE_T RegionSize, ULONG AllocationType, ULONG Protect);
_sys_allocate_virtual_memory proc
    mov r10, rcx
    mov eax, 18h ; NtAllocateVirtualMemory syscall ID (Win10/11)
    syscall
    ret
_sys_allocate_virtual_memory endp

end
