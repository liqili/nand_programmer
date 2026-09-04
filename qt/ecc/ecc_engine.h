/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#ifndef ECC_ENGINE_H
#define ECC_ENGINE_H

#include "ecc_scheme.h"

#include <cstdint>
#include <string>
#include <vector>

/* Applies an EccScheme to whole pages.
 *
 * A page buffer is pageSize data bytes followed by spareSize spare bytes, the
 * layout the programmer already moves over USB. The engine splits that into
 * sectors, and for each one builds the codeword as a bit string over the
 * sector data followed by its slice of spare. Parity is read and written at
 * the bit offset the scheme specifies, so byte aligned and half byte aligned
 * layouts are handled by the same code path.
 */

enum EccSectorStatus
{
    ECC_SECTOR_CLEAN = 0,       /* parity agreed with the data          */
    ECC_SECTOR_CORRECTED,       /* errors found and repaired            */
    ECC_SECTOR_UNCORRECTABLE,   /* beyond the strength of the scheme    */
    ECC_SECTOR_ERASED,          /* all ones, never programmed           */
};

struct EccSectorResult
{
    int sector;
    EccSectorStatus status;
    int errors;                 /* bits for BCH and Hamming, symbols for RS;
                                 * for an erased step, the zero bits found  */
};

struct EccPageResult
{
    int sectors;
    int clean;
    int corrected;
    int uncorrectable;
    int erased;
    int worstErrors;            /* highest per sector error count seen */
    std::vector<EccSectorResult> details;

    EccPageResult();

    bool ok() const { return uncorrectable == 0; }
};

class EccEngine
{
public:
    EccEngine();

    /* Bind a scheme to a chip geometry. bbMarkOffset is the chip's bad block
     * marker position within the page spare area, or -1 when unknown. Returns
     * false and fills why when the scheme cannot be applied.
     */
    bool setScheme(const EccScheme &scheme, int pageSize, int spareSize,
        int bbMarkOffset, std::string &why);

    const EccScheme &scheme() const { return m_scheme; }
    bool isEnabled() const { return m_scheme.algo != ECC_ALGO_NONE; }

    int sectorsPerPage() const { return m_sectors; }

    /* Bytes in one transferred page, data plus spare. This is the unit the
     * stream adapter has to align to.
     */
    int pageBytes() const { return m_pageSize + m_spareSize; }

    /* Generate parity for every sector of a page, writing it into the spare
     * area. Sectors whose data is entirely 0xFF are left untouched, matching
     * what controllers do with erased pages.
     *
     * page must hold pageSize + spareSize bytes.
     */
    void encodePage(uint8_t *page) const;

    /* Check every sector against its stored parity. When correct is set,
     * repairable errors are fixed in place.
     */
    EccPageResult decodePage(uint8_t *page, bool correct) const;

    /* Human readable strength, phrased per algorithm so that symbol based and
     * bit based codes are not silently compared as if they were the same.
     */
    std::string strengthText() const;
    static std::string strengthText(const EccScheme &scheme);

private:
    /* Exact test, used when generating: a step of all 0xFF is left alone so
     * an erased page stays fully erased.
     */
    bool sectorIsBlank(const uint8_t *page, int sector) const;

    /* Tolerant test, used when checking: returns the number of zero bits in
     * the step when it is close enough to erased to count as such, or -1 when
     * the step carries real data. Covers the spare slice as well as the data.
     */
    int sectorErasedBits(const uint8_t *page, int sector) const;

    EccScheme m_scheme;
    int m_pageSize;
    int m_spareSize;
    int m_sectors;
};

#endif /* ECC_ENGINE_H */
