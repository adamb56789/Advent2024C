#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include "shared.h"

#ifdef _WIN32
#include <stdint.h>
#include <stdio.h>
#include <windows.h>

struct WindowsFile {
    HANDLE hFile;
    HANDLE hMap;
    DWORD fileSize;
    char *data;
};

static struct WindowsFile loadFile(const char *path) {
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

static double getTime() {
    LARGE_INTEGER t, f;
    QueryPerformanceCounter(&t);
    QueryPerformanceFrequency(&f);
    return (double) t.QuadPart / (double) f.QuadPart;
}

#define GenericFile WindowsFile

#else

#include <time.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

struct LinuxFile {
    int fd;
    size_t fileSize;
    char *data;
};

static struct LinuxFile loadFile(const char *path) {
    const int fd = open(path, O_RDONLY);
    if (fd == -1) {
        fatal("Could not open file");
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        fatal("fstat failed");
    }

    const size_t fileSize = (size_t) st.st_size;

    char *data = mmap(NULL, fileSize,PROT_READ,MAP_PRIVATE, fd, 0);

    if (data == MAP_FAILED) {
        close(fd);
        fatal("mmap failed");
    }

    return (struct LinuxFile){
        .fd = fd,
        .fileSize = fileSize,
        .data = data
    };
}

static void closeFile(const struct LinuxFile file) {
    munmap(file.data, file.fileSize);
    close(file.fd);
}

static double getTime() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (double) ts.tv_sec + (double) ts.tv_nsec / 1e9;
}

#define GenericFile LinuxFile

#endif

#endif //FILE_UTILS_H
