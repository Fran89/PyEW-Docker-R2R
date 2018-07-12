/******************************************************************************
 *
 *	File:			ewaccel.c
 *
 *	Function:		Program to read 
 *
 *	Author(s):		Scott Hunter, ISTI
 *
 *	Source:			ewthresh.c
 *
 *	Notes:
 *
 *	Change History:
 *			5/23/11	Started source
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time_ew.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <swap.h>
#include <mem_circ_queue.h>
#include <trace_buf.h>
#include "butterworth_c.h"


#define ACCEL_VERSION "0.0.3 2013-08-08"

/* Information specific to a floor */
typedef struct {
        double  mass;                  /* mass for this floor */
        double  convFactor;            /* conversion factor */
        char    *id;                   /* name of this floor; NULL for ground */
        char    read;				   /* = "floor has been read for current time" */
        char    windowActive;          /* = "window contains data" */
        char    sta[TRACE2_STA_LEN];   /* Site name (NULL-terminated) */
        char    net[TRACE2_NET_LEN];   /* Network name (NULL-terminated) */
        char    chan[TRACE2_CHAN_LEN]; /* Component/channel code (NULL-terminated)*/
        char    loc[TRACE2_LOC_LEN];   /* Location code (NULL-terminated) */
} EW_FLOOR_DEF;

/* Information about a highpass filter stage */
typedef struct _HP_STAGE
{
  double d1, d2;
  double f;
  double a1, a2, b1, b2;
} HP_STAGE;

int          alertOn = 0;	/* samples until alert is allowed again */
EW_FLOOR_DEF *flr = NULL;	/* array of floor definitions */
int          nFloor = 0;	/* number of floors, including ground */
int          nFloorAlloc = 5; /* nbr of elements allocated in flr */
double       baseShearThreshold; /* threshold to trip alert */
double       avgWindowLen;  /* # samples in averaging window */
int          dataSize;      /* size of samples in TRACEBUF2 record */
int          awSamples;     /* nbr of samples in window */
int          awBytesAllocated = 0; /* space allocated for averaging window */
double       **avgWindow;	/* array of windows, 1 per floor */
int          awPosn;		/* current element in averaging window */
int          awFull;		/* = "is window full?" */
double       *awSum;		/* array of window sums, 1 per floor */
int          awEmpty;		/* = "is window empty?" */
double       awPeriod;		/* period of samples in window */
int          nHPLevels;		/* nbr of highpass levels */
double       hpFreq=0.0;	/* highpass threshold frequency */
double       hpGain;		/* highpass gain */
int          hpOrder=0;		/* order of highpass filter */
complex      *hpPoles;		/* array of poles for highpass filter */
HP_STAGE     *hpLevel;		/* array of highpass level definitions */
double       awStartTime;	/* time of first item in packet */
int          floorsLeft;	/* nbr of floors not seen for current timeframe */
double       defaultConversion = 0.0; /* default conversion factor */
char         defaultConversionSet = 0; /* ="is defaultConversion set?" */
char         exportForces = 0; /* ="write debugging messages of computed force info" */
int          alertDelay;	/* seconds until another alert can possibly go off */
int          delaySamples;	/* alertDelay in samples */
double       samplePeriod;	/* same as awPeriod */
static double partialSum;	/* forec sum so far */ 
char         *debugMsg = NULL; /* space for debugging msg; no debugging if NULL */
char         *debugMsgTail; /* tail of message (for concatenating) */
int          debugMsgSpace; /* amount of space left in debugMsg */
double       sampleRate;	/* sample rate of data in window */
int          sampleCount;	/* # samples in a TRACEBUF packet */


/* Functions in this source file
 *******************************/
void  ewaccel_config  ( char * );
void  ewaccel_lookup  ( void );
void  ewaccel_status  ( unsigned char, short, char * );
void  ewaccel_free    ( void );
void  Swap( void *data, int size );
int   MakeLocal( TRACE2_HEADER *tbh2x );
int Filter (TracePacket *inBuf);
void writeAlertMessage( int off );
static int InitSCNL();
static int ResetSCNL( TracePacket* );

/* Thread things
 ***************/
#define THREAD_STACK 8192
static unsigned tidStacker;          /* Thread moving messages from transport */
                                     /*   to queue */

#define MSGSTK_OFF    0              /* MessageStacker has not been started      */
#define MSGSTK_ALIVE  1              /* MessageStacker alive and well            */
#define MSGSTK_ERR   -1              /* MessageStacker encountered error quit    */
volatile int MessageStackerStatus = MSGSTK_OFF;

QUEUE OutQueue;              /* from queue.h, queue.c; sets up linked    */
                                     /*    list via malloc and free              */
thr_ret MessageStacker( void * );    /* used to pass messages between main thread */
                                     /*   and Process thread */
thr_ret Process( void * );

/* Message Buffers to be allocated
 *********************************/
static char *MSrawmsg = NULL;        /* MessageStacker's "raw" retrieved message */
static char *SSmsg = NULL;        /* MessageStacker's "raw" retrieved message */

/* Timers
   ******/
time_t now;        /* current time, used for timing heartbeats */
time_t MyLastBeat;         /* time of last local (into Earthworm) hearbeat */

extern int  errno;

static  SHM_INFO  InRegion;     /* shared memory region to use for input  */
static  SHM_INFO  OutRegion;    /* shared memory region to use for output */

MSG_LOGO  GetLogo;             /* array for requesting module,type,instid */

char *Argv0;            /* pointer to executable name */
pid_t MyPid;        /* Our own pid, sent with heartbeat for restart purposes */
static unsigned tidProcess;

/* Things to read or derive from configuration file
 **************************************************/
static char    InRing[MAX_RING_STR];          /* name of transport ring for input  */
static char    OutRing[MAX_RING_STR];         /* name of transport ring for output */
static char    MyModName[MAX_MOD_STR];        /* speak as this module name/id      */
static int     LogSwitch;           /* 0 if no logfile should be written */
static int     HeartBeatInt;        /* seconds between heartbeats        */
static long    MaxMsgSize;          /* max size for input/output msgs    */
static int     RingSize = 10;        /* max messages in output circular buffer       */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          InRingKey;     /* key of transport ring for input    */
static long          OutRingKey;    /* key of transport ring for output   */
static unsigned char InstId;        /* local installation id              */
static unsigned char InstWildcard;
static unsigned char MyModId;       /* Module Id for this program         */
static unsigned char ModWildcard;
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char Type_Tracebuf2;
static unsigned char TypeThreshAlert;


/* Error messages used by export
 ***********************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring        */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer      */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded  */
#define  ERR_QUEUE         3   /* problem w/ queue                        */
static char  errText[256];     /* string for log/error messages           */
	
#define NUMREQ 11			/* # of required commands you expect to process   */
#ifdef _WINNT
/* Handle deprecation of strdup in windows */
static char* mystrdup( const char* src ) {
	char* dest = malloc( strlen(src) + 1 );
	if ( dest != NULL )
		strcpy( dest, src );
	return dest;
}
#else
#define mystrdup strdup
#endif

int main( int argc, char **argv )
{
/* Other variables: */
   int           res;
   long          recsize;   /* size of retrieved message             */
   MSG_LOGO      reclogo;   /* logo of retrieved message             */

   /* Check command line arguments
   ******************************/
   Argv0 = argv[0];
   if ( argc != 2 )
   {
      fprintf( stderr, "Usage: %s <configfile>\n", Argv0 );
      fprintf( stderr, "Version %s\n", ACCEL_VERSION );
      return( 0 );
   }

   /* Initialize name of log-file & open it
   ****************************************/
   logit_init( argv[1], 0, 512, 1 );

   /* Read the configuration file(s)
   ********************************/
   ewaccel_config( argv[1] );
   logit( "et" , "%s(%s): Read command file <%s>\n",
           Argv0, MyModName, argv[1] );
   logit("t", "starting ewaccel version %s\n", ACCEL_VERSION);

   /* Look up important info from earthworm.h tables
   *************************************************/
   ewaccel_lookup();

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
   
   /* Prep for debiasing/filtering/alert-checking 
   ***********************************************************/
	InitSCNL();

   /* Allocate space for input/output messages for all threads
   ***********************************************************/

   /* Buffers for the MessageStacker thread: */
   if ( ( MSrawmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL )
   {
      logit( "e", "%s(%s): error allocating MSrawmsg; exiting!\n",
             Argv0, MyModName );
      ewaccel_free();
      return( -1 );
   }

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
     res = tport_getmsg( &InRegion, &GetLogo, 1,
                         &reclogo, &recsize, MSrawmsg, MaxMsgSize );
   } while (res !=GET_NONE);

   /* One heartbeat to announce ourselves to statmgr
   ************************************************/
   ewaccel_status( TypeHeartBeat, 0, "" );
   time(&MyLastBeat);


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

   /* Buffers for Process thread: */
   if ( ( SSmsg = (char *) malloc(MaxMsgSize+1) ) ==  NULL )
   {
	  logit( "e", "%s(%s): error allocating SSmsg; exiting!\n",
			  Argv0, MyModName );
			tport_detach( &InRegion );
			tport_detach( &OutRegion );
			exit(1);
   }

   /* Create a Mutex to control access to queue
   ********************************************/
   CreateMutex_ew();

   /* Initialize the message queue
   *******************************/
   initqueue( &OutQueue, (unsigned long)RingSize,(unsigned long)MaxMsgSize+1 );

   /* Start the socket writing thread
   ***********************************/
   if ( StartThread(  Process, (unsigned)THREAD_STACK, &tidProcess ) == -1 )
   {
	  logit( "e", "%s(%s): Error starting Process thread; exiting!\n",
			  Argv0, MyModName );
	  tport_detach( &InRegion );
	  tport_detach( &OutRegion );
	  free( SSmsg );
	  exit( -1 );
   }

   /* Start main ewaccel service loop
   **********************************/
   while( tport_getflag( &InRegion ) != TERMINATE  &&
          tport_getflag( &InRegion ) != MyPid         )
   {
     /* Beat the heart into the transport ring
      ****************************************/
      time(&now);
      if (difftime(now,MyLastBeat) > (double)HeartBeatInt )
      {
          ewaccel_status( TypeHeartBeat, 0, "" );
      time(&MyLastBeat);
      }

      /* take a brief nap; added 970624:ldd
       ************************************/
      sleep_ew(500);
   } /*end while of monitoring loop */

   /* Shut it down
   ***************/
   tport_detach( &InRegion );
   tport_detach( &OutRegion );
   ewaccel_free();
   logit("t", "%s(%s): termination requested; exiting!\n",
          Argv0, MyModName );
   return( 0 );
}
/* *******************  end of main *******************************
 ******************************************************************/

/************************  Main Process Thread   *********************
*          Pull a messsage from the queue, parse & run scripts       *
**********************************************************************/

thr_ret Process( void *dummy )
{
   int      ret;
   long     msgSize;
   MSG_LOGO Logo;       /* logo of retrieved message             */

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

     /* Process retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK)
     ***********************************************************/ 
     Filter( (TracePacket*)SSmsg );
     
   }  /* End of main loop */
   return THR_NULL_RET;
}


/********************** Message Stacking Thread *******************
 *           Move messages from transport to memory queue         *
 ******************************************************************/
thr_ret MessageStacker( void *dummy )
{
   long          recsize;   /* size of retrieved message             */
   MSG_LOGO      reclogo;       /* logo of retrieved message             */
   int      ret;
   int       res;
   TRACE2_HEADER  	*tbh2 = (TRACE2_HEADER*)MSrawmsg;
   int				data_size;
   int 				sncl_idx;
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
      res = tport_getmsg( &InRegion, &GetLogo, 1,
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
            ewaccel_status( TypeError, ERR_TOOBIG, errText );
            continue;
         }
         else if( res==GET_MISS )
         {
            sprintf( errText, "missed msg(s) i%d m%d t%d in %s",(int) reclogo.instid,
                    (int) reclogo.mod, (int)reclogo.type, InRing );
            ewaccel_status( TypeError, ERR_MISSMSG, errText );
         }
         else if( res==GET_NOTRACK )
         {
            sprintf( errText, "no tracking for logo i%d m%d t%d in %s",
                     (int) reclogo.instid, (int) reclogo.mod, (int)reclogo.type,
                     InRing );
            ewaccel_status( TypeError, ERR_NOTRACK, errText );
         }
      }

      /* Process retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK)
      ***********************************************************/

      /* First, localize
      ***********************************************/
/*
      data_size = MakeLocal( tbh2 );
      if ( data_size == 0 )
      	 continue;
*/
      if ( WaveMsg2MakeLocal(tbh2) != 0) 
         continue;

      if ( tbh2->datatype[1] == '2' )
         data_size = 2;
      else if ( tbh2->datatype[1] == '4' )
         data_size = 4;

      /* Next, see if it matches one of our SNCLs
      *********************************************/
      for ( sncl_idx=0; sncl_idx<nFloor; sncl_idx++ )
      	if ( strcmp( flr[sncl_idx].sta,  tbh2->sta )  == 0 &&
      		 strcmp( flr[sncl_idx].net,  tbh2->net )  == 0 &&
      		 strcmp( flr[sncl_idx].chan, tbh2->chan ) == 0 &&
      		 strcmp( flr[sncl_idx].loc,  tbh2->loc )  == 0 )
      		break;
      if ( sncl_idx >= nFloor )
      	continue;
      tbh2->pad[0] = (char)sncl_idx;
      	
      RequestMutex();
      ret=enqueue( &OutQueue, MSrawmsg, recsize, reclogo );
      ReleaseMutex_ew();

      if ( ret!= 0 )
      {
         if (ret==-2)  /* Serious: quit */
         {    /* Currently, eneueue() in mem_circ_queue.c never returns this error. */
        sprintf(errText,"internal queue error. Terminating.");
            ewaccel_status( TypeError, ERR_QUEUE, errText );
        goto error;
         }
         if (ret==-1)
         {
            sprintf(errText,"queue cannot allocate memory. Lost message.");
            ewaccel_status( TypeError, ERR_QUEUE, errText );
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
   return THR_NULL_RET;
}

/*****************************************************************************
 *  ewaccel_config() processes command file(s) using kom.c functions;         *
 *                    exits if any errors are encountered.               *
 *****************************************************************************/
void ewaccel_config( char *configfile )
{
   char     init[NUMREQ]; /* init flags, one byte for each required command */
   int      nmiss;        /* number of required commands that were missed   */
   char    *com;
   char     processor[20];
   int      nfiles;
   int      success;
   int      i;
   char    *str, *str2;

/* Set to zero one init flag for each required command
 *****************************************************/
   for( i=0; i<NUMREQ; i++ )  init[i] = 0;

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
            strcpy( processor, "ewaccel_config" );

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

         /* Maximum size (bytes) for incoming/outgoing messages
          *****************************************************/
  /*5*/     else if( k_its("MaxMsgSize") ) {
                MaxMsgSize = k_long();
                init[5] = 1;
            }

  /*6,7*/   else if( k_its("Floor") || k_its("GroundFloor") ) {
  				if ( k_its("Floor") ) {
  					str = k_str();
  					for ( i=1; i<nFloor; i++ )
  						if ( flr[i].id != NULL && strcmp(flr[i].id, str)==0 ) {
  							logit( "e", "%s: <Floor> id '%s' repeated; exiting!\n",
  								Argv0, str );
  							exit(-1);
  						}
  					str=mystrdup(str);
  				} else {
  					if ( nFloor>0 && flr[0].id != NULL ) {
						logit( "e", "%s: <GroundFloor> repeated; exiting!\n",
					 	Argv0);
						exit(-1);
					}
  					str = "base";	/* base floor is always at index 0 */
  					i = 0;
  				}
 				
				if ( flr == NULL || nFloor == nFloorAlloc ) {
					nFloorAlloc *= 2;
					flr = (EW_FLOOR_DEF*)realloc( flr, sizeof(EW_FLOOR_DEF) * nFloorAlloc );
					if ( flr == NULL ) {
						if ( str == NULL )
							logit( "e", "%s: Failed to allocate memory for ground floor; exiting\n",
								Argv0, str );
						else
							logit( "e", "%s: Failed to allocate memory for floor '%s'; exiting\n",
								Argv0, str );
						exit(-1);
					}
					if ( nFloor == 0 )
						flr[0].mass = 0.0; 
				}
				flr[i].id = str;
				if ( i == 0 ) {
					init[7] = 1;
					/* "mass" of base floor is -(sum of other floor masses) */
					flr[i].mass -= k_val();
				} else {
					init[6] = 1;
					flr[i].mass = k_val();
				}
				strcpy( flr[i].sta, k_str() );
				strcpy( flr[i].chan, k_str() );
				strcpy( flr[i].net, k_str() );
				strcpy( flr[i].loc, k_str() );
				if ( !k_err() ) {
					str2 = k_str();
					if ( str2 == NULL ) 
						if ( defaultConversionSet ) 
							flr[i].convFactor = defaultConversion;
						else {
							logit( "e", "%s: <%sFloor> id '%s' missing conversion without default; exiting!\n",
								Argv0, i==0 ? "" : "Ground", str );
							exit(-1);
						}
					else
						flr[i].convFactor = atof( str2 );
					flr[i].read = flr[i].windowActive = 0;
					nFloor++;
					continue;
				}
			}
  /*8*/     else if( k_its("BaseShearThreshold") ) {
                baseShearThreshold = k_val();
                init[8] = 1;
            }
  /*9*/     else if( k_its("AvgWindow") ) {
                avgWindowLen = k_val();
                init[9] = 1;
            }
  /*10*/    else if( k_its("AlertDelay") ) {
                alertDelay = k_int();
                init[10] = 1;
            }
  		    else if( k_its("HighPassFreq") ) {
                hpFreq = k_val();
            }
            else if( k_its("HighPassOrder") ) {
                i = k_int();
                if ( i % 2 ) {
					logit( "e" , "%s: <HighPassOrder> must be even; ignoring.\n",
							 Argv0 );
					continue;
				}
				hpOrder = i;
            }
		    else if( k_its("DefaultConversion") ) {
		    	if ( defaultConversionSet ) {
					logit( "e", "%s: <DefaultConversion> repeated; exiting!\n",
					Argv0);
					exit(-1);
				}    		
                defaultConversion = k_val();
                defaultConversionSet = 1;
            }
		    else if( k_its("DebugForces") ) {
                exportForces = 1;
            }


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
   for ( i=0; i<NUMREQ; i++ )  if( !init[i] ) nmiss++;
   if ( nmiss ) {
       logit( "e", "%s: ERROR, no ", Argv0 );
       if ( !init[0] )  logit( "e", "<LogFile> "      );
       if ( !init[1] )  logit( "e", "<MyModuleId> "   );
       if ( !init[2] )  logit( "e", "<InRing> "     );
       if ( !init[3] )  logit( "e", "<OutRing> "     );
       if ( !init[4] )  logit( "e", "<HeartBeatInt> " );
       if ( !init[5] )  logit( "e", "<MaxMsgSize> "  );
       if ( !init[6] )  logit( "e", "<Floor> "  );
       if ( !init[7] )  logit( "e", "<GroundFloor> "  );
       if ( !init[8] )  logit( "e", "<BaseShearThreshold> "  );
       if ( !init[9] )  logit( "e", "<AvgWindow> "  );
       if ( !init[10])  logit( "e", "<AlertDelay> "  );
       logit( "e" , "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }
   if ( (hpFreq!=0.0 && hpOrder==0) || (hpFreq==0.0 && hpOrder!=0) ) {
   	logit( "e", "%s: Filter requires both <HighPassFreq> and <HighPassOrder>; exiting!\n",
   		Argv0 );
   	exit(-1);
   }
   return;
}

/****************************************************************************
 *  ewaccel_lookup( )   Look up important info from earthworm.h tables       *
 ****************************************************************************/
void ewaccel_lookup( void )
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
  if ( GetInst( "INST_WILDCARD", &InstWildcard ) != 0 ) {
     fprintf(stderr, "%s: Invalid instid <INST_WILDCARD>!\n", Argv0 );
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
  if ( GetModId( "MOD_WILDCARD", &ModWildcard ) != 0 ) {
     fprintf(stderr, "%s: Invalid moduleid <MOD_WILDCARD>!\n", Argv0 );
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
  if ( GetType( "TYPE_TRACEBUF2", &Type_Tracebuf2 ) != 0 ) {
     fprintf(stderr, "%s: Invalid message type <TYPE_TRACEBUF2>!\n", Argv0 );
     exit( -1 );
  }
  if ( GetType( "TYPE_THRESH_ALERT", &TypeThreshAlert ) != 0 ) {
     fprintf( stderr, "%s: Invalid message type <TYPE_THRESH_ALERT>!\n", Argv0 );
     exit( -1 );
  }
  
  GetLogo.mod = ModWildcard;
  GetLogo.instid = InstWildcard;
  GetLogo.type = Type_Tracebuf2;

   return;
}

/***************************************************************************
 * ewaccel_status() builds a heartbeat or error message & puts it into      *
 *                 shared memory.  Writes errors to log file & screen.     *
 ***************************************************************************/
void ewaccel_status( unsigned char type, short ierr, char *note )
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
    sprintf( msg, "%ld %ld\n%c", (long) t, (long) MyPid, 0);
   else if( type == TypeError )
   {
    sprintf( msg, "%ld %hd %s\n%c", (long) t, ierr, note, 0);

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
 * writeAlertMessage() builds & puts alert message into shared memory.     *                 
 ***************************************************************************/
void writeAlertMessage( int off )
{
   MSG_LOGO    logo;
   char        msg[256];
   time_t      t = (time_t)(awStartTime+samplePeriod*off);
   struct tm   theTimeStruct;

/* Build the message
 *******************/
   logo.instid = InstId;
   logo.mod    = MyModId;
   logo.type   = TypeThreshAlert;

   gmtime_ew( &t, &theTimeStruct );

   sprintf( msg, "Thresh=%1lf ForceDiff=%lf Time=%s%c",
 	 baseShearThreshold, partialSum, asctime( &theTimeStruct ), 0 );
   if ( tport_putmsg( &OutRegion, &logo, (long)strlen(msg)+1, msg ) != PUT_OK ) {
	  logit("et", "%s:  Error writing threshold message to ring.\n",
			  Argv0 );
   }
}

/***************************************************************************
 * addForceToDump() adds a value to a debugging message of computed        *
 *     force sums, writing out as a message into shared memory when full.  *
 *     off is the index into the current packet; if -1, write current msg  *
 ***************************************************************************/
void addForceToDump( int off )
{
	int len;
	if ( debugMsg == NULL ) {
		if ( off == -1 )
			return;
		debugMsg = malloc( 4096 );
		if ( debugMsg == NULL ) {
			logit( "e", "%s: Failed to allocate debugging message; exiting!\n",
				Argv0 );
			exit(-1);
		}
		debugMsgTail = debugMsg;
		debugMsgSpace = 4095;
	}
	
	if ( debugMsg == debugMsgTail ) {
		if ( off == -1 )
			return;
		len = sprintf( debugMsg, "%s %s", "Time", "ForceDiff" );
		debugMsgTail += len;
		debugMsgSpace = 4096 - len;
	}
	if ( off == -1 )
		len = debugMsgSpace+1;
	else
		len = snprintf( debugMsgTail, debugMsgSpace, "\n%lf %lf", 
			awStartTime+samplePeriod*off, partialSum );
	if ( len > debugMsgSpace ) {
   		MSG_LOGO    logo;

		*debugMsgTail = 0;

		/* Build the message
		*******************/
		logo.instid = InstId;
		logo.mod    = MyModId;
		logo.type   = 0;
		
		if ( tport_putmsg( &OutRegion, &logo, (long)(debugMsgTail - debugMsg + 1), debugMsg ) != PUT_OK ) {
		  logit("et", "%s:  Error writing debugging message to ring.\n",
				  Argv0 );
		}
		
		debugMsgTail = debugMsg;
		if ( off != -1 )
			addForceToDump( off );
	} else
		debugMsgTail += len;
}

/***************************************************************************
 * ewaccel_free()  free all previously allocated memory                     *
 ***************************************************************************/
void ewaccel_free( void )
{
   free (MSrawmsg);             /* MessageStacker's "raw" retrieved message */
   return;
}

/*****************************************************************************
 *  Swap(): Reverse the byte ordering of the size bytes of *data             *
 *****************************************************************************/
void Swap( void *data, int size )
{
   char temp, *cdat = (char*)data;
   int i,j;
   for ( i=0, j=size-1; i<j; i++, j-- ) {
   		temp = cdat[i]; cdat[i] = cdat[j]; cdat[j] = temp;
   }

}

/*****************************************************************************
 *  MakeLocal(): Convert elements of *tbh2x to this processor's endianess    *
 *****************************************************************************/
int MakeLocal( TRACE2_HEADER *tbh2 ) {
	int size = 1;
#if defined (_SPARC)
	if ( tbh2->datatype[0] == 's' )
		size = -1;
	else if ( tbh2->datatype[0] != 'i' ) {
		logit( "e", "ewaccel: unknown datatype: '%s'", tbh2->datatype );
		return 0;
	}
#elif defined (_INTEL)
	if ( tbh2->datatype[0] == 'i' )
		size = -1;
	else if ( tbh2->datatype[0] != 's' ) {
		logit( "e", "ewaccel: unknown datatype: '%s'", tbh2->datatype );
		return 0;
	}
#else
#error "_INTEL or _SPARC must be set before compiling"
#endif
	else {
		Swap( &tbh2->starttime, sizeof(double) );
		Swap( &tbh2->nsamp, sizeof(int) );
	}
	if ( tbh2->datatype[1] == '2' )
		return size * 2;
	else if ( tbh2->datatype[1] == '4' )
		return size * 4;
	else {
		logit( "e", "ewaccel: unknown datatype size: '%s'", tbh2->datatype );
		return 0;
	}
}

/* Called at start of a run: first packet or when a packet "breaks" the current run */
static int ResetSCNL( TracePacket *inBuf )
{
  int i,j;
  size_t bytesNeeded;        /* size of window in bytes */
  
  dataSize = inBuf->trh2.datatype[1]-'0';

  /* Calc nbr samples needed for length of window, and estimate nbr of packets 
	 for that many samples */
  awSamples = (int)(inBuf->trh2.samprate * avgWindowLen);

  if ( awSamples < 2 ) 
  {
	logit( "e", "ewaccel: WARNING: AvgWindow too low to hold more than 1 sample; lengthened to hold 2\n" );
	awSamples = 2;
  }
  
  samplePeriod = 1.0/inBuf->trh2.samprate;
  delaySamples = (int)(inBuf->trh2.samprate * alertDelay);
    
  /* (Re-)allocation of space for window of samples */
  bytesNeeded = awSamples * dataSize;
  if ( bytesNeeded > awBytesAllocated ) 
  {
  	for ( i=0; i<nFloor; i++ ) {
		if ( awBytesAllocated == 0 )
		  avgWindow[i] = (void*)calloc( awSamples, dataSize );
		else
		  avgWindow[i] = (void*)realloc( avgWindow+i, bytesNeeded );
		if ( avgWindow[i] == NULL )
		{
		  logit( "e", "ewaccel: failed to allocate an averaging window\n" );
		  return EW_FAILURE;
		}
		awSum[i] = 0.0;
	}
	awBytesAllocated = (int)bytesNeeded;
  }
  
  /* Reset counters/flags */
  awPosn = awFull = 0;
  awEmpty = 1;
  
  awPeriod = 1.0 / inBuf->trh2.samprate;
  
  sampleCount = inBuf->trh2.nsamp;
  sampleRate = inBuf->trh2.samprate;

  if ( nHPLevels > 0 ) 
  {
    complex *poles = hpPoles;
    int idx = 0;

    highpass( hpFreq, awPeriod, hpOrder, poles, &hpGain );
  
    for ( j=0; j<nFloor; j++ ) {
		for ( i=0; i<nHPLevels; i++ ) 
		{
		  hpLevel[idx].d1 = hpLevel[i].d2 = 0.0;
		  hpLevel[idx].a1 = -2.0 * poles[i+i].real;
		  hpLevel[idx].a2 = poles[i+i].real*poles[i+i].real + poles[i+i].imag*poles[i+i].imag;
		  hpLevel[idx].b1 = -2.0;
		  hpLevel[idx].b2 = 1.0;
		  idx++;
		}
	}
  }
  
  return EW_SUCCESS;
} 

/***************************************************************************
 * InitSCNL()  Initializations for various stuff                           *
 ***************************************************************************/
static int InitSCNL()
{
  int i;
  complex *poles;

  /* Debias fields */
  avgWindow = NULL;
  awPosn = awBytesAllocated = awSamples = awFull = dataSize = 0;
  awEmpty = 1;
  awSum = calloc( nFloor, sizeof(double) );
  if ( awSum == NULL ) 
  {
      logit ("et", "ewaccel: Failed to allocate summations; exiting.\n" );
      return EW_FAILURE;
  }


  if ( avgWindowLen == 0 ) 
    awSamples = awFull = 1;
    
  /* Integrate & filter fields */
  hpLevel = NULL;
  nHPLevels = (hpOrder+1)/2;
  if ( nHPLevels > 0 )
  {            
    HP_STAGE* stages = (HP_STAGE*)calloc( nHPLevels * nFloor, sizeof(HP_STAGE) );
    if ( stages == NULL ) 
    {
      logit ("et", "ewaccel: Failed to allocate SCNL info stages; exiting.\n" );
      return EW_FAILURE;
    }
    else
      hpLevel = stages;
    poles = (complex*)calloc( sizeof(complex), hpOrder);
    if ( poles == NULL )
    {
      logit ("t", "ewaccel: Failed to allocate SCNL info poles; exiting.\n" );
      free( stages );
      return EW_FAILURE;
    }
    else
      hpPoles = poles;
      
    for ( i=0; i<nFloor*nHPLevels; i++ )
      stages[i].d1 = stages[i].d2 = stages[i].f = 0; 
  }
  floorsLeft = nFloor;
  awStartTime = -1;
  
  avgWindow = (double **)calloc( nFloor, sizeof(double*) );
  if ( avgWindow == NULL ) 
  {
      logit ("et", "ewaccel: Failed to allocate averaging windows; exiting.\n" );
      return EW_FAILURE;
  }
  
  return EW_SUCCESS;   
}

/***************************************************************************
 * ProcessPacket()  Process datum at offset off fro latest packet          *
 *                  for floor f                                            *
 ***************************************************************************/
static void ProcessPacket (double datum, int f, int off )
{
  int i;
  double out, next_out;  
  HP_STAGE *lvl0 = hpLevel+(f*nFloor);
  if ( floorsLeft==nFloor )
	  partialSum =  0.0;

  /* Do the filter */
  out = datum;
  for ( i=0; i<nHPLevels; i++ )
  {
    HP_STAGE *lvl = lvl0+i;
    next_out = out + lvl->d1;
	lvl->d1 = lvl->b1*out - lvl->a1*next_out + lvl->d2 ;
	lvl->d2 = lvl->b2*out - lvl->a2*next_out ;
	out = next_out;
  }
  if ( nHPLevels>0 )
  	out *= hpGain;
  
  partialSum += flr[f].mass * flr[f].convFactor * out;
  if ( floorsLeft==1 ) {
  	  if ( exportForces )
  	  	addForceToDump( off );
	  if ( alertOn ) {
		/* if ( partialSum < baseShearThreshold ) {
			fprintf(stderr, "Alert reset (%lf < %lf @ %lf)!\n", partialSum, baseShearThreshold, off);
			alertOn = 0;
		} else */
			alertOn--;
			/* if ( alertOn == 0 )
				fprintf(stderr, "Alert reset @ %lf!\n", 
					awStartTime+samplePeriod*off);
			else
				fprintf(stderr, "ao=%d\n", alertOn ); */
	  } else if ( partialSum > baseShearThreshold ) {
		alertOn = delaySamples;
		logit("", "Alert goes off (%lf > %lf @ %lf)!\n", 
			partialSum, baseShearThreshold, awStartTime+samplePeriod*off);
		writeAlertMessage( off );
	  }
  }
}

/***************************************************************************
 * Filter()  Process latest packet                                         *
 ***************************************************************************/
int Filter (TracePacket *inBuf)
{
  short *waveShort = NULL, *awShort = NULL;
  int32_t  *waveLong = NULL,  *awLong = NULL;
  int i, j;
  int posn = 0;
  char debiasing = (avgWindowLen != 0);
  double datum;
  TRACE2_HEADER* tbh2 = (TRACE2_HEADER*)inBuf;
  int f = tbh2->pad[0];  

  /* 
  fprintf( stderr, "Filtering floor %s @ %f (fl=%d) ao=%d\n", 
  	flr[f].id, tbh2->starttime, floorsLeft, alertOn );
	*/
	
  if ( flr[f].read || (floorsLeft<nFloor && tbh2->starttime != awStartTime) ) {
    if ( awStartTime != -1 ) {
		logit( "w", "%s: %spacket for floor %s @ %f before %f done\n",
			Argv0, flr[f].read ? "new " : "", 
			flr[f].id,
			tbh2->starttime, awStartTime);
	}
  	ResetSCNL( inBuf );
  } else if ( awStartTime == -1 )
  	ResetSCNL( inBuf );
  else {
  	if ( sampleRate != inBuf->trh2.samprate ) {
  		logit( "e", "ewaccel: floor '%s' has different sample rate (%lf vs %lf); exiting!\n",
  			flr[f].id, sampleRate, inBuf->trh2.samprate );
  		exit(-1);
  	}
  	if ( sampleCount != inBuf->trh2.nsamp ) {
  		logit( "e", "ewaccel: floor '%s' has different sample count (%d vs %d); exiting!\n",
  			flr[f].id, sampleCount, inBuf->trh2.nsamp );
  		exit(-1);
  	}
  	
  }
  
  /* Set some useful pointers */
  if ( dataSize==2 ) 
  {
    waveShort = (short*)(inBuf->msg + sizeof(TRACE2_HEADER));
  } else {
    waveLong  = (int32_t*) (inBuf->msg + sizeof(TRACE2_HEADER));
  }

  if ( debiasing ) 
  {
      posn = awPosn;
      /* Get properly-typed pointer to window */
      if ( dataSize==2 )
        awShort = (short*)(avgWindow[f]);
      else
        awLong = (int32_t*)(avgWindow[f]);
  }

  for ( i=0; i<inBuf->trh2.nsamp; i++ )
  {
    if ( debiasing )
    {
        /* If window in full, remove the oldest entry from sum */
        if ( awFull )
        {
          int oldest_posn = (posn==awSamples ? 0 : posn);
          awSum[f] -= (dataSize==2 ? awShort[oldest_posn] : awLong[oldest_posn]);
        }
    
        /* Add newest entry to sum & window */
        if ( dataSize==2 )
          datum = awShort[posn++] = waveShort[i];
        else
          datum = awLong[posn++] = waveLong[i];
        awSum[f] += datum;
    } else
    {
        datum = (dataSize==2 ? waveShort[i] : waveLong[i]);
    }
    
    /* If window is full, remove bias from latest value */
    if ( awFull )
    {
      double bias = awSum[f] / awSamples;
      /*fprintf( stderr, "C[%d][%3d] = %9.3lf (%9.3lf - %9.3lf)\n", f, j, datum-bias, datum, bias );*/
      ProcessPacket (datum - bias, f, i );
      if ( floorsLeft==1 && posn == awSamples )
        posn = 0;
    }
    /* If we filled window for first time, flush the cache after debiasing it */
    else if ( posn == awSamples )
    {
      double bias = awSum[f] / awSamples;

      if ( awFull == 0 )
      {
        for ( j=0; j<awSamples; j++ )
        {
            double datumVal = (dataSize==2 ? awShort[j] : awLong[j]);
            /*fprintf( stderr, "C[%d][%3d] = %9.3lf (%9.3lf - %9.3lf)\n", f, j, datum-bias, datum, bias );*/
            ProcessPacket (datumVal - bias, f, j );
        }
        
        if ( floorsLeft==1 ) {
        	/* fprintf( stderr, "Window now full!\n" ); */
        	awFull = 1;
        }
      } 
      if ( floorsLeft==1 )
      	posn = 0;
    }
  }
  if ( floorsLeft==1 && exportForces )
  	addForceToDump( -1 );
  floorsLeft--;
  awEmpty = 0;
  if ( floorsLeft == nFloor-1 )
	awStartTime = tbh2->starttime;
  else if ( floorsLeft == 0 ) {
	awPosn = posn;
	floorsLeft = nFloor;
  }

  /* fprintf( stderr, "Done Filtering ao=%d\n", alertOn ); */
  
  return EW_SUCCESS;
}
