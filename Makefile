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
BOARD ?= $(DEFAULT_BOARD)
PROGRAM ?= $(DEFAULT_PROGRAM)
LINK_TARGET ?= flash

#############################################################
# BSP Loading
#############################################################

# Finds the directory in which this BSP is located, ensuring that there is
# exactly one.
board_dir := $(wildcard $(SDKDIR)/bsp/env/$(BOARD))
ifeq ($(words $(board_dir)),0)
$(error Unable to find BSP for $(BOARD), expected to find either "bsp/$(BOARD)" or "bsp-addons/$(BOARD)")
endif
ifneq ($(words $(board_dir)),1)
$(error Found multiple BSPs for $(BOARD): "$(board_dir)")
endif

# There must be a settings makefile fragment in the BSP's board directory.
ifeq ($(wildcard $(board_dir)/settings.mk),)
$(error Unable to find BSP for $(BOARD), expected to find $(board_dir)/settings.mk)
endif
include $(board_dir)/settings.mk

ifeq ($(RISCV_ARCH),)
$(error $(board_dir)/board.mk must set RISCV_ARCH, the RISC-V ISA string to target)
endif

ifeq ($(RISCV_ABI),)
$(error $(board_dir)/board.mk must set RISCV_ABI, the ABI to target)
endif

# Determines the XLEN from the toolchain tuple
ifeq ($(patsubst rv32%,rv32,$(RISCV_ARCH)),rv32)
RISCV_XLEN := 32
else ifeq ($(patsubst rv64%,rv64,$(RISCV_ARCH)),rv64)
RISCV_XLEN := 64
else
$(error Unable to determine XLEN from $(RISCV_ARCH))
endif

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

CROSS_PREFIX  ?= riscv64-unknown-elf
RISCV_PATH    ?= $(SDKDIR)/work/build/riscv-gnu-toolchain/$(CROSS_PREFIX)/prefix
RISCV_GCC     := $(abspath $(RISCV_PATH)/bin/$(CROSS_PREFIX)-gcc)
RISCV_GXX     := $(abspath $(RISCV_PATH)/bin/$(CROSS_PREFIX)-g++)
RISCV_OBJDUMP := $(abspath $(RISCV_PATH)/bin/$(CROSS_PREFIX)-objdump)
RISCV_GDB     := $(abspath $(RISCV_PATH)/bin/$(CROSS_PREFIX)-gdb)
RISCV_AR      := $(abspath $(RISCV_PATH)/bin/$(CROSS_PREFIX)-ar)

#############################################################
# This Section is for Software Compilation
#############################################################
PROGRAM_DIR = $(srcdir)/$(PROGRAM)
PROGRAM_ELF = $(PROGRAM_DIR)/$(PROGRAM)

.PHONY: software_clean
software_clean:
	$(Q) $(MAKE) -C $(PROGRAM_DIR) clean

.PHONY: software
software: software_clean
	$(Q) $(MAKE) -C $(PROGRAM_DIR) CC=$(RISCV_GCC) RISCV_ARCH=$(RISCV_ARCH) RISCV_ABI=$(RISCV_ABI) AR=$(RISCV_AR) BOARD=$(BOARD) LINK_TARGET=$(LINK_TARGET)

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
