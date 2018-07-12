/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: st_setupdate.c 2192 2006-05-25 15:32:13Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/05/25 15:32:13  paulf
 *     first checkin from Hydra
 *
 *     Revision 1.1  2005/06/30 20:40:12  mark
 *     Initial checkin
 *
 *     Revision 1.1  2005/04/21 16:51:30  mark
 *     Initial checkin
 *
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

_SUB	BOOL _tleap(DCC_WORD year)		/* Gregorian leap rules */
{

	if (year%400==0) return(TRUE);	/* Yes on 400s */
	if (year%100==0) return(FALSE);	/* No on 100s */
	if (year%4==0) return(TRUE);	/* Yes on 4s */
	return(FALSE);			/* Otherwise no */

}

STDTIME ST_GetCurrentTime(void);

_SUB	DCC_LONG _calyear(DCC_LONG dy,DCC_LONG epoch,UDCC_LONG timflgs)
{
/*  This routine cannot handle real time data that only transmits the year
    up to the decade or less in the station record header.  The problem
    occurs on a year that changes across the melinia for a time zone 
    ahead of the data processing center time zone.  Fortunately, all 
    stations that telemeter data should be transmitting the full year.     */

	STDTIME curtime;
	DCC_LONG tyear,tfact;

	tyear=dy;

	if (tyear>1900) return(tyear);		/* Don't labor the point */
				/* Not much digital seismic data before 1900 */

	if (epoch<1900) {
		curtime = ST_GetCurrentTime();
		epoch = curtime.year;
	}

	switch(timflgs) {

	case TM_CENT:	tfact=1000;
			break;
	case TM_DECADE:	tfact=100;
			break;
	default:	tfact=10;
			break;
	}

/* Calculate the year based on flags which show how many digits of
   the date are really present.  */

	tyear+=(epoch/tfact)*tfact;
	if (tyear>epoch+1) tyear-=tfact; /* +1 fudge factor needed only
   for stations that telemeter data and transmit the decade of the year
   or less in its station record headers, in which case the year of the
   data could end up greater than the year at the data processing center. */

	return(tyear);

}

_SUB	STDTIME	ST_SetupDate(DCC_LONG dy,DCC_LONG dd,DCC_LONG dh,DCC_LONG dm,
		DCC_LONG ds,DCC_LONG dms,DCC_WORD epoch,UDCC_LONG timflgs)
{

	STDTIME outtime;

	dy = _calyear(dy,epoch,timflgs);
	dd--;		/* Let jul date start at 0 for now */

/*-------------Normalize the date-------------*/

	timenorm(&dy,&dd,&dh,&dm,&ds,&dms);

	outtime.year = dy;
	outtime.day = dd + 1;
	outtime.hour = dh;
	outtime.minute = dm;
	outtime.second = ds;
	outtime.msec = dms;

	return(outtime);		/* Returning a structure! */

}
