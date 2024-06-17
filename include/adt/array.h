#pragma once
#include "common.h"

#define ARRAY_GEN_CODE(NAME, T)                                                                                        \
    typedef struct NAME                                                                                                \
    {                                                                                                                  \
        T* pData;                                                                                                      \
        size_t size;                                                                                                   \
        size_t capacity;                                                                                               \
    } NAME;                                                                                                            \
                                                                                                                       \
    [[maybe_unused]] static inline NAME NAME##Create(size_t cap)                                                       \
    {                                                                                                                  \
        assert(cap > 0);                                                                                               \
        return (NAME) {.pData = (T*)calloc(cap, sizeof(T)), .size = 0, .capacity = cap};                               \
    }                                                                                                                  \
                                                                                                                       \
    [[maybe_unused]] static inline void NAME##Push(NAME* self, T value)                                                \
    {                                                                                                                  \
        if (self->size >= self->capacity)                                                                              \
        {                                                                                                              \
            self->capacity *= 2;                                                                                       \
            self->pData = (T*)reallocarray(self->pData, self->capacity, sizeof(T));                                    \
        }                                                                                                              \
                                                                                                                       \
        self->pData[self->size++] = value;                                                                             \
    }                                                                                                                  \
                                                                                                                       \
    [[maybe_unused]] static inline void NAME##Clean(NAME* self)                                                        \
    {                                                                                                                  \
        free(self->pData);                                                                                             \
    }                                                                                                                  \
                                                                                                                       \
    [[maybe_unused]] static inline T* NAME##Pop(NAME* self)                                                            \
    {                                                                                                                  \
        assert(self->size > 0 && "poping empty array");                                                                \
        return &self->pData[--self->size];                                                                             \
    }
