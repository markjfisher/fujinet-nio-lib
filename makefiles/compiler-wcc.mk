# compiler-wcc.mk - Watcom Compiler Configuration
#
# Configuration for the Open Watcom C compiler (wcc)
# Used for: MS-DOS (x86)

# Compiler
CC := wcc

# Archiver
AR := wlib

# Common C flags
# -0:   Generate 8086 code
# -os:  Optimize for size
# -ms:  Small memory model
CFLAGS := -0 -os -ms

# Assembler flags (wasm)
ASFLAGS :=

# Object file extension
OBJEXT := .obj

# Assembly file extension
ASMEXT := .asm

# Include path prefix
INCC_ARG := -i=
INCS_ARG := -i=

# Linker flags
LDFLAGS :=

# Library creation command
AR_CMD = $(AR) -q $(1) $(2)
