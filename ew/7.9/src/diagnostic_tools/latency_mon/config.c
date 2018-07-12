#include <stdio.h>
#include <string.h>
#include <kom.h>
#include <transport.h>
#include <earthworm.h>
#include "latency_mon.h"

#define ncommand 7          /* Number of commands in the config file */


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

/* Initialize the two types of station file name arrays
   (we had better get only one of these)
   ****************************************************/
   Gparm->FindWaveFile[0]='0';
   Gparm->StaFile[0]='0';

   /* default file row count should yield file size apx = 200K */
   Gparm->FileRowSize = 10000;

/* Set to zero one init flag for each required command
   ***************************************************/
   for ( i=0; i<ncommand; i++ ) init[i] = 0;

/* Open the main configuration file
   ********************************/
   nfiles = k_open( config_file );
   if ( nfiles == 0 )
   {
      fprintf( stderr, "latency_mon: Error opening configuration file <%s>\n",
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
               fprintf( stderr, "latency_mon: Error opening command file <%s>."
			            "\n", &com[1] );
               return -1;
            }
            continue;
         }

/* Read configuration parameters
   *****************************/
         else if ( k_its( "FindWaveFile" ) )
         {
            if ( str = k_str() )
               strcpy( Gparm->FindWaveFile, str );
         }

         else if ( k_its( "StaFile" ) )
         {
            if ( str = k_str() )
               strcpy( Gparm->StaFile, str );
         }

         else if ( k_its( "StaDataFile" ) )
         {
            if ( str = k_str() )
               strcpy( Gparm->StaDataFile, str );
         }

         else if ( k_its( "InRing" ) )
         {
            if ( str = k_str() )
            {
               if( (Gparm->InKey = GetKey(str)) == -1 )
               {
                  fprintf( stderr, "latency_mon: Invalid InRing name <%s>. "
                                   "Exiting.\n", str );
                  return -1;
               }
            }
            init[0] = 1;
         }

         else if ( k_its( "HeartbeatInt" ) )
         {
            Gparm->HeartbeatInt = k_int();
            init[1] = 1;
         }

         else if ( k_its( "Debug" ) )
         {
            Gparm->Debug = k_int();
            init[2] = 1;
         }

         else if ( k_its( "MyModId" ) )
         {
            if ( str = k_str() )
            {
               if ( GetModId(str, &Gparm->MyModId) == -1 )
               {
                  fprintf( stderr, "latency_mon: Invalid MyModId <%s>.\n", str);
                  return -1;
               }
            }
            init[3] = 1;
         }

         else if ( k_its( "LogPath" ) )
         {
            if ( str = k_str() )
               strcpy( Gparm->LogPath, str );
            init[4] = 1;
         }

         else if ( k_its( "PrinterPath" ) )
         {
            if ( str = k_str() )
               strcpy( Gparm->PrinterPath, str );
            init[5] = 1;
         }

         else if ( k_its( "NumTracePerScreen" ) )
         {
            Gparm->NumTracePerScreen = k_int();
            init[6] = 1;
         }

         else if ( k_its("FileRowSize") )
         {
            Gparm->FileRowSize = k_int();
         }

         else
         {
/* An unknown parameter was encountered
   ************************************/
            fprintf( stderr, "latency_mon: <%s> unknown parameter in <%s>\n",
                     com, config_file );
            continue;
         }

/* See if there were any errors processing the command
   ***************************************************/
         if ( k_err() )
         {
            fprintf( stderr, "latency_mon: Bad <%s> command in <%s>.\n", com,
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
      if ( !init[i] ) nmiss++;

   if ( 0 < nmiss )
   {
      fprintf( stderr, "latency_mon: ERROR, no " );
      if ( !init[0] ) fprintf( stderr, "<InRing> " );
      if ( !init[1] ) fprintf( stderr, "<HeartbeatInt> " );
      if ( !init[2] ) fprintf( stderr, "<Debug> " );
      if ( !init[3] ) fprintf( stderr, "<MyModId> " );
      if ( !init[4] ) fprintf( stderr, "<LogPath> " );
      if ( !init[5] ) fprintf( stderr, "<PrinterPath> " );
      if ( !init[6] ) fprintf( stderr, "<NumTracePerScreen> " );
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
   logit( "", "StaFile:          %s\n",    Gparm->StaFile );
   logit( "", "StaDataFile:      %s\n",    Gparm->StaDataFile );
   logit( "", "InKey:            %d\n",    Gparm->InKey );
   logit( "", "HeartbeatInt:     %d\n",    Gparm->HeartbeatInt );
   logit( "", "Debug:            %d\n",    Gparm->Debug );
   logit( "", "MyModId:          %u\n",    Gparm->MyModId );
   logit( "", "NumTracePerScreen %ld\n",   Gparm->NumTracePerScreen );
   logit( "", "FileRowSize       %d\n",    Gparm->FileRowSize );
   logit( "", "LogPath:          %s\n",    Gparm->LogPath );
   logit( "", "PrinterPath:      %s\n\n",  Gparm->PrinterPath );
   return;
}
