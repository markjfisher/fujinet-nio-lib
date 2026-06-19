# cc65-env.mk - cc65 checkout paths
#
# BBC assembly sources include headers from the cc65 tree (e.g. oslib/os.inc).
# Set CC65_HOME in your environment before building bbc or bbc-clib targets:
#   export CC65_HOME=$HOME/dev/bbc/cc65

ifeq ($(strip $(CC65_HOME)),)
$(error CC65_HOME is not set. Export your cc65 checkout root, e.g.: export CC65_HOME=$$HOME/dev/bbc/cc65)
endif

CC65_HOME := $(abspath $(CC65_HOME))
