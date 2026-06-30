#include "four.h"

#define N 140

static i64 countXmasInString(const char *start, const char *end) {
    int count = 0;
    for (const char *p = start; p < end; p += 61) {
        i512 block = _mm512_loadu_si512(p);

        const mask64 mx = _mm512_cmpeq_epi8_mask(block, _mm512_set1_epi8('X'));
        const mask64 mm = _mm512_cmpeq_epi8_mask(block, _mm512_set1_epi8('M'));
        const mask64 ma = _mm512_cmpeq_epi8_mask(block, _mm512_set1_epi8('A'));
        const mask64 ms = _mm512_cmpeq_epi8_mask(block, _mm512_set1_epi8('S'));

        const mask64 forward = mx & (mm >> 1) & (ma >> 2) & (ms >> 3);
        const mask64 reverse = ms & (ma >> 1) & (mm >> 2) & (mx >> 3);

        count += _mm_popcnt_u64(forward | reverse);
    }
    return count;
}

static const u64 SIXTY_ONE_BITS = 0x1FFFFFFFFFFFFFFF;
static const u64 SIXTY_FOUR_BITS = 0xFFFFFFFFFFFFFFFF;
static const u64 EIGHTEEN_BITS = 0x3FFFF;

i64 countXmas2(const char *start, const char *end) {
    const char (*grid)[N + 1] = (char (*)[N + 1]) start;

    int count = 0;
    int h=0, v=0, r=0, l=0;

    for (int j = 0; j < N; j += 61) {
        // mx is current line, mx3 is the line above, etc
        mask64 mx1 = 0, mx2 = 0, mx3 = 0, mx = 0;
        mask64 mm2 = 0, mm3 = 0, mm = 0;
        mask64 ma2 = 0, ma3 = 0, ma = 0;
        mask64 ms1 = 0, ms2 = 0, ms3 = 0, ms = 0;

        const i64 lastRowMask = j > (N - 61) ? EIGHTEEN_BITS : SIXTY_FOUR_BITS;

        for (int i = 0; i < N; ++i) {
            // Shift existing masks up 1
            mx1 = mx2, mx2 = mx3, mx3 = mx;
            mm2 = mm3, mm3 = mm;
            ma2 = ma3, ma3 = ma;
            ms1 = ms2, ms2 = ms3, ms3 = ms;

            i512 block = _mm512_loadu_si512(&grid[i][j]);

            mx = _mm512_cmpeq_epi8_mask(block, _mm512_set1_epi8('X')) & lastRowMask;
            mm = _mm512_cmpeq_epi8_mask(block, _mm512_set1_epi8('M')) & lastRowMask;
            ma = _mm512_cmpeq_epi8_mask(block, _mm512_set1_epi8('A')) & lastRowMask;
            ms = _mm512_cmpeq_epi8_mask(block, _mm512_set1_epi8('S')) & lastRowMask;

            const mask64 forward = mx & (mm >> 1) & (ma >> 2) & (ms >> 3);
            const mask64 reverse = ms & (ma >> 1) & (mm >> 2) & (mx >> 3);
            count += _mm_popcnt_u64(forward | reverse);
            h+=_mm_popcnt_u64(forward | reverse);

            const mask64 downward = mx1 & mm2 & ma3 & ms & SIXTY_ONE_BITS;
            const mask64 upward = ms1 & ma2 & mm3 & mx & SIXTY_ONE_BITS;
            count += _mm_popcnt_u64(downward | upward);
            v += _mm_popcnt_u64(downward | upward);

            const mask64 forwardAndDown = mx1 & (mm2 >> 1) & (ma3 >> 2) & (ms >> 3);
            const mask64 reverseAndUp = ms1 & (ma2 >> 1) & (mm3 >> 2) & (mx >> 3);
            count += _mm_popcnt_u64(forwardAndDown | reverseAndUp);
            r += _mm_popcnt_u64(forwardAndDown | reverseAndUp);

            const mask64 reverseAndDown = (mx1 >> 3) & (mm2 >> 2) & (ma3 >> 1) & ms;
            const mask64 forwardAndUp = (ms1 >> 3) & (ma2 >> 2) & (mm3 >> 1) & mx;
            count += _mm_popcnt_u64(reverseAndDown | forwardAndUp);
            l += _mm_popcnt_u64(reverseAndDown | forwardAndUp);
        }
    }

    return count;
}

i64 countXmas(const char *start, const char *end) {
    countXmas2(start, end);
    const char (*grid)[N + 1] = (char (*)[N + 1]) start;

    int count = 0;

    count += countXmasInString(start, end);

    const int BUFF_SIZE = N * N + (2 * N - 1); // Enough space for '\n' line separators for all diagonals
    char transpose[BUFF_SIZE];

    // Vertical transpose
    int k = 0;
    for (int j = 0; j < N; ++j) {
        for (int i = 0; i < N; ++i) {
            transpose[k++] = grid[i][j];
        }
        transpose[k++] = '\n';
    }
    count += countXmasInString(transpose, &transpose[BUFF_SIZE]);

    // ↘
    k = 0;
    for (int d = 0; d < 2 * N - 1; ++d) {
        int i = (d < N) ? 0 : d - (N - 1);
        int j = (d < N) ? (N - 1 - d) : 0;

        while (i < N && j < N) {
            transpose[k++] = grid[i][j];
            i++;
            j++;
        }

        transpose[k++] = '\n';
    }

    count += countXmasInString(transpose, &transpose[BUFF_SIZE]);

    // ↙
    k = 0;
    for (int d = 0; d < 2 * N - 1; ++d) {
        int i = (d < N) ? 0 : d - (N - 1);
        int j = (d < N) ? d : N - 1;

        while (i < N && j >= 0) {
            transpose[k++] = grid[i][j];
            i++;
            j--;
        }

        transpose[k++] = '\n';
    }

    count += countXmasInString(transpose, &transpose[BUFF_SIZE]);

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
