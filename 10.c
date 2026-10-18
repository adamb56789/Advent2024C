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
    Position items[4];
    u8 length;
} PositionList4;

typedef struct {
    Position items[40];
    u8 length;
} PositionList10;


static void set_add(PositionList10 *next, const Position p) {
    bool found = false;
    for (int k = 0; k < next->length; ++k) {
        if (p.x == next->items[k].x && p.y == next->items[k].y) {
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

    PositionList4 adjacency[N][N] = {};
    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N - 1; ++x) {
            if (map[y][x] + 1 == map[y][x + 1]) {
                append(adjacency[y][x], ((Position){x + 1, y}));
            }
            if (map[y][x] == map[y][x + 1] + 1) {
                append(adjacency[y][x+1], ((Position){x, y}));
            }
        }
    }

    for (int y = 0; y < N - 1; ++y) {
        for (int x = 0; x < N; ++x) {
            if (map[y][x] + 1 == map[y + 1][x]) {
                append(adjacency[y][x], ((Position){x, y+1}));
            }
            if (map[y][x] == map[y + 1][x] + 1) {
                append(adjacency[y+1][x], ((Position){x, y}));
            }
        }
    }

    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
            const PositionList4 list = adjacency[y][x];
            if (list.length > 0) {
                debug("%d,%d: ", x, y);
                for (int k = 0; k < list.length; ++k) {
                    const Position p = list.items[k];
                    debug("%d,%d ", p.x, p.y);
                }
                debug("\n");
            }
        }
    }

    i64 sum = 0;

    PositionList10 currentSet = {};
    PositionList10 nextSet = {};
    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
            if (map[y][x] != 0) continue;
            append(currentSet, ((Position){x, y}));

            debug("%d, %d\n", x, y);

            for (int height = 0; height < 9; ++height) {
                for (int i = 0; i < currentSet.length; ++i) {

                    const Position currentPos = currentSet.items[i];
                    const PositionList4 adjacentPositions = adjacency[currentPos.y][currentPos.x];

                    for (int j = 0; j < adjacentPositions.length; ++j) {
                        set_add(&nextSet, adjacentPositions.items[j]);
                    }
                }
                for (int i = 0; i < nextSet.length; ++i) {
                    debug("(%d, %d) ", nextSet.items[i].x, nextSet.items[i].y);
                }
                debug("\n");

                currentSet = nextSet;
                nextSet.length = 0;
            }
            sum += currentSet.length;
        }
    }
    return sum;
}


i64 idk2(const char *ptr, const char *end);
