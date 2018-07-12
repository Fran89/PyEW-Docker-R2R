
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: platform.h 6858 2016-10-28 17:47:35Z kevin $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.11  2007/04/13 17:30:47  hal
 *     added typedef for ulong when we're compining under cygwin
 *
 *     Revision 1.10  2007/03/27 22:19:51  paulf
 *     added _MACOSX flags
 *
 *     Revision 1.9  2006/04/05 19:30:12  stefan
 *     logit fix for log file slash
 *
 *     Revision 1.8  2006/04/05 14:32:55  stefan
 *     added platform specific DIR_SLASH string
 *
 *     Revision 1.7  2006/03/10 13:50:56  paulf
 *     minor linux related fixes to removing _SOLARIS from the include line
 *
 *     Revision 1.6  2005/07/27 15:11:34  friberg
 *     added in _LINUX ifdefs
 *
 *     Revision 1.5  2004/10/07 21:30:15  mark
 *     Added thread priority constants
 *
 *     Revision 1.4  2003/12/03 01:16:05  davidk
 *     added NT #def for snprintf() in lieu of _snprintf()
 *
 *     Revision 1.3  2000/06/02 21:37:28  davidk
 *     Added a #define for vsnprintf on NT.  Removed comments from #define
 *     lines.  Comments on #define lines can be potentially diasterous if
 *     you comment out a section of code, and there is an end comment (* /)
 *     in a #define that you can't see, then you will get what seem like
 *     random compile errors.
 *
 *     Revision 1.2  2000/03/05 21:51:06  lombard
 *     Added `ifndef LONG_t' around LONG to prevent redefinition errors.
 *
 *     Revision 1.1  2000/02/14 20:05:54  lucky
 *     Initial revision
 *
 *
 */


              /*************************************************
               *                   platform.h                  *
               *                                               *
               *  System-dependent stuff.                      *
               *  This file is included by earthworm.h         *
               *************************************************/

#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef _WINNT
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>              /* Socket stuff */
#include <windows.h>
#include <process.h>               /* Required for getpid() */
#include <sys\types.h>

/* For pre-MSVC 2010 define standard int types, otherwise use inttypes.h */
#if defined(_MSC_VER) && _MSC_VER < 1600
  typedef signed char int8_t;
  typedef unsigned char uint8_t;
  typedef signed short int int16_t;
  typedef unsigned short int uint16_t;
  typedef signed int int32_t;
  typedef unsigned int uint32_t;
  typedef signed __int64 int64_t;
  typedef unsigned __int64 uint64_t;
#else
  #include <inttypes.h>
#endif

#ifndef INT32_MAX
  #define INT32_MAX 2147483647
#endif
#ifndef INT32_MIN
  #define INT32_MIN (-2147483647 - 1)
#endif

/* Thread functions return this */
#define thr_ret void
/* Value returned by thread functions; for Windows, nothing */
#define THR_NULL_RET

#define getpid _getpid
typedef int    pid_t;
typedef HANDLE sema_t;
typedef HANDLE mutex_t;
typedef HANDLE timer_t;

#ifndef EW_INT16
typedef signed __int16 EW_INT16;
#endif

#ifndef EW_INT32
typedef signed __int32 EW_INT32;
#endif


/* added so that logit.c can call vsnprintf for all platforms */
# define vsnprintf _vsnprintf
# define  snprintf  _snprintf

/* Thread priority constants.  These are based off the Win32 constants in winbase.h */
#define EW_PRIORITY_LOWEST      THREAD_PRIORITY_IDLE
#define EW_PRIORITY_LOW         THREAD_PRIORITY_BELOW_NORMAL
#define EW_PRIORITY_NORMAL      THREAD_PRIORITY_NORMAL
#define EW_PRIORITY_HIGH        THREAD_PRIORITY_ABOVE_NORMAL
#define EW_PRIORITY_CRITICAL    THREAD_PRIORITY_TIME_CRITICAL

#define DIR_SLASH   '\\'
#else
#define DIR_SLASH   '/'
#endif /* _WINNT */

#ifdef _OS2
#define INCL_DOSPROCESS
#define INCL_DOSMEMMGR
#define INCL_DOSSEMAPHORES
#define INCL_DOSFILEMGR
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <os2.h>
#include <netinet\in.h>       /* contains typedef of struct sockaddr_in */
#include <process.h>               /* Required for getpid() */
#include <types.h>
#include <nerrno.h>
#include <sys\socket.h>            /* Socket stuff */
#include <netdb.h>                 /* Socket stuff */
typedef void thr_ret;              /* Thread functions return this */
/* Value returned by thread functions; for OS2, nothing */
#define THR_NULL_RET
typedef int  pid_t;
typedef HEV  sema_t;
typedef HMTX mutex_t;
typedef long timer_t;

typedef long DWORD;
#endif /* _OS2 */


#ifdef _LINUX
#define _UNIX
/* broke this out on 2006/03/08 - paulf */
/* note the LINUX/POSIX includes go here, mostly pthread changes */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>            /* Socket stuff */
#include <arpa/inet.h>             /* Socket stuff */
#include <signal.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>            /* Socket stuff */
#include <netdb.h>                 /* Socket stuff */
#define mutex_t pthread_mutex_t
#define sema_t int
#define thread_t int
#define USYNC_THREAD 0
#undef SHM_INFO
#define fork1 fork

/* Thread functions return this */
#define thr_ret void*              
/* Value returned by thread functions; for UNIX, NULL */
#define THR_NULL_RET (NULL)

#ifndef LONG
#define LONG long
#endif
#ifndef LONG_t
#define LONG_t
#ifdef _CYGWIN
typedef unsigned long ulong ;
#endif 
typedef unsigned long ULONG ;
#endif
typedef uint32_t DWORD ;

#ifndef EW_INT16
typedef int16_t EW_INT16;
#endif

#ifndef EW_INT32
typedef int32_t EW_INT32;
#endif

#ifndef SOCKET
#define SOCKET int
#endif

#endif /* _LINUX */

#ifdef _MACOSX
#define _UNIX
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>            /* Socket stuff */
#include <arpa/inet.h>             /* Socket stuff */
#include <signal.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>            /* Socket stuff */
#include <netdb.h>                 /* Socket stuff */
#define mutex_t pthread_mutex_t
#define sema_t int
#define thread_t int
#define USYNC_THREAD 0
#undef SHM_INFO
#define fork1 fork

/* Thread functions return this */
#define thr_ret void*              
/* Value returned by thread functions; for MACOSX, NULL */
#define THR_NULL_RET (NULL)

#ifndef LONG
#define LONG long
#endif
#ifndef LONG_t
#define LONG_t
typedef unsigned long ULONG ;
typedef unsigned long ulong;
#endif
typedef uint32_t DWORD ;

typedef long timer_t;
#ifndef EW_INT16
typedef int16_t EW_INT16;
#endif

#ifndef EW_INT32
typedef int32_t EW_INT32;
#endif

#ifndef SOCKET
#define SOCKET int
#endif

#endif /* _MACOSX */

#ifdef _SOLARIS
/* all SOLARIS includes now specifically go here */
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/types.h>
#include <netinet/in.h>            /* Socket stuff */
#include <arpa/inet.h>             /* Socket stuff */
#include <signal.h>
#include <synch.h>                 /* for mutex's */
#include <sys/ipc.h>
#include <sys/shm.h>
#include <wait.h>
#include <thread.h>
#include <unistd.h>
#include <sys/socket.h>            /* Socket stuff */
#include <netdb.h>                 /* Socket stuff */

/* Thread functions return this */
#define thr_ret void*              
/* Value returned by thread functions; for Solaris, NULL */
#define THR_NULL_RET (NULL)

#ifndef LONG_t
#define LONG_t
typedef long LONG;
#endif
typedef uint32_t DWORD;
#ifndef EW_INT16
typedef int16_t EW_INT16;
#endif
#ifndef EW_INT32
typedef int32_t EW_INT32;
#endif

#ifndef SOCKET
#define SOCKET int
#endif

/* Thread priority constants.  These are arbitrary values chosen over the Solaris priority range
 * (0-127).  These may need to be tweaked.  (MMM 10/7/04)
 */
#define EW_PRIORITY_LOWEST      0
#define EW_PRIORITY_LOW         1
#define EW_PRIORITY_NORMAL      8
#define EW_PRIORITY_HIGH       16
#define EW_PRIORITY_CRITICAL  127

#endif /* _SOLARIS */

#endif /* PLATFORM_H */
