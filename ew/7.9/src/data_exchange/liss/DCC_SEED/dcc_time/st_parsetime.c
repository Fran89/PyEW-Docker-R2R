/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: st_parsetime.c 3363 2008-09-26 22:25:00Z kress $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2008/09/26 22:25:00  kress
 *     Fix numerous compile warnings and some tab-related fortran errors for linux compile
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
#include "st_timepar.h"

static STDTIME setuptime;
static BOOL dateset,timeset;

void timeload(int hour,int minute,int second,int msec)
{

	timeset=TRUE;
	
	setuptime.hour=hour;
	setuptime.minute=minute;
	setuptime.second=second;
	setuptime.msec=msec;
}

void _setyear(void)
{

	int y,f;

	y = setuptime.year;
	if (y>1900) return;		/* Its probably ok */

	f=0;
	if (y>99) f=TM_CENT;
	else if (y>9) f=TM_DECADE;

	setuptime.year = _calyear(y,0,f);

}

void juldateset(int year,int day)
{

	dateset = TRUE;

	setuptime.year=year;
	_setyear();

	setuptime.day=day;

}

extern	UDCC_BYTE _dmsize[];

void nordate(int month,int day,int year)
{
	int ct,i;

	dateset = TRUE;
	setuptime.year = year;
	_setyear();

	_dmsize[1] = _tleap(setuptime.year)?29:28;

	ct=0;
	for (i=0; i<month-1; i++) ct+=_dmsize[i];
	ct += day;

	setuptime.day=ct;
}

STDTIME ST_CleanDate(STDTIME indate, DCC_WORD epoch, UDCC_LONG timflgs);

_SUB 	STDTIME	ST_ParseTime(UDCC_BYTE *inbuff)
{

	int getbuf,i;
	UDCC_BYTE	trsbuf[120];

	for (i=0; i<strlen(inbuff); i++) {
		if (isupper(inbuff[i])) trsbuf[i]=tolower(inbuff[i]);
		else trsbuf[i]=inbuff[i];
	}

	trsbuf[i++] = '\0';

	dateset=timeset=FALSE;

	parstart(trsbuf,strlen(trsbuf));	

	getbuf=timparse();

	if (getbuf!=0) {
		setuptime.year=0;
		return(setuptime);
	}

	if (!timeset) {
		setuptime.hour=0;
		setuptime.minute=0;
		setuptime.second=0;
		setuptime.msec=0;
	}

	if (!dateset) {
		setuptime.year=0;
		return(setuptime);
	}

	return(ST_CleanDate(setuptime,0,TM_MILEN));
				/* Returning a structure! */

}


