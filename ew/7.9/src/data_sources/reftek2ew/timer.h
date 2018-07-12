#pragma ident "$Id: timer.h 5713 2013-08-05 19:08:23Z paulf $"
/* --------------------------------------------------------------------
   Program  :  Any
   Task     :  TIMER.C
   File     :  TIMER.H
   Purpose  :  48 bit millisecond timer routines
   Host     :  CC, GCC, Microsoft Visual C++ 5.x, MCC68K 3.1
   Target   :  Solaris (Sparc and x86), Linux, DOS, Win32, and RTOS
   Packages :  None
   Author   :  Robert Banfill (r_banfill@reftek.com)
   Company  :  Refraction Technology, Inc.
               2626 Lombardy Lane, Suite 105
               Dallas, Texas  75220  USA
               (214) 353-0609 Voice, (214) 353-9659 Fax, info@reftek.com
   Copyright:  (c) 1997 Refraction Technology, Inc. - All Rights Reserved.
   Notes    :	Y2K compliant
   $Revision: 5713 $
   $Logfile :  R:/cpu68000/rt422/struct/version.h_v  $
   Revised  :
      25Mar97  ---- (RLB) Initial version.

-------------------------------------------------------------------- */

#ifndef _TIMER_H_
#define _TIMER_H_

#include <time.h>
#include "platform.h"

/* Constants ------------------------------------------------------- */

/* Milliseconds based time units */

#ifndef MSECOND_MS
#	define MSECOND_MS   1L
#	define SECOND_MS    (MSECOND_MS * 1000L)
#	define MINUTE_MS    (SECOND_MS * 60L)
#	define HOUR_MS      (MINUTE_MS * 60L)
#	define DAY_MS       (HOUR_MS * 24L)
#endif

/* Seconds based time units */

#ifndef SECOND
#	define SECOND      	1L
#	define MINUTE      	(SECOND * 60L)
#	define HOUR        	(MINUTE * 60L)
#	define DAY         	(HOUR * 24L)
#	define YEAR        	(DAY * 365L)
#endif

#define FMT_DEFAULT 0
#define FMT_VERBOSE 1

/* Types ----------------------------------------------------------- */

/* 48 bit timer type */
typedef struct _TIMER48 {
    UINT32 interval;                   /* 32 bit milliseconds = 49+17:02:47.295 */
    UINT32 lower;                      /* 48 bit milliseconds = ~8920 years */
    UINT16 upper;
} TIMER48;

/* Globals --------------------------------------------------------- */


/* Prototypes ------------------------------------------------------ */

#ifdef ANSI_C
VOID Timer48Init( void );
VOID Timer48Start( TIMER48 * timer, UINT32 interval );
VOID Timer48Restart( TIMER48 * timer );
BOOL Timer48Expired( TIMER48 * timer );
UINT32 Timer48MSElapsed( TIMER48 * timer );
UINT32 Timer48Elapsed( TIMER48 * timer );
VOID MSPause( UINT32 interval );
CHAR *MSIntervalToString( CHAR * string, UINT32 interval, UINT16 format );
CHAR *IntervalToString( CHAR * string, UINT32 interval, UINT16 format );
VOID Timer48Init( void );

#else
VOID Timer48Init( );
VOID Timer48Start(  );
VOID Timer48Restart(  );
BOOL Timer48Expired(  );
UINT32 Timer48MSElapsed(  );
UINT32 Timer48Elapsed(  );
VOID MSPause(  );
CHAR *MSIntervalToString(  );
CHAR *IntervalToString(  );

#endif

#endif

/* Revision History
 *
 * $Log$
 * Revision 1.4  2002/11/04 22:42:38  lombard
 * Removed DOS end-of-line chars for use on unix.
 *
 * Revision 1.3  2002/11/04 21:40:53  alex
 * *** empty log message ***
 *
 * Revision 1.2  2001/07/23 18:39:35  nobody
 * Cleanup, a few addtions for 1.9.11
 *
 * Revision 1.1.1.1  2000/06/22 19:13:09  nobody
 * Import existing sources into CVS
 *
 */
