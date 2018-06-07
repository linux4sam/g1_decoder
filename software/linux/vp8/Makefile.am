AUTOMAKE_OPTIONS = subdir-objects

lib_LTLIBRARIES = libdecx170vp8.la

libdecx170vp8_la_CFLAGS = \
  -include $(top_builddir)/config.h \
  -I$(top_srcdir)/software/source/inc \
  -I$(top_srcdir)/software/source/common \
  -I$(top_srcdir)/software/source/dwl \
  -I$(top_srcdir)/software/source/config \
  -I$(top_srcdir)/software/source/vp8 \
  -I$(top_srcdir)/software/test/common/utils

#_DEBUG_PRINT _ERROR_PRINT # decoder trace
if DEBUG
libdecx170vp8_la_CFLAGS += -g -DDEBUG -D_ASSERT_USED -D_RANGE_CHECK -D_ERROR_PRINT -D_DEBUG_PRINT
else
libdecx170vp8_la_CFLAGS +=  -DNDEBUG
endif

#_VP8DEC_TRACE # for API tracing (VP8DecTrace has to be implemented)
if DEC_TRACE
libdecx170vp8_la_CFLAGS += -D_VP8DEC_TRACE
endif

#_DWL_DEBUG # for wrapper layer trace
if DWL_DEBUG
libdecx170vp8_la_CFLAGS += -D_DWL_DEBUG
endif

#VP8_ASIC_TRACE # trace ASIC data
if ASIC_TRACE
libdecx170vp8_la_CFLAGS += -DVP8_ASIC_TRACE  -DASIC_TRACE_SUPPORT #trace ASIC data
endif

if SCSW
libdecx170vp8_la_CFLAGS += -DSINGLE_CORE_SW_ONLY
endif

#libdecx170vp8_la_CFLAGS += -DTRACE_START_MARKER # logic analiser start marker generation

#libdecx170vp8_la_CFLAGS += -DMEMORY_USAGE_TRACE #trace all memory allocations defined in config.h

#decoder output format defined in config.h
#libdecx170vp8_la_CFLAGS += -DDEC_X170_OUTPUT_FORMAT=0 # raster scan output
#libdecx170vp8_la_CFLAGS += -DDEC_X170_OUTPUT_FORMAT=1 # tiled output

# Define the empty picture data TESTING ONLY!!!!!
#if SET_EMPTY_PICTURE_DATA
#libdecx170vp8_la_CFLAGS += -DSET_EMPTY_PICTURE_DATA=$(SET_EMPTY_PICTURE_DATA)
#endif   

libdecx170vp8_la_CFLAGS += -DFIFO_DATATYPE=i32

libdecx170vp8_la_CFLAGS += $(AM_CFLAGS) \
  -Wall -Wextra -std=c99 -pedantic -fpic

libdecx170vp8_la_SOURCES = \
	../../source/vp8/vp8decapi.c \
	../../source/vp8/vp8hwd_bool.c \
	../../source/vp8/vp8hwd_buffer_queue.c \
	../../source/vp8/vp8hwd_probs.c \
	../../source/vp8/vp8hwd_headers.c \
	../../source/vp8/vp8hwd_decoder.c \
	../../source/vp8/vp8hwd_pp_pipeline.c \
	../../source/vp8/vp8hwd_asic.c \
	../../source/vp8/vp8hwd_error.c \
    ../../source/vp8/vp8decmc_internals.h \
    ../../source/vp8/vp8hwd_asic.h \
    ../../source/vp8/vp8hwd_bool.h \
    ../../source/vp8/vp8hwd_buffer_queue.h \
    ../../source/vp8/vp8hwd_container.h  \
    ../../source/vp8/vp8hwd_debug.h \
    ../../source/vp8/vp8hwd_decoder.h \
    ../../source/vp8/vp8hwd_error.h  \
    ../../source/vp8/vp8hwd_headers.h \
    ../../source/vp8/vp8hwd_pp_pipeline.h \
    ../../source/vp8/vp8hwd_probs.h \
	../../source/vp8/vp8decmcapi.c \
	../../test/common/utils/fifo.c

libdecx170vp8_la_LIBADD = ../../source/common/libdecx170_common.la

if SCSW
libdecx170vp8_la_LIBADD += -lpthread
endif

libdecx170vp8includedir = ${includedir}/g1decoder

libdecx170vp8include_HEADERS = \
  $(top_srcdir)/software/source/inc/decapicommon.h \
  $(top_srcdir)/software/source/inc/vp8decapi.h \
  $(top_srcdir)/software/source/inc/basetype.h

if SCSW
libdecx170vp8include_HEADERS += \
  $(top_srcdir)/software/test/common/utils/fifo.h
endif

libdecx170vp8_la_LDFLAGS = -version-number 3:2:0 -no-undefined

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = $(top_builddir)/libdecx170vp8.pc
