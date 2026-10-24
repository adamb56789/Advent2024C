//
// Created by Adam on 18/10/2026.
//

#include "10.h"

#define N 53
#define SQUARES_PER_HEIGHT 350

#define soa_append(soa, i, x) soa.items[i][soa.length[i]++] = x;

typedef struct {
    u16 items[SQUARES_PER_HEIGHT][4];
    u8 length[SQUARES_PER_HEIGHT];
} AdjacencySoa;

typedef struct {
    u16 items[SQUARES_PER_HEIGHT];
    u16 length;
} PositionList350;


i64 sumTrailheadScores(const char *ptr, const char *end) {
    u16 heightLengths[10] = {};
    u16 heightIndexes[N][N];

    // Parse file into NxN grid of height
    u8 map[N][N];
    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
            const u8 height = *ptr++ - '0';
            map[y][x] = height;
        }
        ptr++;
    }
    for (int i = 0; i < N * N; i++) {
        heightIndexes[0][i] = heightLengths[map[0][i]]++;
    }
    // 0.75

    // Create an adjacency list from each index to the indexes you can reach from it by walking up 1
    // The index is the row-major index of the map
    AdjacencySoa adjacency[9] = {};
    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
            if (x < N - 1) {
                if (map[y][x] + 1 == map[y][x + 1]) {
                    soa_append(adjacency[map[y][x]], heightIndexes[y][x], heightIndexes[y][x+1]);
                }
                if (map[y][x] == map[y][x + 1] + 1) {
                    soa_append(adjacency[map[y][x]-1], heightIndexes[y][x+1], heightIndexes[y][x]);
                }
            }
            if (y < N - 1) {
                if (map[y][x] + 1 == map[y + 1][x]) {
                    soa_append(adjacency[map[y][x]], heightIndexes[y][x], heightIndexes[y+1][x]);
                }
                if (map[y][x] == map[y + 1][x] + 1) {
                    soa_append(adjacency[map[y][x]-1], heightIndexes[y+1][x], heightIndexes[y][x]);
                }
            }
        }
    }

    // 3.57

    i512 a[SQUARES_PER_HEIGHT] = {0};
    i512 b[SQUARES_PER_HEIGHT] = {0};

    i512 *currentBits = a;
    i512 *nextBits = b;

    // At height 0 each position reaches only itself
    for (int i = 0; i < heightLengths[0]; ++i) {
        ((u64 *) &currentBits[i])[i / 64] = 1ULL << (i % 64);
    }

    // 3.68

    // Walk uphill in all possible ways from each position of the current height using the adjacency list.
    // Each new position is at height + 1, and can be reached from all the trailheads that led to where we started from.
    // Add to a set as there could be multiple ways to reach here.
    for (int height = 0; height < 9; ++height) {
        for (int i = 0; i < heightLengths[height]; ++i) {
            for (int j = 0; j < adjacency[height].length[i]; ++j) {
                const u16 next = adjacency[height].items[i][j];

                nextBits[next] = nextBits[next] | currentBits[i];
            }
        }

        i512 *tmp = currentBits;
        currentBits = nextBits;
        nextBits = tmp;

        memset(nextBits, 0, heightLengths[height] * sizeof(i512));
    }
    // 6.54

    // We are left with a set of trailheads that can reach each summit, instead of a set of summits that can be reached from each trailhead.
    // The total number of edges in this graph is the same either way.

    i64 sum = 0;
    i512 counts = {};
    for (int i = 0; i < SQUARES_PER_HEIGHT; ++i) {
        counts = counts + _mm512_popcnt_epi64(currentBits[i]);
    }

    for (int i = 0; i < 8; i++) {
        sum += ((u64 *) &counts)[i];
    }

    // 6.63

    return sum;
}


i64 idk2(const char *ptr, const char *end);
