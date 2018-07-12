/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: st_timetoflt.c 1248 2003-06-16 22:08:11Z patton $
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

_SUB	FLTTIME	ST_TimeToFLT(STDTIME intime)
{

	DCC_LONG jul1,secpd;
	FLTTIME tmpacum;

	jul1 = _julday(intime.year,1,intime.day);

	tmpacum = jul1;
        /* #define SECONDS_PER_DAY 24 * 60 * 60  = 86400 */
	tmpacum *= 86400.0;

	secpd = intime.hour * 3600;
	secpd += intime.minute * 60;
	secpd += intime.second;
	tmpacum += ((FLTTIME) secpd);
	tmpacum += ((FLTTIME) intime.msec) / 1000.0;

	return(tmpacum);

}
