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

SRCS = fifo.c async_task.c
ASYNC_UTEST_SRCS = async_task_unittest.c
FIFO_UTEST_SRCS = fifo_unittest.c
OBJS = $(SRCS:.c=.o)
FIFO_UTEST_OBJS = $(FIFO_UTEST_SRCS:.c=.o)
ASYNC_UTEST_OBJS = $(ASYNC_UTEST_SRCS:.c=.o)
LIB = libutils.a
INCLUDE = -I../../../source/inc
ifdef FIFO_DATATYPE
DEFINES=-DFIFO_DATATYPE=$(FIFO_DATATYPE)
else
DEFINES=-DFIFO_DATATYPE=i32
endif

CC=arm-none-linux-gnueabi-gcc -fpic

.PHONY: all
all: lib

utils: CFLAGS=-g -c -Wall -Wextra -pedantic $(INCLUDE) $(DEFINES) $(M32)
utils: LIBS=-lpthread -lrt
utils: $(OBJS)
	$(CC) $(CFLAGS) $(LIBS)

lib: utils
	$(AR) rcv $(LIB) $(OBJS)

fifo_test: EXEC=fifo
fifo_test: CFLAGS=-g -Wall $(INCLUDE) $(DEFINES) $(M32)
fifo_test: LIBS=-lpthread -lrt
fifo_test: lib utils $(FIFO_UTEST_OBJS)
	$(CC) $(CFLAGS) $(FIFO_UTEST_OBJS) $(LIBS) $(LIB) -o $(EXEC)

async_task_test: EXEC=async_task
async_task_test: CFLAGS=-g -Wall $(INCLUDE) $(DEFINES) $(M32)
async_task_test: LIBS=-lpthread
async_task_test: lib utils $(ASYNC_UTEST_OBJS)
	$(CC) $(CFLAGS) $(ASYNC_UTEST_OBJS) $(LIBS) $(LIB) -o $(EXEC)

clean:
	rm -f fifo
	rm -f async_task
	rm -f $(LIB)
	rm -f *.o
