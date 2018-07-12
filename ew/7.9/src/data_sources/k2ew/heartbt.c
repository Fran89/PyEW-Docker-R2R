/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: heartbt.c 5763 2013-08-09 01:27:39Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/07/28 22:36:10  lombard
 *     Initial revision
 *
 *
 *
 */

/* heartbt.c:  Heartbeat thread function for K2-to-Earthworm *
 *                                                           *
 *  3/16/99 -- [ET]                                          *
 *                                                           */

#include <stdio.h>
#include <time.h>
#include <earthworm.h>       /* Earthworm main include file */
#include "glbvars.h"         /* externs for global vars from 'k2ewmain.c' */
#include "heartbt.h"         /* header file for this module */
#include "k2misc.h"      

/**************************************************************************
 * k2ew_heartbeat_fn:  heartbeat thread function started up by the main   *
 *      thread; sends a heartbeat message to the Earthworm status manager *
 *      every 'gcfg_heartbeat_itvl' seconds; checks if main (input from   *
 *      K2) or read (output to EW) threads have died and terminates       *
 *      program if so                                                     *
 *         arg - parameter not used but needs to be declared              *
 **************************************************************************/

thr_ret k2ew_heartbeat_fn(void *arg)
{
  static time_t timevar,last_hb_time = 0;

  do
  {      /* loop while heartbeat thread is running */
    time(&timevar);                  /* get current system time */

    /* if time for heartbeat, and main thread is working, and either
       output thread hasn't started or it's working, then... */
    if ( (difftime(timevar,last_hb_time) >= (double)gcfg_heartbeat_itvl) &&
         (g_mt_working == 1) && 
         (g_output_threadid == (unsigned) -1 || g_ot_working == 1))
    {  
      /* Beat our heart */      
      k2mi_status_hb( g_heart_ltype, 0, "");
      last_hb_time = timevar;       /* save time for last heartbeat */
      
      /* Acknowledge the other threads */
      g_mt_working = 0;
      g_ot_working = 0;
    }
    sleep_ew(1000);             /* twiddle for a second */
  }
  while(g_terminate_flg == FALSE);     /* loop until terminate flag is set */
  return(NULL);
}

