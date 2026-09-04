/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#include "parallel_nor_flash.h"
#include "log.h"

#ifdef PNOR_HOST_TEST
/* Host test build: SPL peripheral calls are stubbed out and bus accesses are
 * routed to the flash chip model instead of the FSMC window.
 */
#include "spl_stub.h"
#else
#include <stm32f10x.h>
#endif

#define CMD_AREA                   (uint32_t)(1<<16)  /* A16 = CLE  high */
#define ADDR_AREA                  (uint32_t)(1<<17)  /* A17 = ALE high */

#define DATA_AREA                  ((uint32_t)0x00000000)

#define Bank_NOR_ADDR      ((uint32_t)0x70000000)

#define UNDEFINED_CMD 0xFF

#define MAX_ADDR_CYCLES 4
#define MAX_BUSY_BIT 7

#ifndef PNOR_HOST_TEST
/* Every bus access in this driver goes through these two primitives. A16 and
 * A17 of the FSMC window select the command, address or data area.
 */
static inline void pnor_bus_write(uint32_t area, uint8_t val)
{
    *(__IO uint8_t *)(Bank_NOR_ADDR | area) = val;
}

static inline uint8_t pnor_bus_read(uint32_t area)
{
    return *(__IO uint8_t *)(Bank_NOR_ADDR | area);
}
#endif

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
} parallel_nor_conf_t;

static parallel_nor_conf_t pnor_conf;

static void pnor_gpio_init()
{
    GPIO_InitTypeDef gpio_init;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE |
        RCC_APB2Periph_GPIOF | RCC_APB2Periph_GPIOG, ENABLE);

    /* CLE, ALE, D0->D3, NOE, NWE and NCE2 pin configuration */
    gpio_init.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_14 | GPIO_Pin_15 |
        GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_7;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOD, &gpio_init);

    /* D4->D7 pin configuration */
    gpio_init.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_Init(GPIOE, &gpio_init);

    /* NWAIT pin configuration. Kept as pull-up input: parallel NOR devices do
     * not drive a ready/busy line, progress is polled with the status command.
     */
    gpio_init.GPIO_Pin = GPIO_Pin_6;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOD, &gpio_init);
}

static void pnor_fsmc_init()
{
    FSMC_NANDInitTypeDef fsmc_init;
    FSMC_NAND_PCCARDTimingInitTypeDef timing_init;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);

    timing_init.FSMC_SetupTime = pnor_conf.setup_time;
    timing_init.FSMC_WaitSetupTime = pnor_conf.wait_setup_time;
    timing_init.FSMC_HoldSetupTime = pnor_conf.hold_setup_time;
    timing_init.FSMC_HiZSetupTime = pnor_conf.hi_z_setup_time;

    fsmc_init.FSMC_Bank = FSMC_Bank2_NAND;
    /* No ready/busy signal on NWAIT, do not let the controller stall on it */
    fsmc_init.FSMC_Waitfeature = FSMC_Waitfeature_Disable;
    fsmc_init.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_8b;
    fsmc_init.FSMC_ECC = FSMC_ECC_Disable;
    fsmc_init.FSMC_ECCPageSize = FSMC_ECCPageSize_2048Bytes;
    fsmc_init.FSMC_TCLRSetupTime = pnor_conf.clr_setup_time;
    fsmc_init.FSMC_TARSetupTime = pnor_conf.ar_setup_time;
    fsmc_init.FSMC_CommonSpaceTimingStruct = &timing_init;
    fsmc_init.FSMC_AttributeSpaceTimingStruct = &timing_init;
    FSMC_NANDInit(&fsmc_init);

    FSMC_NANDCmd(FSMC_Bank2_NAND, ENABLE);
}

static int pnor_init(void *conf, uint32_t conf_size)
{
    parallel_nor_conf_t *pnor_conf_p = (parallel_nor_conf_t *)conf;

    if (conf_size < sizeof(parallel_nor_conf_t))
    {
        ERROR_PRINT("Parallel NOR conf size mismatch: %lu < %lu\r\n",
            (unsigned long)conf_size,
            (unsigned long)sizeof(parallel_nor_conf_t));
        return -1;
    }

    if (!pnor_conf_p->addr_cycles ||
        pnor_conf_p->addr_cycles > MAX_ADDR_CYCLES)
    {
        ERROR_PRINT("Invalid parallel NOR address cycles %u\r\n",
            pnor_conf_p->addr_cycles);
        return -1;
    }

    if (pnor_conf_p->id_addr_cycles > MAX_ADDR_CYCLES)
    {
        ERROR_PRINT("Invalid parallel NOR ID address cycles %u\r\n",
            pnor_conf_p->id_addr_cycles);
        return -1;
    }

    if (pnor_conf_p->busy_bit > MAX_BUSY_BIT)
    {
        ERROR_PRINT("Invalid parallel NOR busy bit %u\r\n",
            pnor_conf_p->busy_bit);
        return -1;
    }

    pnor_conf = *pnor_conf_p;

    pnor_gpio_init();
    pnor_fsmc_init();

    return 0;
}

static void pnor_uninit()
{
    FSMC_NANDCmd(FSMC_Bank2_NAND, DISABLE);
    FSMC_NANDDeInit(FSMC_Bank2_NAND);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, DISABLE);
}

static void pnor_send_cmd(uint8_t cmd)
{
    pnor_bus_write(CMD_AREA, cmd);
}

/* Address is clocked in most significant byte first, as expected by the serial
 * flash style command set these devices implement.
 */
static void pnor_send_addr(uint32_t addr, uint8_t cycles)
{
    while (cycles--)
        pnor_bus_write(ADDR_AREA, (uint8_t)(addr >> (cycles * 8)));
}

static uint8_t pnor_read_data_byte()
{
    return pnor_bus_read(DATA_AREA);
}

static void pnor_write_data_byte(uint8_t val)
{
    pnor_bus_write(DATA_AREA, val);
}

/* Single shot, non blocking. Called from the main loop while a write is in
 * flight, so it must never spin.
 */
static uint32_t pnor_read_status()
{
    uint8_t data;

    if (pnor_conf.status_cmd == UNDEFINED_CMD)
        return FLASH_STATUS_READY;

    pnor_send_cmd(pnor_conf.status_cmd);
    data = pnor_read_data_byte();

    if (!!(data & (1 << pnor_conf.busy_bit)) == !!pnor_conf.busy_state)
        return FLASH_STATUS_BUSY;

    return FLASH_STATUS_READY;
}

/* Blocking variant used by the synchronous erase path */
static uint32_t pnor_get_status()
{
    uint32_t status, timeout = 0x1000000;

    status = pnor_read_status();

    while (status == FLASH_STATUS_BUSY && timeout)
    {
        status = pnor_read_status();
        timeout--;
    }

    if (!timeout)
        status = FLASH_STATUS_TIMEOUT;

    return status;
}

static void pnor_read_id(chip_id_t *chip_id)
{
    if (pnor_conf.read_id_cmd == UNDEFINED_CMD)
    {
        chip_id->maker_id = 0;
        chip_id->device_id = 0;
        chip_id->third_id = 0;
        chip_id->fourth_id = 0;
        chip_id->fifth_id = 0;
        return;
    }

    pnor_send_cmd(pnor_conf.read_id_cmd);
    /* JEDEC read ID (0x9F) takes no address, 0x90 style takes one cycle */
    pnor_send_addr(0, pnor_conf.id_addr_cycles);

    chip_id->maker_id  = pnor_read_data_byte();
    chip_id->device_id = pnor_read_data_byte();
    chip_id->third_id  = pnor_read_data_byte();
    chip_id->fourth_id = pnor_read_data_byte();
    chip_id->fifth_id  = pnor_read_data_byte();
}

static void pnor_write_enable()
{
    if (pnor_conf.write_en_cmd == UNDEFINED_CMD)
        return;

    pnor_send_cmd(pnor_conf.write_en_cmd);
}

static void pnor_write_page_async(uint8_t *buf, uint32_t page,
    uint32_t page_size)
{
    uint32_t i;
    uint32_t addr = page << pnor_conf.page_offset;

    if (pnor_conf.write_cmd == UNDEFINED_CMD)
        return;

    pnor_write_enable();

    pnor_send_cmd(pnor_conf.write_cmd);
    pnor_send_addr(addr, pnor_conf.addr_cycles);

    for (i = 0; i < page_size; i++)
        pnor_write_data_byte(buf[i]);
}

static uint32_t pnor_read_data(uint8_t *buf, uint32_t page,
    uint32_t offset, uint32_t data_size)
{
    uint32_t i, addr = (page << pnor_conf.page_offset) + offset;

    if (pnor_conf.read_cmd == UNDEFINED_CMD)
        return FLASH_STATUS_INVALID_CMD;

    pnor_send_cmd(pnor_conf.read_cmd);
    pnor_send_addr(addr, pnor_conf.addr_cycles);

    for (i = 0; i < data_size; i++)
        buf[i] = pnor_read_data_byte();

    return FLASH_STATUS_READY;
}

static uint32_t pnor_read_page(uint8_t *buf, uint32_t page, uint32_t page_size)
{
    return pnor_read_data(buf, page, 0, page_size);
}

static uint32_t pnor_read_spare_data(uint8_t *buf, uint32_t page,
    uint32_t offset, uint32_t data_size)
{
    (void)buf;
    (void)page;
    (void)offset;
    (void)data_size;

    return FLASH_STATUS_INVALID_CMD;
}

static uint32_t pnor_erase_block(uint32_t page)
{
    uint32_t addr = page << pnor_conf.page_offset;

    if (pnor_conf.erase_cmd == UNDEFINED_CMD)
        return FLASH_STATUS_INVALID_CMD;

    pnor_write_enable();

    pnor_send_cmd(pnor_conf.erase_cmd);
    pnor_send_addr(addr, pnor_conf.addr_cycles);

    return pnor_get_status();
}

static bool pnor_is_bb_supported()
{
    return false;
}

flash_hal_t hal_parallel_nor =
{
    .init = pnor_init,
    .uninit = pnor_uninit,
    .read_id = pnor_read_id,
    .erase_block = pnor_erase_block,
    .read_page = pnor_read_page,
    .read_spare_data = pnor_read_spare_data,
    .write_page_async = pnor_write_page_async,
    .read_status = pnor_read_status,
    .is_bb_supported = pnor_is_bb_supported
};
