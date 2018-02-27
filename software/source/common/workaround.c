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

#include "workaround.h"
#include "dwl.h"

/*------------------------------------------------------------------------------
    2. External identifiers
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

#define MAGIC_WORD_LENGTH   (8)
#define MB_OFFSET           (4)

static const u8 magicWord[MAGIC_WORD_LENGTH] = "Rosebud\0";


/*------------------------------------------------------------------------------
    4. Module indentifiers
------------------------------------------------------------------------------*/

static u32 GetMbOffset( u32 mbNum, u32 vopWidth );

/*------------------------------------------------------------------------------

   5.1  Function name: GetMbOffset

        Purpose: 

        Input:

        Output:

------------------------------------------------------------------------------*/
u32 GetMbOffset( u32 mbNum, u32 vopWidth)
{
    u32 mbRow, mbCol;
    u32 offset;

    mbRow = mbNum / vopWidth;
    mbCol = mbNum % vopWidth;
    offset = mbRow*16*16*vopWidth + mbCol*16;
    
    return offset;
}

/*------------------------------------------------------------------------------

   5.1  Function name: PrepareStuffingWorkaround

        Purpose: 

        Input:

        Output:

------------------------------------------------------------------------------*/
void StuffMacroblock( u32 mbNum, u8 * pDecOut, u8 *pRefPic, u32 vopWidth,
                     u32 vopHeight )
{

    u32 pixWidth;
    u32 mbRow, mbCol;
    u32 offset;
    u32 lumaSize;
    u8 *pSrc;
    u8 *pDst;
    u32 x, y;

    pixWidth = 16*vopWidth;

    mbRow = mbNum / vopWidth;
    mbCol = mbNum % vopWidth;

    offset = mbRow*16*pixWidth + mbCol*16;
    lumaSize = 256*vopWidth*vopHeight;

    if(pRefPic)
    {
    
        pDst = pDecOut;
        pSrc = pRefPic;

        pDst += offset;
        pSrc += offset;
        /* Copy luma data */
        for( y = 0 ; y < 16 ; ++y )
        {
            for( x = 0 ; x < 16 ; ++x )
            {
                pDst[x] = pSrc[x];
            }
            pDst += pixWidth;
            pSrc += pixWidth;
        }

        /* Chroma data */
        offset = mbRow*8*pixWidth + mbCol*16;
        
        pDst = pDecOut;
        pSrc = pRefPic;

        pDst += lumaSize;
        pSrc += lumaSize;
        pDst += offset;
        pSrc += offset;

        for( y = 0 ; y < 8 ; ++y )
        {
            for( x = 0 ; x < 16 ; ++x )
            {
                pDst[x] = pSrc[x];
            }
            pDst += pixWidth;
            pSrc += pixWidth;
        }
    }
    else
    {
        pDst = pDecOut + offset;
        /* Copy luma data */
        for( y = 0 ; y < 16 ; ++y )
        {
            for( x = 0 ; x < 16 ; ++x )
            {
                i32 tmp;
                if( mbCol )
                    tmp = pDst[x-pixWidth] + pDst[x-1] - pDst[x-pixWidth-1];
                else
                    tmp = pDst[x-pixWidth];
                if( tmp < 0 )           tmp = 0;
                else if ( tmp > 255 )   tmp = 255;
                pDst[x] = tmp;
            }
            pDst += pixWidth;
        }

        /* Chroma data */
        offset = mbRow*8*pixWidth + mbCol*16;
        
        pDst = pDecOut + lumaSize + offset;

        for( y = 0 ; y < 8 ; ++y )
        {
            for( x = 0 ; x < 16 ; ++x )
            {
                i32 tmp;
                if( mbCol )
                    tmp = pDst[x-pixWidth] + pDst[x-2] - pDst[x-pixWidth-2];
                else
                    tmp = pDst[x-pixWidth];
                if( tmp < 0 )           tmp = 0;
                else if ( tmp > 255 )   tmp = 255;
                pDst[x] = tmp;
            }
            pDst += pixWidth;
        }
    }
}

/*------------------------------------------------------------------------------

   5.1  Function name: PrepareStuffingWorkaround

        Purpose: 

        Input:

        Output:

------------------------------------------------------------------------------*/
void PrepareStuffingWorkaround( u8 *pDecOut, u32 vopWidth, u32 vopHeight )
{

    u32 i;
    u8 * pBase;

    pBase = pDecOut + GetMbOffset(vopWidth*vopHeight - MB_OFFSET, vopWidth);

    for( i = 0 ; i < MAGIC_WORD_LENGTH ; ++i )
        pBase[i] = magicWord[i];
    
}

/*------------------------------------------------------------------------------

   5.1  Function name: ProcessStuffingWorkaround

        Purpose: Check bytes written in PrepareStuffingWorkaround(). If bytes
                 match magic word, then error happened earlier on in the picture.
                 If bytes mismatch, then HW got to end of picture and error
                 interrupt is most likely because of faulty stuffing. In this
                 case we either conceal tail end of the frame or copy it from
                 previous frame.

        Input:

        Output:
            HANTRO_TRUE     
            HANTRO_FALSE    

------------------------------------------------------------------------------*/
u32  ProcessStuffingWorkaround( u8 * pDecOut, u8 * pRefPic, u32 vopWidth, 
                                u32 vopHeight )
{

    u32 i;
    u8 * pBase;
    u32 numMbs;
    u32 match = HANTRO_TRUE;

    numMbs = vopWidth*vopHeight;

    pBase = pDecOut + GetMbOffset(numMbs - MB_OFFSET, vopWidth);

    for( i = 0 ; i < MAGIC_WORD_LENGTH && match ; ++i )
        if( pBase[i] != magicWord[i] )
            match = HANTRO_FALSE;

    /* If 4th last macroblock is overwritten, then assume it's a stuffing 
     * error. Copy remaining three macroblocks from previous ref frame. */
    if( !match )
    {
        for ( i = 1+numMbs - MB_OFFSET ; i < numMbs ; ++i )
        {
            StuffMacroblock( i, pDecOut, pRefPic, vopWidth, vopHeight );
        }
    }

    return match ? HANTRO_FALSE : HANTRO_TRUE;
    
}

/*------------------------------------------------------------------------------

   5.1  Function name: ProcessStuffingWorkaround

        Purpose: 

        Input:

        Output:

------------------------------------------------------------------------------*/
void InitWorkarounds(u32 decMode, workaround_t *pWorkarounds)
{
    u32 asicId = DWLReadAsicID();
    u32 asicVer = asicId >> 16;
    u32 asicBuild = asicId & 0xFFFF;

    /* set all workarounds off by default */
    pWorkarounds->mpeg.stuffing = HANTRO_FALSE;
    pWorkarounds->mpeg.startCode = HANTRO_FALSE;
    pWorkarounds->rv.multibuffer = HANTRO_FALSE;

    /* exception; set h264 frameNum workaround on by default */
    if (decMode == 0)
        pWorkarounds->h264.frameNum = HANTRO_TRUE;

    /* 8170 decoder does not support bad stuffing bytes. */
    if( asicVer == 0x8170U)
    {
        if( decMode == 5 || decMode == 6 || decMode == 1)
            pWorkarounds->mpeg.stuffing = HANTRO_TRUE;
        else if ( decMode == 8)
            pWorkarounds->rv.multibuffer = HANTRO_TRUE;
    }
    else if( asicVer == 0x8190U )
    {
        switch(decMode)
        {
            case 1: /* MPEG4 */
                if( asicBuild < 0x2570 )
                    pWorkarounds->mpeg.stuffing = HANTRO_TRUE;
                break;
            case 2: /* H263 */
                /* No HW tag supports this */     
                pWorkarounds->mpeg.stuffing = HANTRO_TRUE;
                break;
            case 5: /* MPEG2 */
            case 6: /* MPEG1 */
                if( asicBuild < 0x2470 )
                    pWorkarounds->mpeg.stuffing = HANTRO_TRUE;
                break;
            case 8: /* RV */
                pWorkarounds->rv.multibuffer = HANTRO_TRUE;
        }
    }
    else if(asicVer == 0x9170U)
    {
        if( decMode == 8 && asicBuild < 0x1270 )
            pWorkarounds->rv.multibuffer = HANTRO_TRUE;
    }
    else if(asicVer == 0x9190U)
    {
        if( decMode == 8 && asicBuild < 0x1460 )
            pWorkarounds->rv.multibuffer = HANTRO_TRUE;
    }
    else if(asicVer == 0x6731U) /* G1 */
    {
        if( decMode == 8 && asicBuild < 0x1070 )
            pWorkarounds->rv.multibuffer = HANTRO_TRUE;
        if (decMode == 0 && asicBuild >= 0x2390)
            pWorkarounds->h264.frameNum = HANTRO_FALSE;
    }
    if (decMode == 5 /*MPEG2*/ )
        pWorkarounds->mpeg.startCode = HANTRO_TRUE;

}

/*------------------------------------------------------------------------------

   5.1  Function name: PrepareStartCodeWorkaround

        Purpose: Prepare for start code workaround checking; write magic word
            to last 8 bytes of the picture (frame or field)

        Input:

        Output:

------------------------------------------------------------------------------*/
void PrepareStartCodeWorkaround( u8 *pDecOut, u32 vopWidth, u32 vopHeight,
    u32 topField, u32 dpbMode)
{

    u32 i;
    u8 * pBase;
 
    pBase = pDecOut + vopWidth*vopHeight*256 - 8;
    if (topField)
    {
        if(dpbMode == DEC_DPB_FRAME )
            pBase -= 16*vopWidth;
        else if (dpbMode == DEC_DPB_INTERLACED_FIELD )
            pBase -= 128*vopWidth*vopHeight;
    }

    for( i = 0 ; i < MAGIC_WORD_LENGTH ; ++i )
        pBase[i] = magicWord[i];
    
}

/*------------------------------------------------------------------------------

   5.1  Function name: ProcessStartCodeWorkaround

        Purpose: Check bytes written in PrepareStartCodeWorkaround(). If bytes
                 match magic word, then error happened earlier on in the picture.
                 If bytes mismatch, then HW got to end of picture and timeout
                 interrupt is most likely because of corrupted startcode. In
                 this case we just ignore timeout.

                 Note: in addition to ignoring timeout, SW needs to find
                 next start code as HW does not update stream end pointer
                 properly. Methods of searching next startcode are mode
                 specific and cannot be done here.

        Input:

        Output:
            HANTRO_TRUE     
            HANTRO_FALSE    

------------------------------------------------------------------------------*/
u32  ProcessStartCodeWorkaround( u8 * pDecOut, u32 vopWidth, u32 vopHeight,
    u32 topField, u32 dpbMode)
{

    u32 i;
    u8 * pBase;
    u32 match = HANTRO_TRUE;
    
    pBase = pDecOut + vopWidth*vopHeight*256 - 8;
    if (topField)
    {
        if(dpbMode == DEC_DPB_FRAME )
            pBase -= 16*vopWidth;
        else if (dpbMode == DEC_DPB_INTERLACED_FIELD )
            pBase -= 128*vopWidth*vopHeight;
    }

    for( i = 0 ; i < MAGIC_WORD_LENGTH && match ; ++i )
        if( pBase[i] != magicWord[i] )
            match = HANTRO_FALSE;

    return match ? HANTRO_FALSE : HANTRO_TRUE;
    
}
