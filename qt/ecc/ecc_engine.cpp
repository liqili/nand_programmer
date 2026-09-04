/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#include "ecc_engine.h"
#include "bch_codec.h"
#include "hamming_codec.h"
#include "rs_codec.h"

#include <cstdio>
#include <cstring>

namespace
{

inline int getBit(const uint8_t *buf, int bit)
{
    return (buf[bit >> 3] >> (7 - (bit & 7))) & 1;
}

inline void putBit(uint8_t *buf, int bit, int v)
{
    const uint8_t mask = static_cast<uint8_t>(1u << (7 - (bit & 7)));

    if (v)
        buf[bit >> 3] |= mask;
    else
        buf[bit >> 3] &= static_cast<uint8_t>(~mask);
}

} /* namespace */

EccPageResult::EccPageResult() : sectors(0), clean(0), corrected(0),
    uncorrectable(0), erased(0), worstErrors(0)
{
}

EccEngine::EccEngine() : m_pageSize(0), m_spareSize(0), m_sectors(0)
{
}

bool EccEngine::setScheme(const EccScheme &scheme, int pageSize, int spareSize,
    int bbMarkOffset, std::string &why)
{
    if (scheme.algo == ECC_ALGO_NONE)
    {
        m_scheme = scheme;
        m_pageSize = pageSize;
        m_spareSize = spareSize;
        m_sectors = 0;
        why.clear();
        return true;
    }

    if (!scheme.validateForPage(pageSize, spareSize, bbMarkOffset, why))
        return false;

    m_scheme = scheme;
    m_pageSize = pageSize;
    m_spareSize = spareSize;
    m_sectors = pageSize / scheme.sectorSize;

    return true;
}

bool EccEngine::sectorIsBlank(const uint8_t *page, int sector) const
{
    const uint8_t *d = page + sector * m_scheme.sectorSize;

    for (int i = 0; i < m_scheme.sectorSize; i++)
    {
        if (d[i] != 0xFF)
            return false;
    }

    return true;
}

/* Count zero bits, giving up as soon as the tolerance is passed. */
static int countZeroBits(const uint8_t *b, int n, int limit, int zeros)
{
    for (int i = 0; i < n; i++)
    {
        uint8_t v = static_cast<uint8_t>(~b[i]);

        while (v)
        {
            zeros += v & 1;
            v = static_cast<uint8_t>(v >> 1);
        }

        if (zeros > limit)
            return -1;
    }

    return zeros;
}

int EccEngine::sectorErasedBits(const uint8_t *page, int sector) const
{
    const int tol = m_scheme.erasedBitTolerance();
    int zeros;

    zeros = countZeroBits(page + sector * m_scheme.sectorSize,
        m_scheme.sectorSize, tol, 0);
    if (zeros < 0)
        return -1;

    /* The spare slice counts too. A block that has started to lose charge
     * loses it there as readily as in the data.
     */
    return countZeroBits(page + m_pageSize + sector * m_scheme.oobSize,
        m_scheme.oobSize, tol, zeros);
}

/* Gather one sector into a contiguous region: sectorSize data bytes followed
 * by the oobSize spare bytes that belong to it. Working on a copy keeps the
 * bit addressing simple, and the caller writes any changes back.
 */
static void gatherRegion(const uint8_t *page, int pageSize, int sector,
    const EccScheme &s, std::vector<uint8_t> &region)
{
    region.assign(s.sectorSize + s.oobSize, 0);

    memcpy(&region[0], page + sector * s.sectorSize, s.sectorSize);
    memcpy(&region[s.sectorSize], page + pageSize + sector * s.oobSize,
        s.oobSize);
}

static void scatterRegion(uint8_t *page, int pageSize, int sector,
    const EccScheme &s, const std::vector<uint8_t> &region)
{
    memcpy(page + sector * s.sectorSize, &region[0], s.sectorSize);
    memcpy(page + pageSize + sector * s.oobSize, &region[s.sectorSize],
        s.oobSize);
}

/* Copy the leading messageBits of the region into a whole number of bytes,
 * right aligned so that any padding lands in the high bits. Leading zeros do
 * not change a polynomial remainder, which is what lets a message that is not
 * a whole number of bytes - Broadcom BCH-4 covers 4172 bits - go through a
 * byte oriented codec unchanged.
 */
static void packMessage(const std::vector<uint8_t> &region, int messageBits,
    std::vector<uint8_t> &msg)
{
    const int bytes = (messageBits + 7) / 8;
    const int pad = bytes * 8 - messageBits;

    msg.assign(bytes, 0);

    for (int i = 0; i < messageBits; i++)
        putBit(&msg[0], pad + i, getBit(&region[0], i));
}

/* Read the stored parity out of the region into a right aligned byte buffer,
 * matching the layout the codecs use.
 */
static void readParity(const std::vector<uint8_t> &region, const EccScheme &s,
    int eccBits, std::vector<uint8_t> &parity)
{
    const int bytes = (eccBits + 7) / 8;
    const int pad = bytes * 8 - eccBits;

    parity.assign(bytes, 0);

    for (int i = 0; i < eccBits; i++)
        putBit(&parity[0], pad + i, getBit(&region[0], s.eccBitPos() + i));
}

static void writeParity(std::vector<uint8_t> &region, const EccScheme &s,
    int eccBits, const std::vector<uint8_t> &parity)
{
    const int bytes = (eccBits + 7) / 8;
    const int pad = bytes * 8 - eccBits;

    /* An encoder that stores only whole parity bytes drops the top few bits
     * and leaves whatever the spare already held in their place.
     */
    for (int i = s.truncateTopBits; i < eccBits; i++)
    {
        putBit(&region[0], s.eccBitPos() + i,
            getBit(&parity[0], pad + i));
    }
}

void EccEngine::encodePage(uint8_t *page) const
{
    if (!isEnabled() || m_sectors <= 0)
        return;

    std::vector<uint8_t> region, msg, parity;

    for (int s = 0; s < m_sectors; s++)
    {
        if (sectorIsBlank(page, s))
            continue;

        gatherRegion(page, m_pageSize, s, m_scheme, region);

        switch (m_scheme.algo)
        {
        case ECC_ALGO_BCH:
        {
            BchCodec bch(m_scheme.m, m_scheme.t, m_scheme.primPoly);
            if (!bch.isValid())
                return;

            packMessage(region, m_scheme.messageBits(), msg);
            parity.assign(bch.eccBytes(), 0);
            bch.encode(&msg[0], msg.size(), &parity[0]);
            writeParity(region, m_scheme, bch.eccBits(), parity);
            break;
        }

        case ECC_ALGO_HAMMING:
        {
            const int blocks = m_scheme.sectorSize / HammingCodec::BLOCK_SIZE;
            for (int b = 0; b < blocks; b++)
            {
                uint8_t e[HammingCodec::ECC_BYTES];
                HammingCodec::encode(&region[b * HammingCodec::BLOCK_SIZE], e);

                const int base = m_scheme.eccBitPos() +
                    b * HammingCodec::ECC_BYTES * 8;
                for (int i = 0; i < HammingCodec::ECC_BYTES * 8; i++)
                    putBit(&region[0], base + i, getBit(e, i));
            }
            break;
        }

        case ECC_ALGO_RS:
        {
            RsCodec rs(m_scheme.m, m_scheme.t, m_scheme.primPoly);
            if (!rs.isValid())
                return;

            std::vector<uint16_t> sym, par;
            RsCodec::bytesToSymbols(&region[0], m_scheme.sectorSize,
                m_scheme.m, sym);
            rs.encode(sym, par);

            const int base = m_scheme.eccBitPos();
            for (size_t i = 0; i < par.size(); i++)
            {
                for (int b = 0; b < m_scheme.m; b++)
                {
                    putBit(&region[0], base + static_cast<int>(i) *
                        m_scheme.m + b, (par[i] >> (m_scheme.m - 1 - b)) & 1);
                }
            }
            break;
        }

        case ECC_ALGO_NONE:
            break;
        }

        scatterRegion(page, m_pageSize, s, m_scheme, region);
    }
}

EccPageResult EccEngine::decodePage(uint8_t *page, bool correct) const
{
    EccPageResult res;

    if (!isEnabled() || m_sectors <= 0)
        return res;

    res.sectors = m_sectors;

    std::vector<uint8_t> region, msg, parity;

    for (int s = 0; s < m_sectors; s++)
    {
        EccSectorResult sr;
        sr.sector = s;
        sr.status = ECC_SECTOR_CLEAN;
        sr.errors = 0;

        const int erasedBits = sectorErasedBits(page, s);
        if (erasedBits >= 0)
        {
            sr.status = ECC_SECTOR_ERASED;
            sr.errors = erasedBits;
            res.erased++;
            res.details.push_back(sr);
            continue;
        }

        gatherRegion(page, m_pageSize, s, m_scheme, region);

        int rc = 0;

        switch (m_scheme.algo)
        {
        case ECC_ALGO_BCH:
        {
            BchCodec bch(m_scheme.m, m_scheme.t, m_scheme.primPoly);
            if (!bch.isValid())
            {
                rc = -1;
                break;
            }

            packMessage(region, m_scheme.messageBits(), msg);
            readParity(region, m_scheme, bch.eccBits(), parity);

            /* Parity bits the format never stored carry no information, so
             * take them from the freshly calculated parity. Leaving the spare
             * contents in their place would inject phantom errors and eat
             * into the correction budget for real ones.
             */
            if (m_scheme.truncateTopBits > 0)
            {
                std::vector<uint8_t> calc(bch.eccBytes(), 0);
                const int pad = bch.eccBytes() * 8 - bch.eccBits();

                bch.encode(&msg[0], msg.size(), &calc[0]);

                for (int i = 0; i < m_scheme.truncateTopBits; i++)
                    putBit(&parity[0], pad + i, getBit(&calc[0], pad + i));
            }

            rc = bch.correct(&msg[0], msg.size(), &parity[0]);

            /* Push the repaired message bits back where they came from. */
            if (rc > 0 && correct)
            {
                const int mb = m_scheme.messageBits();
                const int pad = static_cast<int>(msg.size()) * 8 - mb;
                for (int i = 0; i < mb; i++)
                    putBit(&region[0], i, getBit(&msg[0], pad + i));
            }
            break;
        }

        case ECC_ALGO_HAMMING:
        {
            const int blocks = m_scheme.sectorSize / HammingCodec::BLOCK_SIZE;
            for (int b = 0; b < blocks && rc >= 0; b++)
            {
                uint8_t e[HammingCodec::ECC_BYTES];
                const int base = m_scheme.eccBitPos() +
                    b * HammingCodec::ECC_BYTES * 8;

                memset(e, 0, sizeof(e));
                for (int i = 0; i < HammingCodec::ECC_BYTES * 8; i++)
                    putBit(e, i, getBit(&region[0], base + i));

                uint8_t tmp[HammingCodec::BLOCK_SIZE];
                memcpy(tmp, &region[b * HammingCodec::BLOCK_SIZE],
                    sizeof(tmp));

                switch (HammingCodec::correct(tmp, e))
                {
                case HammingCodec::HAMMING_CLEAN:
                    break;
                case HammingCodec::HAMMING_CORRECTED:
                    rc++;
                    if (correct)
                    {
                        memcpy(&region[b * HammingCodec::BLOCK_SIZE], tmp,
                            sizeof(tmp));
                    }
                    break;
                case HammingCodec::HAMMING_ECC_ERROR:
                    rc++;
                    break;
                case HammingCodec::HAMMING_UNCORRECTABLE:
                    rc = -1;
                    break;
                }
            }
            break;
        }

        case ECC_ALGO_RS:
        {
            RsCodec rs(m_scheme.m, m_scheme.t, m_scheme.primPoly);
            if (!rs.isValid())
            {
                rc = -1;
                break;
            }

            std::vector<uint16_t> sym, par(2 * m_scheme.t, 0);
            RsCodec::bytesToSymbols(&region[0], m_scheme.sectorSize,
                m_scheme.m, sym);

            const int base = m_scheme.eccBitPos();
            for (size_t i = 0; i < par.size(); i++)
            {
                uint16_t v = 0;
                for (int b = 0; b < m_scheme.m; b++)
                {
                    v = static_cast<uint16_t>((v << 1) | getBit(&region[0],
                        base + static_cast<int>(i) * m_scheme.m + b));
                }
                par[i] = v;
            }

            rc = rs.correct(sym, par);

            if (rc > 0 && correct)
            {
                RsCodec::symbolsToBytes(sym, m_scheme.m, &region[0],
                    m_scheme.sectorSize);
            }
            break;
        }

        case ECC_ALGO_NONE:
            break;
        }

        if (rc < 0)
        {
            sr.status = ECC_SECTOR_UNCORRECTABLE;
            res.uncorrectable++;
        }
        else if (rc > 0)
        {
            sr.status = ECC_SECTOR_CORRECTED;
            sr.errors = rc;
            res.corrected++;
            if (rc > res.worstErrors)
                res.worstErrors = rc;

            if (correct)
                scatterRegion(page, m_pageSize, s, m_scheme, region);
        }
        else
        {
            res.clean++;
        }

        res.details.push_back(sr);
    }

    return res;
}

std::string EccEngine::strengthText() const
{
    return strengthText(m_scheme);
}

std::string EccEngine::strengthText(const EccScheme &scheme)
{
    char buf[160];

    switch (scheme.algo)
    {
    case ECC_ALGO_NONE:
        return "No correction";

    case ECC_ALGO_HAMMING:
        snprintf(buf, sizeof(buf), "Corrects 1 bit, detects 2, per 256 byte "
            "block (%d block%s per sector)", scheme.sectorSize / 256,
            scheme.sectorSize / 256 == 1 ? "" : "s");
        return buf;

    case ECC_ALGO_BCH:
        snprintf(buf, sizeof(buf), "Corrects up to %d bit%s anywhere in the "
            "%d byte sector", scheme.t, scheme.t == 1 ? "" : "s",
            scheme.sectorSize);
        return buf;

    case ECC_ALGO_RS:
        /* Deliberately phrased in symbols. Quoting t * m bits would invite a
         * comparison with BCH that does not hold: those bits only count when
         * they fall inside the same t symbols.
         */
        snprintf(buf, sizeof(buf), "Corrects up to %d symbol%s of %d bits "
            "(good for bursts, not for %d scattered bit errors)", scheme.t,
            scheme.t == 1 ? "" : "s", scheme.m, scheme.t * scheme.m);
        return buf;
    }

    return "";
}
