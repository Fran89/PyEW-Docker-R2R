
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "earthworm.h"
#include "kom.h"
#include "transport.h"
#include "comserv_incl.h"
#include "misc.h"
#include "externs.h"
#include "scn_map.h"

int IsComment( char [] );  
void setuplogo(MSG_LOGO *);


     /***************************************************************
      *                          GetConfig()                        *
      *         Processes command file using kom.c functions.       *
      *           Returns -1 if any errors are encountered.         *
      ***************************************************************/

#define NCOMMAND 7             /* Number of commands in the config file */

int GetConfig( char *configfile )
{
   const int ncommand = NCOMMAND;

   char     init[NCOMMAND];     /* Flags, one for each command */
   int      nmiss;              /* Number of commands that were missed */
   int      nfiles;
   int      i;

/* Set to zero one init flag for each required command
   ***************************************************/
   for ( i = 0; i < ncommand; i++ )
      init[i] = 0;

/* Open the main configuration file
   ********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 )
   {
      fprintf(stderr, "%s: Error opening configuration file <%s>\n", Progname, configfile );
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
         if ( com[0] == '@' ) {
            success = nfiles + 1;
            nfiles  = k_open( &com[1] );
            if ( nfiles != success ) {
               fprintf(stderr, "%s: Error opening command file <%s>.\n", Progname, &com[1] );
               return -1;
            }
            continue;
         }

/* Read configuration parameters
   *****************************/
         if ( k_its( "ModuleId" ) ) {
            if ( (str = k_str()) ) {
               if ( GetModId(str, &QModuleId) == -1 ) {
                  fprintf( stderr, "%s: Invalid ModuleId <%s>. \n", 
				Progname, str );
                  fprintf( stderr, "%s: Please Register ModuleId <%s> in earthworm.d!\n", 
				Progname, str );
                  return -1;
               }
            }
            init[0] = 1;
         } else if ( k_its( "RingName" ) ) {
            if ( (str = k_str()) != NULL ) {
               if ( (RingKey = GetKey(str)) == -1 )
               {
                  fprintf( stderr, "%s: Invalid RingName <%s>. \n", 
			Progname, str );
                  return -1;
               }
            }
            init[1] = 1;
         } else if ( k_its( "HeartbeatInt" ) ) {
            HeartbeatInt = k_int();
            init[2] = 1;
         } else if ( k_its( "LogFile" ) ) {
            LogFile = k_int();
            init[3] = 1;
         } else if ( k_its( "TimeoutNoSend" ) ) {
            TimeoutNoSend = k_int();
            init[4] = 1;
         } else if ( k_its( "ComservSeqBuf" ) ) {
	    str = k_str();
	    if (!strcmp(str, "CSQ_LAST")) 
            	ComservSeqBuf = CSQ_LAST;
	    else if(!strcmp(str, "CSQ_NEXT"))
            	ComservSeqBuf = CSQ_NEXT;
	    else if(!strcmp(str, "CSQ_FIRST"))
            	ComservSeqBuf = CSQ_FIRST;
	    else {
                  fprintf( stderr, "%s: Invalid ComservSeqBuf type <%s>.\n", 
			Progname, str );
                  fprintf( stderr, "Should only be CSQ_LAST,CSQ_NEXT, or CSQ_FIRST.\n");
                  return -1;
	    }
            init[5] = 1;
         } else if ( k_its( "LOG2LogFile" ) ) {
            LOG2LogFile = k_int();
	    /* turn on the LogFile too! */
	    LogFile = 1;
            init[6] = 1;
         } else if ( k_its( "SCN2pinmap") ) {
	    char *S,*C,*N;
	    int pin;
	    S=k_str();
	    C=k_str();
	    N=k_str();
	    pin=k_int();
	    if (UseTraceBuf2) {
                  fprintf( stderr, "%s: SCN2pinmap entry <%s  %s %s %d> cannot be used with TRACEBUF2!\n",  
			Progname,  S,C,N, pin);
                  fprintf( stderr, "Please change SCN2pinmap to SCNL2pinmap\nnd add location code\n");
		  exit(-2);
	    }
	    if (insertSCN(S,C,N,pin) == -1) {
                  fprintf( stderr, "%s: Invalid SCN2pinmap entry <%s %s %s %d>.\n",  
			Progname, S,C,N,pin);
                  fprintf( stderr, "Follow the SEED header definitions!\n");
	    }
	 } else if ( k_its( "SCNL2pinmap") ) {
	    char *S,*C,*N, *L;
	    int pin;
	    UseTraceBuf2 = 1;
	    S=k_str();
	    C=k_str();
	    N=k_str();
	    L=k_str();
	    pin=k_int();
	    if (insertSCNL(S,C,N,L, pin) == -1) {
                  fprintf( stderr, "%s: Invalid SCNL2pinmap entry entry <%s %s %s %s %d>.\n",  
			Progname, S,C,N, L, pin);
                  fprintf( stderr, "Follow the SEED header definitions!\n");
	    }
	 } else {
	    /* An unknown parameter was encountered */
            fprintf( stderr, "%s: <%s> unknown parameter in <%s>\n", 
		Progname,com, configfile );
            return -1;
         }

/* See if there were any errors processing the command
   ***************************************************/
         if ( k_err() ) {
            fprintf( stderr, "%s: Bad <%s> command in <%s>.\n", 
		Progname, com, configfile );
            return -1;
         }
      }
      nfiles = k_close();
   }

/* After all files are closed, check flags for missed commands
   ***********************************************************/
   nmiss = 0;
	/* note the last argument is optional LOG2LogFile, hence
	the ncommand-1 in the for loop and not simply ncommand */
   for ( i = 0; i < ncommand-1; i++ )
      if ( !init[i] )
         nmiss++;

   if ( nmiss > 0 ) {
      fprintf( stderr,"%s: ERROR, no ", Progname );
      if ( !init[0]  ) fprintf(stderr, "<ModuleId> " );
      if ( !init[1]  ) fprintf(stderr, "<RingName> " );
      if ( !init[2] ) fprintf(stderr, "<HeartbeatInt> " );
      if ( !init[3] ) fprintf(stderr, "<LogFile> " );
      if ( !init[4] ) fprintf(stderr, "<TimeoutNoSend> " );
      if ( !init[5] ) fprintf(stderr, "<ComservSeqBuf> " );
	/* note that LOG2LogFile is an optional one, default is 0=NO */
      fprintf(stderr, "command(s) in <%s>.\n", configfile );
      return -1;
   }
	
   if ( GetType( "TYPE_HEARTBEAT", &TypeHB ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid message type <TYPE_HEARTBEAT>\n",Progname);
      return( -1 );
   }
   if ( GetType( "TYPE_TRACEBUF", &TypeTrace ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid message type <TYPE_TRACEBUF>; exiting!\n", Progname);
        return(-1);
   }
   if ( UseTraceBuf2 && GetType( "TYPE_TRACEBUF2", &TypeTrace2 ) != 0 ) {
      fprintf( stderr,
              "%s: Message type <TYPE_TRACEBUF2> not found in earthworm_global.d; exiting!\n", Progname);
        return(-1);
   } 
   if ( GetType( "TYPE_ERROR", &TypeErr ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid message type <TYPE_ERROR>\n", Progname);
      return( -1 );
   }
/* build the datalogo */
   setuplogo(&DataLogo);
   if (UseTraceBuf2) {
       DataLogo.type=TypeTrace2;
   } else {
       DataLogo.type=TypeTrace;
   }
   setuplogo(&OtherLogo);

   return 0;
}


 /***********************************************************************
  *                              LogConfig()                            *
  *                                                                     *
  *                   Log the configuration parameters                  *
  ***********************************************************************/

void LogConfig( void )
{
   logit( "", "ModuleId:             %8u\n",    QModuleId );
   logit( "", "RingKey:         	%8d\n",    RingKey );
   logit( "", "HeartbeatInt:         %8d\n",    HeartbeatInt );
   logit( "", "LogFile:         %8d\n",    LogFile );
   logit( "", "LOG2LogFile:         %8d\n",    LOG2LogFile );
   logit( "", "TimeoutNoSend:        %8d\n",    TimeoutNoSend );
   logit( "", "ComservSeqBuf:        %8d\n",    ComservSeqBuf );
   return;
}









    /*********************************************************************
     *                             IsComment()                           *
     *                                                                   *
     *  Accepts: String containing one line from a config file.          *
     *  Returns: 1 if it's a comment line                                *
     *           0 if it's not a comment line                            *
     *********************************************************************/

int IsComment( char string[] )
{
   int i;

   for ( i = 0; i < (int)strlen( string ); i++ )
   {
      char test = string[i];

      if ( test!=' ' && test!='\t' )
      {
         if ( test == '#'  )
            return 1;          /* It's a comment line */
         else
            return 0;          /* It's not a comment line */
      }
   }
   return 1;                   /* It contains only whitespace */
}
