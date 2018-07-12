/* 
 * config.c
 * Modified from config.c, Revision 1.6 in src/seismic_processing/pick_ew
 *
 * This file is part of pick_FP, Earthworm module for the FilterPicker phase detector and picker.
 *
 * (C) 2008-2012 Claudio Satriano <satriano@ipgp.fr>,
 * under the same license terms of the Earthworm software system. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kom.h>
#include <transport.h>
#include <earthworm.h>
#include "pick_FP.h"

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
   for ( i = 0; i < ncommand; i++ ) init[i] = 0;
   Gparm->nGetLogo = 0;
   Gparm->GetLogo  = NULL;
   Gparm->PickIndexDir  = NULL; /* optional directory for pick index placement */
   Gparm->NoCoda = 0;		/* off by default, always calculate coda's */
   Gparm->nStaFile = 0;
   Gparm->StaFile  = NULL;
   // default values for the weight table:
   Gparm->WeightTable[0] = 0.02;
   Gparm->WeightTable[1] = 0.05;
   Gparm->WeightTable[2] = 0.50;
   Gparm->WeightTable[3] = 1.00;

/* Open the main configuration file
   ********************************/
   nfiles = k_open( config_file );
   if ( nfiles == 0 )
   {
      logit( "e", "pick_FP: Error opening configuration file <%s>\n",
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
               logit( "e", "pick_FP: Error opening command file <%s>.\n",
                        &com[1] );
               return -1;
            }
            continue;
         }

/* Read configuration parameters
   *****************************/
         else if ( k_its( "StaFile" ) )
         {
            str = k_str();
            if ( str!=NULL  &&  strlen(str)<STAFILE_LEN ) 
            {
               STAFILE *tmp;
               tmp = (STAFILE *)realloc( Gparm->StaFile, (Gparm->nStaFile+1)*sizeof(STAFILE) );
               if( tmp == NULL ) 
               {
                  logit( "e", "pick_FP: Error reallocing Gparm->StaFile. Exiting.\n" );
                  return -1;
               }
               Gparm->StaFile = tmp;
               strcpy( Gparm->StaFile[Gparm->nStaFile].name, str );
               Gparm->StaFile[Gparm->nStaFile].nsta = 0;
               Gparm->nStaFile++;
            } else {
               logit( "e", "pick_FP: Invalid StaFile name <%s>; must be 1-%d chars. " 
                      "Exiting.\n", str, STAFILE_LEN );
               return -1;
            }
            init[0] = 1;
         }

         else if ( k_its( "InRing" ) )
         {
            if ( (str = k_str()) != NULL )
            {
               if( (Gparm->InKey = GetKey(str)) == -1 )
               {
                  logit( "e", "pick_FP: Invalid InRing name <%s>. Exiting.\n", str );
                  return -1;
               }
            }
            init[1] = 1;
         }

         else if ( k_its( "OutRing" ) )
         {
            if ( (str = k_str()) != NULL )
            {
               if ( (Gparm->OutKey = GetKey(str)) == -1 )
               {
                  logit( "e", "pick_FP: Invalid OutRing name <%s>. Exiting.\n", str );
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
            if ( (str = k_str()) != NULL )
            {
               if ( GetModId(str, &Gparm->MyModId) == -1 )
               {
                  logit( "e", "pick_FP: Invalid MyModId <%s>.\n", str );
                  return -1;
               }
            }
            init[7] = 1;
         }
 /*opt*/ else if ( k_its( "NoCoda" ) )
         {
            Gparm->NoCoda = k_int();
         }
 /*opt*/ else if ( k_its( "PickIndexDir" ) )
         {
            Gparm->PickIndexDir = strdup(k_str());
         }
 /*opt*/ else if ( k_its( "WeightTable" ) )
         {
            Gparm->WeightTable[0] = (float) k_val();
            Gparm->WeightTable[1] = (float) k_val();
            Gparm->WeightTable[2] = (float) k_val();
            Gparm->WeightTable[3] = (float) k_val();
         }
 /*opt*/ else if ( k_its( "GetLogo" ) )
         {
            MSG_LOGO *tlogo = NULL;
            int       nlogo = Gparm->nGetLogo;
            tlogo = (MSG_LOGO *)realloc( Gparm->GetLogo, (nlogo+1)*sizeof(MSG_LOGO) );
            if( tlogo == NULL )
            {
               logit( "e", "pick_FP: GetLogo: error reallocing %d bytes.\n",
                      (nlogo+1)*sizeof(MSG_LOGO) );
               return -1;
            }
            Gparm->GetLogo = tlogo;
            
            if( (str=k_str()) != NULL )       /* read instid */
            {
               if( GetInst( str, &(Gparm->GetLogo[nlogo].instid) ) != 0 ) 
               {
                  logit( "e", "pick_FP: Invalid installation name <%s>"
                         " in <GetLogo> cmd!\n", str );
                  return -1;
               }
               if( (str=k_str()) != NULL )    /* read module id */
               {
                  if( GetModId( str, &(Gparm->GetLogo[nlogo].mod) ) != 0 ) 
                  {
                     logit( "e", "pick_FP: Invalid module name <%s>"
                            " in <GetLogo> cmd!\n", str );
                     return -1;
                  }
                  if( (str=k_str()) != NULL ) /* read message type */
                  {
                     if( strcmp(str,"TYPE_TRACEBUF") !=0 &&
                         strcmp(str,"TYPE_TRACEBUF2")!=0    )
                     {
                        logit( "e","pick_FP: Invalid message type <%s> in <GetLogo>"
                               " cmd; must be TYPE_TRACEBUF or TYPE_TRACEBUF2.\n",
                                str );
                        return -1;
                     }
                     if( GetType( str, &(Gparm->GetLogo[nlogo].type) ) != 0 ) {
                        logit( "e", "pick_FP: Invalid message type <%s>"
                               " in <GetLogo> cmd!\n", str );
                        return -1;
                     }
                     Gparm->nGetLogo++;
                  } /* end if msgtype */
               } /* end if modid */  
            } /* end if instid */  
         } /* end GetLogo cmd */

/* An unknown parameter was encountered
   ************************************/
         else
         {
            logit( "e", "pick_FP: <%s> unknown parameter in <%s>\n",
                    com, config_file );
            continue;
         }

/* See if there were any errors processing the command
   ***************************************************/
         if ( k_err() )
         {
            logit( "e", "pick_FP: Bad <%s> command in <%s>.\n", com,
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
      logit( "e", "pick_FP: ERROR, no " );
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
   int i;
   char wtable[100];

   logit( "", "\n" );
   logit( "", "nStaFile:        %6d\n",   Gparm->nStaFile );
   for( i=0; i<Gparm->nStaFile; i++ ) {
      logit( "", "StaFile[%d]:    %s\n",  i, Gparm->StaFile[i].name );
   }
   logit( "", "InKey:           %6d\n",   Gparm->InKey );
   logit( "", "OutKey:          %6d\n",   Gparm->OutKey );
   logit( "", "HeartbeatInt:    %6d\n",   Gparm->HeartbeatInt );
   logit( "", "RestartLength:   %6d\n",   Gparm->RestartLength );
   logit( "", "MaxGap:          %6d\n",   Gparm->MaxGap );

   wtable[0] = '\0';
   for( i=0; i<=3; i++) {
      sprintf(wtable, "%s %.3f", wtable, Gparm->WeightTable[i]);
   }
   logit( "", "WeightTable:     %s\n", wtable); 

   logit( "", "Debug:           %6d\n",   Gparm->Debug );
   logit( "", "MyModId:         %6u\n",   Gparm->MyModId );
   logit( "", "nGetLogo:        %6d\n",   Gparm->nGetLogo );
   for( i=0; i<Gparm->nGetLogo; i++ ) {
      logit( "", "GetLogo[%d]:   i%u m%u t%u\n", i,
            Gparm->GetLogo[i].instid, Gparm->GetLogo[i].mod, 
            Gparm->GetLogo[i].type );
   }

   return;
}

