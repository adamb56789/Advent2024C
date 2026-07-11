    //
    // Created by Adam on 06/07/2025.
    //

    #include "7.h"

#include "file-utils.h"


    static uint64_t myParseInt64(char **s) {
        uint64_t value = 0;
        while (**s >= '0' && **s <= '9') {
            value = value * 10 + (**s - '0');
            (*s)++;
        }
        return value;
    }

    // Byte packed magic numbers on the strings like: "999", "99 ", "99\n", "9 9", "9\n9", "9\n\n".
    // There are 1251 of them which is easily magicked into 4096 byte table which fits in L1 cache.
    // To save if statements for the length to increment the pointer we pack length in the higher 16 bits of the int.
    // We can fit the data in 16 bits instead of 32 but it doesn't make it faster.
    // Saves a few microseconds.

    #define TABLE_SIZE (1 << 12)
    #define MAGIC 140350941329293ULL
    #define SHIFT 43

    #define LENGTH_BIT_SHIFT 16
    #define VAL_BIT_MASK 0xFFFF
    static int lookupTable[TABLE_SIZE];
    static int tableBuilt = 0;

    static uint32_t encodeKey(const char *ptr) {
        const uint32_t val = *(int32_t *) ptr & 0x00FFFFFF;
        return (uint32_t) ((val * MAGIC) >> SHIFT) & (TABLE_SIZE - 1);
    }

    static void buildLookupTable() {
        if (tableBuilt) return;
        for (int i = 100; i <= 999; ++i) {
            char buf[4];
            snprintf(buf, sizeof(buf), "%03d", i);
            const uint32_t index = encodeKey(buf);
            lookupTable[index] = (3 << LENGTH_BIT_SHIFT) | i;
        }

        for (int i = 10; i <= 99; ++i) {
            char buf[4];
            snprintf(buf, sizeof(buf), "%02d ", i);
            uint32_t index = encodeKey(buf);
            lookupTable[index] = (2 << LENGTH_BIT_SHIFT) | i;

            snprintf(buf, sizeof(buf), "%02d\n", i);
            index = encodeKey(buf);
            lookupTable[index] = (2 << LENGTH_BIT_SHIFT) | i;
        }

        for (int i = 1; i <= 9; ++i) {
            char buf[4];
            snprintf(buf, sizeof(buf), "%01d\n\n", i);
            uint32_t index = encodeKey(buf);
            lookupTable[index] = (1 << LENGTH_BIT_SHIFT) | i;
            for (int j = 1; j <= 9; ++j) {
                snprintf(buf, sizeof(buf), "%01d %01d", i, j);
                index = encodeKey(buf);
                lookupTable[index] = (1 << LENGTH_BIT_SHIFT) | i;

                snprintf(buf, sizeof(buf), "%01d\n%01d", i, j);
                index = encodeKey(buf);
                lookupTable[index] = (1 << LENGTH_BIT_SHIFT) | i;
            }
        }

        tableBuilt = 1;
    }

    static int myParse3Digit(const char **s) {
        const int result = lookupTable[encodeKey(*s)];
        *s += result >> LENGTH_BIT_SHIFT;
        return result & VAL_BIT_MASK;
    }

    static int reverseDfs(const int *equation, const int i, const uint64_t acc) {
        // By going backwards through the tree we do much more aggressive culling, making it faster, even with a divide.
        // The div instruction does both divide and modulo at the same time, the compiler knows this of course.
        // It also somehow inlines a recursive function.
        if (i == 0) return acc == equation[0];
        return acc % equation[i] == 0 && reverseDfs(equation, i - 1, acc / equation[i])
               || acc >= equation[i] && reverseDfs(equation, i - 1, acc - equation[i]);
    }

    int totalCalibrationResult(const char *ptr, const char *end) {
        buildLookupTable();
        uint64_t result = 0;
        int equation[12];
        while (ptr < end - 1) {
            const uint64_t testValue = myParseInt64(&ptr);
            ptr++;
            int equationLength = 0;
            while (*ptr != '\n') {
                ptr++;
                equation[equationLength++] = myParse3Digit(&ptr);
            }
            ptr++;

            if (reverseDfs(equation, equationLength - 1, testValue)) {
                result += testValue;
            }
        }
        return result;
    }

    // 0.137 ms
    void seven_1() {
        // The answer is actually 7710205485870 but that doesn't fit in an int. Good enough to verify correct output.
        // benchmarkFunctionOnFile("../input/7.txt", &totalCalibrationResult, 8000, 739189550);
    }

    static uint64_t splitInt64(const uint64_t a, const uint64_t b) {
        // Examples: 12345, 45 -> 123. 12345, 99 -> 0.
        uint64_t divisor = 1;
        uint64_t remaining = b;
        if (remaining < 10) divisor = 10;
        else if (remaining < 100) divisor = 100;
        else if (remaining < 1000) divisor = 1000;
        else if (remaining < 10000) divisor = 10000;
        else {
            while (remaining > 0) {
                divisor *= 10;
                remaining /= 10;
            }
        }

        if (a % divisor != b) return 0;
        return a / divisor; // Replace div with lookup into multiply magic numbers?
    }

    static int reverseDfsConcat(const int *equation, const int i, const uint64_t acc) {
        if (i == 0) return acc == equation[0];
        // This specific order is faster in tests. Probably because % and split are much more likely to cull the tree.
        return acc % equation[i] == 0 && reverseDfsConcat(equation, i - 1, acc / equation[i])
               || splitInt64(acc, equation[i]) != 0 && reverseDfsConcat(equation, i - 1, splitInt64(acc, equation[i]))
               || acc >= equation[i] && reverseDfsConcat(equation, i - 1, acc - equation[i]);
    }

    int totalConcatCalibrationResult(const char *ptr, const char *end) {
        buildLookupTable();
        uint64_t result = 0;
        int equation[12];
        while (ptr < end - 1) {
            const uint64_t testValue = myParseInt64(&ptr);
            ptr++;
            int equationLength = 0;
            while (*ptr != '\n') {
                ptr++;
                equation[equationLength++] = myParse3Digit(&ptr);
            }
            ptr++;

            if (reverseDfsConcat(equation, equationLength - 1, testValue)) {
                result += testValue;
            }
        }
        return result;
    }

    // 0.216 ms
    void seven_2() {
        // 20928985450275
        // benchmarkFunctionOnFile("../input/7.txt", &totalConcatCalibrationResult, 4000, -390183133);
    }
