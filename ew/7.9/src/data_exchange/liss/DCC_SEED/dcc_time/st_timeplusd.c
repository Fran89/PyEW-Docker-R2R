/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: st_timeplusd.c 1248 2003-06-16 22:08:11Z patton $
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

#define _tdays(x) (_tleap(x)?366:365)

_SUB	STDTIME	ST_TimePlusDelta(STDTIME intime,DELTA_T indelta)
{

	STDTIME outtime;
	DCC_LONG dy,dd,dh,dm,ds,dms;

	dy = intime.year;
	dd = intime.day + indelta.nday - 1;
	dh = intime.hour + indelta.nhour;
	dm = intime.minute + indelta.nmin;
	ds = intime.second + indelta.nsec;
	dms = intime.msec + indelta.nmsecs;	

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
