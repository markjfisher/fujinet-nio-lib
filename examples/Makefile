# examples/Makefile
#
# Build system for fujinet-nio-lib examples
#
# Usage:
#   make                          - Build all examples for current target
#   make TARGET=atari             - Build all examples for Atari
#   make TARGET=linux             - Build all examples for Linux
#   make http_get                 - Build specific example
#   make tcp_get                  - Build specific example
#   make compile                  - Compile all examples (no linking)
#   make clean                    - Clean build artifacts
#   make clean-all                - Clean all targets
#
# TCP/TLS Example Configuration:
#   For Linux, defaults can be overridden via environment variables:
#     FN_TCP_HOST, FN_TCP_PORT, FN_TCP_TLS
#
#   For Atari (or to override defaults at compile time):
#     make TARGET=atari FN_TCP_HOST=\"192.168.1.100\" FN_TCP_PORT=\"443\" FN_TCP_TLS=1
#
# Build structure:
#   build/
#     linux/
#       network/
#         http_get.o
#         tcp_get.o
#       clock/
#         ...
#     atari/
#       network/
#         http_get.o
#         tcp_get.o
#   bin/
#     linux/
#       http_get
#       tcp_get
#     atari/
#       http_get.xex
#       tcp_get.xex

# Default target
TARGET ?= linux

# Library directory (parent of examples)
LIB_DIR := ..

# Include paths
INCDIR := $(LIB_DIR)/include

# Build directories
BUILD_DIR := build/$(TARGET)
BIN_DIR := bin/$(TARGET)

# ============================================================================
# TCP/TLS Example Configuration
# ============================================================================
# These can be overridden on the make command line for any target.
# For Atari, this is the primary way to configure the example.
# For Linux, environment variables can also be used at runtime.
#
# Example:
#   make TARGET=atari FN_TCP_HOST=example.com FN_TCP_PORT=443 FN_TCP_TLS=1

# Default values
FN_TCP_HOST ?= localhost
FN_TCP_PORT ?= 7777
FN_TCP_TLS ?= 0

# Pass to compiler as defines (with quotes for C strings)
TCP_DEFINES = -DFN_TCP_HOST=\"$(FN_TCP_HOST)\" \
              -DFN_TCP_PORT=\"$(FN_TCP_PORT)\" \
              -DFN_TCP_TLS=$(FN_TCP_TLS)

# ============================================================================
# Compiler configuration per target
# ============================================================================

ifeq ($(TARGET),linux)
    LIB_FILE := $(LIB_DIR)/build/fujinet-nio-linux.a
    CC := gcc
    CFLAGS := -Wall -Wextra -O2 -std=c99
    LDFLAGS := 
    EXEEXT :=
    # Compile command: gcc -c [flags] -o output.o input.c
    COMPILE = $(CC) $(CFLAGS) -c $< -o $@
    # Link command
    LINK = $(CC) $(CFLAGS) $< $(LIB_FILE) $(LDFLAGS) -o $@
else ifeq ($(TARGET),atari)
    LIB_FILE := $(LIB_DIR)/build/fujinet-nio-atari.lib
    CC := cl65
    CFLAGS := -Osir -O
    LDFLAGS := 
    EXEEXT := .xex
    # Compile command for cc65: cl65 -t atari -c [flags] -o output.o input.c
    COMPILE = $(CC) -t $(TARGET) -c $(CFLAGS) -o $@ $<
    # Link command for cc65
    LINK = $(CC) -t $(TARGET) $(CFLAGS) $< $(LIB_FILE) $(LDFLAGS) -o $@
else
    $(error Unsupported target: $(TARGET))
endif

# Common include path
CFLAGS += -I$(INCDIR)

# ============================================================================
# Example definitions
# ============================================================================

# Network examples (TCP/HTTP/TLS)
NETWORK_EXAMPLES := http_get tcp_get

# Clock examples
CLOCK_EXAMPLES := clock_test

# Disk examples (future)
# DISK_EXAMPLES := disk_info

# All examples (add categories as they are created)
ALL_EXAMPLES := $(NETWORK_EXAMPLES) $(CLOCK_EXAMPLES)

# ============================================================================
# Source and object file mappings
# ============================================================================

# Determine source directory for each example
# Examples are organized by category: network/, clock/, disk/, etc.
HTTP_GET_DIR := network
TCP_GET_DIR := network

# Generic mapping function - can be extended for new categories
# Format: EXAMPLE_DIR maps example name to its source directory
define GET_EXAMPLE_DIR
$(if $(filter $(1),$(NETWORK_EXAMPLES)),network,\
$(if $(filter $(1),$(CLOCK_EXAMPLES)),clock,\
$(if $(filter $(1),$(DISK_EXAMPLES)),disk,\
$(error Unknown example category for $(1)))))
endef

# Source files
SOURCES := $(foreach ex,$(ALL_EXAMPLES),$(call GET_EXAMPLE_DIR,$(ex))/$(ex).c)

# Object files in build directory (preserving category structure)
OBJECTS := $(foreach ex,$(ALL_EXAMPLES),$(BUILD_DIR)/$(call GET_EXAMPLE_DIR,$(ex))/$(ex).o)

# Output binaries
BINARIES := $(foreach ex,$(ALL_EXAMPLES),$(BIN_DIR)/$(ex)$(EXEEXT))

# ============================================================================
# Build targets
# ============================================================================

.PHONY: all clean clean-all lib $(ALL_EXAMPLES) info compile

# Default: build all examples (requires library)
all: $(BINARIES)

# Compile only (no linking) - useful for testing cc65 compatibility
compile: $(OBJECTS)
	@echo "Compilation complete for $(TARGET)"

# Build the library first (only if library file doesn't exist)
lib:
	@if [ ! -f $(LIB_FILE) ]; then \
		echo "Building library for $(TARGET)..."; \
		$(MAKE) -C $(LIB_DIR) $(TARGET); \
	else \
		echo "Library already exists: $(LIB_FILE)"; \
	fi

# Info target for debugging
info:
	@echo "Target: $(TARGET)"
	@echo "Examples: $(ALL_EXAMPLES)"
	@echo "Sources: $(SOURCES)"
	@echo "Objects: $(OBJECTS)"
	@echo "Binaries: $(BINARIES)"
	@echo "Build dir: $(BUILD_DIR)"
	@echo "Bin dir: $(BIN_DIR)"
	@echo "Library: $(LIB_FILE)"
	@echo "Library exists: $$(test -f $(LIB_FILE) && echo yes || echo no)"
	@echo ""
	@echo "TCP Config:"
	@echo "  FN_TCP_HOST: $(FN_TCP_HOST)"
	@echo "  FN_TCP_PORT: $(FN_TCP_PORT)"
	@echo "  FN_TCP_TLS: $(FN_TCP_TLS)"

# ============================================================================
# Pattern rules for building examples
# ============================================================================

# Create build directories for each category
$(BUILD_DIR)/network:
	@mkdir -p $@

$(BUILD_DIR)/clock:
	@mkdir -p $@

$(BUILD_DIR)/disk:
	@mkdir -p $@

# Create bin directory
$(BIN_DIR):
	@mkdir -p $@

# Compile source to object file
# Pattern: build/TARGET/category/example.o from category/example.c
# TCP examples get the TCP config defines
$(BUILD_DIR)/network/tcp_get.o: network/tcp_get.c | $(BUILD_DIR)/network
	@echo "Compiling $< for $(TARGET)..."
ifeq ($(CC),cl65)
	$(CC) -t $(TARGET) -c $(CFLAGS) $(TCP_DEFINES) -o $@ $<
else
	$(CC) $(CFLAGS) $(TCP_DEFINES) -c $< -o $@
endif

$(BUILD_DIR)/network/http_get.o: network/http_get.c | $(BUILD_DIR)/network
	@echo "Compiling $< for $(TARGET)..."
	$(COMPILE)

$(BUILD_DIR)/network/%.o: network/%.c | $(BUILD_DIR)/network
	@echo "Compiling $< for $(TARGET)..."
	$(COMPILE)

$(BUILD_DIR)/clock/%.o: clock/%.c | $(BUILD_DIR)/clock
	@echo "Compiling $< for $(TARGET)..."
	$(COMPILE)

$(BUILD_DIR)/disk/%.o: disk/%.c | $(BUILD_DIR)/disk
	@echo "Compiling $< for $(TARGET)..."
	$(COMPILE)

# Link object file to binary (requires library)
# Pattern: bin/TARGET/example from build/TARGET/category/example.o
$(BIN_DIR)/%$(EXEEXT): $(BUILD_DIR)/network/%.o | $(BIN_DIR) lib
	@echo "Linking $@ for $(TARGET)..."
	$(LINK)

$(BIN_DIR)/%$(EXEEXT): $(BUILD_DIR)/clock/%.o | $(BIN_DIR) lib
	@echo "Linking $@ for $(TARGET)..."
	$(LINK)

# ============================================================================
# Convenience targets for individual examples
# ============================================================================

# Each example target builds its binary
$(ALL_EXAMPLES): %: $(BIN_DIR)/%$(EXEEXT)

# ============================================================================
# Clean targets
# ============================================================================

# Clean current target only
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Clean all targets
clean-all:
	rm -rf build bin

# ============================================================================
# Dependencies
# ============================================================================

# Header dependencies - rebuild all if header changes
$(OBJECTS): $(INCDIR)/fujinet-nio.h
