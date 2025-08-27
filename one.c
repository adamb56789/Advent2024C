#include "one.h"

#include <stdint.h>

#include "file-utils.h"

#define N 1000
#define K 99999
#define OFFSET 10000

#define TABLE_SIZE (1 << 18)
#define MAGIC 3522769201306154299ULL
#define SHIFT 20

static int lookupTable[TABLE_SIZE];
static int tableBuilt = 0;

static uint64_t encodeKey(const char *ptr) {
    return *(int64_t *) ptr & 0x000000FFFFFFFFFF; // <-- evil and dumb
}

// Build the lookup table once
static void buildLookupTable() {
    if (tableBuilt) return;
    for (int i = 10000; i <= 99999; ++i) {
        char buf[6];
        snprintf(buf, sizeof(buf), "%05d", i);
        const uint64_t val = encodeKey(buf);
        const uint32_t index = (uint32_t)((val * MAGIC) >> SHIFT) & (TABLE_SIZE - 1);
        lookupTable[index] = i;
    }
    tableBuilt = 1;
}

/**
 * "They don't know I saved 0.45 microseconds on AoC 2024 Day 1 part 2 by parsing 5-digit numbers with perfect hashing."
 */
static int parse5_fast(const char *ptr) {
    const uint64_t key = encodeKey(ptr);
    const uint32_t index = (uint32_t)((key * MAGIC) >> SHIFT) & (TABLE_SIZE - 1);
    return lookupTable[index];
}

int calculateTotalDistance(const char *ptr, const char *end) {
    buildLookupTable();
    int leftArray[N];
    int rightArray[N];

    for (int i = 0; i < N; ++i) {
        // Example: 82728   61150\n
        leftArray[i] = parse5_fast(ptr);
        rightArray[i] = parse5_fast(ptr+8);
        ptr += 14;
    }

    // Use counting sort since we are in a relatively small range of numbers.
    // Since each step depends on the result of the previous (especially in loop 2), the CPU cannot pipeline it
    // so will have a bunch of slots sitting around idle. We make use of this by computing both arrays simultaneously,
    // since they don't depend on each other.
    int leftSorted[N] = {0};
    int rightSorted[N] = {0};

    int16_t leftCount[K + 1 - OFFSET] = {0};
    int16_t rightCount[K + 1 - OFFSET] = {0};

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

// 0.059117 ms
void one_1() {
    benchmarkFunctionOnFile("../input/1.txt", &calculateTotalDistance, 20000, 2066446);
}

int calculateSimilarityScore(const char *ptr, const char *end) {
    buildLookupTable();
    int leftArray[N];
    int rightArray[N];

    for (int i = 0; i < N; ++i) {
        // Example: 82728   61150\n
        leftArray[i] = parse5_fast(ptr);
        rightArray[i] = parse5_fast(ptr+8);
        ptr += 14;
    }

    // Probably faster in tests to not use the offset to reduce array size, even though it doesn't fit in L1.
    uint8_t rightCount[K] = {0};

    for (int i = 0; i < N; ++i) {
        const int k = rightArray[i];
        rightCount[k]++;
    }

    int similarity = 0;
    for (int i = 0; i < N; ++i) {
        const int left = leftArray[i];
        similarity += left * rightCount[left];
    }
    return similarity;
}

// 0.004845 ms
// Of which about 0.001660 ms is parsing.
void one_2() {
    benchmarkFunctionOnFile("../input/1.txt", &calculateSimilarityScore, 400000, 24931009);
}
