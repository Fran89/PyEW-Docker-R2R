
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: config.c,v 1.2 2002/05/16 17:00:07 patton Exp $
 *
 *    Revision history:
 *     $Log: config.c,v $
 *     Revision 1.2  2002/05/16 17:00:07  patton
 *     Made logit changes.
 *
 *     Revision 1.1  2000/02/14 19:06:49  lucky
 *     Initial revision
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <kom.h>
#include <transport.h>
#include <earthworm.h>
#include "pick_ew.h"

#define ncommand 8          /* Number of commands in the config file */


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
   for ( i = 0; i < ncommand; i++ )
      init[i] = 0;

/* Open the main configuration file
   ********************************/
   nfiles = k_open( config_file );
   if ( nfiles == 0 )
   {
      logit( "e", "pick_ew: Error opening configuration file <%s>\n",
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
               logit( "e", "pick_ew: Error opening command file <%s>.\n",
                        &com[1] );
               return -1;
            }
            continue;
         }

/* Read configuration parameters
   *****************************/
         else if ( k_its( "StaFile" ) )
         {
            if ( str = k_str() )
               strcpy( Gparm->StaFile, str );
            init[0] = 1;
         }

         else if ( k_its( "InRing" ) )
         {
            if ( str = k_str() )
            {
               if( (Gparm->InKey = GetKey(str)) == -1 )
               {
                  logit( "e", "pick_ew: Invalid InRing name <%s>. Exiting.\n", str );
                  return -1;
               }
            }
            init[1] = 1;
         }

         else if ( k_its( "OutRing" ) )
         {
            if ( str = k_str() )
            {
               if ( (Gparm->OutKey = GetKey(str)) == -1 )
               {
                  logit( "e", "pick_ew: Invalid OutRing name <%s>. Exiting.\n", str );
                  return -1;
               }
            }
            init[2] = 1;
         }

         else if ( k_its( "HeartbeatInt" ) )
         {
            Gparm->HeartbeatInt = k_int();
            init[3] = 1;
         }

         else if ( k_its( "RestartLength" ) )
         {
            Gparm->RestartLength = k_int();
            init[4] = 1;
         }

         else if ( k_its( "MaxGap" ) )
         {
            Gparm->MaxGap = k_int();
            init[5] = 1;
         }

         else if ( k_its( "Debug" ) )
         {
            Gparm->Debug = k_int();
            init[6] = 1;
         }

         else if ( k_its( "MyModId" ) )
         {
            if ( str = k_str() )
            {
               if ( GetModId(str, &Gparm->MyModId) == -1 )
               {
                  logit( "e", "pick_ew: Invalid MyModId <%s>.\n", str );
                  return -1;
               }
            }
            init[7] = 1;
         }

/* An unknown parameter was encountered
   ************************************/
         else
         {
            logit( "e", "pick_ew: <%s> unknown parameter in <%s>\n",
                    com, config_file );
            continue;
         }

/* See if there were any errors processing the command
   ***************************************************/
         if ( k_err() )
         {
            logit( "e", "pick_ew: Bad <%s> command in <%s>.\n", com,
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
      logit( "e", "pick_ew: ERROR, no " );
      if ( !init[0] ) logit( "e", "<StaFile> " );
      if ( !init[1] ) logit( "e", "<InRing> " );
      if ( !init[2] ) logit( "e", "<OutRing> " );
      if ( !init[3] ) logit( "e", "<HeartbeatInt> " );
      if ( !init[4] ) logit( "e", "<RestartLength> " );
      if ( !init[5] ) logit( "e", "<MaxGap> " );
      if ( !init[6] ) logit( "e", "<Debug> " );
      if ( !init[7] ) logit( "e", "<MyModId> " );
      logit( "e", "command(s) in <%s>. Exiting.\n", config_file );
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
   logit( "", "InKey:           %6d\n",   Gparm->InKey );
   logit( "", "OutKey:          %6d\n",   Gparm->OutKey );
   logit( "", "HeartbeatInt:    %6d\n",   Gparm->HeartbeatInt );
   logit( "", "RestartLength:   %6d\n",   Gparm->RestartLength );
   logit( "", "MaxGap:          %6d\n",   Gparm->MaxGap );
   logit( "", "Debug:           %6d\n",   Gparm->Debug );
   logit( "", "MyModId:         %6u\n\n", Gparm->MyModId );
   return;
}

