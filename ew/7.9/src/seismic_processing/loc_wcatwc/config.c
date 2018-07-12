#include <stdio.h>
#include <string.h>
#include <kom.h>
#include <transport.h>
#include <earthworm.h>
#include "loc_wcatwc.h"         

#define ncommand 45                  /* Number of commands in the config file */


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
   strcpy( Gparm->ATPLineupFileBB, "\0" );

/* Open the main configuration file
   ********************************/
   nfiles = k_open( config_file );
   if ( nfiles == 0 )
   {
      fprintf( stderr, "loc_wcatwc: Error opening configuration file <%s>\n",
               config_file );
      return -1;
   }

/* Process all nested configuration files
   **************************************/
   while ( nfiles > 0 )                  /* While there are config files open */
   {
      while ( k_rd() )                    /* Read next line from active file  */
      {
         int  success;
         char *com;
         char *str;

         com = k_str();                      /* Get the first token from line */

         if ( !com ) continue;                          /* Ignore blank lines */
         if ( com[0] == '#' ) continue;                    /* Ignore comments */

/* Open another configuration file
   *******************************/
         if ( com[0] == '@' )
         {
            success = nfiles + 1;
            nfiles  = k_open( &com[1] );
            if ( nfiles != success )
            {
               fprintf( stderr, "loc_wcatwc: Error opening command file <%s>."
			            "\n", &com[1] );
               return -1;
            }
            continue;
         }

/* Read configuration parameters
   *****************************/
         else if ( k_its( "StaFile" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->StaFile, str );
            init[23] = 1;
         }
		 
         else if ( k_its( "StaDataFile" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->StaDataFile, str );
            init[0] = 1;
         }

         else if ( k_its( "InRing" ) )
         {
            if (  (str = k_str()) != NULL )
            {
               if( (Gparm->InKey = GetKey(str)) == -1 )
               {
                  fprintf( stderr, "loc_wcatwc: Invalid InRing name <%s>. "
                                   "Exiting.\n", str );
                  return -1;
               }
            }
            init[1] = 1;
         }

         else if ( k_its( "OutRing" ) )
         {
            if (  (str = k_str()) != NULL )
            {
               if ( (Gparm->OutKey = GetKey(str)) == -1 )
               {
                  fprintf( stderr, "loc_wcatwc: Invalid OutRing name <%s>. "
                                   "Exiting.\n", str );
                  return -1;
               }
            }
            init[2] = 1;
         }

         else if ( k_its( "AlarmRing" ) )
         {
            if (  (str = k_str()) != NULL )
            {
               if ( (Gparm->AlarmKey = GetKey(str)) == -1 )
               {
                  fprintf( stderr, "loc_wcatwc: Invalid AlarmRing name <%s>. "
                                   "Exiting.\n", str );
                  return -1;
               }
            }
            init[3] = 1;
         }

         else if ( k_its( "HeartbeatInt" ) )
         {
            Gparm->HeartbeatInt = k_int();
            init[4] = 1;
         }

         else if ( k_its( "NumPBuffs" ) )
         {
            Gparm->NumPBuffs = k_int();
            init[5] = 1;
			if ( Gparm->NumPBuffs > MAX_PBUFFS )
               {
                  fprintf( stderr, "Too many P buffers specified <%d>. "
                                   "Exiting.\n", Gparm->NumPBuffs );
                  return -1;
               }
         }

         else if ( k_its( "MaxTimeBetweenPicks" ) )
         {
            Gparm->MaxTimeBetweenPicks = k_val();
            init[6] = 1;
         }

         else if ( k_its( "NumNearStn" ) )
         {
            Gparm->iNumNearStn = k_int();
            init[24] = 1;
         }

         else if ( k_its( "MaxDistance" ) )
         {
            Gparm->dMaxDist = k_val();
            init[40] = 1;
         }

         else if ( k_its( "MinPs" ) )
         {
            Gparm->MinPs = k_int();
            init[7] = 1;
            if ( Gparm->MinPs < 5 )
               {
                  fprintf( stderr, "MinPs too small <%d>. Minimum 5. "
                                   "Exiting.\n", Gparm->MinPs );
                  return -1;
               }
         }

         else if ( k_its( "Debug" ) )
         {
            Gparm->Debug = k_int();
            init[8] = 1;
         }

         else if ( k_its( "MyModId" ) )
         {
            if (  (str = k_str()) != NULL )
            {
               if ( GetModId(str, &Gparm->MyModId) == -1 )
               {
                  fprintf( stderr, "loc_wcatwc: Invalid MyModId <%s>.\n", str);
                  return -1;
               }
            }
            init[9] = 1;
         }
		 
         else if ( k_its( "BValFile" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->szBValFile, str );
            init[10] = 1;
         }
		 
         else if ( k_its( "OldQuakes" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->szOldQuakes, str );
            init[11] = 1;
         }
		 
         else if ( k_its( "AutoLoc" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->szAutoLoc, str );
            init[12] = 1;
         }
		 
         else if ( k_its( "DummyFile" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->szDummyFile, str );
            init[13] = 1;
         }
		 
         else if ( k_its( "MapFile" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->szMapFile, str );
            init[14] = 1;
         }
		 
         else if ( k_its( "RTPFile" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->szRTPFile, str );
            init[15] = 1;
         }
		 
         else if ( k_its( "QLogFile" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->szQLogFile, str );
            init[16] = 1;
         }
		 
         else if ( k_its( "MwFile" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->szMwFile, str );
            init[17] = 1;
         }
		 
         else if ( k_its( "DepthFile" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->szDepthDataFile, str );
            init[18] = 1;
         }

         else if ( k_its( "SouthernLat" ) )
         {
            Gparm->SouthernLat = k_val();
            init[19] = 1;
         }

         else if ( k_its( "NorthernLat" ) )
         {
            Gparm->NorthernLat = k_val();
            init[20] = 1;
         }

         else if ( k_its( "WesternLon" ) )
         {
            Gparm->WesternLon = k_val();
            init[21] = 1;
         }

         else if ( k_its( "EasternLon" ) )
         {
            Gparm->EasternLon = k_val();
            init[22] = 1;
         }
		 
         else if ( k_its( "PFilePath" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->szPFilePath, str );
            init[25] = 1;
         }
		 
         else if ( k_its( "CityFileWUC" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->CityFileWUC, str );
            init[26] = 1;
         }
		 
         else if ( k_its( "CityFileWLC" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->CityFileWLC, str );
            init[27] = 1;
         }
		 
         else if ( k_its( "CityFileEUC" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->CityFileEUC, str );
            init[28] = 1;
         }
		 
         else if ( k_its( "CityFileELC" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->CityFileELC, str );
            init[29] = 1;
         }
		 
         else if ( k_its( "NameFile" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->szNameFile, str );
            init[30] = 1;
         }
		 
         else if ( k_its( "NameFileLC" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->szNameFileLC, str );
            init[31] = 1;
         }
		 
         else if ( k_its( "IndexFile" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->szIndexFile, str );
            init[32] = 1;
         }
		 
         else if ( k_its( "LatFile" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->szLatFile, str );
            init[33] = 1;
         }

         else if ( k_its( "MinMagToSend" ) )
         {
            Gparm->MinMagToSend = k_val();
            init[34] = 1;
         }
	 
		          else if ( k_its( "ResponseFile" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->ResponseFile, str );
            init[35] = 1;
         }

         else if ( k_its( "Iasp91File" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->szIasp91File, str );
            init[36] = 1;
         }

         else if ( k_its( "Iasp91TblFile" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->szIasp91TblFile, str );
            init[37] = 1;
         }
		 
         else if ( k_its( "ThetaFile" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->szThetaFile, str );
            init[38] = 1;
         }

         else if ( k_its( "RedoLineupFile" ) )
         {
            Gparm->iRedoLineupFile = k_int();
            init[39] = 1;
         }
		 
         else if ( k_its( "FERegionPath" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->szPathRegions, str );
            init[41] = 1;
         }
		 
         else if ( k_its( "DistancePath" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->szPathDistances, str );
            init[42] = 1;
         }
		 
         else if ( k_its( "DirectionPath" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->szPathDirections, str );
            init[43] = 1;
         }
		 
         else if ( k_its( "CitiesPath" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->szPathCities, str );
            init[44] = 1;
         }

/* Optional command when run with ATPlayer
  ****************************************/	 
         else if ( k_its( "ATPLineupFileBB" ) )
         {
            if (  (str = k_str()) != NULL )
               strcpy( Gparm->ATPLineupFileBB, str );
         }

/* An unknown parameter was encountered
   ************************************/
         else
         {
            fprintf( stderr, "loc_wcatwc: <%s> unknown parameter in <%s>\n",
                    com, config_file );
            continue;
         }

/* See if there were any errors processing the command
   ***************************************************/
         if ( k_err() )
         {
            fprintf( stderr, "loc_wcatwc: Bad <%s> command in <%s>.\n", com,
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
      fprintf( stderr, "loc_wcatwc: ERROR, no " );
      if ( !init[0] ) fprintf( stderr, "<StaDataFile> " );
      if ( !init[1] ) fprintf( stderr, "<InRing> " );
      if ( !init[2] ) fprintf( stderr, "<OutRing> " );
      if ( !init[3] ) fprintf( stderr, "<AlarmRing> " );
      if ( !init[4] ) fprintf( stderr, "<HeartbeatInt> " );
      if ( !init[5] ) fprintf( stderr, "<NumPBuffs> " );
      if ( !init[6] ) fprintf( stderr, "<MaxTimeBetweenPicks> " );
      if ( !init[7] ) fprintf( stderr, "<MinPs> " );
      if ( !init[25] ) fprintf( stderr, "<NumNearStn> " );
      if ( !init[40] ) fprintf( stderr, "<MaxDistance> " );
      if ( !init[8] ) fprintf( stderr, "<Debug> " );
      if ( !init[9] ) fprintf( stderr, "<MyModId> " );
      if ( !init[10] ) fprintf( stderr, "<BValFile> " );
      if ( !init[11] ) fprintf( stderr, "<OldQuakes> " );
      if ( !init[12] ) fprintf( stderr, "<AutoLoc> " );
      if ( !init[13] ) fprintf( stderr, "<DummyFile> " );
      if ( !init[14] ) fprintf( stderr, "<MapFile> " );
      if ( !init[15] ) fprintf( stderr, "<RTPFile> " );
      if ( !init[16] ) fprintf( stderr, "<QLogFile> " );
      if ( !init[17] ) fprintf( stderr, "<MwFile> " );
      if ( !init[18] ) fprintf( stderr, "<DepthFile> " );
      if ( !init[19] ) fprintf( stderr, "<NorthernLat> " );
      if ( !init[20] ) fprintf( stderr, "<SouthernLat> " );
      if ( !init[21] ) fprintf( stderr, "<WesternLon> " );
      if ( !init[22] ) fprintf( stderr, "<EasternLon> " );
      if ( !init[23] ) fprintf( stderr, "<StaFile> " );
      if ( !init[25] ) fprintf( stderr, "<PFilePath> " );
      if ( !init[26] ) fprintf( stderr, "<CityFileWUC> " );
      if ( !init[27] ) fprintf( stderr, "<CityFileWLC> " );
      if ( !init[28] ) fprintf( stderr, "<CityFileEUC> " );
      if ( !init[29] ) fprintf( stderr, "<CityFileELC> " );
      if ( !init[30] ) fprintf( stderr, "<NameFile> " );
      if ( !init[31] ) fprintf( stderr, "<NameFileLC> " );
      if ( !init[32] ) fprintf( stderr, "<IndexFile> " );
      if ( !init[33] ) fprintf( stderr, "<LatFile> " );
      if ( !init[34] ) fprintf( stderr, "<MinMagToSend> " );
      if ( !init[35] ) fprintf( stderr, "<ResponseFile> " );
      if ( !init[36] ) fprintf( stderr, "<Iasp91File> " );
      if ( !init[37] ) fprintf( stderr, "<Iasp91TblFile> " );
      if ( !init[38] ) fprintf( stderr, "<ThetaFile> " );
      if ( !init[39] ) fprintf( stderr, "<RedoLineupFile> " );
      if ( !init[41] ) fprintf( stderr, "<FERegionPath> " );
      if ( !init[42] ) fprintf( stderr, "<DistancePath> " );
      if ( !init[43] ) fprintf( stderr, "<DirectionsPath> " );
      if ( !init[44] ) fprintf( stderr, "<CitiesPath> " );
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
   logit( "", "StaFile:            %s\n",    Gparm->StaFile );
   logit( "", "StaDataFile:        %s\n",    Gparm->StaDataFile );
   logit( "", "ResponseFile:       %s\n",    Gparm->ResponseFile );
   logit( "", "InKey:              %d\n",    Gparm->InKey );
   logit( "", "OutKey:             %d\n",    Gparm->OutKey );
   logit( "", "AlarmKey:           %d\n",    Gparm->AlarmKey );
   logit( "", "HeartbeatInt:       %d\n",    Gparm->HeartbeatInt );
   logit( "", "NumPBuffs:          %d\n",    Gparm->NumPBuffs );
   logit( "", "MaxTimeBetweenPicks:%lf\n",   Gparm->MaxTimeBetweenPicks );
   logit( "", "MinPs:              %d\n",    Gparm->MinPs );
   logit( "", "NumNearStn:         %d\n",    Gparm->iNumNearStn );
   logit( "", "MaxDistance:        %lf\n",   Gparm->dMaxDist );
   logit( "", "Debug:              %d\n",    Gparm->Debug );
   logit( "", "MyModId:            %u\n",    Gparm->MyModId );
   logit( "", "BValFile:           %s\n",    Gparm->szBValFile );
   logit( "", "OldQuakes:          %s\n",    Gparm->szOldQuakes );
   logit( "", "AutoLoc:            %s\n",    Gparm->szAutoLoc );
   logit( "", "DummyFile:          %s\n",    Gparm->szDummyFile );
   logit( "", "MapFile:            %s\n",    Gparm->szMapFile );
   logit( "", "RTPFile:            %s\n",    Gparm->szRTPFile );
   logit( "", "MwFile:             %s\n",    Gparm->szMwFile );
   logit( "", "DepthFile  :        %s\n",    Gparm->szDepthDataFile );
   logit( "", "NorthernLat:        %lf\n",   Gparm->NorthernLat );
   logit( "", "SouthernLat:        %lf\n",   Gparm->SouthernLat );
   logit( "", "WesternLon:         %lf\n",   Gparm->WesternLon );
   logit( "", "EasternLon:         %lf\n",   Gparm->EasternLon );
   logit( "", "MinMagToSend:       %lf\n",   Gparm->MinMagToSend );
   logit( "", "CityFileWUC:        %s\n",    Gparm->CityFileWUC );
   logit( "", "CityFileWLC:        %s\n",    Gparm->CityFileWLC );
   logit( "", "CityFileEUC:        %s\n",    Gparm->CityFileEUC );
   logit( "", "CityFileELC:        %s\n",    Gparm->CityFileELC );
   logit( "", "NameFile:           %s\n",    Gparm->szNameFile );
   logit( "", "NameFileLC:         %s\n",    Gparm->szNameFileLC );
   logit( "", "IndexFile:          %s\n",    Gparm->szIndexFile );
   logit( "", "LatFile:            %s\n",    Gparm->szLatFile );
   logit( "", "Iasp91File:         %s\n",    Gparm->szIasp91File );
   logit( "", "Iasp91TblFile:      %s\n",    Gparm->szIasp91TblFile );
   logit( "", "ThetaFile:          %s\n",    Gparm->szThetaFile );
   logit( "", "QLogFile:           %s\n",    Gparm->szQLogFile );
   logit( "", "FERegionPath:       %s\n",    Gparm->szPathRegions );
   logit( "", "DistancePath:       %s\n",    Gparm->szPathDistances );
   logit( "", "DirectionsPath:     %s\n",    Gparm->szPathDirections );
   logit( "", "CitiesPath:         %s\n\n",  Gparm->szPathCities );
   logit( "", "RedoLineupFile:  %d\n",    Gparm->iRedoLineupFile );
   if ( strlen( Gparm->ATPLineupFileBB ) > 2 )
      logit( "", "Loc_ to be used with Player - File: %s\n",
       Gparm->ATPLineupFileBB);
   logit( "", "PFilePath:          %s\n\n",  Gparm->szPFilePath );
   return;
}
