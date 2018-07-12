/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: st_julian.c 2192 2006-05-25 15:32:13Z paulf $
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

#define CAL_CONS 1720982L	/* Nov, 1, 1BC */
	
extern UDCC_BYTE _dmsize[12];
extern BOOL _tleap(short int year);

/* This algorithm is only accurate from 1-Mar-1900 to 28-Feb-2100 */

_SUB DCC_LONG _julday(DCC_LONG year,DCC_LONG mon,DCC_LONG day)
{

	DCC_LONG julian,yearprime,monprime,dayprime;

	yearprime=year;
	monprime=mon+1;
	dayprime=day;

	if (mon==1||mon==2) {
		yearprime=year-1;
		monprime=mon+13;
	}

	julian=dayprime+CAL_CONS;
	julian+=(36525L*yearprime)/100L;
	julian+=(306001L*monprime)/10000L;

	return(julian);

}
_SUB DCC_LONG ST_Julian(DCC_LONG year, DCC_LONG mon, DCC_LONG day)
{

	return(_julday(year,mon,day));
}

/* This algorithm is only accurate from 1-Jan-1901 to 28-Feb-2100 */

_SUB VOID ST_CnvJulToCal(DCC_LONG injul,DCC_WORD *outyr,DCC_WORD *outmon,
	DCC_WORD *outday,DCC_WORD *outjday)
{

	int y1,m1,d,m,y,j,t;
	float a,b,jdu;

	jdu = (float)(injul - CAL_CONS);
	y1 = (int)((jdu-122.1)/365.25);
	a = (365.25f*((float)y1));
	m1 = (int)((jdu-a)/30.6001);
	b = (30.6001f*((float)m1));
	d = (int)(jdu - a - b);
	if (m1==14 || m1==15)
		m=m1-13;
	else 
		m=m1-1;
	if (m>2)
		y=y1;
	else 
		y=y1+1;

	if (_tleap((DCC_WORD)y)) 
		_dmsize[1]=29;
	else
		_dmsize[1]=28;
	
	j = d;
	for (t=0; t<(m-1); t++) j+=_dmsize[t];

	*outyr = y;
	*outmon = m;
	*outday = d;
	*outjday = j;

	return;	

}

_SUB STDTIME ST_CnvJulToSTD(JULIAN injul)
{

	STDTIME rettim = {0,0,0,0,0,0};

	DCC_WORD outyr,outmon,outday,outjday;

	ST_CnvJulToCal(injul,&outyr,&outmon,&outday,&outjday);

	rettim.year = outyr;
	rettim.day = outjday;

	return(rettim);
}

#ifdef MAINTEST

UDCC_BYTE _dmsize[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

BOOL _tleap(year)		/* Gregorian leap rules */
DCC_WORD year;
{

	if (year%400==0) return(TRUE);	/* Yes on 400s */
	if (year%100==0) return(FALSE);	/* No on 100s */
	if (year%4==0) return(TRUE);	/* Yes on 4s */
	return(FALSE);			/* Otherwise no */

}

main()
{

	DCC_LONG yr,mon,day;
	DCC_LONG jday,oday;
	DCC_WORD ry,rm,rd,rj,dm;

	yr = 1700;
	mon = 1;
	day = 1;
	jday = 0;

	printf("Month/Day test\n");

	FOREVER {

		if (yr>2300) break;
		if (mon==1&&day==1) {
			if (_tleap(yr)) _dmsize[1]=29;
			else _dmsize[1]=28;
		}

		jday = _julday(yr,mon,day);

		if (jday==0) oday==jday-1;

		if (oday!=jday-1) {
			printf("Calendar unsync to %d-%d-%d\n",yr,mon,day);
		}

		oday = jday;

		ST_CnvJulToCal(jday,&ry,&rm,&rd,&rj);

		if (ry!=yr||rd!=day||rm!=mon) {
			printf("Calendar reconv to %d-%d-%d\n",yr,mon,day);
			printf("   returned date   %d-%d-%d\n",ry,rm,rd);
		}

		day++;
		if (day>_dmsize[mon-1]) {
			day=1;
			mon++;
		}
		if (mon>12) {
			mon=1;
			yr++;
			/* if (yr%20==0) printf("Begin year %d\n",yr); */
		}

	}
	
	yr = 1700;
	day = 1;
	jday = 0;

	printf("Julian day of year test\n");

	FOREVER {

		if (yr>2300) break;
		if (mon==1&&day==1) {
			if (_tleap(yr)) dm=366;
			else dm=365;
		}

		jday = _julday(yr,1,day);

		if (jday==0) oday==jday-1;

		if (oday!=jday-1) {
			printf("Calendar unsync to %d-%d-%d\n",yr,mon,day);
		}

		oday = jday;

		ST_CnvJulToCal(jday,&ry,&rm,&rd,&rj);

		if (ry!=yr||rj!=day) {
			printf("Calendar reconv to %d-%d\n",yr,day);
			printf("   returned date   %d-%d\n",ry,rj);
		}

		day++;
		if (day>dm) {
			yr++;
			day = 1;
		}

	}
	

}

#endif
