#include <time.h>

#include "eight.h"
#include "five.h"
#include "one.h"
#include "two.h"
#include "three.h"
#include "four.h"
#include "seven.h"
#include "six.h"

int magicSearch();

int main(void) {
    eight_2();
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
                printf("magic: %llu\nshift: %d\ntable size: %d\n", magic, shift, TABLE_SIZE);
                free(valid_keys);
                return 0;
            }
        }
    }

    printf("No perfect hash found\n");
    free(valid_keys);
    return 1;
}