#ifndef THREE_H
#define THREE_H
#include "shared.h"

/**
 * 160672468
 * 25 ms for 1000x file length
 */
i64 sumMul(const char *ptr, const char *end);

/**
 * 84893551
 * 25 ms for 1000x file length
 */
i64 sumEnabledMul(const char *ptr, const char *end);

#endif //THREE_H
