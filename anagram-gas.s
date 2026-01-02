# anagram-gas.s - Programme de recherche d'anagrammes en assembleur i386 (syntaxe AT&T pour gas)
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
    .lcomm input_len, 4         # longueur du mot

    .lcomm dict_line, 256       # ligne du dictionnaire
    .lcomm dict_sorted, 256     # ligne triée

    .lcomm buffer, 4096         # buffer de lecture
    .lcomm file_descriptor, 4

.section .text
    .globl _start

_start:
    # Vérifier les arguments (argc devrait être 3)
    popl %eax                   # argc
    cmpl $3, %eax
    jne usage_error

    popl %ebx                   # argv[0] (nom du programme)
    popl %ebx                   # argv[1] (mot à rechercher)
    popl %ecx                   # argv[2] (fichier dictionnaire)

    # Sauvegarder les pointeurs
    pushl %ecx                  # fichier
    pushl %ebx                  # mot

    # Copier et calculer la longueur du mot d'entrée
    popl %esi                   # mot à rechercher
    movl $input_word, %edi
    call copy_and_lowercase
    movl %ecx, input_len

    # Trier le mot d'entrée
    movl $input_word, %esi
    movl $input_sorted, %edi
    movl input_len, %ecx
    call copy_string
    movl $input_sorted, %esi
    movl input_len, %ecx
    call sort_string

    # Ouvrir le fichier dictionnaire
    popl %ebx                   # nom du fichier
    movl $5, %eax               # sys_open
    xorl %ecx, %ecx             # O_RDONLY
    int $0x80

    cmpl $0, %eax
    jl file_error
    movl %eax, file_descriptor

    # Lire et traiter le dictionnaire
    call process_dictionary

    # Fermer le fichier
    movl $6, %eax               # sys_close
    movl file_descriptor, %ebx
    int $0x80

    # Sortie normale
    movl $1, %eax               # sys_exit
    xorl %ebx, %ebx             # code 0
    int $0x80

usage_error:
    movl $4, %eax               # sys_write
    movl $1, %ebx               # stdout
    movl $usage_msg, %ecx
    movl $usage_len, %edx
    int $0x80

    movl $1, %eax               # sys_exit
    movl $1, %ebx               # code d'erreur
    int $0x80

file_error:
    movl $4, %eax               # sys_write
    movl $1, %ebx               # stdout
    movl $error_open, %ecx
    movl $error_open_len, %edx
    int $0x80

    movl $1, %eax               # sys_exit
    movl $1, %ebx
    int $0x80

# Fonction: process_dictionary
# Lit le dictionnaire ligne par ligne et cherche les anagrammes
process_dictionary:
    pushl %ebp
    movl %esp, %ebp
    subl $8, %esp
    movl $0, -4(%ebp)           # position dans le buffer
    movl $0, -8(%ebp)           # bytes lus

.read_more:
    # Lire depuis le fichier
    movl $3, %eax               # sys_read
    movl file_descriptor, %ebx
    movl $buffer, %ecx
    movl $4096, %edx
    int $0x80

    cmpl $0, %eax
    jle .done                   # EOF ou erreur

    movl %eax, -8(%ebp)         # sauvegarder bytes lus
    movl $0, -4(%ebp)           # reset position

.process_buffer:
    movl -4(%ebp), %edx
    cmpl -8(%ebp), %edx
    jge .read_more

    # Extraire une ligne
    movl $buffer, %esi
    addl %edx, %esi
    movl $dict_line, %edi
    xorl %ecx, %ecx

.extract_line:
    movl -4(%ebp), %edx
    cmpl -8(%ebp), %edx
    jge .check_word

    movzbl buffer(%edx), %eax
    incl -4(%ebp)

    cmpb $10, %al               # newline
    je .check_word

    cmpb $13, %al               # carriage return
    je .extract_line

    # Convertir en minuscule
    cmpb $'A', %al
    jl .store_char
    cmpb $'Z', %al
    jg .store_char
    addb $32, %al

.store_char:
    movb %al, (%edi)
    incl %edi
    incl %ecx
    jmp .extract_line

.check_word:
    cmpl $0, %ecx
    je .process_buffer

    # Vérifier si même longueur
    cmpl input_len, %ecx
    jne .process_buffer

    # Terminer la chaîne
    movb $0, (%edi)

    # Copier et trier le mot du dictionnaire
    pushl %ecx
    movl $dict_line, %esi
    movl $dict_sorted, %edi
    call copy_string
    movl $dict_sorted, %esi
    call sort_string
    popl %ecx

    # Comparer avec le mot d'entrée trié
    movl $input_sorted, %esi
    movl $dict_sorted, %edi
    call compare_strings

    cmpl $1, %eax
    jne .process_buffer

    # C'est un anagramme ! L'afficher
    pushl -4(%ebp)
    pushl -8(%ebp)

    movl $4, %eax               # sys_write
    movl $1, %ebx               # stdout
    movl $dict_line, %ecx
    movl input_len, %edx
    int $0x80

    movl $4, %eax
    movl $1, %ebx
    movl $newline, %ecx
    movl $1, %edx
    int $0x80

    popl -8(%ebp)
    popl -4(%ebp)

    jmp .process_buffer

.done:
    movl %ebp, %esp
    popl %ebp
    ret

# Fonction: copy_and_lowercase
# Entrée: ESI = source, EDI = destination
# Sortie: ECX = longueur
copy_and_lowercase:
    pushl %esi
    pushl %edi
    xorl %ecx, %ecx

.cal_loop:
    lodsb                       # AL = [ESI++]
    cmpb $0, %al
    je .cal_done

    # Convertir en minuscule
    cmpb $'A', %al
    jl .cal_store
    cmpb $'Z', %al
    jg .cal_store
    addb $32, %al

.cal_store:
    stosb                       # [EDI++] = AL
    incl %ecx
    jmp .cal_loop

.cal_done:
    movb $0, (%edi)
    popl %edi
    popl %esi
    ret

# Fonction: copy_string
# Entrée: ESI = source, EDI = destination, ECX = longueur
copy_string:
    pushl %esi
    pushl %edi
    pushl %ecx

.cs_loop:
    cmpl $0, %ecx
    je .cs_done
    lodsb
    stosb
    decl %ecx
    jmp .cs_loop

.cs_done:
    movb $0, (%edi)
    popl %ecx
    popl %edi
    popl %esi
    ret

# Fonction: sort_string (tri à bulles)
# Entrée: ESI = chaîne, ECX = longueur
sort_string:
    pushl %ebp
    movl %esp, %ebp
    pushl %esi
    pushl %ecx

    cmpl $1, %ecx
    jle .ss_done

.ss_outer_loop:
    movl $0, %edx               # flag de swap
    movl -8(%ebp), %edi         # longueur - 1
    decl %edi

.ss_inner_loop:
    cmpl $0, %edi
    jle .ss_check_swap

    movl -4(%ebp), %esi         # restaurer esi
    movzbl -1(%esi,%edi), %eax
    movzbl (%esi,%edi), %ebx

    cmpl %ebx, %eax
    jle .ss_no_swap

    # Échanger
    movl -4(%ebp), %esi
    movb %bl, -1(%esi,%edi)
    movb %al, (%esi,%edi)
    movl $1, %edx               # swap effectué

.ss_no_swap:
    decl %edi
    jmp .ss_inner_loop

.ss_check_swap:
    cmpl $1, %edx
    je .ss_outer_loop

.ss_done:
    popl %ecx
    popl %esi
    popl %ebp
    ret

# Fonction: compare_strings
# Entrée: ESI = string1, EDI = string2, ECX = longueur
# Sortie: EAX = 1 si égales, 0 sinon
compare_strings:
    pushl %esi
    pushl %edi
    pushl %ecx

.cmp_loop:
    cmpl $0, %ecx
    je .cmp_equal

    movzbl (%esi), %eax
    movzbl (%edi), %ebx

    cmpl %ebx, %eax
    jne .cmp_not_equal

    incl %esi
    incl %edi
    decl %ecx
    jmp .cmp_loop

.cmp_equal:
    movl $1, %eax
    jmp .cmp_done

.cmp_not_equal:
    xorl %eax, %eax

.cmp_done:
    popl %ecx
    popl %edi
    popl %esi
    ret
