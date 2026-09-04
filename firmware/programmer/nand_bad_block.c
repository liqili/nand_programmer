/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#include "nand_bad_block.h"
#include <string.h>

static uint8_t nand_bbt[NAND_BBT_MAX_BLOCKS / 8];
static uint32_t nand_bbt_pages_per_block;
static uint32_t nand_bbt_block_count;
static uint32_t nand_bbt_marked;

int nand_bad_block_table_init(uint32_t pages_per_block, uint32_t block_count)
{
    memset(nand_bbt, 0, sizeof(nand_bbt));
    nand_bbt_marked = 0;
    nand_bbt_pages_per_block = pages_per_block;
    nand_bbt_block_count = block_count;

    if (!pages_per_block || !block_count)
    {
        nand_bbt_block_count = 0;
        return -1;
    }

    if (block_count > NAND_BBT_MAX_BLOCKS)
    {
        nand_bbt_block_count = 0;
        return -1;
    }

    return 0;
}

static uint32_t nand_bbt_block_of(uint32_t page)
{
    if (!nand_bbt_pages_per_block)
        return NAND_BBT_MAX_BLOCKS;

    return page / nand_bbt_pages_per_block;
}

int nand_bad_block_table_add(uint32_t page)
{
    uint32_t block = nand_bbt_block_of(page);

    if (block >= nand_bbt_block_count)
        return -1;

    if (!(nand_bbt[block / 8] & (1u << (block % 8))))
    {
        nand_bbt[block / 8] |= (uint8_t)(1u << (block % 8));
        nand_bbt_marked++;
    }

    return 0;
}

bool nand_bad_block_table_lookup(uint32_t page)
{
    uint32_t block = nand_bbt_block_of(page);

    if (block >= nand_bbt_block_count)
        return false;

    return !!(nand_bbt[block / 8] & (1u << (block % 8)));
}

uint32_t nand_bad_block_table_count(void)
{
    return nand_bbt_marked;
}

bool nand_bad_block_table_iter(uint32_t *iter, uint32_t *page)
{
    uint32_t block;

    for (block = *iter; block < nand_bbt_block_count; block++)
    {
        if (nand_bbt[block / 8] & (1u << (block % 8)))
        {
            *page = block * nand_bbt_pages_per_block;
            *iter = block + 1;
            return true;
        }
    }

    *iter = nand_bbt_block_count;

    return false;
}
