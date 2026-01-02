# Makefile pour le programme de recherche d'anagrammes en assembleur

# Détection de l'architecture
ARCH := $(shell uname -m)

# Détection de l'assembleur disponible
NASM := $(shell command -v nasm 2> /dev/null)
GAS := $(shell command -v as 2> /dev/null)

# Configuration selon l'assembleur et l'architecture
ifdef NASM
    AS = nasm
    ifeq ($(ARCH),x86_64)
        ASFLAGS = -f elf64
        SRC = anagram.asm
        LDFLAGS = -m elf_x86_64
    else
        ASFLAGS = -f elf32
        SRC = anagram.asm
        LDFLAGS = -m elf_i386
    endif
    $(info Using NASM assembler for $(ARCH))
else ifdef GAS
    AS = as
    ifeq ($(ARCH),x86_64)
        ASFLAGS = --64
        SRC = anagram64.s
        LDFLAGS = -m elf_x86_64
    else
        ASFLAGS = --32
        SRC = anagram-gas.s
        LDFLAGS = -m elf_i386
    endif
    $(info Using GNU Assembler (gas) for $(ARCH))
else
    $(error No assembler found. Please install nasm or ensure 'as' is available)
endif

# Éditeur de liens
LD = ld

# Cibles
TARGET = anagram
OBJ = anagram.o

# Règle par défaut
all: $(TARGET)

# Compilation et édition de liens
$(TARGET): $(OBJ)
	$(LD) $(LDFLAGS) -o $(TARGET) $(OBJ)

$(OBJ): $(SRC)
	$(AS) $(ASFLAGS) $(SRC) -o $(OBJ)

# Nettoyage
clean:
	rm -f $(OBJ) $(TARGET)

# Test avec un dictionnaire exemple
test: $(TARGET)
	@echo "Test 1: Recherche d'anagrammes de 'chien'"
	./$(TARGET) chien dict-fr.txt
	@echo ""
	@echo "Test 2: Recherche d'anagrammes de 'listen'"
	./$(TARGET) listen dict-en.txt

# Installation (optionnel)
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

uninstall:
	rm -f /usr/local/bin/$(TARGET)

.PHONY: all clean test install uninstall
