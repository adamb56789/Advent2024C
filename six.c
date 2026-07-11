//
// Created by Adam on 27/06/2025.
//

#include "six.h"

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

static int findNext(const char *ptr, const char c) {
    const i512 needle = _mm512_set1_epi8(c);

    // Could leave this as an infinite loop instead, but compiler can do stuff if it is bounded
    for (int i = 0; i < M; i += 64) {
        const mask64 mask = _mm512_cmpeq_epi8_mask(_mm512_loadu_si512(ptr + i), needle);
        if (mask) return i + __builtin_ctzll(mask);
    }
    abort(); // Only use this when will definitely find something
}

static int findPrev(const char *ptr, const char c) {
    const i512 needle = _mm512_set1_epi8(c);

    for (int i = 0; i < M; i += 64) {
        const mask64 mask = _mm512_cmpeq_epi8_mask(_mm512_loadu_si512(ptr - i - 64), needle);
        if (mask) return i + __builtin_clzll(mask) + 1;
    }
    abort();
}

static int countSetBytes(const u8 visited[]) {
    int count = 0;
    for (int i = 0; i < M; i += 64) {
        const i512 block = _mm512_loadu_si512(visited + i);
        const mask64 mask = _mm512_movepi8_mask(block);
        count += _mm_popcnt_u64(mask);
    }
    return count;
}

static int gridNextWallRight(const char *ptr, const int pos, const int x) {
    const int collisionPos = pos + findNext(ptr + pos, '#') - 1;
    const int rowEnd = pos - x + N;

    // If we collided with a wall after the end of the row
    if (collisionPos > rowEnd) return -1;

    return collisionPos;
}

static int gridNextWallLeft(const char *ptr, const int pos, const int x) {
    const int collisionPos = pos - findPrev(ptr + pos, '#') + 1;
    const int rowStart = pos - x;

    // If we collided with a wall before the end of the row
    if (collisionPos < rowStart) return -1;

    return collisionPos;
}

i64 countPointsVisitedByGuard(const char *ptr, const char *end) {
    // Pad with zeroes to align with the SIMD width
    u8 visited[M + (64 - M % 64)] = {0};

    const int start = findNext(ptr, '^');
    visited[start] = 0xFF;

    i8 direction = UP;
    int pos = start;
    int x = pos % R;
    int step = steps[direction];
    while (1) {
        int next = pos + step;

        if ((unsigned) next >= M) break;

        const char c = ptr[next];
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

static bool guardHitsObstacle(const u8 *line, const u8 otherCoord, const u8 obstacleInsertIndex) {
    return getInsertIndex(line, otherCoord) == obstacleInsertIndex;
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

        // Reusing the main walk visited data finds some loops earlier. In my data 1087/1516 hit the main walk.
        if (mainWalkVisited[nextEdgeIndex] || visitedEdges[nextEdgeIndex]) {
            foundLoop = 1;
            break;
        }
        visitedEdges[nextEdgeIndex] = 1;
    }

    return foundLoop;
}

i64 countSuccessfulObstructionPositions(const char *ptr, const char *end) {
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

    // Everything above this point takes about 12 us as of time of writing

    int count = 0;
    u8 visited[M] = {0};
    u8 visitedEdges[EDGE_ARRAY_LENGTH] = {0};

    const int start = findNext(ptr, '^');
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
}

/* Non-exhaustive list of things that didn't make it faster
 * - Using an array of structs instead of struct of arrays to batch arguments (1 us slower).
 * - Calculating the obstacle position in isLoop instead of passing it in
 * - Updating the edge graph at the start of each isLoop and reverting it
 */

/*
 * - Calculate ranges that short circuit when not in obstacle line
 * - Replacing functions per direction with LUTs
 * - re-evaluate precalculating obstacle collision ranges
 *   - try loading obstacle walls once if not
 */
