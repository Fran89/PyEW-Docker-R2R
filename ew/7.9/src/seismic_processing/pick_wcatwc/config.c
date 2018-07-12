#include <stdio.h>
#include <string.h>
#include <kom.h>
#include <transport.h>
#include <earthworm.h>
#include "pick_wcatwc.h"

#define ncommand 28          /* Number of commands in the config file */

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
      fprintf( stderr, "pick_wcatwc: Error opening configuration file <%s>\n",
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
               fprintf( stderr, "pick_wcatwc: Error opening command file <%s>."
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
               strcpy( Gparm->StaFile, str );
            init[0] = 1;
         }
		 
         else if ( k_its( "StaDataFile" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->StaDataFile, str );
            init[1] = 1;
         }
		 
         else if ( k_its( "ResponseFile" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->ResponseFile, str );
            init[24] = 1;
         }

         else if ( k_its( "InRing" ) )
         {
            if ( (str = k_str()) != NULL )
            {
               if( (Gparm->InKey = GetKey(str)) == -1 )
               {
                  fprintf( stderr, "pick_wcatwc: Invalid InRing name <%s>. "
                                   "Exiting.\n", str );
                  return -1;
               }
            }
            init[2] = 1;
         }

         else if ( k_its( "OutRing" ) )
         {
            if ( (str = k_str()) != NULL )
            {
               if ( (Gparm->OutKey = GetKey(str)) == -1 )
               {
                  fprintf( stderr, "pick_wcatwc: Invalid OutRing name <%s>. "
                                   "Exiting.\n", str );
                  return -1;
               }
            }
            init[3] = 1;
         }

         else if ( k_its( "AlarmRing" ) )
         {
            if ( (str = k_str()) != NULL )
            {
               if ( (Gparm->AlarmKey = GetKey(str)) == -1 )
               {
                  fprintf( stderr, "pick_wcatwc: Invalid AlarmRing name <%s>. "
                                   "Exiting.\n", str );
                  return -1;
               }
            }
            init[4] = 1;
         }

         else if ( k_its( "HeartbeatInt" ) )
         {
            Gparm->HeartbeatInt = k_int();
            init[5] = 1;
         }

         else if ( k_its( "MaxGap" ) )
         {
            Gparm->MaxGap = k_int();
            init[6] = 1;
         }

         else if ( k_its( "LowCutFilter" ) )
         {
            Gparm->LowCutFilter = k_val();
            init[7] = 1;
         }

         else if ( k_its( "HighCutFilter" ) )
         {
            Gparm->HighCutFilter = k_val();
            init[8] = 1;
         }

         else if ( k_its( "LTASeconds" ) )
         {
            Gparm->LTASeconds = k_val();
            init[9] = 1;
         }

         else if ( k_its( "MinFreq" ) )
         {
            Gparm->MinFreq = k_val();
            init[10] = 1;
         }

         else if ( k_its( "MinFLoc" ) )
         {
            Gparm->dMinFLoc = k_val();
            init[22] = 1;
         }

         else if ( k_its( "SNLocal" ) )
         {
            Gparm->dSNLocal = k_val();
            init[23] = 1;
         }

         else if ( k_its( "MbCycles" ) )
         {
            Gparm->MbCycles = k_int();
            init[11] = 1;
         }

         else if ( k_its( "LGSeconds" ) )
         {
            Gparm->LGSeconds = k_int();
            init[12] = 1;
         }

         else if ( k_its( "MwpSeconds" ) )
         {
            Gparm->MwpSeconds = k_int();
            init[13] = 1;
         }

         else if ( k_its( "MwpSigNoise" ) )
         {
            Gparm->MwpSigNoise = k_val();
            init[14] = 1;
         }

         else if ( k_its( "AlarmTimeout" ) )
         {
            Gparm->AlarmTimeout = k_val();
            init[15] = 1;
         }

         else if ( k_its( "Debug" ) )
         {
            Gparm->Debug = k_int();
            init[16] = 1;
         }

         else if ( k_its( "MyModId" ) )
         {
            if ( (str = k_str()) != NULL )
            {
               if ( GetModId(str, &Gparm->MyModId) == -1 )
               {
                  fprintf( stderr, "pick_wcatwc: Invalid MyModId <%s>.\n", str);
                  return -1;
               }
            }
            init[17] = 1;
         }

         else if ( k_its( "AlarmTime" ) )
         {
            Gparm->AlarmTime = k_int();
            init[18] = 1;
         }

         else if ( k_its( "AlarmOn" ) )
         {
            Gparm->AlarmOn = k_int();
            init[19] = 1;
         }

         else if ( k_its( "TwoStnAlarmOn" ) )
         {
            Gparm->TwoStnAlarmOn = k_int();
            init[20] = 1;
         }
		 
         else if ( k_its( "TwoStnAlarmFile" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->TwoStnAlarmFile, str );
            init[21] = 1;
         }

         else if ( k_its( "NeuralNet" ) )
         {
            Gparm->NeuralNet = k_int();
            init[25] = 1;
         }

         else if ( k_its( "RedoLineupFile" ) )
         {
            Gparm->iRedoLineupFile = k_int();
            init[26] = 1;
         }

         else if ( k_its( "PickOnAcc" ) )
         {
            Gparm->PickOnAcc = k_int();
            init[27] = 1;
         }
	 
/* Optional command when run with ATPlayer
  ****************************************/	 
         else if ( k_its( "ATPLineupFileBB" ) )
         {
            if ( (str = k_str()) != NULL )
               strcpy( Gparm->ATPLineupFileBB, str );
         }		 

/* An unknown parameter was encountered
   ************************************/
         else
         {
            fprintf( stderr, "pick_wcatwc: <%s> unknown parameter in <%s>\n",
                    com, config_file );
            continue;
         }

/* See if there were any errors processing the command
   ***************************************************/
         if ( k_err() )
         {
            fprintf( stderr, "pick_wcatwc: Bad <%s> command in <%s>.\n", com,
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
      fprintf( stderr, "pick_wcatwc: ERROR, no " );
      if ( !init[0] ) fprintf( stderr, "<StaFile> " );
      if ( !init[1] ) fprintf( stderr, "<StaDataFile> " );
      if ( !init[2] ) fprintf( stderr, "<InRing> " );
      if ( !init[3] ) fprintf( stderr, "<OutRing> " );
      if ( !init[4] ) fprintf( stderr, "<AlarmRing> " );
      if ( !init[5] ) fprintf( stderr, "<HeartbeatInt> " );
      if ( !init[6] ) fprintf( stderr, "<MaxGap> " );
      if ( !init[7] ) fprintf( stderr, "<LowCutFilter> " );
      if ( !init[8] ) fprintf( stderr, "<HighCutFilter> " );
      if ( !init[9] ) fprintf( stderr, "<LTASeconds> " );
      if ( !init[10] ) fprintf( stderr, "<MinFreq> " );
      if ( !init[11] ) fprintf( stderr, "<MbCycles> " );
      if ( !init[12] ) fprintf( stderr, "<LGSeconds> " );
      if ( !init[13] ) fprintf( stderr, "<MwpSeconds> " );
      if ( !init[14] ) fprintf( stderr, "<MwpSigNoise> " );
      if ( !init[15] ) fprintf( stderr, "<AlarmTimeout> " );
      if ( !init[16] ) fprintf( stderr, "<Debug> " );
      if ( !init[17] ) fprintf( stderr, "<MyModId> " );
      if ( !init[18] ) fprintf( stderr, "<AlarmTime> " );
      if ( !init[19] ) fprintf( stderr, "<AlarmOn> " );
      if ( !init[20] ) fprintf( stderr, "<TwoStnAlarmOn> " );
      if ( !init[21] ) fprintf( stderr, "<TwoStnAlarmFile> " );
      if ( !init[22] ) fprintf( stderr, "<MinFLoc> " );
      if ( !init[23] ) fprintf( stderr, "<SNLocal> " );
      if ( !init[24] ) fprintf( stderr, "<ResponseFile> " );
      if ( !init[25] ) fprintf( stderr, "<NeuralNet> " );
      if ( !init[26] ) fprintf( stderr, "<RedoLineupFile> " );
      if ( !init[27] ) fprintf( stderr, "<PickOnAcc> " );
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
   logit( "", "StaFile:       %s\n",      Gparm->StaFile );
   logit( "", "StaDataFile:   %s\n",      Gparm->StaDataFile );
   logit( "", "ResponseFile:  %s\n",      Gparm->ResponseFile );
   logit( "", "InKey:           %d\n",    Gparm->InKey );
   logit( "", "OutKey:          %d\n",    Gparm->OutKey );
   logit( "", "AlarmKey:        %d\n",    Gparm->AlarmKey );
   logit( "", "HeartbeatInt:    %d\n",    Gparm->HeartbeatInt );
   logit( "", "MaxGap:          %d\n",    Gparm->MaxGap );
   logit( "", "LowCutFilter:    %lf\n",   Gparm->LowCutFilter );
   logit( "", "HighCutFilter:   %lf\n",   Gparm->HighCutFilter );
   logit( "", "LTASeconds:      %lf\n",   Gparm->LTASeconds );
   logit( "", "MinFreq:         %lf\n",   Gparm->MinFreq );
   logit( "", "MinFLoc:         %lf\n",   Gparm->dMinFLoc );
   logit( "", "SNLocal:         %lf\n",   Gparm->dSNLocal );
   logit( "", "MbCycles:        %d\n",    Gparm->MbCycles );
   logit( "", "LGSeconds:       %d\n",    Gparm->LGSeconds );
   logit( "", "MwpSeconds:      %d\n",    Gparm->MwpSeconds );
   logit( "", "MwpSigNoise:     %lf\n",   Gparm->MwpSigNoise );
   logit( "", "AlarmTimeout:    %lf\n",   Gparm->AlarmTimeout );
   logit( "", "Debug:           %d\n",    Gparm->Debug );
   logit( "", "AlarmTime:       %d\n",    Gparm->AlarmTime );
   logit( "", "AlarmOn:         %d\n",    Gparm->AlarmOn );
   logit( "", "NeuralNet:       %d\n",    Gparm->NeuralNet );
   logit( "", "PickOnAcc:       %d\n",    Gparm->PickOnAcc );
   logit( "", "TwoStnAlarmOn:   %d\n",    Gparm->TwoStnAlarmOn );
   logit( "", "TwoStnAlarmFile: %s\n",    Gparm->TwoStnAlarmFile );
   logit( "", "RedoLineupFile:  %d\n",    Gparm->iRedoLineupFile );
   if ( strlen( Gparm->ATPLineupFileBB ) > 2 )
      logit( "", "Pick_ to be used with Player - File: %s\n",
       Gparm->ATPLineupFileBB);
   logit( "", "MyModId:         %u\n\n",  Gparm->MyModId );
   return;
}
