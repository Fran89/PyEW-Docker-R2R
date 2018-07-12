/*
 *   THIS FILE IS UNDER CVS - 
 *   DO NOT MODIFY UNLESS YOU HAVE CHECKED IT OUT.
 *
 *    $Id: import_gen_pasv.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.11  2007/09/18 16:51:11  paulf
 *     fixed strncpy to strcpy for config reading
 *
 *     Revision 1.10  2007/03/28 17:13:16  paulf
 *     patched for _MACOSX flags and compilation
 *
 *     Revision 1.9  2007/02/26 14:23:17  paulf
 *     fixed long casting of time_t for heartbeat sprintf()
 *
 *     Revision 1.8  2006/04/13 18:49:30  dietz
 *     Modified RingName and MyModName string lenghts and added a bit of error-
 *     checking in reading those strings from the config file. Another of
 *     Richard Godbee's fixes I forgot to add earlier.
 *
 *     Revision 1.7  2006/04/13 18:11:26  dietz
 *     Changed accept_ew timeout to HeartBeatInt (had been hardcoded to 9sec).
 *     Added inter-connection time interval to logging. Gave all logit calls
 *     consistent format, added/modified some comments.
 *
 *     Revision 1.6  2006/04/12 22:41:10  dietz
 *     Reformatted with proper indenting; fixed some comments
 *
 *     Revision 1.5  2006/04/12 20:16:18  dietz
 *     Incorporated Richard Godbee's bug fixes in accepting socket connections.
 *
 */

/****************************************************************************
 *   import_gen_pasv.c:  Program to receive messages from far away
 *                      via socket and put them into a transport ring.
 *
 *   The intent is that there will be a specific import_xxx module for
 *   importing from institution xxx. That is, there will be as many
 *   import modules as places to import from, each module having it's
 *   own name, and living in its own directory.  Each such module could
 *   have it's own "import_filter" routine, coded for the specific job.
 *   
 *  -AM: Tue Apr  4 14:21:08 MDT 2000
 *  converted the active import_generic to passive, let the export module
 *  initiate communication. import_gen_pasv module prepares a port and waits 
 *  for connection from active export module to receive messages.
 *
 ****************************************************************************/

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
#include <mask_check.h>
#include <trace_buf.h>

#define MAX_LOGO  20
#define PERFECT_MATCH_NETMASK "255.255.255.255"

/* 
   Versioning introduced with 0.1.0 on January 20, 2015 - netmask option added 
*/
#define VERSION "0.1.0 2015-01-20"
#define MAX_IP_STR 17

/* Functions in this source file
*******************************/
void    config( char * );
void    lookup( void );
void    output_status( unsigned char, short, char * );
void    import_filter( char *, int);
int     WriteToSocket( SOCKET, char *, MSG_LOGO * );
thr_ret Heartbeat( void * );
thr_ret MessageReceiver( void * );

extern   int       errno;
static   SHM_INFO  Region;           /* shared memory region to use for i/o */
volatile pid_t     MyPid;            /* process id, sent with heartbeat     */

/* Things to read or derive from configuration file
**************************************************/
static char RingName[MAX_RING_STR];  /* name of transport ring for i/o    */
static char MyModName[MAX_MOD_STR];  /* speak as this module name/id      */
static int  LogSwitch;               /* 0 if no logfile should be written */
static int  HeartBeatInt;            /* seconds between heartbeats        */
                                     /* (to local ring)                   */
static long MaxMsgSize;              /* max size for input/output msgs    */

static char MyAliveString[100];      /* Text of above alive message       */
static int  MyAliveInt;              /* Seconds between sending alive     */
                                     /* message to foreign sender         */

static char ReceiverIpAdr[MAX_IP_STR];       /* Receiver's IP address             */
static int  ReceiverPort;            /* Receiver's well-known port number */

static char SenderIpAdr[MAX_IP_STR];         /* Sender's IP address for checking  */
static char SenderNetMask[MAX_IP_STR];       /* Sender's IP address for netmask check [OPTIONAL]  */

static char SenderHeartText[100];    /* Text making up sender's heartbeat msg    */
static int  SenderHeartRate;         /* Expect alive msgs this often from sender */

MSG_LOGO    PutAsLogo;               /* logo to be used for placing received msgs */
                                     /* into ring. May be superceeded in the msg  */
                                     /* processing routine "import_filter"        */

static unsigned int SocketTimeoutLength; /* Length of timeouts on SOCKET_ew calls */
static int  HeartbeatDebug = 0;          /* set to 1 for heartbeat debug messages */
static int  SOCKET_ewDebug = 0;          /* set to 1 for socket debug messages    */

/* Globals: timers, etc, used by both threads
********************************************/
#define CONNECT_WAIT_DT 10           /* Seconds wait between connect attempts            */
#define THEAD_STACK_SIZE 8192        /* Implies different things on different systems !! */
      /* on os2, the stack grows dynamically beyond this number if needed, but likey     */
      /* at the expesene of performance. Solaris: not sure, but overflow might be fatal  */

volatile time_t LastSenderBeat;      /* times of heartbeats from the sender machine      */
volatile time_t MyLastInternalBeat;  /* time of last heartbeat into local Earthworm ring */
volatile time_t MyLastSocketBeat;    /* time of last heartbeat to server - via socket    */

volatile int HeartThreadStatus=0;     /* < 0 if server croaks. Set by heartbeat thread   */
volatile int MessageReceiverStatus=0; /* status of msg-receiving thread: -1 is bad news  */
									                  
char        *Argv0;                   /* pointer to executable name                      */
char        *MsgBuf;                  /* incoming msg buffer; used by receiver thread    */
unsigned     TidHeart;                /* thread id. was type thread_t on Solaris!        */
unsigned     TidMsgRcv;               /* thread id. was type thread_t on Solaris!        */
SOCKET       PassiveSocket;           /* Passive Socket descriptor (receiver - import)   */
SOCKET       ActiveSocket;            /* Active Socket descriptor (sender - export)    	 */

/* Things to look up in the earthworm.h tables with getutil.c functions
**********************************************************************/
static long          RingKey;         /* key of transport ring for i/o                   */
static unsigned char MyInstId;        /* local installation id                           */
static unsigned char MyModId;         /* Module Id for this program                      */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;

/* Error messages used by import
*********************************/
#define  ERR_SOCK_READ       0      /* error reading from socket               */
#define  ERR_TOOBIG          1      /* retreived msg too large for buffer      */
#define  ERR_TPORTPUT        2      /* error putting message into ring         */
#define  ERR_SENDER_DEAD     3      /* sender heartbeats not received on time  */
#define  ERR_GARBAGE_IN      4      /* something other than STX after ETX      */
#define  ERR_SOCKETSEND      5      /* trouble writing to socket               */
#define  ERR_SOCKETACCEPT    6      /* error accepting socket connection       */
#define  ERR_NOCONN          7      /* no connection after timeout             */
#define  ERR_CONN            8      /* Connection accepted; cancels ERR_NOCONN */
static char  errText[256];          /* string for log/error messages           */

/* Socket status values
***********************/
#define SOCK_NEW             0      /* New socket net yet connected            */
#define SOCK_NOT_CONN        1      /* No connection after timeout             */
#define SOCK_CONNECTED       2      /* Active socket connection                */
int SockStatus = SOCK_NEW;          /* Current socket status (main thread)     */

/*****************************************************************************************/
/*                                   Main                                                */
/*****************************************************************************************/
int main( int argc, char **argv )
{
   struct sockaddr_in skt_snd;
   struct sockaddr_in skt_rcv;
   time_t now, acceptstart;
   char   sender_ip[MAX_IP_STR];       /* Sender IP Address from inet_ntoa() */
   int    senderLen;
   int    on   = 1;
   int    quit = 0;
   int    sockErr;

/* Catch broken socket signals
 *****************************/
#ifdef _SOLARIS
   (void)sigignore(SIGPIPE);
#endif

/* Check command line arguments
 ******************************/
   Argv0 = argv[0];
   if ( argc != 2 )
   {
      fprintf( stderr, "Usage: import_gen_pasv <configfile>\n" );
      fprintf( stderr, "Version: %s\n", VERSION );
      return -1;
   }

/* Initialize name of log-file & open it
 ***************************************/
   logit_init( argv[1], 0, 512, 1 );

/* Read the configuration file(s)
 ********************************/
   config( argv[1] );
   logit( "t", "%s(%s): Read command file <%s>\n", 
          Argv0, MyModName, argv[1] );
   logit( "t", "%s(%s): Version number %s\n", 
          Argv0, MyModName, VERSION );

/* Reinitialize the logging level
 ********************************/
   logit_init( argv[1], 0, 512, LogSwitch );

/* Set Socket Debug appropriately
 ********************************/
   setSocket_ewDebug(SOCKET_ewDebug);
   logit("t", "Using Netmask of <%s> on Sender IP %s\n", SenderNetMask, SenderIpAdr );

/* Look up important info from earthworm.h tables
 ************************************************/
   lookup();

/* Get our Process ID for restart purposes
 *****************************************/
   MyPid = getpid();
   if( MyPid == -1 )
   {
      logit( "e", "%s(%s): Cannot get PID. Exiting.\n", Argv0, MyModName);
      return( -1 );
   }

/* Attach to Input/Output shared memory ring
 *******************************************/
   tport_attach( &Region, RingKey );
   logit( "", "%s(%s): Attached to public memory region %s: %d\n",
          Argv0, MyModName, RingName, RingKey );

/* Allocate the message buffer
 *****************************/
   if( ( MsgBuf = (char *) malloc(MaxMsgSize) ) == (char *) NULL )
   {
      logit( "e", "%s(%s): Can't allocate buffer of size %ld bytes\n",
             Argv0, MyModName, MaxMsgSize );
      return( -1 );
   }

/* Put out one heartbeat before listening to socket
 ***************************************************/
   output_status( TypeHeartBeat, 0, "" );

/* Initialize the socket system
 ******************************/
   SocketSysInit();

/* Create a socket
 *****************/
   if( ( PassiveSocket = socket_ew( AF_INET, SOCK_STREAM, 0) ) == -1 )
   {
      logit( "et", "%s(%s): Error opening socket; exiting!\n", Argv0, MyModName );
      closesocket_ew( PassiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW );
      tport_detach( &Region );
      free( MsgBuf );
      return( -1 );
   }

/* Fill in receiver (import_pasv) socket address structure
 **********************************************************/
   memset( (char *) &skt_rcv, '\0', sizeof(skt_rcv) );
   skt_rcv.sin_family = AF_INET;
   skt_rcv.sin_port   = htons( (short)ReceiverPort );
#if defined(_LINUX) || defined(_MACOSX)
   if ((int)(skt_rcv.sin_addr.s_addr = inet_addr(ReceiverIpAdr)) == -1)
#else
   if ((int)(skt_rcv.sin_addr.S_un.S_addr = inet_addr(ReceiverIpAdr)) == -1)
#endif
   {
      logit( "e", "%s(%s): inet_addr failed for ReceiverIpAdr <%s>;"
             " exiting!\n", Argv0, MyModName, ReceiverIpAdr );
      closesocket_ew( PassiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW );
      tport_detach( &Region );
      free( MsgBuf );
      return( -1 );
   }
 
/* Allow the receiver (import_pasv) to be stopped and restarted
 **************************************************************/
   on = 1;
   if( setsockopt( PassiveSocket, SOL_SOCKET, SO_REUSEADDR,
                  (char *)&on, sizeof(char *) ) != 0 )
   {
      logit( "et", "%s(%s): Error on setsockopt; exiting!\n",
             Argv0, MyModName );
      perror("Export setsockopt");
      closesocket_ew( PassiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW );
      tport_detach( &Region );
      free( MsgBuf );
      return( -1 );
   }

/* Bind socket to a name
 ***********************/
   if( bind_ew( PassiveSocket, (struct sockaddr *) &skt_rcv, sizeof(skt_rcv)) )
   {
      logit( "et", "%s(%s): error binding socket; exiting.\n",
             Argv0, MyModName);
      perror( "Export bind error" );
      closesocket_ew( PassiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW );
      tport_detach( &Region );
      free( MsgBuf );
      return( -1 );
   }

/* Prepare for connect requests
 ******************************/
   if( listen_ew( PassiveSocket, 0 ) )
   {  
      logit( "et", "%s(%s): socket listen error; exiting!\n",
             Argv0, MyModName );
      closesocket_ew( PassiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW );
      tport_detach( &Region );
      free( MsgBuf );
      return( 1 );
   }

/* Main loop; get a connection, start threads, monitor threads
 *************************************************************/
   time(&acceptstart);   

   while( quit == 0 )
   {
   /* Put out one heartbeat
    ***********************/
      output_status( TypeHeartBeat, 0, "" );

   /* Terminate if requested to
    ***************************/
      if( tport_getflag( &Region ) == TERMINATE ||
          tport_getflag( &Region ) == MyPid        )
      {
         tport_detach( &Region );
         logit( "t", "%s(%s): terminating on request\n", Argv0, MyModName );
         closesocket_ew( PassiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW );
         free( MsgBuf );
         return( 0 );
      }

   /* Wait for a connection (blocking, with timeout = heartbeat interval)
    *********************************************************************/
      senderLen = sizeof( skt_snd );
      logit( "et", "%s(%s): Waiting for new connection.\n", Argv0, MyModName );

      SockStatus = SOCK_NEW;
      while( (ActiveSocket = accept_ew( PassiveSocket, 
                                        (struct sockaddr*) &skt_snd,
                                        &senderLen, HeartBeatInt*1000 ) ) 
                                        == INVALID_SOCKET )
      { 
         sockErr = socketGetError_ew();
         if( sockErr == WOULDBLOCK_EW )   /* Timed-out waiting for a connection */
         {
            if( SockStatus == SOCK_NEW )
            {
               SockStatus = SOCK_NOT_CONN;
               sprintf( errText, "No connection after %ld seconds; "
                        "will continue to wait quietly.\n", 
                        (long)(time(&now)-acceptstart) );
               output_status( TypeError, ERR_NOCONN, errText );
            }
         }
         else   /* Had an error on accept */
         {
            logit( "et", "%s(%s): error on accept: %d %s\n; exiting!\n",  
                   Argv0, MyModName, sockErr, strerror(sockErr) );
            closesocket_ew( ActiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW );
            tport_detach( &Region );      
            return( -1 );
         }

      /* Are we supposed to shut down? */
         if( tport_getflag( &Region ) == TERMINATE ) goto shutdown;
         if( tport_getflag( &Region ) == MyPid     ) goto shutdown;

      /* Beat our heart so statmgr knows we're alive */
         output_status( TypeHeartBeat, 0, "" );

      } /* end while(accept_ew==INVALID_SOCKET) */
   
   /* Got a connection! Is the sender who we expect?
    ************************************************/ 
      strcpy( sender_ip, inet_ntoa(skt_snd.sin_addr) );

      if( !ip_in_same_netmask(sender_ip, SenderIpAdr, SenderNetMask)) 
      {
         logit( "et", "%s(%s): Connection after %ld sec refused from IP address %s\n",
                Argv0, MyModName, time(&now)-acceptstart, sender_ip );
         logit( "et", "from IP %s, does not match desired SenderIP in .d file %s and mask %s\n",
                sender_ip, SenderIpAdr, SenderNetMask);
         closesocket_ew( ActiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW );
         acceptstart = now;  /* reset the accept timer */
         sleep_ew( 1000 );   /* this sleep is important to keep us from spinning */
         continue;           /* too fast if a rogue repeatedly tries to connect  */
      }

      if( SockStatus == SOCK_NOT_CONN )
      {  /* we cried before, so now we have to let them know we're OK */
         sprintf( errText, "Connection accepted after %ld sec from IP address %s\n", 
                  (long)(time(&now)-acceptstart), sender_ip);
         output_status( TypeError, ERR_CONN, errText );
      }
      else
      {
         logit( "et", "%s(%s): Connection accepted after %ld sec from IP address %s\n",
                Argv0, MyModName, time(&now)-acceptstart, sender_ip );
      }
      SockStatus = SOCK_CONNECTED;

   /* Start the heartbeat thread
    ****************************/
      HeartThreadStatus  = 0;         /* set HeartThread status flag to OK            */
      MyLastInternalBeat = 0;         /* initialize our last EW heartbeat time        */
      MyLastSocketBeat   = 0;         /* initialize time of our heartbeat over socket */
      time((time_t*)&LastSenderBeat); /* initialize time of last heartbeat from       */
                                      /*    sending machine                           */
      if( StartThread( Heartbeat, (unsigned)THEAD_STACK_SIZE, &TidHeart ) == -1 )
      {
         logit( "e", "%s(%s): Error starting Heartbeat thread; exiting.\n",
                Argv0, MyModName );
         closesocket_ew( PassiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW );
         closesocket_ew( ActiveSocket,  SOCKET_CLOSE_IMMEDIATELY_EW );
         tport_detach( &Region );
         free( MsgBuf );
         return( -1 );
      }

   /* Start the message receiver thread
    ***********************************/
      MessageReceiverStatus = 0;        /* set MsgRcvThread status flag to OK */
      if( StartThread( MessageReceiver, (unsigned)THEAD_STACK_SIZE, &TidMsgRcv ) == -1 )
      {
         logit( "e", "%s(%s): Error starting MessageReceiver thread. Exiting.\n",
                Argv0, MyModName );
         (void)KillThread( TidHeart );
         closesocket_ew( PassiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW );
         closesocket_ew( ActiveSocket,  SOCKET_CLOSE_IMMEDIATELY_EW );
         tport_detach( &Region );
         free( MsgBuf );
         return( -1 );
      }

   /* Working loop with active socket connection:
    * Check on sender heartbeat status, check on receive thread health.
    * Check for shutdown requests. If things go wrong, kill all threads and restart.
    ********************************************************************************/
      while( 1 )
      {
         sleep_ew( 1000 );  /* sleep one second. The receive thread is awake */
         time( &now );

      /* How's the server's heart?
       ***************************/
         if( difftime(now,LastSenderBeat) > (double)SenderHeartRate  && 
             SenderHeartRate != 0 )
         {
            output_status(TypeError, ERR_SENDER_DEAD, "Sender heartbeat lost. Restarting");
            quit = 1;  /*restart*/
         }

      /* How's the receive thread feeling?
       ***********************************/
         if( MessageReceiverStatus == -1 )
         {
            logit( "et", "%s(%s): Receiver thread unhappy. Restarting\n",
                   Argv0, MyModName );
            quit = 1;
         }

      /* How's the heartbeat thread feeling? 
       *************************************/
         if( HeartThreadStatus == -1 )
         {
            logit( "et", "%s(%s): Heartbeat thread unhappy. Restarting\n",
                   Argv0, MyModName );
            quit = 1;
         }

      /* Are we being told to terminate?
       *********************************/
         if( tport_getflag( &Region ) == TERMINATE ||
             tport_getflag( &Region ) == MyPid       )
         {
         shutdown:
            tport_detach( &Region );
            logit( "t", "%s(%s): terminating on request\n", Argv0, MyModName );
            closesocket_ew( PassiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW );
            closesocket_ew( ActiveSocket,  SOCKET_CLOSE_IMMEDIATELY_EW );
            (void)KillThread( TidMsgRcv );
            (void)KillThread( TidHeart  );
            free( MsgBuf );
            return( 0 );
         }

      /* Restart preparations
       **********************/
         if( quit == 1 )
         {
            (void)KillThread( TidMsgRcv );
            (void)KillThread( TidHeart  );
            closesocket_ew( ActiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW );
            time( &acceptstart );
            quit = 0;
            break;
         }

      } /* end active socket loop: while(1) */

   } /* end main loop: while(quit==0) */
   
   return( 0 );

} /* int main( int argc, char **argv ) */


/*********************** Message Receiver Thread **********************
 *   Listen for sender messages and heartbeats; set global variable   *
 *   showing time of last heartbeat which Main thread monitors.       *
 **********************************************************************/
thr_ret MessageReceiver( void *dummy )
{
   static int    state;
   static char   chr;
   static char   lastChr;
   static int    nr;
   static long   nchar;                   /* counter for msg buffer (esc's removed) */
   static char   startCharacter=STX;      /* ASCII STX characer                     */
   static char   endCharacter=ETX;        /* ASCII ETX character */
   static char   escape=ESC;              /* our escape character */
   static char   inBuffer[INBUFFERSIZE];
   static long   inchar;                  /* counter for raw socket buffer (w/esc) */
   char          errtxt[128];             /* array for error text */

/* Tell the main thread we're ok
 *******************************/
   MessageReceiverStatus = 0;

   state = SEARCHING_FOR_MESSAGE_START; /* we're initializing */

/* Start of New code Section DK 11/20/97 Multi-byte Read 
 * Set inchar to be nr-1, so that when inchar is incremented, they will be 
 * the same and a new read from socket will take place.  
 * Set chr to 0 to indicate a null character, since we haven't read any yet.
 * Set nchar to 0 to begin building a new MsgBuffer (with escapes removed).
 */
   inchar = -1;
   nr     =  0;
   chr    =  0;
   nchar  =  0; 

/* Working loop: receive and process messages
 * We are either (0) initializing: searching for message start
 *               (1) expecting a message start: error if not
 *               (2) assembling a message.
 * The variable "state' determines our mood 
 **************************************************************/

   while( 1 )  /* loop over bytes read from socket */
   {
   /* Get next char operation 
    *************************/
      if( ++inchar == nr )
      {
      /* Read from socket operation */
         nr = recv_ew( ActiveSocket, inBuffer, INBUFFERSIZE-1, 0, 
                       SocketTimeoutLength );
         if( nr<=0 ) /* Connection Closed */
         {
            sprintf( errtxt, "%s(%s): recv_ew", Argv0, MyModName );
            SocketPerror( errtxt );
            logit( "t", "%s(%s): Bad socket read: %d\n", Argv0, MyModName, nr );
            MessageReceiverStatus = -1;  /* file a complaint to the main thread */
            KillSelfThread();            /* the main thread will resurrect us   */
         }
         inchar=0;
      }

      lastChr = chr;
      chr = inBuffer[inchar];

   /* Initialization
    ****************/
      if( state==SEARCHING_FOR_MESSAGE_START ) /* throw all away until we see a naked start character */
      {
      /* Do we have a real start character? */
         if( lastChr!=escape && chr==startCharacter )
         {
            state=ASSEMBLING_MESSAGE;  /* from now on, assemble message */
            continue;
         }
      }

   /* Confirm message start 
    ***********************/
      if( state==EXPECTING_MESSAGE_START )  /* the next char better be a start character - naked, that is */
      {
      /* Is it a naked start character? */
         if( chr==startCharacter &&  lastChr != escape ) /* This is it: message start!! */
         {
            nchar = 0;                                   /* start with first char position */
            state = ASSEMBLING_MESSAGE;                  /* go into assembly mode */
            continue;
         }
         else  /* we're eating garbage */
         {
            logit( "et", "%s(%s): Unexpected character from client. Re-synching\n", 
                   Argv0, MyModName);
            state = SEARCHING_FOR_MESSAGE_START;  /* search for next start sequence */
            continue;
         }
      }

   /* In the throes of assembling a message
    ***************************************/
      if( state==ASSEMBLING_MESSAGE )
      {
         if( chr==endCharacter )   /* naked end character: end of the message is at hand */
         {
         /* We have a complete message
          ****************************/
            MsgBuf[nchar]=0;      /* terminate as a string */

            if( strcmp(&MsgBuf[9],SenderHeartText)==0 ) /* got sender's heartbeat */
            {
               if( HeartbeatDebug )
               {
                  logit( "et", "%s(%s): Received heartbeat\n", Argv0, MyModName ); 
               }
               time( (time_t*)&LastSenderBeat ); /* note time of heartbeat */
               state = EXPECTING_MESSAGE_START;  /* reset for next message */
               MsgBuf[0]=' '; MsgBuf[9]=' ';
            }
            else /* got a non-heartbeat message */
            {
               if( HeartbeatDebug )
               {
                  logit( "et", "%s(%s): Received non-heartbeat\n", Argv0, MyModName ); 
               }
               time( (time_t*)&LastSenderBeat ); /* note time of heartbeat */
            /* This is not a genuine heartbeat, but our major concern is that the
             * export module on the other end is still alive, and any message that
             * we receive should indicate life on the other side (in a non-occult 
             * kind of way).
             */
               import_filter( MsgBuf, nchar ); /* process the message via user-routine */
            }
            state = EXPECTING_MESSAGE_START;
            continue;
         }

         else /* process the byte we just read: we know it's not a naked end character */
         {
         /* Escape sequence? 
          ******************/
            if( chr==escape )      
            { 
               if( ++inchar == nr ) /* Get next char operation */
               {
               /* Read from socket operation */
                  nr = recv_ew( ActiveSocket, inBuffer, INBUFFERSIZE-1, 0,
                                SocketTimeoutLength );
                  if( nr<=0 ) /* Connection Closed */
                  {
                     sprintf( errtxt, "import_gen_pasv(%s): recv_ew", MyModName );
                     SocketPerror( errtxt );

                     logit( "t", "%s(%s): Bad socket read: %d\n", Argv0, MyModName, nr );
                     MessageReceiverStatus = -1;  /* file a complaint to the main thread */
                     KillSelfThread();            /* the main thread will resurrect us */
                  }
                  inchar = 0;
               }
               lastChr = chr;
               chr     = inBuffer[inchar];

               if( chr != startCharacter && 
                   chr != endCharacter   &&
                   chr != escape            )  /* bad news: unknown escape sequence */
               {   
                  logit( "et", "%s(%s): Unknown escape sequence in message. Re-synching\n",
                         Argv0, MyModName);
                  state = SEARCHING_FOR_MESSAGE_START; /* search for next start sequence */
                  continue;
               }
               else  /* it's ok: it's a legit escape sequence, and we save the escaped byte */
               {
                  MsgBuf[nchar++] = chr; 

                  if( nchar>MaxMsgSize ) 
                  {
                     logit( "et", "%s(%s): receive buffer overflow after %ld bytes\n",
                            Argv0, MyModName, MaxMsgSize );
                     state = SEARCHING_FOR_MESSAGE_START; /* initialize again */
                     nchar = 0;
                     continue;
                  }
                  continue;
               }
            } /* end if( chr==escape ) */

         /* Naked start character? 
          ************************/
            if( chr==startCharacter )               /* bad news: unescaped start character */
            {  
               logit( "et", "%s(%s): Unescaped start character in message. Re-synching\n", 
                       Argv0, MyModName );
               state = SEARCHING_FOR_MESSAGE_START; /* search for next start sequence */
               continue;
            }

         /* So it's not a naked start, escape, or a naked end: Hey, it's just a normal byte 
          *********************************************************************************/
            MsgBuf[nchar++] = chr; 

            if( nchar>MaxMsgSize )
            {
               logit( "et", "%s(%s): receive buffer overflow after %ld bytes\n",
                      Argv0, MyModName, MaxMsgSize);
               state = SEARCHING_FOR_MESSAGE_START;  /* initialize again */
               nchar = 0;
               continue;
            }

         } /* end of else not-an-endCharacter processing */
      } /* end of state==ASSEMBLING_MESSAGE processing */
   }  /* end of loop over characters */
}  /* end of MessageReceiver thread */


/***************************** Heartbeat **************************
 *           Send a heartbeat to the transport ring buffer        *
 *           Send a heartbeat to the server via socket            *
 *                 Check on our server's hearbeat                 *
 *           Slam socket shut if no Server heartbeat: that        *
 *            really shakes up the main thread                    *
 ******************************************************************/
thr_ret Heartbeat( void *dummy )
{
   MSG_LOGO  reclogo;
   time_t    now;
   char      errtxt[256];    /* array for error text */

/* Initialize heartbeat logo */
   reclogo.instid = MyInstId;
   reclogo.mod    = MyModId;
   reclogo.type   = TypeHeartBeat;

/* Once a second, do the rounds. If anything looks bad, set HeartThreadStatus to -1
 * and go into a long sleep. The main thread should note that our status is -1,
 * and launch into re-start procedure, which includes killing and restarting us. 
 */

   while( 1 )
   {
      time( &now );

    /* Beat our heart (into the local Earthworm) if it's time
     ********************************************************/
      if( difftime(now,MyLastInternalBeat) > (double)HeartBeatInt )
      {
          output_status( TypeHeartBeat, 0, "" );
          MyLastInternalBeat = now;
      }

   /* Beat our heart (over the socket) to our sender
    ************************************************/
      if( difftime(now,MyLastSocketBeat) > (double)MyAliveInt && MyAliveInt != 0 )
      {
         if( WriteToSocket( ActiveSocket, MyAliveString, &reclogo ) != 0 )
         {
         /* If we get an error, simply quit */
            sprintf( errtxt, "import_gen_pasv(%s): error sending alive msg to socket;"
                             "  Heartbeat thread quitting!\n", MyModName );
            output_status( TypeError, ERR_SOCK_READ, errtxt );
            HeartThreadStatus = -1;
            KillSelfThread();  /* the main thread will resurrect us */
         }
         if( HeartbeatDebug )
         {
            logit("et","%s(%s): Heartbeat sent to export\n", Argv0, MyModName);
         }
         MyLastSocketBeat = now;
      }
      sleep_ew( 1000 );
   }
}


/*****************************************************************************
 *  config() processes command file(s) using kom.c functions;                *
 *                    exits if any errors are encountered.                   *
 *****************************************************************************/
void config( char *configfile )
{
   int      ncommand;      /* # of required commands you expect to process   */
   char     init[20];      /* init flags, one byte for each required command */
   int      nmiss;         /* number of required commands that were missed   */
   char     *com;
   char     *str;
   int      nfiles;
   int      success;
   int      i;


   SenderNetMask[0]='\0';
   strcpy(SenderNetMask, PERFECT_MATCH_NETMASK); /* the default as before, match IP exactly */

   /* Set to zero one init flag for each required command
   *****************************************************/
   ncommand = 12;
   for( i=0; i<ncommand; i++ )  init[i] = 0;

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
   while( nfiles > 0 )   /* While there are command files open */
   {
        while( k_rd() )        /* Read next line from active file  */
        {
            com = k_str();         /* Get the first token from line */

            /* Ignore blank lines & comments
            *******************************/
            if( !com )           continue;
            if( com[0] == '#' )  continue;

            /* Open a nested configuration file
            **********************************/
            if( com[0] == '@' ) 
            {
               success = nfiles+1;
               nfiles  = k_open(&com[1]);
               if( nfiles != success ) 
               {
                  logit( "e",
                         "%s: Error opening command file <%s>; exiting!\n",
                         Argv0, &com[1] );
                  exit( -1 );
               }
               continue;
            }

            /* Process anything else as a command
            ************************************/
  /*0*/     if( k_its("MyModuleId") ) 
            {
                str=k_str();
                if( str ) strcpy( MyModName, str );
                init[0] = 1;
            }
  /*1*/     else if( k_its("RingName") ) 
            {
                str = k_str();
                if( str ) strcpy( RingName, str );
                init[1] = 1;
            }
  /*2*/     else if( k_its("HeartBeatInt") ) 
            {
                HeartBeatInt = k_int();
                init[2] = 1;
            }
  /*3*/     else if( k_its("LogFile") ) 
            {
                LogSwitch=k_int();
                init[3]=1;
            }

            /* Maximum size (bytes) for incoming messages
            ********************************************/
  /*4*/     else if( k_its("MaxMsgSize") ) 
            {
                MaxMsgSize = k_long();
                init[4] = 1;
            }

            /* Interval for alive messages to sending machine
            ************************************************/
 /*5*/      else if( k_its("MyAliveInt") ) 
            {
                MyAliveInt = k_int();
                init[5]=1;
            }

            /* Text of alive message to sending machine
            ******************************************/
 /*6*/      else if( k_its("MyAliveString") )
            {
                str=k_str();
                if( str ) strcpy( MyAliveString, str );
                init[6]=1;
            }

            /* Receiver's internet address, in dot notation
            ***********************************************/
 /*7*/      else if( k_its("ReceiverIpAdr") ) 
            {
                str=k_str();
                if (strlen(str) > MAX_IP_STR) {
                  logit( "e",
                         "%s: Error ReceiverIPAdr string too long for legal IP address <%s>; exiting! Maximum %d chars\n",
                         Argv0, str , MAX_IP_STR-1);
                  exit( -1 );
                }
                if( str ) strcpy( ReceiverIpAdr, str );
                init[7]=1;
            }

 /*8*/      /* Receiver's Port Number
            *************************/
            else if( k_its("ReceiverPort") ) 
            {
                ReceiverPort = k_int();
                init[8]=1;
            }

            /* Sender's internet address, in dot notation
            ********************************************/
 /*9*/      else if( k_its("SenderIpAdr") ) 
            {
                str=k_str();
                if (strlen(str) > MAX_IP_STR) {
                  logit( "e",
                         "%s: Error SenderIPAdr string too long for legal IP address <%s>; exiting! Maximum %d chars\n",
                         Argv0, str , MAX_IP_STR-1);
                  exit( -1 );
                }
                if( str ) strcpy( SenderIpAdr, str );
                init[9]=1;
            }
 /*optional*/ else if( k_its("SenderNetMask") ) 
            {
                str=k_str();
                if (strlen(str) > MAX_IP_STR) {
                  logit( "e",
                         "%s: Error SenderNetMask string too long for legal netmask <%s>; exiting! Maximum %d chars\n",
                         Argv0, str , MAX_IP_STR-1);
                  exit( -1 );
                }
                if( str ) strcpy( SenderNetMask, str );
            }

            /* Sender's Heart beat interval
            ******************************/
 /*10*/     else if( k_its("SenderHeartRate") ) 
            {
                SenderHeartRate = k_int();
                init[10]=1;
                SocketTimeoutLength = SenderHeartRate * 1000;
                /* Convert heartrate from sec to msec,
                 * and then base the sockettimeoutlength on
                 * the heartrate 
                 */ 
            }

            /* Sender's heart beat text
            **************************/
 /*11*/     else if( k_its("SenderHeartText") ) 
            {
                str=k_str();
                if( str ) strcpy( SenderHeartText, str );
                init[11]=1;
            }

            /* Optional:  Socket timeout length 
            **********************************/
            else if( k_its("SocketTimeout") ) 
            {
               if (SenderHeartRate == 0)
               {
                  SocketTimeoutLength = k_int();
               }
            }

            /* Optional Command:  Heartbeat Debug Flag
            *****************************************/
            else if( k_its("HeartbeatDebug") ) 
            {
                HeartbeatDebug = k_int();
            }

            /* Optional Command:  Socket Debug Flag
            **************************************/
            else if( k_its("SocketDebug") ) 
            {
                SOCKET_ewDebug = k_int();
            }

            /* Unknown command
            *****************/
            else 
            {
                logit( "e", "%s: <%s> Unknown command in <%s>.\n",
                       Argv0, com, configfile );
                continue;
            }

            /* See if there were any errors processing the command
            *****************************************************/
            if( k_err() ) 
            {
               logit( "e",
                      "%s: Bad <%s> command in <%s>; exiting!\n",
                      Argv0, com, configfile );
               exit( -1 );
            }
        }
        nfiles = k_close();
   }

   /* After all files are closed, check init flags for missed commands
   ******************************************************************/
   nmiss = 0;
   for( i=0; i<ncommand; i++ ) if( !init[i] ) nmiss++;
   if( nmiss ) 
   {
       logit("e", "%s: ERROR, no ", Argv0 );
       if( !init[0] )   logit("e", "<MyModuleId> "      );
       if( !init[1] )   logit("e", "<RingName> "        );
       if( !init[2] )   logit("e", "<HeartBeatInt> "    );
       if( !init[3] )   logit("e", "<LogFile> "         );
       if( !init[4] )   logit("e", "<MaxMsgSize> "      );
       if( !init[5] )   logit("e", "<MyAliveInt> "      );
       if( !init[6] )   logit("e", "<MyAliveString> "   );
       if( !init[7] )   logit("e", "<ReceiverIpAdr> "   );
       if( !init[8] )   logit("e", "<ReceiverPort> "    );
       if( !init[9] )   logit("e", "<SenderIpAdr> "     );
       if( !init[10] )  logit("e", "<SenderHeartRate> " );
       if( !init[11] )  logit("e", "<SenderHeartText> " );
       logit("e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

   return;
}

/****************************************************************************
 *  lookup( )   Look up important info from earthworm.h tables              *
 ****************************************************************************/
void lookup( void )
{

   /* Look up keys to shared memory regions
   ***************************************/
   if( ( RingKey = GetKey( RingName ) ) == -1 ) 
   {
        logit( "e",
               "%s: Invalid ring name <%s>; exiting!\n", Argv0, RingName);
        exit( -1 );
   }

   /* Look up installations of interest
   ***********************************/
   if( GetLocalInst( &MyInstId ) != 0 ) 
   {
      logit( "e",
             "%s: error getting local installation id; exiting!\n", Argv0 );
      exit( -1 );
   }

   /* Look up modules of interest
   *****************************/
   if( GetModId( MyModName, &MyModId ) != 0 ) 
   {
      logit( "e",
             "%s: Invalid module name <%s>; exiting!\n", Argv0, MyModName );
      exit( -1 );
   }

   /* Look up message types of interest
   ***********************************/
   if( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) 
   {
      logit( "e",
             "%s: Invalid message type <TYPE_HEARTBEAT>; exiting!\n", Argv0 );
      exit( -1 );
   }
   if( GetType( "TYPE_ERROR", &TypeError ) != 0 ) 
   {
      logit( "e",
             "%s: Invalid message type <TYPE_ERROR>; exiting!\n", Argv0 );
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
   MSG_LOGO logo;
   char     msg[256];
   long     size;
   time_t   t;

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
      logit( "et", "%s(%s): %s\n", Argv0, MyModName, note );
   }

   size = (int)strlen( msg );   /* don't include the null byte in the message */

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &Region, &logo, size, msg ) != PUT_OK )
   {
      if( type == TypeHeartBeat ) 
      {
         logit( "et", "%s(%s):  Error sending heartbeat.\n", Argv0, MyModName );
      }
      else if( type == TypeError ) 
      {
         logit( "et", "%s(%s):  Error sending error:%d.\n", Argv0, MyModName, ierr );
      }
   }

   return;

} /* void output_status( unsigned char type, short ierr, char *note ) */


/******************************* import_filter ***********************************
 *                    Decide  what to do with this message                       *
 *                                                                               *
 *      This routine is handed each incoming message, and can do what it likes   *
 * with it. The intent is that here is where installation-specificd processing   *
 * is done, such as deciding whether we want this message (e.g. is the pick from *
 * an intersting station?), changing its format, etc.                            *
 *         The default action is to assume that the incoming msg was generated   *
 * by the Earthworm standard "export_gen_pasv" module, which has a companion     *
 * "export-filter" routine. The default form of that routine attaches the logo   *
 * at the start of the message and exports it. So we peel off the logo, and drop * 
 * the message into our local transport medium under that logo.                  *
 *         We assume that the first 9 chars are 3 three-character fields         *
 * giving IstallationId, ModuleId, and MessageType.                              *
 *********************************************************************************/

void import_filter( char *msg, int msgLen )
{
   char cInst[4], cMod[4], cType[4];
   MSG_LOGO logo;

/* We assume that this message was created by our bretheren "export_pasv",
 * which attaches the logo as three groups of three characters at the front
 * of the message. 
 */

/* Peel off the logo characters 
 ******************************/
   strncpy(cInst,msg,    3);  cInst[3]=0;  logo.instid =(unsigned char)atoi(cInst);
   strncpy(cMod ,&msg[3],3);  cMod[3] =0;  logo.mod    =(unsigned char)atoi(cMod);
   strncpy(cType,&msg[6],3);  cType[3]=0;  logo.type   =(unsigned char)atoi(cType);

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &Region, &logo, (long)(msgLen-9), &msg[9]) != PUT_OK )
   {
      logit( "t", "%s(%s):  Error sending message via transport:\n",
             Argv0, MyModName );
      return;
   }
    
   return;

}  /* void import_filter( char *msg, int msgLen ) */


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
   sprintf( asciilogo, "%c%3d%3d%3d",startmsg, (int) logo->instid, 
            (int) logo->mod, (int) logo->type );
   rc = send_ew( ActiveSocket, asciilogo, 10, 0, SocketTimeoutLength );
   if( rc != 10 ) 
   {
      sprintf( errtxt, "import_gen_pasv(%s): send_ew", MyModName );
      SocketPerror( errtxt );
      return( -1 );
   }

/* Send message; break it into chunks if it's big!
 *************************************************/
   rc = send_ew( ActiveSocket, msg, msglength, 0, SocketTimeoutLength );
   if( rc == -1 )
   {
      sprintf( errtxt, "import_gen_pasv(%s): send_ew", MyModName );
      SocketPerror( errtxt );
      return( -1 );
   }

/* Send "end of transmission" flag
 *********************************/
   rc = send_ew( ActiveSocket, &endmsg, 1, 0, SocketTimeoutLength );
   if( rc != 1 ) 
   {
      sprintf( errtxt, "import_gen_pasv(%s): send_ew", MyModName );
      SocketPerror( errtxt );
      return( -1 );
   }

   return( 0 );

} /* int WriteToSocket( SOCKET ActiveSocket, char *msg, MSG_LOGO *logo ) */

/*--------------------------------End Of File-----------------------------------*/
