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

#ifndef VP8HWD_CONTAINER_H
#define VP8HWD_CONTAINER_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"
#include "deccfg.h"
#include "decppif.h"
#include "dwl.h"
#include "decapicommon.h"
#include "refbuffer.h"

#include "vp8decapi.h"
#include "vp8hwd_decoder.h"
#include "vp8hwd_bool.h"
#include "vp8hwd_error.h"
#include "vp8hwd_buffer_queue.h"


#include "fifo.h"

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/

#define VP8DEC_UNINITIALIZED   0U
#define VP8DEC_INITIALIZED     1U
#define VP8DEC_NEW_HEADERS     3U
#define VP8DEC_DECODING        4U
#define VP8DEC_MIDDLE_OF_PIC   5U

#define VP8DEC_MAX_PIXEL_AMOUNT 16370688
#define VP8DEC_MAX_SLICE_SIZE 4096
#define VP8DEC_MAX_PIC_BUFFERS 16
#define VP8DEC_MAX_CORES MAX_ASIC_CORES

#define VP8_UNDEFINED_BUFFER  VP8DEC_MAX_PIC_BUFFERS

typedef struct HwRdyCallbackArg_t
{
    void *pDecCont;
    u32 coreID;
    u32 showFrame;
    u32 displayNumber;
    BufferQueue* bq;
    i32 index;            /* Buffer index of the output buffer. */
    i32 indexP;
    i32 indexA;
    i32 indexG;
    u8* pRefStatus;       /* Pointer to the reference status field. */
    const u8 *pStream;    /* Input buffer virtual address. */
    void* pUserData;      /* User data associated with input buffer. */
    VP8DecMCStreamConsumed *streamConsumedCallback;
    fifo_inst fifoOut;    /* Output FIFO instance. */
    VP8DecPicture pic;    /* Information needed for outputting the frame. */
} HwRdyCallbackArg;

typedef struct
{
    u32 *pPicBufferY[VP8DEC_MAX_PIC_BUFFERS];
    u32 picBufferBusAddrY[VP8DEC_MAX_PIC_BUFFERS];
    u32 *pPicBufferC[VP8DEC_MAX_PIC_BUFFERS];
    u32 picBufferBusAddrC[VP8DEC_MAX_PIC_BUFFERS];
} userMem_t;

/* asic interface */
typedef struct DecAsicBuffers
{
    u32 width, height;
    u32 stridesUsed, customBuffers;
    u32 lumaStride, chromaStride;
    u32 lumaStridePow2, chromaStridePow2;
    u32 chromaBufOffset;
    u32 syncMcOffset;

    DWLLinearMem_t probTbl[VP8DEC_MAX_CORES];
    DWLLinearMem_t segmentMap[VP8DEC_MAX_CORES];
    DWLLinearMem_t *outBuffer;
    DWLLinearMem_t *prevOutBuffer;
    u32 displayIndex[VP8DEC_MAX_PIC_BUFFERS];

    /* Concurrent access to following picture arrays is controlled indirectly
     * through buffer queue. */
    DWLLinearMem_t pictures[VP8DEC_MAX_PIC_BUFFERS];
    DWLLinearMem_t picturesC[VP8DEC_MAX_PIC_BUFFERS];  /* only for usermem */
    VP8DecPicture pictureInfo[VP8DEC_MAX_PIC_BUFFERS];
    DWLLinearMem_t mvs[2];
    u32 mvsCurr;
    u32 mvsRef;

    /* Indexes for picture buffers in pictures[] array */
    u32 outBufferI;
    u32 prevOutBufferI;

    u32 wholePicConcealed;
    u32 disableOutWriting;
    u32 segmentMapSize;
    u32 partition1Base;
    u32 partition1BitOffset;
    u32 partition2Base;
    i32 dcPred[2];
    i32 dcMatch[2];

    userMem_t userMem;
} DecAsicBuffers_t;

typedef struct VP8DecContainer
{
    const void *checksum;
    u32 decMode;
    u32 decStat;
    u32 picNumber;
    u32 displayNumber;
    u32 asicRunning;
    u32 width;
    u32 height;
    u32 vp8Regs[DEC_X170_REGISTERS];
    DecAsicBuffers_t asicBuff[1];
    const void *dwl;         /* DWL instance */
    i32 coreID;
    i32 segmID;
    u32 numCores;
    u32 refBufSupport;
    refBuffer_t refBufferCtrl;
    vp8Decoder_t decoder;
    vpBoolCoder_t bc;
    u32 combinedModeUsed;

    struct
    {
        const void *ppInstance;
        void (*PPDecStart) (const void *, const DecPpInterface *);
        void (*PPDecWaitEnd) (const void *);
        void (*PPConfigQuery) (const void *, DecPpQuery *);
        DecPpInterface decPpIf;
        DecPpQuery ppInfo;
    } pp;

    u32 pictureBroken;
    u32 intraFreeze;
    u32 outCount;
    u32 refToOut;
    u32 pendingPicToPp;

    BufferQueue bq;
    u32 numBuffers;

    u32 intraOnly;
    u32 sliceConcealment;
    u32 userMem;
    u32 sliceHeight;
    u32 totDecodedRows;
    u32 outputRows;

    u32 tiledModeSupport;
    u32 tiledReferenceEnable;

    u32 hwEcSupport;
    u32 strideSupport;
    u32 conceal;
    u32 concealStartMbX;
    u32 concealStartMbY;
    u32 prevIsKey;
    u32 forceIntraFreeze;
    u32 probRefreshDetected;
    vp8ec_t ec;
    HwRdyCallbackArg hwRdyCallbackArg[MAX_ASIC_CORES];

    /* Output related variables. */
    fifo_inst fifoOut;  /* Fifo for output indices. */
    fifo_inst fifoDisplay;  /* Store of output indices for display reordering. */

    /* Stream buffer callback specifics. */
    VP8DecMCStreamConsumed *streamConsumedCallback;
    void* pUserData;
    const u8* pStream;
} VP8DecContainer_t;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

#endif /* #ifdef VP8HWD_CONTAINER_H */
