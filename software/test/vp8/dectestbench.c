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

#include "basetype.h"
#include "ivf.h"

#include "vp8decapi.h"
#include "dwl.h"
#include "trace.h"

#include "vp8hwd_container.h"

#include "vp8filereader.h"
#ifdef USE_EFENCE
#include "efence.h"
#endif

#include "regdrv.h"
#include "tb_cfg.h"
#include "tb_tiled.h"

#ifdef PP_PIPELINE_ENABLED
#include "pptestbench.h"
#include "ppapi.h"
#endif

#include "md5.h"
#include "tb_md5.h"

#include "tb_sw_performance.h"

#ifdef ASIC_TRACE_SUPPORT
#include "trace.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "vp8filereader.h"

TBCfg tbCfg;
/* for tracing */
u32 bFrames=0;
#ifdef ASIC_TRACE_SUPPORT
extern u32 hwDecPicCount;
extern u32 gHwVer;
#endif

#ifdef VP8_EVALUATION
extern u32 gHwVer;
#endif

u32 enableFramePicture = 0;
u32 numberOfWrittenFrames = 0;
u32 numFrameBuffers = 0;

/* SW/SW testing, read stream trace file */
FILE * fStreamTrace = NULL;

u32 disableOutputWriting = 0;
u32 displayOrder = 0;
u32 clock_gating = DEC_X170_INTERNAL_CLOCK_GATING;
u32 data_discard = DEC_X170_DATA_DISCARD_ENABLE;
u32 latency_comp = DEC_X170_LATENCY_COMPENSATION;
u32 output_picture_endian = DEC_X170_OUTPUT_PICTURE_ENDIAN;
u32 bus_burst_length = DEC_X170_BUS_BURST_LENGTH;
u32 asic_service_priority = DEC_X170_ASIC_SERVICE_PRIORITY;
u32 output_format = DEC_X170_OUTPUT_FORMAT;
u32 service_merge_disable = DEC_X170_SERVICE_MERGE_DISABLE;
u32 planarOutput = 0;
u32 heightCrop = 0;
u32 tiledOutput = DEC_REF_FRM_RASTER_SCAN;
u32 convertTiledOutput = 0;

u32 usePeekOutput = 0;
u32 snapShot = 0;
u32 forcedSliceMode = 0;

u32 streamPacketLoss = 0;
u32 streamTruncate = 0;
u32 streamHeaderCorrupt = 0;
u32 streamBitSwap = 0;
u32 hdrsRdy = 0;
u32 picRdy = 0;

u32 framePicture = 0;
u32 outFrameWidth = 0;
u32 outFrameHeight = 0;
u8 *pFramePic = NULL;
u32 includeStrides = 0;
i32 extraStrmBufferBits = 0; /* For HW bug workaround */

u32 userMemAlloc = 0;
u32 alternateAllocOrder = 0;
u32 interleavedUserBuffers = 0;
DWLLinearMem_t userAllocLuma[16];
DWLLinearMem_t userAllocChroma[16];
VP8DecPictureBufferProperties pbp = { 0 };

char *pic_big_endian = NULL;
size_t pic_big_endian_size = 0;

void decsw_performance(void);

void writeRawFrame(FILE * fp, unsigned char *buffer, 
    unsigned char *bufferC, int frameWidth,
    int frameHeight, int cropWidth, int cropHeight, int planar,
    int tiledMode, u32 lumaStride, u32 chromaStride, u32 md5sum );

void writeSlice(FILE * fp, VP8DecPicture *decPic);

void VP8DecTrace(const char *string){
    printf("%s\n", string);
}

void FramePicture( u8 *pIn, u8 *pCh, i32 inWidth, i32 inHeight,
                   i32 inFrameWidth, i32 inFrameHeight,
                   u8 *pOut, i32 outWidth, i32 outHeight,
                   u32 lumaStride, u32 chromaStride );

#ifdef PP_PIPELINE_ENABLED
void HandlePpOutput(u32 picNum, VP8DecInst decoder);
#endif
/*------------------------------------------------------------------------------
    Module defines
------------------------------------------------------------------------------*/

#define DEBUG_PRINT(str) printf str

#define VP8_MAX_STREAM_SIZE  DEC_X170_MAX_STREAM>>1

#ifdef ADS_PERFORMANCE_SIMULATION
includeStrides
volatile u32 tttttt = 0;

void trace_perf()
{
    tttttt++;
}

#undef START_SW_PERFORMANCE
#undef END_SW_PERFORMANCE

#define START_SW_PERFORMANCE trace_perf();
#define END_SW_PERFORMANCE trace_perf();

#endif

/*------------------------------------------------------------------------------
    Local functions
------------------------------------------------------------------------------*/
static u32 FfReadFrame( reader_inst reader, const u8 *buffer,
                        u32 maxBufferSize, u32 *frameSize, u32 pedantic );

/*------------------------------------------------------------------------------

    main

        Main function

------------------------------------------------------------------------------*/
int main(int argc, char**argv)
{
    FILE *fout;
    reader_inst reader;
    u32 i, tmp;
    u32 size;
    u32 moreFrames;
    u32 pedantic_reading = 1;
    u32 maxNumPics = 0;
    u32 picDecodeNumber = 0;
    u32 picDisplayNumber = 0;
    DWLLinearMem_t streamMem;
    DWLHwConfig_t hwConfig;
    VP8DecInst decInst;
    VP8DecRet ret;
    VP8DecRet tmpret;
    VP8DecFormat decFormat;
    VP8DecInput decInput;
    VP8DecOutput decOutput;
    VP8DecPicture decPicture;
    VP8DecInfo info;
    u32 webpLoop = 0;
    u32 md5sum = 0; /* flag to enable md5sum output */

    FILE *fTBCfg;
    u32 seedRnd = 0;
    char outFileName[256] = "out.yuv";

#ifdef ASIC_TRACE_SUPPORT
    gHwVer = 10000; /* default to g1 mode */
#endif 

#ifdef VP8_EVALUATION_G1
    gHwVer = 10000;
#endif 

#ifndef EXPIRY_DATE
#define EXPIRY_DATE (u32)0xFFFFFFFF
#endif /* EXPIRY_DATE */

    /* expiry stuff */
    {
        u8 tmBuf[7];
        time_t sysTime;
        struct tm * tm;
        u32 tmp1;

        /* Check expiry date */
        time(&sysTime);
        tm = localtime(&sysTime);
        strftime(tmBuf, sizeof(tmBuf), "%y%m%d", tm);
        tmp1 = 1000000+atoi(tmBuf);
        if (tmp1 > (EXPIRY_DATE) && (EXPIRY_DATE) > 1 )
        {
            fprintf(stderr,
                "EVALUATION PERIOD EXPIRED.\n"
                "Please contact On2 Sales.\n");
            return -1;
        }
    }

    streamMem.virtualAddress = NULL;
    streamMem.busAddress = 0;
    decPicture.numSliceRows = 0;

#ifndef PP_PIPELINE_ENABLED
    if (argc < 2)
    {
        printf("Usage: %s [options] file.ivf\n", argv[0]);
        printf("\t-Nn forces decoding to stop after n pictures\n");
        printf("\t-Ooutfile write output to \"outfile\" (default out.yuv)\n");
        printf("\t--md5 Output frame based md5 checksum. No YUV output!\n");
        printf("\t-C display cropped image\n");
        printf("\t-P write planar output.\n");
        printf("\t-E use tiled reference frame format.\n");
        printf("\t-G convert tiled output pictures to raster scan\n");
        printf("\t-F Enable frame picture writing (filled black).\n");        
        printf("\t-W Set frame picture width (default 1. frame width).\n");        
        printf("\t-H Set frame picture height (default 1. frame height).\n");        
        printf("\t-Bn to use n frame buffers in decoder\n");
        printf("\t-Z output pictures using VP8DecPeek() function\n");
        printf("\t-ln Set luma buffer stride\n");
        printf("\t-cn Set chroma buffer stride\n");
        printf("\t-X user allocates picture buffers\n");
        printf("\t-Xa same as above but alternate allocation order\n");
        printf("\t-I use interleaved frame buffers (requires stride mode and "\
                "user allocated buffers\n");
        printf("\t-R write uncropped output (if strides used)\n");
        printf("\t-xn Add n bytes of extra space after "\
            "stream buffer for decoder\n");        
        return 0;
    }

    /* read command line arguments */
    for (i = 1; i < (u32)(argc-1); i++)
    {
        if (strncmp(argv[i], "-N", 2) == 0)
            maxNumPics = (u32)atoi(argv[i] + 2);
        else if (strncmp(argv[i], "-P", 2) == 0)
            planarOutput = 1;
        else if (strncmp(argv[i], "-E", 2) == 0)
            tiledOutput = DEC_REF_FRM_TILED_DEFAULT;
        else if (strncmp(argv[i], "-G", 2) == 0)
            convertTiledOutput = 1;        
        else if (strncmp(argv[i], "-C", 2) == 0)
            heightCrop = 1;
        else if (strncmp(argv[i], "-F", 2) == 0)
            framePicture = 1;
        else if (strncmp(argv[i], "-W", 2) == 0)
            outFrameWidth = (u32)atoi(argv[i] + 2);
        else if (strncmp(argv[i], "-H", 2) == 0)
            outFrameHeight = (u32)atoi(argv[i] + 2);
        else if (strncmp(argv[i], "-O", 2) == 0)
            strcpy(outFileName, argv[i] + 2);
        else if(strncmp(argv[i], "-B", 2) == 0)
        {
            numFrameBuffers = atoi(argv[i] + 2);
            if(numFrameBuffers > 16)
                numFrameBuffers = 16;
        }
        else if (strncmp(argv[i], "-Z", 2) == 0)
            usePeekOutput = 1;
        else if (strncmp(argv[i], "-S", 2) == 0)
            snapShot = 1;
        else if (strncmp(argv[i], "-x", 2) == 0)
            extraStrmBufferBits = atoi(argv[i] + 2);
        else if (strncmp(argv[i], "-l", 2) == 0)
        {
            pbp.lumaStride = (u32)atoi(argv[i] + 2);
        }
        else if (strncmp(argv[i], "-c", 2) == 0)
        {
            pbp.chromaStride = (u32)atoi(argv[i] + 2);
        }
        else if (strncmp(argv[i], "-X", 2) == 0)
        {
            userMemAlloc = 1;
            if( strncmp(argv[i], "-Xa", 3) == 0)
                alternateAllocOrder = 1;
        }
        else if (strncmp(argv[i], "-I", 2) == 0)
            interleavedUserBuffers = 1;
        else if (strncmp(argv[i], "-R", 2) == 0)
            includeStrides = 1;
        else if(strcmp(argv[i], "--md5") == 0)
        {
            md5sum = 1;
        }
        else
        {
            DEBUG_PRINT(("UNKNOWN PARAMETER: %s\n", argv[i]));
            return 1;
        }
    }

/* For stride support always force framePicture on; TODO check compatibility
 * issues */
    if(md5sum && (pbp.chromaStride || pbp.lumaStride))
    {
        framePicture = 1;
    }

    reader = rdr_open(argv[argc-1]);
    if (reader == NULL)
    {
        fprintf(stderr, "Unable to open input file\n");
        exit(100);
    }

    fout = fopen(outFileName, "wb");
    if (fout == NULL)
    {
        fprintf(stderr, "Unable to open output file\n");
        exit(100);
    }
#else
    if (argc < 3)
    {
        printf("Usage: %s [options] file.ivf\n", argv[0]);
        printf("\t-Bn to use n frame buffers in decoder\n");
        printf("\t-E use tiled reference frame format.\n");
        printf("\t-Nn forces decoding to stop after n pictures\n");
        printf("\t-X user allocates picture buffers\n");
        printf("\t-Xa same as above but alternate allocation order\n");       
        printf("\t-I use interleaved frame buffers (requires stride mode and "\
                "user allocated buffers\n");       
        printf("\t-xn Add n bytes of extra space after "\
            "stream buffer for decoder\n");
        return 0;
    }

    /* read command line arguments */
    for (i = 1; i < (u32)(argc-1); i++)
    {
        if (strncmp(argv[i], "-N", 2) == 0)
            maxNumPics = (u32)atoi(argv[i] + 2);
        else if (strncmp(argv[i], "-E", 2) == 0)
            tiledOutput = DEC_REF_FRM_TILED_DEFAULT;
        else if (strncmp(argv[i], "-x", 2) == 0)
            extraStrmBufferBits = atoi(argv[i] + 2);
        else if(strncmp(argv[i], "-B", 2) == 0)
        {
            numFrameBuffers = atoi(argv[i] + 2);
            if(numFrameBuffers > 16)
                numFrameBuffers = 16;
        }
        else if (strncmp(argv[i], "-X", 2) == 0)
        {
            userMemAlloc = 1;
            if( strncmp(argv[i], "-Xa", 3) == 0)
                alternateAllocOrder = 1;
        }
    }

    reader = rdr_open(argv[argc-2]);
    if (reader == NULL)
    {
        fprintf(stderr, "Unable to open input file\n");
        exit(100);
    }
#endif

#ifdef ASIC_TRACE_SUPPORT
    tmp = openTraceFiles();
    if (!tmp)
    {
        DEBUG_PRINT(("UNABLE TO OPEN TRACE FILE(S)\n"));
    }
#endif

    /* set test bench configuration */
    TBSetDefaultCfg(&tbCfg);
    fTBCfg = fopen("tb.cfg", "r");
    if (fTBCfg == NULL)
    {
        DEBUG_PRINT(("UNABLE TO OPEN TEST BENCH CONFIGURATION FILE: \"tb.cfg\"\n"));
        DEBUG_PRINT(("USING DEFAULT CONFIGURATION\n"));
    }
    else
    {
        fclose(fTBCfg);
        if (TBParseConfig("tb.cfg", TBReadParam, &tbCfg) == TB_FALSE)
            return -1;
        if (TBCheckCfg(&tbCfg) != 0)
            return -1;
    }

    userMemAlloc |= TBGetDecMemoryAllocation(&tbCfg);
    clock_gating = TBGetDecClockGating(&tbCfg);
    data_discard = TBGetDecDataDiscard(&tbCfg);
    latency_comp = tbCfg.decParams.latencyCompensation;
    output_picture_endian = TBGetDecOutputPictureEndian(&tbCfg);
    bus_burst_length = tbCfg.decParams.busBurstLength;
    asic_service_priority = tbCfg.decParams.asicServicePriority;
    output_format = TBGetDecOutputFormat(&tbCfg);
    service_merge_disable = TBGetDecServiceMergeDisable(&tbCfg);

    DEBUG_PRINT(("Decoder Clock Gating %d\n", clock_gating));
    DEBUG_PRINT(("Decoder Data Discard %d\n", data_discard));
    DEBUG_PRINT(("Decoder Latency Compensation %d\n", latency_comp));
    DEBUG_PRINT(("Decoder Output Picture Endian %d\n", output_picture_endian));
    DEBUG_PRINT(("Decoder Bus Burst Length %d\n", bus_burst_length));
    DEBUG_PRINT(("Decoder Asic Service Priority %d\n", asic_service_priority));
    DEBUG_PRINT(("Decoder Output Format %d\n", output_format));
    seedRnd = tbCfg.tbParams.seedRnd;
    streamHeaderCorrupt = TBGetTBStreamHeaderCorrupt(&tbCfg);
    /* if headers are to be corrupted 
      -> do not wait the picture to readeralize before starting stream corruption */
    if (streamHeaderCorrupt)
        picRdy = 1;
    streamTruncate = TBGetTBStreamTruncate(&tbCfg);
    if (strcmp(tbCfg.tbParams.streamBitSwap, "0") != 0)
    {
        streamBitSwap = 1;
    }
    else
    {
        streamBitSwap = 0;
    }
    if (strcmp(tbCfg.tbParams.streamPacketLoss, "0") != 0)
    {
        streamPacketLoss = 1;
    }
    else
    {
        streamPacketLoss = 0;
    }
    DEBUG_PRINT(("TB Seed Rnd %d\n", seedRnd));
    DEBUG_PRINT(("TB Stream Truncate %d\n", streamTruncate));
    DEBUG_PRINT(("TB Stream Header Corrupt %d\n", streamHeaderCorrupt));
    DEBUG_PRINT(("TB Stream Bit Swap %d; odds %s\n", 
                streamBitSwap, tbCfg.tbParams.streamBitSwap));
    DEBUG_PRINT(("TB Stream Packet Loss %d; odds %s\n",
            streamPacketLoss, tbCfg.tbParams.streamPacketLoss));

    INIT_SW_PERFORMANCE;

    {
        VP8DecApiVersion decApi;
        VP8DecBuild decBuild;

        /* Print API version number */
        decApi = VP8DecGetAPIVersion();
        decBuild = VP8DecGetBuild();
        DWLReadAsicConfig(&hwConfig);
        DEBUG_PRINT((
            "\n8170 VP8 Decoder API v%d.%d - SW build: %d - HW build: %x\n",
            decApi.major, decApi.minor, decBuild.swBuild, decBuild.hwBuild));
        DEBUG_PRINT((
             "HW Supports video decoding up to %d pixels,\n",
             hwConfig.maxDecPicWidth));

        DEBUG_PRINT((
             "supported codecs: %s%s\n", 
                                        hwConfig.vp7Support ? "VP-7 " : "",
                                        hwConfig.vp8Support ? "VP-8" : ""));

        if(hwConfig.ppSupport)
            DEBUG_PRINT((
             "Maximum Post-processor output size %d pixels\n\n",
             hwConfig.maxPpOutPicWidth));
        else
            DEBUG_PRINT(("Post-Processor not supported\n\n"));
    }

    /* check format */
    switch (rdr_identify_format(reader))
    {
        case BITSTREAM_VP7:
            decFormat = VP8DEC_VP7;
            break;
        case BITSTREAM_VP8:
            decFormat = VP8DEC_VP8;
            break;
        case BITSTREAM_WEBP:
            decFormat = VP8DEC_WEBP;
            pedantic_reading = 0;  /* With WebP we rely on non-pedantic reading
                                      mode for correct operation. */
            break;
    }
    if (snapShot)
        decFormat = VP8DEC_WEBP;
    if (decFormat == VP8DEC_WEBP)
        snapShot = 1;

    /* initialize decoder. If unsuccessful -> exit */
    decsw_performance();
    START_SW_PERFORMANCE;
    ret = VP8DecInit(&decInst, decFormat, TBGetDecIntraFreezeEnable( &tbCfg ),
        numFrameBuffers, tiledOutput );
    END_SW_PERFORMANCE;
    decsw_performance();

    if (ret != VP8DEC_OK)
    {
        fprintf(stderr,"DECODER INITIALIZATION FAILED\n");
        goto end;
    }

    /* Set ref buffer test mode */
    ((VP8DecContainer_t *) decInst)->refBufferCtrl.testFunction = TBRefbuTestMode;

    TBSetRefbuMemModel( &tbCfg, 
        ((VP8DecContainer_t *) decInst)->vp8Regs,
        &((VP8DecContainer_t *) decInst)->refBufferCtrl );

#ifdef PP_PIPELINE_ENABLED
    /* Initialize the post processor. If unsuccessful -> exit */

    if(pp_startup
        (argv[argc - 1], decInst,
         snapShot ?  PP_PIPELINED_DEC_TYPE_WEBP : PP_PIPELINED_DEC_TYPE_VP8,
         &tbCfg) != 0)
    {
        fprintf(stderr, "PP INITIALIZATION FAILED\n");
        goto end;
    }
    if(pp_update_config
       (decInst,
         snapShot ? PP_PIPELINED_DEC_TYPE_WEBP : PP_PIPELINED_DEC_TYPE_VP8,
        &tbCfg) == CFG_UPDATE_FAIL)

    {
        fprintf(stdout, "PP CONFIG LOAD FAILED\n");
    }

    /* If unspecified at cmd line, use minimum # of buffers, otherwise
     * use specified amount. */
    if(numFrameBuffers == 0)
        pp_number_of_buffers(1);
    else
        pp_number_of_buffers(numFrameBuffers);
#endif

    SetDecRegister(((VP8DecContainer_t *) decInst)->vp8Regs, HWIF_DEC_LATENCY,
        latency_comp);
    SetDecRegister(((VP8DecContainer_t *) decInst)->vp8Regs, HWIF_DEC_CLK_GATE_E,
        clock_gating);
    SetDecRegister(((VP8DecContainer_t *) decInst)->vp8Regs, HWIF_DEC_OUT_TILED_E,
        output_format);
    SetDecRegister(((VP8DecContainer_t *) decInst)->vp8Regs, HWIF_DEC_OUT_ENDIAN,
        output_picture_endian);
    SetDecRegister(((VP8DecContainer_t *) decInst)->vp8Regs, HWIF_DEC_MAX_BURST,
        bus_burst_length);
    SetDecRegister(((VP8DecContainer_t *) decInst)->vp8Regs, HWIF_DEC_DATA_DISC_E,
        data_discard);
    SetDecRegister(((VP8DecContainer_t *) decInst)->vp8Regs, HWIF_SERV_MERGE_DIS,
                   service_merge_disable);

    TBInitializeRandom(seedRnd);

    /* Allocate stream memory */
    if(DWLMallocLinear(((VP8DecContainer_t *) decInst)->dwl,
            VP8_MAX_STREAM_SIZE,
            &streamMem) != DWL_OK ){
        fprintf(stderr,"UNABLE TO ALLOCATE STREAM BUFFER MEMORY\n");
        return -1;
    }

    DWLmemset(&decInput, 0, sizeof(decInput));

    decInput.sliceHeight = tbCfg.decParams.jpegMcusSlice;

    /* slice mode disabled if dec+pp */
#ifdef PP_PIPELINE_ENABLED
    decInput.sliceHeight = 0;
#endif
    /* No slice mode for VP8 video */
/*    if(!snapShot)
        decInput.sliceHeight = 0;*/

    /* Start decode loop */
    do
    {
        /* read next frame from file format */
        if (!webpLoop)
        {
            tmp = FfReadFrame( reader, (u8*)streamMem.virtualAddress,
                               VP8_MAX_STREAM_SIZE, &size, pedantic_reading );
            /* ugly hack for large webp streams. drop reserved stream buffer
             * and reallocate new with correct stream size. */
            if( tmp == HANTRO_NOK &&
                decFormat == VP8DEC_WEBP &&
                size + extraStrmBufferBits > VP8_MAX_STREAM_SIZE )
            {
                DWLFreeLinear(((VP8DecContainer_t *) decInst)->dwl, &streamMem);
                /* Allocate MORE stream memory */
                if(DWLMallocLinear(((VP8DecContainer_t *) decInst)->dwl,
                        size + extraStrmBufferBits,
                        &streamMem) != DWL_OK ){
                    fprintf(stderr,"UNABLE TO ALLOCATE STREAM BUFFER MEMORY\n");
                    return -1;
                }
                /* try again */
                tmp = FfReadFrame( reader, (u8*)streamMem.virtualAddress,
                                   size, &size, pedantic_reading );

            }
            moreFrames = (tmp==HANTRO_OK) ? HANTRO_TRUE : HANTRO_FALSE;
        }
        if( moreFrames && size != (u32)-1)
        {
            /* Decode */
            if (!webpLoop)
            {
                decInput.pStream = (const u8*)streamMem.virtualAddress;
                decInput.dataLen = size + extraStrmBufferBits;
                decInput.streamBusAddress = (u32)streamMem.busAddress;
            }

            decsw_performance();
            START_SW_PERFORMANCE;
            ret = VP8DecDecode(decInst, &decInput, &decOutput);
            END_SW_PERFORMANCE;
            decsw_performance();
            if (ret == VP8DEC_HDRS_RDY)
            {
                i32 mcuInRow;
                i32 mcuSizeDivider = 1;
                hdrsRdy = 1;
                ret = VP8DecGetInfo(decInst, &info);
                mcuInRow = (info.frameWidth / 16);

#ifndef PP_PIPELINE_ENABLED
#ifdef ASIC_TRACE_SUPPORT
                /* Handle incorrect slice size for HW testing */
                if(decInput.sliceHeight > (info.frameHeight >> 4))
                {
                    decInput.sliceHeight = (info.frameHeight >> 4);
                    printf("FIXED Decoder Slice MB Set %d\n", decInput.sliceHeight);
                }
#endif /* ASIC_TRACE_SUPPORT */

#ifdef ASIC_TRACE_SUPPORT
                /* 8190 and over 16M ==> force to slice mode */
                if((decInput.sliceHeight == 0) &&
                   (snapShot) &&
                   ((info.frameWidth * info.frameHeight) >
                    VP8DEC_MAX_PIXEL_AMOUNT))
                {
                    do
                    {
                        decInput.sliceHeight++;
                    }
                    while(((decInput.sliceHeight * (mcuInRow / mcuSizeDivider)) +
                           (mcuInRow / mcuSizeDivider)) <
                          VP8DEC_MAX_SLICE_SIZE);
                    printf
                        ("Force to slice mode (over 16M) ==> Decoder Slice MB Set %d\n",
                         decInput.sliceHeight);
                    forcedSliceMode = 1;
                }
#else
                /* 8190 and over 16M ==> force to slice mode */
                if((decInput.sliceHeight == 0) &&
                   (snapShot) &&
                   ((info.frameWidth * info.frameHeight) >
                    VP8DEC_MAX_PIXEL_AMOUNT))
                {
                    do
                    {
                        decInput.sliceHeight++;
                    }
                    while(((decInput.sliceHeight * (mcuInRow / mcuSizeDivider)) +
                           (mcuInRow / mcuSizeDivider)) <
                          VP8DEC_MAX_SLICE_SIZE);
                    printf
                        ("Force to slice mode (over 16M) ==> Decoder Slice MB Set %d\n",
                         decInput.sliceHeight);
                    forcedSliceMode = 1;
                }
#endif /* ASIC_TRACE_SUPPORT */
#endif /* PP_PIPELINE_ENABLED */

                DEBUG_PRINT(("\nWidth %d Height %d\n",
                        info.frameWidth, info.frameHeight));

                if (userMemAlloc)
                {
                    u32 sizeLuma;
                    u32 sizeChroma;
                    u32 rotation = 0;
                    u32 cropping = 0;
                    u32 sliceHeight = 0;
                    u32 widthY, widthC;
                    
                    widthY = pbp.lumaStride ? pbp.lumaStride : info.frameWidth;
                    widthC = pbp.chromaStride ? pbp.chromaStride : info.frameWidth;

                    for( i = 0 ; i < 16 ; ++i )
                    {
                        if(userAllocLuma[i].virtualAddress)
                            DWLFreeRefFrm(((VP8DecContainer_t *) decInst)->dwl, &userAllocLuma[i]);
                        if(userAllocChroma[i].virtualAddress)
                            DWLFreeRefFrm(((VP8DecContainer_t *) decInst)->dwl, &userAllocChroma[i]);
                    }

                    DEBUG_PRINT(("User allocated memory\n",
                            info.frameWidth, info.frameHeight));

                    sliceHeight = ((VP8DecContainer_t *) decInst)->sliceHeight;
                    if(forcedSliceMode)
                        sliceHeight = decInput.sliceHeight;
                    sizeLuma = sliceHeight ?
                        (sliceHeight + 1) * 16 * widthY :
                        info.frameHeight * widthY;

                    sizeChroma = sliceHeight ?
                        (sliceHeight + 1) * 8 * widthC :
                        info.frameHeight * widthC / 2;

                    if(decFormat == VP8DEC_WEBP)
                    {
                            
#ifdef PP_PIPELINE_ENABLED
                        rotation = pp_rotation_used();
                        cropping = pp_cropping_used();
                        if (rotation || (cropping && (info.frameWidth <= hwConfig.maxDecPicWidth || info.frameHeight < 4096)))
                           printf("User allocated output memory");
                        else
                            sizeLuma = sizeChroma = 2; /* ugly hack*/
#endif                   
                        
                        if (DWLMallocRefFrm(((VP8DecContainer_t *) decInst)->dwl,
                                sizeLuma, &userAllocLuma[0]) != DWL_OK)
                        {
                            fprintf(stderr,"UNABLE TO ALLOCATE PICTURE MEMORY\n");
                            return -1;
                        }
                        if (DWLMallocRefFrm(((VP8DecContainer_t *) decInst)->dwl,
                                sizeChroma, &userAllocChroma[0]) != DWL_OK)
                        {
                            fprintf(stderr,"UNABLE TO ALLOCATE PICTURE MEMORY\n");
                            return -1;
                        }

                        decInput.pPicBufferY = userAllocLuma[0].virtualAddress;
                        decInput.picBufferBusAddressY = userAllocLuma[0].busAddress;
                        decInput.pPicBufferC = userAllocChroma[0].virtualAddress;
                        decInput.picBufferBusAddressC = userAllocChroma[0].busAddress;
                        if(pbp.lumaStride || pbp.chromaStride )
                        {
                            if( VP8DecSetPictureBuffers( decInst, &pbp ) != VP8DEC_OK )
                            {
                                fprintf(stderr, "ERROR IN SETUP OF CUSTOM FRAME STRIDES\n");
                                return -1;
                            }
                        }
                    }
                    else /* VP8 */
                    {
                        u32 *pPicBufferY[16];
                        u32 *pPicBufferC[16];                        
                        u32 picBufferBusAddressY[16];
                        u32 picBufferBusAddressC[16];                     
                        pbp.numBuffers = numFrameBuffers;
                        if( pbp.numBuffers < 5 )
                            pbp.numBuffers = 5;
                         
                        /* Custom use case: interleaved buffers (strides must 
                         * meet strict requirements here). If met, only one or
                         * two buffers will be allocated, into which all ref
                         * pictures' data will be interleaved into. */   
                        if(interleavedUserBuffers)
                        {
                            u32 sizeBuffer;
                            /* Mode 1: luma / chroma strides same; both can be interleaved */
                            if( ((pbp.lumaStride == pbp.chromaStride) || 
                                 ((2*pbp.lumaStride) == pbp.chromaStride)) && 
                                pbp.lumaStride >= info.frameWidth*2*pbp.numBuffers)
                            {
                                DEBUG_PRINT(("Interleave mode 1: One buffer\n"));                                
                                sizeBuffer = pbp.lumaStride * (info.frameHeight+1);
                                if (DWLMallocRefFrm(((VP8DecContainer_t *) decInst)->dwl,
                                        sizeBuffer, &userAllocLuma[0]) != DWL_OK)
                                {
                                    fprintf(stderr,"UNABLE TO ALLOCATE PICTURE MEMORY\n");
                                    return -1;
                                }
                                
                                for( i = 0 ; i < pbp.numBuffers ; ++i )
                                {
                                    pPicBufferY[i] = userAllocLuma[0].virtualAddress +
                                        (info.frameWidth*2*i)/4;
                                    picBufferBusAddressY[i] = userAllocLuma[0].busAddress +
                                        info.frameWidth*2*i;
                                    pPicBufferC[i] = userAllocLuma[0].virtualAddress +
                                        (info.frameWidth*(2*i+1))/4;
                                    picBufferBusAddressC[i] = userAllocLuma[0].busAddress +
                                        info.frameWidth*(2*i+1);
                                }
                                
                            }
                            else /* Mode 2: separate buffers for luma and chroma */
                            {
                                DEBUG_PRINT(("Interleave mode 2: Two buffers\n"));                                
                                if( (pbp.lumaStride < info.frameWidth*pbp.numBuffers) || 
                                    (pbp.chromaStride < info.frameWidth*pbp.numBuffers))
                                {
                                    fprintf(stderr, "CHROMA STRIDE LENGTH TOO SMALL FOR INTERLEAVED FRAME BUFFERS\n");
                                    return -1;
                                } 
                                
                                sizeBuffer = pbp.lumaStride * (info.frameHeight+1);
                                if (DWLMallocRefFrm(((VP8DecContainer_t *) decInst)->dwl,
                                        sizeBuffer, &userAllocLuma[0]) != DWL_OK)
                                {
                                    fprintf(stderr,"UNABLE TO ALLOCATE PICTURE MEMORY\n");
                                    return -1;
                                }
                                sizeBuffer = pbp.chromaStride * (info.frameHeight+1);
                                if (DWLMallocRefFrm(((VP8DecContainer_t *) decInst)->dwl,
                                        sizeBuffer, &userAllocChroma[0]) != DWL_OK)
                                {
                                    fprintf(stderr,"UNABLE TO ALLOCATE PICTURE MEMORY\n");
                                    return -1;
                                }                                
                                for( i = 0 ; i < pbp.numBuffers ; ++i )
                                {
                                    pPicBufferY[i] = userAllocLuma[0].virtualAddress +
                                        (info.frameWidth*i)/4;
                                    picBufferBusAddressY[i] = userAllocLuma[0].busAddress +
                                        info.frameWidth*i;
                                    pPicBufferC[i] = userAllocChroma[0].virtualAddress +
                                        (info.frameWidth*i)/4;
                                    picBufferBusAddressC[i] = userAllocChroma[0].busAddress +
                                        info.frameWidth*i;
                                }
                            }
                        }
                        else /* dedicated buffers */
                        {
                        
                            if(alternateAllocOrder) /* alloc all lumas first
                                                     * and only then chromas */
                            {
                                for( i = 0 ; i < pbp.numBuffers ; ++i )
                                {
                                    if (DWLMallocRefFrm(((VP8DecContainer_t *) decInst)->dwl,
                                            sizeLuma, &userAllocLuma[i]) != DWL_OK)
                                    {
                                        fprintf(stderr,"UNABLE TO ALLOCATE PICTURE MEMORY\n");
                                        return -1;
                                    }
                                    pPicBufferY[i] = userAllocLuma[i].virtualAddress;
                                    picBufferBusAddressY[i] = userAllocLuma[i].busAddress;
                                }                            
                                for( i = 0 ; i < pbp.numBuffers ; ++i )
                                {
                                    if (DWLMallocRefFrm(((VP8DecContainer_t *) decInst)->dwl,
                                            sizeChroma, &userAllocChroma[i]) != DWL_OK)
                                    {
                                        fprintf(stderr,"UNABLE TO ALLOCATE PICTURE MEMORY\n");
                                        return -1;
                                    }           
                                    pPicBufferC[i] = userAllocChroma[i].virtualAddress;
                                    picBufferBusAddressC[i] = userAllocChroma[i].busAddress;
                                }                            
                            }
                            else
                            {

                                for( i = 0 ; i < pbp.numBuffers ; ++i )
                                {
                                    if (DWLMallocRefFrm(((VP8DecContainer_t *) decInst)->dwl,
                                            sizeLuma, &userAllocLuma[i]) != DWL_OK)
                                    {
                                        fprintf(stderr,"UNABLE TO ALLOCATE PICTURE MEMORY\n");
                                        return -1;
                                    }
                                    if (DWLMallocRefFrm(((VP8DecContainer_t *) decInst)->dwl,
                                            sizeChroma, &userAllocChroma[i]) != DWL_OK)
                                    {
                                        fprintf(stderr,"UNABLE TO ALLOCATE PICTURE MEMORY\n");
                                        return -1;
                                    }           
                                    pPicBufferY[i] = userAllocLuma[i].virtualAddress;
                                    picBufferBusAddressY[i] = userAllocLuma[i].busAddress;
                                    pPicBufferC[i] = userAllocChroma[i].virtualAddress;
                                    picBufferBusAddressC[i] = userAllocChroma[i].busAddress;
                                }                            
                            }
                        }
                        pbp.pPicBufferY = pPicBufferY;
                        pbp.picBufferBusAddressY = picBufferBusAddressY;
                        pbp.pPicBufferC = pPicBufferC;
                        pbp.picBufferBusAddressC = picBufferBusAddressC;

                        if( VP8DecSetPictureBuffers( decInst, &pbp ) != VP8DEC_OK )
                        {
                            fprintf(stderr, "ERROR IN SETUP OF CUSTOM FRAME BUFFERS\n");
                            return -1;
                        }
                    }
                }
                else if( pbp.lumaStride || pbp.chromaStride )
                {
                    if( VP8DecSetPictureBuffers( decInst, &pbp ) != VP8DEC_OK )
                    {
                        fprintf(stderr, "ERROR IN SETUP OF CUSTOM FRAME STRIDES\n");
                        return -1;
                    }
                }
                

                decsw_performance();
                START_SW_PERFORMANCE;

                ret = VP8DecDecode(decInst, &decInput, &decOutput);
                END_SW_PERFORMANCE;
                decsw_performance();
                TBSetRefbuMemModel( &tbCfg, 
                    ((VP8DecContainer_t *) decInst)->vp8Regs,
                    &((VP8DecContainer_t *) decInst)->refBufferCtrl );

            }
            else if (ret != VP8DEC_PIC_DECODED &&
                     ret != VP8DEC_SLICE_RDY)
            {
                continue;
            }

            webpLoop = (ret == VP8DEC_SLICE_RDY);

            picRdy = 1;

            decsw_performance();
            START_SW_PERFORMANCE;
	    
            if (usePeekOutput &&
                VP8DecPeek(decInst, &decPicture) == VP8DEC_PIC_RDY)
            {
                END_SW_PERFORMANCE;
                decsw_performance();

                if (!framePicture ||
                    (outFrameWidth && outFrameWidth < decPicture.frameWidth) ||
                    (outFrameHeight && outFrameHeight < decPicture.frameHeight))
                    writeRawFrame(fout,
                        (unsigned char *) decPicture.pOutputFrame,
                        (unsigned char *) decPicture.pOutputFrameC,
                         decPicture.frameWidth, decPicture.frameHeight,
                         decPicture.codedWidth, decPicture.codedHeight,
                         planarOutput,
                         decPicture.outputFormat,
                         decPicture.lumaStride,
                         decPicture.chromaStride, md5sum);
                else
                {
                    if (!outFrameWidth)
                        outFrameWidth = decPicture.frameWidth;
                    if (!outFrameHeight)
                        outFrameHeight = decPicture.frameHeight;
                    if( pFramePic == NULL)
                        pFramePic = (u8*)malloc(
                            outFrameWidth * outFrameHeight * 3/2 * sizeof(u8));

                    FramePicture( (u8*)decPicture.pOutputFrame,
                        (u8*)decPicture.pOutputFrameC,
                        decPicture.codedWidth,
                        decPicture.codedHeight,
                        decPicture.frameWidth,
                        decPicture.frameHeight,
                        pFramePic, outFrameWidth, outFrameHeight,
                        decPicture.lumaStride,
                        decPicture.chromaStride );
                    writeRawFrame(fout,
                        pFramePic,
                        NULL,
                        outFrameWidth, outFrameHeight,
                        outFrameWidth, outFrameHeight, 1,
                        decPicture.outputFormat,
                        outFrameWidth,
                        outFrameWidth, md5sum);
                }

                decsw_performance();
                START_SW_PERFORMANCE;
    
                while (VP8DecNextPicture(decInst, &decPicture, 0) ==
                       VP8DEC_PIC_RDY);
    
                END_SW_PERFORMANCE;
                decsw_performance();

            }
            else if(!usePeekOutput)
            {
                while (VP8DecNextPicture(decInst, &decPicture, 0) ==
                       VP8DEC_PIC_RDY)
                {
                    END_SW_PERFORMANCE;
                    decsw_performance();

#ifndef PP_PIPELINE_ENABLED
                    if (!framePicture ||
                        decPicture.numSliceRows ||
                        (outFrameWidth &&
                         outFrameWidth < decPicture.frameWidth) ||
                        (outFrameHeight &&
                         outFrameHeight < decPicture.frameHeight))
                    {
                        if (decPicture.numSliceRows)
                        {
                            writeSlice(fout, &decPicture);
                        }
                        else
                        {
                            writeRawFrame(fout,
                                (unsigned char *) decPicture.pOutputFrame,
                                (unsigned char *) decPicture.pOutputFrameC,
                                 decPicture.frameWidth, decPicture.frameHeight,
                                 decPicture.codedWidth, decPicture.codedHeight,
                                 planarOutput,
                                 decPicture.outputFormat,
                                 decPicture.lumaStride, 
                                 decPicture.chromaStride, md5sum);
                        }
                    }
                    else
                    {
                        if (!outFrameWidth)
                            outFrameWidth = decPicture.frameWidth;
                        if (!outFrameHeight)
                            outFrameHeight = decPicture.frameHeight;
                        if( pFramePic == NULL)
                            pFramePic = (u8*)malloc(
                                outFrameWidth * outFrameHeight * 3/2 *
                                sizeof(u8));
                                
                        FramePicture( (u8*)decPicture.pOutputFrame,
                            (u8*)decPicture.pOutputFrameC,
                            decPicture.codedWidth,
                            decPicture.codedHeight,
                            decPicture.frameWidth,
                            decPicture.frameHeight,
                            pFramePic, outFrameWidth, outFrameHeight,
                            decPicture.lumaStride,
                            decPicture.chromaStride );
                        writeRawFrame(fout,
                            pFramePic,
                            NULL,
                            outFrameWidth, outFrameHeight,
                            outFrameWidth, outFrameHeight, 1,
                            decPicture.outputFormat,
                            outFrameWidth,
                            outFrameWidth, md5sum);
                            
                    }
#else
                    HandlePpOutput(picDisplayNumber++, decInst);
#endif
                }
            }

            END_SW_PERFORMANCE;
            decsw_performance();
        }
        if (ret != VP8DEC_SLICE_RDY)
            picDecodeNumber++;
    }
    while( moreFrames && (picDecodeNumber != maxNumPics || !maxNumPics) );

    decsw_performance();
    START_SW_PERFORMANCE;

    while (!usePeekOutput &&
           VP8DecNextPicture(decInst, &decPicture, 1) == VP8DEC_PIC_RDY)
    {    

        END_SW_PERFORMANCE;
        decsw_performance();
    
#ifndef PP_PIPELINE_ENABLED
        writeRawFrame(fout,
            (unsigned char *) decPicture.pOutputFrame,
            (unsigned char *) decPicture.pOutputFrameC,
             decPicture.frameWidth, decPicture.frameHeight,
             decPicture.codedWidth, decPicture.codedHeight,
             planarOutput,
             decPicture.outputFormat,
             decPicture.lumaStride,
             decPicture.chromaStride,
             md5sum);
#else
        HandlePpOutput(picDisplayNumber++, decInst);
#endif
    }

end:

    if(streamMem.virtualAddress)
        DWLFreeLinear(((VP8DecContainer_t *) decInst)->dwl, &streamMem);

#ifdef PP_PIPELINE_ENABLED
    pp_close();
#endif

    if (userMemAlloc)
    {
        for( i = 0 ; i < 16 ; ++i )
        {
            if(userAllocLuma[i].virtualAddress)
                DWLFreeRefFrm(((VP8DecContainer_t *) decInst)->dwl, &userAllocLuma[i]);
            if(userAllocChroma[i].virtualAddress)
                DWLFreeRefFrm(((VP8DecContainer_t *) decInst)->dwl, &userAllocChroma[i]);
        }
    }

    decsw_performance();
    START_SW_PERFORMANCE;
    VP8DecRelease(decInst);
    END_SW_PERFORMANCE;
    decsw_performance();

#ifdef ASIC_TRACE_SUPPORT
    trace_SequenceCtrl(hwDecPicCount, 0);
    trace_RefbufferHitrate();
    /*trace_AVSDecodingTools();*/
    closeTraceFiles();
#endif

    if(reader)
        rdr_close(reader);
#ifndef PP_PIPELINE_ENABLED
    if(fout)
        fclose(fout);
#endif /* PP_PIPELINE_ENABLED */

    if(pFramePic)
        free(pFramePic);

    if (pic_big_endian)
        free(pic_big_endian);

    if(md5sum)
    {
        /* slice mode output is semiplanar yuv, convert to md5sum */
        if (decPicture.numSliceRows)
        {
            char cmd[256];
            sprintf(cmd, "md5sum %s | tr 'a-z' 'A-Z' | sed 's/ .*//' > .md5sum.txt", outFileName);
            system(cmd);
            sprintf(cmd, "mv .md5sum.txt %s", outFileName);
            system(cmd);
        }
    }

    return 0;
}

/*------------------------------------------------------------------------------

    Function name: decsw_performance

    Functional description: breakpoint for performance

    Inputs:  void

    Outputs:
chromaBufOff
    Returns: void

------------------------------------------------------------------------------*/
void decsw_performance(void)
{
}

void writeRawFrame(FILE * fp, unsigned char *buffer,
    unsigned char *bufferCh, int frameWidth,
    int frameHeight, int croppedWidth, int croppedHeight, int planar,
    int tiledMode, u32 lumaStride, u32 chromaStride, u32 md5sum)
{

    int lumaSize = lumaStride * frameHeight;
    int frameSize = lumaSize + (chromaStride*frameHeight/2);
    u8 *rasterScan = NULL;
    static pic_number = 0;
    static struct MD5Context ctx;
    unsigned char digest[16];
    int i = 0;
    unsigned char *cb,*cr;
    DEBUG_PRINT(("WRITING PICTURE %d\n", pic_number++));

    /* Convert back to raster scan format if decoder outputs 
     * tiled format */
    if(tiledMode && convertTiledOutput)
    {
        rasterScan = (u8*)malloc(frameSize);
        if(!rasterScan)
        {
            fprintf(stderr, "error allocating memory for tiled"
                "-->raster conversion!\n");
            return;
        }

        TBTiledToRaster( tiledMode, DEC_DPB_FRAME, buffer,
            rasterScan, frameWidth, frameHeight );
        buffer = rasterScan;

        TBTiledToRaster( tiledMode, DEC_DPB_FRAME, bufferCh,
            rasterScan+lumaSize, frameWidth, frameHeight/2 );
        bufferCh = rasterScan+lumaSize;
    }

#ifndef ASIC_TRACE_SUPPORT
    if(output_picture_endian == DEC_X170_BIG_ENDIAN)
    {
        if(pic_big_endian_size != frameSize)
        {
            if(pic_big_endian != NULL)
                free(pic_big_endian);

            pic_big_endian = (u8 *) malloc(frameSize);
            if(pic_big_endian == NULL)
            {
                DEBUG_PRINT(("MALLOC FAILED @ %s %d", __FILE__, __LINE__));
                if(rasterScan)
                    free(rasterScan);
                return;
            }

            pic_big_endian_size = frameSize;
        }

        memcpy(pic_big_endian, buffer, frameSize);
        TbChangeEndianess(pic_big_endian, frameSize);
        buffer = pic_big_endian;
    }
#endif
    if(md5sum)
    {
        /* chroma should be right after luma */
        if (!userMemAlloc)
        {
            MD5Init(&ctx);
            MD5Update(&ctx, buffer, frameSize);
            MD5Final(digest, &ctx);

            for(i = 0; i < sizeof digest; i++)
            {
               fprintf(fp, "%02X", digest[i]);
            }
            fprintf(fp, "\n");

            if(rasterScan)
                free(rasterScan);

            return;
        }
    }

    if (bufferCh == NULL)
    {
        bufferCh = buffer + lumaSize;
    }

    if (!heightCrop || (croppedHeight == frameHeight && croppedWidth == frameWidth))
    {
        u32 i, j;
        u8 *bufferTmp;
        bufferTmp = buffer;

        for( i = 0 ; i < frameHeight ; ++i )
        {
            fwrite(bufferTmp, includeStrides ? lumaStride : frameWidth, 1, fp );
            bufferTmp += lumaStride;
        }

        /*fwrite(buffer, lumaSize, 1, fp);*/
        if (!planar)
        {
            bufferTmp = bufferCh;
            for( i = 0 ; i < frameHeight / 2 ; ++i )
            {
                fwrite( bufferTmp, includeStrides ? chromaStride : frameWidth, 1, fp );
                bufferTmp += chromaStride;
            }
        }
        else
        {
            bufferTmp = bufferCh;
            for(i = 0; i < frameHeight / 2; i++)
            {
                for( j = 0 ; j < (includeStrides ? chromaStride / 2 : frameWidth / 2); ++j)
                {
                    fwrite(bufferTmp + j * 2, 1, 1, fp);
                }
                bufferTmp += chromaStride;
            }
            bufferTmp = bufferCh + 1;
            for(i = 0; i < frameHeight / 2; i++)
            {
                for( j = 0 ; j < (includeStrides ? chromaStride / 2: frameWidth / 2); ++j)
                {
                    fwrite(bufferTmp + j * 2, 1, 1, fp);
                }
                bufferTmp += chromaStride;
            }
        }
    }
    else
    {
        u32 row;
        for( row = 0 ; row < croppedHeight ; row++) 
        {
            fwrite(buffer + row*lumaStride, croppedWidth, 1, fp);
        }
        if (!planar)
        {
            if(croppedHeight &1)
                croppedHeight++;
            if(croppedWidth & 1)
                croppedWidth++;
            for( row = 0 ; row < croppedHeight/2 ; row++)
                fwrite(bufferCh + row*chromaStride, (croppedWidth*2)/2, 1, fp);
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
                    fwrite(bufferCh + row*chromaStride + i * 2, 1, 1, fp);
            }
            for( row = 0 ; row < croppedHeight/2 ; ++row )
            {
                for(i = 0; i < croppedWidth/2; i++)
                    fwrite(bufferCh + 1 + row*chromaStride + i * 2, 1, 1, fp);
            }
        }
    }

    if(rasterScan)
        free(rasterScan);

}

void FramePicture( u8 *pIn, u8* pCh, i32 inWidth, i32 inHeight,
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

#ifdef PP_PIPELINE_ENABLED
void HandlePpOutput(u32 picNum, VP8DecInst decoder)
{
    PPResult res;

    res = pp_check_combined_status();

    if(res == PP_OK)
    {
        pp_write_output(picNum, 0, 0);
        pp_read_blend_components(((VP8DecContainer_t *) decoder)->pp.ppInstance);
    }
    if(pp_update_config
       (decoder,
        snapShot ?  PP_PIPELINED_DEC_TYPE_WEBP : PP_PIPELINED_DEC_TYPE_VP8,
        &tbCfg) == CFG_UPDATE_FAIL)

    {
        fprintf(stdout, "PP CONFIG LOAD FAILED\n");
    }

}
#endif


void writeSlice(FILE * fp, VP8DecPicture *decPic)
{

    int lumaSize = includeStrides ? decPic->lumaStride * decPic->frameHeight :
                                    decPic->frameWidth * decPic->frameHeight;
    int sliceSize = includeStrides ? decPic->lumaStride * decPic->numSliceRows :
                                     decPic->frameWidth * decPic->numSliceRows;
    int chromaSize = includeStrides ? decPic->chromaStride * decPic->frameHeight / 2:
                                    decPic->frameWidth * decPic->frameHeight / 2;
    static u8 *tmpCh = NULL;
    static u32 rowCount = 0;
    u32 sliceRows;
    u32 lumaStride, chromaStride;
    u32 i, j;
    u8 *ptr = (u8*)decPic->pOutputFrame;

    if (tmpCh == NULL)
    {
        tmpCh = (u8*)malloc(chromaSize);
    }

    sliceRows = decPic->numSliceRows; 

    DEBUG_PRINT(("WRITING SLICE, rows %d\n",decPic->numSliceRows));
    
    lumaStride = decPic->lumaStride;
    chromaStride = decPic->chromaStride;
    
    if(!heightCrop)
    {
        /*fwrite(decPic->pOutputFrame, 1, sliceSize, fp);*/
        for ( i = 0 ; i < decPic->numSliceRows ; ++i )
        {
            fwrite(ptr, 1, includeStrides ? lumaStride : decPic->codedWidth, fp );
            ptr += lumaStride;
        }        
    }
    else
    {
        if(rowCount + sliceRows > decPic->codedHeight )
            sliceRows -= 
                (decPic->frameHeight - decPic->codedHeight);
        for ( i = 0 ; i < sliceRows ; ++i )
        {
            fwrite(ptr, 1, includeStrides ? lumaStride : decPic->codedWidth, fp );
            ptr += lumaStride;
        }
    }

    for( i = 0 ; i < decPic->numSliceRows/2 ; ++i )
    {
        memcpy(tmpCh + ((i+(rowCount/2)) * (includeStrides ? chromaStride : decPic->frameWidth)),
               decPic->pOutputFrameC + (i*chromaStride)/4,
               includeStrides ? chromaStride : decPic->frameWidth );
    }
/*    memcpy(tmpCh + rowCount/2 * decPic->frameWidth, decPic->pOutputFrameC,
        sliceSize/2);*/

    rowCount += decPic->numSliceRows;

    if (rowCount == decPic->frameHeight)
    {
        if(!heightCrop)
        {
            if (!planarOutput)
            {
                fwrite(tmpCh, lumaSize/2, 1, fp);
                /*
                ptr = tmpCh;
                for ( i = 0 ; i < decPic->numSliceRows/2 ; ++i )
                {
                    fwrite(ptr, 1, decPic->codedWidth, fp );
                    ptr += chromaStride;
                } */       
            }
            else
            {
                u32 i, tmp;
                tmp = chromaSize / 2;
                for(i = 0; i < tmp; i++)
                    fwrite(tmpCh + i * 2, 1, 1, fp);
                for(i = 0; i < tmp; i++)
                    fwrite(tmpCh + 1 + i * 2, 1, 1, fp);
            }
        }
        else
        {
            if(!planarOutput)
            {
                ptr = tmpCh;
                for ( i = 0 ; i < decPic->codedHeight/2 ; ++i )
                {
                    fwrite(ptr, 1, decPic->codedWidth, fp );
                    ptr += decPic->frameWidth;
                }
            }
            else
            {
                ptr = tmpCh;
                for ( i = 0 ; i < decPic->codedHeight/2 ; ++i )
                {
                    for( j = 0 ; j < decPic->codedWidth/2 ; ++j ) 
                        fwrite(ptr + 2*j, 1, 1, fp );
                    ptr += decPic->frameWidth;
                }
                ptr = tmpCh+1;
                for ( i = 0 ; i < decPic->codedHeight/2 ; ++i )
                {
                    for( j = 0 ; j < decPic->codedWidth/2 ; ++j ) 
                        fwrite(ptr + 2*j, 1, 1, fp );
                    ptr += decPic->frameWidth;
                }
            }
        }
    }

}

u32 FfReadFrame( reader_inst reader, const u8 *buffer, u32 maxBufferSize,
                 u32 *frameSize, u32 pedantic )
{
    u32 tmp;
    u32 ret = rdr_read_frame(reader, buffer, maxBufferSize, frameSize,
                             pedantic);
    if (ret != HANTRO_OK)
        return ret;

    /* stream corruption, packet loss etc */
    if (streamPacketLoss)
    {
        ret =  TBRandomizePacketLoss(tbCfg.tbParams.streamPacketLoss, &tmp);
        if (ret == 0 && tmp)
            return FfReadFrame(reader, buffer, maxBufferSize, frameSize,
                               pedantic);
    }

    if (streamTruncate && picRdy && (hdrsRdy || streamHeaderCorrupt))
    {
        DEBUG_PRINT(("strmLen %d\n", *frameSize));
        ret = TBRandomizeU32(frameSize);
        DEBUG_PRINT(("Randomized strmLen %d\n", *frameSize));
    }
    if (streamBitSwap)
    {
        if (streamHeaderCorrupt || hdrsRdy)
        {
            if (picRdy)
            {
                ret = TBRandomizeBitSwapInStream(buffer,
                    *frameSize, tbCfg.tbParams.streamBitSwap);
            }
        }
    }
    return HANTRO_OK;
}
