# AUTOMAKE_OPTIONS = gnu subdir-objects
AUTOMAKE_OPTIONS = gnu
ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = nmxptool

nmxptool_SOURCES  = nmxptool.c
nmxptool_SOURCES += nmxptool_getoptlong.c nmxptool_getoptlong.h
nmxptool_SOURCES += nmxptool_ew.c nmxptool_ew.h
nmxptool_SOURCES += nmxptool_chanseq.c nmxptool_chanseq.h
nmxptool_SOURCES += nmxptool_sigcondition.c nmxptool_sigcondition.h
nmxptool_SOURCES += nmxptool_listen.c nmxptool_listen.h

nmxptool_CFLAGS= -I../include
nmxptool_LDADD= ../lib/libnmxp.a

if ENABLE_SEEDLINK
nmxptool_SOURCES += seedlink_plugin.c seedlink_plugin.h
endif

if ENABLE_EWSOURCES
nodist_nmxptool_SOURCES = @EW_LIBSRC_UTIL@/kom.c
nodist_nmxptool_SOURCES += @EW_LIBSRC_UTIL@/getutil.c
nodist_nmxptool_SOURCES += @EW_LIBSRC_UTIL@/logit.c
nodist_nmxptool_SOURCES += @EW_LIBSRC_UTIL@/socket_ew_common.c
nodist_nmxptool_SOURCES += @EW_LIBSRC_UTIL@/swap.c
nodist_nmxptool_SOURCES += @EW_LIBSRC_UTIL@/mem_circ_queue.c
nodist_nmxptool_SOURCES += @EW_LIBSRC_OS@/transport.c
nodist_nmxptool_SOURCES += @EW_LIBSRC_OS@/sleep_ew.c
nodist_nmxptool_SOURCES += @EW_LIBSRC_OS@/socket_ew.c
nodist_nmxptool_SOURCES += @EW_LIBSRC_OS@/time_ew.c
nodist_nmxptool_SOURCES += @EW_LIBSRC_OS@/threads_ew.c
nodist_nmxptool_SOURCES += @EW_LIBSRC_OS@/sema_ew.c
endif

#noinst_HEADERS = gdu.h

# man page
# man_MANS = gdu.1x
#
# # we want these in the dist tarball
# EXTRA_DIST = back.xpm reload.xpm root.xpm \
#              home.xpm stop.xpm about.xpm gdu.1x
