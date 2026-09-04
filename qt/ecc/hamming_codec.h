/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#ifndef HAMMING_CODEC_H
#define HAMMING_CODEC_H

#include <cstdint>
#include <cstddef>

/* Single error correcting, double error detecting Hamming code over a 256
 * byte block, the arrangement NAND controllers have used since SmartMedia.
 *
 * Parity is 22 bits held in 3 bytes: 16 line parity bits selecting the byte
 * and 6 column parity bits selecting the bit within it, each stored as a
 * complementary pair so that a single flip anywhere is locatable and a double
 * flip is distinguishable from a single one.
 *
 * Blocks larger than 256 bytes are handled by the caller as a sequence of
 * independent 256 byte blocks, three parity bytes each.
 */
class HammingCodec
{
public:
    enum Result
    {
        HAMMING_CLEAN = 0,      /* no discrepancy                       */
        HAMMING_CORRECTED,      /* one data bit repaired                */
        HAMMING_ECC_ERROR,      /* flip was in the stored parity itself */
        HAMMING_UNCORRECTABLE,  /* two or more bits in error            */
    };

    static const int BLOCK_SIZE = 256;
    static const int ECC_BYTES = 3;

    /* Compute 3 parity bytes over a 256 byte block. */
    static void encode(const uint8_t *data, uint8_t *ecc);

    /* Compare stored parity against the block and repair a single bit flip in
     * place. data is modified only for HAMMING_CORRECTED.
     */
    static Result correct(uint8_t *data, const uint8_t *recvEcc);
};

#endif /* HAMMING_CODEC_H */
