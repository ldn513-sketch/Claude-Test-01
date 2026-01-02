; anagram.asm - Programme de recherche d'anagrammes en assembleur i386
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
    input_len: resd 1         ; longueur du mot

    dict_line: resb 256       ; ligne du dictionnaire
    dict_sorted: resb 256     ; ligne triée

    buffer: resb 4096         ; buffer de lecture
    file_descriptor: resd 1

section .text
    global _start

_start:
    ; Vérifier les arguments (argc devrait être 3)
    pop eax                   ; argc
    cmp eax, 3
    jne usage_error

    pop ebx                   ; argv[0] (nom du programme)
    pop ebx                   ; argv[1] (mot à rechercher)
    pop ecx                   ; argv[2] (fichier dictionnaire)

    ; Sauvegarder les pointeurs
    push ecx                  ; fichier
    push ebx                  ; mot

    ; Copier et calculer la longueur du mot d'entrée
    pop esi                   ; mot à rechercher
    mov edi, input_word
    call copy_and_lowercase
    mov [input_len], ecx

    ; Trier le mot d'entrée
    mov esi, input_word
    mov edi, input_sorted
    mov ecx, [input_len]
    call copy_string
    mov esi, input_sorted
    mov ecx, [input_len]
    call sort_string

    ; Ouvrir le fichier dictionnaire
    pop ebx                   ; nom du fichier
    mov eax, 5                ; sys_open
    mov ecx, 0                ; O_RDONLY
    int 0x80

    cmp eax, 0
    jl file_error
    mov [file_descriptor], eax

    ; Lire et traiter le dictionnaire
    call process_dictionary

    ; Fermer le fichier
    mov eax, 6                ; sys_close
    mov ebx, [file_descriptor]
    int 0x80

    ; Sortie normale
    mov eax, 1                ; sys_exit
    xor ebx, ebx              ; code 0
    int 0x80

usage_error:
    mov eax, 4                ; sys_write
    mov ebx, 1                ; stdout
    mov ecx, usage_msg
    mov edx, usage_len
    int 0x80

    mov eax, 1                ; sys_exit
    mov ebx, 1                ; code d'erreur
    int 0x80

file_error:
    mov eax, 4                ; sys_write
    mov ebx, 1                ; stdout
    mov ecx, error_open
    mov edx, error_open_len
    int 0x80

    mov eax, 1                ; sys_exit
    mov ebx, 1
    int 0x80

; Fonction: process_dictionary
; Lit le dictionnaire ligne par ligne et cherche les anagrammes
process_dictionary:
    push ebp
    mov ebp, esp
    sub esp, 8
    mov dword [ebp-4], 0      ; position dans le buffer
    mov dword [ebp-8], 0      ; bytes lus

.read_more:
    ; Lire depuis le fichier
    mov eax, 3                ; sys_read
    mov ebx, [file_descriptor]
    mov ecx, buffer
    mov edx, 4096
    int 0x80

    cmp eax, 0
    jle .done                 ; EOF ou erreur

    mov [ebp-8], eax          ; sauvegarder bytes lus
    mov dword [ebp-4], 0      ; reset position

.process_buffer:
    mov edx, [ebp-4]
    cmp edx, [ebp-8]
    jge .read_more

    ; Extraire une ligne
    mov esi, buffer
    add esi, edx
    mov edi, dict_line
    xor ecx, ecx

.extract_line:
    mov edx, [ebp-4]
    cmp edx, [ebp-8]
    jge .check_word

    mov al, [buffer + edx]
    inc dword [ebp-4]

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
    mov [edi], al
    inc edi
    inc ecx
    jmp .extract_line

.check_word:
    cmp ecx, 0
    je .process_buffer

    ; Vérifier si même longueur
    cmp ecx, [input_len]
    jne .process_buffer

    ; Terminer la chaîne
    mov byte [edi], 0

    ; Copier et trier le mot du dictionnaire
    push ecx
    mov esi, dict_line
    mov edi, dict_sorted
    call copy_string
    mov esi, dict_sorted
    call sort_string
    pop ecx

    ; Comparer avec le mot d'entrée trié
    mov esi, input_sorted
    mov edi, dict_sorted
    call compare_strings

    cmp eax, 1
    jne .process_buffer

    ; C'est un anagramme ! L'afficher
    push dword [ebp-4]
    push dword [ebp-8]

    mov eax, 4                ; sys_write
    mov ebx, 1                ; stdout
    mov ecx, dict_line
    mov edx, [input_len]
    int 0x80

    mov eax, 4
    mov ebx, 1
    mov ecx, newline
    mov edx, 1
    int 0x80

    pop dword [ebp-8]
    pop dword [ebp-4]

    jmp .process_buffer

.done:
    mov esp, ebp
    pop ebp
    ret

; Fonction: copy_and_lowercase
; Entrée: ESI = source, EDI = destination
; Sortie: ECX = longueur
copy_and_lowercase:
    push esi
    push edi
    xor ecx, ecx

.loop:
    lodsb                     ; AL = [ESI++]
    cmp al, 0
    je .done

    ; Convertir en minuscule
    cmp al, 'A'
    jl .store
    cmp al, 'Z'
    jg .store
    add al, 32

.store:
    stosb                     ; [EDI++] = AL
    inc ecx
    jmp .loop

.done:
    mov byte [edi], 0
    pop edi
    pop esi
    ret

; Fonction: copy_string
; Entrée: ESI = source, EDI = destination, ECX = longueur
copy_string:
    push esi
    push edi
    push ecx

.loop:
    cmp ecx, 0
    je .done
    lodsb
    stosb
    dec ecx
    jmp .loop

.done:
    mov byte [edi], 0
    pop ecx
    pop edi
    pop esi
    ret

; Fonction: sort_string (tri à bulles)
; Entrée: ESI = chaîne, ECX = longueur
sort_string:
    push ebp
    mov ebp, esp
    push esi
    push ecx

    cmp ecx, 1
    jle .done

.outer_loop:
    mov edx, 0                ; flag de swap
    mov edi, [ebp-8]          ; longueur - 1
    dec edi

.inner_loop:
    cmp edi, 0
    jle .check_swap

    mov al, [esi + edi - 1]
    mov bl, [esi + edi]

    cmp al, bl
    jle .no_swap

    ; Échanger
    mov [esi + edi - 1], bl
    mov [esi + edi], al
    mov edx, 1                ; swap effectué

.no_swap:
    dec edi
    jmp .inner_loop

.check_swap:
    cmp edx, 1
    je .outer_loop

.done:
    pop ecx
    pop esi
    pop ebp
    ret

; Fonction: compare_strings
; Entrée: ESI = string1, EDI = string2, ECX = longueur
; Sortie: EAX = 1 si égales, 0 sinon
compare_strings:
    push esi
    push edi
    push ecx

.loop:
    cmp ecx, 0
    je .equal

    mov al, [esi]
    mov bl, [edi]

    cmp al, bl
    jne .not_equal

    inc esi
    inc edi
    dec ecx
    jmp .loop

.equal:
    mov eax, 1
    jmp .done

.not_equal:
    xor eax, eax

.done:
    pop ecx
    pop edi
    pop esi
    ret
