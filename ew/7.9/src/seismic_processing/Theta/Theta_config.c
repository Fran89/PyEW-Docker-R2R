       /****************************************************************
       *  Theta_config.c                                               *
       *                                                               *
       *  This file contains the configuration routines used by        *
       *  the Theta module.  These are based on standard               *
       *  Earthworm configuration files and source code.               *
       *                                                               *
       *  This code is based on configuration routines used in         *
       *  the WCATWC Earlybird module Engmom.                          *
       *                                                               *
       *  2012: Paul Whitmore, NOAA-WCATWC - paul.whitmore@noaa.gov    *
       *                                                               *
       ****************************************************************/

#include <stdio.h>
#include <string.h>
#include <kom.h>
#include <transport.h>
#include <earthworm.h>
#include "Theta.h"

#define ncommand 27                  /* Number of commands in the config file */


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
   strcpy( Gparm->szATPLineupFileBB, "\0" );

/* Open the main configuration file
   ********************************/
   nfiles = k_open( config_file );
   if ( nfiles == 0 )
   {
      fprintf( stderr, "Theta: Error opening configuration file <%s>\n",
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
               fprintf( stderr, "Theta: Error opening command file <%s>."
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
                  fprintf( stderr, "Theta: Invalid InRing name <%s>. "
                                   "Exiting.\n", str );
                  return -1;
               }
            }
            init[2] = 1;
         }

         else if ( k_its( "HypoRing" ) )
         {
            if ( (str = k_str()) != NULL )
            {
               if ( (Gparm->lHKey = GetKey(str)) == -1 )
               {
                  fprintf( stderr, "Theta: Invalid HypoRing name <%s>. "
                                   "Exiting.\n", str );
                  return -1;
               }
            }
            init[3] = 1;
         }

         else if ( k_its( "HeartbeatInt" ) )
         {
            Gparm->iHeartbeatInt = k_int();
            init[4] = 1;
         }

         else if ( k_its( "Debug" ) )
         {
            Gparm->iDebug = k_int();
            init[5] = 1;
         }

         else if ( k_its( "MyModId" ) )
         {
            if ( (str = k_str()) != NULL )
            {
               if ( GetModId(str, &Gparm->MyModId) == -1 )
               {
                  fprintf( stderr, "Theta: Invalid MyModId <%s>.\n", str);
                  return -1;
               }
            }
            init[6] = 1;
         }

         else if ( k_its( "MinutesInBuff" ) )
         {
            Gparm->iMinutesInBuff = k_int();
            init[7] = 1;
         }
		 
         else if ( k_its( "DummyFile" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->szDummyFile, str );
            init[8] = 1;
         }
		 
         else if ( k_its( "ThetaFile" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->szThetaFile, str );
            init[9] = 1;
         }

         else if ( k_its( "MinDelta" ) )
         {
            Gparm->dMinDelta = k_val();
            init[10] = 1;
         }

         else if ( k_its( "MaxDelta" ) )
         {
            Gparm->dMaxDelta = k_val();
            init[11] = 1;
         }

         else if ( k_its( "WindowLength" ) )
         {
            Gparm->iWindowLength = k_int();
            init[12] = 1;
         }

         else if ( k_its( "FiltLo" ) )
         {
            Gparm->dFiltLo = k_val();
            init[13] = 1;
         }

         else if ( k_its( "FiltHi" ) )
         {
            Gparm->dFiltHi = k_val();
            init[14] = 1;
         }
		 
         else if ( k_its( "DiskPFile" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->szDiskPFile, str );
            init[15] = 1;
         }

         else if ( k_its( "AutoStart" ) )
         {
            Gparm->iAutoStart = k_int();
            init[16] = 1;
         }
		 
         else if ( k_its( "ResponseFile" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->szResponseFile, str );
            init[17] = 1;
         }

         else if ( k_its( "MagThreshForAuto" ) )
         {
            Gparm->dMagThreshForAuto = k_val();
            init[18] = 1;
         }
		 
         else if ( k_its( "DataDirectory" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->szDataDirectory, str );
            init[19] = 1;
         }

         else if ( k_its( "FileSize" ) )
         {
            Gparm->iFileSize = k_int();
            init[20] = 1;
         }
		 
         else if ( k_its( "FileSuffix" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->szFileSuffix, str );
            init[21] = 1;
         }

         else if ( k_its( "NumStnForAuto" ) )
         {
            Gparm->iNumStnForAuto = k_int();
            init[22] = 1;
         }
		 
         else if ( k_its( "LocFilePath" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->szLocFilePath, str );
            init[23] = 1;
         }
		 
         else if ( k_its( "QuakeFile" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->szQuakeFile, str );
            init[24] = 1;
         }
		 
         else if ( k_its( "ArchiveDir" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->szArchiveDir, str );
            init[25] = 1;
         }
		 
         else if ( k_its( "AlarmFile" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->szAlarmFile, str );
            init[26] = 1;
         }

/* Optional command when run with ATPlayer
  ****************************************/	 
         else if ( k_its( "ATPLineupFileBB" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->szATPLineupFileBB, str );
         }

/* An unknown parameter was encountered
   ************************************/
         else
         {
            fprintf( stderr, "Theta: <%s> unknown parameter in <%s>\n",
                     com, config_file );
            continue;
         }

/* See if there were any errors processing the command
   ***************************************************/
         if ( k_err() )
         {
            fprintf( stderr, "Theta: Bad <%s> command in <%s>.\n", com,
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
      fprintf( stderr, "Theta: ERROR, no " );
      if ( !init[0] ) fprintf( stderr, "<StaFile> " );
      if ( !init[1] ) fprintf( stderr, "<StaDataFile> " );
      if ( !init[2] ) fprintf( stderr, "<InRing> " );
      if ( !init[3] ) fprintf( stderr, "<HypoRing> " );
      if ( !init[4] ) fprintf( stderr, "<HeartbeatInt> " );
      if ( !init[5] ) fprintf( stderr, "<Debug> " );
      if ( !init[6] ) fprintf( stderr, "<MyModId> " );
      if ( !init[7] ) fprintf( stderr, "<MinutesInBuff> " );
      if ( !init[8] ) fprintf( stderr, "<DummyFile> " );
      if ( !init[9] ) fprintf( stderr, "<ThetaFile> " );
      if ( !init[10] ) fprintf( stderr, "<MinDelta> " );
      if ( !init[11] ) fprintf( stderr, "<MaxDelta> " );
      if ( !init[12] ) fprintf( stderr, "<WindowLength> " );
      if ( !init[13] ) fprintf( stderr, "<FiltLo> " );
      if ( !init[14] ) fprintf( stderr, "<FiltHi> " );
      if ( !init[15] ) fprintf( stderr, "<DiskPFile> " );
      if ( !init[16] ) fprintf( stderr, "<AutoStart> " );
      if ( !init[17] ) fprintf( stderr, "<ResponseFile> " );
      if ( !init[18] ) fprintf( stderr, "<MagThreshForAuto> " );
      if ( !init[19] ) fprintf( stderr, "<DataDirectory> " );
      if ( !init[20] ) fprintf( stderr, "<FileSize> " );
      if ( !init[21] ) fprintf( stderr, "<FileSuffix> " );
      if ( !init[22] ) fprintf( stderr, "<NumStnForAuto> " );
      if ( !init[23] ) fprintf( stderr, "<LocFilePath> " );
      if ( !init[24] ) fprintf( stderr, "<QuakeFile> " );
      if ( !init[25] ) fprintf( stderr, "<ArchiveDir> " );
      if ( !init[26] ) fprintf( stderr, "<AlarmFile> " );
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
   logit( "", "StaFile:          %s\n",  Gparm->szStaFile );
   logit( "", "StaDataFile:      %s\n",  Gparm->szStaDataFile );
   logit( "", "InKey:            %d\n",  Gparm->lInKey );
   logit( "", "HKey:             %d\n",  Gparm->lHKey );
   logit( "", "HeartbeatInt:     %d\n",  Gparm->iHeartbeatInt );
   logit( "", "Debug:            %d\n",  Gparm->iDebug );
   logit( "", "MyModId:          %u\n",  Gparm->MyModId );
   logit( "", "MinutesInBuff     %d\n",  Gparm->iMinutesInBuff );
   logit( "", "DummyFile:        %s\n",  Gparm->szDummyFile );
   logit( "", "ThetaFile:        %s\n",  Gparm->szThetaFile );
   logit( "", "MinDelta:         %lf\n", Gparm->dMinDelta );
   logit( "", "MaxDelta:         %lf\n", Gparm->dMaxDelta );
   logit( "", "WindowLength:     %d\n",  Gparm->iWindowLength );
   logit( "", "FiltLo:           %lf\n", Gparm->dFiltLo );
   logit( "", "FiltHi:           %lf\n", Gparm->dFiltHi );
   logit( "", "DiskPFile:        %s\n",  Gparm->szDiskPFile );
   logit( "", "AutoStart:        %d\n",  Gparm->iAutoStart );
   logit( "", "ResponseFile:     %s\n",  Gparm->szResponseFile );
   logit( "", "MagThreshForAuto: %lf\n", Gparm->dMagThreshForAuto );
   logit( "", "DataDirectory:    %s\n",  Gparm->szDataDirectory );
   logit( "", "FileSize:         %d\n",  Gparm->iFileSize );
   logit( "", "FileSuffix:       %s\n",  Gparm->szFileSuffix );
   logit( "", "NumStnForAuto:    %d\n",  Gparm->iNumStnForAuto );
   logit( "", "LocFilePath:      %s\n",  Gparm->szLocFilePath );
   logit( "", "QuakeFile:        %s\n",  Gparm->szQuakeFile );
   logit( "", "ArchiveDir:       %s\n",  Gparm->szArchiveDir );
   logit( "", "AlarmFile:        %s\n",  Gparm->szAlarmFile );
   if ( strlen( Gparm->szATPLineupFileBB ) > 2 )
      logit( "", "Theta to be used with Player - File: %s\n",
       Gparm->szATPLineupFileBB );
   return;
}

