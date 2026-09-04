/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#include "rs_codec.h"

RsCodec::RsCodec(int m, int t, uint32_t primPoly) : m_gf(m, primPoly), m_t(t),
    m_valid(false)
{
    if (!m_gf.isValid() || t < 1 || 2 * t >= m_gf.n())
        return;

    /* g(x) = product of (x + alpha^i) for i = 1 .. 2t, coefficients held low
     * degree first.
     */
    m_gen.assign(1, 1);

    for (int i = 1; i <= 2 * t; i++)
    {
        const uint16_t root = m_gf.exp(i);
        std::vector<uint16_t> next(m_gen.size() + 1, 0);

        for (size_t d = 0; d < m_gen.size(); d++)
        {
            next[d + 1] ^= m_gen[d];
            next[d] ^= m_gf.mul(m_gen[d], root);
        }

        m_gen.swap(next);
    }

    m_valid = true;
}

void RsCodec::bytesToSymbols(const uint8_t *data, size_t len, int m,
    std::vector<uint16_t> &out)
{
    const size_t bits = len * 8;
    const size_t count = (bits + m - 1) / m;

    out.assign(count, 0);

    for (size_t i = 0; i < count; i++)
    {
        uint16_t v = 0;

        for (int b = 0; b < m; b++)
        {
            const size_t bit = i * m + b;
            v <<= 1;
            if (bit < bits && ((data[bit >> 3] >> (7 - (bit & 7))) & 1))
                v |= 1;
        }

        out[i] = v;
    }
}

void RsCodec::symbolsToBytes(const std::vector<uint16_t> &symbols, int m,
    uint8_t *out, size_t len)
{
    const size_t bits = len * 8;

    for (size_t i = 0; i < len; i++)
        out[i] = 0;

    for (size_t i = 0; i < symbols.size(); i++)
    {
        for (int b = 0; b < m; b++)
        {
            const size_t bit = i * m + b;
            if (bit >= bits)
                break;

            if ((symbols[i] >> (m - 1 - b)) & 1)
                out[bit >> 3] |= static_cast<uint8_t>(1u << (7 - (bit & 7)));
        }
    }
}

void RsCodec::encode(const std::vector<uint16_t> &data,
    std::vector<uint16_t> &parity) const
{
    const int np = 2 * m_t;

    parity.assign(np, 0);

    if (!m_valid)
        return;

    /* Systematic encoding by long division: shift each message symbol in at
     * the top and fold the feedback back through the generator.
     */
    for (size_t i = 0; i < data.size(); i++)
    {
        const uint16_t fb = data[i] ^ parity[0];

        for (int j = 0; j < np - 1; j++)
            parity[j] = parity[j + 1] ^ m_gf.mul(fb, m_gen[np - 1 - j]);

        parity[np - 1] = m_gf.mul(fb, m_gen[0]);
    }
}

int RsCodec::correct(std::vector<uint16_t> &data,
    const std::vector<uint16_t> &recvParity) const
{
    const int np = 2 * m_t;

    if (!m_valid || static_cast<int>(recvParity.size()) != np)
        return -1;

    const int total = static_cast<int>(data.size()) + np;
    if (total > m_gf.n())
        return -1;

    /* Syndromes. Position j of the concatenated codeword carries degree
     * total - 1 - j.
     */
    std::vector<uint16_t> S(np, 0);
    bool clean = true;

    for (int i = 0; i < np; i++)
    {
        const int e = i + 1;
        uint16_t s = 0;

        for (int j = 0; j < total; j++)
        {
            const uint16_t sym = j < static_cast<int>(data.size()) ?
                data[j] : recvParity[j - data.size()];

            if (sym)
                s ^= m_gf.mul(sym, m_gf.exp(e * (total - 1 - j)));
        }

        S[i] = s;
        if (s)
            clean = false;
    }

    if (clean)
        return 0;

    /* Berlekamp-Massey for the error locator. */
    std::vector<uint16_t> C(np + 2, 0), B(np + 2, 0), T;
    C[0] = 1;
    B[0] = 1;
    int L = 0, shift = 1;
    uint16_t b = 1;

    for (int n = 0; n < np; n++)
    {
        uint16_t d = S[n];
        for (int i = 1; i <= L; i++)
            d ^= m_gf.mul(C[i], S[n - i]);

        if (!d)
        {
            shift++;
            continue;
        }

        const uint16_t coef = m_gf.div(d, b);
        const bool grow = 2 * L <= n;

        if (grow)
            T = C;

        for (size_t i = 0; i + shift < C.size(); i++)
        {
            if (B[i])
                C[i + shift] ^= m_gf.mul(coef, B[i]);
        }

        if (grow)
        {
            L = n + 1 - L;
            B.swap(T);
            b = d;
            shift = 1;
        }
        else
        {
            shift++;
        }
    }

    if (L <= 0 || L > m_t)
        return -1;

    /* Chien search: a root at alpha^-p marks an error at degree p. */
    std::vector<int> pos;
    for (int p = 0; p < total; p++)
    {
        uint16_t v = 0;
        for (int i = 0; i <= L; i++)
        {
            if (C[i])
                v ^= m_gf.mul(C[i], m_gf.exp(-i * p));
        }

        if (!v)
            pos.push_back(p);
    }

    if (static_cast<int>(pos.size()) != L)
        return -1;

    /* Error evaluator omega(x) = S(x) * lambda(x) mod x^2t. */
    std::vector<uint16_t> omega(np, 0);
    for (int i = 0; i < np; i++)
    {
        uint16_t acc = 0;
        for (int j = 0; j <= i && j <= L; j++)
        {
            if (C[j] && S[i - j])
                acc ^= m_gf.mul(C[j], S[i - j]);
        }
        omega[i] = acc;
    }

    /* Forney: the magnitude at position p is omega(x) over the formal
     * derivative of lambda(x), evaluated at x = alpha^-p. In characteristic 2
     * only the odd powers survive differentiation.
     */
    int corrected = 0;
    for (size_t k = 0; k < pos.size(); k++)
    {
        const int p = pos[k];
        uint16_t num = 0;
        for (int i = 0; i < np; i++)
        {
            if (omega[i])
                num ^= m_gf.mul(omega[i], m_gf.exp(-i * p));
        }

        uint16_t den = 0;
        for (int i = 1; i <= L; i += 2)
        {
            if (C[i])
                den ^= m_gf.mul(C[i], m_gf.exp(-(i - 1) * p));
        }

        if (!den)
            return -1;

        /* The first generator root is alpha^1, so the usual X^(1-b) factor is
         * unity and the magnitude is just the ratio.
         */
        const uint16_t mag = m_gf.div(num, den);

        const int idx = total - 1 - p;
        if (idx < 0 || idx >= total)
            return -1;

        /* Only data symbols are repaired; a hit inside the parity leaves the
         * message already correct.
         */
        if (idx < static_cast<int>(data.size()))
            data[idx] ^= mag;

        corrected++;
    }

    return corrected;
}
