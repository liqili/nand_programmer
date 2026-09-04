/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#ifndef ECC_STREAM_H
#define ECC_STREAM_H

#include "ecc_engine.h"

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

/* What to do with the pages passing through. */
enum EccMode
{
    ECC_MODE_RAW = 0,       /* pass bytes through untouched            */
    ECC_MODE_CHECK,         /* verify parity, report, do not modify    */
    ECC_MODE_CORRECT,       /* verify and repair what can be repaired  */
};

/* Running totals for one operation, in the vocabulary MTD uses: how many steps
 * were clean, how many needed repair, how many were beyond repair.
 *
 * Steps inside a block the programmer reported bad are counted separately.
 * Their parity can never check out - the bad block marker is written after any
 * parity was computed, and in layouts that cover the spare it is part of the
 * codeword - so folding them in with real failures would make every chip with
 * bad blocks look broken.
 */
struct EccStats
{
    uint64_t pages;
    uint64_t stepsTotal;
    uint64_t stepsClean;
    uint64_t stepsCorrected;
    uint64_t stepsUncorrectable;
    uint64_t stepsErased;
    uint64_t stepsInBadBlock;

    uint64_t errorsCorrected;   /* total repaired, bits or symbols       */
    int worstStep;              /* most errors seen in a single step     */
    uint64_t firstFailPage;     /* page index of the first failure       */
    bool haveFailure;

    EccStats();

    void reset();

    /* True when a step came close enough to the correction limit that the
     * block should be considered wearing out rather than healthy. Mirrors
     * MTD's bitflip_threshold.
     */
    bool wearWarning(const EccScheme &scheme) const;

    /* One line fit for the log. */
    std::string summary() const;
};

/* Feeds arbitrary sized chunks through the ECC engine a whole page at a time.
 *
 * The reader appends USB payloads with no regard for page boundaries, so
 * something has to hold a partial page back until the rest of it arrives.
 * A short page at the very end of a transfer is passed through untouched
 * rather than padded, so a truncated image is never silently altered.
 */
class EccPageStream
{
public:
    EccPageStream();

    /* engine may be null, or the mode may be RAW, in which case the stream is
     * a pass through. pageBytes is the full page including spare.
     */
    void begin(const EccEngine *engine, int pageBytes, EccMode mode);

    /* Mark a byte range of the stream as belonging to a block the programmer
     * reported bad. Ranges are given in stream offsets and are expected in
     * ascending order, which is how the notifications arrive.
     */
    void addBadSpan(uint64_t start, uint64_t len);

    /* Consume len bytes. Whole pages are processed and appended to out; a
     * partial tail is retained for the next call.
     */
    void push(const uint8_t *data, size_t len, std::vector<uint8_t> &out,
        EccStats &stats);

    /* Flush whatever is left. A short final page is emitted unchanged. */
    void end(std::vector<uint8_t> &out, EccStats &stats);

    uint64_t bytesSeen() const { return m_seen; }

private:
    bool pageIsInBadBlock(uint64_t pageStart) const;
    void processPage(uint8_t *page, uint64_t pageStart, EccStats &stats);

    const EccEngine *m_engine;
    int m_pageBytes;
    EccMode m_mode;
    uint64_t m_seen;            /* bytes consumed so far      */
    uint64_t m_pageIndex;
    std::vector<uint8_t> m_partial;
    std::vector<std::pair<uint64_t, uint64_t> > m_badSpans;
};

#endif /* ECC_STREAM_H */
