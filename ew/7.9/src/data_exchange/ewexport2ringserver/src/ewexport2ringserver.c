
/*******************************************************************************
 * ewexport2ringserver.c:
 *
 * Collect data from an Earthworm export_? process via TCP/IP, package
 * the received data as 512-byte miniSEED records and send them to a
 * DataLink (ringserver) server.
 *
 * The version of import_ack shipped with Earthworm v7.6 (2012) was used
 * as a starting point and shoe-horned into a more stand-alone program.
 *
 * Except for the import_ack.c and other bits from Earthworm:
 * Chad Trabant
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#ifndef _WIN32          /* if not Windows build then   */
#include <pthread.h>    /* use Unix threads            */
#define thr_ret void*   /* thread-function return type */
#else                   /* if Windows build then       */
#include <earthworm.h>  /* use Earthworm threads       */
#endif

#include <libmseed.h>
#include <libdali.h>

/* This file includes structures and defines for Earthworm */
#include "ewdefs.h"

#include "network.h"
#include "util.h"

#define PACKAGE "ewexport2ringserver"
#define VERSION "0.5"

/* Functions in this source file */
static int     WriteToSocket( int, char *, MSG_LOGO * );
static int     SocketHeartbeat( void );
static int     SocketSendAck( char * );
static thr_ret MessageReceiver( void * );

static int     handlestream (char *msg, int msglen);
static int     packtraces (MSTrace *mst, int flush, hptime_t flushtime);
static void    sendrecord (char *record, int reclen, void *handlerdata);
static void    logmststats (MSTrace *mst);

#ifndef _WIN32
static void    dummy_handler(int sig);
static void    term_handler(int sig);
static void    ThreadSignalHandler( int sig );
static int     ThreadStart( pthread_t *tid, void *func(void *), void *arg );
#else
#define THREAD_STACK 8192         /* thread stack size for Windows build */
#define close _close              /* use ISO conformant name */
typedef unsigned pthread_t;       /* thread-ID type for Windows build */
#endif
static int     ThreadKill ( pthread_t tid );
static void    print_timelog (char *msg);
static void    config_params( int argcount, char **argvec );
static void    usage(void);

static MSTraceGroup *mstg = 0;     /* Staging buffer of data for making miniSEED */

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

static int verbose     = 0;
static int stopsig     = 0;        /* 1: termination requested, 2: termination and no flush */

static int flushlatency = 300;     /* Flush data buffers if not updated for latency in seconds */
static int reconnectinterval = 10; /* Interval to wait between reconnection attempts in seconds */
static int int32encoding = DE_STEIM2; /* Encoding for 32-bit integer data */

static char *exaddr    = 0;        /* Export sender address in IP:port format */ 
static char *rsaddr    = 0;        /* DataLink/ringserver receiver address in IP:port format */
static DLCP *dlcp      = 0;        /* DataLink connection handle */

 /* Things to read or derive from configuration file */
static long    MaxMsgSize;          /* max size for input/output msgs    */
static int     MyAliveInt;          /* Seconds between sending alive     */
                                    /* message to foreign sender         */
static char    MyAliveString[MAX_ALIVE_STR] = "";  /* Text of above alive message */

static char   *SenderHost = 0;      /* Sender server host address */
static char   *SenderPort = 0;      /* Sender server port number */
static int     SenderHeartRate;     /* Expect alive messages this often from foreign sender */
static char    SenderHeartText[MAX_ALIVE_STR] = "" ;  /* Text making up the sender's heartbeat msg */

static int SocketTimeoutLength;     /* Timeout on network socket calls */
static int socktimeout;             /* Copy of SocketTimeoutLength potential modified */
static int HeartbeatDebug=0;        /* set to 1 in for heartbeat debug messages */

/* Globals: timers, etc, used by both threads */
volatile time_t LastServerBeat;      /* times of heartbeats from the server machine      */
volatile time_t MyLastSocketBeat;    /* time of last heartbeat to server - via socket    */
volatile int MessageReceiverStatus =0;    /* status of message receiving thread:         */
                                          /* -1 means bad news                           */
volatile int Sd;                     /* Socket descriptor                                */
char     *MsgBuf;                    /* incoming message buffer; used by receiver thread */
pthread_t TidMsgRcv = 0;             /* thread id. was type thread_t on Solaris!         */

MSG_LOGO      heartlogo;
MSG_LOGO      acklogo;

/* Definitions of export types
 *****************************/
#define  EXPORT_UNKNOWN      0   /* export type not discovered yet        */
#define  EXPORT_OLD          1   /* original no-acknowledgment export     */
#define  EXPORT_ACK          2   /* export which expects acknowledgements */


int main ( int argc, char **argv )
{
  time_t now;
  time_t tfirsttry;            /* time of first connection attempt */
  int    quit;
  int    retryCount;           /* to prevent flooding the log file */
  int    connected;            /* connection flag */
  
#ifndef _WIN32
  /* Signal handling, use POSIX calls with standardized semantics */
  struct sigaction sa;
  
  sa.sa_handler = dummy_handler;
  sa.sa_flags = SA_RESTART;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGALRM, &sa, NULL);
  
  sa.sa_handler = term_handler;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGQUIT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  
  sa.sa_handler = SIG_IGN;
  sigaction(SIGHUP, &sa, NULL);
  sigaction(SIGPIPE, &sa, NULL);

  /* Signal-handling function that needs to be inherited by threads */
  sa.sa_handler = ThreadSignalHandler;
  sigaction(SIGUSR1, &sa, NULL);
#endif
  
  /* Process command line arguments */
  config_params (argc, argv);
  
  /* Initialize trace buffer */
  if ( ! (mstg = mst_initgroup ( mstg )) )
    {
      ms_log (2, "Cannot initialize MSTraceList\n");
      exit (1);
    }
  
  /* Heartbeat parameters sanity checks */
  if ( SenderHeartRate >= SocketTimeoutLength )
    {
      ms_log (1,"Socket timeout (%d sec) is less than incoming heartrate (%d sec)\n",
	      SocketTimeoutLength, SenderHeartRate);
      SocketTimeoutLength = SenderHeartRate;
      ms_log (1, "Setting socket timeout to %d sec\n", SocketTimeoutLength);
    }
  
  socktimeout = SocketTimeoutLength;
  
  /* Allocate the message buffer */
  if ( ( MsgBuf = (char *) malloc (MaxMsgSize) ) == (char *) NULL )
    {
      ms_log (2, "Can't allocate buffer of size %ld bytes\n", MaxMsgSize);
      return -1;
    }
  
  /* Allocate and initialize DataLink connection description */
  if ( ! (dlcp = dl_newdlcp (rsaddr, PACKAGE)) )
    {
      fprintf (stderr, "Cannot allocation DataLink descriptor\n");
      exit (1);
    }
  
  /* Connect to destination DataLink server */
  if ( dl_connect (dlcp) < 0 )
    {
      ms_log (1, "Initial connection to DataLink server (%s) failed, will retry later\n", dlcp->addr);
      dl_disconnect (dlcp);
    }
  else if ( verbose )
    ms_log (1, "Connected to ringserver at %s\n", dlcp->addr);
  
  /* to prevent flooding the log file during long reconnect attempts */
  retryCount=0;  /* it may be reset elsewere */
  
  /*----------------------------------------------------------
    Connection loop: connect to export server endlessly until asked to stop.
    ------------------------------------------------------------*/
  
  while ( ! stopsig )
    {
      connected = 0;      /* not connected now */
      time (&tfirsttry);  /* store time of first connection attempt */
      
      /* Try for a network connection - and keep trying forever !! */
      retryCount++;
      
      if (retryCount < 4)
	ms_log (0, "Trying to connect to %s on port %s\n", SenderHost, SenderPort);
      
      if ( (Sd = my_connect (SenderHost, SenderPort, &socktimeout, verbose)) <= 0 )
	{
	  close (Sd);
	  
	  if (retryCount < 4)
	    ms_log (0, "Failed to connect. Waiting\n");
	  
	  if (retryCount == 4)
	    ms_log (0, "Suppressing repeated connect messages\n");
	  
	  safe_usleep (reconnectinterval * 1000000);
	  
	  continue; /* Reconnect */
	}
      
      ms_log (0, "Connected after %d seconds\n", reconnectinterval * (retryCount-1));
      retryCount = 0;
      
      /* Initialize Socketheartbeat things */
      MyLastSocketBeat = 0;             /* will send socketbeat first thing */
      time((time_t*)&LastServerBeat);   /* will think it just got a socketbeat
					   from the serving machine */
      
      /* Start the message receiver thread */
      MessageReceiverStatus = 0; /* set it's status flag to ok */
#ifndef _WIN32     /* if not Windows then do it the Unix way */
      quit = ThreadStart( &TidMsgRcv, MessageReceiver, NULL );
#else              /* if Windows then do it the Earthworm way */
      quit = StartThread( MessageReceiver, (unsigned)THREAD_STACK, &TidMsgRcv );
#endif
      if ( quit == -1 )
	{
	  ms_log (2, "cannot start MessageReceiver thread. Exiting.\n");
	  free(MsgBuf);
	  return -1;
	}
      
      /*-------------------------------------------------------------------------------
	Working loop: check on server heartbeat status, check on receive thread health.
	check for shutdown requests. If things go wrong, kill all threads and restart
	-------------------------------------------------------------------------------*/
      quit = 0; /* to restart or not to restart */
      while ( ! stopsig )
	{
	  safe_usleep (1000000); /* sleep one second. Remember, the receieve thread is awake */
	  time (&now);
	  
	  /* How's the server's heart? */
	  if ( difftime(now,LastServerBeat) > (double)SenderHeartRate && SenderHeartRate !=0 )
	    {
	      ms_log (0, "No heartbeat received for %d seconds. Restarting connection\n", SenderHeartRate);
	      quit = 1; /*restart*/
	    }
	  
	  /* How's the receive thread feeling? */
	  if ( MessageReceiverStatus == -1 )
	    {
	      ms_log (1, "MessageReceiver thread has quit. Restarting connection\n");
	      quit = 1; /*restart*/
	    }
	  
	  /* Restart preparations */
	  if ( quit == 1 )
	    {
	      ThreadKill (TidMsgRcv);
	      close (Sd);
	      quit = 0;
	      break; /* Reconnect */
	    }
	}  /* end of working loop */
    } /* End of reconnect loop */
  
  /* Flush all remaining data streams and close the connections */
  packtraces (NULL, 1, HPTERROR);
  
  ThreadKill (TidMsgRcv);
  
  close (Sd);
  
  if ( dlcp->link != -1 )
    dl_disconnect (dlcp);
  
  if ( verbose )
    {
      MSTrace *mst = mstg->traces;
      
      while ( mst )
        {
          logmststats (mst);
	  
          mst = mst->next;
        }
    }
  
  ms_log (1, "Terminating %s\n", PACKAGE);
  
  return 0;
} /* End of main() */


/********************** Message Receiver Thread *********************
 * Listen for server messages and heartbeats (set global variable   *
 * showing time of last heartbeat). Send acknowledgments for every  *
 * message received if our export expects to see them.              *
 ********************************************************************/

/* Modified to read binary messages, alex 10/10/96: 
 * The scheme (I got it from Carl) is define some sacred characters. 
 * Sacred characters are the start-of-message and end-of-message framing 
 * characters, and an escape character. The sender's job is to cloak 
 * unfortunate bit patters in the data which look like sacred characters 
 * by inserting before them an 'escape' character.  Our problem here is 
 * to recognize, and use, the 'real' start- and end-of-messge characters, 
 * and to 'decloak' any unfortunate look-alikes within the message body.
 */
/* Modified to write acknowledgments if necessary, ldd 4/29/2005. 
 */

static thr_ret MessageReceiver ( void *dummy )
{
  static int  state;
  static char chr, lastChr;
  static int  nr;
  static long nchar;                  /* counter for msg buffer (esc's removed) */
  static char startCharacter=STX;     /* ASCII STX characer                     */
  static char endCharacter=ETX;       /* ASCII ETX character */
  static char escape=ESC;             /* our escape character */
  static char inBuffer[INBUFFERSIZE]; /* buffer for socket receiving */
  static long inchar;                 /* counter for raw socket buffer (w/esc) */
  int         export_type;            /* flavor of export we're talking to */
  char        seqstr[4];              /* sequence string for acknowledgments */
  int         seq;                    /* sequence number */
  char       *msgptr = NULL;          /* pointer to beginning of logo in MsgBuf */
  int         msglen;                 /* length of asciilogo + message */

  char cType[4];
  dltime_t    flushtime = dlp_time(); /* Buffer flushing time stamp */
  
  /* Initialize stuff */
  MessageReceiverStatus = 0;    /* tell the main thread we're OK */
  export_type = EXPORT_UNKNOWN; /* don't know what kind of export is out there */
  
  /* Start of New code Section DK 11/20/97 Multi-byte Read 
   * Set inchar to be nr-1, so that when inchar is incremented, they will be 
   * the same and a new read from socket will take place.  
   * Set chr to 0 to indicate a null character, since we haven't read any yet.
   * Set nchar to 0 to begin building a new MsgBuffer (with escapes removed).
   */
  inchar = -1;
  nr     =  0;
  chr    =  0;
  nchar  =  0;  /* added 980209:LDD */
  /* End of New code Section */
  
  /* Working loop: receive and process messages, send acks
   *------------------------------------------------------
   * We are either (0) initializing: searching for message start
   *               (1) expecting a message start: error if not
   *               (2) assembling a message.
   * The variable "state' determines our mood
   **************************************************************/
  state = SEARCHING_FOR_MESSAGE_START; /* we're initializing */
  
  for (;;) /* loop over bytes read from socket */
    {
      /* Get next char operation */
      if ( ++inchar == nr )
	{
	  /* Read from socket operation */
	  nr = my_recv (Sd, inBuffer, INBUFFERSIZE-1, socktimeout);
	  
	  if ( nr <= 0 ) {   /* Connection Closed */
            ms_log (1, "Bad socket read: %d\n", nr );
            goto suicide;
	  }
	  inchar = 0;
	  /* End of Read from socket operation */
	  
	  /* Flush data buffers not updated for flushlatency seconds.
	   * This functionality only works if something, e.g. heartbeats,
	   * are being received. */
	  if ( flushlatency )
	    {
	      dltime_t now = dlp_time();
	      
	      if ( (now - flushtime) > DL_EPOCH2DLTIME(flushlatency) )
		{
		  if ( packtraces (NULL, 0, (now - DL_EPOCH2DLTIME(flushlatency))) < 0 )
		    {
		      ms_log (2, "Cannot pack trace buffers or send records!\n");
		    }
		  
		  flushtime = now;
		}
	    }
	}
      lastChr = chr;
      chr = inBuffer[inchar];
      /* End of Get next char operation */
            
      /* Initialization: throw all away until we see a naked start character */
      if ( state == SEARCHING_FOR_MESSAGE_START )
	{
	  /* Do we have a real start character? */
	  if ( lastChr != escape && chr == startCharacter )
	    {
	      state = ASSEMBLING_MESSAGE;  /* from now on, assemble message */
	      continue;
	    }
	}

      /* Confirm message start: 
       * the next char had better be a start character - naked, that is */
      if ( state == EXPECTING_MESSAGE_START )  
	{
	  /* Is it a naked start character? */
	  if ( chr==startCharacter && lastChr != escape ) /* This is it: message start!! */
	    {
	      nchar = 0;         /* start with first char position */
	      state = ASSEMBLING_MESSAGE; /* go into assembly mode */
	      continue;
	    }
	  else   /* we're eating garbage */
	    {
	      ms_log (1, "Unexpected character from export. Re-synching\n");
	      state = SEARCHING_FOR_MESSAGE_START; /* search for next start sequence */
	      continue;
	    }
	}

      /* In the throes of assembling a message */
      if ( state == ASSEMBLING_MESSAGE )
	{
	  /* Naked end character: end of the message is at hand */
	  if ( chr == endCharacter ) 
	    {
	      /* We have a complete message! */
	      MsgBuf[nchar] = 0;              /* terminate as a string  */
	      state=EXPECTING_MESSAGE_START;  /* reset for next message */ 
	      
	      /* Discover what type of export we're talking to */
	      if ( export_type == EXPORT_UNKNOWN )
		{
		  if ( strncmp( MsgBuf, "SQ:", 3 ) == 0 ) {
		    ms_log (0, "I will send ACKs to export.\n");
		    export_type = EXPORT_ACK;
		    
		    /* Set default heartbeat text */
		    if ( strlen(MyAliveString) == 0 )
		      strncpy (MyAliveString, "ImpAlive", MAX_ALIVE_STR-1);
		    if ( strlen(SenderHeartText) == 0 ) 
		      strncpy (SenderHeartText, "ExpAlive", MAX_ALIVE_STR-1);
		  }
		  else {
		    ms_log (0, "I will NOT send ACKs to export.\n");
		    export_type = EXPORT_OLD;
		    
		    /* Set default heartbeat text */
		    if ( strlen(MyAliveString) == 0 )
		      strncpy (MyAliveString, "alive", MAX_ALIVE_STR-1);
		    if ( strlen(SenderHeartText) == 0 ) 
		      strncpy (SenderHeartText, "alive", MAX_ALIVE_STR-1);
		  }
		}
	      
	      /* Set the working msg pointer based on type of export;
	       * Get sequence number and send ACK (if necessary) */
	      if ( export_type == EXPORT_ACK )
		{
		  if ( nchar < 15 )  /* too short to be a valid msg from export_acq! */
		    {
		      ms_log (1, "Skipping msg too short for ack-protocol\n%s\n", MsgBuf );
		      continue;
		    }
		  strncpy( seqstr, &MsgBuf[3], 3 );
		  seqstr[3] = 0;
		  seq       = atoi(seqstr); /* stash sequence number */
		  msgptr    = &MsgBuf[6];   /* point to beginning of ascii logo */
		  msglen    = nchar - 6;
		  if ( SocketSendAck( seqstr ) != 0 )
		    goto suicide; /* socket write error */
		}
	      else /* export_type == EXPORT_OLD */
		{
		  if ( nchar < 9 )  /* too short to be a valid msg from old exports! */
		    {
		      ms_log (1, "Skipping msg too short for non-ack-protocol\n%s\n", MsgBuf);
		      continue;
		    }
		  seq    = -1;     /* unknown sequence number */
		  msgptr = MsgBuf; /* point to beginning of ascii logo */
		  msglen = nchar;  
		}
	      
	      /* See if the message is server's heartbeat */
	      strncpy(cType,&msgptr[6],3);  cType[3]=0; 
	      if ( (unsigned char)atoi(cType) == TYPE_HEARTBEAT )
		{
		  if ( HeartbeatDebug )
		    {
		      ms_log (1, "Received heartbeat\n");
		    }
		  
		  if ( strcmp(&msgptr[9], SenderHeartText) ) 
		    {
		      ms_log (1, "Message is TYPE_HEARTBEAT but text (%s) does not match expected (%s)\n",
			      &msgptr[9], SenderHeartText); 
		    }
		}

	      /* Seq# told us to expect heartbeat, but text didn't match;
	       * might not be talking to the correct export! */
	      else if ( seq == HEARTSEQ )
		{
		  ms_log (1, "Rcvd socket heartbeat <%s> " 
			  "does not match expected <%s>; check config!\n",
			  &msgptr[9], SenderHeartText );
		  continue;
		}
	      
	      /* Otherwise, it must be a message to forward along */
	      else                              
		{
		  if ( HeartbeatDebug )
		    {
		      ms_log (1, "Received non-heartbeat\n"); 
		    }
		  
		  /* import_filter( msgptr, msglen ); */ /* process msg via user-routine */
		  handlestream (msgptr, msglen);
		}
	      
	      /* All messages and heartbeats prove that export is alive, */
	      /* so we will update our server heartbeat monitor now.     */
	      time((time_t*)&LastServerBeat); 
	      
	      /* This is a good time to do socket heartbeat stuff */
	      if( SocketHeartbeat() != 0 )
		goto suicide; /* socket write error */
	      continue;
	    }
	  
	  /* Process the message byte we just read: 
	   * we know it's not a naked end character */
	  else
	    {
	      /* Process an escape sequence */
	      if ( chr == escape )  
		{  
		  /* Read from buffer, or get more chars if buffer is empty */
		  /* Get next char operation */
		  if ( ++inchar == nr )
		    {
		      /* Read from socket operation */
		      nr = my_recv (Sd, inBuffer, INBUFFERSIZE-1, socktimeout);
		      
		      if ( nr<=0 ) {   /* Connection Closed */
			ms_log ( 2, "Bad socket read: %d\n", nr);
			goto suicide;  
		      }
		      inchar=0;
		      /* End of Read from socket operation */
		    }
		  lastChr=chr;
		  chr=inBuffer[inchar];
		  /* End of Get next char operation */
		  
		  /* Bad news: unknown escape sequence */
		  if ( chr != startCharacter  && 
		       chr != endCharacter    && 
		       chr != escape )
		    {
		      ms_log(1, "Unknown escape sequence in message. Re-synching\n");
		      state=SEARCHING_FOR_MESSAGE_START; /* search for next start sequence */
		      continue;
		    }
		  /* OK: it's a legit escape sequence; save the escaped byte */
		  else
		    {
		      MsgBuf[nchar++]=chr;
		      if ( nchar >= MaxMsgSize ) goto freak;  
		      continue;
		    }
		}
	      
	      /* Bad news: Naked (unescaped) start character */
	      if ( chr == startCharacter )
		{
		  ms_log (1, "Unescaped start character in message; re-synching\n");
		  state=SEARCHING_FOR_MESSAGE_START; /* search for next start sequence */
		  continue;
		}
	      
	      /* So it's not a naked start, escape, or a naked end: 
	       * Hey, it's just a normal byte, save it! */
	      MsgBuf[nchar++]=chr;
	      if ( nchar >= MaxMsgSize ) goto freak; 
	      continue;
	      
	      /* freakout: message won't fit in MsgBuf! */
            freak:  
	      {
		ms_log (2, "Input buffer overflow at %ld bytes; increase MaxMsgSize and restart!\n",
			MaxMsgSize);
		state=SEARCHING_FOR_MESSAGE_START; /* initialize again */
		nchar=0;
		continue;
	      }
	    } /* end of else not-an-endCharacter processing */
	} /* end of state==ASSEMBLING_MESSAGE processing */
    }  /* end of loop over characters */
  
 suicide:
  ms_log (1, "Error in socket communication; stopping MessageReceiver thread\n");
  MessageReceiverStatus = -1; /* file a complaint to the main thread */
#ifndef _WIN32
  pthread_exit( (void *)NULL );	  /* main thread will restart us */
#else
  KillSelfThread();               /* main thread will restart us */
#endif
  ms_log (1, "Fatal system error: Receiver thread could not pthread_exit()\n");
  exit(-1);
}  /* end of MessageReceiver thread */


/************************** SocketHeartbeat ***********************
 *           Send a heartbeat to the server via socket            *
 ******************************************************************/
static int SocketHeartbeat ( void )
{
  time_t now;
  
  /* Beat our heart (over the socket) to our server */
  time(&now);
  
  if ( MyAliveInt != 0  &&
       difftime(now,MyLastSocketBeat) > (double)MyAliveInt ) 
    {
      if ( WriteToSocket( Sd, MyAliveString, &heartlogo ) != 0 )
	{
	  ms_log (2, "problem sending alive msg to socket\n");
	  return -1;
	}
      
      if ( HeartbeatDebug )
	{
	  ms_log (1,"SocketHeartbeat sent to export\n");
	}
      
      MyLastSocketBeat = now;
    }
  
  return 0;
}


/*************************** SocketSendAck ************************
 *     Send an acknowledgment packet to the server via socket     *
 ******************************************************************/
static int SocketSendAck ( char *seq )
{
  char      ackstr[10];
  
  /* Send an acknowledgment (over the socket) to our server */
  sprintf( ackstr, "ACK:%s", seq );
  
  if ( WriteToSocket( Sd, ackstr, &acklogo ) != 0 )
    {
      ms_log (2, "error sending %s to socket\n");
      return -1;
    }
  
  if ( HeartbeatDebug )
    {
      ms_log (1, "Acknowledgment %s sent to export\n", ackstr);
    }
  
  return 0;
}


/*************************** WriteToSocket ************************
 *    send a message logo and message to the socket               *
 *    returns  0 if there are no errors                           *
 *            -1 if any errors are detected                       *
 ******************************************************************/

static int WriteToSocket ( int sock, char *msg, MSG_LOGO *logo )
{
  char asciilogo[11];       /* ascii version of outgoing logo */
  char startmsg = STX;      /* flag for beginning of message  */
  char endmsg   = ETX;      /* flag for end of message        */
  int  msglength;           /* total length of msg to be sent */
  int  rc;
  
  msglength = strlen(msg);  /* assumes msg is null terminated */
  
  /* Send "start of transmission" flag & ascii representation of logo */
  sprintf ( asciilogo, "%c%3d%3d%3d",startmsg,
	    (int) logo->instid, (int) logo->mod, (int) logo->type );
  
  rc = my_send ( sock, asciilogo, 10, socktimeout);
  if ( rc == -1 )
    {
      ms_log (2, "network send error %s\n", strerror(errno));
      return -1;
    }
  
  /* Debug print of message */
  /* printf("import_ack: sending under logo (%s):\n%s\n",asciilogo, msg); */
  
  /* Send message; break it into chunks if it's big! */
  rc = my_send ( sock, msg, msglength, socktimeout );
  if ( rc == -1 )
    {
      ms_log (2, "network send error %s\n", strerror(errno));
      return -1;
    }
  
  /* Send "end of transmission" flag */
  rc = my_send ( sock, &endmsg, 1, socktimeout );
  if ( rc == -1 ) 
    {
      ms_log (2, "network send error %s\n", strerror(errno));
      return -1;
    }
  
  return 0;
}


/*********************************************************************
 * handlestream:
 *
 * Add the packet data to the trace buffer, pack the buffer and send
 * it on.  If the original packet is a 512-byte miniSEED record it
 * will be sent instead unless re-packaging is specifically requested.
 *
 * We assume that the first 9 characters are three three-character 
 * fields giving IstallationId, ModuleId, and MessageType.
 *
 * Returns the number of records packed/transmitted on success and -1
 * on error.
 *********************************************************************/
static int handlestream ( char *msg, int msglen )
{
  static MSRecord *msr = NULL;
  static int32_t *int32buffer = NULL;
  static int32_t int32count = 0;
  int16_t *int16buffer = NULL;
  int idx;
  
  MSTrace *mst = NULL;
  int recordspacked = 0;
  char originaldatatype[3];
  
  char cInst[4], cMod[4], cType[4];
  MSG_LOGO logo;
  
  TRACE_HEADER *trh = (TRACE_HEADER *) (msg + 9);
  TRACE2_HEADER *trh2 = (TRACE2_HEADER *) (msg + 9);
  
  /* We assume that this message was created by our bretheren "export_generic",
     which attaches the logo as three groups of three characters at the front of
     the message */
  /* Peel off the logo chacters */
  strncpy(cInst,msg,    3);  cInst[3]=0;  logo.instid =(unsigned char)atoi(cInst);
  strncpy(cMod ,&msg[3],3);  cMod[3] =0;  logo.mod    =(unsigned char)atoi(cMod);
  strncpy(cType,&msg[6],3);  cType[3]=0;  logo.type   =(unsigned char)atoi(cType);
  
  /* Initialize MSRecord as a conversion buffer */
  if ( ! (msr = msr_init (msr)) )
    {
      ms_log (2, "Could not (re)initialize MSRecord\n");
      return -1;
    }
  
  /* Handle TRACEBUF message type */
  if ( logo.type == TYPE_TRACEBUF )
    {
      strncpy (originaldatatype, trh->datatype, sizeof(originaldatatype));
      
      /* If necessary, swap bytes in tracebuf message */
      if ( WaveMsgMakeLocal (trh) < 0 )
	{
	  ms_log (2, "WaveMsgMakeLocal() error: %s.%s.%s\n",
		  trh->net, trh->sta, trh->chan);
	  return -1;
	}
      
      if ( verbose > 1 )
	ms_log (1, "Received TRACEBUF: %s.%s.%s, %d samples, start: %.6f, type: %s (now %s)\n",
		trh->net, trh->sta, trh->chan, trh->nsamp, trh->starttime,
		originaldatatype, trh->datatype);
      
      /* Populate an MSRecord */
      ms_strncpclean (msr->network, trh->net, 2);
      ms_strncpclean (msr->station, trh->sta, 5);
      ms_strncpclean (msr->channel, trh->chan, 3);
      
      msr->starttime = (hptime_t)(MS_EPOCH2HPTIME(trh->starttime) + 0.5);
      msr->samprate = trh->samprate;
      
      msr->datasamples = (char *)trh + sizeof(TRACE_HEADER);
      msr->numsamples = trh->nsamp;
      msr->samplecnt = trh->nsamp;
      
      if ( ! strcmp(trh->datatype, "s2") || ! strcmp(trh->datatype, "i2") )
	{
	  /* (Re)Allocate static conversion buffer if more space is needed */
	  if ( int32count < msr->numsamples )
	    {
	      if ( (int32buffer = (int32_t *) realloc (int32buffer,
	            sizeof(int32_t)*((int32_t)(msr->numsamples)))) == NULL )
		{
		  ms_log (2, "Cannot (re)allocate memory for 16->32 bit conversion buffer\n");
		  return -1;
		}
	      int32count = (int32_t)(msr->numsamples);
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
      else if ( ! strcmp(trh->datatype, "s4") || ! strcmp(trh->datatype, "i4") )
	msr->sampletype = 'i';
      else if ( ! strcmp(trh->datatype, "t4") || ! strcmp(trh->datatype, "f4") )
	msr->sampletype = 'f';
      else if ( ! strcmp(trh->datatype, "t8") || ! strcmp(trh->datatype, "f8") )
	msr->sampletype = 'd';
      else {
	ms_log (2, "Unsupported data type: '%s'\n", trh->datatype);
	return -1;
      }
    }
  /* Handle TRACEBUF2 message type */
  else if ( logo.type == TYPE_TRACEBUF2 )
    {      
      strncpy (originaldatatype, trh->datatype, sizeof(originaldatatype));
      
      /* If necessary, swap bytes in tracebuf message */
      if ( WaveMsg2MakeLocal (trh2) < 0 )
	{
	  ms_log (2, "WaveMsg2MakeLocal error: %s.%s.%s.%s\n",
		  trh2->net, trh2->sta, trh2->loc, trh2->chan );
	  return -1;
	}
      
      if ( verbose > 1 )
	ms_log (1, "Received TRACEBUF2: %s.%s.%s.%s, %d samples, start: %.6f, type: %s (now %s)\n",
		trh2->net, trh2->sta, trh2->loc, trh2->chan, trh2->nsamp, trh2->starttime,
		originaldatatype, trh2->datatype);
      
      /* Populate an MSRecord */
      ms_strncpclean (msr->network, trh2->net, 2);
      ms_strncpclean (msr->station, trh2->sta, 5);
      if ( strcmp (trh2->loc, "--") )
	ms_strncpclean (msr->location, trh2->loc, 2);
      ms_strncpclean (msr->channel, trh2->chan, 3);
      
      msr->starttime = (hptime_t)(MS_EPOCH2HPTIME(trh2->starttime) + 0.5);
      msr->samprate = trh2->samprate;
      
      msr->datasamples = (char *)trh2 + sizeof(TRACE2_HEADER);
      msr->numsamples = trh2->nsamp;
      msr->samplecnt = trh2->nsamp;
      
      if ( ! strcmp(trh2->datatype, "s2") || ! strcmp(trh2->datatype, "i2") )
	{
	  /* (Re)Allocate static conversion buffer if more space is needed */
	  if ( int32count < msr->numsamples )
	    {
	      if ( (int32buffer = (int32_t *) realloc (int32buffer,
	            sizeof(int32_t)*((int32_t)(msr->numsamples)))) == NULL )
		{
		  ms_log (2, "Cannot (re)allocate memory for 16->32 bit conversion buffer\n");
		  return -1;
		}
	      int32count = (int32_t)(msr->numsamples);
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
      else if ( ! strcmp(trh2->datatype, "s4") || ! strcmp(trh2->datatype, "i4") )
	msr->sampletype = 'i';
      else if ( ! strcmp(trh2->datatype, "t4") || ! strcmp(trh2->datatype, "f4") )
	msr->sampletype = 'f';
      else if ( ! strcmp(trh2->datatype, "t8") || ! strcmp(trh2->datatype, "f8") )
	msr->sampletype = 'd';
      else {
	ms_log (2, "Unsupported data type: '%s'\n", trh2->datatype);
	return -1;
      }
    }
  else
    {
      ms_log (2, "Received unsupported message type: %d\n", logo.type);
      if ( logo.type == TYPE_ERROR )
	ms_log (2, "  Message is TYPE_ERROR\n");
      else if ( logo.type == TYPE_HEARTBEAT )
	ms_log (2, "  Message is TYPE_HEARTBEAT\n");
      else if ( logo.type == TYPE_ACK )
	ms_log (2, "  Message is TYPE_ACK\n");
      
      return 0;
    }
  
  /* Add data to trace buffer, creating new entry or extending as needed */
  if ( ! (mst = mst_addmsrtogroup (mstg, msr, 1, -1.0, -1.0)) )
    {
      ms_log (3, "Cannot add data to trace buffer!\n");
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
  	  ms_log (3, "Cannot allocate buffer for trace stats!\n");
  	  return -1;
  	}
      
      ((TraceStats *)mst->prvtptr)->earliest = HPTERROR;
      ((TraceStats *)mst->prvtptr)->latest = HPTERROR;
      ((TraceStats *)mst->prvtptr)->update = HPTERROR;
      ((TraceStats *)mst->prvtptr)->xmit = HPTERROR;
      ((TraceStats *)mst->prvtptr)->pktcount = 0;
      ((TraceStats *)mst->prvtptr)->reccount = 0;
    }
  
  ((TraceStats *)mst->prvtptr)->update = dlp_time();
  ((TraceStats *)mst->prvtptr)->pktcount += 1;
  
  if ( (recordspacked = packtraces (mst, 0, HPTERROR)) < 0 )
    {
      ms_log (3, "Cannot pack trace buffer or send records!\n");
      ms_log (3, "  %s.%s.%s.%s %lld\n",
	      mst->network, mst->station, mst->location, mst->channel,
	      (long long int) mst->numsamples);
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
static int packtraces ( MSTrace *mst, int flush, hptime_t flushtime )
{
  static struct blkt_1000_s Blkt1000;
  static struct blkt_1001_s Blkt1001;
  static MSRecord *mstemplate = NULL;

  MSTrace *prevmst;
  void *handlerdata = mst;
  int trpackedrecords = 0;
  int packedrecords = 0;
  int flushflag = flush;
  int encoding = -1;
  
  /* Set up MSRecord template, include blockette 1000 and 1001 */
  if ( (mstemplate = msr_init (mstemplate)) == NULL )
    {
      ms_log (2, "Cannot initialize packing template\n");
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
                                  verbose-2, mstemplate);
      
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
                    ms_log (1, "Flushing data buffer for %s_%s_%s_%s\n",
                            mst->network, mst->station, mst->location, mst->channel);
                    flushflag = 1;
                  }
              
	      strcpy (mstemplate->network, mst->network);
              strcpy (mstemplate->station, mst->station);
              strcpy (mstemplate->location, mst->location);
              strcpy (mstemplate->channel, mst->channel);
	      
              trpackedrecords = mst_pack (mst, sendrecord, handlerdata, 512,
                                          encoding, 1, NULL, flushflag,
                                          verbose-2, mstemplate);
              
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
 * sendrecord:
 *
 * Routine called to send a record to the DataLink server.
 *
 * Returns 0
 *********************************************************************/
static void sendrecord ( char *record, int reclen, void *handlerdata )
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
      ms_log (2, "Error unpacking %s: %s", streamid, ms_errorstr(rv));
      return;
    }
  
  /* Generate stream ID for this record: NET_STA_LOC_CHAN/MSEED */
  msr_srcname (msr, streamid, 0);
  strcat (streamid, "/MSEED");
  
  /* Determine high precision end time */
  endtime = msr_endtime (msr);
  
  if ( verbose >= 2 )
    ms_log (1, "Sending %s\n", streamid);
  
  /* Send record to server, loop */
  while ( dl_write (dlcp, record, reclen, streamid, msr->starttime, endtime, writeack) < 0 )
    {
      if ( dlcp->link == -1 )
        dl_disconnect (dlcp);
      
      if ( stopsig )
        {
          ms_log (2, "Termination signal with no connection to DataLink, the data buffers will be lost");
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
          ms_log (2, "Error re-connecting to DataLink server: %s, sleeping\n", dlcp->addr);
          dlp_usleep (reconnectinterval * (unsigned long)1e6);
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
}  /* End of sendrecord() */


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
  
  ms_log (0, "%s_%s_%s_%s, earliest: %s, latest: %s\n",
          mst->network, mst->station, mst->location, mst->channel,
          (stats->earliest == HPTERROR) ? "NONE":etime,
          (stats->latest == HPTERROR) ? "NONE":ltime);
  ms_log (0, "  last update: %s, xmit time: %s\n",
          (stats->update == HPTERROR) ? "NONE":utime,
          (stats->xmit == HPTERROR) ? "NONE":xtime);
  ms_log (0, "  pktcount: %lld, reccount: %lld\n",
          (long long int) stats->pktcount,
          (long long int) stats->reccount); 
}  /* End of logmststats() */

#ifndef _WIN32
/********************* Signal handling  routines ******************/
static void dummy_handler ( int sig ) { }

static void term_handler ( int sig )
{
  if ( TidMsgRcv )
    ThreadKill (TidMsgRcv);
  
  if ( Sd )
    close (Sd);
  
  stopsig = 1;
}

static void ThreadSignalHandler ( int sig )
{
  switch (sig)
    {
    case SIGUSR1:      
      pthread_exit( (void *)NULL );
    }
}
#endif


#ifndef _WIN32     /* if not Windows then do it the Unix way */
/***************************************************************************
 * ThreadStart():
 *
 * Return -1 on error and 0 on success.
 ***************************************************************************/
static int ThreadStart ( pthread_t *tid, void *func(void *), void *arg )
{
  int rc;
  pthread_attr_t msgrecv_attr;

  /* Initialize thread attributes */
  pthread_attr_init ( &msgrecv_attr );
  pthread_attr_setdetachstate ( &msgrecv_attr, PTHREAD_CREATE_DETACHED );
  
  /* Start the thread */
  rc = pthread_create (tid, &msgrecv_attr, func, arg);
  
  if ( rc != 0 )
    return -1;
  
  return 0;
} /* End of ThreadStart() */
#endif

/***************************************************************************
 * ThreadKill():
 *
 * Return 0 on success, nonzero if error.
 ***************************************************************************/
static int ThreadKill ( pthread_t tid )
{
#ifndef _WIN32          /* if not Windows then do it the Unix way */
  return pthread_kill(tid, SIGUSR1);
#else                   /* if Windows then do it the Earthworm way */
  return KillThread(tid );
#endif
} /* End of ThreadKill() */


/***************************************************************************
 * print_timelog:
 *
 * Log message print handler.  Prefixes a local time string to the
 * message before printing.
 ***************************************************************************/
static void print_timelog ( char *msg )
{
  char timestr[100];
  time_t loc_time;
  
  /* Build local time string and cut off the newline */
  time(&loc_time);
  strcpy(timestr, asctime(localtime(&loc_time)));
  timestr[strlen(timestr) - 1] = '\0';
  
  fprintf (stderr, "%s - %s", timestr, msg);
}  /* End of print_timelog() */


/*****************************************************************************
 *  config_params() processes command line arguments and set defaults        *
 *****************************************************************************/
static void config_params ( int argcount, char **argvec )
{
  int error = 0;
  int optind;
  char *heartlogostr = NULL;
  
  /* Setup defaults */
  MaxMsgSize = 4096;
  SocketTimeoutLength = 80;
  
  MyAliveInt = 120;
  SenderHeartRate = 60;
  
  heartlogo.instid = 0;
  heartlogo.mod    = 0;
  heartlogo.type   = TYPE_HEARTBEAT;
  
  acklogo.instid = 0;
  acklogo.mod    = 0;
  acklogo.type   = TYPE_ACK;
  
  /* Process all command line arguments */
  for ( optind=1 ; optind < argcount ; optind++)
    {
      if ( strncmp(argvec[optind], "-v", 2) == 0 )
	{
	  verbose += strspn (&argvec[optind][1], "v");
	}
      else if ( strcmp(argvec[optind], "-h") == 0 )
	{
	  usage();
	}
      else if ( strcmp (argvec[optind], "-f") == 0 )
        {
	  if ( ++optind < argcount )
	    flushlatency = atoi(argvec[optind]);
	  else error++;
        }
      else if ( strcmp (argvec[optind], "-R") == 0 )
        {
	  if ( ++optind < argcount )
	    reconnectinterval = atoi(argvec[optind]);
	  else error++;
        }
      else if ( strcmp(argvec[optind], "-Ar") == 0 )
	{
	  if ( ++optind < argcount )
	    MyAliveInt = atoi(argvec[optind]);
	  else error++;
	}
      else if ( strcmp(argvec[optind], "-At") == 0 )
	{
	  if ( ++optind < argcount ) {
	    strncpy (MyAliveString, argvec[optind], MAX_ALIVE_STR-1);
	    MyAliveString[MAX_ALIVE_STR-1] = '\0';
	  }
	  else error++;
	}
      else if ( strcmp(argvec[optind], "-Sr") == 0 )
	{
	  if ( ++optind < argcount )
	    SenderHeartRate = atoi(argvec[optind]);
	  else error++;
	}
      else if ( strcmp(argvec[optind], "-St") == 0 )
	{
	  if ( ++optind < argcount ) {
	    strncpy (SenderHeartText, argvec[optind], MAX_ALIVE_STR-1);
	    SenderHeartText[MAX_ALIVE_STR-1] = '\0';
	  }
	  else error++;
	}
      else if ( strcmp(argvec[optind], "-m") == 0 )
	{
	  if ( ++optind < argcount )
	    MaxMsgSize = atoi(argvec[optind]);
	  else error++;
	}
      else if ( strcmp(argvec[optind], "-t") == 0 )
	{
	  if ( ++optind < argcount )
	    SocketTimeoutLength = atoi(argvec[optind]);
	  else error++;
	}
      else if ( strcmp(argvec[optind], "-Ie") == 0 )
	{
	  if ( ++optind < argcount )
	    {
	      if ( ! strcmp (argvec[optind], "STEIM1") )
		int32encoding = DE_STEIM1;
	      else if ( ! strcmp (argvec[optind], "STEIM2") )
		int32encoding = DE_STEIM2;
	      else if ( ! strcmp (argvec[optind], "INT32") )
		int32encoding = DE_INT32;
	      else
		{
		  fprintf (stderr, "Unrecognized option for -Ie: %s\n\n",
			   argvec[optind]);
		  error++;
		}
	    }
	  else error++;
	}
      else if ( strcmp(argvec[optind], "-Hl") == 0 )
	{
	  if ( ++optind < argcount )
	    heartlogostr = argvec[optind];
	  else error++;
	}
      else if ( ! exaddr )
	{
	  exaddr = argvec[optind];
	}
      else if ( ! rsaddr )
	{
	  rsaddr = argvec[optind];
	}
      else
	{
	  fprintf (stderr, "Unknown option: %s\n", argvec[optind]);
	  exit (1);
	}
    }

  /* Initialize the verbosity for the dl_log function */
  dl_loginit (verbose-1, &print_timelog, "", &print_timelog, "");
  
  /* Initialize the logging for the ms_log family */
  ms_loginit (&print_timelog, NULL, &print_timelog, NULL);    
  
  /* Make sure a source export server was specified */
  if ( ! exaddr )
    {
      ms_log (2, "No export server specified\n");
      error++;
    }
  
  /* Make sure a destination DataLink server was specified */
  if ( ! rsaddr )
    {
      ms_log (2, "No DataLink server specified\n");
      error++;
    }
  
  /* Report the program version */
  ms_log (1, "%s version: %s\n", PACKAGE, VERSION);
  
  /* Parse the heartbeat logo if specified */
  if ( heartlogostr )
    {
      char *cptr, *dptr, *nptr;
      
      cptr = strchr (heartlogostr, '/');
      dptr = strrchr (heartlogostr, '/');
      
      if ( cptr == NULL || dptr == NULL || cptr == dptr )
	{
	  ms_log (1, "Heartbeat logo specified incorrectly: %s\n", heartlogostr);
	  error++;
	}
      else
	{
	  nptr = cptr++; nptr = '\0';
	  nptr = dptr++; nptr = '\0';
	  
	  heartlogo.instid = (unsigned char) atoi(heartlogostr);
	  heartlogo.mod    = (unsigned char) atoi(cptr);
	  heartlogo.type   = (unsigned char) atoi(dptr);
	  
	  acklogo.instid = (unsigned char) atoi(heartlogostr);
	  acklogo.mod    = (unsigned char) atoi(cptr);
	  acklogo.type   = TYPE_ACK;
	}
    }
  
  /* Parse host and port from export server address */
  if ( exaddr )
    {
      char *cptr;

      cptr = strchr (exaddr, ':');

      if ( cptr )
	{
	  SenderHost = exaddr;
	  *cptr = '\0';
	  SenderPort = ++cptr;
	}
      else
	{
	  SenderHost = exaddr;
	  SenderPort = "16000";  /* Default DataLink port */
	}
    }

  /* If errors then report the usage message and quit */
  if ( error )
    usage();
  
  return;
} /* End of config_params() */


/***************************************************************************
 * usage():
 *
 * Print usage message and exit.
 ***************************************************************************/
static void usage ( void )
{
  fprintf(stderr,"\n"
	  "Usage: %s [options] <exportserver:port> <ringserver:port>\n"
	  "\n"
	  "  -u            Print this usage message\n"
	  "  -v            Be more verbose, mutiple flags can be used\n"
	  "\n"
	  "  -f latency    Internal buffer flush latency in seconds, default 300\n"
	  "  -R interval   Reconnection interval/wait in seconds, default 10\n"
	  "  -t timeout    Socket timeout in seconds, default 80\n"
	  "  -Ie encoding  Specify encoding for 32-bit integers, default STEIM2\n"
	  "  -Ar rate      Rate (approximate) to send hearbeats to server, default 120\n"
	  "  -At text      Text for heartbeat to server, default='alive' or 'ImpAlive'\n"
	  "  -Sr rate      Rate at which to expect heartbeats from server, default 60\n"
	  "  -St text      Text expected in heartbeats from server, default='ExpAlive'\n"
	  "  -m maxmsg     Maximum message size that can be received, default 4096\n"
	  "  -Hl #/#/#     Specify the logo to use for heartbeats, default '0/0/3'\n"
	  "                   #/#/# is the <inst id>/<mod id>/<heartbeat type>\n"
	  "\n"
	  "The export server and ringserver addresses are specified in host:port format\n"
	  "\n", PACKAGE);
  
  exit (1);
} /* End of usage() */
