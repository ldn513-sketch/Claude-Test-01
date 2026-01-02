; anagram64.asm - Programme de recherche d'anagrammes en assembleur x86-64 (NASM)
; Usage: ./anagram <mot> <fichier_dictionnaire>

section .data
    usage_msg: db "Usage: ./anagram <mot> <fichier_dictionnaire>", 10
    usage_len: equ $ - usage_msg

    error_open: db "Erreur: impossible d'ouvrir le fichier", 10
    error_open_len: equ $ - error_open

    newline: db 10

section .bss
    input_word: resb 256      ; mot à rechercher
    input_sorted: resb 256    ; mot trié
    input_len: resq 1         ; longueur du mot (64-bit)

    dict_line: resb 256       ; ligne du dictionnaire
    dict_sorted: resb 256     ; ligne triée

    buffer: resb 4096         ; buffer de lecture
    file_descriptor: resq 1   ; descripteur de fichier (64-bit)

section .text
    global _start

_start:
    ; Vérifier les arguments (argc devrait être 3)
    pop rax                   ; argc
    cmp rax, 3
    jne usage_error

    pop rbx                   ; argv[0] (nom du programme)
    pop rbx                   ; argv[1] (mot à rechercher)
    pop rcx                   ; argv[2] (fichier dictionnaire)

    ; Sauvegarder les pointeurs
    push rcx                  ; fichier
    push rbx                  ; mot

    ; Copier et calculer la longueur du mot d'entrée
    pop rsi                   ; mot à rechercher
    mov rdi, input_word
    call copy_and_lowercase
    mov [input_len], rcx

    ; Trier le mot d'entrée
    mov rsi, input_word
    mov rdi, input_sorted
    mov rcx, [input_len]
    call copy_string
    mov rsi, input_sorted
    mov rcx, [input_len]
    call sort_string

    ; Ouvrir le fichier dictionnaire
    pop rdi                   ; nom du fichier (1er paramètre syscall)
    mov rax, 2                ; sys_open
    xor rsi, rsi              ; O_RDONLY
    syscall

    cmp rax, 0
    jl file_error
    mov [file_descriptor], rax

    ; Lire et traiter le dictionnaire
    call process_dictionary

    ; Fermer le fichier
    mov rax, 3                ; sys_close
    mov rdi, [file_descriptor]
    syscall

    ; Sortie normale
    mov rax, 60               ; sys_exit
    xor rdi, rdi              ; code 0
    syscall

usage_error:
    mov rax, 1                ; sys_write
    mov rdi, 1                ; stdout
    mov rsi, usage_msg
    mov rdx, usage_len
    syscall

    mov rax, 60               ; sys_exit
    mov rdi, 1                ; code d'erreur
    syscall

file_error:
    mov rax, 1                ; sys_write
    mov rdi, 1                ; stdout
    mov rsi, error_open
    mov rdx, error_open_len
    syscall

    mov rax, 60               ; sys_exit
    mov rdi, 1
    syscall

; Fonction: process_dictionary
; Lit le dictionnaire ligne par ligne et cherche les anagrammes
process_dictionary:
    push rbp
    mov rbp, rsp
    sub rsp, 16
    mov qword [rbp-8], 0      ; position dans le buffer
    mov qword [rbp-16], 0     ; bytes lus

.read_more:
    ; Lire depuis le fichier
    mov rax, 0                ; sys_read
    mov rdi, [file_descriptor]
    mov rsi, buffer
    mov rdx, 4096
    syscall

    cmp rax, 0
    jle .done                 ; EOF ou erreur

    mov [rbp-16], rax         ; sauvegarder bytes lus
    mov qword [rbp-8], 0      ; reset position

.process_buffer:
    mov rdx, [rbp-8]
    cmp rdx, [rbp-16]
    jge .read_more

    ; Extraire une ligne
    mov rsi, buffer
    add rsi, rdx
    mov rdi, dict_line
    xor rcx, rcx

.extract_line:
    mov rdx, [rbp-8]
    cmp rdx, [rbp-16]
    jge .check_word

    mov al, [buffer + rdx]
    inc qword [rbp-8]

    cmp al, 10                ; newline
    je .check_word

    cmp al, 13                ; carriage return
    je .extract_line

    ; Convertir en minuscule
    cmp al, 'A'
    jl .store_char
    cmp al, 'Z'
    jg .store_char
    add al, 32

.store_char:
    mov [rdi], al
    inc rdi
    inc rcx
    jmp .extract_line

.check_word:
    cmp rcx, 0
    je .process_buffer

    ; Vérifier si même longueur
    cmp rcx, [input_len]
    jne .process_buffer

    ; Terminer la chaîne
    mov byte [rdi], 0

    ; Copier et trier le mot du dictionnaire
    push rcx
    mov rsi, dict_line
    mov rdi, dict_sorted
    call copy_string
    mov rsi, dict_sorted
    call sort_string
    pop rcx

    ; Comparer avec le mot d'entrée trié
    mov rsi, input_sorted
    mov rdi, dict_sorted
    call compare_strings

    cmp rax, 1
    jne .process_buffer

    ; C'est un anagramme ! L'afficher
    push qword [rbp-8]
    push qword [rbp-16]

    mov rax, 1                ; sys_write
    mov rdi, 1                ; stdout
    mov rsi, dict_line
    mov rdx, [input_len]
    syscall

    mov rax, 1
    mov rdi, 1
    mov rsi, newline
    mov rdx, 1
    syscall

    pop qword [rbp-16]
    pop qword [rbp-8]

    jmp .process_buffer

.done:
    mov rsp, rbp
    pop rbp
    ret

; Fonction: copy_and_lowercase
; Entrée: RSI = source, RDI = destination
; Sortie: RCX = longueur
copy_and_lowercase:
    push rsi
    push rdi
    xor rcx, rcx

.loop:
    lodsb                     ; AL = [RSI++]
    cmp al, 0
    je .done

    ; Convertir en minuscule
    cmp al, 'A'
    jl .store
    cmp al, 'Z'
    jg .store
    add al, 32

.store:
    stosb                     ; [RDI++] = AL
    inc rcx
    jmp .loop

.done:
    mov byte [rdi], 0
    pop rdi
    pop rsi
    ret

; Fonction: copy_string
; Entrée: RSI = source, RDI = destination, RCX = longueur
copy_string:
    push rsi
    push rdi
    push rcx

.loop:
    cmp rcx, 0
    je .done
    lodsb
    stosb
    dec rcx
    jmp .loop

.done:
    mov byte [rdi], 0
    pop rcx
    pop rdi
    pop rsi
    ret

; Fonction: sort_string (tri à bulles)
; Entrée: RSI = chaîne, RCX = longueur
sort_string:
    push rbp
    mov rbp, rsp
    push rsi
    push rcx

    cmp rcx, 1
    jle .done

.outer_loop:
    mov rdx, 0                ; flag de swap
    mov rdi, [rbp-16]         ; longueur - 1
    dec rdi

.inner_loop:
    cmp rdi, 0
    jle .check_swap

    mov rsi, [rbp-8]          ; restaurer rsi
    mov al, [rsi + rdi - 1]
    mov bl, [rsi + rdi]

    cmp al, bl
    jle .no_swap

    ; Échanger
    mov rsi, [rbp-8]
    mov [rsi + rdi - 1], bl
    mov [rsi + rdi], al
    mov rdx, 1                ; swap effectué

.no_swap:
    dec rdi
    jmp .inner_loop

.check_swap:
    cmp rdx, 1
    je .outer_loop

.done:
    pop rcx
    pop rsi
    pop rbp
    ret

; Fonction: compare_strings
; Entrée: RSI = string1, RDI = string2, RCX = longueur
; Sortie: RAX = 1 si égales, 0 sinon
compare_strings:
    push rsi
    push rdi
    push rcx

.loop:
    cmp rcx, 0
    je .equal

    mov al, [rsi]
    mov bl, [rdi]

    cmp al, bl
    jne .not_equal

    inc rsi
    inc rdi
    dec rcx
    jmp .loop

.equal:
    mov rax, 1
    jmp .done

.not_equal:
    xor rax, rax

.done:
    pop rcx
    pop rdi
    pop rsi
    ret
