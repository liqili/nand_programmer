/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#ifndef _NAND_BAD_BLOCK_H_
#define _NAND_BAD_BLOCK_H_

#include <stdint.h>
#include <stdbool.h>

/* One bit per block rather than a list of page numbers. A list of 20 entries
 * only just covered the factory allowance of a 1Gbit part and overflowed on
 * anything larger or worn; a bitmap covers 8192 blocks in 1KB, which reaches
 * an 8Gbit device with 128KB blocks.
 */
#define NAND_BBT_MAX_BLOCKS 8192

/* Bind the table to the geometry of the selected chip. Returns -1 when the
 * chip has more blocks than the bitmap can hold, in which case bad block
 * handling must not be relied upon.
 */
int nand_bad_block_table_init(uint32_t pages_per_block, uint32_t block_count);

/* page may be any page of the block; the whole block is marked. */
int nand_bad_block_table_add(uint32_t page);

/* True when the block containing this page is marked bad. */
bool nand_bad_block_table_lookup(uint32_t page);

uint32_t nand_bad_block_table_count(void);

/* Walk the marked blocks, yielding the first page of each. Set *iter to 0
 * before the first call; returns false once there are no more.
 */
bool nand_bad_block_table_iter(uint32_t *iter, uint32_t *page);

#endif
