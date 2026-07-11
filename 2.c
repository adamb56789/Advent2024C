#include "2.h"

static i8 PARSE_TABLE[14648];
static bool tableBuilt = false;

static i16 encodeKey(const char *ptr) {
    return *(i16 *) ptr;
}

static void initParseTable() {
    if (tableBuilt) return;
    for (char i = '0'; i <= '9'; ++i) {
        for (char j = '0'; j <= '9'; ++j) {
            const char s[2] = {i, j};
            PARSE_TABLE[encodeKey(s)] = (i - '0') * 10 + j - '0';
        }
    }
    for (char i = '1'; i <= '9'; ++i) {
        const char s[2] = {i, ' '};
        PARSE_TABLE[encodeKey(s)] = i - '0';
        const char s2[2] = {i, '\n'};
        PARSE_TABLE[encodeKey(s2)] = i - '0';
    }
    tableBuilt = true;
}

static bool isCharDigit(const char c) {
    return '0' <= c && c <= '9';
}

static int twoDigitStringParse(const char *ptr) {
    return PARSE_TABLE[encodeKey(ptr)];
}

static bool areLevelsSafe(const int a, const int b, const bool isAscending) {
    const int diff = abs(b - a);
    return isAscending == a < b && 1 <= diff && diff <= 3;
}

static int searchForDangerousLevel(const int levels[], const int numberOfLevels, const int skip) {
    bool first = true;
    bool isAscending = true;

    int a = -1;
    for (int i = 0; i < numberOfLevels && i < 8; i++) {
        if (i == skip) continue;

        const int b = levels[i];
        if (a != -1) {
            if (first) {
                isAscending = a < b;
                first = false;
            }

            if (!areLevelsSafe(a, b, isAscending)) {
                return i - 1;
            }
        }
        a = b;
    }

    return -1;
}

long countDangerousLevels(const char *ptr, const char *end) {
    initParseTable();
    int safeCount = 0;
    int levels[8];
    while (ptr < end) {
        int numberOfLevels = 0;
        while (true) {
            levels[numberOfLevels] = twoDigitStringParse(ptr);
            if (isCharDigit(ptr[1])) {
                ptr += 2;
            } else {
                ptr++;
            }
            numberOfLevels++;
            if (ptr[0] == '\n') {
                break;
            }
            ptr++; // skip space
        }
        ptr++; // skip new line

        const bool isSafe = -1 == searchForDangerousLevel(levels, numberOfLevels, -1);
        if (isSafe) safeCount++;
    }
    return safeCount;
}

long countDangerousLevelsWithTolerance(const char *ptr, const char *end) {
    initParseTable();
    int safeCount = 0;
    while (ptr < end) {
        int levels[8];
        int numberOfLevels = 0;
        while (true) {
            levels[numberOfLevels] = twoDigitStringParse(ptr);
            if (isCharDigit(ptr[1])) {
                ptr += 2;
            } else {
                ptr++;
            }
            numberOfLevels++;
            if (ptr[0] == '\n') {
                break;
            }
            ptr++; // skip space
        }
        ptr++; // skip new line

        const int dangerIndex = searchForDangerousLevel(levels, numberOfLevels, -1);
        bool isSafe = dangerIndex == -1;
        if (!isSafe) {
            isSafe = -1 == searchForDangerousLevel(levels, numberOfLevels, dangerIndex - 1) || -1 ==
                     searchForDangerousLevel(levels, numberOfLevels, dangerIndex) || -1 ==
                     searchForDangerousLevel(levels, numberOfLevels, dangerIndex + 1);
        }
        if (isSafe) safeCount++;
    }
    return safeCount;
}
