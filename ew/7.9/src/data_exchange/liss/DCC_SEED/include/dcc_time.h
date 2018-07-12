/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: dcc_time.h 1248 2003-06-16 22:08:11Z patton $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2003/06/16 22:04:58  patton
 *     Fixed Microsoft WORD typedef issue
 *
 *     Revision 1.1  2000/03/05 21:47:33  lombard
 *     Initial revision
 *
 *
 *
 */

#ifndef TMLB_DEFINED
#define TMLB_DEFINED

struct	__sjtim	{		/* Day of year pseudo-julian date form */
	DCC_WORD	year;
	DCC_WORD	day;
	DCC_BYTE	hour;
	DCC_BYTE	minute;
	DCC_BYTE	second;
	DCC_WORD	msec;
};

typedef struct __sjtim STDTIME;

struct	__deltatime	{	/* Difference of a time amount */
	DCC_LONG	nday;
	UDCC_BYTE	nhour;
	UDCC_BYTE	nmin;
	UDCC_BYTE	nsec;
	UDCC_WORD	nmsecs;
};

typedef struct __deltatime DELTA_T;

typedef long JULIAN;

#define zerotime(x) (x.year == 0)

#define tpr(x) tfpr(stdout,x)
DCC_LONG	timsb();
BOOL	tleap();
STDTIME	*timall();

typedef double FLTTIME;    	/* Number of seconds since julian instant */

#define TM_MILEN 1000
#define TM_CENT 100
#define TM_DECADE 10

#define TIM_LIM (20L*86400L*1000L)	/* Time limit (fits in 31 bits) */
					/* 20 Days */

#define ST_SPAN_NIL	0	/* No intersection */
#define ST_SPAN_ACB	1	/* A contains B */
#define ST_SPAN_BCA	2	/* B contains A */
#define ST_SPAN_ALB	3	/* A less than B */
#define ST_SPAN_BLA	4	/* B less than A */

#include <dcc_time_proto.h>

#endif

