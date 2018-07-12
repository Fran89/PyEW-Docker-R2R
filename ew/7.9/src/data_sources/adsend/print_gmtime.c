/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: print_gmtime.c 888 2002-03-06 01:31:24Z kohler $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2002/03/06 01:31:24  kohler
 *     Removed unused variable line[].
 *     Added type cast (time_t)time.
 *
 *     Revision 1.1  2000/02/14 16:00:43  lucky
 *     Initial revision
 *
 *
 */

#include <stdio.h>
#include <time.h>
#include <math.h>

void PrintGmtime( double time, int dec )
{
   int  hsec;
   struct tm *gmt;
   time_t ltime = (time_t)time;

   gmt = gmtime( &ltime );
   if ( gmt->tm_wday == 0 ) printf( "%s", "Sun" );
   if ( gmt->tm_wday == 1 ) printf( "%s", "Mon" );
   if ( gmt->tm_wday == 2 ) printf( "%s", "Tue" );
   if ( gmt->tm_wday == 3 ) printf( "%s", "Wed" );
   if ( gmt->tm_wday == 4 ) printf( "%s", "Thu" );
   if ( gmt->tm_wday == 5 ) printf( "%s", "Fri" );
   if ( gmt->tm_wday == 6 ) printf( "%s", "Sat" );
   putchar( ' ' );
   if ( gmt->tm_mon == 0  ) printf( "%s", "Jan" );
   if ( gmt->tm_mon == 1  ) printf( "%s", "Feb" );
   if ( gmt->tm_mon == 2  ) printf( "%s", "Mar" );
   if ( gmt->tm_mon == 3  ) printf( "%s", "Apr" );
   if ( gmt->tm_mon == 4  ) printf( "%s", "May" );
   if ( gmt->tm_mon == 5  ) printf( "%s", "Jun" );
   if ( gmt->tm_mon == 6  ) printf( "%s", "Jul" );
   if ( gmt->tm_mon == 7  ) printf( "%s", "Aug" );
   if ( gmt->tm_mon == 8  ) printf( "%s", "Sep" );
   if ( gmt->tm_mon == 9  ) printf( "%s", "Oct" );
   if ( gmt->tm_mon == 10 ) printf( "%s", "Nov" );
   if ( gmt->tm_mon == 11 ) printf( "%s", "Dec" );
   printf( " %2d",  gmt->tm_mday );
   printf( " %02d", gmt->tm_hour );
   printf( ":%02d", gmt->tm_min );
   printf( ":%02d", gmt->tm_sec );
   if ( dec == 2 )                                // Two decimal places
   {
      hsec = (int)(100. * (time - floor(time)));
      printf( ".%02d", hsec );
   }
   printf( " %4d", gmt->tm_year + 1900 );

   return;
}
