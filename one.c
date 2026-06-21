#include "one.h"

#define N 1000
#define K 99999
#define OFFSET 10000

static void parsePacked(const char *ptr, const int *leftArray, const int *rightArray) {
    __m512i bytes = _mm512_loadu_si512(ptr);

    bytes = _mm512_sub_epi8(bytes, _mm512_set1_epi8('0'));

    // ccccc___ ccccc_cc ccc___cc ccc_cccc c___cccc c_ccccc_ __ccccc_ ________

    // Arrange numbers evenly and put all left numbers in the first 4 then right numbers in the last 4
    bytes = _mm512_maskz_permutexvar_epi8(
        0x1f1f1f1f1f1f1f1f,
        _mm512_set_epi8(
            0, 0, 0, 54, 53, 52, 51, 50,
            0, 0, 0, 40, 39, 38, 37, 36,
            0, 0, 0, 26, 25, 24, 23, 22,
            0, 0, 0, 12, 11, 10, 9, 8,
            0, 0, 0, 46, 45, 44, 43, 42,
            0, 0, 0, 32, 31, 30, 29, 28,
            0, 0, 0, 18, 17, 16, 15, 14,
            0, 0, 0, 4, 3, 2, 1, 0
        ),
        bytes
    );

    // ccccc___ ccccc___ ccccc___ ccccc___ ccccc___ ccccc___ ccccc___ ccccc___

    // For each lane abcde___, calculates a*10+b, c*10+b, e, 0

    bytes = _mm512_maddubs_epi16(bytes, _mm512_set1_epi64(0x00000001010a010a));

    // sss_ sss_ sss_ sss_ sss_ sss_ sss_ sss_

    // a*10+b, c*10+b, e, 0 -> 100*(a*10+b)+c*10+b, e = 1000a + 100b + 10c + d, e
    bytes = _mm512_madd_epi16(bytes, _mm512_set1_epi64(0x0000000100010064));

    // i i i i i i i i

    // Each number is i32, but the largest possible value is 9999 which fits in i16, so convert them down
    __m256i bytes256 = _mm512_cvtepi32_epi16(bytes);

    // ss ss ss ss

    // 1000a + 100b + 10c + d, e -> 10*(1000a + 100b + 10c + d) + e = 10000a + 1000b + 100c + 10d + e
    bytes256 = _mm256_madd_epi16(bytes256, _mm256_set1_epi32(0x1000a));

    // Store first 4 ints in left and last 4 in right
    _mm_storeu_si128((__m128i *) leftArray, _mm256_castsi256_si128(bytes256));
    _mm_storeu_si128((__m128i *) rightArray, _mm256_extracti128_si256(bytes256, 1));
}

i64 calculateTotalDistance(const char *ptr, const char *end) {
    int leftArray[N];
    int rightArray[N];

    for (int i = 0; i < N / 4; ++i) {
        // Example: 82728   61150\n
        parsePacked(ptr, &leftArray[i * 4], &rightArray[i * 4]);
        ptr += 14 * 4;
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

    for (int i = 0; i < N / 4; ++i) {
        // Example: 82728   61150\n
        parsePacked(ptr, &leftArray[i * 4], &rightArray[i * 4]);
        ptr += 14 * 4;
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
