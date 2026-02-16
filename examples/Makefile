# examples/Makefile
#
# Build system for fujinet-nio-lib examples
#
# Usage:
#   make                  - Build all examples for current target
#   make TARGET=atari     - Build all examples for Atari
#   make TARGET=linux     - Build all examples for Linux
#   make http_get         - Build specific example
#   make clean            - Clean build artifacts

# Default target
TARGET ?= linux

# Library directory (parent of examples)
LIB_DIR := ..

# Include paths
INCDIR := $(LIB_DIR)/include

# Library to link
ifeq ($(TARGET),linux)
    LIB_FILE := $(LIB_DIR)/build/fujinet-nio-linux.a
    CC := gcc
    CFLAGS := -Wall -Wextra -O2 -std=c99
    LDFLAGS := 
    EXEEXT :=
else ifeq ($(TARGET),atari)
    LIB_FILE := $(LIB_DIR)/build/fujinet-nio-atari.lib
    CC := cl65
    CFLAGS := -t atari -Osir -O
    LDFLAGS := 
    EXEEXT := .xex
else
    $(error Unsupported target: $(TARGET))
endif

# Output directory
OUTDIR := bin/$(TARGET)

# Network examples
NETWORK_EXAMPLES := http_get

# All examples
ALL_EXAMPLES := $(NETWORK_EXAMPLES)

# Build targets
.PHONY: all clean $(ALL_EXAMPLES) lib

all: lib $(ALL_EXAMPLES)

# Build the library first
lib:
	@echo "Building library for $(TARGET)..."
	$(MAKE) -C $(LIB_DIR) $(TARGET)

# Build individual examples
define BUILD_EXAMPLE
$(1): lib | $(OUTDIR)
	@echo "Building $(1) for $(TARGET)..."
	$(CC) $(CFLAGS) -I$(INCDIR) network/$(1).c $(LIB_FILE) $(LDFLAGS) -o $(OUTDIR)/$(1)$(EXEEXT)
endef

$(foreach example,$(ALL_EXAMPLES),$(eval $(call BUILD_EXAMPLE,$(example))))

# Create output directory
$(OUTDIR):
	@mkdir -p $@

# Clean
clean:
	rm -rf bin/
