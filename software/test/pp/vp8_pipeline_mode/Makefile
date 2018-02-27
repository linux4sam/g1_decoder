#------------------------------------------------------------------------------
#-       Copyright (c) 2015-2017, VeriSilicon Inc. All rights reserved        --
#-         Copyright (c) 2011-2014, Google Inc. All rights reserved.          --
#-         Copyright (c) 2007-2010, Hantro OY. All rights reserved.           --
#-                                                                            --
#- This software is confidential and proprietary and may be used only as      --
#-   expressly authorized by VeriSilicon in a written licensing agreement.    --
#-                                                                            --
#-         This entire notice must be reproduced on all copies                --
#-                       and may not be removed.                              --
#-                                                                            --
#-------------------------------------------------------------------------------
#- Redistribution and use in source and binary forms, with or without         --
#- modification, are permitted provided that the following conditions are met:--
#-   * Redistributions of source code must retain the above copyright notice, --
#-       this list of conditions and the following disclaimer.                --
#-   * Redistributions in binary form must reproduce the above copyright      --
#-       notice, this list of conditions and the following disclaimer in the  --
#-       documentation and/or other materials provided with the distribution. --
#-   * Neither the names of Google nor the names of its contributors may be   --
#-       used to endorse or promote products derived from this software       --
#-       without specific prior written permission.                           --
#-------------------------------------------------------------------------------
#- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"--
#- AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE  --
#- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE --
#- ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE  --
#- LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR        --
#- CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF       --
#- SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS   --
#- INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN    --
#- CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)    --
#- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE --
#- POSSIBILITY OF SUCH DAMAGE.                                                --
#-------------------------------------------------------------------------------
#-----------------------------------------------------------------------------*/

SRCS = dectestbench.c ivf.c vp8filereader.c pptestbench.c params.c cfg.c frame.c
OBJS    = $(SRCS:.c=.o)

INCLUDE = -I../../../source/inc \
          -I../../../source/pp \
	  -I../../../source/vp8 \
	  -I../../../source/config \
	  -I../../../source/common \
	  -I../external_mode \
	  -I../../../linux/memalloc \
          -I../../common/swhw \
	  -I../../vp8 \
					-I../../common/utils

LIBDECDIR = -L../../../linux/pp \
            -L../../../linux/dwl \
            -L../../../linux/vp8 \
            -L../../common/swhw \
	    -L../../../../system/models/golden \
						-L../../common/utils
            
TESTBENCH=pp_vp8_pipe

TARGET_ENV=

ifeq ($(USE_MD5SUM), y)
        MD5SUM = -DMD5SUM
endif

ifeq ($(ENABLE_WEBM), y)
	WEBM = -DWEBM_ENABLED
	SRCS += nestegg/src/nestegg.c nestegg/halloc/src/halloc.c
 	INCLUDE+=-Inestegg/include -Inestegg/halloc -I.
	DIRCHECK = dircheck
endif
	
ifeq ($(shell uname -m),x86_64)
	ifneq (,$(findstring pclinux,$(MAKECMDGOALS)))
		export M32 = -m32
	endif
	ifneq (,$(findstring coverage,$(MAKECMDGOALS)))
		export M32 = -m32
	endif
endif

export ASIC_TRACE_SUPPORT
ifeq ($(ASIC_TRACE_SUPPORT), y)
	CFLAGS+=-DASIC_TRACE_SUPPORT
endif

all: versatile

.PHONY: versatile
versatile: CC=arm-none-linux-gnueabi-gcc
versatile: CFLAGS = -O0 -g -mtune=arm926ej-s -fpic -DPP_PIPELINE_ENABLED $(INCLUDE) $(MD5SUM) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE $(WEBM)
versatile: TARGET_ENV=versatile
versatile: DWL_TARGET_ENV=versatile
versatile: LIBS = $(LIBDECDIR) $(EFENCE) -lpthread -ldecx170p -ldwlx170 -ldecx170vp8 -ltbcommon -lutils
versatile: test

.PHONY: pci
pci: CFLAGS = -O0 -g -fpic -DPP_PIPELINE_ENABLED $(INCLUDE) $(WEBM)
pci: TARGET_ENV=pci
pci: DWL_TARGET_ENV=pci
pci: LIBS = $(LIBDECDIR) $(EFENCE) -lpthread -lppx170 -ldwlx170 -ldecx170vp8 -ltbcommon -lutils
pci: test

.PHONY: pclinux
pclinux: CC=gcc
pclinux: CFLAGS += -DASIC_TRACE_SUPPORT
pclinux: CFLAGS += $(M32) -g -O0 -DPP_PIPELINE_ENABLED $(INCLUDE) $(MD5SUM) -DEXPIRY_DATE=1$(expiry) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE $(WEBM) 
pclinux: TARGET_ENV=pclinux
pclinux: LIBS = $(LIBDECDIR) -pthread -ldecx170p -ldwlx170 -ldecx170vp8 -l8170hw -ltbcommon -lutils
pclinux: test

test: $(DIRCHECK) lib $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(TESTBENCH)

.PHONY: dircheck
dircheck:
	@ if ! test -d ./nestegg; then\
		echo "";\
		echo "In order to enable WebM support, nestegg library must be installed:";\
		echo "";\
		echo "git clone https://github.com/kinetiknz/nestegg.git";\
		echo "cd nestegg";\
		echo "autoreconf --install";\
		echo "./configure";\
		echo "cd ..";\
		echo "";\
		echo "Note: nestegg configuration uses autoconf and libtool. These can be downloaded from:";\
		echo "";\
		echo "  - http://www.gnu.org/s/autoconf/";\
		echo "  - http://www.gnu.org/software/libtool/";\
		echo "";\
		exit 1;\
	fi

lib:
	make -C ../../../linux/pp clean
	make -C ../../../linux/pp $(TARGET_ENV) PIPELINE_SUPPORT=-DPP_VP8DEC_PIPELINE_SUPPORT
	make -C ../../../linux/vp8 $(TARGET_ENV)
	make -C ../../../linux/dwl $(TARGET_ENV)
	make -C ../../common/swhw $(TARGET_ENV)
	make -C ../../common/utils CC=$(CC)

clean:
	rm -f *.o core.*
	rm -f .depend
	rm -f $(TESTBENCH)

libclean:
	make -C ../../../linux/pp clean
	make -C ../../../linux/dwl clean
	make -C ../../../linux/vp8 clean
	make -C ../../common/swhw clean
	make -C ../../common/utils clean

depend .depend dep: $(SRCS)
	$(CC) $(CFLAGS) -M  $^ > .depend

ifeq (.depend, $(wildcard .depend))
include .depend
endif
