      /******************************************************************
       *                         mjstime()                              *
       *                                                                *
       *  These functions convert calendar time from/to modified Julian *
       *  time. What is Julian time?  Well, let's start with Julian day.*
       *  The Julian day is not the day of year as it is often called.  *
       *  The actual Julian day is the number of days since 1200UTC on 1*
       *  January, -4713.  A time base known as the Modified Julian Day *
       *  (MJD) is the time base used in WC&ATWC seismic programs.      *
       *  The MJD starts at 0000UTC 17 November, 1858.  The time in     *
       *  seconds from this point onward is called the modified Julian  *
       *  time in these programs.                                       *
       *                                                                *
       *  Starting in the late 1990s, WCATWC started using epochal time *
       *  (since 1/1/1970) as the time base for consistency with almost *
       *  all other computer applications. Some of these functions      *
       *  accomodate this type of conversion.                           *                                                                *
       *                                                                *
       *  April, 2006: Added TWCgmtime since gmtime often causes probs. *
       *                                                                *
       *   Contributors:       Zitek, Whitmore                          *
       *   Date:               1990-2004                                *
       *                                                                *
       ******************************************************************/

#ifdef _WINNT
 #define _CRTDBG_MAP_ALLOC
#endif

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <transport.h>
#ifdef _WINNT
 #include <crtdbg.h>
#endif

#include <earthworm.h>
#include "earlybirdlib.h"

      /******************************************************************
       *                         ConvertTM2ST()                         *
       *                                                                *
       *  This function converts the standard C time structure (tm) to  *
       *  a Windows SYSTEMTIME structure.                               *
       *                                                                *
       *  NOTE: Milliseconds are not contained in tm so must be computed*
       *        in the calling program                                  *
       *                                                                *
       *  Arguments:                                                    *
       *   ptmFrom          Input time structure (tm)                   *
       *   pstTo            Output SYSTEMTIME structure                 *
       *                                                                *
       ******************************************************************/
void ConvertTM2ST( struct tm *ptmFrom, SYSTEMTIME *pstTo )
{
   pstTo->wYear = ptmFrom->tm_year+1900;
   pstTo->wMonth = ptmFrom->tm_mon+1;
   pstTo->wDay = ptmFrom->tm_mday;
   pstTo->wHour = ptmFrom->tm_hour;
   pstTo->wMinute = ptmFrom->tm_min;
   pstTo->wSecond = ptmFrom->tm_sec;
   pstTo->wDayOfWeek = ptmFrom->tm_wday;
}

      /******************************************************************
       *                         CopyDate()                             *
       *                                                                *
       *  This function copies one SYSTEMTIME structure to another.     *
       *                                                                *
       *  Arguments:                                                    *
       *   pstFrom          Input SYSTEMTIME structure                  *
       *   pstTo            Output SYSTEMTIME structure                 *
       *                                                                *
       ******************************************************************/
void CopyDate( SYSTEMTIME *pstFrom, SYSTEMTIME *pstTo )
{
   pstTo->wYear         = pstFrom->wYear;
   pstTo->wMonth        = pstFrom->wMonth;
   pstTo->wDay          = pstFrom->wDay;
   pstTo->wDayOfWeek    = pstFrom->wDayOfWeek;
   pstTo->wHour         = pstFrom->wHour;
   pstTo->wMinute       = pstFrom->wMinute;
   pstTo->wSecond       = pstFrom->wSecond;
   pstTo->wMilliseconds = pstFrom->wMilliseconds;
}

      /******************************************************************
       *                         DateToDay()                            *
       *                                                                *
       * This function converts a year/month/day to the day of year.    *
       *                                                                *
       *  Arguments:                                                    *
       *   pst              SystemTime struct. with starting date       *
       *                                                                *
       *  Return:                                                       *
       *   int              Day of year.                                *
       *                                                                *
       ******************************************************************/
int DateToDay( SYSTEMTIME *pst )
{
   static int  iDay;            /* Day-of-year computed from mmdd */
   static int  iLeap;           /* 1 -> this is a leap year */

        /* Is it a leap year? */
   if ( (pst->wYear % 4) == 0 &&
       ((pst->wYear % 100) != 0 || (pst->wYear % 400) == 0) )
        iLeap = 1;
   else iLeap = 0;

   if ( pst->wMonth == 1 ) iDay = pst->wDay;
   else if ( pst->wMonth == 2 ) iDay = 31 + pst->wDay;
   else if ( pst->wMonth == 3 ) iDay = 59 + pst->wDay + iLeap;
   else if ( pst->wMonth == 4 ) iDay = 90 + pst->wDay + iLeap;
   else if ( pst->wMonth == 5 ) iDay = 120 + pst->wDay + iLeap;
   else if ( pst->wMonth == 6 ) iDay = 151 + pst->wDay + iLeap;
   else if ( pst->wMonth == 7 ) iDay = 181 + pst->wDay + iLeap;
   else if ( pst->wMonth == 8 ) iDay = 212 + pst->wDay + iLeap;
   else if ( pst->wMonth == 9 ) iDay = 243 + pst->wDay + iLeap;
   else if ( pst->wMonth == 10 ) iDay = 273 + pst->wDay + iLeap;
   else if ( pst->wMonth == 11 ) iDay = 304 + pst->wDay + iLeap;
   else iDay = 334 + pst->wDay + iLeap;
   return iDay;
}

      /******************************************************************
       *                DateToModJulianSec()                            *
       *                                                                *
       * This function converts the date/time to modified Julian time   *
       * in seconds. This will work until 2100 due to it not being a    *
       * leap year.                                                     *
       *                                                                *
       * July, 2011: Changed argument to a pointer - PW                 *
       *                                                                *
       *  Arguments:                                                    *
       *   stDate           Date/time structure to convert              *
       *                                                                *
       *  Return:                                                       *
       *   double           Time in modified julian seconds             *
       *                                                                *
       ******************************************************************/
void DateToModJulianSec( SYSTEMTIME *pstDate, double *pdJulianSec )
{
   int    iLeapDays;	/* Number of leap days since 1858 */
   int    iYear;        /* Number of years since 1861 */
   long   lDays;	/* Day-of-year of time to convert */

   iYear = pstDate->wYear - 1861;
   iLeapDays = iYear / 4 - 1;
   lDays = (long) DateToDay( pstDate );
   iYear = pstDate->wYear - 1858;
   lDays += (long) (iYear*365 + iLeapDays - 321);
   *pdJulianSec = (double) lDays * 86400.0;
   *pdJulianSec = *pdJulianSec + (double) pstDate->wHour * 3600.0;
   *pdJulianSec = *pdJulianSec + (double) pstDate->wMinute * 60.0;
   *pdJulianSec = *pdJulianSec + (double) pstDate->wSecond;
   *pdJulianSec = *pdJulianSec + (double) pstDate->wMilliseconds/1000. + 0.000001;
   return;
}

      /******************************************************************
       *                         DayToDate()                            *
       *                                                                *
       * This function converts the day-of-year to the date and month   *
       * in a SYSTEMTIME function.                                      *
       *                                                                *
       *  Arguments:                                                    *
       *   lDay             Day of year                                 *
       *   pst              ptr to structure to put new date in         *
       *                                                                *
       ******************************************************************/
void DayToDate( long lDay, SYSTEMTIME *pst )
{
   static int    iLeap;                   /* 1 -> this is a leap year */

   if ( (pst->wYear % 4) == 0 &&
       ((pst->wYear % 100) !=0 || (pst->wYear % 400) == 0) )
        iLeap = 1;
   else iLeap = 0;

   while ( lDay > 365 + iLeap )
   {
      lDay -= 365 + iLeap;
      pst->wYear++;
      if ( (pst->wYear % 4) == 0 &&
          ((pst->wYear % 100) !=0 || (pst->wYear % 400) == 0) )
           iLeap = 1;
      else iLeap = 0;
   }
   if ( lDay > 334 + iLeap )
   {
      pst->wMonth = 12;
      pst->wDay = (unsigned short) (lDay - 334 - iLeap);
   }
   else if ( lDay > 304 + iLeap )
   {
      pst->wMonth = 11;
      pst->wDay = (unsigned short) (lDay - 304 - iLeap);
   }
   else if ( lDay > 273+iLeap )
   {
      pst->wMonth = 10;
      pst->wDay = (unsigned short) (lDay - 273 - iLeap);
   }
   else if ( lDay > 243+iLeap )
   {
      pst->wMonth = 9;
      pst->wDay = (unsigned short) (lDay - 243 - iLeap);
   }
   else if ( lDay > 212+iLeap )
   {
      pst->wMonth = 8;
      pst->wDay = (unsigned short) (lDay - 212 - iLeap);
   }
   else if ( lDay > 181+iLeap )
   {
      pst->wMonth = 7;
      pst->wDay = (unsigned short) (lDay - 181 - iLeap);
   }
   else if ( lDay > 151+iLeap )
   {
      pst->wMonth = 6;
      pst->wDay = (unsigned short) (lDay - 151 - iLeap);
   }
   else if ( lDay > 120+iLeap )
   {
      pst->wMonth = 5;
      pst->wDay = (unsigned short) (lDay - 120 - iLeap);
   }
   else if ( lDay > 90+iLeap )
   {
      pst->wMonth = 4;
      pst->wDay = (unsigned short) (lDay - 90 - iLeap);
   }
   else if ( lDay > 59+iLeap )
   {
      pst->wMonth = 3;
      pst->wDay = (unsigned short) (lDay - 59 - iLeap);
   }
   else if ( lDay > 31 )
   {
      pst->wMonth = 2;
      pst->wDay = (unsigned short) (lDay - 31);
   }
   else
   {
      pst->wMonth = 1;
      pst->wDay = (unsigned short) (lDay) ;
   }
}

      /******************************************************************
       *                 IsDayLightSavings()                            *
       *                                                                *
       * This function determines if the time sent (which is UTC) + the *
       * Offset*3600 (which is the difference from the time zone we are *
       * interested in and Zulu) is presently observing Daylight Savings*
       * time (DT).  United States rules are used in determining whether*
       * or not it is DT.  That is, at 2AM Standard time (ST) on the    *
       * first Sunday in April, DT starts.  At 2AM DT (or 1AM ST) on the*
       * last Sunday in October, DT ends.  This function should work    *
       * as long as the DT time change rules stay the same.             *
       *                                                                *
       * Dec., 2006: Changed dates where DST is active to 2nd Sunday in *
       *             March to 1st Sunday in Nov.                        *
       *                                                                *
       *  Arguments:                                                    *
       *   pst        System time structure with UTC time               *
       *   iOffset    Hours difference from time zone of interest       *
       *              (standard time) to Z e.g. Alaska = -9, west       *
       *              coast = -8                                        *
       *                                                                *
       *  Return:                                                       *
       *   int              1 if in DT period, 0 -> ST                  *
       *                                                                *
       ******************************************************************/
int IsDayLightSavings ( SYSTEMTIME *pst, int iOffset )
{
   static double        dTime;       /* Present MJS time */
   static SYSTEMTIME    stLocalStd;  /* Present time -> to local, std time */

/* Convert UTC SYSTEMTIME structure to modified Julian seconds */
   DateToModJulianSec( pst, &dTime );
/* Convert this to SYSTEMTIME structure of local, standard time */
   NewDateFromModSec( &stLocalStd, (dTime + (double) iOffset*60.*60.) );

/* Check months for quick DT check */
   if ( stLocalStd.wMonth < 3 || stLocalStd.wMonth > 11 ) return 0; /* ST */
   if ( stLocalStd.wMonth > 3 && stLocalStd.wMonth < 11 ) return 1; /* DT */

/* Check if we're in March (2nd Sunday change to DT at 2AM ST) */
   if ( stLocalStd.wMonth == 3 )           /* March */
   {    /* If past 2nd week, it must be DT; of in first week must be ST */
      if ( stLocalStd.wDay > 14 ) return 1; /* DT */
      if ( stLocalStd.wDay < 8 )  return 0; /* ST */
/* Otherwise, we are in the second week */
/* See if it is Sunday now */
      if ( stLocalStd.wDayOfWeek == 0 )	         /* Sunday */
      {    /* Check if it's before 2AM ST */
         if ( stLocalStd.wHour <  2 ) return 0;  /* ST */
         if ( stLocalStd.wHour >= 2 ) return 1;  /* DT */
      }
/* Next, has Sunday already occurred in this week */
      if ( stLocalStd.wDayOfWeek+7 >= stLocalStd.wDay ) return 0; /* ST */
      if ( stLocalStd.wDayOfWeek+7 <  stLocalStd.wDay ) return 1; /* DT */
   }

/* Check if we're in November (first Sunday change to DT at 1AM ST or 2AM DT) */
   if ( stLocalStd.wMonth == 11 )             /* November */
   {    /* If after first week, it must be ST */
      if ( stLocalStd.wDay > 7 ) return 0;  /* ST */
/* Otherwise, we are in the first week */
/* See if it is Sunday now */
      if ( stLocalStd.wDayOfWeek == 0 )         /* Sunday */
      {     /* Check if it's before 1AM ST (same as 2AM DT) */
         if ( stLocalStd.wHour < 1 ) return 1;  /* DT */
         if ( stLocalStd.wHour >= 1 ) return 0; /* ST */
      }
/* Next, has Sunday already occurred in this week */
      if ( stLocalStd.wDayOfWeek >= stLocalStd.wDay ) return 1; /* DT */
      if ( stLocalStd.wDayOfWeek <  stLocalStd.wDay ) return 0; /* ST */
   }
   return 0;          /* Assume ST if there was a problem */
}

      /******************************************************************
       *                      IsDayLightSavings2()                      *
       *                                                                *
       * This function determines if the time sent (which is UTC) + the *
       * Offset*3600 (which is the difference from the time zone we are *
       * interested in and Zulu) is presently observing Daylight Savings*
       * time (DT).  United States rules are used in determining whether*
       * or not it is DT.  That is, at 2AM Standard time (ST) on the    *
       * first Sunday in April, DT starts.  At 2AM DT (or 1AM ST) on the*
       * last Sunday in October, DT ends.  This function should work as *
       * long as the DT time change rules stay the same.                *
       *                                                                *
       * Dec., 2006: Changed dates where DST is active to 2nd Sunday in *
       *             March to 1st Sunday in Nov.                        *
       *                                                                *
       *  Arguments:                                                    *
       *   dTime            1/1/70 time of interest in seconds          *
       *   iOffset          ST offset of time zone of interest (ak -9)  *
       *                                                                *
       *  Return:                                                       *
       *   1->DT, 0->ST                                                 *
       *                                                                *
       ******************************************************************/
	   
int IsDayLightSavings2( double dTime, int iOffset )
{
   long   lTime;                /* dTime converted to long */
   struct tm *tm;               /* time structure of dTime (local) */

/* Put 1/1/70 time in seconds into tm time structure of Local std time zone */
   lTime = (long) (dTime + (double) iOffset*60.*60.);
   tm = TWCgmtime( lTime );

/* Check months for quick DT check */
   if ( tm->tm_mon+1 < 3 || tm->tm_mon+1 > 11 ) return 0;	/* ST */
   if ( tm->tm_mon+1 > 3 && tm->tm_mon+1 < 11 ) return 1;	/* DT */

/* Check if we're in March (2nd Sunday change to DT at 2AM ST) */
   if (tm->tm_mon+1 == 3)                  /* March */
   {    /* If past 2nd week, it must be DT; of in first week must be ST */
      if ( tm->tm_mday > 14 ) return 1;    /* DT */
      if ( tm->tm_mday < 8 )  return 0;    /* ST */
	  
/* Otherwise, we are in the second week. See if it is Sunday now */
      if ( tm->tm_wday == 0 )                  /* Sunday */
      {                                        /* Check if it's before 2AM ST */
         if ( tm->tm_hour < 2 )  return 0;     /* ST */
         if ( tm->tm_hour >= 2 ) return 1;     /* DT */
      }
	  
/* Next, has Sunday already occurred in this week */
      if ( tm->tm_wday+7 >= tm->tm_mday ) return 0;/* ST */
      if ( tm->tm_wday+7 < tm->tm_mday )  return 1;/* DT */
   }

/* Check if we're in November (first Sunday change to DT at 1AM ST or 2AM DT) */
   if ( tm->tm_mon+1 == 11 )            /* November */
   {    /* If after first week, it must be ST */
      if ( tm->tm_mday > 7 ) return 0;  /* ST */
	  
/* Otherwise, we are in the first week. See if it is Sunday now */
      if ( tm->tm_wday == 0 )              /* Sunday */
      {                       /* Check if it's before 1AM ST (same as 2AM DT) */
         if ( tm->tm_hour < 1 )  return 1; /* DT */
         if ( tm->tm_hour >= 1 ) return 0; /* ST */
      }
	  
/* Next, has Sunday already occurred in this week */
      if ( tm->tm_wday >= tm->tm_mday ) return 1; /* DT */
      if ( tm->tm_wday <  tm->tm_mday ) return 0; /* ST */
   }
   return 0;               /* Assume ST if there was a problem */
}

      /******************************************************************
       *                         MSDayToDate()                          *
       *                                                                *
       *  This function converts day of year to the tm struct day and   *
       *  month.                                                        *
       *                                                                *
       *  NOTE: Milliseconds are not contained in tm so must be computed*
       *        in the calling program                                  *
       *                                                                *
       *  Arguments:                                                    *
       *   lDay             # of days                                   *
       *   tmLocal          Output time structure (tm)                  *
       *                                                                *
       ******************************************************************/

void MSDayToDate( long lDay, struct tm *tmLocal )
{
   static  int     iLeap;		/* 1 -> this is a leap year */

   if ( ((tmLocal->tm_year+1900) % 4) == 0 &&
       (((tmLocal->tm_year+1900) % 100) != 0 || ((tmLocal->tm_year+1900) % 400) == 0) )
      iLeap = 1;
   else iLeap = 0;

   while ( lDay > 365 + iLeap )
   {
      lDay -= 365 + iLeap;
      tmLocal->tm_year++;
      tmLocal->tm_yday = lDay-1;
      if ( ((tmLocal->tm_year+1900) % 4) == 0 &&
          (((tmLocal->tm_year+1900) % 100) != 0 || ((tmLocal->tm_year+1900) % 400) == 0) )
         iLeap = 1;
      else iLeap = 0;
   }
   if ( lDay > 334 + iLeap )
   {
      tmLocal->tm_mon = 11;
      tmLocal->tm_mday = (unsigned short) (lDay - 334 - iLeap);
   }
   else if ( lDay > 304 + iLeap )
   {
      tmLocal->tm_mon = 10;
      tmLocal->tm_mday = (unsigned short) (lDay - 304 - iLeap);
   }
   else if ( lDay > 273+iLeap )
   {
      tmLocal->tm_mon = 9;
      tmLocal->tm_mday = (unsigned short) (lDay - 273 - iLeap);
   }
   else if ( lDay > 243+iLeap )
   {
      tmLocal->tm_mon = 8;
      tmLocal->tm_mday = (unsigned short) (lDay - 243 - iLeap);
   }
   else if ( lDay > 212+iLeap )
   {
      tmLocal->tm_mon = 7;
      tmLocal->tm_mday = (unsigned short) (lDay - 212 - iLeap);
   }
   else if ( lDay > 181+iLeap )
   {
      tmLocal->tm_mon = 6;
      tmLocal->tm_mday = (unsigned short) (lDay - 181 - iLeap);
   }
   else if ( lDay > 151+iLeap )
   {
      tmLocal->tm_mon = 5;
      tmLocal->tm_mday = (unsigned short) (lDay - 151 - iLeap);
   }
   else if ( lDay > 120+iLeap )
   {
      tmLocal->tm_mon = 4;
      tmLocal->tm_mday = (unsigned short) (lDay - 120 - iLeap);
   }
   else if ( lDay > 90+iLeap )
   {
      tmLocal->tm_mon = 3;
      tmLocal->tm_mday = (unsigned short) (lDay - 90 - iLeap);
   }
   else if ( lDay > 59+iLeap )
   {
      tmLocal->tm_mon = 2;
      tmLocal->tm_mday = (unsigned short) (lDay - 59 - iLeap);
   }
   else if ( lDay > 31 )
   {
      tmLocal->tm_mon = 1;
      tmLocal->tm_mday = (unsigned short) (lDay - 31);
   }
   else
   {
      tmLocal->tm_mon = 0;
      tmLocal->tm_mday = (unsigned short) (lDay) ;
   }
}

      /******************************************************************
       *                 NewDateFromModSec()                            *
       *                                                                *
       * This function converts the value of the modified Julian second *
       * to a SYSTEMTIME structure. This will work until 2100 due to it *
       * not being a leap year.                                         *
       *                                                                *
       *  Arguments:                                                    *
       *   pstNew     Structure into which to put the time              *
       *   dOldSec    Modified Julian sec to convert                    *
       *                                                                *
       ******************************************************************/
void NewDateFromModSec( SYSTEMTIME *pstNew, double dOldSec )
{
   static int    iLeap;      /* 1 if year is leap year; else 0 */
   static int    iLeapdays;  /* Number of leap days since start of MJD */
/* This array is the start day (0=Sunday, 1=Mon, ...) of each
   year starting in 1901.  The start day repeats every 28 years.
   This will work until 2100 since that year is not a leap year.
   The array's purpose is to give a way to fill in the
   wDayOfYear field in the SYSTEMTIME structure */
   static int    iStartDay[28] = {2,3,4,5,0,1,2,3,5,6,0,1,3,4,5,6,1,2,3,4,6,0,1,
                                  2,4,5,6,0};
   static int    iTemp;
   static int    iYears;        /* Number of years since start of MJD */
   static int    iYearStartDay; /* Starting DayOfWeek of this year (0=Sun...) */
   static long   lDays;         /* Number of days since start of MJD */
   static long   lTemp;

   lDays = (long) (dOldSec / 86400.0);

   dOldSec -= (double) lDays * 86400.0;
   lDays += 321;
   iYears = (int) (lDays / 365);
   iLeapdays = (iYears - 3) / 4 - 1;
   lDays -= (long) iYears * 365 + (long) iLeapdays;

   if ( lDays <= 0 )
   {
      lDays += (long) iLeapdays;
      iYears--;
      iLeapdays = (iYears - 3) / 4 - 1;
      lDays += 365 - (long) iLeapdays;
   }
   pstNew->wYear = (unsigned short) (iYears + 1858);
/* Figure out which day of the week this is */
   lTemp = lDays;
   iTemp = pstNew->wYear;
   iLeap = 0;
   if ( (iTemp % 4) == 0 &&
       ((iTemp % 100) !=0 || (iTemp % 400) == 0) ) iLeap = 1;
   while ( lTemp > 365 + iLeap )
   {
      lTemp -= (365 + iLeap);
      iTemp++;
      if ( (iTemp % 4) == 0 &&
          ((iTemp % 100 != 0) || (iTemp % 400) == 0) ) iLeap = 1;
      else iLeap = 0;
   }
   iYearStartDay = iStartDay[((iTemp-1901) % 28)];
   pstNew->wDayOfWeek = (WORD) (((lTemp-1) + iYearStartDay) % 7);
/* Fill in time */
   pstNew->wHour = (unsigned short) (dOldSec / 3600.);
   dOldSec -= 3600.0 * (double) (pstNew->wHour);
   pstNew->wMinute = (unsigned short) (dOldSec / 60.);
   dOldSec -= 60.0 * (double) pstNew->wMinute ;
   pstNew->wSecond = (unsigned short) dOldSec;
   dOldSec -= (double) pstNew->wSecond;
   pstNew->wMilliseconds = (unsigned short) ((dOldSec) * 1000. + .4999);
   DayToDate( lDays, pstNew );
}

      /******************************************************************
       *                 NewDateFromModSecRounded()                     *
       *                                                                *
       * This function converts the value of the modified Julian second *
       * to a SYSTEMTIME structure with the time rounded to the nearest *
       * minute. See DateToModJulianSec for a discussion on Julian      *
       * seconds. This will work until 2100 due to it not being a leap  *
       * year.                                                          *
       *                                                                *
       *  Arguments:                                                    *
       *   pstNew     Structure into which to put the time              *
       *   dOldSec    Modified Julian sec to convert                    *
       *                                                                *
       ******************************************************************/
void NewDateFromModSecRounded( SYSTEMTIME *pstNew, double dOldSec )
{
   static long  lDays;     /* Number of days since start of MJD */
   static int   iLeapdays; /* Number of leap days since start of MJD */
   static int   iYears;    /* Number of years since start of MJD */
   static int   iLeap;     /* 1 -> This is a leap year */
	
   iLeap = 0;
   lDays = (long) (dOldSec / 86400.0);

   dOldSec -= (double) lDays * 86400.0;
   lDays += 321;
   iYears = (int) (lDays / 365);
   iLeapdays = (iYears - 3) / 4 - 1;
   lDays -= (long) iYears * 365 + (long) iLeapdays;

   if ( lDays <= 0 )
   {
      lDays += (long) iLeapdays;
      iYears--;
      iLeapdays = (iYears - 3) / 4 - 1;
      lDays += 365 - (long) iLeapdays;
   }
   pstNew->wYear = (unsigned short) (iYears + 1858);
   if ( (pstNew->wYear % 4) == 0 &&
       ((pstNew->wYear % 100) != 0 || (pstNew->wYear % 400) == 0) ) iLeap = 1;
   pstNew->wHour = (unsigned short) (dOldSec / 3600.);
   dOldSec -= 3600.0 * (double) (pstNew->wHour);
   pstNew->wMinute = (unsigned short) (dOldSec / 60.);
   dOldSec -= 60.0 * (double) pstNew->wMinute ;
   pstNew->wSecond = (unsigned short) dOldSec;
   if ( pstNew->wSecond >= 30  )
   {
      pstNew->wMinute += 1;
      if ( pstNew->wMinute > 59 ) 
      {
         pstNew->wMinute = 0;
         pstNew->wHour += 1;
       	 if ( pstNew->wHour > 23 )
         {
            pstNew->wHour = 0;
            lDays += 1;
            if ( lDays > 365+iLeap )
            {
               lDays = 1;
               pstNew->wYear += 1;
            }
         }
      }
   }
   pstNew->wSecond = 0;
   pstNew->wMilliseconds = 0;
   DayToDate( lDays, pstNew );
}

      /******************************************************************
       *                         TWCgmtime()                            *
       *                                                                *
       *  This function converts epochal seconds (1/1/70) to a struct   *
       *  tm structure.                                                 *
       *                                                                *
       *  NOTE: Milliseconds are not contained in tm so must be computed*
       *        in the calling program                                  *
       *                                                                *
       *  Arguments:                                                    *
       *   ptmFrom          Input time structure (tm)                   *
       *   pstTo            Output SYSTEMTIME structure                 *
       *                                                                *
       *  Returns:                                                      *
       *   tInTime          time_t value - # seconds since 1/1/70       *
       *                                                                *
       ******************************************************************/
struct tm *TWCgmtime( time_t tInTime )
{
   static  int	iLeap;		/* 1 if year is leap year; else 0 */
   static  int	iLeapdays;	/* Number of leap days since start of MJD */
/* This array is the start day (0=Sunday, 1=Mon, ...) of each
   year starting in 1901.  The start day repeats every 28 years.
   This will work until 2100 since that year is not a leap year.
   The array's purpose is to give a way to fill in the
   wDayOfYear field in the SYSTEMTIME structure */
   static  int	iStartDay[28] = {2,3,4,5,0,1,2,3,5,6,0,1,3,4,5,6,1,2,3,4,6,0,1,
                                 2,4,5,6,0};
   static  int  iTemp;
   static  int  iYears;		/* Number of years since start of MJD */
   static  int  iYearStartDay;	/* Starting DayOfWeek of this year (0=Sun...) */
   static  long lDays;		/* Number of days since start of MJD */
   static  long	lTemp;
   static  struct tm tmLocal;  /* tm struct to fill */

   lDays = (long) (tInTime / 86400);
   tInTime -= (lDays*86400);
   iTemp = lDays;
   iYears = 0;
   while (iTemp > 0)
   {                      
      if (((iYears+2) % 4) == 0) iTemp -= 366;
      else                       iTemp -= 365;
      iYears++;                          
   }
   iYears -= 1;
   iLeapdays = (iYears+1) / 4;
   lDays -= ((long) iYears*365 + (long) iLeapdays);
   lDays += 1;                          /* Since 1/1/70 = Day 1 and not Day 0 */
   if ( lDays <= 0 )
   {
      lDays += (long) iLeapdays;
      iYears--;
      iLeapdays = (iYears + 1) / 4 ;
      lDays += 365 - (long) iLeapdays;
   }
   tmLocal.tm_year = iYears + 70;		/* Relative to 1900 */

/* Figure out which day of the week this is */
   lTemp = lDays;
   iTemp = tmLocal.tm_year;
   iLeap = 0;
   if ( ((iTemp+1900) % 4) == 0 &&
       (((iTemp+1900) % 100) != 0 || ((iTemp+1900) % 400) == 0) ) iLeap = 1;
   while ( lTemp > 365 + iLeap )
   {
      lTemp -= (365 + iLeap);
      iTemp++;
      if ( ((iTemp+1900) % 4) == 0 &&
          (((iTemp+1900) % 100) != 0 || ((iTemp+1900) % 400) == 0) ) iLeap = 1;
      else iLeap = 0;
   }
   tmLocal.tm_yday = lDays-1;

   iYearStartDay = iStartDay[((iTemp-1) % 28)]; /* -1 to adjust to 1901 */

   tmLocal.tm_wday = (((lTemp-1) + iYearStartDay) % 7);
/* Fill in time */
   tmLocal.tm_hour = (int) (tInTime / 3600);
   tInTime -= (3600 * tmLocal.tm_hour);  
   tmLocal.tm_min = (int) (tInTime / 60);
   tInTime -= (60 * tmLocal.tm_min);
   tmLocal.tm_sec = (int) tInTime;

   MSDayToDate( lDays, &tmLocal );              
   tmLocal.tm_isdst = -1;
   return( &tmLocal );
}


