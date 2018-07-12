/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: arc2trig.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.11  2007/02/26 16:37:01  paulf
 *     made sure time_t are casted to long for heartbeat sprintf()
 *
 *     Revision 1.10  2007/02/23 16:48:21  paulf
 *     fixed long warning for time_t
 *
 *     Revision 1.9  2006/10/13 15:55:16  paulf
 *     upgraded to v7.0 SCNL by David Scott of BGS
 *
 *     Revision 1.8  2002/05/15 19:20:50  patton
 *     Made Logit changes.
 *
 *     Revision 1.7  2001/05/09 17:45:15  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPid.
 *
 *     Revision 1.6  2000/12/06 18:35:07  lucky
 *     Changed Pph (which no longer exists) to Plabel in Hpck structure
 *
 *     Revision 1.5  2000/08/21 20:01:02  lucky
 *     Fixed to use new Hpck structure which can save both P and S picks
 *
 *     Revision 1.4  2000/08/08 18:26:14  lucky
 *     Lint cleanup
 *
 *     Revision 1.3  2000/07/24 20:15:42  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.2  2000/03/30 15:37:31  davidk
 *     changed the duration of the trace to be saved.  The trace was
 *     begin saved from (PickTime - PrePickTime) to (PickTime - PrePickTime +
 *     codalen + PostCodaTime).  The duration was changed so that trace would
 *     be saved until (PickTime + codalen + PostCodaTime).  This fixes a
 *     problem where not enough trace was being saved, and so the entire coda
 *     was usually not captured.
 *
 *     Revision 1.1  2000/02/14 16:04:49  lucky
 *     Initial revision
 *
 *
 */

/*
 * arc2trig.c:
Takes a hypo arc message as input and produces a .trg file
alex 8/5/97

So it says, but I think Lynn wrote this module. I (Alex) am modifying it to do
the author field, 6/16/98:

Story: The grand idea is that messages which relate to an event shall include
an author field. In particular, the trigger message shall have include, after
the "EVENT ID:", an "AUTHOR:", which is followed by the author id. The author
id is to be a series of logos, separated by :'s. Each logo is represented as
three sets of three ascii digits. The first logo is the logo of the module
which originated this event, followed by the logos of the modules which
processed it.

So hypoinverse (eqproc) created the event by emitting an arc message. It
should have stuck its logo in the author field. It doesn't, because we don't
have the stomach to change eqproc. So we make up for it here: we know the logo
of the message as it came to us, and we know our own module id, so we write
eqproc's logo and our own into the trigger message which we produce.

Mon Nov  9 13:21:16 MST 1998 lucky:

      Log file name will be built from the name of the configuration
      file -- argv[1] passed to logit_init().

      Incoming transport ring is flushed at start time to prevent 
      confusion if the module is restarted.

      Process id is sent with the heartbeat for restart purposes.

      ===================== Y2K compliance =====================
      Formats in read_hyp() and read_phs() changed (among other
      things)to include date in the form YYYYMMDD. 

      make_datestr() changed to create date string YYYYMMDD...
      
      calls to julsec15() and date15() replaced by julsec17() and
      date17() respectively (as the Y2K equivalents of old calls)

      Message type names changed to their Y2K equivalents.  */
/* More changes: 12/9/1998, Pete Lombard
   Yanked read_hyp() and read_ph() out to use new versions in 
   libsrc/util/read_arc.c

   Alex Nov 23 99:
	The amount of pre-pick data to be saved is now an optional
   	configuration file parameter. Also, default time changed from 
	5 seconds to 15 seconds.
   */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <chron3.h>
#include <earthworm.h>
#include <read_arc.h>
#include <kom.h>
#include <transport.h>
#include <site.h>
#include "arc2trig.h"

/* 0.0.2 on Oct 7, 2013 - versioning added and S-wave arrival handling!
 * 0.0.3 - 2013-10-08 - added MinDuration and UseOriginTime options
 * 0.1   - 2015-11-17 - added UseStationFile option
 * 0.2   - 2016-05-27 - mods for 64 bit compatibility; squelch warnings
 * 	*/
#define VERSION "0.2 - 2016-05-27"

/* Function prototypes */
int read_hyp (char *, char *, struct Hsum *);
int read_phs (char *, char *, struct Hpck *);

/* Time constants
*****************/
/* These variables are set here, but can be changed via
   optional coniguration file commands */
static double PrePickTime = 15; /* seconds to save before pick time */
static double PostCodaTime = 10; /* seconds past coda or tau time */

static double MinDuration = 0.0;
int UseOriginTime = 0;	/* flag to turn off or on use of OT as start time */

/* Functions in this source file
 *******************************/
void   arc2trig_config  ( char * );
void   arc2trig_lookup  ( void );
void   arc2trig_status  ( unsigned char, short, char * );
int    arc2trig_hinvarc ( char*, char*, MSG_LOGO );
char  *make_datestr( double, char * );
double tau( float );
void   bldtrig_hyp( char *, MSG_LOGO, MSG_LOGO );
void   bldtrig_phs( char * );
double distance(double, double, double, double, char);
void   bldtrig_site( char *, int, double );

static  SHM_INFO  InRegion;      /* shared memory region to use for input  */
static  SHM_INFO  OutRegion;     /* shared memory region to use for output */

#define   MAXLOGO   2
MSG_LOGO  GetLogo[MAXLOGO];    /* array for requesting module,type,instid */
short     nLogo;

static char ArcMsgBuf[MAX_BYTES_PER_EQ]; /* character string to hold event message
                                        MAX_BYTES_PER_EQ is from earthworm.h */

static char TrigMsgBuf[MAX_TRIG_BYTES]; /* to hold the trigger message / file */

/* Things to read or derive from configuration file
 **************************************************/
static char    InRingName[MAX_RING_STR];       /* name of transport ring for input  */
static char    OutRingName[MAX_RING_STR];      /* name of transport ring for output */
static char    MyModName[MAX_MOD_STR];        /* speak as this module name/id      */
static int     LogSwitch;            /* 0 if no logfile should be written */
static long    HeartBeatInterval;    /* seconds between heartbeats        */
static long    StationListDistance;     /* If > 0, get SCNLs from station file w/in SLD km */
static char    StationListPath[MAX_STR] = {0};
static int     Debug = 0;

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          InRingKey;      /* key of transport ring for input   */
static long          OutRingKey;     /* key of transport ring for output  */
static unsigned char InstId;         /* local installation id             */
static unsigned char MyModId;        /* Module Id for this program        */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypeHyp2000Arc;
static unsigned char TypeH71Sum2k;
static unsigned char TypeTrigList;

/* Error messages used by arc2trig
 *********************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_FILEIO        3   /* error opening trigger file             */
#define  ERR_DECODEARC     4   /* error reading input archive message    */
static char  Text[150];        /* string for log/error messages          */

/* Structures for hyp2000 data */
struct Hsum Sum;               /* Hyp2000 summary data                   */
struct Hpck Pick;              /* Hyp2000 pick structure                 */

pid_t	MyPid; 	/** Out process id is sent with heartbeat **/

/** length of string required by make_datestr  **/
#define	DATESTR_LEN		22	

int main( int argc, char **argv )
{
   time_t      timeNow;          /* current time                  */
   time_t      timeLastBeat;     /* time last heartbeat was sent  */
   long      recsize;          /* size of retrieved message     */
   MSG_LOGO  reclogo;          /* logo of retrieved message     */
   int       res;


/* Check command line arguments
 ******************************/
   if ( argc != 2 )
   {
        fprintf( stderr, "Usage: arc2trig <configfile>\n" );
        fprintf( stderr, "Version: %s\n", VERSION );
        exit( 0 );
   }

/* Initialize name of log-file & open it
 ***************************************/
   logit_init( argv[1], 0, 256, 1 );

/* Read the configuration file(s)
 ********************************/
   arc2trig_config( argv[1] );
   logit( "" , "%s: Read command file <%s>\n", argv[0], argv[1] );
   logit( "" , "%s: Version %s\n", argv[0], VERSION);

/* Look up important info from earthworm.h tables
 ************************************************/
   arc2trig_lookup();

/* Reinitialize logit logging to the desired level
 *************************************************/
   logit_init( argv[1], 0, 256, LogSwitch );
   

/* Get our own process ID for restart purposes
 **********************************************/

   if ((MyPid = getpid ()) == -1)
   {
      logit ("e", "arc2trig: Call to getpid failed. Exiting.\n");
      exit (-1);
   }

/* Initialize the trigger file
 *****************************/
   if( writetrig_init() != 0 ) {
      logit("", "arctrig: error opening file in OutputDir <%s>",
            OutputDir);
      logit("", "                          with BaseName  <%s>\n",
            TrigFileBase );
      exit( -1 );
   }
   logit( "", "arc2trig: Writing trigger files in %s\n", OutputDir );

/* Attach to Input shared memory ring
 ************************************/
   tport_attach( &InRegion, InRingKey );
   logit( "", "arc2trig: Attached to public memory region %s: %d\n",
          InRingName, InRingKey );

/* Attach to Output shared memory ring
 *************************************/
   tport_attach( &OutRegion, OutRingKey );
   logit( "", "arc2trig: Attached to public memory region %s: %d\n",
          OutRingName, OutRingKey );

/* Force a heartbeat to be issued in first pass thru main loop
 *************************************************************/
   timeLastBeat = time(&timeNow) - HeartBeatInterval - 1;

/* Flush the incomming transport ring on startup
 **************************************************/

	while (tport_getmsg (&InRegion, GetLogo, nLogo,  &reclogo,
				&recsize, ArcMsgBuf, sizeof(ArcMsgBuf)- 1) != GET_NONE)
		;

/*----------------------- setup done; start main loop -------------------------*/

   while(1)
   {
     /* send arc2trig's heartbeat
      ***************************/
        if  ( time(&timeNow) - timeLastBeat  >=  HeartBeatInterval )
        {
            timeLastBeat = timeNow;
            arc2trig_status( TypeHeartBeat, 0, "" );
        }

     /* Process all new messages
      **************************/
        do
        {
        /* see if a termination has been requested
         *****************************************/
           if ( tport_getflag( &InRegion ) == TERMINATE ||
                tport_getflag( &InRegion ) == MyPid )
           {
                writetrig_close();
           /* detach from shared memory */
                tport_detach( &InRegion );
           /* write a termination msg to log file */
                logit( "t", "arc2trig: Termination requested; exiting!\n" );
                fflush( stdout );
                exit( 0 );
           }

        /* Get msg & check the return code from transport
         ************************************************/
           res = tport_getmsg( &InRegion, GetLogo, nLogo,
                               &reclogo, &recsize, ArcMsgBuf, sizeof(ArcMsgBuf)-1 );

           if( res == GET_NONE )          /* no more new messages     */
           {
                break;
           }
           else if( res == GET_TOOBIG )   /* next message was too big */
           {                              /* complain and try again   */
                sprintf(Text,
                        "Retrieved msg[%ld] (i%u m%u t%u) too big for ArcMsgBuf[%ld]",
                        recsize, reclogo.instid, reclogo.mod, reclogo.type,
                        (long)sizeof(ArcMsgBuf)-1 );
                arc2trig_status( TypeError, ERR_TOOBIG, Text );
                continue;
           }
           else if( res == GET_MISS )     /* got a msg, but missed some */
           {
                sprintf( Text,
                        "Missed msg(s)  i%u m%u t%u  %s.",
                         reclogo.instid, reclogo.mod, reclogo.type, InRingName );
                arc2trig_status( TypeError, ERR_MISSMSG, Text );
           }
           else if( res == GET_NOTRACK ) /* got a msg, but can't tell */
           {                             /* if any were missed        */
                sprintf( Text,
                         "Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
                          reclogo.instid, reclogo.mod, reclogo.type );
                arc2trig_status( TypeError, ERR_NOTRACK, Text );
           }

        /* Process the message
         *********************/
           ArcMsgBuf[recsize] = '\0';      /*null terminate the message*/

           if( reclogo.type == TypeHyp2000Arc )
           {
               int ret;
               ret   = arc2trig_hinvarc( ArcMsgBuf, TrigMsgBuf, reclogo );
               if(ret) arc2trig_status( TypeError, ERR_DECODEARC, "" );
           }
           else if( reclogo.type == TypeH71Sum2k )
           {
              /* not doing anything with these message yet LDD:970814 */
              /* arc2trig_h71sum( ); */
           }

        } while( res != GET_NONE );  /*end of message-processing-loop */

        sleep_ew( 1000 );  /* no more messages; wait for new ones to arrive */

   }
/*-----------------------------end of main loop-------------------------------*/

    if ( Site != NULL )
        free( Site );
}

/******************************************************************************
 *  arc2trig_config() processes command file(s) using kom.c functions;        *
 *                    exits if any errors are encountered.                    *
 ******************************************************************************/
void arc2trig_config( char *configfile )
{
   int      ncommand;     /* # of required commands you expect to process   */
   char     init[10];     /* init flags, one byte for each required command */
   int      nmiss;        /* number of required commands that were missed   */
   char    *com;
   char    *str;
   int      nfiles;
   int      success;
   int      i;
   
   Site = NULL;     /* will need to be freed if UseSiteList command present */
   
/* Set to zero one init flag for each required command
 *****************************************************/
   ncommand = 8;
   for( i=0; i<ncommand; i++ )  init[i] = 0;
   nLogo = 0;

/* Open the main configuration file
 **********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
        logit( "e",
                "arc2trig: Error opening command file <%s>; exiting!\n",
                 configfile );
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
                  logit( "e",
                          "arc2trig: Error opening command file <%s>; exiting!\n",
                           &com[1] );
                  exit( -1 );
               }
               continue;
            }

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
  /*2*/     else if( k_its("InRingName") ) {
                str = k_str();
                if(str) strcpy( InRingName, str );
                init[2] = 1;
            }
  /*3*/     else if( k_its("OutRingName") ) {
                str = k_str();
                if(str) strcpy( OutRingName, str );
                init[3] = 1;
            }
  /*4*/     else if( k_its("HeartBeatInterval") ) {
                HeartBeatInterval = k_long();
                init[4] = 1;
            }


         /* Enter installation & module to get event messages from
          ********************************************************/
  /*5*/     else if( k_its("GetEventsFrom") ) {
                if ( nLogo+1 >= MAXLOGO ) {
                    logit( "e",
                            "arc2trig: Too many <GetEventsFrom> commands in <%s>",
                             configfile );
                    logit( "e", "; max=%d; exiting!\n", (int) MAXLOGO/2 );
                    exit( -1 );
                }
                if( ( str=k_str() ) != NULL ) {
                   if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
                       logit( "e",
                               "arc2trig: Invalid installation name <%s>", str );
                       logit( "e", " in <GetEventsFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].instid = GetLogo[nLogo].instid;
                }
                if( ( str=k_str() ) != NULL ) {
                   if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
                       logit( "e",
                               "arc2trig: Invalid module name <%s>", str );
                       logit( "e", " in <GetEventsFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].mod = GetLogo[nLogo].mod;
                }
                if( GetType( "TYPE_HYP2000ARC", &GetLogo[nLogo].type ) != 0 ) {
                    logit( "e",
                               "arc2trig: Invalid message type <TYPE_HYP2000ARC>" );
                    logit( "e", "; exiting!\n" );
                    exit( -1 );
                }
                if( GetType( "TYPE_H71SUM2K", &GetLogo[nLogo+1].type ) != 0 ) {
                    logit( "e",
                               "arc2trig: Invalid message type <TYPE_H71SUM2K>" );
                    logit( "e", "; exiting!\n" );
                    exit( -1 );
                }
                nLogo  += 2;
                init[5] = 1;
                if ( Debug ) {
                    printf("GetLogo[%d] inst:%d module:%d type:%d\n",
                        nLogo, (int) GetLogo[nLogo].instid,
                               (int) GetLogo[nLogo].mod,
                               (int) GetLogo[nLogo].type );
                    printf("GetLogo[%d] inst:%d module:%d type:%d\n",
                        nLogo+1, (int) GetLogo[nLogo+1].instid,
                               (int) GetLogo[nLogo+1].mod,
                               (int) GetLogo[nLogo+1].type );
                }
            }
  /*6*/     else if( k_its("OutputDir") ) {
                str = k_str();
                if(str) strcpy( OutputDir, str );
                init[6] = 1;
            }
  /*7*/     else if( k_its("BaseName") ) {
                str = k_str();
                if(str) strcpy( TrigFileBase, str );
                init[7] = 1;
            }
  /*optional*/
	    else if( k_its("PrePickTime") ) {
                PrePickTime = k_val();
            }
  /*optional*/
	    else if( k_its("PostCodaTime") ) {
                PostCodaTime = k_val();
            }
  /*optional*/
	    else if( k_its("MinDuration") ) {
                MinDuration = k_val();
            }
  /*optional*/
	    else if( k_its("UseOriginTime") ) {
                UseOriginTime = k_int();
            }
  /*optional*/
	    else if( k_its("Debug") ) {
                Debug = 1;
            }
  /*optional*/
	    else if( k_its("UseStationFile") ) {
                StationListDistance = k_long();
                str = k_str();
                if ( str ) {
                    strcpy( StationListPath, str );
                    site_read ( StationListPath );
                    if ( Debug )
                        logit( "", "arc2trig: Read %d stations from StationFile %s\n", nSite, StationListPath );
                } else {
                    logit( "e", "arc2trig: UseStationFile command missing path to station file; exiting!\n" );
                    exit( -1 );
                }   
            }

         /* Unknown command
          *****************/
            else {
                logit( "e", "arc2trig: <%s> Unknown command in <%s>.\n",
                         com, configfile );
                continue;
            }

        /* See if there were any errors processing the command
         *****************************************************/
            if( k_err() ) {
               logit( "e",
                       "arc2trig: Bad <%s> command in <%s>; exiting!\n",
                        com, configfile );
               exit( -1 );
            }
        }
        nfiles = k_close();
   }

/* After all files are closed, check init flags for missed commands
 ******************************************************************/
   nmiss = (StationListPath[0] && MinDuration==0) ? 1 : 0;
   for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
   if ( nmiss ) {
       logit( "e", "arc2trig: ERROR, no " );
       if ( !init[0] )  logit( "e", "<LogFile> "           );
       if ( !init[1] )  logit( "e", "<MyModuleId> "        );
       if ( !init[2] )  logit( "e", "<InRingName> "        );
       if ( !init[3] )  logit( "e", "<OutRingName> "       );
       if ( !init[4] )  logit( "e", "<HeartBeatInterval> " );
       if ( !init[5] )  logit( "e", "<GetEventsFrom> "     );
       if ( !init[6] )  logit( "e", "<OutputDir>"          );
       if ( !init[7] )  logit( "e", "<BaseName>"           );
       if ( StationListPath[0] && MinDuration==0 )
        logit( "e", "<MinDuration> (for <UseStationFile>)"   );
       logit( "e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

   return;
}

/******************************************************************************
 *  arc2trig_lookup( )   Look up important info from earthworm.h tables       *
 ******************************************************************************/
void arc2trig_lookup( void )
{
/* Look up keys to shared memory regions
   *************************************/
   if( ( InRingKey = GetKey(InRingName) ) == -1 ) {
        fprintf( stderr,
                "arc2trig:  Invalid ring name <%s>; exiting!\n", InRingName);
        exit( -1 );
   }

/* Look up keys to shared memory regions
   *************************************/
   if( ( OutRingKey = GetKey(OutRingName) ) == -1 ) {
        fprintf( stderr,
                "arc2trig:  Invalid ring name <%s>; exiting!\n", OutRingName);
        exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      fprintf( stderr,
              "arc2trig: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModName, &MyModId ) != 0 ) {
      fprintf( stderr,
              "arc2trig: Invalid module name <%s>; exiting!\n", MyModName );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      fprintf( stderr,
              "arc2trig: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      fprintf( stderr,
              "arc2trig: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_HYP2000ARC", &TypeHyp2000Arc ) != 0 ) {
      fprintf( stderr,
              "arc2trig: Invalid message type <TYPE_HYP2000ARC>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_H71SUM2K", &TypeH71Sum2k ) != 0 ) {
      fprintf( stderr,
              "arc2trig: Invalid message type <TYPE_H71SUM2K>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_TRIGLIST_SCNL", &TypeTrigList ) != 0 ) {      /* changed for v7.0, D Scott */
      fprintf( stderr,
              "arc2trig: Invalid message type <TYPE_TRIGLIST_SCNL>; exiting!\n" );
      exit( -1 );
   }

   return;
}

/******************************************************************************
 * arc2trig_status() builds a heartbeat or error message & puts it into       *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
void arc2trig_status( unsigned char type, short ierr, char *note )
{
   MSG_LOGO    logo;
   char        msg[256];
   long        size;
   time_t        t;

/* Build the message
 *******************/
   logo.instid = InstId;
   logo.mod    = MyModId;
   logo.type   = type;

   time( &t );

   if( type == TypeHeartBeat )
   {
        sprintf( msg, "%ld %d\n", (long) t, MyPid );
   }
   else if( type == TypeError )
   {
        sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note);
        logit( "et", "arc2trig: %s\n", note );
   }

   size = (long)strlen( msg );   /* don't include the null byte in the message */

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &OutRegion, &logo, size, msg ) != PUT_OK )
   {
        if( type == TypeHeartBeat ) {
           logit("et","arc2trig:  Error sending heartbeat.\n" );
        }
        else if( type == TypeError ) {
           logit("et","arc2trig:  Error sending error:%d.\n", ierr );
        }
   }

   return;
}

/*******************************************************************
 *  arc2trig_hinvarc( )  read a Hypoinverse archive message        *
 *                       and build a trigger message               *
 *******************************************************************/
int arc2trig_hinvarc( char* arcmsg, char* trigMsg, MSG_LOGO incoming_logo )
{
   MSG_LOGO  outgoing_logo;  /* outgoing logo                         */
   char     *in;             /* working pointer to archive message    */
   char      line[MAX_STR];  /* to store lines from msg               */
   char      shdw[MAX_STR];  /* to store shadow cards from msg        */
   int       msglen;         /* length of input archive message       */
   int       nline;          /* number of lines (not shadows) so far  */


   /* Initialize some stuff
   ***********************/
   nline  = 0;
   msglen = (int)strlen( arcmsg );
   in     = arcmsg;

   /* load outgoing logo
   *********************/
   outgoing_logo.instid = InstId;
   outgoing_logo.mod    = MyModId;
   outgoing_logo.type   = TypeTrigList;


  /* Read one data line and its shadow at a time from arcmsg; process them
   ***********************************************************************/
   while( in < arcmsg+msglen )
   {
      if ( sscanf( in, "%[^\n]", line ) != 1 )  return( -1 );
      in += strlen( line ) + 1;
      if ( sscanf( in, "%[^\n]", shdw ) != 1 )  return( -1 );
      in += strlen( shdw ) + 1;
      nline++;
      if ( Debug ) {
        logit( "e", "%s\n", line );
        logit( "e", "%s\n", shdw );
      }

      /* Process the hypocenter card (1st line of msg) & its shadow
      ************************************************************/
      if( nline == 1 ) {                /* hypocenter is 1st line in msg  */
         read_hyp( line, shdw, &Sum );
         bldtrig_hyp( trigMsg, incoming_logo, outgoing_logo);        /* write 1st part of trigger msg  */

         if ( StationListPath[0] ) {
            /* Write a phase line for each site within StationListDistance
             *************************************************************/
            int i;
            for ( i=0; i < nSite; i++ ) {
                double dist = distance( Site[i].lat, Site[i].lon, Sum.lat, Sum.lon, 'K' );
                if ( dist <= StationListDistance ) 
                    bldtrig_site( trigMsg, i, Sum.ot );
                else if ( Debug ) 
                    logit( "", "Rejected %s.%s.%s.%s: too far (%lf)\n",
                        Site[i].name, Site[i].comp, Site[i].net, Site[i].loc, dist );
                        
            }
            break;
         }  
         continue;
      }

      /* Process all the phase cards & their shadows
      *********************************************/
      if( strlen(line) < (size_t) 75 )  /* found the terminator line      */
         break;
      read_phs( line, shdw, &Pick );    /* load info into Pick structure   */
      bldtrig_phs( trigMsg );           /* write "phase" lines of trigger msg  */

   } /*end while over reading message*/

  /* Write trigger message to output ring
   **************************************/
   if( tport_putmsg( &OutRegion, &outgoing_logo, (long)strlen(trigMsg), trigMsg ) != PUT_OK )
   {
      logit("et","arc2trig: Error writing trigger message to ring.\n" );
   }

   /* Write trigger message to trigger file
    ***************************************/
   if( writetrig( "\n" ) != 0 )  /* add a blank line before trigger list */
   {
      arc2trig_status( TypeError, ERR_FILEIO, "Error opening trigger file" );
   }
   if( writetrig( trigMsg ) != 0 )
   {
      arc2trig_status( TypeError, ERR_FILEIO, "Error opening trigger file" );
   }
   if( writetrig( "\n" ) != 0 )  /* add a blank line after trigger list */
   {
      arc2trig_status( TypeError, ERR_FILEIO, "Error opening trigger file" );
   }

   return(0);
}

/**************************************************************
 * bldtrig_hyp() builds the EVENT line of a trigger message   *
 * Modified for author id by alex 7/10/98                     *
 **************************************************************/
void bldtrig_hyp( char *trigmsg, MSG_LOGO incoming, MSG_LOGO outgoing)
{
   char datestr[DATESTR_LEN];

/* Sample EVENT line for trigger message:
EVENT DETECTED     970729 03:01:13.22 UTC EVENT ID:123456 AUTHOR: asdf:asdf\n
0123456789 123456789 123456789 123456789 123456789 123456789
************************************************************/
   make_datestr( Sum.ot, datestr );
   /* added v2.0 to trigmsg for EW v7.0, D Scott */
   sprintf( trigmsg, "v2.0 EVENT DETECTED     %s UTC EVENT ID: %u AUTHOR: %03d%03d%03d:%03d%03d%03d\n\n", 
			datestr, (int) Sum.qid,
			incoming.type, incoming.mod, incoming.instid,
			outgoing.type, outgoing.mod, outgoing.instid );
   strcat ( trigmsg, "Sta/Cmp/Net/Loc   Date   Time                       start save       duration in sec.\n" );
   strcat ( trigmsg, "---------------   ------ ---------------    ------------------------------------------\n");

   if ( Debug )
    printf( "\n%s", trigmsg );

   return;
}

/****************************************************************
 * bldtrig_phs() builds the "phase" lines of a trigger message  *
 ****************************************************************/
void bldtrig_phs( char *trigmsg )
{
   char str[MEDIUM_STR];
   char pckt_str[DATESTR_LEN];
   char savet_str[DATESTR_LEN];
   double event_duration;  /* coda duration from tau()      */
   double save_duration;   /* total duration to save        */
   long codalen;

/* Convert times in seconds since 1600 to character strings
 **********************************************************/
   if( Pick.Plabel == 'P' || Pick.Ponset == 'P' ) {
   	make_datestr( Pick.Pat,        pckt_str  );
   	make_datestr( Pick.Pat-PrePickTime, savet_str );
   } else {
   	make_datestr( Pick.Sat,        pckt_str  );
   	make_datestr( Pick.Sat-PrePickTime, savet_str );
   }

/* Calculate how much to save based on the longer of
   calculated tau or picker-measured coda length
 ***************************************************/
   event_duration = tau( Sum.Md );
   codalen = Pick.codalen;
   if( codalen < 0 ) codalen = -codalen;

   /* Include PrePickTime in duration time, since we are
      starting at time PickTime-PrePickTime 
      davidk 032400
   ****************************************************/
   if( event_duration > codalen ) 
   {
      save_duration = event_duration + PostCodaTime + PrePickTime;
   }
   else 
   {
      save_duration = codalen + PostCodaTime + PrePickTime;
   }

   if (MinDuration > 0 && save_duration < MinDuration) {
      save_duration = MinDuration;
   }

   if ( UseOriginTime ) {
      make_datestr( Sum.ot, savet_str );
   }


/* Build the "phase" line!  Here's a sample:
alex 11/1/97: changed format to be variable lenth <station> <comp> <net>
separated by spaces:
 MCM VHZ NC N 19970729 03:01:13.34 UTC    save: yyyymmdd 03:00:12.34      120\n
0123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
********************************************************************************/
/* added location code, D Scott 06/10/06 */
   sprintf( str, " %s %s %s %s %c %s UTC    save: %s %8ld\n",
            Pick.site, Pick.comp, Pick.net, Pick.loc, Pick.Plabel,
            pckt_str, savet_str, (long)save_duration );

   if ( Debug )
       printf( "%s",str );

   strcat( trigmsg, str );

   return;
}

/****************************************************************
 * bldtrig_site() builds a site line of a trigger message       *
 ****************************************************************/
void bldtrig_site( char *trigmsg, int i, double ot )
{
   char str[MEDIUM_STR];
   char timestr[DATESTR_LEN];
   //time_t ot_time = ot;
   //struct tm mytimeinfo;
   long duration = (long)MinDuration;
   
   //ot_time -= GSEC1970;
   //gmtime_r ( &ot_time, &mytimeinfo );
   //strftime( timestr, DATESTR_LEN, "%Y%m%d %H:%M:%S.00", &mytimeinfo );
   make_datestr( ot, timestr );
   
   sprintf( str, " %4s %3s %2s %2s %c %s UTC    save: %s %8ld\n",
            Site[i].name, Site[i].comp, Site[i].net, Site[i].loc, ' ',
            timestr, timestr, duration );

   if ( Debug )
       printf( "%s",str );

   strcat( trigmsg, str );

   return;
}

/*********************************************************************
 * make_datestr()  takes a time in seconds since 1600 and converts   *
 *                 it into a character string in the form of:        *
 *                   "19880123 12:34:12.21"                          *
 *                 It returns a pointer to the new character string  *
 *                                                                   *
 *    NOTE: this requires an output buffer >=21 characters long      *
 *                                                                   *
 *  Y2K compliance:                                                  *
 *     date format changed to YYYYMMDD                               *
 *     date15() changed to date17()                                  *
 *                                                                   *
 *********************************************************************/

char *make_datestr( double t, char *datestr )
{
    char str17[18];   /* temporary date string */

/* Convert time to a pick-format character string */
    date17( t, str17 );

/* Convert a date character string in the form of:
   "19880123123412.21"        to one in the form of:
   "19880123 12:34:12.21"
    0123456789 123456789
   Requires a character string at least 21 characters long
*/
    strncpy( datestr, str17,    8 );    /*yyyymmdd*/
    datestr[8] = '\0';
    strcat ( datestr, " " );
    strncat( datestr, str17+8,  2 );    /*hr*/
    strcat ( datestr, ":" );
    strncat( datestr, str17+10,  2 );    /*min*/
    strcat ( datestr, ":" );
    strncat( datestr, str17+12, 5 );    /*seconds*/

    if ( Debug )
        printf( "str17 <%s>  newstr<%s>\n", str17, datestr );

    return( datestr );
}

/******************************************************
 * tau()  Calculate tau (duration) from magnitude     *
 ******************************************************/
double tau( float xmag )
{
/* From Dave Oppenheimer:
 *    TAU = 10.0**((XMAG - FMA)/FMB)
 *  where TAU  is in seconds,
 *        XMAG is the magnitude,
 *        FMA  is the coda magnitude intercept coefficient
 *        FMB  is the coda magnitude duration coefficient
 */

   float fma   = (float) -0.87;  /* value from a paper by Bakun */
   float fmb   = (float)  2.0;   /* value from a paper by Bakun */

   return( pow( (double)10.0, (double) (xmag-fma)/fmb ) );
}


#define pi 3.14159265358979323846

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::  This function converts decimal degrees to radians             :*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
double deg2rad(double deg) {
  return (deg * pi / 180);
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::  This function converts radians to decimal degrees             :*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
double rad2deg(double rad) {
  return (rad * 180 / pi);
}

/******************************************************
 * distance()  Calculate distance between 2 lat/lons, *
 *              in (M)iles, (K)ilometers, or (N)?     *
 ******************************************************/
double distance(double lat1, double lon1, double lat2, double lon2, char unit) {
  double theta, dist;
  theta = lon1 - lon2;
  dist = sin(deg2rad(lat1)) * sin(deg2rad(lat2)) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(theta));
  dist = acos(dist);
  dist = rad2deg(dist);
  dist = dist * 60 * 1.1515;
  switch(unit) {
    case 'M':
      break;
    case 'K':
      dist = dist * 1.609344;
      break;
    case 'N':
      dist = dist * 0.8684;
      break;
  }
  return (dist);
}

