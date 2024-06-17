#pragma once
#include "common.h"

#define QUEUE_FIRST_I(Q) ((Q)->first)
#define QUEUE_LAST_I(Q) (((Q)->size == 0) ? 0 : ((Q)->last - 1))
#define QUEUE_NEXT_I(Q, I) (((I) + 1) >= (Q)->capacity ? 0 : ((I) + 1))
#define QUEUE_PREV_I(Q, I) (((I) - 1) < 0 ? ((Q)->capacity - 1) : ((I) - 1))
#define QUEUE_FOREACH_I(Q, I) for (long I = QUEUE_FIRST_I(Q), t = 0; t < (Q)->size; I = QUEUE_NEXT_I(Q, I), t++)
#define QUEUE_FOREACH_I_REV(Q, I) for (long I = QUEUE_LAST_I(Q), t = 0; t < (Q)->size; I = QUEUE_PREV_I(Q, I), t++)

#define QUEUE_GEN_CODE(NAME, T)                                                                                        \
    typedef struct NAME                                                                                                \
    {                                                                                                                  \
        T* pData;                                                                                                      \
        long size;                                                                                                     \
        long capacity;                                                                                                 \
        long first;                                                                                                    \
        long last;                                                                                                     \
    } NAME;                                                                                                            \
                                                                                                                       \
    [[maybe_unused]] static inline void NAME##Push(NAME* self, T data);                                                \
                                                                                                                       \
    [[maybe_unused]] static inline NAME NAME##Create(size_t cap)                                                       \
    {                                                                                                                  \
        assert(cap > 0);                                                                                               \
        return (NAME) {.pData = (T*)calloc(cap, sizeof(T)), .size = 0, .capacity = cap, .first = 0, .last = 0};        \
    }                                                                                                                  \
                                                                                                                       \
    [[maybe_unused]] static inline void NAME##Clean(NAME* self)                                                        \
    {                                                                                                                  \
        free(self->pData);                                                                                             \
    }                                                                                                                  \
                                                                                                                       \
    [[maybe_unused]] static inline T* NAME##First(NAME* self)                                                          \
    {                                                                                                                  \
        assert(self->size > 0);                                                                                        \
        return &self->pData[self->first];                                                                              \
    }                                                                                                                  \
                                                                                                                       \
    [[maybe_unused]] static inline T* NAME##Last(NAME* self)                                                           \
    {                                                                                                                  \
        assert(self->size > 0);                                                                                        \
        return &self->pData[QUEUE_LAST_I(self)];                                                                       \
    }                                                                                                                  \
                                                                                                                       \
    [[maybe_unused]] static inline void NAME##Resize(NAME* self, size_t size)                                          \
    {                                                                                                                  \
        NAME qNew = NAME##Create(size);                                                                                \
                                                                                                                       \
        QUEUE_FOREACH_I(self, i)                                                                                       \
        {                                                                                                              \
            NAME##Push(&qNew, self->pData[i]);                                                                         \
        }                                                                                                              \
                                                                                                                       \
        NAME##Clean(self);                                                                                             \
        *self = qNew;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    [[maybe_unused]] static inline void NAME##Push(NAME* self, T data)                                                 \
    {                                                                                                                  \
        if (self->size >= self->capacity)                                                                              \
            NAME##Resize(self, self->capacity * 2);                                                                    \
                                                                                                                       \
        long i = self->last;                                                                                           \
        long ni = QUEUE_NEXT_I(self, i);                                                                               \
        self->pData[i] = data;                                                                                         \
        self->last = ni;                                                                                               \
        self->size++;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    [[maybe_unused]] static inline T* NAME##Pop(NAME* self)                                                            \
    {                                                                                                                  \
        assert(self->size > 0);                                                                                        \
        T* ret = &self->pData[self->first];                                                                            \
        self->first = QUEUE_NEXT_I(self, self->first);                                                                 \
        self->size--;                                                                                                  \
        return ret;                                                                                                    \
    }
