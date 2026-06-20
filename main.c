#include <stdio.h>
#include <stdlib.h>

#include "eight.h"
#include "five.h"
#include "one.h"
#include "two.h"
#include "three.h"
#include "four.h"
#include "seven.h"
#include "six.h"

#include "file-utils.h"

void runAndBenchmark(
    const char *inputFilePath,
    int (*f)(const char *, const char *),
    int runs,
    int expected
);

int magicSearch();

int main(int argc, const char **argv) {
    if (argc != 2) {
        fatal("Wrong number of arguments");
    }

    char *dot = strrchr(argv[1], '.');
    const double puzzle = strtod(argv[1], &dot);

    if (puzzle == 8.1) runAndBenchmark("../input/8.txt", &countAntinodes, 1000000, 269); // 1.55 us
    else if (puzzle == 8.2) runAndBenchmark("../input/8.txt", &countHarmonicAntinodes, 1000000, 949); // 2.15 us
    else fatal("Unknown puzzle");
}

void runAndBenchmark(
    const char *inputFilePath,
    int (*f)(const char *, const char *),
    const int runs,
    const int expected
) {
    const struct GenericFile file = loadFile(inputFilePath);
    const char *ptr = file.data;

    // Record time of first execution
    const double firstStart = getTime();
    const int result = f(ptr, ptr + file.fileSize);
    const double firstElapsed = getTime() - firstStart;

    if (result != expected) {
        printf("Result incorrect!\nExpected: %d\nActual: %d\n", expected, result);
        return;
    }

    volatile int dummy = 0; // Compiler sometimes optimises the entire thing away without doing something with the result.

    // Run twice more to warm up
    dummy = f(ptr, ptr + file.fileSize);
    dummy = f(ptr, ptr + file.fileSize);

    const double start = getTime();
    for (int i = 0; i < runs; ++i) {
        dummy = f(ptr, ptr + file.fileSize);
    }
    const double elapsed = getTime() - start;
    const double average = elapsed / runs;
    printf("Average: %f ms\nFirst:   %f ms\nElapsed: %f seconds\n", average * 1000, firstElapsed * 1000, elapsed);
    closeFile(file);
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

    srand((unsigned)time(NULL));

    for (int attempts = 0; attempts < 10000000; ++attempts) {
        const uint64_t magic = ((uint64_t)rand() << 32 | rand()) | 1ULL;

        for (int shift = 0; shift <= MAX_SHIFT; ++shift) {
            uint8_t *seen = calloc(TABLE_SIZE, sizeof(uint8_t));
            if (!seen) {
                free(valid_keys);
                return 1;
            }

            int collision = 0;
            for (int i = 0; i < KEY_COUNT; ++i) {
                const uint64_t key = valid_keys[i];
                const uint32_t index = (uint32_t)(((key * magic) >> shift) & (TABLE_SIZE - 1));
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