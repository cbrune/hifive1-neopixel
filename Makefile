#
# Copyright 2018 Curt Brune <curt@brune.net>
# All rights reserved.
#

V ?= 0
Q = @
ifneq ($V,0)
	Q = 
endif

include config.mk

srcdir := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
srcdir := $(srcdir:/=)

DEFAULT_BOARD := freedom-e300-hifive1
DEFAULT_PROGRAM := neopixel

BOARD ?= $(DEFAULT_BOARD)
PROGRAM ?= $(DEFAULT_PROGRAM)
PROGRAM_DIR = $(srcdir)/$(PROGRAM)
PROGRAM_ELF = $(PROGRAM_DIR)/$(PROGRAM)

SDKARGS	= BOARD=$(BOARD) PROGRAM=$(PROGRAM) PROGRAM_DIR=$(PROGRAM_DIR) \
		PROGRAM_ELF=$(PROGRAM_ELF)

SDK_TARGETS = software_clean software dasm upload

#############################################################
# Prints help message
#############################################################

.PHONY: help
help :
	@echo "  Neopixel Demo"
	@echo "  Makefile targets:"
	@echo ""
	@echo " software"
	@echo "    Build the neopixel demo"
	@echo ""
	@echo " software_clean"
	@echo "    Clean the neopixel demo build files"
	@echo ""
	@echo " upload"
	@echo "    Launch OpenOCD to flash the demo to the on-board flash"
	@echo ""
	@echo " dasm"
	@echo "     Generates the dissassembly output of 'objdump -D' to stdout."
	@echo ""

.PHONY: all
all: 
	echo "All target is not defined"
	echo "Run `make help' for details"

#############################################################
# All the SDK targets share the same rule
#############################################################

.PHONY: $(SDK_TARGETS)
$(SDK_TARGETS):
	$(Q) $(MAKE) -C $(SDKDIR) $(SDKARGS) $@

#############################################
# This Section is for generating cscope files
#############################################

CSCOPE_IDIRS = $(SDKDIR)/bsp/include $(SDKDIR)/bsp/drivers $(SDKDIR)/bsp/env $(SDKDIR)/bsp/env/$(BOARD)
CSCOPE_ARGS = $(foreach x, $(CSCOPE_IDIRS), -I$(x))
CSCOPE_PREFIX = new
CSCOPE_FILES = $(CSCOPE_PREFIX)-cscope.files
CSCOPE_OUT = $(CSCOPE_PREFIX)-cscope.out
CSCOPE_TMP := $(shell echo /tmp/cscope_files_$$$$.txt)
.PHONY: cscope
cscope:
	$(Q) rm -f $(CSCOPE_FILES)
	$(Q) touch $(CSCOPE_FILES)
	$(Q) echo -n "Indexing cscope: $(srcdir) ..."
	$(Q) cscope-indexer -i $(CSCOPE_TMP) -l -r $(srcdir)
	$(Q) cat $(CSCOPE_TMP) >> $(CSCOPE_FILES)
	$(Q) cscope-indexer -i $(CSCOPE_TMP) -l -r $(SDKDIR)/bsp
	$(Q) cat $(CSCOPE_TMP) >> $(CSCOPE_FILES)
	$(Q) rm -f $(CSCOPE_TMP)
	$(Q) cscope -q -b -k $(CSCOPE_ARGS) -i $(CSCOPE_FILES) -f $(CSCOPE_OUT)
	$(Q) rm -f $(CSCOPE_FILES) cscope.*
	$(Q) mv $(CSCOPE_OUT) cscope.out
	$(Q) mv $(CSCOPE_OUT).in cscope.out.in
	$(Q) mv $(CSCOPE_OUT).po cscope.out.po
	$(Q) echo "  done."

.PHONY: clean
clean: software_clean

