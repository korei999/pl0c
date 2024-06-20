#pragma once
#include "common.h"

#define LIST_FIRST(HEAD) ((HEAD)->pFirst)
#define LIST_LAST(HEAD) ((HEAD)->pLast)
#define LIST_NEXT(L) ((L)->pNext)
#define LIST_PREV(L) ((L)->pPrev)
#define LIST_FOREACH(L, IT) for (typeof(LIST_FIRST(L)) (IT) = LIST_FIRST(L); (IT); (IT) = LIST_NEXT(IT))
#define LIST_FOREACH_SAFE(L, IT, TMPIT) for (typeof(LIST_FIRST(L)) (IT) = LIST_FIRST(L), (TMPIT) = NULL; (IT) && ((TMPIT) = LIST_NEXT(IT), true); (IT) = (TMPIT))
#define LIST_FOREACH_REV_SAFE(L, IT, TMPIT) for (typeof(LIST_LAST(L)) (IT) = LIST_LAST(L), (TMPIT) = NULL; (IT) && ((TMPIT) = LIST_PREV(IT), true); (IT) = (TMPIT))

#define LIST_GEN_CODE(NAME, T, CMP)                                                                                    \
    typedef struct NAME##Node                                                                                          \
    {                                                                                                                  \
        T data;                                                                                                        \
        struct NAME##Node* pNext;                                                                                      \
        struct NAME##Node* pPrev;                                                                                      \
    } NAME##Node;                                                                                                      \
                                                                                                                       \
    typedef struct NAME                                                                                                \
    {                                                                                                                  \
        NAME##Node* pFirst;                                                                                            \
        NAME##Node* pLast;                                                                                             \
        size_t size;                                                                                                   \
    } NAME;                                                                                                            \
                                                                                                                       \
    [[maybe_unused]] static inline NAME NAME##Create()                                                                 \
    {                                                                                                                  \
        return (NAME) {.pFirst = NULL, .pLast = NULL, .size = 0};                                                      \
    }                                                                                                                  \
                                                                                                                       \
    [[maybe_unused]] static inline NAME##Node* NAME##PushBack(NAME* self, T value)                                     \
    {                                                                                                                  \
        NAME##Node* pNew = (NAME##Node*)calloc(1, sizeof(NAME##Node));                                                 \
        pNew->data = value;                                                                                            \
                                                                                                                       \
        if (!self->pFirst)                                                                                             \
        {                                                                                                              \
            self->pLast = self->pFirst = pNew;                                                                         \
            self->size = 1;                                                                                            \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            self->pLast->pNext = pNew;                                                                                 \
            pNew->pPrev = self->pLast;                                                                                 \
            self->pLast = pNew;                                                                                        \
            self->size++;                                                                                              \
        }                                                                                                              \
                                                                                                                       \
        return pNew;                                                                                                   \
    }                                                                                                                  \
                                                                                                                       \
    [[maybe_unused]] static inline void NAME##Clean(NAME* self)                                                        \
    {                                                                                                                  \
        LIST_FOREACH_SAFE(self, it, t)                                                                                 \
        {                                                                                                              \
            free(it);                                                                                                  \
        }                                                                                                              \
    }                                                                                                                  \
                                                                                                                       \
    [[maybe_unused]] static inline NAME##Node* NAME##Search(NAME* self, T value)                                       \
    {                                                                                                                  \
        LIST_FOREACH(self, it)                                                                                         \
        {                                                                                                              \
            if (CMP(it->data, value) == 0)                                                                             \
                return it;                                                                                             \
        }                                                                                                              \
                                                                                                                       \
        return NULL;                                                                                                   \
    }                                                                                                                  \
                                                                                                                       \
    [[maybe_unused]] static inline void NAME##Remove(NAME* self, NAME##Node* pNode)                                    \
    {                                                                                                                  \
        if (pNode == self->pFirst)                                                                                     \
        {                                                                                                              \
            self->pFirst = self->pFirst->pNext;                                                                        \
            if (self->pFirst)                                                                                          \
                self->pFirst->pPrev = NULL;                                                                            \
        }                                                                                                              \
        else if (pNode == self->pLast)                                                                                 \
        {                                                                                                              \
            self->pLast = self->pLast->pPrev;                                                                          \
            if (self->pLast)                                                                                           \
                self->pLast->pNext = NULL;                                                                             \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            pNode->pPrev->pNext = pNode->pNext;                                                                        \
            pNode->pNext->pPrev = pNode->pPrev;                                                                        \
        }                                                                                                              \
                                                                                                                       \
        free(pNode);                                                                                                   \
    }                                                                                                                  \
                                                                                                                       \
    [[maybe_unused]] static inline void NAME##RemoveValue(NAME* self, T value)                                         \
    {                                                                                                                  \
        NAME##Remove(self, NAME##Search(self, value));                                                                 \
    }
