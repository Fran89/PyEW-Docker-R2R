
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: import_generic.c 6844 2016-10-18 19:09:32Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.13  2007/03/28 15:42:27  paulf
 *     minor MACOSX directives
 *
 *     Revision 1.12  2007/02/27 00:16:04  stefan
 *     cast to unsigned int line 225
 *
 *     Revision 1.11  2007/02/26 14:23:17  paulf
 *     fixed long casting of time_t for heartbeat sprintf()
 *
 *     Revision 1.10  2005/07/25 20:33:17  friberg
 *     put in 1 _LINUX #ifdef directive
 *
 *     Revision 1.9  2005/03/18 16:45:18  dietz
 *     minor logging change
 *
 *     Revision 1.8  2005/03/17 21:11:38  dietz
 *     Restructured connection loop and clarified log statements.
 *
 *     Revision 1.7  2005/03/16 00:04:50  dietz
 *     Removed Earthworm heartbeat from Heartbeat thread and put it in main().
 *     Renamed Heartbeat thread to SocketHeartbeat (also renamed associated
 *     variables). Now import will continue to beat its earthworm heart while
 *     trying to reconnect to an export.
 *
 *     Revision 1.6  2002/03/18 18:47:50  patton
 *     Made Logit Changes
 *     JMP
 *
 *     Revision 1.5  2001/05/08 22:38:25  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPid..
 *
 *     Revision 1.4  2000/08/08 17:32:49  lucky
 *     Lint Cleanup
 *
 *     Revision 1.3  2000/07/24 19:06:23  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *     Also, set size of the Alive buffer to MAX_ALIVE_STR -- same as in export.
 *
 *     Revision 1.2  2000/04/03 19:10:09  lombard
 *     Cleaned up setting of SocketTimeout: now defaults to same as SenderHeartRate.
 *
 *     Revision 1.1  2000/02/14 18:42:44  lucky
 *     Initial revision
 *
 *
 */

/*
 *   import_generic.c:  Program to receive messages from far away
 *                      via socket and put them into a transport ring.

 *   The intent is that there will be a specific import_xxx module for
 *   importing from institution xxx. That is, there will be as many
 *   import modules as places to import from, each module having it's
 *   own name, and living in its own directory.  Each such module could
 *   have it's own "import_filter" routine, coded for the specific job.
 *   
 *   Upgraded to move binary messages. Alex 10/4/96.
 *   
 *   Change to use sendall() instead of send() to fix partial send() return
 *   under NT (and maybe even Solaris) Alex 11/13/97
 *   
 *   Installed Bug fix suggested by Doug Neuhauser:
 *   "...I believe the following code in state 2:
 *                   if (chr==endCharacter && lastChr != escape)   
 *   should be:
 *                   if (chr==endCharacter)   ..."
 *   Alex 11/13/97
 *   See comments in export_generic.c
 *   
 *     
 *   Modified 11/20/97 to perform Multi-Byte recv() calls 
 *   and buffer information internally.  Also replaced state
 *   numbers with #defined constants  Davidk
 *
 *   Initialize variable nchar to 0 in MessageReceiver thread.
 *   LDD 2/9/98
 *
 *  Lombard: 11/19/98: V4.0 changes: 
 *    0) no Y2k dates 
 *    1) changed argument of logit_init to the config file name.
 *    2) process ID in heartbeat message: already done
 *    3) flush input transport ring: not applicable
 *    4) add `restartMe' to .desc file: already done
 *    5) multi-threaded logit
 *
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
#include <socket_ew.h>
#include <imp_exp_gen.h>
#include <trace_buf.h>

#define MAX_LOGO  20

/* Functions in this source file
   *****************************/
void    config( char * );
void    lookup( void );
void    output_status( unsigned char, short, char * );
void    import_filter( char *, int);
int     WriteToSocket( SOCKET, char *, MSG_LOGO * );
thr_ret SocketHeartbeat( void * );
thr_ret MessageReceiver( void * );

extern int  errno;
static SHM_INFO  Region;   /* shared memory region to use for i/o    */
volatile pid_t   MyPid;    /* process id, sent with heartbeat        */

/* Things to read or derive from configuration file
 **************************************************/
static char    RingName[MAX_RING_STR]; /* name of transport ring for i/o */
static char    MyModName[MAX_MOD_STR]; /* speak as this module name/id   */
static int     LogSwitch;           /* 0 if no logfile should be written */
static int     HeartBeatInt;        /* seconds between heartbeats        */
                                    /* (to local ring)                   */
static long    MaxMsgSize;          /* max size for input/output msgs    */
static int     MyAliveInt;          /* Seconds between sending alive     */
                                    /* message to foreign sender         */
static char    MyAliveString[MAX_ALIVE_STR];  /* Text of above alive message */

static char    SenderIpAdr[20];     /* Foreign sender's address, in dot notation */
static int     SenderPort;          /* Server's well-known port number */
static int     SenderHeartRate;     /* Expect alive messages this often from foreign sender */
static char    SenderHeartText[MAX_ALIVE_STR];/* Text making up the sender's heartbeat msg */
MSG_LOGO       PutAsLogo;           /* logo to be used for placing received msg into ring. */
                                    /* May be superceeded in the message processing routine
                                      "import_filter" */
static unsigned int SocketTimeoutLength; /* Length of timeouts on SOCKET_ew calls */
static int HeartbeatDebug=0;        /* set to 1 in for heartbeat debug messages */
static int SOCKET_ewDebug=0;        /* set to 1 in for heartbeat debug messages */

/* Globals: timers, etc, used by both threads */
/**********************************************/
#define CONNECT_WAIT_DT 10           /* Seconds wait between connect attempts            */
#define THEAD_STACK_SIZE 8192        /* Implies different things on different systems !! */
      /* on os2, the stack grows dynamically beyond this number if needed, but likely    */
      /* at the expesene of performance. Solaris: not sure, but overflow might be fatal  */
volatile time_t MyLastInternalBeat;  /* time of last heartbeat into local Earthworm ring */
volatile time_t LastServerBeat;      /* times of heartbeats from the server machine      */
volatile time_t MyLastSocketBeat;    /* time of last heartbeat to server - via socket    */
volatile int SocketHeartThreadStatus = 0; /* Status of socket heartbeat thread           */
                                          /* -1 if server croaks.                        */ 
volatile int MessageReceiverStatus =0;    /* status of message receiving thread:         */
                                          /* -1 means bad news                           */
volatile SOCKET Sd;             /* Socket descriptor                                */
char     *MsgBuf;               /* incoming message buffer; used by receiver thread */
unsigned  TidSocketHeart;       /* thread id. was type thread_t on Solaris! */
unsigned  TidMsgRcv;            /* thread id. was type thread_t on Solaris! */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          RingKey;       /* key of transport ring for i/o     */
static unsigned char MyInstId;      /* local installation id             */
static unsigned char MyModId;       /* Module Id for this program        */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;

/* Error messages used by import
 *********************************/
#define  ERR_SOCK_READ       0   /* error reading from socket                      */
#define  ERR_TOOBIG          1   /* retreived msg too large for buffer             */
#define  ERR_TPORTPUT        2   /* error putting message into ring                */
#define  ERR_SERVER_DEAD     3   /* server machine heartbeats not received on time */
#define  ERR_GARBAGE_IN      4   /* something other than STX after ETX             */

int main( int argc, char **argv )
{
   struct sockaddr_in insocket;
   time_t now;
   time_t tfirsttry;            /* time of first connection attempt */
   int    quit;
   int    retryCount;           /* to prevent flooding the log file */
   int    connected;            /* connection flag */
   char   errtxt[128];          /* array for error text */

/* Check command line arguments
 ******************************/
   if ( argc != 2 )
   {
      fprintf( stderr, "Usage: import_generic <configfile>\n" );
      return -1;
   }

/* Initialize name of log-file & open it
 ***************************************/
   logit_init( argv[1], 0, 256, 1 );

/* Read the configuration file(s)
 ********************************/
   config( argv[1] );
   logit( "" , "import_generic(%s): Read command file <%s>\n", 
                MyModName, argv[1] );

/* Reinitialize the logging level
 ********************************/
   logit_init( argv[1], 0, 256, LogSwitch );
 
/* Set Socket Debug 
 *******************/
   setSocket_ewDebug(SOCKET_ewDebug);
   
/* Look up important info from earthworm.h tables
 ************************************************/
   lookup();

/* Heartbeat parameters sanity checks
 ************************************/
   logit("","17 Mar 2005 version.\n");
   if(1000 * (unsigned int)SenderHeartRate >= SocketTimeoutLength)
   {
     logit("","Socket timeout (%d ms) is less than incoming heartrate (%d sec)",
		SocketTimeoutLength, SenderHeartRate);
     SocketTimeoutLength = 1000 * SenderHeartRate;
     logit("", "Setting socket timeout to %d ms\n", SocketTimeoutLength);
   }

/* Get our Process ID for restart purposes
 ******************************************/
   MyPid = getpid();
   if( MyPid == -1 )
   {
      logit("e", "import_generic(%s): Cannot get PID. Exiting.\n", MyModName);
      return -1;
   }

/* Attach to Input/Output shared memory ring
 *******************************************/
   tport_attach( &Region, RingKey );
   logit( "", "import_generic(%s): Attached to public memory region %s: %d\n",
          MyModName, RingName, RingKey );

/* Allocate the message buffer
 ******************************/
   if ( ( MsgBuf = (char *) malloc(MaxMsgSize) ) == (char *) NULL )
   {
     logit("e", "import_generic(%s): Can't allocate buffer of size %ld bytes\n",
           MyModName, MaxMsgSize);
     return -1;
   }

/* Put out one heartbeat before trying to connect to socket
 ***********************************************************/
   output_status( TypeHeartBeat, 0, "" );
   MyLastInternalBeat = time(&now);  

   /* Initialize the socket system
   ******************************/
   SocketSysInit();

/* Stuff address and port into socket structure
   ********************************************/
   memset( (char *)&insocket, '\0', sizeof(insocket) );
   insocket.sin_family = AF_INET;
   insocket.sin_port   = htons( (short)SenderPort );

#if defined(_LINUX) || defined(_MACOSX)
   if ((int)(insocket.sin_addr.s_addr = inet_addr(SenderIpAdr)) == -1)
#else
   if ((int)(insocket.sin_addr.S_un.S_addr = inet_addr(SenderIpAdr)) == -1)
#endif
   {
      logit( "e", "import_generic(%s): inet_addr failed for SenderIpAdr <%s>; exiting!\n",
              MyModName, SenderIpAdr );
      return -1;
   }
 
   /* to prevent flooding the log file during long reconnect attempts */
   /********************************************************************/
   retryCount=0;  /* it may be reset elsewere */

/*----------------------------------------------------------
   Connection loop: try to get a socket connection to server
 ------------------------------------------------------------*/

reconnect:            /* we can JUMP here from other places! */
                 
   connected = 0;     /* not connected now */
   time(&tfirsttry);  /* store time of first connection attempt */

/* Try for a network connection - and keep trying forever !!
 ************************************************************/
   while( !connected )
   {
      retryCount++;

   /* Beat our heart (into the local Earthworm) if it's time 
    ********************************************************/
      time(&now);
      if (difftime(now,MyLastInternalBeat) > (double)HeartBeatInt)
      {
         output_status( TypeHeartBeat, 0, "" );
         MyLastInternalBeat = now;
      }

   /* Are we being told to quit? 
    ****************************/
      if ( tport_getflag( &Region ) == TERMINATE  ||
           tport_getflag( &Region ) == MyPid        ) 
      {
         tport_detach( &Region );
         logit("t", "import_generic(%s): terminating on request\n", MyModName);
         (void)KillThread(TidMsgRcv);
         (void)KillThread(TidSocketHeart);
         free(MsgBuf);
         return 0;
      }

   /* Create a socket
    ******************/
      if ( ( Sd = socket_ew( AF_INET, SOCK_STREAM, 0)) == -1 )
      {
         sprintf (errtxt, "import_generic(%s): socket_ew", MyModName);
         SocketPerror(errtxt);
         logit( "et", "import_generic(%s): Error opening socket, exiting!\n", MyModName );
         closesocket_ew((int)Sd,SOCKET_CLOSE_IMMEDIATELY_EW);
         free(MsgBuf);
         return -1;
      }
      if (retryCount <= 4) {
         logit("t", "import_generic(%s): Trying to connect to %s on port %d\n",
	        MyModName, SenderIpAdr, SenderPort);
      }

   /* Attempt the connection, if it fails close socket and sleep 
    ************************************************************/
      if ( connect_ew( (int)Sd, (struct sockaddr *)&insocket, sizeof(insocket),
                     SocketTimeoutLength ) == -1 )
      {
         closesocket_ew((int)Sd, SOCKET_CLOSE_IMMEDIATELY_EW);

         if (retryCount < 4) {
            logit("t", "import_generic(%s): Failed to connect. Waiting...\n", 
                   MyModName );
         }
         if (retryCount == 4) {
            logit("t", "import_generic(%s): Failed to connect. Will try to "
                   "connect every %d seconds, but will not log repetitions.\n", 
                   MyModName, CONNECT_WAIT_DT );
         }
         sleep_ew(CONNECT_WAIT_DT*1000);
      }

   /* else the connection succeeded...let's get out of this loop! 
    *************************************************************/
      else
      {
         logit("t", "import_generic(%s): Connected after %d seconds (on try %d)\n",
                MyModName, (int)(time(&now)-tfirsttry), retryCount );
         connected  = 1;
         retryCount = 0;
      }
   } /* end of connection loop */

/* Start the socketheartbeat thread
 ***********************************/
   SocketHeartThreadStatus=0;          /* set it's status flag to ok */
   time((time_t*)&MyLastSocketBeat);   /* initialize time of our heartbeat over socket */
   time((time_t*)&LastServerBeat);     /* initialize time of last heartbeat from 
                                           serving machine */
   if ( StartThread( SocketHeartbeat, (unsigned)THEAD_STACK_SIZE, &TidSocketHeart ) == -1 )
   {
      logit( "e", "import_generic(%s): Error starting SocketHeartbeat thread. Exiting.\n",
             MyModName );
      tport_detach( &Region );
      free(MsgBuf);
      return -1;
   }

/* Start the message receiver thread
 ***********************************/
   MessageReceiverStatus =0; /* set it's status flag to ok */
   if ( StartThread( MessageReceiver, (unsigned)THEAD_STACK_SIZE, &TidMsgRcv ) == -1 )
   {
      logit( "e", "import_generic(%s): Error starting MessageReceiver thread. Exiting.\n",
             MyModName );
      tport_detach( &Region );
      free(MsgBuf);
      return -1;
   }

/*-------------------------------------------------------------------------------
   Working loop: check on server heartbeat status, check on receive thread health.
   check for shutdown requests. If things go wrong, kill all  threads and restart
 ---------------------------------------------------------------------------------*/
   quit=0; /* to restart or not to restart */
   while (1)
   {
      sleep_ew(1000); /* sleep one second. Remember, the receieve thread is awake */
      time(&now);

      /* Beat our heart (into the local Earthworm) if it's time 
       ********************************************************/
      if (difftime(now,MyLastInternalBeat) > (double)HeartBeatInt)
      {
         output_status( TypeHeartBeat, 0, "" );
         MyLastInternalBeat = now;
      }

      /* How's the server's heart? */ 
      /*****************************/
      if (difftime(now,LastServerBeat) > (double)SenderHeartRate && SenderHeartRate !=0)
      {
         output_status(TypeError,ERR_SERVER_DEAD," Server heartbeat lost. Restarting");
	 logit("et","No heartbeat received for %d seconds. "
                    "Restarting connection\n", SenderHeartRate);
         quit=1; /*restart*/
      }

      /* How's the receive thread feeling ? */
      /**************************************/
      if ( MessageReceiverStatus == -1)
      {
         logit("et", "import_generic(%s): MessageReceiver thread has quit. Restarting\n",
                MyModName);
         quit=1;
      }

      /* How's the socketheartbeat thread feeling ? */
      /**********************************************/
      if ( SocketHeartThreadStatus == -1)
      {
         logit("et", "import_generic(%s): SocketHeartbeat thread unhappy. Restarting\n",
                MyModName);
         quit=1;
      }

      /* Are we being told to quit */
      /*****************************/
      if ( tport_getflag( &Region ) == TERMINATE ||
           tport_getflag( &Region ) == MyPid        )
      {

         tport_detach( &Region );
         logit("t", "import_generic(%s): terminating on request\n", MyModName);
         (void)KillThread(TidMsgRcv);
         (void)KillThread(TidSocketHeart);
         free(MsgBuf);
         return 0;
      }

      /* Any other shutdown conditions here */
      /**************************************/
      /* blah blah blah */

      /* restart preparations */
      /************************/
      if (quit == 1)
      {
         (void)KillThread(TidMsgRcv);
         (void)KillThread(TidSocketHeart);
         closesocket_ew( (int)Sd, SOCKET_CLOSE_IMMEDIATELY_EW);
         quit=0;
         goto reconnect;
      }

   }  /* end of working loop */
}

/************************Messge Receiver Thread *********************
*          Listen for client heartbeats, and set global variable     *
*          showing time of last heartbeat. Main thread will take     *
*          it from there                                             *
**********************************************************************/
/*
Modified to read binary messages, alex 10/10/96: The scheme (I got it from Carl) is define
some sacred characters. Sacred characters are the start-of-message and end-of-message
framing characters, and an escape character. The sender's job is to cloak unfortunate bit
patters in the data which look like sacred characters by inserting before them an 'escape'
character.  Our problem here is to recognize, and use, the 'real' start- and end-of-
messge characters, and to 'decloak' any unfortunate look-alikes within the message body.
*/

thr_ret MessageReceiver( void *dummy )
{
  static int  state;
  static char chr, lastChr;
  static int  nr;
  static long nchar;                     /* counter for msg buffer (esc's removed) */
  static char startCharacter=STX;        /* ASCII STX characer                     */
  static char endCharacter=ETX;          /* ASCII ETX character */
  static char escape=ESC;                /* our escape character */
  static char   inBuffer[INBUFFERSIZE];
  static long   inchar;                  /* counter for raw socket buffer (w/esc) */
  char          errtxt[128];             /* array for error text */

/* Tell the main thread we're ok
 *******************************/
  MessageReceiverStatus=0;

  state=SEARCHING_FOR_MESSAGE_START; /* we're initializing */

  /* Start of New code Section DK 11/20/97 Multi-byte Read 
     Set inchar to be nr-1, so that when inchar is incremented, they will be 
         the same and a new read from socket will take place.  
     Set chr to 0 to indicate a null character, since we haven't read any yet.
     Set nchar to 0 to begin building a new MsgBuffer (with escapes removed).
  */
  inchar = -1;
  nr     =  0;
  chr    =  0;
  nchar  =  0;  /* added 980209:LDD */
  /* End of New code Section */

/* Working loop: receive and process messages
 ********************************************/
/* We are either (0) initializing: searching for message start
                 (1) expecting a message start: error if not
                 (2) assembling a message.
   The variable "state' determines our mood */

  while(1) /* loop over bytes read from socket */
  {
    /* Get next char operation */
    if (++inchar == nr)
    {
      /* Read from socket operation */

      nr=recv_ew((int)Sd,inBuffer,INBUFFERSIZE-1,0, SocketTimeoutLength);
      if (nr<=0)  /* Connection Closed */
        goto suicide;
      /*else*/
      inchar=0;
      /* End of Read from socket operation */
    }
    lastChr=chr;
    chr=inBuffer[inchar];
    /* End of Get next char operation */

    /* Initialization */
    /******************/
    if (state==SEARCHING_FOR_MESSAGE_START)   /* throw all away until we see a naked start character */
    {
    /* Do we have a real start character?
      *************************************/
      if ( lastChr!=escape && chr==startCharacter)
      {
        state=ASSEMBLING_MESSAGE;  /*from now on, assemble message */
        continue;
      }
    }

    /* Confirm message start */
    /*************************/
    if (state==EXPECTING_MESSAGE_START)  /* the next char had better be a start character - naked, that is */
    {
    /* Is it a naked start character?
      *********************************/
      if ( chr==startCharacter &&  lastChr != escape) /* This is it: message start!! */
      {
        nchar=0; /* start with firsts char position */
        state=ASSEMBLING_MESSAGE; /* go into assembly mode */
        continue;
      }
      else   /* we're eating garbage */
      {
        logit("et", "import_generic(%s): Unexpected character from client. Re-synching\n", 
					MyModName);
        state=SEARCHING_FOR_MESSAGE_START; /* search for next start sequence */
        continue;
      }
    }

    /* In the throes of assembling a message */
    /****************************************/
    if (state==ASSEMBLING_MESSAGE)
    {
    /* Is this the end?
      *******************/
      if (chr==endCharacter)   /* naked end character: end of the message is at hand */
      {
        /* We have a complete message */
        /*****************************/
        MsgBuf[nchar]=0; /* terminate as a string */

        if(strcmp(&MsgBuf[9],SenderHeartText)==0) /* Server's heartbeat */
        {
          if(HeartbeatDebug)
          {
            logit("et", "import_generic(%s): Received heartbeat\n", MyModName); 
          }
          time((time_t*)&LastServerBeat); /* note time of heartbeat */
          state=EXPECTING_MESSAGE_START; /* reset for next message */
          MsgBuf[0]=' '; MsgBuf[9]=' ';
        }
        else
        {
          /* got a non-heartbeat message */
          if(HeartbeatDebug)
          {
            logit("et", "import_generic(%s): Received non-heartbeat\n", MyModName); 
          }
          time((time_t*)&LastServerBeat); /* note time of heartbeat */
          /* This is not a genuine heartbeat, but our major concern is that the
             export module on the other end is still alive, and any message that
             we receive should indicate life on the other side(in a non occult 
             kind of way).
          */

          import_filter(MsgBuf,nchar); /* process the message via user-routine */
        }
        state=EXPECTING_MESSAGE_START;
        continue;
      }
      else
      {
        /* process the message byte we just read: we know it's not a naked end character*/
        /********************************************************************************/
        /* Escape sequence? */

        if (chr==escape)
        {  /*  process the escape sequence */

          /* Read from buffer
          *******************/
          /* Get next char operation */
          if (++inchar == nr)
          {
            /* Read from socket operation */
            nr=recv_ew((int)Sd,inBuffer,INBUFFERSIZE-1,0, SocketTimeoutLength);
            if (nr<=0)  /* Connection Closed */
              goto suicide;
            /*else*/
            inchar=0;
            /* End of Read from socket operation */
          }
          lastChr=chr;
          chr=inBuffer[inchar];
          /* End of Get next char operation */

          if( chr != startCharacter && chr != endCharacter && chr != escape)
          {   /* bad news: unknown escape sequence */
            logit("et", "import_generic(%s): Unknown escape sequence in message. Re-synching\n",MyModName);
            state=SEARCHING_FOR_MESSAGE_START; /* search for next start sequence */
            continue;
          }
          else   /* it's ok: it's a legit escape sequence, and we save the escaped byte */
          {
            MsgBuf[nchar++]=chr; if(nchar>MaxMsgSize) goto freak; /*save character */
            continue;
          }
        }

        /*  Naked start character? */
        if (chr==startCharacter)
        {   /* bad news: unescaped start character */
          logit("et", "import_generic(%s): Unescaped start character in message. Re-synching\n", MyModName);
          state=SEARCHING_FOR_MESSAGE_START; /* search for next start sequence */
          continue;
        }

        /* So it's not a naked start, escape, or a naked end: Hey, it's just a normal byte */
        MsgBuf[nchar++]=chr; if(nchar>MaxMsgSize) goto freak; /*save character */
        continue;

                        freak:  /* freakout: message won't fit */
        {
          logit("et", "import_generic(%s): receive buffer overflow after %ld bytes\n",
					MyModName, MaxMsgSize);
          state=SEARCHING_FOR_MESSAGE_START; /* initialize again */
          nchar=0;
          continue;
        }
      } /* end of not-an-endCharacter processing */
    } /* end of state==ASSEMBLING_MESSAGE processing */
  }  /* end of loop over characters */

suicide:
  logit("et", "import_generic(%s): Error from network read. Restarting receive thread\n"
		, MyModName);
  sprintf (errtxt, "import_generic(%s): recv_ew", MyModName);
  SocketPerror(errtxt);
  logit("t", "import_generic(%s): Bad socket read: %d\n", MyModName, nr);
  MessageReceiverStatus = -1; /* file a complaint to the main thread */
  KillSelfThread(); /* main thread will restart us */
  logit("t", "import_generic(%s): Fatal system error: "
             "Receiver thread could not KillSelf\n",
	      MyModName );
  exit(-1);

}  /* end of MessageReceiver thread */


/************************** SocketHeartbeat ***********************
 *           Send a heartbeat to the server via socket            *
 *                 Check on our server's hearbeat                 *
 *           Slam socket shut if no Server heartbeat: that        *
 *            really shakes up the main thread                    *
 ******************************************************************/

thr_ret SocketHeartbeat( void *dummy )
{
   MSG_LOGO  reclogo;
   time_t    now;
   char      errtxt[256];    /* array for error text */

   /* once a second, do the rounds. If anything looks bad, set 
      SocketHeartThreadStatus to -1 and go into a long sleep. 
      The main thread should note that our status is -1, and launch into 
      re-start procedure, which includes killing and restarting us. */
   while ( 1 )
   {
      sleep_ew(1000);
      time(&now);

      /* Beat our heart (over the socket) to our server
      **************************************************/
      if (difftime(now,MyLastSocketBeat) > (double)MyAliveInt && MyAliveInt != 0)
      {
         reclogo.instid = MyInstId;
         reclogo.mod    = MyModId;
         reclogo.type   = TypeHeartBeat;
         if ( WriteToSocket( Sd, MyAliveString, &reclogo ) != 0 )
         {
            /* If we get an error, simply quit */
            sprintf( errtxt, "import_generic(%s): error sending alive msg to socket."
                             "  Heart thread quitting\n", MyModName );
            output_status( TypeError, ERR_SOCK_READ, errtxt );
            logit("t", errtxt);
            SocketHeartThreadStatus=-1;
            KillSelfThread();  /* the main thread will resurect us */
            logit("et", "import_generic(%s): Fatal system error: Heart thread could not KillSelf\n", MyModName);
            exit(-1);
         }
         if(HeartbeatDebug)
         {
            logit("et","import_generic(%s): SocketHeartbeat sent to export\n", MyModName);
         }
         MyLastSocketBeat=now;
      }
   }
}


/*****************************************************************************
 *  config() processes command file(s) using kom.c functions;                *
 *                    exits if any errors are encountered.                   *
 *****************************************************************************/
void config( char *configfile )
{
   int      ncommand;     /* # of required commands you expect to process   */
   char     init[20];     /* init flags, one byte for each required command */
   int      nmiss;        /* number of required commands that were missed   */
   char    *com;
   char    *str;
   int      nfiles;
   int      success;
   int      i;

/* Set to zero one init flag for each required command
 *****************************************************/
   ncommand = 11;
   for( i=0; i<ncommand; i++ )  init[i] = 0;

/* Open the main configuration file
 **********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
        logit("e",
                "import_generic: Error opening command file <%s>; exiting!\n",
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
                  logit("e",
                          "import_generic: Error opening command file <%s>; exiting!\n",
                           &com[1] );
                  exit( -1 );
               }
               continue;
            }

        /* Process anything else as a command
         ************************************/
  /*0*/     if( k_its("MyModuleId") ) {
                str=k_str();
                if(str) strcpy(MyModName,str);
                init[0] = 1;
            }
  /*1*/     else if( k_its("RingName") ) {
                str = k_str();
                if(str) strcpy( RingName, str );
                init[1] = 1;
            }
  /*2*/     else if( k_its("HeartBeatInt") ) {
                HeartBeatInt = k_int();
                init[2] = 1;
            }
  /*3*/     else if(k_its("LogFile") ) {
                LogSwitch=k_int();
                init[3]=1;
            }

         /* Maximum size (bytes) for incoming messages
          ********************************************/
  /*4*/     else if( k_its("MaxMsgSize") ) {
                MaxMsgSize = k_long();
                init[4] = 1;
            }

        /* 5 Interval for alive messages to sending machine
        ***************************************************/
            else if( k_its("MyAliveInt") ) {
                MyAliveInt = k_int();
                init[5]=1;
            }

        /* 6 Text of alive message to sending machine
        *********************************************/
            else if( k_its("MyAliveString") ){
                str=k_str();
                if(str) strcpy(MyAliveString,str);
                init[6]=1;
            }

        /* 7 Sender's internet address, in dot notation
        ***********************************************/
            else if(k_its("SenderIpAdr") ) {
                str=k_str();
                if(str) strcpy(SenderIpAdr,str);
                init[7]=1;
            }

        /* 8 Sender's Port Number
        *************************/
            else if( k_its("SenderPort") ) {
                SenderPort = k_int();
                init[8]=1;
            }

        /* 9 Sender's Heart beat interval
        ********************************/
            else if( k_its("SenderHeartRate") ) {
                SenderHeartRate = k_int();
                init[9]=1;
            }

        /* 10 Sender's heart beat text
        ******************************/
            else if(k_its("SenderHeartText") ) {
                str=k_str();
                if(str) strcpy(SenderHeartText,str);
                init[10]=1;
            }

        /* Optional:  Socket timeout length 
        ******************************/
            else if(k_its("SocketTimeout") ) {
              SocketTimeoutLength = k_int();
            }

        /* Optional Command:  Heartbeat Debug Flag
        ******************************/
            else if(k_its("HeartbeatDebug") ) {
                HeartbeatDebug = k_int();
            }

        /* Optional Command:  Socket Debug Flag
        ******************************/
            else if(k_its("SocketDebug") ) {
                SOCKET_ewDebug = k_int();
            }

        /* Unknown command
        *****************/
            else {
                logit("e", "<%s> Unknown command in <%s>.\n",
                         com, configfile );
                continue;
            }

        /* See if there were any errors processing the command
         *****************************************************/
            if( k_err() ) {
               logit("e",
                       "Bad <%s> command in <%s>; exiting!\n",
                        com, configfile );
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
       logit("e", "import_generic: ERROR, no " );
       if ( !init[0] )   logit("e", "<MyModuleId> "   );
       if ( !init[1] )   logit("e", "<RingName> "     );
       if ( !init[2] )   logit("e", "<HeartBeatInt> " );
       if ( !init[3] )   logit("e", "<LogFile> "      );
       if ( !init[4] )   logit("e", "<MaxMsgSize> "   );
       if ( !init[5] )   logit("e", "<MyAliveInt> "   );
       if ( !init[6] )   logit("e", "<MyAliveString> ");
       if ( !init[7] )   logit("e", "<SenderIpAdr> "  );
       if ( !init[8] )   logit("e", "<SenderPort> "   );
       if ( !init[9] )   logit("e", "<SenderHeartRate> " );
       if ( !init[10] )  logit("e", "<SenderHeartText> " );
       logit("e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

   return;
}

/****************************************************************************
 *  lookup( )   Look up important info from earthworm.h tables       *
 ****************************************************************************/
void lookup( void )
{
/* Look up keys to shared memory regions
   *************************************/
   if( ( RingKey = GetKey(RingName) ) == -1 ) {
        fprintf( stderr,
                "import_generic:  Invalid ring name <%s>; exiting!\n", RingName);
        exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &MyInstId ) != 0 ) {
      fprintf( stderr,
              "import_generic: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModName, &MyModId ) != 0 ) {
      fprintf( stderr,
              "import_generic: Invalid module name <%s>; exiting!\n", MyModName );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      fprintf( stderr,
              "import_generic: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      fprintf( stderr,
              "import_generic: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }
   return; 
}

/*******************************************************************************
 * output_status() builds a heartbeat or error message & puts it into          *
 *                 shared memory.  Writes errors to log file & screen.         *
 *******************************************************************************/
void output_status( unsigned char type, short ierr, char *note )
{
   MSG_LOGO    logo;
   char        msg[256];
   long        size;
   time_t        t;

/* Build the message
 *******************/
   logo.instid = MyInstId;
   logo.mod    = MyModId;
   logo.type   = type;

   time( &t );

   if( type == TypeHeartBeat )
   {
        sprintf( msg, "%ld %ld\n", (long) t, (long) MyPid );
   }
   else if( type == TypeError )
   {
        sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note);
        logit( "t", "import_generic(%s): %s\n", MyModName, note );
   }

   size = (int)strlen( msg );   /* don't include the null byte in the message */

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &Region, &logo, size, msg ) != PUT_OK )
   {
        if( type == TypeHeartBeat ) {
           logit("et", "import_generic(%s):  Error sending heartbeat.\n", MyModName );
        }
        else if( type == TypeError ) {
           logit("et", "import_generic(%s):  Error sending error:%d.\n", MyModName, ierr );
        }
   }

   return;
}

/************************** import_filter *************************
 *           Decide  what to do with this message                 *
 *                                                                *
 ******************************************************************
/*      This routine is handed each incoming message, and can do what it likes
with it. The intent is that here is where installation-specificd processing
is done, such as deciding whether we want this message (e.g. is the pick from
an intersting station?), changing its format, etc.
        The default action is to assume that the incoming message was generated
by the Earthworm standard "export_generic" module, which has a companion "export-filter"
routine. The default form of that routine attaches the logo at the start of the
message and exports it. So we peel it off, and drop it into our local transport
medium under that logo.
        We assume that the first 9 characters are three three-character fields
giving IstallationId, ModuleId, and MessageType.
*/


void import_filter( char *msg, int msgLen )
{
    char cInst[4], cMod[4], cType[4];
    MSG_LOGO logo;

    /* We assume that this message was created by our bretheren "export_default",
       which attaches the logo as three groups of three characters at the front of
       the message */
    /* Peel off the logo chacters */
    strncpy(cInst,msg,    3);  cInst[3]=0;  logo.instid =(unsigned char)atoi(cInst);
    strncpy(cMod ,&msg[3],3);  cMod[3] =0;  logo.mod    =(unsigned char)atoi(cMod);
    strncpy(cType,&msg[6],3);  cType[3]=0;  logo.type   =(unsigned char)atoi(cType);

   /* Write the message to shared memory
   ************************************/

   if( tport_putmsg( &Region, &logo, (long)(msgLen-9), &msg[9]) != PUT_OK )
   {
      logit("t", "import_generic(%s):  Error sending message via transport:\n",
            MyModName );
      return;
   }
   return;
}


/*************************** WriteToSocket ************************
 *    send a message logo and message to the socket               *
 *    returns  0 if there are no errors                           *
 *            -1 if any errors are detected                       *
 ******************************************************************/

int WriteToSocket( SOCKET ActiveSocket, char *msg, MSG_LOGO *logo )
{
   char asciilogo[11];       /* ascii version of outgoing logo */
   char startmsg = STX;      /* flag for beginning of message  */
   char endmsg   = ETX;      /* flag for end of message        */
   int  msglength;           /* total length of msg to be sent */
   int  rc;
   char errtxt[128];         /* array for error text           */

   msglength = (int)strlen(msg);  /* assumes msg is null terminated */


/* Send "start of transmission" flag & ascii representation of logo
 ******************************************************************/
   sprintf( asciilogo, "%c%3d%3d%3d",startmsg,
           (int) logo->instid, (int) logo->mod, (int) logo->type );
   rc = send_ew( ActiveSocket, asciilogo, 10, 0, SocketTimeoutLength);
   if( rc != 10 ) 
   {
       sprintf (errtxt, "import_generic(%s): send_ew", MyModName);
       SocketPerror(errtxt);
       return( -1 );
   }

/* Debug print of message
*************************/
 /* printf("import_generic: sending under logo (%s):\n%s\n",asciilogo,msg); */

/* Send message; break it into chunks if it's big!
 *************************************************/
   rc = send_ew( ActiveSocket, msg, msglength, 0, SocketTimeoutLength );
   if ( rc == -1 )
   {
       sprintf (errtxt, "import_generic(%s): send_ew", MyModName);
       SocketPerror(errtxt);
       return( -1 );
   }
/* Send "end of transmission" flag
 *********************************/
   rc = send_ew( ActiveSocket, &endmsg, 1, 0, SocketTimeoutLength);
   if( rc != 1 ) 
   {
       sprintf (errtxt, "import_generic(%s): send_ew", MyModName);
       SocketPerror(errtxt);
       return( -1 );
   }

   return( 0 );
}



