/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#ifndef ECC_SCHEME_H
#define ECC_SCHEME_H

#include <cstdint>
#include <string>
#include <vector>

/* How a NAND page is divided into ECC protected sectors, and where the parity
 * lives inside the spare area.
 *
 * A page is split into sectors of sectorSize data bytes, each with oobSize
 * bytes of spare. Within one sector region - data followed by its spare - the
 * parity occupies eccBits bits starting at bit eccBitOffset, counted from the
 * first bit of the spare. Working in bits rather than bytes is what lets a
 * single model cover both the tidy schemes, where parity is byte aligned, and
 * real controller layouts such as Broadcom BCH-4, whose 52 bit parity starts
 * half way through spare byte 9.
 *
 * When coverSpare is set the message includes the spare bytes that precede the
 * parity, so user metadata is protected too. Otherwise only the sector data is
 * covered.
 */

enum EccAlgorithm
{
    ECC_ALGO_NONE = 0,
    ECC_ALGO_HAMMING,       /* 1 bit correct, 2 bit detect, per block */
    ECC_ALGO_BCH,           /* t bit correct over GF(2^m)            */
    ECC_ALGO_RS,            /* Reed-Solomon, t symbol correct        */
};

struct EccScheme
{
    EccAlgorithm algo;

    int sectorSize;         /* data bytes per ECC sector: 256, 512, 1024   */
    int oobSize;            /* spare bytes belonging to one sector         */

    int eccBitOffset;       /* parity start, in bits from the spare start  */
    bool coverSpare;        /* include leading spare bytes in the codeword */

    /* BCH and Reed-Solomon parameters. m is the Galois field order, t the
     * correction strength. primPoly may be 0 to take the conventional
     * polynomial for m.
     */
    int m;
    int t;
    uint32_t primPoly;

    /* Some encoders write only whole parity bytes and let the remaining high
     * bits fall into a spare byte they never clear. The reference Broadcom
     * tool does this and loses the top 4 of its 52 parity bits. Set this to
     * reproduce images made by such a tool; leave it clear for real silicon.
     */
    int truncateTopBits;

    EccScheme();

    /* Parity size implied by the algorithm and its parameters. Returns 0 when
     * the scheme is not valid.
     */
    int eccBits() const;
    int eccBytes() const;

    /* Bits of the sector region actually covered by the codeword. */
    int messageBits() const;

    /* Bit position of the parity within the sector region, i.e. counted from
     * the first data bit rather than from the spare.
     */
    int eccBitPos() const { return sectorSize * 8 + eccBitOffset; }

    int sectorsPerPage(int pageSize) const;

    /* True when the scheme is self consistent and fits the geometry. When it
     * is not, why receives a short reason suitable for showing in the UI.
     */
    bool validate(std::string &why) const;

    /* bbMarkOffset is the chip's bad block marker position, counted in bytes
     * from the start of the page spare area. Pass -1 when it is unknown, which
     * skips the check that parity does not land on top of it.
     */
    bool validateForPage(int pageSize, int spareSize, int bbMarkOffset,
        std::string &why) const;

    /* True when the parity region covers the given byte of a step's spare
     * slice. The parity sits at the same place in every slice, so this does
     * not depend on which step is being asked about.
     */
    bool parityCoversSpareByte(int byteInSlice) const;

    /* Zero bits a step may carry and still be treated as erased rather than
     * programmed. An erased block that has started to lose charge reads back
     * with a few zeros; without slack here every such page would be reported
     * uncorrectable.
     */
    int erasedBitTolerance() const;

    /* Largest number of bit errors the scheme repairs per sector. */
    int correctableBits() const;

    /* Named starting points. index 0 is always "None". */
    static int presetCount();
    static EccScheme preset(int index);
    static const char *presetName(int index);
    static const char *presetDescription(int index);

    /* Index of the preset matching this scheme exactly, or -1 when the
     * parameters have been edited into something custom.
     */
    int matchingPreset() const;
};

#endif /* ECC_SCHEME_H */
