# Anagram Finder - Recherche d'Anagrammes en Assembleur i386

Programme de recherche d'anagrammes écrit entièrement en assembleur i386 pour Linux.

## Description

Ce programme recherche tous les anagrammes d'un mot donné dans un fichier dictionnaire. Deux mots sont des anagrammes s'ils contiennent exactement les mêmes lettres dans un ordre différent.

## Prérequis

- **NASM** (Netwide Assembler)
- **ld** (GNU linker)
- Système Linux 32-bit ou support 32-bit sur 64-bit

### Installation des dépendances

Sur Ubuntu/Debian :
```bash
sudo apt-get install nasm
sudo apt-get install gcc-multilib  # Pour support 32-bit sur système 64-bit
```

Sur Fedora/RHEL :
```bash
sudo dnf install nasm
sudo dnf install glibc-devel.i686
```

## Compilation

```bash
make
```

Cela génère l'exécutable `anagram`.

## Usage

```bash
./anagram <mot> <fichier_dictionnaire>
```

### Exemples

Rechercher les anagrammes de "chien" dans le dictionnaire français :
```bash
./anagram chien dict-fr.txt
```

Rechercher les anagrammes de "listen" dans le dictionnaire anglais :
```bash
./anagram listen dict-en.txt
```

## Format du dictionnaire

Le fichier dictionnaire doit contenir un mot par ligne :
```
mot1
mot2
mot3
```

## Fonctionnement interne

1. **Normalisation** : Le mot d'entrée est converti en minuscules
2. **Tri** : Les lettres du mot sont triées alphabétiquement
3. **Lecture** : Le programme lit le dictionnaire ligne par ligne
4. **Comparaison** : Pour chaque mot du dictionnaire :
   - Conversion en minuscules
   - Tri des lettres
   - Comparaison avec le mot d'entrée trié
5. **Affichage** : Les anagrammes trouvés sont affichés

### Algorithme de tri

Le programme utilise un **tri à bulles** (bubble sort) implémenté en assembleur pour trier les lettres.

## Fichiers

### Code source (4 versions)
- `anagram.asm` : NASM i386 (syntaxe Intel 32-bit)
- `anagram64.asm` : NASM x86-64 (syntaxe Intel 64-bit)
- `anagram-gas.s` : GNU Assembler i386 (syntaxe AT&T 32-bit)
- `anagram64.s` : GNU Assembler x86-64 (syntaxe AT&T 64-bit)

### Autres fichiers
- `Makefile` : Automatisation avec détection d'architecture
- `dict-fr.txt` : Dictionnaire de test en français
- `dict-en.txt` : Dictionnaire de test en anglais
- `README.md` : Cette documentation
- `.gitignore` : Exclusion des fichiers compilés

## Caractéristiques techniques

- **Architecture** : i386 (32-bit) et x86-64 (64-bit)
- **Détection automatique** : Architecture et assembleur disponible
- **Appels système** : Linux syscalls directs (sans libc)
  - **32-bit** : `int 0x80` (sys_open=5, sys_read=3, sys_write=4, sys_close=6, sys_exit=1)
  - **64-bit** : `syscall` (sys_open=2, sys_read=0, sys_write=1, sys_close=3, sys_exit=60)
- **Buffer de lecture** : 4096 octets
- **Longueur maximale** : 256 caractères par mot

## Nettoyage

Pour supprimer les fichiers compilés :
```bash
make clean
```

## Installation système (optionnel)

Pour installer dans `/usr/local/bin` :
```bash
sudo make install
```

Pour désinstaller :
```bash
sudo make uninstall
```

## Exemples de résultats

```bash
$ ./anagram chien dict-fr.txt
chien
niche
chine

$ ./anagram listen dict-en.txt
listen
silent
enlist
tinsel
inlets
```

## Limitations

- Supporte uniquement les caractères ASCII
- Longueur maximale de 256 caractères par mot
- Sensible à la casse du fichier d'entrée (converti en minuscules)

## Licence

Code libre - Usage éducatif et pratique

## Auteur

Créé comme démonstration de programmation en assembleur i386 sous Linux.
