/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#include "bch_codec.h"

#include <algorithm>

namespace
{

inline bool regBit(const std::vector<uint32_t> &r, int i)
{
    return (r[i >> 5] >> (i & 31)) & 1;
}

inline void regSetBit(std::vector<uint32_t> &r, int i)
{
    r[i >> 5] |= 1u << (i & 31);
}

/* r <<= 1 across the whole register. */
inline void regShl1(std::vector<uint32_t> &r)
{
    uint32_t carry = 0;

    for (size_t i = 0; i < r.size(); i++)
    {
        uint32_t next = r[i] >> 31;
        r[i] = (r[i] << 1) | carry;
        carry = next;
    }
}

inline void regXor(std::vector<uint32_t> &r, const std::vector<uint32_t> &o)
{
    for (size_t i = 0; i < r.size(); i++)
        r[i] ^= o[i];
}

/* Clear everything at or above bit n, so the register stays a polynomial of
 * degree less than n.
 */
inline void regTrim(std::vector<uint32_t> &r, int n)
{
    for (size_t i = 0; i < r.size(); i++)
    {
        int base = static_cast<int>(i) * 32;
        if (base >= n)
            r[i] = 0;
        else if (base + 32 > n)
            r[i] &= (1u << (n - base)) - 1;
    }
}

/* Multiply two polynomials over GF(2), coefficients held one per entry. */
std::vector<uint8_t> polyMulBinary(const std::vector<uint8_t> &a,
    const std::vector<uint8_t> &b)
{
    std::vector<uint8_t> r(a.size() + b.size() - 1, 0);

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a[i])
            continue;
        for (size_t j = 0; j < b.size(); j++)
            r[i + j] ^= b[j];
    }

    return r;
}

} /* namespace */

BchCodec::BchCodec(int m, int t, uint32_t primPoly) : m_gf(m, primPoly),
    m_t(t), m_eccBits(0), m_words(0), m_valid(false)
{
    if (!m_gf.isValid() || t < 1)
        return;

    /* The code needs 2t distinct roots inside the field, and at least one
     * data bit left over once parity is accounted for.
     */
    if (2 * t >= m_gf.n())
        return;

    buildGenerator();
    if (!m_valid)
        return;

    if (maxDataBits() <= 0)
    {
        m_valid = false;
        return;
    }

    buildTable();
}

/* g(x) is the least common multiple of the minimal polynomials of
 * alpha^1 .. alpha^2t. Over GF(2) the minimal polynomial of alpha^i and of
 * alpha^2i coincide, so only the odd powers contribute a new factor.
 */
void BchCodec::buildGenerator()
{
    const int n = m_gf.n();
    std::vector<uint8_t> seen(n + 1, 0);
    std::vector<uint8_t> g(1, 1);

    for (int i = 1; i <= 2 * m_t; i += 2)
    {
        if (seen[i])
            continue;

        /* Cyclotomic coset of i: { i, 2i, 4i, ... } modulo n. */
        std::vector<int> coset;
        int j = i;
        do
        {
            seen[j] = 1;
            coset.push_back(j);
            j = (j * 2) % n;
        }
        while (j != i);

        /* Minimal polynomial is the product of (x + alpha^j) over the coset.
         * Coefficients live in GF(2^m) while the product is being formed and
         * must collapse to 0 or 1 once the coset is complete.
         */
        std::vector<uint16_t> p(1, 1);
        for (size_t k = 0; k < coset.size(); k++)
        {
            const uint16_t aj = m_gf.exp(coset[k]);
            std::vector<uint16_t> q(p.size() + 1, 0);

            for (size_t d = 0; d < p.size(); d++)
            {
                q[d + 1] ^= p[d];
                q[d] ^= m_gf.mul(p[d], aj);
            }

            p.swap(q);
        }

        std::vector<uint8_t> minPoly(p.size(), 0);
        for (size_t d = 0; d < p.size(); d++)
        {
            if (p[d] > 1)
                return;         /* not a binary polynomial, field is wrong */
            minPoly[d] = static_cast<uint8_t>(p[d]);
        }

        g = polyMulBinary(g, minPoly);
    }

    m_eccBits = static_cast<int>(g.size()) - 1;
    if (m_eccBits <= 0 || !g[m_eccBits])
        return;

    /* Store g without its leading x^eccBits term, which is implicit in the
     * shift register update.
     */
    m_words = (m_eccBits + 31) / 32;
    m_gen.assign(m_words, 0);
    for (int i = 0; i < m_eccBits; i++)
    {
        if (g[i])
            regSetBit(m_gen, i);
    }

    m_valid = true;
}

/* Feed one bit into the shift register. */
static inline void lfsrStep(std::vector<uint32_t> &r,
    const std::vector<uint32_t> &gen, int eccBits, int bit)
{
    const int fb = regBit(r, eccBits - 1) ^ bit;

    regShl1(r);
    regTrim(r, eccBits);

    if (fb)
        regXor(r, gen);
}

/* Precompute the effect of clocking eight zero bits through the register for
 * each possible value of its top byte, so encode() can advance a byte at a
 * time instead of a bit at a time.
 */
void BchCodec::buildTable()
{
    m_table.assign(256, Reg(m_words, 0));

    for (int v = 0; v < 256; v++)
    {
        Reg r(m_words, 0);

        /* Place v at the top of the register, most significant bit first, to
         * match how regShiftXor() reads it back out, then clock it through.
         */
        for (int b = 0; b < 8; b++)
        {
            if ((v >> (7 - b)) & 1)
                regSetBit(r, m_eccBits - 1 - b);
        }
        regTrim(r, m_eccBits);

        for (int b = 0; b < 8; b++)
            lfsrStep(r, m_gen, m_eccBits, 0);

        m_table[v] = r;
    }
}

void BchCodec::regShiftXor(Reg &r, uint8_t byteIn) const
{
    /* Top byte of the register, XOR the incoming byte, selects the feedback. */
    uint8_t top = 0;
    for (int b = 0; b < 8; b++)
    {
        if (regBit(r, m_eccBits - 1 - b))
            top |= 1u << (7 - b);
    }

    const uint8_t idx = top ^ byteIn;

    for (int b = 0; b < 8; b++)
    {
        regShl1(r);
        regTrim(r, m_eccBits);
    }

    regXor(r, m_table[idx]);
}

void BchCodec::encode(const uint8_t *data, size_t len, uint8_t *ecc) const
{
    const int nb = eccBytes();

    if (!m_valid)
    {
        for (int i = 0; i < nb; i++)
            ecc[i] = 0;
        return;
    }

    Reg r(m_words, 0);

    /* The byte-wise update needs at least a byte of register to work with.
     * Very short codes fall back to clocking single bits.
     */
    if (m_eccBits >= 8)
    {
        for (size_t i = 0; i < len; i++)
            regShiftXor(r, data[i]);
    }
    else
    {
        for (size_t i = 0; i < len; i++)
        {
            for (int b = 7; b >= 0; b--)
                lfsrStep(r, m_gen, m_eccBits, (data[i] >> b) & 1);
        }
    }

    /* Emit the remainder right aligned and most significant bit first, so the
     * last ecc byte carries bits 7..0 of the register.
     */
    for (int i = 0; i < nb; i++)
    {
        uint8_t v = 0;
        for (int b = 0; b < 8; b++)
        {
            const int bit = (nb - 1 - i) * 8 + (7 - b);
            if (bit < m_eccBits && regBit(r, bit))
                v |= 1u << (7 - b);
        }
        ecc[i] = v;
    }
}

/* Evaluate the received codeword at alpha^j. The codeword runs data bytes
 * first then parity, most significant bit first, with the very first bit
 * carrying the highest power of x.
 */
uint16_t BchCodec::evalSyndrome(const uint8_t *data, size_t len,
    const uint8_t *ecc, int j) const
{
    const int nb = eccBytes();
    const int total = static_cast<int>(len) * 8 + m_eccBits;
    uint16_t s = 0;
    int idx = 0;

    for (size_t i = 0; i < len; i++)
    {
        uint8_t v = data[i];
        for (int b = 7; b >= 0; b--, idx++)
        {
            if ((v >> b) & 1)
                s ^= m_gf.exp(j * (total - 1 - idx));
        }
    }

    /* Parity is right aligned in nb bytes, so the leading bits of ecc[0] are
     * padding and are skipped.
     */
    const int pad = nb * 8 - m_eccBits;
    for (int i = 0; i < nb * 8; i++)
    {
        if (i < pad)
            continue;

        if ((ecc[i >> 3] >> (7 - (i & 7))) & 1)
            s ^= m_gf.exp(j * (total - 1 - idx));
        idx++;
    }

    return s;
}

int BchCodec::decode(const uint8_t *data, size_t len, const uint8_t *recvEcc,
    std::vector<int> &errLoc) const
{
    errLoc.clear();

    if (!m_valid)
        return -1;

    const int total = static_cast<int>(len) * 8 + m_eccBits;
    if (total > m_gf.n())
        return -1;

    /* Syndromes S_1 .. S_2t. A clean codeword gives all zeroes. */
    std::vector<uint16_t> S(2 * m_t + 1, 0);
    bool clean = true;
    for (int j = 1; j <= 2 * m_t; j++)
    {
        S[j] = evalSyndrome(data, len, recvEcc, j);
        if (S[j])
            clean = false;
    }

    if (clean)
        return 0;

    /* Berlekamp-Massey. Over GF(2) subtraction is exclusive or, so the update
     * of the connection polynomial is a straight XOR of a scaled shift.
     */
    std::vector<uint16_t> C(2 * m_t + 2, 0), B(2 * m_t + 2, 0), T;
    C[0] = 1;
    B[0] = 1;
    int L = 0, shift = 1;
    uint16_t b = 1;

    for (int n = 0; n < 2 * m_t; n++)
    {
        uint16_t d = S[n + 1];
        for (int i = 1; i <= L; i++)
            d ^= m_gf.mul(C[i], S[n + 1 - i]);

        if (!d)
        {
            shift++;
            continue;
        }

        const uint16_t coef = m_gf.div(d, b);

        if (2 * L <= n)
            T = C;

        for (size_t i = 0; i + shift < C.size(); i++)
        {
            if (B[i])
                C[i + shift] ^= m_gf.mul(coef, B[i]);
        }

        if (2 * L <= n)
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

    /* Chien search. The error locator has a root at alpha^-p for every error
     * at degree p, so a codeword position is faulty when the polynomial
     * evaluates to zero there.
     */
    for (int p = 0; p < total; p++)
    {
        uint16_t v = 0;
        for (int i = 0; i <= L; i++)
        {
            if (C[i])
                v ^= m_gf.mul(C[i], m_gf.exp(-i * p));
        }

        if (!v)
            errLoc.push_back(total - 1 - p);
    }

    /* A genuine error pattern of weight L yields exactly L distinct roots.
     * Anything else means the received word is closer to some other codeword
     * and cannot be repaired.
     */
    if (static_cast<int>(errLoc.size()) != L)
    {
        errLoc.clear();
        return -1;
    }

    std::sort(errLoc.begin(), errLoc.end());

    return L;
}

int BchCodec::correct(uint8_t *data, size_t len, const uint8_t *recvEcc) const
{
    std::vector<int> loc;
    const int rc = decode(data, len, recvEcc, loc);

    if (rc <= 0)
        return rc;

    const int dataBits = static_cast<int>(len) * 8;
    for (size_t i = 0; i < loc.size(); i++)
    {
        const int bit = loc[i];
        if (bit < dataBits)
            data[bit >> 3] ^= 1u << (7 - (bit & 7));
    }

    return rc;
}
