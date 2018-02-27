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

#include "asic.h"
#include "dwl_hw_core.h"

#include <assert.h>
#include <errno.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct hw_core_
{
    const void* hw_core_;
    int id_;
    int reserved_;
    sem_t hw_enable;
    sem_t *mc_hw_rdy;
    sem_t dec_rdy;
    sem_t pp_rdy;
    int b_dec_ena;
    int b_dec_rdy;
    int b_pp_ena;
    int b_pp_rdy;
    pthread_t core_thread;
    int bStopped;
} hw_core;

static void* run_asic(void* param);

core hw_core_init(void)
{
    pthread_attr_t attr;
    hw_core* core = NULL;

    core = calloc(1, sizeof(struct hw_core_));

    if (core == NULL)
        return NULL;

    core->hw_core_ = HwCoreInit();
    assert(core->hw_core_);

    sem_init(&core->hw_enable, 0, 0);
    sem_init(&core->dec_rdy, 0, 0);
    sem_init(&core->pp_rdy, 0, 0);

    core->b_dec_ena = core->b_pp_ena = 0;
    core->b_dec_rdy = core->b_pp_rdy = 0;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    core->bStopped = 0;

    pthread_create(&core->core_thread, &attr, run_asic, (void*)core);

    pthread_attr_destroy(&attr);

    return core;
}

void hw_core_release(core instance)
{
    hw_core* core = (hw_core*)instance;

    core->bStopped = 1;

    sem_post(&core->hw_enable);

    pthread_join(core->core_thread, NULL);

    HwCoreRelease((void *)core->hw_core_);

    sem_destroy(&core->hw_enable);
    sem_destroy(&core->dec_rdy);
    sem_destroy(&core->pp_rdy);

    free(core);
}

u32* hw_core_get_base_address(core instance)
{
    hw_core* core = (hw_core*)instance;
    return HwCoreGetBase((void *)core->hw_core_);
}

void hw_core_dec_enable(core instance)
{
    hw_core* core = (hw_core*)instance;
    core->b_dec_ena = 1;
    sem_post(&core->hw_enable);
}

void hw_core_pp_enable(core instance, int start)
{
    hw_core* core = (hw_core*)instance;

    volatile u32 * pReg = HwCoreGetBase((void*)core->hw_core_);

    /* check standalone pp enabled */
    if(pReg[60] & 0x02)
    {
        /* dec+pp pipeline enabled, we only wait for dec ready */
        fprintf(stderr, "\nPIPELINE\n");
    }
    else
    {
        core->b_pp_ena = 1;
    }

    /* if standalone PP then start hw core */
    if(start && core->b_pp_ena)
        sem_post(&core->hw_enable);
}

int  hw_core_wait_dec_rdy(core instance)
{
    hw_core* core = (hw_core*)instance;

    return sem_wait(&core->dec_rdy);
}

int hw_core_is_dec_rdy(core instance)
{
    hw_core* core = (hw_core*)instance;

    int rdy = core->b_dec_rdy;

    if(core->b_dec_rdy)
        core->b_dec_rdy = 0;

    return rdy;
}

int hw_core_is_pp_rdy(core instance)
{
    hw_core* core = (hw_core*)instance;
    int rdy = core->b_pp_rdy;

    if(core->b_pp_rdy)
        core->b_pp_rdy = 0;

    return rdy;
}

int  hw_core_wait_pp_rdy(core instance)
{
    hw_core* core = (hw_core*)instance;

    return sem_wait(&core->pp_rdy);
}

int  hw_core_post_dec_rdy(core instance)
{
    hw_core* core = (hw_core*)instance;

    return sem_post(&core->dec_rdy);
}

int  hw_core_post_pp_rdy(core instance)
{
    hw_core* core = (hw_core*)instance;

    return sem_post(&core->pp_rdy);
}

void hw_core_setid(core instance, int id)
{
    hw_core* core = (hw_core*)instance;
    core->id_ = id;
}

int hw_core_getid(core instance)
{
    hw_core* core = (hw_core*)instance;
    return core->id_;
}

void hw_core_set_hw_rdy_sem(core instance, sem_t *rdy)
{
    hw_core* core = (hw_core*)instance;
    core->mc_hw_rdy = rdy;
}

static pthread_mutex_t core_stat_lock = PTHREAD_MUTEX_INITIALIZER;

int hw_core_try_lock(core inst)
{
    hw_core* core = (hw_core*)inst;
    int success = 0;

    pthread_mutex_lock(&core_stat_lock);
    if(!core->reserved_)
    {
        core->reserved_ = 1;
        success = 1;
    }
    pthread_mutex_unlock(&core_stat_lock);

    return success;
}

void hw_core_unlock(core inst)
{
    hw_core* core = (hw_core*)inst;

    pthread_mutex_lock(&core_stat_lock);
    core->reserved_ = 0;
    pthread_mutex_unlock(&core_stat_lock);
}

void* run_asic(void* param)
{
    hw_core* core = (hw_core *)param;

    volatile u32 * pReg = HwCoreGetBase((void*)core->hw_core_);

    while(!core->bStopped)
    {
        while(sem_wait(&core->hw_enable) != 0 && errno == EINTR) {}

        if(core->bStopped) /* thread terminating */
            break;

        runAsic((void*)core->hw_core_);

        /* clear IRQ line; in real life this is done in the ISR! */
        pReg[1] &= ~(1 << 8);  /* decoder IRQ */
        pReg[60] &= ~(1 << 8); /* PP IRQ */

        if(core->b_dec_ena)
            core->b_dec_rdy = 1;
        if(core->b_pp_ena)
            core->b_pp_rdy = 1;

        core->b_dec_ena = 0;
        core->b_pp_ena = 0;

        sem_post(core->mc_hw_rdy);
    }

    return NULL;
}
