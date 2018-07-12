/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: st_printdec.c 1248 2003-06-16 22:08:11Z patton $
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

LOCAL UDCC_BYTE _ddata[60];

LOCAL UDCC_BYTE *ShortNames[12] = {
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec" };

_SUB	UDCC_BYTE *ST_PrintDECDate(STDTIME odate,BOOL printtime)
{

	DCC_LONG jdate;
	DCC_WORD mon,day,jday,jyr;

	jdate = ST_GetJulian(odate);

	ST_CnvJulToCal(jdate,&jyr,&mon,&day,&jday);

	if (printtime) 
		sprintf(_ddata,"%02d-%s-%04d:%02d:%02d:%02d.%03d",
			day,ShortNames[mon-1],jyr,
			odate.hour,odate.minute,odate.second,odate.msec);
	else
		sprintf(_ddata,"%02d-%s-%04d",day,ShortNames[mon-1],jyr);

	return(_ddata);

}
