/*
 *   THIS FILE IS UNDER CVS - 
 *   DO NOT MODIFY UNLESS YOU HAVE CHECKED IT OUT.
 *
 *    $Id: import_ack.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.5  2007/03/28 15:42:27  paulf
 *     minor MACOSX directives
 *
 *     Revision 1.4  2007/02/26 14:23:17  paulf
 *     fixed long casting of time_t for heartbeat sprintf()
 *
 *     Revision 1.3  2005/07/25 20:33:17  friberg
 *     put in 1 _LINUX #ifdef directive
 *
 *     Revision 1.2  2005/04/29 19:01:08  dietz
 *     Changed ack protocol slightly to enable import_ack to discover what
 *     type of export it's talking to. It will do either old non-ACK or new
 *     ACK protocol as appropriate on the fly.
 *     Modified to use same config commands as export for setting up the
 *     server connection and socket heartbeat strings and intervals. Still
 *     recognizes the original import_generic commands.
 *
 *     Revision 1.1  2005/04/25 22:26:07  dietz
 *     Added new module import_ack, partner module to export*ack.
 *     Import_ack sends an acknowledgment packet for every packet received.
 *     The socket writing thread has been merged with the socket reading
 *     thread. Most useful for low frequency, high importance packets.
 *
 */


/*
 *   import_generic.c Revision history:
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
 *     Implemented global limits to module, installation, ring, 
 *     and message type strings.
 *     Also, set size of the Alive buffer to MAX_ALIVE_STR -- same as in export.
 *
 *     Revision 1.2  2000/04/03 19:10:09  lombard
 *     Cleaned up setting of SocketTimeout: now defaults to same as 
 *     SenderHeartRate.
 *
 *     Revision 1.1  2000/02/14 18:42:44  lucky
 *     Initial revision
 *
 *
 */

/*
 *   import_generic.c:  Program to receive messages from far away
 *                      via socket and put them into a transport ring.
 *
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

/* versioning added starting with number 2.0.0 on July 10, 2013 */
#define VERSION_STR "2.0.1 2013.07.15"
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
int     SocketHeartbeat( void );
int     SocketSendAck( char * );
thr_ret MessageReceiver( void * );

extern int       errno;
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
static unsigned int SocketTimeoutLength; /* Length of timeouts on SOCKET_ew calls */
static int HeartbeatDebug=0;        /* set to 1 in for heartbeat debug messages */
static int SOCKET_ewDebug=0;        /* set to 1 in for heartbeat debug messages */

static int LogoRewrite = 0;         /* default to NOT rewriting logo  */

/* Globals: timers, etc, used by both threads */
/**********************************************/
#define CONNECT_WAIT_DT 10           /* Seconds wait between connect attempts            */
#define THEAD_STACK_SIZE 8192        /* Implies different things on different systems !! */
      /* on os2, the stack grows dynamically beyond this number if needed, but likely    */
      /* at the expesene of performance. Solaris: not sure, but overflow might be fatal  */
volatile time_t MyLastInternalBeat;  /* time of last heartbeat into local Earthworm ring */
volatile time_t LastServerBeat;      /* times of heartbeats from the server machine      */
volatile time_t MyLastSocketBeat;    /* time of last heartbeat to server - via socket    */
volatile int MessageReceiverStatus =0;    /* status of message receiving thread:         */
                                          /* -1 means bad news                           */
volatile SOCKET Sd;                  /* Socket descriptor                                */
char     *MsgBuf;                    /* incoming message buffer; used by receiver thread */
unsigned  TidMsgRcv;                 /* thread id. was type thread_t on Solaris!         */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          RingKey;       /* key of transport ring for i/o     */
static unsigned char MyInstId;      /* local installation id             */
static unsigned char MyModId;       /* Module Id for this program        */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypeAck;

/* Definitions of export types
 *****************************/
#define  EXPORT_UNKNOWN      0   /* export type not discovered yet        */
#define  EXPORT_OLD          1   /* original no-acknowledgment export     */
#define  EXPORT_ACK          2   /* export which expects acknowledgements */

/* Error messages used by import
 *********************************/
#define  ERR_SOCK_READ       0   /* error reading from socket                      */
#define  ERR_TOOBIG          1   /* retreived msg too large for buffer             */
#define  ERR_TPORTPUT        2   /* error putting message into ring                */
#define  ERR_SERVER_DEAD     3   /* server machine heartbeats not received on time */
#define  ERR_GARBAGE_IN      4   /* something other than STX after ETX             */
#define  ERR_SOCK_WRITE      5   /* error writing to socket                        */

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
      fprintf( stderr, "Usage: import_ack <configfile>\n" );
      fprintf( stderr, "Version: %s\n", VERSION_STR );
      return -1;
   }

/* Initialize name of log-file & open it
 ***************************************/
   logit_init( argv[1], 0, 512, 1 );

/* Read the configuration file(s)
 ********************************/
   config( argv[1] );
   logit( "" , "import_ack(%s): Read command file <%s>\nimport_ack: Version %s\n", 
                MyModName, argv[1], VERSION_STR);

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
   logit("","29 Apr 2005 version: I discover if I should send ACKs.\n");
   if( 1000 * SenderHeartRate >= (int)SocketTimeoutLength )
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
      logit("e", "import_ack(%s): Cannot get PID. Exiting.\n", MyModName);
      return -1;
   }

/* Attach to Input/Output shared memory ring
 *******************************************/
   tport_attach( &Region, RingKey );
   logit( "", "import_ack(%s): Attached to public memory region %s: %d\n",
          MyModName, RingName, RingKey );

/* Allocate the message buffer
 ******************************/
   if( ( MsgBuf = (char *) malloc(MaxMsgSize) ) == (char *) NULL )
   {
     logit("e", "import_ack(%s): Can't allocate buffer of size %ld bytes\n",
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
 **********************************************/
   memset( (char *)&insocket, '\0', sizeof(insocket) );
   insocket.sin_family = AF_INET;
   insocket.sin_port   = htons( (short)SenderPort );

#if defined(_LINUX) || defined(_MACOSX)
   if( (int)(insocket.sin_addr.s_addr = inet_addr(SenderIpAdr)) == -1 )
#else
   if( (int)(insocket.sin_addr.S_un.S_addr = inet_addr(SenderIpAdr)) == -1 )
#endif
   {
      logit( "e", "import_ack(%s): inet_addr failed for SenderIpAdr <%s>; exiting!\n",
              MyModName, SenderIpAdr );
      return -1;
   }
 
/* to prevent flooding the log file during long reconnect attempts
 *****************************************************************/
   retryCount=0;  /* it may be reset elsewere */

/*----------------------------------------------------------
   Connection loop: try to get a socket connection to server
 ------------------------------------------------------------*/

reconnect:            /* we can JUMP here from other places! */
                 
   TidMsgRcv = 0;     /* no receiver thread */
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
      if( difftime(now,MyLastInternalBeat) > (double)HeartBeatInt )
      {
         output_status( TypeHeartBeat, 0, "" );
         MyLastInternalBeat = now;
      }

   /* Are we being told to quit? 
    ****************************/
      if( tport_getflag( &Region ) == TERMINATE  ||
          tport_getflag( &Region ) == MyPid        ) 
      {
         tport_detach( &Region );
         logit("t", "import_ack(%s): terminating on request\n", MyModName);
         free(MsgBuf);
         return 0;
      }

   /* Create a socket
    ******************/
      if( ( Sd = socket_ew( AF_INET, SOCK_STREAM, 0)) == -1 )
      {
         sprintf (errtxt, "import_ack(%s): socket_ew", MyModName);
         SocketPerror(errtxt);
         logit( "et", "import_ack(%s): Error opening socket, exiting!\n", MyModName );
         closesocket_ew((int)Sd,SOCKET_CLOSE_IMMEDIATELY_EW);
         free(MsgBuf);
         return -1;
      }
      if( retryCount <= 4 ) {
         logit("t", "import_ack(%s): Trying to connect to %s on port %d\n",
	        MyModName, SenderIpAdr, SenderPort);
      }

   /* Attempt the connection, if it fails close socket and sleep 
    ************************************************************/
      if( connect_ew( (int)Sd, (struct sockaddr *)&insocket, sizeof(insocket),
                     SocketTimeoutLength ) == -1 )
      {
         closesocket_ew((int)Sd, SOCKET_CLOSE_IMMEDIATELY_EW);

         if( retryCount < 4 ) {
            logit("t", "import_ack(%s): Failed to connect. Waiting...\n", 
                   MyModName );
         }
         if( retryCount == 4 ) {
            logit("t", "import_ack(%s): Failed to connect. Will try to "
                   "connect every %d seconds, but will not log repetitions.\n", 
                   MyModName, CONNECT_WAIT_DT );
         }
         sleep_ew(CONNECT_WAIT_DT*1000);
      }

   /* else the connection succeeded...let's get out of this loop! 
    *************************************************************/
      else
      {
         logit( "t", "import_ack(%s): Connected after %d seconds (on try %d)\n",
                 MyModName, (int)(time(&now)-tfirsttry), retryCount );
         connected  = 1;
         retryCount = 0;
      }
   } /* end of connection loop */

/* Initialize Socketheartbeat things
 ***********************************/
   MyLastSocketBeat = 0;             /* will send socketbeat first thing */
   time((time_t*)&LastServerBeat);   /* will think it just got a socketbeat
                                        from the serving machine */

/* Start the message receiver thread
 ***********************************/
   MessageReceiverStatus =0; /* set it's status flag to ok */
   if( StartThread( MessageReceiver, (unsigned)THEAD_STACK_SIZE, &TidMsgRcv ) == -1 )
   {
      logit( "et", "import_ack(%s): Error starting MessageReceiver thread. Exiting.\n",
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
   while( 1 )
   {
      sleep_ew(1000); /* sleep one second. Remember, the receieve thread is awake */
      time(&now);

    /* Beat our heart (into the local Earthworm) if it's time 
     ********************************************************/
      if( difftime(now,MyLastInternalBeat) > (double)HeartBeatInt )
      {
         output_status( TypeHeartBeat, 0, "" );
         MyLastInternalBeat = now;
      }

    /* How's the server's heart?  
     ***************************/
      if( difftime(now,LastServerBeat) > (double)SenderHeartRate && 
          SenderHeartRate !=0 )
      {
         output_status(TypeError,ERR_SERVER_DEAD,"Server heartbeat lost. Restarting");
	 logit("et","No heartbeat received for %d seconds. "
                    "Restarting connection\n", SenderHeartRate);
         quit=1; /*restart*/
      }

    /* How's the receive thread feeling?
     ***********************************/
      if( MessageReceiverStatus == -1 )
      {
         logit("et", "import_ack(%s): MessageReceiver thread has quit. Restarting\n",
                MyModName);
         quit=1;
      }

    /* Are we being told to quit?
     ****************************/
      if( tport_getflag( &Region ) == TERMINATE ||
          tport_getflag( &Region ) == MyPid       )
      {

         tport_detach( &Region );
         logit("t", "import_ack(%s): terminating on request\n", MyModName);
         (void)KillThread(TidMsgRcv);
         return 0;
      }

    /* Restart preparations
     **********************/
      if( quit == 1 )
      {
         (void)KillThread(TidMsgRcv);
         TidMsgRcv = 0;     /* no receiver thread */
         closesocket_ew( (int)Sd, SOCKET_CLOSE_IMMEDIATELY_EW );
         quit=0;
         goto reconnect;
      }

   }  /* end of working loop */
}

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

thr_ret MessageReceiver( void *dummy )
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
   char        errtxt[128];            /* array for error text */
   char        seqstr[4];              /* sequence string for acknowledgments */
   int         seq;                    /* sequence number */
   char       *msgptr = NULL;          /* pointer to beginning of logo in MsgBuf */
   int         msglen;                 /* length of asciilogo + message */

/* Initialize stuff
 ******************/
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
   state=SEARCHING_FOR_MESSAGE_START; /* we're initializing */

   while(1) /* loop over bytes read from socket */
   {
   /* Get next char operation */
      if (++inchar == nr)
      {
      /* Read from socket operation */
         nr = recv_ew( (int)Sd, inBuffer, INBUFFERSIZE, 0, SocketTimeoutLength );
         if( nr<=0 ) {   /* Connection Closed */
            logit( "t", "import_ack(%s): Bad socket read: %d\n", MyModName, nr );
            goto suicide;
         }
         inchar=0;
      /* End of Read from socket operation */
      }
      lastChr=chr;
      chr=inBuffer[inchar];
   /* End of Get next char operation */

   /* Initialization:
    * throw all away until we see a naked start character
    *****************************************************/
      if( state==SEARCHING_FOR_MESSAGE_START )  
      {
      /* Do we have a real start character?
       *************************************/
         if( lastChr!=escape && chr==startCharacter )
         {
            state=ASSEMBLING_MESSAGE;  /* from now on, assemble message */
            continue;
         }
      }

   /* Confirm message start: 
    * the next char had better be a start character - naked, that is  
    ****************************************************************/
      if( state==EXPECTING_MESSAGE_START )  
      {
      /* Is it a naked start character?
      *********************************/
         if( chr==startCharacter && lastChr != escape ) /* This is it: message start!! */
         {
            nchar=0;         /* start with first char position */
            state=ASSEMBLING_MESSAGE; /* go into assembly mode */
            continue;
         }
         else   /* we're eating garbage */
         {
            logit("et", "import_ack(%s): Unexpected character from export. "
                        "Re-synching\n", MyModName);
            state=SEARCHING_FOR_MESSAGE_START; /* search for next start sequence */
            continue;
         }
      }

   /* In the throes of assembling a message 
    ***************************************/
      if( state==ASSEMBLING_MESSAGE )
      {
      /* Naked end character: end of the message is at hand
       ****************************************************/
         if( chr==endCharacter ) 
         {
         /* We have a complete message!
          *****************************/
            MsgBuf[nchar] = 0;              /* terminate as a string  */
            state=EXPECTING_MESSAGE_START;  /* reset for next message */ 
   
         /* Discover what type of export we're talking to
          ***********************************************/
            if( export_type == EXPORT_UNKNOWN )
            {
               if( strncmp( MsgBuf, "SQ:", 3 ) == 0 ) {
                  logit("et","import_ack(%s): I will send ACKs to export.\n",
                         MyModName );
                  export_type = EXPORT_ACK;
               } else {
                  logit("et","import_ack(%s): I will NOT send ACKs to export.\n",
                         MyModName );
                  export_type = EXPORT_OLD;
               }
            }

         /* Set the working msg pointer based on type of export;
          * Get sequence number and send ACK (if necessary) 
          *******************************************************/
            if( export_type == EXPORT_ACK )
            {
               if( nchar < 15 )  /* too short to be a valid msg from export_acq! */
               {
                  logit( "et","import_ack(%s): Skipping msg too short for "
                              "ack-protocol\n%s\n", MyModName, MsgBuf );
                  continue;
               }
               strncpy( seqstr, &MsgBuf[3], 3 );
               seqstr[3] = 0;
               seq       = atoi(seqstr); /* stash sequence number */
               msgptr    = &MsgBuf[6];   /* point to beginning of ascii logo */
               msglen    = nchar - 6;
               if( SocketSendAck( seqstr ) != 0 ) goto suicide; /* socket write error */
            } 
            else /* export_type == EXPORT_OLD */
            {
               if( nchar < 9 )  /* too short to be a valid msg from old exports! */
               {
                  logit( "et","import_ack(%s): Skipping msg too short for "
                              "non-ack-protocol\n%s\n", MyModName, MsgBuf );
                  continue;
               }
               seq    = -1;     /* unknown sequence number */
               msgptr = MsgBuf; /* point to beginning of ascii logo */
               msglen = nchar;  
            }
            
         /* See if the message is server's heartbeat
          ******************************************/
            if( strcmp(&msgptr[9],SenderHeartText)==0 ) 
            {
               if( HeartbeatDebug )
               {
                  logit("et", "import_ack(%s): Received heartbeat\n", MyModName); 
               }
            }

         /* Seq# told us to expect heartbeat, but text didn't match;
          * might not be talking to the correct export!
          **********************************************************/
            else if( seq == HEARTSEQ )
            {
               logit("et", "import_ack(%s): Rcvd socket heartbeat <%s> " 
                     "does not match expected <%s>; check config!\n",
                      MyModName, &msgptr[9], SenderHeartText );
               continue;
            }

         /* Otherwise, it must be a message to forward along
          **************************************************/
            else                              
            {
               if( HeartbeatDebug )
               {
                  logit("et", "import_ack(%s): Received non-heartbeat\n", MyModName); 
               }
               import_filter( msgptr, msglen ); /* process msg via user-routine */
            }
         /* All messages and heartbeats prove that export is alive, */
         /* so we will update our server heartbeat monitor now.     */
            time((time_t*)&LastServerBeat); 

         /* This is a good time to do socket heartbeat stuff 
          **************************************************/
            if( SocketHeartbeat() != 0 ) goto suicide; /* socket write error */
            continue;
         }

      /* Process the message byte we just read: 
       * we know it's not a naked end character
       ****************************************/
         else
         {
         /* Process an escape sequence 
          ****************************/
            if( chr==escape )  
            {  
            /* Read from buffer, or get more chars if buffer is empty
             ********************************************************/
            /* Get next char operation */
               if( ++inchar == nr )
               {
               /* Read from socket operation */
                  nr=recv_ew((int)Sd,inBuffer,INBUFFERSIZE,0, SocketTimeoutLength);
                  if( nr<=0 ) {   /* Connection Closed */
                     logit( "t", "import_ack(%s): Bad socket read: %d\n", 
                            MyModName, nr );
                     goto suicide;  
                  }
                  inchar=0;
               /* End of Read from socket operation */
               }
               lastChr=chr;
               chr=inBuffer[inchar];
            /* End of Get next char operation */
     
            /* Bad news: unknown escape sequence */
               if( chr != startCharacter  && 
                   chr != endCharacter    && 
                   chr != escape             )
               { 
                  logit("et", "import_ack(%s): Unknown escape sequence in message. "
                              "Re-synching\n",MyModName);
                  state=SEARCHING_FOR_MESSAGE_START; /* search for next start sequence */
                  continue;
               }
            /* OK: it's a legit escape sequence; save the escaped byte */
               else 
               {
                  MsgBuf[nchar++]=chr; if(nchar>=MaxMsgSize) goto freak;  
                  continue;
               }
            }
  
         /* Bad news: Naked (unescaped) start character 
          *********************************************/
            if( chr==startCharacter )
            {   
               logit("et", "import_ack(%s): Unescaped start character in message; "
                           "re-synching\n", MyModName);
               state=SEARCHING_FOR_MESSAGE_START; /* search for next start sequence */
               continue;
            }
  
         /* So it's not a naked start, escape, or a naked end: 
          * Hey, it's just a normal byte, save it!
          ****************************************************/
            MsgBuf[nchar++]=chr; if(nchar>=MaxMsgSize) goto freak; 
            continue;
    
         /* freakout: message won't fit in MsgBuf!
          ****************************************/
            freak:  
            {
               logit("et", "import_ack(%s): input buffer overflow at %ld bytes; "
                     "increase MaxMsgSize and restart!\n",
                      MyModName, MaxMsgSize);
               state=SEARCHING_FOR_MESSAGE_START; /* initialize again */
               nchar=0;
               continue;
            }
         } /* end of else not-an-endCharacter processing */
      } /* end of state==ASSEMBLING_MESSAGE processing */
   }  /* end of loop over characters */

suicide:
   logit( "et", "import_ack(%s): Error in socket communication; "
                "stopping MessageReceiver thread\n", MyModName );
   sprintf( errtxt, "import_ack(%s): ", MyModName );
   SocketPerror(errtxt);
   MessageReceiverStatus = -1; /* file a complaint to the main thread */
   KillSelfThread();           /* main thread will restart us */
   logit( "t", "import_ack(%s): Fatal system error: "
               "MessageReceiver thread could not KillSelf\n",
	        MyModName );
   exit(-1);
}  /* end of MessageReceiver thread */


/************************** SocketHeartbeat ***********************
 *           Send a heartbeat to the server via socket            *
 ******************************************************************/
int SocketHeartbeat( void )
{
   MSG_LOGO  hrtlogo;
   time_t    now;
   char      errtxt[256];    /* array for error text */

/* Beat our heart (over the socket) to our server
 ************************************************/
   time(&now);

   if( MyAliveInt != 0     &&
       difftime(now,MyLastSocketBeat) > (double)MyAliveInt ) 
   {
      hrtlogo.instid = MyInstId;
      hrtlogo.mod    = MyModId;
      hrtlogo.type   = TypeHeartBeat;
      if( WriteToSocket( Sd, MyAliveString, &hrtlogo ) != 0 )
      {
         sprintf( errtxt, "error sending alive msg to socket\n" );
         output_status( TypeError, ERR_SOCK_WRITE, errtxt );
         return( -1 );
      }
      if( HeartbeatDebug )
      {
         logit("et","import_ack(%s): SocketHeartbeat sent to export\n", 
               MyModName);
      }
      MyLastSocketBeat = now;
   }
   return( 0 );
}


/*************************** SocketSendAck ************************
 *     Send an acknowledgment packet to the server via socket     *
 ******************************************************************/
int SocketSendAck( char *seq )
{
   MSG_LOGO  acklogo;
   char      ackstr[10];
   char      errtxt[256];    /* array for error text */

/* Send an acknowledgment (over the socket) to our server
 ********************************************************/
   acklogo.instid = MyInstId;
   acklogo.mod    = MyModId;
   acklogo.type   = TypeAck;
   sprintf( ackstr, "ACK:%s", seq );
   if( WriteToSocket( Sd, ackstr, &acklogo ) != 0 )
   {
      sprintf( errtxt, "error sending %s to socket\n", ackstr );
      output_status( TypeError, ERR_SOCK_WRITE, errtxt );
      return( -1 );
   }
   if( HeartbeatDebug )
   {
      logit("et","import_ack(%s): acknowledgment %s sent to export\n", 
             MyModName, ackstr );
   }
   return( 0 );
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
                "import_ack: Error opening command file <%s>; exiting!\n",
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
                          "import_ack: Error opening command file <%s>; exiting!\n",
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

  /*4*/  /* Maximum size (bytes) for incoming messages
          ********************************************/
            else if( k_its("MaxMsgSize") ) {
                MaxMsgSize = k_long();
                init[4] = 1;
            }

  /*5*/  /* Interval for alive messages to sending machine
          ************************************************/
            else if( k_its("MyAliveInt")   ||
                     k_its("SendAliveInt")    ) {
            /* import_ack checks timing for its outgoing alive msgs only
             * after each complete msg recv'd. If import_ack/export_ack 
             * have the same alive interval, it's possible for import_ack 
             * to recv an export_ack alive msg just a fractional second 
             * before it would have sent its own heartbeat, causing 
             * it to "skip" its own heartbeat until it recv's the next 
             * message or heartbeat. Here we subtract 1 from import_ack's 
             * configured MyAliveInterval to help ensure import_ack's 
             * alive messages are sent at close to the expected interval.
             */
                MyAliveInt = k_int() - 1;  
                init[5]=1;
            }

  /*6*/  /* Text of alive message to sending machine
          ******************************************/
            else if( k_its("MyAliveString") ||
                     k_its("SendAliveText")    ) {
                str=k_str();
                if(str) strcpy(MyAliveString,str);
                init[6]=1;
            }

  /*7*/  /* Sender's internet address, in dot notation
          ********************************************/
            else if( k_its("SenderIpAdr") ||
                     k_its("ServerIPAdr")    ) {
                str=k_str();
                if(str) strcpy(SenderIpAdr,str);
                init[7]=1;
            }

  /*8*/  /* Sender's Port Number
          **********************/
            else if( k_its("SenderPort") ||
                     k_its("ServerPort")    ) {
                SenderPort = k_int();
                init[8]=1;
            }

  /*9*/  /* Sender's Heart beat interval
          ******************************/
            else if( k_its("SenderHeartRate") ||
                     k_its("RcvAliveInt")        ) {
                SenderHeartRate = k_int();
                init[9]=1;
            }

  /*10*/ /* Sender's heart beat text
          ***************************/
            else if( k_its("SenderHeartText") ||
                     k_its("RcvAliveText")       ) {
                str=k_str();
                if(str) strcpy(SenderHeartText,str);
                init[10]=1;
            }

         /* Optional: Socket timeout length 
          *********************************/
            else if(k_its("SocketTimeout") ) {
              SocketTimeoutLength = k_int();
            }

         /* Optional: Heartbeat Debug Flag
          *********************************/
            else if(k_its("HeartbeatDebug") ) {
                HeartbeatDebug = k_int();
            }

         /* Optional: Socket Debug Flag
          *****************************/
            else if(k_its("SocketDebug") ) {
                SOCKET_ewDebug = k_int();
            }
  	 /* Optional: LogoRewrite - rewrite the logo as my own ModID and Installation */
            else if(k_its("LogoRewrite") ) {
                LogoRewrite = k_int();
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
       logit("e", "import_ack: ERROR, no " );
       if ( !init[0] )   logit("e", "<MyModuleId> "    );
       if ( !init[1] )   logit("e", "<RingName> "      );
       if ( !init[2] )   logit("e", "<HeartBeatInt> "  );
       if ( !init[3] )   logit("e", "<LogFile> "       );
       if ( !init[4] )   logit("e", "<MaxMsgSize> "    );
       if ( !init[5] )   logit("e", "<SendAliveInt> "  );
       if ( !init[6] )   logit("e", "<SendAliveText> " );
       if ( !init[7] )   logit("e", "<ServerIPAdr> "   );
       if ( !init[8] )   logit("e", "<ServerPort> "    );
       if ( !init[9] )   logit("e", "<RcvAliveInt> "   );
       if ( !init[10] )  logit("e", "<RcvAliveText> "  );
       logit("e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

   return;
}

/****************************************************************************
 *  lookup( )   Look up important info from earthworm*.d tables             *
 ****************************************************************************/
void lookup( void )
{
/* Look up keys to shared memory regions
   *************************************/
   if( ( RingKey = GetKey(RingName) ) == -1 ) {
        logit( "e",
               "import_ack:  Invalid ring name <%s>; exiting!\n", RingName);
        exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if( GetLocalInst( &MyInstId ) != 0 ) {
      logit( "e",
             "import_ack: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if( GetModId( MyModName, &MyModId ) != 0 ) {
      logit( "e",
             "import_ack: Invalid module name <%s>; exiting!\n", MyModName );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      logit( "e",
             "import_ack: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }
   if( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      logit( "e",
             "import_ack: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }
   if( GetType( "TYPE_ACK", &TypeAck ) != 0 ) {
      logit( "e",
             "import_ack: Invalid message type <TYPE_ACK>; exiting!\n" );
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
        logit( "t", "import_ack(%s): %s\n", MyModName, note );
   }

   size = (int)strlen( msg );   /* don't include the null byte in the message */

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &Region, &logo, size, msg ) != PUT_OK )
   {
        if( type == TypeHeartBeat ) {
           logit("et", "import_ack(%s):  Error sending heartbeat.\n", MyModName );
        }
        else if( type == TypeError ) {
           logit("et", "import_ack(%s):  Error sending error:%d.\n", MyModName, ierr );
        }
   }

   return;
}

/************************** import_filter *************************
 *                process this message from export                *
 ******************************************************************

/* This routine is handed each incoming message, and can do what 
 * it likes with it. The intent is that here is where installation-
 * specific processing is done, such as deciding whether we want 
 * this message (e.g. is the pick from an intersting station?), 
 * changing its format, etc.
 *
 * The default action is to assume that export attached a logo at 
 * the start of the message before sending it. So we interpret 
 * that logo, and drop the naked message into our local transport
 * medium under that logo.
 *
 * We assume that the first 9 characters are three three-character 
 * fields giving IstallationId, ModuleId, and MessageType.
 */

void import_filter( char *msg, int msgLen )
{
   char cInst[4], cMod[4], cType[4];
   MSG_LOGO logo;

/* We assume that a logo (three groups of three characters) is attached 
 * in front of the message. Interpret the logo characters, and use them 
 * when placing the message into the transport ring. 
 **********************************************************************/
   strncpy(cInst, msg,   3);  cInst[3]=0;  logo.instid =(unsigned char)atoi(cInst);
   strncpy(cMod ,&msg[3],3);  cMod[3] =0;  logo.mod    =(unsigned char)atoi(cMod);
   strncpy(cType,&msg[6],3);  cType[3]=0;  logo.type   =(unsigned char)atoi(cType);

   if(LogoRewrite) 
   {
     logo.instid = MyInstId;
     logo.mod = MyModId;
   }

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &Region, &logo, (long)(msgLen-9), &msg[9]) != PUT_OK )
   {
      logit( "t", "import_ack(%s):  Error sending message via transport:\n",
             MyModName );
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
      sprintf (errtxt, "import_ack(%s): send_ew logo", MyModName);
      SocketPerror(errtxt);
      return( -1 );
   }

/* Debug print of message
 *************************/
/* printf("import_ack: sending under logo (%s):\n%s\n",asciilogo, msg); */

/* Send message; break it into chunks if it's big!
 *************************************************/
   rc = send_ew( ActiveSocket, msg, msglength, 0, SocketTimeoutLength );
   if( rc == -1 )
   {
      sprintf (errtxt, "import_ack(%s): send_ew msg", MyModName);
      SocketPerror(errtxt);
      return( -1 );
   }

/* Send "end of transmission" flag
 *********************************/
   rc = send_ew( ActiveSocket, &endmsg, 1, 0, SocketTimeoutLength);
   if( rc != 1 ) 
   {
      sprintf (errtxt, "import_ack(%s): send_ew eot", MyModName);
      SocketPerror(errtxt);
      return( -1 );
   }

   return( 0 );
}



