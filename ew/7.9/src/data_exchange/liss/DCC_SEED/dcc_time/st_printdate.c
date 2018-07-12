/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: st_printdate.c 1248 2003-06-16 22:08:11Z patton $
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

_SUB	UDCC_BYTE *ST_PrintDate(STDTIME odate,BOOL fixfmt)
{

  DCC_LONG	ms;
  UDCC_BYTE 	msprt[8];

  if (fixfmt) {
    sprintf(_odata,"%04d,%03d,%02d:%02d:%02d.%03d",
	    (int) odate.year,
	    (int) odate.day,
	    (int) odate.hour,
	    (int) odate.minute,
	    (int) odate.second,
	    (int) odate.msec);
    return(_odata);
  }

  ms = odate.msec;
	
  if (ms==0) msprt[0]='\0';
  else if (ms%100==0) sprintf(msprt,".%01d",ms/100);
  else if (ms%10==0) sprintf(msprt,".%02d",ms/10);
  else sprintf(msprt,".%03d",ms);

  sprintf(_odata,"%d,%d,%d:%02d:%02d%s",
	  (int) odate.year,
	  (int) odate.day,
	  (int) odate.hour,
	  (int) odate.minute,
	  (int) odate.second,
	  msprt);

  return(_odata);

}
