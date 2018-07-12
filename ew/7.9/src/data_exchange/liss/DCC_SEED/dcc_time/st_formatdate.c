/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: st_formatdate.c 44 2000-03-13 23:49:34Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/03/13 23:48:35  lombard
 *     Initial revision
 *
 *
 *
 */

                    /* format_date.c */
/* include files */
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <dcc_std.h>
#include <dcc_time.h>

struct tm *ltimer;

char *format_date( char *date )
/* The return the date format YYYY,DDD,HH24:MI:SS

   Supply the century portion of the year if it is missing and put
   zeros for any portion of the time that may be missing.

   ltimer is a global set to the current system time when the
   program starts.
*/
{
    char temp_buf[50], *ptr;
    int cnt;
    
    /* Make sure the input date is not empty */
    if ( (date == NULL) || (date[0] == '\0') )
      return( date );
    /* Process the year */
    for ( cnt=0, ptr=date; *ptr != ','; cnt++, ptr++ );
    ptr = temp_buf;
    if ( cnt != 4 )
    {
       /* get year including centry */
      strftime( temp_buf, 40, "%Y", ltimer );
       /* truncate to centry part only */
      temp_buf[2] = '\0';
      ptr = &temp_buf[2];
    }
    strcpy( ptr, date );
    for ( ptr++; *ptr != ',' ; ptr++ ); /* walk over year */
    /*walk over day */
    for ( ptr++; (*ptr != ',') && (*ptr != '\0'); ptr++ );
    if ( *ptr == '\0' ) /* Add the time */
    {
      strcpy( ptr, ",00:00:00" );
    } else
    {
      /* walk over hour */
      for ( ptr++; (*ptr != ':') && (*ptr != '\0'); ptr++ );
      if ( *ptr == '\0' ) /* Add the minutes and seconds */
      {
        strcpy( ptr, ":00:00" );
      } else
      {
        /* walk over minute */
        for ( ptr++; (*ptr != ':') && (*ptr != '\0'); ptr++ );
        if ( *ptr == '\0' ) /* Add the seconds only */
        {
          strcpy( ptr, ":00" );
        }
      }
    }
    ptr = malloc( strlen( temp_buf ) );
    strcpy( ptr, temp_buf );
    return( ptr );
}

STDTIME str_to_struct( char *date )
/*
   Put a string with the format 'yyyy,ddd,hhh:mm:ss' into a STDTIME
   structure and return it to the calling routine.

   We use the routine format_date to make sure all parts of the string
   are present.

   If the input string is NULL we return a structure full of zeros.
*/
{
   int i;
   char *cptr, temp[50]; 
   STDTIME *dcc_tm;

   dcc_tm = malloc( sizeof(STDTIME) );
   *dcc_tm = ST_Zero();

   if ( (date == NULL) || (date[0] == '\0') )
     return( *dcc_tm );

   cptr = format_date(date);

   for (i = 0; *cptr != ','; cptr++, i++)
   {
      temp[i] = *cptr;
   }
   temp[i] = '\0';
   dcc_tm->year = atoi(temp);

   for ( i = 0, cptr++; *cptr != ','; cptr++, i++ )
      temp[i] = *cptr;
   temp[i] = '\0';
   dcc_tm->day = atoi(temp);

   for ( i = 0, cptr++; *cptr != ':'; cptr++, i++ )
      temp[i] = *cptr;
   temp[i] = '\0';
   dcc_tm->hour = atoi(temp);

   for ( i = 0, cptr++; *cptr != ':'; cptr++, i++ )
      temp[i] = *cptr;
   temp[i] = '\0';
   dcc_tm->minute = atoi(temp);

   for ( i = 0, cptr++; *cptr != '\0'; cptr++, i++ )
      temp[i] = *cptr;
   temp[i] = '\0';
   dcc_tm->second = atoi(temp);

   return( *dcc_tm );
}
