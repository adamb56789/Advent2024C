//
// Created by Adam on 27/06/2025.
//

#include "six.h"
#include "file-utils.h"

#include <stdint.h>
#include <immintrin.h>

#define u8 uint8_t
#define u16 uint16_t

#define N 130
// Row length includes new line
#define R (N+1)
#define M N*(N+1)
// Highest number of walls in one line
#define W 15
#define EDGE_ARRAY_LENGTH 3000

typedef enum {
    UP = 0,
    RIGHT = 1,
    DOWN = 2,
    LEFT = 3
} Direction;

static const int steps[4] = {-R, 1, R, -1};

typedef struct {
    u8 horizontal[N][W];
    u8 vertical[N][W];
} Walls;

#define NONE 255

typedef struct {
    u8 x;
    u8 y;
    u8 direction;
} Point;

const Point NO_POINT = {NONE, NONE, NONE};

typedef struct {
    Point point;
    u16 nextIndex;
} Edge;

#define EDGE_EXITS_LAB 0

typedef struct {
    Edge edges[EDGE_ARRAY_LENGTH];
    u16 gridToEdge[N][N][4];
} Graph;

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

static int countSetBytes(const u8 visited[]) {
    int count = 0;
    for (int i = 0; i < M; i += 32) {
        const __m256i block = _mm256_load_si256((__m256i *) (visited + i)); // Loads 32 chars from the array
        const int mask = _mm256_movemask_epi8(block); // Squashes 32 bytes to 32 bits based on the MSB
        count += _popcnt32(mask);
    }
    return count;
}

int countPointsVisitedByGuard(const char *ptr, const char *end) {
    u8 visited[M + 32] = {0};

    const int start = find_next(ptr);
    visited[start] = 0xFF;

    u8 direction = UP; // unsigned makes it do mod 4 with only add+and
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

static int pointOutsideLab(const Point point) {
    return point.x == NONE;
}

static u8 insertIndex(const u8 *arr, u8 n) {
    u8 i = 0;
    for (; i < W; ++i) {
        if (arr[i] == 255 || n <= arr[i]) {
            break;
        }
    }
    return i;
}

// ReSharper disable CppDFANotInitializedField
static Point wallsNextPointUp(const Walls *walls, const Point point) {
    const u8 i = insertIndex(walls->vertical[point.x], point.y);
    if (i == 0) return NO_POINT;
    const Point result = {point.x, walls->vertical[point.x][i - 1] + 1, UP};
    return result;
}

// ReSharper disable CppDFANotInitializedField
static Point wallsNextPointRight(const Walls *walls, const Point point) {
    const u8 i = insertIndex(walls->horizontal[point.y], point.x);
    if (walls->horizontal[point.y][i] == 255) return NO_POINT;
    const Point result = {walls->horizontal[point.y][i] - 1, point.y, RIGHT};
    return result;
}

// ReSharper disable CppDFANotInitializedField
static Point wallsNextPointDown(const Walls *walls, const Point point) {
    const u8 i = insertIndex(walls->vertical[point.x], point.y);
    if (walls->vertical[point.x][i] == 255) return NO_POINT;
    const Point result = {point.x, walls->vertical[point.x][i] - 1, DOWN};
    return result;
}

// ReSharper disable CppDFANotInitializedField
static Point wallsNextPointLeft(const Walls *walls, const Point point) {
    const u8 i = insertIndex(walls->horizontal[point.y], point.x);
    if (i == 0) return NO_POINT;
    const Point result = {walls->horizontal[point.y][i - 1] + 1, point.y, LEFT};
    return result;
}

static void insertIntoSortedArray(u8 arr[W], const u8 val) {
    int i = 0;
    while (i < W && arr[i] < val) ++i;
    memmove(&arr[i + 1], &arr[i], (W - 1 - i) * sizeof(u8));
    arr[i] = val;
}

static void removeFromSortedArray(u8 arr[W], const u8 val) {
    int i = 0;
    while (i < W && arr[i] != val) ++i;
    memmove(&arr[i], &arr[i + 1], (W - 1 - i) * sizeof(u8));
}

static int isLoop(const Graph *graph, const Walls *walls, const int start, const u8 direction, const int obstacle) {
    // When the obstacle is involved we can't use the pregenerated edge graph, so use the walls instead.
    // The starting position is directly facing the obstacle, so we start with a wall navigation.
    // After going from the obstacle to a wall, we need to do a wall navigation again since that wall might not be
    // an existing target in the edge graph. You could skip this step and pretend we've just hit a wall to the left
    // of where we're facing and the loop will take care of it, but it is faster to take care of the special case.
    Point p = {start % R, start / R, direction};
    if (direction == UP) {
        p = wallsNextPointRight(walls, p);
        if (pointOutsideLab(p)) return 0;
        p = wallsNextPointDown(walls, p);
    } else if (direction == RIGHT) {
        p = wallsNextPointDown(walls, p);
        if (pointOutsideLab(p)) return 0;
        p = wallsNextPointLeft(walls, p);
    } else if (direction == DOWN) {
        p = wallsNextPointLeft(walls, p);
        if (pointOutsideLab(p)) return 0;
        p = wallsNextPointUp(walls, p);
    } else {
        p = wallsNextPointUp(walls, p);
        if (pointOutsideLab(p)) return 0;
        p = wallsNextPointRight(walls, p);
    }
    if (pointOutsideLab(p)) return 0;
    u16 nextIndex = graph->gridToEdge[p.y][p.x][p.direction];
    if (nextIndex == EDGE_EXITS_LAB) return 0;
    Edge edge = graph->edges[nextIndex];

    const u8 obstacleX = obstacle % R;
    const u8 obstacleY = obstacle / R;
    insertIntoSortedArray(walls->horizontal[obstacleY], obstacleX);
    insertIntoSortedArray(walls->vertical[obstacleX], obstacleY);

    u8 visited[EDGE_ARRAY_LENGTH] = {0};
    u8 foundLoop = 0;

    // Edge edge = {p, 0}; // Dummy value
    while (1) {
        p = edge.point;
        if (p.x == obstacleX && p.direction == LEFT && p.y > obstacleY) {
            p = wallsNextPointUp(walls, p);
            if (pointOutsideLab(p)) break;
            if (p.y == obstacleY + 1) {
                // We've hit the obstacle, so need to do another wall navigation since it isn't in the graph
                p = wallsNextPointRight(walls, p);
                if (pointOutsideLab(p)) break;
            }
            nextIndex = graph->gridToEdge[p.y][p.x][p.direction];
            if (nextIndex == EDGE_EXITS_LAB) break;
            edge = graph->edges[nextIndex];
        } else if (p.x == obstacleX && p.direction == RIGHT && p.y < obstacleY) {
            p = wallsNextPointDown(walls, p);
            if (pointOutsideLab(p)) break;
            if (p.y == obstacleY - 1) {
                p = wallsNextPointLeft(walls, p);
                if (pointOutsideLab(p)) break;
            }
            nextIndex = graph->gridToEdge[p.y][p.x][p.direction];
            if (nextIndex == EDGE_EXITS_LAB) break;
            edge = graph->edges[nextIndex];
        } else if (p.y == obstacleY && p.direction == DOWN && p.x > obstacleX) {
            p = wallsNextPointLeft(walls, p);
            if (pointOutsideLab(p)) break;
            if (p.x == obstacleX + 1) {
                p = wallsNextPointUp(walls, p);
                if (pointOutsideLab(p)) break;
            }
            nextIndex = graph->gridToEdge[p.y][p.x][p.direction];
            if (nextIndex == EDGE_EXITS_LAB) break;
            edge = graph->edges[nextIndex];
        } else if (p.y == obstacleY && p.direction == UP && p.x < obstacleX) {
            p = wallsNextPointRight(walls, p);
            if (pointOutsideLab(p)) break;
            if (p.x == obstacleX - 1) {
                p = wallsNextPointDown(walls, p);
                if (pointOutsideLab(p)) break;
            }
            nextIndex = graph->gridToEdge[p.y][p.x][p.direction];
            if (nextIndex == EDGE_EXITS_LAB) break;
            edge = graph->edges[nextIndex];
        } else {
            nextIndex = edge.nextIndex;
            edge = graph->edges[nextIndex];
            if (nextIndex == EDGE_EXITS_LAB) break;
        }
        if (visited[nextIndex]) {
            foundLoop = 1;
            break;
        }
        visited[nextIndex] = 1;
    }

    removeFromSortedArray(walls->horizontal[obstacleY], obstacleX);
    removeFromSortedArray(walls->vertical[obstacleX], obstacleY);
    return foundLoop;
}

typedef struct {
    u8 x;
    u8 y;
} Coords;

int countSuccessfulObstructionPositions(const char *ptr, const char *end) {
    Coords wallCoords[1000] = {0};
    int wallCoordsI = 0;

    Walls walls;
    for (int i = 0; i < N; ++i) {
        int countH = 0;
        int countV = 0;
        for (int j = 0; j < N; ++j) {
            if (ptr[i * R + j] == '#') {
                const Coords c = {j, i};
                wallCoords[wallCoordsI++] = c;
                walls.horizontal[i][countH++] = j;
            }

            if (ptr[j * R + i] == '#') {
                walls.vertical[i][countV++] = j;
            }
        }
        walls.horizontal[i][countH] = 255;
        walls.vertical[i][countV] = 255;
    }

    Graph graph = {0};

    int edgeIndex = 1; // 0 is null
    for (int i = 0; i < wallCoordsI; ++i) {
        const u8 x = wallCoords[i].x;
        const u8 y = wallCoords[i].y;

        if (ptr[y * R + x] != '#') continue;
        if (y < N - 2 && 0 < x && x < N - 1) {
            const Point in = {x, y + 1, UP};
            const Point out = wallsNextPointRight(&walls, in);
            if (!pointOutsideLab(out)) {
                graph.gridToEdge[in.y][in.x][in.direction] = edgeIndex;
                const Edge e = {out, EDGE_EXITS_LAB};
                graph.edges[edgeIndex++] = e;
            }
        }
        if (x > 1 && 0 < y && y < N - 1) {
            const Point in = {x - 1, y, RIGHT};
            const Point out = wallsNextPointDown(&walls, in);
            if (!pointOutsideLab(out)) {
                graph.gridToEdge[in.y][in.x][in.direction] = edgeIndex;
                const Edge e = {out, EDGE_EXITS_LAB};
                graph.edges[edgeIndex++] = e;
            }
        }
        if (y > 1 && 0 < x && x < N - 1) {
            const Point in = {x, y - 1, DOWN};
            const Point out = wallsNextPointLeft(&walls, in);
            if (!pointOutsideLab(out)) {
                graph.gridToEdge[in.y][in.x][in.direction] = edgeIndex;
                const Edge e = {out, EDGE_EXITS_LAB};
                graph.edges[edgeIndex++] = e;
            }
        }
        if (x < N - 2 && 0 < y && y < N - 1) {
            const Point in = {x + 1, y, LEFT};
            const Point out = wallsNextPointUp(&walls, in);
            if (!pointOutsideLab(out)) {
                graph.gridToEdge[in.y][in.x][in.direction] = edgeIndex;
                const Edge e = {out, EDGE_EXITS_LAB};
                graph.edges[edgeIndex++] = e;
            }
        }
    }

    // We can't add the nextIndex on first pass since we don't know if there is a next edge or where it is
    for (int i = 1; i < edgeIndex; ++i) {
        const Point p = graph.edges[i].point;
        graph.edges[i].nextIndex = graph.gridToEdge[p.y][p.x][p.direction];
    }

    int count = 0;
    u8 visited[M] = {0};

    const int start = find_next(ptr);
    u8 direction = UP;
    visited[start] = 1;

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
            if (!visited[next]) {
                count += isLoop(&graph, &walls, pos, direction, next);
            }
            visited[next] = 1;
            pos = next;
        } else {
            direction = (direction + 1) % 4;
            step = steps[direction];
        }
    }

    return count;
    // return graph.edges[1000].nextIndex == 999 ? 0 : 1516;
}

// 0.004070 ms
// 4070 ns *4.3 GHz = 17050 cycles
// Input file is 17029 bytes
void six_1() {
    benchmarkFunctionOnFile("../input/6.txt", &countPointsVisitedByGuard, 400000, 4433);
}

// 0.763 ms
void six_2() {
    benchmarkFunctionOnFile("../input/6.txt", &countSuccessfulObstructionPositions, 2000, 1516);
}
