/*------------------------------------------------------------------------------
--       Copyright (c) 2015-2017, VeriSilicon Inc. All rights reserved        --
--         Copyright (c) 2011-2014, Google Inc. All rights reserved.          --
--         Copyright (c) 2007-2010, Hantro OY. All rights reserved.           --
--                                                                            --
-- This software is confidential and proprietary and may be used only as      --
--   expressly authorized by VeriSilicon in a written licensing agreement.    --
--                                                                            --
--         This entire notice must be reproduced on all copies                --
--                       and may not be removed.                              --
--                                                                            --
--------------------------------------------------------------------------------
-- Redistribution and use in source and binary forms, with or without         --
-- modification, are permitted provided that the following conditions are met:--
--   * Redistributions of source code must retain the above copyright notice, --
--       this list of conditions and the following disclaimer.                --
--   * Redistributions in binary form must reproduce the above copyright      --
--       notice, this list of conditions and the following disclaimer in the  --
--       documentation and/or other materials provided with the distribution. --
--   * Neither the names of Google nor the names of its contributors may be   --
--       used to endorse or promote products derived from this software       --
--       without specific prior written permission.                           --
--------------------------------------------------------------------------------
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"--
-- AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE  --
-- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE --
-- ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE  --
-- LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR        --
-- CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF       --
-- SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS   --
-- INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN    --
-- CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)    --
-- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE --
-- POSSIBILITY OF SUCH DAMAGE.                                                --
--------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

#include "vp8hwd_buffer_queue.h"

#include <pthread.h>

#include "fifo.h"

/* macro for assertion, used only when _ASSERT_USED is defined */
#ifdef _ASSERT_USED
#ifndef ASSERT
#include <assert.h>
#define ASSERT(expr) assert(expr)
#endif
#else
#define ASSERT(expr)
#endif

#ifdef BUFFER_QUEUE_PRINT_STATUS
#include <stdio.h>
#define PRINT_COUNTS(x) PrintCounts(x)
#else
#define PRINT_COUNTS(x)
#endif /* BUFFER_QUEUE_PRINT_STATUS */

/* Data structure containing the decoder reference frame status at the tail
 * of the queue. */
typedef struct DecoderRefStatus_ {
    i32 iPrev;    /* Index to the previous reference frame. */
    i32 iAlt;     /* Index to the alt reference frame. */
    i32 iGolden;  /* Index to the golden reference frame. */
} DecoderRefStatus;

/* Data structure to hold this picture buffer queue instance data. */
typedef struct BufferQueue_t_
{
    pthread_mutex_t cs;         /* Critical section to protect data. */
    pthread_cond_t pending_cv;/* Sync for DecreaseRefCount and WaitPending. */
    pthread_mutex_t pending;    /* Sync for DecreaseRefCount and WaitPending. */
    i32 nBuffers;         /* Number of buffers contained in total. */
    i32* nReferences;     /* Reference counts on buffers. Index is buffer#.  */
    DecoderRefStatus refStatus; /* Reference status of the decoder. */
    fifo_inst emptyFifo;  /* Queue holding empty, unreferred buffer indices. */
} BufferQueue_t;

static void IncreaseRefCount(BufferQueue_t* q, i32 i);
static void DecreaseRefCount(BufferQueue_t* q, i32 i);
#ifdef BUFFER_QUEUE_PRINT_STATUS
static inline void PrintCounts(BufferQueue_t* q);
#endif

BufferQueue VP8HwdBufferQueueInitialize(i32 nBuffers)
{
#ifdef BUFFER_QUEUE_PRINT_STATUS
    printf(__FUNCTION__);
    printf("(nBuffers=%i)", nBuffers);
    printf("\n");
#endif  /* BUFFER_QUEUE_PRINT_STATUS */
    ASSERT(nBuffers > 0);
    i32 i;
    BufferQueue_t* q = (BufferQueue_t*)DWLcalloc(1, sizeof(BufferQueue_t));
    if (q == NULL)
    {
        return NULL;
    }
    q->nReferences = (i32*)DWLcalloc(nBuffers, sizeof(i32));
    if (q->nReferences == NULL ||
        fifo_init(nBuffers, &q->emptyFifo) != FIFO_OK ||
        pthread_mutex_init(&q->cs, NULL) ||
        pthread_mutex_init(&q->pending, NULL) ||
        pthread_cond_init(&q->pending_cv, NULL))
    {
        VP8HwdBufferQueueRelease(q);
        return NULL;
    }
    /* Add picture buffers among empty picture buffers. */
    for (i = 0; i < nBuffers; i++)
    {
        q->nReferences[i] = 0;
        fifo_push(q->emptyFifo, i);
        q->nBuffers++;
    }
    q->refStatus.iPrev = q->refStatus.iGolden = q->refStatus.iAlt =
        REFERENCE_NOT_SET;
    return q;
}

void VP8HwdBufferQueueRelease(BufferQueue queue)
{
#ifdef BUFFER_QUEUE_PRINT_STATUS
    printf(__FUNCTION__);
    printf("()");
    printf("\n");
#endif  /* BUFFER_QUEUE_PRINT_STATUS */
    ASSERT(queue);
    BufferQueue_t* q = (BufferQueue_t*)queue;
    if (q->emptyFifo)
    {   /* Empty the fifo before releasing. */
        i32 i, j;
        for (i = 0; i < q->nBuffers; i++)
            fifo_pop(q->emptyFifo, &j);
        fifo_release(q->emptyFifo);
    }
    pthread_mutex_destroy(&q->cs);
    pthread_cond_destroy(&q->pending_cv);
    pthread_mutex_destroy(&q->pending);
    DWLfree(q->nReferences);
    DWLfree(q);
}

void VP8HwdBufferQueueUpdateRef(BufferQueue queue, u32 refFlags, i32 buffer)
{
#ifdef BUFFER_QUEUE_PRINT_STATUS
    printf(__FUNCTION__);
    printf("(refFlags=0x%X, buffer=%i)", refFlags, buffer);
    printf("\n");
#endif  /* BUFFER_QUEUE_PRINT_STATUS */
    ASSERT(queue);
    BufferQueue_t* q = (BufferQueue_t*)queue;
    ASSERT(buffer >= 0 && buffer < q->nBuffers);
    pthread_mutex_lock(&q->cs);
    /* Check for each type of reference frame update need. */
    if (refFlags & BQUEUE_FLAG_PREV && buffer != q->refStatus.iPrev)
    {
        q->refStatus.iPrev = buffer;
    }
    if (refFlags & BQUEUE_FLAG_GOLDEN && buffer != q->refStatus.iGolden)
    {
        q->refStatus.iGolden = buffer;
    }
    if (refFlags & BQUEUE_FLAG_ALT && buffer != q->refStatus.iAlt)
    {
        q->refStatus.iAlt = buffer;
    }
    PRINT_COUNTS(q);
    pthread_mutex_unlock(&q->cs);
}

i32 VP8HwdBufferQueueGetPrevRef(BufferQueue queue)
{
    BufferQueue_t* q = (BufferQueue_t*)queue;
#ifdef BUFFER_QUEUE_PRINT_STATUS
    printf(__FUNCTION__);
    printf("()");
    printf(" # %d\n", q->refStatus.iPrev);
#endif  /* BUFFER_QUEUE_PRINT_STATUS */
    return q->refStatus.iPrev;
}

i32 VP8HwdBufferQueueGetGoldenRef(BufferQueue queue)
{
    BufferQueue_t* q = (BufferQueue_t*)queue;
#ifdef BUFFER_QUEUE_PRINT_STATUS
    printf(__FUNCTION__);
    printf("()");
    printf(" # %d\n", q->refStatus.iGolden);
#endif  /* BUFFER_QUEUE_PRINT_STATUS */
    return q->refStatus.iGolden;
}

i32 VP8HwdBufferQueueGetAltRef(BufferQueue queue)
{
    BufferQueue_t* q = (BufferQueue_t*)queue;
#ifdef BUFFER_QUEUE_PRINT_STATUS
    printf(__FUNCTION__);
    printf("()");
    printf(" # %d\n", q->refStatus.iAlt);
#endif  /* BUFFER_QUEUE_PRINT_STATUS */
    return q->refStatus.iAlt;
}

void VP8HwdBufferQueueAddRef(BufferQueue queue, i32 buffer)
{
#ifdef BUFFER_QUEUE_PRINT_STATUS
    printf(__FUNCTION__);
    printf("(buffer=%i)", buffer);
    printf("\n");
#endif  /* BUFFER_QUEUE_PRINT_STATUS */
    BufferQueue_t* q = (BufferQueue_t*)queue;
    ASSERT(buffer >= 0 && buffer < q->nBuffers);
    pthread_mutex_lock(&q->cs);
    IncreaseRefCount(q, buffer);
    pthread_mutex_unlock(&q->cs);
}

void VP8HwdBufferQueueRemoveRef(BufferQueue queue,
                                i32 buffer)
{
#ifdef BUFFER_QUEUE_PRINT_STATUS
    printf(__FUNCTION__);
    printf("(buffer=%i)", buffer);
    printf("\n");
#endif  /* BUFFER_QUEUE_PRINT_STATUS */
    BufferQueue_t* q = (BufferQueue_t*)queue;
    ASSERT(buffer >= 0 && buffer < q->nBuffers);
    pthread_mutex_lock(&q->cs);
    DecreaseRefCount(q, buffer);
    pthread_mutex_unlock(&q->cs);
}

i32 VP8HwdBufferQueueGetBuffer(BufferQueue queue)
{
#ifdef BUFFER_QUEUE_PRINT_STATUS
    printf(__FUNCTION__);
    printf("()");
    printf("\n");
#endif  /* BUFFER_QUEUE_PRINT_STATUS */
    i32 i;
    BufferQueue_t* q = (BufferQueue_t*)queue;
    fifo_pop(q->emptyFifo, &i);
    pthread_mutex_lock(&q->cs);
#ifdef BUFFER_QUEUE_PRINT_STATUS
    printf(__FUNCTION__);
    printf("# %i\n", i);
#endif  /* BUFFER_QUEUE_PRINT_STATUS */
    IncreaseRefCount(q, i);
    pthread_mutex_unlock(&q->cs);
    return i;
}

void VP8HwdBufferQueueWaitPending(BufferQueue queue)
{
#ifdef BUFFER_QUEUE_PRINT_STATUS
    printf(__FUNCTION__);
    printf("()");
    printf("\n");
#endif  /* BUFFER_QUEUE_PRINT_STATUS */
    ASSERT(queue);
    BufferQueue_t* q = (BufferQueue_t*)queue;

    pthread_mutex_lock(&q->pending);

    while (fifo_count(q->emptyFifo) != (u32)q->nBuffers)
        pthread_cond_wait(&q->pending_cv, &q->pending);

    pthread_mutex_unlock(&q->pending);

#ifdef BUFFER_QUEUE_PRINT_STATUS
    printf(__FUNCTION__);
    printf("#\n");
#endif  /* BUFFER_QUEUE_PRINT_STATUS */
}

static void IncreaseRefCount(BufferQueue_t* q, i32 i)
{
#ifdef BUFFER_QUEUE_PRINT_STATUS
    printf(__FUNCTION__);
    printf("(buffer=%i)", i);
    printf("\n");
#endif  /* BUFFER_QUEUE_PRINT_STATUS */
    q->nReferences[i]++;
    ASSERT(q->nReferences[i] >= 0);   /* No negative references. */
    PRINT_COUNTS(q);
}

static void DecreaseRefCount(BufferQueue_t* q, i32 i)
{
#ifdef BUFFER_QUEUE_PRINT_STATUS
    printf(__FUNCTION__);
    printf("(buffer=%i)", i);
    printf("\n");
#endif  /* BUFFER_QUEUE_PRINT_STATUS */
    q->nReferences[i]--;
    ASSERT(q->nReferences[i] >= 0);   /* No negative references. */
    PRINT_COUNTS(q);
    if (q->nReferences[i] == 0)
    {   /* Once picture buffer is no longer referred to, it can be put to
           the empty fifo. */
#ifdef BUFFER_QUEUE_PRINT_STATUS
        printf("Buffer #%i put to empty pool\n", i);
        if(i == q->refStatus.iPrev || i == q->refStatus.iGolden || i == q->refStatus.iAlt)
        {
          printf("released but referenced %d\n", i);
        }
#endif  /* BUFFER_QUEUE_PRINT_STATUS */
        fifo_push(q->emptyFifo, i);

        pthread_mutex_lock(&q->pending);
        if (fifo_count(q->emptyFifo) == (u32)q->nBuffers)
            pthread_cond_signal(&q->pending_cv);
        pthread_mutex_unlock(&q->pending);
    }
}

#ifdef BUFFER_QUEUE_PRINT_STATUS
static inline void PrintCounts(BufferQueue_t* q)
{
    i32 i = 0;
    for (i = 0; i < q->nBuffers; i++)
        printf("%u", q->nReferences[i]);
    printf(" |");
    printf(" P: %i |", q->refStatus.iPrev);
    printf(" G: %i |", q->refStatus.iGolden);
    printf(" A: %i |", q->refStatus.iAlt);
    printf("\n");
}
#endif /* BUFFER_QUEUE_PRINT_STATUS */

