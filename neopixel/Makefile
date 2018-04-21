#
# Copyright 2018 Curt Brune <curt@brune.net>
# All rights reserved.
#

TARGET = neopixel
C_SRCS += neopixel.c init.c
C_OBJS = $(C_SRCS:.c=.o)
CFLAGS += -O2 -fno-builtin-printf -DNO_INIT $(WARN_ERROR_FLAGS)

$(C_OBJS) : WARN_ERROR_FLAGS = -Wall -Werror
$(C_OBJS) : CFLAGS += -std=c99

BSP_BASE = $(SDKDIR)/bsp
include $(BSP_BASE)/env/common.mk
