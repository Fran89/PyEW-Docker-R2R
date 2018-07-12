/* THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU CHECKED IT OUT!
 *
 *  $Id: cdur_config.c 490 2009-11-09 19:16:15Z dietz $
 * 
 *  Revision history:
 *   $Log$
 *   Revision 1.1  2009/11/09 19:16:15  dietz
 *   Initial version, may still contain bugs and debugging statements
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kom.h>
#include <transport.h>
#include <earthworm.h>
#include "coda_dur.h"

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

/* Set defaults for optional commands
 ************************************/
   Gparm->mCaav      = MAX_CAAV;
   Gparm->mPick      = MAX_PICK;

/* Set to zero one init flag for each required command
   ***************************************************/
   for ( i = 0; i < ncommand; i++ ) init[i] = 0;
   Gparm->nGetCaLogo = 0;
   Gparm->GetCaLogo  = NULL;
   Gparm->nGetPkLogo = 0;
   Gparm->GetPkLogo  = NULL;
   Gparm->nStaFile   = 0;
   Gparm->StaFile    = NULL;

/* Open the main configuration file
   ********************************/
   nfiles = k_open( config_file );
   if ( nfiles == 0 )
   {
      logit( "e", "coda_dur: Error opening configuration file <%s>\n",
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
               logit( "e", "coda_dur: Error opening command file <%s>.\n",
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
                  logit( "e", "coda_dur: Error reallocing Gparm->StaFile. Exiting.\n" );
                  return -1;
               }
               Gparm->StaFile = tmp;
               strcpy( Gparm->StaFile[Gparm->nStaFile].name, str );
               Gparm->StaFile[Gparm->nStaFile].nsta = 0;
               Gparm->nStaFile++;
            } else {
               logit( "e", "coda_dur: Invalid StaFile name <%s>; must be 1-%d chars. " 
                      "Exiting.\n", str, STAFILE_LEN );
               return -1;
            }
            init[0] = 1;
         }

         else if ( k_its( "CodaAAVRing" ) )
         {
            if ( str = k_str() )
            {
               if( (Gparm->CaKey = GetKey(str)) == -1 )
               {
                  logit( "e", "coda_dur: Invalid CodaAAVRing name <%s>. Exiting.\n", str );
                  return -1;
               }
            }
            init[1] = 1;
         }

         else if ( k_its( "PickRing" ) )
         {
            if ( str = k_str() )
            {
               if( (Gparm->PkKey = GetKey(str)) == -1 )
               {
                  logit( "e", "coda_dur: Invalid PickRing name <%s>. Exiting.\n", str );
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
                  logit( "e", "coda_dur: Invalid MyModId <%s>.\n", str );
                  return -1;
               }
            }
            init[5] = 1;
         }

 /*opt*/ else if ( k_its( "GetCodaAAVLogo" ) )
         {
            MSG_LOGO *tlogo = NULL;
            int       nlogo = Gparm->nGetCaLogo;
            tlogo = (MSG_LOGO *)realloc( Gparm->GetCaLogo, (nlogo+1)*sizeof(MSG_LOGO) );
            if( tlogo == NULL )
            {
               logit( "e", "coda_dur: GetCodaAAVLogo: error reallocing %d bytes.\n",
                      (nlogo+1)*sizeof(MSG_LOGO) );
               return -1;
            }
            Gparm->GetCaLogo = tlogo;
            
            if( str=k_str() )       /* read instid */
            {
               if( GetInst( str, &(Gparm->GetCaLogo[nlogo].instid) ) != 0 ) 
               {
                  logit( "e", "coda_dur: Invalid installation name <%s>"
                         " in <GetCodaAAVLogo> cmd!\n", str );
                  return -1;
               }
               if( str=k_str() )    /* read module id */
               {
                  if( GetModId( str, &(Gparm->GetCaLogo[nlogo].mod) ) != 0 ) 
                  {
                     logit( "e", "coda_dur: Invalid module name <%s>"
                            " in <GetCodaAAVLogo> cmd!\n", str );
                     return -1;
                  }
               /* hardcoded message type */
                  if( GetType( "TYPE_CODA_AAV", &(Gparm->GetCaLogo[nlogo].type) ) != 0 ) 
                  {
                     logit( "e", "coda_dur: Error getting TYPE_CODA_AAV.\n" );
                     return -1;
                  }
                  Gparm->nGetCaLogo++;
               } /* end if modid */  
            } /* end if instid */  
         } /* end GetCodaAAVLogo cmd */

 /*opt*/ else if ( k_its( "GetPickLogo" ) )
         {
            MSG_LOGO *tlogo = NULL;
            int       nlogo = Gparm->nGetPkLogo;
            tlogo = (MSG_LOGO *)realloc( Gparm->GetPkLogo, (nlogo+1)*sizeof(MSG_LOGO) );
            if( tlogo == NULL )
            {
               logit( "e", "coda_dur: GetPickLogo: error reallocing %d bytes.\n",
                      (nlogo+1)*sizeof(MSG_LOGO) );
               return -1;
            }
            Gparm->GetPkLogo = tlogo;
            
            if( str=k_str() )       /* read instid */
            {
               if( GetInst( str, &(Gparm->GetPkLogo[nlogo].instid) ) != 0 ) 
               {
                  logit( "e", "coda_dur: Invalid installation name <%s>"
                         " in <GetPickLogo> cmd!\n", str );
                  return -1;
               }
               if( str=k_str() )    /* read module id */
               {
                  if( GetModId( str, &(Gparm->GetPkLogo[nlogo].mod) ) != 0 ) 
                  {
                     logit( "e", "coda_dur: Invalid module name <%s>"
                            " in <GetPickLogo> cmd!\n", str );
                     return -1;
                  }
               /* hardcoded message type */
                  if( GetType( "TYPE_PICK_SCNL", &(Gparm->GetPkLogo[nlogo].type) ) != 0 ) 
                  {
                     logit( "e", "coda_dur: Error getting TYPE_PICK_SCNL.\n" );
                     return -1;
                  }
                  Gparm->nGetPkLogo++;
               } /* end if modid */  
            } /* end if instid */  
         } /* end GetPickLogo cmd */

 /*opt*/ else if ( k_its( "MaxCodaAAVperSCNL" ) )
         {
            Gparm->mCaav = (unsigned long) k_long();
         }

 /*opt*/ else if ( k_its( "MaxPickperSCNL" ) )
         {
            Gparm->mPick = (unsigned long) k_long();
         }

/* An unknown parameter was encountered
   ************************************/
         else
         {
            logit( "e", "coda_dur: <%s> unknown parameter in <%s>\n",
                    com, config_file );
            continue;
         }

/* See if there were any errors processing the command
   ***************************************************/
         if ( k_err() )
         {
            logit( "e", "coda_dur: Bad <%s> command in <%s>.\n", com,
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
      logit( "e", "coda_dur: ERROR, no " );
      if ( !init[0] ) logit( "e", "<StaFile> "      );
      if ( !init[1] ) logit( "e", "<CodaAAVRing> "  );
      if ( !init[2] ) logit( "e", "<PickRing> "     );
      if ( !init[3] ) logit( "e", "<HeartbeatInt> " );
      if ( !init[4] ) logit( "e", "<Debug> "        );
      if ( !init[5] ) logit( "e", "<MyModId> "      );
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
   logit( "", "CodaAAVRing Key: %6d\n",   Gparm->CaKey );
   logit( "", "PickRing Key:    %6d\n",   Gparm->PkKey );
   logit( "", "HeartbeatInt:    %6d\n",   Gparm->HeartbeatInt );
   logit( "", "Debug:           %6d\n",   Gparm->Debug );
   logit( "", "MyModId:         %6u\n",   Gparm->MyModId );
   logit( "", "nGetCodaAAVLogo: %6d\n",   Gparm->nGetCaLogo );
   for( i=0; i<Gparm->nGetCaLogo; i++ ) {
      logit( "", "GetCodaAAVLogo[%d]:   i%u m%u t%u\n", i,
            Gparm->GetCaLogo[i].instid, Gparm->GetCaLogo[i].mod, 
            Gparm->GetCaLogo[i].type );
   }
   logit( "", "nGetPickLogo:    %6d\n",   Gparm->nGetPkLogo );
   for( i=0; i<Gparm->nGetPkLogo; i++ ) {
      logit( "", "GetPickLogo[%d]:   i%u m%u t%u\n", i,
            Gparm->GetPkLogo[i].instid, Gparm->GetPkLogo[i].mod, 
            Gparm->GetPkLogo[i].type );
   }
   logit( "", "MaxCodaAAVperSCNL: %4d\n",   Gparm->mCaav );
   logit( "", "MaxPickperSCNL:    %4d\n",   Gparm->mPick );

   return;
}

