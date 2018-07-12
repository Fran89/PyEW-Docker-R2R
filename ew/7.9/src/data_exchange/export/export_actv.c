/****************************************************************************
 *   export_actv.c
 *
 *   Program to read messages (of user-given logos) from
 *   the transport ring and to export them over a socket.
 *
 *   Link export's object file with various filter object files to
 *   create customized export modules.  Dummy filter functions
 *   live in genericfilter.c.
 *
 *  -AM: Sun May  21 14:21:08 MDT 2000
 *  converted the active import_generic to passive, let the export module
 *  initiate communication. import_gen_pasv module prepares a port and waits
 *  for connection from active export_scn_act  module to receive messages.
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
#include <imp_exp_gen.h>
#include <mem_circ_queue.h>
#include <socket_ew.h>

#include "exportfilter.h"

#define EXPACTV_VERSION  "1.2 2015.03.27"

/* Functions in this source file
 *******************************/
void  export_config  ( char * );
void  export_lookup  ( void );
void  export_status  ( unsigned char, short, char * );
int   WriteToSocket  ( SOCKET, char *, long, MSG_LOGO * );
int   binEscape      (char*, long , char* , long* , long );

/* Thread things
 ***************/
#define THREAD_STACK 8192
static unsigned tidSender;           /* SocketSender thread id */
static unsigned tidListener;         /* Thread listening for client heartbeats */
static unsigned tidStacker;          /* Thread moving messages from transport */
                                     /*   to queue */
volatile int SocketSenderStatus=0;   /* 0=> SocketSender thread ok.   <0 => dead */
volatile int SocketListenerStatus=0; /* 0=> SocketListener thread ok. <0 => dead */
volatile int MessageStackerStatus=0; /* 0=> MessageStacker thread ok. <0 => dead */
volatile int connected=0;              /* 0= not connected   1=connected to recver */

QUEUE OutQueue;                                          /* from queue.h, queue.c; sets up linked */
                                       /*   list via malloc and free */
thr_ret SocketSender( void * );
thr_ret SocketListener( void * );
thr_ret MessageStacker( void * );

/* Timers
********/
time_t LastRcvAlive=0;               /* Last time we heard from our client */
time_t LastSendAlive=0;              /* Last time we sent a heartbeat to our client */
time_t now;                                  /* current time, used for timing heartbeats */
time_t MyLastBeat;                   /* time of last local (into Earthworm) hearbeat */
#define CONNECT_WAIT_TIME 5000
#define SHORT_WAIT_TIME   500
#define NOMSG_WAIT_TIME   100

extern int  errno;

static    SHM_INFO  Region;          /* shared memory region to use for i/o    */

#define   LOG_LIMIT 4
#define   MAXLOGO   10
MSG_LOGO  GetLogo[MAXLOGO];            /* array for requesting module,type,instid */
short     nLogo;

char      *Argv0;                          /* pointer to executable name */
pid_t      MyPid;                                /* Our own pid, sent with heartbeat for restart purposes */

/* Global socket things
**********************/
SOCKET    ActiveSocket;              /* Socket descriptor; active socket      */

/* Things to read or derive from configuration file
**************************************************/
#define ALV_LEN 256
static char    RingName[MAX_RING_STR];         /* name of transport ring for i/o    */
static char    MyModName[MAX_MOD_STR];        /* speak as this module name/id      */
static int     LogSwitch;            /* 0 if no logfile should be written */
static int     HeartBeatInt;         /* seconds between heartbeats        */
static char    ReceiverIpAdr[16];    /* receiver's IP address               */
static int     ReceiverPort;         /* receiver's well-known port number   */
static long    MaxMsgSize;           /* max size for input/output msgs    */
static int     RingSize;             /* max messages in output circular buffer       */
static int     SendAliveInt;            /* Send alive messages this often    */
static char    SendAliveText[ALV_LEN];  /* Text of alive message. Max size is traditional */
static int     RcvAliveInt;             /* Expect client heartbeats this often */
static char    RcvAliveText[ALV_LEN];   /* Text of client's alive messages     */
static int     Verbose=0;               /* changed to 1 by Verbose command */
static int     SocketTimeoutLength=-1;  /* Length of timeouts on SOCKET_ew calls */
static int     SOCKET_ewDebug=0;        /* Set to 1 for SOCKET_ew debug statements*/

/* Things to look up in the earthworm.h tables with getutil.c functions
**********************************************************************/
static long          RingKey;        /* key of transport ring for i/o     */
static unsigned char InstId;         /* local installation id             */
static unsigned char MyModId;        /* Module Id for this program        */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;

/* Error messages used by export
*******************************/
#define  ERR_MISSMSG       0         /* message missed in transport ring       */
#define  ERR_TOOBIG        1         /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2         /* msg retreived; tracking limit exceeded */
#define  ERR_SOCKETSEND    3         /* trouble writing to socket              */
#define  ERR_SOCKETACCEPT  4         /* error accepting socket connection      */
#define  ERR_QUEUE         5         /* error queueing message for sending     */

static char  errText[256];           /* string for log/error messages          */


/*****************************************************************************/
/*                             Main                                          */
/*****************************************************************************/
#define VERSION "V1.0.1 October 19, 2012"
int main( int argc, char **argv )
{
    /* Socket variables
    ******************/
    int    on = 1;
    struct sockaddr_in  skt_snd;

    /* Other variables
    *****************/
    time_t        timeNow;       /* current time                    */
    int           res;
    int           retry_cnt;
    int                     rcvcnt= 0;      /* DEBUG                                                    */
    long          recsize;              /* size of retrieved message      */
    MSG_LOGO      reclogo;              /* logo of retrieved message      */
    char          errtxt[256];      /* error text message             */
    char*         msg;                      /* "raw" retrieved message        */

/* Catch broken socket signals
*****************************/
#ifdef _SOLARIS
    (void)sigignore(SIGPIPE);
#endif

    /* Check command line arguments
    *****************************/
    Argv0 = argv[0];
    if ( argc != 2 )
    {
        fprintf( stderr, "Usage: %s <configfile>\nVersion: %s\n", Argv0, EXPACTV_VERSION );
        fprintf( stderr, "Version: %s\n", VERSION );
        return( 0 );
    }

    /* Initialize name of log-file & open it
    ***************************************/
    logit_init( argv[1], 0, 512, 1 );

    /* Read the configuration file(s)
    *******************************/
    export_config( argv[1] );
    logit( "et" , "%s(%s): Read command file <%s>\n", Argv0,
             MyModName, argv[1] );
    logit( "et" , "%s(%s): Version %s\n", Argv0, MyModName);

    /* Set SOCKET_ew debugging on/off
    ********************************/
    setSocket_ewDebug(SOCKET_ewDebug);

    /* Look up important info from earthworm.h tables
    ************************************************/
    export_lookup();

    /* Initialize name of log-file & open it
    ***************************************/
    logit_init( argv[1], 0, 512, LogSwitch );


    /* Get our own Pid for restart purposes
    **************************************/
    MyPid = getpid();
    if(MyPid == -1)
    {
        logit("e", "%s(%s): Cannot get pid; exiting!\n", Argv0, MyModName);
        return(0);
    }

    /* Initialize export filter
    **************************/
    if( exportfilter_init() != 0 )
    {
        logit("e", "%s(%s): Error in exportfilter_init(); exiting!\n",
                    Argv0, MyModName );
        return(-1);
    }

    /* Allocate space for input/output messages
    ******************************************/
    if ( ( msg = (char *) malloc(MaxMsgSize+1) ) == (char *) NULL )
    {
        logit( "e", "%s(%s): error allocating msg; exiting!\n",
                     Argv0, MyModName );
        return( -1 );
    }

    /* Create a Mutex to control access to queue
    *******************************************/
    CreateMutex_ew();

    /* Initialize the message queue
    ******************************/
    initqueue( &OutQueue, (unsigned long)RingSize,(unsigned long)MaxMsgSize+1 );

    /* Attach to Input/Output shared memory ring
    *******************************************/
    tport_attach( &Region, RingKey );

    /* step over all messages from transport ring
    ********************************************/
    do
    {
        res = tport_getmsg( &Region, GetLogo, nLogo, &reclogo, &recsize,
                                                msg, MaxMsgSize );
    } while (res !=GET_NONE);


    /* Initialize the socket system
    ******************************/
    SocketSysInit();

    /* Stuff address and port into socket structure
    **********************************************/
    memset( (char *)&skt_snd, 0, sizeof(skt_snd) );
    skt_snd.sin_family = AF_INET;
    skt_snd.sin_port = htons ( (short)ReceiverPort );
#if defined(_LINUX) || defined(_MACOSX)
    if ( (int)(skt_snd.sin_addr.s_addr = inet_addr(ReceiverIpAdr)) == -1 )
#else
    if ( (int)(skt_snd.sin_addr.S_un.S_addr = inet_addr(ReceiverIpAdr)) == -1 )
#endif
    {
        logit ( "e", "%s(%s): inet_addr failed for ReceiverIpAdr <%s>; exiting.\n",
                        Argv0, MyModName, ReceiverIpAdr );
        return -1;
    }

    /* Prevent flooding the log file during long reconnect attempts
    **************************************************************/
    retry_cnt=0;

    /* loop until connected or told to exit
    **************************************/
    while ( !connected )
    {
                int flag;

        /* Are we being told to quit?
        ****************************/
                flag = tport_getflag ( &Region );
        if ( flag == TERMINATE  ||  flag == MyPid )
        {
            break;
            /* tport_detach( &Region );
             * logit ( "t", "%s(%s): terminating on request\n", Argv0, MyModName );
             * free(msg);
             * closesocket_ew(ActiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW);
             * return 0;
             */
        }

        /* Create a socket
        *****************/
        if ( ( ActiveSocket = socket_ew( AF_INET, SOCK_STREAM, 0) ) == -1 )
        {
            sprintf(errtxt, "%s(%s): socket_ew", Argv0, MyModName);
            SocketPerror(errtxt);
            logit( "et", "%s(%s): Error opening socket; exiting!\n", Argv0, MyModName );
            free(msg);
            tport_detach( &Region );
            return( -1 );
        }

        if ( retry_cnt < LOG_LIMIT )
        {
            logit( "et", "%s(%s): Trying to connect to %s on port %d\n",
                        Argv0, MyModName, ReceiverIpAdr, ReceiverPort );
        }

        if ( retry_cnt == LOG_LIMIT )
        {
            logit( "et", "%s(%s): Repetitions of connect attempts not logged\n",
                    Argv0, MyModName );
        }

        /* Attempt to connect
        ********************/
        if ( connect_ew( ActiveSocket, (struct sockaddr *) &skt_snd, sizeof(skt_snd),
                        SocketTimeoutLength ) == -1 )
        {

            if ( retry_cnt < LOG_LIMIT )
            {
                logit( "et", "%s(%s): Socket connect failed, wainting for retry \n",
                            Argv0, MyModName );
            }

            if ( retry_cnt == LOG_LIMIT )
            {
                logit( "t", "%s(%s): Repetitions of failure to connect not logged \n",
                            Argv0, MyModName );
            }

            sleep_ew(CONNECT_WAIT_TIME);
            if ( retry_cnt < (LOG_LIMIT + 1) )
            {
                retry_cnt++;
            }
            continue;
        }

        /* Successful connection
        ***********************/
        logit( "et", "%s(%s): Connected to %s on port %d\n",
                    Argv0, MyModName, ReceiverIpAdr, ReceiverPort );
        connected=1;
        retry_cnt=0;


        /* Start the  message stacking thread
        ************************************/
        if ( StartThread(  MessageStacker, (unsigned)THREAD_STACK, &tidStacker ) == -1 )
        {
            logit( "e", "%s(%s): Error starting  MessageStacker thread; exiting!\n",
                        Argv0, MyModName );
            tport_detach( &Region );
            closesocket_ew( ActiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW);
            free(msg);
            return( -1 );
        }
        MessageStackerStatus=0;                 /*assume the best*/

        /* One heartbeat to announce ourselves to statmgr
        ***********************************************/
        export_status( TypeHeartBeat, 0, "" );
        time(&MyLastBeat);


        /* Start the  socket writing thread
        **********************************/
        if ( StartThread(  SocketSender, (unsigned)THREAD_STACK, &tidSender ) == -1 )
        {
            logit( "e", "%s(%s): Error starting SocketSender thread; exiting!\n",
                        Argv0, MyModName );
            tport_detach( &Region );
            closesocket_ew( ActiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW);
            free(msg);
            return( -1 );
        }
        SocketSenderStatus=0;               /*assume the best*/


        /* Start the  socket listening thread
        ************************************/
        if ( StartThread(  SocketListener, (unsigned)THREAD_STACK, &tidListener ) == -1 )
        {
            logit( "e", "%s(%s): Error starting SocketListener thread; exiting.\n",
                        Argv0, MyModName );
            tport_detach( &Region );
            closesocket_ew( ActiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW);
            free(msg);
            return( -1 );
        }
        SocketListenerStatus=0;                 /*assume the best*/


        /* fake time of last heartbeat
        *****************************/
        time(&LastRcvAlive);                /* from distant client */

        /* Start main export service loop for current connection
        *******************************************************/
        while( tport_getflag( &Region ) != TERMINATE  &&
                       tport_getflag( &Region ) != MyPid         )
        {
            /* See if our client has beat it's heart
             ***************************************/
            time(&timeNow);
            if(difftime(timeNow,LastRcvAlive) > (double)RcvAliveInt && RcvAliveInt != 0 )
            {
                logit("et", "%s(%s): lost import heartbeat\n", Argv0, MyModName);
                SocketListenerStatus = -1; /* give up */
            }

            /* see how our threads are feeling
            *********************************/
            if( SocketSenderStatus   < 0  ||
                    SocketListenerStatus < 0  ||
                    MessageStackerStatus < 0     )
            {
                logit("et", "%s(%s) restarting. This procedure may hang. "
                                        "Make sure restartMe is set in my .desc file\n",
                            Argv0, MyModName  );

                /* Stop the socket threads
                *************************/
                (void)KillThread(tidListener);
                (void)KillThread(tidSender);
                (void)KillThread(tidStacker);

                SocketListenerStatus=0;
                SocketSenderStatus=0;
                MessageStackerStatus=0;

                sleep_ew(1000);        /* give threads a second to die */

                connected = 0;
                break;
            }

            /* Beat the heart into the transport ring
            ****************************************/
            time(&now);
            if (difftime(now,MyLastBeat) > (double)HeartBeatInt )
            {
                export_status( TypeHeartBeat, 0, "" );
                time(&MyLastBeat);
            }

            /* take a brief nap
            ******************/
            sleep_ew(SHORT_WAIT_TIME);

        } /*  while( tport_getflag( &Region ) != TERMINATE ) */

    } /* while(!connected) */

    /* Shut it down
    *************/
    logit("t", "%s(%s): termination requested; exiting!\n",
                Argv0, MyModName );
    closesocket_ew( ActiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW );
    tport_detach( &Region );
    free(msg);
    exportfilter_shutdown();
    return( 0 );

} /* main */


/************************Socket Listening Thread *********************
*          Listen for client heartbeats, and set global variable     *
*          showing time of last heartbeat. Main thread will take     *
*          it from there                                             *
**********************************************************************/

thr_ret SocketListener( void *dummy )
{
    static int   state;
    static char  chr;
    static int   nr;
    static char  *msgBuf;                    /* incoming message buffer */
    static long  nchar;                        /* counter for above buffer */
    static char  startCharacter=STX;       /* ASCII STX characer */
    static char  endCharacter=ETX;       /* ASCII ETX character */
    static int   iret;                         /* misc. retrun values */
    static time_t now;
    static char  lastChr;
    static char  inBuffer[INBUFFERSIZE];
    static long  inchar;                 /* counter for above buffer */
    long totalinchar=0;

    /* Allocate output buffer
    ************************/
    if ( ( msgBuf = (char *) malloc(MaxMsgSize+1) ) == (char *) NULL )
    {
        logit( "e", "%s(%s): SocketListener error allocating msg; exiting!\n",
                     Argv0, MyModName );
        SocketListenerStatus=-1;   /* file a complaint to the main thread */
        KillSelfThread();            /* main thread will restart us */
        return THR_NULL_RET;         /* make sure this thread exits */
    }

    /* Tell the main thread we're ok
    *******************************/
    SocketListenerStatus=0;

    /* Working loop: receive and process messages
    ********************************************/

    /*
     *  We are either (0) initializing,
     *                    (1) looking for a message start,
     *                (2) assembling a message
     *  the variable "state' determines our mood
     */

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

    while(1)  /* loop over character received */
    {
        /* Read from buffer; get next char operation
        *******************************************/
        if (++inchar == nr)
        {
            /* Read from socket operation
            ****************************/
            nr=recv_ew(ActiveSocket,inBuffer,INBUFFERSIZE-1,0,
                                 SocketTimeoutLength);
            totalinchar+=nr;
            if (nr<=0)                  /* Connection Closed */
            {
                sprintf (errText, "%s(%s): recv_ew()", Argv0, MyModName);
                SocketPerror(errText);
                logit("et", "%s(%s): Bad socket read: %d\n", Argv0, MyModName, nr);
                free(msgBuf);
                SocketListenerStatus=-1; /* file a complaint to the main thread */
                KillSelfThread();          /* main thread will restart us */
                return THR_NULL_RET;       /* make sure this thread exits */
            }

            inchar=0;
        }
        lastChr=chr;
        chr=inBuffer[inchar];

        /* Initialization?
        *****************/
        if (state==SEARCHING_FOR_MESSAGE_START)   /* throw all away until a start character */
        {
            nchar=0;                                        /* next character position to load */
            if (chr == startCharacter)            /* start assembing message from now on */
                state=ASSEMBLING_MESSAGE;
            continue;
        }

        /* Seeking message start
        ***********************/
        if (state==EXPECTING_MESSAGE_START)  /* the next char better be a start character */
        {
            nchar=0;
            if (chr != startCharacter) /* then we're eating garbage */
            {
                logit( "et", "%s(%s): unexpected character from client\n",
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

        /* in the throes of assembling a message
        ***************************************/
        if (state==ASSEMBLING_MESSAGE)
        {
            if (chr==endCharacter)   /* end of the message is at hand */
            {
                /* We have a complete message
                ****************************/
                msgBuf[nchar]=0; /* terminate as a string */
                if(strcmp(&msgBuf[9],RcvAliveText)==0) /* Server's heartbeat */
                {
                    if (Verbose) logit("et", "%s(%s): Received heartbeat\n",
                                                            Argv0, MyModName);
                    time(&LastRcvAlive); /* note time of heartbeat */
                    state=EXPECTING_MESSAGE_START; /* reset for next message */
                    msgBuf[0]=' '; msgBuf[9]=' ';
                }
                else
                {
                    /* got a non-heartbeat message
                    *****************************/
                    logit("et", "%s(%s): Terminating - weird heartbeat from client:",
                                Argv0, MyModName);
                    logit("e", "%s\n", msgBuf);
                    free(msgBuf);
                    SocketListenerStatus=-1;   /* file a complaint to the main thread */
                    KillSelfThread();            /* main thread will restart us */
                    return THR_NULL_RET;         /* make sure this thread exits */
                }
                continue;
            }
            else
            {
                /* keep building the message
                ***************************/
                if (nchar < MaxMsgSize)  /* then there's still room in the buffer */
                {
                    msgBuf[nchar++]=chr;
                }
                else  /* freakout: message won't fit */
                {
                    logit("et", "%s(%s): receive buffer overflow after %ld bytes\n",
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
    time_t      now;
    time_t      lasttime=0;
    MSG_LOGO    reclogo;
    char*       msg;
    char*       binMsg;
    int       ret;
    int             sndcnt=0;
    long            msgSize;
    long            binSize;
    long            maxBinSize;

    /* Allocate message buffer
    **************************/
    if ( ( msg = (char *) malloc(MaxMsgSize+1) ) == (char *) NULL )
    {
        logit( "e", "%s(%s): SocketSender error allocating msg buf; exiting!\n",
                     Argv0, MyModName );
        SocketSenderStatus=-1;              /* file a complaint to the main thread */
        KillSelfThread();                       /* main thread will restart us */
        return THR_NULL_RET;                /* make sure this thread exits */
    }

    /* Allocate buffer for escaped message
    **************************************/
    maxBinSize=MaxMsgSize*2;  /* worst case is that it's twice as long */
    if ( ( binMsg = (char *) malloc(maxBinSize+1) ) == (char *) NULL )
    {
        logit( "e",
                     "%s(%s): SocketSender error allocating bin msg buf; exiting!\n",
                        Argv0, MyModName );
        free(msg);
        SocketSenderStatus=-1;              /* file a complaint to the main thread */
        KillSelfThread();                       /* main thread will restart us */
        return THR_NULL_RET;                /* make sure this thread exits */
    }

    /* beginning of working loop
    ***************************/
    while(1)
    {

        /* Tell the main thread we're ok
        *******************************/
        SocketSenderStatus=0;

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
                /* If we get an error, simply quit
                *********************************/
                sprintf( errText, "error sending alive msg to socket" );
                export_status( TypeError, ERR_QUEUE, errText );
                free(msg);
                free(binMsg);
                SocketSenderStatus=-1;              /* file a complaint to the main thread */
                KillSelfThread();                       /* main thread will restart us */
                return THR_NULL_RET;                /* make sure this thread exits */
            }

            if (Verbose) logit("et", "%s(%s): Sent alive msg\n", Argv0, MyModName );

            lasttime = now;
        }

        /* Get message from queue
        ************************/
        RequestMutex();
        ret=dequeue( &OutQueue, msg, &msgSize, &reclogo);
        ReleaseMutex_ew();

        if(ret < 0 )                     /* -1 means empty queue */
        {
            sleep_ew(SHORT_WAIT_TIME);
            continue;
        }

        /* Make the message safe for binary text
        ***************************************/
        if( binEscape(msg, msgSize, binMsg, &binSize, maxBinSize) < 0)
        {
            logit("et","%s(%s): binEscape overflow; exiting!\n", Argv0, MyModName);
            free(msg);
            free(binMsg);
            SocketSenderStatus=-1;              /* file a complaint to the main thread */
            KillSelfThread();                       /* main thread will restart us */
            return THR_NULL_RET;                /* make sure this thread exits */
        }

        /* Send message
        **************/
        if ( WriteToSocket( ActiveSocket, binMsg, binSize, &reclogo ) != 0 )
        {
            /* If we get an error, simply quit
            *********************************/
            sprintf( errText, "error sending msg to socket." );
            export_status( TypeError, ERR_QUEUE, errText );
            free(msg);
            free(binMsg);
            SocketSenderStatus=-1;              /* file a complaint to the main thread */
            KillSelfThread();                       /* main thread will restart us */
            return THR_NULL_RET;                /* make sure this thread exits */
        }

    } /* while(1) */

}



/********************** Message Stacking Thread *******************
 *           Move messages from transport to memory queue         *
 ******************************************************************/
thr_ret MessageStacker( void *dummy )
{
    char            *msg;                       /* "raw" retrieved message               */
    char                    *filteredMsg;   /* "filtered" message to send to client  */
    unsigned char filteredType;   /* type of message after filtering       */
    MSG_LOGO        reclogo;        /* logo of retrieved message             */
    long            recsize;                /* size of retrieved message             */
    long                    filteredSize;   /* size of message after user-filtering  */
    int                     res;
    int                     ret;
    int           NumOfTimesQueueLapped= 0; /* number of messages lost due */
                                          /* to queue lap */

    /* Allocate space for input/output messages
    ******************************************/
    if ( ( msg = (char *) malloc(MaxMsgSize+1) ) == (char *) NULL )
    {
        logit( "e", "%s(%s): error allocating msg; exiting!\n",
                     Argv0, MyModName );
        MessageStackerStatus=-1;    /* file a complaint to the main thread */
        KillSelfThread();           /* main thread will restart us */
        return THR_NULL_RET;        /* make sure this thread exits */
    }
    if ( ( filteredMsg = (char *) malloc(MaxMsgSize+1) ) == (char *) NULL )
    {
        logit( "e", "%s(%s): error allocating filteredMsg; exiting!\n",
                        Argv0, MyModName );
        MessageStackerStatus=-1;    /* file a complaint to the main thread */
        free(msg);
        KillSelfThread();           /* main thread will restart us */
        return THR_NULL_RET;        /* make sure this thread exits */
    }

    /* Tell the main thread we're ok
    ********************************/
    MessageStackerStatus=0;

    /* Start main export service loop for current connection
    ********************************************************/
    while( 1 )
    {
        /* Get a message from transport ring
        ************************************/
        res = tport_getmsg( &Region, GetLogo, nLogo,
                                                &reclogo, &recsize, msg, MaxMsgSize );

        /* Wait if no messages for us
        ****************************/
        if ( res == GET_NONE )
        {
            /*  if (Verbose) logit("et", "%s(%s): No messages to get from ring\n", Argv0, MyModName );  */
            sleep_ew(NOMSG_WAIT_TIME);
            continue;
        }

        /* Check return code; report errors
        ***********************************/
        if ( res != GET_OK )
        {
             if ( res==GET_TOOBIG )
             {
                    sprintf( errText, "msg[%ld] i%d m%d t%d too long for target, note MaxMsgSize=%ld",
                                                    recsize, (int) reclogo.instid,
                    (int) reclogo.mod, (int)reclogo.type , MaxMsgSize);
                    export_status( TypeError, ERR_TOOBIG, errText );
                    continue;
             }
             else if ( res==GET_MISS )
             {
                    sprintf( errText, "missed msg(s) i%d m%d t%d in %s",(int) reclogo.instid,
                                    (int) reclogo.mod, (int)reclogo.type, RingName );
                    export_status( TypeError, ERR_MISSMSG, errText );
             }
             else if(  res==GET_NOTRACK )
             {
                    sprintf( errText, "no tracking for logo i%d m%d t%d in %s",
                                     (int) reclogo.instid, (int) reclogo.mod, (int)reclogo.type,
                                     RingName );
                    export_status( TypeError, ERR_NOTRACK, errText );
             }
        }

        /* Process retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK)
        **********************************************************/
        msg[recsize] = '\0';

        /* pass it through the filter routine: this may reformat,
         *   or reject as it chooses.
         */
        if ( exportfilter( msg, recsize, reclogo.type, &filteredMsg,
                                            &filteredSize, &filteredType ) == 0 )
        {
            continue;
        }

        if (Verbose)
        {
            logit("et","%s(%s): A valid message to be queued to send\n",
                        Argv0, MyModName);
        }

        reclogo.type = filteredType;  /* note the new message type */

        /* put it into the 'to be shipped' queue
         * the thread SocketSender is in the biz of de-queueng and sending
         */
        RequestMutex();
        ret=enqueue( &OutQueue, filteredMsg, filteredSize, reclogo );
        ReleaseMutex_ew();

        if ( ret!= 0 )
        {
            if (ret==-2)                            /* Serious: quit */
            {
                sprintf(errText,"internal queue error. Terminating.");
                export_status( TypeError, ERR_QUEUE, errText );
                MessageStackerStatus=-1;    /* file a complaint to the main thread */
                free(msg);
                free(filteredMsg);
                KillSelfThread(); /* main thread will restart us */
                return THR_NULL_RET;     /* make sure this thread exits */
            }

            if (ret==-1)
            {
                sprintf(errText,"queue cannot allocate memory. Lost message.");
                export_status( TypeError, ERR_QUEUE, errText );
                continue;
            }

            if (ret==-3  &&  connected)  /* Log only while client's connected */
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

        } /* if ( ret!= 0 ) */

        if (Verbose) logit("et", "%s(%s): queued msg len:%ld\n",
                                             Argv0, MyModName, filteredSize);

    } /* end of while */

}



   /***************************binEscape******************************
    *    make a binary message safe for shipping: insert an escape   *
    *    character before any bit pattern which looks like a sacred  *
    *    character. That will allow the receiving code to recognize  *
    *    it as of the data, and not as a sacred character.           *
    *    Sacred characters are: STX (02), ETX (03), and, of course,  *
    *                           ESC (27) itsself                     *
    *                                                                *
    *    Returns 0 if ok, -1 if sanitized message did not fit        *
    ******************************************************************/

int binEscape(char* inmsg, long inSize, char* outmsg, long* outSize,
                            long outMaxSize )
{
    static char startCharacter=STX; /* from imp_exp_gen.h */
    static char endCharacter=ETX;
    static char escape=ESC;
    static char tmp;
    int    i;

    *outSize = 0;                           /* index to next byte in outgoing msg */
    for ( i=0;i<inSize; i++)   /*loop over bytes in input message */
    {
        tmp=inmsg[i];
        if ( tmp==startCharacter || tmp==endCharacter || tmp==escape )
        {
            if ((*outSize)+1 > outMaxSize)
            {
                return(-1);
            }
            outmsg[(*outSize)++]=escape;
            outmsg[(*outSize)++]=tmp;
        }
        else
        {
            if(*outSize > outMaxSize)
            {
                return(-1);
            }
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

int WriteToSocket( SOCKET ActiveSocket, char *msg, long msglength, MSG_LOGO *logo )
{
    char etext[128];           /* error text */
    char asciilogo[11];        /* ascii version of outgoing logo */
    char startmsg = STX;       /* flag for beginning of message  */
    char endmsg   = ETX;       /* flag for end of message        */
    int  rc;

    /* Send "start of transmission" flag and ascii representation of logo
    ***********************************/
    sprintf( asciilogo, "%c%3d%3d%3d", startmsg, (int) logo->instid,
                     (int) logo->mod, (int) logo->type );
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
 *                    exits if any errors are encountered.                   *
 *****************************************************************************/
void export_config( char *configfile )
{
    char     init[20];     /* init flags, one byte for each required command */
    char     *com;
    char*    str;
    char     processor[20];
    int      ncommand;     /* # of required commands you expect to process   */
    int      nmiss;        /* number of required commands that were missed   */
    int      nfiles;
    int      success;
    int      i;

    /* Set to zero one init flag for each required command
    *****************************************************/
    ncommand = 14;
    for( i=0; i<ncommand; i++ )  init[i] = 0;
    nLogo = 0;

    /* Open the main configuration file
    **********************************/
    nfiles = k_open( configfile );
    if ( nfiles == 0 )
    {
        fprintf( stderr,
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
                if( com[0] == '@' )
                {
                    success = nfiles+1;
                    nfiles  = k_open(&com[1]);
                    if ( nfiles != success )
                    {
                        logit("e",
                                        "%s: Error opening command file <%s>; exiting!\n",
                                         Argv0, &com[1] );
                        exit( -1 );
                    }
                    continue;
                }
                strcpy( processor, "export_config" );

                /* Process anything else as a command
                ************************************/
    /*0*/ if( k_its("LogFile") )
                {
                    LogSwitch = k_int();
                    init[0] = 1;
                }
    /*1*/ else if( k_its("MyModuleId") )
                {
                    str = k_str();
 		    if (strlen(str) > MAX_MOD_STR)
                    {
                        logit("e",
                                        "%s: Error MyModuleId <%s>; too large, must be less than %d chars!\n",
                                         Argv0, str, MAX_MOD_STR );
                        exit( -1 );
                    }
                    if(str) strcpy( MyModName, str );
                    init[1] = 1;
                }
    /*2*/ else if( k_its("RingName") )
                {
                    str = k_str();
 		    if (strlen(str) > MAX_RING_STR)
                    {
                        logit("e",
                                        "%s: Error RingName <%s>; too large, must be less than %d chars!\n",
                                         Argv0, str, MAX_RING_STR );
                        exit( -1 );
                    }
                    if(str) strcpy( RingName, str );
                    init[2] = 1;
                }
    /*3*/ else if( k_its("HeartBeatInt") )
                {
                    HeartBeatInt = k_int();
                    init[3] = 1;
                }


        /* Enter installation & module & message types to get
        ****************************************************/
  /*4*/ else if( k_its("GetMsgLogo") )
                {
          if ( nLogo >= MAXLOGO )
                    {
            logit("e",
                     "%s: Too many <GetMsgLogo> commands in <%s>",
                     Argv0, configfile );
            logit("e", "; max=%d; exiting!\n", (int) MAXLOGO );
                     exit( -1 );
                    }
          if( ( str=k_str() ) )
                    {
            if( GetInst( str, &GetLogo[nLogo].instid ) != 0 )
                        {
              logit("e",
                       "%s: Invalid installation name <%s>", Argv0, str );
              logit("e", " in <GetMsgLogo> cmd; exiting!\n" );
              exit( -1 );
            }
          }
          if( ( str=k_str() ) )
                    {
            if( GetModId( str, &GetLogo[nLogo].mod ) != 0 )
                        {
              logit("e",
                       "%s: Invalid module name <%s>", Argv0, str );
              logit("e", " in <GetMsgLogo> cmd; exiting!\n" );
              exit( -1 );
            }
          }
          if( ( str=k_str() ) )
                    {
            if( GetType( str, &GetLogo[nLogo].type ) != 0 )
                        {
                            logit("e",
                                             "%s: Invalid msgtype <%s>", Argv0, str );
                            logit("e", " in <GetMsgLogo> cmd; exiting!\n" );
                            exit( -1 );
                        }
                    }
                    nLogo++;
                    init[4] = 1;

                    /*DEBUG*/
                    /*    printf("GetLogo[%d] inst:%d module:%d type:%d\n",
                     *                      nLogo, (int) GetLogo[nLogo].instid,
           *            (int) GetLogo[nLogo].mod, (int) GetLogo[nLogo].type );
                     */

        }

        /* IP Address to connect to
        **************************/
  /*5*/ else if( k_its("ReceiverIpAdr") )
                {
          str = k_str();
          if(str)
                    {
            if( strlen(str) >= sizeof( ReceiverIpAdr ) )
                        {
              logit("e",
                       "%s: ReceiverIpAdr <%s> too long; exiting!\n",
                       Argv0, str );
              exit( -1 );
            }
            strcpy( ReceiverIpAdr, str );
            init[5] = 1;
          }
        }


        /* Well-known receiver port number
        *******************************/
  /*6*/ else if( k_its("ReceiverPort") )
                {
          ReceiverPort = k_int();
          init[6] = 1;
        }

        /* Maximum size (bytes) for incoming/outgoing messages
        *****************************************************/
  /*7*/ else if( k_its("MaxMsgSize") )
                {
          MaxMsgSize = k_long();
          init[7] = 1;
        }

        /* Maximum number of messages in outgoing circular buffer
        ********************************************************/
  /*8*/ else if( k_its("RingSize") )
                {
          RingSize = k_long();
          init[8] = 1;
        }

        /* Interval (seconds) between alive msgs to client
        *************************************************/
  /*9*/ else if( k_its("SendAliveInt") )
                {
          SendAliveInt = k_int();
          init[9] = 1;
        }

                /* Text of alive message to client
                **********************************/
 /*10*/ else if( k_its("SendAliveText") )
                {
                    str=k_str();
                    if(str && strlen(str)<(size_t)ALV_LEN)
                    {
                        strcpy(SendAliveText,str);
                        init[10]=1;
                    }
                }

        /* Interval (seconds) between alive msgs from client
        ***************************************************/
 /*11*/ else if( k_its("RcvAliveInt") )
                {
                    RcvAliveInt = k_int();
                    init[11] = 1;
                }

                /* Text of alive message from client
                ***********************************/
 /*12*/ else if( k_its("RcvAliveText") )
                {
                    str=k_str();
                    if(str && strlen(str)<(size_t)ALV_LEN)
                    {
                        strcpy(RcvAliveText,str);
                        init[12]=1;
                    }
                }

        /* Timeout in milliseconds for IP Socket routines
        ************************************************/
/*13*/  else if(k_its("SocketTimeout") )
                {
                    SocketTimeoutLength = k_int();
          if(SocketTimeoutLength != -1)
          {
            if(SocketTimeoutLength < RcvAliveInt * 1000)
            {
              SocketTimeoutLength = RcvAliveInt * 1000;
            }
          }
          init[13]=1;
        }

        /* Optional cmd: Turn on socket debug logging -> BIG LOG FILES!!
        ***************************************************************/
        else if(k_its("SocketDebug") )
                {
           SOCKET_ewDebug = k_int();
        }

        /* Optional cmd: Turn on debug logging -> BIG LOG FILES!!
        ********************************************************/
        else if( k_its("Verbose") )
                {
                    Verbose = k_int();
                }

        /* Pass it off to the filter's config processor
                **********************************************/
        else if( exportfilter_com() ) strcpy( processor, "exportfilter_com" );

        /* Unknown command
        *****************/
                else
                {
           logit("e", "%s: <%s> Unknown command in <%s>.\n",
                    Argv0, com, configfile );
           continue;
        }

        /* See if there were any errors processing the command
        *****************************************************/
        if( k_err() )
                {
                    logit("e",
                                     "%s: Bad <%s> command for %s() in <%s>; exiting!\n",
                   Argv0, com, processor, configfile );
          exit( -1 );
        }

        } /* while(k_rd()) */

        nfiles = k_close();

    } /* while(nfiles > 0) */

    /* After all files are closed, check init flags for missed commands
    ******************************************************************/
  nmiss = 0;
  for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
  if ( nmiss )
    {
       logit("e", "%s: ERROR, no ", Argv0 );
       if ( !init[0] )  logit("e", "<LogFile> "      );
       if ( !init[1] )  logit("e", "<MyModuleId> "   );
       if ( !init[2] )  logit("e", "<RingName> "     );
       if ( !init[3] )  logit("e", "<HeartBeatInt> " );
       if ( !init[4] )  logit("e", "<GetMsgLogo> "   );
       if ( !init[5] )  logit("e", "<ServerIPAdr> "  );
       if ( !init[6] )  logit("e", "<ServerPort> "   );
       if ( !init[7] )  logit("e", "<MaxMsgSize> "   );
       if ( !init[8] )  logit("e", "<RingSize> "     );
       if ( !init[9] )  logit("e", "<SendAliveInt> " );
       if ( !init[10] ) logit("e", "<SendAliveText>" );
       if ( !init[11] ) logit("e", "<RcvAliveInt> "  );
       if ( !init[12] ) logit("e", "<RcvAliveText>"  );
       if ( !init[13] ) logit("e", "<SocketTimeoutLength>");
       logit("e", "command(s) in <%s>; exiting!\n", configfile );
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
  if( ( RingKey = GetKey(RingName) ) == -1 )
    {
        fprintf( stderr, "%s:  Invalid ring name <%s>; exiting!\n",
                         Argv0, RingName);
        exit( -1 );
  }

    /* Look up installations of interest
  ***********************************/
  if ( GetLocalInst( &InstId ) != 0 )
    {
    fprintf( stderr, "%s: error getting local installation id; exiting!\n",
                         Argv0 );
        exit( -1 );
    }

    /* Look up modules of interest
  *****************************/
  if ( GetModId( MyModName, &MyModId ) != 0 )
    {
      fprintf( stderr,
              "%s: Invalid module name <%s>; exiting!\n",
               Argv0, MyModName );
      exit( -1 );
  }

    /* Look up message types of interest
  ***********************************/
  if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 )
    {
      fprintf( stderr,
              "%s: Invalid message type <TYPE_HEARTBEAT>; exiting!\n", Argv0 );
      exit( -1 );
  }
  if ( GetType( "TYPE_ERROR", &TypeError ) != 0 )
    {
      fprintf( stderr,
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
    char          msg[256];
    long          size;
    time_t        t;

    /* Build the message
    *******************/
    logo.instid = InstId;
    logo.mod    = MyModId;
    logo.type   = type;

    time( &t );

    if( type == TypeHeartBeat )
    {
        sprintf( msg, "%ld %ld\n", (long) t, (long) MyPid);
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
                 logit("et","%s(%s):  Error sending heartbeat.\n",
                                Argv0, MyModName );
            }
            else if( type == TypeError )
            {
                 logit("et", "%s(%s):  Error sending error:%d.\n",
                                Argv0, MyModName, ierr );
            }
    }

    return;
}

/*------------------------------End of File--------------------------------*/

