AC_PREREQ([2.63])
AC_INIT([g1decoder],[3.2])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_MACRO_DIRS([m4])
AC_CANONICAL_SYSTEM
PKG_PROG_PKG_CONFIG

AM_INIT_AUTOMAKE([1.10 foreign])

# Enable quiet compiles on automake 1.11.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CC

pkgconfigdir=${libdir}/pkgconfig
AC_SUBST(pkgconfigdir)

AC_USE_SYSTEM_EXTENSIONS

AC_CHECK_HEADER_STDBOOL
AC_CHECK_HEADERS([fcntl.h limits.h memory.h stddef.h stdlib.h string.h sys/ioctl.h sys/time.h sys/timeb.h unistd.h])
AC_CHECK_FUNCS([getpagesize memset munmap strtol])
AC_C_INLINE
AC_TYPE_INT64_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T
AC_FUNC_FSEEKO
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AC_MSG_CHECKING(whether to enable debug)
AC_ARG_ENABLE(debug,
	AS_HELP_STRING([--enable-debug],[enable debugging,[default=no]]),
		debug=${enableval},
		debug=no)
AM_CONDITIONAL(DEBUG, test "x$debug" = "xyes")
AC_MSG_RESULT($debug)

AC_MSG_CHECKING(whether to use decode irq)
AC_ARG_ENABLE(dec_irq,
	AS_HELP_STRING([--disable-dec-irq], [Enable decode irq default=y]),
		[ case "${enableval}" in
			yes) dec_irq=yes;;
			no)  dec_irq=no;;
            *)   AC_MSG_ERROR([bad value ${enableval} for --dec-irq]) ;;
        esac], [dec_irq="yes"])
AM_CONDITIONAL([USE_DEC_IRQ], [test "x$dec_irq" = "xyes"])
AC_MSG_RESULT($dec_irq)

AC_MSG_CHECKING(whether to debug dwl)
AC_ARG_ENABLE(dwl_debug,
	AS_HELP_STRING([--enable-dwl-debug], [Enable dwl debug default=no]),
		[ case "${enableval}" in
			yes) dwl_debug=yes;;
			no)  dwl_debug=no;;
            *)   AC_MSG_ERROR([bad value ${enableval} for --enable-dwl-debug]) ;;
        esac], [dwl_debug="no"])
AM_CONDITIONAL([DWL_DEBUG], [test "x$dwl_debug" = "xyes"])
AC_MSG_RESULT($dwl_debug)

AC_MSG_CHECKING(whether to use dwl single core software)
AC_ARG_ENABLE(scsw,
	AS_HELP_STRING([--disable-scsw], [dwl single core software default=yes]),
		[ case "${enableval}" in
			yes) scsw=yes;;
			no)  scsw=no;;
            *)   AC_MSG_ERROR([bad value ${enableval} for --disable-scsw]) ;;
        esac], [scsw="yes"])
AM_CONDITIONAL([SCSW], [test "x$scsw" = "xyes"])
AC_MSG_RESULT($scsw,)

AC_ARG_ENABLE(ref_buffer,
	AS_HELP_STRING([--disable-ref-buffer], [disable reference buffer default]),
		AC_DEFINE(DWL_REFBUFFER_DISABLE, 1, [Define this to disable reference buffer.]))

AC_ARG_ENABLE(memusage_trace,
	AS_HELP_STRING([--enable-memusage-trace], [trace all memory allocations]),
		AC_DEFINE(MEMORY_USAGE_TRACE, 1, [trace all memory allocations.]))

AC_MSG_CHECKING(whether to asic traces)
AC_ARG_ENABLE(asic_trace,
	AS_HELP_STRING([--enable-asic-trace], [trace asic data]),
        asic_trace=${enableval},
        asic_trace=no)
AM_CONDITIONAL([ASIC_TRACE], test "x$asic_trace" = "xyes")
AC_MSG_RESULT($asic_trace)

AC_MSG_CHECKING(whether to decode API traces)
AC_ARG_ENABLE(dec_trace, 
	AS_HELP_STRING([--enable-dec-trace], [decode API tracing has to be implemented]),
        dec_trace=${enableval},
        dec_trace=no)
AM_CONDITIONAL([DEC_TRACE], test "x$dec_trace" = "xyes")
AC_MSG_RESULT($dec_trace)

AC_MSG_CHECKING(whether to enable post processing control traces)
AC_ARG_ENABLE(ppctrl_trace, 
	AS_HELP_STRING([--enable-ppctrl-trace], [trace PP control in decoder software]),
        ppctrl_trace=${enableval},
        ppctrl_trace=no)
AM_CONDITIONAL([TRACE_PP_CTRL], test "x$ppctrl_trace" = "xyes")
AC_MSG_RESULT($ppctrl_trace)

AC_ARG_VAR([output_format], [decoder output format: default=0 0=raster scan output & 1=tiled output])
if test -z "$output_format"; then
	AC_DEFINE(DEC_X170_OUTPUT_FORMAT, 0, [0=raster scan output & 1=tiled output])
else
	AC_DEFINE(DEC_X170_OUTPUT_FORMAT, 1, [0=raster scan output & 1=tiled output])
fi

AC_ARG_VAR([start_marker], [logic analyzer start marker generation default=0])
if test -z "$start_marker"; then
	AC_DEFINE(TRACE_START_MARKER, 0, [start marker generation])
else
	AC_DEFINE(TRACE_START_MARKER, 1, [start marker generation])
fi

AC_SYS_LARGEFILE

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT()

AC_CONFIG_FILES([Makefile
	software/Makefile
	software/source/Makefile
	software/source/common/Makefile
	software/linux/Makefile
	software/linux/dwl/Makefile
	software/linux/h264high/Makefile
	software/linux/mpeg4/Makefile
	software/linux/jpeg/Makefile
	software/linux/pp/Makefile
	software/linux/vp8/Makefile
	libdecx170h.pc
	libdecx170m.pc
	libdecx170p.pc
	libdecx170vp8.pc
	libdwlx170.pc
	libx170j.pc])
AC_OUTPUT
