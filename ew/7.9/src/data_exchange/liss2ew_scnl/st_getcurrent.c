/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: st_getcurrent.c 2192 2006-05-25 15:32:13Z paulf $
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

UDCC_BYTE _dmsize[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

#if VMS

_SUB	STDTIME ST_GetCurrentTime()
{

	UDCC_LONG sysret;
	UDCC_WORD numret[7];
	STDTIME rettime;
	DCC_WORD i,ct,mon,day;

	sysret = SYS$NUMTIM(numret,0);	/* Get current time */

	if (sysret!=EXIT_NORMAL) {
		printf("ST_GetCurrentTime SYS$NUMTIM failed\n");
	}

	rettime.year = numret[0];
	mon = numret[1];
	day = numret[2];
	rettime.hour = numret[3];
	rettime.minute = numret[4];
	rettime.second = numret[5];
	rettime.msec = numret[6] * 10;

	_dmsize[1] = _tleap(rettime.year)?29:28;

	ct=0;
	for (i=0; i<(mon-1); i++) ct+=_dmsize[i];
	ct += day;

	rettime.day = ct;

	return(rettime);

}

#else

STDTIME ST_GetCurrentTime(void)
{

	struct tm *intime;
	time_t tloc;

	STDTIME rettime;
	DCC_WORD i,ct,mon,day;

	time(&tloc);
	intime = gmtime(&tloc);

	rettime.year = intime->tm_year + 1900;
	mon = intime->tm_mon + 1;
	day = intime->tm_mday;
	rettime.hour = intime->tm_hour;
	rettime.minute = intime->tm_min;
	rettime.second = intime->tm_sec;
	rettime.msec = 0;

	_dmsize[1] = _tleap(rettime.year)?29:28;

	ct=0;
	for (i=0; i<(mon-1); i++) ct+=_dmsize[i];
	ct += day;

	rettime.day = ct;

	return(rettime);

}

#endif
