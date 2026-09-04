# Copyright (C) 2020 NANDO authors
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3.
#
# Builds the simulation firmware run by Renode. The chip configuration is
# generated from the chip database CSV, so the simulation exercises the
# shipped database row.
#
#   make            build sim_fw.elf
#   make clean

CROSS ?= arm-none-eabi-
CC = $(CROSS)gcc
SIZE = $(CROSS)size

SRC_DIR = ../firmware/programmer/
LIB_DIR = ../firmware/libs/
SPL_PATH = $(LIB_DIR)spl/
CSV = ../qt/nando_parallel_chip_db.csv
CHIP ?= S34ML01G1

INCLUDES = -include$(SPL_PATH)stm32f10x_conf.h
INCLUDES += -I../firmware/
INCLUDES += -I$(SPL_PATH)CMSIS/CM3/CoreSupport/
INCLUDES += -I$(SPL_PATH)CMSIS/CM3/DeviceSupport/ST/STM32F10x/
INCLUDES += -I$(SPL_PATH)
INCLUDES += -I$(SPL_PATH)STM32F10x_StdPeriph_Driver/inc
INCLUDES += -I$(SPL_PATH)STM32_USB-FS-Device_Driver/inc
INCLUDES += -I$(SRC_DIR)
INCLUDES += -I.

CFLAGS = -mcpu=cortex-m3 -mthumb -DSTM32F10X_HD -DUSE_STDPERIPH_DRIVER \
  -Wall -Wextra -Os -ffunction-sections -fdata-sections --specs=nosys.specs

# nosys.specs comes from CFLAGS, gcc rejects it being given twice
LDFLAGS = -mcpu=cortex-m3 -mthumb -Wl,--gc-sections -Wl,-Map=sim_fw.map

SRCS = sim_main.c \
  $(SRC_DIR)fsmc_nand.c \
  $(SRC_DIR)uart.c \
  $(SRC_DIR)syscalls.c \
  $(SPL_PATH)CMSIS/CM3/DeviceSupport/ST/STM32F10x/system_stm32f10x.c

TARGET = sim_fw.elf
GENERATED = sim_chip_conf.h

.PHONY: all clean

all: $(TARGET)

$(GENERATED): gen_conf.py $(CSV)
	python3 gen_conf.py $(CSV) $(CHIP) $@

$(TARGET): $(SRCS) $(GENERATED)
	$(CC) $(LDFLAGS) $(CFLAGS) $(INCLUDES) -o $@ $(SRCS) \
	  -L$(SPL_PATH) -lstm32f10x -T$(SRC_DIR)stm32_flash_1.ld
	$(SIZE) $@

clean:
	rm -f $(TARGET) sim_fw.map $(GENERATED)
