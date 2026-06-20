//
// Created by Adam on 20/06/2026.
//

#ifndef ADVENT2024C_SHARED_H
#define ADVENT2024C_SHARED_H

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;

static void fatal(const char *msg) {
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(EXIT_FAILURE);
}

#endif //ADVENT2024C_SHARED_H
