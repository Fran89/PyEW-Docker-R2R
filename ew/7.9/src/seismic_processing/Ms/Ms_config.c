       /****************************************************************
       *  Ms_config.c                                                  *
       *                                                               *
       *  This file contains the configuration routines used by        *
       *  the Ms module.  These are based on standard                  *
       *  Earthworm configuration files and source code.               *
       *                                                               *
       *  This code is based on configuration routines used in         *
       *  WCATWC Earlybird module lpproc.                              *
       *                                                               *
       *  2011: Paul Whitmore, NOAA-WCATWC - paul.whitmore@noaa.gov    *
       *                                                               *
       ****************************************************************/

#include <stdio.h>
#include <string.h>
#include <kom.h>
#include <transport.h>
#include <earthworm.h>
#include "Ms.h"

#define ncommand 23          /* Number of commands in the config file */


 /***********************************************************************
  *                              GetConfig()                            *
  *             Processes command file using kom.c functions.           *
  *               Returns -1 if any errors are encountered.             *
  ***********************************************************************/

int GetConfig( char *config_file, GPARM *Gparm )
{
   char     init[ncommand];     /* Flags, one for each command */
   int      nmiss;              /* Number of commands that were missed */
   int      nfiles;
   int      i;

/* Set to zero one init flag for each required command
   ***************************************************/
   for ( i=0; i<ncommand; i++ ) init[i] = 0;
   strcpy( Gparm->szATPLineupFileLP, "\0" );

/* Open the main configuration file
   ********************************/
   nfiles = k_open( config_file );
   if ( nfiles == 0 )
   {
      fprintf( stderr, "Ms: Error opening configuration file <%s>\n",
               config_file );
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
               fprintf( stderr, "Ms: Error opening command file <%s>."
			            "\n", &com[1] );
               return -1;
            }
            continue;
         }

/* Read configuration parameters
   *****************************/
         else if ( k_its( "StaFile" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->szStaFile, str );
            init[0] = 1;
         }
		 
         else if ( k_its( "StaDataFile" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->szStaDataFile, str );
            init[1] = 1;
         }

         else if ( k_its( "InRing" ) )
         {
            if ( (str = k_str()) != NULL )
            {
               if( (Gparm->lInKey = GetKey(str)) == -1 )
               {
                  fprintf( stderr, "Ms: Invalid InRing name <%s>. "
                                   "Exiting.\n", str );
                  return -1;
               }
            }
            init[2] = 1;
         }

         else if ( k_its( "PRing" ) )
         {
            if ( (str = k_str()) != NULL )
            {
               if ( (Gparm->lPKey = GetKey(str)) == -1 )
               {
                  fprintf( stderr, "Ms: Invalid PRing name <%s>. "
                                   "Exiting.\n", str );
                  return -1;
               }
            }
            init[3] = 1;
         }

         else if ( k_its( "HypoRing" ) )
         {
            if ( (str = k_str()) != NULL )
            {
               if ( (Gparm->lHKey = GetKey(str)) == -1 )
               {
                  fprintf( stderr, "Ms: Invalid HypoRing name <%s>. "
                                   "Exiting.\n", str );
                  return -1;
               }
            }
            init[4] = 1;
         }

         else if ( k_its( "HeartbeatInt" ) )
         {
            Gparm->iHeartbeatInt = k_int();
            init[5] = 1;
         }

         else if ( k_its( "LowCutFilter" ) )
         {
            Gparm->dLowCutFilter = k_val();
            init[6] = 1;
         }

         else if ( k_its( "HighCutFilter" ) )
         {
            Gparm->dHighCutFilter = k_val();
            init[7] = 1;
         }

         else if ( k_its( "SigNoise" ) )
         {
            Gparm->dSigNoise = k_val();
            init[8] = 1;
         }

         else if ( k_its( "Debug" ) )
         {
            Gparm->iDebug = k_int();
            init[9] = 1;
         }

         else if ( k_its( "MyModId" ) )
         {
            if ( (str = k_str()) != NULL )
            {
               if ( GetModId(str, &Gparm->ucMyModId) == -1 )
               {
                  fprintf( stderr, "Ms: Invalid MyModId <%s>.\n", str);
                  return -1;
               }
            }
            init[10] = 1;
         }

         else if ( k_its( "MinutesInBuff" ) )
         {
            Gparm->iMinutesInBuff = k_int();
            init[11] = 1;
         }
		 
         else if ( k_its( "DummyFile" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->szDummyFile, str );
            init[12] = 1;
         }
		 
         else if ( k_its( "LPRTFile" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->szLPRTFile, str );
            init[13] = 1;
         }
		 
         else if ( k_its( "QuakeFile" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->szQuakeFile, str );
            init[14] = 1;
         }

         else if ( k_its( "AutoStart" ) )
         {
            Gparm->iAutoStart = k_int();
            init[15] = 1;
         }
		 
         else if ( k_its( "ResponseFile" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->szResponseFile, str );
            init[16] = 1;
         }

         else if ( k_its( "MagThreshForAuto" ) )
         {
            Gparm->dMagThreshForAuto = k_val();
            init[17] = 1;
         }
		 
         else if ( k_its( "DataDirectory" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->szDataDirectory, str );
            init[18] = 1;
         }

         else if ( k_its( "FileLengthLP" ) )
         {
            Gparm->iFileLengthLP = k_int();
            init[19] = 1;
         }

         else if ( k_its( "NumStnForAuto" ) )
         {
            Gparm->iNumStnForAuto = k_int();
            init[20] = 1;
         }
		 
         else if ( k_its( "FileSuffix" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->szFileSuffix, str );
            init[21] = 1;
         }
		 
         else if ( k_its( "ArchiveDir" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->szArchiveDir, str );
            init[22] = 1;
         }
	 
/* Optional command when run with ATPlayer
  ****************************************/	 
         else if ( k_its( "ATPLineupFileLP" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->szATPLineupFileLP, str );
         }

/* An unknown parameter was encountered
   ************************************/
         else
         {
            fprintf( stderr, "Ms: <%s> unknown parameter in <%s>\n",
                     com, config_file );
            continue;
         }

/* See if there were any errors processing the command
   ***************************************************/
         if ( k_err() )
         {
            fprintf( stderr, "Ms: Bad <%s> command in <%s>.\n", com,
                     config_file );
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
      fprintf( stderr, "Ms: ERROR, no " );
      if ( !init[0]  ) fprintf( stderr, "<StaFile> " );
      if ( !init[1]  ) fprintf( stderr, "<StaDataFile> " );
      if ( !init[2]  ) fprintf( stderr, "<InRing> " );
      if ( !init[3]  ) fprintf( stderr, "<PRing> " );
      if ( !init[4]  ) fprintf( stderr, "<HypoRing> " );
      if ( !init[5]  ) fprintf( stderr, "<HeartbeatInt> " );
      if ( !init[6]  ) fprintf( stderr, "<LowCutFilter> " );
      if ( !init[7]  ) fprintf( stderr, "<HighCutFilter> " );
      if ( !init[8]  ) fprintf( stderr, "<SigNoise> " );
      if ( !init[9]  ) fprintf( stderr, "<Debug> " );
      if ( !init[10] ) fprintf( stderr, "<MyModId> " );
      if ( !init[11] ) fprintf( stderr, "<MinutesInBuff> " );
      if ( !init[12] ) fprintf( stderr, "<DummyFile> " );
      if ( !init[13] ) fprintf( stderr, "<LPRTFile> " );
      if ( !init[14] ) fprintf( stderr, "<QuakeFile> " );
      if ( !init[15] ) fprintf( stderr, "<AutoStart> " );
      if ( !init[16] ) fprintf( stderr, "<ResponseFile> " );
      if ( !init[17] ) fprintf( stderr, "<MagThreshForAuto> " );
      if ( !init[18] ) fprintf( stderr, "<DataDirectory> " );
      if ( !init[19] ) fprintf( stderr, "<FileLengthLP> " );
      if ( !init[20] ) fprintf( stderr, "<NumStnForAuto> " );
      if ( !init[21] ) fprintf( stderr, "<FileSuffix> " );
      if ( !init[21] ) fprintf( stderr, "<ArchiveDir> " );
      fprintf( stderr, "command(s) in <%s>. Exiting.\n", config_file );
      return -1;
   }
   return 0;
}


 /***********************************************************************
  *                              LogConfig()                            *
  *                                                                     *
  *                   Log the configuration parameters                  *
  ***********************************************************************/

void LogConfig( GPARM *Gparm )
{
   logit( "", "\n" );
   logit( "", "StaFile:          %s\n",    Gparm->szStaFile );
   logit( "", "StaDataFile:      %s\n",    Gparm->szStaDataFile );
   logit( "", "InKey:            %d\n",    Gparm->lInKey );
   logit( "", "PKey:             %d\n",    Gparm->lPKey );
   logit( "", "HKey:             %d\n",    Gparm->lHKey );
   logit( "", "HeartbeatInt:     %d\n",    Gparm->iHeartbeatInt );
   logit( "", "LowCutFilter:     %lf\n",   Gparm->dLowCutFilter );
   logit( "", "HighCutFilter:    %lf\n",   Gparm->dHighCutFilter );
   logit( "", "SigNoise:         %lf\n",   Gparm->dSigNoise );
   logit( "", "Debug:            %d\n",    Gparm->iDebug );
   logit( "", "MyModId:          %u\n",    Gparm->ucMyModId );
   logit( "", "MinutesInBuff     %d\n",    Gparm->iMinutesInBuff );
   logit( "", "DummyFile:        %s\n",    Gparm->szDummyFile );
   logit( "", "LPRTFile:         %s\n",    Gparm->szLPRTFile );
   logit( "", "QuakeFile:        %s\n",    Gparm->szQuakeFile );
   logit( "", "ResponseFile:     %s\n",    Gparm->szResponseFile );
   logit( "", "AutoStart:        %d\n",    Gparm->iAutoStart );
   logit( "", "MagThreshForAuto: %lf\n",   Gparm->dMagThreshForAuto );
   logit( "", "DataDirectory:    %s\n",    Gparm->szDataDirectory );
   logit( "", "FileLengthLP:     %d\n",    Gparm->iFileLengthLP );
   logit( "", "NumStnForAuto:    %d\n",    Gparm->iNumStnForAuto );
   logit( "", "FileSuffix:       %s\n",    Gparm->szFileSuffix );
   logit( "", "ArchiveDir:       %s\n",    Gparm->szArchiveDir );
   if ( strlen( Gparm->szATPLineupFileLP ) > 2 )
      logit( "", "Ms to be used with Player - File: %s\n",
       Gparm->szATPLineupFileLP);
   return;
}
