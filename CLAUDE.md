# NANDO — Agent Operating Rules & Project Knowledge

---

# Part 1: Operating Rules

## CRITICAL: Memory and Context Constraints
- **Context Ceiling Alert:** We are running on a local hardware engine with tight context restraints. You MUST minimize token usage on every turn.
- **Do Not Ingest Broadly:** NEVER use sweeping repository searches, large grep passes, or read entire multi-thousand-line source files in a single step.
- **Aggressive Memory Clearing:** If your task requires a sequence of tool calls or file reviews, you MUST explicitly advise the user to run `/clear` between sub-tasks to flush the active token cache and protect the hardware buffer.

## Workflow: Sequence-Driven Task Splitting
When assigned any complex directive, implementation task, or architectural summary, follow this protocol:

1. **Phase 1: Discovery & Scoping (Max 1-2 file reads)**
   - Read ONLY high-level entry points (`CMakeLists.txt`, `Makefile`, `main.c`, device descriptors).
   - Stop and output a bulleted list of the exact sub-units needed.

2. **Phase 2: Micro-Chunk Execution**
   - Execute exactly ONE small atomic unit at a time.
   - Never chain more than 2 tool/shell calls in a single turn.

3. **Phase 3: Intermittent Verification & Handoff**
   - After each atomic change, trigger a targeted compilation check for just that build object.
   - Stop, report status, and request permission before the next block.

## File Inspection Constraints
- Any C source/header over 150 lines (especially generated STM32CubeHAL files) MUST be read in targeted line-range slices. Never view the whole file.
- Use `.gitignore`/`.claudeignore` to block build artifacts (`build/`, `obj/`, `.elf`, `.bin`, `.hex`, `.map`, `.o`, `.d`).

## Agent Policy
1. **Immutability by Default** — do not modify files unless explicitly requested and confirmed. Propose patches and await approval.
2. **Single Source of Truth** — always read current files from disk. Never assume the code matches an earlier snapshot.
3. **Minimal Prompt Context** — include only relevant snippets or explicit struct definitions.

## Runtime Checklist
1. Read the latest files relevant to the task.
2. Build context from current contents, not prior snapshots.
3. Do not modify files unless explicitly requested.
4. Provide patches or full file contents and await approval.
5. Include file paths when summarizing context.

## C & STM32 Firmware Guidelines
- **Build:** standard embedded toolchains (`make`, `cmake`). Always monitor `-Wall -Wextra`.
- **Code Placement Isolation:** in CubeMX-managed files, write only inside `/* USER CODE BEGIN ... */` markers.
- **Peripheral Layering:** keep HAL/LL hardware APIs distinct from application logic.
- **Defensive Resource Handling:** fixed-width types from `<stdint.h>`, `volatile` for hardware registers and ISR flags, explicit clock enables, bounds-checked register pools.
- Keep changes tightly scoped to avoid Flash/SRAM footprint growth.

## Bash Guidelines
- macOS-compatible Bash.
- No permission needed for read-only commands (builds, static analysis, file lookups).
- **NEVER use `rm`/`rm -rf`** — use `trash`. (Note: `trash` is not installed on this machine; move unwanted files to `$TMPDIR` instead.)

## Skill Registry and Layout
Skills live in `~/.claude/skills/` or `.claude/skills/`, one directory per slash command, each containing an uppercase `SKILL.md`:

```
.claude/skills/
└── <skill-name>/          # invoked as /<skill-name>
    └── SKILL.md
```

Each `SKILL.md` documents: **Description**, **Inputs & Deliverables**, **Execution Workflow**.

## Start-Up Behavior
Before non-trivial work, read `.github/analysis/lessons.md` and this file. Treat the workspace as immutable until changes are approved.

---

# Part 2: Project Knowledge

NANDO is an open-source flash programmer: an STM32F103 (HD, LQFP100) board with a
TSOP-48 ZIF socket, driven by a Qt5 desktop application over USB CDC.

## Repository layout

| Path | Contents |
|---|---|
| `firmware/programmer/` | Main application firmware (the programmer itself) |
| `firmware/bootloader/` | Bootloader |
| `firmware/libs/spl/` | ST Standard Peripheral Library + `startup_stm32f10x_hd.s` |
| `firmware/usb_cdc/` | USB CDC stack |
| `qt/` | Qt5 host application and the chip databases (`nando_*_chip_db.csv`) |
| `kicad/` | Schematic (`.sch`, KiCad 5) and PCB; `nand_programmator.net` is the netlist |
| `test/` | Host-native driver tests (no hardware needed) |
| `sim/` | Renode full-system simulation (Docker) |

## Firmware architecture

`nand_programmer.c` is the protocol engine. It receives command packets over USB
CDC and dispatches through a HAL vtable:

```c
static flash_hal_t *hal[] = { &hal_fsmc, &hal_spi, &hal_parallel_nor };
```

The index comes from the host (`conf_cmd->hal`) and **must** match
`ChipInfo::CHIP_HAL_*` in `qt/chip_info.h`:

| Index | HAL | Driver | Chip database |
|---|---|---|---|
| 0 | `CHIP_HAL_PARALLEL` | `fsmc_nand.c` | `nando_parallel_chip_db.csv` |
| 1 | `CHIP_HAL_SPI` | `spi_flash.c` | `nando_spi_chip_db.csv` |
| 2 | `CHIP_HAL_PARALLEL_SERIAL` | `parallel_nor_flash.c` | `nando_parallel_serial_chip_db.csv` |

`flash_hal_t` (in `flash_hal.h`) is the contract every driver implements.

### HAL contract rules (violating these causes real bugs)

- **`read_status` must be non-blocking.** It is polled from the main loop while a
  write is in flight; spinning inside it stalls the USB pump. Every driver keeps
  a *separate* blocking helper for the synchronous erase path
  (`nand_get_status`, `pnor_get_status`, `spi_flash_read_status`).
- **Return `FLASH_STATUS_*` from `flash_hal.h`**, never driver-local constants.
  `READY=0, BUSY=1, ERROR=2, TIMEOUT=3` — a local `TIMEOUT=2` silently becomes
  `ERROR` and gets reported to the host as a bad block.
- **`0xFF` means "command not supported."** Guard every configurable opcode.
- **`read_spare_data` returning `FLASH_STATUS_INVALID_CMD` is normal** for 2 KB-page
  NAND. `np_read_bad_block_info_from_page()` falls back to a full
  `page_size + spare_size` read and indexes `buf[page_size + bb_mark_off]`.
- `chip_id_t` has **five** ID bytes. Populate all of them.

### Host ↔ firmware wire structs

Each driver has a `__attribute__((packed))` config struct whose field order must
match its Qt counterpart **exactly**:

| Firmware | Qt |
|---|---|
| `fsmc_conf_t` in `fsmc_nand.c` | `parallel_chip_info.cpp` |
| `spi_conf_t` in `spi_flash.c` | `spi_chip_info.cpp` |
| `parallel_nor_conf_t` in `parallel_nor_flash.c` | `parallel_serial_chip_info.cpp` |

Prefer all-`uint8_t` structs. A wider member in the middle of a packed struct
forces alignment differences across the MSVC/MinGW ↔ ARM-GCC boundary.

## Hardware facts (from `kicad/nand_programmator.net`)

The ZIF socket exposes only `FSMC_D0..D15`, `FSMC_CLE`, `FSMC_ALE`, `FSMC_NCE2`,
`FSMC_NOE`, `FSMC_NWE`, `FSMC_NWAIT`. **There is no address bus**, so every
supported parallel device must speak a CLE/ALE-multiplexed protocol.

- FSMC **NAND bank 2** at `0x70000000`; `A16` = CLE, `A17` = ALE:
  - `0x70000000` data · `0x70010000` command · `0x70020000` address
- Data pins: `PD14,PD15,PD0,PD1` (D0–D3) and `PE7–PE10` (D4–D7); 8-bit only
- Control: `PD11` CLE, `PD12` ALE, `PD4` NOE, `PD5` NWE, `PD7` NCE2, `PD6` NWAIT
- USART1 debug console on `PA9`/`PA10` at 115200 (`uart.c`, `printf` routes here)
- Application 1 links at **`0x08004000`** (after the bootloader) — see
  `stm32_flash_1.ld`

## Chip database CSV format

Plain CSV with a `#` header row. **`-` means "not defined"**
(`ChipDb::paramNotDefValue`); the firmware sees `0xFF`. Used both for absent
commands and for "stop comparing here" in ID matching (e.g. a 4-byte-ID part
sets `ID5` to `-`).

`row cycles` / `col cycles` are literal counts of address bytes clocked out —
they are read straight into a `switch` in `fsmc_nand.c`. Getting them wrong
corrupts every read, program and erase. Always take them from the datasheet's
address-cycle map, and note that within one part family the 1 Gb device often
needs fewer row cycles than its 2/4 Gb siblings.

## Build commands

```bash
# Firmware (needs --specs=nosys.specs: syscalls.c lacks _exit/_kill/_getpid,
# which modern newlib requires. Pre-existing; not yet fixed in the Makefiles.)
cd firmware/programmer
make -f Makefile.linux CC=arm-none-eabi-gcc AR=arm-none-eabi-ar \
  OBJCOPY=arm-none-eabi-objcopy OBJDUMP=arm-none-eabi-objdump \
  SIZE=arm-none-eabi-size \
  CFLAGS="-mcpu=cortex-m3 -mthumb -DSTM32F10X_HD -DUSE_STDPERIPH_DRIVER \
          -Os -ffunction-sections -fdata-sections --specs=nosys.specs"
```

```bash
# Qt host app. CMake is the macOS path; qt.pro is used by the Linux/Windows
# release workflow. BOTH must list new sources — CMake globs, qmake does not.
cd qt && cmake -B build && cmake --build build
```

On macOS `qt.pro` has no Boost include path and `-Werror` trips over Boost's
deprecated `sprintf`; use the CMake build there.

## Testing

```bash
cd test && make          # host-native, no hardware
```

Drivers are compiled natively against behavioural chip models. Every bus access
goes through `*_bus_read`/`*_bus_write` primitives — inline volatile
dereferences on target, redirected to the model under `-DPNOR_HOST_TEST` /
`-DNAND_HOST_TEST`. `spl_stub.h/.c` stands in for `<stm32f10x.h>` and *captures*
what the driver configures, so FSMC settings are assertable.

The models enforce the protocol (address-cycle counts, write-enable ordering,
NOR/NAND program-only-clears-bits) and report violations rather than returning
plausible data. `test_fsmc_nand.c` reads its config from the real
`nando_parallel_chip_db.csv`, so it tests the shipped database row.

```bash
cd sim && docker-compose up    # Renode, writes to sim/out/ (readable from the repo)
```

Renode notes learned the hard way:
- The stock `platforms/cpus/stm32f103.repl` maps **FSMC bank 1 only**; bank 2 at
  `0x70000000` must be added (`sim/nando.repl`).
- There is **no RCC peripheral**. `RCC_CR` has a platform tag but `RCC_CFGR` does
  not, so `SystemInit()` spins waiting for `SWS`. Tag it with `0x0000000A`.
- `cpu VectorTableOffset` must be `0x08004000`, not `0x08000000` — the first LOAD
  segment starts at file offset 0, so `0x08000000` holds the ELF header.
- `Python.PythonPeripheral` request members are **PascalCase**: `IsInit`,
  `IsRead`, `IsWrite`, `Offset`, `Value`, `Length`. `Init()` runs lazily from
  `EnsureInit()` during the first access.
- Honour `request.Length`: the 8-bit FSMC turns a 32-bit CPU read into four
  consecutive byte fetches (`nand_read_id` depends on this).

## Known issues, not yet addressed

- `fsmc_nand.c` calls `nand_fsmc_init(fsmc_conf)` but the function is declared
  with empty parens and takes no argument. Harmless today (it reads the global),
  GCC is silent, clang warns, invalid under C2x.
- `nand_uninit()` is still a `TODO`, so switching away from NAND leaves FSMC
  bank 2 configured.
- Several 1 Gb entries (`K9F1G08U0D` and similar) specify 3 row cycles while
  16-bit row addressing suggests 2. Only `S34ML01G1` has been verified against
  its datasheet. Unaudited.
- The parallel-serial (`CHIP_HAL_PARALLEL_SERIAL`) path passes host tests but has
  **never been run on real hardware**, and its chip database is empty.
