#ifndef FOUR_H
#define FOUR_H
#include <string.h>

static int countXmasLine(const char *line) {
    int count = 0;
    const char *linePtr = line;
    // printf("%s\n", line);
    while ((linePtr = strstr(linePtr, "XMAS"))) {
        linePtr += 4;
        count++;
    }

    linePtr = line;
    while ((linePtr = strstr(linePtr, "SAMX"))) {
        linePtr += 4;
        count++;
    }
    return count;
}

static int countXmas(const char *start) {
    const int GRID_SIZE = 140;
    const char (*grid)[GRID_SIZE + 1] = (char (*)[GRID_SIZE + 1]) start;

    int count = 0;

    char line[GRID_SIZE + 1];
    for (int i = 0; i < GRID_SIZE; ++i) {
        memcpy(line, grid[i], GRID_SIZE);
        line[GRID_SIZE] = '\0';
        count += countXmasLine(line);
    }

    for (int j = 0; j < GRID_SIZE; ++j) {
        for (int i = 0; i < GRID_SIZE; ++i) {
            line[i] = grid[i][j];
        }
        line[GRID_SIZE] = '\0';
        count += countXmasLine(line);
    }

    for (int i = 0; i < GRID_SIZE; ++i) {
        int index = (GRID_SIZE + 1) * i;
        for (int j = 0; j < GRID_SIZE - i; ++j, index += GRID_SIZE + 2) {
            line[j] = start[index];
        }
        line[GRID_SIZE - i] = '\0';

        count += countXmasLine(line);
    }

    for (int j = 1; j < GRID_SIZE; ++j) {
        int index = j;
        for (int i = 0; i < GRID_SIZE - 1; ++i, index += GRID_SIZE + 2) {
            line[i] = start[index];
        }
        line[GRID_SIZE - j] = '\0';
        count += countXmasLine(line);
    }

    for (int i = 0; i < GRID_SIZE; ++i) {
        int index = GRID_SIZE - 1 + (GRID_SIZE + 1) * i;
        int lineIndex = 0;
        for (int j = GRID_SIZE - 1; j >= i; --j, index += GRID_SIZE, lineIndex++) {
            line[lineIndex] = start[index];
        }
        line[GRID_SIZE - i] = '\0';

        count += countXmasLine(line);
    }

    for (int j = GRID_SIZE - 2; j >= 0; --j) {
        int index = j;
        for (int i = 0; i < j + 1; ++i, index += GRID_SIZE) {
            line[i] = start[index];
        }
        line[j + 1] = '\0';

        count += countXmasLine(line);
    }

    return count;
}

// 2543
static void four_1() {
    // const struct WindowsFile file = readChars("../input/4.txt");
    // const char *ptr = file.data;
    //
    // printf("%d\n", countXmas(ptr));
    // closeFile(file);
}

static int countCrossMas(const char *start) {
    const int GRID_SIZE = 140;
    const char (*grid)[GRID_SIZE + 1] = (char (*)[GRID_SIZE + 1]) start;

    int count = 0;
    for (int i = 1; i < GRID_SIZE - 1; ++i) {
        for (int j = 1; j < GRID_SIZE - 1; ++j) {
            if (
                grid[i][j] == 'A' &&
                grid[i - 1][j - 1] + grid[i + 1][j + 1] == 'S' + 'M' &&
                grid[i - 1][j + 1] + grid[i + 1][j - 1] == 'S' + 'M'
            ) {
                count++;
            }
        }
    }
    return count;
}

//1930
static void four_2() {
    // const struct WindowsFile file = readChars("../input/4.txt");
    // const char *ptr = file.data;
    //
    // printf("%d\n", countCrossMas(ptr));
    // closeFile(file);
}


#endif //FOUR_H
