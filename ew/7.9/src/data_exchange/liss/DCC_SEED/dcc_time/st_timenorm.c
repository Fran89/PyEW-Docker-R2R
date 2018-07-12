/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: st_timenorm.c 1248 2003-06-16 22:08:11Z patton $
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

#define _tdays(x) (_tleap(x)?366:365)

_SUB	VOID timenorm(DCC_LONG *dy,DCC_LONG *dd,DCC_LONG *dh,DCC_LONG *dm,DCC_LONG *ds,DCC_LONG *dms)
{

	DCC_LONG tl;

	if (abs(*dms)>999) {
		tl = *dms/1000;
		*ds += tl;
		*dms %= 1000;
	}		
	while (*dms<0) 	{	*dms+=1000;	(*ds)--;	}
	while (*dms>999) {	*dms-=1000;	(*ds)++;	}
	if (abs(*ds)>59) {
		tl = *ds/60;
		*dm += tl;
		*ds %= 60;
	}
	while (*ds<0) 	{	*ds+=60;	(*dm)--;	}
	while (*ds>59) 	{	*ds-=60;	(*dm)++;	}
	if (abs(*dm)>59) {
		tl = *dm/60;
		*dh += tl;
		*dm %= 60;
	}
	while (*dm<0) 	{	*dm+=60;	(*dh)--;	}
	while (*dm>59) 	{	*dm-=60;	(*dh)++;	}
	if (abs(*dh)>23) {
		tl = *dh/24;
		*dd += tl;
		*dh %= 24;
	}
	while (*dh<0) 	{	*dh+=24;	(*dd)--;	}
	while (*dh>23) 	{	*dh-=24;	(*dd)++;	}

	while (*dd>=_tdays(*dy)) {
		*dd -= _tdays(*dy);
		(*dy)++;
	}

	while (*dd<0) {
		(*dy)--;
		*dd += _tdays(*dy);
	}

	if (*dy>=4000) {
		*dy = 0;
		*dd = 1;
		*dh = 0;
		*dm = 0;
		*ds = 0;
		*dms = 0;
	}

	return;
}

_SUB	VOID timenormd(DCC_LONG *dd,DCC_LONG *dh,DCC_LONG *dm,DCC_LONG *ds,DCC_LONG *dms)
{

	DCC_LONG tl;

	if (abs(*dms)>999) {
		tl = *dms/1000;
		*ds += tl;
		*dms %= 1000;
	}		
	while (*dms<0) 	{	*dms+=1000;	(*ds)--;	}
	while (*dms>999) {	*dms-=1000;	(*ds)++;	}
	if (abs(*ds)>59) {
		tl = *ds/60;
		*dm += tl;
		*ds %= 60;
	}
	while (*ds<0) 	{	*ds+=60;	(*dm)--;	}
	while (*ds>59) 	{	*ds-=60;	(*dm)++;	}
	if (abs(*dm)>59) {
		tl = *dm/60;
		*dh += tl;
		*dm %= 60;
	}
	while (*dm<0) 	{	*dm+=60;	(*dh)--;	}
	while (*dm>59) 	{	*dm-=60;	(*dh)++;	}
	if (abs(*dh)>23) {
		tl = *dh/24;
		*dd += tl;
		*dh %= 24;
	}
	while (*dh<0) 	{	*dh+=24;	(*dd)--;	}
	while (*dh>23) 	{	*dh-=24;	(*dd)++;	}

	return;
}
