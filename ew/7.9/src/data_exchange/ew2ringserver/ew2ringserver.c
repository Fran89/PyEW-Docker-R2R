/*****************************************************************
 * ew2ringserver.c
 *
 * Program to read (selected) messages from a transport ring, create
 * miniSEED and forward packets to a ringserver.  The source code
 * started as ringdup/tbuf2mseed as of 2013/6/1, credit for the kernel
 * used to start goes to those authors.
 *
 * ew2ringserver can listen for TRACEBUF[2] and MSEED message types.
 * For TRACEBUF[2] messages, the data are buffered and packed into
 * 512-byte miniSEED records.  For MSEED message types if the detected
 * record length is 512-bytes it is forwarded to the ringserver
 * unmodified, otherwise it is ignored.
 *
 * Hacked on by Chad Trabant
 *****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <imp_exp_gen.h>
#include <mem_circ_queue.h>
#include <swap.h>

#include <trace_buf.h>
#include <libmseed.h>
#include <libdali.h>

#include "exportfilter.h"

#define PACKAGE "ew2ringserver"
#define PROGRAM_VERSION "1.2 2013-10-12"

static thr_ret MessageStacker (void *); /* Read messages and add to queue */

static void config (char *);
static void lookup (void);
static void logstatus (unsigned char, short, char *);
static void freelocal (void);

static MSTraceGroup *mstg = 0; /* buffer for miniseed packaging */
static void sendrecord (char *record, int reclen, void *handlerdata);
static  int packtraces (MSTrace *mst, int flush, hptime_t flushtime);
static  int handletbuf (char *msgbuf, int nbytes);

static void logmststats (MSTrace *mst);
void logit_msg (char *msg);
void logit_err (char *msg);

#ifndef WIN32
  static void term_handler (int sig);
#endif

/* Per-trace statistics */
typedef struct tracestats_s
{
  hptime_t earliest;
  hptime_t latest;
  hptime_t update;
  hptime_t xmit;
  int64_t pktcount;
  int64_t reccount;
} TraceStats;

/* Thread things */
#define THREAD_STACK 8192
static unsigned tidStacker;          /* Thread moving messages from transport to queue */

#define MSGSTK_OFF    0              /* MessageStacker has not been started      */
#define MSGSTK_ALIVE  1              /* MessageStacker alive and well            */
#define MSGSTK_ERR   -1              /* MessageStacker encountered error quit    */
volatile int MessageStackerStatus = MSGSTK_OFF;

QUEUE MsgQueue;                      /* from queue.h, queue.c; sets up linked    */

/* Message Buffers to be allocated */
static char *Rawmsg = NULL;          /* "raw" retrieved message                  */
static char *Filteredmsg = NULL;     /* MessageStacker's "filtered" message      */
static char *Qmsg = NULL;            /* message from queue                       */
static MSG_LOGO Logo;                /* logo of message to re-send               */

/* Timers */
time_t now;                          /* current time, used for timing heartbeats */
time_t MyLastBeat;                   /* time of last local (into Earthworm) hearbeat */
time_t MyLastFlush;                  /* time of last buffer flush                */

extern int errno;

static  SHM_INFO  InRegion;     /* shared memory region to use for input  */

#define   MAXLOGO   10
MSG_LOGO  GetLogo[MAXLOGO];     /* array for requesting module,type,instid */
short     nLogo;

char *Argv0;        /* pointer to executable name */
pid_t MyPid;        /* Our own pid, sent with heartbeat for restart purposes */

/* Things to read or derive from configuration file */
static char    RingName[MAX_RING_STR];  /* name of transport ring for input  */
static char    MyModName[MAX_MOD_STR];  /* speak as this module name/id      */
static int     LogSwitch;           /* 0 if no logfile should be written */
static int     HeartBeatInt;        /* seconds between heartbeats        */
static long    MaxMsgSize;          /* max size for input/output msgs    */
static int     QueueSize;           /* max messages in output circular buffer */

/* Things to look up in the earthworm.h tables with getutil.c functions */
static long          RingNameKey;   /* key of transport ring for input    */
static unsigned char InstId;        /* local installation id              */
static unsigned char MyModId;       /* Module Id for this program         */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypeMseed;
static unsigned char TypeTracebuf;
static unsigned char TypeTracebuf2;

/* Error messages used by export */
#define  ERR_MISSMSG     0     /* message missed in transport ring        */
#define  ERR_TOOBIG      1     /* retreived msg too large for buffer      */
#define  ERR_NOTRACK     2     /* msg retreived; tracking limit exceeded  */
#define  ERR_QUEUE       3     /* error queueing message for sending      */
static char  errText[256];     /* string for log/error messages           */

static int stopsig     = 0;        /* 1: termination requested, 2: termination and no flush */
static int verbose     = 0;
static int flushlatency = 300;     /* Flush data buffers if not updated for latency in seconds */
static int reconnectinterval = 10; /* Interval to wait between reconnection attempts in seconds */
static int int32encoding = DE_STEIM2; /* Interval to wait between reconnection attempts in seconds */

static char *rsaddr    = 0;        /* DataLink/ringserver receiver address in IP:port format */
static DLCP *dlcp      = 0;        /* DataLink connection handle */


int
main ( int argc, char **argv )
{
  int        res;
  long       recsize;   /* size of retrieved message             */
  MSG_LOGO   reclogo;   /* logo of retrieved message             */
  
  int        ret;
  long       msgSize;
  int        count;
  
#ifndef WIN32
  /* Signal handling, use POSIX calls with standardized semantics */
  struct sigaction sa;

  sa.sa_flags = SA_RESTART;
  sigemptyset (&sa.sa_mask);
  
  sa.sa_handler = term_handler;
  sigaction (SIGINT, &sa, NULL);
  sigaction (SIGQUIT, &sa, NULL);
  sigaction (SIGTERM, &sa, NULL);
  
  sa.sa_handler = SIG_IGN;
  sigaction (SIGHUP, &sa, NULL);
  sigaction (SIGPIPE, &sa, NULL);
#endif

  /* Check command line arguments */
  Argv0 = argv[0];
  if ( argc != 2 )
    {
      fprintf( stderr, "Usage: %s <configfile>\n", Argv0 );
      fprintf( stderr, "Version: V%s\n", PROGRAM_VERSION );
      exit( 0 );
    }
  
  /* Initialize name of log-file & open it */
  logit_init( argv[1], 0, 512, 1 );

  /* Read the configuration file(s) */
  config( argv[1] );
  logit( "et" , "%s(%s): Read command file <%s>\n Version %s\n",
	 Argv0, MyModName, argv[1], PROGRAM_VERSION );
  
  /* Look up important info from earthworm.h tables */
  lookup();

  /* Initialize trace buffer */
  if ( ! (mstg = mst_initgroup ( mstg )) )
    {
      logit ("et", "%s(%s): Cannot initialize MSTraceList\n", Argv0, MyModName);
      exit(1);
    }
  
  /* Reinitialize the logging level */
  logit_init( argv[1], 0, 512, LogSwitch );

  /* Get our own Pid for restart purposes */
  MyPid = getpid();
  if ( MyPid == -1 )
    {
      logit("e", "%s(%s): Cannot get pid; exiting!\n", Argv0, MyModName);
      exit(0);
    }
  
  /* Initialize export filter */
  if ( exportfilter_init() != 0 )
    {
      logit("e", "%s(%s): Error in exportfilter_init(); exiting!\n",
            Argv0, MyModName );
      exit(-1);
    }
  
  /* Allocate space for input/output messages for all threads */
  if ( ( Rawmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL )
    {
      logit( "e", "%s(%s): error allocating Rawmsg; exiting!\n",
             Argv0, MyModName );
      freelocal();
      exit(-1);
    }
  
  /* Buffers for message handling loop */
  if ( ( Qmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL )
    {
      logit( "e", "%s(%s): error allocating Qmsg; exiting!\n",
	     Argv0, MyModName );
      freelocal();
      exit(-1);
    }
  
  /* Buffers for the MessageStacker thread: */
  if ( ( Filteredmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL )
    {
      logit( "e", "%s(%s): error allocating Filteredmsg; exiting!\n",
	     Argv0, MyModName );
      freelocal();
      exit (-1);
    }
  
  /* Create a Mutex to control access to queue */
  CreateMutex_ew();
  
  /* Initialize the message queue */
  initqueue( &MsgQueue, (unsigned long)QueueSize,(unsigned long)MaxMsgSize+1 );
  
  /* Allocate and initialize DataLink connection description */
  if ( ! (dlcp = dl_newdlcp (rsaddr, PACKAGE)) )
    {
      logit ("e", "Cannot allocation DataLink descriptor\n");
      exit (-1);
    }
  
  /* Connect to destination DataLink server */
  if ( dl_connect (dlcp) < 0 )
    {
      logit ("t", "Initial connection to DataLink server (%s) failed, will retry later\n",
	     dlcp->addr);
      dl_disconnect (dlcp);
    }
  else
    logit ("t", "Connected to ringserver at %s\n", dlcp->addr);
  
  /* Attach to Input shared memory ring */
  tport_attach( &InRegion, RingNameKey );
  
  /* step over all messages from transport ring */
  /* As Lynn pointed out: if we're restarted by startstop after hanging,
     we should throw away any of our messages in the transport ring.
     Else we could end up re-sending a previously sent message, causing
     time to go backwards... */
  do
    {
      res = tport_getmsg( &InRegion, GetLogo, nLogo,
			  &reclogo, &recsize, Rawmsg, MaxMsgSize );
    } while (res != GET_NONE);
  
  /* One heartbeat to announce ourselves to statmgr */
  logstatus( TypeHeartBeat, 0, "" );
  time(&MyLastBeat);
  time(&MyLastFlush);
  
  /* Start the message stacking thread if it isn't already running. */
  if ( MessageStackerStatus != MSGSTK_ALIVE )
    {
      if ( StartThread( MessageStacker, (unsigned)THREAD_STACK, &tidStacker ) == -1 )
	{
	  logit( "e",
		 "%s(%s): Error starting  MessageStacker thread; exiting!\n",
		 Argv0, MyModName );
	  tport_detach( &InRegion );
	  freelocal();
	  exit(-1);
	}
      MessageStackerStatus = MSGSTK_ALIVE;
    }
  
  /* Start main service loop */
  while ( tport_getflag( &InRegion ) != TERMINATE  &&
          tport_getflag( &InRegion ) != MyPid &&
	  ! stopsig )
    {
      /* Beat the heart into the transport ring */
      time(&now);
      if ( difftime(now,MyLastBeat) > (double)HeartBeatInt )
	{
	  logstatus ( TypeHeartBeat, 0, "" );
	  MyLastBeat = now;
	}
      /* Check if idle data streams need flusing */
      if ( difftime(now,MyLastFlush) > (double)flushlatency )
	{
	  if ( packtraces (NULL, 0, DL_EPOCH2DLTIME(now) - DL_EPOCH2DLTIME(flushlatency)) < 0 )
	    {
	      logit ("et", "Cannot pack idle trace buffers or send records!\n");
	    }
	  
	  MyLastFlush = now;
	}
      
      /* Process up to 10 messages, then let outer loop have a pass */
      count = 0;
      while ( count < 10 )
	{
	  /* Get message from queue */
	  RequestMutex();
	  ret = dequeue (&MsgQueue, Qmsg, &msgSize, &Logo);
	  ReleaseMutex_ew();
	  
	  /* If queue is empty (ret=-1), sleep 1/2 second and break to outer loop */
	  if ( ret < 0 )
	    {
	      sleep_ew(500);
	      break;
	    }
	  
	  /* Send TRACEBUF/TRACEBUF2 to buffer for packing */
	  if ( Logo.type == TypeTracebuf || Logo.type == TypeTracebuf2 )
	    {
	      handletbuf (Qmsg, msgSize);
	    }
	  /* Send 512-byte miniSEED record message directly to ringserver */
	  else if ( Logo.type == TypeMseed )
	    {
	      if ( ms_detect (Qmsg, msgSize) == 512 )
		{
		  sendrecord (Qmsg, 512, NULL);
		}
	    }
	  else if ( verbose )
	    {
	      logit ("t", "Unrecognized message type (%d), skipping\n", Logo.type);
	    }
	  
	  count++;
	}
    } /* End of main service loop */
  
  /* Shut it all down */
  while ( 1 ) /* Flush message queue */
    {
      RequestMutex();
      ret = dequeue (&MsgQueue, Qmsg, &msgSize, &Logo);
      ReleaseMutex_ew();      
      
      if ( ret < 0 )
	break;
      
      handletbuf (Qmsg, msgSize);      
    }
  
  stopsig = 1;
  
  tport_detach( &InRegion );
  
  packtraces (NULL, 1, HPTERROR);  /* Flush all data in pack buffers */
  
  if ( dlcp->link != -1 )
    dl_disconnect (dlcp);
  
  exportfilter_shutdown();
  
  freelocal();
  
  if ( verbose )
    {
      MSTrace *mst = mstg->traces;
      
      while ( mst )
        {
          logmststats (mst);
	  
          mst = mst->next;
        }
    }
  
  logit ("t", "%s(%s): termination requested; exiting!\n", Argv0, MyModName );
  
  return 0;
}  /* End of main() */


/********************** Message Stacking Thread *******************
 *           Move messages from transport to memory queue         *
 ******************************************************************/
static thr_ret
MessageStacker ( void *dummy )
{
   long      recsize;       /* size of retrieved message             */
   MSG_LOGO  reclogo;       /* logo of retrieved message             */
   long      filteredSize;  /* size of message after user-filtering  */
   unsigned char filteredType;  /* type of message after filtering   */
   int       res;
   int       ret;
   int       NumOfTimesQueueLapped= 0; /* number of messages lost due to
                                             queue lap */

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
                          &reclogo, &recsize, Rawmsg, MaxMsgSize );

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
            logstatus( TypeError, ERR_TOOBIG, errText );
            continue;
         }
         else if( res==GET_MISS )
         {
            sprintf( errText, "missed msg(s) i%d m%d t%d in %s",(int) reclogo.instid,
                    (int) reclogo.mod, (int)reclogo.type, RingName );
            logstatus( TypeError, ERR_MISSMSG, errText );
         }
         else if( res==GET_NOTRACK )
         {
            sprintf( errText, "no tracking for logo i%d m%d t%d in %s",
                     (int) reclogo.instid, (int) reclogo.mod, (int)reclogo.type,
                     RingName );
            logstatus( TypeError, ERR_NOTRACK, errText );
         }
      }

      /* Process retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK)
      ***********************************************************/
      Rawmsg[recsize] = '\0';
      /* pass it through the filter routine: this may reformat,
         or reject as it chooses. */
      if ( exportfilter( Rawmsg, recsize, reclogo.type, &Filteredmsg,
                        &filteredSize, &filteredType ) == 0 )  continue;
      reclogo.type = filteredType;  /* note the new message type */
      
      /* put it into the 'to be shipped' queue */
      /* the main thread is in the biz of de-queueng and handling */
      RequestMutex();
      ret = enqueue (&MsgQueue, Filteredmsg, filteredSize, reclogo);
      ReleaseMutex_ew();

      if ( ret!= 0 )
      {
	 if (ret==-2)  /* Serious: quit */
	 {    /* Currently, eneueue() in mem_circ_queue.c never returns this error. */
	   sprintf(errText,"internal queue error. Terminating.");
	   logstatus( TypeError, ERR_QUEUE, errText );
	   
	   break;
	 }
         if (ret==-1)
         {
            sprintf(errText,"queue cannot allocate memory. Lost message.");
            logstatus( TypeError, ERR_QUEUE, errText );
            continue;
         }
         if (ret==-3)  /* Log only while client's connected */
         {
         /* Queue is lapped too often to be logged to screen.
          * Log circular queue laps to logfile.
          * Maybe queue laps should not be logged at all.
          */
            NumOfTimesQueueLapped++;
            if (!(NumOfTimesQueueLapped % 5))
            {
	      logit("t",
		    "%s(%s): Circular queue lapped 5 times. Messages lost. Queue has %d messages.\n",
		    Argv0, MyModName, MsgQueue.NumOfElements);
               if (!(NumOfTimesQueueLapped % 100))
               {
                  logit( "et",
                        "%s(%s): Circular queue lapped 100 times. Messages lost. Queue has %d messages.\n",
                         Argv0, MyModName, MsgQueue.NumOfElements);
               }
            }
            continue;
         }
      }
   } /* end of while */

   /* we're quitting
   *****************/
   MessageStackerStatus = MSGSTK_ERR; /* file a complaint to the main thread */
   KillSelfThread(); /* main thread will restart us */

   return NULL;
}


/*****************************************************************************
 *  config() processes command file(s) using kom.c functions;         *
 *                    exits if any errors are encountered.               *
 *****************************************************************************/
static void
config ( char *configfile )
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

  /* Set to zero one init flag for each required command */
  ncommand = 8;
  for ( i=0; i < ncommand; i++ )  init[i] = 0;
  nLogo = 0;
  
  /* Open the main configuration file */
  nfiles = k_open( configfile );
  if ( nfiles == 0 )
    {
      logit( "e", "%s: Error opening command file <%s>; exiting!\n",
	     Argv0, configfile );
      exit(-1);
    }
  
  /* Process all command files */
  while ( nfiles > 0 )     /* While there are command files open */
    {
      while ( k_rd() )     /* Read next line from active file */
        {
	  com = k_str();   /* Get the first token from line */
	  
	  /* Ignore blank lines & comments */
	  if ( !com )           continue;
	  if ( com[0] == '#' )  continue;
	  
	  /* Open a nested configuration file */
	  if ( com[0] == '@' )
	    {
	      success = nfiles+1;
	      nfiles  = k_open (&com[1]);
	      if ( nfiles != success )
		{
                  logit( "e", "%s: Error opening command file <%s>; exiting!\n",
			 Argv0, &com[1] );
                  exit(-1);
		}
	      continue;
            }
	  strcpy ( processor, "config" );
	  
	  /* Process anything else as a command */
  /*0*/   if ( k_its("LogFile") )
	    {
	      LogSwitch = k_int();
	      init[0] = 1;
            }
  /*1*/   else if ( k_its("MyModuleId") )
	    {
	      str = k_str();
	      if (str) strcpy ( MyModName, str );
	      init[1] = 1;
            }
  /*2*/   else if ( k_its("RingName") )
	    {
	      str = k_str();
	      if (str) strcpy ( RingName, str );
	      init[2] = 1;
            }
  /*3*/   else if ( k_its("RSAddress") )
	    {
	      str = k_str();
	      if (str) rsaddr = strdup (str);
	      init[3] = 1;
            }
  /*4*/   else if ( k_its("HeartBeatInt") )
	    {
	      HeartBeatInt = k_int();
	      init[4] = 1;
            }
	  
	  /* Enter installation & module & message types to get */
  /*5*/   else if ( k_its("GetMsgLogo") )
	    {
	      if ( nLogo >= MAXLOGO )
		{
		  logit( "e",
			 "%s: Too many <GetMsgLogo> commands in <%s>", Argv0, configfile );
		  logit( "e", "; max=%d; exiting!\n", (int) MAXLOGO );
		  exit(-1);
                }
	      if ( ( str=k_str() ) )
		{
		  if ( GetInst( str, &GetLogo[nLogo].instid ) != 0 )
		    {
		      logit( "e",
			     "%s: Invalid installation name <%s>", Argv0, str );
		      logit( "e", " in <GetMsgLogo> cmd; exiting!\n" );
		      exit(-1);
		    }
                }
	      if ( ( str=k_str() ) )
		{
		  if ( GetModId( str, &GetLogo[nLogo].mod ) != 0 )
		    {
		      logit( "e",
			     "%s: Invalid module name <%s>", Argv0, str );
		      logit( "e", " in <GetMsgLogo> cmd; exiting!\n" );
		      exit(-1);
		    }
                }
	      if ( ( str=k_str() ) )
		{
		  if ( GetType( str, &GetLogo[nLogo].type ) != 0 )
		    {
		      logit( "e",
			     "%s: Invalid msgtype <%s>", Argv0, str );
		      logit( "e", " in <GetMsgLogo> cmd; exiting!\n" );
		      exit(-1);
		    }
                }
	      nLogo++;
	      init[5] = 1;
            }
	  
	  /* Maximum size (bytes) for incoming/outgoing messages */
  /*6*/   else if ( k_its("MaxMsgSize") )
	    {
	      MaxMsgSize = k_long();
	      init[6] = 1;
            }

	  /* Maximum number of messages in outgoing circular buffer */
  /*7*/   else if ( k_its("QueueSize") )
	    {
	      QueueSize = k_long();
	      init[7] = 1;
            }

	  /* Optinal options */
	  else if ( k_its("Verbosity") )
	    {
	      str = k_str();
	      if (str) verbose = atoi (str);
            }
	  else if ( k_its("FlushLatency") )
	    {
	      str = k_str();
	      if (str) flushlatency = atoi (str);
            }
	  else if ( k_its("ReconnectInterval") )
	    {
	      str = k_str();
	      if (str) reconnectinterval = atoi (str);
            }
	  else if ( k_its("Int32Encoding") )
	    {
	      str = k_str();
	      if ( str ) 
		{
		  if ( ! strcmp (str, "STEIM1") )
		    int32encoding = DE_STEIM1;
		  else if ( ! strcmp (str, "STEIM2") )
		    int32encoding = DE_STEIM2;
		  else if ( ! strcmp (str, "INT32") )
		    int32encoding = DE_INT32;
		  else
		    {
		      logit( "e", "%s: Unrecognized Int32Encoding: %s; exiting!\n",
			     Argv0, str);
		      exit( -1 );
		    }
		}
            }
	  
	  /* Pass it off to the filter's config processor */
	  else if ( exportfilter_com() )
	    strcpy ( processor, "exportfilter_com" );

	  /* Unknown command */
	  else
	    {
	      logit( "e", "%s: <%s> Unknown command in <%s>.\n",
		     Argv0, com, configfile );
	      continue;
            }
	  
	  /* See if there were any errors processing the command */
	  if ( k_err() )
	    {
	      logit( "e", "%s: Bad <%s> command for %s() in <%s>; exiting!\n",
		     Argv0, com, processor, configfile );
	      exit( -1 );
            }
	}
      
      nfiles = k_close();
    }
  
  /* After all files are closed, check init flags for missed commands */
  nmiss = 0;
  for ( i=0; i < ncommand; i++ ) if ( !init[i] ) nmiss++;
  if ( nmiss )
    {
      logit( "e", "%s: ERROR, no ", Argv0 );
      if ( !init[0] )  logit( "e", "<LogFile> "      );
      if ( !init[1] )  logit( "e", "<MyModuleId> "   );
      if ( !init[2] )  logit( "e", "<RingName> "     );
      if ( !init[3] )  logit( "e", "<RSAddress> "    );
      if ( !init[4] )  logit( "e", "<HeartBeatInt> " );
      if ( !init[5] )  logit( "e", "<GetMsgLogo> "   );
      if ( !init[6] )  logit( "e", "<MaxMsgSize> "   );
      if ( !init[7] )  logit( "e", "<QueueSize> "    );
      logit( "e" , "command(s) in <%s>; exiting!\n", configfile );
      exit( -1 );
    }

  /* Redirect libmseed & libdali logging to EW facility through shim functions */
  ms_loginit (&logit_msg, NULL, &logit_err, NULL); 
  dl_loginit (verbose, &logit_msg, NULL, &logit_err, NULL);
  
  return;
}  /* End of config() */


/****************************************************************************
 *  lookup( )   Look up important info from earthworm.h tables       *
 ****************************************************************************/
static void
lookup ( void )
{
  /* Look up keys to shared memory regions */
  if ( ( RingNameKey = GetKey(RingName) ) == -1 )
    {
      fprintf ( stderr,
		"%s:  Invalid ring name <%s>; exiting!\n", Argv0, RingName );
      exit(-1);
    }
  
  /* Look up installations of interest */
  if ( GetLocalInst( &InstId ) != 0 )
    {
      fprintf ( stderr,
		"%s: error getting local installation id; exiting!\n", Argv0 );
      exit(-1);
    }
  
  /* Look up modules of interest */
  if ( GetModId( MyModName, &MyModId ) != 0 )
    {
      fprintf ( stderr,
		"%s: Invalid module name <%s>; exiting!\n", Argv0, MyModName );
      exit(-1);
    }
  
  /* Look up message types of interest */
  if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 )
    {
      fprintf ( stderr,
		"%s: Invalid message type <TYPE_HEARTBEAT>; exiting!\n", Argv0 );
      exit(-1);
    }
  if ( GetType( "TYPE_ERROR", &TypeError ) != 0 )
    {
      fprintf ( stderr,
		"%s: Invalid message type <TYPE_ERROR>; exiting!\n", Argv0 );
      exit(-1);
    }
  if ( GetType( "TYPE_MSEED", &TypeMseed ) != 0 )
    {
      fprintf ( stderr,
		"%s: Critical message type <TYPE_MSEED> is missing; exiting!\n", Argv0 );
      exit(-1);
    }
  if ( GetType( "TYPE_TRACEBUF", &TypeTracebuf ) != 0 )
    {
      fprintf ( stderr,
		"%s: Critical message type <TYPE_TRACEBUF> is missing; exiting!\n", Argv0 );
      exit(-1);
    }
  if ( GetType( "TYPE_TRACEBUF2", &TypeTracebuf2 ) != 0 )
    {
      fprintf ( stderr,
		"%s: Critical message type <TYPE_TRACEBUF2> is missing; exiting!\n", Argv0 );
      exit(-1);
    }
  
  return;
}


/***************************************************************************
 * logstatus() builds a heartbeat or error message & puts it into      *
 *                 shared memory.  Writes errors to log file & screen.     *
 ***************************************************************************/
static void
logstatus ( unsigned char type, short ierr, char *note )
{
  MSG_LOGO    logo;
  char        msg[256];
  long        size;
  time_t      t;

  /* Build the message */
  logo.instid = InstId;
  logo.mod    = MyModId;
  logo.type   = type;
  
  time ( &t );

  if ( type == TypeHeartBeat )
    sprintf( msg, "%ld %ld\n", (long) t, (long) MyPid);
  else if ( type == TypeError )
    {
      sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note);
      
      logit( "et", "%s(%s): %s\n", Argv0, MyModName, note );
    }
  
  size = strlen( msg );   /* don't include the null byte in the message */

  /* Write the message to shared memory */
  if ( tport_putmsg( &InRegion, &logo, size, msg ) != PUT_OK )
    {
      if ( type == TypeHeartBeat )
	{
	  logit ("et","%s(%s):  Error sending heartbeat.\n",
		Argv0, MyModName );
      }
      else if ( type == TypeError )
	{
	  logit ("et", "%s(%s):  Error sending error:%d.\n",
		 Argv0, MyModName, ierr );
      }
    }
  
  return;
}


/***************************************************************************
 * freelocal()  free all previously allocated memory                      *
 ***************************************************************************/
static void
freelocal ( void )
{
   free (Rawmsg);               /* "raw" retrieved message   */
   free (Qmsg);                 /* Incoming message buffer   */
   free (Filteredmsg);          /* MessageStacker's "filtered" message to  */
                                /*  be sent to destination                 */
   return;
}


/*********************************************************************
 * handletbuf:
 *
 * Add the tbuf packet data to the trace buffer, pack the buffer and
 * send it on.
 *
 * Returns the number of records packed/transmitted on success and -1
 * on error.
 *********************************************************************/
static int
handletbuf (char *msg, int nbytes)
{
  static MSRecord *msr = NULL;
  static int32_t *int32buffer = NULL;
  static int32_t int32count = 0;
  int16_t *int16buffer = NULL;
  int idx;
  int rv = 0;
  
  MSTrace *mst = NULL;
  int recordspacked = 0;
  TRACE2_HEADER *wfhead;     /* pntr to header of TYPE_TRACEBUF2 msg */
  TracePacket *tp;
  char originaldatatype[3];
  
  if ( ! msg || nbytes <= 0 )
    return -1;
  
  tp = (TracePacket *) msg;
  wfhead = &(tp->trh2);
  strncpy (originaldatatype, wfhead->datatype, sizeof(originaldatatype));
  
  rv = WaveMsg2MakeLocal(wfhead);
  if ( rv != 0 )
    {
      logit("et", "Error with WaveMsg2MakeLocal(): %d\n", rv);
      return -1;
    }
  
  if ( verbose > 1 )
    logit("et", "Received: %s.%s.%s.%s  ns=%d  dt=%4.1f  type=%s (now %s)\n",
	  wfhead->sta, wfhead->chan, wfhead->net, wfhead->loc,
	  wfhead->nsamp, wfhead->samprate, originaldatatype, wfhead->datatype);
  
  if ( ! (msr = msr_init (msr)) )
    {
      logit("et", "Could not (re)initialize MSRecord\n");
      return -1;
    }
  
  /* Populate an MSRecord from a tbuf2 */
  ms_strncpclean (msr->network, wfhead->net, 2);
  ms_strncpclean (msr->station, wfhead->sta, 5);
  if ( strcmp(wfhead->loc, "--") == 0 )
    {
      wfhead->loc[0]=' ';
      wfhead->loc[1]=' ';
    }
  ms_strncpclean (msr->location, wfhead->loc, 2);
  ms_strncpclean (msr->channel, wfhead->chan, 3);
  
  msr->starttime = (hptime_t)(MS_EPOCH2HPTIME(wfhead->starttime) + 0.5);
  msr->samprate = wfhead->samprate;
  msr->datasamples = msg + sizeof(TRACE2_HEADER);
  msr->numsamples = wfhead->nsamp;
  msr->samplecnt = wfhead->nsamp;
  
  if ( ! strcmp(wfhead->datatype, "s2") || ! strcmp(wfhead->datatype, "i2") )
    {
      /* (Re)Allocate static conversion buffer if more space is needed */
      if ( int32count < msr->numsamples )
	{
	  if ( (int32buffer = (int32_t *) realloc (int32buffer, sizeof(int32_t)*msr->numsamples)) == NULL )
	    {
	      logit ("et", "Cannot (re)allocate memory for 16->32 bit conversion buffer\n");
	      return -1;
	    }
	  int32count = msr->numsamples;
	}
      
      /* Convert 16-bit integers to 32-bit integers */
      int16buffer = (int16_t *) msr->datasamples;      
      
      for ( idx=0; idx < msr->numsamples; idx++ )
	{
	  int32buffer[idx] = int16buffer[idx];
	}
      
      msr->datasamples = int32buffer;
      msr->sampletype = 'i';
    }
  else if ( ! strcmp(wfhead->datatype, "s4") || ! strcmp(wfhead->datatype, "i4") )
    msr->sampletype = 'i';
  else if ( ! strcmp(wfhead->datatype, "t4") || ! strcmp(wfhead->datatype, "f4") )
    msr->sampletype = 'f';
  else if ( ! strcmp(wfhead->datatype, "t8") || ! strcmp(wfhead->datatype, "f8") )
    msr->sampletype = 'd';
  else {
    logit ("et", "Unsupported data type: '%s'\n", wfhead->datatype);
    return -1;
  }
  
  /* Add data to trace buffer, creating new entry or extending as needed */
  if ( ! (mst = mst_addmsrtogroup (mstg, msr, 1, -1.0, -1.0)) )
    {
      logit ("et", "Cannot add packet data to trace buffer!\n");
      return -1;
    }
  
  /* To keep small variations in the sample rate or time base from accumulating
   * to large errors, re-base the time of the buffer by back projecting from
   * the endtime, which is calculated from the tracebuf starttime and number
   * of samples.  In essence, this maintains a time line based on the starttime
   * of received tracebufs.  It also retains the variations of the sample rate
   * and other characteristics of the original data stream to some degree. */
  
  mst->starttime = mst->endtime - (hptime_t) (((double)(mst->numsamples - 1) / mst->samprate * HPTMODULUS) + 0.5);
  
  /* Allocate & init per-trace stats structure if needed */
  if ( ! mst->prvtptr )
    {
      if ( ! (mst->prvtptr = malloc (sizeof(TraceStats))) )
	{
	  logit ("et", "Cannot allocate buffer for trace stats!\n");
	  return -1;
	}
      
      ((TraceStats *)mst->prvtptr)->earliest = HPTERROR;
      ((TraceStats *)mst->prvtptr)->latest = HPTERROR;
      ((TraceStats *)mst->prvtptr)->update = HPTERROR;
      ((TraceStats *)mst->prvtptr)->xmit = HPTERROR;
      ((TraceStats *)mst->prvtptr)->pktcount = 0;
      
      ((TraceStats *)mst->prvtptr)->xmit = HPTERROR;
      ((TraceStats *)mst->prvtptr)->pktcount = 0;
      ((TraceStats *)mst->prvtptr)->reccount = 0;
    }
  
  ((TraceStats *)mst->prvtptr)->update = dlp_time();
  ((TraceStats *)mst->prvtptr)->pktcount += 1;
  
  if ( (recordspacked = packtraces (mst, 0, HPTERROR)) < 0 )
    {
      logit ("et", "Cannot pack trace buffer or send records!\n",
	     recordspacked);
      logit ("et", "  %s.%s.%s.%s  ns=%d  dt=%4.1f  type=%s  sampletype: %c\n",
	     wfhead->sta, wfhead->chan, wfhead->net, wfhead->loc,
	     wfhead->nsamp, wfhead->samprate, originaldatatype, mst->sampletype);
      return -1;
    }
  
  return recordspacked;
}  /* End of handlestream() */


/*********************************************************************
 * sendrecord:
 *
 * Routine called to send a mseed record somewhere 
 *	(to a file for now, mseed packets converter later).
 *
 *********************************************************************/
static void
sendrecord (char *record, int reclen, void *handlerdata)
{
  static MSRecord *msr = NULL;
  MSTrace *mst = handlerdata;
  TraceStats *stats;
  hptime_t endtime;
  char streamid[100];
  int writeack = 0;
  int rv;
 
  if ( ! record )
    return;
 
  /* Parse Mini-SEED header */
  if ( (rv = msr_unpack (record, reclen, &msr, 0, 0)) != MS_NOERROR )
    {
      ms_recsrcname (record, streamid, 0);
      logit ("et", "Error unpacking %s: %s", streamid, ms_errorstr(rv));
      return;
    }

  /* Generate stream ID for this record: NET_STA_LOC_CHAN/MSEED */
  msr_srcname (msr, streamid, 0);
  strcat (streamid, "/MSEED");
  
  /* Determine high precision end time */
  endtime = msr_endtime (msr);
  
  /* Send record to server, loop */
  while ( dl_write (dlcp, record, reclen, streamid, msr->starttime, endtime, writeack) < 0 )
    {
      if ( dlcp->link == -1 )
        dl_disconnect (dlcp);
      
      if ( stopsig )
        {
          logit ("et", "Termination signal with no connection to DataLink, the data buffers will be lost\n");
          stopsig = 2;
          break;
        }

      if ( ! reconnectinterval )
        {
          stopsig = 2;
          break;
        }
      else if ( dl_connect (dlcp) < 0 )
        {
          logit ("et", "Error re-connecting to DataLink server: %s, sleeping\n", dlcp->addr);
          dlp_usleep (reconnectinterval * 1e6);
        }
    }
  
  /* Update stats */
  if ( mst )
    {
      stats = (TraceStats *)mst->prvtptr;
      
      if ( stats->earliest == HPTERROR || stats->earliest > msr->starttime )
        stats->earliest = msr->starttime;
      
      if ( stats->latest == HPTERROR || stats->latest < endtime )
        stats->latest = endtime;
      
      stats->xmit = dlp_time();
      stats->reccount += 1;
    }
}


/*********************************************************************
 * packtraces:
 *
 * Package remaining data in buffer(s) into miniSEED records.  If mst
 * is NULL all streams will be packed, otherwise only the specified
 * stream will be packed.
 *
 * If the flush argument is true the stream buffers will be flushed
 * completely, otherwise records are only packed when enough samples
 * are available to fill a record.
 *
 * Returns the number of records packed on success and -1 on error.
 *********************************************************************/
static int packtraces (MSTrace *mst, int flush, hptime_t flushtime)
{
  static struct blkt_1000_s Blkt1000;
  static struct blkt_1001_s Blkt1001;
  static MSRecord *mstemplate = NULL;
  
  MSTrace *prevmst;
  void *handlerdata = mst;
  int trpackedrecords = 0;
  int packedrecords = 0;
  int flushflag = flush;
  int encoding;
  
  /* Set up MSRecord template, include blockette 1000 and 1001 */
  if ( (mstemplate = msr_init (mstemplate)) == NULL )
    {
      logit ("et", "Cannot initialize packing template\n");
      return -1;
    }
  else
    {
      mstemplate->dataquality = 'D';
      
      /* Add blockettes 1000 & 1001 to template */
      memset (&Blkt1000, 0, sizeof(struct blkt_1000_s));
      msr_addblockette (mstemplate, (char *) &Blkt1000,
                        sizeof(struct blkt_1001_s), 1000, 0);
      memset (&Blkt1001, 0, sizeof(struct blkt_1001_s));
      msr_addblockette (mstemplate, (char *) &Blkt1001,
                        sizeof(struct blkt_1001_s), 1001, 0);
    }
  
  if ( mst )
    {
      if ( mst->sampletype == 'f' )
	encoding = DE_FLOAT32;
      else if ( mst->sampletype == 'd' )
	encoding = DE_FLOAT64;
      else
	encoding = int32encoding;
      
      strcpy (mstemplate->network, mst->network);
      strcpy (mstemplate->station, mst->station);
      strcpy (mstemplate->location, mst->location);
      strcpy (mstemplate->channel, mst->channel);
      
      trpackedrecords = mst_pack (mst, sendrecord, handlerdata, 512,
                                  encoding, 1, NULL, flushflag,
                                  0, mstemplate);
      
      if ( trpackedrecords == -1 )
        return -1;
      
      packedrecords += trpackedrecords;
    }
  else
    {
      mst = mstg->traces;
      prevmst = NULL;
      
      while ( mst && stopsig != 2 )
        {
          if ( mst->numsamples > 0 )
            {
	      if ( mst->sampletype == 'f' )
		encoding = DE_FLOAT32;
	      else if ( mst->sampletype == 'd' )
		encoding = DE_FLOAT64;
	      else
		encoding = int32encoding;
	      
              /* Flush data buffer if update time is less than flushtime */
              if ( flush == 0 && mst->prvtptr && flushtime != HPTERROR )
                if (((TraceStats *)mst->prvtptr)->update < flushtime )
                  {
                    logit ("et", "Flushing data buffer for %s_%s_%s_%s\n",
                            mst->network, mst->station, mst->location, mst->channel);
                    flushflag = 1;
                  }
	      
	      strcpy (mstemplate->network, mst->network);
	      strcpy (mstemplate->station, mst->station);
	      strcpy (mstemplate->location, mst->location);
	      strcpy (mstemplate->channel, mst->channel);
	      
              trpackedrecords = mst_pack (mst, sendrecord, handlerdata, 512,
                                          encoding, 1, NULL, flushflag,
                                          0, mstemplate);
	      
              if ( trpackedrecords == -1 )
                return -1;
	      
              packedrecords += trpackedrecords;
            }

          /* Remove trace buffer entry if no samples remaining */
          if ( mst->numsamples <= 0 )
            {
              MSTrace *nextmst = mst->next;
	      
              if ( verbose )
                logmststats (mst);
	      
              if ( ! prevmst )
                mstg->traces = mst->next;
              else
                prevmst->next = mst->next;

              mst_free (&mst);
	      
              mst = nextmst;
            }
          else
            {
              prevmst = mst;
              mst = mst->next;
            }
        }
    }

  return packedrecords;
}  /* End of packtraces() */


/*********************************************************************
 * logmststats:
 *
 * Log MSTrace stats.
 *********************************************************************/
static void logmststats ( MSTrace *mst )
{
  TraceStats *stats;
  char etime[50];
  char ltime[50];
  char utime[50];
  char xtime[50];
  
  stats = (TraceStats *) mst->prvtptr;
  ms_hptime2mdtimestr (stats->earliest, etime, 1);
  ms_hptime2mdtimestr (stats->latest, ltime, 1);
  ms_hptime2mdtimestr (stats->update, utime, 1);
  ms_hptime2mdtimestr (stats->xmit, xtime, 1);
  
  logit ("t", "%s_%s_%s_%s, earliest: %s, latest: %s\n",
	 mst->network, mst->station, mst->location, mst->channel,
	 (stats->earliest == HPTERROR) ? "NONE":etime,
	 (stats->latest == HPTERROR) ? "NONE":ltime);
  logit ("t", "  last update: %s, xmit time: %s\n",
	 (stats->update == HPTERROR) ? "NONE":utime,
	 (stats->xmit == HPTERROR) ? "NONE":xtime);
  logit ("t", "  pktcount: %lld, reccount: %lld\n",
	 (long long int) stats->pktcount,
	 (long long int) stats->reccount); 
}  /* End of logmststats() */


/***************************************************************************
 * logit_msg() and logit_err():
 *
 * Hooks for Earthworm logging facility.  These are used via function
 * pointers by the SeedLink library.
 ***************************************************************************/
void
logit_msg (char *msg)
{
  logit ("t", msg);
}

void
logit_err (char *msg)
{
  logit ("et", msg);
}


#ifndef WIN32
/***************************************************************************
 * term_handler:
 *
 * Signal handler routine to set the termination flag.
 ***************************************************************************/
static void
term_handler (int sig)
{
  stopsig = 1;
}
#endif
