/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#include "parallel_nor_flash.h"
#include "log.h"
#include <stm32f10x.h>

#define CMD_AREA                   (uint32_t)(1<<16)  /* A16 = CLE  high */
#define ADDR_AREA                  (uint32_t)(1<<17)  /* A17 = ALE high */

#define DATA_AREA                  ((uint32_t)0x00000000)

#define FSMC_Bank_NOR     FSMC_Bank2_NAND
#define Bank_NOR_ADDR     Bank2_NAND_ADDR
#define Bank2_NAND_ADDR    ((uint32_t)0x70000000)

#define FLASH_DUMMY_BYTE 0xA5

#define FLASH_READY 0
#define FLASH_BUSY  1
#define FLASH_TIMEOUT 2

/* 1st addressing cycle */
#define ADDR_1st_CYCLE(ADDR) (uint8_t)((ADDR)& 0xFF)
/* 2nd addressing cycle */
#define ADDR_2nd_CYCLE(ADDR) (uint8_t)(((ADDR)& 0xFF00) >> 8)
/* 3rd addressing cycle */
#define ADDR_3rd_CYCLE(ADDR) (uint8_t)(((ADDR)& 0xFF0000) >> 16)
/* 4th addressing cycle */
#define ADDR_4th_CYCLE(ADDR) (uint8_t)(((ADDR)& 0xFF000000) >> 24)

#define UNDEFINED_CMD 0xFF

typedef struct __attribute__((__packed__))
{
    uint8_t page_offset;
    uint8_t read_cmd;
    uint8_t read_id_cmd;
    uint8_t write_cmd;
    uint8_t write_en_cmd;
    uint8_t erase_cmd;
    uint8_t status_cmd;
    uint8_t busy_bit;
    uint8_t busy_state;
    uint32_t freq;
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

    /* CLE, ALE, D0->D3, NOE, NWE and NCE2 NOR pin configuration */
    gpio_init.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_14 | GPIO_Pin_15 |
        GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_7;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOD, &gpio_init);

    /* D4->D7 NOR pin configuration */
    gpio_init.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_Init(GPIOE, &gpio_init);

    /* NWAIT NOR pin configuration */
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
    fsmc_init.FSMC_Waitfeature = FSMC_Waitfeature_Enable;
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
    if (conf_size < sizeof(parallel_nor_conf_t))
        return -1;

    pnor_conf = *(parallel_nor_conf_t *)conf;

    pnor_gpio_init();
    pnor_fsmc_init();

    return 0;
}

static void pnor_uninit()
{
    /* TODO */
}

static uint32_t pnor_read_status()
{
    uint32_t status, timeout = 0x1000000;
    uint8_t data;

    *(__IO uint8_t *)(Bank_NOR_ADDR | CMD_AREA) = pnor_conf.status_cmd;
    data = *(__IO uint8_t *)(Bank_NOR_ADDR | DATA_AREA);

    if (pnor_conf.busy_state == 1 && (data & (1 << pnor_conf.busy_bit)))
        status = FLASH_BUSY;
    else if (pnor_conf.busy_state == 0 && !(data & (1 << pnor_conf.busy_bit)))
        status = FLASH_BUSY;
    else
        status = FLASH_READY;

    /* Wait for an operation to complete or a TIMEOUT to occur */
    while (status == FLASH_BUSY && timeout)
    {
        *(__IO uint8_t *)(Bank_NOR_ADDR | CMD_AREA) = pnor_conf.status_cmd;
        data = *(__IO uint8_t *)(Bank_NOR_ADDR | DATA_AREA);

        if (pnor_conf.busy_state == 1 && (data & (1 << pnor_conf.busy_bit)))
            status = FLASH_BUSY;
        else if (pnor_conf.busy_state == 0 && !(data & (1 << pnor_conf.busy_bit)))
            status = FLASH_BUSY;
        else
            status = FLASH_READY;

        timeout--;
    }

    if (!timeout)
        status = FLASH_TIMEOUT;

    return status;
}

static void pnor_read_id(chip_id_t *chip_id)
{
    *(__IO uint8_t *)(Bank_NOR_ADDR | CMD_AREA) = pnor_conf.read_id_cmd;
    *(__IO uint8_t *)(Bank_NOR_ADDR | ADDR_AREA) = 0x00;

    chip_id->maker_id  = *(__IO uint8_t *)(Bank_NOR_ADDR | DATA_AREA);
    chip_id->device_id = *(__IO uint8_t *)(Bank_NOR_ADDR | DATA_AREA);
    chip_id->third_id  = *(__IO uint8_t *)(Bank_NOR_ADDR | DATA_AREA);
    chip_id->fourth_id = *(__IO uint8_t *)(Bank_NOR_ADDR | DATA_AREA);
}

static void pnor_write_enable()
{
    if (pnor_conf.write_en_cmd == UNDEFINED_CMD)
        return;

    *(__IO uint8_t *)(Bank_NOR_ADDR | CMD_AREA) = pnor_conf.write_en_cmd;
}

static void pnor_write_page_async(uint8_t *buf, uint32_t page,
    uint32_t page_size)
{
    uint32_t i;
    uint32_t addr = page << pnor_conf.page_offset;

    pnor_write_enable();

    *(__IO uint8_t *)(Bank_NOR_ADDR | CMD_AREA) = pnor_conf.write_cmd;

    *(__IO uint8_t *)(Bank_NOR_ADDR | ADDR_AREA) = ADDR_3rd_CYCLE(addr);
    *(__IO uint8_t *)(Bank_NOR_ADDR | ADDR_AREA) = ADDR_2nd_CYCLE(addr);
    *(__IO uint8_t *)(Bank_NOR_ADDR | ADDR_AREA) = ADDR_1st_CYCLE(addr);

    for (i = 0; i < page_size; i++)
        *(__IO uint8_t *)(Bank_NOR_ADDR | DATA_AREA) = buf[i];
}

static uint32_t pnor_read_data(uint8_t *buf, uint32_t page,
    uint32_t page_offset, uint32_t data_size)
{
    uint32_t i, addr = (page << pnor_conf.page_offset) + page_offset;

    *(__IO uint8_t *)(Bank_NOR_ADDR | CMD_AREA) = pnor_conf.read_cmd;

    *(__IO uint8_t *)(Bank_NOR_ADDR | ADDR_AREA) = ADDR_3rd_CYCLE(addr);
    *(__IO uint8_t *)(Bank_NOR_ADDR | ADDR_AREA) = ADDR_2nd_CYCLE(addr);
    *(__IO uint8_t *)(Bank_NOR_ADDR | ADDR_AREA) = ADDR_1st_CYCLE(addr);

    for (i = 0; i < data_size; i++)
        buf[i] = *(__IO uint8_t *)(Bank_NOR_ADDR | DATA_AREA);

    return FLASH_READY;
}

static uint32_t pnor_read_page(uint8_t *buf, uint32_t page, uint32_t page_size)
{
    return pnor_read_data(buf, page, 0, page_size);
}

static uint32_t pnor_read_spare_data(uint8_t *buf, uint32_t page,
    uint32_t offset, uint32_t data_size)
{
    return FLASH_STATUS_INVALID_CMD;
}

static uint32_t pnor_erase_block(uint32_t page)
{
    uint32_t addr = page << pnor_conf.page_offset;

    pnor_write_enable();

    *(__IO uint8_t *)(Bank_NOR_ADDR | CMD_AREA) = pnor_conf.erase_cmd;

    *(__IO uint8_t *)(Bank_NOR_ADDR | ADDR_AREA) = ADDR_3rd_CYCLE(addr);
    *(__IO uint8_t *)(Bank_NOR_ADDR | ADDR_AREA) = ADDR_2nd_CYCLE(addr);
    *(__IO uint8_t *)(Bank_NOR_ADDR | ADDR_AREA) = ADDR_1st_CYCLE(addr);

    return pnor_read_status();
}

static inline bool pnor_is_bb_supported()
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
