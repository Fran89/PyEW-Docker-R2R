/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: st_deltatoms.c 1248 2003-06-16 22:08:11Z patton $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2003/06/16 22:05:26  patton
 *     Fixed Microsoft WORD typedef issue
 *
 *     Revision 1.1  2000/03/13 23:48:35  lombard
 *     Initial revision
 *
 *
 *
 */

#include <dcc_std.h>
#include <dcc_time.h>

DELTA_T ST_MinusDelta(DELTA_T indelta);

_SUB	DCC_LONG ST_DeltaToMS(DELTA_T indelta)
{

	DCC_LONG totalms;
	BOOL minf = FALSE;

	if (indelta.nday>20) return(TIM_LIM);
	if (indelta.nday<-20) return(-TIM_LIM);

	if (indelta.nday<0) {
		minf = TRUE;
		indelta = ST_MinusDelta(indelta);
	}

	totalms = ((DCC_LONG) indelta.nday) * 86400L * 1000L;
	totalms += ((DCC_LONG) indelta.nhour) * 3600L * 1000L;
	totalms += ((DCC_LONG) indelta.nmin) * 60L * 1000L;
	totalms += ((DCC_LONG) indelta.nsec) * 1000L;
	totalms += ((DCC_LONG) indelta.nmsecs);

	if (minf) totalms = -totalms;

	if (totalms>TIM_LIM) totalms = TIM_LIM;
	if (totalms<-TIM_LIM) totalms = - TIM_LIM;

	return(totalms);

}
