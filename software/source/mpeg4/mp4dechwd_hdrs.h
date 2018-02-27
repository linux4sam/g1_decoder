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

#ifndef DECHDRS_H
#define DECHDRS_H

#include "basetype.h"

typedef struct DecHdrs_t
{
    u32 lock;   /* header information lock */
    u32 lastHeaderType;
    u32 profileAndLevelIndication;  /* Visual Object Sequence */
    u32 isVisualObjectIdentifier;   /* Visual Object */
    u32 visualObjectVerid;
    u32 visualObjectPriority;
    u32 visualObjectType;
    u32 videoSignalType;
    u32 videoFormat;
    u32 videoRange;
    u32 colourDescription;
    u32 colourPrimaries;
    u32 transferCharacteristics;
    u32 matrixCoefficients; /* end of Visual Object */
    u32 randomAccessibleVol;    /* start of VOL */
    u32 videoObjectTypeIndication;
    u32 isObjectLayerIdentifier;
    u32 videoObjectLayerVerid;
    u32 videoObjectLayerPriority;
    u32 aspectRatioInfo;
    u32 parWidth;
    u32 parHeight;
    u32 volControlParameters;
    u32 chromaFormat;
    u32 lowDelay;
    u32 vbvParameters;
    u32 firstHalfBitRate;
    u32 latterHalfBitRate;
    u32 firstHalfVbvBufferSize;
    u32 latterHalfVbvBufferSize;
    u32 firstHalfVbvOccupancy;
    u32 latterHalfVbvOccupancy;
    u32 videoObjectLayerShape;
    u32 vopTimeIncrementResolution;
    u32 fixedVopRate;
    u32 fixedVopTimeIncrement;
    u32 videoObjectLayerWidth;
    u32 videoObjectLayerHeight;
    u32 interlaced;
    u32 obmcDisable;
    u32 spriteEnable;
    u32 not8Bit;
    u32 quantType;
    u32 complexityEstimationDisable;
    u32 resyncMarkerDisable;
    u32 dataPartitioned;
    u32 reversibleVlc;
    u32 scalability;
    u32 estimationMethod;
    u32 shapeComplexityEstimationDisable;
    u32 opaque;
    u32 transparent;
    u32 intraCae;
    u32 interCae;
    u32 noUpdate;
    u32 upsampling;
    u32 textureComplexityEstimationSet1Disable;
    u32 intraBlocks;
    u32 interBlocks;
    u32 inter4vBlocks;
    u32 notCodedBlocks;
    u32 textureComplexityEstimationSet2Disable;
    u32 dctCoefs;
    u32 dctLines;
    u32 vlcSymbols;
    u32 vlcBits;
    u32 motionCompensationComplexityDisable;
    u32 apm;
    u32 npm;
    u32 interpolateMcQ;
    u32 forwBackMcQ;
    u32 halfpel2;
    u32 halfpel4;
    u32 version2ComplexityEstimationDisable;
    u32 sadct;
    u32 quarterpel;
    u32 closedGov;
    u32 brokenLink;

    u32 numRowsInSlice;
    u32 rlcTableY, rlcTableC;
    u32 dcTable;
    u32 mvTable;
    u32 skipMbCode;
    u32 flipFlopRounding;
} DecHdrs;

#endif
