# ECC in NANDO — design

Status: **proposal, not implemented.** Covers how host-side ECC should behave
once wired into the read, write and verify paths.

The engine (`qt/ecc/`) and the settings dialog exist and are verified. What
follows is about the part that is missing — the data path — and about three
defects in the existing model that mainstream practice exposes.

---

## 1. Where we are

| Piece | State |
|---|---|
| `GaloisField`, `BchCodec`, `HammingCodec`, `RsCodec` | Done, tested |
| `EccScheme` (layout model, presets, validation) | Done |
| `EccEngine::encodePage()` / `decodePage()` | Done, 113/113 pages match the Broadcom reference encoder |
| `EccSettingsDialog` + `QSettings` persistence | Done |
| **Read / write / verify integration** | **Not started — `encodePage`/`decodePage` are called from nowhere** |

---

## 2. The reference model

Every serious NAND tool converges on the same abstractions. Linux MTD is the
clearest statement of them, and `nanddump` / `nandwrite` are the closest
analogue to what NANDO does. The patterns worth copying:

**Raw and cooked are separate access modes, always both available.**
MTD exposes `MTD_OPS_RAW` (bytes exactly as stored, ECC untouched) alongside
`MTD_OPS_PLACE_OOB` / `MTD_OPS_AUTO_OOB`. `nanddump --noecc` and
`nandwrite --noecc` surface it to users. A programmer that can only do one of
these is not usable for recovery work.

**ECC geometry is (step size, bytes per step, strength).**
`ecc.size` is 512 or 1024, `ecc.bytes` is the parity per step, `ecc.strength`
is bits correctable per step. NANDO calls these `sectorSize`, `eccBytes()` and
`t` — same concepts, different names.

**OOB is described as regions, not as one blob.**
`mtd_ooblayout_ops` splits OOB into an *ECC region* and a *free region*. The
free region is where the bad block marker and any filesystem metadata live.
Writing parity must never touch it.

**Corrections are counted and reported, not silently absorbed.**
`mtd->ecc_stats.corrected` and `.failed`. A read returns `-EUCLEAN` when
bitflips exceed `bitflip_threshold` — meaning "this still reads, but the block
is degrading" — and `-EBADMSG` when uncorrectable. The distinction between
*corrected but worrying* and *lost* is the whole point of having ECC in a
programmer.

**Erased pages are special-cased, with tolerance.**
An erased page is 0xFF everywhere including OOB. Parity over all-0xFF is not
0xFF for any of these algorithms, so a naive verifier reports every erased page
as uncorrectable. Linux has `nand_check_erased_ecc_chunk()`, which tolerates a
few zero bits before deciding a page is not erased.

---

## 3. Gaps and defects in what exists

Three of these are real bugs, not just missing features.

### 3.1 Parity can silently destroy the bad block marker — **defect**

`EccScheme::validate()` checks that parity fits inside the spare. It does not
check *where* it lands. The factory bad block marker lives at OOB byte 0 of the
page (bytes 0–1 on x16 parts). A scheme with `eccBitOffset = 0` — which the
generic `BCH-4 (512 B)`, `BCH-8`, `BCH-16`, `BCH-24` and `Reed-Solomon` presets
all use — puts parity directly on top of it for **step 0**.

Once that marker is overwritten, a factory-marked bad block becomes
indistinguishable from a good one, permanently. This is the highest-severity
item in the document.

Note the nuance: only step 0's OOB slice contains the page BBM. Steps 1..N may
use offset 0 freely. The guard is therefore per-step, not global.

### 3.2 Erased page detection is too strict — **defect**

`EccEngine::sectorIsErased()` requires the data to be *exactly* all-0xFF, and
does not look at the OOB at all. A single retention bitflip in an erased block
turns it into "not erased", the parity check then fails, and the page is
reported uncorrectable. On a used chip this produces false failures across
whole erased regions.

### 3.3 Host ECC and on-die ECC can both be on at once — **defect**

`SETTINGS_ENABLE_HW_ECC` asks the chip to run its internal engine (ONFI
`SET_FEATURES` 0x90). When that is on, the chip corrects internally and the OOB
layout is the vendor's, not ours. Applying host ECC on top computes parity over
bytes the chip is also managing. These must be mutually exclusive, and the UI
must say so rather than letting both be ticked.

### 3.4 Missing: raw write mode

There is a "Generate parity into the spare area" checkbox but no explicit
statement of what happens when it is off and ECC is on. The case matters: an
image produced by `brcm-nand-bch`, or a dump from another tool, **already
contains valid parity**. Regenerating it is wrong; writing it verbatim is
right.

### 3.5 Missing: syndrome page layout

The model assumes `[data0..dataN][oob0..oobN]` — OOB contiguous at the end of
the page, split into equal per-step slices. Some controllers interleave
instead: `[data0][ecc0][data1][ecc1]…` (`NAND_ECC_HW_SYNDROME`).

I propose **not** implementing this in v1, but naming it, because the layout
enum needs to exist from the start or retrofitting it later breaks the settings
format.

---

## 4. Proposed design

### 4.1 Operation modes

Replace the current implicit behaviour with an explicit three-state mode per
direction, mirroring `--noecc`:

| Direction | Mode | Behaviour |
|---|---|---|
| Read | `RAW` | Bytes exactly as stored. No parity check. Today's behaviour. |
| Read | `CHECK` | Verify parity, **do not modify data**. Report statistics. |
| Read | `CORRECT` | Verify and repair correctable errors in the output. |
| Write | `RAW` | File bytes go to the chip verbatim. Source already carries parity. |
| Write | `GENERATE` | Compute parity, write it into the ECC region only. Free OOB bytes, including the BBM, are preserved byte for byte. |

`CHECK` is the correct **default** for read. A programmer dump is evidence: it
should be byte-exact to the silicon unless the user asks otherwise, so that two
dumps can be diffed and wear can be detected. `CORRECT` exists for the separate
job of recovering a usable filesystem image.

### 4.2 Verify must stay raw

This one is easy to get wrong. `Verify` reads the chip back and compares it to
the file. If ECC correction is applied during that read, a page that was
**written badly but is still correctable** compares equal, and the write
failure is hidden.

Verify therefore always reads `RAW` and compares byte-exact, regardless of the
ECC read mode. ECC statistics may be reported alongside as extra information,
but they never affect the comparison.

### 4.3 Erased page policy

Replace the exact-0xFF test with a tolerance test over the whole step region
(data **and** its OOB slice):

```
zeros = popcount_of_zero_bits(step_data ++ step_oob)
if zeros == 0                 -> ERASED, clean
if zeros <= erasedTolerance   -> ERASED, report `zeros` bitflips
else                          -> programmed, run the codec
```

`erasedTolerance` defaults to the scheme's correction strength, which is what
Linux does. On write, an erased step is skipped — no parity is generated, so an
erased page stays fully erased.

### 4.4 Bad block marker guard

Add to `EccScheme::validate()`:

- Compute the byte range covered by parity within step 0's OOB slice.
- If it intersects byte 0 (or bytes 0–1 when the chip is x16), reject the
  scheme with a specific message.

The affected presets get their `eccBitOffset` moved off zero. For a 16-byte
slice with 7 parity bytes the conventional choice is offset 8 bytes (bit 64),
leaving bytes 0–7 free — that is where most controllers put it, and it is what
the Broadcom layouts already do by starting at bit 76.

This also means the spare map should draw the BBM byte distinctly, so the
collision is visible before it is committed.

### 4.5 Statistics and reporting

Aggregate across an operation, in MTD's vocabulary:

```
struct EccStats {
    quint64 stepsTotal, stepsClean, stepsCorrected, stepsUncorrectable,
            stepsErased;
    quint64 bitsCorrected;      // total flips repaired
    int     worstStep;          // highest flips in any single step
    quint64 firstFailPage;      // for the log
};
```

Reported at completion as one line, plus a warning when
`worstStep >= bitflipThreshold` (default: `strength - 1`), phrased as wear
rather than failure — "block degrading, consider re-writing" — because that is
the actionable meaning.

Uncorrectable pages are always logged with their page number, and honour the
existing "Warn when a page cannot be corrected" checkbox.

### 4.6 Pipeline and insertion points

Constraints found in the current code:

- `Writer` is already page-aware: it chunks on `pageSize`, which is the
  **extended** page size (2048 + 64) whenever "include spare area" is on.
- `Reader` is not: `rbuf->buf.insert(...)` appends raw USB payloads with no
  regard for page boundaries.
- ECC needs a whole `pageSize + spareSize` region at once.

Rather than make the two transport threads page-aware — which risks the NAND
path already proven on hardware — apply ECC at the **file ↔ buffer boundary in
`main_window`**, where bytes are contiguous and page alignment is known:

```
READ :  Reader -> SyncBuffer -> [EccPageStream::decode] -> workFile
WRITE:  source -> [image generator] -> generated image -> SyncBuffer -> Writer
```

The two directions are deliberately asymmetric. Reading is diagnostic and wants
statistics as it goes, so it streams. Writing must be exact and auditable, so it
materialises a complete image file first and then sends that file unchanged —
see §5.6. The write side therefore needs no stream adapter at all, and `Writer`
keeps working on a plain file exactly as it does today.

`EccPageStream` is a small adapter used on the **read** path. It accepts
arbitrary-sized chunks, buffers a partial trailing page across calls, and
processes only whole pages:

```cpp
class EccPageStream
{
public:
    void begin(const EccEngine *engine, int pageBytes, EccMode mode);
    // returns processed bytes ready to hand on; keeps a partial tail
    void push(const uint8_t *data, size_t len, QByteArray &out,
        EccStats &stats);
    // process whatever remains; a short final page is passed through untouched
    void end(QByteArray &out, EccStats &stats);
};
```

A trailing partial page is passed through unmodified rather than being padded,
so a truncated image is never silently altered.

`Reader` and `Writer` are not touched at all.

### 4.7 Dependencies and interlocks

| Requires | Reason |
|---|---|
| "Include spare area" on | Parity lives in spare; without it the host never receives those bytes. Already enforced in `rebindEcc()`. |
| Hardware/on-die ECC off | Both correcting the same bytes is meaningless. **To be added.** |
| Chip selected | Geometry needed to validate step count and spare budget. Already enforced. |

When an interlock blocks ECC, the operation proceeds **raw** and says so in the
log. It does not fail, and it does not silently pretend ECC ran.

### 4.8 Terminology

Align the UI with what the datasheets and MTD use, so the settings are
recognisable to anyone who has configured a NAND controller:

| Now | Proposed |
|---|---|
| Sector size | ECC step size |
| Strength t | Correction strength (bits) |
| Spare per sector | Spare bytes per step |
| Parity offset | ECC offset in spare |

`m` and the primitive polynomial stay as they are — they are BCH internals and
already correctly named.

---

## 5. Knowing what the supplied image actually is

### 5.1 The hazard this closes

`slotProgWrite()` currently does:

```cpp
areaSize = workFile.size();
if (areaSize % pageSize)
    areaSize = (areaSize / pageSize + 1) * pageSize;
```

It rounds up and never asks whether the file's layout matches the chip's. With
"include spare area" on, `pageSize` is 2112. Hand it a data-only image whose
pages are 2048 bytes and page 0 gets file bytes 0..2047 as data and file bytes
2048..2111 — which are page 1's data — as its spare. Every page after that
slides further out of step.

The result is a chip full of garbage, with bad block markers overwritten by
whatever data happened to land on them, and no warning at any point. Detecting
the layout is therefore a correctness fix, not only a convenience.

### 5.2 What can be inferred, and how reliably

Given page size `P`, spare size `Sp`, and a file of `S` bytes:

**Size divisibility** — cheap, and usually decisive.

| `S % P` | `S % (P+Sp)` | Conclusion |
|---|---|---|
| 0 | non-zero | Data only |
| non-zero | 0 | Page + spare |
| 0 | 0 | Ambiguous — needs content |
| non-zero | non-zero | Truncated or foreign; low confidence |

The ambiguous row is not rare: for 2048/64 both divide at every multiple of
67584 bytes, so real images do land on it.

**Content shape** — resolves the ambiguity. Assume page+spare, sample pages,
and look at the bytes that would be spare:

- Entirely `0xFF` → spare is present but blank.
- Mostly `0xFF`, with the non-`0xFF` bytes confined to the *same small
  contiguous offset range on every page* → spare containing ECC. The range is
  worth reporting: it is the parity region, and it tells the user where their
  controller puts it.
- Low `0xFF` ratio, non-`0xFF` bytes spread across the whole slice → this is
  ordinary data being mis-sliced. The hypothesis is wrong; the file is data only.

**Parity validation** — decisive when it succeeds. For each preset that fits the
geometry, bind the engine and decode the sampled pages. A scheme whose parity
actually checks out over several pages is not a coincidence: it identifies the
layout *and* the scheme together.

This is where having both Broadcom presets pays off — an image from
`brcm-nand-bch` matches the truncated variant, a dump from real silicon matches
the full one, and the probe can tell the user which.

Accept a page as matching when its steps come back clean **or corrected**; a
dump from a worn chip legitimately carries bitflips. Only a majority of
uncorrectable steps rules a scheme out.

### 5.3 Cost

Trying twelve presets against sixty-four pages would mean roughly a hundred
million field operations — several seconds, far too slow for a dialog that
appears on a button press. Three measures keep it under a second:

- Sample **8** non-erased pages for scheme trials, not 64.
- Skip presets whose parity region does not overlap the non-`0xFF` offsets
  found by the content pass. This prunes most of the list immediately.
- Abandon a preset at its first uncorrectable page.

Erased pages are skipped throughout — they carry no signal either way.

### 5.4 Result

```cpp
enum ImageLayout
{
    IMAGE_LAYOUT_UNKNOWN,
    IMAGE_LAYOUT_DATA_ONLY,     /* P byte pages, no spare        */
    IMAGE_LAYOUT_PAGE_SPARE,    /* (P + Sp) byte pages           */
};

struct ImageProbeResult
{
    ImageLayout layout;
    int      confidence;        /* 0..100                        */
    QString  reason;            /* shown verbatim in the dialog  */

    bool     sparePresent;
    bool     spareBlank;        /* present but all 0xFF          */
    int      eccRegionFirst;    /* detected parity byte range,   */
    int      eccRegionLast;     /*   -1 when nothing was found   */

    int      matchingPreset;    /* validated scheme, or -1       */
    int      pagesSampled;
    int      pagesValidated;

    quint64  fullPages;
    quint32  trailingBytes;     /* short final page, 0 when none */
};
```

### 5.5 What the user is asked

The probe runs when Write is pressed, before any transfer starts. It only
interrupts when there is a genuine decision:

**No spare area in the image** — the case in the request:

```
firmware.bin — 232,501 bytes

  Image     data only, 113 pages of 2048 bytes + 1077 trailing bytes
  Chip      2048 + 64 bytes per page

  This image has no spare area. The chip needs 64 bytes per page.

  ( ) Generate ECC   [ Broadcom BCH-4 (512 B)   v ]   [ Configure... ]
  ( ) Leave spare erased (0xFF) — no error correction
```

**Spare present, scheme identified** — no decision needed by default:

```
  Image     page + spare, 114 pages of 2048 + 64
  Spare     ECC present in bytes 10..15, verified as
            "Broadcom BCH-4, truncated parity" on 8 of 8 sampled pages

  (•) Write as is, preserving the existing ECC        [recommended]
  ( ) Regenerate ECC with [ ... v ]
```

**Spare present but blank** — offer to fill it. **Spare present, no scheme
matches** — default to writing as-is and say plainly that the content was not
recognised, rather than guessing.

**Layout contradicts the chip** (data-only image, spare included in transfer, or
the reverse) — this is the corruption case from §5.1. Block the write and
explain, rather than offering a choice.

### 5.6 Generating the spare area

**The tool generates a complete image file with ECC applied, then writes that
file to the chip.** Nothing is transformed in flight.

This is worth stating as a decision rather than an implementation detail,
because the alternative — expanding pages on their way to the device — is
tempting and worse:

- **Verify has a real reference.** A source image of 2048 byte pages cannot be
  compared byte for byte against a chip holding 2112 byte pages. Re-running
  generation during verify to reconstruct the expectation means two paths that
  can drift apart. With a generated file, verify is the byte-exact comparison
  that already exists.
- **The bytes that reached the chip exist on disk.** They can be hex dumped,
  diffed against a reference image, or fed to another tool when a device
  misbehaves. For a programmer this is the difference between a diagnosable
  failure and a mystery.
- **The write path does not change.** No stream adapter, no recomputed
  `areaSize` — the generated file's size *is* the transfer size.
- **A failed write can be retried** without regenerating.
- It is also how the reference encoder works: `brcm-nand-bch -i in -o out`,
  then flash `out`.

The transform:

```
for each P byte chunk of the source:        /* last chunk padded with 0xFF */
    page = chunk || 0xFF * Sp
    engine.encodePage(page)                 /* fills the parity region     */
    append page to the generated image      /* P + Sp bytes                */
```

A short trailing chunk is padded with `0xFF` before parity is computed, so the
final page is self-consistent rather than carrying parity over undefined bytes.

#### Where the file lives

A `QTemporaryFile` in the system temporary location, removed once the write
completes. Because the generated image is often worth keeping, the pre-write
dialog carries **"Keep the generated image"**, which writes it alongside the
source as `<source>.ecc.bin` instead. That also subsumes the standalone "save
an image with ECC" action — the same code path, minus the write.

Free space is checked against the expanded size before generation starts, so a
full disk fails cleanly rather than part way through.

#### Cost and progress

Generation is a full pass with a BCH encode per step. A 128 MB image is roughly
65536 pages of 4 steps — a few seconds, not instant. It is a distinct phase and
reports its own progress:

```
Generating image with ECC ...  45%
Writing ...                    12%
```

#### Self-check before the chip is touched

After generating, decode a sample of pages from the generated file and confirm
every step comes back clean. This is nearly free at 32 sampled pages and it
catches a misconfigured scheme — a parity offset that overlaps, an `m` too
small for the step size — *before* anything is written to a device rather than
after.

If the self-check fails the write is refused, since the fault is in the
configuration and writing would leave the chip in a state the tool itself
cannot read back.

#### Format

Source and generated image are both **raw binaries** — what the file path field
already reads, and what the Broadcom reference images use. The generated file
is a byte-for-byte picture of what will land on the chip: `P + Sp` bytes per
page, parity in place.

Intel HEX and S-record are out of scope. NANDO has no parser for either.

---

## 6. Bad block management

ECC and bad block handling touch the same bytes of the spare area, so the
existing behaviour needs checking rather than assuming. Most of it holds up.

### 6.1 What needs no change

**The bad block scan is firmware side and inherently raw.**
`np_read_bad_block_info_from_page()` reads the marker through the HAL directly
into `prog->page.buf`. Host ECC never sees it and cannot corrupt or
misinterpret it. The fallback path — `read_spare_data()` returning
`FLASH_STATUS_INVALID_CMD` on 2 KB page parts, then reading the whole
`page_size + spare_size` — is equally unaffected.

**Skipping bad blocks keeps the read stream page aligned.**
`Reader::handleBadBlock()` adjusts a progress counter and inserts no data, so a
skipped block contributes zero bytes to the output. A block is a whole number
of pages, so the stream stays page aligned across skips and `EccPageStream`
needs no special handling. This is worth stating because it is not obvious and
it would have been an awkward bug.

**Parity is position independent.** Skipping a bad block during a write shifts
the file to physical page mapping, but the codeword covers page content only —
no row or block address is mixed in — so parity computed in file order stays
valid wherever the page lands.

### 6.2 The marker offset is chip configurable — refines §3.1

`bb_mark_off` comes from the chip database (`bbMarkOffset`), and is not
necessarily zero. The guard proposed in §3.1 must therefore be written against
the chip's actual value, not against byte 0.

The marker sits at offset `bb_mark_off` within the **page's** spare area, so it
falls in step `bb_mark_off / oobSize`, at byte `bb_mark_off % oobSize` of that
step's slice. The guard is:

```
badStep = bb_mark_off / oobSize
badByte = bb_mark_off % oobSize
reject if parity region of step badStep covers badByte
```

A hardcoded "check byte 0 of step 0" would pass a scheme that clobbers a
marker at, say, offset 5.

### 6.3 ECC must not be checked inside bad blocks — **needs adjustment**

A bad block's contents are unreliable by definition, and its marker byte is
`0x00`. In layouts where the codeword covers the spare — Broadcom BCH-4 covers
bytes 0 to 9, which includes the marker — a bad block's parity can never check
out, because the marker was written after any parity was computed.

Reading a chip that has bad blocks with ECC enabled would therefore fill the
log with uncorrectable errors that mean nothing.

`Reader::handleBadBlock()` already receives address and size for every bad
block encountered. Feed that to the ECC statistics so failures inside those
spans are counted as `stepsInBadBlock` rather than `stepsUncorrectable`, and
excluded from the "block degrading" warning. When bad block skipping is off and
bad blocks are read normally, this is the only thing preventing a misleading
report.

### 6.4 Writing a foreign dump can mark good blocks bad — **needs adjustment**

Not ECC specific, but the image probe of §5 is now the right place to catch it.

A full chip dump taken from a device that had bad blocks contains `0x00` at the
marker offset for those blocks. Writing that image to a *different* chip stamps
the first device's bad block pattern onto the second one's good blocks. The
markers are in the spare area, so they are written verbatim, and the result is
permanent.

The probe already walks the spare area. Have it also count pages whose byte at
`bb_mark_off` is not `0xFF`, and warn before writing:

```
  This image contains 7 bad block markers from the device it was
  taken from. Writing it will mark those blocks bad on this chip.

  ( ) Write markers as they are
  (•) Clear markers to 0xFF while generating       [recommended]
```

Clearing is the safe default: the destination chip's own factory markers are
what matter, and they are already on it.

### 6.5 Generated images never invent markers

The generator fills the spare with `0xFF` before computing parity, so the
marker position reads as a good block. Combined with the guard of §6.2, parity
is never written over it. An image NANDO generates cannot mark a block bad.

### 6.6 Pre-existing gap: no marking on failure

`nand_bad_block_table_add()` is called only during the scan, to populate the
in-RAM table. Nothing marks a block bad **on the chip** after a failed program
or erase, which is what the ONFI specification asks a host to do and what
`nandwrite --markbad` provides.

This is outside the scope of ECC work and is not proposed here, but it is worth
recording: once ECC is reporting per block bitflip counts, the information
needed to decide a block should be retired is available for the first time.

---

## 7. What lands in the file

The decision worth making explicitly, because it is not reversible for a user
who overwrites their only dump.

**Read with ECC produces the same bytes as read with ECC off, unless the user
selects `CORRECT`.** Parity is checked and reported; the file is a faithful
image of the silicon. This matches how `nanddump` behaves without `--noecc`
for the OOB, and it is what makes a dump usable as evidence.

`CORRECT` is offered, clearly labelled as modifying the data, for the recovery
case.

A generated image (§5.6) uses the same format, so it round-trips: an image
NANDO generates can be read back, verified, or written again without further
processing.

We keep the current interleaved `page + spare` on-disk format. It is what
NANDO already produces with "include spare area", it round-trips through the
write path unchanged, and it is what the Broadcom reference images use — which
is what let the engine be validated against them.

---

## 8. Implementation order

Each unit compiles and is verifiable on its own.

1. **Defect fixes in the engine** — bad block marker guard driven by the chip's
   `bb_mark_off` (§6.2), erased-page tolerance, preset offsets moved off byte 0.
   No UI, no data path. Testable against the golden image, which must still
   match 113/113.
2. **`EccStats` + `EccPageStream`** with host tests: chunk-boundary splitting,
   partial tails, erased pages, injected errors.
3. **Read path** — `CHECK` and `CORRECT`, statistics in the log, bad block
   spans attributed separately (§6.3). Non-destructive: safe to point at a real
   chip immediately.
4. **Image probe** (§5) with host tests over synthesised images: data only,
   page + spare with ECC, blank spare, ambiguous sizes, truncated tails, and
   foreign bad block markers (§6.4). Pure analysis, touches nothing.
5. **Layout guard** — block the mismatched write of §5.1. Small, and it closes
   the corruption hazard before any ECC write path exists.
6. **Image generator** — source file to generated image with ECC, plus the
   sampled self-check, "keep the generated image" and free-space check. A pure
   file to file transform, testable against the Broadcom golden pair: feeding
   it `cfe_rt-ac68u-r6250.bin` must reproduce `...-ecc.bin` exactly.
7. **Write path** — send the generated image, or the source unchanged in `RAW`.
   Gated behind a read path confirmed against a real device first.
8. **Verify path** — byte-exact against whichever image was actually written,
   generated or source. ECC stats reported alongside but never affecting the
   comparison.
9. **UI** — the pre-write dialog of §5.5, generation progress phase, mode radio
   buttons per direction, on-die ECC interlock, BBM byte drawn in the spare
   map, terminology rename.
10. **Docs** — `README.md` and `CLAUDE.md`.

Steps 1–5 are the useful milestone: they make ECC diagnostic and close
the corruption hazard, without adding any path that writes parity to a device.

---

## 9. Risks

**Writing parity to a real chip is the only destructive operation here.** The
engine is verified against a reference image, but a wrong `eccBitOffset` writes
into the free OOB region, and if that region holds the BBM the damage is
permanent. Mitigations: the guard in §4.4, preserving free bytes byte-for-byte
in `writeParity()` (already the case), and shipping the read path first.

**The generic BCH/RS presets are unverified against real silicon.** Only the
three Broadcom layouts are validated, against a tool rather than a device.
The others follow conventional parameters but should be treated as starting
points, and the dialog should not imply otherwise.

**Parity truncation is a compatibility hack.** `truncateTopBits` reproduces a
quirk of one tool. It weakens the code — 48 stored bits instead of 52 — and is
correct only for images made by that tool. It should stay visibly labelled as
a compatibility option, not a tuning knob.

---

## 10. Open questions

1. **Correct-on-read default** — I propose `CHECK`. If the expected use is
   filesystem recovery rather than device forensics, `CORRECT` may serve
   better. This changes what users get by default and is worth a decision.
2. **Syndrome layout** — worth building, or is the packed layout enough for the
   parts this programmer targets?
3. **Per-chip ECC schemes** — should the chosen scheme be remembered per chip
   in the CSV database, rather than being one global setting? That is closer to
   how a production programmer works, but it changes the database format.
