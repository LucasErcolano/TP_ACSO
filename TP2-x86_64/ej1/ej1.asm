; /** defines bool y puntero **/
%define NULL 0
%define TRUE 1
%define FALSE 0

; Constantes para los offsets de las estructuras (asumiendo padding estándar x86-64)
; string_proc_list (size 16)
%define LIST_FIRST 0
%define LIST_LAST  8
; string_proc_node (size 32)
%define NODE_NEXT     0
%define NODE_PREVIOUS 8
%define NODE_TYPE     16 ; uint8_t
; padding de 7 bytes aquí
%define NODE_HASH     24 ; char*

section .data
empty_string: db "", 0 ; String vacío para usar en str_concat si es necesario

section .text

; Funciones externas necesarias
extern malloc
extern free
extern str_concat
extern strlen       ; Necesaria para duplicar string inicial en concat
extern strcpy       ; Necesaria para duplicar string inicial en concat

; --- Funciones a Implementar ---

global string_proc_list_create_asm
global string_proc_node_create_asm
global string_proc_list_add_node_asm
global string_proc_list_concat_asm

; string_proc_list* string_proc_list_create_asm(void);
; Retorna: Puntero a la lista en RAX, o NULL si falla malloc
string_proc_list_create_asm:
    push rbp        ; Preservar RBP (aunque no lo usemos explícitamente, es buena práctica)
    mov rbp, rsp

    ; Llamar a malloc(sizeof(string_proc_list)) -> sizeof = 16
    mov edi, 16
    call malloc

    ; Verificar si malloc falló (rax == NULL)
    cmp rax, NULL
    je .fail        ; Si es NULL, saltar a fail

    ; Inicializar la lista: list->first = NULL, list->last = NULL
    mov qword [rax + LIST_FIRST], NULL
    mov qword [rax + LIST_LAST], NULL

.success:
    ; El resultado ya está en RAX
    jmp .end

.fail:
    mov rax, NULL   ; Asegurar que RAX es NULL en caso de fallo

.end:
    mov rsp, rbp    ; Restaurar RSP
    pop rbp         ; Restaurar RBP
    ret

; string_proc_node* string_proc_node_create_asm(uint8_t type, char* hash);
; Parametros: RDI = type (el byte bajo DIL), RSI = hash (puntero)
; Retorna: Puntero al nodo en RAX, o NULL si falla malloc
string_proc_node_create_asm:
    push rbp
    mov rbp, rsp
    ; Guardar parámetros si se van a sobreescribir (RDI/RSI son caller-saved)
    push rdi        ; Guardar type (aunque solo necesitemos DIL)
    push rsi        ; Guardar hash

    ; Llamar a malloc(sizeof(string_proc_node)) -> sizeof = 32
    mov edi, 32
    call malloc

    ; Verificar si malloc falló
    cmp rax, NULL
    je .fail        ; Saltar a fail si es NULL

    ; malloc retornó el puntero en rax (node_ptr)
    mov rbx, rax    ; Guardar node_ptr en RBX (callee-saved) para usar RAX libremente

    ; Recuperar parámetros de la pila
    pop rsi         ; hash en RSI
    pop rdi         ; type en RDI (DIL)

    ; Inicializar el nodo:
    mov qword [rbx + NODE_NEXT], NULL
    mov qword [rbx + NODE_PREVIOUS], NULL
    mov byte [rbx + NODE_TYPE], dil      ; Guardar el byte de tipo
    mov qword [rbx + NODE_HASH], rsi     ; Guardar el puntero hash

    ; El puntero al nodo está en RBX, moverlo a RAX para retorno
    mov rax, rbx

.success:
    jmp .end

.fail:
    ; Recuperar parámetros de la pila para limpiar
    pop rsi
    pop rdi
    mov rax, NULL   ; Asegurar RAX = NULL

.end:
    mov rsp, rbp
    pop rbp
    ret

; void string_proc_list_add_node_asm(string_proc_list* list, uint8_t type, char* hash);
; Parametros: RDI = list, RSI = type (SIL), RDX = hash
; Retorna: void
string_proc_list_add_node_asm:
    push rbp
    mov rbp, rsp
    push rbx        ; Preservar registros callee-saved que usaremos
    push r12
    push r13

    ; Guardar parámetros originales
    mov r12, rdi    ; list en R12
    mov r13b, sil   ; type en R13B (byte bajo de R13)
    mov rbx, rdx    ; hash en RBX

    ; Llamar a string_proc_node_create_asm(type, hash)
    ; Preparar argumentos:
    movzx edi, r13b ; type (uint8_t -> zero extended to 64 bits for RDI)
    mov rsi, rbx    ; hash
    call string_proc_node_create_asm

    ; Verificar si la creación del nodo falló (rax == NULL)
    cmp rax, NULL
    je .end         ; Si falló, simplemente terminar (void function)

    ; Nodo creado exitosamente, puntero en RAX (new_node_ptr)
    mov rbx, rax    ; Guardar new_node_ptr en RBX

    ; Recuperar el puntero a la lista (R12)
    mov rdi, r12    ; list en RDI para trabajar con él

    ; Obtener el último nodo actual: last_node_ptr = list->last
    mov rdx, [rdi + LIST_LAST]

    ; Verificar si la lista está vacía (last_node_ptr == NULL)
    cmp rdx, NULL
    jne .list_not_empty ; Si no es NULL, la lista no está vacía

.list_empty:
    ; La lista está vacía: list->first = new_node, list->last = new_node
    mov [rdi + LIST_FIRST], rbx
    mov [rdi + LIST_LAST], rbx
    jmp .end

.list_not_empty:
    ; La lista no está vacía:
    ; last_node->next = new_node
    mov [rdx + NODE_NEXT], rbx
    ; new_node->previous = last_node
    mov [rbx + NODE_PREVIOUS], rdx
    ; list->last = new_node
    mov [rdi + LIST_LAST], rbx

.end:
    ; Restaurar registros callee-saved
    pop r13
    pop r12
    pop rbx
    mov rsp, rbp
    pop rbp
    ret

; char* string_proc_list_concat_asm(string_proc_list* list, uint8_t type, char* hash);
; Parametros: RDI = list, RSI = type (SIL), RDX = hash
; Retorna: Puntero al nuevo string concatenado en RAX, o NULL
string_proc_list_concat_asm:
    push rbp
    mov rbp, rsp
    push rbx        ; Preservar callee-saved registers
    push r12
    push r13
    push r14
    push r15

    ; Guardar parámetros
    mov r12, rdi    ; list en R12
    mov r13b, sil   ; type en R13B
    mov r14, rdx    ; initial_hash en R14

    ; Verificar si list o initial_hash son NULL
    cmp r12, NULL
    je .fail_null_input
    cmp r14, NULL
    je .fail_null_input

    ; --- Duplicar el hash inicial ---
    ; Necesitamos crear una copia del hash inicial para poder liberarla después
    ; 1. Calcular longitud de initial_hash (r14)
    mov rdi, r14
    call strlen     ; Longitud en RAX (sin el nulo)
    inc rax         ; Añadir 1 para el terminador nulo
    ; 2. Reservar memoria
    mov rdi, rax
    call malloc     ; Puntero a memoria en RAX
    cmp rax, NULL
    je .fail        ; Si malloc falla, retornar NULL
    mov r15, rax    ; Guardar puntero a memoria duplicada en R15 (current_result)
    ; 3. Copiar el string
    mov rdi, r15    ; Destino (memoria nueva)
    mov rsi, r14    ; Fuente (initial_hash)
    call strcpy     ; Copia el string

    ; --- Iterar por la lista ---
    ; current_node = list->first
    mov rbx, [r12 + LIST_FIRST] ; Puntero al nodo actual en RBX

.loop_start:
    ; Verificar si llegamos al final de la lista (current_node == NULL)
    cmp rbx, NULL
    je .loop_end

    ; Obtener el tipo del nodo actual: node_type = current_node->type
    movzx edi, byte [rbx + NODE_TYPE] ; edi = node_type (zero extended)

    ; Comparar node_type con el tipo buscado (r13b)
    cmp dil, r13b   ; Comparamos solo el byte bajo
    jne .next_node  ; Si no coincide, saltar al siguiente nodo

    ; --- Tipos coinciden: Concatenar ---
    ; Obtener el hash del nodo actual: node_hash = current_node->hash
    mov rsi, [rbx + NODE_HASH]

    ; Preparar argumentos para str_concat(current_result, node_hash)
    mov rdi, r15    ; current_result (en r15)
    ; rsi ya tiene node_hash
    call str_concat ; El nuevo resultado está en RAX

    ; Verificar si str_concat falló
    cmp rax, NULL
    je .fail_concat ; Si falla, liberar memoria y retornar NULL

    ; str_concat tuvo éxito, el nuevo resultado está en RAX
    ; Liberar el resultado anterior (que está en r15)
    mov rdi, r15
    call free

    ; Actualizar el puntero al resultado actual
    mov r15, rax

.next_node:
    ; Avanzar al siguiente nodo: current_node = current_node->next
    mov rbx, [rbx + NODE_NEXT]
    jmp .loop_start

.loop_end:
    ; La iteración terminó. El resultado final está en R15.
    mov rax, r15    ; Mover el resultado a RAX para el retorno
    jmp .end

.fail_concat:
    ; str_concat falló. Liberar la memoria del resultado actual (r15)
    mov rdi, r15
    call free
    ; Fallthrough to .fail

.fail:
.fail_null_input:
    mov rax, NULL   ; Retornar NULL en caso de error

.end:
    ; Restaurar registros callee-saved
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    mov rsp, rbp
    pop rbp
    ret