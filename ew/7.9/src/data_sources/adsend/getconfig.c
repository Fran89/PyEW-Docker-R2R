/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: getconfig.c 1410 2004-04-20 16:26:32Z kohler $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.5  2004/04/20 16:26:32  kohler
 *     Modified adsend to produce TYPE_TRACEBUF2 messages, which contain location
 *     codes in the headers.  The configuration file, adsend.d, must contain
 *     location codes in the channel specifications.  Also, cleaned up some error
 *     reporting.  WMK 4/20/04
 *
 *     Revision 1.4  2003/02/24 17:13:41  cjbryan
 *     removed Alex's change that required all channels to be defined
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
#include "adsend.h"


/* Function declaration
   ********************/
int IsComment( char [] );    // Defined below

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
int      ExtTrig;            // External trigger flag
long     OutKey;             // Key to ring where traces will live
int      HeartbeatInt;       // Heartbeat interval in seconds
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
SCNL     *ChanList;          // Array to fill with SCNL and pin values
int      Nchan = 0;          // Number of channels: either specified, or 64* (# of mux's)
int      IrigeIsLocalTime;   // Set to 1 if IRIGE represents local time; 0 if GMT time.
int      TimeoutNoSend;      // Take action if no data sent for this many seconds
int      TimeoutNoSynch;     // Take action if no time synch for this many seconds
int      TimeoutNoTrig;      // Take action if no trigger pulse for this many seconds
int      ErrNoLockTime;      // If no guide lock for ErrNoLockTime sec, report an error
int      EnableBell;         // If non-zero, ring bell if no time synch or no guides


     /***************************************************************
      *                          GetConfig()                        *
      *         Processes command file using kom.c functions.       *
      *           Returns -1 if any errors are encountered.         *
      ***************************************************************/

#define NCOMMAND 26             // Number of mandatory commands in the config file

int GetConfig( char *configfile )
{
   const int ncommand = NCOMMAND;

   char     init[NCOMMAND];     /* Flags, one for each command */
   int      nmiss;              /* Number of commands that were missed */
   int      nfiles;
   int      i;
   int      nChanLines = 0;     /* number of 'chan' specifiers. Must == Nchan */

/* Set to zero one init flag for each required command
   ***************************************************/
   for ( i = 0; i < ncommand; i++ )
      init[i] = 0;

/* Open the main configuration file
   ********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 )
   {
      logit("", "adsend: Error opening configuration file <%s>\n", configfile );
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
               logit("", "adsend: Error opening command file <%s>.\n", &com[1] );
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
                  logit("", "adsend: Invalid ModuleId <%s>. Exiting.\n", str );
                  return -1;
               }
            }
            init[0] = 1;
         }

         else if ( k_its( "OnboardChans" ) )
         {
            OnboardChans = k_int();
            init[1] = 1;
         }

         else if ( k_its( "NumMuxBoards" ) )
         {
            NumMuxBoards = k_int();
            init[2] = 1;
         }

         else if ( k_its( "ChanRate" ) )
         {
            ChanRate = k_val();
            init[3] = 1;
         }

         else if ( k_its( "ChanMsgSize" ) )
         {
            ChanMsgSize = k_int();
            init[4] = 1;
         }

         else if ( k_its( "SampRate" ) )
         {
            SampRate = k_val();
         }

         else if ( k_its( "NumTimeCodeChan" ) )
         {
            NumTimeCodeChan = k_int();
            if ( NumTimeCodeChan > 0 )
            {
               TimeCodeChan = (int *) calloc( NumTimeCodeChan, sizeof(int) );
               if ( TimeCodeChan == NULL )
               {
                  logit("", "Error: Cannot allocate TimeCodeChan array.\n" );
                  return -1;
               }
            }
            else
            {
               logit( "", "Error: NumTimeCodeChan must be > 0\n" );
               return -1;
            }
            init[5] = 1;
         }

         else if ( k_its( "TimeCodeChan" ) )
         {
            for ( i = 0; i < NumTimeCodeChan; i++ )
               TimeCodeChan[i] = k_int();
         }

         else if ( k_its( "Gain" ) )
         {
            Gain = k_int();
            init[6] = 1;
         }

         else if ( k_its( "ExtTrig" ) )
         {
            ExtTrig = k_int();
            init[7] = 1;
         }

         else if ( k_its( "MinGuideSignal" ) )
         {
            MinGuideSignal = k_val();
            init[8] = 1;
         }

         else if ( k_its( "OutRing" ) )
         {
            if ( str = k_str() )
            {
               if ( (OutKey = GetKey(str)) == -1 )
               {
                  logit("", "adsend: Invalid OutRing <%s>. Exiting.\n", str );
                  return -1;
               }
            }
            init[9] = 1;
         }

         else if ( k_its( "HeartbeatInt" ) )
         {
            HeartbeatInt = k_int();
            init[10] = 1;
         }

         else if ( k_its( "MaxGuideNoise" ) )
         {
            MaxGuideNoise = (float)k_val();
            init[11] = 1;
         }

         else if ( k_its( "Year" ) )
         {
            Year = k_int();
            init[12] = 1;
         }

         else if ( k_its( "NumGuide" ) )
         {
            NumGuide = k_int();
            if ( NumGuide > 0 )
            {
               GuideChan = (int *) calloc( NumGuide, sizeof(int) );
               if ( GuideChan == NULL )
               {
                  logit("", "Error: Cannot allocate GuideChan array.\n" );
                  return -1;
               }
            }
            init[13] = 1;
         }

         else if ( k_its( "GuideChan" ) )
         {
            for (i = 0; i < NumGuide; i++)
               GuideChan[i] = k_int();
         }

         else if ( k_its( "TimeForLock" ) )
         {
            TimeForLock = k_int();
            init[14] = 1;
         }

         else if ( k_its( "LowValue" ) )
         {
            LowValue = k_val();
            init[15] = 1;
         }

         else if ( k_its( "HighValue" ) )
         {
            HighValue = k_val();
            init[16] = 1;
         }

         else if ( k_its( "SendBadTime" ) )
         {
            SendBadTime = k_int();
            init[17] = 1;
         }

         else if ( k_its( "UpdateSysClock" ) )
         {
            UpdateSysClock = k_int();
            init[18] = 1;
         }

         else if ( k_its( "YearFromPC" ) )
         {
            YearFromPC = k_int();
            init[19] = 1;
         }

         else if ( k_its( "IrigeIsLocalTime" ) )
         {
            IrigeIsLocalTime = k_int();
            init[20] = 1;
         }

         else if ( k_its( "TimeoutNoSend" ) )
         {
            TimeoutNoSend = k_int();
            init[21] = 1;
         }

         else if ( k_its( "TimeoutNoSynch" ) )
         {
            TimeoutNoSynch = k_int();
            init[22] = 1;
         }

         else if ( k_its( "TimeoutNoTrig" ) )
         {
            TimeoutNoTrig = k_int();
            init[23] = 1;
         }

         else if ( k_its( "ErrNoLockTime" ) )
         {
            ErrNoLockTime = k_int();
            init[24] = 1;
         }

         else if ( k_its( "EnableBell" ) )
         {
            EnableBell = k_int();
            init[25] = 1;
         }

         else if ( k_its( "Nchan" ) )
         {
            Nchan = k_int();
         }

/* Get the channel list
   ********************/
         else if ( k_its( "Chan" ) )                // Scnl value for each channel
         {
            static int first = 1;
            int        chan;
            int        rc;                          // kom function return code

            if ( first )                            // First time a Chan line was found
            {
               if ( init[1]==0 || init[2]==0)
               {
                  logit("", "Error. In the config file, the NumMuxBoards and OnboardChans\n" );
                  logit("", "       specification must appear before any Chan lines.\n" );
                  return -1;
               }
                           if ( Nchan == 0 )
                           {
                               Nchan    = (NumMuxBoards == 0) ? OnboardChans :
                               (4 * OnboardChans * NumMuxBoards );
                           }
               ChanList = (SCNL *) calloc( Nchan, sizeof(SCNL) );
               if ( ChanList == NULL )
               {
                  logit("", "Error. Cannot allocate the channel list.\n" );
                  return -1;
               }
               first = 0;
            }

            chan = k_int();                          // Get channel number
            if ( chan>=Nchan || chan<0 )
            {
               logit("", "Error. Bad channel number (%d) in config file.\n", chan );
               return -1;
            }

            strcpy( ChanList[chan].sta,  k_str() );  // Save Scnl value in chan list
            strcpy( ChanList[chan].comp, k_str() );
            strcpy( ChanList[chan].net,  k_str() );
            strcpy( ChanList[chan].loc,  k_str() );
            ChanList[chan].pin = k_int();

            rc = k_err();
            if ( rc == -1 )                          // No more tokens in line
               ChanList[chan].pin = chan;            // Default pin number is mux chan number

            if ( rc == -2 )                          // Unreadable pin number
            {
               logit("", "Bad pin number for DAQ channel %d\n", chan );
               return -1;
            }
                        nChanLines++;
         }

/* An unknown parameter was encountered
   ************************************/
         else
         {
            logit("", "adsend: <%s> unknown parameter in <%s>\n", com, configfile );
            return -1;
         }

/* See if there were any errors processing the command
   ***************************************************/
         if ( k_err() )
         {
            logit("", "adsend: Bad <%s> command in <%s>.\n", com, configfile );
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
      if ( !init[1]  ) logit("", "<OnboardChans> " );
      if ( !init[2]  ) logit("", "<NumMuxBoards> " );
      if ( !init[3]  ) logit("", "<ChanRate> " );
      if ( !init[4]  ) logit("", "<ChanMsgSize> " );
      if ( !init[5]  ) logit("", "<NumTimeCodeChan> " );
      if ( !init[6]  ) logit("", "<Gain> " );
      if ( !init[7]  ) logit("", "<ExtTrig> " );
      if ( !init[8]  ) logit("", "<MinGuideSignal> " );
      if ( !init[9]  ) logit("", "<OutRing> " );
      if ( !init[10] ) logit("", "<HeartbeatInt> " );
      if ( !init[11] ) logit("", "<MaxGuideNoise> " );
      if ( !init[12] ) logit("", "<Year> " );
      if ( !init[13] ) logit("", "<NumGuide> " );
      if ( !init[14] ) logit("", "<TimeForLock> " );
      if ( !init[15] ) logit("", "<LowValue> " );
      if ( !init[16] ) logit("", "<HighValue> " );
      if ( !init[17] ) logit("", "<SendBadTime> " );
      if ( !init[18] ) logit("", "<UpdateSysClock> " );
      if ( !init[19] ) logit("", "<YearFromPC> " );
      if ( !init[20] ) logit("", "<IrigeIsLocalTime> " );
      if ( !init[21] ) logit("", "<TimeoutNoSend> " );
      if ( !init[22] ) logit("", "<TimeoutNoSynch> " );
      if ( !init[23] ) logit("", "<TimeoutNoTrig> " );
      if ( !init[24] ) logit("", "<ErrNoLockTime> " );
      if ( !init[25] ) logit("", "<EnableBell> " );
      logit("", "command(s) in <%s>.\n", configfile );
      return -1;
   }

   return 0;
}


 /***********************************************************************
  *                              LogConfig()                            *
  *                                                                     *
  *                   Log the configuration parameters                  *
  ***********************************************************************/

void LogConfig( void )
{
   int i;
   int used   = 0;
   int unused = 0;

   logit( "", "ModuleId:             %8u\n",    ModuleId );
   logit( "", "OnboardChans:         %8d\n",    OnboardChans );
   logit( "", "NumMuxBoards:         %8d\n",    NumMuxBoards );
   logit( "", "Total channels:       %8d\n",    Nchan );
   logit( "", "ChanRate:             %8.3lf\n", ChanRate );
   logit( "", "ChanMsgSize:          %8d\n",    ChanMsgSize );
   logit( "", "SampRate:             %10.2lf\n",SampRate );
   logit( "", "NumTimeCodeChan:      %8d\n",    NumTimeCodeChan );
   logit( "", "EnableBell:           %8d\n",    EnableBell );

   if ( NumTimeCodeChan > 0 )
   {
      logit( "", "TimeCodeChan:         " );
      for ( i = 0;  i < NumTimeCodeChan; i++ )
         logit( "", " %d", TimeCodeChan[i] );
      logit( "", "\n" );
   }
   logit( "", "Gain:                 %8d\n",    Gain );
   logit( "", "ExtTrig:              %8d\n",    ExtTrig );
   logit( "", "MinGuideSignal:       %8.1lf\n", MinGuideSignal );
   logit( "", "OutKey:               %8d\n",    OutKey );
   logit( "", "HeartbeatInt:         %8d\n",    HeartbeatInt );
   logit( "", "MaxGuideNoise:        %8.1lf\n", MaxGuideNoise );
   logit( "", "Year:                 %8d\n",    Year );
   logit( "", "NumGuide:             %8d\n",    NumGuide );

   if ( NumGuide > 0 )
   {
      logit( "", "GuideChan:            " );
      for ( i = 0;  i < NumGuide; i++ )
         logit( "", " %d", GuideChan[i] );
      logit( "", "\n" );
   }
   logit( "", "SendBadTime:          %8d\n",    SendBadTime );
   logit( "", "UpdateSysClock:       %8d\n",    UpdateSysClock );
   logit( "", "YearFromPC:           %8d\n",    YearFromPC );
   logit( "", "IrigeIsLocalTime:     %8d\n",    IrigeIsLocalTime );
   logit( "", "TimeoutNoSend:        %8d\n",    TimeoutNoSend );
   logit( "", "TimeoutNoSynch:       %8d\n",    TimeoutNoSynch );
   logit( "", "TimeoutNoTrig:        %8d\n",    TimeoutNoTrig );
   logit( "", "ErrNoLockTime:        %8d\n",    ErrNoLockTime );
   logit( "", "TimeForLock:          %8d\n",    TimeForLock );
   logit( "", "LowValue:             %8.3lf\n", LowValue );
   logit( "", "HighValue:            %8.3lf\n", HighValue );

/* Log the channel list
   ********************/
   logit( "", "\n\n" );
   logit( "", "  DAQ\n" );
   logit( "", "channel   Sta Comp Net  Pin\n" );
   logit( "", "-------   --- ---- ---  ---\n" );

   for ( i = 0; i < Nchan; i++ )
   {
      if ( strlen( ChanList[i].sta  ) > 0 )          /* This channel is used */
      {
         used++;
         logit( "", "  %4d   %-5s %-3s %-2s %5d\n", i,
            ChanList[i].sta,
            ChanList[i].comp,
            ChanList[i].net,
            ChanList[i].pin );
      }
      else                                           /* This channel is unused */
      {
         unused++;
         logit( "", "  %4d     Unused\n", i );
      }
   }

   logit( "", "\n" );
   logit( "", "Number of channels used:   %3d\n", used );
   logit( "", "Number of channels unused: %3d\n", unused );
   return;
}









    /*********************************************************************
     *                             IsComment()                           *
     *                                                                   *
     *  Accepts: String containing one line from a config file.          *
     *  Returns: 1 if it's a comment line                                *
     *           0 if it's not a comment line                            *
     *********************************************************************/

int IsComment( char string[] )
{
   int i;

   for ( i = 0; i < (int)strlen( string ); i++ )
   {
      char test = string[i];

      if ( test!=' ' && test!='\t' )
      {
         if ( test == '#'  )
            return 1;          /* It's a comment line */
         else
            return 0;          /* It's not a comment line */
      }
   }
   return 1;                   /* It contains only whitespace */
}
