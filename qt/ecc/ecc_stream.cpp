/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#include "ecc_stream.h"

#include <cstdio>
#include <cstring>

EccStats::EccStats()
{
    reset();
}

void EccStats::reset()
{
    pages = 0;
    stepsTotal = 0;
    stepsClean = 0;
    stepsCorrected = 0;
    stepsUncorrectable = 0;
    stepsErased = 0;
    stepsInBadBlock = 0;
    errorsCorrected = 0;
    worstStep = 0;
    firstFailPage = 0;
    haveFailure = false;
}

bool EccStats::wearWarning(const EccScheme &scheme) const
{
    const int strength = scheme.correctableBits();

    if (scheme.algo == ECC_ALGO_NONE || strength <= 1)
        return false;

    /* One away from the limit is the point at which the next flip in the same
     * step becomes unrecoverable, which is when the block deserves attention.
     */
    return worstStep >= strength - 1;
}

std::string EccStats::summary() const
{
    char buf[320];

    if (!stepsTotal)
        return "ECC: nothing checked";

    snprintf(buf, sizeof(buf),
        "ECC: %llu pages, %llu steps - %llu clean, %llu erased, %llu corrected "
        "(%llu errors, worst %d), %llu uncorrectable, %llu in bad blocks",
        static_cast<unsigned long long>(pages),
        static_cast<unsigned long long>(stepsTotal),
        static_cast<unsigned long long>(stepsClean),
        static_cast<unsigned long long>(stepsErased),
        static_cast<unsigned long long>(stepsCorrected),
        static_cast<unsigned long long>(errorsCorrected),
        worstStep,
        static_cast<unsigned long long>(stepsUncorrectable),
        static_cast<unsigned long long>(stepsInBadBlock));

    return buf;
}

EccPageStream::EccPageStream() : m_engine(NULL), m_pageBytes(0),
    m_mode(ECC_MODE_RAW), m_seen(0), m_pageIndex(0)
{
}

void EccPageStream::begin(const EccEngine *engine, int pageBytes, EccMode mode)
{
    m_engine = engine;
    m_pageBytes = pageBytes;
    m_mode = mode;
    m_seen = 0;
    m_pageIndex = 0;
    m_partial.clear();
    m_badSpans.clear();

    if (!engine || !engine->isEnabled() || pageBytes <= 0)
        m_mode = ECC_MODE_RAW;
}

void EccPageStream::addBadSpan(uint64_t start, uint64_t len)
{
    if (len)
        m_badSpans.push_back(std::make_pair(start, len));
}

bool EccPageStream::pageIsInBadBlock(uint64_t pageStart) const
{
    for (size_t i = 0; i < m_badSpans.size(); i++)
    {
        const uint64_t s = m_badSpans[i].first;
        const uint64_t e = s + m_badSpans[i].second;

        if (pageStart >= s && pageStart < e)
            return true;
    }

    return false;
}

void EccPageStream::processPage(uint8_t *page, uint64_t pageStart,
    EccStats &stats)
{
    const EccScheme &scheme = m_engine->scheme();
    const int steps = m_engine->sectorsPerPage();

    stats.pages++;

    /* A block the programmer already told us is bad cannot be judged by its
     * parity. Count its steps apart and leave the bytes alone.
     */
    if (pageIsInBadBlock(pageStart))
    {
        stats.stepsTotal += steps;
        stats.stepsInBadBlock += steps;
        return;
    }

    const EccPageResult r = m_engine->decodePage(page,
        m_mode == ECC_MODE_CORRECT);

    stats.stepsTotal += r.sectors;
    stats.stepsClean += r.clean;
    stats.stepsErased += r.erased;
    stats.stepsCorrected += r.corrected;
    stats.stepsUncorrectable += r.uncorrectable;

    for (size_t i = 0; i < r.details.size(); i++)
    {
        if (r.details[i].status == ECC_SECTOR_CORRECTED)
            stats.errorsCorrected += r.details[i].errors;
    }

    if (r.worstErrors > stats.worstStep)
        stats.worstStep = r.worstErrors;

    if (r.uncorrectable && !stats.haveFailure)
    {
        stats.haveFailure = true;
        stats.firstFailPage = m_pageIndex;
    }

    (void)scheme;
}

void EccPageStream::push(const uint8_t *data, size_t len,
    std::vector<uint8_t> &out, EccStats &stats)
{
    if (m_mode == ECC_MODE_RAW)
    {
        out.insert(out.end(), data, data + len);
        m_seen += len;
        return;
    }

    size_t off = 0;

    /* Top up a page held over from the previous call before starting new ones. */
    if (!m_partial.empty())
    {
        const size_t need = static_cast<size_t>(m_pageBytes) - m_partial.size();
        const size_t take = len < need ? len : need;

        m_partial.insert(m_partial.end(), data, data + take);
        off = take;

        if (static_cast<int>(m_partial.size()) < m_pageBytes)
        {
            m_seen += len;
            return;
        }

        processPage(&m_partial[0], m_seen + off - m_pageBytes, stats);
        m_pageIndex++;
        out.insert(out.end(), m_partial.begin(), m_partial.end());
        m_partial.clear();
    }

    while (off + static_cast<size_t>(m_pageBytes) <= len)
    {
        const size_t base = out.size();

        out.insert(out.end(), data + off, data + off + m_pageBytes);
        processPage(&out[base], m_seen + off, stats);
        m_pageIndex++;
        off += m_pageBytes;
    }

    if (off < len)
        m_partial.assign(data + off, data + len);

    m_seen += len;
}

void EccPageStream::end(std::vector<uint8_t> &out, EccStats &stats)
{
    if (m_partial.empty())
        return;

    /* Short final page. Emitting it unchanged is the honest thing to do: it is
     * not a whole page, so its parity cannot be judged, and padding it would
     * put bytes in the output that were never on the device.
     */
    out.insert(out.end(), m_partial.begin(), m_partial.end());
    m_partial.clear();

    (void)stats;
}
