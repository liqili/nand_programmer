/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *
 *  Captured SPL peripheral calls, shared by the driver test binaries.
 */

#include "spl_stub.h"

spl_capture_t spl_capture;


void GPIO_Init(void *port, GPIO_InitTypeDef *init)
{
    (void)port;
    (void)init;
    spl_capture.gpio_init_calls++;
}

void RCC_APB2PeriphClockCmd(uint32_t periph, FunctionalState state)
{
    (void)periph;
    (void)state;
}

void RCC_AHBPeriphClockCmd(uint32_t periph, FunctionalState state)
{
    (void)periph;
    spl_capture.fsmc_clock_enabled = (state == ENABLE);
}

void FSMC_NANDInit(FSMC_NANDInitTypeDef *init)
{
    spl_capture.wait_feature = init->FSMC_Waitfeature;
    spl_capture.data_width = init->FSMC_MemoryDataWidth;
    spl_capture.ecc = init->FSMC_ECC;
    spl_capture.tclr_setup_time = init->FSMC_TCLRSetupTime;
    spl_capture.tar_setup_time = init->FSMC_TARSetupTime;
    spl_capture.setup_time = init->FSMC_CommonSpaceTimingStruct->FSMC_SetupTime;
    spl_capture.wait_setup_time =
        init->FSMC_CommonSpaceTimingStruct->FSMC_WaitSetupTime;
    spl_capture.hold_setup_time =
        init->FSMC_CommonSpaceTimingStruct->FSMC_HoldSetupTime;
    spl_capture.hi_z_setup_time =
        init->FSMC_CommonSpaceTimingStruct->FSMC_HiZSetupTime;
}

void FSMC_NANDCmd(uint32_t bank, FunctionalState state)
{
    (void)bank;
    spl_capture.fsmc_bank_enabled = (state == ENABLE);
}

void FSMC_NANDDeInit(uint32_t bank)
{
    (void)bank;
    spl_capture.fsmc_deinit_calls++;
}
