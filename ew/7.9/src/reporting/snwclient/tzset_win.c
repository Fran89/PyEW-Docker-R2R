
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

void log( char *, char *, ... );


   /*************************************************************************
    *                             tzset_win.c                               *
    *                                                                       *
    *       Set the time zone environmental variable (TZ).                  *
    *       This is required for the gmtime() function to work              *
    *       correctly.                                                      *
    *************************************************************************/

void TzSet( char *tz )
{
   static char envstring[40];

   strcpy( envstring, "TZ=" );
   strcat( envstring, tz );

   if ( _putenv( envstring ) == -1 )
   {
      log( "et", "Error: Can't set environmental variable TZ. Exiting.\n" );
      exit( -1 );
   }
   _tzset();
   return;
}
