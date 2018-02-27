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

#include "fifo.h"

#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>

/*------------------------------------------------------------------------------
    Module defines
------------------------------------------------------------------------------*/

/* Container for instance. */
typedef struct fifo_instance_
{
    sem_t cs_semaphore;    /* Semaphore for critical section. */
    sem_t read_semaphore;  /* Semaphore for readers. */
    sem_t write_semaphore; /* Semaphore for writers. */
    u32 num_of_slots_;
    u32 num_of_objects_;
    u32 tail_index_;
    fifo_object* nodes_;
} fifo_instance;

fifo_ret fifo_init(u32 num_of_slots, fifo_inst* instance)
{
    fifo_instance* inst = calloc(1, sizeof(fifo_instance));
    if (inst == NULL)
        return FIFO_ERROR_MEMALLOC;
    inst->num_of_slots_ = num_of_slots;
    /* Allocate memory for the objects. */
    inst->nodes_ = calloc(num_of_slots, sizeof(fifo_object));
    if (inst == NULL)
    {
        free(inst);
        return FIFO_ERROR_MEMALLOC;
    }
    /* Initialize binary critical section semaphore. */
    sem_init(&inst->cs_semaphore, 0, 1);
    /* Then initialize the read and write semaphores. */
    sem_init(&inst->read_semaphore, 0, 0);
    sem_init(&inst->write_semaphore, 0, num_of_slots);
    *instance = inst;
    return FIFO_OK;
}

u32 fifo_push(fifo_inst inst, fifo_object object)
{
    u32 ret = 0;
    fifo_instance* instance = (fifo_instance*)inst;
    sem_wait(&instance->write_semaphore);
    sem_wait(&instance->cs_semaphore);
    instance->nodes_[(instance->tail_index_ + instance->num_of_objects_) %
      instance->num_of_slots_] = object;
    instance->num_of_objects_++;
    ret = instance->num_of_objects_;
    sem_post(&instance->cs_semaphore);
    sem_post(&instance->read_semaphore);

    return ret;
}

u32 fifo_pop(fifo_inst inst, fifo_object* object)
{
    u32 ret;
    fifo_instance* instance = (fifo_instance*)inst;
    sem_wait(&instance->read_semaphore);
    sem_wait(&instance->cs_semaphore);
    *object = instance->nodes_[instance->tail_index_ % instance->num_of_slots_];
    instance->tail_index_++;
    instance->num_of_objects_--;
    ret = instance->num_of_objects_;
    sem_post(&instance->cs_semaphore);
    sem_post(&instance->write_semaphore);
    return ret;
}

u32 fifo_count(fifo_inst inst)
{
    u32 count;
    fifo_instance* instance = (fifo_instance*)inst;
    sem_wait(&instance->cs_semaphore);
    count = instance->num_of_objects_;
    sem_post(&instance->cs_semaphore);
    return count;
}

void fifo_release(fifo_inst inst)
{
    fifo_instance* instance = (fifo_instance*)inst;
    assert(instance->num_of_objects_ == 0);
    sem_wait(&instance->cs_semaphore);
    sem_destroy(&instance->cs_semaphore);
    sem_destroy(&instance->read_semaphore);
    sem_destroy(&instance->write_semaphore);
    free(instance->nodes_);
    free(instance);
}

