/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: dcc_config.h 2192 2006-05-25 15:32:13Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/05/25 15:32:13  paulf
 *     first checkin from Hydra
 *
 *     Revision 1.1  2005/06/30 20:39:55  mark
 *     Initial checkin
 *
 *     Revision 1.1  2005/04/21 16:55:26  mark
 *     Initial checkin
 *
 *     Revision 1.1  2000/03/05 21:47:33  lombard
 *     Initial revision
 *
 *
 *
 */

/* This include has been manually editted for use with Earthworm, on  *
 * Sun, Linux and NT platforms: Pete Lombard, 2/9/2000                */

/* Define to the type of elements in the array set by `getgroups'.
   Usually this is either `int' or `gid_t'.  */
#define GETGROUPS_T gid_t

/* Define if the `long double' type works.  */
#define HAVE_LONG_DOUBLE 1

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#ifndef _WINNT
#define TIME_WITH_SYS_TIME 1
#endif

#define DCCPACKAGE "LISS"
#define DCCVERSION "0.0"

/* The number of bytes in a int.  */
#define SIZEOF_INT 4

/* The number of bytes in a long.  */
#define SIZEOF_LONG 4

/* The number of bytes in a short.  */
#define SIZEOF_SHORT 2

/* Define if you have the memcpy function.  */
#define HAVE_MEMCPY 1

/* Define if you have the nanosleep function.  */
#ifndef _WINNT
#define HAVE_NANOSLEEP 1
#endif

/* Define if you have the strchr function.  */
#define HAVE_STRCHR 1

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the usleep function.  */
#ifndef _WINNT
#define HAVE_USLEEP 1
#endif

/* Define if you have the <ctype.h> header file.  */
#define HAVE_CTYPE_H 1

/* Define if you have the <errno.h> header file.  */
#define HAVE_ERRNO_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <math.h> header file.  */
#define HAVE_MATH_H 1

/* Define if you have the <memory.h> header file.  */
#define HAVE_MEMORY_H 1

/* Define if you have the <stddef.h> header file.  */
#define HAVE_STDDEF_H 1

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <sys/ioctl.h> header file.  */
#ifndef _WINNT
#define HAVE_SYS_IOCTL_H 1
#endif

/* Define if you have the <sys/mtio.h> header file.  */
#ifndef _WINNT
#define HAVE_SYS_MTIO_H 1
#endif

/* Define if you have the <unistd.h> header file.  */
#ifndef _WINNT
#define HAVE_UNISTD_H 1
#endif

/* Define if you have the nsl library (-lnsl).  */
#ifndef _WINNT
#define HAVE_LIBNSL 1
#endif
