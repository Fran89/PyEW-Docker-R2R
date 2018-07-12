/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: st_timecomp.c 44 2000-03-13 23:49:34Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/03/13 23:48:35  lombard
 *     Initial revision
 *
 *
 *
 */

#include <dcc_std.h>
#include <dcc_time.h>

/*Return 1 if 1st time > 2nd time
	-1 if 1st time < 2nd time
	 0 if 1st time = 2nd time
*/
_SUB	int ST_TimeComp(STDTIME intime,STDTIME insect)
{

	if (intime.year>insect.year) 		return(1);
	if (intime.year<insect.year) 		return(-1);
	if (intime.day>insect.day) 		return(1);
	if (intime.day<insect.day) 		return(-1);
	if (intime.hour>insect.hour) 		return(1);
	if (intime.hour<insect.hour) 		return(-1);
	if (intime.minute>insect.minute) 	return(1);
	if (intime.minute<insect.minute) 	return(-1);
	if (intime.second>insect.second) 	return(1);
	if (intime.second<insect.second) 	return(-1);
	if (intime.msec>insect.msec) 		return(1);
	if (intime.msec<insect.msec) 		return(-1);
	return(0);

}
