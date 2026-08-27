/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#include "fake_nor_chip.h"
#include "spl_stub.h"

#include <string.h>

#define CMD_AREA  (uint32_t)(1 << 16)
#define ADDR_AREA (uint32_t)(1 << 17)
#define DATA_AREA (uint32_t)0

fake_nor_trace_t fake_nor_trace;

static fake_nor_spec_t spec;
static uint8_t mem[FAKE_NOR_SIZE];

static uint8_t cur_cmd;
static uint32_t addr_acc;
static uint8_t addr_cnt;
static uint8_t id_idx;
static int wel;            /* write enable latch */
static uint32_t busy_left; /* status reads still reporting busy */

static void violate(const char *msg)
{
    if (!fake_nor_trace.violation)
        fake_nor_trace.violation = msg;
}

void fake_nor_reset(const fake_nor_spec_t *s)
{
    spec = *s;

    memset(&fake_nor_trace, 0, sizeof(fake_nor_trace));
    memset(&spl_capture, 0, sizeof(spl_capture));
    memset(mem, 0xFF, sizeof(mem));

    cur_cmd = 0;
    addr_acc = 0;
    addr_cnt = 0;
    id_idx = 0;
    wel = 0;
    busy_left = 0;
}

void fake_nor_fill(uint32_t addr, const uint8_t *data, uint32_t len)
{
    if (addr + len > FAKE_NOR_SIZE)
        return;

    memcpy(mem + addr, data, len);
}

uint8_t fake_nor_peek(uint32_t addr)
{
    return addr < FAKE_NOR_SIZE ? mem[addr] : 0xFF;
}

void fake_nor_erase_all(void)
{
    memset(mem, 0xFF, sizeof(mem));
}

/* Addresses beyond the modelled window wrap, so wide address tests still
 * observe the exact byte the driver asked for.
 */
static uint32_t wrap(uint32_t addr)
{
    return addr & (FAKE_NOR_SIZE - 1);
}

static void check_addr_cycles(uint8_t expected, const char *msg)
{
    if (addr_cnt != expected)
        violate(msg);
}

static void start_operation(void)
{
    /* 0xFF marks the command as absent: such a device needs no latch */
    if (!wel && spec.write_en_cmd != 0xFF)
        violate("program or erase attempted without write enable");

    fake_nor_trace.wren_seen = wel;
    wel = 0;
    busy_left = spec.busy_reads;
}

static void cmd_write(uint8_t val)
{
    fake_nor_trace.cmd_writes++;
    fake_nor_trace.last_cmd = val;

    cur_cmd = val;
    addr_acc = 0;
    addr_cnt = 0;
    id_idx = 0;

    if (val == spec.write_en_cmd)
        wel = 1;
}

static void addr_write(uint8_t val)
{
    fake_nor_trace.addr_writes++;

    addr_acc = (addr_acc << 8) | val;
    addr_cnt++;

    fake_nor_trace.last_addr_cycles = addr_cnt;
    fake_nor_trace.last_addr = addr_acc;

    /* An erase has no data phase, it commits once the address is complete */
    if (cur_cmd == spec.erase_cmd && addr_cnt == spec.addr_cycles)
    {
        uint32_t base;

        start_operation();

        base = wrap(addr_acc) & ~(spec.block_size - 1);
        memset(mem + base, 0xFF, spec.block_size);
        fake_nor_trace.erase_count++;
    }
}

static uint8_t status_byte(void)
{
    int busy = busy_left > 0;
    uint8_t bit_val, st;

    if (busy_left)
        busy_left--;

    bit_val = busy ? spec.busy_state : !spec.busy_state;
    st = bit_val ? (uint8_t)(1 << spec.busy_bit) : 0;

    /* Noise in the unrelated bits: a driver comparing the whole byte instead
     * of masking the busy bit will fail here.
     */
    st |= (uint8_t)(0xA5 & ~(1 << spec.busy_bit));

    return st;
}

static uint8_t data_read(void)
{
    fake_nor_trace.data_reads++;

    if (cur_cmd == spec.status_cmd)
        return status_byte();

    if (cur_cmd == spec.read_id_cmd)
    {
        check_addr_cycles(spec.id_addr_cycles,
            "wrong number of address cycles for read ID");
        return spec.id[id_idx++ % 5];
    }

    if (cur_cmd == spec.read_cmd)
    {
        check_addr_cycles(spec.addr_cycles,
            "wrong number of address cycles for read");
        return mem[wrap(addr_acc++)];
    }

    violate("data read outside a read, read ID or status command");

    return 0xFF;
}

static void data_write(uint8_t val)
{
    fake_nor_trace.data_writes++;

    if (cur_cmd != spec.write_cmd)
    {
        violate("data written outside a program command");
        return;
    }

    check_addr_cycles(spec.addr_cycles,
        "wrong number of address cycles for program");

    if (!wel && !fake_nor_trace.wren_seen && spec.write_en_cmd != 0xFF)
        violate("program attempted without write enable");

    /* NOR programming can only clear bits */
    mem[wrap(addr_acc)] &= val;
    addr_acc++;

    /* Re-arm busy on every byte so the device is busy once the page ends */
    if (wel)
        start_operation();
    else
        busy_left = spec.busy_reads;
}

/* ---- bus primitives consumed by parallel_nor_flash.c ------------------- */

void pnor_bus_write(uint32_t area, uint8_t val)
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

uint8_t pnor_bus_read(uint32_t area)
{
    if (area == DATA_AREA)
        return data_read();

    violate("read from a non data bus area");

    return 0xFF;
}
