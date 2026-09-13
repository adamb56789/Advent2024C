//
// Created by Adam on 13/09/2026.
//

#include "9.h"

static i64 sumIntegers(const i64 a, const i64 size) {
    const i64 b = a + size;
    return (a + b - 1) * (b - a) / 2;
}

i64 computeCompactedFilesystemChecksum(const char *ptr, const char *end) {
    i64 sum = 0;
    const i64 inputSize = end - ptr;

    i64 left = 0;
    i64 right = inputSize - (2 - inputSize % 2); // Rightmost file

    i64 rightFileId = right / 2;
    i64 rightBlocksRemaining = ptr[right] - '0';

    i64 i = 0;
    while (left < right) {
        // Lay out and sum the current file
        const i64 fileId = left / 2;
        const u8 fileSize = ptr[left] - '0';

        // Formula for sum of integers between a and b
        sum += fileId * sumIntegers(i, fileSize);
        i += fileSize;

        u8 spaceSize = ptr[left + 1] - '0';

        // Pull blocks from the rightmost file into the space until all its blocks are used, or we fill the space
        while (spaceSize > 0) {
            const i64 blocksMovedFromRight = rightBlocksRemaining < spaceSize ? rightBlocksRemaining : spaceSize;
            sum += rightFileId * sumIntegers(i, blocksMovedFromRight);

            i += blocksMovedFromRight;
            rightBlocksRemaining -= blocksMovedFromRight;
            spaceSize -= blocksMovedFromRight;

            // If used all blocks from the right, get the next one
            if (rightBlocksRemaining == 0) {
                right -= 2;

                if (left == right) break;

                rightFileId--;
                rightBlocksRemaining = ptr[right] - '0';
            }
        }
        left += 2;
    }

    // Could be some blocks in the right file left
    sum += rightFileId * sumIntegers(i, rightBlocksRemaining);

    return sum;
}
