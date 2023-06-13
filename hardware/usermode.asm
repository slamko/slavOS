section .data
    syscall_msg db "In kernel", 10, 0
    legacy_msg db "Using legacy int", 10, 0

section .bss
    _sysenter_avl resb 1
    
section .text

extern fb_newline
extern fb_print_num
usermode_bootstrap:
    mov eax, cs
    and eax, 0x3
    jz loop
    call usermode
    
loop:
    jmp loop

SYSENTER_CS  equ 0x174
SYSENTER_ESP equ 0x175
SYSENTER_EIP equ 0x176
EFLAGS_ID    equ 0x200000

get_eip:
    mov eax, [esp]
    ret

extern syscall_handler
scall_wrapper:
    ;; push 4
    push ecx
    push edx

    mov edi, 5
_push_args:
    dec edi
    push dword [ecx + edi*4 + 8]

    cmp edi, 0
    jne _push_args

    call [ecx + 4] 

    add esp, 5 * 4

    pop edx
    pop ecx
    sysexit

global syscall_setup
syscall_setup:
    mov byte [_sysenter_avl], 0

    pushfd
    or dword [esp], EFLAGS_ID
    popfd

    pushfd
    mov eax, [esp]
    shr eax, 21
    and eax, 0x1
    jz _legacy_setup
    popfd

    mov eax, 1
    cpuid
    shr edx, 11
    and edx, 1
    jz _legacy_setup

_sysenter_setup:
    mov byte [_sysenter_avl], 1
    mov ecx, SYSENTER_CS
    mov eax, 0x8
    wrmsr

    mov ecx, SYSENTER_ESP
    mov eax, kernel_int_stack_end
    wrmsr

    mov ecx, SYSENTER_EIP
    mov eax, scall_wrapper
    wrmsr   

    ret

_legacy_setup:
    ret

global jump_usermode
extern usermode
extern fb_print_black    
extern fb_print_num    

jump_usermode:
    cli
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov eax, esp
    push dword 0x23
    push eax
    pushfd
    push 0x1B
    sti
    push usermode_bootstrap
    iret

extern kernel_int_stack_end
global sysenter
sysenter:
    test byte [_sysenter_avl], 0
    jnz _legacy

    mov ecx, esp
    mov edx, _after
    sysenter

    push legacy_msg
    call fb_print_black
    
    jmp _after

_legacy:
    int 0x80
    
_after:
    ret

global ltr
ltr:
    mov ax, 0x28 | 3
    ltr ax
    ret
