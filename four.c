#include "four.h"

#define N 140

static const u64 SIXTY_ONE_BITS = 0x1FFFFFFFFFFFFFFF;
static const u64 EIGHTEEN_BITS = 0x3FFFF;

i64 countXmas(const char *start, const char *end) {
    int count = 0;

    for (int j = 0; j < N; j += 61) {
        // We maintain XMAS equality masks in a sliding window of 4 rows.
        // On each new row, shift the window, load and compute the masks for the new row,
        // and shift bits around in the masks to find XMAS.

        // mx1 is the top line in the window, then mx2 and mx3, just mx is the bottom and current line
        mask64 mx1 = 0, mx2 = 0, mx3 = 0, mx = 0;
        mask64 mm2 = 0, mm3 = 0, mm = 0;
        mask64 ma2 = 0, ma3 = 0, ma = 0;
        mask64 ms1 = 0, ms2 = 0, ms3 = 0, ms = 0;

        const bool isLastBlock = j > (N - 61);

        for (int i = 0; i < N; ++i) {
            // Shift existing masks up 1
            mx1 = mx2, mx2 = mx3, mx3 = mx;
            mm2 = mm3, mm3 = mm;
            ma2 = ma3, ma3 = ma;
            ms1 = ms2, ms2 = ms3, ms3 = ms;

            if (!isLastBlock) {
                i512 block = _mm512_loadu_epi8(start + i * (N + 1) + j);

                mx = _mm512_cmpeq_epi8_mask(block, _mm512_set1_epi8('X'));
                mm = _mm512_cmpeq_epi8_mask(block, _mm512_set1_epi8('M'));
                ma = _mm512_cmpeq_epi8_mask(block, _mm512_set1_epi8('A'));
                ms = _mm512_cmpeq_epi8_mask(block, _mm512_set1_epi8('S'));
            } else {
                // The last block has 140-(61*2) = 18 characters we need, a newline, then wraps around to the next line.
                // So mask off the last bits to avoid seeing XMAS in the wrap around.
                // Also narrow to i256/32 byte SIMD to not waste work, saved ~0.05 us
                i256 block = _mm256_loadu_epi8(start + i * (N + 1) + j);

                mx = _mm256_cmpeq_epi8_mask(block, _mm256_set1_epi8('X')) & EIGHTEEN_BITS;
                mm = _mm256_cmpeq_epi8_mask(block, _mm256_set1_epi8('M')) & EIGHTEEN_BITS;
                ma = _mm256_cmpeq_epi8_mask(block, _mm256_set1_epi8('A')) & EIGHTEEN_BITS;
                ms = _mm256_cmpeq_epi8_mask(block, _mm256_set1_epi8('S')) & EIGHTEEN_BITS;
            }

            // Horizontal XMAS all within the current line. The shifts and ands and together a bit of x with the corresponding bit of m next to it and so on.
            // or forward and reverse before popcount because we know that a string can't be both XMAS and SAMX at the same time and or is faster than popcount
            const mask64 forward = mx & (mm >> 1) & (ma >> 2) & (ms >> 3);
            const mask64 reverse = ms & (ma >> 1) & (mm >> 2) & (mx >> 3);
            count += _mm_popcnt_u64(forward | reverse);

            // Vertical is the same principal as horizontal but using the above rows instead
            // The & SIXTY_ONE_BITS to avoid double-counting the overlapping characters. Only needed in the vertical cause the shift rights pull in zeroes.
            // - clang skips it anyway if you do add it to the other directions
            // - it uses bzni for some reason
            const mask64 downward = mx1 & mm2 & ma3 & ms;
            const mask64 upward = ms1 & ma2 & mm3 & mx;
            count += _mm_popcnt_u64((downward | upward) & SIXTY_ONE_BITS);

            // Combine the two for ↘
            const mask64 forwardAndDown = mx1 & (mm2 >> 1) & (ma3 >> 2) & (ms >> 3);
            const mask64 reverseAndUp = ms1 & (ma2 >> 1) & (mm3 >> 2) & (mx >> 3);
            count += _mm_popcnt_u64(forwardAndDown | reverseAndUp);

            // ↙
            const mask64 reverseAndDown = (mx1 >> 3) & (mm2 >> 2) & (ma3 >> 1) & ms;
            const mask64 forwardAndUp = (ms1 >> 3) & (ma2 >> 2) & (mm3 >> 1) & mx;
            count += _mm_popcnt_u64(reverseAndDown | forwardAndUp);
        }
    }
    return count;
}

i64 countCrossMas(const char *start, const char *end) {
    const char (*grid)[N + 1] = (char (*)[N + 1]) start;

    int count = 0;
    for (int i = 1; i < N - 1; ++i) {
        for (int j = 1; j < N - 1; ++j) {
            if (grid[i][j] == 'A') {
                if (
                    (grid[i - 1][j - 1] ^ grid[i + 1][j + 1]) + (grid[i - 1][j + 1] ^ grid[i + 1][j - 1])
                    == ('S' ^ 'M') + ('S' ^ 'M')
                ) {
                    count++;
                }
            }
        }
    }
    return count;
}
