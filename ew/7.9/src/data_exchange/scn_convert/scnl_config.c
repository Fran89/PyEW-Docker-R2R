/*
 *   This file is managed using Concurrent Versions System (CVS).
 *
 *    $Id: scnl_config.c 6056 2014-03-08 18:42:36Z paulf $
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kom.h>
#include <earthworm.h>
#include "scn_convert.h"

#define ncommand 4           /* Number of commands in the config file */
extern int Debug;

   /*****************************************************************
    *                          GetConfig()                          *
    *         Processes command file using kom.c functions.         *
    *****************************************************************/

void GetConfig( char *config_file, GPARM *Gparm )
{
   char init[ncommand];     /* Flags, one for each command */
   int  nmiss;              /* Number of commands that were missed */
   int  nfiles;
   int  i;
   char     processor[15];

/* Set to zero one init flag for each required command
   ***************************************************/
   for ( i = 0; i < ncommand; i++ ) init[i] = 0;

/* Initialize Configuration parameters
   ***********************************/
   Gparm->MyModName[0]   = '\0';

/* Open the main configuration file
   ********************************/
   nfiles = k_open( config_file );
   if ( nfiles == 0 )
   {
      logit( "e", "Error opening configuration file <%s> Exiting.\n",
              config_file );
      exit( -1 );
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
               logit( "e", "Error opening command file <%s>. Exiting.\n",
                       &com[1] );
               exit( -1 );
            }
            continue;
         }

/* Read configuration parameters
   *****************************/
         else if ( k_its( "InRing" ) )
         {
            if ( (str = k_str() ) != NULL)
            {
               strncpy( Gparm->InRing, str, MAX_RING_STR );
               Gparm->InRing[MAX_RING_STR-1] = '\0';

               if( (Gparm->InKey = GetKey(str)) == -1 )
               {
                  logit( "e", "Invalid InRing name <%s>. Exiting.\n", str );
                  exit( -1 );
               }
            }
	    strcpy( processor, "scnl_config" );
            init[0] = 1;
         }

         else if ( k_its( "OutRing" ) )
         {
            if ( (str = k_str()) != NULL )
            {
               strncpy( Gparm->OutRing, str, MAX_RING_STR );
               Gparm->OutRing[MAX_RING_STR-1] = '\0';

               if( (Gparm->OutKey = GetKey(str)) == -1 )
               {
                  logit( "e", "Invalid InRing name <%s>. Exiting.\n", str );
                  exit( -1 );
               }
            }
	    strcpy( processor, "scnl_config" );
            init[1] = 1;
         }

         else if ( k_its( "HeartbeatInt" ) )
         {
            Gparm->HeartbeatInt = k_int();
            init[2] = 1;
         }

         else if ( k_its( "MyModuleId" ) )
         {
            if ( (str=k_str()) != NULL )
            {
               strncpy( Gparm->MyModName, str, MAX_MOD_STR );
               Gparm->MyModName[MAX_MOD_STR-1] = '\0';
	       init[3] = 1;
	       strcpy( processor, "scnl_config" );
            }
         }

	 /* optional debug */
	 else if ( k_its( "Debug" ) )
	 {
	     Debug = 1;
	 }

	 else if ( s2s_com() ) strcpy( processor, "s2s_config" );

/* An unknown parameter was encountered
   ************************************/
         else
         {
            logit( "e", "<%s> unknown parameter in <%s>\n",
                    com, config_file );
            continue;
         }

/* See if there were any errors processing the command
   ***************************************************/
         if ( k_err() )
         {
            logit( "e", "Bad <%s> command in <%s>. Exiting.\n", com,
                    config_file );
            exit( -1 );
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
      logit( "e", "Error. No " );
      if ( !init[0] ) logit( "e", "<InRing> " );
      if ( !init[1] ) logit( "e", "<OutRing> " );
      if ( !init[2] ) logit( "e", "<HeartbeatInt> " );
      if ( !init[3] ) logit( "e", "<MyModuleId> " );
      logit( "e", "command(s) in <%s>. Exiting.\n", config_file );
      exit( -1 );
   }
   return;
}


   /**************************************************************
    *                         LogConfig()                        *
    *              Log the configuration parameters              *
    **************************************************************/

void LogConfig( GPARM *Gparm, char *version )
{
   logit( "", "Version %s\n", version );
   logit( "", "InRing:               %s\n",   Gparm->InRing );
   logit( "", "OutRing:              %s\n",   Gparm->OutRing );
   logit( "", "HeartbeatInt:        %6d\n",   Gparm->HeartbeatInt );
   logit( "", "MyModName:            %s\n",   Gparm->MyModName );
   logit( "", "\n" );
   return;
}

