# targets.mk - Target Platform Definitions
#
# Maps build targets to platforms and compiler families.

# Platform mapping
# Each target maps to a platform directory under src/platform/

PLATFORM_atari      := atari
PLATFORM_apple2     := apple2
PLATFORM_apple2enh  := apple2
PLATFORM_c64        := c64
PLATFORM_coco       := coco
PLATFORM_msdos      := msdos
PLATFORM_linux      := linux

# Get platform for current target
PLATFORM := $(PLATFORM_$(TARGET))

# Compiler family mapping
# Determines which compiler configuration to use

COMPILER_FAMILY_atari      := cc65
COMPILER_FAMILY_apple2     := cc65
COMPILER_FAMILY_apple2enh  := cc65
COMPILER_FAMILY_c64        := cc65
COMPILER_FAMILY_coco       := cmoc
COMPILER_FAMILY_msdos      := wcc
COMPILER_FAMILY_linux      := gcc

# Get compiler family for current target
COMPILER_FAMILY := $(COMPILER_FAMILY_$(TARGET))

# Target-specific flags

# Atari
TARGET_CFLAGS_atari     :=
TARGET_ASFLAGS_atari    :=

# Apple II
TARGET_CFLAGS_apple2    :=
TARGET_ASFLAGS_apple2   :=

# Apple II Enhanced
TARGET_CFLAGS_apple2enh :=
TARGET_ASFLAGS_apple2enh :=

# Commodore 64
TARGET_CFLAGS_c64       :=
TARGET_ASFLAGS_c64      :=

# CoCo
TARGET_CFLAGS_coco      :=
TARGET_ASFLAGS_coco     :=

# MS-DOS
TARGET_CFLAGS_msdos     :=
TARGET_ASFLAGS_msdos    :=

# Linux (native)
TARGET_CFLAGS_linux     :=
TARGET_ASFLAGS_linux    :=

# Get flags for current target
TARGET_CFLAGS  := $(TARGET_CFLAGS_$(TARGET))
TARGET_ASFLAGS := $(TARGET_ASFLAGS_$(TARGET))
