
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: time_ew.c 2058 2006-01-19 22:36:13Z friberg $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/01/19 22:36:11  friberg
 *     installed for mitch withers
 *
 *     Revision 1.1  2003/08/25 22:59:39  davidk
 *     Initial revision
 *
 *     Revision 1.5  2001/01/23 16:49:43  dietz
 *     Corrected roundoff problem in datestr23 and datestr23_local
 *
 *     Revision 1.4  2000/11/30 22:12:07  lombard
 *     fixed bug in timegm_ew: _timezone variable was not being set
 *     by call to _tzset() before call to mktime(). And the changes
 *     to the TZ environment variable were unnecessary.
 *
 *     Revision 1.3  2000/09/14 19:26:23  lucky
 *     Added datestr23_local which returns time string in local time
 *
 *     Revision 1.2  2000/03/10 23:35:58  davidk
 *     added includes from stdlib.h and stdio.h to resolve some compiler warnings.
 *
 *     Revision 1.1  2000/02/14 18:53:30  lucky
 *     Initial revision
 *
 *
 */

     /********************************************************
      *              time_ew.c   Windows NT version          *
      *                                                      *
      *  This file contains earthworm multi-thread safe      *
      *  versions of time routines.                          *
      ********************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys\timeb.h>
#include <sys\types.h>
#include <time_ew.h>

/********************************************************
 *  gmtime_ew() converts time in seconds since 1970 to  *
 *  a time/date structure expressed as UTC (GMT)        *
 ********************************************************/
struct tm *gmtime_ew( const time_t *epochsec, struct tm *res )
{
    *res = *gmtime( epochsec );
    return( res );
}

/********************************************************
 *  localtime_ew() converts time in seconds since 1970  *
 *  to a time/date structure expressed as local time    *
 *  (using time zone and daylight savings corrections)  *
 ********************************************************/
struct tm *localtime_ew( const time_t *epochsec, struct tm *res )
{
    *res = *localtime( epochsec );
    return( res );
}

/********************************************************
 *  ctime_ew() converts time in seconds since 1970 to   *
 *  a 26 character string expressed as local time       *
 *  (using time zone and daylight savings corrections)  *
 *   Example:  "Fri Sep 13 00:00:00 1986\n\0"           *
 ********************************************************/
char *ctime_ew( const time_t *epochsec, char *buf, int buflen )
{
    strcpy( buf, ctime( epochsec ) );
    return( buf );
}

/********************************************************
 *  asctime_ew() converts time/date structure to        *
 *  a 26 character string                               *
 *   Example:  "Fri Sep 13 00:00:00 1986\n\0"           *
 ********************************************************/
char *asctime_ew( const struct tm *tm, char *buf, int buflen )
{
    strcpy( buf, asctime( tm ) );
    return( buf );
}

/*******************************************************
 * hrtime_ew() returns a high-resolution system clock  *
 *             time as a double in seconds since       *
 *             midnight Jan 1, 1970                    *
 *******************************************************/
double hrtime_ew( double *tnow )
{
    struct _timeb t;

    _ftime( &t );
    *tnow = (double)t.time + (double)t.millitm*0.001;
    return( *tnow );
}

/********************************************************
 *                      timegm_ew()                     *
 * Convert struct tm to time_t using GMT as time zone   *
 ********************************************************/
time_t timegm_ew( struct tm *stm )
{
   time_t tt;

/* Change time zone to GMT; do conversion
 ****************************************/
   _timezone = 0L;
   tt = mktime( stm ); 

/* Restore original _timezone setting
 *****************************/
   _tzset();

   return( tt );
}


/**********************************************************
 * Converts time (double, seconds since 1970:01:01) to    *
 * a 22-character, null-terminated string in the form of  *
 *            yyyy/mm/dd hh:mm:ss.ss                      *
 * Time string returned is in UTC time                    *
 * Target buffer must be 23-chars long to have room for   *
 * null-character                                         *
 **********************************************************/ 
char *datestr23( double t, char *pbuf, int len )
{  
   time_t    tt;       /* time as time_t                  */
   struct tm stm;      /* time as struct tm               */
   int       t_hsec;   /* hundredths-seconds part of time */

/* Make sure target is big enough
 ********************************/
   if( len < DATESTR23 ) return( (char *)NULL );

/* Convert double time to other formats 
 **************************************/
   t += 0.005;  /* prepare to round to the nearest 100th */
   tt     = (time_t) t;
   t_hsec = (int)( (t - tt) * 100. );
   gmtime_ew( &tt, &stm );

/* Build character string
 ************************/
   sprintf( pbuf, 
           "%04d/%02d/%02d %02d:%02d:%02d.%02d",
            stm.tm_year+1900,
            stm.tm_mon+1,
            stm.tm_mday,
            stm.tm_hour,
            stm.tm_min,
            stm.tm_sec,            
            t_hsec );
 
   return( pbuf );
}


/**********************************************************
 * Converts time (double, seconds since 1970:01:01) to    *
 * a 22-character, null-terminated string in the form of  *
 *            yyyy/mm/dd hh:mm:ss.ss                      *
 * Time string returned is in LOCAL time                  *
 * Target buffer must be 23-chars long to have room for   *
 * null-character                                         *
 **********************************************************/ 
char *datestr23_local( double t, char *pbuf, int len )
{  
   time_t    tt;       /* time as time_t                  */
   struct tm stm;      /* time as struct tm               */
   int       t_hsec;   /* hundredths-seconds part of time */

/* Make sure target is big enough
 ********************************/
   if( len < DATESTR23 ) return( (char *)NULL );

/* Convert double time to other formats 
 **************************************/
   t += 0.005;  /* prepare to round to the nearest 100th */
   tt     = (time_t) t;
   t_hsec = (int)( (t - tt) * 100. );
   localtime_ew( &tt, &stm );

/* Build character string
 ************************/
   sprintf( pbuf, 
           "%04d/%02d/%02d %02d:%02d:%02d.%02d",
            stm.tm_year+1900,
            stm.tm_mon+1,
            stm.tm_mday,
            stm.tm_hour,
            stm.tm_min,
            stm.tm_sec,            
            t_hsec );
 
   return( pbuf );
}
