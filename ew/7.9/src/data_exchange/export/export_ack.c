/*
 *   THIS FILE IS UNDER CVS -
 *   DO NOT MODIFY UNLESS YOU HAVE CHECKED IT OUT.
 *
 *    $Id: export_ack.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.12  2007/03/28 15:26:44  paulf
 *     minor MACOSX changes, mostly removing malloc.h since that should be defined in platform.h already
 *
 *     Revision 1.11  2007/02/26 14:23:17  paulf
 *     fixed long casting of time_t for heartbeat sprintf()
 *
 *     Revision 1.10  2007/02/23 05:50:28  stefan
 *     time_t
 *
 *     Revision 1.9  2005/07/25 16:16:31  friberg
 *     added _LINUX directive for S_un socket addr struct
 *
 *     Revision 1.8  2005/05/17 21:49:10  dietz
 *     Moved first earthworm heartbeat to before the socket initialization
 *     to get PID out to statmgr so restarts can be requested.
 *
 *     Revision 1.7  2005/05/12 21:18:33  dietz
 *     Increased processor variable to avoid overflow
 *
 *     Revision 1.6  2005/05/12 21:17:06  dietz
 *     Reverted to version 1.4 (turns out FlushOnStart command was a bad idea!)
 *
 *     Revision 1.4  2005/05/02 17:48:53  dietz
 *     Added some logging to SendQueue status check loop (start of SocketSender)
 *
 *     Revision 1.3  2005/04/29 19:04:54  dietz
 *     Changed ACK protocol slightly to allow import to automatically detect
 *     what type of export it's communicating with.
 *
 *     Revision 1.2  2005/04/28 18:52:57  dietz
 *     Re-implemented a socket reading thread so that acknowledgments can be rcvd
 *     asynchronously. Added a SendQueue to track status of msgs after they
 *     been pulled from the MessageStacker queue. If socket is broken/reconnected,
 *     any msgs in the SendQueue which haven't been acknowledged are re-sent.
 *     This version can be used with high data rates.
 *
 *     Revision 1.1  2005/04/25 22:29:39  dietz
 *     New module export_ack, export_scnl_ack (partner to import_ack).
 *     Expects to receive an acknowledgement packet after every packet it
 *     writes to the socket (even alive messages). Best for low-frequency,
 *     high importance packet transfer.
 *
 */

/*
*    export.c Revision history:
*     Revision 1.13  2005/04/22 17:14:18  dietz
*     added msglen arg to exportfilter_logmsg() to correct logging of msgs.
*
*     Revision 1.12  2005/03/23 19:13:01  dietz
*     Added more logging in "Verbose" mode using the new function
*     exportfilter_logmsg().
*
*     Revision 1.11  2004/04/20 22:52:52  dietz
*     moved export_actv.c and its sample configs from subdir export_actv
*     into subdir export to eliminated duplication of export filter code
*
*     Revision 1.10  2002/10/29 19:39:17  dietz
*     Moved tport_detach to the end of export_shutdown() to avoid
*     shutdown error on NT.
*
*     Revision 1.9  2002/07/19 23:02:43  dietz
*     changed to look for EW_PRIORITY_x returns from exportfilter()
*
*     Revision 1.8  2002/06/25 21:43:50  alex
*     mods for monitoring thread life. Alex
*
*     Revision 1.7  2002/03/18 18:15:11  patton
*     Made Logging changes.
*
*     Revision 1.6  2001/05/08 22:25:28  dietz
*     Changed to shut down gracefully if the transport flag is
*     set to TERMINATE or MyPid.
*
*     Revision 1.5  2001/04/17 16:43:17  davidk
*     Changed SocketTimeoutLength to an int instead of an unsigned int.
*     Changed code so that the minimum SocketTimeoutLength is
*     RcvAliveInt + 3 seconds (per Alex's recomendation).
*     Added comments to let others know that SocketTimeouts appear to
*     be essentially useless in export, because there is no code in export
*     that handles socket timeouts, and there is not a lot of point (as far
*     as I can see) to having code that handles timeouts.  Any code that
*     currently runs after a socket timeout is squashed by the main thread
*     doing a restart, courtesy of RcvAliveInt.
*
*     Revision 1.4  2000/07/24 19:05:35  lucky
*     Implemented global limits to module, installation, ring, and message type strings.
*     Also, set size of the Alive buffer to MAX_ALIVE_STR -- same as in import.
*
*     Revision 1.3  2000/07/10 21:01:01  lombard
*     Corrected return value check from accept_ew; removed some unused variables
*
*     Revision 1.2  2000/04/03 19:11:10  lombard
*     Cleaned up setting of Si=ocketTimeout: now defaults to same as
*     RecvAliveInt.
*
*     Revision 1.1  2000/02/14 17:23:11  lucky
*     Initial revision
*
*
*/

/*
 *   export.c
 *
 *   Program to read messages (of user-given logos) from
 *   the transport ring and to export them over a socket.
 *   Modified 10/8/96 to deal with binary messages. Alex.
 *   Modified 11/20/97 to perform Multi-Byte recv() (now recv_ew())
 *   calls and buffer information internally.  Also replaced state
 *   numbers with #defined constants  Davidk
 *
 *   Link export's object file with various filter object files to
 *   create customized export modules.  Dummy filter functions
 *   live in genericfilter.c.
 *
 *    Modified to re-send crippled message. Alex Nov12 99
 *
 *   Fix for horrible thread hang: after 180 days of uptime, Lynn's
 *   Solaris-based export failed: Apparently, the message stacking
 *   thread hung and the rest kept running, including all heartbeats.
 *   The result was very bad: no messages were being exported, but
 *   all heartbeats were being issued. The fix, as per suggestion
 *   by Dave Kragness, is to have the main thread set thread-indicator
 *   variables to 'off', and observe them being set back to 'alive' by
 *   the threads as proof that they're running through their working loops.
 *   Alex 6/22/2
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
#include <transport.h>
#include <imp_exp_gen.h>
#include <mem_circ_queue.h>
#include <priority_level.h>   /* for EW_PRIORITY definitions only */
#include <socket_ew.h>
#include "exportfilter.h"

#define EXPACK_VERSION  "1.15 2015.03.27"

/* CONSTANTS */
#define TIME_SLOP 3 /* seconds */

/* Threads in this source file
 *****************************/
thr_ret SocketSender  ( void * );    /* thread to write to socket            */
thr_ret SocketRecv    ( void * );    /* thread to read from socket           */
thr_ret MessageStacker( void * );    /* used to pass messages between main   */
                                     /*    thread and SocketSender           */
/* Other Thread things
 *********************/
#define THREAD_STACK 8192
static unsigned TidSocketSend;       /* SocketSender thread id                */
static unsigned TidSocketRecv;       /* SocketRecv thread id                  */
static unsigned TidStacker;          /* Thread moving messages from transport */
                                     /*   to queue                            */
/* Thread flags
 **************/
#define THREAD_OFF    0              /* thread has not been started         */
#define THREAD_TEST   1              /* thread alive, main is testing reset */
#define THREAD_ALIVE  2              /* thread alive and well               */
#define THREAD_ERR   -1              /* thread encountered error quit       */
/* The xxStatus variables are for filing complaints */
volatile int MessageStackerStatus = THREAD_OFF;
volatile int SocketSenderStatus   = THREAD_OFF;
volatile int SocketRecvStatus     = THREAD_OFF;
/* The xxAlive variables are for proving vitality */
volatile int MessageStackerAlive  = THREAD_OFF;
volatile int SocketSenderAlive    = THREAD_OFF;
volatile int SocketRecvAlive      = THREAD_OFF;

/* Socket status values
 ***********************/
#define SOCK_CLOSED      0           /* No sockets set up yet                     */
#define SOCK_NEW         1           /* New socket net yet connected              */
#define SOCK_NOT_CONN    2           /* No connection after timeout               */
#define SOCK_CONNECTED   3           /* connected to client                       */
volatile int SocketStatus = SOCK_CLOSED;

/* Global socket things
 **********************/
SOCKET    PassiveSocket = 0;    /* Socket descriptor; passive socket     */
SOCKET    ActiveSocket;         /* Socket descriptor; active socket      */

/* Message stacking queue
 ************************/
QUEUE OutQueue;              /* from mem_circ_queue.h,c; sets up linked   */
                                     /*    list via malloc and free               */

/* Send Queue stuff: used for asynchronous tracking of acknowledgments
 *********************************************************************/
typedef struct _SEND_QUEUE {
  char         *msg;                 /* pointer to raw message (not bin-escaped!)*/
  long          msglen;              /* length of raw message                    */
  MSG_LOGO      logo;                /* logo used in WriteSocket                 */
  unsigned char sequence;            /* sequence number used in WriteSocket      */
  volatile int  status;              /* current status of this message           */
} SEND_QUEUE;

SEND_QUEUE *SQ     = NULL;           /* allocate to proper length                */
char        *SQmsg = NULL;           /* allocate to SendQueueLen*MaxMsgSize;     */
                                     /*   have SQ.msg point in here.             */
int           SendQueueLen = 100;    /* default length of send queue (1-254 OK)  */
unsigned char SendSequence = 0;      /* next sequence number for WriteSocket;    */
                                     /*  also used as index into the SendQueue.  */
                                     /*  (value restricted: 0 to SendQueueLen)   */

/* Definitions of SendQueue status: */
#define SQS_NEW         1            /* new pkt dequeued from MessageStacker     */
#define SQS_SENT        2            /* pkt sent to socket; waiting for ACK      */
#define SQS_ACK_RCVD    3            /* ACK received, slot can be (re)used       */

/* Message Buffers to be allocated
 *********************************/
static char *Rawmsg        = NULL;   /* "raw" retrieved msg for main thread      */
static char *SSbinmsg      = NULL;   /* SocketSender's binary-escaped msg buffer */
static char *SRmsg         = NULL;   /* SocketRecv's incoming message buffer     */
static char *MSrawmsg      = NULL;   /* MessageStacker's "raw" retrieved message */
static char *MSfilteredmsg = NULL;   /* MessageStacker's "filtered" message to   */
                                     /*    be sent to client                     */
static char  inBuffer[INBUFFERSIZE]; /* working input buffer for SocketRecv      */

/* Heart beat stuff
 *******************/
time_t LastRcvAlive  = 0;  /* Last time we heard from our client */
time_t LastSendAlive = 0;  /* Last time we sent a heartbeat to our client */
time_t now;        /* current time, used for timing heartbeats */
time_t MyLastBeat;         /* time of last local (into Earthworm) hearbeat */
int    ImportAlive   = 1;  /* switch for noting if our import is sending heart beats ok */

extern int  errno;

static    SHM_INFO  Region;       /* shared memory region to use for i/o    */

#define   MAXLOGO   10
MSG_LOGO  GetLogo[MAXLOGO];     /* array for requesting module,type,instid */
short     nLogo;

char *Argv0;            /* pointer to executable name */
pid_t MyPid;        /* Our own pid, sent with heartbeat for restart purposes */

/* Things to read or derive from configuration file
 **************************************************/
static char    RingName[MAX_RING_STR]; /* name of transport ring for i/o    */
static char    MyModName[MAX_MOD_STR]; /* speak as this module name/id      */
static int     LogSwitch;              /* 0 if no logfile should be written */
static int     HeartBeatInt;           /* seconds between heartbeats        */
static char    ServerIPAdr[16];        /* server's IP address               */
static int     ServerPort;             /* Server's well-known port number   */
static long    MaxMsgSize;             /* max size for input/output msgs    */
static int     RingSize;           /* max messages in output circular buffer */
static int     SendAliveInt;           /* Send alive messages this often    */
static char    SendAliveText[MAX_ALIVE_STR]; /* Text of alive message. Max size is traditional */
static int     RcvAliveInt;            /* Expect client heartbeats this often */
static char    RcvAliveText[MAX_ALIVE_STR];  /* Text of client's alive messages */
static int     Verbose=0;              /* changed to 1 by Verbose command       */

static int     SocketTimeoutLength=0;  /* Length of timeouts on SOCKET_ew calls */
/* Socket timeouts are not handled (well) in export, so there is not a
 * whole lot of point to setting SocketTimeoutLength to any useful value.
 * Currently it is set to atleast the RcvAliveInt. */

static int     SOCKET_ewDebug = 0;     /* Set to 1 for SOCKET_ew debug statements*/

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          RingKey;       /* key of transport ring for i/o      */
static unsigned char InstId;        /* local installation id              */
static unsigned char MyModId;       /* Module Id for this program         */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;

/* Error messages used by export
 ********************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring        */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer      */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded  */
#define  ERR_SOCKETSEND    3   /* trouble writing to socket               */
#define  ERR_SOCKETACCEPT  4   /* error accepting socket connection       */
#define  ERR_QUEUE         5   /* error queueing message for sending      */
#define  ERR_NOCONN        6   /* no connection after timeout             */
#define  ERR_CONN          7   /* Connection accepted; cancels ERR_NOCONN */
#define  ERR_SOCKETREAD    8   /* trouble reading from socket             */
static char  errText[256];     /* string for log/error messages           */

/* Functions in this source file
 *******************************/
void  export_config  ( char * );
void  export_lookup  ( void );
void  export_status  ( unsigned char, short, char * );
void  export_free    ( void );
int   SendPacket     ( SEND_QUEUE *sq );
int   WriteToSocket  ( SOCKET, char *, long, MSG_LOGO *, unsigned char );
int   binEscape      ( char*, long , char* , long* , long );
void  export_shutdown( int );


int main( int argc, char **argv )
{
/* Socket variables: */
   int    on = 1;
   int    clientLen;
   char   client_ip[16];        /* IP address of client from inet_ntoa   */
   struct sockaddr_in  skt;
   struct sockaddr_in  client;

/* Other variables: */
   time_t        timeNow;       /* current time                          */
   int           res;
   long          recsize;   /* size of retrieved message             */
   MSG_LOGO      reclogo;   /* logo of retrieved message             */
   int           AcceptTimeout; /* time in milliseconds for accept_ew()  */
   int           sockErr;
   char         *ptr;           /* temp pointer for setting up SendQueue */
   int           i;

/* The timers and time limits for assertaining thread life */
   time_t messageStackerTimer;
   time_t socketSenderTimer;
   time_t socketRecvTimer;
   time_t messageStackerTimeout;
   time_t socketSenderTimeout;
   time_t socketRecvTimeout;

/* Catch broken socket signals
 ******************************/
#ifdef _SOLARIS
    (void)sigignore(SIGPIPE);
#endif

/* Check command line arguments
 ******************************/
   Argv0 = argv[0];
   if( argc != 2 )
   {
      fprintf( stderr, "Usage: %s <configfile>\nVersion: %s\n", Argv0, EXPACK_VERSION );
      return( 0 );
   }

/* Initialize name of log-file & open it
 ****************************************/
   logit_init( argv[1], 0, 512, 1 );

/* Read the configuration file(s) & lookup info from earthworm.h
 ****************************************************************/
   export_config( argv[1] );
   export_lookup();
   logit( "et" , "%s(%s): Read command file <%s>\n Program version %s\n",
         Argv0, MyModName, argv[1], EXPACK_VERSION );

/* Reinitilize the logging level
 ********************************/
   logit_init( argv[1], 0, 512, LogSwitch );

/* Set SOCKET_ew debugging on/off
 *********************************/
   setSocket_ewDebug(SOCKET_ewDebug);

/* Introduce ourselves
 **********************/
   logit("","29 Apr 2005 version: "
            "I do re-sends, I expect ACKs, I check thread life.\n");

/* Heartbeat parameters sanity checks
 ************************************/
/* Do a sanity check on the SocketTimeoutLength.
   If it is -1 (no timeout; block on the call) then
   leave it alone. */
   if( SocketTimeoutLength != -1 &&
       1000*(RcvAliveInt) >= SocketTimeoutLength )
   {
   /* WARNING: This code insures that export's socket thread will never
    * survive a socket timeout.  By setting them to greater than the
    * RcvAliveInt, the main thread will clobber the other threads and
    * perform a restart, any time that the socket actually hits a timeout.
    * This was done, because there is currently no code in export.c to handle
    * socket timeouts, (and no real reason to.)
    * DavidK 04/16/2001
    **************************************************************************/
      logit("","Socket timeout (%d ms) is less than incoming heartrate "
            "(%d sec)", SocketTimeoutLength, RcvAliveInt );
      SocketTimeoutLength = 1000*(RcvAliveInt);
      logit("", "Setting socket timeout to %d ms\n", SocketTimeoutLength);
   }

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
   if( ( Rawmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL )
   {
      logit( "e", "%s(%s): error allocating Rawmsg; exiting!\n",
            Argv0, MyModName );
      export_free();
      return( -1 );
   }

/* Buffers for SocketSender thread: */
   if( ( SSbinmsg = (char *) malloc(MaxMsgSize*2+1) ) ==  NULL )
   {
      logit( "e",
            "%s(%s): error allocating SSbinmsg; exiting!\n",
            Argv0, MyModName );
      export_free();
      return( -1 );
   }

/* Buffers for Send Queue */
   if( ( SQ = (SEND_QUEUE *)calloc((size_t)SendQueueLen,
                                    sizeof(SEND_QUEUE)) ) ==  NULL )
   {
      logit( "e", "%s(%s): error allocating SendQueue; exiting!\n",
            Argv0, MyModName );
      export_free();
      return( -1 );
   }
   if( ( SQmsg = (char *)calloc((size_t)SendQueueLen*MaxMsgSize+1,
                                 sizeof(char)) ) ==  NULL )
   {
      logit( "e", "%s(%s): error allocating SQmsg; exiting!\n",
            Argv0, MyModName );
      export_free();
      return( -1 );
   }
/* Initialize Send Queue */
   for( i=0, ptr=SQmsg; i<SendQueueLen; i++, ptr+=MaxMsgSize )
   {
      SQ[i].msg    = ptr;          /* point to memory in SQmsg   */
      SQ[i].status = SQS_ACK_RCVD; /* this slot can be (re)used! */
   }

/* Buffer for SocketRecv thread: */
   if( ( SRmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL )
   {
      logit( "e", "%s(%s): error allocating SRmsg; exiting!\n",
            Argv0, MyModName );
      export_free();
      return( -1 );
   }

/* Buffers for the MessageStacker thread: */
   if( ( MSrawmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL )
   {
      logit( "e", "%s(%s): error allocating MSrawmsg; exiting!\n",
            Argv0, MyModName );
      export_free();
      return( -1 );
   }
   if( ( MSfilteredmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL )
   {
      logit( "e", "%s(%s): error allocating MSfilteredmsg; exiting!\n",
            Argv0, MyModName );
      export_free();
      return( -1 );
   }

/* Attach to Input/Output shared memory ring; beat heart first time
 ******************************************************************/
   tport_attach( &Region, RingKey );
   export_status( TypeHeartBeat, 0, "" );
   time(&MyLastBeat);

/* Create a Mutex to control access to message stacking queue
 ************************************************************/
   CreateMutex_ew();

/* Initialize the message stacking queue
 ***************************************/
   initqueue( &OutQueue, (unsigned long)RingSize,(unsigned long)MaxMsgSize+1 );

/* step over all messages from transport ring
 *********************************************/
/* As Lynn pointed out: if we're restarted by startstop after hanging,
   we should throw away any of our messages in the transport ring.
   Else we could end up re-sending a previously sent message, causing
   time to go backwards... */
   do
   {
      res = tport_getmsg( &Region, GetLogo, nLogo,
                          &reclogo, &recsize, Rawmsg, MaxMsgSize );
   } while (res !=GET_NONE);


/************************/
   retry:
/************************/
/* Come here after communications with client have been broken,     */
/* the socket has been closed, and the threads have been terminated. */

/* Initialize the socket system
 *******************************/
   SocketSysInit();

   if( ( PassiveSocket = socket_ew( PF_INET, SOCK_STREAM, 0) ) == -1 )
   {
      logit( "et", "%s(%s): Error opening socket; exiting!\n", Argv0, MyModName );
      tport_detach( &Region );
      return( -1 );
   }
   SocketStatus = SOCK_NEW;

/* Fill in server's socket address structure
 ********************************************/
   memset( (char *) &skt, '\0', sizeof(skt) );
   skt.sin_family = AF_INET;
   skt.sin_port   = htons( (short)ServerPort );
#if defined(_LINUX) || defined(_MACOSX)
   if ((int)(skt.sin_addr.s_addr = inet_addr(ServerIPAdr)) == -1)
#else
   if ((int)(skt.sin_addr.S_un.S_addr = inet_addr(ServerIPAdr)) == -1)
#endif
   {
      logit( "e", "%s(%s): inet_addr failed for ServerIPAdr <%s>;"
            " exiting!\n", Argv0, MyModName, ServerIPAdr );
      return( -1 );
   }

/* Allows the server to be stopped and restarted
 ************************************************/
   on=1;
   if( setsockopt( PassiveSocket, SOL_SOCKET, SO_REUSEADDR,
                  (char *)&on, sizeof(char *) ) != 0 )
   {
      logit( "et", "%s(%s): Error on setsockopt; exiting!\n",
            Argv0, MyModName );
      perror("Export setsockopt");
      export_shutdown( -1 );
      return( -1 );
   }

/* Bind socket to a name
 ************************/
   if ( bind_ew( PassiveSocket, (struct sockaddr *) &skt, sizeof(skt)) )
   {
      logit("et", "%s(%s): error binding socket; exiting.\n",
            Argv0, MyModName);
      perror("Export bind error");
      export_shutdown( -1 );
      return( -1 );
   }

/* Prepare for connect requests
 *******************************/
   if( listen_ew( PassiveSocket, 0 ) )
   {
      logit("et", "%s(%s): socket listen error; exiting!\n",
            Argv0, MyModName );
      export_shutdown( 1 );
      return( -1 );
   }

/* Accept a connection (blocking)
 *********************************/
   clientLen = sizeof( client );
   logit( "et", "%s(%s): Waiting for new connection.\n", Argv0, MyModName );
   AcceptTimeout = 1000 * HeartBeatInt;

   while( (ActiveSocket = accept_ew( PassiveSocket,
                                    (struct sockaddr*) &client,
                            &clientLen, AcceptTimeout) ) == INVALID_SOCKET )
   {
      sockErr = socketGetError_ew();
      if( sockErr == WOULDBLOCK_EW )
      {
         if( SocketStatus == SOCK_NEW )
         {
            SocketStatus = SOCK_NOT_CONN;
            sprintf( errText, "No connection after %d seconds\n", HeartBeatInt );
            export_status( TypeError, ERR_NOCONN, errText );
         }
      }
      else
      {
         logit("et", "%s(%s): error on accept: %d %s\n\t; exiting!\n", Argv0,
               MyModName, sockErr, strerror(sockErr) );
         export_shutdown(-1);
         return( -1 );
      }
   /* Are we supposed to shut down? */
      if( tport_getflag( &Region ) == TERMINATE ) goto shutdown;
      if( tport_getflag( &Region ) == MyPid     ) goto shutdown;

   /* Beat our heart so statmgr knows we're alive */
      export_status( TypeHeartBeat, 0, "" );
      time(&MyLastBeat);
   }

   /* close the passive socket so others don't get fooled into getting a connection */
   closesocket_ew( PassiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW );

   strcpy( client_ip, inet_ntoa(client.sin_addr) );
   if( SocketStatus == SOCK_NOT_CONN )
   { /* we cried before, so now we have to let them know we're OK */
      sprintf( errText, "Connection accepted from IP address %s\n", client_ip);
      export_status( TypeError, ERR_CONN, errText );
   }
   else
   {
      logit("et", "%s(%s): Connection accepted from IP address %s\n",
            Argv0, MyModName, client_ip );
   }
   SocketStatus = SOCK_CONNECTED;

/* Start the message stacking thread if it isn't already running.
 ****************************************************************/
   if( MessageStackerStatus != THREAD_ALIVE)
   {
      if( StartThread( MessageStacker, (unsigned)THREAD_STACK, &TidStacker ) == -1 )
      {
         logit( "e",
               "%s(%s): Error starting  MessageStacker thread; exiting!\n",
               Argv0, MyModName );
         export_shutdown( -1 );
         return( -1 );
      }
      MessageStackerStatus  = THREAD_ALIVE;
      MessageStackerAlive   = THREAD_ALIVE; /* assume the best */
   /* There are no inherent time-delays in the stacker */
      messageStackerTimeout = TIME_SLOP;
   }

/* Start the socket writing thread
 *********************************/
   if( StartThread( SocketSender, (unsigned)THREAD_STACK, &TidSocketSend ) == -1 )
   {
      logit( "e", "%s(%s): Error starting SocketSender thread; exiting!\n",
            Argv0, MyModName );
      export_shutdown( -1 );
      return( -1 );
   }
/* It will set its status, but set it here to prevent timing problems */
   SocketSenderStatus = THREAD_ALIVE;
   SocketSenderAlive  = THREAD_ALIVE;
/* Timeout is at worst sending an old message, a heartbeat, and a message.
   Each is at worst one socketTimeout */
   socketSenderTimeout = 3*(SocketTimeoutLength/1000) + TIME_SLOP;

/* Start the socket reading thread
 *********************************/
   if( StartThread( SocketRecv, (unsigned)THREAD_STACK, &TidSocketRecv ) == -1 )
   {
      logit( "e", "%s(%s): Error starting SocketRecv thread; exiting!\n",
            Argv0, MyModName );
      export_shutdown( -1 );
      return( -1 );
   }
/* It will set its status, but set it here to prevent timing problems */
   SocketRecvStatus  = THREAD_ALIVE;
   SocketRecvAlive   = THREAD_ALIVE;
/* At most one slurp from the socket */
   socketRecvTimeout = (SocketTimeoutLength/1000) + TIME_SLOP;

/* set import heartbeat timer & flag
 ************************************/
   time(&LastRcvAlive);  /* from distant client */
   ImportAlive = 1;  /* assume we'll be hearing from our import */

/* Start main export monitoring loop for current connection
 **********************************************************/
   while( tport_getflag( &Region ) != TERMINATE &&
          tport_getflag( &Region ) != MyPid         )
   {
   /* See if our import has beat it's heart
    ****************************************/
      time(&timeNow);
      if(difftime(timeNow,LastRcvAlive) > (double)RcvAliveInt && RcvAliveInt != 0 )
      {
         logit("et", "%s(%s): lost import heartbeat\n", Argv0, MyModName);
         ImportAlive = 0; /* conclusion: no heartbeats from our import partner */
      }

   /* see how our socket thread is are feeling
    ******************************************/
   /* Each thread complains by setting its ...Status to -1, and exiting. */
      if( SocketSenderStatus != THREAD_ALIVE  ||
          SocketRecvStatus   != THREAD_ALIVE  ||
          ImportAlive        != 1 )
      {
         logit("et", "%s(%s) restarting. This procedure may hang. "
               "Make sure restartMe is set in my .desc file\n",
               Argv0, MyModName  );
      /* The code below occasionally hangs before the "waiting for new connection"
       * statement. The solution is to set the "restartMe" option in the .desc
       * file, which will cause me to be restarted by statmgr/startstop. */
      /* One cause of hanging is that KillThread() does nothing about mutexes held
       * by the threads being killed. If a thread holds a mutex when it is killed,
       * the mutex is held forever. It would be better to use thread cancellation
       * once earthworm is free from Solaris threads. PNL, 1/10/00 */
         closesocket_ew( ActiveSocket,  SOCKET_CLOSE_IMMEDIATELY_EW );
         SocketStatus = SOCK_CLOSED;
      /* Stop the socket threads, but leave the MessageStacker running. */
         (void)KillThread(TidSocketSend);
         SocketSenderStatus = THREAD_OFF;
         SocketSenderAlive  = THREAD_OFF;
         (void)KillThread(TidSocketRecv);
         SocketRecvStatus   = THREAD_OFF;
         SocketRecvAlive    = THREAD_OFF;

         sleep_ew(1000);          /* give threads a second to die */

      /* go & re-try from scratch */
         goto retry;
      }

   /* See if threads are really running
    ************************************/
   /* The scheme is to knock down the thread's 'alive' flags
      and observe that the threads are healthy enough to
      raise them up again within a reasonable amount of time */
      if( MessageStackerAlive == THREAD_ALIVE ) /* Hurray, it's running */
      {
         MessageStackerAlive = THREAD_TEST; /* see if it gets reset */
         time( &messageStackerTimer );      /* within the timelimit */
      }
      else
      {
         if( difftime( messageStackerTimer, time(NULL)) > messageStackerTimeout )
         { /* Very weird and very bad. Exit */
            logit("et","%s(%s): MessageStacker thread hung. Exiting\n",
                  Argv0, MyModName);
            export_shutdown(-1);
            return( -1 );
         }
      }

      if( SocketSenderAlive == THREAD_ALIVE ) /* Hurray, it's running */
      {
         SocketSenderAlive = THREAD_TEST; /* see if it gets reset */
         time( &socketSenderTimer );      /* within the timelimit */
      }
      else
      {
         if( difftime( socketSenderTimer, time(NULL)) > socketSenderTimeout )
         { /* Very weird and very bad. Exit */
            logit("et","%s(%s): SocketSender thread hung. Exiting\n",
                  Argv0, MyModName);
            export_shutdown(-1);
            return( -1 );
         }
      }

      if( SocketRecvAlive == THREAD_ALIVE ) /* Hurray, it's running */
      {
         SocketRecvAlive = THREAD_TEST; /* see if it gets reset */
         time( &socketRecvTimer );      /* within the timelimit */
      }
      else
      {
         if( difftime( socketRecvTimer, time(NULL)) > socketRecvTimeout )
         { /* Very weird and very bad. Exit */
            logit("et","%s(%s): SocketRecv thread hung. Exiting\n",
                  Argv0, MyModName);
            export_shutdown(-1);
            return( -1 );
         }
      }

   /* Beat the heart into the transport ring
    ****************************************/
      time(&now);
      if( difftime(now,MyLastBeat) > (double)HeartBeatInt )
      {
         export_status( TypeHeartBeat, 0, "" );
         time(&MyLastBeat);
      }

   /* take a brief nap; added 970624:ldd
    ************************************/
      sleep_ew(100);
   } /*end while of monitoring loop */

shutdown:
/* Shut it down
 ***************/
   logit("t", "%s(%s): termination requested; exiting!\n",
         Argv0, MyModName );
   export_shutdown(0);
   exit(0);
   return(0);
}
/*************************** end of main **************************/


/******************************************************************
 * export_shutdown()  take care of all final matters and exit     *
 ******************************************************************/
void export_shutdown( int status )
{
/* Stop threads
 *********************************/
   if( SocketSenderStatus   != THREAD_OFF ) (void)KillThread(TidSocketSend);
   if( SocketRecvStatus     != THREAD_OFF ) (void)KillThread(TidSocketRecv);
   if( MessageStackerStatus != THREAD_OFF ) (void)KillThread(TidStacker);

/* Close sockets if they exist
 *****************************/
   if( SocketStatus == SOCK_CONNECTED )
      closesocket_ew( ActiveSocket,  SOCKET_CLOSE_IMMEDIATELY_EW );
   if( SocketStatus != SOCK_CLOSED )
      closesocket_ew( PassiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW );

/* Clean up filter, memory, etc
 ******************************/
   exportfilter_shutdown();
   export_free();
   tport_detach( &Region );
   exit( status );
}

/**********************************************************************
 *                          SocketRecv()                              *
 * Read socket for client heartbeats and set global variable showing  *
 * time of last heartbeat. Main thread will take it from there.       *
 * Also read socket for acknowledgments.                              *
 *                                                                    *
 * If things go badly, set the global SocketStatus non-zero to alert  *
 * the main thread.                                                   *
 **********************************************************************/

thr_ret SocketRecv( void *dummy )
{
   static int  state;
   static char chr;
   static int  nr;                 /* total #chars in inBuffer     */
   static long inchar;             /* current position in inBuffer */
   static long nchar;              /* current position in SRmsg    */
   static char startCharacter=STX; /* ASCII STX characer  */
   static char endCharacter  =ETX; /* ASCII ETX character */

   SocketRecvStatus = THREAD_ALIVE;

/* Initialize these variables once per active socket
 ***************************************************/
/* Working loop: receive and process messages
 * We are either (0) initializing,
 *               (1) looking for a message start,
 *               (2) assembling a message
 * The variable "state' determines our mood.
 *********************************************/
   state=SEARCHING_FOR_MESSAGE_START;  /* we're initializing */

/* Start of New code Section DK 11/20 Multi-byte Read
 * Set inchar to be nr-1, so that when inchar is incremented, they will be the
 * same and a new read from socket will take place.  Set chr to 0 to indicate
 * a null character, since we haven't read any yet.
 */
   inchar=-1;
   nr=0;
   chr=0;
/* End of New code Section */

   while(1)  /* loop over socket reads */
   {
   /* Tell the main thread we're OK. Do this each pass thru the
    * loop, as the main thread will keep resetting it to THREAD_OFF,
    * just to see us raise it as a sign of continued life.
    ****************************************************************/
      SocketRecvAlive = THREAD_ALIVE;

   /* Read from buffer
    ******************/
   /* Get next char operation */
      if (++inchar == nr)
      {
      /* Read from socket operation */
         nr=recv_ew(ActiveSocket,inBuffer,INBUFFERSIZE,0,SocketTimeoutLength);
         if (nr<=0)  /* Connection Closed */
         {
            sprintf (errText, "%s(%s): recv_ew()", Argv0, MyModName);
            SocketPerror(errText);
            logit("et", "%s(%s): Bad socket read: %d\n", Argv0, MyModName, nr);
            SocketRecvStatus = THREAD_ERR; /* complain to main thread     */
            KillSelfThread();              /* main thread will restart us */
            return THR_NULL_RET;           /* make sure this thread exits */
         }
         inchar=0;
      /* End of Read from socket operation */
      }
      chr=inBuffer[inchar];
   /* End of Get next char operation */

   /* Initialization:
    * Throw all away until we see a start character
    ***********************************************/
      if (state==SEARCHING_FOR_MESSAGE_START)
      {
         if (chr == startCharacter) state=ASSEMBLING_MESSAGE; /* OK to start assembing message */
         nchar=0; /* next character position to load */
         continue;
      }

   /* Seeking message start:
    * The next char had better be a start character
    ***********************************************/
      if (state==EXPECTING_MESSAGE_START)
      {
         nchar=0;
         if (chr != startCharacter) /* then we're eating garbage */
         {
            logit("et", "%s(%s): unexpected character from client\n",
                  Argv0, MyModName);
            state=SEARCHING_FOR_MESSAGE_START; /* initialization mode */
            continue;
         }
         else   /* we got the message start, and we're in assembly mode */
         {
            nchar=0; /* start with firsts char position */
            state=ASSEMBLING_MESSAGE; /* go into assembly mode */
            continue;
         }
      }

   /* In the throes of assembling a message
    ***************************************/
      if (state==ASSEMBLING_MESSAGE)
      {
      /* We have a complete message
       ****************************/
         if (chr==endCharacter)   /* end of the message is at hand */
         {
            SRmsg[nchar]=0; /* terminate as a string */

         /* Client's heartbeat */
            if(strcmp(&SRmsg[9],RcvAliveText)==0)
            {
               if(Verbose) logit("et", "%s(%s): Received heartbeat\n",
                                 Argv0, MyModName);
            }

         /* Acknowledgment */
            else if(strncmp(&SRmsg[9],"ACK:",4)==0)
            {
               int  ack;
               char ackstr[4];
               strncpy( ackstr, &SRmsg[13], 3 );  ackstr[3] = 0;
               ack = atoi( ackstr );
               if( ack == HEARTSEQ ) {
                  if(Verbose) logit("et", "%s(%s): Received ACK:%3d (heartbeat)\n",
                                     Argv0, MyModName, ack);
               } else {
                  SEND_QUEUE *sq = &(SQ[ack%SendQueueLen]);
                  sq->status     = SQS_ACK_RCVD;
                  if(Verbose) {
                     logit("et", "%s(%s): Received ACK:%3d\n",
                            Argv0, MyModName, ack);
                     exportfilter_logmsg( sq->msg, sq->msglen, sq->logo.type,
                                         "ack-rcvd:       " );
                  }
               }
            }

         /* Got a non-heartbeat, non-acknowledgment message!  */
         /* We don't know to whom we're talking; stop thread! */
            else
            {
               logit("et",
                     "%s(%s): Weird message from client:\n%s\n",
                      Argv0, MyModName, SRmsg );
               SocketRecvStatus = THREAD_ERR; /* complain to main thread     */
               KillSelfThread();              /* main thread will restart us */
               return THR_NULL_RET;           /* make sure this thread exits */
            }

         /* Both heartbeats and acks prove that import is alive. */
         /* Reset the recv timer and get ready for the next msg  */
            time(&LastRcvAlive);
            state=EXPECTING_MESSAGE_START;
            continue;
         }

      /* Keep building the message
       ***************************/
         else
         {
            if( nchar < MaxMsgSize )  /* then there's still room in the buffer */
            {
               SRmsg[nchar++]=chr;
            }
            else  /* freakout: message won't fit */
            {
               logit("et",
                     "%s(%s): receive buffer overflow after %ld bytes\n",
                     Argv0, MyModName, MaxMsgSize);
               state=SEARCHING_FOR_MESSAGE_START; /* initialize again */
               nchar=0;
               continue;
            }
         }
      } /* end of state==ASSEMBLING_MESSAGE processing */
   }  /* end while of loop over characters in inBuffer */
}  /* end of SocketRecv thread */


/************************** SocketSender ******************************
 * Pull a messsage from the queue, and push it out the socket with a  *
 * known sequence number.                                             *
 *                                                                    *
 * If things go badly, set the global SocketStatus non-zero to alert  *
 * the main thread.                                                   *
 **********************************************************************/

thr_ret SocketSender( void *dummy )
{
   SEND_QUEUE *sq;   /* working pointer into SendQueue   */
   time_t      now, lasttime=0;  /* heartbeat things */
   MSG_LOGO    hrtlogo;
   int         ret;
   int         ifirst, ilast;    /* indices for looking in SendQueue */
   int         oldstatus, i;
   int         nresent = 0;

/* Tell the main thread we're ok; initialize stuff
 **************************************************/
   SocketSenderStatus = THREAD_ALIVE;
   SocketSenderAlive  = THREAD_ALIVE;
   hrtlogo.instid = InstId;
   hrtlogo.mod    = MyModId;
   hrtlogo.type   = TypeHeartBeat;

/* Check status of all msgs in SendQueue, resend if necessary
 ************************************************************/
   ifirst = SendSequence;
   ilast  = SendSequence + SendQueueLen;
   logit("et", "%s(%s): Reviewing SendQueue for non-ACKed msgs...\n",
          Argv0, MyModName );
   for( i=ifirst; i<ilast; i++ )
   {
      sq = &(SQ[i%SendQueueLen]);
      if( sq->status == SQS_ACK_RCVD ) continue;

   /* Must be SQS_NEW or SQS_SENT: send (re-send) packet!
    *****************************************************/
      oldstatus = sq->status;
      if( SendPacket( sq ) != 0 )
      {
         SocketSenderStatus = THREAD_ERR;  /* complain to main thread */
         KillSelfThread();             /* main thread will restart us */
         return THR_NULL_RET;          /* make sure this thread exits */
      }
      sq->status = SQS_SENT; /* updates to SQS_ACK_RCVD in SocketRecv */
      if( Verbose )
      {
         if( oldstatus == SQS_NEW ) {
            exportfilter_logmsg( sq->msg, sq->msglen, sq->logo.type,
                                "sent2socket:    " );
         } else {
            exportfilter_logmsg( sq->msg, sq->msglen, sq->logo.type,
                             "re-sent2socket: " );
         }
      }
      nresent++;
   } /* end check status for loop */
   if(nresent) logit("et", "%s(%s): Re-sent %d non-ACKed msgs from SendQueue.\n",
                      Argv0, MyModName, nresent );
   else        logit("et", "%s(%s): SendQueue was all ACKed; no msgs re-sent.\n",
                      Argv0, MyModName, nresent );

/*----------------------- main SocketSender loop ------------------------*/

   while( 1 )
   {
      sq = &(SQ[SendSequence]);  /* ptr to next available SendQueue slot */

   /* Tell the main thread we're still alive
    ****************************************/
      SocketSenderAlive = THREAD_ALIVE;

   /* Send alive message to client program if it's time to
    ******************************************************/
      time( &now );
      if( (now-lasttime)>=SendAliveInt  &&  SendAliveInt != 0 )
      {
         if( WriteToSocket( ActiveSocket, SendAliveText,
                (long)strlen(SendAliveText),
                            &hrtlogo, HEARTSEQ ) != 0 )
         {
            sprintf( errText, "error sending alive msg to socket" );
            export_status( TypeError, ERR_SOCKETSEND, errText );
            SocketSenderStatus = THREAD_ERR; /* complain to main thread */
            KillSelfThread();            /* main thread will restart us */
            return THR_NULL_RET;         /* make sure this thread exits */
         }
         if(Verbose) logit("et", "%s(%s): Sent alive msg\n", Argv0, MyModName );
         lasttime = now;
      }

   /* See if next SendQueue slot is ready to (re)fill.
    * If not, sleep a tiny bit and check again, until it's ACKed.
    * SQS_SENT is the only status that we have to do this for.
    * SQS_ACK_RCVD status slots can be reused immediately.
    * SQS_NEW status is impossible here (if SocketSender thread were
    * killed by main thread between dequeue and SendPacket, status
    * could be SQS_NEW, but on thread restart, status would've been
    * changed to SQS_SENT in "check status" loop before the main loop).
    *******************************************************************/
      if( sq->status == SQS_SENT )
      {
         if(Verbose) logit( "et", "Waiting for ACK in SendQueue[%d]\n",
                            SendSequence );
         while( sq->status != SQS_ACK_RCVD ) sleep_ew( 10 );
         if(Verbose) logit( "et", "Rcvd ACK; re-using SendQueue[%d]\n",
                            SendSequence );
      }

   /* Put next message from MessageStacker queue into SendQueue
    ***********************************************************/
      RequestMutex();
      ret = dequeue( &OutQueue, sq->msg, &(sq->msglen), &(sq->logo) );
      ReleaseMutex_ew();
      if( ret < 0 )  /* -1 means empty queue */
      {
         sleep_ew(100);
         continue;
      }
      sq->status   = SQS_NEW;
      sq->sequence = SendSequence;
      SendSequence = (SendSequence+1)%SendQueueLen;
      if(Verbose) exportfilter_logmsg( sq->msg, sq->msglen, sq->logo.type,
                                      "dequeued:       " );

   /* Send the just-dequeued message
    ********************************/
      if( SendPacket( sq ) != 0 )
      {
         SocketSenderStatus = THREAD_ERR; /* complain to main thread */
         KillSelfThread();            /* main thread will restart us */
         return THR_NULL_RET;         /* make sure this thread exits */
      }
      sq->status = SQS_SENT; /* updates to SQS_ACK_RCVD in SocketRecv */
      if(Verbose) exportfilter_logmsg( sq->msg, sq->msglen, sq->logo.type,
                                      "sent2socket:    " );

   } /* End of SocketSender while(1) loop */
}

/******************************************************************
 * SendPacket()  Do the binary escape and write to socket         *
 ******************************************************************/
int SendPacket( SEND_QUEUE *sq )
{
   long binlen;  /* length of binary-escaped message */

/* Make the message safe for binary text
 ***************************************/
   if( binEscape( sq->msg, sq->msglen,
                  SSbinmsg, &binlen, MaxMsgSize*2 ) < 0 )
   {
      logit("et","%s(%s): binEscape overflow; bad news!\n",
            Argv0, MyModName);
      return( -2 );
   }

/* Write the escaped message to the socket.
 ******************************************/
   if( WriteToSocket( ActiveSocket, SSbinmsg, binlen,
                      &(sq->logo), sq->sequence ) != 0 )
   {
      sprintf( errText, "error sending msg to socket." );
      export_status( TypeError, ERR_SOCKETSEND, errText );
      return( -1 );
   }
   return( 0 );
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
   int           NumOfTimesQueueLapped = 0; /* # msgs lost due to queue lap */

/* Start main export service loop for current connection
********************************************************/
   while( 1 )
   {
   /* Tell the main thread we're still ok
    *************************************/
      MessageStackerAlive = THREAD_ALIVE;

   /* Get a message from transport ring
    ************************************/
      res = tport_getmsg( &Region, GetLogo, nLogo,
                          &reclogo, &recsize, MSrawmsg, MaxMsgSize );
   /* if( res==GET_OK ) logit("et","Got message from transport of %ld bytes,
                          res=%d\n",recsize,res); */ /*DEBUG*/

   /* Wait if no messages for us
    ****************************/
      if( res == GET_NONE ) { sleep_ew(100); continue; }

   /* Check return code; report errors
    ***********************************/
      if( res != GET_OK )
      {
         if( res==GET_TOOBIG )
         {
            sprintf( errText, "msg[%ld] i%d m%d t%d too long for target",
                     recsize, (int) reclogo.instid,
                     (int) reclogo.mod, (int)reclogo.type );
            export_status( TypeError, ERR_TOOBIG, errText );
            continue;
         }
         else if( res==GET_MISS )
         {
            sprintf( errText, "missed msg(s) i%d m%d t%d in %s",(int) reclogo.instid,
                     (int) reclogo.mod, (int)reclogo.type, RingName );
            export_status( TypeError, ERR_MISSMSG, errText );
         }
         else if( res==GET_NOTRACK )
         {
            sprintf( errText, "no tracking for logo i%d m%d t%d in %s",
                     (int) reclogo.instid, (int) reclogo.mod, (int)reclogo.type,
                     RingName );
            export_status( TypeError, ERR_NOTRACK, errText );
         }
      }

   /* Process retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK)
    ***********************************************************/
      MSrawmsg[recsize] = '\0';
   /* pass it through the filter routine;
    * this may reformat, or reject as it chooses. */
      if ( exportfilter( MSrawmsg, recsize, reclogo.type, &MSfilteredmsg,
                        &filteredSize, &filteredType ) == EW_PRIORITY_NONE )  continue;
      reclogo.type = filteredType;  /* note the new message type */

   /* put it into the 'to be shipped' queue; */
   /* the thread SocketSender is in the biz of de-queueng and sending */
      RequestMutex();
      ret = enqueue( &OutQueue, MSfilteredmsg, filteredSize, reclogo );
      ReleaseMutex_ew();

      if( ret!= 0 )
      {
         if( ret==-2 )  /* Serious: quit */
         {  /* Currently, eneueue() in mem_circ_queue.c never returns this error. */
            sprintf(errText,"internal queue error. Terminating.");
            export_status( TypeError, ERR_QUEUE, errText );
            break;
         }
         if( ret==-1 )
         {
            sprintf(errText,"queue cannot allocate memory. Lost message.");
            export_status( TypeError, ERR_QUEUE, errText );
            if(Verbose) exportfilter_logmsg( MSfilteredmsg, filteredSize,
                                             reclogo.type, "cannot-queue:   " );
            continue;
         }
         if( ret==-3  &&  SocketStatus==SOCK_CONNECTED )  /* Log only while client's connected */
         {
         /* Queue is lapped too often to be logged to screen.
          * Log some circular queue laps to logfile.
          ***************************************************/
            NumOfTimesQueueLapped++;
            if( !(NumOfTimesQueueLapped%100)                             ||
               (!(NumOfTimesQueueLapped%5) && NumOfTimesQueueLapped<100) ||
                  NumOfTimesQueueLapped==1                                  )
            {
               logit( "et",
                      "%s(%s): Circular queue lapped %d times. Messages lost.\n",
                       Argv0, MyModName, NumOfTimesQueueLapped );
            }
            continue;
         }
      }
      if(Verbose) exportfilter_logmsg( MSfilteredmsg, filteredSize,
                                       reclogo.type, "enqueued:       " );
   } /* end of while */

/* We're quitting due to an error
 ********************************/
   MessageStackerStatus = THREAD_ERR; /* file a complaint to the main thread */
   KillSelfThread();                  /* main thread will restart us */
   return THR_NULL_RET;	/* should never reach here */
}


/***************************binEscape******************************
 *    make a binary message safe for shipping: insert an escape   *
 *    character before any bit pattern which looks like a sacred  *
 *    character. That will allow the receiving code to recognize  *
 *    it as of the data, and not as a sacred character.           *
 *    Sacred characters are: STX (02), ETX (03), and, of course,  *
 *                           ESC (27) itsself                     *
 *                                                                *
 *    Returns 0 if ok, -1 if sanitized message might not fit      *
 ******************************************************************/

int binEscape( char* inmsg, long inSize,
               char* outmsg, long* outSize, long outMaxSize )
{
   static char startCharacter=STX; /* from imp_exp_gen.h */
   static char endCharacter=ETX;
   static char escape=ESC;
   static char tmp;
   int    i;

/* Test output buffer size only once */
   if (outMaxSize < 2 * inSize) return(-1);

   *outSize = 0; /* index to next byte in outgoing message */
   for( i=0;i<inSize; i++ )   /*loop over bytes in input message */
   {
      tmp=inmsg[i];
      if( tmp==startCharacter || tmp==endCharacter || tmp==escape )
      {
         outmsg[(*outSize)++]=escape;
         outmsg[(*outSize)++]=tmp;
      }
      else
      {
         outmsg[(*outSize)++]=tmp;
      }
   }
   return(0);
}


/*************************** WriteToSocket ************************
 *    send a message logo and message to the socket               *
 *    returns  0 if there are no errors                           *
 *            -1 if any errors are detected                       *
 ******************************************************************/

int WriteToSocket( SOCKET ActiveSocket, char *msg, long msglength,
                   MSG_LOGO *logo, unsigned char seq )
{
   char etext[128];        /* error text */
   char asciilogo[17];     /* ascii version of seq#, outgoing logo */
   char startmsg = STX;    /* flag for beginning of message        */
   char endmsg   = ETX;    /* flag for end of message              */
   int  rc;

/* Send "start of transmission" flag, sequence#, ascii logo
 **********************************************************/
   sprintf( asciilogo, "%cSQ:%3d%3d%3d%3d", startmsg, (int) seq,
            (int) logo->instid, (int) logo->mod, (int) logo->type  );
   rc = send_ew( ActiveSocket, asciilogo, (int)strlen(asciilogo), 0, SocketTimeoutLength );
   if( rc != (int)strlen(asciilogo) )
   {
      sprintf( etext, "%s: socket send of message logo", Argv0 );
      SocketPerror( etext );
      return( -1 );
   }

/* Send message; break it into chunks if it's big!
 *************************************************/
   rc = send_ew( ActiveSocket, msg, msglength, 0, SocketTimeoutLength);
   if( rc == -1 )
   {
      sprintf( etext, "%s: message send error", Argv0 );
      SocketPerror( etext );
      return( -1 );
   }

/* Send "end of transmission" flag
 *********************************/
   rc = send_ew( ActiveSocket, &endmsg, 1, 0, SocketTimeoutLength);
   if( rc != 1 )
   {
      sprintf( etext, "%s: socket send EOT flag", Argv0 );
      SocketPerror( etext );
      return( -1 );
   }

   return( 0 );
}


/*****************************************************************************
 *  export_config() processes command file(s) using kom.c functions;         *
 *                    exits if any errors are encountered.               *
 *****************************************************************************/
void export_config( char *configfile )
{
   int      ncommand;     /* # of required commands you expect to process   */
   char     init[20];     /* init flags, one byte for each required command */
   int      nmiss;        /* number of required commands that were missed   */
   char    *com;
   char     processor[20];
   int      nfiles;
   int      success;
   int      i;
   char*    str;

/* Set to zero one init flag for each required command
 *****************************************************/
   ncommand = 13;
   for( i=0; i<ncommand; i++ )  init[i] = 0;
   nLogo = 0;

/* Open the main configuration file
 **********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
      logit( "e",
             "%s: Error opening command file <%s>; exiting!\n",
             Argv0, configfile );
      exit( -1 );
   }

/* Process all command files
 ***************************/
   while(nfiles > 0)   /* While there are command files open */
   {
      while(k_rd())    /* Read next line from active file  */
      {
         com = k_str();   /* Get the first token from line */

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
                      "%s: Error opening command file <%s>; exiting!\n",
                       Argv0, &com[1] );
               exit( -1 );
            }
            continue;
         }
         strcpy( processor, "export_config" );

      /* Process anything else as a command
       ************************************/
/*0*/    if( k_its("LogFile") ) {
            LogSwitch = k_int();
            init[0] = 1;
         }
/*1*/    else if( k_its("MyModuleId") ) {
            str = k_str();
            if(str) strcpy( MyModName, str );
            init[1] = 1;
         }
/*2*/    else if( k_its("RingName") ) {
            str = k_str();
            if(str) strcpy( RingName, str );
            init[2] = 1;
         }
/*3*/    else if( k_its("HeartBeatInt") ) {
            HeartBeatInt = k_int();
            init[3] = 1;
         }

      /* Enter installation & module & message types to get
       ****************************************************/
/*4*/    else if( k_its("GetMsgLogo") ) {
            if ( nLogo >= MAXLOGO ) {
               logit( "e",
                     "%s: Too many <GetMsgLogo> commands in <%s>",
                     Argv0, configfile );
               logit( "e", "; max=%d; exiting!\n", (int) MAXLOGO );
               exit( -1 );
            }
            if( ( str=k_str() ) ) {
               if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
                  logit( "e",
                        "%s: Invalid installation name <%s>", Argv0, str );
                  logit( "e", " in <GetMsgLogo> cmd; exiting!\n" );
                  exit( -1 );
               }
            }
            if( ( str=k_str() ) ) {
               if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
                  logit( "e",
                        "%s: Invalid module name <%s>", Argv0, str );
                  logit( "e", " in <GetMsgLogo> cmd; exiting!\n" );
                  exit( -1 );
               }
            }
            if( ( str=k_str() ) ) {
               if( GetType( str, &GetLogo[nLogo].type ) != 0 ) {
                  logit( "e",
                        "%s: Invalid msgtype <%s>", Argv0, str );
                  logit( "e", " in <GetMsgLogo> cmd; exiting!\n" );
                  exit( -1 );
               }
            }
            nLogo++;
            init[4] = 1;
      /*    printf("GetLogo[%d] inst:%d module:%d type:%d\n",
                     nLogo, (int) GetLogo[nLogo].instid,
                     (int) GetLogo[nLogo].mod,
                     (int) GetLogo[nLogo].type ); */  /*DEBUG*/
         }

      /* IP Address to bind to socket
       ******************************/
/*5*/    else if( k_its("ServerIPAdr") ) {
            str = k_str();
            if(str) {
               if( strlen(str) >= sizeof( ServerIPAdr ) ) {
                  logit( "e",
                        "%s: ServerIPAdr <%s> too long; exiting!\n",
                        Argv0, str );
                  exit( -1 );
               }
               strcpy( ServerIPAdr, str );
            }
            init[5] = 1;
         }

      /* Well-known server port number
       *******************************/
/*6*/    else if( k_its("ServerPort") ) {
            ServerPort = k_int();
            init[6] = 1;
         }

      /* Maximum size (bytes) for incoming/outgoing messages
       *****************************************************/
/*7*/    else if( k_its("MaxMsgSize") ) {
            MaxMsgSize = k_long();
            init[7] = 1;
         }

      /* Maximum number of messages in outgoing circular buffer
       ********************************************************/
/*8*/    else if( k_its("RingSize") ) {
            RingSize = k_long();
            init[8] = 1;
         }

      /* Interval (seconds) between alive msgs to client
       *************************************************/
/*9*/    else if( k_its("SendAliveInt") ) {
            SendAliveInt = k_int();
            init[9] = 1;
         }

      /* Text of alive message to client
       **********************************/
/*10*/   else if( k_its("SendAliveText") ) {
            str=k_str();
            if(str && strlen(str)<(size_t)MAX_ALIVE_STR) {
               strcpy(SendAliveText,str);
               init[10]=1;
            }
         }

      /* Interval (seconds) between alive msgs from client
       ***************************************************/
/*11*/   else if( k_its("RcvAliveInt") ) {
            RcvAliveInt = k_int();
            init[11] = 1;
         }

      /* Text of alive message from client
       ***********************************/
/*12*/   else if( k_its("RcvAliveText") ) {
            str=k_str();
            if(str && strlen(str)<(size_t)MAX_ALIVE_STR) {
               strcpy(RcvAliveText,str);
               init[12]=1;
            }
         }

      /* Optional: Reset length of Send Queue
       **************************************/
         else if( k_its("SendQueueLength") ) {
            int tmp;
            tmp = k_int();
            if( tmp < 1  ||  tmp > 254 ) {
               logit("et", "%s: Invalid SendQueueLength %d (valid values: 1-254);"
                     " exiting!\n", Argv0, tmp );
               exit( -1 );
            }
            SendQueueLen = (unsigned char)tmp;
         }

      /* Optional: Timeout in milliseconds for IP Socket routines
       **********************************************************/
         else if(k_its("SocketTimeout") ) {
            if(RcvAliveInt == 0)
            {
               SocketTimeoutLength = k_int();
            }
         }

      /* Optional: Turn on socket debug logging -> BIG LOG FILES!!
       ***********************************************************/
         else if(k_its("SocketDebug") ) {
            SOCKET_ewDebug = k_int();
         }

      /* Optional: Turn on debug logging -> BIG LOG FILES!!
       *****************************************************/
         else if( k_its("Verbose") ) {
            Verbose = 1;
         }



      /* Pass it off to the filter's config processor
       **********************************************/
         else if( exportfilter_com() ) strcpy( processor, "exportfilter_com" );

      /* Unknown command
       *****************/
         else {
            logit( "e", "%s: <%s> Unknown command in <%s>.\n",
                  Argv0, com, configfile );
            continue;
         }

      /* See if there were any errors processing the command
       *****************************************************/
         if( k_err() ) {
            logit( "e",
                  "%s: Bad <%s> command for %s() in <%s>; exiting!\n",
                  Argv0, com, processor, configfile );
            exit( -1 );
         }
      }  /* while reading from a file */
      nfiles = k_close();
   } /* while files are open */

   /* After all files are closed, check init flags for missed commands
   ******************************************************************/
   nmiss = 0;
   for( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
   if( nmiss ) {
       logit( "e", "%s: ERROR, no ", Argv0 );
       if ( !init[0] )   logit( "e", "<LogFile> "      );
       if ( !init[1] )   logit( "e", "<MyModuleId> "   );
       if ( !init[2] )   logit( "e", "<RingName> "     );
       if ( !init[3] )   logit( "e", "<HeartBeatInt> " );
       if ( !init[4] )   logit( "e", "<GetMsgLogo> "   );
       if ( !init[5] )   logit( "e", "<ServerIPAdr> "  );
       if ( !init[6] )   logit( "e", "<ServerPort> "   );
       if ( !init[7] )   logit( "e", "<MaxMsgSize> "   );
       if ( !init[8] )   logit( "e", "<RingSize> "     );
       if ( !init[9] )   logit( "e", "<SendAliveInt> " );
       if ( !init[10] )  logit( "e", "<SendAliveText>" );
       if ( !init[11] )  logit( "e", "<RcvAliveInt> "  );
       if ( !init[12] )  logit( "e", "<RcvAliveText>"  );
       logit( "e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

   return;
}

/****************************************************************************
 *  export_lookup( )   Look up important info from earthworm.h tables       *
 ****************************************************************************/
void export_lookup( void )
{
/* Look up keys to shared memory regions
 ***************************************/
   if( ( RingKey = GetKey(RingName) ) == -1 ) {
      logit( "e",
            "%s:  Invalid ring name <%s>; exiting!\n",
            Argv0, RingName);
      exit( -1 );
   }

/* Look up installations of interest
 ***********************************/
   if( GetLocalInst( &InstId ) != 0 ) {
      logit( "e",
            "%s: error getting local installation id; exiting!\n",
            Argv0 );
      exit( -1 );
   }

/* Look up modules of interest
 *****************************/
   if( GetModId( MyModName, &MyModId ) != 0 ) {
      logit( "e",
            "%s: Invalid module name <%s>; exiting!\n",
            Argv0, MyModName );
      exit( -1 );
   }

/* Look up message types of interest
 ***********************************/
   if( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      logit( "e",
            "%s: Invalid message type <TYPE_HEARTBEAT>; exiting!\n", Argv0 );
      exit( -1 );
   }
   if( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      logit( "e",
            "%s: Invalid message type <TYPE_ERROR>; exiting!\n", Argv0 );
      exit( -1 );
   }
   return;
}

/***************************************************************************
 * export_status() builds a heartbeat or error message & puts it into      *
 *                 shared memory.  Writes errors to log file & screen.     *
 ***************************************************************************/
void export_status( unsigned char type, short ierr, char *note )
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
   size = (int)strlen( msg );   /* don't include the null byte in the message */

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &Region, &logo, size, msg ) != PUT_OK )
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
 * export_free()  free all previously allocated memory                     *
 ***************************************************************************/
void export_free( void )
{
   free (Rawmsg);            /* "raw" retrieved msg for main thread      */
   free (SSbinmsg);          /* SocketSender's binary-escaped msg buffer */
   free (SRmsg);             /* SocketRecv's incoming message buffer     */
   free (MSrawmsg);          /* MessageStacker's "raw" retrieved message */
   free (MSfilteredmsg);     /* MessageStacker's "filtered" message to   */
                             /*  be sent to client                       */
   free (SQmsg);             /* SendQueue message storage                */
   free (SQ);                /* SendQueue parameter storage              */
   return;
}
