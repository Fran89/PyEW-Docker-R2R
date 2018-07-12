/***************************************
  errors.c

  Error handling functions for adsendxs
 ***************************************/

#include <stdio.h>
#include <time.h>
#include <earthworm.h>
#include <transport.h>
#include <time_ew.h>

#define OUTMSGSIZE 100

/* Global variables
   ****************/
extern unsigned char ModuleId;      // Data source id
extern SHM_INFO      Region;        // In adsendxs.c


/**************************************
  ReportErrorToStatmgr
 **************************************/

void ReportErrorToStatmgr( int errNum,     // Error number recognized by statmgr
                           char *errmsg )  // Text of error message
{
   int               lineLen;
   time_t            time_now;           // The current time
   static MSG_LOGO   logo;               // Logo of error messages
   static int        first = TRUE;       // TRUE the first time this function is called
   char              outmsg[OUTMSGSIZE]; // To hold the complete message

/* Initialize the error message logo
   *********************************/
   if ( first )
   {
      GetLocalInst( &logo.instid );
      logo.mod = ModuleId;
      GetType( "TYPE_ERROR", &logo.type );
      first = FALSE;
   }

/* Encode the output message and send it
   *************************************/
   time( &time_now );
   sprintf_s( outmsg, OUTMSGSIZE, "%ld %d ", (long) time_now, errNum );
   strcat_s( outmsg, OUTMSGSIZE, errmsg );
   lineLen = strlen( outmsg );
   tport_putmsg( &Region, &logo, lineLen, outmsg );
   return;
}

