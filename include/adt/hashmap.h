#pragma once
#include "common.h"

#define ADT_HASHMAP_DEFAULT_LOAD_FACTOR 0.5

#define HASHMAP_GEN_CODE(NAME, T, FNHASH, CMP, LOAD_FACTOR)                                                            \
    /* Hash map with linear probing */                                                                                 \
    typedef struct NAME##Bucket                                                                                        \
    {                                                                                                                  \
        T data;                                                                                                        \
        bool bOccupied;                                                                                                \
        bool bDeleted;                                                                                                 \
    } NAME##Bucket;                                                                                                    \
                                                                                                                       \
    typedef struct NAME                                                                                                \
    {                                                                                                                  \
        NAME##Bucket* pBuckets;                                                                                        \
        size_t bucketCount;                                                                                            \
        size_t capacity;                                                                                               \
    } NAME;                                                                                                            \
                                                                                                                       \
    typedef struct NAME##ReturnNode                                                                                    \
    {                                                                                                                  \
        T* pData;                                                                                                      \
        size_t hash;                                                                                                   \
        size_t idx;                                                                                                    \
        bool bInserted;                                                                                                \
    } NAME##ReturnNode;                                                                                                \
                                                                                                                       \
    [[maybe_unused]] static inline NAME##ReturnNode NAME##Insert(NAME* self, T data);                                  \
                                                                                                                       \
    [[maybe_unused]] static inline NAME NAME##Create(size_t cap)                                                       \
    {                                                                                                                  \
        assert(cap > 0 && "cap should be > 0");                                                                        \
        return (NAME) {                                                                                                \
            .pBuckets = (NAME##Bucket*)calloc(cap, sizeof(NAME##Bucket)), .bucketCount = 0, .capacity = cap};          \
    }                                                                                                                  \
                                                                                                                       \
    [[maybe_unused]] static inline void NAME##Clean(NAME* self)                                                        \
    {                                                                                                                  \
        free(self->pBuckets);                                                                                          \
    }                                                                                                                  \
                                                                                                                       \
    [[maybe_unused]] static inline double NAME##LoadFactor(NAME* self)                                                 \
    {                                                                                                                  \
        return (double)self->bucketCount / (double)self->capacity;                                                     \
    }                                                                                                                  \
                                                                                                                       \
    [[maybe_unused]] static inline void NAME##Rehash(NAME* self, size_t cap)                                           \
    {                                                                                                                  \
        NAME mapNew = NAME##Create(cap);                                                                               \
                                                                                                                       \
        for (size_t i = 0; i < self->capacity; i++)                                                                    \
            if (self->pBuckets[i].bOccupied)                                                                           \
                NAME##Insert(&mapNew, self->pBuckets[i].data);                                                         \
                                                                                                                       \
        NAME##Clean(self);                                                                                             \
        *self = mapNew;                                                                                                \
    }                                                                                                                  \
                                                                                                                       \
    [[maybe_unused]] static inline NAME##ReturnNode NAME##Insert(NAME* self, T data)                                   \
    {                                                                                                                  \
        if (NAME##LoadFactor(self) >= LOAD_FACTOR)                                                                     \
            NAME##Rehash(self, self->capacity * 2);                                                                    \
                                                                                                                       \
        size_t hash = FNHASH(data);                                                                                    \
        size_t idx = hash % self->capacity;                                                                            \
                                                                                                                       \
        while (self->pBuckets[idx].bOccupied)                                                                          \
        {                                                                                                              \
            idx++;                                                                                                     \
            if (idx >= self->capacity)                                                                                 \
                idx = 0;                                                                                               \
        }                                                                                                              \
        self->pBuckets[idx].data = data;                                                                               \
        self->pBuckets[idx].bOccupied = true;                                                                          \
        self->pBuckets[idx].bDeleted = false;                                                                          \
        self->bucketCount++;                                                                                           \
                                                                                                                       \
        return (NAME##ReturnNode) {.pData = &self->pBuckets[idx].data, .hash = hash, .idx = idx, .bInserted = true};   \
    }                                                                                                                  \
                                                                                                                       \
    [[maybe_unused]] static inline NAME##ReturnNode NAME##Search(NAME* self, T data)                                   \
    {                                                                                                                  \
        size_t hash = FNHASH(data);                                                                                    \
        size_t idx = hash % self->capacity;                                                                            \
                                                                                                                       \
        NAME##ReturnNode ret;                                                                                          \
        ret.hash = hash;                                                                                               \
        ret.pData = nullptr;                                                                                           \
        ret.bInserted = false;                                                                                         \
                                                                                                                       \
        while (self->pBuckets[idx].bOccupied || self->pBuckets[idx].bDeleted)                                          \
        {                                                                                                              \
            if (CMP(self->pBuckets[idx].data, data) == 0)                                                              \
            {                                                                                                          \
                ret.pData = &self->pBuckets[idx].data;                                                                 \
                break;                                                                                                 \
            }                                                                                                          \
                                                                                                                       \
            idx++;                                                                                                     \
            if (idx >= self->capacity)                                                                                 \
                idx = 0;                                                                                               \
        }                                                                                                              \
                                                                                                                       \
        ret.idx = idx;                                                                                                 \
        return ret;                                                                                                    \
    }                                                                                                                  \
                                                                                                                       \
    [[maybe_unused]] static inline void NAME##Remove(NAME* self, size_t i)                                             \
    {                                                                                                                  \
        self->pBuckets[i].bDeleted = true;                                                                             \
        self->pBuckets[i].bOccupied = false;                                                                           \
    }                                                                                                                  \
                                                                                                                       \
    [[maybe_unused]] static inline NAME##ReturnNode NAME##TryInsert(NAME* self, T data)                                \
    {                                                                                                                  \
        NAME##ReturnNode f = NAME##Search(self, data);                                                                 \
        if (f.pData)                                                                                                   \
            return f;                                                                                                  \
        else                                                                                                           \
            return NAME##Insert(self, data);                                                                           \
    }
