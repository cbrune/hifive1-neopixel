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
wrkdir := $(srcdir)/work

DEFAULT_BOARD := freedom-e300-hifive1
DEFAULT_PROGRAM := neopixel

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
	@echo " upload"
	@echo "    Launch OpenOCD to flash the demo to the on-board flash"
	@echo ""
	@echo " run_debug"
	@echo "    Launch OpenOCD & GDB to load or debug "
	@echo "    running programs. Does not allow Ctrl-C to halt running programs."
	@echo ""
	@echo " run_openocd"
	@echo " run_gdb"
	@echo "     Launch OpenOCD or GDB seperately. Allows Ctrl-C to halt running"
	@echo "     programs."
	@echo ""
	@echo " dasm"
	@echo "     Generates the dissassembly output of 'objdump -D' to stdout."
	@echo ""
	@echo " For more information, visit dev.sifive.com"

target32 := riscv32-unknown-linux-gnu


.PHONY: all
all: 
	echo "All target is not defined"

#############################################################
# This section is for tool installation
#############################################################

toolchain_srcdir := $(SDKDIR)/riscv-gnu-toolchain
toolchain32_wrkdir := $(SDKDIR)/work/riscv32-gnu-toolchain
toolchain_dest := $(SDKDIR)/toolchain
PATH := $(toolchain_dest)/bin:$(PATH)

#############################################################
# This Section is for Software Compilation
#############################################################
BOARD ?= $(DEFAULT_BOARD)
PROGRAM ?= $(DEFAULT_PROGRAM)
PROGRAM_DIR = $(srcdir)/$(PROGRAM)
PROGRAM_ELF = $(PROGRAM_DIR)/$(PROGRAM)

.PHONY: software_clean
software_clean:
	$(Q) $(MAKE) -C $(PROGRAM_DIR) clean

.PHONY: software
software: software_clean
	$(Q) $(MAKE) -C $(PROGRAM_DIR)

dasm: software	
	$(Q) $(toolchain_dest)/bin/riscv32-unknown-elf-objdump -D $(PROGRAM_ELF)

#############################################################
# This Section is for uploading a program to SPI Flash
#############################################################
OPENOCD_UPLOAD = $(SDKDIR)/bsp/tools/openocd_upload.sh
OPENOCDCFG ?= $(SDKDIR)/bsp/env/$(BOARD)/openocd.cfg
upload:
	$(Q) $(OPENOCD_UPLOAD) $(PROGRAM_ELF) $(OPENOCDCFG)

#############################################################
# This Section is for launching the debugger
#############################################################

OPENOCD     = $(toolchain_dest)/bin/openocd
OPENOCDARGS += -f $(OPENOCDCFG)

GDB     = $(toolchain_dest)/bin/riscv32-unknown-elf-gdb
GDBCMDS += -ex "target extended-remote localhost:3333"
GDBARGS =

run_openocd:
	$(Q) $(OPENOCD) $(OPENOCDARGS)

run_gdb:
	$(Q) $(GDB) $(PROGRAM_DIR)/$(PROGRAM) $(GDBARGS)

run_debug:
	$(Q) $(OPENOCD) $(OPENOCDARGS) &
	$(Q) $(GDB) $(PROGRAM_DIR)/$(PROGRAM) $(GDBARGS) $(GDBCMDS)

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
	$(Q) $(HOME)/bin2/cb-cscope-indexer -i $(CSCOPE_TMP) -l -r $(srcdir)
	$(Q) cat $(CSCOPE_TMP) >> $(CSCOPE_FILES)
	$(Q) $(HOME)/bin2/cb-cscope-indexer -i $(CSCOPE_TMP) -l -r $(SDKDIR)/bsp
	$(Q) cat $(CSCOPE_TMP) >> $(CSCOPE_FILES)
	$(Q) rm -f $(CSCOPE_TMP)
	$(Q) /usr/bin/cscope -q -b -k $(CSCOPE_ARGS) -i $(CSCOPE_FILES) -f $(CSCOPE_OUT)
	$(Q) rm -f $(CSCOPE_FILES) cscope.*
	$(Q) mv $(CSCOPE_OUT) cscope.out
	$(Q) mv $(CSCOPE_OUT).in cscope.out.in
	$(Q) mv $(CSCOPE_OUT).po cscope.out.po
	$(Q) echo "  done."

.PHONY: clean
clean:
	rm -rf -- $(wrkdir) 
