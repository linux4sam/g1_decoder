AUTOMAKE_OPTIONS = subdir-objects

lib_LTLIBRARIES = libdecx170p.la

libdecx170p_la_CFLAGS = \
  -include $(top_builddir)/config.h \
  -I$(top_srcdir)/software/linux/pp \
  -I$(top_srcdir)/software/source/inc \
  -I$(top_srcdir)/software/source/dwl \
  -I$(top_srcdir)/software/source/common \
  -I$(top_srcdir)/software/source/config \
  -I$(top_srcdir)/software/source/h264high \
  -I$(top_srcdir)/software/source/mpeg4 \
  -I$(top_srcdir)/software/source/jpeg \
  -I$(top_srcdir)/software/source/vp8 \
  -I$(top_srcdir)/software/source/pp

# Add your debugging flag (or not) to CFLAGS. Enable, if needed
#DEBFLAGS += -D_DEBUG_PRINT -D_ERROR_PRINT # decoder trace
if DEBUG
libdecx170p_la_CFLAGS +=  -g -DDEBUG -D_ASSERT_USED -D_DEBUG_PRINT -D_ERROR_PRINT -D_PPDEBUG_PRINT
else
libdecx170p_la_CFLAGS +=  -DNDEBUG
endif

#PP_TRACE # for API tracing (PPDecTrace has to be implemented)
if DEC_TRACE
libdecx170p_la_CFLAGS += -DPP_TRACE
endif

#_DWL_DEBUG # for wrapper layer trace
if DWL_DEBUG
libdecx170p_la_CFLAGS += -D_DWL_DEBUG # for wrapper layer trace
endif

#PP_ASIC_TRACE # trace ASIC data
if ASIC_TRACE
libdecx170p_la_CFLAGS += -DPP_ASIC_TRACE  -DASIC_TRACE_SUPPORT # trace ASIC data
endif

#DEBFLAGS += -DMEMORY_USAGE_TRACE #trace all memory allocations
#DEBFLAGS += -D_X170_DEMO_DEVEL # Enable demo development functionality

# Postprocessor and decoder pipeline support
PIPELINE_SUPPORT = -DPP_H264DEC_PIPELINE_SUPPORT # H.264 decoder support
PIPELINE_SUPPORT += -DPP_MPEG4DEC_PIPELINE_SUPPORT # MPEG4 decoder support
PIPELINE_SUPPORT += -DPP_JPEGDEC_PIPELINE_SUPPORT # JPEG decoder support
PIPELINE_SUPPORT += -DPP_VP8DEC_PIPELINE_SUPPORT # VP8 decoder support

libdecx170p_la_CFLAGS += $(AM_CFLAGS) $(PIPELINE_SUPPORT) -Wall -Wextra -std=c99 -pedantic -fpic

libdecx170p_la_SOURCES = \
	../../source/pp/ppapi.c \
	../../source/pp/ppinternal.c \
	../../source/pp/ppinternal.h \
	../../source/config/ppcfg.h

if DEBUG
libdecx170p_la_SOURCES += ppdebug.h
endif

libdecx170pincludedir = ${includedir}/g1decoder

libdecx170pinclude_HEADERS = \
  $(top_srcdir)/software/source/inc/decapicommon.h \
  $(top_srcdir)/software/source/inc/ppapi.h \
  $(top_srcdir)/software/source/inc/basetype.h

libdecx170p_la_LDFLAGS = -version-number 3:2:0 -no-undefined

libdecx170p_la_LIBADD = ../../source/common/libdecx170_common.la

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = $(top_builddir)/libdecx170p.pc
