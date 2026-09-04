/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#ifndef IMAGE_PROBE_H
#define IMAGE_PROBE_H

#include "ecc_scheme.h"

#include <cstdint>
#include <cstddef>
#include <string>

/* Works out what a supplied image actually is before it is written.
 *
 * Getting this wrong is destructive: writing a data only image while the
 * transfer includes the spare area shifts every page by the spare size, so
 * data lands in spare areas, bad block markers are overwritten and the whole
 * chip ends up garbage. The programmer has enough information to tell the
 * difference, so it should.
 */

enum ImageLayout
{
    IMAGE_LAYOUT_UNKNOWN = 0,
    IMAGE_LAYOUT_DATA_ONLY,     /* pageSize byte pages, no spare      */
    IMAGE_LAYOUT_PAGE_SPARE,    /* (pageSize + spareSize) byte pages  */
};

struct ImageProbeResult
{
    ImageLayout layout;
    int confidence;             /* 0..100                                  */
    std::string reason;         /* plain wording, fit to show to the user  */

    bool spareBlank;            /* spare present but entirely 0xFF         */
    int eccRegionFirst;         /* first spare byte seen non 0xFF, or -1   */
    int eccRegionLast;          /* last such byte, or -1                   */

    int matchingPreset;         /* preset whose parity validates, or -1    */
    int pagesSampled;
    int pagesValidated;

    uint64_t fullPages;         /* whole pages under the chosen layout     */
    uint32_t trailingBytes;     /* short final page, 0 when none           */

    /* Pages whose byte at the bad block marker offset is not 0xFF. In a
     * source image these are markers inherited from the device the image came
     * from; writing them would mark good blocks bad on the destination.
     */
    uint64_t foreignBadMarkers;

    ImageProbeResult();

    /* True when the image can be written to this chip without the caller
     * having to ask the user anything.
     */
    bool needsDecision() const;
};

class ImageProbe
{
public:
    /* Reads len bytes at offset into buf. Returns false on short read or
     * error. Keeping this a callback avoids pulling a whole chip image into
     * memory just to look at a few dozen pages of it.
     */
    typedef bool (*ReadFn)(void *ctx, uint64_t offset, uint8_t *buf,
        size_t len);

    /* pageSize is the data page, spareSize the spare that goes with it.
     * bbMarkOffset may be -1 when unknown, which skips the marker count.
     */
    static ImageProbeResult probe(ReadFn read, void *ctx, uint64_t fileSize,
        int pageSize, int spareSize, int bbMarkOffset);

    /* Pages examined for content shape, and the smaller number decoded when
     * trying to identify a scheme.
     */
    static const int SAMPLE_PAGES = 64;
    static const int SCHEME_PAGES = 8;
};

#endif /* IMAGE_PROBE_H */
