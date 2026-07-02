#include "four.h"

#define N 140

static const u64 SIXTY_ONE_BITS = 0x1FFFFFFFFFFFFFFF;
static const u64 EIGHTEEN_BITS = 0x3FFFF;


static i256 shiftAdd256(
    const i256 mx, const i8 sx, const i256 mm, const i8 sm,
    const i256 ma, const i8 sa, const i256 ms, const i8 ss
) {
    // Can't use >> for shifting as it is arithmetic
    return _mm256_srli_epi64(mx, sx) & _mm256_srli_epi64(mm, sm) &
           _mm256_srli_epi64(ma, sa) & _mm256_srli_epi64(ms, ss);
}

static i256 computeLineEqualsMask(const i512 block1, const i512 block2, const i256 block3, const char c,
                                  const u64 endMask) {
    return _mm256_set_epi64x(
        0,
        _mm256_cmpeq_epi8_mask(block3, _mm256_set1_epi8(c)) & endMask,
        _mm512_cmpeq_epi8_mask(block2, _mm512_set1_epi8(c)),
        _mm512_cmpeq_epi8_mask(block1, _mm512_set1_epi8(c))
    );
}

i64 countXmas(const char *start, const char *end) {
    // We maintain XMAS equality masks in a sliding window of 4 rows.
    // On each new row, shift the window, load and compute the masks for the new row,
    // and shift bits around in the masks to find XMAS.

    // mx1 is the top line in the window, mx4 is the bottom and current line
    i256 mx1 = {0}, mx2 = {0}, mx3 = {0}, mx4 = {0};
    i256 mm2 = {0}, mm3 = {0}, mm4 = {0};
    i256 ma2 = {0}, ma3 = {0}, ma4 = {0};
    i256 ms1 = {0}, ms2 = {0}, ms3 = {0}, ms4 = {0};

    i256 pops = {0};
    for (int i = 0; i < N; ++i) {
        mx1 = mx2, mx2 = mx3, mx3 = mx4;
        mm2 = mm3, mm3 = mm4;
        ma2 = ma3, ma3 = ma4;
        ms1 = ms2, ms2 = ms3, ms3 = ms4;

        const i512 block1 = _mm512_loadu_epi8(start + i * (N + 1));
        const i512 block2 = _mm512_loadu_epi8(start + i * (N + 1) + (64 - 3));
        const i256 block3 = _mm256_loadu_epi8(start + i * (N + 1) + 2 * (64 - 3));

        mx4 = computeLineEqualsMask(block1, block2, block3, 'X', EIGHTEEN_BITS);
        mm4 = computeLineEqualsMask(block1, block2, block3, 'M', EIGHTEEN_BITS);
        ma4 = computeLineEqualsMask(block1, block2, block3, 'A', EIGHTEEN_BITS);
        ms4 = computeLineEqualsMask(block1, block2, block3, 'S', EIGHTEEN_BITS);

        // Horizontal XMAS all within the current line. The shifts and ands and together a bit of x with the corresponding bit of m next to it and so on.
        // or forward and reverse before popcount because we know that a string can't be both XMAS and SAMX at the same time and or is faster than popcount
        const i256 forward = shiftAdd256(mx4, 0, mm4, 1, ma4, 2, ms4, 3);
        const i256 reverse = shiftAdd256(mx4, 3, mm4, 2, ma4, 1, ms4, 0);
        pops = pops + _mm256_popcnt_epi64(forward | reverse);

        // Vertical is the same principal as horizontal but using the above rows instead
        // The & SIXTY_ONE_BITS to avoid double-counting the overlapping characters. Only needed in the vertical cause the shift rights pull in zeroes.
        // - clang skips it anyway if you do add it to the other directions
        const i256 downward = mx1 & mm2 & ma3 & ms4;
        const i256 upward = ms1 & ma2 & mm3 & mx4;
        pops = pops + _mm256_popcnt_epi64((downward | upward) & SIXTY_ONE_BITS);

        // Combine the two for ↘
        const i256 forwardAndDown = shiftAdd256(mx1, 0, mm2, 1, ma3, 2, ms4, 3);
        const i256 reverseAndUp = shiftAdd256(mx4, 3, mm3, 2, ma2, 1, ms1, 0);
        pops = pops + _mm256_popcnt_epi64(forwardAndDown | reverseAndUp);

        // ↙
        const i256 reverseAndDown = shiftAdd256(mx1, 3, mm2, 2, ma3, 1, ms4, 0);
        const i256 forwardAndUp = shiftAdd256(mx4, 0, mm3, 1, ma2, 2, ms1, 3);
        pops = pops + _mm256_popcnt_epi64(reverseAndDown | forwardAndUp);
    }

    return _mm256_extract_epi64(pops, 0) + _mm256_extract_epi64(pops, 1) + _mm256_extract_epi64(pops, 2);
}

i64 countCrossMas(const char *start, const char *end) {
    typedef struct {
        i512 rotatedLeft;
        i512 raw;
        i512 rotatedRight;
    } Line;

    // For masking of 1 bit in each end of a column, cause there cannot be a cross centered on the end the rotations
    // mean we have loaded data into those positions.
    static const u64 colMasks[3] = {0x7FFFFFFFFFFFFFFE, 0x7FFFFFFFFFFFFFFE, 0x7FFE};
    Line lines[3] = {0};

    i64 sum = 0;
    for (int col = 0; col < N; col += 62) {
        for (int row = 0; row < N; row++) {
            lines[0] = lines[1];
            lines[1] = lines[2];

            lines[2].raw = _mm512_loadu_epi8(start + row * (N + 1) + col);
            lines[2].rotatedLeft = _mm512_loadu_epi8(start + row * (N + 1) + col + 1);

            // Loading offset by -1 segfaults, but fixing that it is still ~20% slower.
            // I don't know why, llvm-mca isn't revealing anything obvious.
            // Doing both rotations with permute is also slower as I originally suspected.
            // Guessing this is a balance between memory loads and computation.
            const i512 rotRight = _mm512_set_epi8(
                62, 61, 60, 59, 58, 57, 56,
                55, 54, 53, 52, 51, 50, 49, 48,
                47, 46, 45, 44, 43, 42, 41, 40,
                39, 38, 37, 36, 35, 34, 33, 32,
                31, 30, 29, 28, 27, 26, 25, 24,
                23, 22, 21, 20, 19, 18, 17, 16,
                15, 14, 13, 12, 11, 10, 9, 8,
                7, 6, 5, 4, 3, 2, 1, 0, 63
            );
            lines[2].rotatedRight = _mm512_permutexvar_epi8(rotRight, lines[2].raw);

            const mask64 aMask = _mm512_cmpeq_epi8_mask(lines[1].raw, _mm512_set1_epi8('A'));

            const i512 crossHash = (lines[0].rotatedLeft ^ lines[2].rotatedRight) +
                                   (lines[2].rotatedLeft ^ lines[0].rotatedRight);

            const i512 sXorM = _mm512_set1_epi8(('S' ^ 'M') + ('S' ^ 'M'));

            sum += _mm_popcnt_u64(_mm512_mask_cmpeq_epi8_mask(aMask, crossHash, sXorM) & colMasks[col / 62]);
        }
    }
    return sum;
}

i64 countCrossMasBitShifting(const char *start, const char *end) {
    typedef struct {
        i256 mLeft;
        i256 mRight;
        i256 a;
        i256 sLeft;
        i256 sRight;
    } LineMask;

    LineMask lineMasks[3] = {0};

    i256 counts = {0};
    for (int i = 0; i < N; ++i) {
        lineMasks[0] = lineMasks[1];
        lineMasks[1] = lineMasks[2];

        const i512 block1 = _mm512_loadu_epi8(start + i * (N + 1));
        const i512 block2 = _mm512_loadu_epi8(start + i * (N + 1) + (64 - 2));
        const i256 block3 = _mm256_loadu_epi8(start + i * (N + 1) + 2 * (64 - 2));

        const i256 mMask = computeLineEqualsMask(block1, block2, block3, 'M', 0xFFFF);
        lineMasks[2].mLeft = _mm256_slli_epi64(mMask, 1);
        lineMasks[2].mRight = _mm256_srli_epi64(mMask, 1);

        lineMasks[2].a = computeLineEqualsMask(block1, block2, block3, 'A', 0xFFFF);

        const i256 sMask = computeLineEqualsMask(block1, block2, block3, 'S', 0xFFFF);
        lineMasks[2].sLeft = _mm256_slli_epi64(sMask, 1);
        lineMasks[2].sRight = _mm256_srli_epi64(sMask, 1);

        const i256 lineCrossMask = _mm256_popcnt_epi64(
            lineMasks[1].a &
            (lineMasks[0].mLeft & lineMasks[2].sRight | lineMasks[0].sLeft & lineMasks[2].mRight) &
            (lineMasks[0].mRight & lineMasks[2].sLeft | lineMasks[0].sRight & lineMasks[2].mLeft)
        );
        counts = counts + lineCrossMask;
    }

    return _mm256_extract_epi64(counts, 0) + _mm256_extract_epi64(counts, 1) + _mm256_extract_epi64(counts, 2);
}
