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
#include "decapicommon.h"
#include "dwl_linux.h"
#include "dwl.h"
#include "hx170dec.h"
#include "memalloc.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef INTERNAL_TEST
#include "internal_test.h"
#endif

/* the decoder device driver nod */
const char *dec_dev = DEC_MODULE_PATH;

/* the memalloc device driver nod */
const char *mem_dev = MEMALLOC_MODULE_PATH;

/* counters for core usage statistics */
u32 coreUsageCounts[MAX_ASIC_CORES] = {0};

/* a mutex protecting the wrapper init */
static pthread_mutex_t x170_init_mutex = PTHREAD_MUTEX_INITIALIZER;
static int nDwlInstanceCount = 0;

static struct mc_listener_thread_params_ listenerThreadParams = {0};
static pthread_t mc_listener_thread;

extern u32 dwlShadowRegs[MAX_ASIC_CORES][128];

void * thread_mc_listener(void *args)
{
    mc_listener_thread_params *params = (mc_listener_thread_params *) args;

    while (!params->bStopped)
    {
        u32 id;
        struct core_desc core;

#ifdef DWL_USE_DEC_IRQ
        if (ioctl(params->fd, HX170DEC_IOCG_CORE_WAIT, &id))
        {
            DWL_DEBUG("ioctl HX170DEC_IOCG_CORE_WAIT failed\n");
        }

        if (params->bStopped)
            break;

        DWL_DEBUG("DEC IRQ by Core %d\n", id);

        core.id = id;
        core.regs = dwlShadowRegs[id];
        core.size = 60*4;

        if (ioctl(params->fd, HX170DEC_IOCS_DEC_PULL_REG, &core))
        {
            DWL_DEBUG("ioctl HX170DEC_IOCS_DEC_PULL_REG failed\n");
            assert(0);
        }

        if(params->pCallback[id] != NULL)
        {
            params->pCallback[id](params->pCallbackArg[id], id);
        }
        else
        {
            DWL_DEBUG("SINGLE CORE IRQ, core = %d\n", id);
            sem_post(params->sc_dec_rdy_sem + id);
        }
#else
        core.size = 60 * 4;

        for (id = 0; id < params->nDecCores; id++)
        {
            u32 irq_stats;
            const unsigned int usec = 1000; /* 1 ms polling interval */

            /* Skip cores that are not part of multicore, they call directly
             * DWLWaitHwReady(), which does its own polling.
             */
            if (params->pCallback[id] == NULL)
            {
                continue;
            }

            /* Skip not enabled cores also */
            if ((dwlShadowRegs[id][1] & 0x01) == 0)
            {
                continue;
            }

            core.id = id;
            core.regs = dwlShadowRegs[id];

            if (ioctl(params->fd, HX170DEC_IOCS_DEC_PULL_REG, &core))
            {
                DWL_DEBUG("ioctl HX170DEC_IOCS_DEC_PULL_REG failed\n");
                continue;
            }

            irq_stats = dwlShadowRegs[id][1];

            irq_stats = (irq_stats >> 11) & 0xFF;

            if (irq_stats != 0)
            {
                DWL_DEBUG("DEC IRQ by Core %d\n", id);
                params->pCallback[id](params->pCallbackArg[id], id);
            }

            usleep(usec);
        }
#endif
    }

    return NULL;
}

/*------------------------------------------------------------------------------
    Function name   : DWLInit
    Description     : Initialize a DWL instance

    Return type     : const void * - pointer to a DWL instance

    Argument        : void * param - not in use, application passes NULL
------------------------------------------------------------------------------*/
const void *DWLInit(DWLInitParam_t * param)
{
    hX170dwl_t *dec_dwl;
    unsigned long multicoreBase[MAX_ASIC_CORES];
    unsigned int i;

    DWL_DEBUG("INITIALIZE\n");

    dec_dwl = (hX170dwl_t *) calloc(1, sizeof(hX170dwl_t));

    if(dec_dwl == NULL)
    {
        DWL_DEBUG("failed to alloc hX170dwl_t struct\n");
        return NULL;
    }

    dec_dwl->clientType = param->clientType;

    pthread_mutex_lock(&x170_init_mutex);

#ifdef INTERNAL_TEST
    InternalTestInit();
#endif

    dec_dwl->fd = -1;
    dec_dwl->fd_mem = -1;
    dec_dwl->fd_memalloc = -1;

    /* open the device */
    dec_dwl->fd = open(dec_dev, O_RDWR);
    if(dec_dwl->fd == -1)
    {
        DWL_DEBUG("failed to open '%s'\n", dec_dev);
        goto err;
    }

    /* Linear memories not needed in pp */
    if(dec_dwl->clientType != DWL_CLIENT_TYPE_PP)
    {
        /* open memalloc for linear memory allocation */
        dec_dwl->fd_memalloc = open(mem_dev, O_RDWR | O_SYNC);

        if(dec_dwl->fd_memalloc == -1)
        {
            DWL_DEBUG("failed to open: %s\n", mem_dev);
            goto err;
        }
    }

    dec_dwl->fd_mem = open("/dev/mem", O_RDWR | O_SYNC);

    if(dec_dwl->fd_mem == -1)
    {
        DWL_DEBUG("failed to open: %s\n", "/dev/mem");
        goto err;
    }

    switch (dec_dwl->clientType)
    {
    case DWL_CLIENT_TYPE_H264_DEC:
    case DWL_CLIENT_TYPE_MPEG4_DEC:
    case DWL_CLIENT_TYPE_JPEG_DEC:
    case DWL_CLIENT_TYPE_VC1_DEC:
    case DWL_CLIENT_TYPE_MPEG2_DEC:
    case DWL_CLIENT_TYPE_VP6_DEC:
    case DWL_CLIENT_TYPE_VP8_DEC:
    case DWL_CLIENT_TYPE_RV_DEC:
    case DWL_CLIENT_TYPE_AVS_DEC:
    case DWL_CLIENT_TYPE_PP:
        {
            break;
        }
    default:
        {
            DWL_DEBUG("Unknown client type no. %d\n", dec_dwl->clientType);
            goto err;
        }
    }


    if(ioctl(dec_dwl->fd, HX170DEC_IOC_MC_CORES,  &dec_dwl->numCores) == -1)
    {
        DWL_DEBUG("ioctl HX170DEC_IOC_MC_CORES failed\n");
        goto err;
    }

    assert(dec_dwl->numCores <= MAX_ASIC_CORES);

    if(ioctl(dec_dwl->fd, HX170DEC_IOC_MC_OFFSETS, multicoreBase) == -1)
    {
        DWL_DEBUG("ioctl HX170DEC_IOC_MC_OFFSETS failed\n");
        goto err;
    }

    if(ioctl(dec_dwl->fd, HX170DEC_IOCGHWIOSIZE, &dec_dwl->regSize) == -1)
    {
        DWL_DEBUG("ioctl HX170DEC_IOCGHWIOSIZE failed\n");
        goto err;
    }

    /* Allocate the signal handling and cores just once */
    if (!nDwlInstanceCount)
    {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        nDwlInstanceCount++;

        listenerThreadParams.fd = dec_dwl->fd;
        listenerThreadParams.nDecCores = dec_dwl->numCores;
        listenerThreadParams.nPPCores  = 1; /* no multi-core support */

        for (i = 0; i < listenerThreadParams.nDecCores; i++)
        {
            sem_init(listenerThreadParams.sc_dec_rdy_sem + i, 0,0);
            sem_init(listenerThreadParams.sc_pp_rdy_sem + i, 0,0);

            listenerThreadParams.pCallback[i] = NULL;
        }

        listenerThreadParams.bStopped = 0;

        if (pthread_create(&mc_listener_thread, &attr, thread_mc_listener,
                         &listenerThreadParams) != 0)
            goto err;
    }

    dec_dwl->sync_params = &listenerThreadParams;

    DWL_DEBUG("SUCCESS\n");

    pthread_mutex_unlock(&x170_init_mutex);
    return dec_dwl;

  err:

    DWL_DEBUG("FAILED\n");
    pthread_mutex_unlock(&x170_init_mutex);
    DWLRelease(dec_dwl);

    return NULL;
}

/*------------------------------------------------------------------------------
    Function name   : DWLRelease
    Description     : Release a DWl instance

    Return type     : i32 - 0 for success or a negative error code

    Argument        : const void * instance - instance to be released
------------------------------------------------------------------------------*/
i32 DWLRelease(const void *instance)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;
    unsigned int i = 0;

    DWL_DEBUG("RELEASE\n");

    assert(dec_dwl != NULL);

    pthread_mutex_lock(&x170_init_mutex);

    nDwlInstanceCount--;

    /* Release the signal handling and cores just when
     * nobody is referencing them anymore
     */
    if(!nDwlInstanceCount)
    {
        listenerThreadParams.bStopped = 1;
    }

    for (i = 0 ; i < dec_dwl->numCores ; i++)
    {
        sem_destroy(listenerThreadParams.sc_dec_rdy_sem + i);
        sem_destroy(listenerThreadParams.sc_pp_rdy_sem + i);
    }

    if(dec_dwl->fd_mem != -1)
        close(dec_dwl->fd_mem);

    if(dec_dwl->fd != -1)
        close(dec_dwl->fd);

    /* linear memory allocator */
    if(dec_dwl->fd_memalloc != -1)
        close(dec_dwl->fd_memalloc);


    /* print core usage stats */
    if (dec_dwl->clientType != DWL_CLIENT_TYPE_PP)
    {
        u32 totalUsage = 0;
        u32 cores = dec_dwl->numCores;
        for(i = 0; i < cores; i++)
            totalUsage += coreUsageCounts[i];
        /* avoid zero division */
        totalUsage = totalUsage ? totalUsage : 1;

        printf("\nMulti-core usage statistics:\n");
        for(i = 0; i < cores; i++)
            printf("\tCore[%2d] used %6d times (%2d%%)\n",
                    i, coreUsageCounts[i],
                    (coreUsageCounts[i] * 100) / totalUsage );

        printf("\n");
    }

#ifdef INTERNAL_TEST
    InternalTestFinalize();
#endif

    free(dec_dwl);

    pthread_mutex_unlock(&x170_init_mutex);

    DWL_DEBUG("SUCCESS\n");

    return (DWL_OK);
}

/* HW locking */

/*------------------------------------------------------------------------------
    Function name   : DWLReserveHwPipe
    Description     :
    Return type     : i32
    Argument        : const void *instance
    Argument        : i32 *coreID - ID of the reserved HW core
------------------------------------------------------------------------------*/
i32 DWLReserveHwPipe(const void *instance, i32 *coreID)
{
    i32 ret;
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;

    assert(dec_dwl != NULL);
    assert(dec_dwl->clientType != DWL_CLIENT_TYPE_PP);

    DWL_DEBUG("Start\n");

    /* reserve decoder */
    *coreID = ioctl(dec_dwl->fd, HX170DEC_IOCH_DEC_RESERVE,
                    dec_dwl->clientType);

    if (*coreID != 0)
    {
        return DWL_ERROR;
    }

    /* reserve PP */
    ret = ioctl(dec_dwl->fd, HX170DEC_IOCQ_PP_RESERVE);

    /* for pipeline we expect same core for both dec and PP */
    if (ret != *coreID)
    {
        /* release the decoder */
        ioctl(dec_dwl->fd, HX170DEC_IOCT_DEC_RELEASE, coreID);
        return DWL_ERROR;
    }

    coreUsageCounts[*coreID]++;

    dec_dwl->bPPReserved = 1;

    DWL_DEBUG("Reserved DEC+PP core %d\n", *coreID);

    return DWL_OK;
}

/*------------------------------------------------------------------------------
    Function name   : DWLReserveHw
    Description     :
    Return type     : i32
    Argument        : const void *instance
    Argument        : i32 *coreID - ID of the reserved HW core
------------------------------------------------------------------------------*/
i32 DWLReserveHw(const void *instance, i32 *coreID)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;
    int isPP;

    assert(dec_dwl != NULL);

    isPP = dec_dwl->clientType == DWL_CLIENT_TYPE_PP ? 1 : 0;

    DWL_DEBUG(" %s\n", isPP ? "PP" : "DEC");

    if (isPP)
    {
        *coreID = ioctl(dec_dwl->fd, HX170DEC_IOCQ_PP_RESERVE);

        /* PP is single core so we expect a zero return value */
        if (*coreID != 0)
        {
            return DWL_ERROR;
        }
    }
    else
    {
        *coreID = ioctl(dec_dwl->fd, HX170DEC_IOCH_DEC_RESERVE,
                        dec_dwl->clientType);
    }

    /* negative value signals an error */
    if (*coreID < 0)
    {
        DWL_DEBUG("ioctl HX170DEC_IOCS_%s_RESERVE failed, %d\n",
                  isPP ? "PP" : "DEC", *coreID);
        return DWL_ERROR;
    }

    coreUsageCounts[*coreID]++;

    DWL_DEBUG("Reserved %s core %d\n", isPP ? "PP" : "DEC", *coreID);

    return DWL_OK;
}

/*------------------------------------------------------------------------------
    Function name   : DWLReleaseHw
    Description     :
    Return type     : void
    Argument        : const void *instance
------------------------------------------------------------------------------*/
void DWLReleaseHw(const void *instance, i32 coreID)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;
    int isPP;

    assert((u32)coreID < dec_dwl->numCores);
    assert(dec_dwl != NULL);

    isPP = dec_dwl->clientType == DWL_CLIENT_TYPE_PP ? 1 : 0;

    if ((u32) coreID >= dec_dwl->numCores)
    {
        assert(0);
        return;
    }

    DWL_DEBUG(" %s core %d\n", isPP ? "PP" : "DEC", coreID);

    if (isPP)
    {
        assert(coreID == 0);

        ioctl(dec_dwl->fd, HX170DEC_IOCT_PP_RELEASE, coreID);
    }
    else
    {
        if (dec_dwl->bPPReserved)
        {
            /* decoder has reserved PP also => release it */
            DWL_DEBUG("DEC released PP core %d\n", coreID);

            dec_dwl->bPPReserved = 0;

            assert(coreID == 0);

            ioctl(dec_dwl->fd, HX170DEC_IOCT_PP_RELEASE, coreID);
        }

        ioctl(dec_dwl->fd, HX170DEC_IOCT_DEC_RELEASE, coreID);
    }
}

void DWLSetIRQCallback(const void *instance, i32 coreID,
                        DWLIRQCallbackFn *pCallbackFn, void* arg)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;

    dec_dwl->sync_params->pCallback[coreID] = pCallbackFn;
    dec_dwl->sync_params->pCallbackArg[coreID] = arg;
}
