# fujinet-nio-lib Makefile
#
# A clean, multi-platform 6502 library for FujiNet-NIO
#
# Usage:
#   make            - Build all targets
#   make atari      - Build for Atari
#   make bbc        - Build for BBC B
#   make linux      - build for linux/posix environment
#   make clean      - Remove build artifacts
#   make help       - Show this help

# Supported targets
# TARGETS = atari apple2 apple2enh bbc c64 coco msdos linux
TARGETS = atari bbc bbc-clib linux

# Output library name
override PROGRAM := fujinet-nio

# Phony targets
.PHONY: all clean help disk-bbc disk-bbc-clib test-bbc test-bbc-scripted test-bbc-real $(TARGETS)

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
	$(MAKE) -C examples clean-all
	rm -rf examples/ssd
	@echo "Done."

disk-bbc:
	$(MAKE) bbc
	$(MAKE) -C examples TARGET=bbc disk

disk-bbc-clib:
	$(MAKE) bbc-clib
	$(MAKE) -C examples TARGET=bbc-clib disk

test-bbc:
	@echo "========================================="
	@echo "BBC test scaffold"
	@echo "========================================="
	$(MAKE) bbc
	$(MAKE) -C examples TARGET=bbc all
	@echo ""
	@echo "Built BBC library, supported BBC examples, and BBC disk images."
	@echo "See integration-tests/beebium/README.md for scripted and real E2E setup."

test-bbc-scripted:
	@echo "Running scripted BBC test scaffold..."
	$(MAKE) bbc
	$(MAKE) -C examples TARGET=bbc all
	@if [ -d integration-tests/beebium ]; then \
		cd integration-tests/beebium && uv run pytest -q scripted; \
	else \
		echo "integration-tests/beebium not present"; \
		exit 1; \
	fi

test-bbc-real:
	@echo "Running real BBC test scaffold..."
	$(MAKE) bbc
	$(MAKE) -C examples TARGET=bbc all
	@if [ -d integration-tests/beebium ]; then \
		cd integration-tests/beebium && uv run pytest -q real; \
	else \
		echo "integration-tests/beebium not present"; \
		exit 1; \
	fi

# Help
help:
	@echo "FujiNet-NIO Library Build System"
	@echo ""
	@echo "Usage:"
	@echo "  make            - Build all targets"
	@echo "  make <target>   - Build specific target (atari, apple2, coco, etc.)"
	@echo "  make disk-bbc   - Build BBC example disk image(s)"
	@echo "  make disk-bbc-clib - Build BBC CLIB example disk image(s)"
	@echo "  make clean      - Remove all build artifacts"
	@echo "  make help       - Show this help message"
	@echo ""
	@echo "Supported targets:"
	@echo "  atari       - Atari 8-bit (cc65)"
	@echo "  bbc         - BBC Micro (cc65)"
	@echo "  bbc-clib    - BBC Micro CLIB ROM target (cc65)"
	@echo "  linux       - Linux/Posix (gcc)"
	@echo ""
	@echo "Environment:"
	@echo "  CC65_HOME   - cc65 checkout root (required for bbc/bbc-clib)"
	@echo ""
	@echo "Output:"
	@echo "  Libraries are placed in build/"
	@echo "  Object files are placed in obj/"
