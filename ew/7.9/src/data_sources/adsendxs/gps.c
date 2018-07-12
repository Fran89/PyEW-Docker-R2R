/************************************************
  gps.c
 ************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <sys/timeb.h>

#include <earthworm.h>
#include <time_ew.h>
#include <swap.h>
#include "adsendxs.h"

#define TIMESTRSIZE 26

/* Functions prototypes
   ********************/
void ReportErrorToStatmgr( int errNum, char *errmsg );
int  PurgeComInput( void );
int  GetPrimaryTimingPacket( UINT8 ptp[] );
int  GetSupplementalTimingPacket( UINT8 stp[] );
void GetTimeFromPTP( UINT8 ptp[], time_t *tgps );

/* Declared in getconfig.h
   ***********************/
extern int Debug;             // If > 0, log GPS flags


/******************************************************************************************
  GetGpsTime

  Returns antennaOpen. Possible values are UNKNOWN, FALSE, or TRUE.

  Returns CRITICAL_ERROR if no GPS time is available, and we want to exit the program.
  Returns GPS_TIME_UNAVAILABLE if no GPS time is available.
  Returns UNLOCKED_GPS_TIME_AVAILABLE if time is available, but no satellite lock.
  Returns LOCKED_GPS_TIME_AVAILABLE if time is available, and clock is locked to satellites.
 ******************************************************************************************/

int GetGpsTime( time_t *tgps, int *antennaOpen )
{
   UINT8  ptp[MAXGPSPACKETSIZE];      // Primary timing packet
   UINT8  stp[MAXGPSPACKETSIZE];      // Supplemental timing packet
   UINT8  timingFlags;                // From PTP
   UINT8  discipliningMode;           // From STP
   UINT16 minorAlarms;                // From STP
   UINT8  gpsDecodingStatus;          // From STP
   int    ptpError, stpError;
   int    i;
   char   errMsg[ERRMSGSIZE];

/* Trimble constants
   *****************/
   const UINT8 no_utc_info              = 0x08;   // PTP Timing Flag
   const UINT8 locked_to_gps            = 0;      // STP Disciplining Mode
   const UINT8 power_up                 = 1;      // STP Disciplining Mode
   const UINT8 auto_holdover            = 2;      // STP Disciplining Mode
   const UINT8 recovering_discipline    = 4;      // STP Disciplining Mode
   const UINT8 doing_fixes              = 0x00;   // STP GPS Decoding Status
   const UINT8 no_usable_satellites     = 0x08;   // STP GPS Decoding Status
   const UINT8 only_1_usable_satellite  = 0x09;   // STP GPS Decoding Status
   const UINT8 only_2_usable_satellites = 0x0A;   // STP GPS Decoding Status
   const UINT8 only_3_usable_satellites = 0x0B;   // STP GPS Decoding Status

   *antennaOpen = UNKNOWN;

/* Read Primary Timing Packet (8F-AB) from GPS.
   A Critical Error will cause the program to exit.
   ***********************************************/
   ptpError = GetPrimaryTimingPacket( ptp );
   if ( Debug == 2 )      // Log complete PTP data
   {
      logit( "e", "\nptp:" );
      for ( i = 0; i < 17; i++ )
         logit( "e", " %02X", ptp[i] );
      logit( "e", "\n" );
   }
   if ( ptpError )
   {
      logit( "et", "Error reading Primary Timing Packet from GPS.\n" );
      strcpy_s( errMsg, ERRMSGSIZE, "Error reading Primary Timing Packet from GPS.\n" );
      ReportErrorToStatmgr( GPS_ERROR, errMsg );
      return CRITICAL_ERROR;
   }

   memcpy( &timingFlags, ptp+9, 1 );
   if ( Debug == 1 )           // Log timing flags
      logit( "e", "\n" );
   if ( Debug > 0 )
      logit( "e", "timingFlags: %02X\n", timingFlags );

/* Read Supplemental Timing Packet (8F-AC) from GPS.
   A Critical Error will cause the program to exit.
   ************************************************/
   stpError = GetSupplementalTimingPacket( stp );
   if ( Debug == 2 )      // Log complete STP data
   {
      logit( "e", "stp:" );
      for ( i = 0; i < 60; i++ )
         logit( "e", " %02X", stp[i] );
      logit( "e", "\n" );
   }
   if ( stpError )
   {
      logit( "et", "Error reading Supplemental Timing Packet from GPS.\n" );
      strcpy_s( errMsg, ERRMSGSIZE, "Error reading Supplemental Timing Packet from GPS.\n" );
      ReportErrorToStatmgr( GPS_ERROR, errMsg );
      return CRITICAL_ERROR;
   }

   memcpy( &discipliningMode, stp+2, 1 );
   memcpy( &minorAlarms, stp+10, 2 );
   SwapShort( &minorAlarms );
   memcpy( &gpsDecodingStatus, stp+12, 1 );
   if ( Debug > 0 )
   {
      logit( "e", "discipliningMode:%02X", discipliningMode );
      logit( "e", "  minorAlarms:%04X", minorAlarms );
      logit( "e", "  gpsDecodingStatus:%02X\n", gpsDecodingStatus );
   }
   *antennaOpen = (minorAlarms & 0x0002) ? TRUE : FALSE;

/* If the "DAC is at rail", exit program
   *************************************/
   {
      UINT16 criticalAlarms;
      memcpy( &criticalAlarms, stp+8, 2 );
      SwapShort( &criticalAlarms );
      if ( criticalAlarms & 0x0010 )
      {
         logit( "et", "Error.  GPS DAC at rail.\n" );
         strcpy_s( errMsg, ERRMSGSIZE, "Error.  GPS DAC at rail.\n" );
         ReportErrorToStatmgr( GPS_ERROR, errMsg );
         return CRITICAL_ERROR;
      }
   }

/* Log any changes in disciplining mode, which is an indication
   of GPS receiver health, satellite health, and antenna cable
   health.  In debug mode, log disciplining mode every second.
   ************************************************************/
   {
      static UINT8 discipliningModePrev = 5;   // Mode 5 not used by Trimble
      static first = 1;                        // First time we ran this code

      if ( (Debug > 0) || (discipliningMode != discipliningModePrev) )
      {
         if ( discipliningMode == locked_to_gps )
            logit( "et", "GPS clock locked to satellites.\n" );
         else if ( discipliningMode == power_up )
            logit( "et", "GPS powering up.\n" );
         else if ( discipliningMode == auto_holdover )
            logit( "et", "GPS clock is undisciplined.\n" );
//       else if ( discipliningMode == recovering_discipline )
//          logit( "et", "GPS clock recovering discipline from satellites.\n" );
      }

/* If GPS clock loses or regains satellite synch, send an email alarm
   ******************************************************************/
      if ( (discipliningModePrev == locked_to_gps) &&
           (discipliningMode != locked_to_gps ) )
      {
         strcpy_s( errMsg, ERRMSGSIZE, "Lost synch to GPS satellites." );
         ReportErrorToStatmgr( GPS_ERROR, errMsg );
      }

      if ( !first &&
           (discipliningModePrev != locked_to_gps) &&
           (discipliningMode == locked_to_gps ) )
      {
         strcpy_s( errMsg, ERRMSGSIZE, "Regained synch to GPS satellites." );
         ReportErrorToStatmgr( GPS_ERROR, errMsg );
      }
      discipliningModePrev = discipliningMode;
      first = 0;
   }

/* Log if not enough usable satellites
   ***********************************/
   {
      static UINT8 gpsDecodingStatusPrev = 0xFF;    // 0xFF not used by Trimble

      if ( gpsDecodingStatus != gpsDecodingStatusPrev )
      {
         if ( gpsDecodingStatus == no_usable_satellites )
            logit( "et", "No usable satellites.\n" );
         else if ( gpsDecodingStatus == only_1_usable_satellite )
            logit( "et", "Only 1 usable satellite.\n" );
         else if ( gpsDecodingStatus == only_2_usable_satellites )
            logit( "et", "Only 2 usable satellites.\n" );
         else if ( gpsDecodingStatus == only_3_usable_satellites )
            logit( "et", "Only 3 usable satellites.\n" );
      }
      gpsDecodingStatusPrev = gpsDecodingStatus;
   }

/* See if accurate time is available, whether or not
   the GPS clock is locked to satellites.
   *************************************************/
   if ( discipliningMode & power_up )
   {
      logit( "et", "GPS powering up.\n" );
      return GPS_TIME_UNAVAILABLE;
   }
   if ( timingFlags & no_utc_info )
   {
      logit( "et", "No UTC info.\n" );
      return GPS_TIME_UNAVAILABLE;
   }
   if ( (discipliningMode != locked_to_gps) &&
        (discipliningMode != auto_holdover) &&
        (discipliningMode != recovering_discipline) )
      return GPS_TIME_UNAVAILABLE;

/* The Trimble clock time should now be ok.
   Get time from the PTP and return it to calling program.
   In debug mode, convert time to readable form using ctime.
   Warning: ctime_s may fail in 64-bit environments.
   ********************************************************/
   GetTimeFromPTP( ptp, tgps );
   if ( Debug > 0 )                               // Log GPS clock time
   {
      char timestr[TIMESTRSIZE];
      if ( ctime_s( timestr, TIMESTRSIZE, tgps ) != 0 )
      {
         logit( "et", "Error converting GPS time to string using ctime_s\n" );
         return GPS_TIME_UNAVAILABLE;
      }
      timestr[24] = '\0';                         // Chop off trailing CR
      logit( "e", "GPS clock: %s\n", timestr+4 ); // Chop off day-of-week
   }

/* Is the GPS clock locked to satellites?
   **************************************/
   if ( discipliningMode == locked_to_gps )
      return LOCKED_GPS_TIME_AVAILABLE;
   else
      return UNLOCKED_GPS_TIME_AVAILABLE;
}


/*****************************************************
  GetTimeFromPTP

  Get UTC time from Primary Timing Packet.
  Convert to seconds since midnight Jan 1, 1970.
 *****************************************************/

void GetTimeFromPTP( UINT8 ptp[], time_t *tgps )
{
   struct tm stm;
   short  year;

   stm.tm_sec  = ptp[10];
   stm.tm_min  = ptp[11];
   stm.tm_hour = ptp[12];
   stm.tm_mday = ptp[13];
   stm.tm_mon  = ptp[14] - 1;
   memcpy( &year, ptp+15, 2 );
   SwapShort( &year );
   stm.tm_year = year - 1900;
   stm.tm_isdst = 0;             // Disable daylight savings mode
   *tgps = timegm_ew( &stm );    // Whole numbers of seconds
   return;
}
