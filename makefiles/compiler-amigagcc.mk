# compiler-amigagcc.mk - amiga-gcc cross-compiler for m68k-amigaos
#
# Requires amiga-gcc toolchain installed, e.g.:
#   https://github.com/bebbo/amiga-gcc
#
# Typical install location: /opt/amiga/bin/
# Add to PATH: export PATH=/opt/amiga/bin:$PATH

# Compiler tools
CC      = m68k-amigaos-gcc
AR      = m68k-amigaos-ar
AS      = m68k-amigaos-as

# Compiler flags
# -mcpu=68000: target base Amiga CPU (A500/A1000/A2000 compatible)
# -msoft-float: no FPU (68000 has none)
# -DNO_INLINE_MULDIV: avoid inline multiply/divide that may need 68020+
CFLAGS  = -Wall -O2 -std=c99 -mcpu=68000 -msoft-float -DNO_INLINE_MULDIV

# Keep the compiler-generated m68k assembly beside each Amiga object.  Using
# GCC's obj-relative save-temps mode lets the normal compile still produce the
# object while retaining the optimized .s output for inspection.
CFLAGS += -save-temps=obj

# Assembler flags
ASFLAGS =

# Archiver flags
ARFLAGS = rcs

# Output extension
LIBEXT  = .a
