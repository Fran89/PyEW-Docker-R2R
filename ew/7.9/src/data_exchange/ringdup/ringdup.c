/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ringdup.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision ? 2013/10/02 ~13:00 chad
 *     miniSEED support for data types: short, float, double.  shorts are converted to ints.
 *     allow output miniSEED stream to match sample rate characteristics of input stream.
 *
 *     Revision 1.10  2007/09/20 21:00:27  paulf
 *     upped the size of the processor array
 *
 *     Revision 1.9  2007/03/28 16:51:38  paulf
 *     removed malloc.h and tweeked make for MACOSX
 *
 *     Revision 1.8  2007/02/26 14:23:17  paulf
 *     fixed long casting of time_t for heartbeat sprintf()
 *
 *     Revision 1.7  2007/02/23 05:35:53  stefan
 *     long to time_t
 *
 *     Revision 1.6  2004/05/27 16:45:56  dietz
 *     minor tweak
 *
 *     Revision 1.5  2002/03/22 18:43:43  lucky
 *     Fixed the configure routine which had unmatched '()' and '"' causing it not
 *     to compiled
 *
 *     Revision 1.4  2002/03/20 17:32:17  patton
 *     Made logit changes.
 *     JMP 03/20/2002
 *
 *     Revision 1.3  2001/05/08 22:43:43  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPid.
 *
 *     Revision 1.2  2000/07/24 19:07:06  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.1  2000/05/24 17:53:07  lucky
 *     Initial revision
 *
 *     Revision 1.1  2000/03/29 16:16:00  whitmore
 *     Initial revision
 *
 *
 *
 */

/*
 *   ringdup.c
 *
 *   Program to read messages (with user given logos) from one ring
 *   and deposit them in another.  This is mainly copied from
 *   export.
 *
 *   Whitmore - 3/21/00
 *
 *   Link ringdup's object file with various filter object files to
 *   create customized ringdup modules (as in export).  Dummy filter functions
 *   live in genericfilter.c.
 *
 *   Modified to re-send crippled message. Alex Nov12 99
 */

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
#include <swap.h>
#include <transport.h>
#include <imp_exp_gen.h>
#include <mem_circ_queue.h>

#include "exportfilter.h"

/* first version introduced late in the game, same for ringdup_generic, ringdup_scn, tbuf2mseed */
#define  PROGRAM_VERSION "0.0.9 2016-09-01"

#ifdef _TBUF2MSEED

#include <libmseed.h>
#include <trace_buf.h>

/* verbose level for 'libmseed' fns; 0=errors only, 1=basic diag, 2=details */
#define LMS_VERBOSE 0

/** Macro to scale a Unix/POSIX epoch time to a high precision time */
#define TB_EPOCH2DLTIME(X) X * (int64_t) 1000000
  
static MSTraceGroup *mstg = 0; /* for mseed buffering */
static void sendrecord (char *record, int reclen, void *handlerdata);	 /* what to do with a ms record when complete */
static  int packtraces (MSTrace *mst, int flush, hptime_t flushtime);
static  int handletbuf (char *msgbuf, int nbytes);
static unsigned char TypeMseed;
static mutex_t ms_mutex;          /* thread mutex for miniseed operations */

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
#endif

#ifdef _MSEED2TBUF
#include <libmseed.h>
#include <trace_buf.h>

/* verbose level for 'libmseed' fns; 0=errors only, 1=basic diag, 2=details */
#define LMS_VERBOSE 0

static  int handlemseed (char *msgbuf, int nbytes);
static unsigned char TypeTracebuf2;
#endif

/* Functions in this source file
 *******************************/
void  ringdup_config  ( char * );
void  ringdup_lookup  ( void );
void  ringdup_status  ( unsigned char, short, char * );
void  ringdup_free    ( void );

#if defined(_TBUF2MSEED) || defined(_MSEED2TBUF)
void logit_msg (char *msg);
void logit_err (char *msg);
#endif

/* Thread things
 ***************/
#define THREAD_STACK 8192
static unsigned tidDup;              /* Dup thread id */
static unsigned tidStacker;          /* Thread moving messages from transport */
                                     /*   to queue */

#define MSGSTK_OFF    0              /* MessageStacker has not been started      */
#define MSGSTK_ALIVE  1              /* MessageStacker alive and well            */
#define MSGSTK_ERR   -1              /* MessageStacker encountered error quit    */
volatile int MessageStackerStatus = MSGSTK_OFF;

QUEUE OutQueue;              /* from queue.h, queue.c; sets up linked    */
                                     /*    list via malloc and free              */
thr_ret Dup( void * );
thr_ret MessageStacker( void * );    /* used to pass messages between main thread */
                                     /*   and Dup thread */
/* Message Buffers to be allocated
 *********************************/
static char *Rawmsg = NULL;          /* "raw" retrieved msg for main thread      */
static char *SSmsg = NULL;           /* Dup's incoming message buffer   */
static MSG_LOGO Logo;        /* logo of message to re-send */
static char *MSrawmsg = NULL;        /* MessageStacker's "raw" retrieved message */
static char *MSfilteredmsg = NULL;   /* MessageStacker's "filtered" message to   */
                                     /*    be sent to client                     */

/* Timers
   ******/
time_t now;        /* current time, used for timing heartbeats */
time_t MyLastBeat;         /* time of last local (into Earthworm) hearbeat */
time_t MyLastFlush;        /* time of last buffer flush */

extern int  errno;

static  SHM_INFO  InRegion;     /* shared memory region to use for input  */
static  SHM_INFO  OutRegion;    /* shared memory region to use for output */

#define   MAXLOGO   10
MSG_LOGO  GetLogo[MAXLOGO];     /* array for requesting module,type,instid */
short     nLogo;

char *Argv0;            /* pointer to executable name */
pid_t MyPid;        /* Our own pid, sent with heartbeat for restart purposes */

/* Things to read or derive from configuration file
 **************************************************/
static char    InRing[MAX_RING_STR];          /* name of transport ring for input  */
static char    OutRing[MAX_RING_STR];         /* name of transport ring for output */
static char    MyModName[MAX_MOD_STR];       /* speak as this module name/id      */
static int     LogSwitch;           /* 0 if no logfile should be written */
static int     HeartBeatInt;        /* seconds between heartbeats        */
static long    MaxMsgSize;          /* max size for input/output msgs    */
static int     RingSize;            /* max messages in output circular buffer       */
static int     Debug=0;             /*  echo debug messages (mseed mostly) */
static long    UseOriginalLogo=1;   /*  if non-zero, output TYPE_TRACEBUF2 with original logo */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          InRingKey;     /* key of transport ring for input    */
static long          OutRingKey;    /* key of transport ring for output   */
static unsigned char InstId;        /* local installation id              */
static unsigned char MyModId;       /* Module Id for this program         */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;

/* Error messages used by export
 ***********************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring        */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer      */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded  */
#define  ERR_QUEUE         3   /* error queueing message for sending      */
static char  errText[256];     /* string for log/error messages           */

#ifdef _TBUF2MSEED
static int flushlatency = 300; /* Flush data buffers if not updated for latency in seconds */
static int int32encoding = DE_STEIM2;  /* miniSEED encoding for 32-bit integers */
#endif

int main( int argc, char **argv )
{
/* Other variables: */
   int           res;
   long          recsize;   /* size of retrieved message             */
   MSG_LOGO      reclogo;   /* logo of retrieved message             */
#ifdef _TBUF2MSEED
   int           rv;
#endif

   /* Check command line arguments
   ******************************/
   Argv0 = argv[0];
   if ( argc != 2 )
   {
      fprintf( stderr, "Usage: %s <configfile>\n", Argv0 );
      fprintf( stderr, "Version: V%s\n", PROGRAM_VERSION );
      return( 0 );
   }

   /* Initialize name of log-file & open it
   ****************************************/
   logit_init( argv[1], 0, 512, 1 );

   /* Read the configuration file(s)
   ********************************/
   ringdup_config( argv[1] );
   logit( "et" , "%s(%s): Read command file <%s>\n Version %s\n",
           Argv0, MyModName, argv[1], PROGRAM_VERSION );

   /* Look up important info from earthworm.h tables
   *************************************************/
   ringdup_lookup();

#if defined(_TBUF2MSEED) || defined(_MSEED2TBUF)
   /* Redirect libmseed logging to EW facility through shim functions */
   ms_loginit (&logit_msg, NULL, &logit_err, NULL); 
#endif

#ifdef _TBUF2MSEED
   logit ("et", "Using libmseed library version %s %s\n",
                                        LIBMSEED_VERSION, LIBMSEED_RELEASE);
   /* Initialize trace buffer */
   if ( ! (mstg = mst_initgroup ( mstg )) )
   {
      logit ("et", "tbuf2mseed: Cannot initialize MSTraceList\n");
      exit(1);
   }
   CreateSpecificMutex(&ms_mutex);  /* init mutex for miniseed operations */
#endif


   /* Reinitialize the logging level
   *********************************/
   logit_init( argv[1], 0, 512, LogSwitch );

   /* Get our own Pid for restart purposes
   ***************************************/
   MyPid = getpid();
   if(MyPid == -1)
   {
      logit("e", "%s(%s): Cannot get pid; exiting!\n", Argv0, MyModName);
      return(0);
   }

   /* Initialize export filter
   ***************************/
   if( exportfilter_init() != 0 )
   {
      logit("e", "%s(%s): Error in exportfilter_init(); exiting!\n",
            Argv0, MyModName );
      return(-1);
   }

   /* Allocate space for input/output messages for all threads
   ***********************************************************/
   /* Buffer for main thread: */
   if ( ( Rawmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL )
   {
      logit( "e", "%s(%s): error allocating Rawmsg; exiting!\n",
             Argv0, MyModName );
      ringdup_free();
      return( -1 );
   }

   /* Buffers for Dup thread: */
   if ( ( SSmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL )
   {
      logit( "e", "%s(%s): error allocating SSmsg; exiting!\n",
              Argv0, MyModName );
      ringdup_free();
      return( -1 );
   }

   /* Buffers for the MessageStacker thread: */
   if ( ( MSrawmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL )
   {
      logit( "e", "%s(%s): error allocating MSrawmsg; exiting!\n",
             Argv0, MyModName );
      ringdup_free();
      return( -1 );
   }
   if ( ( MSfilteredmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL )
   {
      logit( "e", "%s(%s): error allocating MSfilteredmsg; exiting!\n",
              Argv0, MyModName );
      ringdup_free();
      return( -1 );
   }

   /* Create a Mutex to control access to queue
   ********************************************/
   CreateMutex_ew();

   /* Initialize the message queue
   *******************************/
   initqueue( &OutQueue, (unsigned long)RingSize,(unsigned long)MaxMsgSize+1 );

   /* Attach to Input/Output shared memory ring
   ********************************************/
   tport_attach( &InRegion, InRingKey );
   tport_attach( &OutRegion, OutRingKey );

   /* step over all messages from transport ring
   *********************************************/
   /* As Lynn pointed out: if we're restarted by startstop after hanging,
      we should throw away any of our messages in the transport ring.
      Else we could end up re-sending a previously sent message, causing
      time to go backwards... */
   do
   {
     res = tport_getmsg( &InRegion, GetLogo, nLogo,
                         &reclogo, &recsize, Rawmsg, MaxMsgSize );
   } while (res !=GET_NONE);

   /* One heartbeat to announce ourselves to statmgr
   ************************************************/
   ringdup_status( TypeHeartBeat, 0, "" );
   time(&MyLastBeat);
   time(&MyLastFlush);

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

   /* Start the socket writing thread
   ***********************************/
   if ( StartThread(  Dup, (unsigned)THREAD_STACK, &tidDup ) == -1 )
   {
      logit( "e", "%s(%s): Error starting Dup thread; exiting!\n",
              Argv0, MyModName );
      tport_detach( &InRegion );
      tport_detach( &OutRegion );
      return( -1 );
   }

   /* Start main ringdup service loop
   **********************************/
   while( tport_getflag( &InRegion ) != TERMINATE  &&
          tport_getflag( &InRegion ) != MyPid         )
   {
     /* Beat the heart into the transport ring
      ****************************************/
      time(&now);
      if (difftime(now,MyLastBeat) > (double)HeartBeatInt )
      {
          ringdup_status( TypeHeartBeat, 0, "" );
	  MyLastBeat = now;
      }

#ifdef _TBUF2MSEED
      /* Check if idle data streams need flushing (and maintain 'mstg' buffer) */
      if ( difftime(now,MyLastFlush) > (double)flushlatency )
      {
        if (Debug)
          logit("et", "Debug: Performing packtraces(NULL)\n");
        RequestSpecificMutex(&ms_mutex);  /* grab thread mutex (when available) */
        rv = packtraces (NULL, 0,
                    (TB_EPOCH2DLTIME(now) - TB_EPOCH2DLTIME(flushlatency)));
        ReleaseSpecificMutex(&ms_mutex);  /* release thread mutex */
        if ( rv < 0 )
          logit ("et", "Error(s) flagged packing idle trace buffers\n");
        MyLastFlush = now;
      }
#endif

      /* take a brief nap; added 970624:ldd
       ************************************/
      sleep_ew(500);
   } /*end while of monitoring loop */

   /* Shut it down
   ***************/
#ifdef _TBUF2MSEED
   RequestSpecificMutex(&ms_mutex);    /* grab thread mutex (when available) */
   packtraces(NULL, 1, HPTERROR);
   ReleaseSpecificMutex(&ms_mutex);    /* release thread mutex */
#endif
   tport_detach( &InRegion );
   tport_detach( &OutRegion );
   exportfilter_shutdown();
   ringdup_free();
   logit("t", "%s(%s): termination requested; exiting!\n",
          Argv0, MyModName );
   return( 0 );
}
/* *******************  end of main *******************************
 ******************************************************************/

/**************************  Main Dup Thread   ***********************
*          Pull a messsage from the queue, and put it on OutRing     *
**********************************************************************/

thr_ret Dup( void *dummy )
{
   int      ret;
   long     msgSize;

   while (1)   /* main loop */
   {
     /* Get message from queue
      *************************/
     RequestMutex();
     ret=dequeue( &OutQueue, SSmsg, &msgSize, &Logo);
     ReleaseMutex_ew();
     if(ret < 0 )
     { /* -1 means empty queue */
       sleep_ew(500); /* wait a bit (changed from 1000 to 500 on 970624:ldd) */
       continue;
     }

     /* Duplicate message in OutRing
      ******************************/
#if !(defined(_TBUF2MSEED) || defined(_MSEED2TBUF))
     if (UseOriginalLogo==0) {  /* use current inst/modid instead */
       Logo.instid = InstId;
       Logo.mod    = MyModId;
     }
     if (tport_putmsg (&OutRegion, &Logo, msgSize, SSmsg) != PUT_OK)
         logit ("et", "Dup: Error sending message to out ring\n");
#endif

#ifdef _TBUF2MSEED
     RequestSpecificMutex(&ms_mutex);  /* grab thread mutex (when available) */
     handletbuf(SSmsg, msgSize);       /* send out miniseed packet data */
     ReleaseSpecificMutex(&ms_mutex);  /* release thread mutex */
#endif
#ifdef _MSEED2TBUF
     /* add miniseed packet data to tbuf2s and tport them out */
     handlemseed(SSmsg, msgSize);
#endif

   }   /* End of main loop */

   return THR_NULL_RET; /* Should never get here */
}

/********************** Message Stacking Thread *******************
 *           Move messages from transport to memory queue         *
 ******************************************************************/
thr_ret MessageStacker( void *dummy )
{
   long          recsize;   /* size of retrieved message             */
   MSG_LOGO      reclogo;       /* logo of retrieved message             */
   long      filteredSize;  /* size of message after user-filtering  */
   unsigned char filteredType;  /* type of message after filtering       */
   int       res;
   int       ret;
   int           NumOfTimesQueueLapped= 0; /* number of messages lost due to
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
            ringdup_status( TypeError, ERR_TOOBIG, errText );
            continue;
         }
         else if( res==GET_MISS )
         {
            sprintf( errText, "missed msg(s) i%d m%d t%d in %s",(int) reclogo.instid,
                    (int) reclogo.mod, (int)reclogo.type, InRing );
            ringdup_status( TypeError, ERR_MISSMSG, errText );
         }
         else if( res==GET_NOTRACK )
         {
            sprintf( errText, "no tracking for logo i%d m%d t%d in %s",
                     (int) reclogo.instid, (int) reclogo.mod, (int)reclogo.type,
                     InRing );
            ringdup_status( TypeError, ERR_NOTRACK, errText );
         }
      }

      /* Process retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK)
      ***********************************************************/
      MSrawmsg[recsize] = '\0';
      /* pass it through the filter routine: this may reformat,
         or reject as it chooses. */
      if ( exportfilter( MSrawmsg, recsize, reclogo.type, &MSfilteredmsg,
                        &filteredSize, &filteredType ) == 0 )  continue;
      reclogo.type = filteredType;  /* note the new message type */

      /* put it into the 'to be shipped' queue */
      /* the Dup thread is in the biz of de-queueng and sending */
      RequestMutex();
      ret=enqueue( &OutQueue, MSfilteredmsg, filteredSize, reclogo );
      ReleaseMutex_ew();

      if ( ret!= 0 )
      {
         if (ret==-2)  /* Serious: quit */
         {    /* Currently, eneueue() in mem_circ_queue.c never returns this error. */
        sprintf(errText,"internal queue error. Terminating.");
            ringdup_status( TypeError, ERR_QUEUE, errText );
        goto error;
         }
         if (ret==-1)
         {
            sprintf(errText,"queue cannot allocate memory. Lost message.");
            ringdup_status( TypeError, ERR_QUEUE, errText );
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
                     "%s(%s): Circular queue lapped 5 times. Messages lost.\n",
                      Argv0, MyModName);
               if (!(NumOfTimesQueueLapped % 100))
               {
                  logit( "et",
                        "%s(%s): Circular queue lapped 100 times. Messages lost.\n",
                         Argv0, MyModName);
               }
            }
            continue;
         }
      }
   } /* end of while */

   /* we're quitting
   *****************/
error:
   MessageStackerStatus = MSGSTK_ERR; /* file a complaint to the main thread */
   KillSelfThread(); /* main thread will restart us */
   return THR_NULL_RET; /* Should never get here */
}

/*****************************************************************************
 *  ringdup_config() processes command file(s) using kom.c functions;         *
 *                    exits if any errors are encountered.               *
 *****************************************************************************/
void ringdup_config( char *configfile )
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

/* Set to zero one init flag for each required command
 *****************************************************/
   ncommand = 8;
   for( i=0; i<ncommand; i++ )  init[i] = 0;
   nLogo = 0;

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
            strcpy( processor, "ringdup_config" );

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
  /*2*/     else if( k_its("InRing") ) {
                str = k_str();
                if(str) strcpy( InRing, str );
                init[2] = 1;
            }
  /*3*/     else if( k_its("OutRing") ) {
                str = k_str();
                if(str) strcpy( OutRing, str );
                init[3] = 1;
            }
  /*4*/     else if( k_its("HeartBeatInt") ) {
                HeartBeatInt = k_int();
                init[4] = 1;
            }


         /* Enter installation & module & message types to get
          ****************************************************/
  /*5*/     else if( k_its("GetMsgLogo") ) {
                if ( nLogo >= MAXLOGO ) {
                    logit( "e" ,
                            "%s: Too many <GetMsgLogo> commands in <%s>",
                             Argv0, configfile );
                    logit( "e" , "; max=%d; exiting!\n", (int) MAXLOGO );
                    exit( -1 );
                }
                if( ( str=k_str() ) != NULL ) {
                   if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
                       logit( "e" ,
                               "%s: Invalid installation name <%s>", Argv0, str );
                       logit( "e" , " in <GetMsgLogo> cmd; exiting!\n" );
                       exit( -1 );
                   }
                }
                if( ( str=k_str() ) != NULL ) {
                   if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
                       logit( "e" ,
                               "%s: Invalid module name <%s>", Argv0, str );
                       logit( "e" , " in <GetMsgLogo> cmd; exiting!\n" );
                       exit( -1 );
                   }
                }
                if( ( str=k_str() ) != NULL ) {
                   if( GetType( str, &GetLogo[nLogo].type ) != 0 ) {
                       logit( "e" ,
                               "%s: Invalid msgtype <%s>", Argv0, str );
                       logit( "e" , " in <GetMsgLogo> cmd; exiting!\n" );
                       exit( -1 );
                   }
                }
                nLogo++;
                init[5] = 1;
            }

         /* Maximum size (bytes) for incoming/outgoing messages
          *****************************************************/
  /*6*/     else if( k_its("MaxMsgSize") ) {
                MaxMsgSize = k_long();
                init[6] = 1;
            }

         /* Maximum number of messages in outgoing circular buffer
          ********************************************************/
  /*7*/     else if( k_its("RingSize") ) {
                RingSize = k_long();
                init[7] = 1;
            }

         /* Optional commands
          ********************/
  /*opt*/   else if( k_its("UseOriginalLogo") ) {
                UseOriginalLogo = k_long();
            }
  /*opt*/   else if( k_its("Debug") ) {
                Debug = k_long();
            }
#ifdef _TBUF2MSEED
            else if ( k_its("FlushLatency") ) {
                str = k_str();
                if (str) flushlatency = atoi (str);
            }
            else if ( k_its("Int32Encoding") ) {
                str = k_str();
                if ( str ) {
                    if ( ! strcmp (str, "STEIM1") )
                      int32encoding = DE_STEIM1;
                    else if ( ! strcmp (str, "STEIM2") )
                      int32encoding = DE_STEIM2;
                    else if ( ! strcmp (str, "INT32") )
                      int32encoding = DE_INT32;
                    else {
                        logit( "e", "%s: Unrecognized Int32Encoding: %s; exiting!\n",
                  	     Argv0, str);
                        exit( -1 );
                      }
                  }
              }
#endif

         /* Pass it off to the filter's config processor
      **********************************************/
            else if( exportfilter_com() ) strcpy( processor, "exportfilter_com" );

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

/* After all files are closed, check init flags for missed commands
 ******************************************************************/
   nmiss = 0;
   for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
   if ( nmiss ) {
       logit( "e", "%s: ERROR, no ", Argv0 );
       if ( !init[0] )  logit( "e", "<LogFile> "      );
       if ( !init[1] )  logit( "e", "<MyModuleId> "   );
       if ( !init[2] )  logit( "e", "<InRing> "     );
       if ( !init[3] )  logit( "e", "<OutRing> "     );
       if ( !init[4] )  logit( "e", "<HeartBeatInt> " );
       if ( !init[5] )  logit( "e", "<GetMsgLogo> "   );
       if ( !init[6] )  logit( "e", "<MaxMsgSize> "  );
       if ( !init[7] )  logit( "e", "<RingSize> "   );
       logit( "e" , "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }
   return;
}

/****************************************************************************
 *  ringdup_lookup( )   Look up important info from earthworm.h tables       *
 ****************************************************************************/
void ringdup_lookup( void )
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

/* these next 2 are needed for the mseed 2 from converters introduced in EW 7.7 */

#if defined(_MSEED2TBUF) 
   if ( GetType( "TYPE_TRACEBUF2", &TypeTracebuf2 ) != 0 ) {
      fprintf( stderr,
              "%s: Critical message type <TYPE_TRACEBUF2> is missing; exiting!\n", Argv0 );
      exit( -1 );
   }
#endif

#if defined(_TBUF2MSEED) 
   if ( GetType( "TYPE_MSEED", &TypeMseed ) != 0 ) {
      fprintf( stderr,
              "%s: Critical message type <TYPE_MSEED> is missing; exiting!\n", Argv0 );
      exit( -1 );
   }
#endif
   return;
}

/***************************************************************************
 * ringdup_status() builds a heartbeat or error message & puts it into      *
 *                 shared memory.  Writes errors to log file & screen.     *
 ***************************************************************************/
void ringdup_status( unsigned char type, short ierr, char *note )
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
    sprintf( msg, "%ld %ld\n", (long) t, (long) MyPid);
   else if( type == TypeError )
   {
    sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note);

    logit( "et", "%s(%s): %s\n", Argv0, MyModName, note );
   }

   size = (long)strlen( msg );   /* don't include the null byte in the message */

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
 * ringdup_free()  free all previously allocated memory                     *
 ***************************************************************************/
void ringdup_free( void )
{
   free (Rawmsg);               /* "raw" retrieved msg for main thread    */
   free (SSmsg);                /* Dup's incoming message buffer   */
   free (MSrawmsg);             /* MessageStacker's "raw" retrieved message */
   free (MSfilteredmsg);        /* MessageStacker's "filtered" message to   */
                                /*    be sent to client                     */
   return;
}



#ifdef _TBUF2MSEED
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
  static MSRecord *msr=NULL;
  char streamid[100];
  int rv;
 
  if ( ! record )
    return;
 
  /* Parse Mini-SEED header */
  if ( (rv = msr_unpack (record, reclen, &msr, 0, LMS_VERBOSE)) != MS_NOERROR )
    {
      ms_recsrcname (record, streamid, 0);
      logit ("et", "Error unpacking %s: %s\n", streamid, ms_errorstr(rv));
      return;
    }

  /* Generate stream ID for this record: NET_STA_LOC_CHAN/MSEED */
  msr_srcname (msr, streamid, 0);

  /* Determine high precision end time */
  /* endtime = msr_endtime (msr); */

  /* this is at risk of having the wrong LOGO for installation and sender at the end */
  Logo.type=TypeMseed;
  Logo.mod =MyModId; /* rewrite the module to come from me */
  msr_free(&msr);
  
  if (tport_putmsg (&OutRegion, &Logo, reclen, record) != PUT_OK)
         logit ("et", "Dup: Error sending mseed message to out ring\n");
/*
  logit("t", "Wrote 512byte mseed record for %s to OutRing\n", streamid);
*/
}

/*********************************************************************
 * handletbuf:
 *
 * Add the tbuf packet data to the trace buffer, pack the buffer and send
 * it on.  
 *
 * Returns the number of records packed/transmitted on success and -1
 * on error.
 *********************************************************************/
static int handletbuf (char *msg, int nbytes)
{
  static MSRecord *msr = NULL;
  static int32_t *int32buffer = NULL;
  static int32_t int32count = 0;
  int16_t *int16buffer = NULL;
  MSTrace *mst = NULL;
  int recordspacked = 0;
  TRACE2_HEADER *wfhead;          /* pntr to header of TYPE_TRACEBUF2 msg  */
  TracePacket *tp;
  time_t t;
  int idx;
  int rv;

  if ( ! msg || nbytes <= 0 )
    return -1;
  
  tp = (TracePacket *) msg;
  wfhead = &(tp->trh2);

  rv = WaveMsg2MakeLocal(wfhead);
  if ( rv != 0 )
    {
      logit("et", "Error with WaveMsg2MakeLocal(): %d\n", rv);
      return -1;
    }

  if (Debug) 
     logit("et", "Debug: %s.%s.%s.%s ns=%d dt=%4.1f type=%s nbytes=%d\n",
	wfhead->sta, wfhead->chan, wfhead->net, wfhead->loc,
	wfhead->nsamp, wfhead->samprate, wfhead->datatype, nbytes);

  if ( ! (msr = msr_init (msr)) )
    {
      logit("et", "Could not (re)initialize MSRecord\n");
      return -1;
    }

  /* Populate an MSRecord from a tbuf2 */
  ms_strncpclean (msr->network, wfhead->net, 2);
  ms_strncpclean (msr->station, wfhead->sta, 5);
  if (strcmp(wfhead->loc, "--") == 0) {
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
          if ( (int32buffer = (int32_t *) realloc (int32buffer,
                    sizeof(int32_t)*((int32_t)(msr->numsamples)))) == NULL )
            {
              logit ("et", "Cannot (re)allocate memory for 16->32 bit conversion buffer\n");
              return -1;
            }
          int32count = ((int32_t)(msr->numsamples));
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
      logit ("et", "Cannot add packet data to trace buffer (%s.%s.%s.%s)\n",
                       wfhead->sta, wfhead->chan, wfhead->net, wfhead->loc);
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

  time(&t);
  ((TraceStats *)mst->prvtptr)->update = t;
  ((TraceStats *)mst->prvtptr)->pktcount += 1;

  if ( (recordspacked = packtraces (mst, 0, HPTERROR)) < 0 )
  {
      logit ("et", "Cannot pack trace buffer or send records (%s.%s.%s.%s)\n",
                       wfhead->sta, wfhead->chan, wfhead->net, wfhead->loc);
      return -1;
  }
  return recordspacked;
}  /* End of handlestream() */

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
  MSTrace *prevmst;
  MSRecord *mstemplate = NULL;
  void *handlerdata = mst;
  int trpackedrecords = 0;
  int packedrecords = 0;
  int flushflag = flush;
  int encoding;
  int errflag = 0;
 
  if ( mst )
    {
      if ( mst->sampletype == 'f' )
        encoding = DE_FLOAT32;
      else if ( mst->sampletype == 'd' )
        encoding = DE_FLOAT64;
      else
        encoding = int32encoding;

      trpackedrecords = mst_pack (mst, sendrecord, handlerdata, 512,
                                  encoding, 1, NULL, flushflag,
                                  LMS_VERBOSE, mstemplate);

      if ( trpackedrecords == -1 )
        return -1;

      packedrecords += trpackedrecords;
    }
  else
    {
      mst = mstg->traces;
      prevmst = NULL;

      while ( mst )
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
                    if (Debug)
                      {
                        logit ("et", "Flushing data buffer for %s_%s_%s_%s\n",
                            mst->network, mst->station, mst->location, mst->channel);
                      }
                    flushflag = 1;
                  }

              trpackedrecords = mst_pack (mst, sendrecord, handlerdata, 512,
                                          encoding, 1, NULL, flushflag,
                                          LMS_VERBOSE, mstemplate);
/*              if (Debug)  */
/*                {         */
/*                  logit("et",  */
/*                      "Debug: packtraces(NULL) called mst_pack(), ret=%d\n",  */
/*                                                           trpackedrecords);  */
/*                }        */
              if ( trpackedrecords != -1 )
                {  /* no error; add to record count */
                  packedrecords += trpackedrecords;
                }
              else      /* error; set flag so -1 returned below */
                {       /* but keep processing so list is maintained */
                  errflag = 1;
                }
            }

          /* Remove trace buffer entry if no samples remaining */
          /*  or if error flagged while packing entry (above) */
          if ( mst->numsamples <= 0 || trpackedrecords == -1 )
            {
              MSTrace *nextmst = mst->next;
/*              if (Debug)  */
/*                {         */
/*                  logit("et",  */
/*                     "Debug: packtraces(NULL) removing entry, m->nsamps=%ld, mst_pack()=%d\n",  */
/*                                  (long)(mst->numsamples), trpackedrecords);                    */
/*                }        */

/*
              if ( verbose )
                logmststats (mst);
*/

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

  return !errflag ? packedrecords : -1;
}  /* End of packtraces() */

#endif /* _TBUF2MSEED */

#ifdef _MSEED2TBUF
#define TRACE2_UNDEF_STRING "  "
/*********************************************************************
 * handlemseed:
 *
 * Add convert the mseed TYPE_MSEED packet to tbuf2s and tport them out
 */
static int handlemseed (char *msg, int nbytes)
{
  static MSRecord *msr = NULL;
  int dataflag = 1; /* uncompress the data */
  TRACE2_HEADER *trh;
  int32_t *ds;
  double starttime;
  int actual_samples;         /* number of samples written to tracebuf */
  int current_sample;         /* pointer into msr mseed record data */
  char outbuf[MAX_TRACEBUF_SIZ];
  int ret, max_samps, i, datalen;
  int32_t *lp;


  ret = msr_unpack(msg, nbytes, &msr, (flag)dataflag, LMS_VERBOSE);
  if (ret != MS_NOERROR) {
      logit("e", "Error unpacking %d byte mseed record destined for OutRing\n", nbytes);
      return(0);
  }

/* now convert it to tbuf2's, as many as necessary */
  trh = (TRACE2_HEADER *)outbuf;
  lp = (int32_t *)(trh+1);
  max_samps=1000;

  memset((void*)trh, 0, sizeof(TRACE2_HEADER));
  trh->version[0] = TRACE2_VERSION0;
  trh->version[1] = TRACE2_VERSION1;
  strcpy(trh->sta, msr->station);
  strcpy(trh->chan, msr->channel);
  strcpy(trh->net, msr->network);
  if (strlen(msr->location) != 0 && strcmp(msr->location, "  ")!=0) {
      strncpy(trh->loc, msr->location, TRACE2_LOC_LEN-1);
      trh->loc[TRACE2_LOC_LEN-1] = '\0';
  } else {
      strcpy(trh->loc, LOC_NULL_STRING);
  }
  
  trh->quality[0] = msr->dataquality;
  trh->samprate = msr->samprate;
  
#ifdef _INTEL
  strcpy(trh->datatype, "i4");
#endif
#ifdef _SPARC
  strcpy(trh->datatype, "s4");
#endif

  current_sample = 0;
  starttime = msr->starttime/1000000.; 
  while (current_sample < msr->numsamples)
  {
      if (msr->numsamples - current_sample > max_samps) 
      { 
         actual_samples = max_samps;
      } else {
         actual_samples = (int)(msr->numsamples-current_sample);
      }
      trh->starttime = starttime + current_sample/msr->samprate;
      trh->endtime = trh->starttime + (actual_samples-1)/msr->samprate;
      trh->nsamp = actual_samples;
      ds = (int32_t *) msr->datasamples;
      for(i = 0; i < actual_samples; i++)
      {
        lp[i] = (int32_t) *(ds+current_sample);
        current_sample++;
      }
      datalen = actual_samples*4;
      Logo.type = TypeTracebuf2; /* we only do new tbuf2's  since we are SCNL */
      Logo.mod  = MyModId; /* rewrite the module to come from me */

  
      datalen = actual_samples*4;
      if (tport_putmsg (&OutRegion, &Logo, sizeof(TRACE_HEADER) + datalen, outbuf) != PUT_OK)
         logit ("et", "Dup: Error sending mseed message to out ring\n");
      if (Debug) 
         logit("et", "Debug: writing tbuf2 %s.%s.%s.%s  ns=%d  dt=%4.1f  type=%s\n",
	    trh->sta, trh->chan, trh->net, trh->loc,
	    trh->nsamp, trh->samprate, trh->datatype);

  } /* end of while writing tracebufs out for a given msr */
  return(1);
}
#endif /* _MSEED2TBUF */

#if defined(_TBUF2MSEED) || defined(_MSEED2TBUF)

/***************************************************************************
 * logit_msg() and logit_err():
 * Hooks for Earthworm logging facility.  These are used via function
 * pointers by the SeedLink library.
 ***************************************************************************/
void logit_msg (char *msg)
{
  logit ("t", "LIBMSEED: %s", msg);
}

void logit_err (char *msg)
{
  logit ("et", "LIBMSEED %s", msg);
}

#endif
