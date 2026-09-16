//
// Created by Adam on 13/09/2026.
//

#include "9.h"

// Number of files
#define N 10000

static u8 ctoi(const char c) {
    return c - '0';
}

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
    i64 rightBlocksRemaining = ctoi(ptr[right]);

    i64 i = 0;
    while (left < right) {
        // Lay out and sum the current file
        const i64 fileId = left / 2;
        const u8 fileSize = ctoi(ptr[left]);

        // Formula for sum of integers between a and b
        sum += fileId * sumIntegers(i, fileSize);
        i += fileSize;

        u8 spaceSize = ctoi(ptr[left + 1]);

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
                rightBlocksRemaining = ctoi(ptr[right]);
            }
        }
        left += 2;
    }

    // Could be some blocks in the right file left
    sum += rightFileId * sumIntegers(i, rightBlocksRemaining);

    return sum;
}

i64 defragmentedCompactedFilesystemChecksum(const char *ptr, const char *end) {
    u8 files[N] = {0};
    u8 spaces[N] = {0};

    for (int i = 0; i < N - 1; i++) {
        files[i] = ctoi(ptr[2 * i]);
        spaces[i] = ctoi(ptr[2 * i + 1]);
    }
    files[N - 1] = ctoi(ptr[2 * (N - 1)]);

    int fileStartPosition[N] = {0};
    int spaceStartPosition[N] = {0};
    spaceStartPosition[0] = files[0];
    for (int i = 1; i < N; ++i) {
        fileStartPosition[i] = fileStartPosition[i - 1] + files[i - 1] + spaces[i - 1];
        spaceStartPosition[i] = fileStartPosition[i] + files[i];
    }

    int index_this_file_size_was_last_placed[10] = {0};

    i64 sum = 0;
    for (int right = N - 1; right >= 0; right--) {
        const int fileSize = files[right];

        // We can start searching from the last index that this size of file was placed, is there is no way for space
        // to have opened up to the left of that point.
        for (int left = index_this_file_size_was_last_placed[fileSize]; left < right; left++) {
            if (spaces[left] >= fileSize) {
                index_this_file_size_was_last_placed[fileSize] = left;

                // Found a space, place file into the space after the "left" file and checksum it
                sum += right * sumIntegers(spaceStartPosition[left], fileSize);

                // Update the space we placed into
                spaceStartPosition[left] += fileSize;
                spaces[left] -= fileSize;

                // No need to update the original location of the "right" file as we calculate sum as we go and only move left
                goto next;
            }
        }
        // Found no space to place the file in, so sum it where it stands
        sum += right * sumIntegers(fileStartPosition[right], fileSize);

    next:;
    }

    return sum;
}
