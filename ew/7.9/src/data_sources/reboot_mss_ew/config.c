
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: config.c 3762 2010-01-02 00:39:51Z kress $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.5  2010/01/02 00:39:51  kress
 *     bookeeping on gcf2ew.  reboot_mss and reboot_mss_ew now both work in linux.
 *
 *     Revision 1.4  2004/06/25 18:27:27  dietz
 *     modified to work with TYPE_TRACEBUF2 and location code
 *
 *     Revision 1.3  2002/06/11 14:31:22  patton
 *     Made logit changes.
 *
 *     Revision 1.2  2001/04/27 00:54:06  kohler
 *     New parameter: Logout
 *
 *     Revision 1.1  2001/04/26 17:48:42  kohler
 *     Initial revision
 *
 *
 *
 */
#include <stdio.h>
#include <string.h>
#include <kom.h>
#include <transport.h>
#include <earthworm.h>
#include "reboot_mss_ew.h"

#define ncommand 6           /* Number of commands in the config file */


int CountSCNL( char *config_file )
{
   int nSCNL = 0;
   int nfiles;

/* Open the main configuration file
   ********************************/
   nfiles = k_open( config_file );
   if ( nfiles == 0 )
   {
      logit("e", "reboot_mss_ew: Error opening configuration file <%s>\n",
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
               logit("e", "reboot_mss_ew: Error opening command file <%s>.\n",
                       &com[1] );
               return -1;
            }
            continue;
         }

/* Read configuration parameters
   *****************************/
         else if ( k_its( "SCNL" ) ) nSCNL++;   /* new command name */
         else if ( k_its( "Scn"  ) ) nSCNL++;   /* old command name */

/* See if there were any errors processing the command
   ***************************************************/
         if ( k_err() )
         {
            logit("e", "reboot_mss_ew: Bad <%s> command in <%s>.\n", com,
                    config_file );
            return -1;
         }
      }
      nfiles = k_close();
   }
   return nSCNL;
}


 /*******************************************************************
  *                            GetConfig()                          *
  *           Processes command file using kom.c functions.         *
  *             Returns -1 if any errors are encountered.           *
  *******************************************************************/

int GetConfig( char *config_file, GPARM *Gparm, SCNL **pscnl ) {
  char     init[ncommand];     /* Flags, one for each command */
  int      nmiss;              /* Number of commands that were missed */
  int      nfiles;
  int      i;
  int      nSCNL = 0;
  SCNL      *scnl;
  
  /* Count the number of SCNL lines in the config file
************************************************/
  Gparm->nSCNL = CountSCNL( config_file );
  if ( Gparm->nSCNL < 0 ){
    logit("e", "reboot_mss_ew: Error counting SCNL lines in config file\n" );
    return -1;
  }
  else if ( Gparm->nSCNL == 0 ){
    logit("e", "reboot_mss_ew: No SCNL lines in the config file.\n" );
    return -1;
  }

  /* Allocate the SCNL buffers
************************/
  scnl = (SCNL *) malloc( Gparm->nSCNL * sizeof(SCNL) );
  if ( scnl == NULL ) {
    logit("e", "reboot_mss_ew: Can't allocate the scnl buffers.\n" );
    return -1;
  }

  /* Set to zero one init flag for each required command
***************************************************/
  for ( i = 0; i < ncommand; i++ ) init[i] = 0;

  /* Open the main configuration file
********************************/
  nfiles = k_open( config_file );
  if ( nfiles == 0 ){
    logit("e", "reboot_mss_ew: Error opening configuration file <%s>\n",
	  config_file );
    return -1;
  }

  /* Process all nested configuration files
**************************************/
  while ( nfiles > 0 ) {         /* While there are config files open */
    while ( k_rd() ) {           /* Read next line from active file  */
      int  success;
      char *com;
      char *str;
      
      com = k_str();          /* Get the first token from line */
      
      if ( !com ) continue;             /* Ignore blank lines */
      if ( com[0] == '#' ) continue;    /* Ignore comments */
      
      /* Open another configuration file
*******************************/
      if ( com[0] == '@' ) {
	success = nfiles + 1;
	nfiles  = k_open( &com[1] );
	if ( nfiles != success ){
	  logit("e", "reboot_mss_ew: Error opening command file <%s>.\n",
		&com[1] );
	  return -1;
	}
	continue;
      }

      /* Read configuration parameters
*****************************/
      else if ( k_its( "InRing" ) ){
	if ( str = k_str() ){
	  strcpy( Gparm->InRing, str );
	  
	  if( (Gparm->InKey = GetKey(str)) == -1 ){
	    logit("e", "reboot_mss_ew: Invalid InRing name <%s>. Exiting.\n", str );
	    return -1;
	  }
	}
	init[0] = 1;
      }

      else if ( k_its( "HeartbeatInt" ) ) {
	Gparm->HeartbeatInt = k_int();
	init[1] = 1;
      }

      else if ( k_its( "RebootGap" ) ) {
	Gparm->RebootGap = k_int();
	init[2] = 1;
      }

      else if ( k_its( "MyModuleId" ) ) {
	if ( str=k_str() )
	  strcpy( Gparm->MyModName, str );
	init[3] = 1;
      }

      else if ( k_its( "ProgName" ) ) {
	if ( str=k_str() )
	  strcpy( Gparm->ProgName, str );
	init[4] = 1;
      }

      else if ( k_its( "Logout" ) ) {
	Gparm->Logout = k_int();
	init[5] = 1;
      }

      else if ( k_its( "SCNL" ) ) { /* new command */
	char *pwd2;
	strcpy( scnl[nSCNL].sta,     k_str() );
	strcpy( scnl[nSCNL].comp,    k_str() );
	strcpy( scnl[nSCNL].net,     k_str() );
	strcpy( scnl[nSCNL].loc,     k_str() );
	strcpy( scnl[nSCNL].mss_ip,  k_str() );
	strcpy( scnl[nSCNL].mss_apwd,k_str() );
	pwd2=k_str();
	if (pwd2==(char *)0){ /*access password defaults to factory default "access"*/
	  strcpy(scnl[nSCNL].mss_ppwd,scnl[nSCNL].mss_apwd);
	  strcpy(scnl[nSCNL].mss_apwd, "access" );
	} else {
	  strcpy(scnl[nSCNL].mss_ppwd, pwd2 );
	}	  
	nSCNL++;
      }
 
      else if ( k_its( "Scn" ) ) {  /* old command */
	char *pwd2;
	strcpy( scnl[nSCNL].sta,     k_str() );
	strcpy( scnl[nSCNL].comp,    k_str() );
	strcpy( scnl[nSCNL].net,     k_str() );
	strcpy( scnl[nSCNL].loc,     LOC_NULL_STRING );
	strcpy( scnl[nSCNL].mss_ip,  k_str() );
	strcpy( scnl[nSCNL].mss_apwd,k_str() );
	pwd2=k_str();
	if (pwd2==(char *)0){ /*access password defaults to factory default "access"*/
	  strcpy(scnl[nSCNL].mss_ppwd,scnl[nSCNL].mss_apwd);
	  strcpy(scnl[nSCNL].mss_apwd, "access" );
	} else {
	  strcpy(scnl[nSCNL].mss_ppwd, pwd2 );
	}	  
	nSCNL++;
      }


      /* An unknown parameter was encountered
************************************/
      else {
	logit("e", "reboot_mss_ew: <%s> unknown parameter in <%s>\n",
	      com, config_file );
	continue;
      }

      /* See if there were any errors processing the command
***************************************************/
      if ( k_err() ){
	logit("e", "reboot_mss_ew: Bad <%s> command in <%s>.\n", com,
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

  if ( nmiss > 0 ){
    logit("e", "reboot_mss_ew: ERROR, no " );
    if ( !init[0] ) logit("e", "<InRing> " );
    if ( !init[1] ) logit("e", "<HeartbeatInt> " );
    if ( !init[2] ) logit("e", "<RebootGap> " );
    if ( !init[3] ) logit("e", "<MyModuleId> " );
    if ( !init[4] ) logit("e", "<ProgName> " );
    if ( !init[5] ) logit("e", "<Logout> " );
    logit("e", "command(s) in <%s>. Exiting.\n", config_file );
    return -1;
  }
  *pscnl = scnl;  /* Return address of scnl array */
  return 0;
}


 /***********************************************************************
  *                              LogConfig()                            *
  *                                                                     *
  *                   Log the configuration parameters                  *
  ***********************************************************************/

void LogConfig( GPARM *Gparm, SCNL *scnl )
{
   int i;

   logit( "", "\n" );
   logit( "", "InRing:        %s\n",   Gparm->InRing );
   logit( "", "MyModName:     %s\n",   Gparm->MyModName );
   logit( "", "ProgName:      %s\n",   Gparm->ProgName );
   logit( "", "HeartbeatInt:  %-6d\n", Gparm->HeartbeatInt );
   logit( "", "RebootGap:     %-6d\n", Gparm->RebootGap );
   logit( "", "Logout:        %-6d\n", Gparm->Logout );
   logit( "", "\n" );

   logit( "", "Sta Comp Net Loc       MSS IP       MSS Pwd\n" );
   logit( "", "----------------       ------       -------\n" );

   for ( i = 0; i < Gparm->nSCNL; i++ )
   {
      logit( "", "%-5s %-3s %-2s %-2s    %-15s    %-15s  %s\n", 
             scnl[i].sta, scnl[i].comp, scnl[i].net, scnl[i].loc, 
             scnl[i].mss_ip, scnl[i].mss_apwd, scnl[i].mss_ppwd );
   }
   logit( "", "\n" );
   return;
}

