
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <transport.h>


         /*************************************************
          *                 ReportStatus()                *
          *  Builds a heartbeat or error message and      *
          *  puts it in shared memory.                    *
          *************************************************/

int ReportStatus( unsigned char type,
                  short         ierr,
                  char          *note )
{
   extern unsigned char TypeHeartBeat;
   extern unsigned char TypeError;
   extern unsigned char InstId;
   extern unsigned char MyModId;
   extern pid_t         myPid;
   extern SHM_INFO      Region;

   MSG_LOGO logo;
   char     msg[256];
   int      res;
   long     size;
   time_t   t;

   logo.instid = InstId;
   logo.mod    = MyModId;
   logo.type   = type;

   time( &t );

   if( type == TypeHeartBeat )
   {
      sprintf( msg, "%ld %d\n", (long)t, myPid );
   }
   else if( type == TypeError )
   {
      sprintf( msg, "%ld %d %s\n", (long)t, ierr, note );
   }

   size = strlen( msg );
   res  = tport_putmsg( &Region, &logo, size, msg );
   return res;
}
