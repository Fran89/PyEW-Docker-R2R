/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: errors.c 2917 2007-04-02 19:12:01Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2007/04/02 19:12:01  paulf
 *     changed the time_t printout to a long
 *
 *     Revision 1.3  2007/04/02 19:09:55  paulf
 *     fixed another time_t casting error
 *
 *     Revision 1.2  2002/03/06 01:32:11  kohler
 *     Removed unused variable msgLog.
 *     Included <earthworm.h> for function prototypes.
 *
 *     Revision 1.1  2000/02/14 16:00:43  lucky
 *     Initial revision
 *
 *
 */

                  /*******************************************
                   *                errors.c                 *
                   *  Error handling functions for adsend    *
                   *******************************************/

#include <stdio.h>
#include <time.h>
#include <earthworm.h>
#include <transport.h>
#include <time_ew.h>

/* Function prototypes
   *******************/
void SetCurPos( int, int );

/* Global variables
   ****************/
extern unsigned char ModuleId;         // Data source id
extern SHM_INFO      OutRegion;        // In adsend.c


              /****************************************************
               *                   ReportError()                  *
               *         Send an error message to statmgr         *
               ****************************************************/

void ReportError( int errNum,          // Error number recognized by statmgr
                  char *errmsg )       // Text of error message
{
   int               lineLen;
   time_t            time_now;         // The current time
   static MSG_LOGO   logo;             // Logo of error messages
   static int        first = TRUE;     // TRUE the first time this function is called
   char              outmsg[100];      // To hold the complete message
   static time_t     time_prev;        // When Heartbeat() was last called

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
   sprintf( outmsg, "%ld %d ", (long) time_now, errNum );
   strcat( outmsg, errmsg );
   lineLen = strlen( outmsg );
   tport_putmsg( &OutRegion, &logo, lineLen, outmsg );
   return;
}


void LogAndReportError( int errNum, char *errMsg )
{
   const int  maxchr = 70;
   int        i,
              len;              // Length of current message
   struct tm  res;		// time string stuff stolen from logit()
   time_t     now;

/* To the status manager and log file
   **********************************/
   ReportError( errNum, errMsg );
   logit( "t", "%s", errMsg );

/* Limit the message to fit the console window
   *******************************************/
   len = strlen( errMsg );
   if ( len > maxchr )
   {
      errMsg[maxchr] = '\0';
      len = maxchr;
   }

/* Log the current message to console.
   Pad the screen message with blanks.
   ***********************************/
   time( &now );
   gmtime_ew( &now, &res );

   SetCurPos( 2, 23 );
   printf( "%4d%02d%02d_UTC_%02d:%02d:%02d  %s",(res.tm_year + 1900),
          (res.tm_mon + 1), res.tm_mday, res.tm_hour, res.tm_min, res.tm_sec, errMsg);

   for ( i = len; i < maxchr; i++ )
      putchar( ' ' );

   return;
}


              /****************************************************
               *                 DAQ_ErrorHandler()               *
               *                                                  *
               *           Function to handle DAQ errors          *
               ****************************************************/

void DAQ_ErrorHandler( char *fun_name, short status )
{
   char     errMsg[100];
   const    errNum = 1;                 // DAQ errors are number 1

   if ( status == 0 )                   // No error
      return;


/* Encode the error message
   ************************/
   if ( status < 0 )
      sprintf( errMsg, "%s did not execute because of an error: %hd\n",
             fun_name, status );

   if ( status > 0 )
      sprintf( errMsg, "%s executed but with a potentially serious side effect: %hd\n",
             fun_name, status );

/* Log and report the error message
   ********************************/
   LogAndReportError( errNum, errMsg );
   return;
}

