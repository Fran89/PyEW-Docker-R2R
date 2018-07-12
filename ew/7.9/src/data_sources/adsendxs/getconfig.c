
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <kom.h>
#include <earthworm.h>
#include "adsendxs.h"

#define TRIGGERCHANSIZE 80


/* Function declaration
   ********************/
int IsComment( char string[] ); // Defined below

/* Read these params from configuration file
   *****************************************/
unsigned char ModuleId;       // Module id of this program
int      Nchan = 0;           // Number of channels to digitize
int      SampRate;            // Samples per second per channel
long     OutKey;              // Key to ring where traces will live
int      HeartbeatInt;        // Heartbeat interval in seconds
int      GetGpsDelay;         // Wait this many msec before requesting GPS time
int      UpdatePcClock;       // If 1, update PC clock from GPS
int      PcClockUpdInt;       // Seconds between PC clock updates from GPS
SCNL     *ChanList;           // Array to fill with SCNL and pin values
char     TriggerChan[TRIGGERCHANSIZE]; // Physical channel where the 1-PPS trigger is found
int      ComPort;             // Com port for reading GPS time
int      BaudRate;            // Baud rate of com port
int      Debug;
int      SnwReportInterval;   // In seconds
char     StaName[STASIZE];    // For SeisnetWatch
char     NetCode[NETSIZE];    // For SeisnetWatch
int      ReadFileTimeout;     // ReadFile will timeout in this many milliseconds when
                              // reading bytes from the comm port.



/***********************************************
  GetConfig

  Process command file using kom.c functions.
  Return  0 if all ok.
  Return -1 if any errors are encountered.
 ***********************************************/

#define NCOMMAND 15         // Number of mandatory commands in config file


int GetConfig( char *configfile )
{
   const int ncommand = NCOMMAND;

   char     init[NCOMMAND];     // Flags, one for each command
   int      nmiss;              // Number of commands that were missed
   int      nfiles;
   int      i;

/* Allocate channel list
   *********************/
   ChanList = (SCNL *) calloc( MAXCHAN, sizeof(SCNL) );
   if ( ChanList == NULL )
   {
      logit("", "Error. Cannot allocate the channel list.\n" );
      return -1;
   }

/* Set to zero one init flag for each required command
   ***************************************************/
   for ( i = 0; i < ncommand; i++ )
      init[i] = 0;

/* Open the main configuration file
   ********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 )
   {
      logit("", "Error opening configuration file <%s>\n", configfile );
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
               logit("", "Error opening command file <%s>.\n", &com[1] );
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
                  logit("", "Invalid ModuleId <%s>. Exiting.\n", str );
                  return -1;
               }
            }
            init[0] = 1;
         }

         else if ( k_its( "SampRate" ) )
         {
            SampRate = k_int();
            init[1] = 1;
         }

         else if ( k_its( "OutRing" ) )
         {
            if ( str = k_str() )
            {
               if ( (OutKey = GetKey(str)) == -1 )
               {
                  logit("", "Invalid OutRing <%s>. Exiting.\n", str );
                  return -1;
               }
            }
            init[2] = 1;
         }

         else if ( k_its( "HeartbeatInt" ) )
         {
            HeartbeatInt = k_int();
            init[3] = 1;
         }

         else if ( k_its( "UpdatePcClock" ) )
         {
            UpdatePcClock = k_int();
            init[4] = 1;
         }

         else if ( k_its( "TriggerChan" ) )
         {
            strcpy_s( TriggerChan, TRIGGERCHANSIZE, k_str() );
            init[5] = 1;
         }

         else if ( k_its( "ComPort" ) )
         {
            ComPort = k_int();
            init[6] = 1;
         }

         else if ( k_its( "GetGpsDelay" ) )
         {
            GetGpsDelay = k_int();
            init[7] = 1;
         }

         else if ( k_its( "Debug" ) )
         {
            Debug = k_int();
            init[8] = 1;
         }

         else if( k_its( "StaName" ) )
         {
            str = k_str();
            if (str)
            {
               if ( strlen(str) > 5 )
               {
                  logit( "", "adsendxs: Error. StaName is more than five characters long.\n" );
                  logit( "", "Length: %d   Exiting.\n", strlen(str) );
                  return -1;
               }
               strcpy_s( StaName, STASIZE, str );
               init[9] = 1;
            }
         }

         else if( k_its( "NetCode" ) )
         {
            str = k_str();
            if (str)
            {
               if ( strlen(str) > 2 )
               {
                  logit( "", "adsendxs: Error. NetCode is more than two characters long.\n" );
                  logit( "", "Length: %d   Exiting.\n", strlen(str) );
                  return -1;
               }
               strcpy_s( NetCode, NETSIZE, str );
               init[10] = 1;
            }
         }

         else if ( k_its( "SnwReportInterval" ) )
         {
            SnwReportInterval = k_int();
            init[11] = 1;
         }

         else if ( k_its( "BaudRate" ) )
         {
            BaudRate = k_int();
            init[12] = 1;
         }

         else if ( k_its( "PcClockUpdInt" ) )
         {
            PcClockUpdInt = k_int();
            init[13] = 1;
         }

         else if ( k_its( "ReadFileTimeout" ) )
         {
            ReadFileTimeout = k_int();
            init[14] = 1;
         }

/* Get the Scnl value for each channel
   ***********************************/
         else if ( k_its( "Chan" ) )
         {
            if ( Nchan >= MAXCHAN )
            {
               logit( "", "Too many Chan lines in config file.\n" );
               logit( "", "Max = %d\n", MAXCHAN );
               return -1;
            }
            ChanList[Nchan].pin = k_int();
            strcpy_s( ChanList[Nchan].sta, STASIZE,     k_str() );
            strcpy_s( ChanList[Nchan].comp, COMPSIZE,    k_str() );
            strcpy_s( ChanList[Nchan].net, NETSIZE,     k_str() );
            strcpy_s( ChanList[Nchan].loc, LOCSIZE,     k_str() );
            strcpy_s( ChanList[Nchan].daqChan, DAQCHANSIZE, k_str() );
            Nchan++;
         }

/* An unknown parameter was encountered
   ************************************/
         else
         {
            logit("", "<%s> unknown parameter in <%s>\n", com, configfile );
            return -1;
         }

/* See if there were any errors processing the command
   ***************************************************/
         if ( k_err() )
         {
            logit("", "Bad <%s> command in <%s>.\n", com, configfile );
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
      logit("", "ERROR, no " );
      if ( !init[0]  ) logit("", "<ModuleId> " );
      if ( !init[1]  ) logit("", "<SampRate> " );
      if ( !init[2]  ) logit("", "<OutRing> " );
      if ( !init[3]  ) logit("", "<HeartbeatInt> " );
      if ( !init[4]  ) logit("", "<UpdatePcClock> " );
      if ( !init[5]  ) logit("", "<TriggerChan> " );
      if ( !init[6]  ) logit("", "<ComPort> " );
      if ( !init[7]  ) logit("", "<GetGpsDelay> " );
      if ( !init[8]  ) logit("", "<Debug> " );
      if ( !init[9]  ) logit("", "<StaName> " );
      if ( !init[10] ) logit("", "<NetCode> " );
      if ( !init[11] ) logit("", "<SnwReportInterval> " );
      if ( !init[12] ) logit("", "<BaudRate> " );
      if ( !init[13] ) logit("", "<PcClockUpdInt> " );
      if ( !init[14] ) logit("", "<ReadFileTimeout> " );
      logit("", "command(s) in <%s>.\n", configfile );
      return -1;
   }

/* Make sure there are some channels to digitize
   *********************************************/
   if ( Nchan == 0 )
   {
      logit( "", "No channels to digitize are specified in the config file.\n" );
      logit( "", "There must be at least one 'Chan' command .\n" );
      return -1;
   }
   return 0;
}


/***************************************
  LogConfig
  Log the configuration file parameters.
 ***************************************/

void LogConfig( void )
{
   int i;

   logit( "", "Debug:                %8d\n", Debug );
   logit( "", "ModuleId:             %8u (Earthworm module id)\n", ModuleId );
   logit( "", "SampRate:             %8d (Samples per second per channel)\n", SampRate );
   logit( "", "OutKey:               %8d (Shared-memory key)\n",    OutKey );
   logit( "", "HeartbeatInt:         %8d seconds\n", HeartbeatInt );
   logit( "", "ComPort:              %8d\n", ComPort );
   logit( "", "BaudRate:             %8d\n", BaudRate );
   logit( "", "GetGpsDelay:          %8d\n", GetGpsDelay );
   logit( "", "ReadFileTimeout:      %8d milliseconds\n", ReadFileTimeout );
   logit( "", "TriggerChan:          %s (NI physical channel of trigger pulse input)\n", TriggerChan );
   logit( "", "UpdatePcClock:        %8d (If 1, update PC clock from GPS)\n", UpdatePcClock );
   logit( "", "PcClockUpdInt:        %8d (Seconds between PC clock updates from GPS)\n", PcClockUpdInt );
   logit( "", "StaName:              %7s (For SeisnetWatch)\n", StaName );
   logit( "", "NetCode:              %7s (For SeisnetWatch)\n", NetCode );
   logit( "", "SnwReportInterval:    %8d seconds\n", SnwReportInterval );
   logit( "", "Nchan:                %8d (Number of channels to digitize)\n", Nchan );
   logit( "", "\n" );
   logit( "", "  Pin    Sta Comp Net Loc   daqChan\n" );
   logit( "", "  ---    --- ---- --- ---   -------\n" );

   for ( i = 0; i < Nchan; i++ )
      logit( "", " %4d   %-5s %-3s  %-2s  %-2s   %-s\n", ChanList[i].pin,
         ChanList[i].sta, ChanList[i].comp, ChanList[i].net, ChanList[i].loc,
         ChanList[i].daqChan );
   logit( "", "\n" );
   return;
}


/*************************************************************
  IsComment

  Accepts: String containing one line from a config file.
  Returns: 1 if it's a comment line
           0 if it's not a comment line
 *************************************************************/

int IsComment( char string[] )
{
   unsigned i;

   for ( i = 0; i < strlen( string ); i++ )
   {
      char test = string[i];

      if ( test != ' ' && test != '\t' )
      {
         if ( test == '#'  )
            return 1;          // This is a comment line
         else
            return 0;          // Not a comment line
      }
   }
   return 1;                   // Line contains only whitespace
}
