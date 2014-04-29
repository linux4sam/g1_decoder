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

SRCS = dectestbench.c mcdectestbench.c libav-wrapper.c bytestream_parser.c

OBJS    = $(SRCS:.c=.o)

INCLUDE = -I../../source/h264high -I../../source/inc -I../../source/config \
          -I../../source/h264high/legacy -I../../source/dwl -I../common/swhw \
          -I../../source/common -I../../linux/h264high

DECLIBDIR = -L../../linux/h264high \
            -L../../linux/dwl \
            -L../common \
            -L../common/swhw \
            -L../../../system/models/golden \
            -L../common/utils

# libav stuff
ifneq (,$(findstring pclinux,$(MAKECMDGOALS)))
	# i386 32-bits libav
	SYSROOT=/auto/hantro-projects/sw-codecs/libav/i386
else
	# arm libav
	SYSROOT=../../../../libav/_install
endif
PKG_CONFIG_SYSROOT_DIR=$(SYSROOT)
PKG_CONFIG_LIBDIR="$(SYSROOT)/lib/pkgconfig"

LIBAV_CFLAGS=$(shell PKG_CONFIG_DIR="$(PKG_CONFIG_SYSROOT_DIR)" PKG_CONFIG_LIBDIR="$(PKG_CONFIG_LIBDIR)" pkg-config --cflags libavcodec libavformat libavutil)
LIBAV_LIBS=$(shell PKG_CONFIG_DIR="$(PKG_CONFIG_SYSROOT_DIR)" PKG_CONFIG_LIBDIR="$(PKG_CONFIG_LIBDIR)" pkg-config --libs libavcodec libavformat libavutil)

DECLIB = libdecx170h.a
DWLLIB = libdwlx170.a
TBLIB = libtbcommon.a
HWSIMLIB = lib8170hw.a

TESTDEC=hx170dec
TARGET_ENV=integrator

CC=$(CROSS)gcc

# Enable SW performance  measurement
#CFLAGS = -DSW_PERFORMANCE

# code does not support 64-bits!!!
ifeq ($(shell uname -m),x86_64)
	ifneq (,$(findstring pclinux,$(MAKECMDGOALS)))
		M32 = -m32
	endif
	ifneq (,$(findstring coverage,$(MAKECMDGOALS)))
		M32 = -m32
	endif
	ifneq (,$(findstring verification,$(MAKECMDGOALS)))
		M32 = -m32
	endif
endif

ifeq ($(ENABLE_2ND_CHROMA_FLAG), y)
	ENABLE_2ND_CHROMA=-D_ENABLE_2ND_CHROMA
endif

ifeq ($(DISABLE_PIC_FREEZE_FLAG), y)
	DISABLE_PIC_FREEZE=-D_DISABLE_PIC_FREEZE
endif

export ASIC_TRACE_SUPPORT
ifeq ($(ASIC_TRACE_SUPPORT), y)
	CFLAGS+=-DASIC_TRACE_SUPPORT
endif

ifneq ($(TB_WATCHDOG), )
	CFLAGS+=-DTESTBENCH_WATCHDOG
endif

export M32
export SET_EMPTY_PICTURE_DATA
export ENABLE_2ND_CHROMA
export DISABLE_PIC_FREEZE

all: versatile

.PHONY: libav
libav:
		@echo ""
		@echo "For multicore tesbench libav components must be available!"
		@echo "Sources available from:"
		@echo "git clone git://git.libav.org/libav.git"
		@echo -e "\nlibav x86 configuration:"
		@echo "./configure --enable-shared --disable-static --enable-runtime-cpudetect"
		@echo -e "\nlibav x86_64 configuration:"
		@echo "./configure --cc=\"gcc -m32\" --enable-shared --disable-static --enable-runtime-cpudetect"
		@echo -e "\nlibav ARM configuration:"
		@echo "./configure --cross-prefix=arm-none-linux-gnueabi- --arch=arm --cpu=armv6 --target-os=linux --enable-shared --disable-static --enable-runtime-cpudetect"
		@echo ""

.PHONY: versatile
versatile: TARGET_ENV=versatile
versatile: CROSS=arm-none-linux-gnueabi-
versatile: ARCH=-march=armv6
versatile: arm

.PHONY: linaro
linaro: TARGET_ENV=linaro
linaro: CROSS=arm-linux-gnueabi-
linaro: ARCH=-march=armv7-a
linaro: arm

.PHONY: arm
arm: CFLAGS += -O -g $(ARCH) $(INCLUDE)
arm: CFLAGS += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE
arm: CFLAGS += $(LIBAV_CFLAGS)
arm: LIBS = $(DECLIBDIR) -ldecx170h -ldwlx170  -ltbcommon
arm: LIBS += -pthread
arm: TESTDEC=hx170dec-arm
arm: test

.PHONY: pclinux
pclinux: CFLAGS += $(M32) -O -g $(INCLUDE) $(MD5SUM)
pclinux: CFLAGS += -D_FILE_OFFSET_BITS=64  -D_LARGEFILE64_SOURCE
pclinux: CFLAGS += -DEXPIRY_DATE=1$(expiry)
pclinux: CFLAGS += $(LIBAV_CFLAGS)
pclinux: TESTDEC=hx170dec_pclinux
pclinux: TARGET_ENV=pclinux
pclinux: LIBS = $(DECLIBDIR) -ldecx170h -ldwlx170 -l8170hw -ltbcommon
pclinux: LIBS += -pthread
pclinux: test

.PHONY: pclinux_eval_8170
pclinux_eval_8170: CFLAGS += -DH264_EVALUATION_8170
pclinux_eval_8170: pclinux_eval

.PHONY: pclinux_eval_8190
pclinux_eval_8190: CFLAGS += -DH264_EVALUATION_8190
pclinux_eval_8190: pclinux_eval

.PHONY: pclinux_eval_9170
pclinux_eval_9170: CFLAGS += -DH264_EVALUATION_9170
pclinux_eval_9170: pclinux_eval

.PHONY: pclinux_eval_9190
pclinux_eval_9190: CFLAGS += -DH264_EVALUATION_9190
pclinux_eval_9190: pclinux_eval

.PHONY: pclinux_eval_G1
pclinux_eval_G1: CFLAGS += -DH264_EVALUATION_G1
pclinux_eval_G1: pclinux_eval

.PHONY: pclinux_eval
pclinux_eval: TARGET_ENV=pclinux_eval
pclinux_eval: INCLUDE+=-I$(MP4FF)/x86
pclinux_eval: DECLIBDIR+=-L$(MP4FF)/x86
pclinux_eval: CFLAGS += $(M32) -O $(INCLUDE) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE 
pclinux_eval: CFLAGS +=  -DH264_EVALUATION -DEXPIRY_DATE=1$(expiry)
pclinux_eval: TESTDEC=hx170dec_pclinux_eval
pclinux_eval: LIBS = $(DECLIBDIR) -ldecx170h -ldwlx170 -l8170hw -ltbcommon
pclinux_eval: LIBS += -pthread
pclinux_eval: DEBFLAGS = -O3 -DNDEBUG
pclinux_eval: test

.PHONY: test
test: CFLAGS += $(ENABLE_2ND_CHROMA)
test: lib libav $(OBJS)
	$(CC) $(CFLAGS) dectestbench.o $(LIBS) -o $(TESTDEC)
	$(CC) $(CFLAGS) $(LIBAV_CFLAGS) mcdectestbench.o libav-wrapper.o bytestream_parser.o \
           $(LIBS) $(LIBAV_LIBS) -o $(TESTDEC)-mc

lib:
	make -C ../../linux/h264high $(TARGET_ENV) ARMCPU=$(ARMCPU)
	make -C ../../linux/dwl $(TARGET_ENV) ARMCPU=$(ARMCPU)
	make -C ../common/swhw $(TARGET_ENV) ARMCPU=$(ARMCPU)

clean:
	rm -f *.o
	rm -f .depend
	rm -f hx170dec*

libclean:
	make -C ../../linux/h264high clean
	make -C ../../linux/dwl clean
	make -C ../common/swhw clean

depend .depend dep: $(SRCS)
	$(CC) $(CFLAGS) -M  $^ > .depend

ifeq (.depend, $(wildcard .depend))
include .depend
endif
