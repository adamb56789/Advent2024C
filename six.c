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
#define W 14
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
} Coords;

typedef struct {
    u8 x;
    u8 y;
    u8 direction;
} Corner;

const Corner NO_CORNER = {NONE, NONE, NONE};

typedef struct {
    Corner corner;
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

static int isCornerOutsideLab(const Corner corner) {
    return corner.x == NONE;
}

/**
 * Finds the index that n would be inserted into in this sorted array of 16 elements.
 */
static u8 getInsertIndex(const u8 *arr, const u8 n) {
    // add 128 to everything because we don't have unsigned compare on this
    const __m128i bias = _mm_set1_epi8((char) 0x80);
    const __m128i needle = _mm_set1_epi8((char) (n ^ 0x80));
    const __m128i block = _mm_load_si128((__m128i *) arr);
    const __m128i cmp = _mm_cmpgt_epi8(_mm_xor_si128(block, bias), needle);
    const int mask = _mm_movemask_epi8(cmp);
    return __builtin_ctz(mask);
}

static void insertIntoSortedArray(u8 arr[W], const u8 val) {
    const u8 i = getInsertIndex(arr, val);
    memmove(&arr[i + 1], &arr[i], (W - 1 - i) * sizeof(u8));
    arr[i] = val;
}

static void removeFromSortedArray(u8 arr[W], const u8 val) {
    const u8 i = getInsertIndex(arr, val) - 1;
    memmove(&arr[i], &arr[i + 1], (W - 1 - i) * sizeof(u8));
}

// ReSharper disable CppDFANotInitializedField
static Corner wallsNextCornerUp(const Walls *walls, const Corner corner) {
    const u8 i = getInsertIndex(walls->vertical[corner.x], corner.y);
    if (i == 0) return NO_CORNER;
    const Corner result = {corner.x, walls->vertical[corner.x][i - 1] + 1, UP};
    return result;
}

// ReSharper disable CppDFANotInitializedField
static Corner wallsNextCornerRight(const Walls *walls, const Corner corner) {
    const u8 i = getInsertIndex(walls->horizontal[corner.y], corner.x);
    if (walls->horizontal[corner.y][i] == 255) return NO_CORNER;
    const Corner result = {walls->horizontal[corner.y][i] - 1, corner.y, RIGHT};
    return result;
}

// ReSharper disable CppDFANotInitializedField
static Corner wallsNextCornerDown(const Walls *walls, const Corner corner) {
    const u8 i = getInsertIndex(walls->vertical[corner.x], corner.y);
    if (walls->vertical[corner.x][i] == 255) return NO_CORNER;
    const Corner result = {corner.x, walls->vertical[corner.x][i] - 1, DOWN};
    return result;
}

// ReSharper disable CppDFANotInitializedField
static Corner wallsNextCornerLeft(const Walls *walls, const Corner corner) {
    const u8 i = getInsertIndex(walls->horizontal[corner.y], corner.x);
    if (i == 0) return NO_CORNER;
    const Corner result = {walls->horizontal[corner.y][i - 1] + 1, corner.y, LEFT};
    return result;
}

static int isLoop(
    const Graph *graph, Walls *walls, const int start, const u8 direction, const int obstacle, const u8 *mainWalkVisited
) {
    // When the obstacle is involved we can't use the pregenerated edge graph, so use the walls instead.
    // The starting position is directly facing the obstacle, so we start with a wall navigation.
    // You could skip this step and pretend we've just hit a wall to the left
    // of where we're facing and the loop will take care of it, but it is faster to take care of the special case.
    // After going from the obstacle to a wall, we might need to do a wall navigation again since that wall might not be
    // an existing target in the edge graph. I forgot about that when I first wrote it, and it still works, so whatever.
    Corner p = {start % R, start / R, direction};

    if (direction == UP) p = wallsNextCornerRight(walls, p);
    else if (direction == RIGHT) p = wallsNextCornerDown(walls, p);
    else if (direction == DOWN) p = wallsNextCornerLeft(walls, p);
    else p = wallsNextCornerUp(walls, p);

    if (isCornerOutsideLab(p)) return 0;
    u16 nextEdgeIndex = graph->gridToEdge[p.y][p.x][p.direction];
    if (nextEdgeIndex == EDGE_EXITS_LAB) return 0;
    Edge edge = graph->edges[nextEdgeIndex];

    const u8 obstacleX = obstacle % R;
    const u8 obstacleY = obstacle / R;
    insertIntoSortedArray(walls->horizontal[obstacleY], obstacleX);
    insertIntoSortedArray(walls->vertical[obstacleX], obstacleY);

    u8 visitedEdges[EDGE_ARRAY_LENGTH] = {0};
    u8 foundLoop = 0;

    // Edge edge = {p, 0}; // Dummy value
    while (1) {
        p = edge.corner;
        if (p.x == obstacleX && p.direction == LEFT && p.y > obstacleY) {
            p = wallsNextCornerUp(walls, p);
            if (isCornerOutsideLab(p)) break;
            if (p.y == obstacleY + 1) {
                // We've hit the obstacle, so need to do another wall navigation since it isn't in the graph
                p = wallsNextCornerRight(walls, p);
                if (isCornerOutsideLab(p)) break;
            }
            nextEdgeIndex = graph->gridToEdge[p.y][p.x][p.direction];
            if (nextEdgeIndex == EDGE_EXITS_LAB) break;
            edge = graph->edges[nextEdgeIndex];
        } else if (p.x == obstacleX && p.direction == RIGHT && p.y < obstacleY) {
            p = wallsNextCornerDown(walls, p);
            if (isCornerOutsideLab(p)) break;
            if (p.y == obstacleY - 1) {
                p = wallsNextCornerLeft(walls, p);
                if (isCornerOutsideLab(p)) break;
            }
            nextEdgeIndex = graph->gridToEdge[p.y][p.x][p.direction];
            if (nextEdgeIndex == EDGE_EXITS_LAB) break;
            edge = graph->edges[nextEdgeIndex];
        } else if (p.y == obstacleY && p.direction == DOWN && p.x > obstacleX) {
            p = wallsNextCornerLeft(walls, p);
            if (isCornerOutsideLab(p)) break;
            if (p.x == obstacleX + 1) {
                p = wallsNextCornerUp(walls, p);
                if (isCornerOutsideLab(p)) break;
            }
            nextEdgeIndex = graph->gridToEdge[p.y][p.x][p.direction];
            if (nextEdgeIndex == EDGE_EXITS_LAB) break;
            edge = graph->edges[nextEdgeIndex];
        } else if (p.y == obstacleY && p.direction == UP && p.x < obstacleX) {
            p = wallsNextCornerRight(walls, p);
            if (isCornerOutsideLab(p)) break;
            if (p.x == obstacleX - 1) {
                p = wallsNextCornerDown(walls, p);
                if (isCornerOutsideLab(p)) break;
            }
            nextEdgeIndex = graph->gridToEdge[p.y][p.x][p.direction];
            if (nextEdgeIndex == EDGE_EXITS_LAB) break;
            edge = graph->edges[nextEdgeIndex];
        } else {
            nextEdgeIndex = edge.nextIndex;
            edge = graph->edges[nextEdgeIndex];
            if (nextEdgeIndex == EDGE_EXITS_LAB) break;
        }
        // Reusing the main walk visited data finds some loops earlier. In my data 1087/1516 hit the main walk.
        if (mainWalkVisited[nextEdgeIndex] || visitedEdges[nextEdgeIndex]) {
            foundLoop = 1;
            break;
        }
        visitedEdges[nextEdgeIndex] = 1;
    }

    removeFromSortedArray(walls->horizontal[obstacleY], obstacleX);
    removeFromSortedArray(walls->vertical[obstacleX], obstacleY);
    return foundLoop;
}

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
            const Corner in = {x, y + 1, UP};
            const Corner out = wallsNextCornerRight(&walls, in);
            if (!isCornerOutsideLab(out)) {
                graph.gridToEdge[in.y][in.x][in.direction] = edgeIndex;
                const Edge e = {out, EDGE_EXITS_LAB};
                graph.edges[edgeIndex++] = e;
            }
        }
        if (x > 1 && 0 < y && y < N - 1) {
            const Corner in = {x - 1, y, RIGHT};
            const Corner out = wallsNextCornerDown(&walls, in);
            if (!isCornerOutsideLab(out)) {
                graph.gridToEdge[in.y][in.x][in.direction] = edgeIndex;
                const Edge e = {out, EDGE_EXITS_LAB};
                graph.edges[edgeIndex++] = e;
            }
        }
        if (y > 1 && 0 < x && x < N - 1) {
            const Corner in = {x, y - 1, DOWN};
            const Corner out = wallsNextCornerLeft(&walls, in);
            if (!isCornerOutsideLab(out)) {
                graph.gridToEdge[in.y][in.x][in.direction] = edgeIndex;
                const Edge e = {out, EDGE_EXITS_LAB};
                graph.edges[edgeIndex++] = e;
            }
        }
        if (x < N - 2 && 0 < y && y < N - 1) {
            const Corner in = {x + 1, y, LEFT};
            const Corner out = wallsNextCornerUp(&walls, in);
            if (!isCornerOutsideLab(out)) {
                graph.gridToEdge[in.y][in.x][in.direction] = edgeIndex;
                const Edge e = {out, EDGE_EXITS_LAB};
                graph.edges[edgeIndex++] = e;
            }
        }
    }

    // We can't add the nextIndex on first pass since we don't know if there is a next edge or where it is
    for (int i = 1; i < edgeIndex; ++i) {
        const Corner p = graph.edges[i].corner;
        graph.edges[i].nextIndex = graph.gridToEdge[p.y][p.x][p.direction];
    }

    int count = 0;
    u8 visited[M] = {0};
    u8 visitedEdges[EDGE_ARRAY_LENGTH] = {0};

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
                count += isLoop(&graph, &walls, pos, direction, next, visitedEdges);
            }
            visited[next] = 1;
            pos = next;
        } else {
            visitedEdges[graph.gridToEdge[pos / R][pos % R][direction]] = 1;
            direction = (direction + 1) % 4;
            step = steps[direction];
        }
    }

    return count;
    return graph.edges[1000].nextIndex == 999 ? 0 : 1516;
}

// 0.004070 ms
// 4070 ns *4.3 GHz = 17050 cycles
// Input file is 17029 bytes
void six_1() {
    benchmarkFunctionOnFile("../input/6.txt", &countPointsVisitedByGuard, 400000, 4433);
}

// 0.509 ms
void six_2() {
    benchmarkFunctionOnFile("../input/6.txt", &countSuccessfulObstructionPositions, 2000, 1516);
}
