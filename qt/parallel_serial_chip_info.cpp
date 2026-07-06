/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#include "parallel_serial_chip_info.h"

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
} ParallelSerialChipConf;

ParallelSerialChipInfo::ParallelSerialChipInfo()
{
    hal = CHIP_HAL_PARALLEL_SERIAL;
}

ParallelSerialChipInfo::~ParallelSerialChipInfo()
{
}

const QByteArray &ParallelSerialChipInfo::getHalConf()
{
    ParallelSerialChipConf conf;

    conf.page_offset = static_cast<uint8_t>(params[CHIP_PARAM_PAGE_OFF]);
    conf.read_cmd = static_cast<uint8_t>(params[CHIP_PARAM_READ_CMD]);
    conf.read_id_cmd = static_cast<uint8_t>(params[CHIP_PARAM_READ_ID_CMD]);
    conf.write_cmd = static_cast<uint8_t>(params[CHIP_PARAM_WRITE_CMD]);
    conf.write_en_cmd = static_cast<uint8_t>(params[CHIP_PARAM_WRITE_EN_CMD]);
    conf.erase_cmd = static_cast<uint8_t>(params[CHIP_PARAM_ERASE_CMD]);
    conf.status_cmd = static_cast<uint8_t>(params[CHIP_PARAM_STATUS_CMD]);
    conf.busy_bit = static_cast<uint8_t>(params[CHIP_PARAM_BUSY_BIT]);
    conf.busy_state = static_cast<uint8_t>(params[CHIP_PARAM_BUSY_STATE]);
    conf.freq = params[CHIP_PARAM_FREQ];
    conf.setup_time = static_cast<uint8_t>(params[CHIP_PARAM_SETUP_TIME]);
    conf.wait_setup_time = static_cast<uint8_t>(params[CHIP_PARAM_WAIT_SETUP_TIME]);
    conf.hold_setup_time = static_cast<uint8_t>(params[CHIP_PARAM_HOLD_SETUP_TIME]);
    conf.hi_z_setup_time = static_cast<uint8_t>(params[CHIP_PARAM_HI_Z_SETUP_TIME]);
    conf.clr_setup_time = static_cast<uint8_t>(params[CHIP_PARAM_CLR_SETUP_TIME]);
    conf.ar_setup_time = static_cast<uint8_t>(params[CHIP_PARAM_AR_SETUP_TIME]);

    halConf.clear();
    halConf.append(reinterpret_cast<const char *>(&conf), sizeof(conf));

    return halConf;
}

quint64 ParallelSerialChipInfo::getParam(uint32_t num)
{
    if (num >= CHIP_PARAM_NUM)
        return 0;

    return params[num];
}

int ParallelSerialChipInfo::setParam(uint32_t num, quint64 value)
{
    if (num >= CHIP_PARAM_NUM)
        return -1;

    params[num] = value;

    return 0;
}
