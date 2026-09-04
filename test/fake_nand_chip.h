/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *
 *  Behavioural model of a Skyhigh/Cypress S34ML01G1 SLC NAND flash on the
 *  FSMC NAND bus, per datasheet 002-00676 Rev *W.
 *
 *  The device geometry and the address cycle map are fixed here because they
 *  are properties of the silicon. Whatever the driver was configured with has
 *  to agree with them, so a wrong chip database entry shows up as a protocol
 *  violation rather than as plausible looking data.
 */

#ifndef _FAKE_NAND_CHIP_H_
#define _FAKE_NAND_CHIP_H_

#include <stdint.h>

/* Datasheet section 1.5, Array Organization, x8 */
#define NAND_PAGE_MAIN 2048
#define NAND_PAGE_SPARE 64
#define NAND_PAGE_TOTAL (NAND_PAGE_MAIN + NAND_PAGE_SPARE)
#define NAND_PAGES_PER_BLOCK 64
#define NAND_BLOCK_MAIN (NAND_PAGE_MAIN * NAND_PAGES_PER_BLOCK)
#define NAND_TOTAL_BLOCKS 1024
#define NAND_TOTAL_SIZE ((uint32_t)NAND_BLOCK_MAIN * NAND_TOTAL_BLOCKS)

/* Datasheet Table 4, Address Cycle Map, 1 Gb device, x8:
 * 1st/2nd column, 3rd/4th row. Anything else is a violation.
 */
#define NAND_COL_CYCLES 2
#define NAND_ROW_CYCLES 2

/* Datasheet section 3.16: 01h, F1h, 00h, 1Dh */
#define NAND_ID_1 0x01
#define NAND_ID_2 0xF1
#define NAND_ID_3 0x00
#define NAND_ID_4 0x1D

/* Only a few blocks are backed by memory, row addresses wrap into them */
#define NAND_MODELLED_BLOCKS 4
#define NAND_MODELLED_PAGES (NAND_MODELLED_BLOCKS * NAND_PAGES_PER_BLOCK)

typedef struct
{
    uint32_t cmd_writes;
    uint32_t addr_writes;
    uint32_t data_reads;
    uint32_t data_writes;

    uint8_t last_cmd;
    uint32_t last_col;        /* column address of the last setup */
    uint32_t last_row;        /* row (page) address of the last setup */
    uint8_t last_addr_cycles; /* cycles received before the confirm command */

    uint32_t reads;
    uint32_t programs;
    uint32_t erases;

    const char *violation;
} fake_nand_trace_t;

extern fake_nand_trace_t fake_nand_trace;

void fake_nand_reset(uint32_t busy_reads);
void fake_nand_fill(uint32_t page, uint32_t offset, const uint8_t *d,
    uint32_t len);
uint8_t fake_nand_peek(uint32_t page, uint32_t offset);

#endif /* _FAKE_NAND_CHIP_H_ */
