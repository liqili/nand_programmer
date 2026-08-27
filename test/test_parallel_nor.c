/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *
 *  Host tests for parallel_nor_flash.c driven against fake_nor_chip.c.
 */

#include "fake_nor_chip.h"
#include "spl_stub.h"
#include "parallel_nor_flash.h"

#include <stdio.h>
#include <string.h>

/* Mirror of the wire format in parallel_nor_flash.c and of
 * ParallelSerialChipConf in qt/parallel_serial_chip_info.cpp.
 */
typedef struct __attribute__((__packed__))
{
    uint8_t page_offset;
    uint8_t addr_cycles;
    uint8_t id_addr_cycles;
    uint8_t read_cmd;
    uint8_t read_id_cmd;
    uint8_t write_cmd;
    uint8_t write_en_cmd;
    uint8_t erase_cmd;
    uint8_t status_cmd;
    uint8_t busy_bit;
    uint8_t busy_state;
    uint8_t setup_time;
    uint8_t wait_setup_time;
    uint8_t hold_setup_time;
    uint8_t hi_z_setup_time;
    uint8_t clr_setup_time;
    uint8_t ar_setup_time;
} pnor_conf_wire_t;

#define UNDEFINED_CMD 0xFF
#define PAGE_OFF 8
#define PAGE_SIZE (1 << PAGE_OFF)
#define BLOCK_SIZE 4096

static int checks_failed;
static int tests_run;
static int tests_failed;

#define CHECK(cond, ...)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            checks_failed++;                                                   \
            printf("\n      line %d: ", __LINE__);                              \
            printf(__VA_ARGS__);                                               \
            printf("\n");                                                      \
        }                                                                      \
    } while (0)

#define CHECK_CLEAN()                                                          \
    CHECK(!fake_nor_trace.violation, "chip protocol violation: %s",            \
        fake_nor_trace.violation ? fake_nor_trace.violation : "")

static const fake_nor_spec_t base_spec = {
    .id = { 0xEF, 0x40, 0x18, 0x5A, 0xA5 },
    .read_cmd = 0x03,
    .read_id_cmd = 0x9F,
    .write_cmd = 0x02,
    .write_en_cmd = 0x06,
    .erase_cmd = 0xD8,
    .status_cmd = 0x05,
    .busy_bit = 0,
    .busy_state = 1,
    .addr_cycles = 3,
    .id_addr_cycles = 0,
    .block_size = BLOCK_SIZE,
    .busy_reads = 3,
};

static pnor_conf_wire_t base_conf(void)
{
    pnor_conf_wire_t c;

    memset(&c, 0, sizeof(c));
    c.page_offset = PAGE_OFF;
    c.addr_cycles = 3;
    c.id_addr_cycles = 0;
    c.read_cmd = 0x03;
    c.read_id_cmd = 0x9F;
    c.write_cmd = 0x02;
    c.write_en_cmd = 0x06;
    c.erase_cmd = 0xD8;
    c.status_cmd = 0x05;
    c.busy_bit = 0;
    c.busy_state = 1;
    c.setup_time = 2;
    c.wait_setup_time = 3;
    c.hold_setup_time = 4;
    c.hi_z_setup_time = 5;
    c.clr_setup_time = 6;
    c.ar_setup_time = 7;

    return c;
}

/* Bring the driver and the chip model up together */
static int setup(const fake_nor_spec_t *spec, pnor_conf_wire_t *conf)
{
    fake_nor_reset(spec);

    return hal_parallel_nor.init(conf, sizeof(*conf));
}

/* ---- tests ------------------------------------------------------------- */

static void test_init_validates_conf(void)
{
    pnor_conf_wire_t c = base_conf();

    fake_nor_reset(&base_spec);

    CHECK(hal_parallel_nor.init(&c, sizeof(c) - 1) != 0,
        "short conf accepted");

    c = base_conf();
    c.addr_cycles = 0;
    CHECK(hal_parallel_nor.init(&c, sizeof(c)) != 0, "addr_cycles 0 accepted");

    c = base_conf();
    c.addr_cycles = 5;
    CHECK(hal_parallel_nor.init(&c, sizeof(c)) != 0, "addr_cycles 5 accepted");

    c = base_conf();
    c.id_addr_cycles = 5;
    CHECK(hal_parallel_nor.init(&c, sizeof(c)) != 0,
        "id_addr_cycles 5 accepted");

    c = base_conf();
    c.busy_bit = 8;
    CHECK(hal_parallel_nor.init(&c, sizeof(c)) != 0, "busy_bit 8 accepted");

    c = base_conf();
    CHECK(hal_parallel_nor.init(&c, sizeof(c)) == 0, "valid conf rejected");
}

static void test_fsmc_setup(void)
{
    pnor_conf_wire_t c = base_conf();

    CHECK(setup(&base_spec, &c) == 0, "init failed");

    CHECK(spl_capture.wait_feature == FSMC_Waitfeature_Disable,
        "wait feature must be disabled, these parts do not drive NWAIT");
    CHECK(spl_capture.ecc == FSMC_ECC_Disable, "hardware ECC must be disabled");
    CHECK(spl_capture.data_width == FSMC_MemoryDataWidth_8b,
        "bus must be 8 bit");
    CHECK(spl_capture.fsmc_clock_enabled, "FSMC clock not enabled");
    CHECK(spl_capture.fsmc_bank_enabled, "FSMC bank not enabled");
    CHECK(spl_capture.gpio_init_calls == 3, "expected 3 GPIO_Init calls, got %d",
        spl_capture.gpio_init_calls);

    CHECK(spl_capture.setup_time == 2 && spl_capture.wait_setup_time == 3 &&
        spl_capture.hold_setup_time == 4 && spl_capture.hi_z_setup_time == 5 &&
        spl_capture.tclr_setup_time == 6 && spl_capture.tar_setup_time == 7,
        "timing parameters not propagated from conf");
}

static void test_uninit_releases_bus(void)
{
    pnor_conf_wire_t c = base_conf();

    CHECK(setup(&base_spec, &c) == 0, "init failed");

    hal_parallel_nor.uninit();

    CHECK(!spl_capture.fsmc_bank_enabled, "FSMC bank still enabled after uninit");
    CHECK(spl_capture.fsmc_deinit_calls == 1, "FSMC not deinitialised");
    CHECK(!spl_capture.fsmc_clock_enabled, "FSMC clock still on after uninit");
}

static void test_read_id_jedec(void)
{
    pnor_conf_wire_t c = base_conf();
    chip_id_t id;

    memset(&id, 0xCC, sizeof(id));
    CHECK(setup(&base_spec, &c) == 0, "init failed");

    hal_parallel_nor.read_id(&id);

    CHECK(fake_nor_trace.last_cmd == 0x9F, "wrong read ID command 0x%02X",
        fake_nor_trace.last_cmd);
    CHECK(fake_nor_trace.addr_writes == 0,
        "JEDEC read ID must send no address cycles, sent %u",
        fake_nor_trace.addr_writes);

    CHECK(id.maker_id == 0xEF, "maker_id 0x%02X", id.maker_id);
    CHECK(id.device_id == 0x40, "device_id 0x%02X", id.device_id);
    CHECK(id.third_id == 0x18, "third_id 0x%02X", id.third_id);
    CHECK(id.fourth_id == 0x5A, "fourth_id 0x%02X", id.fourth_id);
    CHECK(id.fifth_id == 0xA5, "fifth_id 0x%02X, must not be left uninitialised",
        id.fifth_id);
    CHECK_CLEAN();
}

static void test_read_id_legacy_90h(void)
{
    fake_nor_spec_t spec = base_spec;
    pnor_conf_wire_t c = base_conf();
    chip_id_t id;

    spec.read_id_cmd = 0x90;
    spec.id_addr_cycles = 1;
    c.read_id_cmd = 0x90;
    c.id_addr_cycles = 1;

    CHECK(setup(&spec, &c) == 0, "init failed");

    hal_parallel_nor.read_id(&id);

    CHECK(fake_nor_trace.addr_writes == 1,
        "90h read ID must send one address cycle, sent %u",
        fake_nor_trace.addr_writes);
    CHECK(id.maker_id == 0xEF && id.device_id == 0x40, "wrong ID bytes");
    CHECK_CLEAN();
}

static void test_read_id_undefined(void)
{
    fake_nor_spec_t spec = base_spec;
    pnor_conf_wire_t c = base_conf();
    chip_id_t id;

    memset(&id, 0xCC, sizeof(id));
    c.read_id_cmd = UNDEFINED_CMD;

    CHECK(setup(&spec, &c) == 0, "init failed");

    hal_parallel_nor.read_id(&id);

    CHECK(fake_nor_trace.cmd_writes == 0,
        "undefined read ID must not touch the bus");
    CHECK(id.maker_id == 0 && id.fifth_id == 0, "ID not cleared");
}

static void test_read_page_addressing(void)
{
    pnor_conf_wire_t c = base_conf();
    uint8_t pattern[PAGE_SIZE], buf[PAGE_SIZE];
    uint32_t page = 5, expect_addr = 5 << PAGE_OFF;
    uint32_t i;

    for (i = 0; i < PAGE_SIZE; i++)
        pattern[i] = (uint8_t)(i ^ 0x5A);

    CHECK(setup(&base_spec, &c) == 0, "init failed");
    fake_nor_fill(expect_addr, pattern, PAGE_SIZE);

    memset(buf, 0, sizeof(buf));
    CHECK(hal_parallel_nor.read_page(buf, page, PAGE_SIZE) ==
        FLASH_STATUS_READY, "read_page did not report ready");

    CHECK(fake_nor_trace.addr_writes == 3, "expected 3 address cycles, got %u",
        fake_nor_trace.addr_writes);
    CHECK(fake_nor_trace.last_addr == expect_addr,
        "chip assembled address 0x%06X, expected 0x%06X "
        "(address must be sent most significant byte first)",
        fake_nor_trace.last_addr, expect_addr);
    CHECK(!memcmp(buf, pattern, PAGE_SIZE), "page data mismatch");
    CHECK_CLEAN();
}

static void test_four_byte_addressing(void)
{
    fake_nor_spec_t spec = base_spec;
    pnor_conf_wire_t c = base_conf();
    uint8_t buf[16];
    uint32_t page = 0x12345;
    uint32_t expect_addr = page << PAGE_OFF; /* 0x1234500, beyond 16 MiB */

    spec.addr_cycles = 4;
    c.addr_cycles = 4;

    CHECK(setup(&spec, &c) == 0, "init failed");

    hal_parallel_nor.read_page(buf, page, sizeof(buf));

    CHECK(fake_nor_trace.addr_writes == 4, "expected 4 address cycles, got %u",
        fake_nor_trace.addr_writes);
    CHECK(fake_nor_trace.last_addr == expect_addr,
        "address 0x%08X, expected 0x%08X (24 bit addressing would truncate)",
        fake_nor_trace.last_addr, expect_addr);
    CHECK_CLEAN();
}

static void test_read_spare_unsupported(void)
{
    pnor_conf_wire_t c = base_conf();
    uint8_t buf[8];

    CHECK(setup(&base_spec, &c) == 0, "init failed");

    CHECK(hal_parallel_nor.read_spare_data(buf, 0, 0, sizeof(buf)) ==
        FLASH_STATUS_INVALID_CMD, "spare data must report invalid command");
    CHECK(!hal_parallel_nor.is_bb_supported(), "bad blocks must be unsupported");
}

static void test_write_page_and_status_polling(void)
{
    pnor_conf_wire_t c = base_conf();
    uint8_t data[PAGE_SIZE];
    uint32_t page = 3, addr = 3 << PAGE_OFF;
    uint32_t i, busy_polls = 0, reads_before, cmds_before;
    uint32_t status;

    for (i = 0; i < PAGE_SIZE; i++)
        data[i] = (uint8_t)(0xF0 | (i & 0x0F));

    CHECK(setup(&base_spec, &c) == 0, "init failed");

    hal_parallel_nor.write_page_async(data, page, PAGE_SIZE);

    CHECK(fake_nor_trace.wren_seen, "write enable was not issued before program");
    CHECK(fake_nor_trace.last_addr == addr, "program address 0x%06X, expected 0x%06X",
        fake_nor_trace.last_addr, addr);
    CHECK(fake_nor_trace.data_writes == PAGE_SIZE, "wrote %u bytes, expected %u",
        fake_nor_trace.data_writes, PAGE_SIZE);

    /* read_status is polled from the main loop, so each call must issue
     * exactly one command and one data read and then return.
     */
    for (i = 0; i < 16; i++)
    {
        reads_before = fake_nor_trace.data_reads;
        cmds_before = fake_nor_trace.cmd_writes;

        status = hal_parallel_nor.read_status();

        CHECK(fake_nor_trace.data_reads - reads_before == 1 &&
            fake_nor_trace.cmd_writes - cmds_before == 1,
            "read_status must not spin: %u data reads, %u commands in one call",
            fake_nor_trace.data_reads - reads_before,
            fake_nor_trace.cmd_writes - cmds_before);

        if (status == FLASH_STATUS_BUSY)
        {
            busy_polls++;
            continue;
        }

        CHECK(status == FLASH_STATUS_READY, "unexpected status %u", status);
        break;
    }

    CHECK(busy_polls == 3, "expected 3 busy polls, got %u", busy_polls);

    for (i = 0; i < PAGE_SIZE; i++)
    {
        if (fake_nor_peek(addr + i) != data[i])
        {
            CHECK(0, "programmed data mismatch at offset %u: 0x%02X != 0x%02X",
                i, fake_nor_peek(addr + i), data[i]);
            break;
        }
    }

    CHECK_CLEAN();
}

static void test_program_only_clears_bits(void)
{
    pnor_conf_wire_t c = base_conf();
    uint8_t zeros[4] = { 0x00, 0x00, 0x00, 0x00 };
    uint8_t ones[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
    uint32_t addr = 1 << PAGE_OFF;

    CHECK(setup(&base_spec, &c) == 0, "init failed");

    hal_parallel_nor.write_page_async(zeros, 1, sizeof(zeros));
    CHECK(fake_nor_peek(addr) == 0x00, "program did not clear bits");

    hal_parallel_nor.write_page_async(ones, 1, sizeof(ones));
    CHECK(fake_nor_peek(addr) == 0x00,
        "programming 0xFF must not set bits back, erase is required");
    CHECK_CLEAN();
}

static void test_erase_block(void)
{
    pnor_conf_wire_t c = base_conf();
    uint8_t zeros[64];
    uint32_t page = BLOCK_SIZE >> PAGE_OFF; /* first page of block 1 */
    uint32_t addr = page << PAGE_OFF;
    uint32_t status, i;

    memset(zeros, 0x00, sizeof(zeros));

    CHECK(setup(&base_spec, &c) == 0, "init failed");
    fake_nor_fill(addr, zeros, sizeof(zeros));

    status = hal_parallel_nor.erase_block(page);

    CHECK(status == FLASH_STATUS_READY, "erase returned %u, expected ready",
        status);
    CHECK(fake_nor_trace.erase_count == 1, "erase not executed by the chip");
    CHECK(fake_nor_trace.wren_seen, "write enable was not issued before erase");

    for (i = 0; i < sizeof(zeros); i++)
    {
        if (fake_nor_peek(addr + i) != 0xFF)
        {
            CHECK(0, "block not erased at offset %u", i);
            break;
        }
    }

    CHECK_CLEAN();
}

static void test_erase_undefined_command(void)
{
    pnor_conf_wire_t c = base_conf();

    c.erase_cmd = UNDEFINED_CMD;

    CHECK(setup(&base_spec, &c) == 0, "init failed");

    CHECK(hal_parallel_nor.erase_block(0) == FLASH_STATUS_INVALID_CMD,
        "undefined erase command must report invalid command");
    CHECK(fake_nor_trace.cmd_writes == 0, "undefined erase touched the bus");
}

static void test_status_undefined_command(void)
{
    pnor_conf_wire_t c = base_conf();

    c.status_cmd = UNDEFINED_CMD;

    CHECK(setup(&base_spec, &c) == 0, "init failed");

    CHECK(hal_parallel_nor.read_status() == FLASH_STATUS_READY,
        "chip without a status register must report ready");
    CHECK(fake_nor_trace.cmd_writes == 0, "undefined status touched the bus");
}

static void test_inverted_busy_polarity(void)
{
    fake_nor_spec_t spec = base_spec;
    pnor_conf_wire_t c = base_conf();
    uint8_t data[4] = { 1, 2, 3, 4 };
    uint32_t busy_polls = 0, i, status;

    /* Bit 7 clear means busy on this device */
    spec.busy_bit = 7;
    spec.busy_state = 0;
    c.busy_bit = 7;
    c.busy_state = 0;

    CHECK(setup(&spec, &c) == 0, "init failed");

    hal_parallel_nor.write_page_async(data, 0, sizeof(data));

    for (i = 0; i < 16; i++)
    {
        status = hal_parallel_nor.read_status();
        if (status == FLASH_STATUS_BUSY)
        {
            busy_polls++;
            continue;
        }
        break;
    }

    CHECK(busy_polls == 3, "inverted polarity: expected 3 busy polls, got %u",
        busy_polls);
    CHECK(status == FLASH_STATUS_READY, "final status %u", status);
    CHECK_CLEAN();
}

/* ---- runner ------------------------------------------------------------ */

static void run(const char *name, void (*fn)(void))
{
    int before = checks_failed;

    tests_run++;
    printf("  %-34s ... ", name);
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
    printf("parallel NOR driver tests\n\n");

    run("init validates conf", test_init_validates_conf);
    run("FSMC setup", test_fsmc_setup);
    run("uninit releases bus", test_uninit_releases_bus);
    run("read ID, JEDEC 9Fh", test_read_id_jedec);
    run("read ID, legacy 90h", test_read_id_legacy_90h);
    run("read ID, undefined command", test_read_id_undefined);
    run("read page addressing", test_read_page_addressing);
    run("four byte addressing", test_four_byte_addressing);
    run("spare data unsupported", test_read_spare_unsupported);
    run("write page and status polling", test_write_page_and_status_polling);
    run("program only clears bits", test_program_only_clears_bits);
    run("erase block", test_erase_block);
    run("erase, undefined command", test_erase_undefined_command);
    run("status, undefined command", test_status_undefined_command);
    run("inverted busy polarity", test_inverted_busy_polarity);

    printf("\n%d tests, %d failed, %d checks failed\n", tests_run, tests_failed,
        checks_failed);

    return tests_failed ? 1 : 0;
}
