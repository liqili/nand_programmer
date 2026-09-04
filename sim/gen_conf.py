#!/usr/bin/env python3
#  Copyright (C) 2020 NANDO authors
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 3.
#
#  Emits the HAL configuration for one chip straight out of the chip database
#  CSV, so the simulation exercises the shipped database row rather than a
#  hand written copy of it.

import sys

CSV = sys.argv[1] if len(sys.argv) > 1 else "../qt/nando_parallel_chip_db.csv"
NAME = sys.argv[2] if len(sys.argv) > 2 else "S34ML01G1"
OUT = sys.argv[3] if len(sys.argv) > 3 else "sim_chip_conf.h"

# CSV column name -> C macro
WANTED = [
    ("page size", "PAGE_SIZE"),
    ("block size", "BLOCK_SIZE"),
    ("total size", "TOTAL_SIZE"),
    ("spare size", "SPARE_SIZE"),
    ("bad block mark off.", "BB_MARK_OFF"),
    ("row cycles", "ROW_CYCLES"),
    ("col. cycles", "COL_CYCLES"),
    ("read 1 cycle com.", "READ1_CMD"),
    ("read 2 cycle com.", "READ2_CMD"),
    ("read spare com.", "READ_SPARE_CMD"),
    ("read ID com.", "READ_ID_CMD"),
    ("reset com.", "RESET_CMD"),
    ("write 1 cycle com.", "WRITE1_CMD"),
    ("write 2 cycle com.", "WRITE2_CMD"),
    ("erase 1 cycle com.", "ERASE1_CMD"),
    ("erase 2 cycle com.", "ERASE2_CMD"),
    ("status com.", "STATUS_CMD"),
    ("set feat. com.", "SET_FEATURES_CMD"),
    ("en. ECC addr", "ENABLE_ECC_ADDR"),
    ("en. ECC val.", "ENABLE_ECC_VALUE"),
    ("dis. ECC val.", "DISABLE_ECC_VALUE"),
    ("ID1", "ID1"),
    ("ID2", "ID2"),
    ("ID3", "ID3"),
    ("ID4", "ID4"),
    ("ID5", "ID5"),
]

hdr = None
row = None

with open(CSV) as f:
    for line in f:
        line = line.rstrip("\n")
        if not line.strip():
            continue
        if line.startswith("#"):
            hdr = [c.strip() for c in line.lstrip("#").split(",")]
            continue
        fields = [c.strip() for c in line.split(",")]
        if fields and fields[0] == NAME:
            row = fields
            break

if hdr is None:
    sys.exit("no header line in %s" % CSV)
if row is None:
    sys.exit("%s not found in %s" % (NAME, CSV))
if len(row) != len(hdr):
    sys.exit("%s has %d fields, header has %d" % (NAME, len(row), len(hdr)))

lookup = dict(zip(hdr, row))

lines = [
    "/* Generated from %s by gen_conf.py. Do not edit. */" % CSV,
    "",
    "#ifndef _SIM_CHIP_CONF_H_",
    "#define _SIM_CHIP_CONF_H_",
    "",
    '#define SIM_CHIP_NAME "%s"' % NAME,
    "",
]

for col, macro in WANTED:
    if col not in lookup:
        sys.exit("column %r missing from header" % col)
    v = lookup[col]
    # '-' is ChipDb::paramNotDefValue, the firmware sees it as 0xFF
    lines.append("#define SIM_%-20s %s" % (macro, "0xFF" if v == "-" else v))

lines += ["", "#endif /* _SIM_CHIP_CONF_H_ */", ""]

with open(OUT, "w") as f:
    f.write("\n".join(lines))

print("%s: generated %s from row %s" % (OUT, NAME, CSV))
