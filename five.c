//
// Created by Adam on 20/06/2025.
//

#include "five.h"

#include <stdbool.h>
#include <stdint.h>

#include "file-utils.h"

/*
 * The input nodes form a directed graph, with all nodes connected to all other nodes in one direction only.
 * Each node has exactly 24 outgoing and 24 incoming edges, so it is not a DAG and there has to be cycles,
 * meaning it is only partially ordered. But, if there are no cycles of length less than 24, the subgraph consisting of
 * the nodes of a 23-long input (the max) will be a DAG with a valid ordering, and thus the edges can be used as a
 * comparator in a normal pairwise ordering check or sorting function. Making this assumption gets to the correct
 * answer, so while it could just be that these particular inputs happen to work with it, I suspect that the author
 * constructed a graph with these properties in order to guarantee that all inputs would have valid orderings.
 */

// Using int saves ~200 ns
int boolRules[100][100] = {};

static int8_t PARSE_TABLE[14648];
static bool tableBuilt = false;

static void initParseTable() {
    if (tableBuilt) return;
    for (char i = '0'; i <= '9'; ++i) {
        for (char j = '0'; j <= '9'; ++j) {
            const char s[2] = {i, j};
            PARSE_TABLE[((int16_t *) s)[0]] = (i - '0') * 10 + j - '0';
        }
    }
    tableBuilt = true;
}

static int twoDigitStringParse(const char *ptr) {
    return PARSE_TABLE[*(int16_t *) ptr];
}

static int checkRules(int a, int b) {
    return boolRules[a][b];
}

int sumCorrectMiddlePages(const char *ptr, const char *end) {
    initParseTable();
    while (*ptr != '\n') {
        const int left = twoDigitStringParse(ptr);
        const int right = twoDigitStringParse(ptr + 3);
        ptr += 6;
        boolRules[left][right] = true;
    }
    ptr++;
    int update[23];
    int sum = 0;
    while (ptr < end) {
        // Always at least 5 numbers which we inline
        update[0] = twoDigitStringParse(ptr);
        update[1] = twoDigitStringParse(ptr + 3);
        update[2] = twoDigitStringParse(ptr + 6);
        update[3] = twoDigitStringParse(ptr + 9);
        update[4] = twoDigitStringParse(ptr + 12);
        ptr += 14;
        int updateLength = 5;
        while (*ptr == ',') {
            ptr++;
            // Always an odd number of numbers so reduce jumps by doing two at a time
            update[updateLength] = twoDigitStringParse(ptr);
            update[updateLength + 1] = twoDigitStringParse(ptr + 3);
            updateLength += 2;
            ptr += 5;
        }
        ptr++;

        if (checkRules(update[0], update[1]) &&
            checkRules(update[1], update[2]) &&
            checkRules(update[2], update[3]) &&
            checkRules(update[3], update[4])) {
            bool updateIsValid = true;
            for (int i = 4; i < updateLength - 1; i += 2) {
                if (!checkRules(update[i], update[i + 1]) || !checkRules(update[i + 1], update[i + 2])) {
                    updateIsValid = false;
                    break;
                }
            }
            if (updateIsValid) {
                sum += update[updateLength / 2];
            }
        }
    }
    return sum;
}

// 0.002254 ms
// Could probably speed up by not copying to updates and skipping parsing if violation already found.
// I tried, and it was both slower and didn't work, probably just a skill issue tbh.
// But 0.00240/1000000 = 2400 ns * 4.3GHZ = 10320 cycles / 15949 bytes input = 0.65 cycles per byte which is crazy so whatever.
void five_1() {
    benchmarkFunctionOnFile("../input/5.txt", &sumCorrectMiddlePages, 1000000, 5452);
}

void swap(int *a, int *b) {
    const int _a = *a;
    const int _b = *b;
    *a = _b;
    *b = _a;
}

int partition(int *list, const int left, const int right, const int pivotIndex) {
    const int pivotValue = list[pivotIndex];
    swap(&list[pivotIndex], &list[right]);
    int storeIndex = left;
    for (int i = left; i < right; ++i) {
        if (boolRules[pivotValue][list[i]]) {
            swap(&list[storeIndex], &list[i]);
            storeIndex++;
        }
    }
    swap(&list[right], &list[storeIndex]);
    return storeIndex;
}

int quickSelect(int *list, int left, int right, const int k) {
    while (1) {
        if (left == right) return list[left];
        int pivotIndex = right; // right performs the fastest for some reason
        pivotIndex = partition(list, left, right, pivotIndex);
        if (k == pivotIndex) return list[k];
        if (k < pivotIndex) right = pivotIndex - 1;
        else left = pivotIndex + 1;
    }
}

int sumCorrectedIncorrectMiddlePages(const char *ptr, const char *end) {
    initParseTable();
    while (*ptr != '\n') {
        const int left = twoDigitStringParse(ptr);
        const int right = twoDigitStringParse(ptr + 3);
        ptr += 6;
        boolRules[left][right] = true;
    }
    ptr++;
    int update[23];
    int sum = 0;
    while (ptr < end) {
        update[0] = twoDigitStringParse(ptr);
        update[1] = twoDigitStringParse(ptr + 3);
        update[2] = twoDigitStringParse(ptr + 6);
        update[3] = twoDigitStringParse(ptr + 9);
        update[4] = twoDigitStringParse(ptr + 12);
        ptr += 14;
        int updateLength = 5;
        while (*ptr == ',') {
            ptr++;
            update[updateLength] = twoDigitStringParse(ptr);
            update[updateLength + 1] = twoDigitStringParse(ptr + 3);
            updateLength += 2;
            ptr += 5;
        }
        ptr++;

        bool updateIsValid = false;
        if (checkRules(update[0], update[1]) &&
            checkRules(update[1], update[2]) &&
            checkRules(update[2], update[3]) &&
            checkRules(update[3], update[4])) {
            updateIsValid = true;
            for (int i = 4; i < updateLength - 1; i += 2) {
                if (!checkRules(update[i], update[i + 1]) || !checkRules(update[i + 1], update[i + 2])) {
                    updateIsValid = false;
                    break;
                }
            }
        }

        if (!updateIsValid) {
            // Supposed to be topological sort, but works for the inputs anyway (see top comment)
            // Might be faster to do selection since we don't actually need to sort https://en.wikipedia.org/wiki/Selection_algorithm
            // This insertion sort is faster than passing the comparator to qsort
            sum += quickSelect(update, 0, updateLength - 1, updateLength / 2);
        }
    }
    return sum;
}

// 0.005602 ms
void five_2() {
    benchmarkFunctionOnFile("../input/5.txt", &sumCorrectedIncorrectMiddlePages, 500000, 4598);
}
