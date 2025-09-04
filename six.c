//
// Created by Adam on 27/06/2025.
//

#include "six.h"
#include "file-utils.h"

#include <stdint.h>
#include <immintrin.h>
#include <stdatomic.h>

#include "thpool.h"

typedef uint8_t u8;
typedef uint16_t u16;

#define N 130
// Row length includes new line
#define R (N+1)
#define M N*(N+1)
// Highest number of walls in one line
#define W 14
#define EDGE_ARRAY_LENGTH 3000

// This is the answer to part 1 plus a bit
#define TASKS 4500
#define BATCHES 10
#define BATCH_SIZE (TASKS / BATCHES)
#define THREADS 10

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

static int gridNextWallRight(const char *ptr, const int pos, const int x) {
    const int rowStart = pos - x;
    // Start search window at column 2 since the first two columns are unreachable, and it means we can fit everything into 4 blocks
    const char *base = ptr + rowStart + 2;

    const int scanAreaStart = x + 1;
    const int relStart = scanAreaStart - 2; // relative index into 0..127

    const int startBlockNumber = relStart >> 5;
    int blockNumber = startBlockNumber;
    const int startBit = relStart & 31;

    const __m256i needle = _mm256_set1_epi8('#');
    for (; blockNumber < 4; ++blockNumber) {
        const __m256i block = _mm256_loadu_si256((__m256i *) (base + 32 * blockNumber));
        const __m256i cmp = _mm256_cmpeq_epi8(block, needle);
        int mask = _mm256_movemask_epi8(cmp);

        if (blockNumber == startBlockNumber && startBit > 0) {
            const int lowMask = (1 << startBit) - 1;
            mask &= ~lowMask; // ignore bytes left of scanAreaStart in first block
        }

        if (mask) {
            const int lsb = __builtin_ctz(mask);
            // return the cell immediately to the left of the found wall
            return rowStart + (blockNumber << 5) + lsb + 1;
        }
    }

    return -1;
}


static int gridNextWallLeft(const char *ptr, const int pos, const int x) {
    const int rowStart = pos - x;
    const char *base = ptr + rowStart;

    const int scanAreaEnd = x - 1;

    const int startBlockNumber = scanAreaEnd >> 5;
    int blockNumber = startBlockNumber;
    const int endBit = scanAreaEnd & 31;

    const __m256i needle = _mm256_set1_epi8('#');
    for (; blockNumber >= 0; --blockNumber) {
        const __m256i block = _mm256_loadu_si256((__m256i *) (base + 32 * blockNumber));
        const __m256i cmp = _mm256_cmpeq_epi8(block, needle);
        int mask = _mm256_movemask_epi8(cmp);

        if (blockNumber == startBlockNumber && endBit < 31) {
            // mask out bytes to the right of pos-1
            const int keep = (1 << (endBit + 1)) - 1;
            mask &= keep;
        }

        if (mask) {
            const int msb = 31 - __builtin_clz(mask);
            // Return 1 to the right of what we found (where the guard is after hitting the wall)
            return rowStart + (blockNumber << 5) + msb + 1;
        }
    }

    return -1;
}


int countPointsVisitedByGuard(const char *ptr, const char *end) {
    u8 visited[M + 32] = {0};

    const int start = find_next(ptr);
    visited[start] = 0xFF;

    u8 direction = UP; // unsigned makes it do mod 4 with only add+and
    int pos = start;
    int x = pos % R;
    int step = steps[direction];
    while (1) {
        int next = pos + step;

        if (next >= M) break;

        const char c = ptr[next];
        if (c == '\n') {
            break;
        }
        if (c != '#') {
            visited[next] = 0xFF;
            pos = next;
        } else {
            direction = (direction + 1) % 4;
            // For horizontal collision detection we can use SIMD to find the next wall much faster
            if (direction == RIGHT) {
                next = gridNextWallRight(ptr, pos, x);
                if (next == -1) {
                    memset(&visited[pos], 0xFF, N - pos % R);
                    break;
                }
                memset(&visited[pos], 0xFF, 1 + next - pos);
                x += next - pos;
                pos = next;
                direction = DOWN;
            } else if (direction == LEFT) {
                next = gridNextWallLeft(ptr, pos, x);
                if (next == -1) {
                    memset(&visited[pos - pos % R], 0xFF, pos % R + 1);
                    break;
                }
                memset(&visited[next], 0xFF, 1 + pos - next);
                x -= pos - next;
                pos = next;
                direction = UP;
            }
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

static Corner wallsNextCornerUp(const Walls *walls, const Corner corner) {
    const u8 i = getInsertIndex(walls->vertical[corner.x], corner.y);
    if (i == 0) return NO_CORNER;
    return (Corner){corner.x, walls->vertical[corner.x][i - 1] + 1, UP};
}

static Corner wallsNextCornerRight(const Walls *walls, const Corner corner) {
    const u8 i = getInsertIndex(walls->horizontal[corner.y], corner.x);
    if (walls->horizontal[corner.y][i] == 255) return NO_CORNER;
    return (Corner){walls->horizontal[corner.y][i] - 1, corner.y, RIGHT};
}

static Corner wallsNextCornerDown(const Walls *walls, const Corner corner) {
    const u8 i = getInsertIndex(walls->vertical[corner.x], corner.y);
    if (walls->vertical[corner.x][i] == 255) return NO_CORNER;
    return (Corner){corner.x, walls->vertical[corner.x][i] - 1, DOWN};
}

static Corner wallsNextCornerLeft(const Walls *walls, const Corner corner) {
    const u8 i = getInsertIndex(walls->horizontal[corner.y], corner.x);
    if (i == 0) return NO_CORNER;
    return (Corner){walls->horizontal[corner.y][i - 1] + 1, corner.y, LEFT};
}

static u8 checkObstacleHit(const u8 *line, const u8 obstacleCoord, const u8 otherCoord, u8 *cachedInsertIndex) {
    if (*cachedInsertIndex == NONE) *cachedInsertIndex = getInsertIndex(line, obstacleCoord);
    return getInsertIndex(line, otherCoord) == *cachedInsertIndex;
}


static int isLoop(const Graph *graph, const Walls *walls, const int start, const u8 direction, const int obstacle) {
    // When the obstacle is involved we can't use the pregenerated edge graph, so use the walls instead.
    // The starting position is directly facing the obstacle, so we start with a wall navigation.
    // You could skip this step and pretend we've just hit a wall to the left
    // of where we're facing and the loop will take care of it, but it is faster to take care of the special case.
    // After going from the obstacle to a wall, we might need to do a wall navigation again since that wall might not be
    // an existing target in the edge graph. I forgot about that when I first wrote it, and it still works, so whatever.
    Corner c = {start % R, start / R, direction};

    if (direction == UP) c = wallsNextCornerRight(walls, c);
    else if (direction == RIGHT) c = wallsNextCornerDown(walls, c);
    else if (direction == DOWN) c = wallsNextCornerLeft(walls, c);
    else c = wallsNextCornerUp(walls, c);

    if (isCornerOutsideLab(c)) return 0;
    u16 nextEdgeIndex = graph->gridToEdge[c.y][c.x][c.direction];
    if (nextEdgeIndex == EDGE_EXITS_LAB) return 0;
    Edge edge = graph->edges[nextEdgeIndex];

    const u8 obstacleX = obstacle % R;
    const u8 obstacleY = obstacle / R;

    // Horizontal wall navigation used 218 times, vertical 185 times, out of total 3671 calls to isLoop
    // So we don't generate in advance.
    u8 obstacleXInsertIndex = NONE;
    u8 obstacleYInsertIndex = NONE;

    u8 visitedEdges[EDGE_ARRAY_LENGTH] = {0};
    u8 foundLoop = 0;

    while (1) {
        c = edge.corner;
        if (c.x == obstacleX && c.direction == LEFT && c.y > obstacleY) {
            if (checkObstacleHit(walls->vertical[c.x], obstacleY, c.y, &obstacleYInsertIndex)) {
                // Corners on the obstacle aren't in the graph, so we need to do another wall navigation
                c = wallsNextCornerRight(walls, (Corner){c.x, obstacleY + 1, UP});
            } else {
                // It might look like this wallsNextCornerUp unnecessarily recalculates the insert index. But the compiler
                // output is the same either way.
                c = wallsNextCornerUp(walls, c);
            }
            if (isCornerOutsideLab(c)) break;

            nextEdgeIndex = graph->gridToEdge[c.y][c.x][c.direction];
        } else if (c.x == obstacleX && c.direction == RIGHT && c.y < obstacleY) {
            if (checkObstacleHit(walls->vertical[c.x], obstacleY, c.y, &obstacleYInsertIndex)) {
                c = wallsNextCornerLeft(walls, (Corner){c.x, obstacleY - 1, DOWN});
            } else {
                c = wallsNextCornerDown(walls, c);
            }
            if (isCornerOutsideLab(c)) break;

            nextEdgeIndex = graph->gridToEdge[c.y][c.x][c.direction];
        } else if (c.y == obstacleY && c.direction == DOWN && c.x > obstacleX) {
            if (checkObstacleHit(walls->horizontal[c.y], obstacleX, c.x, &obstacleXInsertIndex)) {
                c = wallsNextCornerUp(walls, (Corner){obstacleX + 1, c.y, LEFT});
            } else {
                c = wallsNextCornerLeft(walls, c);
            }
            if (isCornerOutsideLab(c)) break;

            nextEdgeIndex = graph->gridToEdge[c.y][c.x][c.direction];
        } else if (c.y == obstacleY && c.direction == UP && c.x < obstacleX) {
            if (checkObstacleHit(walls->horizontal[c.y], obstacleX, c.x, &obstacleXInsertIndex)) {
                c = wallsNextCornerDown(walls, (Corner){obstacleX - 1, c.y, RIGHT});
            } else {
                c = wallsNextCornerRight(walls, c);
            }
            if (isCornerOutsideLab(c)) break;

            nextEdgeIndex = graph->gridToEdge[c.y][c.x][c.direction];
        } else {
            nextEdgeIndex = edge.nextIndex;
        }

        edge = graph->edges[nextEdgeIndex];
        if (nextEdgeIndex == EDGE_EXITS_LAB) break;

        // Reusing the main walk visited data finds some loops earlier. In my data 1087/1516 hit the main walk.
        if (visitedEdges[nextEdgeIndex]) {
            foundLoop = 1;
            break;
        }
        visitedEdges[nextEdgeIndex] = 1;
    }

    return foundLoop;
}

typedef struct {
    const Graph *graph;
    Walls *walls;
    int starts[BATCH_SIZE];
    uint8_t directions[BATCH_SIZE];
    int obstacles[BATCH_SIZE];
    int count;
} IsLoopTaskArgs;

volatile static int atomicCounter = 0;

void isLoopTask(const void *arg) {
    const IsLoopTaskArgs *task = arg;
    int sum = 0;
    for (int i = 0; i < task->count; ++i) {
        sum += isLoop(task->graph, task->walls, task->starts[i], task->directions[i], task->obstacles[i]);
    }
    atomic_fetch_add(&atomicCounter, sum);
}

threadpool pool;

int countSuccessfulObstructionPositions(const char *ptr, const char *end) {
    Coords wallCoords[1000] = {0};
    int wallCoordsI = 0;

    Walls walls;
    for (int i = 0; i < N; ++i) {
        int countH = 0;
        int countV = 0;
        for (int j = 0; j < N; ++j) {
            if (ptr[i * R + j] == '#') {
                wallCoords[wallCoordsI++] = (Coords){j, i};
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

    int edgeIndex = 1; // 0 is EDGE_EXITS_LAB
    for (int i = 0; i < wallCoordsI; ++i) {
        const u8 x = wallCoords[i].x;
        const u8 y = wallCoords[i].y;

        if (ptr[y * R + x] != '#') continue;
        if (y < N - 2 && 0 < x && x < N - 1) {
            const Corner in = {x, y + 1, UP};
            const Corner out = wallsNextCornerRight(&walls, in);
            if (!isCornerOutsideLab(out)) {
                graph.gridToEdge[in.y][in.x][in.direction] = edgeIndex;
                graph.edges[edgeIndex++] = (Edge){out, EDGE_EXITS_LAB};
            }
        }
        if (x > 1 && 0 < y && y < N - 1) {
            const Corner in = {x - 1, y, RIGHT};
            const Corner out = wallsNextCornerDown(&walls, in);
            if (!isCornerOutsideLab(out)) {
                graph.gridToEdge[in.y][in.x][in.direction] = edgeIndex;
                graph.edges[edgeIndex++] = (Edge){out, EDGE_EXITS_LAB};
            }
        }
        if (y > 1 && 0 < x && x < N - 1) {
            const Corner in = {x, y - 1, DOWN};
            const Corner out = wallsNextCornerLeft(&walls, in);
            if (!isCornerOutsideLab(out)) {
                graph.gridToEdge[in.y][in.x][in.direction] = edgeIndex;
                graph.edges[edgeIndex++] = (Edge){out, EDGE_EXITS_LAB};
            }
        }
        if (x < N - 2 && 0 < y && y < N - 1) {
            const Corner in = {x + 1, y, LEFT};
            const Corner out = wallsNextCornerUp(&walls, in);
            if (!isCornerOutsideLab(out)) {
                graph.gridToEdge[in.y][in.x][in.direction] = edgeIndex;
                graph.edges[edgeIndex++] = (Edge){out, EDGE_EXITS_LAB};
            }
        }
    }

    // We can't add the nextIndex on first pass since we don't know if there is a next edge or where it is
    for (int i = 1; i < edgeIndex; ++i) {
        const Corner p = graph.edges[i].corner;
        graph.edges[i].nextIndex = graph.gridToEdge[p.y][p.x][p.direction];
    }

    u8 visited[M] = {0};

    const int start = find_next(ptr);
    u8 direction = UP;
    visited[start] = 1;

    atomicCounter = 0;

    IsLoopTaskArgs batches[BATCHES] = {0};
    int currentBatch = 0;
    int indexInBatch = 0;

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
                batches[currentBatch].graph = &graph;
                batches[currentBatch].walls = &walls;
                batches[currentBatch].starts[indexInBatch] = pos;
                batches[currentBatch].directions[indexInBatch] = direction;
                batches[currentBatch].obstacles[indexInBatch] = next;
                batches[currentBatch].count++;

                indexInBatch++;
                if (indexInBatch == BATCH_SIZE) {
                    // Start working on the batch as soon as ready
                    thpool_add_work(pool, isLoopTask, &batches[currentBatch]);
                    currentBatch++;
                    indexInBatch = 0;
                }
            }
            visited[next] = 1;
            pos = next;
        } else {
            direction = (direction + 1) % 4;
            step = steps[direction];
        }
    }

    // Start last batch
    thpool_add_work(pool, isLoopTask, &batches[currentBatch]);

    thpool_wait(pool);

    return atomicCounter;
}

// 2.63 us
void six_1() {
    benchmarkFunctionOnFile("../input/6.txt", &countPointsVisitedByGuard, 400000, 4433);
}


// 440 us single-threaded
// 164 us with the thread pool but no main walk visited corner reuse
void six_2() {
    pool = thpool_init(THREADS);
    benchmarkFunctionOnFile("../input/6.txt", &countSuccessfulObstructionPositions, 10000, 1516);
    thpool_destroy(pool);
}
