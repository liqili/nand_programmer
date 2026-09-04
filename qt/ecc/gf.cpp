/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#include "gf.h"

uint32_t GaloisField::defaultPrimPoly(int m)
{
    /* Primitive polynomials over GF(2), indexed by degree. Bit k is the
     * coefficient of x^k, so 0x201b is x^13 + x^4 + x^3 + x + 1.
     */
    switch (m)
    {
    case 3:  return 0x000b;     /* x^3  + x + 1              */
    case 4:  return 0x0013;     /* x^4  + x + 1              */
    case 5:  return 0x0025;     /* x^5  + x^2 + 1            */
    case 6:  return 0x0043;     /* x^6  + x + 1              */
    case 7:  return 0x0083;     /* x^7  + x + 1              */
    case 8:  return 0x011d;     /* x^8  + x^4 + x^3 + x^2 + 1 */
    case 9:  return 0x0211;     /* x^9  + x^4 + 1            */
    case 10: return 0x0409;     /* x^10 + x^3 + 1            */
    case 11: return 0x0805;     /* x^11 + x^2 + 1            */
    case 12: return 0x1053;     /* x^12 + x^6 + x^4 + x + 1  */
    case 13: return 0x201b;     /* x^13 + x^4 + x^3 + x + 1  */
    case 14: return 0x402b;     /* x^14 + x^5 + x^3 + x + 1  */
    case 15: return 0x8003;     /* x^15 + x + 1              */
    default: return 0;
    }
}

GaloisField::GaloisField(int m, uint32_t primPoly) : m_m(m), m_n(0),
    m_prim(primPoly), m_valid(false)
{
    if (m < 3 || m > 15)
        return;

    if (!m_prim)
        m_prim = defaultPrimPoly(m);

    /* The polynomial must be of exactly degree m, otherwise the reduction
     * below would not keep elements inside the field.
     */
    if (m_prim >> m != 1)
        return;

    m_n = (1 << m) - 1;
    m_exp.assign(m_n, 0);
    m_log.assign(1u << m, 0);

    /* Walk the powers of alpha. Multiplying by alpha is a left shift, folded
     * back with the primitive polynomial whenever it overflows degree m - 1.
     */
    uint32_t x = 1;
    bool closedEarly = false;

    for (int i = 0; i < m_n; i++)
    {
        m_exp[i] = static_cast<uint16_t>(x);
        m_log[x] = static_cast<uint16_t>(i);

        x <<= 1;
        if (x & (1u << m))
            x ^= m_prim;

        /* Alpha must have full order. A polynomial that is irreducible but not
         * primitive sends it round a shorter cycle, which then repeats: the
         * walk still ends on 1, so testing only the final value would let it
         * through with most of the field missing from the tables.
         */
        if (x == 1 && i != m_n - 1)
        {
            closedEarly = true;
            break;
        }
    }

    if (closedEarly || x != 1)
    {
        m_exp.clear();
        m_log.clear();
        m_n = 0;
        return;
    }

    m_valid = true;
}
