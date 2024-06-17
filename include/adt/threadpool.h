#pragma once
#include "queue.h"

#include <stdatomic.h>
#include <threads.h>

#ifdef __linux__
    #include <sys/sysinfo.h>
    #define hwConcurrency() get_nprocs()
#endif

typedef struct TaskNode
{
    thrd_start_t pFn;
    void* pArg;
} TaskNode;

QUEUE_GEN_CODE(TaskQ, TaskNode);

typedef struct ThreadPool
{
    bool bDone;
    atomic_int activeTasks;
    thrd_t* aThreads;
    size_t nThreads;
    cnd_t cndQ, cndWait;
    mtx_t mtxQ, mtxWait;
    TaskQ qTasks;
} ThreadPool;

bool
ThreadPoolBusy(ThreadPool* self)
{
    mtx_lock(&self->mtxQ);
    bool ret = self->qTasks.size > 0;
    mtx_unlock(&self->mtxQ);

    return ret || self->activeTasks > 0;
}

static inline int
threadLoop(void* pData)
{
    ThreadPool* self = (ThreadPool*)pData;

    while (!self->bDone)
    {
        TaskNode task;
        {
            mtx_lock(&self->mtxQ);

            while (!(self->qTasks.size > 0 || self->bDone))
                cnd_wait(&self->cndQ, &self->mtxQ);

            if (self->bDone)
            {
                mtx_unlock(&self->mtxQ);
                return 0;
            }

            task = *TaskQPop(&self->qTasks);
            self->activeTasks++; /* increment before unlocking mtxQ to avoid 0 tasks and 0 q possibility */

            mtx_unlock(&self->mtxQ);
        }

        task.pFn(task.pArg);
        self->activeTasks--;

        if (!ThreadPoolBusy(self))
            cnd_signal(&self->cndWait); /* signal for the `ThreadPoolWait()` */
    }

    return 0;
}

static inline void
ThreadPoolSubmit(ThreadPool* self, TaskNode task)
{
    mtx_lock(&self->mtxQ);
    TaskQPush(&self->qTasks, task);
    mtx_unlock(&self->mtxQ);

    cnd_signal(&self->cndQ);
}

static inline ThreadPool
ThreadPoolCreate(size_t nThreads)
{
    ThreadPool tp;
    tp.bDone = false;
    tp.activeTasks = 0;
    tp.aThreads = (thrd_t*)calloc(nThreads, sizeof(thrd_t));
    tp.nThreads = nThreads;
    cnd_init(&tp.cndQ);
    mtx_init(&tp.mtxQ, mtx_plain);
    cnd_init(&tp.cndWait);
    mtx_init(&tp.mtxWait, mtx_plain);
    tp.qTasks = TaskQCreate(nThreads);

    return tp;
}

static inline void
ThreadPoolClean(ThreadPool* self)
{
    free(self->aThreads);
    TaskQClean(&self->qTasks);
    cnd_destroy(&self->cndQ);
    mtx_destroy(&self->mtxQ);
    cnd_destroy(&self->cndWait);
    mtx_destroy(&self->mtxWait);
}

static inline void
ThreadPoolStart(ThreadPool* self)
{
    for (size_t i = 0; i < self->nThreads; i++)
        thrd_create(&self->aThreads[i], threadLoop, self);
}

/* wait until last task is finished */
static inline void
ThreadPoolWait(ThreadPool* self)
{
    while (ThreadPoolBusy(self))
    {
        mtx_lock(&self->mtxWait);
        cnd_wait(&self->cndWait, &self->mtxWait);
        mtx_unlock(&self->mtxWait);
    }
}

static inline void
ThreadPoolStop(ThreadPool* self)
{
    self->bDone = true;
    cnd_broadcast(&self->cndQ);
    for (size_t i = 0; i < self->nThreads; i++)
        thrd_join(self->aThreads[i], nullptr);
}
