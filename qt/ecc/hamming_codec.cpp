/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#include "hamming_codec.h"

namespace
{

inline uint8_t parity8(uint8_t v)
{
    v ^= v >> 4;
    v ^= v >> 2;
    v ^= v >> 1;

    return v & 1;
}

/* Column parity pairs. Each entry masks the bit positions inside a byte whose
 * contributions form one half of a complementary pair, so cpMask[k] and its
 * complement together cover all eight bits.
 */
const uint8_t cpMask[3] = { 0xAA, 0xCC, 0xF0 };

} /* namespace */

void HammingCodec::encode(const uint8_t *data, uint8_t *ecc)
{
    uint8_t cp[6] = { 0, 0, 0, 0, 0, 0 };
    uint16_t rp = 0;        /* 8 pairs of line parity, even in the low bit */
    uint8_t rpOdd[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    uint8_t rpEven[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

    for (int i = 0; i < BLOCK_SIZE; i++)
    {
        const uint8_t b = data[i];

        /* Column parity: the six sums over halves of the bit positions. */
        for (int k = 0; k < 3; k++)
        {
            cp[2 * k] ^= parity8(b & cpMask[k]);
            cp[2 * k + 1] ^= parity8(b & static_cast<uint8_t>(~cpMask[k]));
        }

        /* Line parity: the byte contributes to one half of each address pair
         * according to the bits of its index.
         */
        const uint8_t p = parity8(b);
        if (p)
        {
            for (int k = 0; k < 8; k++)
            {
                if ((i >> k) & 1)
                    rpOdd[k] ^= 1;
                else
                    rpEven[k] ^= 1;
            }
        }
    }

    for (int k = 0; k < 8; k++)
        rp |= static_cast<uint16_t>((rpOdd[k] << (2 * k)) | (rpEven[k] << (2 * k + 1)));

    /* Pack line parity into the first two bytes and column parity into the
     * third, leaving its top two bits set so an all-ones spare area reads as
     * an erased, unwritten block.
     */
    ecc[0] = static_cast<uint8_t>(rp & 0xFF);
    ecc[1] = static_cast<uint8_t>((rp >> 8) & 0xFF);
    ecc[2] = static_cast<uint8_t>(cp[0] | (cp[1] << 1) | (cp[2] << 2) |
        (cp[3] << 3) | (cp[4] << 4) | (cp[5] << 5) | 0xC0);
}

HammingCodec::Result HammingCodec::correct(uint8_t *data,
    const uint8_t *recvEcc)
{
    uint8_t calc[ECC_BYTES];

    encode(data, calc);

    const uint8_t d0 = calc[0] ^ recvEcc[0];
    const uint8_t d1 = calc[1] ^ recvEcc[1];
    const uint8_t d2 = static_cast<uint8_t>((calc[2] ^ recvEcc[2]) & 0x3F);

    if (!d0 && !d1 && !d2)
        return HAMMING_CLEAN;

    /* A single flipped data bit shows up as exactly one bit set in each
     * complementary pair, so all 11 pairs disagree: 8 line pairs across d0 and
     * d1, and 3 column pairs in d2.
     */
    const uint16_t line = static_cast<uint16_t>(d0 | (d1 << 8));
    int lineOk = 0;
    uint8_t byteOff = 0;

    for (int k = 0; k < 8; k++)
    {
        const int odd = (line >> (2 * k)) & 1;
        const int even = (line >> (2 * k + 1)) & 1;

        if (odd ^ even)
        {
            lineOk++;
            if (odd)
                byteOff |= static_cast<uint8_t>(1u << k);
        }
    }

    int colOk = 0;
    uint8_t bitOff = 0;

    for (int k = 0; k < 3; k++)
    {
        const int lo = (d2 >> (2 * k)) & 1;
        const int hi = (d2 >> (2 * k + 1)) & 1;

        if (lo ^ hi)
        {
            colOk++;
            if (lo)
                bitOff |= static_cast<uint8_t>(1u << k);
        }
    }

    if (lineOk == 8 && colOk == 3)
    {
        data[byteOff] ^= static_cast<uint8_t>(1u << bitOff);
        return HAMMING_CORRECTED;
    }

    /* A single set bit overall means the stored parity took the hit, not the
     * data, so the block itself is intact.
     */
    int total = 0;
    for (int k = 0; k < 8; k++)
        total += (d0 >> k) & 1;
    for (int k = 0; k < 8; k++)
        total += (d1 >> k) & 1;
    for (int k = 0; k < 6; k++)
        total += (d2 >> k) & 1;

    if (total == 1)
        return HAMMING_ECC_ERROR;

    return HAMMING_UNCORRECTABLE;
}
