/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: heartbt.c 3536 2009-01-15 22:09:51Z tim $
 * 
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2009/01/15 22:09:51  tim
 *     Clean up
 *
 *
 */

#include <stdio.h>			/*likely not needed*/
#include <time.h>
#include <earthworm.h>       /* Earthworm main include file */
#include "glbvars.h"         /* externs for global vars from 'k2ewmain.c' */
//TODO: glbvars.h NEEDED for gcfg_heartbeat_itvl, g_heart_ltype!
#include "heartbt.h"         /* header file for this module */
#include "samtac2ew_misc.h"  /* header file that has samtac2mi_status_hb();  */

/*******************************************************************************
 *	int beat_heart(time_t *last_hb_time);                                      *
 *	                                                                           *
 *	This function beats the heart if needed.                                   *
 *	                                                                           *
 *	Pass by reference the last time the heart beat,                            *
 *	                                                                           *
 *	if it is time to beat it will update last_hb_time and return 1.            *
 *	if it is NOT time to beat, it will return 0.                               *
 ******************************************************************************/

int
beat_heart(time_t *last_hb_time)
{
	static time_t timevar = 0;
	
	/* get current system time */
	time(&timevar);

	// if it is time, beat the heart
	if (difftime(timevar,*last_hb_time) >= (double)gcfg_heartbeat_itvl) 
	{
		/* Beat our heart */
		samtac2mi_status_hb( g_heart_ltype, 0, "");
		if (gcfg_debug > 2)
		{
			logit("et", "heartbt: I just beat my heart\ndifftime is: %d and itvl is: %d\n lasthb: %d and now: %d\n", difftime(timevar,*last_hb_time), gcfg_heartbeat_itvl, *last_hb_time , timevar);
		}

		*last_hb_time = timevar;       /* save time for last heartbeat */
		if (gcfg_debug > 2)
		{
			logit("et", "heartbt: last_hb_time was just saved to:%d\n", *last_hb_time);
		}
        return 1;
    }
	// else return;
	return 0;
}
