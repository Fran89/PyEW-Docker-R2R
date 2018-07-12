/***************************************************************************
 *  This code is a part of rayloc_ew / USGS EarthWorm module               *
 *                                                                         *
 *  It is written by ISTI (Instrumental Software Technologies, Inc.)       *
 *          as a part of a contract with CERI USGS.                        *
 * For support contact info@isti.com                                       *
 *   Ilya Dricker (i.dricker@isti.com)                                     *
 *                                                   Aug 2004              *
 ***************************************************************************/

/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: rayloc_ew_config.c 1669 2004-08-05 04:15:11Z friberg $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2004/08/05 04:15:11  friberg
 *     First commit of rayloc_ew in EW-CENTRAL CVS
 *
 *     Revision 1.7  2004/08/04 19:27:54  ilya
 *     Towards version 1.0
 *
 *     Revision 1.6  2004/07/29 21:32:03  ilya
 *     New logging; tests; fixes
 *
 *     Revision 1.5  2004/06/25 14:22:17  ilya
 *     Working version: no output
 *
 *     Revision 1.4  2004/06/24 18:34:20  ilya
 *     Fixed numbering
 *
 *     Revision 1.3  2004/06/24 16:47:05  ilya
 *     Version compiles
 *
 *     Revision 1.2  2004/06/24 16:15:07  ilya
 *     Integration phase started
 *
 *     Revision 1.1.1.1  2004/06/22 21:12:06  ilya
 *     initial import into CVS
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <kom.h>
#include <transport.h>
#include <earthworm.h>
#include "rayloc_ew.h"

#define ncommand 7          /* Number of commands in the config file */


 /***********************************************************************
  *                              GetConfig()                            *
  *             Processes command file using kom.c functions.           *
  *               Returns -1 if any errors are encountered.             *
  ***********************************************************************/

int rayloc_ew_GetConfig( char *config_file, GPARM *Gparm,  RAYLOC_PROC_FLAGS *flags)
{
   char     init[ncommand];     /* Flags, one for each command */
   int      nmiss;              /* Number of commands that were missed */
   int      nfiles;
   int      i;

   /* Set flags defaults */
   memset(flags, 0, sizeof(RAYLOC_PROC_FLAGS));
   memset(Gparm, 0, sizeof(GPARM));
   flags->fix_depth = 1;
   flags->use_PKP = 1;
   flags->use_depth_ph = 1;
   flags->use_S_ph = 1;
   flags->Dmax1 = 180;
   flags->Dmax2 = 180;
   flags->Dmax3 = 180;
   flags->Dmax4 = 180;
   flags->Dmax5 = 180;

/* Set to zero one init flag for each required command
   ***************************************************/
   for ( i = 0; i < ncommand; i++ )
      init[i] = 0;

/* Open the main configuration file
   ********************************/
   nfiles = k_open( config_file );
   if ( nfiles == 0 )
   {
      logit( "pt", "rayloc_ew: Error opening configuration file <%s>\n",
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
               logit( "pt", "pick_ew: Error opening command file <%s>.\n",
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
           {
              strcpy( Gparm->StaFile, str );
              init[0] = 1;
           }
           else
           {
              logit( "pt", "rayloc_ew: Invalid station list name file <%s>. Exiting.\n", str );
              return -1;
           }
         }

         else if ( k_its( "WorkDir" ) )
         {
            if ( str = k_str() )
            {
               strcpy( Gparm->workDirName, str );
               init[1] = 1;
            }
            else
            {
               logit( "pt", "rayloc_ew: Invalid work directory <%s>. Exiting.\n", str );
               return -1;
            }
         }

         else if ( k_its( "InRing" ) )
         {
            if ( str = k_str() )
            {
               if( (Gparm->InKey = GetKey(str)) == -1 )
               {
                 logit( "pt", "rayloc_ew: Invalid InRing name <%s>. Exiting.\n", str );
                  return -1;
               }
            }
            init[2] = 1;
         }

         else if ( k_its( "OutRing" ) )
         {
            if ( str = k_str() )
            {
               if ( (Gparm->OutKey = GetKey(str)) == -1 )
               {
                  logit( "pt", "rayloc_ew: Invalid OutRing name <%s>. Exiting.\n", str );
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

         else if ( k_its( "Debug" ) )
         {
            Gparm->Debug = k_int();
            init[5] = 1;
         }

         else if ( k_its( "MyModId" ) )
         {
            if ( str = k_str() )
            {
               if ( GetModId(str, &Gparm->MyModId) == -1 )
               {
                  logit( "pt", "rayloc_ew: Invalid MyModId <%s>.\n", str );
                  return -1;
               }
            }
            init[6] = 1;
         }

         else if ( k_its( "hold_params" ) )
            flags->hold_params = k_int();

         else if ( k_its( "fix_depth" ) )
            flags->fix_depth = k_int();

         else if ( k_its( "use_PKP" ) )
            flags->use_PKP = k_int();

         else if ( k_its( "use_depth_ph" ) )
            flags->use_depth_ph = k_int();

         else if ( k_its( "use_S_ph" ) )
            flags->use_S_ph = k_int();

         else if ( k_its( "pick_weight_interval" ) )
            flags->pick_weight_interval = k_int();

         else if ( k_its( "Rmin" ) )
            flags->Rmin = k_int();

         else if ( k_its( "Rmax" ) )
            flags->Rmax = k_int();

         else if ( k_its( "D1" ) )
            flags->D1 = k_int();

         else if ( k_its( "Dmin1" ) )
            flags->Dmin1 = k_int();

         else if ( k_its( "Dmax1" ) )
            flags->Dmax2 = k_int();

         else if ( k_its( "D2" ) )
            flags->D2 = k_int();

         else if ( k_its( "Dmin2" ) )
            flags->Dmin2 = k_int();

         else if ( k_its( "Dmax2" ) )
            flags->Dmax2 = k_int();

         else if ( k_its( "D3" ) )
            flags->D3 = k_int();

         else if ( k_its( "Dmin3" ) )
            flags->Dmin3 = k_int();

         else if ( k_its( "Dmax3" ) )
            flags->Dmax3 = k_int();

         else if ( k_its( "D4" ) )
            flags->D4 = k_int();

         else if ( k_its( "Dmin4" ) )
            flags->Dmin4 = k_int();

         else if ( k_its( "Dmax4" ) )
            flags->Dmax4 = k_int();

         else if ( k_its( "D5" ) )
            flags->D5 = k_int();

         else if ( k_its( "Dmin5" ) )
            flags->Dmin5 = k_int();

         else if ( k_its( "Dmax5" ) )
            flags->Dmax5 = k_int();

/* An unknown parameter was encountered
   ************************************/
         else
         {
            logit( "pt", "rayloc_ew: <%s> unknown parameter in <%s>\n",
                    com, config_file );
            continue;
         }

/* See if there were any errors processing the command
   ***************************************************/
         if ( k_err() )
         {
            logit( "pt", "rayloc_ew: Bad <%s> command in <%s>.\n", com,
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
      logit( "pt", "rayloc_ew: ERROR, no " );
      if ( !init[0] ) logit( "pt", "<InRing> " );
      if ( !init[1] ) logit( "pt", "<OutRing> " );
      if ( !init[2] ) logit( "pt", "<HeartbeatInt> " );
      if ( !init[3] ) logit( "pt", "<Debug> " );
      if ( !init[4] ) logit( "pt", "<MyModId> " );
      logit( "pt", "command(s) in <%s>. Exiting.\n", config_file );
      return -1;
   }
   return 0;
}


 /***********************************************************************
  *                              LogConfig()                            *
  *                                                                     *
  *                   Log the configuration parameters                  *
  ***********************************************************************/

void rayloc_ew_LogConfig( GPARM *Gparm )
{
   logit( "", "\n" );
   logit( "pt", "InKey:           %6d\n",   Gparm->InKey );
   logit( "pt", "OutKey:          %6d\n",   Gparm->OutKey );
   logit( "pt", "HeartbeatInt:    %6d\n",   Gparm->HeartbeatInt );
   logit( "pt", "MyModId:         %6u\n\n", Gparm->MyModId );
   return;
}

      /*******************************************************
       *                      GetEwh()                       *
       *                                                     *
       *      Get parameters from the earthworm.h file.      *
       *******************************************************/

int rayloc_ew_GetEwh( EWH *Ewh )
{
   if ( GetLocalInst( &Ewh->MyInstId ) != 0 )
   {
      fprintf( stderr, "pick_ew: Error getting MyInstId.\n" );
      return -1;
   }
   if ( GetInst( "INST_WILDCARD", &Ewh->GetThisInstId ) != 0 )
   {
      fprintf( stderr, "pick_ew: Error getting GetThisInstId.\n" );
      return -2;
   }
   if ( GetModId( "MOD_WILDCARD", &Ewh->GetThisModId ) != 0 )
   {
      fprintf( stderr, "pick_ew: Error getting GetThisModId.\n" );
      return -3;
   }
   if ( GetType( "TYPE_HEARTBEAT", &Ewh->TypeHeartBeat ) != 0 )
   {
      fprintf( stderr, "pick_ew: Error getting TypeHeartbeat.\n" );
      return -4;
   }
   if ( GetType( "TYPE_ERROR", &Ewh->TypeError ) != 0 )
   {
      fprintf( stderr, "pick_ew: Error getting TypeError.\n" );
      return -5;
   }
   if ( GetType( "TYPE_PICK2K", &Ewh->TypePick2k ) != 0 )
   {
      fprintf( stderr, "pick_ew: Error getting TypePick2k.\n" );
      return -6;
   }
   if ( GetType( "TYPE_CODA2K", &Ewh->TypeCoda2k ) != 0 )
   {
      fprintf( stderr, "pick_ew: Error getting TypeCoda2k.\n" );
      return -7;
   }
   if ( GetType( "TYPE_TRACEBUF", &Ewh->TypeWaveform ) != 0 )
   {
      fprintf( stderr, "pick_ew: Error getting TYPE_TRACEBUF.\n" );
      return -8;
   }
   return 0;
}
