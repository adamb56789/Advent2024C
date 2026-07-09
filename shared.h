//
// Created by Adam on 20/06/2026.
//

#ifndef ADVENT2024C_SHARED_H
#define ADVENT2024C_SHARED_H

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <immintrin.h>
#include <assert.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef __m128i i128;
typedef __m256i i256;
typedef __m512i i512;

typedef __mmask16 mask16;
typedef __mmask32 mask32;
typedef __mmask64 mask64;

static void fatal(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

// #define ENABLE_DEBUG_LOGS

#ifdef ENABLE_DEBUG_LOGS
#define debug(M, ...) printf(M, ##__VA_ARGS__)
#else
#define debug(M, ...)
#endif

#endif //ADVENT2024C_SHARED_H
