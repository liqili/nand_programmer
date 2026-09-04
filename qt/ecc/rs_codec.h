/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#ifndef RS_CODEC_H
#define RS_CODEC_H

#include "gf.h"

#include <cstdint>
#include <cstddef>
#include <vector>

/* Reed-Solomon over GF(2^m), correcting up to t symbol errors with 2t parity
 * symbols.
 *
 * Where BCH counts individual bit flips, Reed-Solomon counts symbols: a symbol
 * with all m bits wrong costs the same as a symbol with one bit wrong. That
 * makes it the better fit for burst errors, which is why older NAND
 * controllers reached for it.
 *
 * The codec works in symbols. Data arriving as bytes is packed most
 * significant bit first into m bit symbols by bytesToSymbols(), with the final
 * symbol zero padded when the byte count is not a whole number of symbols.
 */
class RsCodec
{
public:
    RsCodec(int m, int t, uint32_t primPoly = 0);

    bool isValid() const { return m_valid; }

    int m() const { return m_gf.m(); }
    int t() const { return m_t; }

    int paritySymbols() const { return 2 * m_t; }
    int parityBits() const { return 2 * m_t * m_gf.m(); }
    int parityBytes() const { return (parityBits() + 7) / 8; }

    /* Longest message the field allows, in symbols. */
    int maxDataSymbols() const { return m_gf.n() - 2 * m_t; }

    /* Compute 2t parity symbols over the message. */
    void encode(const std::vector<uint16_t> &data,
        std::vector<uint16_t> &parity) const;

    /* Repair the message in place. Returns the number of symbols corrected,
     * 0 when clean, or -1 when the error pattern exceeds t symbols.
     */
    int correct(std::vector<uint16_t> &data,
        const std::vector<uint16_t> &recvParity) const;

    /* Pack len bytes into ceil(len * 8 / m) symbols, most significant bit
     * first, zero padding the last symbol.
     */
    static void bytesToSymbols(const uint8_t *data, size_t len, int m,
        std::vector<uint16_t> &out);

    /* Inverse of bytesToSymbols, writing len bytes. */
    static void symbolsToBytes(const std::vector<uint16_t> &symbols, int m,
        uint8_t *out, size_t len);

private:
    GaloisField m_gf;
    int m_t;
    bool m_valid;
    std::vector<uint16_t> m_gen;    /* generator polynomial, low degree first */
};

#endif /* RS_CODEC_H */
