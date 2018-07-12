/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: chron.c 1147 2002-11-25 22:55:49Z alex $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2002/11/25 22:55:49  alex
 *     Initial revision
 *
 *     Revision 1.1  2000/02/14 16:00:43  lucky
 *     Initial revision
 *
 *
 */

#include <sys/types.h>
#include <sys/timeb.h>
#include <stdio.h>
/*
 * chron.c : Time/date conversion routines.
 *
 *$ 91May07 CEJ Version 1.0
 */
/*********************C O P Y R I G H T   N O T I C E ***********************/
/* Copyright 1991 by Carl Johnson.  All rights are reserved. Permission     */
/* is hereby granted for the use of this product for nonprofit, commercial, */
/* or noncommercial publications that contain appropriate acknowledgement   */
/* of the author. Modification of this code is permitted as long as this    */
/* notice is included in each resulting source module.                      */
/****************************************************************************/

#include "chron.h"

struct Greg G;
int       mo[] =
{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334,
 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335};
char     *cmo[] =
{"Jan", "Feb", "Mar", "Apr", "May", "Jun",
 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

/*
 * Calculate julian minute from gregorian date and time.
 */
long      julmin(g)
     struct Greg *g;
{
   long      julian();

   return (1440L * (julian(g) - 2305448L) + 60L * g->hour + g->minute);
}

/*
 * Calculate gregorian date and time from julian minutes.
 */
struct Greg *grg(min)
     long      min;
{
   struct Greg *gregor();
   struct Greg *g;
   long      j;
   long      m;

   j = min / 1440L;
   m = min - 1440L * j;
   j += 2305448L;
   g = gregor(j);
   g->hour = m / 60;
   g->minute = m - 60 * g->hour;
   return (g);
}

/*
 * julian : Calculate julian date from gregorian date.
 */
long      julian(g)
     struct Greg *g;
{
   long      jul;
   int       leap;
   int       year;
   int       n;

   jul = 0;
   year = g->year;
   if (year < 1)
      goto x110;
   year--;
   jul = 365;

/* four hundred year rule */
   n = year / 400;
   jul += n * 146097L;
   year -= n * 400;

/* hundred year rule */
   n = year / 100;
   jul += n * 36524L;
   year -= n * 100;

/* four year rule */
   n = year / 4;
   jul += n * 1461L;
   year -= n * 4;

/* one year rule */
   jul += year * 365L;

/* Handle days in current year */
 x110:
   leap = 0;
   if (g->year % 4 == 0)
      leap = 12;
   if (g->year % 100 == 0)
      leap = 0;
   if (g->year % 400 == 0)
      leap = 12;
   jul += mo[g->month + leap - 1] + g->day + 1721060L;
   return (jul);
}

/*
 * gregor : Calculate gregorian date from julian date.
 */
struct Greg *gregor(min)
     long      min;
{
   long      test;
   long      check;
   int       leap;
   int       left;
   int       imo;

   G.year = (min - 1721061L) / 365L;
   G.month = 1;
   G.day = 1;
   test = julian(&G);
   if (test <= min)
      goto x110;

 x20:
   G.year--;
   test = julian(&G);
   if (test > min)
      goto x20;
   goto x210;

 x105:
   G.year++;
   test = julian(&G);

 x110:
   check = test - min - 366L;
   if (check < 0)
      goto x210;
   if (check > 0)
      goto x105;

   if (G.year % 400 == 0)
      goto x210;
   if (G.year % 100 == 0)
      goto x105;
   if (G.year % 4 == 0)
      goto x210;
   goto x105;

 x210:
   left = min - test;
   leap = 0;
   if (G.year % 4 == 0)
      leap = 12;
   if (G.year % 100 == 0)
      leap = 0;
   if (G.year % 400 == 0)
      leap = 12;
   for (imo = 1; imo < 12; imo++)
   {
      if (mo[imo + leap] <= left)
         continue;
      G.month = imo;
      G.day = left - mo[imo + leap - 1] + 1;
      return (&G);
   }
   G.month = 12;
   G.day = left - mo[11 + leap] + 1;
   return (&G);
}

/*
 * date18 : Calcualate 18 char date in the form 88Jan23 1234 12.21
 *              from the julian seconds.  Remember to leave space for the
 *              string termination (NUL).
 */
void date18(double secs, char *c18)
{
   struct Greg *g;
   long      minute;
   double    sex;
   int       hrmn;

   minute = (long)(secs / 60.0);
   sex = secs - 60.0 * minute;
   g = grg(minute);
   hrmn = 100 * g->hour + g->minute;
   sprintf(c18, "%2d %3s %2d %2.2d:%2.2d:%5.2f",
        g->year - 1900, cmo[g->month - 1], g->day, g->hour, g->minute, sex);
   return;
}

/*
 * now : Returns current system time for time stamping
 */
double    tnow()
{
   struct Greg g;
   struct timeb q;
   double    secs;

   g.year = 1970;
   g.month = 1;
   g.day = 1;
   g.hour = 0;
   g.minute = 0;
   ftime(&q);
   secs = 60.0 * julmin(&g) + q.time + 0.001 * q.millitm;
   return secs;
}
