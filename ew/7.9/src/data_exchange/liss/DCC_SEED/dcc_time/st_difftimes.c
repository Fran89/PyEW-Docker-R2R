/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: st_difftimes.c 1248 2003-06-16 22:08:11Z patton $
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

_SUB	DELTA_T	ST_DiffTimes(STDTIME intime,STDTIME insect)
{

	DELTA_T outtime;
	DCC_LONG jul1,jul2;
	DCC_LONG dd,dh,dm,ds,dms;

	jul1 = _julday(intime.year,1,intime.day);
	jul2 = _julday(insect.year,1,insect.day);

	dd = jul1 - jul2;
	dh = intime.hour - insect.hour;
	dm = intime.minute - insect.minute;
	ds = intime.second - insect.second;
	dms = intime.msec - insect.msec;

/*-------------Normalize the date-------------*/

	timenormd(&dd,&dh,&dm,&ds,&dms);

	outtime.nday = dd;
	outtime.nhour = dh;
	outtime.nmin = dm;
	outtime.nsec = ds;
	outtime.nmsecs = dms;

	return(outtime);		/* Returning a structure! */

}
