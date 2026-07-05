#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "one.h"
#include "two.h"
#include "three.h"
#include "four.h"
#include "five.h"
#include "six.h"
#include "seven.h"
#include "eight.h"

#include "file-utils.h"

int magicSearch();

typedef struct {
    i64 (*function)(const char *, const char *);

    int runs;
    i64 expected;
    char *fileNameOverride;
} Puzzle;

const Puzzle PUZZLES[][3] = {
    {
        {calculateTotalDistance, 20000, 2066446}, // 24.42 us
        {calculateSimilarityScore, 800000, 24931009}, // 0.948 us
        {questionOneParseOnly, 100000000, 124139}
        // 67 ns parsing - 210 GB/s or average 0.36 cycles and 67 picoseconds (!) per row
    },
    {
        {countDangerousLevels, 200000, 356}, // 5.26 us
        {countDangerousLevelsWithTolerance, 100000, 413} // 9.15 us
    },
    {
        {sumMul, 1000000, 160672468}, // 2.86 us
        {sumEnabledMul, 1000000, 84893551} // 3.92 us
    },
    {
        {countXmas, 2000000, 2543}, // 0.611 us
        {countCrossMas, 5000000, 1930}, // 0.181 us
    },
    {
        {sumCorrectMiddlePages, 1000000, 5452}, // 1.77 us
        {sumCorrectedIncorrectMiddlePages, 500000, 4598} // 3.33 us
    },
    {
        {countPointsVisitedByGuard, 800000, 4433}, // 1.23 us
        {countSuccessfulObstructionPositions, 10000, 1516} // 228 us
    },
    {{}, {}},
    {
        {countAntinodes, 1000000, 269}, // 0.686 us
        {countHarmonicAntinodes, 1000000, 949} // 1.349 us
    }
};

void runAndBenchmark(const char *inputFilePath, Puzzle puzzle);

int testAll();

int main(const int argc, const char **argv) {
    if (argv[1][0] == 'A') {
        return testAll();
    }

    if (argc != 3) {
        fatal("Wrong number of arguments");
    }

    const int day = atoi(argv[1]); // NOLINT(*-err34-c)
    const int part = atoi(argv[2]); // NOLINT(*-err34-c)

    const Puzzle puzzle = PUZZLES[day - 1][part - 1];

    char fileNameBuffer[100];

    sprintf(fileNameBuffer, "../input/%d.txt", day);

    runAndBenchmark(fileNameBuffer, puzzle);
}

volatile i64 sink = 0;

static void consume(i64 x) {
    asm volatile("" :: "r"(x) : "memory");
    sink ^= x;
}

void runAndBenchmark(const char *inputFilePath, const Puzzle puzzle) {
    const struct GenericFile file = loadFile(inputFilePath);
    const char *ptr = file.data;

    // Record time of first execution
    const double firstStart = getTime();
    const i64 result = puzzle.function(ptr, ptr + file.fileSize);
    const double firstElapsed = getTime() - firstStart;

    if (result != puzzle.expected) {
        printf("Result incorrect!\nExpected: %ld\nActual: %ld\n", puzzle.expected, result);
        return;
    }

    // Run twice more to warm up
    consume(puzzle.function(ptr, ptr + file.fileSize));
    consume(puzzle.function(ptr, ptr + file.fileSize));

    const double start = getTime();
    for (int i = 0; i < puzzle.runs; ++i) {
        consume(puzzle.function(ptr, ptr + file.fileSize));
    }
    const double elapsed = getTime() - start;
    double average = elapsed / puzzle.runs;

    char *averageUnits;
    if (average > 1.0) {
        average *= 1000;
        averageUnits = "ms";
    } else {
        average *= 1000000;
        averageUnits = "us";
    }

    printf("Average: %f %s\nFirst:   %f ms\nElapsed: %f seconds\n", average, averageUnits, firstElapsed * 1000,
           elapsed);
    closeFile(file);
}

int testAll() {
    int failures = 0;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 2; j++) {
            const Puzzle puzzle = PUZZLES[i][j];
            if (!puzzle.function) continue;

            char fileNameBuffer[100];

            sprintf(fileNameBuffer, "../input/%d.txt", i + 1);

            const struct GenericFile file = loadFile(fileNameBuffer);
            const char *ptr = file.data;

            const i64 result = puzzle.function(ptr, ptr + file.fileSize);

            if (result != puzzle.expected) {
                printf("Result incorrect for %d.%d\nExpected: %ld\nActual: %ld\n", i + 1, j + 1, puzzle.expected,
                       result);
                failures++;
            } else {
                printf("%d.%d passed\n", i + 1, j + 1);
            }
            closeFile(file);
        }
    }
    if (!failures) {
        printf("\033[1;32mAll tests passed\033[0m\n");
    } else {
        printf("\033[1;31m%d tests failed\033[0m\n", failures);
    }
    return failures;
}

#define TABLE_BITS 10
#define TABLE_SIZE (1 << TABLE_BITS)
#define MAX_SHIFT (64 - TABLE_BITS)
#define KEY_COUNT 1251

static uint32_t encodeKey(const char *ptr) {
    return *(int32_t *) ptr & 0x00FFFFFF;
}

int magicSearch() {
    uint64_t *valid_keys = malloc(sizeof(uint64_t) * KEY_COUNT);
    if (!valid_keys) return 1;

    int k = 0;
    for (int i = 100; i <= 999; ++i) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%03d", i);
        valid_keys[k++] = encodeKey(buf);
    }

    for (int i = 10; i <= 99; ++i) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02d ", i);
        valid_keys[k++] = encodeKey(buf);

        snprintf(buf, sizeof(buf), "%02d\n", i);
        valid_keys[k++] = encodeKey(buf);
    }

    for (int i = 1; i <= 9; ++i) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%01d\n\n", i);
        valid_keys[k++] = encodeKey(buf);
        for (int j = 1; j <= 9; ++j) {
            snprintf(buf, sizeof(buf), "%01d %01d", i, j);
            valid_keys[k++] = encodeKey(buf);

            snprintf(buf, sizeof(buf), "%01d\n%01d", i, j);
            valid_keys[k++] = encodeKey(buf);
        }
    }

    srand((unsigned) time(NULL));

    for (int attempts = 0; attempts < 10000000; ++attempts) {
        const uint64_t magic = ((uint64_t) rand() << 32 | rand()) | 1ULL;

        for (int shift = 0; shift <= MAX_SHIFT; ++shift) {
            uint8_t *seen = calloc(TABLE_SIZE, sizeof(uint8_t));
            if (!seen) {
                free(valid_keys);
                return 1;
            }

            int collision = 0;
            for (int i = 0; i < KEY_COUNT; ++i) {
                const uint64_t key = valid_keys[i];
                const uint32_t index = (uint32_t) (((key * magic) >> shift) & (TABLE_SIZE - 1));
                if (seen[index]) {
                    collision = 1;
                    break;
                }
                seen[index] = 1;
            }
            free(seen);
            if (!collision) {
                printf("magic: %lu\nshift: %d\ntable size: %d\n", magic, shift, TABLE_SIZE);
                free(valid_keys);
                return 0;
            }
        }
    }

    printf("No perfect hash found\n");
    free(valid_keys);
    return 1;
}
