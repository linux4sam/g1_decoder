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

SRCS = mpeg4dectest.c rm_length.c
OBJS    = $(SRCS:.c=.o)

INCLUDE = -I../../source/inc -I../../source/mpeg4/ -I../../source/config \
          -I../../source/common  -I../common/swhw

DECLIBDIR = -L../../linux/mpeg4 -L../common/swhw \
            -L../../../system/models/golden -L../../linux/dwl -L../common/utils

DECLIB =  libdecx170m.a
DWLLIB = libdwlx170.a
TBLIB = libtbcommon.a
MODLIB = lib8170hw.a

ALLTESTDECS= mx170dec_versatile mx170dec_integrator \
             mx170dec_ads11 mx170dec_pclinux mx170dec_pclinux_eval \
	     mx170dec_ads9 mx170dec_coverage
TARGET_ENV=

# Enable SW performance  measurement
#CFLAGS = -DSW_PERFORMANCE

ifeq ($(USE_EFENCE), y)
        EFENCE= -DUSE_EFENCE -L/afs/hantro.com/projects/adder/users/mahe/efence_2_4_13 \
                -lefence -lpthread
endif

ifeq ($(USE_MD5SUM), y)
        MD5SUM = -DMD5SUM
endif

ifeq ($(shell uname -m),x86_64)
	ifneq (,$(findstring pclinux,$(MAKECMDGOALS)))
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

CFLAGS+= -Wextra

all: versatile

.PHONY: versatile
versatile: CC=arm-none-linux-gnueabi-gcc
versatile: CFLAGS += -O -g -mtune=arm926ej-s -fpic $(INCLUDE) $(MD5SUM) -D_LARGEFILE64_SOURCE
versatile: TESTDEC=mx170dec_versatile
versatile: TARGET_ENV=versatile
versatile: LIBS = $(DECLIBDIR) -lpthread -ldecx170m -ldwlx170 -ltbcommon -lutils $(EFENCE) $(MD5SUM)
versatile: test


.PHONY: integrator
integrator: CC=arm-linux-gcc
integrator: CFLAGS += -O -g -mtune=arm920tdmi -fpic  $(INCLUDE) $(MD5SUM)
integrator: TESTDEC=mx170dec_integrator
integrator: TARGET_ENV=integrator
integrator: LIBS = $(DECLIBDIR) -lpthread -ldecx170m -ldwlx170 -ltbcommon -lutils $(EFENCE)
integrator: test


.PHONY: ads11
ads11: CC=armcc
ads11: CFLAGS = -g -cpu ARM1136J-S -O2 -Otime --no_inline -DMP4DEC_EXTERNAL_ALLOC_DISABLE $(INCLUDE)
ads11: TESTDEC=mx170dec_ads11
ads11: TARGET_ENV=ads11
ads11: LIBS = -l../../linux/mpeg4/$(DECLIB) -l../../linux/dwl/$(DWLLIB) -l../../../system/models/golden/$(MODLIB) -l../common/swhw/$(TBLIB) -D_ADS
ads11: test

.PHONY: ads9
ads9: CC=armcc
ads9: CFLAGS = -g -cpu ARM926EJ-S -O2 -Otime --no_inline  -DMP4DEC_EXTERNAL_ALLOC_DISABLE $(INCLUDE)
ads9: TESTDEC=mx170dec_ads9
ads9: TARGET_ENV=ads9
ads9: LIBS = -l../../linux/mpeg4/$(DECLIB) -l../../linux/dwl/$(DWLLIB) -l../../../system/models/golden/$(MODLIB) -l../common/swhw/$(TBLIB) -D_ADS
ads9: test

.PHONY: pclinux
pclinux: CC=gcc
pclinux: CFLAGS+= -g $(INCLUDE) $(MD5SUM) $(M32) -D_FILE_OFFSET_BITS=64 -DEXPIRY_DATE=1$(expiry) -D_LARGEFILE64_SOURCE
pclinux: TESTDEC=mx170dec_pclinux
pclinux: TARGET_ENV=pclinux
pclinux: LIBS = $(DECLIBDIR) -pthread -ldecx170m -ldwlx170 -l8170hw -ltbcommon -lutils $(EFENCE)
pclinux: test

.PHONY: verification
verification: SYSTEM_VERIFICATION_TEST = -DSYSTEM_VERIFICATION
verification: CFLAGS += -DSYSTEM_VERIFICATION
verification: pclinux

.PHONY: coverage
coverage: CC=covc --abs --retain gcc
coverage: CFLAGS = -O -g $(INCLUDE) $(MD5SUM) $(M32)
coverage: TESTDEC=mx170dec_coverage
coverage: TARGET_ENV=coverage
coverage: LIBS = $(DECLIBDIR) -ldecx170m -ldwlx170 -l8170hw -ltbcommon -lutils $(EFENCE)
coverage: test

.PHONY: pclinux_eval_8170
pclinux_eval_8170: CFLAGS += -DMPEG4_EVALUATION_8170
pclinux_eval_8170: pclinux_eval

.PHONY: pclinux_eval_8190
pclinux_eval_8190: CFLAGS += -DMPEG4_EVALUATION_8190
pclinux_eval_8190: pclinux_eval

.PHONY: pclinux_eval_9170
pclinux_eval_9170: CFLAGS += -DMPEG4_EVALUATION_9170
pclinux_eval_9170: pclinux_eval

.PHONY: pclinux_eval_9190
pclinux_eval_9190: CFLAGS += -DMPEG4_EVALUATION_9190
pclinux_eval_9190: pclinux_eval

.PHONY: pclinux_eval_G1
pclinux_eval_G1: CFLAGS += -DMPEG4_EVALUATION_G1
pclinux_eval_G1: pclinux_eval

.PHONY: pclinux_eval
pclinux_eval: CC=gcc
pclinux_eval: CFLAGS += $(INCLUDE) $(MD5SUM) $(M32) -DMPEG4_EVALUATION -D_FILE_OFFSET_BITS=64 -DEXPIRY_DATE=1$(expiry)
pclinux_eval: TESTDEC=mx170dec_pclinux_eval
pclinux_eval: TARGET_ENV=pclinux_eval
pclinux_eval: LIBS = $(DECLIBDIR) -ldecx170m -ldwlx170 -l8170hw -ltbcommon -lutils $(EFENCE)
pclinux_eval: DEBFLAGS = -O3 -DNDEBUG
pclinux_eval: test


export SYSTEM_VERIFICATION_TEST
export CUSTOM_FMT_SUPPORT

test: lib  $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(TESTDEC)

lib:
	make -C ../../linux/mpeg4 $(TARGET_ENV)
	make -C ../../linux/dwl $(TARGET_ENV)
	make -C ../common/swhw $(TARGET_ENV)
	make -C ../common/utils

clean:
	rm -f *.o
	rm -f .depend
	rm -f $(ALLTESTDECS)

libclean:
	make -C ../../linux/dwl clean
	make -C ../../linux/mpeg4 clean
	make -C ../common/swhw clean
	make -C ../common/utils clean

depend .depend dep: $(SRCS)
	$(CC) $(CFLAGS) -M  $^ > .depend

ifeq (.depend, $(wildcard .depend))
include .depend
endif
