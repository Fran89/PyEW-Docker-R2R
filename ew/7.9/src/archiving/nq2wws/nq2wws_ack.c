/*
*   nq2wws.c
*
*   Program to read miniSEED files from NetQuakes and to export them over a socket
*   to a Winston Waveserver.
*
*                         Jim Luetgert 09/10/09
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
#include "trace_buf.h"
#include <imp_exp_gen.h>
#include <mem_circ_queue.h>
#include <priority_level.h>   /* for EW_PRIORITY definitions only */
#include <socket_ew.h>

#include "nq2wws.h"

int ShutMeDown;		/* shut down flag */

char   progname[] = "nq2wws";
char   NetworkName[TRACE_NET_LEN];     /* Network name. Constant from trace_buf.h   */
char   NQFilesInDir[FILE_NAM_LEN];     /* directory from whence cometh the          */
                                       /* miniSEED input files */
char   NQFilesOutDir[FILE_NAM_LEN];    /* directory to which we put processed files */
char   NQFilesErrorDir[FILE_NAM_LEN];  /* directory to which we put problem files   */


/* CONSTANTS */
#define TIME_SLOP 3 /* seconds */

/* Functions in this source file
*******************************/
void  export_config  ( char * );
void  export_lookup  ( void );
void  export_status  ( unsigned char, short, char * );
void  export_free    ( void );
int   WriteToSocket  ( int, char *, long, MSG_LOGO * );
int   binEscape      (char*, long , char* , long* , long );
void  export_shutdown();

int mseed_to_tbuf(int *bytes, DATA_HDR *mseed_hdr, int out_data_size); 
int queue_putmsg( long recsize, char* trace_buffer);

/* Thread things
***************/
#define THREAD_STACK 8192
static unsigned TidSender;           /* SocketSender thread id */
static unsigned TidListener;         /* Thread listening for client heartbeats */
static unsigned TidStacker;          /* Thread moving messages from transport */
									 /*   to queue */

#define THREAD_OFF    0              /* thread has not been started      */
#define THREAD_ALIVE  1              /* thread alive and well            */
#define THREAD_ERR   -1              /* thread encountered error quit    */

/* Thread flags
***************/
/* The xxStatus variables are for filing complaints */
volatile int MessageStackerStatus =	THREAD_OFF;
volatile int SocketSenderStatus=	THREAD_OFF;
volatile int SocketListenerStatus=	THREAD_OFF;
/* The xxAlive variables are for proving vitality */
volatile int MessageStackerAlive =	THREAD_ALIVE;
volatile int SocketSenderAlive =	THREAD_ALIVE;
volatile int SocketListenerAlive=	THREAD_ALIVE;

volatile int LiveConnection=0;       /* 0= not connected   1=connected to client */

QUEUE OutQueue; 		     /* from queue.h, queue.c; sets up linked    */
                                     /*    list via malloc and free              */
thr_ret SocketSender( void * );
thr_ret SocketListener( void * );
thr_ret MessageStacker( void * );    /* used to pass messages between main thread */
                                     /*   and SocketSender thread */

/* Message Buffers to be allocated
*********************************/
static char *Rawmsg = NULL;          /* "raw" retrieved msg for main thread      */
static char *SSmsg = NULL;           /* SocketSender's incoming message buffer   */
static char *SSbinmsg = NULL;        /* SocketSender's binary-escaped msg buffer */
static long  BinSize=0;              /* amount of stuff in SSbinmsg. For re-sending */
static MSG_LOGO BinLogo;             /* logo of message to re-send */
static char *SLmsg = NULL;           /* SocketListener's incoming message buffer */
static char *MSrawmsg = NULL;        /* MessageStacker's "raw" retrieved message */

/* Heart beat stuff
*******************/
time_t LastRcvAlive=0;     /* Last time we heard from our client */
time_t LastSendAlive=0;    /* Last time we sent a heartbeat to our client */
time_t now;		   /* current time, used for timing heartbeats */
time_t MyLastBeat;         /* time of last local (into Earthworm) hearbeat */
int    ImportAlive =1;     /* switch for noting if our import is sending heart beats ok */

extern int  errno;

static  SHM_INFO  Region;       /* shared memory region to use for i/o    */

#define   MAXLOGO   10
MSG_LOGO  GetLogo[MAXLOGO];     /* array for requesting module,type,instid */
short 	  nLogo;

char *Argv0;            /* pointer to executable name */
pid_t MyPid;		/* Our own pid, sent with heartbeat for restart purposes */

/* Global socket things
**********************/
int    PassiveSocket = 0;       /* Socket descriptor; passive socket     */
int    ActiveSocket;	        /* Socket descriptor; active socket      */

/* Things to read or derive from configuration file
**************************************************/
static char    RingName[MAX_RING_STR]; /* name of transport ring for i/o    */
static char    MyModName[MAX_MOD_STR]; /* speak as this module name/id      */
static int     LogSwitch;              /* 0 if no logfile should be written */
static int     HeartBeatInt;           /* seconds between heartbeats        */
static char    ServerIPAdr[16];        /* server's IP address               */
static int     ServerPort;             /* Server's well-known port number   */
static long    MaxMsgSize;             /* max size for input/output msgs    */
static int     RingSize;	       /* max messages in output circular buffer */
static int     SendAliveInt;           /* Send alive messages this often    */
static char    SendAliveText[MAX_ALIVE_STR]; /* Text of alive message. Max size is traditional */
static int     RcvAliveInt;            /* Expect client heartbeats this often */
static char    RcvAliveText[MAX_ALIVE_STR];  /* Text of client's alive messages */
static int     Verbose=0;              /* changed to 1 by Verbose command       */

int Debug;
int err_exit;				/* an nq2ring_die() error number */
int WWSMS;               /* # of msecs to pad tracebuf transmissions */
int nchans;
DATABUF chanbuf[MAXCHANS];
double    endtime;

static int     SocketTimeoutLength=0;  /* Length of timeouts on SOCKET_ew calls */
/*  Socket timeouts are not handled(well) in export, so there is not a whole lot
of point to setting SocketTimeoutLength to any useful value.  Currently
it is set to atleast the RcvAliveInt + a few seconds, so the main thread
always squashes the socket code after a timeout.  DavidK 2001/04/16 */

static int     SOCKET_ewDebug=0;       /* Set to 1 for SOCKET_ew debug statements*/

/* Things to look up in the earthworm.h tables with getutil.c functions
**********************************************************************/
static long          RingKey;       /* key of transport ring for i/o      */
static unsigned char InstId;        /* local installation id              */
static unsigned char MyModId;       /* Module Id for this program         */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypeTrace2;

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

/* Socket status values
***********************/
#define SOCK_NEW 0             /* New socket net yet connected           */
#define SOCK_NOT_CONN 1        /* No connection after timeout            */
#define SOCK_CONNECTED 2
int SockStatus;

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
	long          recsize;	/* size of retrieved message             */
	MSG_LOGO      reclogo;	/* logo of retrieved message             */
	int           AcceptTimeout; /* time in milliseconds for accept_ew()  */
	int           sockErr;
   char         *ptr;           /* temp pointer for setting up SendQueue */
   int           i;

	/* The timers and time limits for assertaining thread life */
	time_t messageStackerTimer;
	time_t socketSenderTimer;
	time_t socketListenerTimer;
	time_t messageStackerTimeout;
	time_t socketSenderTimeout;
	time_t socketListenerTimeout;

	/* Catch broken socket signals
	******************************/
#ifdef _SOLARIS
	(void)sigignore(SIGPIPE);
#endif

	/* init some globals */
	Verbose = FALSE;	/* not well used */
	ShutMeDown = FALSE;

	/* Check command line arguments
	******************************/
	Argv0 = argv[0];
	if ( argc != 2 ) {
		fprintf( stderr, "Usage: %s <configfile>\n", Argv0 );
		return( 0 );
	}
	/* Initialize name of log-file & open it 
	****************************************/
	logit_init( argv[1], 0, 512, 1 );

	/* init some locals */
	err_exit = -1;

	/* Read the configuration file(s) & lookup info from earthworm.h
	****************************************************************/
	export_config( argv[1] );
	export_lookup();
	logit( "et" , "%s(%s): Read command file <%s>\n",
		Argv0, MyModName, argv[1] );

	/* Reinitialize the logging level
	*********************************/
	logit_init( argv[1], 0, 512, LogSwitch );

	/* Set SOCKET_ew debugging on/off
	*********************************/
	setSocket_ewDebug(SOCKET_ewDebug);

	/* introduce ourselves
	**********************/
   logit("","24 Feb 2010 version: "
            "I do re-sends, I expect ACKs, I check thread life.\n");

	/* Heartbeat parameters sanity checks
	************************************/
	/* Do a sanity chcek on the SocketTimeoutLength.
	If it is -1(no timeout; block on the call) then
	leave it alone. */
	if(SocketTimeoutLength != -1 &&
		1000 * (RcvAliveInt + TIME_SLOP) >= SocketTimeoutLength) {
		/* WARNING:  This code insures that the export's socket thread will never
		survive a socket timeout.  By setting them to greater than the
		RcvAliveInt, the main thread will clobber the other threads and
		perform a restart, any time that the socket actually hits a timeout.
		This was done, because there is currently no code in export.c to handle
		socket timeouts, (and no real reason to.)
		DavidK 04/16/2001
		**********************************************************************/
		logit("","Socket timeout (%d ms) is less than incoming heartrate and buffertime(%d sec)",
			SocketTimeoutLength, RcvAliveInt + 3);
		SocketTimeoutLength = 1000 * (RcvAliveInt + TIME_SLOP);
		logit("", "Setting socket timeout to %d ms\n", SocketTimeoutLength);
	}

	/* Get our own Pid for restart purposes
	***************************************/
	MyPid = getpid();
	if(MyPid == -1) {
		logit("e", "%s(%s): Cannot get pid; exiting!\n", Argv0, MyModName);
		return(0);
	}

	/* Attach to Input/Output shared memory ring
	********************************************/
	tport_attach( &Region, RingKey );

	/* One heartbeat to announce ourselves to statmgr
	************************************************/
	export_status( TypeHeartBeat, 0, "" );
	time(&MyLastBeat);

	/* Allocate space for input/output messages for all threads
	***********************************************************/
	/* Buffer for main thread: */
	if ( ( Rawmsg = (char *) malloc(MaxMsgSize) ) ==  NULL ) {
		logit( "e", "%s(%s): error allocating Rawmsg; exiting!\n",
			Argv0, MyModName );
		export_free();
		return( -1 );
	}

	/* Buffers for SocketSender thread: */
	if ( ( SSmsg = (char *) malloc(MaxMsgSize) ) ==  NULL ) {
		logit( "e", "%s(%s): error allocating SSmsg; exiting!\n",
			Argv0, MyModName );
		export_free();
		return( -1 );
	}
	if ( ( SSbinmsg = (char *) malloc(MaxMsgSize*2) ) ==  NULL ) {
		logit( "e",
			"%s(%s): error allocating SSbinmsg; exiting!\n",
			Argv0, MyModName );
		export_free();
		return( -1 );
	}
	BinSize=0; /*indicate empty buffer */

	/* Buffer for SocketListener thread: */
	if ( ( SLmsg = (char *) malloc(MaxMsgSize) ) ==  NULL ) {
		logit( "e", "%s(%s): error allocating SLmsg; exiting!\n",
			Argv0, MyModName );
		export_free();
		return( -1 );
	}

	/* Buffers for the MessageStacker thread: */
	if ( ( MSrawmsg = (char *) malloc(MaxMsgSize) ) ==  NULL ) {
		logit( "e", "%s(%s): error allocating MSrawmsg; exiting!\n",
			Argv0, MyModName );
		export_free();
		return( -1 );
	}

	/* Create a Mutex to control access to queue
	********************************************/
	CreateMutex_ew();

	/* Initialize the message queue
	*******************************/
	initqueue( &OutQueue, (unsigned long)RingSize,(unsigned long)MaxMsgSize );


			/************************/
					retry:
			/************************/
	/* Come here after communications with client have been broken,     */
	/* the socket has been closed, and the threads have been terminated. */

	/* Initialize the socket system
	*******************************/
	SocketSysInit();

	if ( ( PassiveSocket = socket_ew( PF_INET, SOCK_STREAM, 0) ) == -1 ) {
		logit( "et", "%s(%s): Error opening socket; exiting!\n", Argv0, MyModName );
		tport_detach( &Region );
		return( -1 );
	}
	SockStatus = SOCK_NEW;

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
		tport_detach( &Region );
		return( -1 );
	}

	/* Bind socket to a name
	************************/
	if ( bind_ew( PassiveSocket, (struct sockaddr *) &skt, sizeof(skt)) )
	{
		logit("et", "%s(%s): error binding socket; exiting.\n",
            Argv0, MyModName);
		perror("Export bind error");
		closesocket_ew(PassiveSocket,SOCKET_CLOSE_IMMEDIATELY_EW);
		tport_detach( &Region );
		return( -1 );
	}

	/* Prepare for connect requests
	*******************************/
	if ( listen_ew( PassiveSocket, 0))
	{
		logit("et", "%s(%s): socket listen error; exiting!\n",
			Argv0, MyModName );
		closesocket_ew( PassiveSocket,SOCKET_CLOSE_IMMEDIATELY_EW );
		tport_detach( &Region );
		return( 1 );
	}

	/* One heartbeat to announce ourselves to statmgr
	************************************************/
	export_status( TypeHeartBeat, 0, "" );
	time(&MyLastBeat);

	/* Accept a connection (blocking)
	*********************************/
	clientLen = sizeof( client );
	logit( "et", "%s(%s): Waiting for new connection.\n", Argv0, MyModName );
	AcceptTimeout = 1000 * HeartBeatInt;

	while ( (ActiveSocket = accept_ew( PassiveSocket,
		(struct sockaddr*) &client,
		&clientLen, AcceptTimeout) ) == INVALID_SOCKET )
	{
		sockErr = socketGetError_ew();
		if ( sockErr == WOULDBLOCK_EW)
		{
			if (SockStatus == SOCK_NEW)
			{
				SockStatus = SOCK_NOT_CONN;
				sprintf( errText, "No connection after %d seconds\n", HeartBeatInt );
				export_status( TypeError, ERR_NOCONN, errText );
			}
		}
		else
		{
			logit("et", "%s(%s): error on accept: %d %s\n\t; exiting!\n", Argv0,
			       MyModName, sockErr, strerror(sockErr) );
			closesocket_ew( ActiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW);
			tport_detach( &Region );
			return(-1);
		}
		/* Are we supposed to shut down? */
		if ( tport_getflag( &Region ) == TERMINATE ) goto shutdown;
		if ( tport_getflag( &Region ) == MyPid     ) goto shutdown;

		/* Beat our heart so statmgr knows we alive */
		export_status( TypeHeartBeat, 0, "" );
		time(&MyLastBeat);
	}

	strcpy( client_ip, inet_ntoa(client.sin_addr) );
	LiveConnection = 1; /*970623:ldd*/
	if (SockStatus == SOCK_NOT_CONN)
	{  /* we cried before, so now we have to let them know we're OK */
		sprintf( errText, "Connection accepted from IP address %s\n", client_ip);
		export_status( TypeError, ERR_CONN, errText );
	}
	else
		logit("et", "%s(%s): Connection accepted from IP address %s\n",
		Argv0, MyModName, client_ip );

	SockStatus = SOCK_CONNECTED;

	/* Start the message stacking thread if it isn't already running.
    ****************************************************************/
	if (MessageStackerStatus != THREAD_ALIVE )
	{
		if ( StartThread(  MessageStacker, (unsigned)THREAD_STACK, &TidStacker ) == -1 )
		{
			logit( "e",
				"%s(%s): Error starting  MessageStacker thread; exiting!\n",
				Argv0, MyModName );
			tport_detach( &Region );
			closesocket_ew( ActiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW);
			return( -1 );
		}
		MessageStackerStatus = THREAD_ALIVE;
		MessageStackerAlive = THREAD_ALIVE; /* assume the best */
		/* There are no inherent time-delays in the stacker */
		messageStackerTimeout = TIME_SLOP;
	}

	/* Start the socket writing thread
	***********************************/
	if ( StartThread(  SocketSender, (unsigned)THREAD_STACK, &TidSender ) == -1 )
	{
		logit( "e", "%s(%s): Error starting SocketSender thread; exiting!\n",
			Argv0, MyModName );
		tport_detach( &Region );
		closesocket_ew( ActiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW);
		return( -1 );
	}
	/* It will set its status, but set it here to prevent timing problems */
	SocketSenderStatus = THREAD_ALIVE;
	SocketSenderAlive = THREAD_ALIVE;
	/* Timeout is at worst sending an old message, a heartbeat, and a message.
	Each is at worst one socketTimeout */
	socketSenderTimeout = 3*(SocketTimeoutLength/1000) + TIME_SLOP;

	/* Start the socket listening thread
	*************************************/
	if ( StartThread(  SocketListener, (unsigned)THREAD_STACK, &TidListener ) == -1 )
	{
		logit( "e", "%s(%s): Error starting SocketListener thread; exiting.\n",
			Argv0, MyModName );
		tport_detach( &Region );
		closesocket_ew( ActiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW);
		return( -1 );
	}
	/* It will set its status, but set it here to prevent timing problems */
	SocketListenerStatus = THREAD_ALIVE;
	SocketListenerAlive = THREAD_ALIVE;
	/* At most one slurp from the socket */
	socketListenerTimeout = (SocketTimeoutLength/1000) + TIME_SLOP;

	/* set import heartbeat timer & flag
	************************************/
	time(&LastRcvAlive); /* from distant client */
	ImportAlive = 1;	 /* assume we'll be hearing from our import */

	/* Start main export service loop for current connection
	********************************************************/
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

		/* see how our socket threads are feeling
		****************************************/
		/* Each thread complains by setting its ...Status to -1, and exiting. */
		if( SocketSenderStatus   != THREAD_ALIVE  ||
			SocketListenerStatus != THREAD_ALIVE  ||
			ImportAlive          != 1 )
		{
			logit("et", "%s(%s) restarting. This procedure may hang. "
				"Make sure restartMe is set in my .desc file\n",
				Argv0, MyModName  );
			/* The code below occasionally hangs before the "waiting for new connection"
			statement. The solution is to set the "restartMe" option in the .desc
			file, which will cause me to be restarted by statmgr/startstop. */
			/* One cause of hanging is that KillThread() does nothing about mutexes held
			* by the threads being killed. If a thread holds a mutex when it is killed,
			* the mutex is held forever. It would be better to use thread cancellation
			* once earthworm is free from Solaris threads. PNL, 1/10/00 */
			closesocket_ew( ActiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW);
			closesocket_ew( PassiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW);
			LiveConnection = 0; /*970623:ldd*/
			/* Stop the socket threads, but leave the MessageStacker running. */
			(void)KillThread(TidListener);	 SocketListenerStatus=THREAD_OFF;
			(void)KillThread(TidSender);    SocketSenderStatus=THREAD_OFF;
			/* go & re-try from scratch */
			goto retry;
		}

		/* See if threads are really running
		************************************/
		/* The scheme is to knock down the thread's 'alive' flags
		and observe that the threads are healthy enough to raise them up again. */
		if (MessageStackerAlive == THREAD_ALIVE) { /* Hurray, it's running */
			/* start another test */
			MessageStackerAlive = THREAD_OFF; /* see if will be reset */
			time( &messageStackerTimer ); /* within the timelimit */
		}
		else {
			if (difftime( messageStackerTimer, time(NULL)) > messageStackerTimeout )
			{	/* Very weird and very bad. Exit */
				logit("et","%s(%s): Message Stacker Thread hung. Exiting\n",
					Argv0, MyModName);
				export_shutdown();
			}
		}
		if (SocketSenderAlive == THREAD_ALIVE) /* Hurray, it's running */
		{
			/* start another test */
			SocketSenderAlive = THREAD_OFF;
			time( &socketSenderTimer );
		}
		else {
			if (difftime( socketSenderTimer, time(NULL)) > socketSenderTimeout )
			{	/* Very weird and very bad. Exit */
				logit("et","%s(%s): Socket Sender Thread hung. Exiting\n",
					Argv0, MyModName);
				export_shutdown();
				return(-1);
			}
		}
		if (SocketListenerAlive == THREAD_ALIVE) /* Hurray, it's running */
		{
			/* start another test */
			SocketListenerAlive = THREAD_OFF;
			time( &socketListenerTimer );
		}
		else {
			if (difftime( socketListenerTimer, time(NULL)) > socketListenerTimeout )
			{	/* Very weird and very bad. Exit */
				logit("et","%s(%s): Socket Listener Thread hung. Exiting\n",
					Argv0, MyModName);
				export_shutdown();
				return(-1);
			}
		}
	    /* see if we had a problem anywhere in processing the last data */
	    if (ShutMeDown == TRUE) {
			logit("t", "%s(%s): termination requested by program; exiting!\n", Argv0, MyModName );
			export_shutdown();
	    }

		/* Beat the heart into the transport ring
		****************************************/
		time(&now);
		if (difftime(now,MyLastBeat) > (double)HeartBeatInt ) {
			export_status( TypeHeartBeat, 0, "" );
			time(&MyLastBeat);
		}

		/* take a brief nap
		********************/
		sleep_ew(100); 
	} /*end while of monitoring loop */

shutdown:
	/* Shut it down
	***************/
	logit("t", "%s(%s): termination requested; exiting!\n", Argv0, MyModName );
	export_shutdown();
}
/***************************** end of main *************************/

/********************** shutdown routine **************************
*				take care of all final matters and exit           *
*******************************************************************/
void export_shutdown()
{
	(void)KillThread(TidListener);
	(void)KillThread(TidSender);
	closesocket_ew( ActiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW);
	closesocket_ew( PassiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW );
	export_free();
	tport_detach( &Region );
	exit ( 0 );
}

/* ***************************************************************
* Below are the three threads: One for listening on the socket - *
* to get heartbeats, one for putting messages into the queue,    *
* and one to send messages and heartbeats                        *
******************************************************************/

/************************Socket Listening Thread *********************
*          Listen for client heartbeats, and set global variable     *
*          showing time of last heartbeat. Main thread will take     *
*          it from there                                             *
**********************************************************************/

thr_ret SocketListener( void *dummy )
{
	static int state;
	static char chr;
	static int nr;
	static long  nchar;			     /* counter for above buffer */
	static char startCharacter=STX;	     /* ASCII STX characer */
	static char endCharacter=ETX;             /* ASCII ETX character */
	static char inBuffer[INBUFFERSIZE];
	static long  inchar;                       /* counter for above buffer */
	long totalinchar=0;

	SocketListenerStatus= THREAD_ALIVE;

	/* Working loop: receive and process messages
	*********************************************/
	/* We are either  (0) initializing,
	(1) looking for a message start,
	(2) assembling a message
	the variable "state' determines our mood */

	state=SEARCHING_FOR_MESSAGE_START;  /* we're initializing */

	/* Start of New code Section DK 11/20 Multi-byte Read
	Set inchar to be nr-1, so that when inchar is incremented, they will be the
	same and a new read from socket will take place.  Set chr to 0 to indicate
	a null character, since we haven't read any yet.
    */
	inchar=-1;
	nr=0;
	chr=0;
	/* End of New code Section */

	while(1)  /* loop over character received */
	{
	   /* Tell the main thread we're ok
	    ********************************/
		/* Do this once through the loop, as the main thread will keep setting it
		to THREAD_OFF, just to see us raise it as a sign of continued life */
		SocketListenerAlive= THREAD_ALIVE;

		/* Read from buffer
		****************/
		/* Get next char operation */
		if (++inchar == nr)
		{
			/* Read from socket operation */
            nr=recv_ew(ActiveSocket,inBuffer,INBUFFERSIZE-1,0,
				SocketTimeoutLength);
            totalinchar+=nr;
            if (nr<=0)  /* Connection Closed */
            {
                sprintf (errText, "%s(%s): recv_ew()", Argv0, MyModName);
                SocketPerror(errText);
                logit("et", "%s(%s): Bad socket read: %d\n", Argv0, MyModName, nr);
                SocketListenerStatus=THREAD_ERR; /* file a complaint to the main thread */
                KillSelfThread(); /* main thread will restart us */
            }
            /*else*/
            inchar=0;
            /* End of Read from socket operation */
        }
        chr=inBuffer[inchar];
        /* End of Get next char operation */

		/* Initialization?
		******************/
		if (state==SEARCHING_FOR_MESSAGE_START)   /* throw all away until we see a start character */
        {
			nchar=0; /* next character position to load */
			if (chr == startCharacter) state=ASSEMBLING_MESSAGE; /* start assembing message from now on */
			continue;
        }

		/* Seeking message start
		************************/
        if (state==EXPECTING_MESSAGE_START)  /* the next char had better be a start character */
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

		/* in the throes of assembling a message */
		/*****************************************/
        if (state==ASSEMBLING_MESSAGE)
        {
			if (chr==endCharacter)   /* end of the message is at hand */
			{
				/* We have a complete message */
				/******************************/
				SLmsg[nchar]=0; /* terminate as a string */
				if(strcmp(&SLmsg[9],RcvAliveText)==0) /* Server's heartbeat */
				{
					if(Verbose) logit("et", "%s(%s): Received heartbeat\n",
						Argv0, MyModName);
					time(&LastRcvAlive); /* note time of heartbeat */
					state=EXPECTING_MESSAGE_START; /* reset for next message */
					SLmsg[0]=' '; SLmsg[9]=' ';
				}
				else
				{
					/* got a non-heartbeat message */
					logit("et",
						"%s(%s): Terminating - weird heartbeat from client:",
                        Argv0, MyModName);
					logit("e", "%s\n", SLmsg);
					exit(-1);    /* shut down. We don't know whom we're talking to */
				}
				continue;
			}
			else
			{
				/* keep building the message */
				/*****************************/
				if (nchar < MaxMsgSize)  /* then there's still room in the buffer */
				{
					SLmsg[nchar++]=chr;
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
   }  /* end while of loop over characters */
}  /* end of SocketListener thread */



/**************************Socket Writing Thread *********************
*          Pull a messsage from the queue, and push it out the       *
*          socket. If things go badly, set the global SocketStatus   *
*          non-zero to alert the main thread                         *
**********************************************************************/

thr_ret SocketSender( void *dummy )
{
   time_t 	now, lasttime = 0;
   MSG_LOGO 	reclogo;
   long		msgSize = 0;
   int          ret;
   long		maxBinSize;

   maxBinSize=MaxMsgSize*2;  /* worst case is that it's twice as long */


   /* Tell the main thread we're ok
   ********************************/
   SocketSenderStatus= THREAD_ALIVE;
   SocketSenderAlive= THREAD_ALIVE;

   /* as we start up, we check to see if there's anything in SSbinmsg. If there is,
   we presume that it was in the process of being sent when the socket broke, and
   this thread  terminated. We re-send that message first, then go into the working
   loop. */
   if(BinSize > 0)
   {
	   /* Re-send old message
	   **********************/
	   if(Verbose) logit("et","Resending old message\n");
	   if ( WriteToSocket( ActiveSocket, SSbinmsg, BinSize, &BinLogo ) != 0 )
	   {
		   /* If we get an error, simply quit */
		   sprintf( errText, "error re-sending msg to socket." );
		   export_status( TypeError, ERR_QUEUE, errText );
		   SocketSenderStatus=-1;
		   SocketSenderStatus=-1; /* file a complaint to the main thread */
		   KillSelfThread(); /* main thread will restart us */
	   }
	   BinSize=0; /* indicate empty buffer */
   }


   while (1)   /* main loop */
   {
	   /* Tell the main thread we're alive
	   ********************************/
	   SocketSenderAlive= THREAD_ALIVE;

	   /* Send alive message to client program
	   **************************************/
	   time( &now );
	   if ( now - lasttime >= (double)SendAliveInt && SendAliveInt != 0 )
	   {
		   reclogo.instid = InstId;
		   reclogo.mod    = MyModId;
		   reclogo.type   = TypeHeartBeat;
		   if ( WriteToSocket( ActiveSocket, SendAliveText,
			   (long)strlen(SendAliveText), &reclogo ) != 0 )
		   {
			   /* If we get an error, simply quit */
			   sprintf( errText, "error sending alive msg to socket" );
			   export_status( TypeError, ERR_QUEUE, errText );
			   SocketSenderStatus=THREAD_ERR; /* file a complaint to the main thread */
			   KillSelfThread(); /* main thread will restart us */
		   }
		   else
		   {
			   if(Verbose) logit("et", "%s(%s): Sent alive msg\n",
				   Argv0, MyModName );
		   }
		   lasttime = now;
	   }

	   /* Get message from queue
	   *************************/
	   RequestMutex();
	   ret=dequeue( &OutQueue, SSmsg, &msgSize, &BinLogo);
	   ReleaseMutex_ew();
	   if(ret < 0 )
	   { /* -1 means empty queue */
		   sleep_ew(500); /* wait a bit (changed from 1000 to 500 on 970624:ldd) */
		   continue;
	   }

	   /* Make the message safe for binary text */
	   /*****************************************/
	   if( binEscape(SSmsg, msgSize, SSbinmsg, &BinSize, maxBinSize) < 0)
	   {
		   logit("et","%s(%s): binEscape overflow; exiting!\n", Argv0, MyModName);
		   exit(-1);
	   }

	   /* Send message
	   ***************/
	   if ( WriteToSocket( ActiveSocket, SSbinmsg, BinSize, &BinLogo ) != 0 )
	   {
		   /* If we get an error, simply quit */
		   sprintf( errText, "error sending msg to socket." );
		   export_status( TypeError, ERR_QUEUE, errText );
		   SocketSenderStatus=THREAD_ERR; /* file a complaint to the main thread */
		   KillSelfThread(); /* main thread will restart us */
	   }
	   BinSize = 0;  /* nothing left in SSbinmsg to send */
	   sleep_ew(WWSMS); /* wait a bit so the WWS doesn't get overrun */

   }   /* End of main loop */

}




/********************** Message Stacking Thread *******************
*          Move messages from mSEED files to memory queue         *
******************************************************************/
thr_ret MessageStacker( void *dummy )
{
	long          recsize;	/* size of retrieved message             */
	MSG_LOGO      reclogo;       /* logo of retrieved message             */
	int		 i, res, ret;
	int           NumOfTimesQueueLapped= 0; /* number of messages lost due to queue lap */
	int           NumOfElementsInQueue; /* number of messages in the queue  */
	int           NumOfTBufs; /* number of tracebufs burst from this file  */

    char    whoami[50], subname[] = "Main";
    FILE   *fp;
    char    fname[FILE_NAM_LEN];         /* name of ascii version of evt file */
    char    outFname[FILE_NAM_LEN];      /* Full name of output file */
    char    errbuf[100];                 /* Buffer for error messages */
    int     FileOK, rc;
    DATA_HDR    mseed_hdr, *mseed_hdr_ptr;
    int         max_num_points, msdata[20000];
	unsigned char alert, okay;			/* flags */
	time_t stime, etime;		   

    sprintf(whoami, "%s: %s: ", progname, "MessageStacker");
    
    /* Change working directory to "NQFilesInDir" - where the files should be
     ************************************************************************/
	if ( chdir_ew( NQFilesInDir ) == -1 ) {
		logit( "e", "Error. Can't change working directory to %s\n Exiting.", NQFilesInDir );
		goto error;
	}
	if(Debug) logit( "e", "%sChanged working directory to %s\n ", whoami, NQFilesInDir );
	okay = 1;
	
   /* Start main export service loop for current connection
   ********************************************************/
	while( 1 ) {
		/* Tell the main thread we're ok
		 ********************************/
		MessageStackerAlive = THREAD_ALIVE;

	    /* Start of working loop over files.
	    ************************************/
	    while ((GetFileName (fname)) != 1 && okay) {    
			time(&stime);
	        /* Open the file for reading only.  Since the file name
	        is in the directory we know the file exists, but it
	        may be in use by another process.  If so, fopen_excl()
	        will return NULL.  In this case, wait a second for the
	        other process to finish and try opening the file again.
	        ******************************************************/
			if( strcmp(fname,"core")==0 ) {
				logit("et", "%s Deleting core file <%s>; \n", whoami, fname );
				if( remove( fname ) != 0) {
					logit("et", "%s Cannot delete core file <%s>; exiting!", whoami, fname );
					break;
				}
				continue;
			}
	        if ((fp = fopen(fname, "rb" )) == NULL) {
	            logit ("e", "Could not open file <%s>\n", fname);
	            continue;
	        }
			else {
				if(Debug) {
					logit("e", "\n\n ************************************************************ \n");
					logit("et", "\n *** Begin Processing file %s *** \n", fname);
					logit("e", "\n ************************************************************ \n");
				}
				
			}

	        /* Prime the data buffers
	        *************************/
	        nchans = 0;
	        for(i=0;i<MAXCHANS;i++) {
	        	chanbuf[i].bufptr = 0;
	        	strcpy(chanbuf[i].sncl, " ");
	        }

	        /* Read the mseed file
	        ************************/
	        NumOfTBufs = 0;
			FileOK = TRUE;
	        max_num_points = 10000;
	        mseed_hdr_ptr = &mseed_hdr;
	        while ((rc=read_ms (&mseed_hdr_ptr,  msdata, max_num_points, fp )) != EOF) {
	            if(rc >= 0) {
					if(Debug) {
						logit("e", "\nrc: %d seq_no: %d S_C_N_L: %s_%s_%s_%s  nsamp: %d %d %d %c\n", 
								rc, mseed_hdr_ptr->seq_no, mseed_hdr_ptr->station_id, mseed_hdr_ptr->channel_id, 
								mseed_hdr_ptr->network_id, mseed_hdr_ptr->location_id, mseed_hdr_ptr->num_samples, 
								mseed_hdr_ptr->sample_rate, mseed_hdr_ptr->data_type, mseed_hdr_ptr->record_type );
	                }
					if (rc == 0) continue;	/* No data in record */
	                
					if(mseed_to_tbuf(msdata, mseed_hdr_ptr, 1000) == 0) {
					
					}
	                
	            }
	            else {
					strcpy(errbuf, " ");
					if(rc == -2) strcpy(errbuf, "(MiniSEED error)");
					if(rc == -3) strcpy(errbuf, "(Malloc error)");
					if(rc == -4) strcpy(errbuf, "(Time error)");
					logit("e", "\n *** Error Processing file %s, Error: %d %s ***\n", 
										fname, rc, errbuf);
					FileOK = FALSE;
					if(rc < -1) break;
	            }
	            /* Really long files, like ringbuffers, need to take a break */
	            if(NumOfTBufs++ > 1200) {
	            	NumOfTBufs = 0;
					sleep_ew(2000);
	            }
	        } 

	        
	        fclose (fp);
	        
	        /* Dispose of file
	        *********************/
	        if (FileOK) {
	            /* all ok; move the file to the output directory */

	            sprintf (outFname, "%s/%s", NQFilesOutDir, fname);
	            if ( rename( fname, outFname ) != 0 ) {
	                logit( "e", "Error moving %s: %s\n", fname, strerror(errno));
	                goto error;
	            }
	            if(Debug) logit( "e", "Moved %s to %s\n", fname, NQFilesOutDir);
	        }
	        else {      /* something blew up on this file. Preserve the evidence */
	            logit("e","Error processing file %s\n",fname);

	            /* move the file to the error directory */
	            sprintf (outFname, "%s/%s", NQFilesErrorDir, fname);

	            if (rename (fname, outFname) != 0 ) {
	                logit( "e", "Fatal: Error moving %s: %s\n", fname, strerror(errno));
	                goto error;
	            } 
	            if(Debug) logit( "e", "Moved %s to %s\n", fname, NQFilesErrorDir);
	            continue;
	        }
	        
			time(&etime);
			logit("et","Done with file %s %.4f seconds.\n",fname, difftime(etime,stime));
			sleep_ew(1000);
		}
	        
	    
		sleep_ew(2000);



	} /* end of while */

	/* we're quitting
   *****************/
error:
	MessageStackerStatus = THREAD_ERR; /* file a complaint to the main thread */
	KillSelfThread(); /* main thread will restart us */
}


/********************************************************************
 *  mseed_to_tbuf converts MiniSEED to TraceBuf2 and                *
 *  writes to queue with tracebufs starting on integer seconds.     *
 ********************************************************************/

static TracePacket trace_buffer;

int mseed_to_tbuf(int *bytes, DATA_HDR *mseed_hdr, int out_data_size) 
{
    char    whoami[50];
    TRACE2_HEADER   *trace2_hdr;		/* ew trace header */
    int              i, j, k, offset, out_message_size;
    int*             inpTracePtr;
    char*            myTracePtr;
    char             sncl[20];
	int           NumOfElementsInQueue; /* number of messages in the queue  */

    sprintf(whoami, "%s: %s: ", progname, "mseed_to_tbuf");

	/*** If we have a log channel, return! ***/
	if (mseed_hdr->sample_rate == 0 || 
	    mseed_hdr->sample_rate_mult == 0 ||
	    mseed_hdr->num_samples == 0)  {
		return 1;
	}

	/*** Pre-fill some of the tracebuf header ***/
	trace2_hdr = (TRACE2_HEADER *) &trace_buffer.trh;
	memset((void*) trace2_hdr, 0, sizeof(TRACE2_HEADER));
	trace2_hdr->pinno = 0;		/* Unknown item */
	trace2_hdr->samprate = sps_rate(mseed_hdr->sample_rate,mseed_hdr->sample_rate_mult);
	strcpy(trace2_hdr->sta,trim(mseed_hdr->station_id));
	strcpy(trace2_hdr->net,trim(mseed_hdr->network_id));
	strcpy(trace2_hdr->chan,trim(mseed_hdr->channel_id));
	strcpy(trace2_hdr->loc,trim(mseed_hdr->location_id));
	if (0 == strncmp(trace2_hdr->loc, "  ", 2) || 0 == memcmp(trace2_hdr->loc, "\000\000", 2))
		strcpy(trace2_hdr->loc,"--");
	sprintf(sncl, "%s_%s_%s_%s", trace2_hdr->sta,trace2_hdr->net,trace2_hdr->chan,trace2_hdr->loc);
	
	strcpy(trace2_hdr->datatype,(my_wordorder == SEED_BIG_ENDIAN) ? "s4" : "i4");
			trace2_hdr->version[0]=TRACE2_VERSION0;
			trace2_hdr->version[1]=TRACE2_VERSION1;
	trace2_hdr->quality[1] = (char)mseed_hdr->data_quality_flags;
	
	/*** Move the data into the floating buffer. ***/
	k = -1;
	for(i=0;i<nchans;i++) {
		if(strcmp(chanbuf[i].sncl, sncl) == 0) k = i;
	}
	
	if(k == -1) {
		/*** New channel. Start floating buffer at next integer second. ***/
		k = nchans++;
		strcpy(chanbuf[k].sncl, sncl);
		offset = ((USECS_PER_SEC - mseed_hdr->begtime.usec)*trace2_hdr->samprate)/USECS_PER_SEC;
		for(i=offset,j=0;i<mseed_hdr->num_samples;i++,j++) chanbuf[k].buf[j] = bytes[i];
		chanbuf[k].bufptr = j;
		chanbuf[k].starttime = (double)unix_time_from_int_time(mseed_hdr->begtime) + 1;
		chanbuf[k].endtime   = chanbuf[k].starttime + (double)(trace2_hdr->nsamp - 1)/trace2_hdr->samprate;
		chanbuf[k].endtime   = chanbuf[k].endtime - 1;
		if(Debug) logit("et", "%sk: %d offset: %d numsamps: %d \n", whoami, k, offset, mseed_hdr->num_samples); 
	} else {
		for(i=0,j=chanbuf[k].bufptr;i<mseed_hdr->num_samples;i++,j++) chanbuf[k].buf[j] = bytes[i];
		chanbuf[k].bufptr = j;
	}
	
	if(Debug) {
		trace2_hdr->starttime = (double)unix_time_from_int_time(mseed_hdr->begtime) +
					((double)(mseed_hdr->begtime.usec)/USECS_PER_SEC);
		trace2_hdr->endtime = (double)unix_time_from_int_time(mseed_hdr->endtime) +
					((double)(mseed_hdr->endtime.usec)/USECS_PER_SEC);
		logit("et", "%sRcvd from mSEED   %s_%s_%s_%s %d Time: %f  %f \n",  whoami,
				trace2_hdr->sta, trace2_hdr->chan, trace2_hdr->net, trace2_hdr->loc, 
				mseed_hdr->num_samples, 
				trace2_hdr->starttime, trace2_hdr->endtime);
	}
	
	while(chanbuf[k].bufptr > trace2_hdr->samprate) {
		
		trace2_hdr->nsamp     = trace2_hdr->samprate;
		trace2_hdr->starttime = chanbuf[k].starttime;
		endtime = chanbuf[k].endtime;
		chanbuf[k].endtime    = chanbuf[k].starttime + (double)(trace2_hdr->nsamp - 1)/trace2_hdr->samprate;
		trace2_hdr->endtime   = chanbuf[k].endtime;

		myTracePtr = &trace_buffer.msg[0] + sizeof(TRACE2_HEADER);        
		inpTracePtr = &chanbuf[k].buf[0];        
	
		/* Move the trace */
		memcpy( (void*)myTracePtr, (void*)inpTracePtr, trace2_hdr->nsamp*sizeof(int32_t) );
    	
		out_message_size = sizeof(TRACE2_HEADER)+sizeof(int)*trace2_hdr->nsamp;
		
		if(Debug) {
			logit("et", "%sSENT to EARTHWORM %s_%s_%s_%s %d Time: %f  %f %f %f %d %d %d\n", whoami, 
					trace2_hdr->sta, trace2_hdr->chan, 
					trace2_hdr->net, trace2_hdr->loc, trace2_hdr->nsamp, 
					trace2_hdr->starttime, trace2_hdr->endtime, trace2_hdr->endtime-trace2_hdr->starttime, 
					trace2_hdr->starttime-endtime, out_message_size, chanbuf[k].bufptr, j-i);
		}
		/* transport it off to the queue */
		if ( queue_putmsg((long) out_message_size, (char*)&trace_buffer) != 0) {
			logit("et", "%s: Fatal Error sending trace via queue_putmsg()\n", progname);
			ShutMeDown = TRUE;
			err_exit = Q2EW_DEATH_EW_PUTMSG;
		} else if ( Verbose==TRUE ) {
			fprintf(stderr, "SENT to EARTHWORM %s_%s_%s_%s %d Time: \n", 
					trace2_hdr->sta, trace2_hdr->chan, 
					trace2_hdr->net, trace2_hdr->loc, trace2_hdr->nsamp);
		}
		
		/* Reset the floating buffer */
		for(i=0,j=trace2_hdr->nsamp;j<chanbuf[k].bufptr;i++,j++) chanbuf[k].buf[i] = chanbuf[k].buf[j];
		chanbuf[k].bufptr -= trace2_hdr->nsamp;
		chanbuf[k].starttime += 1;
    }
             
	return 0;
} 

/******************************************************************
*          Move tracebufs from mSEED files to memory queue        *
******************************************************************/
int queue_putmsg( long recsize, char* trace_buffer)
{
    char    whoami[50];
	MSG_LOGO      reclogo;       /* logo of retrieved message             */
	int		 res, ret, sleep;
	int           NumOfTimesQueueLapped= 0; /* number of messages lost due to queue lap */
	int           NumOfElementsInQueue; /* number of messages in the queue  */

    sprintf(whoami, "%s: %s: ", progname, "queue_putmsg");
	sleep = 0;
	RequestMutex();
	NumOfElementsInQueue = getNumOfElementsInQueue(&OutQueue);
	ReleaseMutex_ew();
	if(Debug) logit("et", "%sQueue has %d elements.  \n", whoami, NumOfElementsInQueue); 
	while(NumOfElementsInQueue > RingSize-5) {
		if(++sleep > 100) {
			logit("et", "%sQueue is stalled.  \n", whoami);
			return -1;
		}
		sleep_ew(500);
		MessageStackerAlive = THREAD_ALIVE;
		RequestMutex();
		NumOfElementsInQueue = getNumOfElementsInQueue(&OutQueue);
		if(Debug) logit("et", "Queue has %d elements.  %d\n", NumOfElementsInQueue, sleep); 
		ReleaseMutex_ew();
	}
	if(Debug && sleep > 0) logit("et", "Queue was filled. Slept %d sec.  \n", sleep); 

	/* Process retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK)
	***********************************************************/
	trace_buffer[recsize] = '\0';
	
	reclogo.instid = InstId;
	reclogo.mod    = MyModId;
	reclogo.type   = TypeTrace2;
	
	/* put it into the 'to be shipped' queue */
	/* the thread SocketSender is in the biz of de-queueng and sending */
	RequestMutex();
	ret=enqueue( &OutQueue, trace_buffer, recsize, reclogo );
	ReleaseMutex_ew();
	
	if ( ret!= 0 ) {
	   if (ret==-2) {  /* Serious: quit */
	      /* Currently, enqueue() in mem_circ_queue.c never returns this error. */
		   sprintf(errText,"internal queue error. Terminating.");
		   export_status( TypeError, ERR_QUEUE, errText );
			logit("et", "%s%s  \n", whoami, errText);
		   goto error;
	   }
	   if (ret==-1) {
		   sprintf(errText,"queue cannot allocate memory. Lost message.");
		   export_status( TypeError, ERR_QUEUE, errText );
			logit("et", "%s%s  \n", whoami, errText);
	   }
	   if (ret==-3  &&  LiveConnection) { /* Log only while client's connected */
	   /* Queue is lapped too often to be logged to screen.
	   * Log circular queue laps to logfile.
	   * Maybe queue laps should not be logged at all.   */
		
		   NumOfTimesQueueLapped++;
		   if (!(NumOfTimesQueueLapped % 5)) {
			   logit("t",
				   "%s(%s): Circular queue lapped 5 times. Messages lost.\n",
				   Argv0, MyModName);
			   if (!(NumOfTimesQueueLapped % 100)) {
				   logit( "et",
					   "%s(%s): Circular queue lapped 100 times. Messages lost.\n",
					   Argv0, MyModName);
			   }
		   }
	   }
	}
	return 0;

	/* we're quitting
   *****************/
error:
   MessageStackerStatus = THREAD_ERR; /* file a complaint to the main thread */
   KillSelfThread(); /* main thread will restart us */
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

int binEscape(char* inmsg, long inSize,
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
	for ( i=0;i<inSize; i++)   /*loop over bytes in input message */
	{
		tmp=inmsg[i];
		if ( tmp==startCharacter || tmp==endCharacter || tmp==escape )
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

int WriteToSocket( int ActiveSocket, char *msg, long msglength, MSG_LOGO *logo )
{
	char etext[128];           /* error text */
	char asciilogo[11];        /* ascii version of outgoing logo */
	char startmsg = STX;       /* flag for beginning of message  */
	char endmsg   = ETX;       /* flag for end of message        */
	int  rc;

	/* Send "start of transmission" flag and ascii representation of logo
	*********************************************************************/
	sprintf( asciilogo, "%c%3d%3d%3d", startmsg,
		(int) logo->instid, (int) logo->mod, (int) logo->type );
	rc = send_ew( ActiveSocket, asciilogo, 10, 0, SocketTimeoutLength );
	if( rc != 10 )
	{
		sprintf( etext, "%s: socket send of message logo", Argv0 );
		SocketPerror( etext );
		return( -1 );
	}


	/* Send message; break it into chunks if it's big!
	*************************************************/
	rc = send_ew( ActiveSocket, msg, msglength, 0, SocketTimeoutLength);
	if ( rc == -1 )
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
*                    exits if any errors are encountered.	             *
*****************************************************************************/
void export_config( char *configfile )
{
	int      ncommand;     /* # of required commands you expect to process   */
	char     init[20];     /* init flags, one byte for each required command */
	int      nmiss;        /* number of required commands that were missed   */
	char    *com;
	char     processor[17]; /* 15 to 16 change suggested by Wendy Tucker / Symmetric Research */
	int      nfiles;
	int      success;
	int      i;
	char*    str;

	/* Set to zero one init flag for each required command
	*****************************************************/
	ncommand = 16;
	for( i=0; i<ncommand; i++ )  init[i] = 0;
	nLogo = 0;
	WWSMS = 100;

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
						"%s: Error opening command file <%s>; exiting!\n",
						Argv0, &com[1] );
					exit( -1 );
				}
				continue;
            }
            strcpy( processor, "export_config" );

			/* Process anything else as a command
			************************************/
/*0*/     
			if( k_its("LogFile") ) {
				LogSwitch = k_int();
				init[0] = 1;
            }
/*1*/     
			else if( k_its("MyModuleId") ) {
				str = k_str();
				if(str) strcpy( MyModName, str );
				init[1] = 1;
            }
/*2*/     
			else if( k_its("RingName") ) {
				str = k_str();
				if(str) strcpy( RingName, str );
				init[2] = 1;
            }
/*3*/     
			else if( k_its("HeartBeatInt") ) {
				HeartBeatInt = k_int();
				init[3] = 1;
            }

			/* Enter installation & module & message types to get
			****************************************************/
/*4*/     
			else if( k_its("GetMsgLogo") ) {
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
/*5*/     
			else if( k_its("ServerIPAdr") ) {
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
/*6*/     
			else if( k_its("ServerPort") ) {
				ServerPort = k_int();
				init[6] = 1;
            }

			/* Maximum size (bytes) for incoming/outgoing messages
			*****************************************************/
/*7*/     
			else if( k_its("MaxMsgSize") ) {
				MaxMsgSize = k_long();
				if(MaxMsgSize < sizeof(TRACE2_HEADER)+sizeof(int)*300) MaxMsgSize = sizeof(TRACE2_HEADER)+sizeof(int)*300;
				init[7] = 1;
            }

			/* Maximum number of messages in outgoing circular buffer
			********************************************************/
/*8*/     
			else if( k_its("RingSize") ) {
				RingSize = k_long();
				init[8] = 1;
            }

			/* Interval (seconds) between alive msgs to client
			*************************************************/
/*9*/     
			else if( k_its("SendAliveInt") ) {
				SendAliveInt = k_int();
				init[9] = 1;
            }

			/* Text of alive message to client
			**********************************/
/*10*/    
			else if( k_its("SendAliveText") ) {
				str=k_str();
				if(str && strlen(str)<(size_t)MAX_ALIVE_STR) {
					strcpy(SendAliveText,str);
					init[10]=1;
				}
			}

			/* Interval (seconds) between alive msgs from client
			***************************************************/
/*11*/    
			else if( k_its("RcvAliveInt") ) {
				RcvAliveInt = k_int();
				init[11] = 1;
            }

			/* Text of alive message from client
			***********************************/
/*12*/    
			else if( k_its("RcvAliveText") ) {
				str=k_str();
				if(str && strlen(str)<(size_t)MAX_ALIVE_STR) {
					strcpy(RcvAliveText,str);
					init[12]=1;
				}
			}
/*13*/     
			else if( k_its("NQFilesInDir") ) {
                str = k_str();
                if(str) strcpy( NQFilesInDir, str );
                init[13] = 1;
            }
/*14*/     
			else if( k_its("NQFilesOutDir") ) {
                str = k_str();
                if(str) strcpy( NQFilesOutDir, str );
                init[14] = 1;
            }
/*15*/     
			else if( k_its("NQFilesErrorDir") ) {
                str = k_str();
                if(str) strcpy( NQFilesErrorDir, str );
                init[15] = 1;
            }
			else if ( k_its( "Debug" ) ) {
				Debug = 1;
				/* turn on the LogFile too! */
				LogSwitch = 1;
			 }  

			/* Timeout in milliseconds for IP Socket routines
			***********************************/
            else if(k_its("SocketTimeout") ) {
                if(RcvAliveInt == 0)
                {
					SocketTimeoutLength = k_int();
                }
            }

			/* Optional cmd: Turn on socket debug logging -> BIG LOG FILES!!
			***********************************/
            else if(k_its("SocketDebug") ) {
                SOCKET_ewDebug = k_int();
            }

			/* Optional cmd: Turn on debug logging -> BIG LOG FILES!!
			********************************************************/
            else if( k_its("WWSMS") ) {
				WWSMS = k_int();
            }

			/* Optional cmd: Turn on debug logging -> BIG LOG FILES!!
			********************************************************/
            else if( k_its("Verbose") ) {
				Verbose = 1;
            }

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
	}
	nfiles = k_close();
   }

   /* After all files are closed, check init flags for missed commands
   ******************************************************************/
   nmiss = 0;
   for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
   if ( nmiss ) {
       logit( "e", "%s: ERROR, no ", Argv0 );
       if ( !init[0] )  logit( "e", "<LogFile> "       );
       if ( !init[1] )  logit( "e", "<MyModuleId> "    );
       if ( !init[2] )  logit( "e", "<RingName> "      );
       if ( !init[3] )  logit( "e", "<HeartBeatInt> "  );
       if ( !init[4] )  logit( "e", "<GetMsgLogo> "    );
       if ( !init[5] )  logit( "e", "<ServerIPAdr> "   );
       if ( !init[6] )  logit( "e", "<ServerPort> "    );
       if ( !init[7] )  logit( "e", "<MaxMsgSize> "    );
       if ( !init[8] )  logit( "e", "<RingSize> "      );
       if ( !init[9] )  logit( "e", "<SendAliveInt> "  );
       if ( !init[10] ) logit( "e", "<SendAliveText>"  );
       if ( !init[11] ) logit( "e", "<RcvAliveInt> "   );
       if ( !init[12] ) logit( "e", "<RcvAliveText>"   );
       if ( !init[13] ) logit( "e", "<NQFilesInDir>"   );
       if ( !init[14] ) logit( "e", "<NQFilesOutDir>"  );
       if ( !init[15] ) logit( "e", "<NQFilesErrorDir>");
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
	*************************************/
	if( ( RingKey = GetKey(RingName) ) == -1 ) {
		logit( "e",
			"%s:  Invalid ring name <%s>; exiting!\n",
			Argv0, RingName);
		exit( -1 );
	}

	/* Look up installations of interest
	*********************************/
	if ( GetLocalInst( &InstId ) != 0 ) {
		logit( "e",
			"%s: error getting local installation id; exiting!\n",
			Argv0 );
		exit( -1 );
	}

	/* Look up modules of interest
	***************************/
	if ( GetModId( MyModName, &MyModId ) != 0 ) {
		logit( "e",
			"%s: Invalid module name <%s>; exiting!\n",
			Argv0, MyModName );
		exit( -1 );
	}

	/* Look up message types of interest
	*********************************/
	if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
		logit( "e",
		        "%s: Invalid message type <TYPE_HEARTBEAT>; exiting!\n", Argv0 );
		exit( -1 );
	}
	if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
		logit( "e",
		        "%s: Invalid message type <TYPE_ERROR>; exiting!\n", Argv0 );
		exit( -1 );
	}
   if ( GetType( "TYPE_TRACEBUF2", &TypeTrace2 ) != 0 ) {
		logit( "e",
              "%s: Message type <TYPE_TRACEBUF2> not found in earthworm_global.d; exiting!\n", progname);
        exit(-1);
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
	char	       msg[256];
	long	       size;
	time_t        t;

	/* Build the message
	*******************/
	logo.instid = InstId;
	logo.mod    = MyModId;
	logo.type   = type;

	time( &t );

	if( type == TypeHeartBeat )
        sprintf( msg, "%ld %ld\n\0", (long) t, (long)MyPid);
    else if( type == TypeError )
    {
        sprintf( msg, "%ld %hd %s\n\0", (long) t, ierr, note);

		logit( "et", "%s(%s): %s\n", Argv0, MyModName, note );
	}

	size = strlen( msg );   /* don't include the null byte in the message */

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
	free (Rawmsg);               /* "raw" retrieved msg for main thread      */
	free (SSmsg);                /* SocketSender's incoming message buffer   */
	free (SSbinmsg);             /* SocketSender's binary-escaped msg buffer */
	free (SLmsg);                /* SocketListener's incoming message buffer */
	free (MSrawmsg);             /* MessageStacker's "raw" retrieved message */
                                     /*    be sent to client                     */
	return;
}
