extern malloc
extern free
extern strlen
extern fwrite
extern stderr
extern str_concat 

global string_proc_list_has_cycle_asm
global string_proc_list_create_asm
global string_proc_node_create_asm
global string_proc_list_add_node_asm
global string_proc_list_concat_asm

section .rodata  ; Or section .data
.LC0: db "Error: Cycle detected in list during concat. Aborting.\n", 0
.LC1: db "", 0
.LC2: db "Error: Failed to allocate initial result hash.\n", 0
.LC3: db "Error: Concatenation would exceed MAX_RESULT_LEN. Aborting.\n", 0
.LC4: db "Error: str_concat failed during concatenation (malloc failed?). Aborting.\n", 0

section .text    ; Switch back to the code section before functions

string_proc_list_has_cycle_asm:
        push    rbp
        mov     rbp, rsp
        mov     qword [rbp-24], rdi
        cmp     qword [rbp-24], 0
        je      .L2
        mov     rax, qword [rbp-24]
        mov     rax, qword [rax]
        test    rax, rax
        jne     .L3
.L2:
        mov     eax, 0
        jmp     .L4
.L3:
        mov     rax, qword [rbp-24]
        mov     rax, qword [rax]
        mov     qword [rbp-8], rax
        mov     rax, qword [rbp-24]
        mov     rax, qword [rax]
        mov     qword [rbp-16], rax
        jmp     .L5
.L7:
        mov     rax, qword [rbp-8]
        mov     rax, qword [rax]
        mov     qword [rbp-8], rax
        mov     rax, qword [rbp-16]
        mov     rax, qword [rax]
        mov     rax, qword [rax]
        mov     qword [rbp-16], rax
        mov     rax, qword [rbp-8]
        cmp     rax, qword [rbp-16]
        jne     .L5
        mov     eax, 1
        jmp     .L4
.L5:
        cmp     qword [rbp-16], 0
        je      .L6
        mov     rax, qword [rbp-16]
        mov     rax, qword [rax]
        test    rax, rax
        jne     .L7
.L6:
        mov     eax, 0
.L4:
        pop     rbp
        ret

string_proc_list_create_asm:
        push    rbp
        mov     rbp, rsp
        sub     rsp, 16
        mov     edi, 16
        call    malloc
        mov     qword [rbp-8], rax
        cmp     qword [rbp-8], 0
        jne     .L9
        mov     eax, 0
        jmp     .L10
.L9:
        mov     rax, qword [rbp-8]
        mov     qword [rax], 0
        mov     rax, qword [rbp-8]
        mov     qword [rax+8], 0
        mov     rax, qword [rbp-8]
.L10:
        leave
        ret
string_proc_node_create_asm:
        push    rbp
        mov     rbp, rsp
        sub     rsp, 32
        mov     eax, edi
        mov     qword [rbp-32], rsi
        mov     BYTE [rbp-20], al
        mov     edi, 32
        call    malloc
        mov     qword [rbp-8], rax
        cmp     qword [rbp-8], 0
        jne     .L12
        mov     eax, 0
        jmp     .L13
.L12:
        mov     rax, qword [rbp-8]
        mov     qword [rax], 0
        mov     rax, qword [rbp-8]
        mov     qword [rax+8], 0
        mov     rax, qword [rbp-8]
        movzx   edx, BYTE [rbp-20]
        mov     BYTE [rax+16], dl
        mov     rax, qword [rbp-8]
        mov     rdx, qword [rbp-32]
        mov     qword [rax+24], rdx
        mov     rax, qword [rbp-8]
.L13:
        leave
        ret
string_proc_list_add_node_asm:
        push    rbp
        mov     rbp, rsp
        sub     rsp, 48
        mov     qword [rbp-24], rdi
        mov     eax, esi
        mov     qword [rbp-40], rdx
        mov     BYTE [rbp-28], al
        cmp     qword [rbp-24], 0
        je      .L19
        movzx   eax, BYTE [rbp-28]
        mov     rdx, qword [rbp-40]
        mov     rsi, rdx
        mov     edi, eax
        call    string_proc_node_create_asm
        mov     qword [rbp-8], rax
        cmp     qword [rbp-8], 0
        je      .L20
        mov     rax, qword [rbp-24]
        mov     rax, qword [rax+8]
        test    rax, rax
        jne     .L18
        mov     rax, qword [rbp-24]
        mov     rdx, qword [rbp-8]
        mov     qword [rax], rdx
        mov     rax, qword [rbp-24]
        mov     rdx, qword [rbp-8]
        mov     qword [rax+8], rdx
        jmp     .L14
.L18:
        mov     rax, qword [rbp-24]
        mov     rax, qword [rax+8]
        mov     rdx, qword [rbp-8]
        mov     qword [rax], rdx
        mov     rax, qword [rbp-24]
        mov     rdx, qword [rax+8]
        mov     rax, qword [rbp-8]
        mov     qword [rax+8], rdx
        mov     rax, qword [rbp-24]
        mov     rdx, qword [rbp-8]
        mov     qword [rax+8], rdx
        jmp     .L14
.L19:
        nop
        jmp     .L14
.L20:
        nop
.L14:
        leave
        ret

string_proc_list_concat_asm:
        push    rbp
        mov     rbp, rsp
        sub     rsp, 80
        mov     qword [rbp-56], rdi
        mov     eax, esi
        mov     qword [rbp-72], rdx
        mov     BYTE [rbp-60], al
        cmp     qword [rbp-56], 0
        je      .L22
        cmp     qword [rbp-72], 0
        jne     .L23
.L22:
        mov     eax, 0
        jmp     .L24
.L23:
        mov     rax, qword [rbp-56]
        mov     rdi, rax
        call    string_proc_list_has_cycle_asm
        test    al, al
        je      .L25
        mov rax, [rel stderr]
        mov     rcx, rax
        mov     edx, 55
        mov     esi, 1
		mov 	edi, .LC0        ; Load address of .LC0 into RDI (first arg)
        call    fwrite
        mov     eax, 0
        jmp     .L24
.L25:
        mov     rax, qword [rbp-72]
        mov     rsi, rax
		mov 	edi, .LC1        ; Load address of .LC1 into RDI
        mov     eax, 0
        call    str_concat
        cdqe
        mov     qword [rbp-8], rax
        cmp     qword [rbp-8], 0
        jne     .L26
        mov rax, [rel stderr]
        mov     rcx, rax
        mov     edx, 47
        mov     esi, 1
		mov 	edi, .LC2        ; Load address of .LC2 into RDI
        call    fwrite
        mov     eax, 0
        jmp     .L24
.L26:
        mov     rax, qword [rbp-8]
        mov     rdi, rax
        call    strlen
        mov     qword [rbp-16], rax
        mov     rax, qword [rbp-56]
        mov     rax, qword [rax]
        mov     qword [rbp-24], rax
        jmp     .L27
.L31:
        mov     rax, qword [rbp-24]
        movzx   eax, BYTE [rax+16]
        cmp     BYTE [rbp-60], al
        jne     .L28
        mov     rax, qword [rbp-24]
        mov     rax, qword [rax+24]
        test    rax, rax
        je      .L28
        mov     rax, qword [rbp-24]
        mov     rax, qword [rax+24]
        mov     rdi, rax
        call    strlen
        mov     qword [rbp-32], rax
        mov     eax, 1048576
        sub     rax, qword [rbp-16]
        cmp     qword [rbp-32], rax
        jb      .L29
        mov rax, [rel stderr]
        mov     rcx, rax
        mov     edx, 60
        mov     esi, 1
		mov 	edi, .LC3        ; Load address of .LC3 into RDI
        call    fwrite
        mov     rax, qword [rbp-8]
        mov     rdi, rax
        call    free
        mov     eax, 0
        jmp     .L24
.L29:
        mov     rax, qword [rbp-24]
        mov     rdx, qword [rax+24]
        mov     rax, qword [rbp-8]
        mov     rsi, rdx
        mov     rdi, rax
        mov     eax, 0
        call    str_concat
        cdqe
        mov     qword [rbp-40], rax
        cmp     qword [rbp-40], 0
        jne     .L30
        mov rax, [rel stderr]
        mov     rcx, rax
        mov     edx, 74
        mov     esi, 1
		mov 	edi, .LC4        ; Load address of .LC4 into RDI
        call    fwrite
        mov     rax, qword [rbp-8]
        mov     rdi, rax
        call    free
        mov     eax, 0
        jmp     .L24
.L30:
        mov     rax, qword [rbp-8]
        mov     rdi, rax
        call    free
        mov     rax, qword [rbp-40]
        mov     qword [rbp-8], rax
        mov     rax, qword [rbp-32]
        add     qword [rbp-16], rax
.L28:
        mov     rax, qword [rbp-24]
        mov     rax, qword [rax]
        mov     qword [rbp-24], rax
.L27:
        cmp     qword [rbp-24], 0
        jne     .L31
        mov     rax, qword [rbp-8]
.L24:
        leave
        ret