//
// Created by Adam on 18/10/2026.
//

#include "10.h"

#define N 53

#define soa_append(soa, i, x) soa.items[i][soa.length[i]++] = x;

typedef struct {
    u16 items[350][4];
    u8 length[350];
} AdjacencySoa;

typedef struct {
    u16 items[350];
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
    // 1.30

    // Create an adjacency list from each index to the indexes you can reach from it by walking up 1
    // The index is the row-major index of the map
    AdjacencySoa adjacency[9] = {};
    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N - 1; ++x) {
            if (map[y][x] + 1 == map[y][x + 1]) {
                soa_append(adjacency[map[y][x]], heightIndexes[y][x], heightIndexes[y][x+1]);
            }
            if (map[y][x] == map[y][x + 1] + 1) {
                soa_append(adjacency[map[y][x]-1], heightIndexes[y][x+1], heightIndexes[y][x]);
            }
        }
    }

    for (int y = 0; y < N - 1; ++y) {
        for (int x = 0; x < N; ++x) {
            if (map[y][x] + 1 == map[y + 1][x]) {
                soa_append(adjacency[map[y][x]], heightIndexes[y][x], heightIndexes[y+1][x]);
            }
            if (map[y][x] == map[y + 1][x] + 1) {
                soa_append(adjacency[map[y][x]-1], heightIndexes[y+1][x], heightIndexes[y][x]);
            }
        }
    }
    // 3.51

    // A list of trailheads that can reach this position at the current height
    i512 bits[10][350] = {};

    // At height 0 each position reaches only itself
    for (int i = 0; i < heightLengths[0]; ++i) {
        ((u64 *) &bits[0][i])[i / 64] |= 1UL << (i % 64);
    }

    // 4.36

    // TODO there is definitely an easier way of doing this

    // Walk uphill in all possible ways from each position of the current height using the adjacency list.
    // Each new position is at height + 1, and can be reached from all the trailheads that led to where we started from.
    // Add to a set as there could be multiple ways to reach here.
    for (int height = 0; height < 9; ++height) {
        for (int i = 0; i < heightLengths[height]; ++i) {
            for (int j = 0; j < adjacency[height].length[i]; ++j) {
                const u16 adjacentPosition = adjacency[height].items[i][j];

                bits[height + 1][adjacentPosition] = bits[height + 1][adjacentPosition] | bits[height][i];
            }
        }
    }
    // 7.64

    // We are left with a list of trailheads that can reach each summit, instead of a list of summits that can be reached from each trailhead.
    // The total number of edges in this graph is the same either way.

    i64 sum = 0;
    i512 counts = {};
    for (int i = 0; i < 350; ++i) {
        counts = counts + _mm512_popcnt_epi64(bits[9][i]);
    }

    for (int i = 0; i < 8; i++) {
        sum += ((u64 *) &counts)[i];
    }

    // 7.87

    return sum;
}


i64 idk2(const char *ptr, const char *end);
