//
// Created by Adam on 18/10/2026.
//

#include "10.h"

#define N 53

#define append(xs, x) xs.items[xs.length++] = x;
#define appendp(xs, x)  ((xs)->items[(xs)->length++] = (x))

typedef struct {
    u8 x;
    u8 y;
} Position;

typedef struct {
    u16 items[4];
    u8 length;
} PositionList4;

typedef struct {
    u16 items[10];
    u8 length;
} PositionList10;


static void set_add(PositionList10 *xs, const u16 x) {
    bool found = false;
    for (int k = 0; k < xs->length; ++k) {
        if (xs->items[k] == x) {
            found = true;
            break;
        }
    }
    if (!found) {
        appendp(xs, x);
    }
}

static void set_add_all(PositionList10 *xs, const PositionList10 *ys) {
    for (int i = 0; i < ys->length; ++i) {
        set_add(xs, ys->items[i]);
    }
}

i64 sumTrailheadScores(const char *ptr, const char *end) {
    // Parse file into NxN grid of height
    u8 map[N][N];
    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
            map[y][x] = *ptr++ - '0';
        }
        ptr++;
    }

    // Create an adjacency list from each index to the indexes you can reach from it by walking up 1
    // The index is the row-major index of the map

#define IDX(x, y) ((y) * N + (x))
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

    // A list of trailheads that can reach this position at the current height
    PositionList10 trailheads[N * N] = {};

    // At height 0 each position reaches only itself
    for (int i = 0; i < N * N; ++i) {
        if (map[0][i] != 0) continue;
        append(trailheads[i], i);
    }

    // TODO there is definitely an easier way of doing this

    // Walk uphill in all possible ways from each position of the current height using the adjacency list.
    // Each new position is at height + 1, and can be reached from all the trailheads that led to where we started from.
    // Add to a set as there could be multiple ways to reach here.
    for (int height = 0; height < 9; ++height) {
        for (int i = 0; i < N * N; ++i) {
            if (map[0][i] != height) continue;

            for (int j = 0; j < adjacency[i].length; ++j) {
                const u16 adjacentPosition = adjacency[i].items[j];
                set_add_all(&trailheads[adjacentPosition], trailheads[i].items);
            }

            // Clear the current heights as we re-use the same memory.
            trailheads[i].length = 0;
        }
    }

    // We are left with a list of trailheads that can reach each summit, instead of a list of summits that can be reached from each trailhead.
    // The total number of edges in this graph is the same either way.

    i64 sum = 0;
    for (int i = 0; i < N * N; ++i) {
        sum += trailheads[i].length;
    }

    return sum;
}


i64 idk2(const char *ptr, const char *end);
