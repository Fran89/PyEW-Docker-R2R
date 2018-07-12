/*************************************
  snw.c
  Report stuff to SeisnetWatch.
 *************************************/


#include <stdio.h>
#include <time.h>
#include <string.h>
#include <earthworm.h>
#include <transport.h>
#include "adsendxs.h"

#define SNWMSGSIZE 120

/* Declared in other files
   ***********************/
extern int      SnwReportInterval;  // In seconds
extern char     StaName[6];         // For SeisnetWatch
extern char     NetCode[3];         // For SeisnetWatch
extern SHM_INFO Region;             // Info structure for output region
extern MSG_LOGO snwLogo;            // Logo of SeisnetWatch message to put out


/************************************************************
  ReportTimeSinceLastLockToSNW

  Report time since GPS was last locked to SNW.  If GPS
  clock is currently locked to satellites, update gpsLockFile.
 ************************************************************/

void ReportTimeSinceLastLockToSNW( time_t tgps,
                                   int gpsTimeStatus,
                                   FILE *gpsLockFile )
{
   time_t now;                     // Current PC time
   static time_t tSnwPrev = 0;     // Last PC time we sent to SNW

   time( &now );
   if ( (now - tSnwPrev) >= SnwReportInterval )  // It's time to report to SNW
   {
      time_t tgpsLastLock;              // Time of last GPS lock from file

      rewind( gpsLockFile );
      if ( fscanf_s( gpsLockFile, "%d", &tgpsLastLock ) == 1 )
      {
         char   snwMsg[120];
         int    snwMsgSize;
         double daysSinceLastLock = (tgps - tgpsLastLock)/86400.0;

         sprintf_s( snwMsg, SNWMSGSIZE, "%s-%s:1:Days since last T-Bolt satellite lock=%.3lf\n",
                  NetCode, StaName, daysSinceLastLock );
         snwMsgSize = strlen( snwMsg );
         if ( tport_putmsg(&Region, &snwLogo, snwMsgSize, snwMsg) != PUT_OK )
            logit( "et", "Error sending SNW message for days since last GPS lock to ring.\n" );
      }
      tSnwPrev = now;        // Record time of last report to SNW
   }
   if ( gpsTimeStatus == LOCKED_GPS_TIME_AVAILABLE )
   {
      rewind( gpsLockFile );
      if ( fprintf(gpsLockFile, "%d\n", tgps) == 0 )
         logit( "et", "fprintf error updating gpsLockFile\n" );
      fflush( gpsLockFile );
   }
   return;
}


/************************************************************
  ReportAntennaStatusToSNW

  Report the antenna status to SNW.
  The only valid states are 0 (not open) or 1 (open).
 ************************************************************/

void ReportAntennaStatusToSNW( int antennaOpen )
{
   time_t now;                     // Current PC time
   static time_t tSnwPrev = 0;     // Last PC time we sent to SNW

   if ( antennaOpen == UNKNOWN ) return;

   time( &now );
   if ( (now - tSnwPrev) >= SnwReportInterval )  // It's time to report to SNW
   {
         char snwMsg[120];
         int  snwMsgSize;

         sprintf_s( snwMsg, SNWMSGSIZE, "%s-%s:1:T-Bolt GPS antenna open=%d\n",
                  NetCode, StaName, antennaOpen );
         snwMsgSize = strlen( snwMsg );
         if ( tport_putmsg(&Region, &snwLogo, snwMsgSize, snwMsg) != PUT_OK )
            logit( "et", "Error sending SNW message for antenna-open to ring.\n" );
      tSnwPrev = now;        // Record time of last report to SNW
   }

   return;
}
