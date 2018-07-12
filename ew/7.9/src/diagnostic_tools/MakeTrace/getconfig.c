/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: getconfig.c 1147 2002-11-25 22:55:49Z alex $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2002/11/25 22:55:49  alex
 *     Initial revision
 *
 *     Revision 1.1  2000/02/14 16:00:43  lucky
 *     Initial revision
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <kom.h>
#include <earthworm.h>
#include "makeTrace.h"

/* The configuration file parameters
   *********************************/
unsigned char ModuleId;      // Module id of this program
int      OnboardChans;       // The number of channels on the DAQ board
int      NumMuxBoards;       // Number of mux cards
double   ChanRate;           // Rate in samples per second per channel
int      ChanMsgSize;        // Message size in samples per channel
double   SampRate=250000.0;  // Sample rate per scan (in samp/s)
int      NumTimeCodeChan;    // How many time-code channels there are
int      *TimeCodeChan;      // The chans where we find the IRIGE signals
int      Gain;               // Gain of amp in front of A/D converter.
long     OutKey;             // Key to ring where traces will live
double   MinGuideSignal;     // Guides are declared dead if the mean value of
                             //   guide 1st differences < MinGuideSignal
double   MaxGuideNoise;      // Guides are declared dead if standard error of
                             //   guide 1st differences > MaxGuideNoise
int      Year;               // The current year
int      NumGuide;           // Number of guide channels
int      *GuideChan;         // The chans where we find the reference voltage
int      TimeForLock;        // Guides are assumed locked on after this many secs
double   LowValue;           // The low hysteresis voltage for external triggering
double   HighValue;          // The high hysteresis voltage for external triggering
int      SendBadTime;        // Send data even if no IRIGE signal is present
int      UpdateSysClock;     // Use a good IRIGE time to update the PC clock
int      YearFromPC;         // Take the date year from the PC clock instead of .d file
SCN      *ChanList;          // Array to fill with SCN and pin values
int      Nchan = 0;          // Number of channels: either specified, or 64* (# of mux's)
int      IrigeIsLocalTime;   // Set to 1 if IRIGE represents local time; 0 if GMT time.
int      TimeoutNoSend;      // Take action if no data sent for this many seconds
int      TimeoutNoSynch;     // Take action if no time synch for this many seconds
int      TimeoutNoTrig;      // Take action if no trigger pulse for this many seconds
int      ErrNoLockTime;      // If no guide lock for ErrNoLockTime sec, report an error
int      Debug;         

     /***************************************************************
      *                          GetConfig()                        *
      *         Processes command file using kom.c functions.       *
      *           Returns -1 if any errors are encountered.         *
      ***************************************************************/

#define NCOMMAND 6             // Number of mandatory commands in the config file

int GetConfig( char *configfile )
{
   const int ncommand = NCOMMAND;

   char     init[NCOMMAND];     /* Flags, one for each command */
   int      nmiss;              /* Number of commands that were missed */
   int      nfiles;
   int      i;
   int		nChanLines = 0;		/* number of 'chan' specifiers. Must == Nchan */

/* Set to zero one init flag for each required command
   ***************************************************/
   for ( i = 0; i < ncommand; i++ )
      init[i] = 0;

/* Open the main configuration file
   ********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 )
   {
      logit("", "makeTrace: Error opening configuration file <%s>\n", configfile );
      return -1;
   }

/* Process all nested configuration files
   **************************************/
   while ( nfiles > 0 )          /* While there are config files open */
   {
      while ( k_rd() )           /* Read next line from active file  */
      {
         int  success;
         char *com;
         char *str;

         com = k_str();          /* Get the first token from line */

         if ( !com ) continue;             /* Ignore blank lines */
         if ( com[0] == '#' ) continue;    /* Ignore comments */

/* Open another configuration file
   *******************************/
         if ( com[0] == '@' )
         {
            success = nfiles + 1;
            nfiles  = k_open( &com[1] );
            if ( nfiles != success )
            {
               logit("", "makeTrace: Error opening command file <%s>.\n", &com[1] );
               return -1;
            }
            continue;
         }

/* Read configuration parameters
   *****************************/
         else if ( k_its( "ModuleId" ) )
         {
            if ( str = k_str() )
            {
               if ( GetModId(str, &ModuleId) == -1 )
               {
                  logit("", "makeTrace: Invalid ModuleId <%s>. Exiting.\n", str );
                  return -1;
               }
            }
            init[0] = 1;
         }
         else if ( k_its( "Nchan" ) )
         {
            Nchan = k_int();
            init[1] = 1;
         }
         else if ( k_its( "ChanMsgSize" ) )
         {
            ChanMsgSize = k_int();
            init[2] = 1;
         }
         else if ( k_its( "ChanRate" ) )
         {
            ChanRate = k_val();
            init[3] = 1;
         }
         else if ( k_its( "OutRing" ) )
         {
            if ( str = k_str() )
            {
               if ( (OutKey = GetKey(str)) == -1 )
               {
                  logit("", "makeTrace: Invalid OutRing <%s>. Exiting.\n", str );
                  return -1;
               }
            }
            init[4] = 1;
         }
         else if ( k_its( "Debug" ) )
         {
            Debug = k_int();
            init[5] = 1;
         }

/* An unknown parameter was encountered
   ************************************/
         else
         {
            logit("", "makeTrace: <%s> unknown parameter in <%s>\n", com, configfile );
            return -1;
         }

/* See if there were any errors processing the command
   ***************************************************/
         if ( k_err() )
         {
            logit("", "makeTrace: Bad <%s> command in <%s>.\n", com, configfile );
            return -1;
         }
      }
      nfiles = k_close();
   }

/* After all files are closed, check flags for missed commands
   ***********************************************************/
   nmiss = 0;
   for ( i = 0; i < ncommand; i++ )
      if ( !init[i] )
         nmiss++;

   if ( nmiss > 0 )
   {
      logit("", "adsend: ERROR, no " );
      if ( !init[0]  ) logit("", "<ModuleId> " );
      if ( !init[1]  ) logit("", "<Nchan> " );
      if ( !init[2]  ) logit("", "<ChanMsgSize> " );
      if ( !init[3]  ) logit("", "<ChanRate> " );
      if ( !init[4]  ) logit("", "<OutRing> " );
      if ( !init[5]  ) logit("", "<Debug> " );
      logit("", "command(s) in <%s>.\n", configfile );
      return -1;
   }
   return 0;
}
