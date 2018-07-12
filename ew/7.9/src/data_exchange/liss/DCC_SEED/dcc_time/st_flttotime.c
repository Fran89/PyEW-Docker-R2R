/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: st_flttotime.c 1248 2003-06-16 22:08:11Z patton $
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

_SUB	STDTIME	ST_FLTToTime(FLTTIME intime)
{

	STDTIME outtime;

	DCC_LONG secpd,juldate;
	DCC_WORD t1;
	FLTTIME tmpacum=0.0;

	juldate = tmpacum/86400.0;
	tmpacum = fmod(intime, 86400.0);	/* Calculate Days */	

	ST_CnvJulToCal(juldate,&outtime.year,&t1,&t1,&outtime.day);

	secpd = tmpacum;
	tmpacum = ((tmpacum - ((FLTTIME) secpd)) * 1000.0) + .5;
	outtime.second = secpd % 60;
	secpd /= 60;
	outtime.minute = secpd % 60;
	secpd /= 60;
	outtime.hour = secpd;
	outtime.msec = tmpacum;

	return(outtime);

}
