AUTOMAKE_OPTIONS = subdir-objects

lib_LTLIBRARIES = libdecx170h.la

libdecx170h_la_CFLAGS = \
  -include $(top_builddir)/config.h \
  -I$(top_srcdir)/software/linux/h264high \
  -I$(top_srcdir)/software/source/inc \
  -I$(top_srcdir)/software/source/common \
  -I$(top_srcdir)/software/source/dwl \
  -I$(top_srcdir)/software/source/h264high \
  -I$(top_srcdir)/software/source/h264high/legacy \
  -I$(top_srcdir)/software/source/config 

#-D_DEBUG_PRINT # decoder trace
#-D_ERROR_PRINT # decoder error prints
if DEBUG
libdecx170h_la_CFLAGS += -g -DDEBUG -D_ASSERT_USED -D_RANGE_CHECK -D_ERROR_PRINT -D_DEBUG_PRINT
else
libdecx170h_la_CFLAGS += -DNDEBUG
endif

#DH264DEC_TRACE # for API tracing (H264DecTrace has to be implemented)
if DEC_TRACE
libdecx170h_la_CFLAGS += -DH264DEC_TRACE 
#-DASIC_TRACE_SUPPORT
#libdecx170h_la_CFLAGS += -I$(top_srcdir)/software/test/common/swhw/
endif

#_DWL_DEBUG # for wrapper layer trace
if DWL_DEBUG
libdecx170h_la_CFLAGS += -D_DWL_DEBUG
endif

if USE_DEC_IRQ
libdecx170h_la_CFLAGS += -DDEC_X170_USING_IRQ=1
endif

#TRACE_PP_CTRL # trace PP control in DEC software
if TRACE_PP_CTRL
libdecx170h_la_CFLAGS += -DTRACE_PP_CTRL # trace PP control in DEC software
endif

#-DMEMORY_USAGE_TRACE #trace all memory allocationsa defined in config.h
#-DTRACE_START_MARKER # logic analiser start marker generation

#-DDEC_PRINT_BAD_IRQ # trace irqs other than PIC_RDY in multicore

# decoder output format define in config.h
#-DDEC_X170_OUTPUT_FORMAT=0 # raster scan output
#-DDEC_X170_OUTPUT_FORMAT=1 # tiled output

# Set length of SW timeout in milliseconds. default: infinite wait (-1)
# This is just a parameter passed to the wrapper layer, so the real
# waiting happens there and is based on that impelmentation
#-DDEC_X170_TIMEOUT_LENGTH=-1

# Set evaluation limit in number of decoded frames
#-DH264DEC_EVALUATION=1000

libdecx170h_la_CFLAGS += $(AM_CFLAGS) \
  -Wall -Wextra -std=c99 -pedantic -fpic \
  -DDEC_X170_TIMEOUT_LENGTH=-1

libdecx170h_la_SOURCES = \
	../../source/h264high/h264decapi.c \
	../../source/h264high/h264hwd_asic.c \
	../../source/h264high/h264hwd_intra_prediction.c \
	../../source/h264high/h264hwd_inter_prediction.c \
	../../source/h264high/legacy/h264hwd_util.c \
	../../source/h264high/h264hwd_storage.c \
	../../source/h264high/h264hwd_dpb.c \
	../../source/h264high/h264hwd_decoder.c \
	../../source/h264high/h264hwd_conceal.c \
	../../source/h264high/h264_pp_pipeline.c \
	../../source/h264high/h264hwd_cabac.c \
	../../source/h264high/h264decapi_e.c \
	../../source/h264high/h264hwd_dpb_lock.c \
	../../source/h264high/h264decmcapi.c \
	../../source/h264high/h264hwd_slice_data.c \
	../../source/h264high/h264hwd_macroblock_layer.c \
	../../source/h264high/h264hwd_cavlc.c \
    ../../source/h264high/h264decapi_e.h \
    ../../source/h264high/h264hwd_asic.h \
    ../../source/h264high/h264hwd_cavlc.h \
    ../../source/h264high/h264hwd_container.h \
    ../../source/h264high/h264hwd_dpb.h \
    ../../source/h264high/h264hwd_exports.h \
    ../../source/h264high/h264hwd_macroblock_layer.h \
    ../../source/h264high/h264hwd_slice_data.h \
    ../../source/h264high/h264hwd_util.h \
    ../../source/h264high/h264_pp_pipeline.h \
    ../../source/h264high/h264decmc_internals.h \
    ../../source/h264high/h264hwd_cabac.h \
    ../../source/h264high/h264hwd_conceal.h \
    ../../source/h264high/h264hwd_decoder.h \
    ../../source/h264high/h264hwd_dpb_lock.h \
    ../../source/h264high/h264hwd_image.h \
    ../../source/h264high/h264hwd_regdrv.h \
    ../../source/h264high/h264hwd_storage.h \
    ../../source/h264high/h264_pp_multibuffer.h \
	../../source/h264high/legacy/h264hwd_byte_stream.c \
	../../source/h264high/legacy/h264hwd_seq_param_set.c \
	../../source/h264high/legacy/h264hwd_pic_param_set.c \
	../../source/h264high/legacy/h264hwd_slice_header.c \
	../../source/h264high/legacy/h264hwd_stream.c \
	../../source/h264high/legacy/h264hwd_vlc.c \
	../../source/h264high/legacy/h264hwd_nal_unit.c \
	../../source/h264high/legacy/h264hwd_neighbour.c \
	../../source/h264high/legacy/h264hwd_slice_group_map.c \
	../../source/h264high/legacy/h264hwd_vui.c \
	../../source/h264high/legacy/h264hwd_pic_order_cnt.c \
    ../../source/h264high/legacy/h264hwd_byte_stream.h \
    ../../source/h264high/legacy/h264hwd_nal_unit.h \
    ../../source/h264high/legacy/h264hwd_pic_order_cnt.h \
    ../../source/h264high/legacy/h264hwd_seq_param_set.h \
    ../../source/h264high/legacy/h264hwd_slice_header.h \
    ../../source/h264high/legacy/h264hwd_vlc.h \
    ../../source/h264high/legacy/h264hwd_cfg.h \
    ../../source/h264high/legacy/h264hwd_neighbour.h \
    ../../source/h264high/legacy/h264hwd_pic_param_set.h \
    ../../source/h264high/legacy/h264hwd_slice_group_map.h \
    ../../source/h264high/legacy/h264hwd_stream.h \
    ../../source/h264high/legacy/h264hwd_vui.h 

if DEBUG
libdecx170h_la_SOURCES += \
    h264hwd_debug.h
endif
libdecx170hincludedir = ${includedir}/g1decoder

libdecx170hinclude_HEADERS = \
  $(top_srcdir)/software/source/inc/decapicommon.h \
  $(top_srcdir)/software/source/inc/h264decapi.h \
  $(top_srcdir)/software/source/inc/basetype.h

libdecx170h_la_LDFLAGS = -version-number 3:2:0 -no-undefined

libdecx170h_la_LIBADD = ../../source/common/libdecx170_common.la

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = $(top_builddir)/libdecx170h.pc
