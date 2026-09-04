/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#define SETTINGS_ORGANIZATION_NAME "NANDO"
#define SETTINGS_APPLICATION_NAME "NANDO"

#define SETTINGS_PROGRAMMER_SECTION "programmer/"
#define SETTINGS_GUI_SECTION "GUI/"
#define SETTINGS_USB_DEV_NAME SETTINGS_PROGRAMMER_SECTION "usb_dev_name"
#define SETTINGS_SKIP_BAD_BLOCKS SETTINGS_PROGRAMMER_SECTION "skip_bad_blocks"
#define SETTINGS_INCLUDE_SPARE_AREA SETTINGS_PROGRAMMER_SECTION \
    "include_spare_area"
#define SETTINGS_ENABLE_HW_ECC SETTINGS_PROGRAMMER_SECTION \
    "enable_hw_ecc"
#define SETTINGS_ENABLE_ALERT SETTINGS_GUI_SECTION "enable_alert"
#define SETTINGS_WORK_FILE_PATH SETTINGS_GUI_SECTION "work_file_path"

/* Software ECC applied by the host application. This is separate from
 * SETTINGS_ENABLE_HW_ECC, which asks the chip to use its own on-die engine.
 */
#define SETTINGS_ECC_SECTION "ecc/"
#define SETTINGS_ECC_ENABLED SETTINGS_ECC_SECTION "enabled"
#define SETTINGS_ECC_ALGO SETTINGS_ECC_SECTION "algorithm"
#define SETTINGS_ECC_SECTOR_SIZE SETTINGS_ECC_SECTION "sector_size"
#define SETTINGS_ECC_OOB_SIZE SETTINGS_ECC_SECTION "oob_size"
#define SETTINGS_ECC_BIT_OFFSET SETTINGS_ECC_SECTION "bit_offset"
#define SETTINGS_ECC_COVER_SPARE SETTINGS_ECC_SECTION "cover_spare"
#define SETTINGS_ECC_M SETTINGS_ECC_SECTION "field_order"
#define SETTINGS_ECC_T SETTINGS_ECC_SECTION "strength"
#define SETTINGS_ECC_PRIM_POLY SETTINGS_ECC_SECTION "prim_poly"
#define SETTINGS_ECC_TRUNCATE SETTINGS_ECC_SECTION "truncate_top_bits"
#define SETTINGS_ECC_CORRECT_ON_READ SETTINGS_ECC_SECTION "correct_on_read"
#define SETTINGS_ECC_GENERATE_ON_WRITE SETTINGS_ECC_SECTION \
    "generate_on_write"
#define SETTINGS_ECC_WARN_UNCORRECTABLE SETTINGS_ECC_SECTION \
    "warn_uncorrectable"

#endif // SETTINGS_H
