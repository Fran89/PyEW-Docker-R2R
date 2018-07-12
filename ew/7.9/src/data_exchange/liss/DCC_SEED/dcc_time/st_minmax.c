/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: st_minmax.c 44 2000-03-13 23:49:34Z lombard $
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

_SUB 	STDTIME ST_TimeMin(STDTIME intime,STDTIME insect)
{

	if (ST_TimeComp(intime,insect)<=0) return(intime);
	else return(insect);

}

_SUB 	STDTIME ST_TimeMax(STDTIME intime,STDTIME insect)
{

	if (ST_TimeComp(intime,insect)>=0) return(intime);
	else return(insect);

}

