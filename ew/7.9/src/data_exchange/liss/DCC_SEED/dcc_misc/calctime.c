/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: calctime.c 44 2000-03-13 23:49:34Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/03/13 23:45:14  lombard
 *     Initial revision
 *
 *
 *
 */

/*----------------------------------------------------------------------*
 *	Modification and history 					*
 *-----Edit---Date----Who-------Description of changes------------------*
 *	000 04-Nov-88 RCM	Extract from routines			*
 *----------------------------------------------------------------------*/

#include <dcc_std.h>

#include <sys/times.h>
#include <limits.h>

#define QTICKS 100	/* times CPU time units are 1/100th of a second */

#ifndef LOGFILE
#define LOG printf
#endif

time_t qsecs[2];
struct tms sbuff[2];

_SUB void timeon()
{

  times(&sbuff[0]);
  time(&qsecs[0]);

}

_SUB void timeof()
{

  long	user_ticks,sys_ticks,frac;
	
  times(&sbuff[1]);
  time(&qsecs[1]);

  LOG("Elapsed seconds were %ld.\n",qsecs[1]-qsecs[0]);
  user_ticks=sbuff[1].tms_utime-sbuff[0].tms_utime;
  sys_ticks=sbuff[1].tms_stime-sbuff[0].tms_stime;
  if (user_ticks>0) {
    frac=((user_ticks%QTICKS)*1000)/QTICKS;
    user_ticks/=QTICKS;
    LOG("Elapsed user CPU seconds were %ld.%03ld.\n",user_ticks,frac);
  }
  if (sys_ticks>0) {
    frac=((sys_ticks%QTICKS)*1000)/QTICKS;
    sys_ticks/=QTICKS;
    LOG("Elapsed system CPU seconds were %ld.%03ld.\n",sys_ticks,frac);
  }
}
