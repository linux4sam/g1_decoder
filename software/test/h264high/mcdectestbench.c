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

#include "h264decapi.h"
#include "dwl.h"

#include "bytestream_parser.h"
#include "libav-wrapper.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>

#ifdef ASIC_TRACE_SUPPORT
#include "trace.h"
#endif

#include "h264hwd_container.h"
#include "tb_cfg.h"
#include "tb_tiled.h"
#include "regdrv.h"

#include "tb_md5.h"
#include "tb_sw_performance.h"

#ifdef _ENABLE_2ND_CHROMA
#include "h264decapi_e.h"
#endif

/*------------------------------------------------------------------------------
    Module defines
------------------------------------------------------------------------------*/

/* Debug prints */
#undef DEBUG_PRINT
#define DEBUG_PRINT(argv) printf argv

#define NUM_RETRY   100 /* how many retries after HW timeout */
#define ALIGNMENT_MASK 7

void WriteOutput(char *filename, char *filenameTiled, u8 * data, u32 picSize,
                 u32 picWidth, u32 picHeight, u32 frameNumber, u32 monoChrome,
                 u32 view, u32 tiledMode);
u32 NextPacket(u8 ** pStrm);

u32 CropPicture(u8 * pOutImage, u8 * pInImage,
                u32 frameWidth, u32 frameHeight, H264CropParams * pCropParams,
                u32 monoChrome);
static void printDecodeReturn(i32 retval);
void printH264MCPicCodingType(u32 picType);

/* Global variables for stream handling */
u8 *streamStop = NULL;
u32 packetize = 0;
u32 nalUnitStream = 0;
FILE *foutput = NULL, *foutput2 = NULL;
FILE *fTiledOutput = NULL;
FILE *fchroma2 = NULL;

/* flag to enable md5sum output */
u32 md5sum = 0;

FILE *findex = NULL;

/* stream start address */
u32 traceUsedStream = 0;

/* output file writing disable */
u32 disableOutputWriting = 0;
u32 retry = 0;

u32 clock_gating = DEC_X170_INTERNAL_CLOCK_GATING;
u32 data_discard = DEC_X170_DATA_DISCARD_ENABLE;
u32 latency_comp = DEC_X170_LATENCY_COMPENSATION;
u32 output_picture_endian = DEC_X170_OUTPUT_PICTURE_ENDIAN;
u32 bus_burst_length = DEC_X170_BUS_BURST_LENGTH;
u32 asic_service_priority = DEC_X170_ASIC_SERVICE_PRIORITY;
u32 output_format = DEC_X170_OUTPUT_FORMAT;
u32 service_merge_disable = DEC_X170_SERVICE_MERGE_DISABLE;

u32 streamTruncate = 0;
u32 streamPacketLoss = 0;
u32 streamHeaderCorrupt = 0;
u32 hdrsRdy = 0;
u32 picRdy = 0;
TBCfg tbCfg;

u32 longStream = 0;
FILE *finput;
u32 planarOutput = 0;
u32 isInputMp4 = 0;
void *mp4file = NULL;

const u32 tiledOutput = DEC_REF_FRM_RASTER_SCAN;
u32 dpbMode = DEC_DPB_FRAME;
u32 convertTiledOutput = 0;

u32 usePeekOutput = 0;
u32 enableMvc = 0;
u32 mvcSeparateViews = 0;
u32 skipNonReference = 0;
u32 convertToFrameDpb = 0;

u32 bFrames;

char *grey_chroma = NULL;
size_t grey_chroma_size = 0;

char *pic_big_endian = NULL;
size_t pic_big_endian_size = 0;

#ifdef ASIC_TRACE_SUPPORT
extern u32 hwDecPicCount;
extern u32 gHwVer;
extern u32 h264HighSupport;
#endif

#ifdef H264_EVALUATION
extern u32 gHwVer;
extern u32 h264HighSupport;
#endif

#ifdef ADS_PERFORMANCE_SIMULATION

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

H264DecInst decInst;
H264DecInfo decInfo;

const char *outFileName = "out.yuv";
const char *outFileNameTiled = "out_tiled.yuv";
u32 picSize;
u32 cropDisplay = 0;
u8 *tmpImage = NULL;

pthread_t output_thread;
int output_thread_run = 0;

#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>

u32 picDisplayNumber = 0;
u32 old_picDisplayNumber = 0;

pid_t main_pid;

#ifdef TESTBENCH_WATCHDOG
void watchdog1(int signal_number)
{
    if(picDisplayNumber == old_picDisplayNumber)
    {
        fprintf(stderr, "\n\nTESTBENCH TIMEOUT\n");
        kill(main_pid, SIGTERM);
    }
    else
    {
        old_picDisplayNumber = picDisplayNumber;
    }

}
#endif

void SetMissingField2Const( u8 *pOutputPicture,
                            u32 picWidth,
                            u32 picHeight,
                            u32 monochrome,
                            u32 topField)
{
    u8 *fieldBase;
    i32 i;

    /* TODO: Does not support tiled input */

    if (dpbMode != DEC_DPB_FRAME)
    {
        /* luma */
        fieldBase = pOutputPicture;

        if (!topField)
        {
            /* bottom */
            fieldBase += picWidth * picHeight / 2;
        }

        memset(fieldBase, 128, picWidth * picHeight / 2);

        if (monochrome)
            return;

        /* chroma */
        fieldBase = pOutputPicture + picWidth * picHeight;

        if (!topField)
        {
            /* bottom */
            fieldBase += picWidth * ((picHeight / 2 + 1) / 2);
        }

        memset(fieldBase, 128, picWidth * ((picHeight / 2 + 1) / 2));

        return;
    }

    /* luma */
    fieldBase = pOutputPicture;

    if (!topField)
    {
        /* bottom */
        fieldBase += picWidth;
    }

    for (i = 0; i < picHeight / 2; i++)
    {
        memset(fieldBase, 128, picWidth);
        fieldBase += 2 * picWidth;
    }

    if (monochrome)
        return;

    /* chroma */
    fieldBase = pOutputPicture + picWidth * picHeight;

    if (!topField)
    {
        /* bottom */
        fieldBase += picWidth;
    }

    for (i = 0; i < (picHeight / 2 + 1) / 2; i++)
    {
        memset(fieldBase, 128, picWidth);
        fieldBase += 2 * picWidth;
    }
}

/* Output thread entry point. */
static void* h264_output_thread(void* arg)
{
    H264DecPicture decPicture;

#ifdef TESTBENCH_WATCHDOG
    /* fpga watchdog: 30 sec timer for frozen decoder */
    {
        struct itimerval t = {{30,0}, {30,0}};
        struct sigaction sa;

        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = watchdog1;
        sa.sa_flags |= SA_RESTART;  /* restart of system calls */
        sigaction(SIGALRM, &sa, NULL);

        setitimer(ITIMER_REAL, &t, NULL);
    }
#endif

    while(output_thread_run)
    {
        u8 *imageData;
        H264DecRet ret;

        ret = H264DecMCNextPicture(decInst, &decPicture);

        if(ret == H264DEC_PIC_RDY)
        {
                DEBUG_PRINT(("PIC %d/%d, view %d, type %s",
                        picDisplayNumber,
                        decPicture.picId,
                        decPicture.viewId,
                        decPicture.isIdrPicture ? "    IDR," : "NON-IDR,"));

                 /* pic coding type */
                 printH264MCPicCodingType(decPicture.picCodingType);

                if(decPicture.nbrOfErrMBs)
                {
                    DEBUG_PRINT((", concealed %d", decPicture.nbrOfErrMBs));
                }

                if(decPicture.interlaced)
                {
                    DEBUG_PRINT((", INTERLACED "));
                    if(decPicture.fieldPicture)
                    {
                        DEBUG_PRINT(("FIELD %s",
                                     decPicture.topField ? "TOP" : "BOTTOM"));

                        SetMissingField2Const((u8*)decPicture.pOutputPicture,
                                              decPicture.picWidth,
                                              decPicture.picHeight,
                                              decInfo.monoChrome,
                                              !decPicture.topField );
                    }
                    else
                    {
                        DEBUG_PRINT(("FRAME"));
                    }
                }

                DEBUG_PRINT((", Crop: (%d, %d), %dx%d\n",
                        decPicture.cropParams.cropLeftOffset,
                        decPicture.cropParams.cropTopOffset,
                        decPicture.cropParams.cropOutWidth,
                        decPicture.cropParams.cropOutHeight));

                fflush(stdout);

                /* Write output picture to file */
                imageData = (u8 *) decPicture.pOutputPicture;

                if(cropDisplay)
                {
                    int tmp = CropPicture(tmpImage, imageData,
                                          decPicture.picWidth,
                                          decPicture.picHeight,
                                          &decPicture.cropParams,
                                          decInfo.monoChrome);
                    if(tmp)
                    {
                        DEBUG_PRINT(("ERROR in cropping!\n"));
                    }
                }

                WriteOutput(outFileName, outFileNameTiled,
                            cropDisplay ? tmpImage : imageData, picSize,
                            cropDisplay ? decPicture.cropParams.cropOutWidth : decPicture.picWidth,
                            cropDisplay ? decPicture.cropParams.cropOutHeight : decPicture.picHeight,
                            picDisplayNumber - 1, decInfo.monoChrome,
                            decPicture.viewId,
                            decPicture.outputFormat);

                H264DecMCPictureConsumed(decInst, &decPicture);

                //usleep(1000); /* 1ms  sleep */

                picDisplayNumber++;
        }
        else if(ret == H264DEC_END_OF_STREAM)
        {
            DEBUG_PRINT(("END-OF-STREAM received in output thread\n"));
            break;
        }
    }
}

/* one extra stream buffer so that we can decode ahead,
 * and be ready when core has finished
 */
#define MAX_STRM_BUFFERS    (MAX_ASIC_CORES + 1)

static DWLLinearMem_t streamMem[MAX_STRM_BUFFERS];
static u32 streamMemStatus[MAX_STRM_BUFFERS];
static u32 allocated_buffers = 0;

static sem_t stream_buff_free;
static pthread_mutex_t strm_buff_stat_lock = PTHREAD_MUTEX_INITIALIZER;


void StreamBufferConsumed(void *pStream, void *pUserData)
{
    int idx;
    pthread_mutex_lock(&strm_buff_stat_lock);

    idx = 0;
    do
    {
        if ((u8*)pStream >= (u8*)streamMem[idx].virtualAddress &&
            (u8*)pStream < (u8*)streamMem[idx].virtualAddress + streamMem[idx].size)
        {
            streamMemStatus[idx] = 0;
            assert(pUserData == streamMem[idx].virtualAddress);
            break;
        }
        idx++;
    }
    while(idx < allocated_buffers);

    assert(idx < allocated_buffers);

    pthread_mutex_unlock(&strm_buff_stat_lock);

    sem_post(&stream_buff_free);
}

int GetFreeStreamBuffer()
{
    int idx;
    sem_wait(&stream_buff_free);

    pthread_mutex_lock(&strm_buff_stat_lock);

    idx = 0;
    while(streamMemStatus[idx])
    {
        idx++;
    }
    assert(idx < allocated_buffers);

    streamMemStatus[idx] = 1;

    pthread_mutex_unlock(&strm_buff_stat_lock);

    return idx;
}

/*------------------------------------------------------------------------------

    Function name:  main

    Purpose:
        main function of decoder testbench. Provides command line interface
        with file I/O for H.264 decoder. Prints out the usage information
        when executed without arguments.

------------------------------------------------------------------------------*/

int main(int argc, char **argv)
{

    u32 i, tmp;
    u32 maxNumPics = 0;
    u8 *imageData;
    long int strmLen;

    H264DecRet ret;
    H264DecInput decInput;
    H264DecOutput decOutput;
    H264DecPicture decPicture;

    i32 dwlret;

    u32 picDecodeNumber = 0;

    u32 numErrors = 0;
    u32 disableOutputReordering = 0;
    u32 useDisplaySmoothing = 0;
    u32 rlcMode = 0;
    u32 mbErrorConcealment = 0;
    u32 forceWholeStream = 0;
    const u8 *ptmpstream = NULL;
    u32 streamWillEnd = 0;

    FILE *fTBCfg;
    u32 seedRnd = 0;
    u32 streamBitSwap = 0;
    i32 corruptedBytes = 0;  /*  */

    const char *inFileName = argv[argc - 1];

#ifdef ASIC_TRACE_SUPPORT
    gHwVer = 8190; /* default to 8190 mode */
    h264HighSupport = 1;
#endif

#ifdef H264_EVALUATION_8170
    gHwVer = 8170;
    h264HighSupport = 0;
#elif H264_EVALUATION_8190
    gHwVer = 8190;
    h264HighSupport = 1;
#elif H264_EVALUATION_9170
    gHwVer = 9170;
    h264HighSupport = 1;
#elif H264_EVALUATION_9190
    gHwVer = 9190;
    h264HighSupport = 1;
#elif H264_EVALUATION_G1
    gHwVer = 10000;
    h264HighSupport = 1;
#endif

    main_pid = getpid();

#ifndef EXPIRY_DATE
#define EXPIRY_DATE (u32)0xFFFFFFFF
#endif /* EXPIRY_DATE */

    /* expiry stuff */
    {
        u8 tmBuf[7];
        time_t sysTime;
        struct tm * tm;
        u32 tmp1;

        /* Check expiration date */
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

    for(i = 0; i < MAX_STRM_BUFFERS; i++)
    {
        streamMem[i].virtualAddress = NULL;
        streamMem[i].busAddress = 0;
    }

    /* set test bench configuration */
    TBSetDefaultCfg(&tbCfg);
    fTBCfg = fopen("tb.cfg", "r");
    if(fTBCfg == NULL)
    {
        DEBUG_PRINT(("UNABLE TO OPEN INPUT FILE: \"tb.cfg\"\n"));
        DEBUG_PRINT(("USING DEFAULT CONFIGURATION\n"));
    }
    else
    {
        fclose(fTBCfg);
        if(TBParseConfig("tb.cfg", TBReadParam, &tbCfg) == TB_FALSE)
            return -1;
        if(TBCheckCfg(&tbCfg) != 0)
            return -1;
    }

    INIT_SW_PERFORMANCE;

    {
        H264DecApiVersion decApi;
        H264DecBuild decBuild;
        u32 nCores;

        /* Print API version number */
        decApi = H264DecGetAPIVersion();
        decBuild = H264DecGetBuild();
        nCores = H264DecMCGetCoreCount();

        DEBUG_PRINT(("\nX170 H.264 Decoder, SW API v%d.%d - build: %d.%d"
                     "\n                    HW: %4x - build: %4x, %d cores\n\n",
                decApi.major, decApi.minor,
                decBuild.swBuild >> 16, decBuild.swBuild & 0xFFFF,
                decBuild.hwBuild >> 16, decBuild.hwBuild & 0xFFFF,
                nCores));

        /* number of stream buffers to allocate */
        allocated_buffers = nCores + 1;
    }

    /* Check that enough command line arguments given, if not -> print usage
     * information out */
    if(argc < 2)
    {
        DEBUG_PRINT(("Usage: %s [options] file.h264\n", argv[0]));
        DEBUG_PRINT(("\t-Nn forces decoding to stop after n pictures\n"));
        DEBUG_PRINT(("\t-O<file> write output to <file> (default out_wxxxhyyy.yuv)\n"));
        DEBUG_PRINT(("\t--md5 Output frame based md5 checksum. No YUV output!\n"));
        DEBUG_PRINT(("\t-X Disable output file writing\n"));
        DEBUG_PRINT(("\t-C display cropped image (default decoded image)\n"));
        DEBUG_PRINT(("\t-R disable DPB output reordering\n"));
        DEBUG_PRINT(("\t-J enable double DPB for smooth display\n"));
        DEBUG_PRINT(("\t-P write planar output\n"));
        DEBUG_PRINT(("\t-M Enable MVC decoding (use it only with MVC streams)\n"));
        DEBUG_PRINT(("\t--separate-fields-in-dpb DPB stores interlaced content"\
                    " as fields (default: frames)\n"));
        DEBUG_PRINT(("\t--output-frame-dpb Convert output to frame mode even if"\
            " field DPB mode used\n"));
        DEBUG_PRINT(("\t-M Enable MVC decoding (use it only with MVC streams)\n"));
        DEBUG_PRINT(("\t-V Write MVC views to separate files\n"));
        DEBUG_PRINT(("\t-Q Skip decoding non-reference pictures.\n"));

        return 0;
    }

    /* read command line arguments */
    for(i = 1; i < (u32) (argc - 1); i++)
    {
        if(strncmp(argv[i], "-N", 2) == 0)
        {
            maxNumPics = (u32) atoi(argv[i] + 2);
        }
        else if(strncmp(argv[i], "-O", 2) == 0)
        {
            outFileName = argv[i] + 2;
        }
        else if(strcmp(argv[i], "-X") == 0)
        {
            disableOutputWriting = 1;
        }
        else if(strcmp(argv[i], "-C") == 0)
        {
            cropDisplay = 1;
        }
        else if (strncmp(argv[i], "-E", 2) == 0)
        {
            DEBUG_PRINT(("WARNING! Tiled mode ignored, not supported with multicore!"));
        }
        else if (strncmp(argv[i], "-G", 2) == 0)
        {
            convertTiledOutput = 1;
        }
        else if(strcmp(argv[i], "-R") == 0)
        {
            disableOutputReordering = 1;
        }
        else if(strcmp(argv[i], "-J") == 0)
        {
            useDisplaySmoothing = 1;
        }
        else if(strcmp(argv[i], "-W") == 0)
        {
            forceWholeStream = 1;
        }
        else if(strcmp(argv[i], "-L") == 0)
        {
            longStream = 1;
        }
        else if(strcmp(argv[i], "-P") == 0)
        {
            planarOutput = 1;
        }
        else if(strcmp(argv[i], "-M") == 0)
        {
            enableMvc = 1;
        }
        else if(strcmp(argv[i], "-V") == 0)
        {
            mvcSeparateViews = 1;
        }
        else if(strcmp(argv[i], "-Q") == 0)
        {
            skipNonReference = 1;
        }
        else if(strcmp(argv[i], "--separate-fields-in-dpb") == 0)
        {
            dpbMode = DEC_DPB_INTERLACED_FIELD;
        }
        else if(strcmp(argv[i], "--output-frame-dpb") == 0)
        {
            convertToFrameDpb = 1;
        }
        else if(strcmp(argv[i], "-Z") == 0)
        {
            usePeekOutput = 1;
        }
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

    if(enableMvc)
    {
        finput = fopen(inFileName, "r");
        if(!finput)
        {
            fprintf(stderr, "Failed to open input file: %s\n", inFileName);
            return -1;
        }
    }
    else
    {
        libav_init();

        if(libav_open(inFileName) < 0)
            return -1;
    }

    /*TBPrintCfg(&tbCfg); */
    mbErrorConcealment = 0; /* TBGetDecErrorConcealment(&tbCfg); */
    clock_gating = TBGetDecClockGating(&tbCfg);
    data_discard = TBGetDecDataDiscard(&tbCfg);
    latency_comp = tbCfg.decParams.latencyCompensation;
    output_picture_endian = TBGetDecOutputPictureEndian(&tbCfg);
    bus_burst_length = tbCfg.decParams.busBurstLength;
    asic_service_priority = tbCfg.decParams.asicServicePriority;
    service_merge_disable = TBGetDecServiceMergeDisable(&tbCfg);

    DEBUG_PRINT(("Decoder Macro Block Error Concealment %d\n",
                 mbErrorConcealment));
    DEBUG_PRINT(("Decoder Clock Gating %d\n", clock_gating));
    DEBUG_PRINT(("Decoder Data Discard %d\n", data_discard));
    DEBUG_PRINT(("Decoder Latency Compensation %d\n", latency_comp));
    DEBUG_PRINT(("Decoder Output Picture Endian %d\n", output_picture_endian));
    DEBUG_PRINT(("Decoder Bus Burst Length %d\n", bus_burst_length));
    DEBUG_PRINT(("Decoder Asic Service Priority %d\n", asic_service_priority));

    seedRnd = tbCfg.tbParams.seedRnd;
    streamHeaderCorrupt = TBGetTBStreamHeaderCorrupt(&tbCfg);
    /* if headers are to be corrupted
     * -> do not wait the picture to finalize before starting stream corruption */
    if(streamHeaderCorrupt)
        picRdy = 1;
    streamTruncate = TBGetTBStreamTruncate(&tbCfg);
    if(strcmp(tbCfg.tbParams.streamBitSwap, "0") != 0)
    {
        streamBitSwap = 1;
    }
    else
    {
        streamBitSwap = 0;
    }
    if(strcmp(tbCfg.tbParams.streamPacketLoss, "0") != 0)
    {
        streamPacketLoss = 1;
    }
    else
    {
        streamPacketLoss = 0;
    }

    longStream = 1;
    packetize = 1;
    nalUnitStream = 0;

    DEBUG_PRINT(("TB Packet by Packet  %d\n", packetize));
    DEBUG_PRINT(("TB Nal Unit Stream %d\n", nalUnitStream));
    DEBUG_PRINT(("TB Seed Rnd %d\n", seedRnd));
    DEBUG_PRINT(("TB Stream Truncate %d\n", streamTruncate));
    DEBUG_PRINT(("TB Stream Header Corrupt %d\n", streamHeaderCorrupt));
    DEBUG_PRINT(("TB Stream Bit Swap %d; odds %s\n", streamBitSwap,
                 tbCfg.tbParams.streamBitSwap));
    DEBUG_PRINT(("TB Stream Packet Loss %d; odds %s\n", streamPacketLoss,
                 tbCfg.tbParams.streamPacketLoss));

    {
        remove("regdump.txt");
        remove("mbcontrol.hex");
        remove("intra4x4_modes.hex");
        remove("motion_vectors.hex");
        remove("rlc.hex");
        remove("picture_ctrl_dec.trc");
    }

#ifdef ASIC_TRACE_SUPPORT
    /* open tracefiles */
    tmp = openTraceFiles();
    if(!tmp)
    {
        DEBUG_PRINT(("UNABLE TO OPEN TRACE FILE(S)\n"));
    }
    if(nalUnitStream)
        trace_h264DecodingTools.streamMode.nalUnitStrm = 1;
    else
        trace_h264DecodingTools.streamMode.byteStrm = 1;
#endif

    /* initialize multicore decoder. If unsuccessful -> exit */
    {
        H264DecMCConfig mcInitCfg;

        sem_init(&stream_buff_free, 0, allocated_buffers);

        mcInitCfg.noOutputReordering = disableOutputReordering;
        mcInitCfg.useDisplaySmoothing = useDisplaySmoothing;

        mcInitCfg.dpbFlags = dpbMode ? DEC_DPB_ALLOW_FIELD_ORDERING : 0;
        mcInitCfg.streamConsumedCallback = StreamBufferConsumed;

        START_SW_PERFORMANCE;
        ret = H264DecMCInit(&decInst, &mcInitCfg);
        END_SW_PERFORMANCE;
    }

    if(ret != H264DEC_OK)
    {
        DEBUG_PRINT(("DECODER INITIALIZATION FAILED\n"));
        goto end;
    }

    /* configure decoder to decode both views of MVC stereo high streams */
    if (enableMvc)
        H264DecSetMvc(decInst);

    /* Set ref buffer test mode */
    ((decContainer_t *) decInst)->refBufferCtrl.testFunction = TBRefbuTestMode;

    SetDecRegister(((decContainer_t *) decInst)->h264Regs, HWIF_DEC_LATENCY,
                   latency_comp);
    SetDecRegister(((decContainer_t *) decInst)->h264Regs, HWIF_DEC_CLK_GATE_E,
                   clock_gating);
    SetDecRegister(((decContainer_t *) decInst)->h264Regs, HWIF_DEC_OUT_ENDIAN,
                   output_picture_endian);
    SetDecRegister(((decContainer_t *) decInst)->h264Regs, HWIF_DEC_MAX_BURST,
                   bus_burst_length);
    if ((DWLReadAsicID() >> 16) == 0x8170U)
    {
        SetDecRegister(((decContainer_t *) decInst)->h264Regs, HWIF_PRIORITY_MODE,
                       asic_service_priority);
    }
    SetDecRegister(((decContainer_t *) decInst)->h264Regs, HWIF_DEC_DATA_DISC_E,
                   data_discard);
    SetDecRegister(((decContainer_t *) decInst)->h264Regs, HWIF_SERV_MERGE_DIS,
                   service_merge_disable);

#ifdef _ENABLE_2ND_CHROMA
    if (!TBGetDecCh8PixIleavOutput(&tbCfg))
    {
        ((decContainer_t *) decInst)->storage.enable2ndChroma = 0;
    }
    else
    {
        /* cropping not supported when additional chroma output format used */
        cropDisplay = 0;
    }
#endif

    TBInitializeRandom(seedRnd);

    decInput.skipNonReference = skipNonReference;

    for(i = 0; i < allocated_buffers; i++)
    {
        if(DWLMallocLinear(((decContainer_t *) decInst)->dwl,
                4096*1165, streamMem + i) != DWL_OK)
        {
            DEBUG_PRINT(("UNABLE TO ALLOCATE STREAM BUFFER MEMORY\n"));
            goto end;
        }
    }
    {
        int id = GetFreeStreamBuffer();

        decInput.pStream = (u8 *) streamMem[id].virtualAddress;
        decInput.streamBusAddress = (u32) streamMem[id].busAddress;

        /* stream processed callback param */
        decInput.pUserData = streamMem[id].virtualAddress;
    }

    /* get pointer to next packet and the size of packet
     * (for packetize or nalUnitStream modes) */
    ptmpstream = decInput.pStream;
    if((tmp = NextPacket((u8 **) (&decInput.pStream))) != 0)
    {
        decInput.dataLen = tmp;
        decInput.streamBusAddress += (u32) (decInput.pStream - ptmpstream);
    }

    picDecodeNumber = picDisplayNumber = 1;

    /* main decoding loop */
    do
    {
        if(streamTruncate && picRdy && (hdrsRdy || streamHeaderCorrupt) &&
           (longStream || (!longStream && (packetize || nalUnitStream))))
        {
            i32 ret;

            ret = TBRandomizeU32(&decInput.dataLen);
            if(ret != 0)
            {
                DEBUG_PRINT(("RANDOM STREAM ERROR FAILED\n"));
                return 0;
            }
            DEBUG_PRINT(("Randomized stream size %d\n", decInput.dataLen));
        }

        /* If enabled, break the stream */
        if(streamBitSwap)
        {
            if((hdrsRdy && !streamHeaderCorrupt) || streamHeaderCorrupt)
            {
                /* Picture needs to be ready before corrupting next picture */
                if(picRdy && corruptedBytes <= 0)
                {
                    ret =
                        TBRandomizeBitSwapInStream(decInput.pStream,
                                                   decInput.dataLen,
                                                   tbCfg.tbParams.
                                                   streamBitSwap);
                    if(ret != 0)
                    {
                        DEBUG_PRINT(("RANDOM STREAM ERROR FAILED\n"));
                        goto end;
                    }

                    corruptedBytes = decInput.dataLen;
                    DEBUG_PRINT(("corruptedBytes %d\n", corruptedBytes));
                }
            }
        }

        /* Picture ID is the picture number in decoding order */
        decInput.picId = picDecodeNumber;

        /* call API function to perform decoding */
        START_SW_PERFORMANCE;
        ret = H264DecMCDecode(decInst, &decInput, &decOutput);
        END_SW_PERFORMANCE;
        printDecodeReturn(ret);
        switch (ret)
        {
        case H264DEC_STREAM_NOT_SUPPORTED:
            {
                DEBUG_PRINT(("ERROR: UNSUPPORTED STREAM!\n"));
                goto end;
            }
        case H264DEC_HDRS_RDY:
            {
                /* Set a flag to indicate that headers are ready */
                hdrsRdy = 1;
                TBSetRefbuMemModel( &tbCfg,
                    ((decContainer_t *) decInst)->h264Regs,
                    &((decContainer_t *) decInst)->refBufferCtrl );

                /* Stream headers were successfully decoded
                 * -> stream information is available for query now */

                START_SW_PERFORMANCE;
                tmp = H264DecGetInfo(decInst, &decInfo);
                END_SW_PERFORMANCE;
                if(tmp != H264DEC_OK)
                {
                    DEBUG_PRINT(("ERROR in getting stream info!\n"));
                    goto end;
                }

                DEBUG_PRINT(("Width %d Height %d\n",
                             decInfo.picWidth, decInfo.picHeight));

                DEBUG_PRINT(("Cropping params: (%d, %d) %dx%d\n",
                             decInfo.cropParams.cropLeftOffset,
                             decInfo.cropParams.cropTopOffset,
                             decInfo.cropParams.cropOutWidth,
                             decInfo.cropParams.cropOutHeight));

                DEBUG_PRINT(("MonoChrome = %d\n", decInfo.monoChrome));
                DEBUG_PRINT(("Interlaced = %d\n", decInfo.interlacedSequence));
                DEBUG_PRINT(("DPB mode   = %d\n", decInfo.dpbMode));
                DEBUG_PRINT(("Pictures in DPB = %d\n", decInfo.picBuffSize));
                DEBUG_PRINT(("Pictures in Multibuffer PP = %d\n", decInfo.multiBuffPpSize));

                dpbMode = decInfo.dpbMode;

                /* check if we do need to crop */
                if(decInfo.cropParams.cropLeftOffset == 0 &&
                   decInfo.cropParams.cropTopOffset == 0 &&
                   decInfo.cropParams.cropOutWidth == decInfo.picWidth &&
                   decInfo.cropParams.cropOutHeight == decInfo.picHeight )
                {
                    cropDisplay = 0;
                }

                /* crop if asked to do so */
                if(cropDisplay)
                {
                    /* release the old one */
                    if(tmpImage)
                        free(tmpImage);

                    /* Cropped frame size in planar YUV 4:2:0 */
                    picSize = decInfo.cropParams.cropOutWidth *
                        decInfo.cropParams.cropOutHeight;
                    if(!decInfo.monoChrome)
                        picSize = (3 * picSize) / 2;
                    tmpImage = malloc(picSize);
                    if(tmpImage == NULL)
                    {
                        DEBUG_PRINT(("ERROR in allocating cropped image!\n"));
                        goto end;
                    }
                }
                else
                {
                    picSize = decInfo.picWidth * decInfo.picHeight;
                    if(!decInfo.monoChrome)
                        picSize = (3 * picSize) / 2;
                }

                DEBUG_PRINT(("videoRange %d, matrixCoefficients %d\n",
                             decInfo.videoRange, decInfo.matrixCoefficients));

                if(!output_thread_run)
                {
                    output_thread_run = 1;
                    pthread_create(&output_thread, NULL, h264_output_thread, NULL);
                }

                break;
            }
        case H264DEC_ADVANCED_TOOLS:
            {
                /* ASO/FMO detected and not supported in multicore mode */
                DEBUG_PRINT(("ASO/FMO detected, decoding will stop\n", ret));
                goto end;
            }
        case H264DEC_PIC_DECODED:
            /* we should never return data in MC mode.
             * buffer shall contain one full frame
             */
            /* when new access unit is detected then is OK to return
             * all data
             */
            if(decOutput.dataLeft)
                DEBUG_PRINT(("\tUnfinished buffer, %d bytes\n",
                         decOutput.dataLeft));
        case H264DEC_PENDING_FLUSH:
            /* case H264DEC_FREEZED_PIC_RDY: */
            /* Picture is now ready */
            picRdy = 1;

            /*lint -esym(644,tmpImage,picSize) variable initialized at
             * H264DEC_HDRS_RDY_BUFF_NOT_EMPTY case */

            /* If enough pictures decoded -> force decoding to end
             * by setting that no more stream is available */
            if(picDecodeNumber == maxNumPics)
                decInput.dataLen = 0;

            printf("DECODED PICTURE %d\n", picDecodeNumber);
            /* Increment decoding number for every decoded picture */
            picDecodeNumber++;

            retry = 0;
            break;

        case H264DEC_STRM_PROCESSED:
        case H264DEC_NONREF_PIC_SKIPPED:
        case H264DEC_STRM_ERROR:
            {
                /* Used to indicate that picture decoding needs to finalized prior to corrupting next picture
                 * picRdy = 0; */

                break;
            }

        case H264DEC_OK:
            /* nothing to do, just call again */
            break;
        case H264DEC_HW_TIMEOUT:
            DEBUG_PRINT(("Timeout\n"));
            goto end;
        default:
            DEBUG_PRINT(("FATAL ERROR: %d\n", ret));
            goto end;
        }

        /* break out of do-while if maxNumPics reached (dataLen set to 0) */
        if(decInput.dataLen == 0)
            break;

            if(decOutput.dataLeft)
            {
                decInput.streamBusAddress += (decOutput.pStrmCurrPos - decInput.pStream);
                corruptedBytes -= (decInput.dataLen - decOutput.dataLeft);
                decInput.dataLen = decOutput.dataLeft;
                decInput.pStream = decOutput.pStrmCurrPos;
            }
            else
            {

                {
                    int id = GetFreeStreamBuffer();

                    decInput.pStream = (u8 *) streamMem[id].virtualAddress;
                    decInput.streamBusAddress = streamMem[id].busAddress;
                    /* stream processed callback param */
                    decInput.pUserData = streamMem[id].virtualAddress;
                }

                ptmpstream = decInput.pStream;

                decInput.dataLen = NextPacket((u8 **) (&decInput.pStream));

                decInput.streamBusAddress +=
                    (u32) (decInput.pStream - ptmpstream);

                corruptedBytes = 0;
            }

        /* keep decoding until all data from input stream buffer consumed
         * and all the decoded/queued frames are ready */
    }
    while(decInput.dataLen > 0);

end:
    DEBUG_PRINT(("Decoding ended, flush the DPB\n"));

    H264DecMCEndOfStream(decInst);

    if(output_thread)
        pthread_join(output_thread, NULL);

    sem_destroy(&stream_buff_free);

    fflush(stdout);

    /* have to release stream buffers before releasing decoder as we need DWL */
    for(i = 0; i < allocated_buffers; i++)
    {
        if(streamMem[i].virtualAddress != NULL)
        {
            DWLFreeLinear(((decContainer_t *) decInst)->dwl, &streamMem[i]);
        }
    }


    /* release decoder instance */
    START_SW_PERFORMANCE;
    H264DecRelease(decInst);
    END_SW_PERFORMANCE;

    libav_release();

    if(foutput)
        fclose(foutput);
    if(foutput2)
        fclose(foutput2);
    if(fchroma2)
        fclose(fchroma2);
    if(fTiledOutput)
        fclose(fTiledOutput);
    if(finput)
        fclose(finput);

    /* free allocated buffers */
    if(tmpImage != NULL)
        free(tmpImage);
    if(grey_chroma != NULL)
        free(grey_chroma);
    if(pic_big_endian)
        free(pic_big_endian);

    foutput = fopen(outFileName, "rb");
    if(NULL == foutput)
    {
        strmLen = 0;
    }
    else
    {
        fseek(foutput, 0L, SEEK_END);
        strmLen = (u32) ftell(foutput);
        fclose(foutput);
    }

    DEBUG_PRINT(("Output file: %s\n", outFileName));

    DEBUG_PRINT(("OUTPUT_SIZE %d\n", strmLen));

    FINALIZE_SW_PERFORMANCE;

    DEBUG_PRINT(("DECODING DONE\n"));

#ifdef ASIC_TRACE_SUPPORT
    trace_SequenceCtrl(hwDecPicCount, 0);
    trace_H264DecodingTools();
    /* close trace files */
    closeTraceFiles();
#endif

    if(retry > NUM_RETRY)
    {
        return -1;
    }

    if(numErrors || picDecodeNumber == 1)
    {
        DEBUG_PRINT(("ERRORS FOUND %d %d\n", numErrors, picDecodeNumber));
        /*return 1;*/
        return 0;
    }

    return 0;
}

/*------------------------------------------------------------------------------

    Function name:  WriteOutput

    Purpose:
        Write picture pointed by data to file. Size of the
        picture in pixels is indicated by picSize.

------------------------------------------------------------------------------*/
void WriteOutput(char *filename, char *filenameTiled, u8 * data, u32 picSize,
                 u32 picWidth, u32 picHeight, u32 frameNumber, u32 monoChrome,
                 u32 view, u32 tiledMode)
{

    u32 i, tmp;
    FILE **fout;
    char altFileName[256];
    char *fn;
    u8 *rasterScan = NULL;

    if(disableOutputWriting != 0)
    {
        return;
    }

    fout = (view && mvcSeparateViews) ? &foutput2 : &foutput;
    /* foutput is global file pointer */
    if(*fout == NULL)
    {
        if (view && mvcSeparateViews)
        {
            strcpy(altFileName, filename);
            sprintf(altFileName+strlen(altFileName)-4, "_%d.yuv", view);
            fn = altFileName;
        }
        else
            fn = filename;

        /* open output file for writing, can be disabled with define.
         * If file open fails -> exit */
        if(strcmp(filename, "none") != 0)
        {
            *fout = fopen(fn, "wb");
            if(*fout == NULL)
            {
                DEBUG_PRINT(("UNABLE TO OPEN OUTPUT FILE\n"));
                return;
            }
        }
#ifdef _ENABLE_2ND_CHROMA
        if (TBGetDecCh8PixIleavOutput(&tbCfg) && !monoChrome)
        {
            fchroma2 = fopen("out_ch8pix.yuv", "wb");
            if(fchroma2 == NULL)
            {
                DEBUG_PRINT(("UNABLE TO OPEN OUTPUT FILE\n"));
                return;
            }
        }
#endif
    }

#if 0
    if(fTiledOutput == NULL && tiledOutput)
    {
        /* open output file for writing, can be disabled with define.
         * If file open fails -> exit */
        if(strcmp(filenameTiled, "none") != 0)
        {
            fTiledOutput = fopen(filenameTiled, "wb");
            if(fTiledOutput == NULL)
            {
                DEBUG_PRINT(("UNABLE TO OPEN TILED OUTPUT FILE\n"));
                return;
            }
        }
    }
#endif


    /* Convert back to raster scan format if decoder outputs
     * tiled format */
    if(tiledMode && convertTiledOutput)
    {
        u32 effHeight = (picHeight + 15) & (~15);
        rasterScan = (u8*)malloc(picWidth*effHeight*3/2);
        if(!rasterScan)
        {
            fprintf(stderr, "error allocating memory for tiled"
                "-->raster conversion!\n");
            return;
        }

        TBTiledToRaster( tiledMode, convertToFrameDpb ? dpbMode : DEC_DPB_FRAME,
            data, rasterScan, picWidth,
            effHeight );
        if(!monoChrome)
            TBTiledToRaster( tiledMode, convertToFrameDpb ? dpbMode : DEC_DPB_FRAME,
            data+picWidth*effHeight,
            rasterScan+picWidth*effHeight, picWidth, effHeight/2 );
        data = rasterScan;
    }
    else if ( convertToFrameDpb && (dpbMode != DEC_DPB_FRAME))
    {
        u32 effHeight = (picHeight + 15) & (~15);
        rasterScan = (u8*)malloc(picWidth*effHeight*3/2);
        if(!rasterScan)
        {
            fprintf(stderr, "error allocating memory for tiled"
                "-->raster conversion!\n");
            return;
        }

        TBFieldDpbToFrameDpb( convertToFrameDpb ? dpbMode : DEC_DPB_FRAME,
            data, rasterScan, monoChrome, picWidth, effHeight );

        data = rasterScan;
    }

    if(monoChrome)
    {
        if(grey_chroma_size != (picSize / 2))
        {
            if(grey_chroma != NULL)
                free(grey_chroma);

            grey_chroma = (char *) malloc(picSize / 2);
            if(grey_chroma == NULL)
            {
                DEBUG_PRINT(("UNABLE TO ALLOCATE GREYSCALE CHROMA BUFFER\n"));
                if(rasterScan)
                    free(rasterScan);
                return;
            }
            grey_chroma_size = picSize / 2;
            memset(grey_chroma, 128, grey_chroma_size);
        }
    }

    if(*fout == NULL || data == NULL)
    {
        return;
    }

#ifndef ASIC_TRACE_SUPPORT
    if(output_picture_endian == DEC_X170_BIG_ENDIAN)
    {
        if(pic_big_endian_size != picSize)
        {
            if(pic_big_endian != NULL)
                free(pic_big_endian);

            pic_big_endian = (char *) malloc(picSize);
            if(pic_big_endian == NULL)
            {
                DEBUG_PRINT(("MALLOC FAILED @ %s %d", __FILE__, __LINE__));
                if(rasterScan)
                    free(rasterScan);
                return;
            }

            pic_big_endian_size = picSize;
        }

        memcpy(pic_big_endian, data, picSize);
        TbChangeEndianess(pic_big_endian, picSize);
        data = pic_big_endian;
    }
#endif

    if(md5sum)
    {
        TBWriteFrameMD5Sum(*fout, data, picSize, frameNumber);
    }
    else
    {
        /* this presumes output has system endianess */
        if(planarOutput && !monoChrome)
        {
            tmp = picSize * 2 / 3;
            fwrite(data, 1, tmp, *fout);
            for(i = 0; i < tmp / 4; i++)
                fwrite(data + tmp + i * 2, 1, 1, *fout);
            for(i = 0; i < tmp / 4; i++)
                fwrite(data + tmp + 1 + i * 2, 1, 1, *fout);
        }
        else    /* semi-planar */
        {
            fwrite(data, 1, picSize, *fout);
            if(monoChrome)
            {
                fwrite(grey_chroma, 1, grey_chroma_size, *fout);
            }
        }
    }

#ifdef _ENABLE_2ND_CHROMA
    if (TBGetDecCh8PixIleavOutput(&tbCfg) && !monoChrome)
    {
        u8 *pCh;
        u32 tmp;
        H264DecRet ret;

        ret = H264DecNextChPicture(decInst, &pCh, &tmp);
        ASSERT(ret == H264DEC_PIC_RDY);
#ifndef ASIC_TRACE_SUPPORT
        if(output_picture_endian == DEC_X170_BIG_ENDIAN)
        {
            if(pic_big_endian_size != picSize/3)
            {
                if(pic_big_endian != NULL)
                    free(pic_big_endian);

                pic_big_endian = (char *) malloc(picSize/3);
                if(pic_big_endian == NULL)
                {
                    DEBUG_PRINT(("MALLOC FAILED @ %s %d", __FILE__, __LINE__));
                    if(rasterScan)
                        free(rasterScan);
                    return;
                }

                pic_big_endian_size = picSize/3;
            }

            memcpy(pic_big_endian, pCh, picSize/3);
            TbChangeEndianess(pic_big_endian, picSize/3);
            pCh = pic_big_endian;
        }
#endif

        if(md5sum)
            TBWriteFrameMD5Sum(fchroma2, pCh, picSize/3, frameNumber);
        else
            fwrite(pCh, 1, picSize/3, fchroma2);
    }
#endif

    if(rasterScan)
        free(rasterScan);


}

/*------------------------------------------------------------------------------

    Function name: NextPacket

        Returns the packet size in bytes

------------------------------------------------------------------------------*/
u32 NextPacket(u8 ** pStrm)

{
    u32 length;

    if (enableMvc)
    {
        /* libav does not support MVC */
        length = NextNALFromFile(finput, *pStrm, streamMem->size);
    }
    else
    {
        length = libav_read_frame(*pStrm, streamMem->size);
    }

    if (length)
        DEBUG_PRINT(("NextPacket = %d\n", length));

    return length;
}

/*------------------------------------------------------------------------------

    Function name: CropPicture

    Purpose:
        Perform cropping for picture. Input picture pInImage with dimensions
        frameWidth x frameHeight is cropped with pCropParams and the resulting
        picture is stored in pOutImage.

------------------------------------------------------------------------------*/
u32 CropPicture(u8 * pOutImage, u8 * pInImage,
                u32 frameWidth, u32 frameHeight, H264CropParams * pCropParams,
                u32 monoChrome)
{

    u32 i, j;
    u32 outWidth, outHeight;
    u8 *pOut, *pIn;

    if(pOutImage == NULL || pInImage == NULL || pCropParams == NULL ||
       !frameWidth || !frameHeight)
    {
        return (1);
    }

    if(((pCropParams->cropLeftOffset + pCropParams->cropOutWidth) >
        frameWidth) ||
       ((pCropParams->cropTopOffset + pCropParams->cropOutHeight) >
        frameHeight))
    {
        return (1);
    }

    outWidth = pCropParams->cropOutWidth;
    outHeight = pCropParams->cropOutHeight;

    /* Calculate starting pointer for luma */
    pIn = pInImage + pCropParams->cropTopOffset * frameWidth +
        pCropParams->cropLeftOffset;
    pOut = pOutImage;

    /* Copy luma pixel values */
    for(i = outHeight; i; i--)
    {
        for(j = outWidth; j; j--)
        {
            *pOut++ = *pIn++;
        }
        pIn += frameWidth - outWidth;
    }

#if 0   /* planar */
    outWidth >>= 1;
    outHeight >>= 1;

    /* Calculate starting pointer for cb */
    pIn = pInImage + frameWidth * frameHeight +
        pCropParams->cropTopOffset * frameWidth / 4 +
        pCropParams->cropLeftOffset / 2;

    /* Copy cb pixel values */
    for(i = outHeight; i; i--)
    {
        for(j = outWidth; j; j--)
        {
            *pOut++ = *pIn++;
        }
        pIn += frameWidth / 2 - outWidth;
    }

    /* Calculate starting pointer for cr */
    pIn = pInImage + 5 * frameWidth * frameHeight / 4 +
        pCropParams->cropTopOffset * frameWidth / 4 +
        pCropParams->cropLeftOffset / 2;

    /* Copy cr pixel values */
    for(i = outHeight; i; i--)
    {
        for(j = outWidth; j; j--)
        {
            *pOut++ = *pIn++;
        }
        pIn += frameWidth / 2 - outWidth;
    }
#else /* semiplanar */

    if(monoChrome)
        return 0;

    outHeight >>= 1;

    /* Calculate starting pointer for chroma */
    pIn = pInImage + frameWidth * frameHeight +
        (pCropParams->cropTopOffset * frameWidth / 2 +
         pCropParams->cropLeftOffset);

    /* Copy chroma pixel values */
    for(i = outHeight; i; i--)
    {
        for(j = outWidth; j; j -= 2)
        {
            *pOut++ = *pIn++;
            *pOut++ = *pIn++;
        }
        pIn += (frameWidth - outWidth);
    }

#endif

    return (0);
}

/*------------------------------------------------------------------------------

    Function name:  H264DecTrace

    Purpose:
        Example implementation of H264DecTrace function. Prototype of this
        function is given in H264DecApi.h. This implementation appends
        trace messages to file named 'dec_api.trc'.

------------------------------------------------------------------------------*/
void H264DecTrace(const char *string)
{
    FILE *fp;

#if 0
    fp = fopen("dec_api.trc", "at");
#else
    fp = stderr;
#endif

    if(!fp)
        return;

    fprintf(fp, "%s", string);

    if(fp != stderr)
        fclose(fp);
}

/*------------------------------------------------------------------------------

    Function name:  bsdDecodeReturn

    Purpose: Print out decoder return value

------------------------------------------------------------------------------*/
static void printDecodeReturn(i32 retval)
{

    DEBUG_PRINT(("TB: H264MCDecDecode returned: "));
    switch (retval)
    {

    case H264DEC_OK:
        DEBUG_PRINT(("H264DEC_OK\n"));
        break;
    case H264DEC_NONREF_PIC_SKIPPED:
        DEBUG_PRINT(("H264DEC_NONREF_PIC_SKIPPED\n"));
        break;
    case H264DEC_STRM_PROCESSED:
        DEBUG_PRINT(("H264DEC_STRM_PROCESSED\n"));
        break;
    case H264DEC_PIC_RDY:
        DEBUG_PRINT(("H264DEC_PIC_RDY\n"));
        break;
    case H264DEC_PIC_DECODED:
        DEBUG_PRINT(("H264DEC_PIC_DECODED\n"));
        break;
    case H264DEC_PENDING_FLUSH:
        DEBUG_PRINT(("H264DEC_PENDING_FLUSH\n"));
        break;
    case H264DEC_ADVANCED_TOOLS:
        DEBUG_PRINT(("H264DEC_ADVANCED_TOOLS\n"));
        break;
    case H264DEC_HDRS_RDY:
        DEBUG_PRINT(("H264DEC_HDRS_RDY\n"));
        break;
    case H264DEC_STREAM_NOT_SUPPORTED:
        DEBUG_PRINT(("H264DEC_STREAM_NOT_SUPPORTED\n"));
        break;
    case H264DEC_DWL_ERROR:
        DEBUG_PRINT(("H264DEC_DWL_ERROR\n"));
        break;
    case H264DEC_STRM_ERROR:
        DEBUG_PRINT(("H264DEC_STRM_ERROR\n"));
        break;
    case H264DEC_HW_TIMEOUT:
        DEBUG_PRINT(("H264DEC_HW_TIMEOUT\n"));
        break;
    default:
        DEBUG_PRINT(("Other %d\n", retval));
        break;
    }
}

/*------------------------------------------------------------------------------

    Function name:            printH264MCPicCodingType

    Functional description:   Print out picture coding type value

------------------------------------------------------------------------------*/
void printH264MCPicCodingType(u32 picType)
{
    switch (picType)
    {
    case DEC_PIC_TYPE_I:
        printf(" DEC_PIC_TYPE_I");
        break;
    case DEC_PIC_TYPE_P:
        printf(" DEC_PIC_TYPE_P");
        break;
    case DEC_PIC_TYPE_B:
        printf(" DEC_PIC_TYPE_B");
        break;
    default:
        printf("Other %d\n", picType);
        break;
    }
}
