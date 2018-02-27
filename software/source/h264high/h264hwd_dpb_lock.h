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

#ifndef H264HIGH_H264HWD_DPB_LOCK_H_
#define H264HIGH_H264HWD_DPB_LOCK_H_

#include "basetype.h"
#include "h264decapi.h"

#include <pthread.h>
#include <semaphore.h>

#define MAX_FRAME_BUFFER_NUMBER     34
#define FB_NOT_VALID_ID             ~0U

#define FB_HW_OUT_FIELD_TOP         0x10U
#define FB_HW_OUT_FIELD_BOT         0x20U
#define FB_HW_OUT_FRAME             (FB_HW_OUT_FIELD_TOP | FB_HW_OUT_FIELD_BOT)

typedef struct FrameBufferStatus_
{
    u32 nRefCount;
    u32 bUsed;
    const void * data;
} FrameBufferStatus;

typedef struct OutElement_
{
    u32 memIdx;
    H264DecPicture pic;
} OutElement;

typedef struct FrameBufferList_
{
    int bInitialized;
    struct FrameBufferStatus_ fbStat[MAX_FRAME_BUFFER_NUMBER];
    struct OutElement_ outFifo[MAX_FRAME_BUFFER_NUMBER];
    int wrId;
    int rdId;
    int freeBuffers;
    int numOut;

    struct
    {
        int id;
        const void * desc;
    } lastOut;

    sem_t out_count_sem;
    pthread_mutex_t out_count_mutex;
    pthread_cond_t out_empty_cv;
    pthread_mutex_t ref_count_mutex;
    pthread_cond_t ref_count_cv;
    pthread_cond_t hw_rdy_cv;
} FrameBufferList;

struct dpbStorage;
struct H264DecPicture_;

u32 InitList(FrameBufferList *fbList);
void ReleaseList(FrameBufferList *fbList);

u32 AllocateIdUsed(FrameBufferList *fbList, const void * data);
u32 AllocateIdFree(FrameBufferList *fbList, const void * data);
void ReleaseId(FrameBufferList *fbList, u32 id);
void * GetDataById(FrameBufferList *fbList, u32 id);
u32 GetIdByData(FrameBufferList *fbList, const void *data);

void IncrementRefUsage(FrameBufferList *fbList, u32 id);
void DecrementRefUsage(FrameBufferList *fbList, u32 id);

void IncrementDPBRefCount(struct dpbStorage *dpb);
void DecrementDPBRefCount(struct dpbStorage *dpb);

void MarkHWOutput(FrameBufferList *fbList, u32 id, u32 type);
void ClearHWOutput(FrameBufferList *fbList, u32 id, u32 type);

void MarkTempOutput(FrameBufferList *fbList, u32 id);
void ClearOutput(FrameBufferList *fbList, u32 id);

void FinalizeOutputAll(FrameBufferList *fbList);
void RemoveTempOutputAll(FrameBufferList *fbList);

u32 GetFreePicBuffer(FrameBufferList * fbList, u32 old_id);
void SetFreePicBuffer(FrameBufferList * fbList, u32 id);
u32 GetFreeBufferCount(FrameBufferList *fbList);

void PushOutputPic(FrameBufferList *fbList, const struct H264DecPicture_ *pic,
        u32 id);
u32 PeekOutputPic(FrameBufferList *fbList, struct H264DecPicture_ *pic);
u32 PopOutputPic(FrameBufferList *fbList, const struct H264DecPicture_ *pic);

void MarkOutputPicCorrupt(FrameBufferList *fbList, u32 id, u32 errors);

u32 IsBufferReferenced(FrameBufferList *fbList, u32 id);
u32 IsOutputEmpty(FrameBufferList *fbList);
void WaitOutputEmpty(FrameBufferList *fbList);
void WaitListNotInUse(FrameBufferList *fbList);

#endif  /*  H264HIGH_H264HWD_DPB_LOCK_H_ */
