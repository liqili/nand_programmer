#  Copyright (C) 2020 NANDO authors
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 3.
#
#  Renode model of a Skyhigh/Cypress S34ML01G1 SLC NAND flash sitting on FSMC
#  NAND bank 2, per datasheet 002-00676 Rev *W.
#
#  Same state machine as test/fake_nand_chip.c. The geometry and the address
#  cycle map are properties of the silicon, so a wrong chip database entry
#  shows up here as a reported violation rather than as plausible data.
#
#  Offsets within the peripheral window:
#    0x00000  data area
#    0x10000  command area (A16 = CLE)
#    0x20000  address area (A17 = ALE)

#  The request object exposes PascalCase members: IsInit, IsRead, IsWrite,
#  Offset, Value, Length.

# State is set up lazily, because Init() is invoked from EnsureInit() during
# the first access rather than ahead of it.
try:
    _ready
except NameError:
    PAGE_MAIN = 2048
    PAGE_SPARE = 64
    PAGE_TOTAL = PAGE_MAIN + PAGE_SPARE
    PAGES_PER_BLOCK = 64

    # Datasheet Table 4: 1st/2nd column, 3rd/4th row
    COL_CYCLES = 2
    ROW_CYCLES = 2

    ID_BYTES = [0x01, 0xF1, 0x00, 0x1D]

    CMD_READ_1 = 0x00
    CMD_READ_2 = 0x30
    CMD_PROGRAM_1 = 0x80
    CMD_PROGRAM_2 = 0x10
    CMD_ERASE_1 = 0x60
    CMD_ERASE_2 = 0xD0
    CMD_STATUS = 0x70
    CMD_READ_ID = 0x90
    CMD_RESET = 0xFF

    STATUS_READY = 0x40
    BUSY_READS = 2

    DATA_AREA = 0x00000
    CMD_AREA = 0x10000
    ADDR_AREA = 0x20000

    pages = {}
    page_buf = [0xFF] * PAGE_TOTAL

    phase = "idle"
    col = 0
    row = 0
    addr_cnt = 0
    ptr = 0
    busy_left = 0
    id_idx = 0

    violations = []
    widths = {}

    _ready = True

    print("[chip] S34ML01G1 model ready, expects %d col + %d row cycles"
          % (COL_CYCLES, ROW_CYCLES))


def _say(msg):
    print(msg)


def _note_width(kind, n):
    key = "%s%d" % (kind, n)
    if key not in widths:
        widths[key] = True
        _say("[chip] %s access is %d byte(s) wide" % (kind, n))


def _violate(msg):
    if msg not in violations:
        violations.append(msg)
        _say("[chip] VIOLATION: %s" % msg)


def _page(n):
    if n not in pages:
        pages[n] = [0xFF] * PAGE_TOTAL
    return pages[n]


def _read_byte():
    global busy_left, id_idx, ptr

    if phase == "status":
        if busy_left > 0:
            busy_left -= 1
            return 0x00
        return STATUS_READY

    if phase == "read_id":
        if addr_cnt != 1:
            _violate("read ID needs exactly one address cycle, got %d"
                     % addr_cnt)
        b = ID_BYTES[id_idx] if id_idx < len(ID_BYTES) else 0xFF
        id_idx += 1
        return b

    if phase == "read_data":
        if ptr < PAGE_TOTAL:
            b = page_buf[ptr]
            ptr += 1
            return b
        return 0xFF

    _violate("data read outside a read, read ID or status command")

    return 0xFF


def _check_cycles(expected, what):
    if addr_cnt != expected:
        _violate("%s: got %d address cycles, device latches %d"
                 % (what, addr_cnt, expected))


if request.IsInit:
    pass

elif request.IsWrite:
    off = request.Offset
    val = request.Value & 0xFF

    _note_width("write", request.Length)

    if request.Length != 1:
        _violate("wide write of %d bytes, the driver should write bytes"
                 % request.Length)

    if off >= ADDR_AREA:
        # ---- address cycle ----
        if phase in ("read_addr", "program_addr"):
            if addr_cnt < COL_CYCLES:
                col |= val << (8 * addr_cnt)
            elif addr_cnt < COL_CYCLES + ROW_CYCLES:
                row |= val << (8 * (addr_cnt - COL_CYCLES))
        elif phase == "erase_addr":
            if addr_cnt < ROW_CYCLES:
                row |= val << (8 * addr_cnt)
        elif phase == "read_id":
            pass
        else:
            _violate("address cycle outside a command that takes one")

        addr_cnt += 1

        if phase == "program_addr" and addr_cnt == COL_CYCLES + ROW_CYCLES:
            ptr = col
            page_buf = [0xFF] * PAGE_TOTAL
            phase = "program_data"

    elif off >= CMD_AREA:
        # ---- command ----
        if val == CMD_RESET:
            phase = "idle"
            addr_cnt = 0
            busy_left = BUSY_READS
            _say("[chip] reset")

        elif val == CMD_READ_1:
            phase, col, row, addr_cnt = "read_addr", 0, 0, 0

        elif val == CMD_READ_2:
            if phase != "read_addr":
                _violate("read confirm without a read setup")
            else:
                _check_cycles(COL_CYCLES + ROW_CYCLES, "page read")
                page_buf = list(_page(row))
                ptr = col
                phase = "read_data"
                _say("[chip] read page row=%d col=%d (%d address cycles)"
                     % (row, col, addr_cnt))

        elif val == CMD_PROGRAM_1:
            phase, col, row, addr_cnt = "program_addr", 0, 0, 0

        elif val == CMD_PROGRAM_2:
            if phase not in ("program_data", "program_addr"):
                _violate("program confirm without a program setup")
            else:
                tgt = _page(row)
                for i in range(PAGE_TOTAL):
                    tgt[i] &= page_buf[i]
                busy_left = BUSY_READS
                phase = "status"
                _say("[chip] program page row=%d" % row)

        elif val == CMD_ERASE_1:
            phase, col, row, addr_cnt = "erase_addr", 0, 0, 0

        elif val == CMD_ERASE_2:
            if phase != "erase_addr":
                _violate("erase confirm without an erase setup")
            else:
                _check_cycles(ROW_CYCLES, "block erase")
                base = row - (row % PAGES_PER_BLOCK)
                for i in range(PAGES_PER_BLOCK):
                    pages[base + i] = [0xFF] * PAGE_TOTAL
                busy_left = BUSY_READS
                phase = "status"
                _say("[chip] erase block at row=%d (%d address cycles)"
                     % (row, addr_cnt))

        elif val == CMD_STATUS:
            phase = "status"

        elif val == CMD_READ_ID:
            phase, col, row, addr_cnt = "read_id", 0, 0, 0
            id_idx = 0
            _say("[chip] read ID")

        else:
            _violate("unsupported command 0x%02X" % val)

    else:
        # ---- data ----
        if phase != "program_data":
            _violate("data written outside a program command")
        elif ptr < PAGE_TOTAL:
            page_buf[ptr] &= val
            ptr += 1

elif request.IsRead:
    off = request.Offset
    n = request.Length

    _note_width("read", n)

    if off < CMD_AREA:
        # The FSMC drives an 8 bit bus, so a wider CPU read becomes that many
        # consecutive byte accesses on the device. read_id relies on this.
        v = 0
        for _i in range(n):
            v |= _read_byte() << (8 * _i)
        request.Value = v
    else:
        _violate("read from the command or address area")
        request.Value = 0xFF
