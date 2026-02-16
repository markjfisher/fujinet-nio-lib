# compiler-cc65.mk - CC65 Compiler Configuration
#
# Configuration for the CC65 C compiler suite (cl65)
# Used for: Atari, Apple II, Commodore 64, and other 6502 targets

# Compiler
CC := cl65

# Archiver
AR := ar65

# Common C flags
# -Osir: Optimize for size, inline runtime functions
# -O:    Enable optimizer
CFLAGS := -Osir -O

# Assembler flags
ASFLAGS :=

# Object file extension
OBJEXT := .o

# Assembly file extension
ASMEXT := .s

# Include path prefix (for -I flags)
INCC_ARG := --include-dir 
INCS_ARG := --asm-include-dir 

# Linker flags (if needed for testing)
LDFLAGS :=

# Library creation command
# ar65 a library.lib obj1.o obj2.o ...
AR_CMD = $(AR) a $(1) $(2)
