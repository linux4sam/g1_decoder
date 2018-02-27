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

#include <stdlib.h>
#include <string.h>

#include "tb_tiled.h"
#include "tb_md5.h"


void TbWriteTiledOutput(FILE *file, u8 *data, u32 mbWidth, u32 mbHeight, 
        u32 picNum, u32 md5SumOutput, u32 inputFormat)
{

/* Variables */

    u8 *pTiledYuv = NULL;
    u8 *planarData = NULL;
    u8 *pOutPixel = NULL;
    u8 *pRead = NULL;
    u32 lineStartOffset = 0;
    u32 row = 0, col = 0;
    u32 i, line;
    u32 width = mbWidth << 4;
    u32 height = mbHeight << 4;
    u32 cbOffset = 0, crOffset = 0;
    u32 lumaPels, chPels, mbLumWidth, mbLumHeight, mbChWidth, mbChHeight;
    
    u8 *pFrame = NULL;

    if (file == NULL) {
        return;
    }

    pTiledYuv = (u8 *) malloc((width*height*3)/2);
    
    if(pTiledYuv == NULL) {
        return;
    }

    if(md5SumOutput) {
        pFrame = pTiledYuv;
    }

    pOutPixel = pTiledYuv;
    
    /* convert semi-planar to planar */
    if (inputFormat) 
    {
        u8* planarDataBase;
        u32 tmp = width*height;
        
        planarData = (u8*) malloc((width*height*3)/2);
        planarDataBase = planarData;

        if(planarData == NULL) {
            free(pTiledYuv);
            return;
        }       
     
        memcpy(planarData, data, tmp);
        planarData += tmp;
                
        for(i = 0; i < tmp / 4; i++)
        {
            memcpy(planarData, data + tmp + i * 2, 1);
            ++planarData;
        }
                    
        for(i = 0; i < tmp / 4; i++)
        {
            memcpy(planarData, data + tmp + 1 + i * 2, 1);
            ++planarData;
        }
        
        planarData = planarDataBase;
        data = planarData;
    }

/* Code */    

    lumaPels = 256;
    chPels = 64;
    mbLumWidth = 16;
    mbLumHeight = 16;
    mbChWidth = 8;
    mbChHeight = 8;
    /* Luma */
    /* loop through MB rows */
    for(row=0; row<mbHeight; row++)
    {
        /* loop through mbs in a MB row */
        for(col=0; col<mbWidth; col++)
        {
            /* MB start location */
            pRead = data + (row * mbWidth * lumaPels) + (col * mbLumWidth);

            /* loop through all the lines of the MB */
            for(line=0; line <  mbLumHeight; line++)
            {
                /* offset for each line inside the 
                 * MB from the start of the MB */
                lineStartOffset = (mbLumWidth * mbWidth * line);

                /* Write pels in a line to the tiled temp picture */
                for(i=0;i < mbLumWidth; i++)
                {
                                    /*mboffs + line inside MB  + this pixel*/
                    *(pOutPixel++) = *(pRead + lineStartOffset + i);
                }
            }
        }
    }
    
    /* Chroma */
    /* Cb start offset */
    cbOffset = width * height;
    crOffset = cbOffset + (cbOffset>>2);

    /* loop through MB rows */
    for(row=0; row<mbHeight; row++)
    {
        /* loop through mbs in a MB row */
        for(col=0; col<mbWidth; col++)
        {
            /* MB start location */
            pRead =  data + (row * mbWidth * chPels) + (col * mbChWidth);

            /* loop through all the lines of the MB */
            for(line=0; line <  mbChHeight; line++)
            {
                /* offset for each line inside the 
                 * MB from the start of the MB */
                lineStartOffset = (mbChWidth * mbWidth * line);

                /* Write pels in a line to the tiled temp picture */
                for(i=0;i < mbChWidth; i++)
                {
                    /* Write Cb */
                                    /*mboffs + line inside MB  + this pixel*/
                    *(pOutPixel++) = *(pRead + cbOffset + lineStartOffset + i);
                    /* Write Cr */
                    *(pOutPixel++) = *(pRead + crOffset + lineStartOffset + i);
                }
            }
        }
    }

    pRead = pTiledYuv;

    if(md5SumOutput) 
    {
        TBWriteFrameMD5Sum(file, pFrame, (width*height*3)/2, picNum);
    }
    else
    {
        /* Write temp frame out */
        fwrite(pRead, sizeof(u8), (width*height*3)/2, file);
    }

    free(pTiledYuv);
    if (planarData != NULL)
    {
        free(planarData);
    }
}

void TbChangeEndianess(u8 *data, u32 dataSize)
{
    u32 x = 0;
    for(x = 0; x < dataSize; x += 4)
    {
        u32 tmp = 0;
        u32 *word = (u32 *) (data + x);

        tmp |= (*word & 0xFF) << 24;
        tmp |= (*word & 0xFF00) << 8;
        tmp |= (*word & 0xFF0000) >> 8;
        tmp |= (*word & 0xFF000000) >> 24;
        *word = tmp;
    }
}

/*------------------------------------------------------------------------------

    Function name: TBTiledToRaster

        Functional description:
            test bench common function to convert tiled mode ref picture to
            raster scan picture

        Inputs:

        Outputs:

        Returns: 

------------------------------------------------------------------------------*/
void TBTiledToRaster( u32 tileMode, u32 dpbMode, u8 *pIn, u8 *pOut, u32 picWidth, 
                    u32 picHeight )
{
    u32 i, j;
    u32 k, l;
    u32 s, t;

    const u32 tileWidth = 8, tileHeight = 4;
    
    if( tileMode == 1 )
    {
        if(dpbMode == 0 )
        {
            for( i = 0 ; i < picHeight ; i += tileHeight )
            {
                t = 0;
                s = 0;
                for( j = 0 ; j < picWidth ; j += tileWidth )
                {
                    /* copy this tile */
                    for( k = 0 ; k < tileHeight ; ++k )
                    {
                        for( l = 0 ; l < tileWidth ; ++l )
                        {
                            pOut[ k*picWidth + l + s ] = pIn[t++];
                        }
                    }

                    /* move to next horizontal tile */
                    s += tileWidth;
                }
                pOut += picWidth * tileHeight;
                pIn += picWidth * tileHeight;
            }
        }
        else if ( dpbMode == 1 )
        {
            u8  *pOutBot = pOut + picWidth;
            u8  *pInBot = pIn + (picWidth * picHeight / 2);
            /* 1st field */
            for( i = 0 ; i < picHeight/2 ; i += tileHeight )
            {
                t = 0;
                s = 0;
                for( j = 0 ; j < picWidth ; j += tileWidth )
                {
                    /* copy this tile */
                    for( k = 0 ; k < tileHeight ; ++k )
                    {
                        for( l = 0 ; l < tileWidth ; ++l )
                        {
                            pOut[ k*2*picWidth + l + s ] = pIn[t++];
                        }
                    }

                    /* move to next horizontal tile */
                    s += tileWidth;
                }
                pOut += picWidth * tileHeight * 2;
                pIn += picWidth * tileHeight;
            }
            /* 2nd field */
            pOut = pOutBot;
            pIn = pInBot;
            for( i = 0 ; i < picHeight/2 ; i += tileHeight )
            {
                t = 0;
                s = 0;
                for( j = 0 ; j < picWidth ; j += tileWidth )
                {
                    /* copy this tile */
                    for( k = 0 ; k < tileHeight ; ++k )
                    {
                        for( l = 0 ; l < tileWidth ; ++l )
                        {
                            pOut[ k*2*picWidth + l + s ] = pIn[t++];
                        }
                    }

                    /* move to next horizontal tile */
                    s += tileWidth;
                }
                pOut += picWidth * tileHeight * 2;
                pIn += picWidth * tileHeight;
            }
        }
    }
     
}

/*------------------------------------------------------------------------------

    Function name: TBFieldDpbToFrameDpb

        Functional description:
            test bench common function to convert tiled mode ref picture to
            raster scan picture

        Inputs:

        Outputs:

        Returns: 

------------------------------------------------------------------------------*/
void TBFieldDpbToFrameDpb( u32 dpbMode, u8 *pIn, u8 *pOut, u32 monochrome,
                           u32 picWidth, u32 picHeight )
{
    u32 i, j;
    u32 k, l;
    u32 s, t;

    if ( dpbMode == 1 )
    {
        u8  *pOutTop = pOut;
        u8  *pOutBot = pOut + picWidth;
        u8  *pInTop = pIn;
        u8  *pInBot = pIn + (picWidth * picHeight / 2);

        /* Y */
        for( i = 0 ; i < picHeight/2 ; i += 1 )
        {
            memcpy(pOutTop, pInTop, picWidth);
            memcpy(pOutBot, pInBot, picWidth);
            pInTop+=picWidth;
            pInBot+=picWidth;

            pOutTop += 2*picWidth;
            pOutBot += 2*picWidth;
        }

        if(monochrome)
            return;

        /* Cr */
        pOutTop = pOut + picWidth*picHeight;
        pOutBot = pOutTop + picWidth;

        pInTop = pIn + picWidth*picHeight;
        pInBot = pInTop + (picWidth * picHeight / 4);

        for( i = 0 ; i < picHeight/4 ; i += 1 )
        {
            memcpy(pOutTop, pInTop, picWidth);
            memcpy(pOutBot, pInBot, picWidth);
            pInTop+=picWidth;
            pInBot+=picWidth;

            pOutTop += 2*picWidth;
            pOutBot += 2*picWidth;
        }
    }
     
}

