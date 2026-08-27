/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#include "fake_nand_chip.h"
#include "spl_stub.h"

#include <string.h>

#define CMD_AREA  (uint32_t)(1 << 16)
#define ADDR_AREA (uint32_t)(1 << 17)
#define DATA_AREA (uint32_t)0

/* Command set, datasheet Table 8 */
#define CMD_READ_1     0x00
#define CMD_READ_2     0x30
#define CMD_PROGRAM_1  0x80
#define CMD_PROGRAM_2  0x10
#define CMD_ERASE_1    0x60
#define CMD_ERASE_2    0xD0
#define CMD_STATUS     0x70
#define CMD_READ_ID    0x90
#define CMD_RESET      0xFF

/* Status register: bit 6 ready, bit 0 fail */
#define STATUS_READY 0x40
#define STATUS_FAIL  0x01

enum
{
    PHASE_IDLE,
    PHASE_READ_ADDR,
    PHASE_READ_DATA,
    PHASE_PROGRAM_ADDR,
    PHASE_PROGRAM_DATA,
    PHASE_ERASE_ADDR,
    PHASE_STATUS,
    PHASE_READ_ID,
};

fake_nand_trace_t fake_nand_trace;

static uint8_t mem[NAND_MODELLED_PAGES * NAND_PAGE_TOTAL];
static uint8_t page_buf[NAND_PAGE_TOTAL];

static int phase;
static uint32_t col, row;
static uint8_t addr_cnt;
static uint32_t ptr;
static uint32_t busy_left;
static uint32_t busy_reload;
static uint8_t id_idx;

static void violate(const char *msg)
{
    if (!fake_nand_trace.violation)
        fake_nand_trace.violation = msg;
}

void fake_nand_reset(uint32_t busy_reads)
{
    memset(&fake_nand_trace, 0, sizeof(fake_nand_trace));
    memset(&spl_capture, 0, sizeof(spl_capture));
    memset(mem, 0xFF, sizeof(mem));
    memset(page_buf, 0xFF, sizeof(page_buf));

    phase = PHASE_IDLE;
    col = row = 0;
    addr_cnt = 0;
    ptr = 0;
    busy_left = 0;
    busy_reload = busy_reads;
    id_idx = 0;
}

static uint8_t *page_ptr(uint32_t page)
{
    return mem + (page % NAND_MODELLED_PAGES) * NAND_PAGE_TOTAL;
}

void fake_nand_fill(uint32_t page, uint32_t offset, const uint8_t *d,
    uint32_t len)
{
    if (offset + len <= NAND_PAGE_TOTAL)
        memcpy(page_ptr(page) + offset, d, len);
}

uint8_t fake_nand_peek(uint32_t page, uint32_t offset)
{
    return offset < NAND_PAGE_TOTAL ? page_ptr(page)[offset] : 0xFF;
}

static void check_cycles(uint8_t expected, const char *msg)
{
    if (addr_cnt != expected)
        violate(msg);
}

static void begin_addr(int next_phase)
{
    phase = next_phase;
    col = 0;
    row = 0;
    addr_cnt = 0;
}

static void cmd_write(uint8_t val)
{
    fake_nand_trace.cmd_writes++;
    fake_nand_trace.last_cmd = val;

    switch (val)
    {
    case CMD_RESET:
        phase = PHASE_IDLE;
        addr_cnt = 0;
        busy_left = busy_reload;
        break;

    case CMD_READ_1:
        begin_addr(PHASE_READ_ADDR);
        break;

    case CMD_READ_2:
        if (phase != PHASE_READ_ADDR)
        {
            violate("read confirm without a read setup");
            break;
        }
        check_cycles(NAND_COL_CYCLES + NAND_ROW_CYCLES,
            "wrong number of address cycles for page read");
        memcpy(page_buf, page_ptr(row), NAND_PAGE_TOTAL);
        ptr = col;
        phase = PHASE_READ_DATA;
        fake_nand_trace.reads++;
        break;

    case CMD_PROGRAM_1:
        begin_addr(PHASE_PROGRAM_ADDR);
        memset(page_buf, 0xFF, sizeof(page_buf));
        break;

    case CMD_PROGRAM_2:
        if (phase != PHASE_PROGRAM_DATA && phase != PHASE_PROGRAM_ADDR)
        {
            violate("program confirm without a program setup");
            break;
        }
        {
            /* NAND programming can only clear bits */
            uint8_t *p = page_ptr(row);
            uint32_t i;

            for (i = 0; i < NAND_PAGE_TOTAL; i++)
                p[i] &= page_buf[i];
        }
        busy_left = busy_reload;
        phase = PHASE_STATUS;
        fake_nand_trace.programs++;
        break;

    case CMD_ERASE_1:
        begin_addr(PHASE_ERASE_ADDR);
        break;

    case CMD_ERASE_2:
        if (phase != PHASE_ERASE_ADDR)
        {
            violate("erase confirm without an erase setup");
            break;
        }
        /* Erase takes the row address only */
        check_cycles(NAND_ROW_CYCLES,
            "wrong number of address cycles for block erase");
        {
            uint32_t base = row - (row % NAND_PAGES_PER_BLOCK);
            uint32_t i;

            for (i = 0; i < NAND_PAGES_PER_BLOCK; i++)
                memset(page_ptr(base + i), 0xFF, NAND_PAGE_TOTAL);
        }
        busy_left = busy_reload;
        phase = PHASE_STATUS;
        fake_nand_trace.erases++;
        break;

    case CMD_STATUS:
        phase = PHASE_STATUS;
        break;

    case CMD_READ_ID:
        begin_addr(PHASE_READ_ID);
        id_idx = 0;
        break;

    default:
        violate("unsupported command");
        break;
    }
}

static void addr_write(uint8_t val)
{
    fake_nand_trace.addr_writes++;

    switch (phase)
    {
    case PHASE_READ_ADDR:
    case PHASE_PROGRAM_ADDR:
        /* Column first, then row, each least significant byte first */
        if (addr_cnt < NAND_COL_CYCLES)
            col |= (uint32_t)val << (8 * addr_cnt);
        else if (addr_cnt < NAND_COL_CYCLES + NAND_ROW_CYCLES)
            row |= (uint32_t)val << (8 * (addr_cnt - NAND_COL_CYCLES));
        break;

    case PHASE_ERASE_ADDR:
        if (addr_cnt < NAND_ROW_CYCLES)
            row |= (uint32_t)val << (8 * addr_cnt);
        break;

    case PHASE_READ_ID:
        break;

    default:
        violate("address cycle outside a command that takes one");
        break;
    }

    addr_cnt++;
    fake_nand_trace.last_addr_cycles = addr_cnt;
    fake_nand_trace.last_col = col;
    fake_nand_trace.last_row = row;

    if (phase == PHASE_PROGRAM_ADDR &&
        addr_cnt == NAND_COL_CYCLES + NAND_ROW_CYCLES)
    {
        ptr = col;
        phase = PHASE_PROGRAM_DATA;
    }
}

static uint8_t status_byte(void)
{
    if (busy_left)
    {
        busy_left--;
        return 0x00; /* ready bit clear while busy */
    }

    return STATUS_READY;
}

static uint8_t id_byte(void)
{
    static const uint8_t id[4] = { NAND_ID_1, NAND_ID_2, NAND_ID_3, NAND_ID_4 };

    if (addr_cnt != 1)
        violate("read ID needs exactly one address cycle");

    /* This device defines four ID bytes, anything past that is undefined */
    return id_idx < 4 ? id[id_idx++] : 0xFF;
}

static uint8_t data_read(void)
{
    fake_nand_trace.data_reads++;

    switch (phase)
    {
    case PHASE_STATUS:
        return status_byte();
    case PHASE_READ_ID:
        return id_byte();
    case PHASE_READ_DATA:
        return ptr < NAND_PAGE_TOTAL ? page_buf[ptr++] : 0xFF;
    default:
        violate("data read outside a read, read ID or status command");
        return 0xFF;
    }
}

static void data_write(uint8_t val)
{
    fake_nand_trace.data_writes++;

    if (phase != PHASE_PROGRAM_DATA)
    {
        violate("data written outside a program command");
        return;
    }

    if (ptr < NAND_PAGE_TOTAL)
        page_buf[ptr++] = val;
}

/* ---- bus primitives consumed by fsmc_nand.c ---------------------------- */

void nand_bus_write(uint32_t area, uint8_t val)
{
    if (area == CMD_AREA)
        cmd_write(val);
    else if (area == ADDR_AREA)
        addr_write(val);
    else if (area == DATA_AREA)
        data_write(val);
    else
        violate("write to an undecoded bus area");
}

uint8_t nand_bus_read(uint32_t area)
{
    if (area == DATA_AREA)
        return data_read();

    violate("read from a non data bus area");

    return 0xFF;
}

uint32_t nand_bus_read32(uint32_t area, uint32_t index)
{
    uint32_t v = 0;
    int i;

    (void)index;

    for (i = 0; i < 4; i++)
        v |= (uint32_t)nand_bus_read(area) << (8 * i);

    return v;
}
