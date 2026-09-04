/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *
 *  Host tests for the bad block table. It has no dependency on the SPL or on
 *  any hardware, so it compiles and runs natively as it stands.
 */

#include "nand_bad_block.h"

#include <stdio.h>
#include <string.h>

static int failures;

static void expect(int cond, const char *what)
{
    if (cond)
    {
        printf("  ok   %s\n", what);
    }
    else
    {
        printf("  FAIL %s\n", what);
        failures++;
    }
}

/* A 1Gbit part: 128KB blocks of 64 pages, 1024 blocks. */
#define PPB 64
#define BLOCKS 1024

static void test_basics(void)
{
    printf("-- basics --\n");

    expect(!nand_bad_block_table_init(PPB, BLOCKS), "init with a 1Gbit geometry");
    expect(nand_bad_block_table_count() == 0, "starts empty");
    expect(!nand_bad_block_table_lookup(0), "nothing marked yet");

    expect(!nand_bad_block_table_add(7 * PPB), "mark block 7");
    expect(nand_bad_block_table_count() == 1, "one block marked");
    expect(nand_bad_block_table_lookup(7 * PPB), "block 7 looks up bad");
    expect(!nand_bad_block_table_lookup(6 * PPB), "its neighbours are unaffected");
    expect(!nand_bad_block_table_lookup(8 * PPB), "its neighbours are unaffected");

    /* The table is keyed by block, so any page of a bad block answers true.
     * The write path needs this: a block only becomes bad part way through.
     */
    expect(nand_bad_block_table_lookup(7 * PPB + 1), "a mid-block page looks up bad");
    expect(nand_bad_block_table_lookup(7 * PPB + PPB - 1), "the last page too");

    expect(!nand_bad_block_table_add(7 * PPB + 30),
        "marking the same block through another page succeeds");
    expect(nand_bad_block_table_count() == 1, "but does not double count");
}

static void test_capacity(void)
{
    printf("-- capacity --\n");

    /* The old table held 20 entries, which only just covered the factory
     * allowance of a 1Gbit part and overflowed on anything larger or worn.
     */
    uint32_t i, marked = 0;

    nand_bad_block_table_init(PPB, BLOCKS);

    for (i = 0; i < 200; i++)
    {
        if (!nand_bad_block_table_add(i * 3 * PPB))
            marked++;
    }

    expect(marked == 200, "200 bad blocks recorded, well past the old limit of 20");
    expect(nand_bad_block_table_count() == 200, "count agrees");
    expect(nand_bad_block_table_lookup(150 * 3 * PPB), "a late entry is still found");

    expect(nand_bad_block_table_add(BLOCKS * PPB),
        "a page past the end of the chip is refused");
    expect(nand_bad_block_table_count() == 200, "and does not change the count");
}

static void test_geometry_limits(void)
{
    printf("-- geometry limits --\n");

    expect(nand_bad_block_table_init(PPB, NAND_BBT_MAX_BLOCKS + 1),
        "a chip with more blocks than the bitmap holds is rejected");
    expect(!nand_bad_block_table_lookup(0),
        "a rejected geometry answers false rather than indexing out of bounds");
    expect(nand_bad_block_table_add(0), "and refuses to record anything");

    expect(nand_bad_block_table_init(0, BLOCKS), "zero pages per block is rejected");
    expect(nand_bad_block_table_init(PPB, 0), "zero blocks is rejected");

    expect(!nand_bad_block_table_init(64, NAND_BBT_MAX_BLOCKS),
        "the largest supported geometry is accepted");
    expect(!nand_bad_block_table_add((NAND_BBT_MAX_BLOCKS - 1) * 64),
        "the last block can be marked");
    expect(nand_bad_block_table_lookup((NAND_BBT_MAX_BLOCKS - 1) * 64 + 63),
        "and looks up through its last page");
}

static void test_iteration(void)
{
    printf("-- iteration --\n");

    const uint32_t want[] = { 3, 17, 18, 900 };
    uint32_t iter = 0, page, seen = 0;
    int ordered = 1, correct = 1;
    unsigned i;

    nand_bad_block_table_init(PPB, BLOCKS);
    for (i = 0; i < sizeof(want) / sizeof(want[0]); i++)
        nand_bad_block_table_add(want[i] * PPB);

    while (nand_bad_block_table_iter(&iter, &page))
    {
        if (seen >= sizeof(want) / sizeof(want[0]))
        {
            correct = 0;
            break;
        }
        if (page != want[seen] * PPB)
            correct = 0;
        seen++;
    }

    expect(seen == 4, "the iterator yields every marked block");
    expect(correct, "it yields the first page of each, in block order");
    expect(ordered, "ascending");

    iter = 0;
    nand_bad_block_table_init(PPB, BLOCKS);
    expect(!nand_bad_block_table_iter(&iter, &page), "an empty table yields nothing");
}

/* The reason any of this matters: a block that fails to program mid-way must
 * be skipped whole on the next attempt, not re-entered at the failing page.
 */
static void test_write_skip_semantics(void)
{
    printf("-- write path semantics --\n");

    uint32_t page;
    int advanced = 0;

    nand_bad_block_table_init(PPB, BLOCKS);
    nand_bad_block_table_add(10 * PPB + 37);    /* failed part way through */

    expect(nand_bad_block_table_lookup(10 * PPB),
        "the whole block is retired, not just the failing page");

    /* Mirror the firmware's skip loop: land on the block start and step over
     * it a whole block at a time.
     */
    page = 10 * PPB;
    while (nand_bad_block_table_lookup(page))
    {
        page += PPB;
        advanced++;
    }

    expect(advanced == 1, "the skip loop steps over it exactly once");
    expect(page == 11 * PPB, "and lands on the next block boundary");
}

int main(void)
{
    printf("=== NANDO bad block table tests ===\n");

    test_basics();
    test_capacity();
    test_geometry_limits();
    test_iteration();
    test_write_skip_semantics();

    printf("=== %s (%d failure%s) ===\n", failures ? "FAIL" : "PASS", failures,
        failures == 1 ? "" : "s");

    return failures ? 1 : 0;
}
