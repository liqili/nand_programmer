/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *
 *  Simulation firmware. Drives the real fsmc_nand.c HAL against the flash
 *  model mapped at the FSMC NAND bank 2 window, using the configuration
 *  generated from the chip database CSV.
 *
 *  Results go out over USART1, which Renode captures to a file. The chip
 *  model logs the bus traffic it sees from the other side.
 */

#include "fsmc_nand.h"
#include "uart.h"
#include "sim_chip_conf.h"
#include "ecc_page.h"

#include <stdio.h>
#include <string.h>

/* Mirror of fsmc_conf_t in fsmc_nand.c */
typedef struct __attribute__((__packed__))
{
    uint8_t setup_time;
    uint8_t wait_setup_time;
    uint8_t hold_setup_time;
    uint8_t hi_z_setup_time;
    uint8_t clr_setup_time;
    uint8_t ar_setup_time;
    uint8_t row_cycles;
    uint8_t col_cycles;
    uint8_t read1_cmd;
    uint8_t read2_cmd;
    uint8_t read_spare_cmd;
    uint8_t read_id_cmd;
    uint8_t reset_cmd;
    uint8_t write1_cmd;
    uint8_t write2_cmd;
    uint8_t erase1_cmd;
    uint8_t erase2_cmd;
    uint8_t status_cmd;
    uint8_t set_features_cmd;
    uint8_t enable_ecc_addr;
    uint8_t enable_ecc_value;
    uint8_t disable_ecc_value;
} sim_conf_t;

#define PAGES_PER_BLOCK (SIM_BLOCK_SIZE / SIM_PAGE_SIZE)
#define PAGE_FULL (SIM_PAGE_SIZE + SIM_SPARE_SIZE)

/* Block 1 is the scratch block, block 2 checks that erase stays inside its
 * own block, and block 1015 exercises the upper row address byte.
 */
/* The chip model refuses programs aimed at block 6, which is the only way to
 * reach the firmware's failure handling without a worn out device.
 */
#define FAIL_BLOCK 6
#define FAIL_PAGE (FAIL_BLOCK * PAGES_PER_BLOCK)

/* Fresh blocks for the later checks, so nothing they do depends on what an
 * earlier check left behind. NAND programming only clears bits, so a block
 * that already holds data cannot simply be written over.
 */
#define AFTER_FAIL_PAGE (8 * PAGES_PER_BLOCK)
#define ECC_BLOCK_PAGE (12 * PAGES_PER_BLOCK)

#define BLOCK1_PAGE (1 * PAGES_PER_BLOCK)
#define BLOCK2_PAGE (2 * PAGES_PER_BLOCK)
#define HIGH_PAGE (1015 * PAGES_PER_BLOCK)

static uint8_t buf[PAGE_FULL];
static uint8_t ref[PAGE_FULL];

static int failures;

static void expect(int cond, const char *what)
{
    if (cond)
    {
        printf("  ok   %s\r\n", what);
    }
    else
    {
        printf("  FAIL %s\r\n", what);
        failures++;
    }
}

static void fill(uint8_t *b, uint32_t n, uint8_t seed)
{
    uint32_t i;

    for (i = 0; i < n; i++)
        b[i] = (uint8_t)(i * 31 + seed);
}

static int same(const uint8_t *a, const uint8_t *b, uint32_t n)
{
    uint32_t i;

    for (i = 0; i < n; i++)
    {
        if (a[i] != b[i])
            return 0;
    }

    return 1;
}

static int all_erased(const uint8_t *b, uint32_t n)
{
    uint32_t i;

    for (i = 0; i < n; i++)
    {
        if (b[i] != 0xFF)
            return 0;
    }

    return 1;
}

/* write_page_async starts the program, the caller polls until the device
 * reports it is done, exactly as np_nand_handle_status() does.
 */
static uint32_t program_page(uint8_t *b, uint32_t page, uint32_t n)
{
    uint32_t i, status = FLASH_STATUS_BUSY;

    hal_fsmc.write_page_async(b, page, n);

    for (i = 0; i < 64; i++)
    {
        status = hal_fsmc.read_status();
        if (status != FLASH_STATUS_BUSY)
            break;
    }

    return status;
}

/* Program a pattern into one page and read it straight back */
static int roundtrip(uint32_t page, uint32_t n, uint8_t seed)
{
    fill(ref, n, seed);

    if (program_page(ref, page, n) != FLASH_STATUS_READY)
        return 0;

    memset(buf, 0, n);
    if (hal_fsmc.read_page(buf, page, n) != FLASH_STATUS_READY)
        return 0;

    return same(buf, ref, n);
}

static int page_is_erased(uint32_t page, uint32_t n)
{
    memset(buf, 0, n);
    hal_fsmc.read_page(buf, page, n);

    return all_erased(buf, n);
}

int main(void)
{
    sim_conf_t conf;
    chip_id_t id;
    uint32_t status;

    uart_init();

    printf("\r\n=== NANDO simulation: %s ===\r\n", SIM_CHIP_NAME);
    printf("config from chip database: row cycles %u, col cycles %u\r\n",
        SIM_ROW_CYCLES, SIM_COL_CYCLES);

    memset(&conf, 0, sizeof(conf));

    /* Nominal FSMC timings, the host application derives these from the
     * nanosecond values in the database.
     */
    conf.setup_time = 2;
    conf.wait_setup_time = 3;
    conf.hold_setup_time = 2;
    conf.hi_z_setup_time = 2;
    conf.clr_setup_time = 2;
    conf.ar_setup_time = 2;

    conf.row_cycles = SIM_ROW_CYCLES;
    conf.col_cycles = SIM_COL_CYCLES;
    conf.read1_cmd = SIM_READ1_CMD;
    conf.read2_cmd = SIM_READ2_CMD;
    conf.read_spare_cmd = SIM_READ_SPARE_CMD;
    conf.read_id_cmd = SIM_READ_ID_CMD;
    conf.reset_cmd = SIM_RESET_CMD;
    conf.write1_cmd = SIM_WRITE1_CMD;
    conf.write2_cmd = SIM_WRITE2_CMD;
    conf.erase1_cmd = SIM_ERASE1_CMD;
    conf.erase2_cmd = SIM_ERASE2_CMD;
    conf.status_cmd = SIM_STATUS_CMD;
    conf.set_features_cmd = SIM_SET_FEATURES_CMD;
    conf.enable_ecc_addr = SIM_ENABLE_ECC_ADDR;
    conf.enable_ecc_value = SIM_ENABLE_ECC_VALUE;
    conf.disable_ecc_value = SIM_DISABLE_ECC_VALUE;

    expect(hal_fsmc.init(&conf, sizeof(conf)) == 0, "hal init");

    /* Read ID */
    memset(&id, 0, sizeof(id));
    hal_fsmc.read_id(&id);
    printf("read ID: %02X %02X %02X %02X\r\n", id.maker_id, id.device_id,
        id.third_id, id.fourth_id);
    expect(id.maker_id == SIM_ID1 && id.device_id == SIM_ID2 &&
        id.third_id == SIM_ID3 && id.fourth_id == SIM_ID4,
        "ID matches database");

    /* ---- erase ---- */
    status = hal_fsmc.erase_block(BLOCK1_PAGE);
    printf("erase status: %lu\r\n", (unsigned long)status);
    expect(status == FLASH_STATUS_READY, "erase block");
    expect(page_is_erased(BLOCK1_PAGE, SIM_PAGE_SIZE), "erased page reads 0xFF");

    /* The whole block must come back erased, not just its first page */
    expect(page_is_erased(BLOCK1_PAGE + 1, 64) &&
        page_is_erased(BLOCK1_PAGE + PAGES_PER_BLOCK - 1, 64),
        "whole block erased");

    /* ---- single page round trip ---- */
    expect(roundtrip(BLOCK1_PAGE, SIM_PAGE_SIZE, 0x11), "program and read page");

    /* ---- several pages in one block, distinct data ---- */
    hal_fsmc.erase_block(BLOCK1_PAGE);
    expect(roundtrip(BLOCK1_PAGE + 0, 64, 0x21) &&
        roundtrip(BLOCK1_PAGE + 1, 64, 0x22) &&
        roundtrip(BLOCK1_PAGE + PAGES_PER_BLOCK - 1, 64, 0x23),
        "multiple pages in a block");

    /* Re-read the first of them: writing later pages must not disturb it */
    fill(ref, 64, 0x21);
    memset(buf, 0, 64);
    hal_fsmc.read_page(buf, BLOCK1_PAGE, 64);
    expect(same(buf, ref, 64), "earlier page undisturbed");

    /* ---- page plus spare area ---- */
    hal_fsmc.erase_block(BLOCK1_PAGE);
    expect(roundtrip(BLOCK1_PAGE, PAGE_FULL, 0x31), "program and read page + spare");

    /* This device has no spare read command, the caller falls back to a full
     * page read to reach the bad block marker.
     */
    expect(hal_fsmc.read_spare_data(buf, BLOCK1_PAGE, 0, 1) ==
        FLASH_STATUS_INVALID_CMD, "spare read reports invalid command");
    expect(hal_fsmc.is_bb_supported(), "bad block handling supported");

    /* ---- erase stays inside its own block ---- */
    hal_fsmc.erase_block(BLOCK2_PAGE);
    expect(roundtrip(BLOCK2_PAGE, 64, 0x41), "program page in next block");

    hal_fsmc.erase_block(BLOCK1_PAGE);
    fill(ref, 64, 0x41);
    memset(buf, 0, 64);
    hal_fsmc.read_page(buf, BLOCK2_PAGE, 64);
    expect(same(buf, ref, 64), "erase left the next block intact");
    expect(page_is_erased(BLOCK1_PAGE, 64), "erased block really erased");

    /* ---- high row address, exercises the upper row cycle ---- */
    printf("high row test at page %lu (row 0x%04lX)\r\n",
        (unsigned long)HIGH_PAGE, (unsigned long)HIGH_PAGE);
    hal_fsmc.erase_block(HIGH_PAGE);
    expect(roundtrip(HIGH_PAGE, 64, 0x51), "program and read at high row address");

    /* A high row must not alias onto a low one */
    expect(page_is_erased(BLOCK1_PAGE, 64), "high row did not alias low row");

    /* ---- program failure is surfaced, not swallowed ---- */
    printf("program failure handling at block %d\r\n", FAIL_BLOCK);
    hal_fsmc.erase_block(FAIL_PAGE);
    fill(ref, 64, 0x61);
    status = program_page(ref, FAIL_PAGE, 64);
    printf("failed-program status: %lu (expect %lu)\r\n",
        (unsigned long)status, (unsigned long)FLASH_STATUS_ERROR);
    expect(status == FLASH_STATUS_ERROR,
        "a refused program reports FLASH_STATUS_ERROR");

    /* Another block must still work: the failure is the device's, not a
     * wedged driver. It has to be an erased one - programming can only clear
     * bits, so a block still holding earlier data would never read back as
     * the new pattern.
     */
    hal_fsmc.erase_block(AFTER_FAIL_PAGE);
    expect(roundtrip(AFTER_FAIL_PAGE, 64, 0x62),
        "a fresh block still programs after a failure");

    /* ---- retiring a block ---- */
    /* This is the mechanism np_nand_mark_bad() relies on: programming can only
     * clear bits, so a page of 0xFF with one byte zeroed changes that byte and
     * nothing else. Done on a good block so the write itself succeeds.
     */
    hal_fsmc.erase_block(BLOCK1_PAGE);

    memset(buf, 0xFF, PAGE_FULL);
    buf[SIM_PAGE_SIZE + 0] = 0x00;          /* bad block marker */
    expect(program_page(buf, BLOCK1_PAGE, PAGE_FULL) == FLASH_STATUS_READY,
        "marker page programs");

    memset(buf, 0, PAGE_FULL);
    hal_fsmc.read_page(buf, BLOCK1_PAGE, PAGE_FULL);
    expect(buf[SIM_PAGE_SIZE] == 0x00, "the marker byte reads back as 0x00");
    expect(all_erased(buf, SIM_PAGE_SIZE),
        "the data area is untouched by marking");
    {
        int spare_clean = 1;
        uint32_t i;

        for (i = 1; i < SIM_SPARE_SIZE; i++)
        {
            if (buf[SIM_PAGE_SIZE + i] != 0xFF)
                spare_clean = 0;
        }
        expect(spare_clean, "every other spare byte is untouched");
    }

    /* And it survives a re-read, which is what a later bad block scan does. */
    memset(buf, 0, 64);
    hal_fsmc.read_page(buf, BLOCK1_PAGE, SIM_PAGE_SIZE + 1);
    expect(buf[SIM_PAGE_SIZE] != 0xFF,
        "a later scan would see this block as bad");

    /* Erasing clears the marker again, which is why a retired block must never
     * be handed to erase afterwards.
     */
    hal_fsmc.erase_block(BLOCK1_PAGE);
    memset(buf, 0, 64);
    hal_fsmc.read_page(buf, BLOCK1_PAGE, SIM_PAGE_SIZE + 1);
    expect(buf[SIM_PAGE_SIZE] == 0xFF, "erase clears the marker again");

    /* ---- a page carrying real ECC parity ---- */
    /* The correction itself lives in the host application, so what matters
     * here is the ground it stands on: that a page carrying parity survives a
     * real write and read through the FSMC driver and the chip model without
     * a single byte moving, spare included. If the transport mangled the
     * spare area, every correction built on top of it would be meaningless.
     */
    printf("ECC page round trip at block %d\r\n", ECC_BLOCK_PAGE / PAGES_PER_BLOCK);
    hal_fsmc.erase_block(ECC_BLOCK_PAGE);

    expect(program_page((uint8_t *)ecc_page, ECC_BLOCK_PAGE, ECC_PAGE_TOTAL) ==
        FLASH_STATUS_READY, "ECC page programs");

    memset(buf, 0, PAGE_FULL);
    expect(hal_fsmc.read_page(buf, ECC_BLOCK_PAGE, ECC_PAGE_TOTAL) ==
        FLASH_STATUS_READY, "ECC page reads back");
    expect(same(buf, ecc_page, ECC_PAGE_SIZE), "data area survives byte for byte");
    expect(same(buf + ECC_PAGE_SIZE, ecc_page + ECC_PAGE_SIZE, ECC_PAGE_SPARE),
        "parity in the spare area survives byte for byte");

    /* ---- bit errors on the chip are read back faithfully ---- */
    /* Reproduced with real NAND semantics rather than a hook in the model:
     * programming can only clear bits, so writing 0xFF with three bits zeroed
     * clears exactly those three and leaves everything else alone. That is
     * what a weak cell looks like from the outside.
     */
    {
        uint32_t i, flipped = 0, other = 0;

        memset(ref, 0xFF, PAGE_FULL);
        for (i = 0; i < ECC_FLIP_COUNT; i++)
        {
            ref[ecc_flips[i].byte] &= (uint8_t)~(1u << ecc_flips[i].bit);
        }

        expect(program_page(ref, ECC_BLOCK_PAGE, ECC_PAGE_TOTAL) ==
            FLASH_STATUS_READY, "mask page programs");

        memset(buf, 0, PAGE_FULL);
        hal_fsmc.read_page(buf, ECC_BLOCK_PAGE, ECC_PAGE_TOTAL);

        for (i = 0; i < ECC_PAGE_TOTAL; i++)
        {
            uint8_t diff = (uint8_t)(buf[i] ^ ecc_page[i]);
            uint32_t k;
            int expected = 0;

            if (!diff)
                continue;

            for (k = 0; k < ECC_FLIP_COUNT; k++)
            {
                if (ecc_flips[k].byte == i &&
                    diff == (uint8_t)(1u << ecc_flips[k].bit))
                {
                    expected = 1;
                }
            }

            if (expected)
                flipped++;
            else
                other++;
        }

        printf("read back: %lu injected bit(s) present, %lu unexpected\r\n",
            (unsigned long)flipped, (unsigned long)other);
        expect(flipped == ECC_FLIP_COUNT,
            "every injected bit error is visible on read back");
        expect(other == 0, "and nothing else changed");
    }

    printf("=== SIM %s (%d failures) ===\r\n", failures ? "FAIL" : "PASS",
        failures);
    printf("SIM_COMPLETE\r\n");

    while (1)
        ;
}
