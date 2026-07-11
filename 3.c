#include "3.h"

i64 sumMul(const char *ptr, const char *end) {
    // mul\(\d+,\d+\) regex finds 671 occurrences
    int sum = 0;
    while (ptr < end) {
        if (!(ptr[0] == 'm' && ptr + 4 < end && ptr[1] == 'u' && ptr[2] == 'l' && ptr[3] == '(')) {
            ptr++;
            continue;
        }
        ptr += 4;

        int x;
        if (ptr[1] == ',') {
            x = ptr[0] - '0';
            ptr += 2;
        } else if (ptr[2] == ',') {
            x = (ptr[0] - '0') * 10 + ptr[1] - '0';
            ptr += 3;
        } else if (ptr[3] == ',') {
            x = (ptr[0] - '0') * 100 + (ptr[1] - '0') * 10 + ptr[2] - '0';
            ptr += 4;
        } else {
            continue;
        }
        int y;
        if (ptr[1] == ')') {
            y = ptr[0] - '0';
            ptr += 2;
        } else if (ptr[2] == ')') {
            y = (ptr[0] - '0') * 10 + ptr[1] - '0';
            ptr += 3;
        } else if (ptr[3] == ')') {
            y = (ptr[0] - '0') * 100 + (ptr[1] - '0') * 10 + ptr[2] - '0';
            ptr += 4;
        } else {
            continue;
        }

        sum += x * y;
    }
    return sum;
}

i64 sumEnabledMul(const char *ptr, const char *end) {
    // mul\(\d+,\d+\) regex finds 671 occurrences
    int sum = 0;
    bool enabled = true;
    while (ptr < end) {
        while (*ptr != 'm' && *ptr != 'd' && ptr < end) ptr++;
        if (ptr[0] == 'm') {
            ptr++;
            if (!(ptr[0] == 'u' && ptr[1] == 'l' && ptr[2] == '(')) continue;
            ptr += 3;

            int x;
            if (ptr[1] == ',') {
                x = ptr[0] - '0';
                ptr += 2;
            } else if (ptr[2] == ',') {
                x = (ptr[0] - '0') * 10 + ptr[1] - '0';
                ptr += 3;
            } else if (ptr[3] == ',') {
                x = (ptr[0] - '0') * 100 + (ptr[1] - '0') * 10 + ptr[2] - '0';
                ptr += 4;
            } else {
                continue;
            }
            int y;
            if (ptr[1] == ')') {
                y = ptr[0] - '0';
                ptr += 2;
            } else if (ptr[2] == ')') {
                y = (ptr[0] - '0') * 10 + ptr[1] - '0';
                ptr += 3;
            } else if (ptr[3] == ')') {
                y = (ptr[0] - '0') * 100 + (ptr[1] - '0') * 10 + ptr[2] - '0';
                ptr += 4;
            } else {
                continue;
            }

            if (enabled) {
                sum += x * y;
            }
        } else {
            if (ptr[1] != 'o') continue;
            if (ptr[2] == '(' && ptr[3] == ')') {
                enabled = true;
                ptr += 4;
            } else if (ptr[2] == 'n' && ptr[3] == '\'' && ptr[4] == 't' && ptr[5] == '(' && ptr[6] == ')') {
                enabled = false;
                ptr += 7;
            } else {
                ptr++;
            }
        }
    }
    return sum;
}