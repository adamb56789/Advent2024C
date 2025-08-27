#ifndef FILE_UTILS_H
#define FILE_UTILS_H
#include <stdint.h>
#include <stdio.h>
#include <windows.h>

struct WindowsFile {
    HANDLE hFile;
    HANDLE hMap;
    DWORD fileSize;
    char *data;
};

static struct WindowsFile readChars(const char *path) {
    const HANDLE hFile = CreateFileA(
        path, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL
    );

    const DWORD fileSize = GetFileSize(hFile, NULL);
    const HANDLE hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

    char *data = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);

    const struct WindowsFile file = {hFile, hMap, fileSize, data};
    return file;
}

static void closeFile(const struct WindowsFile file) {
    UnmapViewOfFile(file.data);
    CloseHandle(file.hMap);
    CloseHandle(file.hFile);
}

static void runFunctionOnFile(const char *inputFilePath, int (*f)(const char *, const char *)) {
    const struct WindowsFile file = readChars(inputFilePath);
    const char *ptr = file.data;

    printf("%d\n", f(ptr, ptr + file.fileSize));
    closeFile(file);
}

static double getTime() {
    LARGE_INTEGER t, f;
    QueryPerformanceCounter(&t);
    QueryPerformanceFrequency(&f);
    return (double) t.QuadPart / (double) f.QuadPart;
}

static void benchmarkFunctionOnFile(
    const char *inputFilePath,
    int (*f)(const char *, const char *),
    const int runs,
    const int expected
) {
    const struct WindowsFile file = readChars(inputFilePath);
    const char *ptr = file.data;

    // Record time of first execution
    const double firstStart = getTime();
    const int result = f(ptr, ptr + file.fileSize);
    const double firstElapsed = getTime() - firstStart;

    if (result != expected) {
        printf("Result incorrect!\nExpected: %d\nActual: %d\n", expected, result);
        return;
    }

    volatile int dummy = 0; // Compiler sometimes optimises the entire thing away without doing something with the result.

    // Run twice more to warm up
    dummy = f(ptr, ptr + file.fileSize);
    dummy = f(ptr, ptr + file.fileSize);

    const double start = getTime();
    for (int i = 0; i < runs; ++i) {
        dummy = f(ptr, ptr + file.fileSize);
    }
    const double elapsed = getTime() - start;
    const double average = elapsed / runs;
    printf("Average: %f ms\nFirst:   %f ms\nElapsed: %f seconds\n", average * 1000, firstElapsed * 1000, elapsed);
    closeFile(file);
}


#endif //FILE_UTILS_H
