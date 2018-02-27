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

#ifndef __VP6DECAPI_H__
#define __VP6DECAPI_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "basetype.h"
#include "decapicommon.h"

/*------------------------------------------------------------------------------
    API type definitions
------------------------------------------------------------------------------*/
    /* Return values */
    typedef enum VP6DecRet_
    {
        VP6DEC_OK = 0,
        VP6DEC_STRM_PROCESSED = 1,
        VP6DEC_PIC_RDY = 2,
        VP6DEC_PIC_DECODED = 3,
        VP6DEC_HDRS_RDY = 4,
        VP6DEC_ADVANCED_TOOLS = 5,

        VP6DEC_PARAM_ERROR = -1,
        VP6DEC_STRM_ERROR = -2,
        VP6DEC_NOT_INITIALIZED = -3,
        VP6DEC_MEMFAIL = -4,
        VP6DEC_INITFAIL = -5,
        VP6DEC_HDRS_NOT_RDY = -6,
        VP6DEC_STREAM_NOT_SUPPORTED = -8,

        VP6DEC_HW_RESERVED = -254,
        VP6DEC_HW_TIMEOUT = -255,
        VP6DEC_HW_BUS_ERROR = -256,
        VP6DEC_SYSTEM_ERROR = -257,
        VP6DEC_DWL_ERROR = -258,

        VP6DEC_EVALUATION_LIMIT_EXCEEDED = -999,
        VP6DEC_FORMAT_NOT_SUPPORTED = -1000
    } VP6DecRet;

    /* decoder  output Frame format */
    typedef enum VP6DecOutFormat_
    {
        VP6DEC_SEMIPLANAR_YUV420 = 0x020001,
        VP6DEC_TILED_YUV420 = 0x020002
    } VP6DecOutFormat;

    /* Input structure */
    typedef struct VP6DecInput_
    {
        const u8 *pStream;   /* Pointer to the input data */
        u32 streamBusAddress;   /* DMA bus address of the input stream */
        u32 dataLen;         /* Number of bytes to be decoded         */
    } VP6DecInput;

    /* Output structure */
    typedef struct VP6DecOutput_
    {
    } VP6DecOutput;

#define VP6_SCALE_MAINTAIN_ASPECT_RATIO     0
#define VP6_SCALE_TO_FIT                    1
#define VP6_SCALE_CENTER                    2
#define VP6_SCALE_OTHER                     3

    /* stream info filled by VP6DecGetInfo */
    typedef struct VP6DecInfo_
    {
        u32 vp6Version;
        u32 vp6Profile;
        u32 frameWidth;      /* coded width */
        u32 frameHeight;     /* coded height */
        u32 scaledWidth;     /* scaled width of the displayed video */
        u32 scaledHeight;    /* scaled height of the displayed video */
        u32 scalingMode;     /* way to scale the frame to output */
        DecDpbMode dpbMode;             /* DPB mode; frame, or field interlaced */              
        VP6DecOutFormat outputFormat;   /* format of the output frame */
    } VP6DecInfo;

    /* Version information */
    typedef struct VP6DecApiVersion_
    {
        u32 major;           /* API major version */
        u32 minor;           /* API minor version */
    } VP6DecApiVersion;

    typedef struct DecSwHwBuild_  VP6DecBuild;

    /* Output structure for VP6DecNextPicture */
    typedef struct VP6DecPicture_
    {
        u32 frameWidth;        /* pixels width of the frame as stored in memory */
        u32 frameHeight;       /* pixel height of the frame as stored in memory */
        const u32 *pOutputFrame;    /* Pointer to the frame */
        u32 outputFrameBusAddress;  /* DMA bus address of the output frame buffer */
        u32 picId;           /* Identifier of the Frame to be displayed */
        u32 picCodingType;   /* Picture coding type */
        u32 isIntraFrame;    /* Indicates if Frame is an Intra Frame */
        u32 isGoldenFrame;   /* Indicates if Frame is a Golden reference Frame */
        u32 nbrOfErrMBs;     /* Number of concealed MB's in the frame  */
        DecOutFrmFormat outputFormat;
    } VP6DecPicture;

    /* Decoder instance */
    typedef const void *VP6DecInst;

/*------------------------------------------------------------------------------
    Prototypes of Decoder API functions
------------------------------------------------------------------------------*/
    VP6DecApiVersion VP6DecGetAPIVersion(void);

    VP6DecBuild VP6DecGetBuild(void);

    VP6DecRet VP6DecInit(VP6DecInst * pDecInst,
                         u32 useVideoFreezeConcealment,
                         u32 numFrameBuffers,
                         DecDpbFlags dpbFlags);

    void VP6DecRelease(VP6DecInst decInst);

    VP6DecRet VP6DecDecode(VP6DecInst decInst,
                           const VP6DecInput * pInput, VP6DecOutput * pOutput);

    VP6DecRet VP6DecNextPicture(VP6DecInst decInst,
                                VP6DecPicture * pOutput, u32 endOfStream);

    VP6DecRet VP6DecGetInfo(VP6DecInst decInst, VP6DecInfo * pDecInfo);

    VP6DecRet VP6DecPeek(VP6DecInst decInst, VP6DecPicture * pOutput);

/*------------------------------------------------------------------------------
    Prototype of the API trace funtion. Traces all API entries and returns.
    This must be implemented by the application using the decoder API!
    Argument:
        string - trace message, a null terminated string
------------------------------------------------------------------------------*/
    void VP6DecTrace(const char *string);

#ifdef __cplusplus
}
#endif

#endif                       /* __VP6DECAPI_H__ */
