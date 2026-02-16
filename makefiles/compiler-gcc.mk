# compiler-gcc.mk - GCC Compiler Configuration for Linux/POSIX
#
# Native Linux build using GCC

# Compiler tools
CC = gcc
AR = ar
AS = as

# Platform name (used for source directory)
PLATFORM = linux

# Compiler flags
CFLAGS = -Wall -Wextra -O2 -std=c99

# Assembler flags (not typically used for Linux target)
ASFLAGS =

# Archiver flags
ARFLAGS = rcs

# Output extension
LIBEXT = .a
