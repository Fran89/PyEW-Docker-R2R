/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: st_spanprint.c 1248 2003-06-16 22:08:11Z patton $
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

static UDCC_BYTE _odata[60];

_SUB	UDCC_BYTE *ST_SpanPrint(STDTIME ftime,STDTIME etime,BOOL fixfmt)
{

	DCC_LONG	ms;
	UDCC_BYTE 	msprt[8];
	UDCC_BYTE	outfp[50];
	UDCC_BYTE	yrprt[8],
		dayprt[8];

	ms = ftime.msec;
	
	if (fixfmt) {
sprintf(_odata,"%d,%03d,%02d:%02d:%02d.%03d->%d,%03d,%02d:%02d:%02d.%03d",
			(int) ftime.year,
			(int) ftime.day,
			(int) ftime.hour,
			(int) ftime.minute,
			(int) ftime.second,
			(int) ftime.msec,
			(int) etime.year,
			(int) etime.day,
			(int) etime.hour,
			(int) etime.minute,
			(int) etime.second,
			(int) etime.msec);
		return(_odata);
	}

	if (ms==0) msprt[0]='\0';
	else if (ms%100==0) sprintf(msprt,".%01d",ms/100);
	else if (ms%10==0) sprintf(msprt,".%02d",ms/10);
	else sprintf(msprt,".%03d",ms);

	sprintf(outfp,"%d,%d,%d:%02d:%02d%s",
		(int) ftime.year,
		(int) ftime.day,
		(int) ftime.hour,
		(int) ftime.minute,
		(int) ftime.second,
		msprt);

	yrprt[0]='\0';
	dayprt[0]='\0';

	if (ftime.year!=etime.year) {
		sprintf(yrprt,"%d,",etime.year%10);
		sprintf(dayprt,"%d,",etime.day);
	} else if (ftime.day!=etime.day) {
		sprintf(dayprt,"%d,",etime.day);
	}

	ms = ftime.msec;
	
	if (ms==0) msprt[0]='\0';
	else if (ms%100==0) sprintf(msprt,".%01d",ms/100);
	else if (ms%10==0) sprintf(msprt,".%02d",ms/10);
	else sprintf(msprt,".%03d",ms);

	sprintf(_odata,"%s->%s%s%d:%02d:%02d%s",	
		outfp,
		yrprt,
		dayprt,
		(int) etime.hour,
		(int) etime.minute,
		(int) etime.second,
		msprt);

	return(_odata);
}
