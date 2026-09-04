# Copyright (C) 2020 NANDO authors
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3.
#
# Host tests for the flash drivers. Each driver is compiled natively against a
# behavioural model of the flash chip, no target hardware needed.
#
#   make        build and run everything
#   make nor    parallel NOR driver only
#   make nand   S34ML01G1 NAND driver and database entry only
#   make bbt    bad block table only
#   make ecc    ECC engine used by the host application only
#   make clean

CC ?= cc
CXX ?= c++

# spl_stub.h is force included the same way the firmware build force includes
# stm32f10x_conf.h, so the SPL stand-ins are visible everywhere.
FW = ../firmware/programmer

BASE_CFLAGS = -std=gnu99 -Wall -Wextra -O1 -g -I. -I$(FW) -include spl_stub.h

NOR_CFLAGS = $(BASE_CFLAGS) -DPNOR_HOST_TEST
NAND_CFLAGS = $(BASE_CFLAGS) -DNAND_HOST_TEST

NOR_SRCS = test_parallel_nor.c fake_nor_chip.c spl_stub.c $(FW)/parallel_nor_flash.c
NAND_SRCS = test_fsmc_nand.c fake_nand_chip.c spl_stub.c $(FW)/fsmc_nand.c

# The bad block table is plain logic with no SPL dependency, so it needs none
# of the stand-ins the driver tests do.
BBT_CFLAGS = -std=gnu99 -Wall -Wextra -O1 -g -I$(FW)
BBT_SRCS = test_bad_block.c $(FW)/nand_bad_block.c

# The ECC engine belongs to the Qt application but is deliberately free of Qt,
# so it compiles and runs natively here.
ECC = ../qt/ecc
ECC_CFLAGS = -std=c++14 -Wall -Wextra -O2 -g -I. -I$(ECC) -I../sim
ECC_SRCS = test_ecc.cpp $(ECC)/gf.cpp $(ECC)/bch_codec.cpp \
	$(ECC)/hamming_codec.cpp $(ECC)/rs_codec.cpp $(ECC)/ecc_scheme.cpp \
	$(ECC)/ecc_engine.cpp $(ECC)/ecc_stream.cpp $(ECC)/image_probe.cpp

COMMON_DEPS = spl_stub.h spl_stub.c Makefile

.PHONY: all run nor nand bbt ecc clean

all: run

pnor_test: $(NOR_SRCS) fake_nor_chip.h $(COMMON_DEPS)
	$(CC) $(NOR_CFLAGS) $(NOR_SRCS) -o $@

nand_test: $(NAND_SRCS) fake_nand_chip.h $(COMMON_DEPS)
	$(CC) $(NAND_CFLAGS) $(NAND_SRCS) -o $@

bbt_test: $(BBT_SRCS) $(FW)/nand_bad_block.h Makefile
	$(CC) $(BBT_CFLAGS) $(BBT_SRCS) -o $@

ecc_test: $(ECC_SRCS) ecc_golden.h ../sim/ecc_page.h Makefile
	$(CXX) $(ECC_CFLAGS) $(ECC_SRCS) -o $@

nor: pnor_test
	@./pnor_test

nand: nand_test
	@./nand_test

bbt: bbt_test
	@./bbt_test

ecc: ecc_test
	@./ecc_test

run: nor nand bbt ecc

clean:
	rm -f pnor_test nand_test bbt_test ecc_test
	rm -rf pnor_test.dSYM nand_test.dSYM bbt_test.dSYM ecc_test.dSYM
