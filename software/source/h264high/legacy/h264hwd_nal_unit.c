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

#include "h264hwd_nal_unit.h"
#include "h264hwd_util.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    Function name: h264bsdDecodeNalUnit

        Functional description:
            Decode NAL unit header information

        Inputs:
            pStrmData       pointer to stream data structure

        Outputs:
            pNalUnit        NAL unit header information is stored here

        Returns:
            HANTRO_OK       success
            HANTRO_NOK      invalid NAL unit header information

------------------------------------------------------------------------------*/

u32 h264bsdDecodeNalUnit(strmData_t *pStrmData, nalUnit_t *pNalUnit)
{

/* Variables */

    u32 tmp;

/* Code */

    ASSERT(pStrmData);
    ASSERT(pNalUnit);
    ASSERT(pStrmData->bitPosInWord == 0);

    (void)DWLmemset(pNalUnit, 0, sizeof(nalUnit_t));

    /* forbidden_zero_bit (not checked to be zero, errors ignored) */
    tmp = h264bsdGetBits(pStrmData, 1);
    /* Assuming that NAL unit starts from byte boundary => don't have to check
     * following 7 bits for END_OF_STREAM */
    if (tmp == END_OF_STREAM)
        return(HANTRO_NOK);

    tmp = h264bsdGetBits(pStrmData, 2);
    pNalUnit->nalRefIdc = tmp;

    tmp = h264bsdGetBits(pStrmData, 5);
    pNalUnit->nalUnitType = (nalUnitType_e)tmp;

    DEBUG_PRINT(("NAL TYPE %d\n", tmp));

    /* data partitioning NAL units not supported */
    if ( (tmp == NAL_CODED_SLICE_DP_A) ||
         (tmp == NAL_CODED_SLICE_DP_B) ||
         (tmp == NAL_CODED_SLICE_DP_C) )
    {
        ERROR_PRINT(("DP slices not allowed!!!"));
        return(HANTRO_NOK);
    }

    /* nal_ref_idc shall not be zero for these nal_unit_types */
    if ( ( (tmp == NAL_SEQ_PARAM_SET) || (tmp == NAL_PIC_PARAM_SET) ||
           (tmp == NAL_CODED_SLICE_IDR) ) && (pNalUnit->nalRefIdc == 0) )
    {
        ERROR_PRINT(("nal_ref_idc shall not be zero!!!"));
        return(HANTRO_NOK);
    }
    /* nal_ref_idc shall be zero for these nal_unit_types */
    else if ( ( (tmp == NAL_SEI) || (tmp == NAL_ACCESS_UNIT_DELIMITER) ||
                (tmp == NAL_END_OF_SEQUENCE) || (tmp == NAL_END_OF_STREAM) ||
                (tmp == NAL_FILLER_DATA) ) && (pNalUnit->nalRefIdc != 0) )
    {
        ERROR_PRINT(("nal_ref_idc shall be zero!!!"));
        return(HANTRO_NOK);
    }

    if (pNalUnit->nalUnitType == NAL_PREFIX ||
        pNalUnit->nalUnitType == NAL_CODED_SLICE_EXT)
    {
        tmp = h264bsdGetBits(pStrmData, 1);

        if (tmp == END_OF_STREAM)
            return(HANTRO_NOK);

        pNalUnit->svcExtensionFlag = tmp;

        if(tmp == 0)
        {
            /* MVC Annex H*/
            tmp = h264bsdGetBits(pStrmData, 1);
            pNalUnit->nonIdrFlag = tmp;
            tmp = h264bsdGetBits(pStrmData, 6);
            pNalUnit->priorityId = tmp;
            tmp = h264bsdGetBits(pStrmData, 10);
            pNalUnit->viewId = tmp;
            tmp = h264bsdGetBits(pStrmData, 3);
            pNalUnit->temporalId = tmp;
            tmp = h264bsdGetBits(pStrmData, 1);
            pNalUnit->anchorPicFlag = tmp;
            tmp = h264bsdGetBits(pStrmData, 1);
            pNalUnit->interViewFlag = tmp;
            /* reserved_one_bit */
            tmp = h264bsdGetBits(pStrmData, 1);
        }
        else
        {
            /* SVC Annex G*/
            tmp = h264bsdGetBits(pStrmData, 1); /* idr_flag */
            tmp = h264bsdGetBits(pStrmData, 6); /* priority_id */
            tmp = h264bsdGetBits(pStrmData, 1); /* no_inter_layer_pred_flag */
            tmp = h264bsdGetBits(pStrmData, 3); /* dependency_id */
            tmp = h264bsdGetBits(pStrmData, 4); /* quality_id */
            tmp = h264bsdGetBits(pStrmData, 3); /* temporal_id */
            tmp = h264bsdGetBits(pStrmData, 1); /* use_ref_base_pic_flag */
            tmp = h264bsdGetBits(pStrmData, 1); /* discardable_flag */
            tmp = h264bsdGetBits(pStrmData, 1); /* output_flag */
            tmp = h264bsdGetBits(pStrmData, 2); /* reserved_three_2_bits */
        }

        if (tmp == END_OF_STREAM)
            return(HANTRO_NOK);
    }

    return(HANTRO_OK);

}
