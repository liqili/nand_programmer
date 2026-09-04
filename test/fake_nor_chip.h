/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *
 *  Behavioural model of a parallel NOR flash on the FSMC NAND style bus.
 *
 *  The device is driven purely through byte accesses with A16 (CLE) and A17
 *  (ALE) selecting the command, address and data areas, so this model is a
 *  direct stand in for the real part and ports unchanged to a Renode
 *  memory mapped peripheral at 0x70000000.
 */

#ifndef _FAKE_NOR_CHIP_H_
#define _FAKE_NOR_CHIP_H_

#include <stdint.h>

#define FAKE_NOR_SIZE (1 << 20) /* 1 MiB window, sparse beyond that */

typedef struct
{
    uint8_t id[5];

    uint8_t read_cmd;
    uint8_t read_id_cmd;
    uint8_t write_cmd;
    uint8_t write_en_cmd;
    uint8_t erase_cmd;
    uint8_t status_cmd;

    uint8_t busy_bit;   /* status register bit reporting progress */
    uint8_t busy_state; /* bit value that means "busy" */

    uint8_t addr_cycles;    /* cycles the device expects per transfer */
    uint8_t id_addr_cycles; /* cycles the device expects after read ID */

    uint32_t block_size;
    uint32_t busy_reads; /* status reads returning busy after an operation */
} fake_nor_spec_t;

/* Instrumentation, reset together with the device */
typedef struct
{
    uint32_t cmd_writes;
    uint32_t addr_writes;
    uint32_t data_reads;
    uint32_t data_writes;

    uint8_t last_cmd;
    uint8_t last_addr_cycles; /* cycles received for the most recent command */
    uint32_t last_addr;       /* address assembled for the most recent command */

    int wren_seen;   /* write enable issued since the last program or erase */
    int erase_count;

    const char *violation; /* first protocol violation observed, or NULL */
} fake_nor_trace_t;

extern fake_nor_trace_t fake_nor_trace;

void fake_nor_reset(const fake_nor_spec_t *spec);
void fake_nor_fill(uint32_t addr, const uint8_t *data, uint32_t len);
uint8_t fake_nor_peek(uint32_t addr);
void fake_nor_erase_all(void);

#endif /* _FAKE_NOR_CHIP_H_ */
