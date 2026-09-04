/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#include "image_probe.h"
#include "ecc_engine.h"

#include <cstdio>
#include <cstring>
#include <vector>

ImageProbeResult::ImageProbeResult() : layout(IMAGE_LAYOUT_UNKNOWN),
    confidence(0), spareBlank(false), eccRegionFirst(-1), eccRegionLast(-1),
    matchingPreset(-1), pagesSampled(0), pagesValidated(0), fullPages(0),
    trailingBytes(0), foreignBadMarkers(0)
{
}

bool ImageProbeResult::needsDecision() const
{
    /* A page + spare image whose parity is already identified can go straight
     * to the chip. Everything else deserves a question.
     */
    return !(layout == IMAGE_LAYOUT_PAGE_SPARE && matchingPreset >= 0 &&
        !foreignBadMarkers);
}

namespace
{

struct SpareShape
{
    uint64_t spareBytes;
    uint64_t ffBytes;
    int first;
    int last;
    int pages;
    bool nonFFSeen;

    SpareShape() : spareBytes(0), ffBytes(0), first(-1), last(-1), pages(0),
        nonFFSeen(false) {}

    double ffRatio() const
    {
        return spareBytes ? static_cast<double>(ffBytes) / spareBytes : 1.0;
    }
};

bool allFF(const uint8_t *b, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        if (b[i] != 0xFF)
            return false;
    }

    return true;
}

/* Walk a spread of pages, treating the image as page + spare, and record what
 * the presumed spare bytes look like.
 */
void sampleSpare(ImageProbe::ReadFn read, void *ctx, uint64_t pages,
    int pageSize, int spareSize, int bbMarkOffset, SpareShape &shape,
    uint64_t &badMarkers, std::vector<uint64_t> &usable)
{
    const int stride = pageSize + spareSize;
    const uint64_t step = pages > ImageProbe::SAMPLE_PAGES ?
        pages / ImageProbe::SAMPLE_PAGES : 1;

    std::vector<uint8_t> page(stride);

    for (uint64_t i = 0; i < pages; i += step)
    {
        if (!read(ctx, i * static_cast<uint64_t>(stride), &page[0], stride))
            break;

        const uint8_t *sp = &page[pageSize];

        if (bbMarkOffset >= 0 && bbMarkOffset < spareSize &&
            sp[bbMarkOffset] != 0xFF)
        {
            badMarkers++;
        }

        /* Erased pages say nothing about layout either way. */
        if (allFF(&page[0], pageSize) && allFF(sp, spareSize))
            continue;

        shape.pages++;
        shape.spareBytes += spareSize;

        for (int b = 0; b < spareSize; b++)
        {
            if (sp[b] == 0xFF)
            {
                shape.ffBytes++;
                continue;
            }

            shape.nonFFSeen = true;
            if (shape.first < 0 || b < shape.first)
                shape.first = b;
            if (b > shape.last)
                shape.last = b;
        }

        if (usable.size() < ImageProbe::SCHEME_PAGES)
            usable.push_back(i);
    }
}

/* Does this preset's parity land anywhere near the bytes that were actually
 * written? Presets that do not overlap cannot be the one in use, and skipping
 * them keeps the trial cheap.
 */
bool plausible(const EccScheme &s, int spareSize, int first, int last)
{
    if (first < 0)
        return false;

    const int steps = spareSize / s.oobSize;

    for (int st = 0; st < steps; st++)
    {
        const int base = st * s.oobSize;
        const int lo = base + s.eccBitOffset / 8;
        const int hi = base + (s.eccBitOffset + s.eccBits() - 1) / 8;

        if (lo <= last && first <= hi)
            return true;
    }

    return false;
}

} /* namespace */

ImageProbeResult ImageProbe::probe(ReadFn read, void *ctx, uint64_t fileSize,
    int pageSize, int spareSize, int bbMarkOffset)
{
    ImageProbeResult res;
    char buf[256];

    if (pageSize <= 0 || spareSize <= 0 || !fileSize)
    {
        res.reason = "Nothing to examine.";
        return res;
    }

    const uint64_t stride = static_cast<uint64_t>(pageSize) + spareSize;
    const bool divData = (fileSize % pageSize) == 0;
    const bool divFull = (fileSize % stride) == 0;

    SpareShape shape;
    std::vector<uint64_t> usable;
    const uint64_t pagesIfFull = fileSize / stride;

    if (pagesIfFull)
    {
        sampleSpare(read, ctx, pagesIfFull, pageSize, spareSize, bbMarkOffset,
            shape, res.foreignBadMarkers, usable);
    }

    res.pagesSampled = shape.pages;

    /* Content shape under the page + spare hypothesis. A real spare area is
     * mostly 0xFF with its written bytes bunched together; ordinary data
     * mis-sliced as spare looks like ordinary data.
     */
    const double ratio = shape.ffRatio();
    const bool looksLikeSpare = shape.pages == 0 || ratio >= 0.45;

    if (divFull && !divData)
    {
        res.layout = IMAGE_LAYOUT_PAGE_SPARE;
        res.confidence = 95;
        res.reason = "Size is a whole number of page plus spare units.";
    }
    else if (divData && !divFull)
    {
        res.layout = IMAGE_LAYOUT_DATA_ONLY;
        res.confidence = 95;
        res.reason = "Size is a whole number of data pages and does not "
            "divide into page plus spare.";
    }
    else if (divData && divFull)
    {
        /* Both divide, which happens more often than it sounds. Content
         * decides.
         */
        if (looksLikeSpare)
        {
            res.layout = IMAGE_LAYOUT_PAGE_SPARE;
            res.confidence = 80;
            snprintf(buf, sizeof(buf), "Size fits either layout; the presumed "
                "spare bytes are %d%% 0xFF, which is what a spare area looks "
                "like.", static_cast<int>(ratio * 100));
        }
        else
        {
            res.layout = IMAGE_LAYOUT_DATA_ONLY;
            res.confidence = 75;
            snprintf(buf, sizeof(buf), "Size fits either layout, but the "
                "presumed spare bytes are only %d%% 0xFF, so they are data "
                "rather than spare.", static_cast<int>(ratio * 100));
        }
        res.reason = buf;
    }
    else
    {
        res.layout = looksLikeSpare && pagesIfFull ?
            IMAGE_LAYOUT_PAGE_SPARE : IMAGE_LAYOUT_DATA_ONLY;
        res.confidence = 40;
        res.reason = "Size is not a whole number of pages in either layout; "
            "the image looks truncated.";
    }

    if (res.layout == IMAGE_LAYOUT_PAGE_SPARE)
    {
        res.fullPages = fileSize / stride;
        res.trailingBytes = static_cast<uint32_t>(fileSize % stride);
        res.spareBlank = !shape.nonFFSeen;
        res.eccRegionFirst = shape.first;
        res.eccRegionLast = shape.last;
    }
    else
    {
        res.fullPages = fileSize / pageSize;
        res.trailingBytes = static_cast<uint32_t>(fileSize % pageSize);
        res.foreignBadMarkers = 0;      /* no spare, so no markers to inherit */
        return res;
    }

    if (res.spareBlank || usable.empty())
        return res;

    /* Identify the scheme. A layout whose parity actually checks out over
     * several pages is not there by chance, and it pins down both the scheme
     * and the layout at once.
     *
     * Several presets can survive the same image, so the first one that does
     * not fail is not necessarily the right answer. A near miss shows up as
     * pages that decode only after correction, and a preset that ignores some
     * parity bits accepts anything a stricter one accepts. Score them and take
     * the best: most pages clean, then fewest corrections, then the scheme
     * that explains the most bits.
     */
    std::vector<uint8_t> page(stride);
    int bestClean = -1;
    uint64_t bestErrors = 0;
    int bestTrunc = 0;

    for (int p = 0; p < EccScheme::presetCount(); p++)
    {
        const EccScheme s = EccScheme::preset(p);
        std::string why;

        if (s.algo == ECC_ALGO_NONE)
            continue;
        if (!s.validateForPage(pageSize, spareSize, bbMarkOffset, why))
            continue;
        if (!plausible(s, spareSize, shape.first, shape.last))
            continue;

        EccEngine eng;
        if (!eng.setScheme(s, pageSize, spareSize, bbMarkOffset, why))
            continue;

        int good = 0;
        int cleanPages = 0;
        uint64_t errors = 0;
        bool failed = false;

        for (size_t i = 0; i < usable.size() && !failed; i++)
        {
            if (!read(ctx, usable[i] * stride, &page[0],
                static_cast<size_t>(stride)))
            {
                failed = true;
                break;
            }

            const EccPageResult r = eng.decodePage(&page[0], false);

            /* Corrected still counts as a match: a dump taken off a worn chip
             * legitimately carries bitflips. Only uncorrectable rules it out.
             */
            if (r.uncorrectable)
            {
                failed = true;
                break;
            }

            if (r.clean || r.corrected)
                good++;
            if (!r.corrected)
                cleanPages++;

            for (size_t d = 0; d < r.details.size(); d++)
            {
                if (r.details[d].status == ECC_SECTOR_CORRECTED)
                    errors += r.details[d].errors;
            }
        }

        if (failed || !good)
            continue;

        const bool better = cleanPages > bestClean ||
            (cleanPages == bestClean && errors < bestErrors) ||
            (cleanPages == bestClean && errors == bestErrors &&
                s.truncateTopBits < bestTrunc);

        if (!better)
            continue;

        bestClean = cleanPages;
        bestErrors = errors;
        bestTrunc = s.truncateTopBits;

        res.matchingPreset = p;
        res.pagesValidated = good;
        res.confidence = cleanPages == static_cast<int>(usable.size()) ?
            99 : 85;

        snprintf(buf, sizeof(buf), "Spare holds parity in bytes %d..%d, "
            "verified as \"%s\" on %d of %d sampled pages%s.", shape.first,
            shape.last, EccScheme::presetName(p), good,
            static_cast<int>(usable.size()),
            cleanPages == good ? "" : " (some needed correction)");
        res.reason = buf;
    }

    return res;
}
