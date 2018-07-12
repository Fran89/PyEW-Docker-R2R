/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: file2ew.c 6852 2016-10-20 21:01:59Z kevin $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.10  2007/02/26 14:23:17  paulf
 *     fixed long casting of time_t for heartbeat sprintf()
 *
 *     Revision 1.9  2007/02/23 16:14:19  paulf
 *     fixed long to time_t warning
 *
 *     Revision 1.8  2002/12/06 00:33:22  dietz
 *     Fixed faulty logic in handling a file that cannot be opened.
 *     Added instid argument to the file2ew_ship function.
 *     Changed lots of fprintf(stderr...) to logit("e"...)
 *
 *     Revision 1.7  2002/10/23 18:10:51  dietz
 *     Added missing argument in a logit call.
 *
 *     Revision 1.6  2002/06/05 16:28:20  patton
 *     Made logit changes.
 *
 *     Revision 1.5  2002/06/05 16:19:44  lucky
 *     I don't remember
 *
 *     Revision 1.4  2001/05/07 23:38:24  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or myPid.
 *
 *     Revision 1.3  2001/03/27 01:09:57  dietz
 *     Added support for reading heartbeat file contents.
 *
 *     Revision 1.2  2001/02/15 01:10:40  dietz
 *     Combined PeerHeartBeatFile and PeerHeartBeatInterval commands
 *
 *     Revision 1.1  2001/02/08 16:36:02  dietz
 *     Initial revision
 *
 *     Revision 1.8  2000/11/03 23:17:56  dietz
 *     *** empty log message ***
 *
 *     Revision 1.7  2000/10/24 23:29:37  dietz
 *     *** empty log message ***
 *
 *     Revision 1.6  2000/10/24 22:18:56  dietz
 *     *** empty log message ***
 *
 *     Revision 1.5  2000/10/20 18:47:14  dietz
 *     *** empty log message ***
 *
 *     Revision 1.4  2000/09/28 23:59:30  dietz
 *     changed rename() to rename_ew(); on NT allows rename to overwrite
 *     an existing file of the same name as the desired target
 *
 *     Revision 1.3  2000/09/26 22:59:05  dietz
 *     Added support to handle NSMP data, multiple heartbeat files and
 *     extra paging (outside statmgr) on loss of peer heartbeat
 *
 *     Revision 1.2  2000/09/07 21:40:21  dietz
 *     *** empty log message ***
 *
 *     Revision 1.1  2000/02/14 19:19:05  lucky
 *     Initial revision
 *
 *
 */

/*
 * file2ew.c:
 *
 * Merged version of sm_file2ring and reftek2ew, with the addition
 * of filters for various kinds of incoming strongmotion files.
 * Generalized later to work on file types other than strongmotion.
 *
 * Periodically checks a specified directory for files.
 * Passes the file thru a filter subroutine, and ultimately
 * writes some message(s) into tbe configured transport ring.  
 * In the filter, the contents of the file may be reformatted or not. 
 * 
 * Recognizes heartbeat file names; knows the interval at which they
 * should arrive; writes explicit TYPE_ERROR message to the
 * ring if that interval is exceeded.
 *
 * LDD December1999
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <errno.h>
#include "file2ew.h"

/* Functions in this source file 
 *******************************/
void file2ew_config ( char * );
void file2ew_lookup ( void );
void file2ew_page   ( char *, char * );
void file2ew_logbeat( FILE *, char * );
void file2ew_logmsg ( char *, int );

SHM_INFO  Region;             /* shared memory region to use for i/o    */
pid_t     myPid;              /* for restarts by startstop              */

#define MAXPEER 5             /* maximum # of peer heartbeats to expect  */
static int nPeer = 0;         /* actual number of configured peers       */
static int nPage = 0;         /* # groups to page on loss of peer        */
typedef struct _PEERHEART_ {
   char   sysname[NAM_LEN];   /* name of system sending heartbeat        */
   char   hbname[NAM_LEN];    /* name of heartbeat file to expect        */
   int    hbinterval;         /* max allowed secs between heartbeat files*/     
   time_t tlate;              /* time at which this file is overdue      */
   int    hbstatus;           /* current status of this heartbeat        */
} PEERHEART;

static char *TroubleSubdir = "trouble";   /* subdir for problem files    */
static char *SaveSubdir = "save";         /* subdir for processed files  */

/* Things to read or derive from configuration file
 **************************************************/
static char      RingName[20];        /* name of transport ring for i/o    */
static char      MyModName[50];       /* speak as this module name/id      */
static int       LogSwitch;           /* 0 if no logfile should be written */
static int       HeartBeatInterval;   /* seconds betweeen beats to statmgr */
static char      GetFromDir[NAM_LEN]; /* directory to monitor for data     */
static unsigned  CheckPeriod;         /* secs between looking for new files*/
static int	 OpenTries;
static int	 OpenWait;
static int       SaveDataFiles;       /* if non-zero, move to SaveSubdir,  */ 
                                      /*           0, remove files         */
static PEERHEART PeerHeartBeat[MAXPEER];   /* monitor heartbeat files here */
static char      PageOnLostPeer[MAXPEER][GRP_LEN]; /* notify if peer dies  */
static int       LogHeartBeatFile;    /* if non-zero, write contents of    */ 
                                      /*  each heartbeat file to daily log */
static int       LogOutgoingMsg = 0;  /* if non-zero, write each outgoing  */
                                      /*  msg to the daily log file        */
static int       SMFileFormat;        /* format of strong motion file      */

char      	 NetworkName[10];     /* Network code to use by default    */
int	         Debug;               /* non-zero -> debug logging         */

char      	 authority[10];       /* Network Authority code to use     */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          RingKey;       /* key of transport ring for i/o     */
static unsigned char InstId;        /* local installation id             */
static unsigned char MyModId;       /* Module Id for this program        */
static unsigned char TypeHeartBeat; 
       unsigned char TypeError;
       unsigned char TypePage;

char Text[TEXT_LEN];         /* string for log/error messages      */
char ProgName[NAM_LEN];      /* program name for logging purposes  */
#define LOGIT_LEN TEXT_LEN*2

/* File handling stuff
**********************/
int main( int argc, char **argv )
{
   int       result;
   int 	     read_error = 0;
   int	     i;
   char      fname[100];
   char      fnew[125];
   char     *c;
   FILE     *fp;
   time_t    tnextbeat;    /* next time for local heartbeat  */
   time_t    tnow;         /* current time */
   int       flag;         /* transport flag value */


/* Check command line arguments 
 ******************************/
   if ( argc != 2 )
   {
        fprintf( stderr, "Usage: %s <configfile>\n", argv[0] );
        exit( 0 );
   }
   strcpy( ProgName, argv[1] );
   c = strchr( ProgName, '.' );
   if( c ) *c = '\0';

/* Initialize name of log-file & open it 
 ***************************************/
   logit_init( argv[1], 0, LOGIT_LEN, 1 );   

/* Read the configuration file(s)
 ********************************/
   file2ew_config( argv[1] );
   logit( "" , "%s: Read command file <%s>\n", argv[0], argv[1] );

/* Look up important info from earthworm.h tables
 ************************************************/
   file2ew_lookup();

/* Reinitialize logit do desired logging level
 *********************************************/
   logit_init( argv[1], 0, LOGIT_LEN, LogSwitch );

/* Initialize the file2ewmsg filter 
 **********************************/
   result = file2ewfilter_init();
   if( result != 0 ) {
      logit("e","%s: Trouble initializing file2ew_filter; exiting!\n", 
             ProgName );
      file2ewfilter_shutdown();
      return( -1 );
   }

/* Get process ID for heartbeat messages 
 ***************************************/
   myPid = getpid();
   if( myPid == -1 )
   {
     logit("e","%s: Cannot get pid; exiting!\n", ProgName);
     file2ewfilter_shutdown();
     return( -1 );
   }

/* Change to the directory with the input files
 ***********************************************/
   if( chdir_ew( GetFromDir ) == -1 )
   {
      logit( "e", "%s: GetFromDir directory <%s> not found; "
                 "exiting!\n", ProgName, GetFromDir );
      file2ewfilter_shutdown();
      return( -1 );
   }
   if(Debug)logit("et","%s: changed to directory <%s>\n", ProgName,GetFromDir);

/* Make sure trouble subdirectory exists
 ***************************************/
   if( CreateDir( TroubleSubdir ) != EW_SUCCESS ) {
      logit( "e", "%s: trouble creating trouble directory: %s/%s\n",
              ProgName, GetFromDir, TroubleSubdir ); 
      file2ewfilter_shutdown();
      return( -1 );
   }

/* Make sure save subdirectory exists (if it will be used)
 *********************************************************/
   if( SaveDataFiles ) {
      if( CreateDir( SaveSubdir ) != EW_SUCCESS ) {
         logit( "e", "%s: trouble creating save directory: %s/%s\n",
                ProgName, GetFromDir, SaveSubdir ); 
      file2ewfilter_shutdown();
      return( -1 );
      }
   }

/* Attach to Output shared memory ring 
 *************************************/
   tport_attach( &Region, RingKey );
   logit( "", "%s: Attached to public memory region %s: %d\n", 
          ProgName, RingName, RingKey );

/* Force local heartbeat first time thru main loop
   but give peers the full interval before we expect a file
 **********************************************************/
   tnextbeat  = time(NULL) - 1;
   for( i=0; i<nPeer; i++ ) {
     PeerHeartBeat[i].tlate    = time(NULL) + PeerHeartBeat[i].hbinterval;
     PeerHeartBeat[i].hbstatus = ERR_PEER_ALIVE; 
   }


/****************  top of working loop ********************************/
   while(1)
   {
     /* Check on heartbeats
      *********************/
        tnow = time(NULL);
        if( tnow >= tnextbeat ) {  /* time to beat local heart */
           file2ew_status( TypeHeartBeat, 0, "" );
           tnextbeat = tnow + HeartBeatInterval;
        }
        if( PeerHeartBeat[0].hbinterval ) {     /* if we're monitoring hrtbeats */
           for( i=0; i<nPeer; i++ ) {           /* check each heartbeat file */
              if( tnow > PeerHeartBeat[i].tlate ) {    /* this peer is late! */
                 if( PeerHeartBeat[i].hbstatus == ERR_PEER_ALIVE ) { 
                    int j;                                       /* complain */
                    sprintf(Text, 
                           "No PeerHeartBeatFile <%s> from %s in over %d sec!",
                            PeerHeartBeat[i].hbname,PeerHeartBeat[i].sysname, 
                            PeerHeartBeat[i].hbinterval );
                    file2ew_status( TypeError, ERR_PEER_LOST, Text );
                    for(j=0;j<nPage; j++) file2ew_page(PageOnLostPeer[j],Text);
                 }      
                 PeerHeartBeat[i].hbstatus = ERR_PEER_LOST;   
              }
           }
        }

     /* See if termination has been requested 
      ****************************************/
        flag = tport_getflag( &Region );
 	if( flag == TERMINATE  ||  flag == myPid ) 
        {
           logit( "t", "%s: Termination requested; exiting!\n", ProgName );
           break;
        }

     /* Get a file name
      ******************/    
	result = GetFileName( fname );
	if( result == 1 ) {  /* No files found; wait for one to appear */
           sleep_ew( CheckPeriod*1000 ); 
	   continue;
	}
        if(Debug)logit("et","%s: got file name <%s>\n",ProgName,fname);

     /* Open the file.
      * We open for updating (even though we only want to read it), 
      * as that will hopefully get us an exclusive open. 
      * We don't ever want to look at a file that's being written to. 
      ***************************************************************/
        fp = NULL;
	for( i=0; i<OpenTries; i++ ) {
           fp = fopen( fname, "rb+" );
           if( fp != NULL ) break;
           sleep_ew( OpenWait );
	}
        if( fp == NULL ) { /* failed to open file! */
	   logit( "et","%s: Error: Could not open %s after %d*%d msec.",
                  ProgName, fname, OpenTries, OpenWait);
           result = -1;
           goto ProcessedFile;
        }
	if( i>0 ) {
           logit("t","Warning: %d attempts required to open file %s\n",
                  i+1, fname);
        }

     /* If it's a heartbeat file, reset tlate and delete the file.
      * We don't care when the heartbeat was created, only when we
      * saw it. This prevents 'heartbeats from the past' from 
      * confusing things, but may not be wise... 
      ***********************************************************/
        for( i=0; i<nPeer; i++ ) { /* does fname match a heartbeatfile? */
           if( strcmp(fname,PeerHeartBeat[i].hbname)==0 ) break;
        }
        if( i < nPeer ) {          /* yes, it's a heartbeat file */
           PeerHeartBeat[i].tlate = time(NULL) + PeerHeartBeat[i].hbinterval;
           if( PeerHeartBeat[i].hbstatus == ERR_PEER_LOST ) {  /* it's alive! */
              int j;
              sprintf(Text,"Received PeerHeartBeatFile <%s> from %s "
                      "(Peer is alive)",
                      PeerHeartBeat[i].hbname, PeerHeartBeat[i].sysname );
              file2ew_status( TypeError, ERR_PEER_ALIVE, Text );
              for(j=0;j<nPage; j++) file2ew_page(PageOnLostPeer[j],Text);
              PeerHeartBeat[i].hbstatus = ERR_PEER_ALIVE;            
           }
           if( LogHeartBeatFile ) file2ew_logbeat( fp, fname );

        /* Look inside heartbeat file for signs of trouble */
           file2ewfilter_hbeat( fp, fname, PeerHeartBeat[i].sysname );
           fclose( fp );
	   if( remove( fname ) != 0) {
	      logit("et",
                    "%s: Cannot delete heartbeat file <%s>;"
                    " exiting!", ProgName, fname );
	      break;
	   }
           continue;
        }

     /* Ignore any core files in the directory (Solaris issue)
        ******************************************************/
        else if( strcmp(fname,"core")==0 ) {
            fclose( fp );
            if( remove( fname ) != 0) {
                logit("et",
                    "%s: Cannot delete core file <%s>;"
                    " exiting!", ProgName, fname );
                break;
            }
            continue;
        }

     /* Pass non-heartbeat files to the appropriate file2ewmsg filter 
        *************************************************************/
        logit("et","%s: Processing %s ; ", ProgName, fname );
        result = file2ewfilter( fp, fname );
        fclose( fp );

     /* Everything went fine...
        result>0 means Earthworm msgs were created properly
        result=0 means file was recognized, but no msgs were produced
      ***************************************************************/
     ProcessedFile:
        if( result >= 0 ) {
           logit("e","created %3d EW msg, ", result );

        /* Keep file around */
           if( SaveDataFiles ) { 
              sprintf(fnew,"%s/%s",SaveSubdir,fname );
              if( rename_ew( fname, fnew ) != 0 ) {
                 logit( "e", "error moving file to ./%s\n; exiting!", 
                        fnew );
                 break;
              } else {
	         logit("e","moved to ./%s\n", SaveSubdir );
              }
           }

        /* Delete the file */
           else { 
	      if( remove( fname ) != 0 ) {
                 logit("e","error deleting file; exiting!\n");
                 break;
              } else  {
                 logit("e","deleted file.\n");
              }
           }
        }

    /* ...or there was trouble (result<0)! 
     *************************************/
        else { 
           logit("e","\n");
           sprintf( Text,"Trouble processing: %s ;", fname );
           file2ew_status( TypeError, ERR_CONVERT, Text );
           sprintf(fnew,"%s/%s",TroubleSubdir,fname );
           if( rename_ew( fname, fnew ) != 0 ) {
             logit( "e", " error moving file to ./%s ; exiting!\n", 
                     fnew );
             break;
           } else {
             logit( "e", " moved to ./%s\n", fnew );
           }
        }

   } /* end of while */

/************************ end of working loop ****************************/
	
/* detach from shared memory */
   tport_detach( &Region ); 

/* write a termination msg to log file */
   fflush( stdout );
   file2ewfilter_shutdown();
   return( 0 );

}  
/************************* end of main ***********************************/



/******************************************************************************
 *  file2ew_config() processes command file(s) using kom.c functions;         *
 *                    exits if any errors are encountered.                    *
 ******************************************************************************/
#define ncommand 13
void file2ew_config( char *configfile )
{
   char     init[ncommand]; /* init flags, one byte for each required command */
   int      nmiss;          /* number of required commands that were missed   */
   char    *com;
   char    *str;
   char     processor[20];
   int      nfiles;
   int      success;
   int      i;

/* Set to zero one init flag for each required command 
 *****************************************************/   
   for( i=0; i<ncommand; i++ )  init[i] = 0;

	strcpy (NetworkName, "--");
	strcpy (authority, "--");

/* Open the main configuration file 
 **********************************/
   nfiles = k_open( configfile ); 
   if ( nfiles == 0 ) {
        logit("e","%s: Error opening command file <%s>; exiting!\n", 
               ProgName, configfile );
        exit( -1 );
   }

/* Process all command files
 ***************************/
   while(nfiles > 0)   /* While there are command files open */
   {
        while(k_rd())        /* Read next line from active file  */
        {  
            com = k_str();         /* Get the first token from line */

        /* Ignore blank lines & comments
         *******************************/
            if( !com )           continue;
            if( com[0] == '#' )  continue;

        /* Open a nested configuration file 
         **********************************/
            if( com[0] == '@' ) {
               success = nfiles+1;
               nfiles  = k_open(&com[1]);
               if ( nfiles != success ) {
                  logit("e","%s: Error opening command file <%s>; exiting!\n",
                         ProgName, &com[1] );
                  exit( -1 );
               }
               continue;
            }
            strcpy( processor, "file2ew_config" );

        /* Process anything else as a command 
         ************************************/
  /*0*/     if( k_its("LogFile") ) {
                LogSwitch = k_int();
                init[0] = 1;
            }
  /*1*/     else if( k_its("MyModuleId") ) {
                str = k_str();
                if(str) strcpy( MyModName, str );
                init[1] = 1;
            }
  /*2*/     else if( k_its("RingName") ) {
                str = k_str();
                if(str) strcpy( RingName, str );
                init[2] = 1;
            }
  /*3*/     else if( k_its("GetFromDir") ) {
                str = k_str();
                if(str) strncpy( GetFromDir, str, NAM_LEN );
                init[3] = 1;
            }
  /*4*/     else if( k_its("CheckPeriod") ) {
                CheckPeriod = k_int();
                init[4] = 1;
            }
  /*5*/     else if( k_its("Debug") ) {
                Debug = k_int();
                init[5] = 1;
            }
  /*6*/     else if( k_its("OpenTries") ) {
                OpenTries = k_int();
                init[6] = 1;
            }
  /*7*/     else if( k_its("OpenWait") ) {
                OpenWait = k_int();
                init[7] = 1;
            }
  /*8*/     else if( k_its("PeerHeartBeatFile") ) {
                if( nPeer >= MAXPEER ) {
                   logit( "e", "%s: too many PeerHeartBeatFile cmds "
                          "(max=%d); exiting!\n", ProgName, MAXPEER );
                   exit( -1 );
                }
                if( str = k_str() ) {
                   strncpy( PeerHeartBeat[nPeer].sysname, str, NAM_LEN-1);
                   PeerHeartBeat[nPeer].sysname[NAM_LEN-1]='\0';
                   if( str = k_str() ) {
                      strncpy( PeerHeartBeat[nPeer].hbname, str, NAM_LEN-1);
                      PeerHeartBeat[nPeer].hbname[NAM_LEN-1]='\0';
                      PeerHeartBeat[nPeer].hbinterval = k_int();
                      nPeer++;
                   }
                }
                init[8] = 1;
            }

  /*9*/     else if( k_its("SaveDataFiles") ) {
                SaveDataFiles = k_int();
                init[9] = 1;
            }

  /*10*/    else if( k_its("HeartBeatInterval") ) {
                HeartBeatInterval = k_int();
                init[10] = 1;
            }
  /*11*/    else if( k_its("LogHeartBeatFile") ) {
                LogHeartBeatFile = k_int();
                init[11] = 1;
            }
  /*12*/   else if( k_its("LogOutgoingMsg") ) {
                LogOutgoingMsg = k_int();
                init[12] = 1;
            }
  /*opt*/   else if( k_its("NetworkName") ) {
                str = k_str();
                if(str) strncpy( NetworkName, str, 10 );
            }
 /*opt*/    else if( k_its("Authority") ) {
                str = k_str();
                str[2] = 0;
                if(str) strcpy( authority, str );
            }
  /*opt*/   else if( k_its("PageOnLostPeer") ) {
                if( nPage >= MAXPEER ) {
                   logit("e", "%s: too many PageOnLostPeer cmds "
                         "(max=%d); exiting!\n", ProgName, MAXPEER );
                   exit( -1 );
                }
                str = k_str();
                if( str && strlen(str)<GRP_LEN ) {
                   strcpy( PageOnLostPeer[nPage], str);
                   nPage++;
                } else {
                   logit("e", "%s: PageOnLostPeer arg <%s> too long; "
                         "must be 1-%d chars; exiting!\n", 
                          ProgName, str, GRP_LEN-1 );
                   exit( -1 );
               }
            }

         /* Pass it off to the filter's config processor
          **********************************************/
            else if( file2ewfilter_com() ) {
               strcpy( processor, "file2ewfilter_com" );
            }
 
         /* Unknown command
          *****************/ 
            else {
               logit( "e", "%s: <%s> Unknown command in <%s>.\n", 
                     ProgName, com, configfile );
               continue;
            }

        /* See if there were any errors processing the command 
         *****************************************************/
            if( k_err() ) {
               logit( "e", "%s: Bad <%s> command for %s() in <%s>; exiting!\n",
                      ProgName, com, processor, configfile );
               exit( -1 );
            }
        }
        nfiles = k_close();
   }

/* After all files are closed, check init flags for missed commands
 ******************************************************************/
   nmiss = 0;
   for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
   if ( nmiss ) {
       logit("e", "%s: ERROR, no ", ProgName );
       if ( !init[0] )  logit("e", "<LogFile> "               );
       if ( !init[1] )  logit("e", "<MyModuleId> "            );
       if ( !init[2] )  logit("e", "<RingName> "              );
       if ( !init[3] )  logit("e", "<GetFromDir> "            );
       if ( !init[4] )  logit("e", "<CheckPeriod> "           );
       if ( !init[5] )  logit("e", "<Debug> "                 );
       if ( !init[6] )  logit("e", "<OpenTries> "             );
       if ( !init[7] )  logit("e", "<OpenWait> "              );
       if ( !init[8] )  logit("e", "<PeerHeartBeatFile> "     );
       if ( !init[9] )  logit("e", "<SaveDataFiles> "         );
       if ( !init[10])  logit("e", "<HeartBeatInterval> "     );
       if ( !init[11])  logit("e", "<LogHeartBeatFile> "      );
       if ( !init[12])  logit("e", "<LogOutgoingMsg> "        );
       logit("e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

   return;
}

/******************************************************************************
 *  file2ew_lookup( )   Look up important info from earthworm.h tables        *
 ******************************************************************************/
void file2ew_lookup( void )
{
/* Look up keys to shared memory regions
   *************************************/
   if( ( RingKey = GetKey(RingName) ) == -1 ) {
        logit( "e", "%s:  Invalid ring name <%s>; exiting!\n",
               ProgName, RingName);
        exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      logit( "e", "%s: error getting local installation id; exiting!\n",
             ProgName );
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModName, &MyModId ) != 0 ) {
      logit( "e", "%s: Invalid module name <%s>; exiting!\n", 
             ProgName, MyModName );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      logit( "e", "%s: Invalid message type <TYPE_HEARTBEAT>; exiting!\n",
             ProgName );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      logit( "e", "%s: Invalid message type <TYPE_ERROR>; exiting!\n",
             ProgName );
      exit( -1 );
   }
   if ( GetType( "TYPE_PAGE", &TypePage ) != 0 ) {
      logit( "e", "%s: Invalid message type <TYPE_PAGE>; exiting!\n",
             ProgName );
      exit( -1 );
   }
   return;
} 

/******************************************************************************
 * file2ew_status() builds a heartbeat or error message & puts it into        *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
void file2ew_status( unsigned char type, short ierr, char *note )
{
   MSG_LOGO    logo;
   char        msg[256];
   long        size;
   time_t      t;
 
/* Build the message
 *******************/ 
   logo.instid = InstId;
   logo.mod    = MyModId;
   logo.type   = type;

   time( &t );

   if( type == TypeHeartBeat )
   {
        sprintf( msg, "%ld %ld\n", (long) t, (long) myPid);
   }
   else if( type == TypeError )
   {
        sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note);
        logit( "et", "%s: %s\n", ProgName, note );
   }

   size = strlen( msg );   /* don't include the null byte in the message */     

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &Region, &logo, size, msg ) != PUT_OK )
   {
        if( type == TypeHeartBeat ) {
           logit("et","%s:  Error sending heartbeat.\n", ProgName );
        }
        else if( type == TypeError ) {
           logit("et","%s:  Error sending error:%d.\n", ProgName, ierr );
        }
   }

   return;
}

/******************************************************************************
 * file2ew_page() builds a TYPE_PAGE message & puts it into shared memory.    *
 *                   Writes pager messages to log file & screen.              *
 ******************************************************************************/
void file2ew_page( char *group, char *note )
{
   MSG_LOGO    logo;
   char        msg[512];
 
/* Set logo values
 ******************/ 
   logo.instid = InstId;
   logo.mod    = MyModId;
   logo.type   = TypePage;

/* Assemble pageit message in msg
   ******************************/
   sprintf( msg, "group: %s %s#", group, note ); 
   logit("et","%s: Sending TYPE_PAGE msg: %s\n", ProgName, msg );

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &Region, &logo, strlen(msg), msg ) != PUT_OK )
   {
      logit("et","%s: Error sending TYPE_PAGE msg to transport region:\n", 
             ProgName );
   }
   return;
}


/******************************************************************************
 * file2ew_ship() Drop a message created elsewhere into the transport ring    * 
 *                using given message type and installation id.               *
 *                If given instid=0, use local installation id.               *
 ******************************************************************************/
int file2ew_ship( unsigned char type, unsigned char iid,
                  char *msg, size_t msglen )
{
   MSG_LOGO    logo;
   int         rc;

/* Fill in the logo
 *******************/ 
   if( iid==0 ) logo.instid = InstId;
   else         logo.instid = iid;
   logo.mod  = MyModId;
   logo.type = type;

   if( LogOutgoingMsg ) file2ew_logmsg( msg, msglen );

   rc = tport_putmsg( &Region, &logo, msglen, msg );
   if( rc != PUT_OK ){
      logit("et","%s: Error putting msg in ring.\n",
             ProgName );
      return( -1 );
   }
   return( 0 );
}

/******************************************************************************
 * file2ew_logmsg() logs an Earthworm message in chunks that are small enough *
 *                  that they won't overflow logit's buffer                   *
 ******************************************************************************/
void file2ew_logmsg(char *msg, int msglen )
{
   char  chunk[LOGIT_LEN];
   char *next     = msg;
   char *endmsg   = msg+msglen;
   int   chunklen = LOGIT_LEN-10;

   logit( "", "\n" );
   while( next < endmsg )
   {
      strncpy( chunk, next, chunklen );
      chunk[chunklen]='\0';
      logit( "", "%s", chunk );
      next += chunklen;
   }
   return;
}

/******************************************************************************
 * file2ew_logbeat() writes contents of a file to the log file                *
 ******************************************************************************/
void file2ew_logbeat( FILE *fp, char *name )
{
   char line[NAM_LEN];

   logit( "t","Contents of PeerHeartBeatFile <%s>:\n", name );
   while( fgets(line, NAM_LEN, fp) ) logit("","%s",line);

   return;
}
