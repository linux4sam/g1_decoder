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
#include <string.h>

#include "vp8writeoutput.h"
#include "testparams.h"
#include "md5.h"

typedef struct output_s
{
    u8 *frame_pic_;
    test_params *params;
    FILE *file_;
} output_t;

static void FramePicture( u8 *pIn, u8* pCh, i32 inWidth, i32 inHeight,
                   i32 inFrameWidth, i32 inFrameHeight,
                   u8 *pOut, i32 outWidth, i32 outHeight,
                   u32 lumaStride, u32 chromaStride );

output_inst output_open(char* filename, test_params *params)
{

    output_t* inst = calloc(1, sizeof(output_t));
    if (inst==NULL)
        return NULL;

    if(params==NULL)
        return NULL;

    inst->params = params;

    if(!params->disable_write_)
    {
        inst->file_ = fopen(filename, "wb");
        if (inst->file_ == NULL)
        {
            free(inst);
            return NULL;
        }
    }
    else
    {
        inst->file_ = NULL;
    }
    return inst;

}

void output_close(output_inst inst)
{
    output_t* output = (output_t*)inst;
    if(output->file_ != NULL)
    {
        fclose(output->file_);
    }
    if(output->frame_pic_ != NULL)
    {
        free(output->frame_pic_);
    }

    free(output);
}

i32 output_write_pic(output_inst inst, unsigned char *buffer,
    unsigned char *bufferCh, i32 frameWidth,
    i32 frameHeight, i32 croppedWidth, i32 croppedHeight, i32 planar,
    i32 tiledMode, u32 lumaStride, u32 chromaStride, u32 picNum)
{
    output_t* output = (output_t *)inst;

    int lumaSize = lumaStride * frameHeight;
    int frameSize = lumaSize + (chromaStride*frameHeight/2);
    static pic_number = 0;
    int heightCrop = 0;
    int includeStrides  = 0;
    static struct MD5Context ctx;
    unsigned char digest[16];
    int i = 0;
    unsigned char *cb,*cr;
    int ret;
    unsigned char *outputL = buffer;
    unsigned char *outputCh = bufferCh;
    int writePlanar = planar;
    unsigned int strideLumaLocal = lumaStride;
    unsigned int strideChromaLocal = chromaStride;
    unsigned char *localBuffer = buffer;


    if(output->file_ == NULL)
        return 0;

/* TODO(mheikkinen) TILED format */
/* TODO(mheikkinen) DEC_X170_BIG_ENDIAN */
    if (output->params->num_of_decoded_pics_ <= picNum &&
        output->params->num_of_decoded_pics_)
    {
        return 1;
    }


    if (output->params->frame_picture_)
    {
        if (output->frame_pic_ == NULL)
        {
            output->frame_pic_ =
                (u8*)malloc( frameHeight * frameWidth *3/2 * sizeof(u8));
        }
        FramePicture((u8*)buffer,
                     (u8*)bufferCh,
                     croppedWidth,
                     croppedHeight,
                     frameWidth,
                     frameHeight,
                     output->frame_pic_, frameWidth, frameHeight,
                     output->params->luma_stride_,
                     output->params->chroma_stride_);
       outputL = output->frame_pic_;
       outputCh = NULL;
       writePlanar = 1;
       strideLumaLocal = strideChromaLocal = frameWidth;
       lumaSize = frameWidth * frameHeight;
       frameSize = lumaSize * 3/2;
       localBuffer = output->frame_pic_;
    }

    if (output->params->md5_)
    {
        /* chroma should be right after luma */
        MD5Init(&ctx);
        MD5Update(&ctx, buffer, frameSize);
        MD5Final(digest, &ctx);

        for(i = 0; i < sizeof digest; i++)
        {
           fprintf(output->file_, "%02X", digest[i]);
        }
        fprintf(output->file_, "\n");

        return 0;
    }
    else
    {
        if (outputCh == NULL)
        {
            outputCh = outputL + lumaSize;
        }

        if (!heightCrop || (croppedHeight == frameHeight && croppedWidth == frameWidth))
        {
            u32 i, j;
            u8 *bufferTmp;
            bufferTmp = localBuffer;

            for( i = 0 ; i < frameHeight ; ++i )
            {
                fwrite( bufferTmp, includeStrides ? strideLumaLocal : frameWidth, 1, output->file_);
                bufferTmp += strideLumaLocal;
            }

            if (!writePlanar)
            {

                bufferTmp = outputCh;
                for( i = 0 ; i < frameHeight / 2 ; ++i )
                {
                    fwrite( bufferTmp, includeStrides ? strideChromaLocal : frameWidth, 1, output->file_);
                    bufferTmp += strideChromaLocal;
                }
            }
            else
            {
                bufferTmp = outputCh;
                for(i = 0; i < frameHeight / 2; i++)
                {
                    for( j = 0 ; j < (includeStrides ? strideChromaLocal / 2 : frameWidth / 2); ++j)
                    {
                        fwrite(bufferTmp + j * 2, 1, 1, output->file_);
                    }
                    bufferTmp += strideChromaLocal;
                }
                bufferTmp = outputCh + 1;
                for(i = 0; i < frameHeight / 2; i++)
                {
                    for( j = 0 ; j < (includeStrides ? strideChromaLocal / 2: frameWidth / 2); ++j)
                    {
                        fwrite(bufferTmp + j * 2, 1, 1, output->file_);
                    }
                    bufferTmp += strideChromaLocal;
                }
            }
        }
        else
        {
            u32 row;
            for( row = 0 ; row < croppedHeight ; row++)
            {
                fwrite(localBuffer + row*strideLumaLocal, croppedWidth, 1, output->file_);
            }
            if (!writePlanar)
            {
                if(croppedHeight &1)
                    croppedHeight++;
                if(croppedWidth & 1)
                    croppedWidth++;
                for( row = 0 ; row < croppedHeight/2 ; row++)
                    fwrite(outputCh + row*strideChromaLocal, (croppedWidth*2)/2, 1, output->file_);
            }
            else
            {
                u32 i, tmp;
                tmp = frameWidth*croppedHeight/4;

                if(croppedHeight &1)
                    croppedHeight++;
                if(croppedWidth & 1)
                    croppedWidth++;

                for( row = 0 ; row < croppedHeight/2 ; ++row )
                {
                    for(i = 0; i < croppedWidth/2; i++)
                        fwrite(outputCh + row*strideChromaLocal + i * 2, 1, 1, output->file_);
                }
                for( row = 0 ; row < croppedHeight/2 ; ++row )
                {
                    for(i = 0; i < croppedWidth/2; i++)
                        fwrite(outputCh + 1 + row*strideChromaLocal + i * 2, 1, 1, output->file_);
                }
            }
        }
    }

    return 0;

}

static void FramePicture( u8 *pIn, u8* pCh, i32 inWidth, i32 inHeight,
                   i32 inFrameWidth, i32 inFrameHeight,
                   u8 *pOut, i32 outWidth, i32 outHeight,
                   u32 lumaStride, u32 chromaStride )
{

/* Variables */

    i32 x, y;

/* Code */
    memset( pOut, 0, outWidth*outHeight );
    memset( pOut+outWidth*outHeight, 128, outWidth*outHeight/2 );

    /* Luma */
    for ( y = 0 ; y < inHeight ; ++y )
    {
        for( x = 0 ; x < inWidth; ++x )
            *pOut++ = *pIn++;
        pIn += ( lumaStride - inWidth );
        pOut += ( outWidth - inWidth );
    }

    pOut += outWidth * ( outHeight - inHeight );
    if(pCh)
        pIn = pCh;
    else
        pIn += lumaStride * ( inFrameHeight - inHeight );

    inFrameHeight /= 2;
    inFrameWidth /= 2;
    outHeight /= 2;
    outWidth /= 2;
    inHeight /= 2;
    inWidth /= 2;
    /* Chroma */
    for ( y = 0 ; y < inHeight ; ++y )
    {
        for( x = 0 ; x < 2*inWidth; ++x )
            *pOut++ = *pIn++;
        pIn += 2 * ( (chromaStride/2) - inWidth );
        pOut += 2 * ( outWidth - inWidth );
    }
}
