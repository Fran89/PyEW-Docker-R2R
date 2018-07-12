
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: trig2disk.c 6221 2015-01-20 20:58:04Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.26  2010/12/27 14:15:00  saurel
 *     Postproc external script is now launched with tow arguments :
 *      date (YYYYMMDD) and time (HHMM) of the event processed
 *
 *     Revision 1.25  2010/02/11 17:12:21  scott
 *     Fixed check of snippet subnet being blank (as per Track ticket #21)
 *
 *     Revision 1.24  2008/03/14 15:29:49  paulf
 *     new Postproc command from Alexandre Nercessian
 *
 *     Revision 1.23  2007/02/26 20:47:04  paulf
 *     yet another time_t issue
 *
 *     Revision 1.22  2007/02/26 14:47:38  paulf
 *     made sure time_t are casted to long for heartbeat sprintf()
 *
 *     Revision 1.21  2006/10/18 14:31:03  paulf
 *     fix to error message for logo, contributed by Michael Lofgren
 *
 *     Revision 1.20  2005/11/03 17:40:30  luetgert
 *     .
 *
 *     Revision 1.18  2005/11/02 18:04:27  luetgert
 *     Increased array dimensions to accommodate more stations being saved to disk.
 *     .
 *
 *     Revision 1.17  2004/07/16 20:49:40  lombard
 *     Modified to provide minimal support for SEED location codes.
 *     trig2disk new reads TYPE_TRIGLIST_SCNL and query both SCN (old)
 *     and SCNL (new) wave_serverV.
 *
 *     Revision 1.16  2004/05/31 17:55:07  lombard
 *     Modified for location code.
 *
 *     Revision 1.15  2001/10/01 20:41:09  patton
 *     Made changes due to slightly reworked logit.
 *     JMP 10/1/2001
 *
 *     Revision 1.14  2001/08/30 21:34:49  patton
 *     Made logit changes due to new logit code.
 *     JMP 8/30/2001.
 *
 *     Revision 1.13  2001/08/09 17:06:20  lucky
 *     Fixed several superfluous logit statements in Debug mode.
 *
 *     Revision 1.12  2001/08/02 18:09:22  davidk
 *     Changed the logic of the program so that it does not exit
 *     because of wave_server errors.  Moved the return code handling
 *     of wsAppendMenu and wsGetTraceBin into their own separate
 *     functions.  Tried to concentrate the calls to KillSelfThread
 *     for the SnippetMaker thread into one place.
 *
 *     Revision 1.11  2001/05/08 00:00:23  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPid.
 *
 *     Revision 1.10  2001/04/12 15:29:56  lombard
 *     cleaned up DEBUG statements
 *
 *     Revision 1.9  2001/04/12 04:08:22  lombard
 *     Major cleanup and reformating of code.
 *     Added optional DelayTime, saving of queue to disk.
 *     Bug fixes that should stop crashes in NT.
 *     Removed checks for TravelTimeout which was never implemented on wave_serverV
 *
 *     Revision 1.8  2001/03/22 20:59:51  cjbryan
 *     removed now obsolete EventMod and EventInst varibles from
 *     putaway routine prototypes and function calls
 *     undid Lucky's acquiring of EventMod and EventInst from the
 *     snippet since these values are no longer used by any
 *     downstream routines
 *
 *     Revision 1.7  2001/03/21 02:09:37  alex
 *     added EventSubnet to carry Snippet.subnet to putaway routines
 *
 *     Revision 1.6  2001/03/08 16:36:29  lucky
 *     *** empty log message ***
 *
 *     Revision 1.5  2000/08/08 17:06:35  lucky
 *     *** empty log message ***
 *
 *     Revision 1.4  2000/08/03 19:31:43  lucky
 *     Fixed a bug which caused periodic crashes on NT and Solaris. It was caused
 *     by a non-null-terminated string. Also, cleaned up many things, like:
 *       *  Fixed lint problems;
 *       *  Defined things as static where appropriate;
 *       *  Kill the menu before rebuilding it each time through the loop
 *
 *     Revision 1.3  2000/07/24 18:46:58  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.2  2000/03/23 15:51:31  lucky
 *     changed the logit buffer size to MAX_TRIG_BYTES + 256
 *
 *     Revision 1.1  2000/02/14 19:48:44  lucky
 *     Initial revision
 *
 *
 */

/*
  Thu Jan 20 00 Alex
  SNIPPET structure member .eventId changed from being a long to being
  a char[EVENTID_SIZE]. (EVENTID_SIZE is defined in earthworm.h)

  Thu Jul 22 14:56:37 MDT 1999 Lucky Vidmar
  Major structural changes to accomodate making all putaway routines
  into library calls so that they can be shared between trig2disk
  and wave2disk. Eliminated global variables set and allocated here
  for use outside of this file. We should only have static globals from 
  now on, with everything else being passed in via the putaway.c calls.
  The idea is that whenever a new output format is to be added, or
  an existing format code changed, those changes should be done 
  in one place - putaway.c. All other code, like trig2disk or wave2disk
  can remain unchaged. 

  Tue Apr 13 11:56:39 MDT 1999 Lucky Vidmar
  Replaced Snip2DirName() with GetInfoFromSnippet() which fills
  several arrays with various values, like EventID, EventInst, 
  EventMod...which can be used more flexibly in naming output 
  files and directories.

  Thu Mar 18 10:41:11 MST 1999 Lucky Vidmar
  Added a new PC-SUDS output format. Works much the same as sac and ah. 
  Note that suds format will produce suds compliant files on both NT
  and solaris. However, suds files viewable with the sudsplot program, a
  very old beast, can only be produced under NT (because solaris insists
  on byte-padding structs which makes them unreadable to sudsplot).

  Added a new configuration parameter OutputFormat which indicates the 
  platform (intel or sparc) for which the files are being produced. This
  is important because of the different byte order between sparc and intel.

  General code cleanup: added more extensive error detection and reporting,
  comments, testing.

  v3.9 modified call to logit_init to use argv[1] instead of trig2disk.
  modified Snip2DirName and CatPseudoTrig to use Snppt.startYYYYMMDD
  (fixed bug in CatPseudoTrig while I was at it to find minimum start time)
  changed message type for TYPE_TRIGLIST to TYPE_TRIGLIST2K
  (parse_trig used throughout so that should be it).
  removed all RETURN_SUCCESS and RETURN_FAILURE (locally defined) and
  replaced with EW_SUCCESS and EW_FAILURE (earthworm defined).

  trig2disk.c: modified ora_trace_save to work with sacputaway (from earth2sac)
  also works with ahputaway (cloned from sacputaway).
  ahputaway uses <rpc/rpc.h> xdr routines.
  also makes use of ioroutin.c, a libraray of handy ah io routines
  that I (withers) flagrantly copied.  Got no idea where 
  they originally came from; probably Lamont.

  Reads trigger messages and writes specified trace snippets
  to files using specified format.

  todo:
  -- fill event portion of header with EVENT line from trigger
  -- fill station location (and response?) information
  -- allow output data types other than float
  -- putaway is kludgey, need 5 routines per data type and a strcmp 
  for each call to each routine. Should streamline this.

  Story: we pick up TYPE_TRIG messages which are generated by various
  detector algorithms. It's a verbose ascii message which we contacted
  from exposure to the Menlo volcano crowd. It lists an event id, event
  time, and a number of lines, each listing a station by name, trigger
  start time, and duration of data to be saved for that station.
  Wildcards are permitted in either the station, network, component or 
  location field (suggestion made by Steve Malone).

  We loop over the lines of the message. For each line we construct an
  array of implied trace requests. These had better be less than the
  configuration parameter MaxTrace. We then loop over this array,
  requesting and disposing of each trace, one at a time.  

*/

/* added versioning at SEISAN fix 2013.01.11 
   please upgrade version on this and waveman2disk 
   if putaway funcs get upgraded 

   1.0.2 - mseed works on Linux now!

   1.0.4 - made Postproc command workable on Windows
  
   1.0.5 - UW output added for  USBoR

*/
#define VERSION "1.0.5 2015-01-20"


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <earthworm.h>
#include <time.h>
#include <kom.h>
#include <transport.h>
#include <ws_clientII.h>
#include <mem_circ_queue.h>
#include <parse_trig.h>
#include <putaway.h>
#include "trig2disk.h"

/* Functions in this source file 
*******************************/
static int     trig2disk_config ( char * );
static int     trig2disk_lookup ( void );
static void    trig2disk_status ( MSG_LOGO *, short, char *, long );
static int     matchSCNL( SNIPPET* pSnppt, WS_PSCNL pscnl);
static int     duplicateSCNL( WS_PSCNL pscnl, TRACE_REQ* pTrReq, int nTraceReq);
static int     matchSCN( SNIPPET* pSnppt, WS_PSCNL pscnl);
static int     duplicateSCN( WS_PSCNL pscnl, TRACE_REQ* pTrReq, int nTraceReq);
static thr_ret MessageStacker( void * );
static thr_ret SnippetMaker( void * );
static int     GetInfoFromSnippet (SNIPPET *);
static void set_origin_date_time( char * msg_p, int msgSize );

static int CallAppendMenu(char * wsIp, char * wsPort, WS_MENU_QUEUE_REC * pMenuList, 
                          int TimeoutSeconds, char * szCallingFunction, int Debug);
static int CallGetTraceBin(TRACE_REQ * pTraceReq, WS_MENU_QUEUE_REC * pMenuList, 
                           int TimeoutSeconds, char * szCallingFunction, int Debug);

/* The message queue
*******************/
static	QUEUE OutQueue;             /* from queue.h, queue.c; sets up linked */
static	WS_MENU_QUEUE_REC MenuList; /* the list of menues */

/* Thread things
***************/
#define THREAD_STACK 8192
static unsigned tidSnipper;      /* Message processing thread id */
static unsigned tidStacker;      /* Thread moving messages from transport *
                                  *   to queue */
static	int MessageStackerStatus=0;     /* 0=> Stacker thread ok. <0 => dead */
static	int SnippetMakerStatus=0;       /* 0=> Snipper thread ok. <0 => dead */

/* Transport global variables
****************************/
static  SHM_INFO  Region;      /* shared memory region to use for i/o    */
#define MAXLOGO   10
static	MSG_LOGO  GetLogo[MAXLOGO];   /* array for requesting module,type,instid */
static  MSG_LOGO  errLogo;        /* Error logo                    */
static	short     nLogo;

        
/* Globals to set from configuration file
****************************************/
static int     QueueSize = 10;     /* max messages in output circular buffer */
static char    RingName[MAX_RING_STR+1];   /* name of transport ring for i/o */
static char    MyModName[MAX_MOD_STR+1];   /* speak as this module name/id   */
static int     LogSwitch;           /* 0 if no logfile should be written   */
static long    HeartBeatInt;        /* seconds between heartbeats          */
static long    TimeoutSeconds;      /* seconds to wait for reply from ws */
static long    MaxTraces;           /* max traces per message we'll ever deal with  */
static int     nWaveServers;        /* number of wave servers we know about */
static char    wsIp[MAX_WAVESERVERS][MAX_ADRLEN];
static char    wsPort[MAX_WAVESERVERS][MAX_ADRLEN];
static char    OutDir[MAXTXT];      /*base output data directory*/
static double  GapThresh;  
static int     MinDuration;         /* minimum duration of data files for *
                                     * stations not in triglist */
static char    OutputFormat[MAXTXT];/* intel or sparc */
static char    DataFormat[MAXTXT];  /* sac, suds, ah, tank are supported */
static char   *QueueFile = NULL;    /* optional file to saveread queue */
static char   *Postproc  = NULL;    /* optional program to postprocess event */
static time_t  DelayTime = 0;       /* seconds to wait to process trigger */

/** Snippet information section - Filled by GetInfoFromSnippet () **/
#define MOD_CHARS   3
#define INST_CHARS  3
#define TIME_CHARS  11

static char EventID[MAXTXT + 4];
static char EventDate[MAXTXT];
static char EventTime[MAXTXT + 4];
static char EventSubnet[MAXTXT];
static char OriginTime[MAXTXT + 4];
static char OriginDate[MAXTXT];

/* Things to look up in the earthworm.h tables with getutil.c functions
**********************************************************************/
static long          RingKey;       /* key of transport ring for i/o      */
static unsigned char InstId;        /* local installation id              */
static unsigned char MyModId;       /* Module Id for this program         */
static unsigned char TypeHeartBeat; 
static unsigned char TypeError;
static unsigned char TypeTrig;

/* Error messages used by trig2disk 
***************************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_QUEUE         3   /* error queueing message for sending     */
#define  ERR_FILE          4   /* error writing queue dump file          */
static char Text[MAXTXT];      /* string for log/error messages          */

/* Things to be malloc'd
***********************/
/* story: Some of these things don't have to be global; they are only
   so that we can malloc them from main at startup time. Having them 
   malloc'd by the routine which uses them presents the danger that the 
   module would start ok, but die (if unable to malloc) only when the 
   first event hits. Which could be much later, at a bad time...*/

static char       *TraceBuffer;      /* where we store the trace snippet  */
static long        TraceBufferLen;   /* bytes of largest snippet we're up for *
                                      *  from configuration file */
static TRACE_REQ*  TraceReq;         /* request forms for wave server.  */
static int          FormatIndex;     /* numerical value for our output format */
static long         OutBufferLen;    

/* Debug debug DEBUG */
static int Debug = 0;  

static SCNL      FStation[MAX_STATIONS];   /* Station struct from file */
static int       nstations = 0;            /* number of stations read */
static pid_t      MyPid;

int main( int argc, char **argv )
{
    time_t       timeNow;            /* current time                   */       
    time_t       timeLastBeat;       /* time last heartbeat was sent   */
    MSG_LOGO     hrtLogo;            /* heartbeat logo                 */
    int          i, rc = 0;

    /* Check command line arguments */
    if ( argc != 2 ) {
	fprintf( stderr, "Usage: trig2disk <configfile>\n" );
	fprintf( stderr, "trig2disk: version %s\n" , VERSION);
	return( 1 );
    }
   
    /* Initialize name of log-file & open it initially
       defaults to disk logging. */
    logit_init( argv[1], 0, (DB_MAX_TRIG_BYTES + 256), 1 );


    /* Zero the wave server arrays */
    for (i=0; i< MAX_WAVESERVERS; i++) {
	memset( wsIp[i], 0, MAX_ADRLEN);
	memset( wsPort[i], 0, MAX_ADRLEN);
    }
       
    /* Read the configuration file(s) */
    if (trig2disk_config (argv[1]) != EW_SUCCESS) {
	logit( "e", "trig2disk: Call to trig2disk_config failed \n");
	return( 1 );
    }
    
    /* Reset logit to the desired level of logging */
    logit_init( argv[1], 0, (DB_MAX_TRIG_BYTES + 256), LogSwitch );
    logit( "t" , "trig2disk: Read command file <%s>\n", argv[1] );
    logit( "t" , "trig2disk: Version %s\n", VERSION );

    /* Look up important info from earthworm.h tables */
    if (trig2disk_lookup () != EW_SUCCESS) {
	logit( "e", "trig2disk: Call to trig2disk_lookup failed \n");
	return( 1 );
    }
 
    /* initialize the socket system  */
    SocketSysInit();

    /* get the station list (if it doesnt exist, use whatever gets picked */
    if ( nstations > 0) {
	logit("","trig2disk using appended station list of %d stations\n",
	      nstations);
	if(Debug == 1) {
	    for(i=0; i<nstations; i++)
		logit("","station: %d %s %s %s %s\n",i,
		      FStation[i].sta, FStation[i].chan, FStation[i].net, 
		      FStation[i].loc);
	}
    }
    
    /* get pid for restart */
    MyPid = getpid();

    hrtLogo.instid = InstId;
    hrtLogo.mod    = MyModId;
    hrtLogo.type   = TypeHeartBeat;

    /* show the wave server info */
    if (Debug == 1) {
	logit("","wave server stuff:\n TimeoutSeconds=%d\n",TimeoutSeconds);
	for (i=0; i < nWaveServers; i++) {
	    logit(""," wsIp[%d]=.%s.",i,wsIp[i]);
	    logit(""," wsPort[%d]=.%s.\n",i,wsPort[i]);
	}
    }
    
    /* Allocate the trace snippet buffer */
    if ( (TraceBuffer = malloc( (size_t)TraceBufferLen)) == NULL ) {
	logit("e", 
	      "trig2disk: Cannot allocate snippet buffer of %ld bytes. Exiting\n",
	      TraceBufferLen);
	return( 1 );
    }

    /* Allocate the trace request structures */
    if ((TraceReq = calloc ((size_t)MaxTraces, sizeof(TRACE_REQ))) == NULL) {
	logit("e",
	      "trig2disk: Can't allocate %ld trace request structures. Exiting\n",
	      MaxTraces);
	return( 1 );
    }

    /* Initialize the disposal system */
    if (PA_init (DataFormat, TraceBufferLen, &OutBufferLen, 
		 &FormatIndex, OutDir, OutputFormat, Debug) != EW_SUCCESS) {
	logit ("e", "trig2disk: Call to PA_init failed; exiting!\n");
	return( 1 );
    }
    
    /* Force a heartbeat to be issued in first pass thru main loop */
    timeLastBeat = time(&timeNow) - HeartBeatInt - 1;
   
    /* Create a Mutex to control access to queue */
    CreateMutex_ew();

    /* Initialize the message queue */
    initqueue ( &OutQueue, (unsigned long)QueueSize,
		(unsigned long)DB_MAX_BYTES_PER_EQ + sizeof(time_t));

    /* Load the queue from the dump file */
    if (QueueFile != NULL) {
	rc = undumpqueue(&OutQueue, QueueFile);
	switch( rc ) {
	case 0:   /* All went well */
	    if (Debug)
		logit("", "successfully read %d msgs into queue\n", 
		      getNumOfElementsInQueue( &OutQueue));
	    break;
	case +1:  /* File doesn't exist */
	    logit("e", "trig2disk: QueueFile <%s> doesn't exist\n", QueueFile);
	    break;
	case -1:  /* Error reading file */
	    logit("e", "trig2disk: error reading <%s>: %s\n", QueueFile, 
		  strerror(errno));
	    break;
	case -2:  /* Too many entries */
	    logit("e", "trig2disk: too many entries in %s for queue; exiting\n",
		  QueueFile);
	    return( 1 );
	case -3:   /* entry too large */
	    logit("e", "trig2disk: entry too large in %s; exiting\n", QueueFile);
	    return( 1 );
	case -4:   /* Time stamps don't match */
	    logit("e", "trig2disk: timestamp mismatch in %s; queue starts empty\n",
		  QueueFile);
	    break;
	default:
	    logit("e", "trig2disk: unknown return (%d) from undumpqueue\n", rc);
	}
    }

    /* Attach to Input/Output shared memory ring  */
    tport_attach( &Region, RingKey );
    if (Debug == 1)
	logit( "", "Attached to public memory region %s: %d\n", 
	       RingName, RingKey );

    if(Debug == 1) 
	logit("e","starting to watch for trigger messages\n");

    /* Start the  message stacking thread */
    if ( StartThread(  MessageStacker, (unsigned)THREAD_STACK, &tidStacker ) == -1 ) {
	logit( "e", "Error starting  MessageStacker thread. Exiting.\n" );
	tport_detach( &Region );
	return( 1 );
    }
    MessageStackerStatus=0; /*assume the best*/

    /* Start the  snippet maker thread */
    if ( StartThread(  SnippetMaker, (unsigned)THREAD_STACK, &tidSnipper ) == -1 ) {
	logit( "e", "Error starting  MessageStacker thread. Exiting.\n" );
	tport_detach( &Region );
	return( 1 );    
    }
    SnippetMakerStatus=0; /*assume the best*/

    /* Having delegated message collecting, and message processing, there's *
     * not much for us left to do: watch thread status, and beat the heart  */

    /* begin main loop till Earthworm shutdown (level 1) */
    while( tport_getflag(&Region) != TERMINATE  &&
	   tport_getflag(&Region) != MyPid         ) {
	/* send trig2disk's heartbeat */
	if  ( time(&timeNow) - timeLastBeat  >=  HeartBeatInt ) {
	    timeLastBeat = timeNow;
	    trig2disk_status( &hrtLogo, 0, "", MyPid ); 
	}

	/* see how our threads are feeling */
	if ( SnippetMakerStatus < 0) {
	    logit("et","Snippet processing thread died. Exiting\n");
	    return( 1 );
	}
	if ( MessageStackerStatus < 0) {
	    logit("et","Message stacking thread died. Exiting\n");
	    return( 1 );
	}

	sleep_ew( 3000 );   
    }  /* end of until shutdown requested */  

    /* Tell our threads to die */
    MessageStackerStatus = 1;
    SnippetMakerStatus = 1;
    sleep_ew(1000);
  
    /* Termination has been requested */
    tport_detach( &Region ); /* detach from shared memory */
    logit( "t", "Termination requested; exiting!\n" );

    /* clean up after ourselves */
    if (PA_close (FormatIndex, Debug) != EW_SUCCESS) {
	logit( "", "Call to PA_close failed!\n");
    }
    
    return( 0 );
}
/*------------------------ end of main() -----------------------------*/


/***********************************************************************
 *  trig2disk_config() processes command file(s) using kom.c functions;*
 *                  exits if any errors are encountered.               *
 ***********************************************************************/
int trig2disk_config (char *configfile)
{
    int      ncommand;     /* # of required commands you expect to process   */ 
    char     init[20];     /* init flags, one byte for each required command */
    int      nmiss;        /* number of required commands that were missed   */
    char    *com;
    char    *str;
    int      nfiles;
    int      success;
    int      i;

    /* Set to zero one init flag for each required command  */
    ncommand = 14;
    for( i=0; i < ncommand; i++ )  
	init[i] = 0;
    nLogo = 0;
    nWaveServers = 0;
    Debug = 0;

    /* Open the main configuration file  */
    nfiles = k_open( configfile ); 
    if ( nfiles == 0 ) {
	logit( "e",
	       "trig2disk: Error opening command file <%s>; exiting!\n", 
	       configfile );
	return EW_FAILURE;
    }

    /* Process all command files */
    while(nfiles > 0) {   /* While there are command files open */
	while(k_rd()) {        /* Read next line from active file  */
	    com = k_str();         /* Get the first token from line */
	    
	    /* Ignore blank lines & comments */
	    if( !com )           continue;
	    if( com[0] == '#' )  continue;
	    
	    /* Open a nested configuration file  */
	    if( com[0] == '@' ) {
		success = nfiles+1;
		nfiles  = k_open(&com[1]);
		if ( nfiles != success ) {
		    logit( "e", 
			   "trig2disk: Error opening command file <%s>; exiting!\n",
			   &com[1] );
		    return EW_FAILURE;
		}
		continue;
	    }
	    
	    /* Process anything else as a command  */
	    /*0*/     if( k_its("LogFile") ) {
		LogSwitch = k_int();
		init[0] = 1;
	    }
	    /*1*/     else if( k_its("MyModuleId") ) {
		str = k_str();
		if(str) {
		    strncpy( MyModName, str, MAX_MOD_STR-1 );
		    MyModName[MAX_MOD_STR-1] = '\0';
		}
		init[1] = 1;
	    }
	    /*2*/     else if( k_its("RingName") ) {
		str = k_str();
		if(str) {
		    strncpy( RingName, str, MAX_RING_STR-1 );
		    RingName[MAX_RING_STR-1] = '\0';
		}
		init[2] = 1;
	    }
	    /*3*/     else if( k_its("HeartBeatInt") ) {
		HeartBeatInt = k_long();
		init[3] = 1;
	    }
	    
	    /* Enter installation & module to get event messages from */
	    /*4*/     else if( k_its("GetEventsFrom") ) {
		if ( nLogo >= MAXLOGO ) {
		    logit( "e", 
			   "trig2disk: Too many <GetEventsFrom> commands in <%s>", 
			   configfile );
		    logit( "e", "; max=%d; exiting!\n", (int) MAXLOGO );
		    return EW_FAILURE;
		}
		if( ( str=k_str() ) ) {
		    if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
			logit( "e", 
			       "trig2disk: Invalid installation name <%s>", str ); 
			logit( "e", " in <GetEventsFrom> cmd; exiting!\n" );
			return EW_FAILURE;
		    }
		    GetLogo[nLogo+1].instid = GetLogo[nLogo].instid;
		}
		if( ( str=k_str() ) ) {
		    if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
			logit( "e", 
			       "trig2disk: Invalid module name <%s>", str ); 
			logit( "e", " in <GetEventsFrom> cmd; exiting!\n" );
			return EW_FAILURE;
		    }
		    GetLogo[nLogo+1].mod = GetLogo[nLogo].mod;
		}
		if( GetType( "TYPE_TRIGLIST_SCNL", &GetLogo[nLogo].type ) != 0 ) {
		    logit( "e", 
			   "trig2disk: Invalid message type <TYPE_TRIGLIST_SCNL>" ); 
		    logit( "e", "; exiting!\n" );
		    return EW_FAILURE;
		}
		nLogo++;
		init[4] = 1;
	    }
	    /*5*/     else if( k_its("TimeoutSeconds") ) {
		TimeoutSeconds = k_int(); 
		init[5] = 1;
	    }
	    
	    /* wave server addresses and port numbers to get trace snippets from */
	    /*6*/     else if( k_its("WaveServer") ) {
		if ( nWaveServers >= MAX_WAVESERVERS ) 
		    {
			logit( "e", 
			       "trig2disk: Too many <WaveServer> commands in <%s>", 
			       configfile );
			logit( "e", "; max=%d; exiting!\n", (int) MAX_WAVESERVERS );
			return EW_FAILURE;
		    }
		if( ( str=k_str() ) )  
		    strcpy(wsIp[nWaveServers],str);
		if( ( str=k_str() ) )  
		    strcpy(wsPort[nWaveServers],str);
		nWaveServers++;
		init[6]=1;
	    }
	    
	    /*7*/     else if( k_its("TraceBufferLen") ) {
		TraceBufferLen = k_int() * 1024; /* convert from kilobytes to bytes */
		init[7] = 1;
	    }
	    
	    /*8*/     else if( k_its("MaxTraces") ) {
		MaxTraces = k_int(); 
		init[8] = 1;
	    }
	    
	    /*9*/    else if( k_its("MinDuration") ) {
		MinDuration = k_int();
		init[9]=1;
	    }
	    /*10*/    else if( k_its("DataFormat") ) {
		if( ( str=k_str() ) )  
		    strcpy( DataFormat, str );
		
		init[10] = 1;
	    }
	    /*11*/    else if( k_its("OutDir") ) {
		if( ( str=k_str() ) )  
		    strcpy( OutDir, str );
		init[11] = 1;
	    }
	    /*12*/    else if( k_its("GapThresh") ) {
		GapThresh = k_val();
		init[12]=1;
	    }
	    /*13*/    else if( k_its("OutputFormat") ) {
		if ((str = k_str()))  
		    strcpy (OutputFormat, str);
		
		/* check validity */
		if ((strcmp (OutputFormat, "intel") != 0) &&
		    (strcmp (OutputFormat, "sparc") != 0)) {
		    logit( "e", "trig2disk: Invalid OutputFormat %s\n", OutputFormat);
		    return EW_FAILURE;
		}
		init[13] = 1;
	    }
	    
	    /* Optional Commands */
	    /*NR*/    else if( k_its("Debug") )
		{  /*optional command*/
		    Debug = 1;
		}
	    
	    /*NR*/    else if( k_its("TrigStation") ) { 
		if ( nstations >= MAX_STATIONS ) {
		    logit( "e",
			   "trig2disk: Too many <TrigStation> commands in <%s>",
			   configfile );
		    logit( "e", "; max=%d; exiting!\n", (int) MAX_STATIONS );
		    return EW_FAILURE;
		}
		
		if( ( str=k_str() ) ) {
		    strncpy(FStation[nstations].sta, str, TRACE2_STA_LEN-1);
		    FStation[nstations].sta[TRACE2_STA_LEN-1] = '\0';
		}
		if( ( str=k_str() ) ) {
		    strncpy(FStation[nstations].chan, str, TRACE2_CHAN_LEN-1);
		    FStation[nstations].chan[TRACE2_CHAN_LEN-1] = '\0';
		}
		if( ( str=k_str() ) ) {
		    strncpy(FStation[nstations].net, str, TRACE2_NET_LEN-1);
		    FStation[nstations].net[TRACE2_NET_LEN-1] = '\0';
		}
		if( ( str=k_str() ) ) {
		    strncpy(FStation[nstations].loc, str, TRACE2_LOC_LEN-1);
		    FStation[nstations].loc[TRACE2_LOC_LEN-1] = '\0';
		}
		nstations++;
	    }
	    
	    /*NR*/   else if (k_its("QueueSize") ) {
		QueueSize = k_int();
	    }
	    
	    /*NR*/   else if (k_its("QueueFile") ) {
		if ( (str = k_str() ) )
		    {
			if ( (QueueFile = strdup(str)) == NULL) {
			    logit( "e", "trig2disk: out of memory for QueueFile name\n");
			    return EW_FAILURE;
			}
		    }
	    }

	    /*NR*/   else if (k_its("Postproc") ) {
		if ( (str = k_str() ) )
		    {
			if ( (Postproc = strdup(str)) == NULL) {
			    logit( "e", "trig2disk: out of memory for Postproc\n");
			    return EW_FAILURE;
			}
		    }
	    }
	    
	    /*NR*/   else if (k_its("DelayTime") ) {
		DelayTime = (time_t)k_int();
	    }
	    
	    /* Unknown command */
	    else {
		logit( "e", "trig2disk: <%s> Unknown command in <%s>.\n", 
		       com, configfile );
		continue;
	    }
	    
	    /* See if there were any errors processing the command  */
	    if( k_err() ) {
		logit( "e",
		       "trig2disk: Bad <%s> command in <%s>; exiting!\n",
		       com, configfile );
		return EW_FAILURE;
	    }
	} /* while k_rd() */
	
	nfiles = k_close();
    } /* while nfiles > 0 */
    
    /* After all files are closed, check init flags for missed commands */
    nmiss = 0;
    for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
    if ( nmiss ) {
	logit( "e", "trig2disk: ERROR, no " );
	if ( !init[0] )  logit( "e", "<LogFile> "       );
	if ( !init[1] )  logit( "e", "<MyModuleId> "    );
	if ( !init[2] )  logit( "e", "<RingName> "      );
	if ( !init[3] )  logit( "e", "<HeartBeatInt> "  );
	if ( !init[4] )  logit( "e", "<GetEventsFrom> " );
	if ( !init[5] )  logit( "e", "<TimeoutSeconds> ");
	if ( !init[6] )  logit( "e", "<WaveServer> "    );
	if ( !init[7] )  logit( "e", "<TraceBufferLen> ");
	if ( !init[8] )  logit( "e", "<MaxTraces> "     );
	if ( !init[9] )  logit( "e", "<MinDuration> "   );
	if ( !init[10] ) logit( "e", "<DataFormat> "   );
	if ( !init[11] ) logit( "e", "<OutDir> "   );
	if ( !init[12] ) logit( "e", "<GapThresh> "   );
	if ( !init[13] ) logit( "e", "<OutputFormat> "   );
	logit( "e", "command(s) in <%s>; exiting!\n", configfile );
	return EW_FAILURE;
    }

    return EW_SUCCESS;
}

/************************************************************************
 *  trig2disk_lookup( )   Look up important info from earthworm.h tables*
 ************************************************************************/
int trig2disk_lookup( void )
{
    /* Look up keys to shared memory regions */
    if( ( RingKey = GetKey(RingName) ) == -1 ) {
	fprintf( stderr,
		 "trig2disk:  Invalid ring name <%s>; exiting!\n", RingName);
	return EW_FAILURE;
    }

    /* Look up installations of interest */
    if ( GetLocalInst( &InstId ) != 0 ) {
	fprintf( stderr, 
		 "trig2disk: error getting local installation id; exiting!\n" );
	return EW_FAILURE;
    }

    /* Look up modules of interest */
    if ( GetModId( MyModName, &MyModId ) != 0 ) {
	fprintf( stderr, 
		 "trig2disk: Invalid module name <%s>; exiting!\n", MyModName );
	return EW_FAILURE;
    }

    /* Look up message types of interest */
    if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
	fprintf( stderr, 
		 "trig2disk: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
	return EW_FAILURE;
    }
    if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
	fprintf( stderr, 
		 "trig2disk: Invalid message type <TYPE_ERROR>; exiting!\n" );
	return EW_FAILURE;
    }
    if ( GetType( "TYPE_TRIGLIST_SCNL", &TypeTrig ) != 0 ) {
	fprintf( stderr, 
		 "trig2disk: Invalid message type <TYPE_TRIGLIST_SCNL>; exiting!\n" );
	return EW_FAILURE;
    }
    return EW_SUCCESS;
} 

/*************************************************************************
 * trig2disk_status() builds a heartbeat or error message & puts it into *
 *                 shared memory.  Writes errors to log file & screen.   *
 *************************************************************************/
void trig2disk_status( MSG_LOGO *pLogo, short ierr, char *note, long MyPid )
{
    char        msg[256];
    long        size;
    time_t      t;
 
    time( &t );

    if( pLogo->type == TypeHeartBeat ) {
	sprintf( msg, "%ld %ld\n", (long) t, (long) MyPid);
    }
    else if( pLogo->type == TypeError ) {
	sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note);
	logit( "et", "trig2disk: %s\n", note );
    }
    
    size = strlen( msg );   /* don't include the null byte in the message */     

    /* Write the message to shared memory */
    if( tport_putmsg( &Region, pLogo, size, msg ) != PUT_OK ) {
	if( pLogo->type == TypeHeartBeat ) {
	    logit("et","trig2disk:  Error sending heartbeat.\n" );
	}
	else if( pLogo->type == TypeError ) {
	    logit("et","trig2disk:  Error sending error:%d.\n", ierr );
	}
    }
}

/******************************************************************************
 * Given a pointer to a snippet structure (as parsed from the trigger         *
 * message) and the array of blank trace request structures, fill them with   *
 * the requests implied by the snippet. Don't overflow, and return the number *
 * of requests generated.                                                     *
 *                                                                            *
 * args: pmenu: pointer to the client routine's menu list                     *
 *   pSnppt: pointer to the structure containing the parsed trigger line from *
 *   the trigger message.                                                     *
 *   pTrReq: pointer to arrray of trace request forms                         *
 *   maxTraceReq: size of this array                                          *
 *   pnTraceReq: pointer to count of currently used trace request forms       *
 *****************************************************************************/
int snippet2trReq(WS_MENU_QUEUE_REC* pMenuList, SNIPPET* pSnppt, 
                  TRACE_REQ* pTrReq, int maxTraceReq, int* pnTraceReq )
{
    int ret =  WS_ERR_SCNL_NOT_IN_MENU;
    WS_MENU server; 

    if ((pMenuList == NULL) || (pSnppt == NULL) || (pTrReq == NULL) ||
	(pnTraceReq == NULL) || (maxTraceReq < 0)) {
	logit ("e", "Invalid arguments passed in\n");
	return WS_ERR_INPUT;
    }

    server = pMenuList->head;

    while ( server != NULL ) {
	WS_PSCNL tank = server->pscnl;
	
	if(Debug == 1) 
	    logit("","Searching through Server %s:%s:\n",server->addr,server->port);
	
	while ( tank != NULL ) {
	    if (strlen(tank->loc) > 0) {
		if ( matchSCNL(pSnppt, tank)==1 && duplicateSCNL(tank, pTrReq, *pnTraceReq)==0) {
		    ret = WS_ERR_NONE;
		    strcpy( pTrReq[*pnTraceReq].sta, tank->sta  );
		    strcpy( pTrReq[*pnTraceReq].net, tank->net  );
		    strcpy( pTrReq[*pnTraceReq].chan,tank->chan );
		    strcpy( pTrReq[*pnTraceReq].loc,tank->loc );
		    (*pnTraceReq)++;
		    if( *pnTraceReq >= maxTraceReq) {
			logit("","snippet2trReq: overflowed trace request array\n");
			return WS_ERR_BUFFER_OVERFLOW;
		    }
		}
	    } else {
		if ( matchSCN(pSnppt, tank)==1 && duplicateSCN(tank, pTrReq, *pnTraceReq)==0) {
		    ret = WS_ERR_NONE;
		    strcpy( pTrReq[*pnTraceReq].sta, tank->sta  );
		    strcpy( pTrReq[*pnTraceReq].net, tank->net  );
		    strcpy( pTrReq[*pnTraceReq].chan,tank->chan );
		    pTrReq[*pnTraceReq].loc[0] = '\0';
		    (*pnTraceReq)++;
		    if (strcmp(pSnppt->loc, "--") && strcmp(pSnppt->loc, "*"))
			logit("et", "snippet2trReq WARNING: specific location code <%s>"
			      "served by SCN server\n", pSnppt->loc);
		    if( *pnTraceReq >= maxTraceReq) {
			logit("","snippet2trReq: overflowed trace request array\n");
			return WS_ERR_BUFFER_OVERFLOW;
		    }
		}
	    }
	    tank = tank->next;
	}  /* End while(Tank) */
	
	server = server->next;
    }    /* End While (server) */
    
    if ( ret == WS_ERR_EMPTY_MENU )   
	logit( "","snippet2trReq(): Empty menu\n" );
    
    if (Debug && ret < 0) {
	logit("", "snippet2trReq error %d for <%s.%s.%s.%s>\n", ret, 
	      pTrReq[*pnTraceReq].sta, pTrReq[*pnTraceReq].chan,
	      pTrReq[*pnTraceReq].net, pTrReq[*pnTraceReq].loc);
    }
    
    return ret;
}


/****************************************************************************
 *   helper routine for snippet2trReq above: See if the SCNL name in the Snippet
 *   structure matches the menu item. Wildcards allowed               
 ******************************************************************************/
int matchSCNL( SNIPPET* pSnppt, WS_PSCNL pscnl)
{
    int staMatch =0;
    int netMatch =0;
    int chanMatch=0;
    int locMatch=0;
    
    if (strcmp( pSnppt->sta , "*") == 0)  staMatch =1;
    if (strcmp( pSnppt->chan, "*") == 0)  chanMatch=1;
    if (strcmp( pSnppt->net , "*") == 0)  netMatch =1;
    if (strcmp( pSnppt->loc,  "*") == 0)  locMatch =1;
    if (staMatch+netMatch+chanMatch+locMatch == 4) 
	return(1);

    if ( !staMatch && strcmp( pSnppt->sta, pscnl->sta )==0 ) staMatch=1;
    if ( !chanMatch && strcmp( pSnppt->chan, pscnl->chan )==0 ) chanMatch=1;
    if ( !netMatch && strcmp( pSnppt->net, pscnl->net )==0 ) netMatch=1;
    if ( !locMatch && strcmp( pSnppt->loc, pscnl->loc )==0 ) locMatch=1;

    if (staMatch+netMatch+chanMatch+locMatch == 4) 
	return(1);
    else
	return(0);
}
int matchSCN( SNIPPET* pSnppt, WS_PSCNL pscnl)
{
    int staMatch =0;
    int netMatch =0;
    int chanMatch=0;
    
    if (strcmp( pSnppt->sta , "*") == 0)  staMatch =1;
    if (strcmp( pSnppt->chan, "*") == 0)  chanMatch=1;
    if (strcmp( pSnppt->net , "*") == 0)  netMatch =1;
    if (staMatch+netMatch+chanMatch == 3) 
	return(1);

    if ( !staMatch && strcmp( pSnppt->sta, pscnl->sta )==0 ) staMatch=1;
    if ( !chanMatch && strcmp( pSnppt->chan, pscnl->chan )==0 ) chanMatch=1;
    if ( !netMatch && strcmp( pSnppt->net, pscnl->net )==0 ) netMatch=1;

    if (staMatch+netMatch+chanMatch == 3) 
	return(1);
    else
	return(0);
}
/*******************************************************************************
 *   helper routine for snippet2trReq above: See if the SCNL name in the Snippet 
 *   structure is a duplicate of any existing request                          
 ******************************************************************************/
int duplicateSCNL( WS_PSCNL pscnl, TRACE_REQ* pTrReq, int nTraceReq)
{
    int i;
    for (i=0; i<nTraceReq; i++) {
	if( strcmp( pTrReq[i].sta, pscnl->sta  )==0 &&
	    strcmp( pTrReq[i].net, pscnl->net  )==0 &&
	    strcmp( pTrReq[i].chan,pscnl->chan )==0 &&
	    strcmp( pTrReq[i].loc, pscnl->loc  )==0 ) 
	    return(1); /* meaning: yes, it's a duplicate */
    }
    return(0); /* 'twas not a duplicate */
}
int duplicateSCN( WS_PSCNL pscnl, TRACE_REQ* pTrReq, int nTraceReq)
{
    int i;
    for (i=0; i<nTraceReq; i++) {
	if( strcmp( pTrReq[i].sta, pscnl->sta  )==0 &&
	    strcmp( pTrReq[i].net, pscnl->net  )==0 &&
	    strcmp( pTrReq[i].chan,pscnl->chan )==0 )
	    return(1); /* meaning: yes, it's a duplicate */
    }
    return(0); /* 'twas not a duplicate */
}



/********************** Message Stacking Thread *******************
 *           Move messages from transport to memory queue         *
 ******************************************************************/
thr_ret MessageStacker( void *dummy )
{
    char        *msg;           /* message buffer, including receipt time  */
    char        *msg_p;         /* the actual trigger message    */
    int          res;
    long         recsize;       /* size of retrieved message     */
    MSG_LOGO     reclogo;       /* logo of retrieved message     */
    int          ret, rc, err;
    time_t       recvd;         /* time we received this message */

    /* Allocate space for input/output messages */
    if ( ( msg = (char *) malloc(DB_MAX_BYTES_PER_EQ) ) == (char *) NULL ) {
	logit( "e", "trig2disk: error allocating msg; exiting!\n" );
	goto error;
    }

    /* In the queue, we put the timestamp followed by the trigger message */
    msg_p = msg + sizeof(time_t);
  
    errLogo.instid = InstId;
    errLogo.mod    = MyModId;
    errLogo.type   = TypeError;

    /* Tell the main thread we're ok */
    MessageStackerStatus=0;

    /* flush the input ring buffer */
    while( tport_getmsg( &Region, GetLogo, nLogo, &reclogo, &recsize, 
			 msg_p, DB_MAX_BYTES_PER_EQ-1) != GET_NONE )
	;

    /* Start service loop, picking up trigger messages */
    while( 1 ) {
	if (MessageStackerStatus > 0)
	    break;  /* We're terminated! */
	
	/* Get a message from transport ring */
	res = tport_getmsg( &Region, GetLogo, nLogo, &reclogo, 
			    &recsize, msg_p, DB_MAX_BYTES_PER_EQ-1 );
	if(Debug == 1) 
	    if(res==GET_OK)
		logit("et","Got message from transport of %ld bytes, res=%d\n",
		      recsize,res); 
	
	if( res == GET_NONE ) {
	    sleep_ew(1000); 
	    continue;
	} /*wait if no messages for us */
	
	/* Check return code; report errors */
	switch ( res ) {
	case GET_OK:
	    break;
	case GET_TOOBIG:
	    sprintf( Text, "msg[%ld] i%d m%d t%d too long for target",
		     recsize, (int) reclogo.instid,
		     (int) reclogo.mod, (int)reclogo.type );
	    trig2disk_status( &errLogo, ERR_TOOBIG, Text, MyPid );
	    continue;
	case GET_MISS:
	    sprintf( Text, "missed msg(s) i%d m%d t%d in %s",
		     (int) reclogo.instid, (int) reclogo.mod, 
		     (int)reclogo.type, RingName );
	    trig2disk_status( &errLogo, ERR_MISSMSG, Text, MyPid );
	    break;
	case GET_NOTRACK:
	    sprintf( Text, "no tracking for logo i%d m%d t%d in %s",
		     (int) reclogo.instid, (int) reclogo.mod, 
		     (int)reclogo.type, RingName );
	    trig2disk_status( &errLogo, ERR_NOTRACK, Text, MyPid );
	    break;
	}

	recvd = time(0);
	memcpy(msg, (char *)&recvd, sizeof(time_t));    
	recsize += (long)sizeof(time_t);
    
	/* Queue retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK) */
	RequestMutex();
	rc = 0;
	ret = enqueue( &OutQueue, msg, recsize, reclogo );
	if (ret == 0 && QueueFile != NULL) {
	    rc = dumpqueue( &OutQueue, QueueFile);
	    err = errno;  /* save the error */
	}
	ReleaseMutex_ew();

	if(Debug == 1) 
	    logit("","Queued a message\n");
	switch ( ret ) {
	case 0:
	    break;
	case -2:   /* Serious: quit */
	    sprintf(Text,"internal queue error. Terminating.");
	    trig2disk_status( &errLogo, ERR_QUEUE, Text, MyPid );
	    goto error;
	case -1:
	    sprintf(Text,"queue cannot allocate memory. Lost message.");
	    trig2disk_status( &errLogo, ERR_QUEUE, Text, MyPid );
	    continue;
	case -3:
	    sprintf(Text,"Queue full. Message lost.");
	    trig2disk_status( &errLogo, ERR_QUEUE, Text, MyPid );
	    continue;
	}

	if (rc < 0) {
	    sprintf(Text,"Error writing queue file: %s.", strerror(errno));
	    trig2disk_status( &errLogo, ERR_FILE, Text, MyPid );
	}
	else if (Debug && QueueFile != NULL) {
	    logit("t", "MessageStacker: dumped %d queue messages to %s\n", 
		  getNumOfElementsInQueue( &OutQueue), QueueFile);
	}
    
	if(Debug == 1) 
	    logit("et", "stacker thread: queued msg len:%ld\n",recsize);
    }

    /* we're quitting */
 error:
    MessageStackerStatus = -1; /* file a complaint to the main thread */
    free(msg);
  
    KillSelfThread(); 
    return(NULL);  /* should never reach here */
}

/******************** Message Processing Thread *******************
 *           Take messages from memory queue, create trace        *
 *      snippets, and call the disposal (PutAway) routines        *
 ******************************************************************/
/* 

story: the trigger message contains lines specifying 'snippets' of
trace data (EventId, SCNL, start time, and duration to save). The S,C,
and N specification can be a *, meaning wildcard. What we do below is
to loop over the lines in the message, creating an array of trace
request structures (TRACE_REQ). We then loop over those, trying to
extract the trace from the WaveServers. 

*/

thr_ret SnippetMaker( void *dummy )
{
    static char  trgMsg[DB_MAX_TRIG_BYTES + sizeof(time_t)]; 
    char        *msg_p;          /* pointer to the actual trigger message */
    char        *time_p;         /* pointer to the time preceeding the    *
				  * trigger message, which may not be     *
				  * byte-aligned                          */
    int          i;
    long         msgSize;        /* size of retrieved message             */
    int          ret, rc;
    SNIPPET      Snppt;          /* holds params for trace snippet. From 
				    parse_trig.h */
    MSG_LOGO     reclogo;        /* logo of retrieved message             */
    char*        nxtSnippetPtr;  /* pointer to next line in trigger message */
    int          iTrReq=0;       /* running index over TRACE_REQ structures */
    int          oldNTrRq=0;     /* temp for storing number of tr req's */
    int          nTrReq=0;       /* number of TRACE_REQ structures 
				    from the entire message*/
    time_t       now;            /* Current time, according to the clock */
    time_t       rec;            /* Time the msg was received by stacker */

    WS_MENU server = NULL; 
    msg_p = trgMsg + sizeof(time_t);

    /* Tell the main thread we're ok */
    SnippetMakerStatus = 0;
    while (1) {
	if (SnippetMakerStatus > 0)
	    break;
	
	/* Get message from queue */
	rc = 0;
	RequestMutex();
	if (DelayTime > 0) {
	    /* take a peek at the next queued item without dequeueing it */
	    time_p = peekNextElement( &OutQueue );
	    if (time_p != NULL) {
		/* Look at the time the msg was received MessageStacker */
		memcpy(&rec, time_p, sizeof(time_t));
		now = time(0);
		rec += DelayTime;
		if (rec > now) {      /* It's too new; wait a little while */
		    ReleaseMutex_ew();  /* let the MessageStacker have the queue */
		    if (Debug)
			logit("et", "waiting %ld seconds to process trigger\n", rec - now);
		    sleep_ew(1000);
		    continue;     /* This message we just examined may get overwritten *
				   * by the MessageStacker. So we will need to check   *
				   * again to see if the "next" queued message is old  *
				   * enough to process. If the MessageStacker is fast  *
				   * enough, we'll never have to do any work! Maybe    *
				   * we should take "too young" messages if the queue  *
				   * is nearly full...                                 */
		}
	    }
	    else {  /* Nothing in queue */ 
		ReleaseMutex_ew();
		sleep_ew(1000);
		continue;
	    }
	}  /* end (DelayTime > 0) */
	ret = dequeue( &OutQueue, trgMsg, &msgSize, &reclogo);
	if (ret == 0 && QueueFile != NULL)
	    rc = dumpqueue(&OutQueue, QueueFile);
	ReleaseMutex_ew();
	
	/* -1 means empty queue */
	if (ret < 0 ) { 
	    sleep_ew(500); /* wait a bit then try again */
	    continue;
	}
	
	if (rc < 0) {
	    sprintf(Text,"Error writing queue file: %s.", strerror(errno));
	    trig2disk_status( &errLogo, ERR_FILE, Text, MyPid );
	}
	else if (Debug && QueueFile != NULL) {
	    logit("t", "SnippetMaker: dumped %d queue messages to %s\n",
		  getNumOfElementsInQueue( &OutQueue), QueueFile);
	}
	
	if (reclogo.type != TypeTrig) {
	    logit("","illegal (non-trigger) message in queue; exiting!\n");
	    SnippetMakerStatus = -1;
	    KillSelfThread();
	}
	trgMsg[msgSize] = '\0';   /*null terminate the message*/
	
	logit ("", "*** processing new trigger message (%d bytes) ***\n",
	       msgSize);
	logit ("", "%s\n", msg_p);
        /* Set the real OriginTime and OriginDate */
        set_origin_date_time(msg_p,msgSize);
        logit ("", "*** processing new trigger message after (%d bytes) ***\n",
	       msgSize);
	logit ("", "%s\n", msg_p);
	
	/* Build the current wave server menus */
	wsKillMenu( &MenuList );
	for (i = 0;i < nWaveServers; i++) {
	    rc = CallAppendMenu(wsIp[i], wsPort[i], &MenuList, 
				TimeoutSeconds, "SnippetMaker", Debug);
	    if(rc == -1) {
		logit("e","Fatal error returned from CallAppendMenu(), exiting!\n");
		goto ShutdownSnippetMaker;
	    }
	}
	/* Make sure that we have at least one menu */
	
	if ( (server = MenuList.head) == (WS_MENU)NULL) {
	    logit("","SnippetMaker: WARNING:  wsAppendMenu built no menus; continuing!\n"); 
	    logit("et","trig2disk: WARNING: No Wave Server menus retrieved for message "
		  "with logo (i%u m%u t%u)!\n", reclogo.instid, reclogo.mod, reclogo.type);
	    continue;
	}
	
	/*Debug: show off our menu list */
	if(Debug == 1) {
	    logit( "","\n Total Menu List:\n" );
	    while ( server != NULL ) {
		WS_PSCNL tank = server->pscnl;
		logit("","Server %s:%s:\n",server->addr,server->port);
		while ( tank != NULL ) {
		    logit("","%s %s %s %s %f %f\n", 
			  tank->sta,tank->chan,tank->net, tank->loc, 
			  tank->tankStarttime, tank->tankEndtime);
		    tank = tank->next;
		}
		server = server->next;
	    } 
	    logit("","End of Menu\n");
	}
	
	/* Initialize stuff for loop over snippets */
	if(nstations > 0) {
	    if ( CatPsuedoTrig(msg_p, FStation, nstations, MinDuration) 
		 != EW_SUCCESS )
		logit("","CatPsuedoTrig failed to add stations \n");
	}
	if(Debug == 1) 
	    logit("","return from CatPseudoTrig. New trgMsg size = %d\n",
		  strlen(msg_p));
	
	nxtSnippetPtr = msg_p; 
	
	/* begin loop over lines in message     */
	nTrReq = 0;   /* total number of trace requests from all snippet lines */
	
	if(Debug == 1) 
	    logit("","Calling parseSnippet()\n");

	
	while (parseSnippet(msg_p, &Snppt, &nxtSnippetPtr) == EW_SUCCESS) {
	    /* get next snippet params from msg (level 1)*/
	    if(Debug == 1) 
		logit("","After parseSnippet() in while loop\n");
	    
	    /* create requests implied by this snippet - (might include wildcards) 
	     * routine below will create the request structures. It has access 
	     * to the WaveServer Menu lists.
	     * A little clumsiness: snippet2trReq only fills in the SCNL name
	     * portion of the trace request.  The rest is done in the loop after
	     * the call.
	     */ 

	    oldNTrRq = nTrReq; /* save current value of trace reqeusts */ 
	    rc = snippet2trReq( &MenuList, &Snppt, TraceReq, MaxTraces, &nTrReq ); 
	    if (rc != WS_ERR_NONE) {
		switch ( rc ) {
		case WS_ERR_BUFFER_OVERFLOW:
		    logit("et", "MaxTraces (%d) exceeded. Some traces not saved\n",
			  MaxTraces); 
		    break;
		case WS_ERR_INPUT:
		case WS_ERR_EMPTY_MENU:
		    logit("et", 
			  "Call to snippet2trReq failed: %d \n", rc);
		    goto ShutdownSnippetMaker;
		    break;
		}
	    }

	    if (Debug == 1)
		logit("",
		      "return from snippet2trReq: total of %d requests so far (was %d)\n",
		      nTrReq,oldNTrRq);

	    if (oldNTrRq == nTrReq) /* then this snippet generated no requests. 
				       Suspicious. Log it */
		logit(""," WARNING: in event %s %s %s %s %s was either "
		      "multiply requested, or not found \n",
		      Snppt.eventId,Snppt.sta,Snppt.chan,Snppt.net,Snppt.loc);

	    for (iTrReq = oldNTrRq; iTrReq < nTrReq; iTrReq++) {       
		TraceReq[iTrReq].reqStarttime = Snppt.starttime;
		TraceReq[iTrReq].reqEndtime = Snppt.starttime + Snppt.duration ;
		TraceReq[iTrReq].pBuf = TraceBuffer;
		TraceReq[iTrReq].bufLen = TraceBufferLen;
		TraceReq[iTrReq].timeout = TimeoutSeconds;

		if (Debug ==1) {  /* dump the newly created request */
		    logit("","Request %d:\n <%s.%s.%s.%s> reqStarttime=%lf, "
			  "reqEndtime=%lf, timeout=%ld\n",
			  iTrReq,TraceReq[iTrReq].sta,TraceReq[iTrReq].chan,
			  TraceReq[iTrReq].net, TraceReq[iTrReq].loc,
			  TraceReq[iTrReq].reqStarttime,
			  TraceReq[iTrReq].reqEndtime, TraceReq[iTrReq].timeout);
		}
	    }     /* end of for(iTrReq .. oldNTrRq) loop over requests (level 2) */
        
	    if(Debug == 1)
		logit("","Calling GetInfoFromSnippet\n");

	    if (GetInfoFromSnippet (&Snppt) != EW_SUCCESS) {
		logit ("", "Call to GetInfoFromSnippet failed.\n");
		goto ShutdownSnippetMaker;
	    }
	} /* end of while parseSnippet() loop over lines in message (level 1)*/

	if (SnippetMakerStatus > 0)
	    break;
    
	/* Call the Put Away initializer */
	/* 	
	 * This routine is responsible for initializing the scheme for 
	 * disposing of the trace snippets we're hopefully about to produce 
	 */

	strcat (EventTime, ".00");
	/* if no subnet, use the network name */
	if (strlen(Snppt.subnet) != 0)
	    strcpy(EventSubnet, Snppt.subnet);
	else 
	    strcpy(EventSubnet, Snppt.net);
        
	if(Debug == 1)
	    logit("","calling PA_next_ev(TraceReq = %u, nTrReq=%u,"
		  "Snppt.eventID = %s,  Snppt.author = %s,"
		  "Snppt.subnet = %s, Debug = %u, OriginDate=%s, OriginTime =%s, EventDate=%s, EventTime=%s \n",
		  TraceReq, nTrReq, Snppt.eventId, Snppt.author, 
		  EventSubnet, Debug, OriginDate, OriginTime, EventDate, EventTime );

	if (PA_next_ev (EventID, TraceReq, nTrReq, FormatIndex, 
			OutDir, OriginDate, OriginTime, EventSubnet, 
			Debug) != EW_SUCCESS) {
	    logit("", "Call to PA_next_ev failed; exiting!\n");
	    goto ShutdownSnippetMaker;
	}

	if(Debug == 1) 
	    logit("","returning from PA_next_ev\n");

	/* begin loop over retrieving and disposing of the trace snippets */
	for ( iTrReq = 0; iTrReq < nTrReq; iTrReq++) {
	    rc = CallGetTraceBin(&TraceReq[iTrReq], &MenuList, 
				 TimeoutSeconds, "SnippetMaker", Debug);

	    if(rc == -1) {
		logit("et","trig2disk: SnippetMaker:  Fatal ERROR encountered in "
		      "CallGetTraceBin(). Exiting!\n");
		goto ShutdownSnippetMaker;
	    }
	    else if(rc != 0) {
		if(Debug)
		    logit("","Problem retrieving snippet for (%s,%s,%s,%s) %.2f - %.2f\n",
			  TraceReq[iTrReq].sta, TraceReq[iTrReq].chan, 
			  TraceReq[iTrReq].net, TraceReq[iTrReq].loc,
			  TraceReq[iTrReq].reqStarttime,TraceReq[iTrReq].reqEndtime);
		/* We didn't get anything we can use, so continue to the next snippet. */
		continue;
	    }

	    /* Now call the snippet disposer */
	    if (Debug == 1)
		logit("",
		      "Calling PA_next(Snptt.eventId= %s, %s,%s,%s,%s)\n",
		      Snppt.eventId, TraceReq[iTrReq].sta,TraceReq[iTrReq].chan, 
		      TraceReq[iTrReq].net, TraceReq[iTrReq].loc);  

	    if (PA_next (&(TraceReq[iTrReq]), FormatIndex, 
			 GapThresh, OutBufferLen, Debug) != EW_SUCCESS) {
               logit("", "Call to PA_next failed; skipping.!\n");
               if(Debug)
                   logit("","Problem calling PA_next() for (%s,%s,%s,%s) %.2f - %.2f\n",
                         TraceReq[iTrReq].sta, TraceReq[iTrReq].chan, 
                         TraceReq[iTrReq].net, TraceReq[iTrReq].loc,
                         TraceReq[iTrReq].reqStarttime,TraceReq[iTrReq].reqEndtime);
               continue;
	    }

	    if (Debug == 1)
		logit("","Returned from PA_next: %d\n", rc);
        
	} /* if we got everything */
      
	if (PA_end_ev(FormatIndex, Debug) != EW_SUCCESS) {
	    logit("", "Call to PA_end_ev failed; exiting\n");
	    goto ShutdownSnippetMaker;
	}
	if(Postproc != NULL) {
		char cmdl[512];	/* increased to accomodate large absolute paths */
		logit("","Call to Postproc %s %s %s\n",Postproc,OriginDate,OriginTime);
#ifdef _WINNT
                sprintf(cmdl,"%s %s %s",Postproc,OriginDate,OriginTime);
#else
                sprintf(cmdl,"%s %s %s >/dev/null &",Postproc,OriginDate,OriginTime);
#endif
		system(cmdl);
	}
      
	logit ("", "*** Done processing message (%d bytes) ***\n", msgSize);
      
	sleep_ew(500);
    }  /* while(1) loop forever; the main thread will kill us */
  
 ShutdownSnippetMaker:
    logit("e", "Trig2disk:  Self-Terminating SnippetMaker thread!\n");
    SnippetMakerStatus = -1;
    KillSelfThread();
    return(NULL);  /* should never reach here */

}  /* end SnippetMaker Thread */


/* 
 *  Look into the Snippet, and return strings which may be   
 *  useful in file and directory names: 
 *    EventId, EventTime (maybe more in the future)
 */
int GetInfoFromSnippet (SNIPPET *Snppt)
{
    int i = 0;
    int j = 0;

    if (Snppt == NULL) {
	logit ("e", "Invalid argument passed in; exiting!\n");
	return EW_FAILURE;
    }
		
    /**** EventID *****/
    i = (MAXTXT<EVENTID_SIZE) ? MAXTXT : EVENTID_SIZE;
    strncpy (EventID, Snppt->eventId,i);
    EventID[i-1]='\0';

    /**** EventDate *****/
    strcpy (EventDate, Snppt->startYYYYMMDD);


    /**** EventTime *****/
    /* must strip off : delimiters */
    j = 0;
    for (i = 0; i < TIME_CHARS; i++) {
	if (Snppt->startHHMMSS[i] != ':') {
	    EventTime[j] = Snppt->startHHMMSS[i];
	    j++;
	}
    }
    EventTime[j] = '\0';

    return EW_SUCCESS;
}


static int CallAppendMenu(char * wsIp, char * wsPort, 
			  WS_MENU_QUEUE_REC * pMenuList, int TimeoutSeconds, 
			  char * szCallingFunction, int Debug)
{
    int ret, rc;

    /* CallAppendMenu() has three possible return codes:
       0  Success
       1  Non-Fatal Error.  The call to wsAppendMenu completed with
       warning, or failed, but the failure is not deemed critical
       to the point that we should quit and go home.
       -2  Fatal Error for the current wave_server.  The call to wsAppendMenu 
       completed with failure that impacts the current wave_server only.
       -1  Fatal Error.  We should quit and go home.  Pull the fire
       alarm as you're running down the hallway.
    *******************************************************/

    if (Debug == 1) {
	logit("",
	      "calling wsAppendMenu with wsIp=%s," "wsPort=%s," 
	      "Timout=%d\n", wsIp, wsPort, TimeoutSeconds*1000);
    }
  
    ret = wsAppendMenu(wsIp, wsPort, pMenuList, TimeoutSeconds*1000);
    switch (ret) {
    case WS_ERR_NONE:
	rc = 0;
	break;
    case WS_ERR_INPUT:
	logit("e", "%s: missing input to wsAppendMenu()\n", szCallingFunction);
	rc = -1;
	break;
    case WS_ERR_MEMORY:
	logit("e", "%s: wsAppendMenu failed allocating memory\n", szCallingFunction);
	rc = -1;
	break;
    case WS_ERR_NO_CONNECTION:
	logit("e","%s: wsAppendMenu could not get a connection to %s:%s\n",
	      szCallingFunction,wsIp,wsPort);
	rc = -2;
	break;
    case WS_ERR_SOCKET:
	logit("e", "%s: wsAppendMenu returned socket error for %s:%s\n",
	      szCallingFunction,wsIp,wsPort);
	rc = -2;
	break;
    case WS_ERR_TIMEOUT:
	logit("e","%s: timeout getting menu from %s:%s\n",
	      szCallingFunction,wsIp,wsPort);
	rc = 1;
	break;
    case WS_ERR_BROKEN_CONNECTION:
	logit("e","%s: connection to %s:%s broke during menu\n",
	      szCallingFunction,wsIp,wsPort);
	rc = -2;
	break;
    default:
	logit("e", "%s: wsAppendMenu returned unknown error %d\n",
	      szCallingFunction,ret);
	rc = -1;
    }
  
    return(rc);
}  /* end CallAppendMenu() */
  
  
static int CallGetTraceBin(TRACE_REQ * pTraceReq, WS_MENU_QUEUE_REC * pMenuList, 
			   int TimeoutSeconds, char * szCallingFunction, int Debug)
{
    int ret, rc;
  
    /* Start the return code as success */
    rc = 0;
  
    /* get this trace; rummage through all the servers */
    if(Debug == 1)
	logit("","calling wsGetTraceBin for request (%s,%s,%s,%s) [%.2f-%.2f]\n",
	      pTraceReq->sta,pTraceReq->chan,pTraceReq->net,pTraceReq->loc,
	      pTraceReq->reqStarttime, pTraceReq->reqEndtime);
  
    /* Earthworm location codes in SCNLs are never 0-length strings *
     * so here it means we're dealing with and SCN instead.         */
    if (strlen(pTraceReq->loc) > 0)
	ret = wsGetTraceBinL( pTraceReq, pMenuList, TimeoutSeconds*1000 );
    else
	ret = wsGetTraceBin( pTraceReq, pMenuList, TimeoutSeconds*1000 );
    
    if (Debug == 1) {
	logit("","Retval=%d; actStarttime=%lf, actEndtime=%lf, actLen=%ld\n",
	      ret, pTraceReq->actStarttime, pTraceReq->actEndtime, pTraceReq->actLen);
    }
  
    if (ret != WS_ERR_NONE ) {
	logit(""," problem retrieving %s %s %s %s - ",
	      pTraceReq->sta, pTraceReq->chan, 
	      pTraceReq->net, pTraceReq->loc); 
	switch( ret ) {
	case WS_WRN_FLAGGED:
	    logit("","%s: wave server returned problem flag; continuing.\n", szCallingFunction);
	    rc = 1;
	    break;
	case WS_ERR_SCNL_NOT_IN_MENU:
	    logit("","%s: SCNL not found in menu list; continuing.\n", szCallingFunction);
	    rc = 1;
	    break;
      
	    /* following errors will cause the socket to be closed - exit */
	case WS_ERR_EMPTY_MENU:
	    logit("","%s: ERROR: no menu list found; continuing.\n", szCallingFunction);
	    rc = -2;
	    break;
	case WS_ERR_NO_CONNECTION:
	    logit("","%s: ERROR: socket to wave server already closed!\n", szCallingFunction);
	    rc = -2;
	    break;
	case WS_ERR_BUFFER_OVERFLOW:
	    rc = 1;
	    logit("","%s: ERROR: trace buffer overflow!\n", szCallingFunction);
	    break;
	case WS_ERR_PARSE:
	    rc = 1;
	    logit("","%s: ERROR: Couldn't parse server's reply!\n", szCallingFunction);
	    break;
	case WS_ERR_TIMEOUT:
	    rc = 1;
	    logit("","%s: ERROR: timeout talking to wave server!\n", szCallingFunction);
	    break;
	case WS_ERR_BROKEN_CONNECTION:
	    rc = -2;
	    logit("","%s: ERROR: connection to wave server broken!\n", szCallingFunction);
	    break;
	case WS_ERR_SOCKET:
	    rc = -2;
	    logit("","%s: ERROR: error changing socket options!\n", szCallingFunction);
	    break;
	default:
	    rc = -1;
	    logit("","%s: ERROR: unknown error code %d from wsGetTraceBin()!\n", szCallingFunction, ret);
	}
    }
    if(Debug == 1) {
	if(rc == 0)
	    logit("",
		  "trace %s %s %s %s: went ok first time. Got %ld bytes\n",
		  pTraceReq->sta, pTraceReq->chan,
		  pTraceReq->net, pTraceReq->loc, pTraceReq->actLen); 
    }

    return(rc);

}  /* end CallGetTraceBin() */


/******************** set_origin_date_time() **********************
 *      Set the global variables OriginDate and OriginTime from   *
 *      trigger message msg_p.                                    *
 ******************************************************************/
/* The line to find should be
 * v2.0 EVENT DETECTED     20120201 00:43:09.97 UTC EVENT ID
 * Or
 * EVENT DETECTED     20120201 00:43:09.97 UTC EVENT ID
 */
static void set_origin_date_time( char * msg_p, int msgSize) {
    char TmpOriginTime[MAXTXT + 4];
    char * TRIGGER_REM;
    char *ch;
    int i,j; 

    OriginTime[0] = '\0';

    if ( (TRIGGER_REM = malloc( (size_t) msgSize)) == NULL ) {
	logit("e", 
		"trig2disk: Cannot allocate trigger buffer of %ld bytes. Exiting\n",
		msgSize);
	return ;

    }

    ch = strstr(msg_p, "DETECTED");
    sscanf(ch,"DETECTED     %s %s UTC EVENT ID: %s\n\n",OriginDate,TmpOriginTime,TRIGGER_REM);
    /* must strip off : delimiters */
    j = 0;
    for (i = 0; i < TIME_CHARS; i++) {
	if (TmpOriginTime[i] != ':') {
	    OriginTime[j] = TmpOriginTime[i];
	    j++;
	}
    }
    OriginTime[j] = '\0';
    strcat(OriginTime,".00");

    free(TRIGGER_REM);
}

