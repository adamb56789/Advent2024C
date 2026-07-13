//
// Created by Adam on 27/06/2025.
//

#include "6_parallel.h"

#include <pthread.h>
#include <stdatomic.h>

#include "dumb_lil_threadpool.h"
#include "platform.h"

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

static const Corner NO_CORNER = (Corner){NONE, NONE, NONE};

typedef struct {
    Corner corner;
    u16 nextIndex;
} Edge;

#define EDGE_EXITS_LAB 0

typedef struct {
    Edge edges[EDGE_ARRAY_LENGTH];
    u16 gridToEdge[N][N][4];
} Graph;

static int findNext(const char *ptr, const char c) {
    const i512 needle = _mm512_set1_epi8(c);

    // Could leave this as an infinite loop instead, but compiler can do stuff if it is bounded
    for (int i = 0; i < M; i += 64) {
        const mask64 mask = _mm512_cmpeq_epi8_mask(_mm512_loadu_si512(ptr + i), needle);
        if (mask) return i + __builtin_ctzll(mask);
    }
    abort(); // Only use this when will definitely find something
}

static int isCornerOutsideLab(const Corner corner) {
    return corner.x == NONE;
}

/**
 * Finds the index that n would be inserted into in this sorted array of 16 elements.
 */
static u8 getInsertIndex(const u8 *arr, const u8 n) {
    const i128 needle = _mm_set1_epi8((char) n);
    const i128 block = _mm_loadu_epi8(arr);
    const mask16 mask = _mm_cmpgt_epu8_mask(block, needle);
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

static int isLoop(
    const Graph *graph, const Walls *walls, const int start, const u8 direction, const int obstacle
) {
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

    // Horizontal wall navigation used 436,654 times, vertical 370,555 times, out of total 7,353,013 calls to isLoop.
    // But now faster to always generate in advance anyway, likely because of reduced branches.
    const u8 obstacleXInsertIndex = getInsertIndex(walls->horizontal[obstacleY], obstacleX);
    const u8 obstacleYInsertIndex = getInsertIndex(walls->vertical[obstacleX], obstacleY);

    const u8 obstacleCollisionXMin = obstacleXInsertIndex == 0
                                         ? 0
                                         : walls->horizontal[obstacleY][obstacleXInsertIndex - 1];
    const u8 obstacleCollisionXMax = walls->horizontal[obstacleY][obstacleXInsertIndex];
    const u8 obstacleCollisionYMin = obstacleYInsertIndex == 0
                                         ? 0
                                         : walls->vertical[obstacleX][obstacleYInsertIndex - 1];
    const u8 obstacleCollisionYMax = walls->vertical[obstacleX][obstacleYInsertIndex];

    u8 visitedEdges[EDGE_ARRAY_LENGTH] = {0};
    u8 foundLoop = 0;

    while (1) {
        c = edge.corner;

        // Short circuit condition in first statement, is true most of the time
        if (c.x != obstacleX && c.y != obstacleY) {
            nextEdgeIndex = edge.nextIndex;
        } else if (c.x == obstacleX && c.direction == LEFT && c.y > obstacleY && c.y < obstacleCollisionYMax) {
            // Corners on the obstacle aren't in the graph, so we need to do another wall navigation
            c = wallsNextCornerRight(walls, (Corner){c.x, obstacleY + 1, UP});
            if (isCornerOutsideLab(c)) break;

            nextEdgeIndex = graph->gridToEdge[c.y][c.x][c.direction];
        } else if (c.x == obstacleX && c.direction == RIGHT && c.y < obstacleY && c.y > obstacleCollisionYMin) {
            c = wallsNextCornerLeft(walls, (Corner){c.x, obstacleY - 1, DOWN});
            if (isCornerOutsideLab(c)) break;

            nextEdgeIndex = graph->gridToEdge[c.y][c.x][c.direction];
        } else if (c.y == obstacleY && c.direction == DOWN && c.x > obstacleX && c.x < obstacleCollisionXMax) {
            c = wallsNextCornerUp(walls, (Corner){obstacleX + 1, c.y, LEFT});
            if (isCornerOutsideLab(c)) break;

            nextEdgeIndex = graph->gridToEdge[c.y][c.x][c.direction];
        } else if (c.y == obstacleY && c.direction == UP && c.x < obstacleX && c.x > obstacleCollisionXMin) {
            c = wallsNextCornerDown(walls, (Corner){obstacleX - 1, c.y, RIGHT});
            if (isCornerOutsideLab(c)) break;

            nextEdgeIndex = graph->gridToEdge[c.y][c.x][c.direction];
        } else {
            nextEdgeIndex = edge.nextIndex;
        }

        edge = graph->edges[nextEdgeIndex];
        if (nextEdgeIndex == EDGE_EXITS_LAB) break;

        if (visitedEdges[nextEdgeIndex]) {
            foundLoop = 1;
            break;
        }
        visitedEdges[nextEdgeIndex] = 1;
    }

    return foundLoop;
}

// This is the answer to part 1
#define TASKS 4433
#define THREADS 10
#define BATCHES (THREADS + 1)
#define BATCH_MAX_SIZE 418

// First batches are smaller since they take longer.
const int batchSizes[BATCHES] = {308, 363, 418, 418, 418, 418, 418, 418, 418, 418, 418};

typedef struct {
    const Graph *graph;
    Walls *walls;
    u16 starts[BATCH_MAX_SIZE];
    u8 directions[BATCH_MAX_SIZE];
    u16 obstacles[BATCH_MAX_SIZE];
    // Visited edges of the main walk at the start of this batch
    int batchSize;
} IsLoopTaskArgs;

typedef struct {
    atomic_int workAvailable;
    void *args;

    void (*function)(void *);
} Task;

static _Atomic int successfulObstructionPositionCount_atomic;

// ReSharper disable once CppParameterMayBeConstPtrOrRef
static void runIsLoopTask(void *genericArgs) {
    const IsLoopTaskArgs *args = genericArgs;
    int sum = 0;
    for (int i = 0; i < args->batchSize; ++i) {
        sum += isLoop(args->graph, args->walls, args->starts[i], args->directions[i], args->obstacles[i]);
    }
    successfulObstructionPositionCount_atomic += sum;
}

i64 countSuccessfulObstructionPositionsParallel(const char *ptr, const char *end) {
    Coords wallCoords[1000] = {0};
    int wallCoordsI = 0;

    Walls walls;
    for (int i = 0; i < N; ++i) {
        int countH = 0;
        for (int j = 0; j < N; ++j) {
            if (ptr[i * R + j] == '#') {
                wallCoords[wallCoordsI++] = (Coords){j, i};
                walls.horizontal[i][countH++] = j;
            }
        }
        walls.horizontal[i][countH] = 255;
    }
    for (int i = 0; i < N; ++i) {
        int countV = 0;
        for (int j = 0; j < N; ++j) {
            if (ptr[j * R + i] == '#') {
                walls.vertical[i][countV++] = j;
            }
        }
        walls.vertical[i][countV] = 255;
    }

    Graph graph;

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

    // Everything above this point takes about 12 us as of time of writing

    u8 visited[M] = {0};

    const int start = findNext(ptr, '^');
    u8 direction = UP;
    visited[start] = 1;

    successfulObstructionPositionCount_atomic = 0;

    IsLoopTaskArgs tasks[BATCHES] = {0};
    for (int i = 0; i < BATCHES; ++i) {
        tasks[i].graph = &graph;
        tasks[i].walls = &walls;
    }

    int taskNumber = 0;
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
                tasks[taskNumber].starts[indexInBatch] = pos;
                tasks[taskNumber].directions[indexInBatch] = direction;
                tasks[taskNumber].obstacles[indexInBatch] = next;
                tasks[taskNumber].batchSize++;

                indexInBatch++;
                if (indexInBatch == batchSizes[taskNumber]) {
                    // Start working on the batch as soon as ready
                    dumb_lil_threadpool_add_work(runIsLoopTask, &tasks[taskNumber], taskNumber);

                    taskNumber++;
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

    // Run the last task in the main thread
    runIsLoopTask(&tasks[taskNumber]);

    dumb_lil_threadpool_wait();

    return successfulObstructionPositionCount_atomic;
}
