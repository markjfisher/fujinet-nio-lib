# fujinet-nio-lib Makefile
#
# A clean, multi-platform 6502 library for FujiNet-NIO
#
# Usage:
#   make            - Build all targets
#   make atari      - Build for Atari
#   make apple2     - Build for Apple II
#   make coco       - Build for CoCo
#   make clean      - Remove build artifacts
#   make help       - Show this help

# Supported targets
TARGETS = atari apple2 apple2enh c64 coco msdos linux

# Output library name
PROGRAM := fujinet-nio

# Phony targets
.PHONY: all clean help $(TARGETS)

# Default target: build all
all:
	@echo "========================================="
	@echo "FujiNet-NIO Library Build"
	@echo "========================================="
	@for target in $(TARGETS); do \
		echo ""; \
		echo "Building for $$target..."; \
		echo "-----------------------------------------"; \
		$(MAKE) -f makefiles/build.mk TARGET=$$target PROGRAM=$(PROGRAM) lib || exit 1; \
	done
	@echo ""
	@echo "========================================="
	@echo "Build complete!"
	@echo "========================================="

# Individual platform targets
$(TARGETS):
	@echo "Building for $@..."
	$(MAKE) -f makefiles/build.mk TARGET=$@ PROGRAM=$(PROGRAM) lib

# Clean all targets
clean:
	@echo "Cleaning build artifacts..."
	rm -rf build/ obj/ dist/
	@echo "Done."

# Help
help:
	@echo "FujiNet-NIO Library Build System"
	@echo ""
	@echo "Usage:"
	@echo "  make            - Build all targets"
	@echo "  make <target>   - Build specific target (atari, apple2, coco, etc.)"
	@echo "  make clean      - Remove all build artifacts"
	@echo "  make help       - Show this help message"
	@echo ""
	@echo "Supported targets:"
	@echo "  atari       - Atari 8-bit (cc65)"
	@echo "  apple2      - Apple II (cc65)"
	@echo "  apple2enh   - Apple II enhanced (cc65)"
	@echo "  c64         - Commodore 64 (cc65)"
	@echo "  coco        - Tandy CoCo (CMOC)"
	@echo "  msdos       - MS-DOS (Watcom)"
	@echo ""
	@echo "Output:"
	@echo "  Libraries are placed in build/"
	@echo "  Object files are placed in obj/"
