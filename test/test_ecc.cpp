/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *
 *  Host tests for the ECC engine used by the Qt application. Nothing here
 *  touches Qt or the target, so the codecs, the layout model, the page stream
 *  and the image probe can all be exercised natively.
 */

#include "gf.h"
#include "bch_codec.h"
#include "hamming_codec.h"
#include "rs_codec.h"
#include "ecc_scheme.h"
#include "ecc_engine.h"
#include "ecc_stream.h"
#include "image_probe.h"
#include "ecc_golden.h"
#include "ecc_page.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>

static int failures;

static void expect(bool cond, const char *what)
{
    if (cond)
    {
        printf("  ok   %s\n", what);
    }
    else
    {
        printf("  FAIL %s\n", what);
        failures++;
    }
}

/* ------------------------------------------------------------ Galois -- */

static void testGaloisField()
{
    printf("-- Galois field --\n");

    GaloisField gf(13);
    expect(gf.isValid(), "GF(2^13) builds");
    expect(gf.primPoly() == 0x201b, "conventional polynomial for m = 13 is 0x201b");
    expect(gf.n() == 8191, "8191 non-zero elements");

    bool inverses = true, assoc = true;
    for (int i = 1; i <= 500; i++)
    {
        const uint16_t a = gf.exp(i * 7);
        if (gf.mul(a, gf.inv(a)) != 1)
            inverses = false;
        const uint16_t b = gf.exp(i * 13), c = gf.exp(i * 29);
        if (gf.mul(gf.mul(a, b), c) != gf.mul(a, gf.mul(b, c)))
            assoc = false;
    }
    expect(inverses, "a * a^-1 == 1 across the field");
    expect(assoc, "multiplication is associative");

    /* A polynomial that is irreducible but not primitive must be rejected
     * rather than producing a log table with holes in it. x^4+x^3+x^2+x+1 is
     * the classic trap: alpha has order 5, so the walk closes early and then
     * repeats, ending back on 1 as though nothing were wrong.
     */
    GaloisField bad(4, 0x1F);
    expect(!bad.isValid(), "an irreducible but non-primitive polynomial is rejected");
    expect(GaloisField(4, 0x13).isValid(), "a primitive one of the same degree is not");

    /* Stronger than any single case: the number of primitive polynomials of
     * degree m over GF(2) is phi(2^m - 1) / m. Accepting exactly that many at
     * every m shows the test is neither too strict nor too lax.
     */
    bool counted = true;
    for (int m = 3; m <= 12; m++)
    {
        long n = (1L << m) - 1, t = n, r = n, acc = 0;

        for (long p = 2; p * p <= t; p++)
        {
            if (t % p == 0)
            {
                while (t % p == 0)
                    t /= p;
                r -= r / p;
            }
        }
        if (t > 1)
            r -= r / t;

        for (uint32_t poly = (1u << m) + 1; poly < (2u << m); poly += 2)
        {
            if (GaloisField(m, poly).isValid())
                acc++;
        }

        if (acc != r / m)
        {
            printf("       m = %d: accepted %ld, expected %ld\n", m, acc, r / m);
            counted = false;
        }
    }
    expect(counted, "exactly phi(2^m - 1) / m polynomials accepted, m = 3..12");
}

/* --------------------------------------------------------------- BCH -- */

static void testBch()
{
    printf("-- BCH --\n");

    BchCodec bch(13, 4, 0x201b);
    expect(bch.isValid(), "BCH(13, 4) builds");
    expect(bch.eccBits() == 52, "parity is m * t = 52 bits");
    expect(bch.eccBytes() == 7, "parity occupies 7 bytes");

    const size_t len = 522;
    std::vector<uint8_t> d(len), d2(len), ecc(8);

    srand(1234);
    for (size_t i = 0; i < len; i++)
        d[i] = static_cast<uint8_t>(rand());
    bch.encode(&d[0], len, &ecc[0]);

    std::vector<int> loc;
    expect(bch.decode(&d[0], len, &ecc[0], loc) == 0, "clean codeword decodes clean");

    for (int ne = 1; ne <= 4; ne++)
    {
        bool ok = true;
        for (int trial = 0; trial < 200 && ok; trial++)
        {
            d2 = d;
            int bits[4];
            for (int k = 0; k < ne; k++)
            {
                int b, dup;
                do
                {
                    b = rand() % static_cast<int>(len * 8);
                    dup = 0;
                    for (int q = 0; q < k; q++)
                        if (bits[q] == b) dup = 1;
                }
                while (dup);
                bits[k] = b;
                d2[b >> 3] ^= static_cast<uint8_t>(1u << (7 - (b & 7)));
            }
            if (bch.correct(&d2[0], len, &ecc[0]) != ne || d2 != d)
                ok = false;
        }
        char msg[80];
        snprintf(msg, sizeof(msg), "corrects %d bit error(s), 200 trials", ne);
        expect(ok, msg);
    }

    /* Beyond t the code owes nothing: a weight-5 pattern can sit closer to a
     * different codeword. What must hold is that it is nearly always flagged.
     */
    int flagged = 0, mis = 0;
    for (int trial = 0; trial < 200; trial++)
    {
        d2 = d;
        int bits[5];
        for (int k = 0; k < 5; k++)
        {
            int b, dup;
            do
            {
                b = rand() % static_cast<int>(len * 8);
                dup = 0;
                for (int q = 0; q < k; q++)
                    if (bits[q] == b) dup = 1;
            }
            while (dup);
            bits[k] = b;
            d2[b >> 3] ^= static_cast<uint8_t>(1u << (7 - (b & 7)));
        }
        const int rc = bch.correct(&d2[0], len, &ecc[0]);
        if (rc < 0)
            flagged++;
        else if (d2 != d)
            mis++;
    }
    printf("       5 distinct errors: %d flagged, %d miscorrected of 200\n",
        flagged, mis);
    expect(flagged + mis == 200, "over-strength damage is flagged or miscorrected");
    expect(flagged >= 190, "the large majority are flagged");

    std::vector<uint8_t> e2 = ecc;
    e2[6] ^= 0x01;
    d2 = d;
    expect(bch.correct(&d2[0], len, &e2[0]) == 1 && d2 == d,
        "a flip in the parity is located, data left alone");
}

/* ------------------------------------------------------------ golden -- */

/* The reference encoder's layout: the codeword is the whole 528 byte sector
 * region and the parity is its final 52 bits, of which only the low 48 are
 * actually stored. Presented to the codec as 522 whole bytes by shifting the
 * message right 4 bits; leading zeros do not change a remainder.
 */
static void testGoldenVectors()
{
    printf("-- Broadcom BCH-4 against reference output --\n");

    BchCodec bch(13, 4, 0x201b);
    size_t matched = 0;

    for (size_t v = 0; v < ECC_GOLDEN_COUNT; v++)
    {
        const ecc_golden_t &g = ecc_golden[v];
        uint8_t src[522], msg[522], ecc[7];

        memcpy(src, g.data, 512);
        memcpy(src + 512, g.oob, 10);

        msg[0] = static_cast<uint8_t>(src[0] >> 4);
        for (int j = 1; j < 522; j++)
            msg[j] = static_cast<uint8_t>((src[j] >> 4) | (src[j - 1] << 4));

        bch.encode(msg, 522, ecc);

        if (!memcmp(ecc + 1, g.oob + 10, 6))
            matched++;
        else
            printf("       page %u sector %u mismatch\n", g.page, g.sector);
    }

    printf("       %zu / %zu reference sectors reproduced\n", matched,
        ECC_GOLDEN_COUNT);
    expect(matched == ECC_GOLDEN_COUNT,
        "clean-room BCH matches the reference encoder bit for bit");
}

/* ---------------------------------------------------------- Hamming -- */

static void testHamming()
{
    printf("-- Hamming --\n");

    uint8_t blk[256], ref[256], ecc[3];

    srand(7);
    for (int i = 0; i < 256; i++)
        ref[i] = static_cast<uint8_t>(rand());
    memcpy(blk, ref, 256);
    HammingCodec::encode(blk, ecc);

    expect(HammingCodec::correct(blk, ecc) == HammingCodec::HAMMING_CLEAN,
        "clean block");

    bool all = true;
    for (int b = 0; b < 2048 && all; b++)
    {
        memcpy(blk, ref, 256);
        blk[b >> 3] ^= static_cast<uint8_t>(1u << (b & 7));
        if (HammingCodec::correct(blk, ecc) != HammingCodec::HAMMING_CORRECTED ||
            memcmp(blk, ref, 256))
        {
            all = false;
        }
    }
    expect(all, "corrects every one of the 2048 single-bit positions");

    int detected = 0, total = 0;
    for (int trial = 0; trial < 3000; trial++)
    {
        const int b1 = rand() % 2048, b2 = rand() % 2048;
        if (b1 == b2)
            continue;
        memcpy(blk, ref, 256);
        blk[b1 >> 3] ^= static_cast<uint8_t>(1u << (b1 & 7));
        blk[b2 >> 3] ^= static_cast<uint8_t>(1u << (b2 & 7));
        total++;
        if (HammingCodec::correct(blk, ecc) == HammingCodec::HAMMING_UNCORRECTABLE)
            detected++;
    }
    expect(detected == total, "detects every double-bit error");

    uint8_t e2[3];
    memcpy(e2, ecc, 3);
    e2[0] ^= 0x01;
    memcpy(blk, ref, 256);
    expect(HammingCodec::correct(blk, e2) == HammingCodec::HAMMING_ECC_ERROR,
        "a flip in the stored parity is reported, data intact");
}

/* ---------------------------------------------------- Reed-Solomon -- */

static void testReedSolomon()
{
    printf("-- Reed-Solomon --\n");

    RsCodec rs(10, 4);
    expect(rs.isValid(), "RS over GF(2^10), t = 4 builds");
    expect(rs.parityBytes() == 10, "parity occupies 10 bytes");

    uint8_t sect[512];
    srand(99);
    for (int i = 0; i < 512; i++)
        sect[i] = static_cast<uint8_t>(rand());

    std::vector<uint16_t> sym, par;
    RsCodec::bytesToSymbols(sect, 512, 10, sym);
    rs.encode(sym, par);

    std::vector<uint16_t> s2 = sym;
    expect(rs.correct(s2, par) == 0, "clean codeword");

    for (int ne = 1; ne <= 4; ne++)
    {
        bool ok = true;
        for (int trial = 0; trial < 200 && ok; trial++)
        {
            s2 = sym;
            int used[4];
            for (int k = 0; k < ne; k++)
            {
                int p, dup;
                do
                {
                    p = rand() % static_cast<int>(sym.size());
                    dup = 0;
                    for (int q = 0; q < k; q++)
                        if (used[q] == p) dup = 1;
                }
                while (dup);
                used[k] = p;
                uint16_t e = 0;
                while (!e)
                    e = static_cast<uint16_t>(rand() & 0x3FF);
                s2[p] ^= e;
            }
            if (rs.correct(s2, par) != ne || s2 != sym)
                ok = false;
        }
        char msg[80];
        snprintf(msg, sizeof(msg), "corrects %d symbol error(s), 200 trials", ne);
        expect(ok, msg);
    }

    uint8_t back[512];
    RsCodec::symbolsToBytes(sym, 10, back, 512);
    expect(!memcmp(back, sect, 512), "byte and symbol packing round-trips");
}

/* ----------------------------------------------------------- scheme -- */

static void testScheme()
{
    printf("-- layout model --\n");

    bool allValid = true;
    for (int i = 0; i < EccScheme::presetCount(); i++)
    {
        const EccScheme s = EccScheme::preset(i);
        std::string why;

        if (s.algo == ECC_ALGO_NONE)
            continue;

        const int page = 2048;
        const int spare = s.oobSize * (page / s.sectorSize);

        if (!s.validateForPage(page, spare, 0, why))
        {
            printf("       %s: %s\n", EccScheme::presetName(i), why.c_str());
            allValid = false;
        }
    }
    expect(allValid, "every preset keeps parity clear of the bad block marker");

    /* The marker offset comes from the chip database and is not always zero,
     * so the guard has to use it rather than assume byte 0.
     */
    EccScheme s = EccScheme::preset(1);
    s.eccBitOffset = 0;
    s.coverSpare = false;
    std::string why;
    expect(!s.validateForPage(2048, 64, 0, why), "parity over marker byte 0 rejected");
    expect(!s.validateForPage(2048, 64, 5, why), "parity over marker byte 5 rejected");
    expect(s.validateForPage(2048, 64, -1, why), "allowed when the offset is unknown");

    expect(EccScheme::preset(1).validateForPage(2048, 64, 0, why),
        "the Broadcom layout clears the marker");

    /* Correction strength has to be phrased per algorithm: quoting t * m bits
     * for Reed-Solomon invites a comparison with BCH that does not hold.
     */
    const std::string rsText = EccEngine::strengthText(EccScheme::preset(11));
    expect(rsText.find("symbol") != std::string::npos,
        "Reed-Solomon strength is described in symbols, not bits");
}

/* ----------------------------------------------------------- engine -- */

static void testEngine()
{
    printf("-- page engine --\n");

    EccEngine eng;
    std::string why;
    expect(eng.setScheme(EccScheme::preset(1), 2048, 64, 0, why),
        "engine binds to a 2048 + 64 chip");
    expect(eng.sectorsPerPage() == 4, "four steps per page");

    uint8_t page[2112], ref[2112];

    srand(2024);
    for (int i = 0; i < 2048; i++)
        page[i] = static_cast<uint8_t>(rand());
    memset(page + 2048, 0xFF, 64);
    eng.encodePage(page);
    memcpy(ref, page, 2112);

    expect(eng.decodePage(page, false).clean == 4, "freshly encoded page is clean");

    for (int ne = 1; ne <= 4; ne++)
    {
        bool ok = true;
        for (int trial = 0; trial < 40 && ok; trial++)
        {
            memcpy(page, ref, 2112);
            for (int k = 0; k < ne; k++)
            {
                const int b = rand() % (512 * 8);
                page[b >> 3] ^= static_cast<uint8_t>(1u << (7 - (b & 7)));
            }
            const EccPageResult r = eng.decodePage(page, true);
            if (r.uncorrectable || memcmp(page, ref, 2112))
                ok = false;
        }
        char msg[80];
        snprintf(msg, sizeof(msg), "page round-trip with %d error(s) in a step", ne);
        expect(ok, msg);
    }

    memcpy(page, ref, 2112);
    for (int k = 0; k < 9; k++)
    {
        const int b = rand() % (512 * 8);
        page[b >> 3] ^= static_cast<uint8_t>(1u << (7 - (b & 7)));
    }
    expect(eng.decodePage(page, true).uncorrectable >= 1,
        "damage past the correction limit is reported, not hidden");

    /* Erased pages: tolerant when checking, exact when generating. A block
     * that has started to lose charge still reads as erased, but a step with
     * real data in it must get parity even if almost all of it is 0xFF.
     */
    memset(page, 0xFF, 2112);
    memcpy(ref, page, 2112);
    eng.encodePage(page);
    expect(!memcmp(page, ref, 2112), "a blank page is left fully erased");

    EccPageResult r = eng.decodePage(page, false);
    expect(r.erased == 4 && r.uncorrectable == 0, "blank page reported erased");

    memset(page, 0xFF, 2112);
    page[10] &= 0xFE;
    page[300] &= 0xBF;
    page[2050] &= 0xF7;
    r = eng.decodePage(page, false);
    expect(r.erased >= 1 && r.uncorrectable == 0,
        "erased page with a few retention flips is still erased");

    memset(page, 0xFF, 2112);
    for (int i = 0; i < 40; i++)
        page[i] = 0x00;
    expect(eng.decodePage(page, false).erased < 4,
        "a heavily zeroed step counts as programmed");

    memset(page, 0xFF, 2112);
    page[5] &= 0xFE;
    eng.encodePage(page);
    bool spareTouched = false;
    for (int i = 2048; i < 2112; i++)
        if (page[i] != 0xFF) spareTouched = true;
    expect(spareTouched, "a nearly blank but real step still gets parity");
}

/* ----------------------------------------------------------- stream -- */

static EccEngine streamEngine;

static std::vector<uint8_t> buildImage(int pages)
{
    std::vector<uint8_t> img(static_cast<size_t>(pages) * 2112);

    for (int p = 0; p < pages; p++)
    {
        uint8_t *pg = &img[static_cast<size_t>(p) * 2112];

        if (p % 3 == 2)
        {
            memset(pg, 0xFF, 2112);
            continue;
        }

        for (int i = 0; i < 2048; i++)
            pg[i] = static_cast<uint8_t>(rand());
        memset(pg + 2048, 0xFF, 64);
        streamEngine.encodePage(pg);
    }

    return img;
}

static void runStream(const std::vector<uint8_t> &img, size_t chunk,
    EccMode mode, std::vector<uint8_t> &out, EccStats &st)
{
    EccPageStream s;

    s.begin(&streamEngine, 2112, mode);
    out.clear();
    st.reset();

    for (size_t o = 0; o < img.size(); o += chunk)
    {
        const size_t n = img.size() - o < chunk ? img.size() - o : chunk;
        s.push(&img[o], n, out, st);
    }

    s.end(out, st);
}

static void testStream()
{
    printf("-- page stream --\n");

    std::string why;
    streamEngine.setScheme(EccScheme::preset(1), 2048, 64, 0, why);

    srand(4242);
    const std::vector<uint8_t> img = buildImage(9);

    std::vector<uint8_t> ref;
    EccStats rs;
    runStream(img, 2112, ECC_MODE_CHECK, ref, rs);

    expect(ref == img, "checking does not alter the data");
    expect(rs.pages == 9 && rs.stepsTotal == 36, "9 pages, 36 steps");
    expect(rs.stepsErased == 12, "three erased pages give twelve erased steps");
    expect(rs.stepsUncorrectable == 0, "a clean image reports no failures");

    /* The reader hands over USB payloads with no regard for page boundaries,
     * so the result must not depend on how the bytes arrive.
     */
    const size_t chunks[] = { 1, 7, 64, 2111, 2112, 2113, 4096, 65536 };
    bool invariant = true;
    for (size_t i = 0; i < sizeof(chunks) / sizeof(chunks[0]); i++)
    {
        std::vector<uint8_t> o;
        EccStats st;
        runStream(img, chunks[i], ECC_MODE_CHECK, o, st);
        if (o != ref || st.stepsTotal != rs.stepsTotal ||
            st.stepsErased != rs.stepsErased || st.pages != rs.pages)
        {
            printf("       chunk size %zu differs\n", chunks[i]);
            invariant = false;
        }
    }
    expect(invariant, "same output and totals at every chunk size");

    std::vector<uint8_t> trunc(img.begin(), img.begin() + 2112 * 3 + 700);
    std::vector<uint8_t> o;
    EccStats st;
    runStream(trunc, 1000, ECC_MODE_CHECK, o, st);
    expect(o == trunc, "a truncated image passes through byte-exact");
    expect(st.pages == 3, "only whole pages are judged");

    std::vector<uint8_t> dmg = img;
    dmg[10] ^= 0x01;
    dmg[20] ^= 0x02;
    dmg[30] ^= 0x04;
    for (int k = 0; k < 5; k++)
        dmg[2112 + 100 + k * 7] ^= 0x08;

    runStream(dmg, 777, ECC_MODE_CHECK, o, st);
    expect(o == dmg, "check mode leaves the bytes as read");
    expect(st.stepsCorrected == 1 && st.errorsCorrected == 3,
        "a three-bit error is counted but not applied");
    expect(st.stepsUncorrectable == 1, "a five-bit error is reported unrepairable");
    expect(st.haveFailure && st.firstFailPage == 1, "the first failing page is recorded");

    std::vector<uint8_t> o2;
    EccStats st2;
    runStream(dmg, 777, ECC_MODE_CORRECT, o2, st2);
    expect(!memcmp(&o2[0], &img[0], 2112), "correct mode repairs the repairable page");
    expect(memcmp(&o2[2112], &img[2112], 2112) != 0,
        "an unrepairable page is left as it was read");

    /* A block the programmer reported bad cannot be judged by its parity: the
     * marker is written after the parity was computed.
     */
    dmg = img;
    for (int k = 0; k < 9; k++)
        dmg[2112 * 4 + 50 + k * 11] ^= 0x10;
    dmg[2048 + 2112 * 4] = 0x00;

    {
        EccPageStream s;
        EccStats bst;
        std::vector<uint8_t> bo;

        s.begin(&streamEngine, 2112, ECC_MODE_CHECK);
        s.addBadSpan(2112 * 4, 2112);
        for (size_t off = 0; off < dmg.size(); off += 333)
        {
            const size_t n = dmg.size() - off < 333 ? dmg.size() - off : 333;
            s.push(&dmg[off], n, bo, bst);
        }
        s.end(bo, bst);

        expect(bst.stepsInBadBlock == 4, "a bad block's steps are counted apart");
        expect(bst.stepsUncorrectable == 0,
            "damage inside a bad block is not counted as a failure");
    }

    runStream(img, 999, ECC_MODE_RAW, o, st);
    expect(o == img && st.stepsTotal == 0, "raw mode copies and checks nothing");

    EccStats wear;
    wear.worstStep = 3;
    expect(wear.wearWarning(EccScheme::preset(1)),
        "three of four correctable bits raises the wear warning");
    wear.worstStep = 1;
    expect(!wear.wearWarning(EccScheme::preset(1)), "one of four does not");
}

/* ------------------------------------------------------------ probe -- */

struct MemImage
{
    const uint8_t *p;
    uint64_t n;
};

static bool memRead(void *ctx, uint64_t off, uint8_t *b, size_t len)
{
    MemImage *m = static_cast<MemImage *>(ctx);

    if (off + len > m->n)
        return false;

    memcpy(b, m->p + off, len);

    return true;
}

static ImageProbeResult probeOf(const std::vector<uint8_t> &v)
{
    MemImage m = { v.empty() ? NULL : &v[0], v.size() };

    return ImageProbe::probe(memRead, &m, v.size(), 2048, 64, 0);
}

static void testProbe()
{
    printf("-- image probe --\n");

    EccEngine eng;
    std::string why;
    eng.setScheme(EccScheme::preset(1), 2048, 64, 0, why);
    srand(11);

    std::vector<uint8_t> withEcc(40 * 2112);
    for (int p = 0; p < 40; p++)
    {
        uint8_t *pg = &withEcc[static_cast<size_t>(p) * 2112];
        for (int i = 0; i < 2048; i++)
            pg[i] = static_cast<uint8_t>(rand());
        memset(pg + 2048, 0xFF, 64);
        eng.encodePage(pg);
    }

    ImageProbeResult r = probeOf(withEcc);
    expect(r.layout == IMAGE_LAYOUT_PAGE_SPARE, "page plus spare recognised");
    expect(r.matchingPreset == 1, "the scheme in use is identified");
    expect(!r.needsDecision(), "an identified image needs no question");

    std::vector<uint8_t> blank(40 * 2112);
    for (int p = 0; p < 40; p++)
    {
        uint8_t *pg = &blank[static_cast<size_t>(p) * 2112];
        for (int i = 0; i < 2048; i++)
            pg[i] = static_cast<uint8_t>(rand());
        memset(pg + 2048, 0xFF, 64);
    }
    r = probeOf(blank);
    expect(r.layout == IMAGE_LAYOUT_PAGE_SPARE && r.spareBlank,
        "a blank spare area is recognised as blank");
    expect(r.needsDecision(), "a blank spare asks the user what to do");

    std::vector<uint8_t> dataOnly(40 * 2048);
    for (size_t i = 0; i < dataOnly.size(); i++)
        dataOnly[i] = static_cast<uint8_t>(rand());
    r = probeOf(dataOnly);
    expect(r.layout == IMAGE_LAYOUT_DATA_ONLY, "a data only image is recognised");

    /* 33 pages of 2048 is also 32 pages of 2112, so size alone cannot decide;
     * the content has to.
     */
    std::vector<uint8_t> ambiguous(67584);
    for (size_t i = 0; i < ambiguous.size(); i++)
        ambiguous[i] = static_cast<uint8_t>(rand());
    r = probeOf(ambiguous);
    expect(r.layout == IMAGE_LAYOUT_DATA_ONLY,
        "an ambiguous size with dense bytes reads as data only");

    std::vector<uint8_t> foreign = withEcc;
    for (int p = 0; p < 40; p += 8)
        foreign[static_cast<size_t>(p) * 2112 + 2048] = 0x00;
    r = probeOf(foreign);
    expect(r.foreignBadMarkers > 0, "inherited bad block markers are counted");
    expect(r.needsDecision(),
        "inherited markers force a question even with a known scheme");

    std::vector<uint8_t> trunc(2112 * 5 + 300, 0xFF);
    r = probeOf(trunc);
    expect(r.trailingBytes != 0, "a short final page is reported");
}

/* The other half of the simulation.
 *
 * sim/ecc_page.h holds one page encoded by this engine, together with the bit
 * errors the simulation injects into it. The simulation proves that page
 * survives a real write and read through the FSMC driver and the chip model
 * byte for byte, and that the injected bits really do come back flipped. This
 * proves the engine puts them right again. Neither half means much alone.
 */
static void testSimulationPage()
{
    printf("-- shared page with the simulation --\n");

    EccEngine eng;
    std::string why;

    expect(eng.setScheme(EccScheme::preset(1), ECC_PAGE_SIZE, ECC_PAGE_SPARE,
        0, why), "the layout the simulation writes binds here too");

    uint8_t page[ECC_PAGE_TOTAL];
    memcpy(page, ecc_page, ECC_PAGE_TOTAL);

    expect(eng.decodePage(page, false).clean == 4,
        "the generated page is clean as it stands");

    /* Exactly what the simulation does to it. */
    for (size_t i = 0; i < ECC_FLIP_COUNT; i++)
        page[ecc_flips[i].byte] ^= static_cast<uint8_t>(1u << ecc_flips[i].bit);

    expect(memcmp(page, ecc_page, ECC_PAGE_TOTAL) != 0,
        "the injected errors really do change the page");

    const EccPageResult r = eng.decodePage(page, true);

    printf("       %d clean, %d corrected, %d uncorrectable, worst %d\n",
        r.clean, r.corrected, r.uncorrectable, r.worstErrors);

    expect(r.uncorrectable == 0, "the damaged page is repairable");
    expect(r.worstErrors == static_cast<int>(ECC_FLIP_COUNT),
        "all three flips are found in the one step they landed in");
    expect(!memcmp(page, ecc_page, ECC_PAGE_TOTAL),
        "the page is restored byte for byte");
}

int main()
{
    printf("=== NANDO ECC host tests ===\n");

    testGaloisField();
    testBch();
    testGoldenVectors();
    testHamming();
    testReedSolomon();
    testScheme();
    testEngine();
    testStream();
    testProbe();
    testSimulationPage();

    printf("=== %s (%d failure%s) ===\n", failures ? "FAIL" : "PASS", failures,
        failures == 1 ? "" : "s");

    return failures ? 1 : 0;
}
