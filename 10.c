//
// Created by Adam on 18/10/2026.
//

#include "10.h"

#define N 53

#define append(xs, x) xs.items[xs.length++] = x;
#define append_p(xs, x) xs->items[xs->length++] = x;
#define soa_append(soa, i, x) soa.items[i][soa.length[i]++] = x;
#define soa_append_p(soa, i, x) soa->items[i][soa->length[i]++] = x;
#define IDX(x, y) ((y) * N + (x))

typedef struct {
    u16 items[4];
    u8 length;
} PositionList4;

typedef struct {
    u16 items[350];
    u16 length;
} PositionList350;

typedef struct {
    u16 items[N * N][10];
    u8 length[N * N];
} TrailheadsSoa;


static void soa_set_add(TrailheadsSoa *soa, const u16 i, const u16 x) {
    bool found = false;
    for (int j = 0; j < soa->length[i]; ++j) {
        if (soa->items[i][j] == x) {
            found = true;
            break;
        }
    }
    if (!found) {
        soa_append_p(soa, i, x);
    }
}

static void soa_set_add_all(TrailheadsSoa *soa, const u16 i, const TrailheadsSoa *y_soa, const u16 j) {
    for (int k = 0; k < y_soa->length[j]; ++k) {
        soa_set_add(soa, i, y_soa->items[j][k]);
    }
}

i64 sumTrailheadScores(const char *ptr, const char *end) {
    PositionList350 heightIndexes[10] = {};

    // Parse file into NxN grid of height
    u8 map[N][N];
    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
            const u8 height = *ptr++ - '0';
            map[y][x] = height;
            append(heightIndexes[height], IDX(x, y));
        }
        ptr++;
    }
    // 1.57

    // Create an adjacency list from each index to the indexes you can reach from it by walking up 1
    // The index is the row-major index of the map
    PositionList4 adjacency[N * N] = {};
    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N - 1; ++x) {
            if (map[y][x] + 1 == map[y][x + 1]) {
                append(adjacency[IDX(x, y)], IDX(x+1, y));
            }
            if (map[y][x] == map[y][x + 1] + 1) {
                append(adjacency[IDX(x+1, y)], IDX(x, y));
            }
        }
    }

    for (int y = 0; y < N - 1; ++y) {
        for (int x = 0; x < N; ++x) {
            if (map[y][x] + 1 == map[y + 1][x]) {
                append(adjacency[IDX(x, y)], IDX(x, y+1));
            }
            if (map[y][x] == map[y + 1][x] + 1) {
                append(adjacency[IDX(x, y+1)], IDX(x, y));
            }
        }
    }
    // 3.96

    // A list of trailheads that can reach this position at the current height
    TrailheadsSoa trailhead = {};

    // At height 0 each position reaches only itself
    for (int k = 0; k < heightIndexes[0].length; ++k) {
        const u16 i = heightIndexes[0].items[k];
        soa_append(trailhead, i, i);
    }

    // 4.33

    // TODO there is definitely an easier way of doing this

    // Walk uphill in all possible ways from each position of the current height using the adjacency list.
    // Each new position is at height + 1, and can be reached from all the trailheads that led to where we started from.
    // Add to a set as there could be multiple ways to reach here.
    for (int height = 0; height < 9; ++height) {
        for (int k = 0; k < heightIndexes[height].length; ++k) {
            const u16 i = heightIndexes[height].items[k];
            for (int j = 0; j < adjacency[i].length; ++j) {
                const u16 adjacentPosition = adjacency[i].items[j];
                soa_set_add_all(&trailhead, adjacentPosition, &trailhead, i);
            }

            // Clear the current heights as we re-use the same memory.
            trailhead.length[i] = 0;
        }
    }
    // 10.8

    // We are left with a list of trailheads that can reach each summit, instead of a list of summits that can be reached from each trailhead.
    // The total number of edges in this graph is the same either way.

    i64 sum = 0;
    for (int i = 0; i < N * N; ++i) {
        sum += trailhead.length[i];
    }

    // 10.9

    return sum;
}


i64 idk2(const char *ptr, const char *end);
