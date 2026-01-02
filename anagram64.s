# anagram64.s - Programme de recherche d'anagrammes en assembleur x86-64 (syntaxe AT&T)
# Usage: ./anagram <mot> <fichier_dictionnaire>

.section .data
    usage_msg: .ascii "Usage: ./anagram <mot> <fichier_dictionnaire>\n"
    usage_len = . - usage_msg

    error_open: .ascii "Erreur: impossible d'ouvrir le fichier\n"
    error_open_len = . - error_open

    newline: .ascii "\n"

.section .bss
    .lcomm input_word, 256      # mot à rechercher
    .lcomm input_sorted, 256    # mot trié
    .lcomm input_len, 8         # longueur du mot

    .lcomm dict_line, 256       # ligne du dictionnaire
    .lcomm dict_sorted, 256     # ligne triée

    .lcomm buffer, 4096         # buffer de lecture
    .lcomm file_descriptor, 8

.section .text
    .globl _start

_start:
    # Vérifier les arguments (argc devrait être 3)
    popq %rax                   # argc
    cmpq $3, %rax
    jne usage_error

    popq %rbx                   # argv[0] (nom du programme)
    popq %rbx                   # argv[1] (mot à rechercher)
    popq %rcx                   # argv[2] (fichier dictionnaire)

    # Sauvegarder les pointeurs
    pushq %rcx                  # fichier
    pushq %rbx                  # mot

    # Copier et calculer la longueur du mot d'entrée
    popq %rsi                   # mot à rechercher
    movq $input_word, %rdi
    call copy_and_lowercase
    movq %rcx, input_len(%rip)

    # Trier le mot d'entrée
    movq $input_word, %rsi
    movq $input_sorted, %rdi
    movq input_len(%rip), %rcx
    call copy_string
    movq $input_sorted, %rsi
    movq input_len(%rip), %rcx
    call sort_string

    # Ouvrir le fichier dictionnaire
    popq %rdi                   # nom du fichier (1er arg pour syscall)
    movq $2, %rax               # sys_open
    xorq %rsi, %rsi             # O_RDONLY
    syscall

    cmpq $0, %rax
    jl file_error
    movq %rax, file_descriptor(%rip)

    # Lire et traiter le dictionnaire
    call process_dictionary

    # Fermer le fichier
    movq $3, %rax               # sys_close
    movq file_descriptor(%rip), %rdi
    syscall

    # Sortie normale
    movq $60, %rax              # sys_exit
    xorq %rdi, %rdi             # code 0
    syscall

usage_error:
    movq $1, %rax               # sys_write
    movq $1, %rdi               # stdout
    movq $usage_msg, %rsi
    movq $usage_len, %rdx
    syscall

    movq $60, %rax              # sys_exit
    movq $1, %rdi               # code d'erreur
    syscall

file_error:
    movq $1, %rax               # sys_write
    movq $1, %rdi               # stdout
    movq $error_open, %rsi
    movq $error_open_len, %rdx
    syscall

    movq $60, %rax              # sys_exit
    movq $1, %rdi
    syscall

# Fonction: process_dictionary
process_dictionary:
    pushq %rbp
    movq %rsp, %rbp
    subq $16, %rsp
    movq $0, -8(%rbp)           # position dans le buffer
    movq $0, -16(%rbp)          # bytes lus

.pd_read_more:
    # Lire depuis le fichier
    movq $0, %rax               # sys_read
    movq file_descriptor(%rip), %rdi
    movq $buffer, %rsi
    movq $4096, %rdx
    syscall

    cmpq $0, %rax
    jle .pd_done                # EOF ou erreur

    movq %rax, -16(%rbp)        # sauvegarder bytes lus
    movq $0, -8(%rbp)           # reset position

.pd_process_buffer:
    movq -8(%rbp), %rdx
    cmpq -16(%rbp), %rdx
    jge .pd_read_more

    # Extraire une ligne
    movq $dict_line, %rdi
    xorq %rcx, %rcx

.pd_extract_line:
    movq -8(%rbp), %rdx
    cmpq -16(%rbp), %rdx
    jge .pd_check_word

    leaq buffer(%rip), %rsi
    movzbl (%rsi,%rdx), %eax
    incq -8(%rbp)

    cmpb $10, %al               # newline
    je .pd_check_word

    cmpb $13, %al               # carriage return
    je .pd_extract_line

    # Convertir en minuscule
    cmpb $'A', %al
    jl .pd_store_char
    cmpb $'Z', %al
    jg .pd_store_char
    addb $32, %al

.pd_store_char:
    movb %al, (%rdi)
    incq %rdi
    incq %rcx
    jmp .pd_extract_line

.pd_check_word:
    cmpq $0, %rcx
    je .pd_process_buffer

    # Vérifier si même longueur
    cmpq input_len(%rip), %rcx
    jne .pd_process_buffer

    # Terminer la chaîne
    movb $0, (%rdi)

    # Copier et trier le mot du dictionnaire
    pushq %rcx
    movq $dict_line, %rsi
    movq $dict_sorted, %rdi
    call copy_string
    movq $dict_sorted, %rsi
    call sort_string
    popq %rcx

    # Comparer avec le mot d'entrée trié
    movq $input_sorted, %rsi
    movq $dict_sorted, %rdi
    call compare_strings

    cmpq $1, %rax
    jne .pd_process_buffer

    # C'est un anagramme ! L'afficher
    pushq -8(%rbp)
    pushq -16(%rbp)

    movq $1, %rax               # sys_write
    movq $1, %rdi               # stdout
    movq $dict_line, %rsi
    movq input_len(%rip), %rdx
    syscall

    movq $1, %rax
    movq $1, %rdi
    movq $newline, %rsi
    movq $1, %rdx
    syscall

    popq -16(%rbp)
    popq -8(%rbp)

    jmp .pd_process_buffer

.pd_done:
    movq %rbp, %rsp
    popq %rbp
    ret

# Fonction: copy_and_lowercase
copy_and_lowercase:
    pushq %rsi
    pushq %rdi
    xorq %rcx, %rcx

.cal_loop:
    lodsb
    cmpb $0, %al
    je .cal_done

    cmpb $'A', %al
    jl .cal_store
    cmpb $'Z', %al
    jg .cal_store
    addb $32, %al

.cal_store:
    stosb
    incq %rcx
    jmp .cal_loop

.cal_done:
    movb $0, (%rdi)
    popq %rdi
    popq %rsi
    ret

# Fonction: copy_string
copy_string:
    pushq %rsi
    pushq %rdi
    pushq %rcx

.cs_loop:
    cmpq $0, %rcx
    je .cs_done
    lodsb
    stosb
    decq %rcx
    jmp .cs_loop

.cs_done:
    movb $0, (%rdi)
    popq %rcx
    popq %rdi
    popq %rsi
    ret

# Fonction: sort_string
sort_string:
    pushq %rbp
    movq %rsp, %rbp
    pushq %rsi
    pushq %rcx

    cmpq $1, %rcx
    jle .ss_done

.ss_outer_loop:
    movq $0, %rdx               # flag de swap
    movq -16(%rbp), %rdi        # longueur - 1
    decq %rdi

.ss_inner_loop:
    cmpq $0, %rdi
    jle .ss_check_swap

    movq -8(%rbp), %rsi
    movzbl -1(%rsi,%rdi), %eax
    movzbl (%rsi,%rdi), %ebx

    cmpq %rbx, %rax
    jle .ss_no_swap

    movq -8(%rbp), %rsi
    movb %bl, -1(%rsi,%rdi)
    movb %al, (%rsi,%rdi)
    movq $1, %rdx

.ss_no_swap:
    decq %rdi
    jmp .ss_inner_loop

.ss_check_swap:
    cmpq $1, %rdx
    je .ss_outer_loop

.ss_done:
    popq %rcx
    popq %rsi
    popq %rbp
    ret

# Fonction: compare_strings
compare_strings:
    pushq %rsi
    pushq %rdi
    pushq %rcx

.cmp_loop:
    cmpq $0, %rcx
    je .cmp_equal

    movzbl (%rsi), %eax
    movzbl (%rdi), %ebx

    cmpq %rbx, %rax
    jne .cmp_not_equal

    incq %rsi
    incq %rdi
    decq %rcx
    jmp .cmp_loop

.cmp_equal:
    movq $1, %rax
    jmp .cmp_done

.cmp_not_equal:
    xorq %rax, %rax

.cmp_done:
    popq %rcx
    popq %rdi
    popq %rsi
    ret
