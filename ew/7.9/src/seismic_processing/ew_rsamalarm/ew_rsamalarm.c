/******************************************************************************
 *
 *	File:			ew_rsamalarm.c
 *
 *	Function:		Program to read TRACEBUF2X messages and report (via a
 *                  message to another ring) when events (station values
 *					exceeding a threshold for long enough) and alarmStart
 *					(when enough events are concurrent) by writing message(s)
 *					to a specified ring.
 *
 *	Author(s):		Scott Hunter, ISTI
 *
 *	Source:			Started anew.
 *
 *	Notes:
 *
 *	Change History:
 *		01/03/12	Started source
 *      10/01/14    Corrected logit re-initialisation with MaxMsgSize value
 *      27/01/14    Fixed buffer overflow & improved message in WriteAlarmMessage
 *      11/02/15    Better fix to buffer overflow
 *      10/03/16    Bigger buffer; better handling of log & message overflow
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time_ew.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <mem_circ_queue.h>
#include <trace_buf.h>
#include <swap.h>

#define TRIGGER_PERIOD_INDEX 5
#define TREMOR_PERIOD_INDEX 1
#define EVENT_PERIOD_INDEX 0

// MAX_LOG_MSG_LEN must be at least 256
#define MAX_LOG_MSG_LEN 65536

#define VERSION  "1.1.1 2016.09.28"

#define nDEBUG
#ifdef DEBUG
#define dbpf printf
#else
#define dbpf( fmt, ... ) ( (void) 0 )
#endif

/* Period, in seconds, of windows */
double WIN_SIZE[6] = { 2.56, 10.0, 0, 0, 0, 2.56 };

/* 
Trigger requirements for a single station.

A station triggers when 'enabled' is true and alarm criteria have
been met for 'time' continous seconds. A trigger ends when alarm
criteria are not met, or when the trigger has lasted 'timeout'
continous seconds. 

Alarm criteria:
	RSAM value above 'threshold'
	Ratio of most recent non-trigger RSAM value to current RSAM value is
above 1:'ratio'

When trigger that ends by exceeding its 'timeout', the next RSAM value
will become the new non-trigger RSAM value. This results in a station
re-triggering only if rsam values continue to increase.
*/
typedef struct {
	double	threshold;
	double	ratio;
	double	time;
	double	timeout;
	char    enabled;
} STATION_WINDEF;

/* Dynamic information about a station's window */
typedef struct {
	char 	inEvent;
	char	nData;
	int 	preEventTriggerValue;
	int 	lastValue;
	int 	firstValue;
	double	triggerTime;
	double	eventSum;
	double	eventMax;
	int		eventCount;
} STATION_WINDATA;

/* Static information about a station */
typedef struct {
	int		*subList;
	STATION_WINDATA windata[6];
	union {
		TRACE_HEADER scn;
		TRACE2_HEADER scnl;
	} hdr;
	char type2;
	STATION_WINDEF windef[6];
	char defined;
	int  subnetCount;
	char inhibitor;
} STATION_DEF;

/* 
Alarm information for a subnet.

A subnet will alarm when 'enabled' is true, and at least 'minForAlarm'
stations have triggered. An alarm will end when fewer than 'minForAlarm'
stations remain in a triggered state, or when 'resetTime' seconds have
passed. 

An alarm that ends on resetTime is likely to transition back into an
alarm statne on the next pass. That is expected and will generate a
second alarm.

*/
typedef struct {
	int minForAlarm;
	char enabled;
	int resetTime;
} SUBNET_WINDEF;

/* Dynamic information about a subnet window */
typedef struct {
	int		nInEvent;
	char 	inAlarm;
	char	inhibited;
	char	alarmWritten;
	double	alarmStart;
} SUBNET_WINDATA;

/* Static information about a subnet */
typedef struct {
	int 	*staList;
	int		nSta, nStaAlloc;
	SUBNET_WINDATA windata[6];
	SUBNET_WINDEF windef[6];
	char 	name[50];
	int		CheckInhibitStationsDelay;
	char    defined;
} SUBNET_DEF;
	
/* A sample */
typedef struct {
	int		scnl_idx;
	int		period_idx;
	int		datum;
	double	starttime;
	double	endtime;
} SAMPLE_DEF;

/* Functions in this source file
 *******************************/
void  ew_rsamalarm_config  ( char );
void  ew_rsamalarm_lookup  ( void );
void  ew_rsamalarm_status  ( unsigned char, short, char * );
void  ew_rsamalarm_free    ( void );
static void fillStaName( char *name, STATION_DEF sta );

STATION_WINDEF windef_defaults[5];

/* Thread things
 ***************/
#define THREAD_STACK 8192
static unsigned tidStacker;          /* Thread moving messages from transport */
                                     /*   to queue */

#define MSGSTK_OFF    0              /* MessageStacker has not been started      */
#define MSGSTK_ALIVE  1              /* MessageStacker alive and well            */
#define MSGSTK_ERR   -1              /* MessageStacker encountered error quit    */
volatile int MessageStackerStatus = MSGSTK_OFF;

QUEUE OutQueue;              /* from queue.h, queue.c; sets up linked    */
                                     /*    list via malloc and free              */
thr_ret MessageStacker( void * );    /* used to pass messages between main thread */
                                     /*   and Process thread */
thr_ret Process( void * );

/* Message Buffers to be allocated
 *********************************/
/*static MSG_LOGO Logo;        * logo of message to re-send */
static char *MSrawmsg = NULL;        /* MessageStacker's "raw" retrieved message */

/* Timers
   ******/
time_t now;        /* current time, used for timing heartbeats */
time_t MyLastBeat;         /* time of last local (into Earthworm) hearbeat */

extern int  errno;

static  SHM_INFO  InRegion;     /* shared memory region to use for input  */
static  SHM_INFO  OutRegion;    /* shared memory region to use for output */

#define   MAXLOGO   10
MSG_LOGO  GetLogo[MAXLOGO];     /* array for requesting module,type,instid */
short     nLogo;

static char *configfile;
char *Argv0;            /* pointer to executable name */
pid_t MyPid;        /* Our own pid, sent with heartbeat for restart purposes */
static unsigned tidProcess;

mutex_t reconfig_mutex;

/* Things to read or derive from configuration file
 **************************************************/
static char    InRing[MAX_RING_STR];          /* name of transport ring for input  */
static char    OutRing[MAX_RING_STR];         /* name of transport ring for output */
static char    MyModName[MAX_MOD_STR];        /* speak as this module name/id      */
static int     LogSwitch;           /* 0 if no logfile should be written */
static int     HeartBeatInt;        /* seconds between heartbeats        */
static long    MaxMsgSize;          /* max size for input/output msgs    */
static int     RingSize = 10;        /* max messages in output circular buffer       */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          InRingKey;     /* key of transport ring for input    */
static long          OutRingKey;    /* key of transport ring for output   */
static unsigned char InstId;        /* local installation id              */
static unsigned char InstWildcard;
static unsigned char MyModId;       /* Module Id for this program         */
static unsigned char ModWildcard;
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char Type_Tracebuf1;
static unsigned char Type_Tracebuf2;
static unsigned char TypeActivate;
static unsigned char TypeRsamEvent;
static unsigned char TypeRsamAlarm;

/* Error messages used by export
 ***********************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring        */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer      */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded  */
static char  errText[256];     /* string for log/error messages           */


STATION_DEF *staDef = NULL;
int nSta = 0, nStaAlloc = 0;

SUBNET_DEF *subDef= NULL;
int nSub = 0, nSubAlloc = 0;

static STATION_DEF	currSta;
static int			currStaID = -1;
static char 		stationChanged;
static char			inStationDef = 0;
static SUBNET_DEF	currSub;
static int			currSubID = -1;
static char			inSubnetDef = 0;

int main( int argc, char **argv )
{
/* Other variables: */
   int           res;
   long          recsize;   /* size of retrieved message             */
   MSG_LOGO      reclogo;   /* logo of retrieved message             */
   int i;

   /* Check command line arguments
   ******************************/
   Argv0 = argv[0];
   if ( argc != 2 )
   {
      fprintf( stderr, "Usage: %s <configfile>\n", Argv0 );
	  fprintf (stderr, "Version: %s\n", VERSION);
	  return EW_FAILURE;
   }

   /* Initialize name of log-file & open it
   ****************************************/
   logit_init( argv[1], 0, MAX_LOG_MSG_LEN, 1 );

   /* Initialize list of station defs (first will contains defaults)
   *****************************************************************/
   nStaAlloc = 10;
   staDef = malloc( nStaAlloc * sizeof( STATION_DEF ) );
   memset( staDef, 0, sizeof(currSta) );
   nSta = 1; 
   memset( &currSub, 0, sizeof(currSub) );

   /* Read the configuration file(s)
   ********************************/
   configfile = argv[1];
   ew_rsamalarm_config( 0 );
   logit( "t" , "%s(%s): Read command file <%s>\n",
           Argv0, MyModName, configfile );

   /* Look up important info from earthworm.h tables
   *************************************************/
   ew_rsamalarm_lookup();

   /* Reinitialize the logging level
   *********************************/
   logit_init( argv[1], 0, MAX_LOG_MSG_LEN, LogSwitch );

   /* Get our own Pid for restart purposes
   ***************************************/
   MyPid = getpid();
   if(MyPid == -1)
   {
      logit("e", "%s(%s): Cannot get pid; exiting!\n", Argv0, MyModName);
      return(0);
   }


   /* Allocate space for input/output messages for all threads
   ***********************************************************/

   /* Buffers for the MessageStacker thread: */
   if ( ( MSrawmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL )
   {
      logit( "e", "%s(%s): error allocating MSrawmsg; exiting!\n",
             Argv0, MyModName );
      ew_rsamalarm_free();
      return( -1 );
   }
   
   /* Attach to Input/Output shared memory ring
   ********************************************/
   tport_attach( &InRegion, InRingKey );
   tport_attach( &OutRegion, OutRingKey );

  /* Specify logos to get
  ***********************/
  nLogo = 3;
  if ( GetType( "TYPE_TRACEBUF", &Type_Tracebuf1 ) != 0 ) {
     fprintf(stderr, "%s: Invalid message type <TYPE_TRACEBUF>!\n", argv[0] );
     exit( -1 );
  }
  if ( GetType( "TYPE_TRACEBUF2", &Type_Tracebuf2 ) != 0 ) {
     fprintf(stderr, "%s: Invalid message type <TYPE_TRACEBUF2>!\n", argv[0] );
     exit( -1 );
  }
  if ( GetModId( "MOD_WILDCARD", &ModWildcard ) != 0 ) {
     fprintf(stderr, "%s: Invalid moduleid <MOD_WILDCARD>!\n", argv[0] );
     exit( -1 );
  }
  if ( GetInst( "INST_WILDCARD", &InstWildcard ) != 0 ) {
     fprintf(stderr, "%s: Invalid instid <INST_WILDCARD>!\n", argv[0] );
     exit( -1 );
  }

  for( i=0; i<nLogo; i++ ) {
      GetLogo[i].instid = InstWildcard;
      GetLogo[i].mod    = ModWildcard;
  }
  GetLogo[0].type = Type_Tracebuf1;
  GetLogo[1].type = Type_Tracebuf2;
  GetLogo[2].type = TypeActivate;
  
   /* step over all messages from transport ring
   *********************************************/
   /* As Lynn pointed out: if we're restarted by startstop after hanging,
      we should throw away any of our messages in the transport ring.
      Else we could end up re-sending a previously sent message, causing
      time to go backwards... */
   do
   {
     res = tport_getmsg( &InRegion, GetLogo, nLogo,
                         &reclogo, &recsize, MSrawmsg, MaxMsgSize );
   } while (res !=GET_NONE);

   /* One heartbeat to announce ourselves to statmgr
   ************************************************/
   ew_rsamalarm_status( TypeHeartBeat, 0, "" );
   time(&MyLastBeat);


   /* Start the message stacking thread if it isn't already running.
    ****************************************************************/
   if (MessageStackerStatus != MSGSTK_ALIVE )
   { 
     if ( StartThread(  MessageStacker, (unsigned)THREAD_STACK, &tidStacker ) == -1 )
     {
       logit( "e",
              "%s(%s): Error starting  MessageStacker thread; exiting!\n",
          Argv0, MyModName );
       tport_detach( &InRegion );
       tport_detach( &OutRegion );
       return( -1 );
     }
     MessageStackerStatus = MSGSTK_ALIVE;
   }

   /* Create a Mutex to control access to queue
   ********************************************/
   CreateMutex_ew();
   CreateSpecificMutex(&reconfig_mutex);

   /* Initialize the message queue
   *******************************/
   initqueue( &OutQueue, (unsigned long)RingSize,(unsigned long)MaxMsgSize+1 );

   /* Start the socket writing thread
   ***********************************/
   if ( StartThread(  Process, (unsigned)THREAD_STACK, &tidProcess ) == -1 )
   {
	  logit( "e", "%s(%s): Error starting Process thread; exiting!\n",
			  Argv0, MyModName );
	  tport_detach( &InRegion );
	  tport_detach( &OutRegion );
	  exit( -1 );
   }
   
   /* Start main ew_rsamalarm service loop
   **********************************/
   while( tport_getflag( &InRegion ) != TERMINATE  &&
          tport_getflag( &InRegion ) != MyPid         )
   {
     /* Beat the heart into the transport ring
      ****************************************/
      time(&now);
      if (difftime(now,MyLastBeat) > (double)HeartBeatInt )
      {
          ew_rsamalarm_status( TypeHeartBeat, 0, "" );
      time(&MyLastBeat);
      }

      /* take a brief nap; added 970624:ldd
       ************************************/
      sleep_ew(500);
   } /*end while of monitoring loop */

   /* Shut it down
   ***************/
   tport_detach( &InRegion );
   tport_detach( &OutRegion );
   ew_rsamalarm_free();
   logit("t", "%s(%s): termination requested; exiting!\n",
          Argv0, MyModName );
   return( 0 );
}
/* *******************  end of main *******************************
 ******************************************************************/

/*****************************************************************************
 *  k_bool(): Interpret next k_str as a boolean                              *
 *****************************************************************************/
int k_bool() {
	return (strcasecmp( k_str(), "False" ) ? 1 : 0);
}

/*****************************************************************************
 * WriteEventMessage(): Write event message for specified station's window   * 
 * Events are written when a station trigger ends			     *
 *****************************************************************************/
void WriteEventMessage( int sta_idx, int win_idx ) {
	STATION_DEF *sta = staDef+sta_idx;
	STATION_WINDATA *win = sta->windata + win_idx;
	time_t when = win->triggerTime;
	struct tm res;
	char msg[1000], name[20];
	MSG_LOGO    logo;
	long        size;

	fillStaName( name, *sta );
	gmtime_ew( &when, &res );
	sprintf (msg, "%4d-%02d-%02d %02d:%02d:%02d %s %lf %lf %lf\n", 
		(res.tm_year + TM_YEAR_CORR), (res.tm_mon + 1), res.tm_mday, 
		res.tm_hour, res.tm_min, res.tm_sec,
		name, win->eventMax, win->eventSum / win->eventCount, 
		win->eventCount * WIN_SIZE[win_idx] );
		
	logit( "", msg );

	logo.instid = InstId;
	logo.mod    = MyModId;
	logo.type   = TypeRsamEvent;

	size = strlen( msg ) - 1; /* Leave off trailing \n */
	if( tport_putmsg( &OutRegion, &logo, size, msg ) != PUT_OK )
		logit("et","%s(%s):  Error writing end-of-event message.\n",
                  Argv0, MyModName );
    else
    	dbpf( "Logged: %s", msg );

}

/*****************************************************************************
 * WriteAlarmMessage(): Write alarm message for specified subnet's window    *
 * for specified time (unless inhibited)                                     *
 *****************************************************************************/
void WriteAlarmMessage( SUBNET_DEF *sub, int win_idx, time_t when, char inhibited ) {
	char *msgEnd;
	struct tm res;
	int i;
	long contOff;
	long size, hdrSize;
	char *msg;
	char *logBodyStart;
	long bufferSize = MAX_LOG_MSG_LEN;
	
	if ( !sub->windef[win_idx].enabled ) {
		dbpf("Skipping alarm message; window disabled\n");
		return;
	}
	
	if ( bufferSize < MaxMsgSize )
	    bufferSize = MaxMsgSize;
	msg = malloc( bufferSize+200 );
	if ( msg==NULL ) {
	    logit( "et", "Allocation failure when writing alarm\n" );
	    return;
	}
	gmtime_ew( &when, &res );
	sprintf (msg, "%s %sR%d %4d-%02d-%02d %02d:%02d:%02d ",
		sub->name, inhibited ? "INHIBITED " : "", win_idx,
		(res.tm_year + TM_YEAR_CORR), (res.tm_mon + 1), res.tm_mday, 
		res.tm_hour, res.tm_min, res.tm_sec );
	contOff = strlen(msg);
	msgEnd = logBodyStart = msg + contOff;
	strcpy(msgEnd, "      \nStation            PreTrigVal     Start    Thresh In/Out\n" );
	hdrSize = size = strlen(msg);
	logBodyStart = msgEnd = msg + size;

	for ( i=0; i<sub->nSta; i++ ) {
	    char* oldMsgEnd = msgEnd;
		int si = sub->staList[i];
        if ( !inhibited && staDef[si].inhibitor )
            continue;
        strcpy( msgEnd, "                    " );
        fillStaName( msgEnd, staDef[si] );
        msgEnd[strlen(msgEnd)] = ' ';
        msgEnd += 20;
        sprintf( msgEnd, "%9d %9d %9.0f %3s\n",
            staDef[si].windata[win_idx].preEventTriggerValue,
            staDef[si].windata[win_idx].firstValue,
            staDef[si].windef[win_idx].threshold, 
            staDef[si].windata[win_idx].inEvent ? "in" : "out");
        msgEnd += strlen( msgEnd );
		if ( msgEnd - logBodyStart + hdrSize >= MAX_LOG_MSG_LEN ) {
		    // Write out what we have
		    char tmp = oldMsgEnd[0], tmp2 = msg[hdrSize-1];
		    oldMsgEnd[0] = 0;
		    msg[hdrSize-1] = 0;
		    logit( "", "%s\n%s\n", msg, logBodyStart );
		    // Fix header, mark start of next log
		    msg[hdrSize-1] = tmp2;
		    oldMsgEnd[0] = tmp;
		    strncpy( msg+contOff, "(cont)", 6 );
		    logBodyStart = oldMsgEnd;
		}
		if ( msgEnd - msg > bufferSize )
		    break;
	}
	
	if ( i <sub->nSta && msgEnd - msg > bufferSize ) {
	    strcpy( msgEnd, "*** TRUNCATED ***" );
	    msgEnd += 18;
	}
    if ( *logBodyStart ) {
        char tmp = msgEnd[0], tmp2 = msg[hdrSize-1];
        msgEnd[0] = 0;
        msg[hdrSize-1] = 0;
        logit( "", "%s\n%s\n", msg, logBodyStart );
        // Fix header, mark start of next log
        msg[hdrSize-1] = tmp2;
        msgEnd[0] = tmp;
    }
	sub->windata[win_idx].alarmWritten = 1;
    strncpy( msg+contOff, "      ", 6 );
	if ( !inhibited ) {
		MSG_LOGO    logo;
		logo.instid = InstId;
		logo.mod    = MyModId;
		logo.type   = TypeRsamAlarm;
	
		size = msgEnd - msg - 1; /* Leave off trailing \n */
		if( tport_putmsg( &OutRegion, &logo, size, msg ) != PUT_OK )
			logit("et","%s(%s):  Error writing alarm message; check log for its contents.\n",
					  Argv0, MyModName );
		else
			printf( "Logged: %s\n", msg );
		dbpf( "'Writing' uninhibited alarm\n");
	} else
		dbpf( "'Writing' inhibited alarm\n");
}


/********************** Message Stacking Thread *******************
 *           Move messages from transport to memory queue         *
 ******************************************************************/
thr_ret MessageStacker( void *dummy )
{
   long          recsize;   /* size of retrieved message             */
   MSG_LOGO      reclogo;       /* logo of retrieved message             */
   int       res;
   int       ret;
   TRACE2X_HEADER  	*tbh2x = (TRACE2X_HEADER*)MSrawmsg;
   TRACE_HEADER		*tbh = (TRACE_HEADER*)MSrawmsg;
   int				*long_data  =  (int *)( MSrawmsg + sizeof(TRACE2X_HEADER) );
   short			*short_data = (short *)( MSrawmsg + sizeof(TRACE2X_HEADER) );
   int				data_size;
   int 				i, scnl_idx;
   long nsamp;
   SAMPLE_DEF	outMsg;

   /* Tell the main thread we're ok
   ********************************/
   MessageStackerStatus = MSGSTK_ALIVE;

   /* Start main export service loop for current connection
   ********************************************************/
   while( 1 )
   {

      /* Get a message from transport ring
      ************************************/
      res = tport_getmsg( &InRegion, GetLogo, nLogo,
                          &reclogo, &recsize, MSrawmsg, MaxMsgSize );

      /* Wait if no messages for us
       ****************************/
      if( res == GET_NONE ) {sleep_ew(100); continue;}

      /* Check return code; report errors
      ***********************************/
      if( res != GET_OK )
      {
         if( res==GET_TOOBIG )
         {
            sprintf( errText, "msg[%ld] i%d m%d t%d too long for target",
                            recsize, (int) reclogo.instid,
                (int) reclogo.mod, (int)reclogo.type );
            ew_rsamalarm_status( TypeError, ERR_TOOBIG, errText );
            continue;
         }
         else if( res==GET_MISS )
         {
            sprintf( errText, "missed msg(s) i%d m%d t%d in %s",(int) reclogo.instid,
                    (int) reclogo.mod, (int)reclogo.type, InRing );
            ew_rsamalarm_status( TypeError, ERR_MISSMSG, errText );
         }
         else if( res==GET_NOTRACK )
         {
            sprintf( errText, "no tracking for logo i%d m%d t%d in %s",
                     (int) reclogo.instid, (int) reclogo.mod, (int)reclogo.type,
                     InRing );
            ew_rsamalarm_status( TypeError, ERR_NOTRACK, errText );
         }
      }

     /* Process retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK)
      ***********************************************************/

	  if ( reclogo.type == TypeActivate ) {
	  		/* Request for reconfig */
			RequestMutex();
			outMsg.scnl_idx = -1;
			ret=enqueue( &OutQueue, (void*)&outMsg, sizeof(outMsg), reclogo );
			ReleaseMutex_ew();
			RequestSpecificMutex(&reconfig_mutex);
			ReleaseSpecificMutex(&reconfig_mutex);
			continue;
	  }
	  	
      /* First, localize & make sure it is a TRACEBUF(2) message
      ***********************************************/
      if ( WaveMsg2XMakeLocal(tbh2x) != 0) 
         continue;

      if ( tbh2x->datatype[1] == '2' )
         data_size = 2;
      else if ( tbh2x->datatype[1] == '4' )
         data_size = 4;

      /* Next, see if it matches one of our stations
      *********************************************/
      for ( scnl_idx=0; scnl_idx<nSta; scnl_idx++ ) {
      	int off;
      	char *period_txt;
      	
      	if ( staDef[scnl_idx].type2 ) {
      		//dbpf("Rejecting message: bad type\n");
      		continue;
      	} 
		if ( strcmp( staDef[scnl_idx].hdr.scn.sta,  tbh->sta ) ) {
			/*dbpf("Rejecting message(%d): STA (%s vs %s)\n", scnl_idx, staDef[scnl_idx].hdr.scn.sta,  tbh->sta );*/
			continue;
		}
		if ( strcmp( staDef[scnl_idx].hdr.scn.net,  tbh->net ) ) {
			dbpf("Rejecting message(%d): NET (%s vs %s)\n", scnl_idx, staDef[scnl_idx].hdr.scn.net,  tbh->net );
			continue;
		}
		off = strlen(staDef[scnl_idx].hdr.scn.chan);
		i = strncmp( staDef[scnl_idx].hdr.scn.chan, tbh->chan, off );
		if ( i ) {
			dbpf("Rejecting message(%d): CHAN (%s vs %s)\n", scnl_idx, staDef[scnl_idx].hdr.scn.chan,  tbh->chan );
			continue;
		}
		
		if ( reclogo.type == Type_Tracebuf1 ) {
			if ( tbh->chan[off] != '_' ) {
				dbpf("Rejecting message(%d): TB1 CHAN (%d:'%c'-%d)\n", scnl_idx, off, tbh->chan[off], tbh->chan[off] );
				continue;
			}
			period_txt = tbh->chan+off+1;
		} else {
			period_txt = tbh2x->loc;
		}

		if ( period_txt[0] != 'R' ) {
			dbpf("Rejecting message(%d): Not R (%c)\n", scnl_idx, period_txt[0] );
			continue;
		} 
		if ( period_txt[1] < '0' || period_txt[1] > '4' ) {
			dbpf("Rejecting message(%d): Not 0-4 (%c)\n", scnl_idx, period_txt[1] );
			continue;
		}
		if ( (period_txt[1]=='0' && staDef[scnl_idx].windef[5].enabled) ||
					staDef[scnl_idx].windef[period_txt[1]-'0'].enabled )
			nsamp = tbh->nsamp;
		else
			continue;
		if ( nsamp != 1 ) {
			char name[20];
			fillStaName( name, staDef[scnl_idx] );
			logit("t","%s: RSAM message from %s w/ %ld samples (expected 1); using only first\n",Argv0,name,nsamp);
		}
	    outMsg.starttime = tbh->starttime;
	    outMsg.endtime = tbh->endtime;
	    outMsg.period_idx = period_txt[1]-'0';
		break;
	  }
      if ( scnl_idx >= nSta ) {
      	//dbpf("Rejecting message(%d) %s %s %s\n", scnl_idx, tbh->sta, tbh->net, tbh->chan );
      	continue;
      }
	  outMsg.scnl_idx = scnl_idx;
	  
	  /*dbpf( "Stacking message from %s:%s:%s (%f) %c\n", tbh->sta, tbh->chan, tbh->net, tbh->samprate,
	  			staDef[scnl_idx].inhibitor ? 'I' : '.' );*/


      /* Put datum in message & send to Process
      *******************************************/
	  if ( data_size == 2 )
	  {
	  	outMsg.datum = short_data[0];
	  } else {
	 	outMsg.datum = long_data[0];
	  }
      RequestMutex();
      ret=enqueue( &OutQueue, (void*)&outMsg, sizeof(outMsg), reclogo );
      ReleaseMutex_ew();

	  /* If converted max exceeds threshold, spit out a message
	  ************************************************
	  if (  max*convFactor > threshDef[scnl_idx].threshold ) {
	  	  char msg[100];
	  	  struct tm theTimeStruct;
	  	  time_t theTime = tbh2x->starttime + maxi * (1.0/tbh2x->samprate);

	  	  gmtime_ew( &theTime, &theTimeStruct );

	  	  sprintf( msg, "SNCL=%s.%s.%s.%s Thresh=%1.1lf Value=%lf Time=%s%c",
	  	  	tbh2x->sta, tbh2x->net, tbh2x->chan, tbh2x->loc,
	  	  	threshDef[scnl_idx].threshold, max*convFactor, asctime( &theTimeStruct ), 0 );
	      reclogo.type = ThreshAlarmType;  * note the new message type *
		  if ( tport_putmsg( &OutRegion, &reclogo, strlen(msg)+1, msg ) != PUT_OK ) {
		  	  logit("et", "%s:  Error writing threshold message to ring.\n",
					  Argv0 );
		  }
		  if ( tvNumTrig > 0 )
			  tvAddAlarm( theTime );
	  } */

   } /* end of while */

   /* we're quitting
   *****************/
   MessageStackerStatus = MSGSTK_ERR; /* file a complaint to the main thread */
   KillSelfThread(); /* main thread will restart us */
   return(NULL);
}

/*****************************************************************************
 * StationEntersEvent(): Start an event specified by the given sample        *
 *****************************************************************************/
void StationEntersEvent( SAMPLE_DEF msg ) {
	char name[20];
	int i;

	fillStaName( name, staDef[msg.scnl_idx] );
	staDef[msg.scnl_idx].windata[msg.period_idx].inEvent = 1;
	dbpf( "%-20s starts event(%d) %c\n", name, msg.period_idx, staDef[msg.scnl_idx].inhibitor ? 'I' : '.' );
	for ( i=0; i<staDef[msg.scnl_idx].subnetCount; i++ ) {
		SUBNET_DEF *sub = &subDef[staDef[msg.scnl_idx].subList[i]];
		char alarmOn;
		char trippedAlarm = 0;
		if ( !staDef[msg.scnl_idx].inhibitor ) {
			sub->windata[msg.period_idx].nInEvent++;
			if ( !sub->windata[msg.period_idx].inAlarm ) {
				alarmOn = (sub->windata[msg.period_idx].nInEvent == sub->windef[msg.period_idx].minForAlarm);
				dbpf("alarmOn: %d %d of %d = %c\n", msg.period_idx, sub->windata[msg.period_idx].nInEvent, sub->windef[msg.period_idx].minForAlarm, alarmOn );
				if ( alarmOn ) {
					sub->windata[msg.period_idx].inAlarm = 1;
					dbpf("InAlarm: sub[%d].win[%d] @ %lf\n", staDef[msg.scnl_idx].subList[i], msg.period_idx, msg.starttime );
					sub->windata[msg.period_idx].alarmStart = msg.starttime;
					trippedAlarm = 1;
				} else
					dbpf("\tSubnet+ %s[%d] (%d:%d)%s\n", sub->name, msg.period_idx, sub->windata[msg.period_idx].nInEvent, 
					  sub->windef[msg.period_idx].minForAlarm,
					  alarmOn ? " ALARM ON" : "--");
			}
		} else {
			/* New inhibitor; check active alarms */
			int wi;
			dbpf("Checking inhibitor after inAlarm\n");
			for ( wi=0; wi<6; wi++ ) {
				if ( sub->windata[wi].inAlarm ) {
					dbpf( "sub[%d].win[%d]: %lf <? %lf (%lf + %d)\n",
						staDef[msg.scnl_idx].subList[i], wi, 
						msg.starttime, sub->windata[wi].alarmStart + sub->CheckInhibitStationsDelay,
						sub->windata[wi].alarmStart, sub->CheckInhibitStationsDelay );
					if ( msg.starttime < sub->windata[wi].alarmStart + sub->CheckInhibitStationsDelay ) 
						WriteAlarmMessage( sub, wi, sub->windata[wi].alarmStart, 1 );
				}
			}
			continue;
		}
		if ( trippedAlarm ) {
			int si,wi, xi;
			for ( wi=0; wi<6; wi++ )
				if ( sub->windata[wi].inhibited ) {
					trippedAlarm = 0;
					break;
				} else if ( sub->windata[wi].inAlarm ) {
					for ( si=0; si<sub->nSta; si++ ) {
						if ( !staDef[sub->staList[si]].inhibitor )
							continue;
						if ( staDef[sub->staList[si]].windata[wi].triggerTime < sub->windata[wi].alarmStart + sub->CheckInhibitStationsDelay
							) {
								trippedAlarm = 0;
								for ( xi=0; xi<6; xi++ )
									sub->windata[xi].inhibited = 1;
								wi = 6; // force break out of outer loop
								break;
						}
					}
				} 
			if ( !trippedAlarm ) {
				WriteAlarmMessage( sub, msg.period_idx, sub->windata[msg.period_idx].alarmStart, 1 );
			}
		}
		if ( trippedAlarm ) {
			WriteAlarmMessage( sub, msg.period_idx, sub->windata[msg.period_idx].alarmStart, 0 );
		}
	}
}

/*****************************************************************************
 * StationExitsEvent(): Finish an event specified by the given sample        *
 *****************************************************************************/
void StationExitsEvent( SAMPLE_DEF msg ) {
	int i;
	STATION_DEF *sta = staDef + msg.scnl_idx;
	int period_idx = msg.period_idx;
	char inhibitor =  sta->inhibitor;

	/*
	char name[20];	
	fillStaName( name, staDef[msg.scnl_idx] );
	dbpf( "%-20s  ends event(%d)\n", name, msg.scnl_idx );
	
	*/
	WriteEventMessage( msg.scnl_idx, msg.period_idx );
	
	for ( i=0; i<sta->subnetCount; i++ ) {
		char alarmOff;
		int subID = sta->subList[i];
		SUBNET_DEF *sub = &subDef[subID];
		if ( inhibitor ) {
			/* If subnet is inhibited, no longer is */
			if ( sub->windata[period_idx].inhibited ) {
				dbpf("Inhibitor exits event\n");
				sub->windata[period_idx].inhibited = 0;
			}
		} else {
			sub->windata[period_idx].nInEvent--;
			alarmOff = (sub->windata[period_idx].nInEvent+1 == sub->windef[period_idx].minForAlarm);
			dbpf("\tSubnet- %s[%d] (%d:%d)%s\n", sub->name, period_idx, sub->windata[period_idx].nInEvent,
				 sub->windef[period_idx].minForAlarm,
				 alarmOff ? " ALARM OFF" : "");
			if ( sub->windata[msg.period_idx].inAlarm && alarmOff ) {
				dbpf("Resetting an alarm\n");
				sub->windata[period_idx].inAlarm = 0;
				sub->windata[period_idx].alarmWritten = 0;
			}

		}
	}
}

/*****************************************************************************
 * ProcessWindow(): Add the sample to the matching window                    *
 *****************************************************************************/
void ProcessWindow( SAMPLE_DEF msg ) {
   	char in_leadup = 0;
	STATION_DEF *sta = staDef + msg.scnl_idx;
	STATION_WINDATA *dat = sta->windata + msg.period_idx;
	STATION_WINDEF *def = sta->windef + msg.period_idx;
	char name[20];

	fillStaName( name, staDef[msg.scnl_idx] );

    /* If we haven't had enough data yet, remember this */
	if ( dat->nData < 2) {
		dat->preEventTriggerValue = dat->lastValue;
		dat->lastValue = msg.datum;
		dat->nData++;
		return;
	}

	dbpf( "%s(%d) l=%d p=%d d=%d ie=%d \n", name, msg.period_idx, 
		dat->lastValue, dat->preEventTriggerValue, msg.datum, dat->inEvent ); 
	if ( msg.datum > def->threshold && msg.datum > def->ratio * dat->preEventTriggerValue ) {
		/* triggered */

		if (dat->triggerTime <= 0 )
			dat->triggerTime = msg.starttime;
	
		if (msg.starttime - dat->triggerTime > def->timeout) { 
			/* stop trigger overtime */
			if ( dat->inEvent )
				StationExitsEvent( msg );

			dat->inEvent = 0;
			dat->triggerTime = 0;
			dbpf("%s(%d) timesout (max=%f, avg=%f) t=%f\n", name, msg.period_idx,
				dat->eventMax, dat->eventSum/dat->eventCount, msg.starttime - dat->triggerTime );
		} else if (!dat->inEvent) {
			if ( msg.starttime - dat->triggerTime > def->time) { 
				/* start trigger */
				StationEntersEvent( msg );
				dat->inEvent = 1;
				dat->eventSum = 0;
				dat->eventCount = 0;
				dat->eventMax = msg.datum;
				dat->firstValue = msg.datum;
				dbpf("%s(%d) in event\n", name, msg.period_idx );
			} else {
				in_leadup = 1;
			}
		}
	} else {
		/* Not triggered */
		if ( dat->inEvent ) {
			StationExitsEvent( msg );
			dat->inEvent = 0;

			dbpf("%s(%d) untriggers (max=%f, avg=%f) thr=%lf r=%lf t=%f\n", name, msg.period_idx,
				dat->eventMax, dat->eventSum/dat->eventCount, 
				def->threshold, def->ratio,
				msg.starttime - dat->triggerTime );
		} 
		dat->triggerTime = 0; // reset triggerTime in case trigger ended in leadup
	}
     
	if ( dat->inEvent ) {
		int i;

		dat->eventSum += msg.datum;
		dat->eventCount++;
		if ( msg.datum > dat->eventMax )
			dat->eventMax = msg.datum;
		if ( sta->inhibitor ) {
			for ( i=0; i<sta->subnetCount; i++ ) {
				int subID = sta->subList[i];
				SUBNET_DEF *sub = &subDef[subID];
				int w;
				for ( w=0; w<6; w++ )
					if ( sub->windata[w].inAlarm && !sub->windata[w].alarmWritten ) {
						dbpf( "Checking sub[%d].win[%d] for delayed alarm %lf >= %lf (%lf + %d)\n",
							subID, w, msg.starttime, sub->windata[w].alarmStart + sub->CheckInhibitStationsDelay,
							sub->windata[w].alarmStart, sub->CheckInhibitStationsDelay );
						if ( msg.starttime >= sub->windata[w].alarmStart + sub->CheckInhibitStationsDelay )
							WriteAlarmMessage( sub, w, sub->windata[w].alarmStart, 0 );
				}
			}
		}
	}
     	
	if ( !dat->inEvent && !in_leadup ) 
		dat->preEventTriggerValue = dat->lastValue;
	dat->lastValue = msg.datum;
}

/************************  Main Process Thread   *********************
*          Pull a messsage from the queue, parse & run scripts       *
**********************************************************************/

thr_ret Process( void *dummy )
{
   int      ret;
   long     msgSize;
   MSG_LOGO Logo;       /* logo of retrieved message             */
   SAMPLE_DEF msg;

	RequestSpecificMutex(&reconfig_mutex);
   while (1)   /* main loop */
   {
     /* Get message from queue
      *************************/
     RequestMutex();
     ret=dequeue( &OutQueue, (void*)&msg, &msgSize, &Logo);
     ReleaseMutex_ew();
     if(ret < 0 )
     { /* -1 means empty queue */
       sleep_ew(500); /* wait a bit (changed from 1000 to 500 on 970624:ldd) */
       continue;
     }

	 if ( msg.scnl_idx == -1 ) {
	 	ew_rsamalarm_config( 1 );
		ReleaseSpecificMutex(&reconfig_mutex);
		RequestSpecificMutex(&reconfig_mutex);
		continue;
	 }
     /* Process retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK)
     ************************************************************/
     
     ProcessWindow( msg );
     if ( msg.period_idx == 0 ) {
     	//msg.period_idx = TRIGGER_PERIOD_INDEX;
     	ProcessWindow( msg );
     }
   }  /* End of main loop */

   return(NULL);
}

/*****************************************************************************
 *  parseStationArgs(): We're parsing a line a station definition section    *
 *                      section of a config file for currSta (and, if        *
 *                      currStaID > -1, also staDef[currStaID]); parses the  *
 *                      5 arguments for the win window of the station,       *
 *                      and return 1 if any of the args differ from the      *
 *                      values in staDef, 0 if they don't or currStaID<=-1   *
 *****************************************************************************/
static char parseStationArgs( int win, STATION_DEF *currSta, int currStaID ) 
{
	char changed = 0;
	double arg;
	
	
	arg = currSta->windef[win].threshold = k_val();
	if ( currStaID > -1 && staDef[currStaID].windef[win].threshold != arg )
		changed = 1;
	currSta->windef[win].ratio = k_val();
	if ( currStaID > -1 && staDef[currStaID].windef[win].ratio != arg )
		changed = 1;
	currSta->windef[win].time = k_val();
	if ( currStaID > -1 && staDef[currStaID].windef[win].time != arg )
		changed = 1;
	currSta->windef[win].timeout = k_val();
	if ( currStaID > -1 && staDef[currStaID].windef[win].timeout != arg )
		changed = 1;
	currSta->windef[win].enabled = k_bool();
	if ( currStaID > -1 && staDef[currStaID].windef[win].enabled != currSta->windef[win].enabled )
		changed = 1;
	return changed;
		
}



/*****************************************************************************
 *  staLookup(): look up the station named scn_l; if found, its index        *
 *              in subDef is returned.  If it isn't found:                   *
 *              - if addIfNew is 0, return -1                                *
 *              - o/w add the subnet to subDef & return its index            *
 *              If staSection is 1, then this lookup is from a station       *
 *              definition section of a config file, and initializes         *
 *              currSta for this station.                                    *
 *****************************************************************************/
static int staLookup( char *scn_l, char addIfNew, char staSection ) {
	char *sta;
	char *chan;
	char *net;
	char *loc = NULL;
	char isSCN_L = 0;
	int i;
	
	sta = strtok( scn_l, ":" );
	chan = strtok( NULL, ":" );
	if ( chan ) {
		net = strtok( NULL, ":" );
		if ( net ) {
			isSCN_L = 1;
			loc = strtok( NULL, "]" );
		} else if ( staSection ) {
			if ( net[strlen(net)-1] == ']' )
				net[strlen(net)-1] = 0;
			else {
				logit( "e" , "%s: <[%s> Unknown command in <%s>; exiting!\n",
					 Argv0, scn_l, configfile );
				exit( -1 );
			} 
		}
	}
	if ( isSCN_L ) {
		if ( staSection )
			inStationDef = 1;
		if ( loc == NULL ) {
			for ( i=0; i<nSta; i++ )
				if ( strcmp( sta, staDef[i].hdr.scn.sta ) == 0 &&
					strcmp( chan, staDef[i].hdr.scn.chan ) == 0 &&
					strcmp( net, staDef[i].hdr.scn.net ) == 0 )
					break;
			strcpy( currSta.hdr.scn.sta, sta );
			strcpy( currSta.hdr.scn.chan, chan );
			strcpy( currSta.hdr.scn.net, net );
			currSta.type2 = 0;
		} else {
			for ( i=0; i<nSta; i++ )
				if ( strcmp( sta, staDef[i].hdr.scnl.sta ) == 0 &&
					strcmp( chan, staDef[i].hdr.scnl.chan ) == 0 &&
					strcmp( net, staDef[i].hdr.scnl.net ) == 0 &&
					strcmp( loc, staDef[i].hdr.scnl.loc ) == 0 )
					break;
			strcpy( currSta.hdr.scnl.sta, sta );
			strcpy( currSta.hdr.scnl.chan, chan );
			strcpy( currSta.hdr.scnl.net, net );
			strcpy( currSta.hdr.scnl.loc, loc );
			currSta.type2 = 1;
		}
		if ( !addIfNew )
			return (i == nSta ? -1 : i);
		if ( i == nStaAlloc ) {
			nStaAlloc += 10;
			staDef = realloc( staDef, nStaAlloc * sizeof( STATION_DEF ) );
		}
		if ( i == nSta ) {
			staDef[i] = currSta;
			staDef[i].defined = 0;
			nSta++; 
		} else
			currSta.subnetCount = staDef[i].subnetCount;
		return i;
	} else
		return -2; 	
}

/*****************************************************************************
 *  subLookup(): look up the subnet named subnet; if found, its index        *
 *              in subDef is returned.  If it isn't found:                   *
 *              - if addIfNew is 0, return -1                                *
 *              - o/w add the subnet to subDef & return its index            *
 *****************************************************************************/
static int subLookup( char *subnet, char addIfNew ) {
	int i;
	for ( i=0; i<nSub; i++ )
		if ( strcasecmp( subnet, subDef[i].name ) == 0 )
			break;
	strcpy( currSub.name, subnet );
	if ( !addIfNew )
		return (i == nSub ? -1 : i);
	if ( i == nSubAlloc ) {
		nSubAlloc += 10;
		subDef = realloc( subDef, nSubAlloc * sizeof( SUBNET_DEF ) );
	}
	if ( i == nSub ) {
		subDef[i] = currSub;
		subDef[i].defined = 0;
		nSub++;
	}
	return i;
}

/*****************************************************************************
 *  fillStaName() fills name w/ the name of sta.                             *
 *****************************************************************************/
static void fillStaName( char *name, STATION_DEF sta ) {
	if ( currSta.type2 )
		sprintf( name, "%s.%s.%s.%s", sta.hdr.scnl.sta,
		 sta.hdr.scnl.chan, sta.hdr.scnl.net, sta.hdr.scnl.loc );
	else
		sprintf( name, "%s.%s.%s", sta.hdr.scn.sta,
		 sta.hdr.scn.chan, sta.hdr.scn.net );
}


/*****************************************************************************
 * finishStaDef(): Station definition from config file has been completed    *
 *****************************************************************************/
void finishStaDef( char reconfig ) {
	currSta.defined = 1;
	if ( currStaID>0 && memcmp( &currSta.hdr, &staDef[currStaID].hdr, sizeof(currSta)-((void*)&staDef[0].hdr-(void*)&staDef[0]) ) ) {
		/* Station changed or was added */
		if ( reconfig ) {
			/* OK, reset it */
		} else if ( staDef[currStaID].defined ) {
			char name[20];
			fillStaName( name, currSta );
			logit( "e" , "%s: Conflicting definitions for %s in <%s>; exiting!\n",
					 Argv0, name, configfile );
			exit( -1 );
		}		  						
	}
	/*currSta.subnetCount = staDef[currSubID].subnetCount;*/
	staDef[currStaID] = currSta;
	inStationDef = 0;
	currStaID = -1;
	currSta = staDef[0];
}


/*****************************************************************************
 * finishSubDef(): Subnet definition from config file has been completed     *
 *****************************************************************************/
void finishSubDef() {
	currSub.defined = 1;
	subDef[currSubID] = currSub;
	inSubnetDef = 0;
	currSubID = -1;
	memset( &currSub, 0, sizeof(currSub) );
}

/*****************************************************************************
 *  ew_rsamalarm_config() processes command file(s) using kom.c functions;   *
 *                    exits if any errors are encountered.                   *
 *****************************************************************************/
void ew_rsamalarm_config( char reconfig )
{
   int      ncommand;     /* # of required commands you expect to process   */
   char     init[10];     /* init flags, one byte for each required command */
   int      nmiss;        /* number of required commands that were missed   */
   char    *com;
   char     processor[20];
   int      nfiles;
   int      success;
   int      i;
   char*    str;
   char		useEWformat = 1;
      
   dbpf("In %sconfig(%s)\n", reconfig ? "re-" : "", configfile );
   
   currStaID = -1;
   inStationDef = 0;
   currSubID = -1;
   inSubnetDef = 0;
   
/* Set to zero one init flag for each required command
 *****************************************************/
   ncommand = 6;
   for( i=0; i<ncommand; i++ )  init[i] = 0;
   for ( i=0; i<5; i++ ) windef_defaults[i].threshold = -1;

/* Open the main configuration file
 **********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
    logit( "e" ,
                "%s: Error opening command file <%s>; exiting!\n",
                Argv0, configfile );
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
                  logit( "e" ,
                          "%s: Error opening command file <%s>; exiting!\n",
                           Argv0, &com[1] );
                  exit( -1 );
               }
               continue;
            }
            strcpy( processor, "ew_rsamalarm_config" );
        
        if ( k_its("TreatAsIni") ) {
        	useEWformat = 0;
        	k_treat_as_ini();
        	continue;
        }
        
        /* Process anything else as a command
         ************************************/
         
  /*0*/     if( k_its("LogFile") ) {
                LogSwitch = k_int();
                init[0] = 1;
            }
  /*1*/     else if( k_its("ModuleId") ) {
                str = k_str();
                if(str) strcpy( MyModName, str );
                init[1] = 1;
            }
  /*2*/     else if( k_its("InputRing") ) {
                str = k_str();
                if(str) strcpy( InRing, str );
                init[2] = 1;
            }
  /*3*/     else if( k_its("OutputRing") ) {
                str = k_str();
                if(str) strcpy( OutRing, str );
                init[3] = 1;
            }
  /*4*/     else if( k_its("HeartbeatInterval") ) {
                HeartBeatInt = k_int();
                init[4] = 1;
            }


         /* Maximum size (bytes) for incoming/outgoing messages
          *****************************************************/
  /*5*/     else if( k_its("MaxMsgSize") ) {
                MaxMsgSize = k_long();
                init[5] = 1;
            }
            
  		else if ( k_its("EventCountParameters") ) {
  			continue;
  		} 
  		else if ( k_its("TriggerParameters") ) {
  			if ( !inStationDef ) {
				logit( "e" , "%s: %s not in a Station section; exiting!\n", Argv0, com );
				exit( -1 );
			}
  			if ( parseStationArgs( TRIGGER_PERIOD_INDEX, &currSta, currStaID ) )
  				stationChanged = 1;
  		} 
  		else if ( k_its("EventAlarmTracker") ) {
  			if ( !inStationDef ) {
				logit( "e" , "%s: %s not in a Station section; exiting!\n", Argv0, com );
				exit( -1 );
			}
  			if ( parseStationArgs( EVENT_PERIOD_INDEX, &currSta, currStaID ) )
  				stationChanged = 1;
  		} else if ( k_its("TremorAlarmTracker") ) {
  			if ( !inStationDef ) {
				logit( "e" , "%s: %s not in a Station section; exiting!\n", Argv0, com );
				exit( -1 );
			}
  			if ( parseStationArgs( TREMOR_PERIOD_INDEX, &currSta, currStaID ) )
  				stationChanged = 1;
  		} 
  		else if ( k_its("StationsNecessaryForTremorAlarm") ) {
  			if ( !inSubnetDef ) {
				logit( "e" , "%s: %s not in a Subnet section; exiting!\n", Argv0, com );
				exit( -1 );
			}
  			currSub.windef[TREMOR_PERIOD_INDEX].minForAlarm = k_int();
  		}
  		else if ( k_its("TremorAlarmEnabled") ) {
  			if ( !inSubnetDef ) {
				logit( "e" , "%s: %s not in a Subnet section; exiting!\n", Argv0, com );
				exit( -1 );
			}
  			currSub.windef[TREMOR_PERIOD_INDEX].enabled = k_bool();
  		}
  		else if ( k_its("EventAlarmEnabled") ) {
  			if ( !inSubnetDef ) {
				logit( "e" , "%s: %s not in a Subnet section; exiting!\n", Argv0, com );
				exit( -1 );
			}
  			currSub.windef[EVENT_PERIOD_INDEX].enabled = k_bool();
  		}
  		else if ( k_its("StationsNecessaryForEventAlarm") ) {
   			if ( !inSubnetDef ) {
				logit( "e" , "%s: %s not in a Subnet section; exiting!\n", Argv0, com );
				exit( -1 );
			}
  			currSub.windef[EVENT_PERIOD_INDEX].minForAlarm = k_int();
 		}
  		else if ( k_its("CheckInhibitStationsDelay") ) {
   			if ( !inSubnetDef ) {
				logit( "e" , "%s: %s not in a Subnet section; exiting!\n", Argv0, com );
				exit( -1 );
			}
  			currSub.CheckInhibitStationsDelay = k_int();
  		}
  		else if ( k_its("EventAlarmResetTime") ) {
   			if ( !inSubnetDef ) {
				logit( "e" , "%s: %s not in a Subnet section; exiting!\n", Argv0, com );
				exit( -1 );
			}
  			currSub.windef[EVENT_PERIOD_INDEX].resetTime = k_int();
  		}
  		else if (  k_its("TremorAlarmResetTime") ) {
   			if ( !inSubnetDef ) {
				logit( "e" , "%s: %s not in a Subnet section; exiting!\n", Argv0, com );
				exit( -1 );
			}
  			currSub.windef[TREMOR_PERIOD_INDEX].resetTime = k_int();
  		} else if ( k_its("AlarmSubnet") ) {
  			i = subLookup( k_str(), 1 );
  		} 
  		else if ( k_its("Station") ) {
  			if ( !inSubnetDef ) {
                logit( "e" , "%s: %s '%s' not in a [Subnet] section in <%s>; exiting!\n",
                         Argv0, com, k_str(), configfile );
                exit( -1 );
  			}
  			i = staLookup( k_str(), 1, 0 );
  			//dbpf("Sta #%d in subnet %s (%d,%d)\n", i, currSub.name, staDef[i].subnetCount, currSub.nSta );
  			staDef[i].subnetCount++;
  			if ( currSub.nSta == currSub.nStaAlloc ) {
  				currSub.nStaAlloc += 10;
  				currSub.staList = realloc( currSub.staList, sizeof(int)*currSub.nStaAlloc );
  				memset( currSub.staList + currSub.nStaAlloc - 10, 0, 10*sizeof(currSub.staList[0]) );
  			}
 			currSub.staList[currSub.nSta++] = i;
  		}
  		else if ( k_its("UseAsInhibitor") ) {
  			dbpf("Inhibitor? #%d\n", currStaID );
  			if ( !inStationDef ) {
                logit( "e" , "%s: %s '%s' not in a station's definition section in <%s>; exiting!\n",
                         Argv0, com, k_str(), configfile );
                exit( -1 );
  			}
  			currSta.inhibitor = k_bool();
  			dbpf("Inhibitor: #%d %c\n", currStaID, currSta.inhibitor ? 'Y' : 'N' );
  		}

  		/* Ignored commands */
  		else if ( k_its("RingId") || k_its("RingName") || k_its("RingSize") || k_its("WatchLogo") )
  			continue;
  		else if ( k_its("Bob10MinuteDir") || k_its("Bob1MinuteDir") || k_its("BobEventCountDir") || k_its("BobTriggerDir") )
  			continue;
  		else if ( k_its("DisplayOnly") || k_its("DailyEmailTime") || k_its("DailyEmailRecipients") || k_its("AlarmLogFile") )
  			continue;
  		else if ( k_its("AlarmSoundedLog") || k_its("WindowPosition") )
  			continue;
  		else if ( k_its("DefaultRingSize") || k_its("ActiveRing") || k_its("TransportIdFile") || k_its("HeartbeatRing") )
  			continue;
  		else if ( k_its("Display") || k_its("InstId") )
  			continue;
  		else if ( k_its("IpAddress") || k_its("Port") || k_its("MaxMessageLength") || k_its("MaxClients") )
  			continue;
  		else if ( k_its("AsciiMessagesOnly") || k_its("ConnectionAnnouncement") || k_its("EnableOnStartup") || k_its("Account") )
  			continue;
  		else if ( k_its("DtrNotificationEnabled") || k_its("DtrPort") || k_its("EmailNotificationEnabled") || k_its("EmailRecipients") )
  			continue;
  		else if ( k_its("PagerEnabled") || k_its("PagerGroup") || k_its("FutureUse") || k_its("YaxisMaximum") )
  			continue;
  
  
  		 /* Section commands
  		  ******************/
  		else if ( com[0] == '[' ) {
  			char com2[100];
  			
  			if ( inStationDef ) {
  				finishStaDef( reconfig );
  			} else if ( inSubnetDef ) {
  				finishSubDef();
  			}
  			
  			if ( strncmp( com, "[trRing_", 8 ) == 0 )
  				continue;
  			if ( k_its("[TCPServer") || k_its("[RSAM") )
  				continue;
  				
  			/* Can only be a station or subnet def (or an error) */
  			if ( k_its("[StationDefaults") ) {
  				currStaID = 0;
  				currSta = staDef[0];
  				inStationDef = 1;
  				continue;
  			}
  			/*
  			if ( k_its("[SubnetDefaults") ) {
  				currStaID = 0;
  				currSta = staDef[0];
  				inStationDef = 1;
  				continue;
  			}
  			*/
			strncpy( com2, com+1, 100 );
			currStaID = staLookup( com2, 1, 1 );
			if ( currStaID == -2 ) {
				i = subLookup( com+1, 0 );	
				if ( i > -1 ) {
					inSubnetDef = 1;
					strcpy( currSub.name, com+1 );
					currSubID = i;
				} else {
					logit( "e" , "%s: Unknown station? %s] in <%s>; exiting!\n",
							 Argv0, com, configfile );
					exit( -1 );
				}		
  			} else if ( currStaID == -1 ) {
                logit( "e" , "%s: %s] not defined by AlarmSubnet in <%s>; exiting!\n",
                         Argv0, com, configfile );
                exit( -1 );
            } 
  		}
  
         /* Unknown command
          *****************/
        else {
                logit( "e" , "%s: <%s> Unknown command in <%s>.\n",
                         Argv0, com, configfile );
                continue;
            }

        /* See if there were any errors processing the command
         *****************************************************/
            if( k_err() ) {
               logit( "e" ,
                       "%s: Bad <%s> command for %s() in <%s>; exiting!\n",
                        Argv0, com, processor, configfile );
               exit( -1 );
            }
    }
    nfiles = k_close();
   }

	if ( inStationDef ) {
		finishStaDef( reconfig );
	} else if ( inSubnetDef ) {
		finishSubDef();
	}
 
   /*dbpf( "%d subnets, %d stations\n", nSub, nSta-1 );*/
   nmiss = 0;
   for ( i=1; i<nSta; i++ )
		if ( !staDef[i].defined ) {
			char name[20];
			fillStaName( name, staDef[i] );
			logit( "e", "'%s'(%d) ", name, i );
			nmiss++;
		} else if ( staDef[i].subnetCount > 0 ) {
			dbpf( "Station %d in %d subnets\n", i, staDef[i].subnetCount );
			staDef[i].subList = malloc( sizeof(int) * staDef[i].subnetCount );
			staDef[i].subList[0] = 1;
		}
   if ( nmiss ) {
   		logit( "e", "station(s) referenced but undefined in <%s>; exiting!\n", configfile );
   		exit(-1);
   	}
   for ( i=0; i<nSub; i++ ) {
   		int j;
		if ( !subDef[i].defined ) {
			logit( "e", "'%s' ", subDef[i].name );
			nmiss++;
		}
		dbpf( "%s:\n", subDef[i].name );
		for ( j=0; j<subDef[i].nSta; j++ ) {
			char name[20];
			int	 staID = abs(subDef[i].staList[j]);
#ifdef DEBUG
			char inhibitor = staDef[staID].inhibitor;
#endif
			int  subListPos = staDef[staID].subList[0];
			fillStaName( name, staDef[staID] );
			dbpf( "\t%s%s(%d)\n", name, inhibitor ? "*" : "", staID );
			if ( subListPos == staDef[staID].subnetCount )
				staDef[staID].subList[0] = i;
			else {
				staDef[staID].subList[subListPos] = i;
				staDef[staID].subList[0]++;
			}
		}
	}
   if ( nmiss ) {
   		logit( "e", "subnet(s) referenced but undefined in <%s>; exiting!\n", configfile );
   		exit(-1);
   	}

/* After all files are closed, check init flags for missed commands
 ******************************************************************/
   nmiss = 0;
   for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
   if ( nmiss ) {
       logit( "e", "%s: ERROR, no ", Argv0 );
       if ( !init[0] )  logit( "e", "<LogFile> "      );
       if ( !init[1] )  logit( "e", "<ModuleId> "   );
       if ( !init[2] )  logit( "e", "<InputRing> "     );
       if ( !init[3] )  logit( "e", "<OutputRing> "     );
       if ( !init[4] )  logit( "e", "<HeartbeatInterval> " );
       if ( !init[5] )  logit( "e", "<MaxMsgSize> "  );
       logit( "e" , "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }
   return;
}

/****************************************************************************
 *  ew_rsamalarm_lookup( )   Look up important info from earthworm.h tables *
 ****************************************************************************/
void ew_rsamalarm_lookup( void )
{
/* Look up keys to shared memory regions
   *************************************/
   if( ( InRingKey = GetKey(InRing) ) == -1 ) {
    fprintf( stderr,
            "%s:  Invalid ring name <%s>; exiting!\n",
                 Argv0, InRing);
    exit( -1 );
   }
   if( ( OutRingKey = GetKey(OutRing) ) == -1 ) {
    fprintf( stderr,
            "%s:  Invalid ring name <%s>; exiting!\n",
                 Argv0, OutRing);
    exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      fprintf( stderr,
              "%s: error getting local installation id; exiting!\n",
               Argv0 );
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModName, &MyModId ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid module name <%s>; exiting!\n",
               Argv0, MyModName );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid message type <TYPE_HEARTBEAT>; exiting!\n", Argv0 );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid message type <TYPE_ERROR>; exiting!\n", Argv0 );
      exit( -1 );
   }
   if ( GetType( "TYPE_ACTIVATE_MODULE", &TypeActivate ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid message type <TYPE_ACTIVATE_MODULE>; exiting!\n", Argv0 );
      exit( -1 );
   }
   if ( GetType( "TYPE_RSAM_EVENT", &TypeRsamEvent ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid message type <TYPE_RSAM_EVENT>; exiting!\n", Argv0 );
      exit( -1 );
   }
   if ( GetType( "TYPE_RSAM_ALARM", &TypeRsamAlarm ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid message type <TYPE_RSAM_ALARM>; exiting!\n", Argv0 );
      exit( -1 );
   }
   return;
}

/***************************************************************************
 * ew_rsamalarm_status() builds a heartbeat or error message & puts it into*
 *                 shared memory.  Writes errors to log file & screen.     *
 ***************************************************************************/
void ew_rsamalarm_status( unsigned char type, short ierr, char *note )
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
    sprintf( msg, "%ld %ld\n%c", (long) t, (long) MyPid, 0);
   else if( type == TypeError )
   {
    sprintf( msg, "%ld %hd %s\n%c", (long) t, ierr, note, 0);

    logit( "et", "%s(%s): %s\n", Argv0, MyModName, note );
   }

   size = strlen( msg );   /* don't include the null byte in the message */

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &InRegion, &logo, size, msg ) != PUT_OK )
   {
        if( type == TypeHeartBeat ) {
           logit("et","%s(%s):  Error sending heartbeat.\n",
                  Argv0, MyModName );
    }
    else if( type == TypeError ) {
           logit("et", "%s(%s):  Error sending error:%d.\n",
                  Argv0, MyModName, ierr );
    }
   }

   return;
}

/***************************************************************************
 * ew_rsamalarm_free()  free all previously allocated memory               *
 ***************************************************************************/
void ew_rsamalarm_free( void )
{
   free (MSrawmsg);             /* MessageStacker's "raw" retrieved message */
   return;
}
