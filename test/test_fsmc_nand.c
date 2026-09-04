/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *
 *  Host tests for fsmc_nand.c driven against fake_nand_chip.c.
 *
 *  The driver configuration is read from the real chip database CSV rather
 *  than hand written here, so these tests check the shipped database entry
 *  for S34ML01G1 against the behaviour of the modelled silicon.
 */

#include "fake_nand_chip.h"
#include "spl_stub.h"
#include "fsmc_nand.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CSV_PATH "../qt/nando_parallel_chip_db.csv"
#define CHIP_NAME "S34ML01G1"

#define UNDEFINED_CMD 0xFF
#define BUSY_READS 3

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
} nand_conf_wire_t;

/* CSV column indices */
enum
{
    C_NAME, C_PAGE_SIZE, C_BLOCK_SIZE, C_TOTAL_SIZE, C_SPARE_SIZE, C_BB_OFF,
    C_TCS, C_TCLS, C_TALS, C_TCLR, C_TAR, C_TWP, C_TRP, C_TDS, C_TCH, C_TCLH,
    C_TALH, C_TWC, C_TRC, C_TREA, C_ROW_CYCLES, C_COL_CYCLES,
    C_READ1, C_READ2, C_READ_SPARE, C_READ_ID, C_RESET, C_WRITE1, C_WRITE2,
    C_ERASE1, C_ERASE2, C_STATUS, C_SET_FEAT, C_ECC_ADDR, C_ECC_EN, C_ECC_DIS,
    C_ID1, C_ID2, C_ID3, C_ID4, C_ID5,
    C_NUM,
};

static char *fields[C_NUM];
static char csv_line[1024];

static int checks_failed, tests_run, tests_failed;

#define CHECK(cond, ...)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            checks_failed++;                                                   \
            printf("\n      line %d: ", __LINE__);                             \
            printf(__VA_ARGS__);                                               \
        }                                                                      \
    } while (0)

#define CHECK_CLEAN()                                                          \
    CHECK(!fake_nand_trace.violation, "chip protocol violation: %s",           \
        fake_nand_trace.violation ? fake_nand_trace.violation : "")

/* '-' in the CSV is ChipDb::paramNotDefValue, the firmware sees 0xFF */
static uint32_t field_val(int idx)
{
    if (!strcmp(fields[idx], "-"))
        return UNDEFINED_CMD;

    return (uint32_t)strtoul(fields[idx], NULL, 10);
}

static int load_csv_row(void)
{
    FILE *f = fopen(CSV_PATH, "r");
    char *p;
    int n;

    if (!f)
    {
        printf("cannot open %s\n", CSV_PATH);
        return -1;
    }

    while (fgets(csv_line, sizeof(csv_line), f))
    {
        if (csv_line[0] == '#')
            continue;
        if (strncmp(csv_line, CHIP_NAME ",", strlen(CHIP_NAME) + 1))
            continue;

        n = 0;
        for (p = strtok(csv_line, ",\n"); p && n < C_NUM;
            p = strtok(NULL, ",\n"))
        {
            while (*p == ' ')
                p++;
            fields[n++] = p;
        }

        fclose(f);

        if (n != C_NUM)
        {
            printf("expected %d fields, found %d\n", C_NUM, n);
            return -1;
        }

        return 0;
    }

    fclose(f);
    printf("%s not found in %s\n", CHIP_NAME, CSV_PATH);

    return -1;
}

static nand_conf_wire_t conf_from_csv(void)
{
    nand_conf_wire_t c;

    memset(&c, 0, sizeof(c));

    /* FSMC timings are derived from the ns values by the host application.
     * They do not affect protocol behaviour, so nominal values are used.
     */
    c.setup_time = 2;
    c.wait_setup_time = 3;
    c.hold_setup_time = 2;
    c.hi_z_setup_time = 2;
    c.clr_setup_time = 2;
    c.ar_setup_time = 2;

    c.row_cycles = (uint8_t)field_val(C_ROW_CYCLES);
    c.col_cycles = (uint8_t)field_val(C_COL_CYCLES);
    c.read1_cmd = (uint8_t)field_val(C_READ1);
    c.read2_cmd = (uint8_t)field_val(C_READ2);
    c.read_spare_cmd = (uint8_t)field_val(C_READ_SPARE);
    c.read_id_cmd = (uint8_t)field_val(C_READ_ID);
    c.reset_cmd = (uint8_t)field_val(C_RESET);
    c.write1_cmd = (uint8_t)field_val(C_WRITE1);
    c.write2_cmd = (uint8_t)field_val(C_WRITE2);
    c.erase1_cmd = (uint8_t)field_val(C_ERASE1);
    c.erase2_cmd = (uint8_t)field_val(C_ERASE2);
    c.status_cmd = (uint8_t)field_val(C_STATUS);
    c.set_features_cmd = (uint8_t)field_val(C_SET_FEAT);
    c.enable_ecc_addr = (uint8_t)field_val(C_ECC_ADDR);
    c.enable_ecc_value = (uint8_t)field_val(C_ECC_EN);
    c.disable_ecc_value = (uint8_t)field_val(C_ECC_DIS);

    return c;
}

static int setup(nand_conf_wire_t *c)
{
    fake_nand_reset(BUSY_READS);

    return hal_fsmc.init(c, sizeof(*c));
}

/* ---- database entry versus datasheet ----------------------------------- */

static void test_db_geometry(void)
{
    CHECK(field_val(C_PAGE_SIZE) == NAND_PAGE_MAIN, "page size %u, datasheet %u",
        field_val(C_PAGE_SIZE), NAND_PAGE_MAIN);
    CHECK(field_val(C_SPARE_SIZE) == NAND_PAGE_SPARE, "spare size %u, datasheet %u",
        field_val(C_SPARE_SIZE), NAND_PAGE_SPARE);
    CHECK(field_val(C_BLOCK_SIZE) == NAND_BLOCK_MAIN, "block size %u, datasheet %u",
        field_val(C_BLOCK_SIZE), NAND_BLOCK_MAIN);
    CHECK(field_val(C_TOTAL_SIZE) == NAND_TOTAL_SIZE, "total size %u, datasheet %u",
        field_val(C_TOTAL_SIZE), NAND_TOTAL_SIZE);
}

static void test_db_address_cycles(void)
{
    CHECK(field_val(C_ROW_CYCLES) == NAND_ROW_CYCLES,
        "row cycles %u, datasheet Table 4 gives %u for the 1 Gb x8 device",
        field_val(C_ROW_CYCLES), NAND_ROW_CYCLES);
    CHECK(field_val(C_COL_CYCLES) == NAND_COL_CYCLES,
        "col cycles %u, datasheet Table 4 gives %u",
        field_val(C_COL_CYCLES), NAND_COL_CYCLES);
}

static void test_db_command_set(void)
{
    CHECK(field_val(C_READ1) == 0x00, "read 1 command 0x%02X", field_val(C_READ1));
    CHECK(field_val(C_READ2) == 0x30, "read 2 command 0x%02X", field_val(C_READ2));
    CHECK(field_val(C_WRITE1) == 0x80, "write 1 command 0x%02X", field_val(C_WRITE1));
    CHECK(field_val(C_WRITE2) == 0x10, "write 2 command 0x%02X", field_val(C_WRITE2));
    CHECK(field_val(C_ERASE1) == 0x60, "erase 1 command 0x%02X", field_val(C_ERASE1));
    CHECK(field_val(C_ERASE2) == 0xD0, "erase 2 command 0x%02X", field_val(C_ERASE2));
    CHECK(field_val(C_STATUS) == 0x70, "status command 0x%02X", field_val(C_STATUS));
    CHECK(field_val(C_READ_ID) == 0x90, "read ID command 0x%02X", field_val(C_READ_ID));
    CHECK(field_val(C_RESET) == 0xFF, "reset command 0x%02X", field_val(C_RESET));
    /* 2 KB page devices reach the spare area with a column address */
    CHECK(field_val(C_READ_SPARE) == UNDEFINED_CMD,
        "read spare command should be undefined for a 2 KB page device");
}

static void test_db_ids(void)
{
    CHECK(field_val(C_ID1) == NAND_ID_1, "ID1 0x%02X, datasheet 0x%02X",
        field_val(C_ID1), NAND_ID_1);
    CHECK(field_val(C_ID2) == NAND_ID_2, "ID2 0x%02X, datasheet 0x%02X",
        field_val(C_ID2), NAND_ID_2);
    CHECK(field_val(C_ID3) == NAND_ID_3, "ID3 0x%02X, datasheet 0x%02X",
        field_val(C_ID3), NAND_ID_3);
    CHECK(field_val(C_ID4) == NAND_ID_4, "ID4 0x%02X, datasheet 0x%02X",
        field_val(C_ID4), NAND_ID_4);
    CHECK(field_val(C_ID5) == UNDEFINED_CMD,
        "ID5 should be undefined, this device reports only four ID bytes");
}

/* ---- driver against the modelled device -------------------------------- */

static void test_read_id(void)
{
    nand_conf_wire_t c = conf_from_csv();
    chip_id_t id;

    memset(&id, 0xCC, sizeof(id));
    CHECK(setup(&c) == 0, "init failed");

    hal_fsmc.read_id(&id);

    CHECK(id.maker_id == NAND_ID_1, "maker_id 0x%02X", id.maker_id);
    CHECK(id.device_id == NAND_ID_2, "device_id 0x%02X", id.device_id);
    CHECK(id.third_id == NAND_ID_3, "third_id 0x%02X", id.third_id);
    CHECK(id.fourth_id == NAND_ID_4, "fourth_id 0x%02X", id.fourth_id);
    CHECK_CLEAN();
}

static void test_read_page(void)
{
    nand_conf_wire_t c = conf_from_csv();
    uint8_t pattern[NAND_PAGE_MAIN], buf[NAND_PAGE_MAIN];
    uint32_t page = 70; /* block 1, page 6 */
    uint32_t i;

    for (i = 0; i < NAND_PAGE_MAIN; i++)
        pattern[i] = (uint8_t)((i * 7) ^ 0x3C);

    CHECK(setup(&c) == 0, "init failed");
    fake_nand_fill(page, 0, pattern, NAND_PAGE_MAIN);

    memset(buf, 0, sizeof(buf));
    hal_fsmc.read_page(buf, page, NAND_PAGE_MAIN);

    CHECK(fake_nand_trace.last_addr_cycles == NAND_COL_CYCLES + NAND_ROW_CYCLES,
        "sent %u address cycles, device latches %u",
        fake_nand_trace.last_addr_cycles, NAND_COL_CYCLES + NAND_ROW_CYCLES);
    CHECK(fake_nand_trace.last_row == page, "row address %u, expected %u",
        fake_nand_trace.last_row, page);
    CHECK(fake_nand_trace.last_col == 0, "column address %u, expected 0",
        fake_nand_trace.last_col);
    CHECK(!memcmp(buf, pattern, NAND_PAGE_MAIN), "page data mismatch");
    CHECK_CLEAN();
}

/* This device has no separate spare read command. The driver must say so and
 * let np_read_bad_block_info_from_page() fall back to a full page read.
 */
static void test_bad_block_marker_path(void)
{
    nand_conf_wire_t c = conf_from_csv();
    uint8_t buf[NAND_PAGE_TOTAL];
    uint32_t page = 12, status, cmds_before;
    uint32_t bb_off = field_val(C_BB_OFF);
    uint8_t marker = 0x00; /* a factory marked bad block */

    CHECK(setup(&c) == 0, "init failed");

    /* init issues a reset command, so measure the delta across the call */
    cmds_before = fake_nand_trace.cmd_writes;

    status = hal_fsmc.read_spare_data(buf, page, bb_off, 1);
    CHECK(status == FLASH_STATUS_INVALID_CMD,
        "read_spare_data returned %u, expected invalid command so the caller "
        "falls back to a full page read", status);
    CHECK(fake_nand_trace.cmd_writes == cmds_before,
        "unsupported spare read must not touch the bus");

    fake_nand_fill(page, NAND_PAGE_MAIN + bb_off, &marker, 1);

    memset(buf, 0xFF, sizeof(buf));
    status = hal_fsmc.read_page(buf, page, NAND_PAGE_MAIN + NAND_PAGE_SPARE);

    CHECK(status == FLASH_STATUS_READY, "page read returned %u", status);
    CHECK(fake_nand_trace.last_col == 0, "column %u, expected 0",
        fake_nand_trace.last_col);
    CHECK(buf[NAND_PAGE_MAIN + bb_off] == marker,
        "bad block marker read as 0x%02X, expected 0x%02X",
        buf[NAND_PAGE_MAIN + bb_off], marker);
    CHECK_CLEAN();
}

static void test_program_page(void)
{
    nand_conf_wire_t c = conf_from_csv();
    uint8_t data[NAND_PAGE_MAIN];
    uint32_t page = 33, i, status, guard = 0;

    for (i = 0; i < NAND_PAGE_MAIN; i++)
        data[i] = (uint8_t)(i ^ 0xA5);

    CHECK(setup(&c) == 0, "init failed");

    hal_fsmc.write_page_async(data, page, NAND_PAGE_MAIN);

    CHECK(fake_nand_trace.last_row == page, "program row %u, expected %u",
        fake_nand_trace.last_row, page);
    CHECK(fake_nand_trace.data_writes == NAND_PAGE_MAIN,
        "wrote %u bytes, expected %u", fake_nand_trace.data_writes,
        NAND_PAGE_MAIN);
    CHECK(fake_nand_trace.programs == 1, "program not committed");

    do {
        status = hal_fsmc.read_status();
    } while (status == FLASH_STATUS_BUSY && ++guard < 32);

    CHECK(status == FLASH_STATUS_READY, "final status %u", status);

    for (i = 0; i < NAND_PAGE_MAIN; i++)
    {
        if (fake_nand_peek(page, i) != data[i])
        {
            CHECK(0, "programmed data mismatch at %u: 0x%02X != 0x%02X", i,
                fake_nand_peek(page, i), data[i]);
            break;
        }
    }

    CHECK_CLEAN();
}

static void test_erase_block(void)
{
    nand_conf_wire_t c = conf_from_csv();
    uint8_t zeros[NAND_PAGE_MAIN];
    uint32_t page = NAND_PAGES_PER_BLOCK * 2; /* first page of block 2 */
    uint32_t status, i;

    memset(zeros, 0x00, sizeof(zeros));

    CHECK(setup(&c) == 0, "init failed");
    fake_nand_fill(page, 0, zeros, NAND_PAGE_MAIN);
    fake_nand_fill(page + 5, 0, zeros, NAND_PAGE_MAIN);

    status = hal_fsmc.erase_block(page);

    CHECK(status == FLASH_STATUS_READY, "erase returned %u", status);
    CHECK(fake_nand_trace.erases == 1, "erase not executed");
    CHECK(fake_nand_trace.last_addr_cycles == NAND_ROW_CYCLES,
        "erase sent %u address cycles, device expects %u (row only)",
        fake_nand_trace.last_addr_cycles, NAND_ROW_CYCLES);

    for (i = 0; i < 32; i++)
    {
        if (fake_nand_peek(page, i) != 0xFF ||
            fake_nand_peek(page + 5, i) != 0xFF)
        {
            CHECK(0, "block not fully erased at offset %u", i);
            break;
        }
    }

    CHECK_CLEAN();
}

static void test_bad_block_supported(void)
{
    nand_conf_wire_t c = conf_from_csv();

    CHECK(setup(&c) == 0, "init failed");
    CHECK(hal_fsmc.is_bb_supported(), "NAND must support bad block handling");
}

/* Regression guard: the database used to say three row cycles for this part */
static void test_wrong_row_cycles_is_rejected(void)
{
    nand_conf_wire_t c = conf_from_csv();
    uint8_t buf[64];

    c.row_cycles = 3;

    CHECK(setup(&c) == 0, "init failed");

    hal_fsmc.read_page(buf, 70, sizeof(buf));

    CHECK(fake_nand_trace.violation != NULL,
        "device accepted five address cycles, the model is too permissive");
    CHECK(fake_nand_trace.last_addr_cycles == 5,
        "expected 5 address cycles to be sent, saw %u",
        fake_nand_trace.last_addr_cycles);
}

/* ---- runner ------------------------------------------------------------ */

static void run(const char *name, void (*fn)(void))
{
    int before = checks_failed;

    tests_run++;
    printf("  %-38s ... ", name);
    fflush(stdout);

    fn();

    if (checks_failed == before)
    {
        printf("ok\n");
    }
    else
    {
        printf("\n      -> FAIL\n");
        tests_failed++;
    }
}

int main(void)
{
    printf("S34ML01G1 NAND tests (database entry + fsmc_nand driver)\n\n");

    if (load_csv_row())
        return 2;

    printf("  database row: %s\n\n", fields[C_NAME]);

    run("DB geometry matches datasheet", test_db_geometry);
    run("DB address cycles match datasheet", test_db_address_cycles);
    run("DB command set matches datasheet", test_db_command_set);
    run("DB ID bytes match datasheet", test_db_ids);
    run("read ID", test_read_id);
    run("read page", test_read_page);
    run("bad block marker path", test_bad_block_marker_path);
    run("program page", test_program_page);
    run("erase block", test_erase_block);
    run("bad block support", test_bad_block_supported);
    run("wrong row cycles is detected", test_wrong_row_cycles_is_rejected);

    printf("\n%d tests, %d failed, %d checks failed\n", tests_run, tests_failed,
        checks_failed);

    return tests_failed ? 1 : 0;
}
