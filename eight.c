//
// Created by Adam on 15/09/2025.
//

#include "eight.h"

#include <assert.h>

#include "file-utils.h"

#include <stdint.h>
#include <immintrin.h>


#define N 50
#define R (N+1)

typedef uint8_t u8;
typedef uint16_t u16;

typedef struct {
    u8 x;
    u8 y;
} Point;

typedef struct {
    u8 length;
    Point data[4];
} PointList;

static int countSetBytes(const u8 *arr) {
    int count = 0;
    for (int i = 0; i < N * N; i += 32) {
        const __m256i block = _mm256_load_si256((__m256i *) (arr + i)); // Loads 32 chars from the array
        const int mask = _mm256_movemask_epi8(block); // Squashes 32 bytes to 32 bits based on the MSB
        count += _popcnt32(mask);
    }
    return count;
}

static void tryAddAntinode(u8 antinodes[N][N], const Point a, const Point b) {
    const u8 x = 2 * a.x - b.x;
    const u8 y = 2 * a.y - b.y;
    if (x < N && y < N) {
        antinodes[x][y] = 0xFF;
    }
}

int countAntinodes(const char *ptr, const char *end) {
    // 0-9 + A-Z + a-z = 62, 4 is the most of any you can find
    PointList antennaLists[62] = {0};

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            const char c = ptr[i * R + j];
            if (c != '.') {
                int charIndex;
                if (c <= '9') {
                    charIndex = c - '0';
                } else if (c <= 'Z') {
                    charIndex = c - 'A' + 10;
                } else {
                    charIndex = c - 'a' + 36;
                }
                const Point p = {j, i};
                antennaLists[charIndex].data[antennaLists[charIndex].length] = p;
                antennaLists[charIndex].length++;
            }
        }
    }

    // Add an extra row on the end cause that's easier than updating countSetBytes to handle the trailing bytes
    u8 antinodes[N + 1][N] = {0};

    for (int i = 0; i < 62; ++i) {
        const PointList list = antennaLists[i];
        assert(list.length == 0 || list.length == 3 || list.length == 4);

        for (int j = 1; j < list.length; ++j) {
            const Point a = list.data[j];
            for (int k = j - 1; k >= 0; --k) {
                const Point b = list.data[k];
                tryAddAntinode(antinodes, a, b);
                tryAddAntinode(antinodes, b, a);
            }
        }
    }

    // for (int i = 0; i < N; ++i) {
    //     for (int j = 0; j < N; ++j) {
    //         if (antinodes[i][j]) {
    //             printf("#");
    //         } else {
    //             printf("%c", ptr[i * R + j]);
    //         }
    //     }
    //     printf("\n");
    // }
    return countSetBytes(antinodes);
}

static void tryAddHarmonicAntinode(u8 antinodes[N][N], const Point a, const Point b) {
    u8 x = 2 * a.x - b.x;
    u8 y = 2 * a.y - b.y;
    while (x < N && y < N) {
        antinodes[y][x] = 0xFF;

        x += a.x - b.x;
        y += a.y - b.y;
    }
}

int countHarmonicAntinodes(const char *ptr, const char *end) {
    // 0-9 + A-Z + a-z = 62, 4 is the most of any you can find
    PointList antennaLists[62] = {0};

    // Add an extra row on the end cause that's easier than updating countSetBytes to handle the trailing bytes
    u8 antinodes[N + 1][N] = {0};

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            const char c = ptr[i * R + j];
            if (c != '.') {
                int charIndex;
                if (c <= '9') {
                    charIndex = c - '0';
                } else if (c <= 'Z') {
                    charIndex = c - 'A' + 10;
                } else {
                    charIndex = c - 'a' + 36;
                }
                const Point p = {j, i};
                antennaLists[charIndex].data[antennaLists[charIndex].length] = p;
                antennaLists[charIndex].length++;

                antinodes[p.y][p.x] = 0xFF;
            }
        }
    }

    for (int i = 0; i < 62; ++i) {
        const PointList list = antennaLists[i];
        assert(list.length == 0 || list.length == 3 || list.length == 4);

        for (int j = 1; j < list.length; ++j) {
            const Point a = list.data[j];
            for (int k = j - 1; k >= 0; --k) {
                const Point b = list.data[k];
                tryAddHarmonicAntinode(antinodes, a, b);
                tryAddHarmonicAntinode(antinodes, b, a);
            }
        }
    }

    return countSetBytes(antinodes);
}

// 2.02 us
void eight_1() {
    benchmarkFunctionOnFile("../input/8.txt", &countAntinodes, 1000000, 269);
}

// 2.78 us
void eight_2() {
    benchmarkFunctionOnFile("../input/8.txt", &countHarmonicAntinodes, 1000000, 949);
}
