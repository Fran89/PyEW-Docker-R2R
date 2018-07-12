

                /************************************************
                 *                config_ws.c                   *
                 *                                              *
                 *   Listens to all trace messages on a ring    *
                 *   and generates a coherant set of tank files *
                 ************************************************/

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
#include <swap.h>
#include <trace_buf.h>
#include <mem_circ_queue.h>

typedef struct {
	TRACE2_HEADER TrHdr;
        MSG_LOGO      logo;
	int           maxMsg;
	int           minMsg;
	double        totalMsgTime;
	int           nMsg;
} TABLE ;

TABLE *SCNLtable;
int TableLast = 0;	/*Next available entry */


#define HDSLEN 100
typedef struct {
	int    hd_size;
	char   hd_path[512];
	char   hd_name[64];
	int    hd_target_size;
} HDS ;

HDS HDStruct[HDSLEN];
int HDSLast = 0;	/*Next available entry */

/* Function prototypes
   *******************/
void config_ws_config( char * );
void config_ws_log_config( void );
void config_ws_lookup( void );
void config_ws_status( unsigned char, short, char * );
void config_ws_shutdown( int );
int  FindSCNL( TABLE* , TRACE2_HEADER*, MSG_LOGO* );
int  AddSCNL(TABLE* , TRACE2_HEADER*, int,  MSG_LOGO* );
void UpdateSCNL(TABLE*, TRACE2_HEADER* , int , int );
int  RewriteFile(TABLE* );
int  compare( const void*, const void* );
void DumpToDisk( TABLE* );
void LoadFromDisk( TABLE*);
long SumMB( TABLE*);
long ComputeTankSize(TABLE);
int  ComputeRecordSize(TABLE);
void GenerateTankFile( TABLE* , int * , HDS , int );
void WriteTankLine( TABLE , HDS , FILE *, int , int );

static mutex_t QueueMutex;
static mutex_t TableMutex;

/* Thread things
 ***************/
#define THREAD_STACK 8192
static unsigned tidHandler;           /* handler thread id */
static unsigned tidStacker;           /* Thread moving messages from transport */
static unsigned tidInteractive;       /* Thread handleing interactions     */

                                     /* to queue */
#define MSGSTK_OFF    0              /* MessageStacker has not been started      */
#define MSGSTK_ALIVE  1              /* MessageStacker alive and well            */
#define MSGSTK_CHECKALIVE -1         /* Value to set Status To when checking     */
                                     /* Thread Life                              */
#define MSGSTK_ERR   -2              /* MessageStacker encountered error quit    */
volatile int MessageStackerStatus = MSGSTK_OFF;

#define HANDLER_OFF    0              /* Handler has not been started      */
#define HANDLER_ALIVE  1              /* Handler alive and well            */
#define HANDLER_CHECKALIVE -1         /* Value to set Status To when checking     */
                                      /* Thread Life                              */
#define HANDLER_ERR   -2              /* Handler encountered error quit    */
volatile int MessageHandlerStatus = HANDLER_OFF;

#define INTERACTIVE_OFF    0              /* Interactive has not been started      */
#define INTERACTIVE_ALIVE  1              /* Interactive alive and well            */
#define INTERACTIVE_CHECKALIVE -1         /* Value to set Status To when checking     */
                                          /* Thread Life                              */
#define INTERACTIVE_ERR   -2              /* Interactive encountered error quit    */
#define INTERACTIVE_SHUTDOWN   -3         /* user wants shutdown quit    */
volatile int InteractiveStatus = INTERACTIVE_OFF;

thr_ret MessageHandler( void * );      /* Handles the data        */
thr_ret Interactive( void * );         /* Handles the data        */
thr_ret MessageStacker( void * );      /* used to pass messages between main thread */
                                       /*   and Handler thread */

static int  Terminate = 0;

QUEUE OutQueue; 		               /* from queue.h, queue.c; sets up linked    */
                                       /*    list via malloc and free              */

static SHM_INFO  Region;               /* Shared memory to use for i/o     */
static char      Attached = 0;         /* Flag=1 when attached to sharedmem*/
static char     *StackerBuffer = NULL; /* Character string to hold msg to be stacked */
static char     *HandlerBuffer = NULL; /* Character string to hold msg to handled */


/* earthworm*d defined logo members */
unsigned char LocalInstId;
unsigned char TypeTracebuf;
unsigned char TypeTracebuf2;

/*MSG_LOGO    getlogo[2];*/
MSG_LOGO    getlogo;

/* Sort codes
 ************/
#define SORTBY_STATION  0
#define SORTBY_CHANNEL  1
#define SORTBY_NETWORK  2
#define SORTBY_LOCATION 3
#define SORTBY_LOGO     4

/* Things to read or derive from configuration file
   ************************************************/
static char RingName[MAX_RING_STR];     /* Name of transport ring for i/o         */
static char MyModuleId[MAX_MOD_STR];    /* Speak as this module name/id           */
static int  LogSwitch;                  /* 0 if no log file should be written     */
static long HeartBeatInterval;          /* Seconds between heartbeats             */
static long MsgMaxBytes;                /* Maximum message size in bytes          */
static int  MaxLogo;                    /* Maximum number of logos to track       */
static int  Debug = 0;                  /* If 1, print debug info on screen       */
                                        /* If 2, print logos as well              */
static long InputQueueLen = 1000;       /* Length of internal queue               */
static char stationFile[512];
static char OutPath[512];
static int  NumDays;
static int  IndexSize = 10000;
static int  MessageSize = -1;
static int  EnforceLimit = 0;
static int  FileWriteInt = 300;
static int  SortBy = SORTBY_STATION;
static int  ForceInstIdToWild = 0;
static int  ForceModIdToWild = 0;
static int  RecieveSCNL = 1;
static int  TableSize = 2000;


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

#define ERR_TOOMANYLOGO    4   /* Number of logos > MaxLogo from config file */
#define ERR_QUEUE          5   /* error queueing message for sending      */
static char Text[150];         /* String for log/error messages */

/* Global to this file
   *******************/
static timer_t timerHandle;    /* Handle for timer object */
static pid_t myPid;            /* for restarts by startstop */

int PacketsPerBurst;

/* strings for writing to output files */
char *Inst_Unlisted = "inst_unlisted";
char *Inst_Wild     = "INST_WILDCARD";
char *Mod_Unlisted  = "mod_unlisted";
char *Mod_NotLocal  = "mod_not_local";
char *Mod_Wild      = "MOD_WILDCARD";
char *Type_Unlisted = "type_unlisted";

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
      fprintf( stderr, "Usage: config_ws <configfile>\n" );
      return -1;
   }

/* Initialize name of log-file & open it
   *************************************/
   logit_init( argv[1], 0, 256, 1 );

/* Read the configuration file(s)
   ******************************/
   config_ws_config( argv[1] );
   logit( "t" , "Read command file <%s>\n", argv[1] );

/* Malloc our table
   ****************/
   SCNLtable = (TABLE *) calloc(TableSize, sizeof(TABLE));

/* Load in our previous run's table if avalible
   ********************************************/
   LoadFromDisk(SCNLtable);

/* Look up important info from earthworm.h tables
   **********************************************/
   config_ws_lookup();

/* Log Our configuration to disk
   *****************************/
   config_ws_log_config();

/* Initialize name of log-file & open it
   *************************************/
   logit_init( argv[1], 0, 256, LogSwitch );

/* Get process ID for heartbeat messages
   *************************************/
   myPid = getpid();
   if( myPid == -1 )
   {
      logit("e","config_ws: Cannot get pid; exiting!\n");
      config_ws_shutdown( -1 );
   }

/* Malloc the message buffers
   **************************/
   StackerBuffer = (char *)malloc( (size_t)(MsgMaxBytes+1) * sizeof(char) );
   if ( StackerBuffer == NULL )
   {
      logit( "e", "config_ws: malloc error for Stacker message buffer; "
                       "exiting!\n" );
      config_ws_shutdown( -1 );
   }

   HandlerBuffer = (char *)malloc( (size_t)(MsgMaxBytes+1) * sizeof(char) );
   if ( HandlerBuffer == NULL )
   {
      logit( "e", "config_ws: malloc error for Handler message buffer; "
                       "exiting!\n" );
      config_ws_shutdown( -1 );
   }

  /* Create a mutex for the queue
   ******************************/
   CreateSpecificMutex( &QueueMutex );   

  /* Create a mutex for the queue
   ******************************/
   CreateSpecificMutex( &TableMutex );

   /* Create a Mutex to control access to queue
    ********************************************/
 /*  CreateMutex_ew();*/

   /* Initialize the message queue
    *******************************/
   initqueue( &OutQueue, (unsigned long)InputQueueLen,(unsigned long)MsgMaxBytes );

/* Attach to Input/Output shared memory ring
   *****************************************/
   tport_attach( &Region, RingKey );
   Attached = 1;
   logit( "et", "Attached to public memory region %s: %d\n", RingName, RingKey );

/* Flush the incoming transport ring on startup
   ********************************************/
   while ( tport_copyfrom( &Region, &getlogo, (short)2, &reclogo, &recsize,
			   StackerBuffer, MsgMaxBytes, &recseq ) != GET_NONE );
   logit( "et", "config_ws: inRing flushed.\n");
 
/* Force a heartbeat to be issued in first pass thru main loop
   ***********************************************************/
   timeLastBeat = time(&timeNow) - HeartBeatInterval - 1;

/* Start the message stacking thread if it isn't already running.
 ****************************************************************/
   if (MessageStackerStatus != MSGSTK_ALIVE )
   {
      if ( StartThread(MessageStacker, (unsigned)THREAD_STACK, &tidStacker) == -1)
      {
         logit( "et", "Error Starting Message Stacker Thread; Exiting!\n" );
         config_ws_shutdown( -1 );
      }
      MessageStackerStatus = MSGSTK_ALIVE;
   }

   /* Start the Message Handling thread
    ***********************************/
   if (MessageHandlerStatus != HANDLER_ALIVE )
   {
      if ( StartThread(MessageHandler, (unsigned)THREAD_STACK, &tidHandler )== -1)
      {
         logit( "et", "Error starting Handler thread; exiting!\n");
         config_ws_shutdown( -1 );
	  }
      MessageHandlerStatus = HANDLER_ALIVE;
   }

   /* Start the Interactive thread
    ******************************/
   if (InteractiveStatus != INTERACTIVE_ALIVE )
   {
      if ( StartThread(Interactive, (unsigned)THREAD_STACK, &tidInteractive )== -1)
      {
         logit( "et", "Error starting Interactive thread; exiting!\n");
         config_ws_shutdown( -1 );
	  }
      InteractiveStatus = INTERACTIVE_ALIVE;
   }

   /* Start main config_ws service loop 
    **********************************/
   while((tport_getflag(&Region) != TERMINATE) &&
          (tport_getflag(&Region) != myPid))
   {
       if (InteractiveStatus == INTERACTIVE_SHUTDOWN)
	   {
          logit( "et", "Shutdown requested by user; exiting!\n");
          config_ws_shutdown( -1 );
	   } 

      /* Send config_ws's heartbeat
       ***************************/
       if ( time(&timeNow) - timeLastBeat  >=  HeartBeatInterval )
       {
          if ((MessageStackerStatus == MSGSTK_ALIVE) &&
              (MessageHandlerStatus == HANDLER_ALIVE) &&
			  (InteractiveStatus    == INTERACTIVE_ALIVE))
		  {
             timeLastBeat = timeNow;
			 if (HeartBeatInterval > 0)
			 {
                config_ws_status( TypeHeartBeat, 0, "" );
			 }
		  }
		  else if (MessageStackerStatus == MSGSTK_ERR)
		  {
             logit( "et", "Message Stacker Thread Encountered Error; exiting!\n");
             config_ws_shutdown( -1 );
		  }
		  else if (MessageStackerStatus == MSGSTK_CHECKALIVE)
		  {
             logit( "et", "Lost Contact with Message Stacker Thread; exiting!\n");
             config_ws_shutdown( -1 );
		  } 
		  else if (MessageHandlerStatus == HANDLER_ERR)
		  {
             logit( "et", "Message Handler Thread Encountered Error; exiting!\n");
             config_ws_shutdown( -1 );
		  }
		  else if (MessageHandlerStatus == HANDLER_CHECKALIVE)
		  {
             logit( "et", "Lost Contact with Message Handler Thread; exiting!\n");
             config_ws_shutdown( -1 );
		  } 
		  else if (InteractiveStatus == INTERACTIVE_ERR)
		  {
             logit( "et", "Interactive Thread Encountered Error; exiting!\n");
             config_ws_shutdown( -1 );
		  }
		  else if (InteractiveStatus == INTERACTIVE_CHECKALIVE)
		  {
             logit( "et", "Lost Contact with Interactive Thread; exiting!\n");
             config_ws_shutdown( -1 );
		  } 
       }

	  /* "Knock down" thread status values to check for thread life
	   ************************************************************/
	   MessageStackerStatus = MSGSTK_CHECKALIVE;
       MessageHandlerStatus = HANDLER_CHECKALIVE;
/*     Don't check status on interactive thread
	   InteractiveStatus    = INTERACTIVE_CHECKALIVE;*/
	   

	  /* take a brief nap
       ******************/
      sleep_ew(500);
   } /*end while of monitoring loop */

   /* Shut it down
   ***************/
   Terminate = 1;
   sleep_ew(1000);
   config_ws_shutdown( -1 );

   return( 0 );	
}

/********************** Interactive Thread *******************
 *                      Do user interactions                 *
 **************************************************************/
thr_ret Interactive( void *dummy )
{
	char line[100];
	char param[100];
	char fish[100];

	do
	{
	   /* Tell the main thread we're ok
		********************************/
		InteractiveStatus = INTERACTIVE_ALIVE;

		line[0] = 0;
		param[0] = 0;
		gets(fish);
		sscanf(fish,"%[^ \t]%*[ \t]%[^ \t]",line,param);

		if ( strlen( line ) == 0 )
		{
			fprintf( stderr, "Config_WS: Found %d channels\n", TableLast );
			fprintf( stderr, "   type 'savefile<cr>' to save tankfiles to disk.\n" );
			fprintf( stderr, "   type 'quit<cr>' to stop config_ws.\n" );
		}

		{
			char *ptr=line;
			while (*ptr) *(ptr++)=tolower(*ptr);

			if ( strcmp( line, "savefile" ) == 0 )
			{
				RewriteFile(SCNLtable);
			}
		}
	}
	while ( strcmp( line, "quit" ) != 0 );

	InteractiveStatus = INTERACTIVE_SHUTDOWN; /* The main thread checks this flag */

   /* main thread will restart us 
    ******************************/
	KillSelfThread(); 

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
   while(Terminate == 0)
   {
     /* Tell the main thread we're ok
      ********************************/
      MessageStackerStatus = MSGSTK_ALIVE;

	  if(Terminate == 1)
	  {
		 goto error;
	  }

     /* Get a message from the transport ring
      *************************************/
      tport_res = tport_copyfrom(&Region, &getlogo, (short)2, &reclogo,
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
          config_ws_status(TypeError, ERR_TOOBIG, Text);
          continue;
      }
      else if (tport_res == GET_MISS_LAPPED) /* Got a msg, but missed some */
      {
          sprintf(Text, "Missed msgs (overwritten, ring lapped)  i%u m%u t%u seq%u  %s.",
                  reclogo.instid, reclogo.mod, reclogo.type, recseq, RingName);
          config_ws_status( TypeError, ERR_MISSMSG, Text );
      }
      else if (tport_res == GET_MISS_SEQGAP) /* Got a msg, but saw sequence gap */
      {
          sprintf(Text, "Missed msgs (sequence gap) i%u m%u t%u seq%u  %s.",
                  reclogo.instid, reclogo.mod, reclogo.type, recseq, RingName);
          config_ws_status(TypeError, ERR_MISSMSG, Text);
      }
      else if (tport_res == GET_NOTRACK)  /* Got a msg, but can't tell */
      {                                   /* if any were missed        */
          sprintf(Text, "Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
                  reclogo.instid, reclogo.mod, reclogo.type );
          config_ws_status( TypeError, ERR_NOTRACK, Text );
      }
      StackerBuffer[recsize] = '\0';         /* Null-terminate the message */

      /* Put Message into the 'to be shipped' queue 
	   * the UDPSender thread is in the biz of de-queueng and sending 
	   **************************************************************/
      RequestSpecificMutex(&QueueMutex);
      queue_ret = enqueue(&OutQueue, StackerBuffer, recsize, reclogo); 
      ReleaseSpecificMutex(&QueueMutex);

      if (queue_ret != 0)
      {       
          if (queue_ret == -2)  /* Serious: quit */
          {    
			  /* Currently, eneueue() in mem_circ_queue.c never returns this error. 
			   ********************************************************************/
	          sprintf(Text,"internal queue error. Terminating.");
              config_ws_status(TypeError, ERR_QUEUE, Text);
	          goto error;
          }
          if (queue_ret == -1) 
          {
              sprintf(Text,"queue cannot allocate memory. Lost message.");
              config_ws_status(TypeError, ERR_QUEUE, Text);
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


/*************************** Handler Thread ***********************
 *                 Do all non stacking tasks                      *
 ******************************************************************/
thr_ret MessageHandler( void *dummy )
{
   long      recsize;	  /* size of retrieved message */
   MSG_LOGO  reclogo;     /* logo of retrieved message */
   int 		 queue_ret;
   TracePacket tpkt;
   int       indx;
   short     *short_data;
   long      *long_data;
   int       ret;

   time_t nowTime;        /* Current time */
   time_t timeLastWrite = 0;   

  /* Initialize things
   ********************/
   long_data  =  (long *)( tpkt.msg + sizeof(TRACE2_HEADER) ); 
   short_data = (short *)( tpkt.msg + sizeof(TRACE2_HEADER) );   

  /* Tell the main thread we're ok
   ********************************/
   MessageHandlerStatus = HANDLER_ALIVE;

   /* Start Stacking service loop 
    ******************************/
   while(1)
   {
     /* Tell the main thread we're ok
      ********************************/
      MessageHandlerStatus = HANDLER_ALIVE;

	  if(Terminate == 1)
	  {
		 goto HandlerShutdown;
	  }


     /* Get a message from queue
	  **************************/
      RequestSpecificMutex(&QueueMutex);
	  queue_ret=dequeue( &OutQueue, tpkt.msg, &recsize, &reclogo);
	  ReleaseSpecificMutex(&QueueMutex);

      if(queue_ret < 0 )
	  { 
	     /* -1 means empty queue 
	      **********************/
	     sleep_ew(50); 
	     continue;
	  }
 
      WaveMsg2MakeLocal( &tpkt.trh2 );

	 /* Have we seen this SCNL before?
      ********************************/
	  RequestSpecificMutex(&TableMutex);
      indx = FindSCNL( SCNLtable, &tpkt.trh2, &reclogo );
      if (indx == -1 ) /* it's a new one */
	  {
         if (ret = AddSCNL(SCNLtable, &tpkt.trh2, recsize, &reclogo ) <0)
		 {
            logit("et","Table overflow. More than %d SCNLs found\n",TableSize);
		 }
	  }
      else
	  {
         UpdateSCNL(SCNLtable, &tpkt.trh2, indx, recsize);
	  }
	  ReleaseSpecificMutex(&TableMutex);

     if ( time(&nowTime) - timeLastWrite  >=  FileWriteInt )
     {
		 timeLastWrite = nowTime;
         fprintf( stderr, "Found %d channels, Press Enter for a command list\n", TableLast );
		 RewriteFile(SCNLtable);
	 }

   } /* end of while */

   /* we're quitting 
   *****************/
HandlerShutdown:

   /* file a complaint to the main thread 
    *************************************/
   MessageHandlerStatus = HANDLER_ERR; 
   /* main thread will restart us 
    ******************************/
   KillSelfThread(); 
}

/******************************************************************************
 * FindSCNL()
 * To see if the SCNL in the message is already in the table. 
 * Return an index if so, otherwise return a -1.
 *****************************************************************************/
int FindSCNL( TABLE* SCNLtable, TRACE2_HEADER* msg, MSG_LOGO* logo )
{
   int i;
	
   for(i=0;i<TableSize;i++)
   {
      if( strcmp(SCNLtable[i].TrHdr.sta,  msg->sta)  != 0 ) continue;
      if( strcmp(SCNLtable[i].TrHdr.chan, msg->chan) != 0 ) continue;
      if( strcmp(SCNLtable[i].TrHdr.net,  msg->net)  != 0 ) continue;
      if( logo->type==TypeTracebuf2  && 
          strcmp(SCNLtable[i].TrHdr.loc,  msg->loc)  != 0 ) continue;
      if( SCNLtable[i].logo.instid != logo->instid )        continue;
      if( SCNLtable[i].logo.mod    != logo->mod    )        continue;
      if( SCNLtable[i].logo.type   != logo->type   )        continue;
      return(i);  /* found a match! */
   }
   return(-1);    /* no match */
}

/******************************************************************************
 * AddSCNL()
 * Add a new SCNL to the table
 *****************************************************************************/
int AddSCNL( TABLE* SCNLtable, TRACE2_HEADER* msg, int msgSize, MSG_LOGO* logo )
{
   if(TableLast == TableSize) return(-1); /* no more room */
	
   memcpy( (void*)&(SCNLtable[TableLast].TrHdr), msg,  sizeof(TRACE2_HEADER) );
   memcpy( (void*)&(SCNLtable[TableLast].logo),  logo, sizeof(MSG_LOGO)      );
   SCNLtable[TableLast].maxMsg  = msgSize;
   SCNLtable[TableLast].minMsg  = msgSize;
   SCNLtable[TableLast].totalMsgTime =   
                SCNLtable[TableLast].TrHdr.endtime        /* time of last data point  */
              - SCNLtable[TableLast].TrHdr.starttime      /* time of first data point */
              + 1.0/SCNLtable[TableLast].TrHdr.samprate;  /* one sample period        */
   SCNLtable[TableLast].nMsg    = 1;
   TableLast++;
 
/* Sort the table
 *****************/
   qsort( (void*)SCNLtable, TableLast, sizeof(TABLE), compare);

   return(0);
}

/*************************************************************
 * compare() function for qsort                              *
 *************************************************************/
int compare( const void* s1, const void* s2 )
{

   int rc = 0;
   TABLE* t1 = (TABLE*) s1;
   TABLE* t2 = (TABLE*) s2;
 
   if (SortBy == SORTBY_STATION)
   {
	   rc = strcmp( t1->TrHdr.sta,  t2->TrHdr.sta);
	   if (rc != 0) return(rc);
	   rc = strcmp( t1->TrHdr.net,  t2->TrHdr.net);
	   if (rc != 0) return(rc);
	   rc = strcmp( t1->TrHdr.loc,  t2->TrHdr.loc);
	   if (rc != 0) return(rc);
	   rc = strcmp( t1->TrHdr.chan, t2->TrHdr.chan);
       return(rc);
   }
   else if (SortBy == SORTBY_CHANNEL)
   {
	   rc = strcmp( t1->TrHdr.chan, t2->TrHdr.chan);
	   if (rc != 0) return(rc);
	   rc = strcmp( t1->TrHdr.sta,  t2->TrHdr.sta);
	   if (rc != 0) return(rc);
	   rc = strcmp( t1->TrHdr.net,  t2->TrHdr.net);
	   if (rc != 0) return(rc);
	   rc = strcmp( t1->TrHdr.loc,  t2->TrHdr.loc);
       return(rc);
   }
   else if (SortBy == SORTBY_NETWORK)
   {
	   rc = strcmp( t1->TrHdr.net,  t2->TrHdr.net);
	   if (rc != 0) return(rc);
	   rc = strcmp( t1->TrHdr.sta,  t2->TrHdr.sta);
	   if (rc != 0) return(rc);
	   rc = strcmp( t1->TrHdr.loc,  t2->TrHdr.loc);
	   if (rc != 0) return(rc);
	   rc = strcmp( t1->TrHdr.chan, t2->TrHdr.chan);
	   return(rc);
   }
   else if (SortBy == SORTBY_LOCATION)
   {
	   rc = strcmp( t1->TrHdr.loc,  t2->TrHdr.loc);
	   if (rc != 0) return(rc);
	   rc = strcmp( t1->TrHdr.sta,  t2->TrHdr.sta);
	   if (rc != 0) return(rc);
	   rc = strcmp( t1->TrHdr.net,  t2->TrHdr.net);
	   if (rc != 0) return(rc);
	   rc = strcmp( t1->TrHdr.chan, t2->TrHdr.chan);
	   return(rc);
   }
   else if (SortBy == SORTBY_LOGO)
   {
	   /* Sort by logo */
	   if( t1->logo.instid < t2->logo.instid ) return(-1);
	   if( t1->logo.instid > t2->logo.instid ) return( 1);

	   if( t1->logo.mod   < t2->logo.mod     ) return(-1);
	   if( t1->logo.mod   > t2->logo.mod     ) return( 1);

	   if( t1->logo.type  < t2->logo.type    ) return(-1);
	   if( t1->logo.type  > t2->logo.type    ) return( 1);

	   /* Then SNLC */
	   rc = strcmp( t1->TrHdr.sta,  t2->TrHdr.sta);
	   if (rc != 0) return(rc);
	   rc = strcmp( t1->TrHdr.net,  t2->TrHdr.net);
	   if (rc != 0) return(rc);
	   rc = strcmp( t1->TrHdr.loc,  t2->TrHdr.loc);
	   if (rc != 0) return(rc);
	   rc = strcmp( t1->TrHdr.chan, t2->TrHdr.chan);
	   return(rc);
   }
   return(rc);
}

/******************************************************************************
   UPdateSCNL()
   Update statistics for an existing entry
 *****************************************************************************/
void UpdateSCNL(TABLE* SCNLtable, TRACE2_HEADER* msg, int index, int msgSize )
{
   if( SCNLtable[index].maxMsg < msgSize) SCNLtable[index].maxMsg = msgSize;
   if( SCNLtable[index].minMsg > msgSize) SCNLtable[index].minMsg = msgSize;

   /* Update average message time
    ******************************/
   SCNLtable[index].totalMsgTime =  SCNLtable[index].totalMsgTime 
                                  + SCNLtable[index].TrHdr.endtime
                                  - SCNLtable[index].TrHdr.starttime 
                                  + 1.0/SCNLtable[index].TrHdr.samprate;
   (SCNLtable[index].nMsg)++;

   return;
}

/*****************************************************************************
  SumMB()
  Sum the total mb required by the current config and table
 *****************************************************************************/
long SumMB( TABLE* SCNLtable)
{
   int  i;
   long TotalMBRequired = 0;

   RequestSpecificMutex(&TableMutex);
   for (i=0; i<TableLast; i++)
   {
 	  TotalMBRequired += ComputeTankSize(SCNLtable[i]);
   }
   ReleaseSpecificMutex(&TableMutex);

   return (TotalMBRequired);
}

/*****************************************************************************
  ComputeTankSize()
  Compute the MB required for a single tank.
 *****************************************************************************/
long ComputeTankSize( TABLE SCNLtable)
{
   int    TankRecordSize;
   double secondsPerDay;
   double bytesPerDay;
   double MBPerDay;
   long   totalMB;

  /* compute Record size */ 
   TankRecordSize = ComputeRecordSize(SCNLtable);

   secondsPerDay = (double)(24*60*60); /*  secs/day */

  /* compute bytes per day of WaveServer tank */
   bytesPerDay = (secondsPerDay / (SCNLtable.totalMsgTime / SCNLtable.nMsg)) * TankRecordSize;
   MBPerDay = bytesPerDay / 1000000.;
   totalMB  = (long)(MBPerDay * NumDays + 0.999);

   /* Don't allow sizes greater than 1 G */
   if (EnforceLimit == 1)
   {
      if (totalMB > 1073) /* 1 GB = 1073741824, max ws tank size = 1073741823 */
	  {
         totalMB = 1073;
	  }
   } 

   return (totalMB);
}

/*****************************************************************************
  ComputeRecordSize()
  Compute the record size needed for a single tank.
 *****************************************************************************/
int ComputeRecordSize( TABLE SCNLtable)
{
   int  RecordSize;

   /* User entered a specific size */ 
   if(MessageSize > 0)
      RecordSize = MessageSize;
   /* User wants a "padded size" */
   else if(MessageSize == -2)
   {
      RecordSize = SCNLtable.maxMsg + SCNLtable.maxMsg - SCNLtable.minMsg;
      if(RecordSize > MAX_TRACEBUF_SIZ)
         RecordSize = MAX_TRACEBUF_SIZ;
   }
   /* no fancy tricks (old style findwave) */
   else
      RecordSize = SCNLtable.maxMsg;

   return (RecordSize);
}

void WriteTankLine( TABLE CurrentSCNL, HDS CurrentHD, FILE *fp, int TankRecordSize, int TankSize)
{
    /* character strings for writing to file */
    char   *inst = NULL;
    char   *modid = NULL;
    char   *msgtype = NULL;

    if(ForceInstIdToWild == 0)
	{
		inst = GetInstName (CurrentSCNL.logo.instid );
		if( inst == NULL ) 
			inst = Inst_Unlisted;
	}
	else
		inst = Inst_Wild;

	if(ForceModIdToWild == 0)
	{
		modid = GetModIdName(CurrentSCNL.logo.mod );
		if( modid == NULL ) 
			modid = Mod_Unlisted;
		if( CurrentSCNL.logo.instid != LocalInstId ) 
			modid = Mod_NotLocal;
	}
	else
		modid = Mod_Wild;

	msgtype = GetTypeName (CurrentSCNL.logo.type   );

	if( inst == NULL ) inst  = Inst_Wild;

	if (RecieveSCNL == 1)
	{
		fprintf(fp, "Tank %-5s %-3s %-2s %-2s  %4d  %-15s %-15s %5d  %d  %s%s_%s_%s_%s.tnk\n", 
				CurrentSCNL.TrHdr.sta, CurrentSCNL.TrHdr.chan, CurrentSCNL.TrHdr.net, CurrentSCNL.TrHdr.loc,  
				TankRecordSize, inst, modid, TankSize, IndexSize, CurrentHD.hd_path, 
				CurrentSCNL.TrHdr.sta, CurrentSCNL.TrHdr.chan, CurrentSCNL.TrHdr.net, CurrentSCNL.TrHdr.loc);
	}
	else
	{
		fprintf(fp, "Tank %-5s %-3s %-2s  %4d  %-15s %-15s %5d  %d  %s%s_%s_%s.tnk\n", 
				CurrentSCNL.TrHdr.sta, CurrentSCNL.TrHdr.chan, CurrentSCNL.TrHdr.net,  
				TankRecordSize, inst, modid, TankSize, IndexSize, CurrentHD.hd_path, 
				CurrentSCNL.TrHdr.sta, CurrentSCNL.TrHdr.chan, CurrentSCNL.TrHdr.net);
	}

}


/*****************************************************************************
  GenerateTankFile()
  Generate a tank file.
 *****************************************************************************/
void GenerateTankFile( TABLE* SCNLtable, int * CurrentTableIndex, HDS CurrentHD, int isLastHD)
{
    FILE   *filepointer;
	long   UsedMB = 0;
	double DesiredPercentUsage;
	double ActualPercentUsage;
	char   outfilename[512];
	char   tempfilename[64];
	int    StationCount = 0; 
	int    TankRecordSize;
	int    TankSize;
	
	DesiredPercentUsage = (double)CurrentHD.hd_target_size / (double)CurrentHD.hd_size;

	sprintf(tempfilename, "waveserverv_%s.tlst", CurrentHD.hd_name);
	strcpy(outfilename, OutPath);
	strcat(outfilename, tempfilename);

	filepointer = fopen( outfilename, "w");
	if (filepointer == NULL) 
	{
		logit("et","Cannot create file %s\n", outfilename);
	}
	else
	{
		fprintf(filepointer, "# config_ws: Generated configuration for waveserver %s.\n", CurrentHD.hd_name);
		fprintf(filepointer, "# Tank file Path: %s.\n", CurrentHD.hd_path);
		fprintf(filepointer, "#\n");
		if (RecieveSCNL == 1)
			fprintf(filepointer, "#    SCNL            Record            Logo             File Size   Index Size    File Name\n");
		else
			fprintf(filepointer, "#    SCN             Record            Logo             File Size   Index Size    File Name\n");
		fprintf(filepointer, "#    names            size     Instid        ModuleId   (megabytes) (max breaks)  (full path)\n");

		while ((UsedMB < CurrentHD.hd_target_size) && (*CurrentTableIndex < TableLast))
		{
			TankRecordSize = ComputeRecordSize(SCNLtable[*CurrentTableIndex]);
			TankSize = ComputeTankSize(SCNLtable[*CurrentTableIndex]);
			UsedMB += TankSize;
	
			WriteTankLine(SCNLtable[*CurrentTableIndex], CurrentHD, filepointer, TankRecordSize, TankSize);

			*CurrentTableIndex = *CurrentTableIndex + 1;
			StationCount++;

		}

		if (isLastHD == 1)
		{
			while (*CurrentTableIndex < TableLast)
			{
				TankRecordSize = ComputeRecordSize(SCNLtable[*CurrentTableIndex]);
				TankSize = ComputeTankSize(SCNLtable[*CurrentTableIndex]);
				UsedMB += TankSize;
		
				WriteTankLine(SCNLtable[*CurrentTableIndex], CurrentHD, filepointer, TankRecordSize, TankSize);

				*CurrentTableIndex++;
				StationCount++;
			}
		}

		ActualPercentUsage = (double)UsedMB / (double)CurrentHD.hd_size;

        fprintf(stderr, "Harddrive %s, Path %s, Size %d, Target Size %d, Stations Archived %d, Actual Size %d, Percentage Used %3.2f\n", 
			    CurrentHD.hd_name, CurrentHD.hd_path, CurrentHD.hd_size, CurrentHD.hd_target_size, StationCount, UsedMB, ActualPercentUsage*100);

		fprintf(filepointer, "#\n");
		fprintf(filepointer, "# Hard Drive is %3.2f full (%3.2f desired).  %d channels archived on this waveserver using %d megabytes\n",
			    ActualPercentUsage*100, DesiredPercentUsage*100, StationCount, UsedMB);
		fprintf(filepointer, "#\n");
		fclose(filepointer);
	}

}

/*****************************************************************************
  RewriteFile()
  Dump the scnl table to the file
 *****************************************************************************/
int RewriteFile( TABLE* SCNLtable)
{
	FILE   *fp;
	int    i;
	int    j;
	char   outfile[512];
	HDS    BlankHD;
	int    totalMB;
    int    iRecordSize;
    long   TotalMBRequired = 0;
    long   TotalMBAvalible = 0;
    double TotalPercentUsed = 0;
    int    CurrentTableIndex = 0;

	/* Get total ammount of needed hd space */
    TotalMBRequired = SumMB(SCNLtable);

	/* Get total ammount of actual hd space */
	for (j=0; j<HDSLast; j++)
	{
        TotalMBAvalible += HDStruct[j].hd_size;
	}

	/* if any hds are configured at all */
	if (TotalMBAvalible > 0)
	{
		/* make sure we have enough space */
		if (TotalMBAvalible < TotalMBRequired)
		{
			logit("e", "Cannot generate files, TotalMBAvalible %d is < TotalMBRequired %d\n", TotalMBAvalible, TotalMBRequired);
			return 0;
		}
		/* Figure out how much each drive should be filled */
		TotalPercentUsed = (double)TotalMBRequired / (double)TotalMBAvalible;

		fprintf(stderr, "TotalMBRequired = %d, TotalMBAvalible = %d, TotalPercentUsed = %3.2f\n",
			    TotalMBRequired, TotalMBAvalible, TotalPercentUsed*100);

	    for (j=0; j<HDSLast; j++)
		{
           HDStruct[j].hd_target_size = (int)(HDStruct[j].hd_size * TotalPercentUsed);
		   /*fprintf(stderr, "Harddrive %s, Path %s, Size %d, Target Size %d\n", HDStruct[j].hd_name, HDStruct[j].hd_path, HDStruct[j].hd_size,
			       HDStruct[j].hd_target_size);*/
		}
		fprintf(stderr, "\n");

		/* build each hd tank file */
		RequestSpecificMutex(&TableMutex);
		for (j=0; j<HDSLast; j++)
		{
			if (j == HDSLast)
			{
				GenerateTankFile(SCNLtable, &CurrentTableIndex,HDStruct[j],1);
			}
			else
			{
				GenerateTankFile(SCNLtable, &CurrentTableIndex,HDStruct[j],0);
			}
		}
		fprintf(stderr, "\n");
		ReleaseSpecificMutex(&TableMutex);
	}
	else
	{
		strcpy(outfile, OutPath);
		strcat(outfile, "waveserver_tank_list.txt");

		strcpy(BlankHD.hd_name, "");
		strcpy(BlankHD.hd_path, "<Insert_Path_Here>");
		BlankHD.hd_size = 0;
		BlankHD.hd_target_size = 0;

		/* Create file of SCNLS, not broken into
		 * harddrives
		 *******************************************/
		fp = fopen( outfile, "w");

		if (fp == NULL) 
		{
			logit("et","Cannot create file %s\n", outfile);
		}
		else
		{
			logit("e","Found %d SCNLs requireing %d MB, type quit<cr> to halt.\n",TableLast, TotalMBRequired);

			fprintf(fp, "# config_ws: Found %d SCNLs\n", TableLast);
			fprintf(fp, "#     Found %d SCNLs, Num Days = %d, Msg Size = %d\n", 
						  TableLast, NumDays, MessageSize);  
			if (EnforceLimit == 1)
				fprintf(fp, "#     Enforce 1 Gig limit = yes\n");
			else
				fprintf(fp, "#     Enforce 1 Gig limit = no\n");

			fprintf(fp, "#    SCNL            Record            Logo             File Size   Index Size    File Name\n");
			fprintf(fp, "#    names            size     Instid        ModuleId   (megabytes) (max breaks)  (full path)\n");

			RequestSpecificMutex(&TableMutex);
			for (i=0; i<TableLast; i++)
			{
				iRecordSize = ComputeRecordSize(SCNLtable[i]);
				totalMB = ComputeTankSize(SCNLtable[i]);

				WriteTankLine(SCNLtable[i], BlankHD, fp, iRecordSize, totalMB);
			}
			ReleaseSpecificMutex(&TableMutex);

			fclose(fp);
			logit("et","Raw File Dump completed.\n");
		}
	}

   return(0);
}


void DumpToDisk( TABLE* SCNLtable)
{
   int   i;
   FILE  *fp;

  /* Create file to preserve the station table
   *******************************************/
   fp = fopen( stationFile, "wb");
   if (fp == NULL) 
   {
      logit("et","Cannot create file %s\n", stationFile);
   }
   else
   {
	  logit("et","Beginning table dump to %s\n", stationFile);

	  RequestSpecificMutex(&TableMutex);
      for (i=0; i<TableLast; i++)
	  {
		  fwrite(&SCNLtable[i], sizeof(TABLE), 1, fp);
	  }
	  ReleaseSpecificMutex(&TableMutex);
	  fclose(fp);
	  logit("et","Completed table dump\n");
   }

}

void LoadFromDisk( TABLE* SCNLtable)
{
   int   i = 0;
   FILE  *fp;

  /* Initialize table
   *******************/
   memset( (void*)SCNLtable, 0, sizeof(TABLE) * TableSize );
   TableLast = 0;

  /* Load table from file
   **********************/
   fp = fopen( stationFile, "rb");
   if (fp == NULL) 
   {
      logit("et","No Previous %s found, starting from scratch\n", stationFile);
   }
   else
   {
	   while(!feof(fp))
	   {
		   fread(&SCNLtable[i], sizeof(TABLE), 1, fp);
		   i++;
		   if (i > TableSize)
		   {
			   logit("et", "%s is too large for tablesize %d, exiting\n", stationFile, TableSize);
			   exit(-1);
		   }
	   }
	   TableLast = i-1;
	   fclose(fp);

	/* Sort the table after loading incase the sort has been changed
	 ***************************************************************/
	   qsort( (void*)SCNLtable, TableLast, sizeof(TABLE), compare);
   }
}


 /************************************************************************
  *                          config_ws_config()                       *
  *                                                                      *
  *           Processes command file(s) using kom.c functions.           *
  *                  Exits if any errors are encountered.                *
  ************************************************************************/
#define ncommand 7      /* # of required commands you expect to process   */

void config_ws_config( char *configfile )
{
   char   init[ncommand]; /* Init flags, one byte for each required command */
   int    nmiss;          /* Number of required commands that were missed   */
   char  *com;
   char  *str;
   int    nfiles;
   int    success;
   int    i;
   int    temp;

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
                "config_ws: Error opening command file <%s>; exiting!\n",
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
                 "config_ws: Error opening command file <%s>; exiting!\n",
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
         else if ( k_its( "StationFile" ) )
         {
            str = k_str();
            if (str) strcpy( stationFile, str );
            init[5] = 1;
         }
		 else if( k_its("DaysToSave") ) 
		 {
		    NumDays = k_int();  
            init[6] = 1;			
		 }
		 /* Optional commands */
		 else if( k_its("TraceType") ) 
		 {
			if( ( str = k_str() ) )
			{	
				if( strcmp( str, "SCNL") == 0)
				{
					RecieveSCNL = 1;
				}
				else if ( strcmp( str, "SCN") == 0)
				{
					RecieveSCNL = 0;
				}
			}
		 }
		 else if( k_its("InputQueueLen") ) 
		 {
		    InputQueueLen = k_long();               
		 }
         else if ( k_its( "OutPath" ) )
         {
            str = k_str();
            if (str) strcpy( OutPath, str );
         }

		 else if( k_its("IndexSize") ) 
		 {
		    IndexSize = k_long();               
		 }
		 else if( k_its("MessageSize") ) 
		 {
		    MessageSize = k_long();               
		 }
         else if ( k_its( "EnforceLimit" ) )
         {
            EnforceLimit = k_int();
         }
         else if ( k_its( "FileWriteInt" ) )
         {
            FileWriteInt = k_int();
         }
         else if ( k_its( "ForceInstIdToWild" ) )
         {
            ForceInstIdToWild = k_int();
         }
         else if ( k_its( "ForceModIdToWild" ) )
         {
            ForceModIdToWild = k_int();
         }
         else if ( k_its( "TableSize" ) )
         {
            TableSize = k_int();
         }
		 else if ( k_its( "Harddrive" ) )
         {
			if(HDSLast <= HDSLEN)
			{
				if( ( str = k_str() ) )
				{	
					/* Name of HD */
					strcpy(HDStruct[HDSLast].hd_name, str);
				}
				if( ( str = k_str() ) )
				{	
					/* Path to tanks on HD */
					strcpy(HDStruct[HDSLast].hd_path, str);
				}
				if( ( temp = k_int() ) )
				{	
					/* Path to tanks on HD */
					HDStruct[HDSLast].hd_size = temp;
				}
				HDSLast++;
			}
		 }
         else if ( k_its( "SortBy" ) )
         {
			if( ( str = k_str() ) )
			{	
				if( strcmp( str, "SCNL") == 0)
				{
					if( ( str = k_str() ) )
					{
						if(strcmp( str, "S") == 0)
							SortBy = SORTBY_STATION;
						if(strcmp( str, "C") == 0)
							SortBy = SORTBY_CHANNEL;
						if(strcmp( str, "N") == 0)
							SortBy = SORTBY_NETWORK;
						if(strcmp( str, "L") == 0)
							SortBy = SORTBY_LOCATION;
					}
					else
					{
						SortBy = SORTBY_STATION;
					}
				}
				else if ( strcmp( str, "Logo") == 0)
				{
					SortBy = SORTBY_LOGO;
				}

			}
		 }

/* Unknown command
   ***************/
         else
         {
             logit( "e", "config_ws: `%s' Unknown command in `%s'.\n",
                      com, configfile );
             continue;
         }

/* See if there were any errors processing the command
   ***************************************************/
         if ( k_err() )
         {
            logit( "e", "config_ws: Bad `%s' command in `%s'; exiting!\n",
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
      logit( "e", "config_ws: ERROR, no " );
      if ( !init[0] )  logit( "e", "<LogSwitch> " );
      if ( !init[1] )  logit( "e", "<MyModuleId> " );
      if ( !init[2] )  logit( "e", "<RingName> " );
      if ( !init[3] )  logit( "e", "<HeartBeatInterval> " );
      if ( !init[4] )  logit( "e", "<MsgMaxBytes> " );
      if ( !init[5] )  logit( "e", "<StationFile> " );
      if ( !init[6] )  logit( "e", "<DaysToSave> " );
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

void config_ws_log_config( void )
{

   logit( "", "config_ws Configuration:\n" );
   logit( "", "MyModuleId:          %s => %2u\n", MyModuleId, MyModId);
   logit( "", "RingName:            %s => %d\n", RingName, RingKey);
   logit( "", "HeartBeatInterval:   %d \n", HeartBeatInterval);
   logit( "", "LogSwitch:           %d \n", LogSwitch);
   logit( "", "Debug:               %d \n", Debug);
   logit( "", "MsgMaxBytes:         %d \n", MsgMaxBytes);
   logit( "", "StationFile:         %s \n", stationFile);
   logit( "", "InputQueueLen:       %d \n", InputQueueLen);
   logit( "", "OutPath:             %s \n", OutPath);
   logit( "", "DaysToSave:          %d \n", NumDays);
   logit( "", "MessageSize:         %d \n", MessageSize);
   logit( "", "EnforceLimit:        %d \n", EnforceLimit);
   logit( "", "SortBy:              %d \n", SortBy);

   logit( "", "\n");

   return;
}


  /****************************************************************
   *                      config_ws_lookup()                   *
   *                                                              *
   *        Look up important info from earthworm.h tables        *
   ****************************************************************/

void config_ws_lookup( void )
{
/* Look up keys to shared memory regions
   *************************************/
   if( ( RingKey = GetKey(RingName) ) == -1 )
   {
        fprintf( stderr,
          "config_ws:  Invalid ring name <%s>; exiting!\n", RingName);
        exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &LocalInstId ) != 0 )
   {
      fprintf( stderr,
        "config_ws: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModuleId, &MyModId ) != 0 )
   {
      fprintf( stderr,
        "config_ws: Invalid module name <%s>; exiting!\n", MyModuleId );
      exit( -1 );
   }

   /* Specify logos to get
    ***********************/
   if( GetType("TYPE_TRACEBUF2", &TypeTracebuf2) ) exit( -1 );
   if( GetType("TYPE_TRACEBUF",  &TypeTracebuf ) ) exit( -1 );

/*   if( GetInst("INST_WILDCARD",  &getlogo[0].instid) ) exit( -1 );
   if( GetModId("MOD_WILDCARD",  &getlogo[0].mod   ) ) exit( -1 );
   getlogo[0].type   = TypeTracebuf2;

   getlogo[1].instid = getlogo[0].instid;
   getlogo[1].mod    = getlogo[0].mod;
   getlogo[1].type   = TypeTracebuf;*/
 
   if( GetInst("INST_WILDCARD",  &getlogo.instid) ) exit( -1 );
   if( GetModId("MOD_WILDCARD",  &getlogo.mod   ) ) exit( -1 );

   if (RecieveSCNL == 1)
      getlogo.type   = TypeTracebuf2;
   else
      getlogo.type   =  TypeTracebuf;


/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 )
   {
      fprintf( stderr,
              "config_ws: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }

   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 )
   {
      fprintf( stderr,
              "config_ws: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }

   if ( GetType( "TYPE_RESTART", &TypeRestart ) != 0 )
   {
      fprintf( stderr,
              "config_ws: Invalid message type <TYPE_RESTART>; exiting!\n" );
      exit( -1 );
   }
   return;
}


  /****************************************************************
   *                      config_ws_status()                     *
   *                                                              *
   *      Builds a heartbeat or error message & puts it into      *
   *      shared memory.  Writes errors to log file & screen.     *
   ****************************************************************/

void config_ws_status( unsigned char type, short ierr, char *note )
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
      logit( "et", "config_ws: %s\n", note );
   }

   size = strlen( msg );   /* don't include the null byte in the message */

/* Write the message to shared memory
   **********************************/
   if( tport_putmsg( &Region, &logo, size, msg ) != PUT_OK )
   {
      if( type == TypeHeartBeat )
         logit("et","config_ws:  Error sending heartbeat.\n" );

      else if( type == TypeError )
         logit("et","config_ws:  Error sending error:%d.\n", ierr );
   }

   return;
}

 /************************************************************************
  *                        config_ws_shutdown()                         *
  *                                                                      *
  *    Shuts down politely (frees memory, detaches from rings, etc.)     *
  ************************************************************************/
void config_ws_shutdown( int estatus )
{
   RewriteFile(SCNLtable);
   DumpToDisk(SCNLtable);

   free (SCNLtable);

   if( StackerBuffer ) free( StackerBuffer );
   if( HandlerBuffer ) free( HandlerBuffer );
   
   if( Attached  ) tport_detach( &Region );   /* Detach from shared memory */
 
/* CloseSpecificMutex(QueueMutex);
   CloseSpecificMutex(TableMutex);*/
   
   exit( estatus );
}

