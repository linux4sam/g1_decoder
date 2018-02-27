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

#include <stdio.h>
#include <stdlib.h>

#include "dwl.h"
#include "testparams.h"
#include "vp8bufferalloc.h"
#include "vp8decapi.h"
#include "vp8hwd_container.h"

typedef struct useralloc_s
{
    DWLLinearMem_t user_alloc_luma[16];
    DWLLinearMem_t user_alloc_chroma[16];
    VP8DecPictureBufferProperties pbp;
    test_params* params;
} useralloc_t;

useralloc_inst useralloc_open(test_params* params)
{
    useralloc_t* inst = calloc(1, sizeof(useralloc_t));
    if (inst==NULL)
        return NULL;

    if(params==NULL)
        return NULL;

    inst->params = params;

    return inst;
}

void useralloc_close(useralloc_inst inst)
{
    useralloc_t* useralloc = (useralloc_t*)inst;
    if(useralloc)
        free(useralloc);
}



void useralloc_free(useralloc_inst inst,
                    VP8DecInst dec_inst)
{
    useralloc_t* ualloc = (useralloc_t*)inst;
    i32 i;

    for( i = 0 ; i < 16 ; ++i )
    {
        if (ualloc->user_alloc_luma[i].virtualAddress)
        {
            DWLFreeRefFrm(((VP8DecContainer_t *) dec_inst)->dwl,
                          &ualloc->user_alloc_luma[i]);
        }
        if (ualloc->user_alloc_chroma[i].virtualAddress)
        {
            DWLFreeRefFrm(((VP8DecContainer_t *) dec_inst)->dwl,
                          &ualloc->user_alloc_chroma[i]);
        }
    }
}


i32 useralloc_alloc(useralloc_inst inst,
              VP8DecInst dec_inst)
{
    useralloc_t* ualloc = (useralloc_t*)inst;

    u32 sizeLuma;
    u32 sizeChroma;
    u32 widthY, widthC, i;
    VP8DecInfo info;
    VP8DecRet ret;
    VP8DecPictureBufferProperties* pbp = &ualloc->pbp;
    test_params* params = ualloc->params;

    ret = VP8DecGetInfo(dec_inst, &info);
    if (ret != VP8DEC_OK)
    {
        return ret;
    }

    pbp->lumaStride = params->luma_stride_;
    pbp->chromaStride = params->chroma_stride_;
    pbp->numBuffers = params->num_frame_buffers_;

    widthY = pbp->lumaStride ? pbp->lumaStride : info.frameWidth;
    widthC = pbp->chromaStride ? pbp->chromaStride : info.frameWidth;

    useralloc_free(ualloc, dec_inst);

    sizeLuma = info.frameHeight * widthY;
    sizeChroma = info.frameHeight * widthC / 2;

    u32 *pPicBufferY[16];
    u32 *pPicBufferC[16];
    u32 picBufferBusAddressY[16];
    u32 picBufferBusAddressC[16];
    if( pbp->numBuffers < 5 )
        pbp->numBuffers = 5;

    /* TODO(mheikkinen) Alloc space for ref status. */

    /* Custom use case: interleaved buffers (strides must
     * meet strict requirements here). If met, only one or
     * two buffers will be allocated, into which all ref
     * pictures' data will be interleaved into. */
    if(params->interleaved_buffers_)
    {
        u32 sizeBuffer;
        /* Mode 1: luma / chroma strides same; both can be interleaved */
        if( ((pbp->lumaStride == pbp->chromaStride) ||
             ((2*pbp->lumaStride) == pbp->chromaStride)) &&
            pbp->lumaStride >= info.frameWidth*2*pbp->numBuffers)
        {
            sizeBuffer = pbp->lumaStride * (info.frameHeight+1);
            if (DWLMallocRefFrm(((VP8DecContainer_t *) dec_inst)->dwl,
                    sizeBuffer, &ualloc->user_alloc_luma[0]) != DWL_OK)
            {
                fprintf(stderr,"UNABLE TO ALLOCATE PICTURE MEMORY\n");
                return -1;
            }

            for( i = 0 ; i < pbp->numBuffers ; ++i )
            {
                pPicBufferY[i] = ualloc->user_alloc_luma[0].virtualAddress +
                    (info.frameWidth*2*i)/4;
                picBufferBusAddressY[i] = ualloc->user_alloc_luma[0].busAddress +
                    info.frameWidth*2*i;
                pPicBufferC[i] = ualloc->user_alloc_luma[0].virtualAddress +
                    (info.frameWidth*(2*i+1))/4;
                picBufferBusAddressC[i] = ualloc->user_alloc_luma[0].busAddress +
                    info.frameWidth*(2*i+1);
            }

        }
        else /* Mode 2: separate buffers for luma and chroma */
        {
            if( (pbp->lumaStride < info.frameWidth*pbp->numBuffers) ||
                (pbp->chromaStride < info.frameWidth*pbp->numBuffers))
            {
                fprintf(stderr, "CHROMA STRIDE LENGTH TOO SMALL FOR INTERLEAVED FRAME BUFFERS\n");
                return -1;
            }

            sizeBuffer = pbp->lumaStride * (info.frameHeight+1);
            if (DWLMallocRefFrm(((VP8DecContainer_t *) dec_inst)->dwl,
                                sizeBuffer,
                                &ualloc->user_alloc_luma[0]) != DWL_OK)
            {
                fprintf(stderr,"UNABLE TO ALLOCATE PICTURE MEMORY\n");
                return -1;
            }
            sizeBuffer = pbp->chromaStride * (info.frameHeight+1);
            if (DWLMallocRefFrm(((VP8DecContainer_t *) dec_inst)->dwl,
                                sizeBuffer,
                                &ualloc->user_alloc_chroma[0]) != DWL_OK)
            {
                fprintf(stderr,"UNABLE TO ALLOCATE PICTURE MEMORY\n");
                return -1;
            }
            for( i = 0 ; i < pbp->numBuffers ; ++i )
            {
                pPicBufferY[i] = ualloc->user_alloc_luma[0].virtualAddress +
                    (info.frameWidth*i)/4;
                picBufferBusAddressY[i] = ualloc->user_alloc_luma[0].busAddress +
                    info.frameWidth*i;
                pPicBufferC[i] = ualloc->user_alloc_chroma[0].virtualAddress +
                    (info.frameWidth*i)/4;
                picBufferBusAddressC[i] = ualloc->user_alloc_chroma[0].busAddress +
                    info.frameWidth*i;
            }
        }
    }
    else /* dedicated buffers */
    {
        if(params->user_allocated_buffers_ == VP8DEC_EXTERNAL_ALLOC_ALT)
        {
            /* alloc all lumas first and only then chromas */
            for( i = 0 ; i < pbp->numBuffers ; ++i )
            {
                if (DWLMallocRefFrm(((VP8DecContainer_t *) dec_inst)->dwl,
                                    sizeLuma,
                                    &ualloc->user_alloc_luma[i]) != DWL_OK)
                {
                    fprintf(stderr,"UNABLE TO ALLOCATE PICTURE MEMORY\n");
                    return -1;
                }
                pPicBufferY[i] = ualloc->user_alloc_luma[i].virtualAddress;
                picBufferBusAddressY[i] = ualloc->user_alloc_luma[i].busAddress;
            }
            for( i = 0 ; i < pbp->numBuffers ; ++i )
            {
                if (DWLMallocRefFrm(((VP8DecContainer_t *) dec_inst)->dwl,
                                    sizeChroma+16,
                                    &ualloc->user_alloc_chroma[i]) != DWL_OK)
                {
                    fprintf(stderr,"UNABLE TO ALLOCATE PICTURE MEMORY\n");
                    return -1;
                }
                pPicBufferC[i] = ualloc->user_alloc_chroma[i].virtualAddress;
                picBufferBusAddressC[i] = ualloc->user_alloc_chroma[i].busAddress;
            }
        }
        else
        {

            for( i = 0 ; i < pbp->numBuffers ; ++i )
            {
                if (DWLMallocRefFrm(((VP8DecContainer_t *) dec_inst)->dwl,
                        sizeLuma, &ualloc->user_alloc_luma[i]) != DWL_OK)
                {
                    fprintf(stderr,"UNABLE TO ALLOCATE PICTURE MEMORY\n");
                    return -1;
                }
                if (DWLMallocRefFrm(((VP8DecContainer_t *) dec_inst)->dwl,
                        sizeChroma+16, &ualloc->user_alloc_chroma[i]) != DWL_OK)
                {
                    fprintf(stderr,"UNABLE TO ALLOCATE PICTURE MEMORY\n");
                    return -1;
                }
                pPicBufferY[i] = ualloc->user_alloc_luma[i].virtualAddress;
                picBufferBusAddressY[i] = ualloc->user_alloc_luma[i].busAddress;
                pPicBufferC[i] = ualloc->user_alloc_chroma[i].virtualAddress;
                picBufferBusAddressC[i] = ualloc->user_alloc_chroma[i].busAddress;
            }
        }
    }
    pbp->pPicBufferY = pPicBufferY;
    pbp->picBufferBusAddressY = picBufferBusAddressY;
    pbp->pPicBufferC = pPicBufferC;
    pbp->picBufferBusAddressC = picBufferBusAddressC;

    if( VP8DecSetPictureBuffers( dec_inst, pbp ) != VP8DEC_OK )
    {
        fprintf(stderr, "ERROR IN SETUP OF CUSTOM FRAME BUFFERS\n");
        return -1;
    }

    return 0;
}
