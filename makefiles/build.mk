# build.mk - Core Build Logic
#
# This makefile handles the actual compilation for a specific target.
# It is included by the top-level Makefile.

# Require TARGET to be set
ifeq ($(TARGET),)
    $(error TARGET must be specified)
endif

# Include platform and compiler configuration
-include makefiles/targets.mk
-include makefiles/compiler.mk

# Directory structure
SRCDIR      := src
BUILDDIR    := build
OBJDIR      := obj
DISTDIR     := dist
INCDIR      := include

# Platform-specific source directory
PLATFORM_DIR := $(SRCDIR)/platform/$(PLATFORM)

# Output files
LIBRARY     := $(BUILDDIR)/$(PROGRAM)-$(TARGET).lib

# Common sources (shared across all platforms)
COMMON_SRCS := $(SRCDIR)/common/fn_slip.c \
               $(SRCDIR)/common/fn_packet.c \
               $(SRCDIR)/common/fn_network.c

# Platform-specific sources
PLATFORM_SRCS := $(wildcard $(PLATFORM_DIR)/*.c)
PLATFORM_ASMS := $(wildcard $(PLATFORM_DIR)/*.s)

# All sources
SOURCES := $(COMMON_SRCS) $(PLATFORM_SRCS)

# Object files
OBJECTS_C   := $(SOURCES:.c=.o)
OBJECTS_ASM := $(PLATFORM_ASMS:.s=.o)
OBJECTS     := $(patsubst $(SRCDIR)/%,$(OBJDIR)/$(TARGET)/%,$(OBJECTS_C)) \
               $(patsubst $(SRCDIR)/%,$(OBJDIR)/$(TARGET)/%,$(OBJECTS_ASM))

# Dependency files
DEPENDS := $(OBJECTS:.o=.d)

# Include paths
INCLUDES := -I$(INCDIR)

# Compiler flags
CFLAGS   += $(INCLUDES) $(TARGET_CFLAGS)
ASFLAGS  += $(INCLUDES) $(TARGET_ASFLAGS)

# Phony targets
.PHONY: lib clean-obj

# Main build target (named 'lib' to avoid conflict with build/ directory)
lib: $(LIBRARY)

# Include dependency files
-include $(DEPENDS)

# Create directories
$(OBJDIR)/$(TARGET):
	@mkdir -p $@

$(OBJDIR)/$(TARGET)/common:
	@mkdir -p $@

$(OBJDIR)/$(TARGET)/platform/$(PLATFORM):
	@mkdir -p $@

$(BUILDDIR):
	@mkdir -p $@

# Compile C files
$(OBJDIR)/$(TARGET)/common/%.o: $(SRCDIR)/common/%.c | $(OBJDIR)/$(TARGET)/common
	@echo "  CC $<"
	$(CC) -t $(TARGET) -c $(CFLAGS) --create-dep $(@:.o=.d) -o $@ $<

$(OBJDIR)/$(TARGET)/platform/$(PLATFORM)/%.o: $(SRCDIR)/platform/$(PLATFORM)/%.c | $(OBJDIR)/$(TARGET)/platform/$(PLATFORM)
	@echo "  CC $<"
	$(CC) -t $(TARGET) -c $(CFLAGS) --create-dep $(@:.o=.d) -o $@ $<

# Assemble ASM files
$(OBJDIR)/$(TARGET)/platform/$(PLATFORM)/%.o: $(SRCDIR)/platform/$(PLATFORM)/%.s | $(OBJDIR)/$(TARGET)/platform/$(PLATFORM)
	@echo "  AS $<"
	$(CC) -t $(TARGET) -c $(ASFLAGS) -o $@ $<

# Create library
$(LIBRARY): $(OBJECTS) | $(BUILDDIR)
	@echo "  AR $@"
	$(AR) a $@ $(OBJECTS)
	@echo "  Created $@"

# Clean object files for this target
clean-obj:
	rm -rf $(OBJDIR)/$(TARGET)
