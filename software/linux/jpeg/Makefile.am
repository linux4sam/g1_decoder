AUTOMAKE_OPTIONS = subdir-objects

lib_LTLIBRARIES = libx170j.la

libx170j_la_CFLAGS = \
  -include $(top_builddir)/config.h \
  -I$(top_srcdir)/software/linux/jpeg \
  -I$(top_srcdir)/software/source/inc \
  -I$(top_srcdir)/software/source/common \
  -I$(top_srcdir)/software/source/dwl \
  -I$(top_srcdir)/software/source/config \
  -I$(top_srcdir)/software/source/jpeg

#libx170j_la_CFLAGS += -D_DEBUG_PRINT -D_ERROR_PRINT # decoder trace
#libx170j_la_CFLAGS += -D_ASSERT_USED
if DEBUG
libx170j_la_CFLAGS += -g -DDEBUG -D_ASSERT_USED -D_RANGE_CHECK -D_ERROR_PRINT -D_DEBUG_PRINT
else
libx170j_la_CFLAGS += -DNDEBUG
endif

#libx170j_la_CFLAGS += -DJPEGDEC_TRACE # for API tracing (JpegDecTrace has to be implemented)
#libx170j_la_CFLAGS += -DJPEGDEC_TRACE_INTERNAL # for JPEG/HW data tracing
#libx170j_la_CFLAGS += -DJPEGDEC_API_TRC # for control trace
if DEC_TRACE
libx170j_la_CFLAGS += -DJPEGDEC_TRACE -DJPEGDEC_TRACE_INTERNAL -DJPEGDEC_API_TRC
endif

#libx170j_la_CFLAGS += -D_DWL_DEBUG # for wrapper layer trace
if DWL_DEBUG
libx170j_la_CFLAGS += -D_DWL_DEBUG
endif

#JPEGDEC_ASIC_TRACE # for JPEG/HW data tracing
if ASIC_TRACE
libx170j_la_CFLAGS += -DJPEGDEC_ASIC_TRACE -DASIC_TRACE_SUPPORT  # for JPEG/HW data tracing
endif

#libx170j_la_CFLAGS += -DMEMORY_USAGE_TRACE #trace all memory allocations
#libx170j_la_CFLAGS += -DJPEGDEC_INTEGRATOR # for integrator

# Define this for performance measurement
#libx170j_la_CFLAGS += -DJPEGDEC_PERFORMANCE # for performance

# Define this for testing error cases
#libx170j_la_CFLAGS += -DJPEGDEC_RESET_OUTPUT

# Add error resilience flag (or not) to CFLAGS
# libx170j_la_CFLAGS += -DJPEGDEC_ERROR_RESILIENCE

libx170j_la_CFLAGS += $(AM_CFLAGS) \
  -Wall -Wextra -std=c99 -pedantic -fpic

libx170j_la_SOURCES = \
	../../source/jpeg/jpegdecapi.c \
	../../source/jpeg/jpegdechdrs.c \
	../../source/jpeg/jpegdecinternal.c \
	../../source/jpeg/jpegdecscan.c \
	../../source/jpeg/jpegdecutils.c \
    ../../source/jpeg/jpegdeccontainer.h \
    ../../source/jpeg/jpegdechdrs.h \
    ../../source/jpeg/jpegdecinternal.h \
    ../../source/jpeg/jpegdecmarkers.h \
    ../../source/jpeg/jpegdecscan.h \
    ../../source/jpeg/jpegdecutils.h \
    ../../source/jpeg/jpeg_pp_pipeline.h \
    ../../source/jpeg/jpegregdrv.h

if DEBUG
libx170j_la_SOURCES += \
   jpegasicdbgtrace.c \
   jpegasicdbgtrace.h \
   jpegdebug.h
endif

libx170jincludedir = ${includedir}/g1decoder

libx170jinclude_HEADERS = \
  $(top_srcdir)/software/source/inc/decapicommon.h \
  $(top_srcdir)/software/source/inc/jpegdecapi.h \
  $(top_srcdir)/software/source/inc/basetype.h

libx170j_la_LDFLAGS = -version-number 3:2:0 -no-undefined

libx170j_la_LIBADD = ../../source/common/libdecx170_common.la

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = $(top_builddir)/libx170j.pc
