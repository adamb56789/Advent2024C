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


static void set_add(PositionList10 *next, const u16 p) {
    bool found = false;
    for (int k = 0; k < next->length; ++k) {
        if (next->items[k] == p) {
            found = true;
            break;
        }
    }
    if (!found) {
        appendp(next, p);
    }
}

i64 sumTrailheadScores(const char *ptr, const char *end) {
    u8 map[N][N];
    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
            map[y][x] = *ptr++ - '0';
        }
        ptr++;
    }

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

    i64 sum = 0;

    PositionList10 currentSet = {};
    PositionList10 nextSet = {};
    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
            if (map[y][x] != 0) continue;
            append(currentSet, IDX(x, y));

            for (int height = 0; height < 9; ++height) {
                for (int i = 0; i < currentSet.length; ++i) {
                    const u16 currentPos = currentSet.items[i];
                    const PositionList4 adjacentPositions = adjacency[currentPos];

                    for (int j = 0; j < adjacentPositions.length; ++j) {
                        set_add(&nextSet, adjacentPositions.items[j]);
                    }
                }

                currentSet = nextSet;
                nextSet.length = 0;
            }
            sum += currentSet.length;
        }
    }
    return sum;
}


i64 idk2(const char *ptr, const char *end);
