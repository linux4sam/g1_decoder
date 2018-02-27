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
#include "decapicommon.h"
#include "dwl_hw_core_array.h"

#include <assert.h>
#include <semaphore.h>
#include <stdlib.h>

#ifdef CORES
#if CORES > MAX_ASIC_CORES
#error Too many cores! Check max number of cores in <decapicommon.h>
#else
#define HW_CORE_COUNT       CORES
#endif
#else
#define HW_CORE_COUNT       1
#endif

typedef struct hw_core_container_
{
    core core_;
} hw_core_container;

typedef struct hw_core_array_instance_
{
    u32 num_of_cores_;
    hw_core_container* cores_;
    sem_t core_lock_;
    sem_t core_rdy_;
} hw_core_array_instance;

hw_core_array initialize_core_array()
{
    u32 i;
    hw_core_array_instance* array = malloc(sizeof(hw_core_array_instance));
    array->num_of_cores_ = get_core_count();
    sem_init(&array->core_lock_, 0, array->num_of_cores_);

    sem_init(&array->core_rdy_, 0, 0);

    array->cores_ = calloc(array->num_of_cores_, sizeof(hw_core_container));
    assert(array->cores_);
    for (i = 0; i < array->num_of_cores_; i++)
    {
        array->cores_[i].core_ = hw_core_init();
        assert(array->cores_[i].core_);

        hw_core_setid(array->cores_[i].core_, i);
        hw_core_set_hw_rdy_sem(array->cores_[i].core_, &array->core_rdy_);
    }
    return array;
}

void release_core_array(hw_core_array inst)
{
    u32 i;

    hw_core_array_instance *array = (hw_core_array_instance *)inst;

    /* TODO(vmr): Wait for all cores to finish. */
    for (i = 0; i < array->num_of_cores_; i++)
    {
        hw_core_release(array->cores_[i].core_);
    }

    free(array->cores_);
    sem_destroy(&array->core_lock_);
    sem_destroy(&array->core_rdy_);
    free(array);
}

core borrow_hw_core(hw_core_array inst)
{
    u32 i = 0;
    hw_core_array_instance* array = (hw_core_array_instance*)inst;

    sem_wait(&array->core_lock_);

    while(!hw_core_try_lock(array->cores_[i].core_))
    {
        i++;
    }

    return array->cores_[i].core_;
}

void return_hw_core(hw_core_array inst, core core)
{
    hw_core_array_instance* array = (hw_core_array_instance*)inst;

    hw_core_unlock(core);

    sem_post(&array->core_lock_);
}

u32 get_core_count() {
    /* TODO(vmr): implement dynamic mechanism for calculating. */
    return HW_CORE_COUNT;
}

core get_core_by_id(hw_core_array inst, int nth)
{
    hw_core_array_instance* array = (hw_core_array_instance*)inst;

    assert(nth < (int)get_core_count());

    return array->cores_[nth].core_;
}

int wait_any_core_rdy(hw_core_array inst)
{
    hw_core_array_instance* array = (hw_core_array_instance*)inst;

    return sem_wait(&array->core_rdy_);
}

int stop_core_array(hw_core_array inst)
{
    hw_core_array_instance* array = (hw_core_array_instance*)inst;
    return sem_post(&array->core_rdy_);
}
