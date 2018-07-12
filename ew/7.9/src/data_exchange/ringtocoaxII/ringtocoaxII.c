

                /************************************************
                 *                ringtocoaxII.c                *
                 *                                              *
                 *   Gets all messages from a transport ring    *
                 *   and broadcasts them to an ethernet port.   *
                 ************************************************/
/* 4/13/00 Alex:
 *   Moved heartbeat logic into working loop to prevent heart stoppage
 *   during very busy times.
 *
 * 9/30/99 Whitmore:
 *   Added Restart messages to list of messages not broadcast when
 *   configured as such.
 *
 * 4/19/99  Dietz:
 *   Added optional configuration command "BroadcastLogo" so that
 *   one can specify which logos to broadcast.  If this command
 *   is omitted, "broadcast everything" is the default.
 *
 * 11/19/98  V4.0 changes Lombard:
 *   0) no Y2k dates
 *   1) changed argument of logit_init to the config file name.
 *   2) process ID in heartbeat message
 *   3) flush input transport ring: not applicable
 *   4) add `restartMe' to .desc file
 *   5) multi-threaded logit: not applicable
 *
 * 3/29/96  Alex:
 *   Added switch to conrol whether heartbeats are passed or blocked
 */

/* System Libs */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* EW Libs */
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <ew_packet.h>
#include <signal.h>
#include <mem_circ_queue.h>

/* Function prototypes
   *******************/
void UpdatePacketsPerBurst( long );
void ringtocoaxII_config( char * );
void ringtocoaxII_log_config( void );
void ringtocoaxII_lookup( void );
void ringtocoaxII_status( unsigned char, short, char * );
void ringtocoaxII_shutdown( int );
void BroadcastMsg( MSG_LOGO, long, char * );
int  SocketInit( void );
int  SendPacket( PACKET *, long, char *, int );
void SocketShutdown( void );

/* Thread things
 ***************/
#define THREAD_STACK 8192
static unsigned tidSender;            /* Sender thread id */
static unsigned tidStacker;           /* Thread moving messages from transport */
static unsigned tidAdaptive;          /* Thread controlling output rate        */

                                     /* to queue */
#define MSGSTK_OFF    0              /* MessageStacker has not been started      */
#define MSGSTK_ALIVE  1              /* MessageStacker alive and well            */
#define MSGSTK_CHECKALIVE -1         /* Value to set Status To when checking     */
                                     /* Thread Life                              */
#define MSGSTK_ERR   -2              /* MessageStacker encountered error quit    */
volatile int MessageStackerStatus = MSGSTK_OFF;

#define UDPSND_OFF    0              /* UDP Sender has not been started      */
#define UDPSND_ALIVE  1              /* UDP Sender alive and well            */
#define UDPSND_CHECKALIVE -1         /* Value to set Status To when checking     */
                                     /* Thread Life                              */
#define UDPSND_ERR   -2              /* UDP Sender encountered error quit    */
volatile int UPDSenderStatus = UDPSND_OFF;

#define ADAPTIVE_OFF    0            /* Adaptive has not been started      */
#define ADAPTIVE_ALIVE  1            /* Adaptive alive and well            */
#define ADAPTIVE_CHECKALIVE -1       /* Value to set Status To when checking     */
                                     /* Thread Life                              */
#define ADAPTIVE_ERR   -2            /* Adaptive encountered error quit    */
volatile int AdaptiveStatus = ADAPTIVE_OFF;

thr_ret UDPSender( void * );           /* Sends out data via udp broadcasts        */
thr_ret MessageStacker( void * );      /* used to pass messages between main thread */
                                       /*   and UDPSender thread */
thr_ret Adaptive( void * );            /* manages the output rate.        */

QUEUE OutQueue; 		               /* from queue.h, queue.c; sets up linked    */
                                       /*    list via malloc and free              */

static SHM_INFO  Region;               /* Shared memory to use for i/o     */
static char      Attached = 0;         /* Flag=1 when attached to sharedmem*/
static char     *StackerBuffer = NULL; /* Character string to hold msg to be stacked */
static char     *SenderBuffer = NULL;  /* Character string to hold msg to be sent    */
static MSG_LOGO *trackLogo = NULL;     /* Array of logos being tracked     */
static unsigned char *trackSeq = NULL; /* Array of msg sequence numbers    */
static MSG_LOGO *GetLogo = NULL;       /* logo(s) to get from shared memory*/
static short     nGetLogo = 0;         /* # logos we're configured to get  */

/* Things to read or derive from configuration file
   ************************************************/
static char RingName[MAX_RING_STR];     /* Name of transport ring for i/o         */
static char MyModuleId[MAX_MOD_STR];    /* Speak as this module name/id           */
static int  LogSwitch;                  /* 0 if no log file should be written     */
static long HeartBeatInterval;          /* Seconds between heartbeats             */
static long MsgMaxBytes;                /* Maximum message size in bytes          */
static char OutAddress[20];             /* IP address of line of broadcast on     */
static int  OutPortNumber;              /* Well-known port number                 */
static int  MaxLogo;                    /* Maximum number of logos to track       */
static int  BurstInterval;              /* Msec between bursts of packets         */
static int  Debug = 0;                  /* If 1, print debug info on screen       */
                                        /* If 2, print logos as well              */
static int  CopyStatus;                 /* If 1, copy heartbeats                  */
static int  MaxPacketsPerBurst;         /* Max Num of messages per timer interval */
static int  MinPacketsPerBurst = 1;     /* Max Num of messages per timer interval */ 
static int  BurstCount;                 /* Num of messages per timer interval     */
static int  SqrtCount;                  /* Num of sqrt roots between messages     */
static long InputQueueLen;              /* Length of internal queue               */
static int  QueueReportInt = 300;       /* Seconds in between queue size reports  */
static int  ThroughputReportInt = 300;  /* Seconds in between queue size reports  */
static double STAInterval = 1.5;        /* Desired interval to determine short    */
										/* term average over                      */
static double ExcedenceConstant = 1;    /* factor to multiply the excedence by    */
                                        /* to control how much the desired        */
										/* excedence rate can change              */
static double QueueOptimumPercent = .10;/* Optimum Queue Level in percent         */

/* Things to look up in the earthworm.h tables with getutil.c functions
   ********************************************************************/
static long          RingKey;       /* Key of transport ring for i/o     */
static unsigned char InstId;        /* Local installation id             */
static unsigned char MyModId;       /* Module Id for this program        */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypeRestart;

/* Error messages used by this program
   ***********************************/
#define ERR_MISSMSG        0   /* Message missed in transport ring */
#define ERR_TOOBIG         1   /* Retreived msg too large for buffer */
#define ERR_NOTRACK        2   /* Msg retreived; tracking limit exceeded */
#define ERR_SENDPACKET     3   /* Error sending a packet to ethernet */
#define ERR_TOOMANYLOGO    4   /* Number of logos > MaxLogo from config file */
#define ERR_QUEUE          5   /* error queueing message for sending      */
static char Text[150];         /* String for log/error messages */

/* Global to this file
   *******************/
static timer_t timerHandle;    /* Handle for timer object */
static pid_t myPid;            /* for restarts by startstop */

int PacketsPerBurst;

         /***************************************************
          *          The main program starts here           *
          ***************************************************/

int main( int argc, char *argv[] )
{
   time_t          timeNow;        /* Current time */
   time_t          timeLastBeat;   /* Time last heartbeat was sent */
   long          recsize;        /* Size of retrieved message */
   MSG_LOGO      reclogo;        /* Logo of retrieved message */
   unsigned char recseq;         /* transport seq# in input ring */
/*   int           i;
*/
/* Check command line arguments
   ****************************/
   if ( argc != 2 )
   {
      fprintf( stderr, "Usage: ringtocoaxII <configfile>\n" );
      return -1;
   }

/* Initialize name of log-file & open it
   *************************************/
   logit_init( argv[1], 0, 256, 1 );

/* Read the configuration file(s)
   ******************************/
   ringtocoaxII_config( argv[1] );
   logit( "t" , "Read command file <%s>\n", argv[1] );

/* Look up important info from earthworm.h tables
   **********************************************/
   ringtocoaxII_lookup();

/* Log Our configuration to disk
   *****************************/
   ringtocoaxII_log_config();

/* Initialize name of log-file & open it
   *************************************/
   logit_init( argv[1], 0, 256, LogSwitch );

/* Start out at 50% of max packets per burst 
   *****************************************/ 
   PacketsPerBurst = MaxPacketsPerBurst/2;

/* Get process ID for heartbeat messages
   *************************************/
   myPid = getpid();
   if( myPid == -1 )
   {
      logit("e","ringtocoaxII: Cannot get pid; exiting!\n");
      ringtocoaxII_shutdown( -1 );
   }

/* Malloc the message buffers
   **************************/
   StackerBuffer = (char *)malloc( (size_t)(MsgMaxBytes+1) * sizeof(char) );
   if ( StackerBuffer == NULL )
   {
      logit( "e", "ringtocoaxII: malloc error for Stacker message buffer; "
                       "exiting!\n" );
      ringtocoaxII_shutdown( -1 );
   }
   SenderBuffer = (char *)malloc( (size_t)(MsgMaxBytes+1) * sizeof(char) );
   if ( SenderBuffer == NULL )
   {
      logit( "e", "ringtocoaxII: malloc error for Stacker message buffer; "
                       "exiting!\n" );
      ringtocoaxII_shutdown( -1 );
   }

/* Malloc the array of logos to track
   **********************************/
   trackLogo = (MSG_LOGO *)malloc( MaxLogo * sizeof(MSG_LOGO) );
   if ( trackLogo == NULL )
   {
      logit( "e", "ringtocoaxII: malloc error for trackLogo; exiting!\n" );
      ringtocoaxII_shutdown( -1 );
   }

   trackSeq = (unsigned char *)malloc( MaxLogo * sizeof(unsigned char) );
   if( trackSeq == NULL )
   {
      logit( "e", "ringtocoaxII: malloc error for trackSeq; exiting!\n" );
      ringtocoaxII_shutdown( -1 );
   }

/* Malloc the logos to broadcast
   (if no BroadcastLogo commands were in the config file)
   ******************************************************/
   if( nGetLogo == 0 )
   {
      GetLogo = (MSG_LOGO *) malloc( sizeof(MSG_LOGO) );
      if( GetLogo == NULL )
      {
         logit( "e", "ringtocoaxII: malloc error for GetLogo; exiting!\n" );
         ringtocoaxII_shutdown( -1 );
      }
      GetLogo[0].instid = 0;  /* default to 0 = wildcard */
      GetLogo[0].mod    = 0;
      GetLogo[0].type   = 0;
      nGetLogo = 1;
   }

   /* Create a Mutex to control access to queue
    ********************************************/
   CreateMutex_ew();

   /* Initialize the message queue
    *******************************/
   initqueue( &OutQueue, (unsigned long)InputQueueLen,(unsigned long)MsgMaxBytes );

/* Attach to Input/Output shared memory ring
   *****************************************/
   tport_attach( &Region, RingKey );
   Attached = 1;
   logit( "t", "Attached to public memory region %s: %d\n", RingName, RingKey );

/* Flush the incoming transport ring on startup
   ********************************************/
   while ( tport_copyfrom( &Region, GetLogo, nGetLogo, &reclogo, &recsize,
			   StackerBuffer, MsgMaxBytes, &recseq ) != GET_NONE );
 
/* Force a heartbeat to be issued in first pass thru main loop
   ***********************************************************/
   timeLastBeat = time(&timeNow) - HeartBeatInterval - 1;

/* Initialize the socket system for broadcasting
   *********************************************/
   if ( SocketInit() == -1 )
   {
      logit( "et", "SocketInit error; exiting!\n" );
      ringtocoaxII_shutdown( -1 );
   }


/* Start the message stacking thread if it isn't already running.
 ****************************************************************/
   if (MessageStackerStatus != MSGSTK_ALIVE )
   {
      if ( StartThread(MessageStacker, (unsigned)THREAD_STACK, &tidStacker) == -1)
      {
         logit( "et", "Error Starting Message Stacker Thread; Exiting!\n" );
         ringtocoaxII_shutdown( -1 );
      }
      MessageStackerStatus = MSGSTK_ALIVE;
   }

   /* Start the UDP writing thread
    ***********************************/
   if (UPDSenderStatus != UDPSND_ALIVE )
   {
      if ( StartThread(UDPSender, (unsigned)THREAD_STACK, &tidSender )== -1)
      {
         logit( "et", "Error starting UDP Sending thread; exiting!\n");
         ringtocoaxII_shutdown( -1 );
	  }
      UPDSenderStatus = UDPSND_ALIVE;
   }

   /* Start the Adaptive thread
    ***************************/
   if (AdaptiveStatus != ADAPTIVE_ALIVE )
   {
      if ( StartThread(  Adaptive, (unsigned)THREAD_STACK, &tidAdaptive) == -1)
      {
         logit( "et", "Error starting Adaptive thread; exiting!\n");
         ringtocoaxII_shutdown( -1 );
	  }
      AdaptiveStatus = ADAPTIVE_ALIVE;
   }

   /* Start main ringtocoaxII service loop 
    **********************************/
   while((tport_getflag(&Region) != TERMINATE) &&
          (tport_getflag(&Region) != myPid))
   {

      /* Send ringtocoaxII's heartbeat
       ***************************/
       if ( time(&timeNow) - timeLastBeat  >=  HeartBeatInterval )
       {
          if ((MessageStackerStatus == MSGSTK_ALIVE) &&
              (UPDSenderStatus == UDPSND_ALIVE))
		  {
             timeLastBeat = timeNow;
             ringtocoaxII_status( TypeHeartBeat, 0, "" );
		  }
		  else if (MessageStackerStatus == MSGSTK_ERR)
		  {
             logit( "et", "Message Stacker Thread Encountered Error; exiting!\n");
             ringtocoaxII_shutdown( -1 );
		  }
		  else if (MessageStackerStatus == MSGSTK_CHECKALIVE)
		  {
             logit( "et", "Lost Contact with Message Stacker Thread; exiting!\n");
             ringtocoaxII_shutdown( -1 );
		  } 
		  else if (UPDSenderStatus == UDPSND_ERR)
		  {
             logit( "et", "UDP Sender Thread Encountered Error; exiting!\n");
             ringtocoaxII_shutdown( -1 );
		  }
		  else if (UPDSenderStatus == UDPSND_CHECKALIVE)
		  {
             logit( "et", "Lost Contact with UDP Sender Thread; exiting!\n");
             ringtocoaxII_shutdown( -1 );
		  } 
       }

	  /* "Knock down" thread status values to check for thread life
	   ************************************************************/
	   MessageStackerStatus = MSGSTK_CHECKALIVE;
       UPDSenderStatus = UDPSND_CHECKALIVE;
       AdaptiveStatus  = ADAPTIVE_CHECKALIVE;

	  /* take a brief nap
       ******************/
      sleep_ew(500);
   } /*end while of monitoring loop */

   /* Shut it down
   ***************/
   ringtocoaxII_shutdown( -1 );

   return( 0 );	
}


/********************** Message Stacking Thread *******************
 *           Move messages from transport to memory queue         *
 ******************************************************************/
thr_ret MessageStacker( void *dummy )
{
   long          recsize;        /* Size of retrieved message    */
   MSG_LOGO      reclogo;        /* Logo of retrieved message    */
   unsigned char recseq;         /* transport seq# in input ring */
   int 		     tport_res;      /* transport return value       */
   int 		     queue_ret;      /* queue return value           */
   int       NumOfTimesQueueLapped= 0; /* number of messages lost due to 
                                             queue lap */
   /* Start Stacking service loop 
   ******************************/
   while(1)
   {
     /* Tell the main thread we're ok
      ********************************/
      MessageStackerStatus = MSGSTK_ALIVE;

     /* Get a message from the transport ring
      *************************************/
      tport_res = tport_copyfrom(&Region, GetLogo, nGetLogo, &reclogo,
                                 &recsize, StackerBuffer, MsgMaxBytes, &recseq);

     /* Wait if no messages for us 
      ****************************/
      if(tport_res == GET_NONE) 
	  { 
		  sleep_ew(50); 
		  continue;
	  } 
      else if (tport_res == GET_TOOBIG)   /* Next message was too big */
      {                                   /* complain and try again   */
		  logit("e", "Retrieved msg[%ld] (i%u m%u t%u seq%u) too big for Buffer[%d]",
                 recsize, reclogo.instid, reclogo.mod, reclogo.type, recseq,MsgMaxBytes);
          ringtocoaxII_status(TypeError, ERR_TOOBIG, Text);
          continue;
      }
      else if (tport_res == GET_MISS_LAPPED) /* Got a msg, but missed some */
      {
          sprintf(Text, "Missed msgs (overwritten, ring lapped)  i%u m%u t%u seq%u  %s.",
                  reclogo.instid, reclogo.mod, reclogo.type, recseq, RingName);
          ringtocoaxII_status( TypeError, ERR_MISSMSG, Text );
      }
      else if (tport_res == GET_MISS_SEQGAP) /* Got a msg, but saw sequence gap */
      {
          sprintf(Text, "Missed msgs (sequence gap) i%u m%u t%u seq%u  %s.",
                  reclogo.instid, reclogo.mod, reclogo.type, recseq, RingName);
          ringtocoaxII_status(TypeError, ERR_MISSMSG, Text);
      }
      else if (tport_res == GET_NOTRACK)  /* Got a msg, but can't tell */
      {                                   /* if any were missed        */
          sprintf(Text, "Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
                  reclogo.instid, reclogo.mod, reclogo.type );
          ringtocoaxII_status( TypeError, ERR_NOTRACK, Text );
      }
      StackerBuffer[recsize] = '\0';         /* Null-terminate the message */

      /* Is it a heartbeat or error, and should we pass them on,
	   * if not, don't add them to queue.
       *********************************************************/
      if ((CopyStatus == 0) && (reclogo.type == TypeHeartBeat))
		  continue;
      if ((CopyStatus == 0) && (reclogo.type == TypeError))
		  continue;
      if ((CopyStatus == 0) && (reclogo.type == TypeRestart))
		  continue;

      /* Put Message into the 'to be shipped' queue 
	   * the UDPSender thread is in the biz of de-queueng and sending 
	   **************************************************************/
      RequestMutex();
      queue_ret = enqueue(&OutQueue, StackerBuffer, recsize, reclogo); 
      ReleaseMutex_ew();

      if (queue_ret != 0)
      {       
          if (queue_ret == -2)  /* Serious: quit */
          {    
			  /* Currently, eneueue() in mem_circ_queue.c never returns this error. 
			   ********************************************************************/
	          sprintf(Text,"internal queue error. Terminating.");
              ringtocoaxII_status(TypeError, ERR_QUEUE, Text);
	          goto error;
          }
          if (queue_ret == -1) 
          {
              sprintf(Text,"queue cannot allocate memory. Lost message.");
              ringtocoaxII_status(TypeError, ERR_QUEUE, Text);
              continue;
          }
          if (queue_ret == -3)  
          {
			 /* Queue is lapped too often to be logged to screen.  
		      * Log circular queue laps to logfile.  
              * Maybe queue laps should not be logged at all.
              ****************************************************/
              NumOfTimesQueueLapped++;
              if (!(NumOfTimesQueueLapped % 5))
			  {
                 logit("t", "Circular queue lapped 5 times. Messages lost.\n");
			     if (!(NumOfTimesQueueLapped % 100))
				 {
                    logit("et", "Circular queue lapped 100 times. Messages lost.\n");
				    sprintf(Text,"Circular queue lapped 100 times. Messages lost.\n");
				    ringtocoaxII_status(TypeError, ERR_QUEUE, Text);
				 }
			  }
              continue; 
          }
	  }

   } /* end of while */

   /* we're quitting 
   *****************/
error:
   /* file a complaint to the main thread 
    *************************************/
   MessageStackerStatus = MSGSTK_ERR; 
   /* main thread will restart us 
    ******************************/
   KillSelfThread(); 
}


/************************* UDP Sending Thread *********************
 *       Send Messages from memory queue out as udp messages      *
 ******************************************************************/
thr_ret UDPSender( void *dummy )
{
   long      recsize;	  /* size of retrieved message */
   MSG_LOGO  reclogo;     /* logo of retrieved message */
   int 		 queue_ret;
/*   long		 SizeOfQueue;
*/
  /* Tell the main thread we're ok
   ********************************/
   UPDSenderStatus = UDPSND_ALIVE;

   /* Start Stacking service loop 
    ******************************/
   while( 1 )
   {
     /* Tell the main thread we're ok
      ********************************/
      UPDSenderStatus = UDPSND_ALIVE;

     /* Get a message from queue
	  **************************/
      RequestMutex();
	  queue_ret=dequeue( &OutQueue, SenderBuffer, &recsize, &reclogo);
/*	  SizeOfQueue = OutQueue.NumOfElements;
*/	  ReleaseMutex_ew();

	 /* Using the current queue size, update
	  * the packets per burst value appropriatly
	  ******************************************/
/*	  UpdatePacketsPerBurst(SizeOfQueue);
*/
      if(queue_ret < 0 )
	  { 
	     /* -1 means empty queue 
	      **********************/
	     sleep_ew(50); 
	     continue;
	  }

     /* Split the message into packets.
      * Then, broadcast packets onto network.
      ***************************************/
      BroadcastMsg( reclogo, recsize, SenderBuffer );

   } /* end of while */

   /* we're quitting 
   *****************/

   /* file a complaint to the main thread 
    *************************************/
   UPDSenderStatus = UDPSND_ERR; 
   /* main thread will restart us 
    ******************************/
   KillSelfThread(); 
}


/************************* Adaptive Thread *************************
 *   Modify the output rate depending on the current queue state   *
 *******************************************************************/
thr_ret Adaptive( void *dummy )
{
   long		 SizeOfQueue;

  /* Tell the main thread we're ok
   ********************************/
   AdaptiveStatus = ADAPTIVE_ALIVE;

   /* Start Stacking service loop 
    ******************************/
   while( 1 )
   {
     /* Tell the main thread we're ok
      ********************************/
      AdaptiveStatus = ADAPTIVE_ALIVE;

     /* Check the queue size
	  **********************/
      RequestMutex();
	  SizeOfQueue = OutQueue.NumOfElements;
	  ReleaseMutex_ew();

	 /* Using the current queue size, update
	  * the packets per burst value appropriatly
	  ******************************************/
	  UpdatePacketsPerBurst(SizeOfQueue);

	  /* Sleep a little while after last check
	   ***************************************/
      sleep_ew(10); 

   } /* end of while */

   /* we're quitting 
   *****************/

   /* file a complaint to the main thread 
    *************************************/
   AdaptiveStatus = ADAPTIVE_ERR; 
   /* main thread will restart us 
    ******************************/
   KillSelfThread(); 
}

  /*******************************************************************
   *                          BroadcastMsg()                         *
   *                                                                 *
   *   Split a message into packets.  Then, broadcast onto network.  *
   *   The sequence numbers of messages are stored by logo in the    *
   *   array trackSeq[].                                             *
   *******************************************************************/

void BroadcastMsg( MSG_LOGO logo, long msgSize, char *message )
{
   static int    packetCounter = 0;
   static int    nlogo = 0;
   int           i, j;
   int           lenSent;
   int           msgBytesToSend;
   int           packetBytes;
   int           packetDataBytes;
   PACKET        packet;
   unsigned char fragNum;
   unsigned char msgSeqNum;
   unsigned char lastOfMsg;
   char          *packetDataStart;

  /* Timers for throughput checks/reports
   **************************************/
   time_t          ThroughputCheckTimer = 0;
   static time_t   LastThroughputCheck  = 0;
   int             ThroughputCheckInt   = 1;
   time_t          ThroughputReportTimer = 0;
   static time_t   LastThroughputReport  = 0;


  /* Counters for throughput checks/reports
   ****************************************/
   static int packetcount = 0;
   static int burstcount = 1;
   static int packetsperburstcount = 0;
   static unsigned long DataBytesCount = 0;
   
  /* Get the index (j) of the next message
   * sequence number for this logo
   ***************************************/
   j = -1;                            /* -1 if not found */
   for (i = 0; i < nlogo; i++)
      if ( logo.type   == trackLogo[i].type &&
           logo.mod    == trackLogo[i].mod  &&
           logo.instid == trackLogo[i].instid )
      {
         j = i;
         break;
      }

  /* We fell through the loop.
   * This is a new logo.
   **************************/
   if (j == -1)
   {
      if ( nlogo == MaxLogo )
      {
         sprintf( Text, "Number of logo types exceeds the max= %d\n", MaxLogo );
         ringtocoaxII_status( TypeError, ERR_TOOMANYLOGO, Text );
         return;
      }

      j = nlogo;
      trackLogo[j] = logo;
      trackSeq[j]  = 0;
      nlogo++;
   }
   msgSeqNum = trackSeq[j]++;

  /* Print some header values
   ************************/
   if (Debug == 2)
   {
      fprintf(stderr, "sending: ");
      fprintf(stderr, "nlogo:%d",  nlogo);
      fprintf(stderr, " inst:%u",  logo.instid);
      fprintf(stderr, " mid:%2u",  logo.mod);
      fprintf(stderr, " typ:%2u",  logo.type);
      fprintf(stderr, " len:%5ld", msgSize);
      fprintf(stderr, " cseq:%3u", msgSeqNum);
      fprintf(stderr, "\n");
   }

  /* Split the message into packets with a maximum size of 1472 bytes
   ******************************************************************/
   msgBytesToSend  = msgSize;
   fragNum         = 0;
   packetDataStart = message;

   do
   {
     /* Set packet header values
      **************************/
      if ( msgBytesToSend > UDP_DAT )
      {
         packetDataBytes = UDP_DAT;
         lastOfMsg = 0;
      }
      else
      {
         packetDataBytes = msgBytesToSend;
         lastOfMsg = 1;
      }
      msgBytesToSend -= packetDataBytes;

      packet.msgInst   = logo.instid; /* Installation id */
      packet.msgType   = logo.type;   /* Message Type */
      packet.modId     = logo.mod;    /* Id of module originating message */
      packet.fragNum   = fragNum++;   /* Packet number of message; 0=>first */
      packet.msgSeqNum = msgSeqNum;   /* Message Sequence number */
      packet.lastOfMsg = lastOfMsg;   /* 1=> last packet of message, else 0 */

     /* Copy data to the packet cargo bay
      ***********************************/
      memcpy(packet.text, packetDataStart, packetDataBytes);
	  packetDataStart += packetDataBytes;

    /* Broadcast the packet onto the network
      ***************************************/
      packetBytes = packetDataBytes + UDP_HDR;
      DataBytesCount += packetDataBytes;

      lenSent = SendPacket( &packet, packetBytes, OutAddress, OutPortNumber );
		
      if ( lenSent != packetBytes )
      {
         sprintf( Text, "Error sending packet. lenSent: %d  packetBytes: %d\n",
                lenSent, packetBytes );
         ringtocoaxII_status( TypeError, ERR_SENDPACKET, Text );
      }

	 /* Increment the number of packets sent
	  * thus far in this burst
	  **************************************/
	  packetCounter++;

	 /* Increment total packets counter
	  *********************************/
	  packetcount++;

     /* Check to see if we've hit the end of a burst
      ************************************************/
      if (packetCounter >= PacketsPerBurst)
      {
		/* Reset the number of packets sent thus far
		 *******************************************/
         packetCounter = 0;

		/* Increment total bursts counter
		 ********************************/
		 burstcount++;

        /* Check our current PacketsPerBurst level
         *****************************************/
         if ( time(&ThroughputCheckTimer) - LastThroughputCheck  >=  ThroughputCheckInt )
		 {
		    packetsperburstcount += PacketsPerBurst;
	        LastThroughputCheck = ThroughputCheckTimer;
		 }

		 /* Sleep the required ammount of time between
		  * bursts
		  ********************************************/
         if ( BurstInterval > 0 )
            sleep_ew( BurstInterval );
      }
      else
      {
        /* Still in current burst, wait a 
		 * really really tiny ammount
		 * Given todays processors/operating systems
		 * this code block is suspect.
		 *******************************************/
         int i;
         for ( i = 0; i < SqrtCount; i++ )
         {
            double a;
            a = sqrt( (double)i );
         }
      }
	  
     /* Report our current queue level to screen / disk
      *************************************************/
      if ((time(&ThroughputReportTimer) - LastThroughputReport) >= ThroughputReportInt)
	  {
         logit("et", "ringtocoaxII:");
		 logit("e", "\nThroughput Report:");
		 logit("e", "\n\tAverage Packets Per Second Sent = %5.2f",
			   (((double)packetcount) / ((double)ThroughputReportInt)));
		 logit("e", "\n\tAverage Bursts Per Second Sent  = %5.2f",
			   (((double)burstcount) / ((double)ThroughputReportInt)));
		 logit("e", "\n\tAverage Bytes Per Second Sent   = %5.2f",
			   (((double)DataBytesCount) / ((double)ThroughputReportInt)));
		 logit("e", "\n\tAverage Packets Per Burst       = %5.2f",
			   (((double)packetsperburstcount) / ((double)ThroughputReportInt)));
		 logit("e", "\n\tUDP Bytes Per Packet            = %d\n\n", 
			   UDP_DAT);
       
	     LastThroughputReport = ThroughputReportTimer;

        /* Reset counters
	     ****************/
	     packetcount = 0;
         burstcount = 1;
         DataBytesCount = 0;
		 packetsperburstcount = 0;
	  }

   } while ( msgBytesToSend > 0 );

   return;
}


  /*******************************************************************
   *                      UpdatePacketsPerBurst()                    *
   *                                                                 *
   *   Using the current queue size, determine the appropriate       *
   *   packets per burst up to maximum / down to minimum to keep     *
   *   the queue from overflowing.                                   *
   *******************************************************************/

void UpdatePacketsPerBurst(long CurrentSizeOfQueue)
{

  /* The algorithm for determining the PacketsPerBurst is as follows:
   *
   *   From the utilization level of the queue we can detect both changes 
   * in the data input, which show up as surges/drops in the queue level, 
   * and resource starvation, which show up as slow rises/falls in the 
   * queue level.
   *
   *   The goal in this algorithm is to survive, aka not loose data, as 
   * best we can in both cases.
   *
   *   To handle the case of resource starvation we take a short term average 
   * (user configured time window) of the queue level.  We then determine a 
   * desired output rate by dividing the short term average by the total queue 
   * size, and multiplying this value by the maximum PacketsPerBurst. 
   *      I.E. (STA / MaxQueueSize) * MaxPacketsPerBurst.
   *
   *   To handle the case of an input surge we take a measure of the percent 
   * utilization of the queue as each message comes off the queue.  This value is 
   * then subtracted by the desired percent queue utilization to get percent 
   * that the current queue exceeds the desired level.  We then determine a 
   * desired output rate by multiplying the excedence by control constant, and 
   * then by the maximum PacketsPerBurst. 
   *      I.E. (Excedence * ExcedenceConstant) * MaxPacketsPerBurst.
   * 
   *   Now, we add the two rates together to determine our desired PacketsPerBurst.  
   * The short term average rate will damp out any wild changes in data rate, while 
   * the excedence rate will both maintain the data rate at a level that keeps the 
   * queue near the desired percent utilization and will react quickly in case of a 
   * surge.  We then check the resulting PacketsPerBurst against the minimum and 
   * maximum bounds, and set PacketsPerBurst.
   *
   * JMP & AB 06/08/2005
   ********************************************************************************/

  /* Timers for queue reports
   **************************/
   time_t          QueueReportTimer = 0;
   static time_t   LastQueueReport = 0;

  /* Timers for queue size checks reports
   **************************************/
   time_t          QueueCheckTimer = 0;
   static time_t   LastCheckReport = 0;
   int			   QueueCheckInt = 1;

  /* Counters for queue checks
   ***************************/
   static long QueueLengthcount = 0;
   double avgtemp;
   static double excedenceavg;

  /* Clock timers for Short Term Average
   *************************************/
   clock_t        STA_Check_Timer = 0;
   static clock_t STA_Last_Check = 0;

  /* Clock timers for Percent Excedence
   *************************************/
   clock_t        Excedence_Check_Timer = 0;
   static clock_t Excedence_Last_Check = 0;

  /* Counters for Short Term Average
   *********************************/
   static double STA_Clock_Delta = 0;
   static long   STA_Queue_Count = 0;
   static double ShortTermAverage = 0;
   static long   STA_NumAdded = 0;
   static double  STA_Queue_Average = 0;
   double MaxQueueSize = InputQueueLen;

  /* Varibles for Percent Excedence
   ********************************/
   static double Excedence_Clock_Delta = 0;
   static long   ExcedenceQueueCount = 0;
   static long   ExcedenceNumAdded = 0; 
   static double ExcedenceQueueAverage = 0; 
   static double QueuePercentFull = 0;
   static double Excedence = 0;

  /* Rate Varibles
   ***************/
   static double DesiredStaRate = 0;
   static double DesiredExcedenceRate = 0;
   static double DesiredPacketsPerBurst = 0;

  /* Increment Short Term Average counters, and 
   * check time to see if it's time to compute
   * the Short term average again
   ********************************************/
   STA_Check_Timer = clock();
   STA_Clock_Delta = (STA_Check_Timer - STA_Last_Check) / ((double)CLOCKS_PER_SEC);
   STA_Queue_Count += CurrentSizeOfQueue;
   STA_NumAdded++;

   if (STA_Clock_Delta >= STAInterval)
   {
      /* Compute Short Term Average of the Queue level
       ***********************************************/
	   STA_Last_Check = STA_Check_Timer;
	   ShortTermAverage = ((double)STA_Queue_Count) / ((double)STA_NumAdded);
	   STA_Queue_Average = ShortTermAverage / MaxQueueSize;
	   STA_Queue_Count = 0;
	   STA_NumAdded = 0;
   }


  /* Increment Excedence counters, and 
   * check time to see if it's time to compute
   * the Excedence
   ********************************************/
   Excedence_Check_Timer = clock();
   Excedence_Clock_Delta = (Excedence_Check_Timer - Excedence_Last_Check) / ((double)CLOCKS_PER_SEC);
//   ExcedenceQueueCount += CurrentSizeOfQueue;
   ExcedenceQueueAverage += ((double)CurrentSizeOfQueue) / MaxQueueSize;
   ExcedenceNumAdded++;

  /* Excedence is computed 100 times more often than
   * STA
   ************************************************/
   if (Excedence_Clock_Delta >= (STAInterval / 10))
   {
      /* Compute Excedence of the Queue level
       ***********************************************/
	   Excedence_Last_Check = Excedence_Check_Timer;

      /* Compute the Percent Excedence of the queue level  
       * over the optimum queue level
       **************************************************/
	   //ExcedenceQueueAverage = ((double)ExcedenceQueueCount) / ((double)ExcedenceNumAdded);

	   //QueuePercentFull = ExcedenceQueueAverage / MaxQueueSize;

       QueuePercentFull = ExcedenceQueueAverage / ((double)ExcedenceNumAdded);
	   Excedence = QueuePercentFull - QueueOptimumPercent;

/*	   logit("e", "ExcedenceQueueCount = %d, ExcedenceNumAdded = %d. MaxQueueSize = %f, QueuePercentFull = %f, QueueOptimumPercent = %f => Excedence = %f\n", 
		     ExcedenceQueueCount, ExcedenceNumAdded, MaxQueueSize, QueuePercentFull, QueueOptimumPercent, Excedence);
*/
	   //ExcedenceQueueCount = 0;
	   ExcedenceQueueAverage = 0;
	   ExcedenceNumAdded = 0;
   }

  /* Compute New Desired Packets Per Burst
   ***************************************/
   DesiredStaRate = MaxPacketsPerBurst * STA_Queue_Average;
   DesiredExcedenceRate = (Excedence * ExcedenceConstant) * MaxPacketsPerBurst;
   DesiredPacketsPerBurst = DesiredStaRate + DesiredExcedenceRate;

   if (DesiredPacketsPerBurst > MaxPacketsPerBurst)
   {
       PacketsPerBurst = MaxPacketsPerBurst;
   }
   else if (DesiredPacketsPerBurst < MinPacketsPerBurst)
   {
       PacketsPerBurst = MinPacketsPerBurst;
   }
   else
   {  
       PacketsPerBurst = (int)DesiredPacketsPerBurst;
   }


   /* Check our current queue level
    *******************************/
   if ( time(&QueueCheckTimer) - LastCheckReport  >=  QueueCheckInt )
   {
	   QueueLengthcount += CurrentSizeOfQueue;
	   LastCheckReport = QueueCheckTimer;
   }

   /* Report our current queue level to screen / disk
    *************************************************/
   if ( time(&QueueReportTimer) - LastQueueReport  >=  QueueReportInt )
   {
	   avgtemp = ((double)QueueLengthcount) / ((double)QueueReportInt);

       logit("et", "ringtocoaxII:");
	   logit("e",  "\nQueue Report:");
	   logit("e",  "\n\tAverage Queue Utilization       = %2.2f%% ( %5.2f / %5.0f)", 
		     (avgtemp / MaxQueueSize) * 100.0,
		     avgtemp,
			 MaxQueueSize-1);
       
	   logit("e", "\n\tQueue Short Term Average         = %5.2f", ShortTermAverage);
	   logit("e", "\n\tQueue Short Term Average Percent = %3.2f%%", STA_Queue_Average*100);
	   logit("e", "\n\tQueue Current Percent Full       = %3.2f%%", QueuePercentFull*100);
	   logit("e", "\n\tQueue Optimum Percent            = %3.2f%%", QueueOptimumPercent*100);
	   logit("e", "\n\tQueue Percent Excedence          = %3.2f%%", Excedence*100);
	   logit("e", "\n\tDesired Short Term Average Rate  = %5.2f", DesiredStaRate);
	   logit("e", "\n\tDesired Excedence Rate           = %5.2f", DesiredExcedenceRate);
	   logit("e", "\n\tDesired Packets Per Burst        = %5.2f", DesiredPacketsPerBurst);
	   logit("e", "\n\tActual Packets Per Burst         = %d.\n\n", PacketsPerBurst);

	   LastQueueReport = QueueReportTimer;

       QueueLengthcount = 0;
   }
}

 /************************************************************************
  *                          ringtocoaxII_config()                       *
  *                                                                      *
  *           Processes command file(s) using kom.c functions.           *
  *                  Exits if any errors are encountered.                *
  ************************************************************************/
#define ncommand 13      /* # of required commands you expect to process   */

void ringtocoaxII_config( char *configfile )
{
   char   init[ncommand]; /* Init flags, one byte for each required command */
   int    nmiss;          /* Number of required commands that were missed   */
   char  *com;
   char  *str;
   int    nfiles;
   int    success;
   int    i;

/* Set to zero one init flag for each required command
   ***************************************************/
   for ( i = 0; i < ncommand; i++ )
      init[i] = 0;

/* Open the main configuration file
   ********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 )
   {
        logit( "e", 
                "ringtocoaxII: Error opening command file <%s>; exiting!\n",
                 configfile );
        exit( -1 );
   }

/* Process all command files
   *************************/
   while ( nfiles > 0 )       /* While there are command files open */
   {
      while ( k_rd() )        /* Read next line from active file  */
      {
         com = k_str();       /* Get the first token from line */

 /* Ignore blank lines & comments
    *****************************/
         if ( !com )          continue;
         if ( com[0] == '#' ) continue;

 /* Open a nested configuration file
    ********************************/
         if ( com[0] == '@' )
         {
            success = nfiles + 1;
            nfiles  = k_open( &com[1] );
            if ( nfiles != success )
            {
               logit( "e", 
                 "ringtocoaxII: Error opening command file <%s>; exiting!\n",
                  &com[1] );
               exit( -1 );
            }
            continue;
         }

/* Process anything else as a command
   **********************************/
         if ( k_its( "LogSwitch" ) )
         {
            LogSwitch = k_int();
            init[0] = 1;
         }
         else if ( k_its( "MyModuleId" ) )
         {
            str = k_str();
            if (str) strcpy( MyModuleId, str );
            init[1] = 1;
         }
         else if ( k_its( "RingName" ) )
         {
            str = k_str();
            if (str) strcpy( RingName, str );
            init[2] = 1;
         }
         else if ( k_its( "HeartBeatInterval" ) )
         {
            HeartBeatInterval = k_long();
            init[3] = 1;
         }
         else if ( k_its( "MsgMaxBytes" ) )
         {
            MsgMaxBytes = k_int();
            init[4] = 1;
         }
         else if ( k_its( "OutAddress" ) )
         {
            str = k_str();
            if (str) strcpy( OutAddress, str );
            init[5] = 1;
         }
         else if ( k_its( "OutPortNumber" ) )
         {
            OutPortNumber = k_int();
            init[6] = 1;
         }
         else if ( k_its( "MaxLogo" ) )
         {
            MaxLogo = k_int();
            init[7] = 1;
         }
         else if ( k_its( "BurstInterval" ) )
         {
            BurstInterval = k_int();
            init[8] = 1;
         }
         else if ( k_its( "CopyStatus" ) )
         {
            CopyStatus = k_int();
            if(CopyStatus == 0 || CopyStatus == 1) init[9] = 1;
         }
         else if ( k_its( "MaxPacketsPerBurst" ) )
         {
            MaxPacketsPerBurst = k_int();
            init[10] = 1;
         }
         else if ( k_its( "SqrtCount" ) )
         {
            SqrtCount = k_int();
            init[11] = 1;
         }
		 else if( k_its("InputQueueLen") ) 
		 {
		    InputQueueLen = k_long();               
            init[12] = 1;
		 }
         else if ( k_its( "Debug" ) )          /*OPTIONAL*/
         {
            Debug = k_int();
         }
         else if (k_its("MinPacketsPerBurst")) /*OPTIONAL*/
         {
            MinPacketsPerBurst = k_int();
         }
         else if (k_its("QueueReportInt"))     /*OPTIONAL*/
         {
            QueueReportInt = k_int();
         }
         else if (k_its("ThroughputReportInt"))/*OPTIONAL*/
         {
            ThroughputReportInt = k_int();
         }
         else if (k_its("STAInterval"))/*OPTIONAL*/
         {
            STAInterval = k_val();
         }
         else if (k_its("ExcedenceConstant"))/*OPTIONAL*/
         {
            ExcedenceConstant = k_val();
         }
         else if (k_its("QueueOptimumPercent"))/*OPTIONAL*/
         {
            QueueOptimumPercent = k_val();
			/* accept percents eigther in decimal (.10) or
			 * integer (10) versions
			 *********************************************/
			if (QueueOptimumPercent > 1)
			{
				QueueOptimumPercent = QueueOptimumPercent / 100;
			}
         }



         else if (k_its("BroadcastLogo"))      /*OPTIONAL*/
         {
            MSG_LOGO *tlogo = NULL;
            tlogo = (MSG_LOGO *)realloc( GetLogo, (nGetLogo+1)*sizeof(MSG_LOGO) );
            if( tlogo == NULL )
            {
               logit( "e", "ringtocoaxII: BroadcastLogo: error reallocing"
                               " %d bytes; exiting!\n",
                       (nGetLogo+1)*sizeof(MSG_LOGO) );
               exit( -1 );
            }
            GetLogo = tlogo;

            if( ( str = k_str() ) )
            {
               if ( GetInst( str, &(GetLogo[nGetLogo].instid) ) != 0 )
               {
                  fprintf( stderr,
                          "ringtocoaxII: BroadcastLogo: invalid installation `%s'"
                          " in `%s'; exiting!\n", str, configfile );
                  exit( -1 );
               }
            }
            if( ( str = k_str() ) )
            {
               if ( GetModId( str, &(GetLogo[nGetLogo].mod) ) != 0 )
               {
                  fprintf( stderr,
                          "ringtocoaxII: BroadcastLogo: invalid module id `%s'"
                          " in `%s'; exiting!\n", str, configfile );
                  exit( -1 );
               }
            }
            if( ( str = k_str() ) )
            {
               if ( GetType( str, &(GetLogo[nGetLogo].type) ) != 0 )
               {
                  logit( "e",
                          "ringtocoaxII: BroadcastLogo: invalid msgtype `%s'"
                          " in `%s'; exiting!\n", str, configfile );
                  exit( -1 );
               }
            }
            nGetLogo++;
         }

/* Unknown command
   ***************/
         else
         {
             logit( "e", "ringtocoaxII: `%s' Unknown command in `%s'.\n",
                      com, configfile );
             continue;
         }

/* See if there were any errors processing the command
   ***************************************************/
         if ( k_err() )
         {
            logit( "e", "ringtocoaxII: Bad `%s' command in `%s'; exiting!\n",
                    com, configfile );
            exit( -1 );
         }
      }
      nfiles = k_close();
   }

/* After all files are closed, check init flags for missed commands
   ****************************************************************/
   nmiss = 0;
   for ( i = 0; i < ncommand; i++ )
      if ( !init[i] )
         nmiss++;

   if ( nmiss )
   {
      logit( "e", "ringtocoaxII: ERROR, no " );
      if ( !init[0] )  logit( "e", "<LogSwitch> " );
      if ( !init[1] )  logit( "e", "<MyModuleId> " );
      if ( !init[2] )  logit( "e", "<RingName> " );
      if ( !init[3] )  logit( "e", "<HeartBeatInterval> " );
      if ( !init[4] )  logit( "e", "<MsgMaxBytes> " );
      if ( !init[5] )  logit( "e", "<OutAddress> " );
      if ( !init[6] )  logit( "e", "<OutPortNumber> " );
      if ( !init[7] )  logit( "e", "<MaxLogo> " );
      if ( !init[8] )  logit( "e", "<BurstInterval> " );
      if ( !init[9] )  logit( "e", "<CopyStatus> (or invalid CopyStatus value)" );
      if ( !init[10])  logit( "e", "<MaxPacketsPerBurst> " );
      if ( !init[11])  logit( "e", "<SqrtCount> " );
      if ( !init[12])  logit( "e", "<InputQueueLen> " );
      logit( "e", "command(s) in <%s>; exiting!\n", configfile );
      exit( -1 );
   }
   return;
}

 /***********************************************************************
  *                              LogConfig()                            *
  *                                                                     *
  *                   Log the configuration parameters                  *
  ***********************************************************************/

void ringtocoaxII_log_config( void )
{
   int i;

   logit( "", "ringtocoaxII Configuration:\n" );
   logit( "", "MyModuleId:          %s => %2u\n", MyModuleId, MyModId);
   logit( "", "RingName:            %s => %d\n", RingName, RingKey);
   logit( "", "HeartBeatInterval:   %d \n", HeartBeatInterval);
   logit( "", "LogSwitch:           %d \n", LogSwitch);
   logit( "", "Debug:               %d \n", Debug);

   logit( "", "MsgMaxBytes:         %d \n", MsgMaxBytes);
   logit( "", "CopyStatus:          %d \n", CopyStatus);
   logit( "", "MaxLogo:             %d \n", MaxLogo);
   for(i = 0; i < nGetLogo; i++ ) 
   {
      logit( "", "GetLogo[%d]:          i%u m%u t%u\n", i,
            GetLogo[i].instid, GetLogo[i].mod, GetLogo[i].type);
   }

   logit( "", "OutAddress:          %s \n", OutAddress);
   logit( "", "OutPortNumber:       %d \n", OutPortNumber);

   logit( "", "InputQueueLen:       %d \n", InputQueueLen);
   logit( "", "QueueOptimumPercent: %2.2f \n", QueueOptimumPercent*100);
   logit( "", "ExcedenceConstant:   %2.2f \n", ExcedenceConstant);
   logit( "", "STAInterval:         %2.2f \n", STAInterval);
   logit( "", "QueueReportInt:      %d \n", QueueReportInt);

   logit( "", "MaxPacketsPerBurst:  %d \n", MaxPacketsPerBurst);
   logit( "", "MinPacketsPerBurst:  %d \n", MinPacketsPerBurst);
   logit( "", "SqrtCount:           %d \n", SqrtCount);
   logit( "", "BurstInterval:       %d \n", BurstInterval);
   logit( "", "ThroughputReportInt: %d \n", ThroughputReportInt);
   logit( "", "\n");

   return;
}


  /****************************************************************
   *                      ringtocoaxII_lookup()                   *
   *                                                              *
   *        Look up important info from earthworm.h tables        *
   ****************************************************************/

void ringtocoaxII_lookup( void )
{
/* Look up keys to shared memory regions
   *************************************/
   if( ( RingKey = GetKey(RingName) ) == -1 )
   {
        fprintf( stderr,
          "ringtocoaxII:  Invalid ring name <%s>; exiting!\n", RingName);
        exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 )
   {
      fprintf( stderr,
        "ringtocoaxII: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModuleId, &MyModId ) != 0 )
   {
      fprintf( stderr,
        "ringtocoaxII: Invalid module name <%s>; exiting!\n", MyModuleId );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 )
   {
      fprintf( stderr,
              "ringtocoaxII: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }

   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 )
   {
      fprintf( stderr,
              "ringtocoaxII: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }

   if ( GetType( "TYPE_RESTART", &TypeRestart ) != 0 )
   {
      fprintf( stderr,
              "ringtocoaxII: Invalid message type <TYPE_RESTART>; exiting!\n" );
      exit( -1 );
   }
   return;
}


  /****************************************************************
   *                      ringtocoaxII_status()                     *
   *                                                              *
   *      Builds a heartbeat or error message & puts it into      *
   *      shared memory.  Writes errors to log file & screen.     *
   ****************************************************************/

void ringtocoaxII_status( unsigned char type, short ierr, char *note )
{
   MSG_LOGO logo;
   char     msg[256];
   long     size;
   time_t   t;

/* Build the message
   *****************/
   logo.instid = InstId;
   logo.mod    = MyModId;
   logo.type   = type;

   time( &t );

   if( type == TypeHeartBeat )
      sprintf( msg, "%ld %d\n\0", (long) t, (int) myPid);

   else if( type == TypeError )
   {
      sprintf( msg, "%ld %hd %s\n\0", (long) t, ierr, note);
      logit( "et", "ringtocoaxII: %s\n", note );
   }

   size = strlen( msg );   /* don't include the null byte in the message */

/* Write the message to shared memory
   **********************************/
   if( tport_putmsg( &Region, &logo, size, msg ) != PUT_OK )
   {
      if( type == TypeHeartBeat )
         logit("et","ringtocoaxII:  Error sending heartbeat.\n" );

      else if( type == TypeError )
         logit("et","ringtocoaxII:  Error sending error:%d.\n", ierr );
   }

   return;
}

 /************************************************************************
  *                        ringtocoaxII_shutdown()                         *
  *                                                                      *
  *    Shuts down politely (frees memory, detaches from rings, etc.)     *
  ************************************************************************/
void ringtocoaxII_shutdown( int estatus )
{
   if( StackerBuffer ) free( StackerBuffer );
   if( SenderBuffer ) free( SenderBuffer );
   if( trackLogo ) free( trackLogo );
   if( trackSeq  ) free( trackSeq  );
   if( GetLogo   ) free( GetLogo   );
   if( Attached  ) tport_detach( &Region );   /* Detach from shared memory */
   SocketShutdown();                          /* close socket */
   exit( estatus );
}

