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

#include "bqueue.h"
#include "dwl.h"
#ifndef HANTRO_OK
    #define HANTRO_OK   (0)
#endif /* HANTRO_TRUE */

#ifndef HANTRO_NOK
    #define HANTRO_NOK  (1)
#endif /* HANTRO_FALSE*/

/*------------------------------------------------------------------------------
    2. External identifiers
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Module indentifiers
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    BqueueInit
        Initialize buffer queue
------------------------------------------------------------------------------*/
u32 BqueueInit( bufferQueue_t *bq, u32 numBuffers )
{
    u32 i;

    if(DWLmemset(bq, 0, sizeof(*bq)) != bq)
        return HANTRO_NOK;

    if( numBuffers == 0 )
        return HANTRO_OK;

    bq->picI = (u32*)DWLmalloc( sizeof(u32)*numBuffers);
    if( bq->picI == NULL )
    {
        return HANTRO_NOK;
    }
    for( i = 0 ; i < numBuffers ; ++i )
    {
        bq->picI[i] = 0;
    }
    bq->queueSize = numBuffers;
    bq->ctr = 1;

    return HANTRO_OK;
}

/*------------------------------------------------------------------------------
    BqueueRelease
------------------------------------------------------------------------------*/
void BqueueRelease( bufferQueue_t *bq )
{
    if(bq->picI)
    {
        DWLfree(bq->picI);
        bq->picI = NULL;
    }
    bq->prevAnchorSlot  = 0;
    bq->queueSize       = 0;
}

/*------------------------------------------------------------------------------
    BqueueNext
        Return "oldest" available buffer.
------------------------------------------------------------------------------*/
u32 BqueueNext( bufferQueue_t *bq, u32 ref0, u32 ref1, u32 ref2, u32 bPic )
{
    u32 minPicI = 1<<30;
    u32 nextOut = (u32)0xFFFFFFFFU;
    u32 i;
    /* Find available buffer with smallest index number  */
    i = 0;

    while( i < bq->queueSize )
    {
        if(i == ref0 || i == ref1 || i == ref2) /* Skip reserved anchor pictures */
        {
            i++;
            continue;
        }
        if( bq->picI[i] < minPicI )
        {
            minPicI = bq->picI[i];
            nextOut = i;
        }
        i++;
    }

    if( nextOut == (u32)0xFFFFFFFFU)
    {
        return 0; /* No buffers available, shouldn't happen */
    }

    /* Update queue state */
    if( bPic )
    {
        bq->picI[nextOut] = bq->ctr-1;
        bq->picI[bq->prevAnchorSlot]++;
    }
    else
    {
        bq->picI[nextOut] = bq->ctr;
    }
    bq->ctr++;
    if( !bPic )
    {
        bq->prevAnchorSlot = nextOut;
    }

    return nextOut;
}

/*------------------------------------------------------------------------------
    BqueueDiscard
        "Discard" output buffer, e.g. if error concealment used and buffer
        at issue is never going out.
------------------------------------------------------------------------------*/
void BqueueDiscard( bufferQueue_t *bq, u32 buffer )
{
    bq->picI[buffer] = 0;
}
