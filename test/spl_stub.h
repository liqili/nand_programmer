/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *
 *  Host test build support for the flash drivers.
 *
 *  Stands in for <stm32f10x.h> so a driver can be compiled and exercised
 *  natively. SPL peripheral calls are captured rather than executed, and the
 *  bus primitives are provided by the corresponding flash chip model.
 */

#ifndef _SPL_STUB_H_
#define _SPL_STUB_H_

#include <stdint.h>
#include <stdio.h>

/* Bus primitives, implemented by the chip models */
void pnor_bus_write(uint32_t area, uint8_t val);
uint8_t pnor_bus_read(uint32_t area);
void nand_bus_write(uint32_t area, uint8_t val);
uint8_t nand_bus_read(uint32_t area);
uint32_t nand_bus_read32(uint32_t area, uint32_t index);

/* ---- captured SPL state ------------------------------------------------ */

typedef struct
{
    int gpio_init_calls;
    int fsmc_clock_enabled;
    int fsmc_bank_enabled;
    int fsmc_deinit_calls;

    uint32_t wait_feature;
    uint32_t data_width;
    uint32_t ecc;
    uint32_t tclr_setup_time;
    uint32_t tar_setup_time;
    uint32_t setup_time;
    uint32_t wait_setup_time;
    uint32_t hold_setup_time;
    uint32_t hi_z_setup_time;
} spl_capture_t;

extern spl_capture_t spl_capture;

/* ---- minimal SPL surface used by the driver ---------------------------- */

typedef enum { DISABLE = 0, ENABLE = 1 } FunctionalState;

#define GPIO_Pin_0   0x0001
#define GPIO_Pin_1   0x0002
#define GPIO_Pin_4   0x0010
#define GPIO_Pin_5   0x0020
#define GPIO_Pin_6   0x0040
#define GPIO_Pin_7   0x0080
#define GPIO_Pin_8   0x0100
#define GPIO_Pin_9   0x0200
#define GPIO_Pin_10  0x0400
#define GPIO_Pin_11  0x0800
#define GPIO_Pin_12  0x1000
#define GPIO_Pin_14  0x4000
#define GPIO_Pin_15  0x8000

#define GPIO_Speed_50MHz 3
#define GPIO_Mode_AF_PP  0x18
#define GPIO_Mode_IPU    0x48

#define GPIOD ((void *)1)
#define GPIOE ((void *)2)

#define RCC_APB2Periph_GPIOD 0x20
#define RCC_APB2Periph_GPIOE 0x40
#define RCC_APB2Periph_GPIOF 0x80
#define RCC_APB2Periph_GPIOG 0x100
#define RCC_AHBPeriph_FSMC   0x100

#define FSMC_Bank2_NAND 0x10

#define FSMC_Waitfeature_Disable 0x00
#define FSMC_Waitfeature_Enable  0x02
#define FSMC_MemoryDataWidth_8b  0x00
#define FSMC_ECC_Disable         0x00
#define FSMC_ECC_Enable          0x40
#define FSMC_ECCPageSize_2048Bytes 0x60000

typedef struct
{
    uint16_t GPIO_Pin;
    uint32_t GPIO_Speed;
    uint32_t GPIO_Mode;
} GPIO_InitTypeDef;

typedef struct
{
    uint32_t FSMC_SetupTime;
    uint32_t FSMC_WaitSetupTime;
    uint32_t FSMC_HoldSetupTime;
    uint32_t FSMC_HiZSetupTime;
} FSMC_NAND_PCCARDTimingInitTypeDef;

typedef struct
{
    uint32_t FSMC_Bank;
    uint32_t FSMC_Waitfeature;
    uint32_t FSMC_MemoryDataWidth;
    uint32_t FSMC_ECC;
    uint32_t FSMC_ECCPageSize;
    uint32_t FSMC_TCLRSetupTime;
    uint32_t FSMC_TARSetupTime;
    FSMC_NAND_PCCARDTimingInitTypeDef *FSMC_CommonSpaceTimingStruct;
    FSMC_NAND_PCCARDTimingInitTypeDef *FSMC_AttributeSpaceTimingStruct;
} FSMC_NANDInitTypeDef;

void GPIO_Init(void *port, GPIO_InitTypeDef *init);
void RCC_APB2PeriphClockCmd(uint32_t periph, FunctionalState state);
void RCC_AHBPeriphClockCmd(uint32_t periph, FunctionalState state);
void FSMC_NANDInit(FSMC_NANDInitTypeDef *init);
void FSMC_NANDCmd(uint32_t bank, FunctionalState state);
void FSMC_NANDDeInit(uint32_t bank);

#endif /* _SPL_STUB_H_ */
