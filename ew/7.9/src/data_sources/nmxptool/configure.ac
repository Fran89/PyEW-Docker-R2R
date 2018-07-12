#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])

dnl Name your program here
m4_define([full_package_name], [nmxptool])

dnl These three define the plug-in version number
m4_define([major_version], [2])
m4_define([minor_version], [1])
m4_define([micro_version], [2])
# Comment build_version for stable release
# m4_define([build_version], [rc2])

m4_ifdef([build_version], [
	  m4_define([version],
		    [major_version.minor_version.micro_version-build_version])
	  ],
	  [
	  m4_define([version],
		    [major_version.minor_version.micro_version])
	   ]
	  )



m4_define([bug_report_address], [matteo.quintiliani@ingv.it])

AC_INIT([full_package_name], [version], [bug_report_address])

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_SRCDIR([src/nmxptool.c])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_SRCDIR([config.h.in])

AC_CANONICAL_HOST
AC_CANONICAL_SYSTEM

# Set BINARY_DIST_NAME_SUFFIX
BINARY_DIST_NAME_SUFFIX=$host
AC_MSG_NOTICE(BINARY_DIST_NAME_SUFFIX set to $BINARY_DIST_NAME_SUFFIX)
AC_SUBST(BINARY_DIST_NAME_SUFFIX)

avail_libmseed=YES
avail_seedlink=YES
avail_ew=YES

AM_INIT_AUTOMAKE
# Default AM_MAINTAINER_MODE is enable
# AM_MAINTAINER_MODE([enable])
# Default argument for AM_MAINTAINER_MODE is disable
AM_MAINTAINER_MODE([disable])

# Checks for programs.
# AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AM_PROG_CC_C_O
# AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
# AC_PROG_RANLIB Using `AC_PROG_RANLIB' is rendered obsolete by `AC_PROG_LIBTOOL'
AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB(nsl, socket, LIBS="$LIBS -lsocket -lnsl", , -lsocket)

AC_CHECK_LIB(rt, nanosleep, LIBS="$LIBS -lrt", , -lrt)

AC_CHECK_LIB(m, fabs, LIBS="$LIBS -lm", , -lm)

AC_CHECK_LIB(ws2_32, puts, LIBS="$LIBS -lws2_32", , -lws2_32)

AC_ARG_ENABLE([memdebug],
	      [AS_HELP_STRING([--enable-memdebug], [compile nmxptool for debugging memory leaks])],
	    [], 
	    [enable_memdebug=no]
) 

AC_ARG_ENABLE([libmseed],
	      [AS_HELP_STRING([--disable-libmseed], [disable saving data in mini-SEED records])],
	    [], 
	    [enable_libmseed=yes]
) 

AC_ARG_ENABLE([ew],
	      [AS_HELP_STRING([--disable-ew], [do not compile nmxptool as Earthworm module])],
	    [], 
	    [enable_ew=yes]
) 

AC_ARG_ENABLE([seedlink],
	      [AS_HELP_STRING([--disable-seedlink], [do not compile nmxptool as Seedlink plug-in])],
	    [], 
	    [enable_seedlink=yes]
) 

AC_MSG_WARN(Cross-compiling: $cross_compiling $build $host $target)
AC_MSG_WARN(OSTYPE: $OSTYPE)

case "$OSTYPE" in
solaris*)
    CFLAGS="$CFLAGS -D_POSIX_PTHREAD_SEMANTICS"
;;
win32*)
    WINSOURCES=$OSTYPE
;;
msys*)
    WINSOURCES=$OSTYPE
;;
cygwin*)
    WINSOURCES=$OSTYPE
;;
*)
    AC_MSG_WARN(Operating system is not Windows. $OSTYPE)
;;
esac
AM_CONDITIONAL(ENABLE_WINSOURCES, test x$WINSOURCES != x)

# mtheo
AM_PROG_LIBTOOL


# memdebug check.
AS_IF([test "x$enable_memdebug" != xno], 
      [
       CFLAGS="$CFLAGS -DNMXP_MEM_DEBUG"
       AC_MSG_WARN([memdebug has been enabled!])
       ]
)
AM_CONDITIONAL(ENABLE_MEMDEBUG, test x$enable_memdebug != xno)

AM_CONDITIONAL(ENABLE_DLL_PTHREAD, test x$DLL_PTHREAD != x)
AC_ARG_VAR(DIR_PTHREAD, [Directory containing PThread DDL library])
AC_ARG_VAR(DLL_PTHREAD, [Name of PThread DLL library - i.e. pthreadVC2.dll])


# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h stdint.h stdlib.h string.h sys/socket.h sys/stat.h sys/time.h unistd.h pthread.h])
AC_CHECK_HEADERS([windows.h winsock2.h])

AS_IF([test "x$enable_libmseed" != xno], 
      [AC_CHECK_LIB([mseed], [msr_init],
		    [LIBS="$LIBS -lmseed"
		     AC_DEFINE([HAVE_LIBMSEED], [1], [Define if you have libmseed]) 
		     # AC_CHECK_LIB([nmxp], [nmxp_openSocket], [], [AC_MSG_ERROR([libnmxp not found!])]) 
		     # AC_CHECK_LIB([nmxp], [nmxp_data_msr_pack],
		#		  [],
		#		  [AC_MSG_FAILURE([libmseed found but libnmxp does not support it. (Try --disable-libmseed to disable)])])
		     ], 
		     [
		      avail_libmseed=NO
		      AC_MSG_WARN([libmseed is not available (--disable-libmseed to disable)])
		     # AC_CHECK_LIB([nmxp], [nmxp_openSocket], [], [AC_MSG_ERROR([libnmxp not found or it supports libmseed
				   # but libmseed is not available anymore! Try to recompile libnmxp!
				   # ])]) 
				   ], 
				   )
      ],
      [
       # AC_CHECK_LIB([nmxp], [nmxp_openSocket], [], [AC_MSG_ERROR([libnmxp not found or supports libmseed (--enable-libmseed to enable)!])]) 
       avail_libmseed=NO
       AC_MSG_WARN([libmseed feature has been disabled!])
      ]
) 
AM_CONDITIONAL(ENABLE_LIBMSEED, test x$enable_libmseed != xno)

AC_ARG_VAR(EW_HOME, [Earthworm home directory])
AC_ARG_VAR(EW_VERSION, [Earthworm version directory])
AC_ARG_VAR(EW_PARAMS, [Earthworm configuration files directory])
AC_ARG_VAR(GLOBALFLAGS, [C compiler flags for Earthworm])

EWOSOURCES=no

# Earthworm check.
AS_IF([test "x$enable_ew" != xno], 
      [
    AS_IF([test "x$EW_HOME" != x], 
	  [
	   # CFLAGS="$CFLAGS -m32"
	   CFLAGS="$CFLAGS -I$EW_HOME/$EW_VERSION/include $GLOBALFLAGS"
	AC_CHECK_HEADERS([earthworm.h kom.h transport.h trace_buf.h],
			 [AC_DEFINE([HAVE_EARTHWORMFILES_H], [1], [Define if you have earthworm header file])],
			 AC_MSG_ERROR([Earthworm header files not found!])
			 )
	EWLIB=$EW_HOME/$EW_VERSION/lib
	EWOBJS="$EWLIB/kom.o $EWLIB/getutil.o $EWLIB/logit_mt.o $EWLIB/socket_ew_common.o $EWLIB/transport.o $EWLIB/sleep_ew.o $EWLIB/socket_ew.o $EWLIB/time_ew.o $EWLIB/threads_ew.o $EWLIB/sema_ew.o $EWLIB/swap.o $EWLIB/mem_circ_queue.o"
	AC_DEFINE([HAVE_EARTHWORMOBJS], [1], [Define if you have all earthworm object files])
	AS_IF([test "x$EWOSOURCES" != xno],
	      [ 
		case "$OSTYPE" in
		solaris*)
		    EWOS=solaris
		;;
		darwin*)
		    EWOS=unix
		;;
		linux*)
		    EWOS=unix
		;;
		win32*)
		    EWOS=winnt
		;;
		msys*)
		    EWOS=winnt
		;;
		cygwin*)
		    EWOS=winnt
		;;
		*)
		    EWOS=unix
		    AC_MSG_WARN(Unrecognised OS $OSTYPE)
		;;
		esac
		AC_MSG_WARN(EWOS set to $EWOS)
		AC_SUBST(EWOS)
		EW_LIBSRC=$EW_HOME/$EW_VERSION/src/libsrc
		EW_LIBSRC_UTIL=$EW_LIBSRC/util
		EW_LIBSRC_OS=$EW_LIBSRC/$EWOS
		AC_SUBST(EW_LIBSRC)
		AC_SUBST(EW_LIBSRC_UTIL)
		AC_SUBST(EW_LIBSRC_OS)
	       ],
	      [
	       LIBS="$LIBS $EWOBJS"
	       ])
	],
	[
	 avail_ew=NO
	 AC_MSG_WARN([Earthworm files are not available (--disable-ew to disable)])]
    )
    ],
    [
     avail_ew=NO
     AC_MSG_WARN([ew feature has been disabled!])]
)
AM_CONDITIONAL(ENABLE_EWSOURCES, test x$EWOS != x)
AM_CONDITIONAL(ENABLE_EW, test x$enable_ew != xno)

# Checks for files.

AC_ARG_VAR(SEISCOMPDIR, [SeisComP root directory - default is /home/sysop/seiscomp3])

# Seedlink check.
AS_IF([test "x$enable_seedlink" != xno], 
      [

       AS_IF([test "x$SEISCOMPDIR" == x],
	      [
	       SEISCOMPDIR=/home/sysop/seiscomp3
	       ]
	      )
       AC_MSG_WARN([SEISCOMPDIR is set to ${SEISCOMPDIR}])
       AC_SUBST(SEISCOMPDIR)
       AC_DEFINE([HAVE_SEEDLINK], [1], [Define if you have libmseed]) 

       ],
    [
     avail_seedlink=NO
     AC_MSG_WARN([seedlink feature has been disabled!])]
)
AM_CONDITIONAL(ENABLE_SEEDLINK, test x$enable_seedlink != xno)


# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT32_T
AC_TYPE_INT8_T
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_STRUCT_TM
# AC_CHECK_SIZEOF([long int])

# Checks for library functions.
# AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STRERROR_R
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([getcwd gethostbyname inet_ntoa memmove memset mkdir select setenv socket strcasecmp strchr strerror strstr strtol tzset])
AC_CHECK_FUNCS([gettimeofday], [], [
			       AC_MSG_ERROR([function gettimeofday() not found!])
])
AC_CHECK_FUNCS([timegm], [], [
		AC_CHECK_FUNCS([getenv setenv unsetenv tzset],
			       [],
			       [AC_MSG_WARN([
				Computation of packet latencies could be wrong
				if local time is not equal to UTC.
				])]
			       )
])
AC_CHECK_MEMBERS([struct tm.tm_gmtoff])

# Test whether _timezone is available. (On Windows only)
AC_MSG_CHECKING([for global _timezone variable])
dnl FIXME: As above, don't want link check
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <time.h>]],
		  [[long z2 = _timezone;]])],
		  [AC_DEFINE(HAVE_UNDERSCORE_TIMEZONE, 1,
		   [Define if your platform has the global _timezone variable.])
		  AC_MSG_RESULT(yes)],
		  [AC_MSG_RESULT(no)])



# Test whether SO_RCVTIMEO is broken. (On Solaris SO_RCVTIMEO is defined but not implemented)
AC_CACHE_CHECK([whether setsockopt(SO_RCVTIMEO) is broken],
ac_cv_so_rcvtimeo_broken, [dnl
			   AC_RUN_IFELSE([AC_LANG_SOURCE([[
					  #if defined(HAVE_SYS_TYPES_H)
					  #include <sys/types.h>
					  #endif

					  #if defined(HAVE_SYS_SOCKET_H)
					  #include <sys/socket.h>
					  #endif

					  #if defined(HAVE_SYS_TIME_H)
					  #include <sys/time.h>
					  #endif

					  int main(void) {

					  #ifdef HAVE_WINDOWS_H
					  return 0;
					  #else
					  
					  int fd;
					  int ret;
					  struct timeval new_tv;

					  /* Open the socket (INET/TCP).*/
					  fd = socket(AF_INET, SOCK_STREAM, 0);

					  /* set the timeout for the incoming queue */
					  /* 1 second for example */
					  new_tv.tv_sec = 1;
					  new_tv.tv_usec = 0;

					  ret = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &new_tv, sizeof(new_tv)); 
					  return ret;

					  #endif
				      }
				      ]])],[ac_cv_so_rcvtimeo_broken=no],[ac_cv_so_rcvtimeo_broken=yes],[ac_cv_so_rcvtimeo_broken=cross])])
if test x"$ac_cv_so_rcvtimeo_broken" = x"yes"; then
    AC_DEFINE(HAVE_BROKEN_SO_RCVTIMEO, 1, [Define if setsockopt(SO_RCVTIMEO) is broken])
    fi

AC_CONFIG_FILES([Makefile
                 lib/Makefile
                 src/Makefile])

# AC_CONFIG_SUBDIRS([libnmxp])

AC_OUTPUT

# AC_MSG_NOTICE([
#      Enabled features: libmseed ($enable_ew), SeedLink ($enable_seedlink), Earthworm ($enable_ew).])
AC_MSG_NOTICE([Enabled features
          libmseed : $avail_libmseed
	  SeedLink : $avail_seedlink
	  Earthworm: $avail_ew
	  Cross-compiling: $cross_compiling $build $host $target])

AC_MSG_NOTICE([
      Compile nmxptool using 'make'.])

AS_IF([test "x$enable_ew" != xno], 
      [AC_MSG_NOTICE([
      After compilation you can install nmxptool for Earthworm:
            make install-ew-bin
            make install-ew-conf])
      ]
)
AS_IF([test "x$enable_seedlink" != xno], 
      [AC_MSG_NOTICE([
      After compilation you can install nmxptool for Seedlink:
            make install-seiscomp-bin
            make install-seiscomp-templates])
      ]
)

