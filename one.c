#include "one.h"

#define N 1000
#define K 99999
#define OFFSET 10000

static void parse8Rows(const char *ptr, const int *leftArray, const int *rightArray) {
    __m512i lane1 = _mm512_loadu_si512(ptr);
    __m512i lane2 = _mm512_loadu_si512(ptr + 14*4);

    // Convert ascii to number
    lane1 = _mm512_sub_epi8(lane1, _mm512_set1_epi8('0'));
    lane2 = _mm512_sub_epi8(lane2, _mm512_set1_epi8('0'));

    // lllll___rrrrr_lllll___rrrrr_lllll___rrrrr_lllll___rrrrr_________
    // lllll___rrrrr_lllll___rrrrr_lllll___rrrrr_lllll___rrrrr_________

    // For each lane abcde___, calculates a*10+b, c*10+b, e, 0
    const __m512i tenOneTenOneOne = _mm512_set_epi8(
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 1, 10, 1, 10, 0, 0, 0, 1, 1, 10, 1, 10,
        0, 1, 1, 10, 1, 10, 0, 0, 0, 1, 1, 10, 1, 10,
        0, 1, 1, 10, 1, 10, 0, 0, 0, 1, 1, 10, 1, 10,
        0, 1, 1, 10, 1, 10, 0, 0, 0, 1, 1, 10, 1, 10
    );
    lane1 = _mm512_maddubs_epi16(lane1, tenOneTenOneOne);
    lane2 = _mm512_maddubs_epi16(lane2, tenOneTenOneOne);

    // l_l_l___r_r_r_l_l_l___r_r_r_l_l_l___r_r_r_l_l_l___r_r_r_________
    // l_l_l___r_r_r_l_l_l___r_r_r_l_l_l___r_r_r_l_l_l___r_r_r_________

    // Arrange numbers evenly and put all left numbers in the left half and right in the right half
    // Squash the two lanes into one because while they are i16s the largest value is 99, so the higher byte is always 0
    const __m512i permuteIdx = _mm512_set_epi8(
        1, 118, 116, 114, 1, 104, 102, 100, 1, 90, 88, 86, 1, 76, 74, 72,
        1, 54, 52, 50, 1, 40, 38, 36, 1, 26, 24, 22, 1, 12, 10, 8, 1,
        110, 108, 106, 1, 96, 94, 92, 1, 82, 80, 78, 1, 68, 66, 64,
        1, 46, 44, 42, 1, 32, 30, 28, 1, 18, 16, 14, 1, 4, 2, 0
    );
    __m512i leftRightBytes = _mm512_permutex2var_epi8(lane1, permuteIdx, lane2);

    // lll_lll_lll_lll_lll_lll_lll_lll_rrr_rrr_rrr_rrr_rrr_rrr_rrr_rrr_

    // a*10+b, c*10+b, e, 0 -> 100*(a*10+b)+c*10+b, e = 1000a + 100b + 10c + d, e
    leftRightBytes = _mm512_maddubs_epi16(leftRightBytes, _mm512_set1_epi32(0x00010164));

    // 1000a + 100b + 10c + d, e -> 10*(1000a + 100b + 10c + d) + e = 10000a + 1000b + 100c + 10d + e
    leftRightBytes = _mm512_madd_epi16(leftRightBytes, _mm512_set1_epi32(0x0001000a));

    _mm256_storeu_si256((__m256i *) leftArray, _mm512_castsi512_si256(leftRightBytes));
    _mm256_storeu_si256((__m256i *) rightArray, _mm512_extracti64x4_epi64(leftRightBytes, 1));
}

i64 calculateTotalDistance(const char *ptr, const char *end) {
    int leftArray[N];
    int rightArray[N];

    for (int i = 0; i < N / 8; ++i) {
        // Example: 82728   61150\n
        parse8Rows(ptr, &leftArray[i * 8], &rightArray[i * 8]);
        ptr += 14 * 8;
    }

    // Use counting sort since we are in a relatively small range of numbers.
    // Since each step depends on the result of the previous (especially in loop 2), the superscalar CPU cannot process in parallel
    // so will have a bunch of slots sitting around idle. We make use of this by computing both arrays simultaneously,
    // since they don't depend on each other.
    int leftSorted[N] = {0};
    int rightSorted[N] = {0};

    i16 leftCount[K + 1 - OFFSET] = {0};
    i16 rightCount[K + 1 - OFFSET] = {0};

    for (int i = 0; i < N; ++i) {
        const int j = leftArray[i];
        leftCount[j - OFFSET]++;
        const int k = rightArray[i];
        rightCount[k - OFFSET]++;
    }

    for (int i = 1; i < K - OFFSET; ++i) {
        leftCount[i] += leftCount[i - 1];
        rightCount[i] += rightCount[i - 1];
    }

    for (int i = N - 1; i >= 0; --i) {
        const int j = leftArray[i];
        leftCount[j - OFFSET]--;
        leftSorted[leftCount[j - OFFSET]] = leftArray[i];

        const int k = rightArray[i];
        rightCount[k - OFFSET]--;
        rightSorted[rightCount[k - OFFSET]] = rightArray[i];
    }

    int sum = 0;
    for (int i = 0; i < N; ++i) {
        sum += abs(leftSorted[i] - rightSorted[i]);
    }
    return sum;
}

i64 calculateSimilarityScore(const char *ptr, const char *end) {
    int leftArray[N];
    int rightArray[N];

    for (int i = 0; i < N / 8; ++i) {
        // Example: 82728   61150\n
        parse8Rows(ptr, &leftArray[i * 8], &rightArray[i * 8]);
        ptr += 14 * 8;
    }

    u8 rightCount[K - OFFSET] = {0};

    for (int i = 0; i < N; ++i) {
        const int k = rightArray[i];
        rightCount[k - OFFSET]++;
    }

    int similarity = 0;
    for (int i = 0; i < N; ++i) {
        const int left = leftArray[i];
        similarity += left * rightCount[left - OFFSET];
    }
    return similarity;
}
