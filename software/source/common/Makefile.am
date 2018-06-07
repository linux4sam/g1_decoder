
AUTOMAKE_OPTIONS = subdir-objects

noinst_LTLIBRARIES = libdecx170_common.la

libdecx170_common_la_CFLAGS = \
  -include $(top_builddir)/config.h \
  -I$(top_srcdir)/software/source/inc \
  -I$(top_srcdir)/software/source/common \
  -I$(top_srcdir)/software/source/config

libdecx170_common_la_SOURCES = \
  bqueue.c \
  commonconfig.c \
  errorhandling.c \
  refbuffer.c \
  regdrv.c \
  tiledref.c \
  workaround.c \
  8170enum.h \
  8170table.h \
  bqueue.h \
  commonconfig.h \
  decppif.h \
  errorhandling.h \
  refbuffer.h \
  regdrv.h \
  tiledref.h \
  version.h \
  workaround.h \
  ../config/deccfg.h

libdecx170_common_la_CFLAGS += $(AM_CFLAGS)
