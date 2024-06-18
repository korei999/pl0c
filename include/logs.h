#pragma once

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef LOGS

    #define COUT(...) fprintf(stdout, __VA_ARGS__)
    #define CERR(...) fprintf(stderr, __VA_ARGS__)

enum _LOG_SEV
{
    _LOG_SEV_OK,
    _LOG_SEV_GOOD,
    _LOG_SEV_WARN,
    _LOG_SEV_BAD,
    _LOG_SEV_FATAL,
    _LOG_SEV_ENUM_SIZE
};

static const char* _LOG_SEV_STR[] = {"", "GOOD: ", "WARNING: ", "BAD: ", "FATAL: "};

    #define _LOG(SEV, ...)                                                                                             \
        {                                                                                                              \
            assert(SEV >= 0 && SEV < _LOG_SEV_ENUM_SIZE && "wrong _LOG_SEV*");                                         \
            CERR("(%s%s, %d): ", _LOG_SEV_STR[SEV], __FILE__, __LINE__);                                               \
            CERR(__VA_ARGS__);                                                                                         \
            switch (SEV)                                                                                               \
            {                                                                                                          \
                default:                                                                                               \
                    break;                                                                                             \
                case _LOG_SEV_BAD:                                                                                     \
                    exit((int)SEV);                                                                                    \
                case _LOG_SEV_FATAL:                                                                                   \
                    abort();                                                                                           \
            }                                                                                                          \
        }

    #define LOG_OK(...) _LOG(_LOG_SEV_OK, __VA_ARGS__)
    #define LOG_WARN(...) _LOG(_LOG_SEV_WARN, __VA_ARGS__)
    #define LOG_BAD(...) _LOG(_LOG_SEV_BAD, __VA_ARGS__)
    #define LOG_FATAL(...) _LOG(_LOG_SEV_FATAL, __VA_ARGS__)

#endif
