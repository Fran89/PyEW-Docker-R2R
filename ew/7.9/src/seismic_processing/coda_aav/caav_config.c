/* THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU CHECKED IT OUT!
 *
 *  $Id: caav_config.c 490 2009-11-09 19:16:15Z dietz $
 * 
 *  Revision history:
 *   $Log$
 *   Revision 1.1  2009/11/09 19:15:28  dietz
 *   Initial version, may still contain bugs and debugging statements
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kom.h>
#include <transport.h>
#include <earthworm.h>
#include "coda_aav.h"

#define ncommand 6          /* Number of commands in the config file */


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
   Gparm->nStaFile = 0;
   Gparm->StaFile  = NULL;

/* Open the main configuration file
   ********************************/
   nfiles = k_open( config_file );
   if ( nfiles == 0 )
   {
      logit( "e", "coda_aav: Error opening configuration file <%s>\n",
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
               logit( "e", "coda_aav: Error opening command file <%s>.\n",
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
               tmp = (STAFILE *)realloc( Gparm->StaFile, 
                                        (Gparm->nStaFile+1)*sizeof(STAFILE) );
               if( tmp == NULL ) 
               {
                  logit( "e", "coda_aav: Error reallocing Gparm->StaFile. Exiting.\n" );
                  return -1;
               }
               Gparm->StaFile = tmp;
               strcpy( Gparm->StaFile[Gparm->nStaFile].name, str );
               Gparm->StaFile[Gparm->nStaFile].nsta = 0;
               Gparm->nStaFile++;
            } else {
               logit( "e", "coda_aav: Invalid StaFile name <%s>; must be 1-%d chars. " 
                      "Exiting.\n", str, STAFILE_LEN );
               return -1;
            }
            init[0] = 1;
         }

         else if ( k_its( "InRing" ) )
         {
            if ( str = k_str() )
            {
               if( (Gparm->InKey = GetKey(str)) == -1 )
               {
                  logit( "e", "coda_aav: Invalid InRing name <%s>. Exiting.\n", str );
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
                  logit( "e", "coda_aav: Invalid OutRing name <%s>. Exiting.\n", str );
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

         else if ( k_its( "Debug" ) )
         {
            Gparm->Debug = k_int();
            init[4] = 1;
         }

         else if ( k_its( "MyModId" ) )
         {
            if ( str = k_str() )
            {
               if ( GetModId(str, &Gparm->MyModId) == -1 )
               {
                  logit( "e", "coda_aav: Invalid MyModId <%s>.\n", str );
                  return -1;
               }
            }
            init[5] = 1;
         }
 
 /*opt*/ else if ( k_its( "GetLogo" ) )
         {
            MSG_LOGO *tlogo = NULL;
            int       nlogo = Gparm->nGetLogo;
            tlogo = (MSG_LOGO *)realloc( Gparm->GetLogo, (nlogo+1)*sizeof(MSG_LOGO) );
            if( tlogo == NULL )
            {
               logit( "e", "coda_aav: GetLogo: error reallocing %d bytes.\n",
                      (nlogo+1)*sizeof(MSG_LOGO) );
               return -1;
            }
            Gparm->GetLogo = tlogo;
            
            if( str=k_str() )       /* read instid */
            {
               if( GetInst( str, &(Gparm->GetLogo[nlogo].instid) ) != 0 ) 
               {
                  logit( "e", "coda_aav: Invalid installation name <%s>"
                         " in <GetLogo> cmd!\n", str );
                  return -1;
               }
               if( str=k_str() )    /* read module id */
               {
                  if( GetModId( str, &(Gparm->GetLogo[nlogo].mod) ) != 0 ) 
                  {
                     logit( "e", "coda_aav: Invalid module name <%s>"
                            " in <GetLogo> cmd!\n", str );
                     return -1;
                  }
                  if( str=k_str() ) /* read message type */
                  {
                     if( strcmp(str,"TYPE_TRACEBUF") !=0 &&
                         strcmp(str,"TYPE_TRACEBUF2")!=0    )
                     {
                        logit( "e","coda_aav: Invalid message type <%s> in <GetLogo>"
                               " cmd; must be TYPE_TRACEBUF or TYPE_TRACEBUF2.\n",
                                str );
                        return -1;
                     }
                     if( GetType( str, &(Gparm->GetLogo[nlogo].type) ) != 0 ) {
                        logit( "e", "coda_aav: Invalid message type <%s>"
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
            logit( "e", "coda_aav: <%s> unknown parameter in <%s>\n",
                    com, config_file );
            continue;
         }

/* See if there were any errors processing the command
   ***************************************************/
         if ( k_err() )
         {
            logit( "e", "coda_aav: Bad <%s> command in <%s>.\n", com,
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
      logit( "e", "coda_aav: ERROR, no " );
      if ( !init[0] ) logit( "e", "<StaFile> " );
      if ( !init[1] ) logit( "e", "<InRing> " );
      if ( !init[2] ) logit( "e", "<OutRing> " );
      if ( !init[3] ) logit( "e", "<HeartbeatInt> " );
      if ( !init[4] ) logit( "e", "<Debug> " );
      if ( !init[5] ) logit( "e", "<MyModId> " );
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

   logit( "", "\n" );
   logit( "", "nStaFile:        %6d\n",   Gparm->nStaFile );
   for( i=0; i<Gparm->nStaFile; i++ ) {
      logit( "", "StaFile[%d]:    %s\n",  i, Gparm->StaFile[i].name );
   }
   logit( "", "InKey:           %6d\n",   Gparm->InKey );
   logit( "", "OutKey:          %6d\n",   Gparm->OutKey );
   logit( "", "HeartbeatInt:    %6d\n",   Gparm->HeartbeatInt );
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

