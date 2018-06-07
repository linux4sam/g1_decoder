

lib_LTLIBRARIES = libdwlx170.la

libdwlx170_la_CFLAGS = \
  -include $(top_builddir)/config.h \
  -I$(top_srcdir)/software/linux/dwl \
  -I$(top_srcdir)/software/source/inc \
  -I$(top_srcdir)/software/source/common \
  -I$(top_srcdir)/software/linux/memalloc \
  -I$(top_srcdir)/software/linux/ldriver \
  -I$(top_srcdir)/software/test/common/utils

if DEBUG
libdwlx170_la_CFLAGS += -g -DDEBUG
else
libdwlx170_la_CFLAGS += -DNDEBUG
endif

libdwlx170_la_SOURCES = \
  dwl_linux.c \
  dwl_linux.h \
  dwl_defs.h \
  ../memalloc/memalloc.h \
  ../ldriver/hx170dec.h

if SCSW
libdwlx170_la_CFLAGS += -DDWL_SINGLE_CORE_SW_ONLY
libdwlx170_la_SOURCES += dwl_linux_sc.c
else
libdwlx170_la_SOURCES += dwl_linux_mc.c
endif

if USE_DEC_IRQ
libdwlx170_la_CFLAGS += -DDWL_USE_DEC_IRQ
endif

#-D_DWL_DEBUG # for wrapper layer trace
if DWL_DEBUG
libdwlx170_la_CFLAGS += -D_DWL_DEBUG
endif

libdwlx170_la_CFLAGS += $(AM_CFLAGS) \
  -Wall -Wextra -Wno-missing-field-initializers \
  -DDWL_DISABLE_REG_PRINTS -fpic \
  -DDEC_MODULE_PATH=\"/dev/vdec\" \
  -DMEMALLOC_MODULE_PATH=\"/dev/memalloc\"

libdwlx170includedir = ${includedir}/g1decoder

libdwlx170include_HEADERS = \
  $(top_srcdir)/software/source/inc/dwl.h \
  $(top_srcdir)/software/source/inc/basetype.h

libdwlx170_la_LDFLAGS = -version-number 3:2:0 -no-undefined

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = $(top_builddir)/libdwlx170.pc
