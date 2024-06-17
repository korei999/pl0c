#pragma once

#include <stdlib.h>
#include <time.h>

#define LENGTH(A) (sizeof(A) / sizeof(A[0]))

static inline size_t
hashFNV(const char* str)
{
    size_t hash = 0xCBF29CE484222325;
    for (; *str; str++)
        hash = (hash ^ *str) * 0x100000001B3;
    return hash;
}

static inline size_t
hashMurmurOAAT64(const char* key)
{
    size_t h = 525201411107845655ull;
    for (; *key; ++key)
    {
        h ^= *key;
        h *= 0x5bd1e9955bd1e995;
        h ^= h >> 47;
    }
    return h;
}

static inline size_t
hashInt(int num)
{
    return (size_t)num;
}

static inline int
intCmp(int a, int b)
{
    if (a > b) return 1;
    else if (a < b) return -1;
    else return 0;
}

static inline char*
randomString(size_t length)
{
    const char charset[] = "0123456789"
                           "abcdefghijklmnopqrstuvwxyz"
                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    char* ret = calloc(length, sizeof(char));

    for (size_t i = 0; i < length - 1; i++)
    {
        size_t idx = rand() % (LENGTH(charset) - 1);
        ret[i] = charset[idx];
    }

    return ret;
}

static inline double
msTimeNow()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    time_t micros = ts.tv_sec * 1000000000;
    micros += ts.tv_nsec;
    return micros / 1000000.0;
}
