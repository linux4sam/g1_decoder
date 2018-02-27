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

SRCS = hantro_rv_test.c pptestbench.c params.c cfg.c frame.c
OBJS    = $(SRCS:.c=.o)

SRCS_API = api_rv_test.c pptestbench.c params.c cfg.c frame.c
OBJS_API    = $(SRCS_API:.c=.o)

INCLUDE = -I../../../source/inc \
          -I../../../source/pp \
	  -I../../../source/rv \
	  -I../../rv/rm_parser/include \
	  -I../../../source/config \
	  -I../../../source/common \
	  -I../external_mode \
	  -I../../../linux/memalloc \
          -I../../common/swhw

DECLIBDIR = -L../../../linux/pp  -L../../../linux/dwl -L../../../linux/rv -L../../rv/rm_parser -L../../common/swhw -L../../../../system/models/golden
DECLIB = libppx170.a libdwlx170.a libdecx170.a librvparser.a

TESTBENCH=pp_rv_pipe
TARGET_ENV=

ifeq ($(USE_MD5SUM), y)
        MD5SUM = -DMD5SUM
endif

ifeq ($(shell uname -m),x86_64)
	ifneq (,$(findstring pclinux,$(MAKECMDGOALS)))
		export M32 = -m32
	endif
	ifneq (,$(findstring api,$(MAKECMDGOALS)))
		export M32 = -m32
	endif
	ifneq (,$(findstring coverage,$(MAKECMDGOALS)))
		export M32 = -m32
	endif
	ifneq (,$(findstring verification,$(MAKECMDGOALS)))
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
versatile: CFLAGS = -O -g -mtune=arm926ej-s -fpic -DPP_PIPELINE_ENABLED $(INCLUDE) $(MD5SUM) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE 
versatile: TARGET_ENV=versatile
versatile: LIBS = $(DECLIBDIR) $(EFENCE) -lpthread -ldecx170p -ldwlx170 -ldecx170rv -ltbcommon -lrvparser
versatile: test

.PHONY: pci
pci: CFLAGS = -O -g -fpic -DPP_PIPELINE_ENABLED $(INCLUDE)
pci: TARGET_ENV=pci
pci: LIBS = $(DECLIBDIR) $(EFENCE) -lpthread -lppx170 -ldwlx170 -ldecx170rv -ltbcommon -lrvparser
pci: test

.PHONY: pclinux
pclinux: CC=gcc
pclinux: CFLAGS += -DASIC_TRACE_SUPPORT
pclinux: CFLAGS += -g -DPP_PIPELINE_ENABLED $(INCLUDE) $(M32) $(MD5SUM) -DEXPIRY_DATE=1$(expiry) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE 
pclinux: TESTBENCH=pp_rv_pipe
pclinux: TARGET_ENV=pclinux
pclinux: LIBS = $(DECLIBDIR) -pthread -ldecx170p -ldwlx170 -ldecx170rv -l8170hw -ltbcommon -lrvparser
pclinux: test

.PHONY: api
api: CC=gcc
api: CFLAGS = -g -DPP_PIPELINE_ENABLED $(INCLUDE) $(M32) $(MD5SUM) -DEXPIRY_DATE=1$(expiry) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE 
api: TESTBENCH=pp_rv_pipe
api: TARGET_ENV=pclinux
api: LIBS = $(DECLIBDIR) -ldecx170p -ldwlx170 -ldecx170rv -l8170hw -ltbcommon -lrvparser
api: test_api


.PHONY: coverage
coverage: CC=covc --abs --retain gcc
coverage: CFLAGS = -g -DPP_PIPELINE_ENABLED $(INCLUDE) $(M32) $(MD5SUM) -DEXPIRY_DATE=1$(expiry)
coverage: TESTBENCH=pp_rv_pipe
coverage: TARGET_ENV=coverage
coverage: LIBS = $(DECLIBDIR) -ldecx170p -ldwlx170 -ldecx170rv -l8170hw -ltbcommon -lrvparser
coverage: test

test: lib dwl $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(TESTBENCH)

test_api: lib dwl $(OBJS_API)
	$(CC) $(CFLAGS) $(OBJS_API) $(LIBS) -o $(TESTBENCH)

lib:
	make -C ../../../linux/pp clean
	make -C ../../../linux/pp $(TARGET_ENV) PIPELINE_SUPPORT=-DPP_RVDEC_PIPELINE_SUPPORT IS_8190=1
	make -C ../../../linux/rv $(TARGET_ENV)
	make -C ../../rv/rm_parser $(TARGET_ENV)
	make -C ../../common/swhw $(TARGET_ENV)
	make -C ../../common/utils
	
dwl:
	make -C ../../../linux/dwl $(TARGET_ENV)

clean:
	rm -f *.o
	rm -f .depend
	rm -f $(TESTBENCH)

libclean:
	make -C ../../../linux/pp clean
	make -C ../../../linux/dwl clean
	make -C ../../../linux/rv clean
	make -C ../../rv/rm_parser clean
	make -C ../../common/swhw clean
	make -C ../../common/utils clean

depend .depend dep: $(SRCS)
	$(CC) $(CFLAGS) -M  $^ > .depend

ifeq (.depend, $(wildcard .depend))
include .depend
endif
