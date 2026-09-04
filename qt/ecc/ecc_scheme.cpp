/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#include "ecc_scheme.h"
#include "bch_codec.h"

#include <cstdio>
#include <cstring>

namespace
{

struct Preset
{
    const char *name;
    const char *desc;
    EccScheme scheme;
};

EccScheme mk(EccAlgorithm algo, int sectorSize, int oobSize, int eccBitOffset,
    bool coverSpare, int m, int t, uint32_t primPoly, int truncateTopBits = 0)
{
    EccScheme s;

    s.algo = algo;
    s.sectorSize = sectorSize;
    s.oobSize = oobSize;
    s.eccBitOffset = eccBitOffset;
    s.coverSpare = coverSpare;
    s.m = m;
    s.t = t;
    s.primPoly = primPoly;
    s.truncateTopBits = truncateTopBits;

    return s;
}

/* The layouts a user of this programmer is most likely to meet. Broadcom
 * BCH-4 is the one verified byte for byte against images produced by the
 * reference encoder; the rest follow the conventional parameters for their
 * family and should be checked against a real device before being trusted.
 *
 * The generic layouts put their parity at the end of the spare slice, leaving
 * the low bytes free. That is where controllers conventionally put it, and it
 * keeps the parity clear of the bad block marker, which lives in the first
 * byte or two of the page spare area.
 */
const Preset presets[] =
{
    {
        "None",
        "Read and write raw bytes. No parity is generated or checked.",
        mk(ECC_ALGO_NONE, 512, 16, 0, false, 0, 0, 0)
    },
    {
        "Broadcom BCH-4 (512 B)",
        "Broadcom NAND controllers below v5.0. 52 bit parity ending the "
        "16 byte spare, covering the sector and the first 9 1/2 spare bytes.",
        mk(ECC_ALGO_BCH, 512, 16, 76, true, 13, 4, 0x201b)
    },
    {
        "Broadcom BCH-4, truncated parity",
        "As above, but reproduces images from the brcm-nand-bch tool, which "
        "writes only whole parity bytes and loses the top 4 bits.",
        mk(ECC_ALGO_BCH, 512, 16, 76, true, 13, 4, 0x201b, 4)
    },
    {
        "Broadcom BCH-4 (v5.0+)",
        "Broadcom NAND controllers v5.0 and later. 56 bit parity over "
        "GF(2^14), occupying the last 7 bytes of the spare.",
        mk(ECC_ALGO_BCH, 512, 16, 72, true, 14, 4, 0x5803)
    },
    {
        "Hamming (256 B)",
        "SmartMedia style. Corrects one bit and detects two per 256 byte "
        "block, using 3 parity bytes.",
        mk(ECC_ALGO_HAMMING, 256, 8, 40, false, 0, 1, 0)
    },
    {
        "Hamming (512 B)",
        "Corrects one bit and detects two per 512 byte block, using "
        "6 parity bytes.",
        mk(ECC_ALGO_HAMMING, 512, 16, 80, false, 0, 1, 0)
    },
    {
        "BCH-4 (512 B)",
        "Generic 4 bit correction over GF(2^13). 52 bit parity ending the "
        "16 byte spare, leaving the low bytes free.",
        mk(ECC_ALGO_BCH, 512, 16, 76, false, 13, 4, 0)
    },
    {
        "BCH-8 (512 B)",
        "8 bit correction over GF(2^13), 104 bit parity. Fills all but the "
        "first 3 bytes of a 16 byte spare.",
        mk(ECC_ALGO_BCH, 512, 16, 24, false, 13, 8, 0)
    },
    {
        "BCH-8 (1024 B)",
        "8 bit correction over GF(2^14) on 1 KB sectors, 112 bit parity.",
        mk(ECC_ALGO_BCH, 1024, 28, 112, false, 14, 8, 0)
    },
    {
        "BCH-16 (1024 B)",
        "16 bit correction over GF(2^14), 224 bit parity. Common on MLC "
        "parts requiring 16 bits per 1 KB.",
        mk(ECC_ALGO_BCH, 1024, 32, 32, false, 14, 16, 0)
    },
    {
        "BCH-24 (1024 B)",
        "24 bit correction over GF(2^14), 336 bit parity. Needs at least "
        "42 spare bytes per sector.",
        mk(ECC_ALGO_BCH, 1024, 44, 16, false, 14, 24, 0)
    },
    {
        "Reed-Solomon t=4 (512 B)",
        "Classic RS over GF(2^10) correcting 4 symbols, 10 parity bytes. "
        "Used by older controllers and by SmartMedia successors.",
        mk(ECC_ALGO_RS, 512, 16, 48, false, 10, 4, 0)
    },
};

const int presetsCount = static_cast<int>(sizeof(presets) / sizeof(presets[0]));

bool sameScheme(const EccScheme &a, const EccScheme &b)
{
    return a.algo == b.algo && a.sectorSize == b.sectorSize &&
        a.oobSize == b.oobSize && a.eccBitOffset == b.eccBitOffset &&
        a.coverSpare == b.coverSpare && a.m == b.m && a.t == b.t &&
        a.primPoly == b.primPoly && a.truncateTopBits == b.truncateTopBits;
}

} /* namespace */

EccScheme::EccScheme() : algo(ECC_ALGO_NONE), sectorSize(512), oobSize(16),
    eccBitOffset(0), coverSpare(false), m(0), t(0), primPoly(0),
    truncateTopBits(0)
{
}

int EccScheme::eccBits() const
{
    switch (algo)
    {
    case ECC_ALGO_NONE:
        return 0;

    case ECC_ALGO_HAMMING:
        /* Three bytes of parity per 256 byte block, which is the layout every
         * Hamming based NAND scheme settled on.
         */
        return (sectorSize / 256) * 24;

    case ECC_ALGO_BCH:
    {
        /* The generator degree is m * t except where the cyclotomic cosets
         * overlap, so ask the codec rather than assuming.
         */
        BchCodec bch(m, t, primPoly);
        return bch.isValid() ? bch.eccBits() : 0;
    }

    case ECC_ALGO_RS:
        /* 2t parity symbols of m bits each. */
        return 2 * t * m;
    }

    return 0;
}

int EccScheme::eccBytes() const
{
    return (eccBits() + 7) / 8;
}

int EccScheme::messageBits() const
{
    if (algo == ECC_ALGO_NONE)
        return 0;

    return coverSpare ? sectorSize * 8 + eccBitOffset : sectorSize * 8;
}

int EccScheme::correctableBits() const
{
    switch (algo)
    {
    case ECC_ALGO_NONE:
        return 0;
    case ECC_ALGO_HAMMING:
        return sectorSize / 256;    /* one bit per 256 byte block */
    case ECC_ALGO_BCH:
        return t;
    case ECC_ALGO_RS:
        return t * m;               /* t symbols, worst case m bits each */
    }

    return 0;
}

int EccScheme::sectorsPerPage(int pageSize) const
{
    if (sectorSize <= 0)
        return 0;

    return pageSize / sectorSize;
}

bool EccScheme::parityCoversSpareByte(int byteInSlice) const
{
    const int bits = eccBits();

    if (algo == ECC_ALGO_NONE || bits <= 0 || byteInSlice < 0)
        return false;

    const int lo = eccBitOffset;
    const int hi = eccBitOffset + bits;
    const int byteLo = byteInSlice * 8;

    return lo < byteLo + 8 && byteLo < hi;
}

int EccScheme::erasedBitTolerance() const
{
    switch (algo)
    {
    case ECC_ALGO_NONE:
        return 0;
    case ECC_ALGO_HAMMING:
        return sectorSize / 256;
    case ECC_ALGO_BCH:
        return t;
    case ECC_ALGO_RS:
        /* Counted in bits, so t rather than t * m. Being generous here would
         * let a genuinely programmed page be mistaken for an erased one.
         */
        return t;
    }

    return 0;
}

bool EccScheme::validate(std::string &why) const
{
    char buf[192];

    why.clear();

    if (algo == ECC_ALGO_NONE)
        return true;

    if (sectorSize != 256 && sectorSize != 512 && sectorSize != 1024)
    {
        why = "Sector size must be 256, 512 or 1024 bytes.";
        return false;
    }

    if (oobSize <= 0)
    {
        why = "Spare bytes per sector must be greater than zero.";
        return false;
    }

    if (algo == ECC_ALGO_BCH)
    {
        if (m < 3 || m > 15)
        {
            why = "Galois field order m must be between 3 and 15.";
            return false;
        }

        if (t < 1)
        {
            why = "Correction strength t must be at least 1.";
            return false;
        }

        GaloisField gf(m, primPoly);
        if (!gf.isValid())
        {
            snprintf(buf, sizeof(buf), "0x%X is not a primitive polynomial of "
                "degree %d over GF(2).", primPoly ?
                primPoly : GaloisField::defaultPrimPoly(m), m);
            why = buf;
            return false;
        }

        BchCodec bch(m, t, primPoly);
        if (!bch.isValid())
        {
            why = "No BCH code exists for this combination of m and t.";
            return false;
        }

        if (messageBits() + bch.eccBits() > gf.n())
        {
            snprintf(buf, sizeof(buf), "Codeword of %d bits exceeds the %d bit "
                "limit of GF(2^%d). Use a larger m or a smaller sector.",
                messageBits() + bch.eccBits(), gf.n(), m);
            why = buf;
            return false;
        }
    }

    if (algo == ECC_ALGO_RS)
    {
        if (m < 3 || m > 15)
        {
            why = "Galois field order m must be between 3 and 15.";
            return false;
        }

        if (t < 1)
        {
            why = "Correction strength t must be at least 1.";
            return false;
        }

        const int symbols = (sectorSize * 8 + m - 1) / m;
        if (symbols + 2 * t > (1 << m) - 1)
        {
            why = "Sector needs more Reed-Solomon symbols than the field "
                "provides. Use a larger m.";
            return false;
        }
    }

    const int bits = eccBits();
    if (bits <= 0)
    {
        why = "This combination produces no parity.";
        return false;
    }

    if (eccBitOffset < 0)
    {
        why = "Parity offset cannot be negative.";
        return false;
    }

    if (eccBitOffset + bits > oobSize * 8)
    {
        snprintf(buf, sizeof(buf), "Parity needs %d bits at offset %d, which "
            "runs past the %d bits of spare per sector.", bits, eccBitOffset,
            oobSize * 8);
        why = buf;
        return false;
    }

    if (truncateTopBits < 0 || truncateTopBits >= bits)
    {
        why = "Truncated parity bits must be fewer than the parity size.";
        return false;
    }

    return true;
}

bool EccScheme::validateForPage(int pageSize, int spareSize, int bbMarkOffset,
    std::string &why) const
{
    char buf[192];

    if (!validate(why))
        return false;

    if (algo == ECC_ALGO_NONE)
        return true;

    if (pageSize <= 0 || sectorSize > pageSize)
    {
        why = "Sector is larger than the page of the selected chip.";
        return false;
    }

    if (pageSize % sectorSize)
    {
        snprintf(buf, sizeof(buf), "Page of %d bytes is not a whole number of "
            "%d byte sectors.", pageSize, sectorSize);
        why = buf;
        return false;
    }

    const int sectors = pageSize / sectorSize;
    if (sectors * oobSize > spareSize)
    {
        snprintf(buf, sizeof(buf), "%d sectors need %d spare bytes but the "
            "chip provides %d.", sectors, sectors * oobSize, spareSize);
        why = buf;
        return false;
    }

    /* The factory bad block marker must survive. Once parity is written over
     * it a marked block becomes indistinguishable from a good one, and no
     * later read can tell the difference.
     *
     * The marker is at bbMarkOffset within the page spare area, so it belongs
     * to one particular step's slice; the parity region sits at the same place
     * in every slice, so covering that byte anywhere covers it there.
     */
    if (bbMarkOffset >= 0)
    {
        const int markStep = bbMarkOffset / oobSize;

        if (markStep < sectors &&
            parityCoversSpareByte(bbMarkOffset % oobSize))
        {
            snprintf(buf, sizeof(buf), "Parity would be written over the bad "
                "block marker at spare byte %d. Move the parity offset so it "
                "starts after byte %d of each sector's spare.", bbMarkOffset,
                bbMarkOffset % oobSize);
            why = buf;
            return false;
        }
    }

    return true;
}

int EccScheme::presetCount()
{
    return presetsCount;
}

EccScheme EccScheme::preset(int index)
{
    if (index < 0 || index >= presetsCount)
        return EccScheme();

    return presets[index].scheme;
}

const char *EccScheme::presetName(int index)
{
    if (index < 0 || index >= presetsCount)
        return "";

    return presets[index].name;
}

const char *EccScheme::presetDescription(int index)
{
    if (index < 0 || index >= presetsCount)
        return "";

    return presets[index].desc;
}

int EccScheme::matchingPreset() const
{
    for (int i = 0; i < presetsCount; i++)
    {
        if (sameScheme(*this, presets[i].scheme))
            return i;
    }

    return -1;
}
