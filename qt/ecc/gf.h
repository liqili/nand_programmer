/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#ifndef GF_H
#define GF_H

#include <cstdint>
#include <vector>

/* Arithmetic in GF(2^m) for 3 <= m <= 15.
 *
 * Field elements are plain integers in [0, 2^m): bit k holds the coefficient
 * of alpha^k in the polynomial basis. Products go through log/antilog tables,
 * which is what keeps the syndrome and Chien search loops in the BCH and
 * Reed-Solomon codecs cheap enough to run over a whole chip image.
 *
 * The tables cost 2 * 2^m entries of 16 bits. That is 32 KB at m = 13, which
 * is why ECC lives in the host application and not in the STM32 firmware.
 */
class GaloisField
{
public:
    /* primPoly is the primitive polynomial as a bit mask, including the x^m
     * term, e.g. 0x201b for x^13 + x^4 + x^3 + x + 1. Pass 0 to take the
     * conventional polynomial for this m.
     */
    explicit GaloisField(int m, uint32_t primPoly = 0);

    bool isValid() const { return m_valid; }

    int m() const { return m_m; }

    /* Order of alpha, and the number of non-zero elements: 2^m - 1. This is
     * also the maximum codeword length in bits for a BCH code over the field.
     */
    int n() const { return m_n; }

    uint32_t primPoly() const { return m_prim; }

    /* alpha^i. i may be negative or >= n, it is reduced modulo n. */
    uint16_t exp(int i) const
    {
        i %= m_n;
        if (i < 0)
            i += m_n;
        return m_exp[i];
    }

    /* Base-alpha logarithm. log(0) is undefined; it returns 0 so that callers
     * which guard against zero separately stay in bounds.
     */
    uint16_t log(uint16_t a) const { return m_log[a]; }

    uint16_t mul(uint16_t a, uint16_t b) const
    {
        if (!a || !b)
            return 0;

        int s = m_log[a] + m_log[b];
        if (s >= m_n)
            s -= m_n;

        return m_exp[s];
    }

    /* Multiplicative inverse. inv(0) is undefined and returns 0. */
    uint16_t inv(uint16_t a) const
    {
        return a ? exp(m_n - m_log[a]) : 0;
    }

    uint16_t div(uint16_t a, uint16_t b) const
    {
        if (!a || !b)
            return 0;

        int s = m_log[a] - m_log[b];
        if (s < 0)
            s += m_n;

        return m_exp[s];
    }

    uint16_t sqr(uint16_t a) const { return mul(a, a); }

    /* The conventional primitive polynomial for a given m. These are the same
     * ones NAND controllers and the Linux BCH library assume, so codes built
     * on them interoperate; m = 13 gives 0x201b, which the Broadcom BCH-4
     * scheme relies on.
     */
    static uint32_t defaultPrimPoly(int m);

private:
    int m_m;
    int m_n;
    uint32_t m_prim;
    bool m_valid;
    std::vector<uint16_t> m_exp;    /* alpha^i for i in [0, n)   */
    std::vector<uint16_t> m_log;    /* log_alpha(a) for a in [0, 2^m) */
};

#endif /* GF_H */
