/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#ifndef BCH_CODEC_H
#define BCH_CODEC_H

#include "gf.h"

#include <cstdint>
#include <cstddef>
#include <vector>

/* Binary BCH encoder and decoder over GF(2^m), correcting up to t bit errors.
 *
 * Bit convention: data is processed most significant bit first within each
 * byte, and the parity is returned right aligned in eccBytes() bytes. That is
 * the convention NAND controllers and the Linux BCH library use, so codes
 * produced here interoperate with codes found on real devices.
 *
 * The codec deliberately knows nothing about pages, sectors or spare area
 * layout. Schemes that need an unusual alignment - the Broadcom BCH-4 layout
 * shifts its message by half a byte, for instance - pre-shift the buffer and
 * hand a plain byte string to encode().
 */
class BchCodec
{
public:
    /* m is the Galois field order, t the correction strength in bits.
     * primPoly may be 0 to take the conventional polynomial for this m.
     */
    BchCodec(int m, int t, uint32_t primPoly = 0);

    bool isValid() const { return m_valid; }

    int m() const { return m_gf.m(); }
    int t() const { return m_t; }
    uint32_t primPoly() const { return m_gf.primPoly(); }

    /* Degree of the generator polynomial. This is at most m * t and is
     * usually exactly m * t, but can be less when the cyclotomic cosets
     * overlap.
     */
    int eccBits() const { return m_eccBits; }

    int eccBytes() const { return (m_eccBits + 7) / 8; }

    /* Longest message the code can protect, in bits and in whole bytes. */
    int maxDataBits() const { return m_gf.n() - m_eccBits; }
    int maxDataBytes() const { return maxDataBits() / 8; }

    /* Compute parity over len data bytes. ecc must have room for eccBytes()
     * and is fully overwritten.
     */
    void encode(const uint8_t *data, size_t len, uint8_t *ecc) const;

    /* Locate errors in a received codeword without modifying it.
     *
     * Returns the number of errors found, 0 when the codeword is clean, or
     * -1 when the error pattern is beyond the correction capability. On
     * success errLoc receives one entry per error, each a bit index into the
     * concatenation of the data bytes followed by the ecc bytes, counted MSB
     * first from the start of the data. Indices at or past len * 8 fall in
     * the parity itself, where there is nothing to repair in the data.
     */
    int decode(const uint8_t *data, size_t len, const uint8_t *recvEcc,
        std::vector<int> &errLoc) const;

    /* decode() followed by flipping every located bit that lands in the data.
     * Returns the same value as decode().
     */
    int correct(uint8_t *data, size_t len, const uint8_t *recvEcc) const;

private:
    void buildGenerator();
    void buildTable();

    /* The parity register, held as 32 bit words with bit i of the polynomial
     * in word i / 32. Only the low m_eccBits bits are ever set.
     */
    typedef std::vector<uint32_t> Reg;

    void regShiftXor(Reg &r, uint8_t byteIn) const;
    uint16_t evalSyndrome(const uint8_t *data, size_t len,
        const uint8_t *ecc, int j) const;

    GaloisField m_gf;
    int m_t;
    int m_eccBits;
    int m_words;
    bool m_valid;

    Reg m_gen;                      /* generator polynomial, x^eccBits dropped */
    std::vector<Reg> m_table;       /* byte-wise feedback table, 256 entries   */
};

#endif /* BCH_CODEC_H */
