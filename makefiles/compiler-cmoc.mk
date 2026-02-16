# compiler-cmoc.mk - CMOC Compiler Configuration
#
# Configuration for the CMOC C compiler
# Used for: Tandy Color Computer (6809)

# Compiler
CC := cmoc

# Archiver
AR := lwar

# Common C flags
# -O2: Optimize
CFLAGS := -O2

# Assembler flags (lwasm)
ASFLAGS :=

# Object file extension
OBJEXT := .o

# Assembly file extension
ASMEXT := .s

# Include path prefix
INCC_ARG := -I 
INCS_ARG := -I 

# Linker flags
LDFLAGS :=

# Library creation command
AR_CMD = $(AR) -q $(1) $(2)
