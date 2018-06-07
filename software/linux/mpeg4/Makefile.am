AUTOMAKE_OPTIONS = subdir-objects

lib_LTLIBRARIES = libdecx170m.la

libdecx170m_la_CFLAGS = \
  -include $(top_builddir)/config.h \
  -I$(top_srcdir)/software/linux/mpeg4 \
  -I$(top_srcdir)/software/source/inc \
  -I$(top_srcdir)/software/source/dwl \
  -I$(top_srcdir)/software/source/common \
  -I$(top_srcdir)/software/source/mpeg4 \
  -I$(top_srcdir)/software/source/config 

libdecx170m_la_SOURCES =

#_DEBUG_PRINT _ERROR_PRINT _MPEG4APITRACE _DEC_PP_USAGE  # decoder trace
if DEBUG
libdecx170m_la_CFLAGS += -g -DDEBUG -D_ASSERT_USED -D_DEBUG_PRINT -D_ERROR_PRINT -D_DEC_PP_USAGE
else
libdecx170m_la_CFLAGS += -DNDEBUG
endif

#MP4DEC_TRACE _MPEG4APITRACE  # decoder trace
if DEC_TRACE
libdecx170m_la_CFLAGS += -D_MPEG4APITRACE -DMP4DEC_TRACE
libdecx170m_la_CFLAGS += -I$(top_srcdir)/software/test/common/swhw/
libdecx170m_la_SOURCES += $(top_srcdir)/software/test/common/swhw/trace.h
endif

#_DWL_DEBUG # for wrapper layer trace
if DWL_DEBUG
libdecx170m_la_CFLAGS += -D_DWL_DEBUG
endif

#MP4_ASIC_TRACE # trace ASIC data
if ASIC_TRACE
libdecx170m_la_CFLAGS += -DMP4_ASIC_TRACE -DASIC_TRACE_SUPPORT # trace ASIC data
endif

#libdecx170m_la_CFLAGS += -DMEMORY_USAGE_TRACE #trace all memory allocations defined in config.h

#libdecx170m_la_CFLAGS += -D_X170_DEMO_DEVEL # Enable demo development functionality

#-DDEC_X170_OUTPUT_FORMAT=DEC_X170_OUTPUT_FORMAT_RASTER_SCAN defined in config.h

libdecx170m_la_CFLAGS += $(AM_CFLAGS) \
  -Wall -Wextra -std=c99 -fpic \
  -D_MP4_RLC_BUFFER_SIZE=384

libdecx170m_la_SOURCES += \
	../../source/mpeg4/mp4dechwd_error_conceal.c \
	../../source/mpeg4/mp4decapi.c \
	../../source/mpeg4/mp4decapi_internal.c \
	../../source/mpeg4/mp4dechwd_rvlc.c \
	../../source/mpeg4/mp4dechwd_strmdec.c \
	../../source/mpeg4/mp4dechwd_headers.c \
	../../source/mpeg4/mp4dechwd_motiontexture.c \
	../../source/mpeg4/mp4dechwd_shortvideo.c \
	../../source/mpeg4/mp4dechwd_utils.c \
	../../source/mpeg4/mp4dechwd_videopacket.c \
	../../source/mpeg4/mp4dechwd_vlc.c \
	../../source/mpeg4/mp4dechwd_vop.c \
	../../source/mpeg4/mp4dechwd_generic.c \
    ../../source/mpeg4/mp4decapihwd_storage.h \
    ../../source/mpeg4/mp4decdrv.h \
    ../../source/mpeg4/mp4dechwd_mvstorage.h \
    ../../source/mpeg4/mp4dechwd_storage.h \
    ../../source/mpeg4/mp4dechwd_svdesc.h \
    ../../source/mpeg4/mp4dechwd_vlc.h \
    ../../source/mpeg4/mpeg4_pp_pipeline.h \
    ../../source/mpeg4/mp4decapi_internal.h \
    ../../source/mpeg4/mp4dechwd_container.h \
    ../../source/mpeg4/mp4dechwd_hdrs.h \
    ../../source/mpeg4/mp4dechwd_mbsetdesc.h \
    ../../source/mpeg4/mp4dechwd_rvlc.h \
    ../../source/mpeg4/mp4dechwd_strmdec.h \
    ../../source/mpeg4/mp4dechwd_utils.h \
    ../../source/mpeg4/mp4dechwd_vopdesc.h \
    ../../source/mpeg4/mp4deccfg.h \
    ../../source/mpeg4/mp4dechwd_error_conceal.h \
    ../../source/mpeg4/mp4dechwd_headers.h \
    ../../source/mpeg4/mp4dechwd_motiontexture.h \
    ../../source/mpeg4/mp4dechwd_shortvideo.h \
    ../../source/mpeg4/mp4dechwd_strmdesc.h \
    ../../source/mpeg4/mp4dechwd_videopacket.h \
    ../../source/mpeg4/mp4dechwd_vop.h \
    ../../source/mpeg4/mp4dechwd_mbdesc.h \
    ../../source/mpeg4/mp4dechwd_generic.h

if DEBUG
libdecx170m_la_SOURCES += \
    mpeg4asicdbgtrace.c \
    mp4debug.h \
    mpeg4asicdbgtrace.h
endif

libdecx170mincludedir = ${includedir}/g1decoder

libdecx170minclude_HEADERS = \
  $(top_srcdir)/software/source/inc/decapicommon.h \
  $(top_srcdir)/software/source/inc/mp4decapi.h \
  $(top_srcdir)/software/source/inc/basetype.h

libdecx170m_la_LDFLAGS = -version-number 3:2:0 -no-undefined

libdecx170m_la_LIBADD = ../../source/common/libdecx170_common.la

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = $(top_builddir)/libdecx170m.pc
