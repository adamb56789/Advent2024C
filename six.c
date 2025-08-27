//
// Created by Adam on 27/06/2025.
//

#include "six.h"
#include "file-utils.h"

#include <stdint.h>
#include <immintrin.h>

#define N 130
#define M N*(N+1)
// Highest number of walls in one line
#define W 14

typedef enum {
    UP = 0,
    RIGHT = 1,
    DOWN = 2,
    LEFT = 3
} Direction;

static const int steps[4] = {-(N + 1), 1, (N + 1), -1};

static int find_next(const char *ptr) {
    const __m256i needle = _mm256_set1_epi8('^'); // Sets 32 '^' in a row

    for (int i = 0; /* lol */ ; i += 32) {
        const __m256i block = _mm256_load_si256((__m256i *) (ptr + i)); // Loads 32 chars from the array
        const __m256i cmp = _mm256_cmpeq_epi8(block, needle); // Compare each byte, set to 0xFF if equal
        const int mask = _mm256_movemask_epi8(cmp); // Squashes 32 bytes to 32 bits based on the MSB
        if (mask) {
            const int offset = __builtin_ctz(mask); // Count trailing zeros
            return i + offset;
        }
    }
}

static int countSetBytes(const uint8_t visited[]) {
    int count = 0;
    for (int i = 0; i < M; i += 32) {
        const __m256i block = _mm256_load_si256((__m256i *) (visited + i)); // Loads 32 chars from the array
        const int mask = _mm256_movemask_epi8(block); // Squashes 32 bytes to 32 bits based on the MSB
        count += _popcnt32(mask);
    }
    return count;
}

int countPointsVisitedByGuard(const char *ptr, const char *end) {
    uint8_t visited[M + 32] = {0};

    const int start = find_next(ptr);
    visited[start] = 0xFF;

    uint8_t direction = UP; // unsigned makes it do mod 4 with only add+and
    int pos = start;
    int step = steps[direction];
    while (1) {
        const int next = pos + step;

        if ((unsigned) next >= M) break;

        const char c = ptr[next];
        if (c == '\n') {
            break;
        }
        if (c != '#') {
            visited[next] = 0xFF;
            pos = next;
        } else {
            direction = (direction + 1) % 4;
            step = steps[direction];
        }
    }

    return countSetBytes(visited);
}

static int findNextWall(const int horizontalWalls[N][W], const int verticalWalls[N][W], const int y, const int x,
                        const uint8_t direction) {
    if (direction == UP) {
        int result = -1;
        for (int i = 1; i < W; i++) {
            if (verticalWalls[x][i] > y) {
                result = verticalWalls[x][i-1];
                break;
            }
        }
        return result;
    }
    if (direction == DOWN) {
        int result = -1;
        for (int i = 0; i < W; i++) {
            if (verticalWalls[x][i] > y) {
                result = verticalWalls[x][i];
                break;
            }
        }
        if (result >= N) return -1;
        return result;
    }
    if (direction == LEFT) {
        int result = -1;
        for (int i = 1; i < W; i++) {
            if (horizontalWalls[x][i] > y) {
                result = horizontalWalls[x][i-1];
                break;
            }
        }
        return result;
    }
    if (direction == RIGHT) {
        int result = -1;
        for (int i = 0; i < W; i++) {
            if (horizontalWalls[x][i] > y) {
                result = horizontalWalls[x][i];
                break;
            }
        }
        if (result >= N) return -1;
        return result;
    }
    exit(-1);
}

static int isLoop(char grid[M], const int start, const int obstacle, uint8_t direction) {
    grid[obstacle] = '#'; // Instead of copying the whole thing, set and reset.

    uint8_t visited[M][4] = {0};
    int pos = start;
    int step = steps[direction];
    while (1) {
        if (visited[pos][direction]) {
            grid[obstacle] = '.';
            return 1;
        }
        visited[pos][direction] = 1;
        const int next = pos + step;

        if ((unsigned) next >= M) break;

        const char c = grid[next];
        if (c == '\n') {
            break;
        }
        if (c != '#') {
            pos = next;
        } else {
            direction = (direction + 1) % 4;
            step = steps[direction];
        }
    }
    grid[obstacle] = '.';
    return 0;
}


int countSuccessfulObstructionPositions(const char *ptr, const char *end) {
    int horizontalWalls[N][W];
    int verticalWalls[N][W];
    for (int i = 0; i < N; ++i) {
        int count = 0;
        for (int j = 0; j < N; ++j) {
            if (ptr[i * (N + 1) + j] == '#') {
                horizontalWalls[i][count++] = j;
            }
        }
        while (count < W) horizontalWalls[i][count++] = 999;
    }
    for (int j = 0; j < N; ++j) {
        int count = 0;
        for (int i = 0; i < N; ++i) {
            if (ptr[i * (N + 1) + j] == '#') {
                verticalWalls[j][count++] = i;
            }
        }
        while (count < W) verticalWalls[j][count++] = 999;
    }

    int count = 0;
    char grid[M];
    memcpy(grid, ptr, M);
    uint8_t visited[M + 32] = {0};

    const int start = find_next(grid);
    uint8_t direction = UP;
    visited[start] = 1;

    int pos = start;
    int step = steps[direction];
    while (1) {
        const int next = pos + step;

        if ((unsigned) next >= M) break;

        const char c = grid[next];
        if (c == '\n') {
            break;
        }
        if (c != '#') {
            if (!visited[next]) {
                count += isLoop(grid, pos, next, direction);
            }
            visited[next] = 1;
            pos = next;
        } else {
            direction = (direction + 1) % 4;
            step = steps[direction];
        }
    }

    return count;
}

// 0.004070 ms
// 4070 ns *4.3 GHz = 17050 cycles
// Input file is 17029 bytes
void six_1() {
    benchmarkFunctionOnFile("../input/6.txt", &countPointsVisitedByGuard, 400000, 4433);
}

// 10.63 ms
void six_2() {
    benchmarkFunctionOnFile("../input/6.txt", &countSuccessfulObstructionPositions, 100, 1516);
}
