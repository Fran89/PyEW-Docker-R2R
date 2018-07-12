
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: time_dtos.c 7 2000-02-14 17:27:23Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 17:15:33  lucky
 *     Initial revision
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time_ew.h>


   /*********************************************************************
    *                            Time_DtoS()                            *
    *         Convert time from a double to a character string          *
    *                                                                   *
    *  dtime = seconds since midnight, Jan 1, 1970.                     *
    *  time_str must be declared at least 25 characters long.           *
    *********************************************************************/

void Time_DtoS( double dtime, char *time_str )
{
   struct tm  g;
   time_t     tsec; 
   int        thun;
   
/* Convert time in seconds since midnight Jan 1, 1970 to date and time    **********************************************************************/
   tsec = (int)floor( dtime );
   thun = (int)((100. * (dtime - (double)tsec)) + 0.5);
   if ( thun == 100 )  tsec++, thun = 0;
   gmtime_ew( &tsec, &g );

/* Encode the output string
   ************************/
   sprintf( time_str,    "%04d",  g.tm_year+1900 );
   sprintf( time_str+4,  "%02d",  g.tm_mon+1 );
   sprintf( time_str+6,  "%02d",  g.tm_mday );
   sprintf( time_str+8,  " %02d", g.tm_hour );
   sprintf( time_str+11, ":%02d", g.tm_min );
   sprintf( time_str+14, ":%02d", g.tm_sec );
   sprintf( time_str+17, ".%02d", thun );
   sprintf( time_str+20, " UTC" );
   time_str[24] = '\0';
   return;
}
