//
// Created by Adam on 13/09/2026.
//

#include "9.h"

i64 computeCompactedFilesystemChecksum(const char *ptr, const char *end) {
    i64 sum = 0;
    int inputSize = end - ptr;
    int left = 0;
    int right = inputSize - (2 - inputSize % 2); // Rightmost file
    int rightFileId = right / 2;
    int rightRemainingLength = ptr[right] - '0';

    char buffer[50000] = {0}; // It appears to completely optimize this out on release

    int i = 0;
    while (left < right) {
        // Lay out and sum the current file
        const int fileId = left / 2;
        const u8 size = ptr[left] - '0';
        for (u8 j = 0; j < size; ++j) {
            buffer[i] = fileId + '0';
            sum += fileId * i++;
        }

        u8 spaceSize = ptr[left + 1] - '0';

        // Pull blocks from the rightmost file into the space until all its blocks are used, or we fill the space
        while (spaceSize > 0) {
            for (; rightRemainingLength > 0 && spaceSize > 0; --rightRemainingLength, --spaceSize) {
                buffer[i] = rightFileId + '0';
                sum += rightFileId * i++;
            }

            // If used all blocks from the right, get the next one
            if (rightRemainingLength == 0) {
                right -= 2;

                if (left == right) break;

                rightFileId--;
                rightRemainingLength = ptr[right] - '0';
            }
        }
        left += 2;
    }
    for (; rightRemainingLength > 0; rightRemainingLength--) {
        buffer[i] = rightFileId + '0';
        sum += rightFileId * i++;
    }
    return sum;
}
