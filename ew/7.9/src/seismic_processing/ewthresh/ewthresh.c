/******************************************************************************
 *
 *      File:                   ewthresh_TLSalarm.c
 *
 *      Function:               Program to read TRACEBUF2X messages and report (via a
 *                  		    message to another ring) stations at which a converted 
 *                              value exceeds a specified threshold.
 * 								Additionally, the module works as a traffic-light-system (TLS) alert
 * 								with acoustic notification for two different TLS levels
 *
 *      Author(s):              Scott Hunter, ISTI
 *
 *      Source:                 based on ewthresh.c
 *
 *      Notes: 					needs testing
 *
 *      Change History:
 *                      8/20/15 modified for demeaning data
 *                              correct coincidence trigger and acoustic alarm
 *                      4/26/11 Started source
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
#ifdef _WINNT 
#include <windows.h>
#include <mmsystem.h>           /* PlaySound */
#endif

#include <stdio.h>
#include <string.h>


#ifdef _WINNT 
#pragma comment(lib,"winmm.lib")  //for MSV C++  
	BOOL play(char *name)
	{
	  /*fprintf(stderr,"current alarmsound:  <%s>\n",name);*/
	  PlaySound(name, NULL, SND_FILENAME | SND_ASYNC);
	  return 0;
	}
#endif

#define THRESH_VERSION "0.2.1 2016-04-13"

typedef struct {
        char    sta[TRACE2_STA_LEN];   /* Site name (NULL-terminated) */
        char    net[TRACE2_NET_LEN];   /* Network name (NULL-terminated) */
        char    chan[TRACE2_CHAN_LEN]; /* Component/channel code (NULL-terminated)*/
        char    loc[TRACE2_LOC_LEN];   /* Location code (NULL-terminated) */
        double  threshold;             /* threshold to trigger alert for this SNCL */
        double  conversion;             /* convert from counts to whatever threshold units are */
        time_t  lastSent;               /* time the last time a THRESH message was sent */    
} EW_THRESHOLD_DEF;

#define MAX_THRESH_DEFS 1000    /* maximum number of SNCL/threshold definitions */

#define CSU_VERSION     "v2.0"  /* version of TYPE_TRIGLIST_SCNL        */
#define TVMSG_HDR_LEN 100
#define TVMSG_SCNL_LEN 100

static double  Threshold_yellow=0.0;
static double  Threshold_red=0.0;
static char alarmsound_yellow[128]="nofile";
static char alarmsound_red[128]="nofile";
static long SecsBetweenThreshMsgs = 0;
static long NetStaWait = 0;
static time_t NoMessagesBefore;



EW_THRESHOLD_DEF threshDef[MAX_THRESH_DEFS];    /* threshold definitions */

int nThreshDefs = 0;                                                    /* and how many of them */

/* Functions in this source file
 *******************************/
void  ewthresh_config  ( char * );
void  ewthresh_lookup  ( void );
void  ewthresh_status  ( unsigned char, short, char * );
void  ewthresh_free    ( void );
void  Swap( void *data, int size );
int   MakeLocal( TRACE2X_HEADER *tbh2x );


/* Thread things
 ***************/
#define THREAD_STACK 8192
static unsigned tidStacker;          /* Thread moving messages from transport */
                                     /*   to queue */

#define MSGSTK_OFF    0              /* MessageStacker has not been started      */
#define MSGSTK_ALIVE  1              /* MessageStacker alive and well            */
#define MSGSTK_ERR   -1              /* MessageStacker encountered error quit    */
volatile int MessageStackerStatus = MSGSTK_OFF;

thr_ret MessageStacker( void * );    /* used to pass messages between main thread */
                                     /*   and Dup thread */
/* Message Buffers to be allocated
 *********************************/
static char *MSrawmsg = NULL;        /* MessageStacker's "raw" retrieved message */

/* Timers
   ******/
time_t now;        /* current time, used for timing heartbeats */
time_t MyLastBeat;         /* time of last local (into Earthworm) hearbeat */

extern int  errno;

static  SHM_INFO  InRegion;     /* shared memory region to use for input  */
static  SHM_INFO  OutRegion;    /* shared memory region to use for output */

#define   MAXLOGO   10
MSG_LOGO  GetLogo[MAXLOGO];     /* array for requesting module,type,instid */
short     nLogo;

char *Argv0;            /* pointer to executable name */
pid_t MyPid;        /* Our own pid, sent with heartbeat for restart purposes */

/* Things to read or derive from configuration file
 **************************************************/
static char    InRing[MAX_RING_STR];          /* name of transport ring for input  */
static char    OutRing[MAX_RING_STR];         /* name of transport ring for output */
static char    MyModName[MAX_MOD_STR];        /* speak as this module name/id      */
static int     LogSwitch;           /* 0 if no logfile should be written */
static int     HeartBeatInt;        /* seconds between heartbeats        */
static long    MaxMsgSize;          /* max size for input/output msgs    */
static int     DebugLevel = 0;       /* turn on debug messages, off by default */
static int     triggervalue = 0;


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
static unsigned char ThreshAlertType;

static unsigned char TypeTrigList;
static int           tvNumTrig = 0;     /* # of triggers to fire triglist message
                                                                                < 1 means never fire */
static double            tvTimeSpanSeconds;     /* span of triggers to fire triglist message */
static double            tvPreEventSeconds; /* subtract from 1st trigger's time for triglist message */
static int                       tvNumSCNLs;    /* # SCNLs to put in triglist message */
static EW_THRESHOLD_DEF *tvSCNLs;       /* array of SCNLs to put in triglist message */
static time_t            *tvAlertTimes; /* array of most recent alert times */
static int           tvIndex = 0;   /* index of oldest most recent alert time */
static char                      *tvMsg = NULL; /* space for triglist message */
static char          tvFilled = 0;  /* tvAlertTimes has been filled */
static time_t        tvHoldVotingUntil = 0;  /* ignore voting until this time */

void tvAddAlert( time_t t ) {
	tvAlertTimes[tvIndex] = t;
	tvIndex = (tvIndex+1) % tvNumTrig;
	if ( tvIndex == 0 )
		tvFilled = 1;
	if ( tvFilled && (t - tvAlertTimes[tvIndex] <= tvTimeSpanSeconds) ) {
   		MSG_LOGO      reclogo;       /* logo of retrieved message             */
  		time_t	ttTimeOn = tvAlertTimes[tvIndex] - (time_t)tvPreEventSeconds;
  		time_t duration = (time_t)(tvTimeSpanSeconds + tvPreEventSeconds);
		struct tm	tmTimeOn;	/* Event time on as a tm structure.	*/
		char * msgPtr;
		int i;
		
		/* write triglist message */
   		
   		/*	Convert network onTime to a tm structure			*/
  		gmtime_ew( &ttTimeOn, &tmTimeOn );

     	sprintf( tvMsg, 
	       "%s EVENT DETECTED     %4d%02d%02d %02d:%02d:%02d.00 UTC "
	       "EVENT ID: %lu AUTHOR: %03u%03u%03u\n\n",
	       CSU_VERSION,
	       tmTimeOn.tm_year + 1900, tmTimeOn.tm_mon + 1,
	       tmTimeOn.tm_mday, tmTimeOn.tm_hour, tmTimeOn.tm_min, 
	       tmTimeOn.tm_sec, 0L,
	       TypeTrigList, MyModId,
	       InstId );
	    msgPtr = tvMsg + strlen( tvMsg );
  		strcat( msgPtr, "Sta/Cmp/Net/Loc   Date   Time                       "
	  		"start save       duration in sec.\n" );
	    logit( "o", "%s", tvMsg );
	    msgPtr += strlen( msgPtr );
	    strcat( msgPtr, "---------------   ------ --------------- "
	  		"   -----------------------------------------\n" );
	  	logit( "o", "%s", msgPtr );
	  	msgPtr += strlen( msgPtr );
	  	for ( i=0; i<tvNumSCNLs; i++ ) {
	  		sprintf( msgPtr, " %s %s %s %s P %4d%02d%02d %02d:%02d:%02d.%02d UTC  save: %4d%02d%02d %02d:%02d:%02d.00      %ld\n",
		       tvSCNLs[i].sta, tvSCNLs[i].chan,
		       tvSCNLs[i].net, tvSCNLs[i].loc,
		       tmTimeOn.tm_year + 1900, tmTimeOn.tm_mon + 1,
		       tmTimeOn.tm_mday, tmTimeOn.tm_hour, tmTimeOn.tm_min,
		       tmTimeOn.tm_sec, (int)duration,
		       tmTimeOn.tm_year + 1900, tmTimeOn.tm_mon + 1,
		       tmTimeOn.tm_mday, tmTimeOn.tm_hour, tmTimeOn.tm_min,
		       tmTimeOn.tm_sec, (long)duration );
		    logit( "o", "%s", msgPtr );
		    msgPtr += strlen( msgPtr );
	  	}
	  	reclogo.type = TypeTrigList;  /* note the new message type */
		if ( tport_putmsg( &OutRegion, &reclogo, (long)strlen(tvMsg), tvMsg ) != PUT_OK ) {
		  logit("et", "%s:  Error writing triglist message to ring.\n",
				  Argv0 );
		}
		tvFilled = (char)(tvIndex = 0);		/* reset triglist */
		tvHoldVotingUntil = t + (time_t)tvTimeSpanSeconds;
	}	
}

/* Error messages used by export
 ***********************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring        */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer      */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded  */
static char  errText[256];     /* string for log/error messages           */

int main( int argc, char **argv )
{
/* Other variables: */
   int           res;
   long          recsize;   /* size of retrieved message             */
   MSG_LOGO      reclogo;   /* logo of retrieved message             */
   int idx;

   /* Check command line arguments
   ******************************/
   Argv0 = argv[0];
   if ( argc != 2 )
   {
      fprintf( stderr, "Usage: %s <configfile>\n", Argv0 );
      fprintf( stderr, "Version %s\n", THRESH_VERSION );
      return( 0 );
   }

   /* Initialize name of log-file & open it
   ****************************************/
   logit_init( argv[1], 0, 512, 1 );

   /* Read the configuration file(s)
   ********************************/
   time( &NoMessagesBefore );
   ewthresh_config( argv[1] );
   logit( "et" , "%s(%s): Read command file <%s>\n",
           Argv0, MyModName, argv[1] );
   logit("t", "starting ewthresh version %s\n", THRESH_VERSION);

   /* Look up important info from earthworm.h tables
   *************************************************/
   ewthresh_lookup();

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


   /* Allocate space for input/output messages for all threads
   ***********************************************************/

   /* Buffers for the MessageStacker thread: */
   if ( ( MSrawmsg = (char *) calloc(1, MaxMsgSize+1) ) ==  NULL )
   {
      logit( "e", "%s(%s): error allocating MSrawmsg; exiting!\n",
             Argv0, MyModName );
      ewthresh_free();
      return( -1 );
   }
   
   /* Allocate space for dealing w/ triglist messages if needed */
   if ( tvNumTrig > 0 ) {
          if ( ( tvAlertTimes = (time_t*)calloc( tvNumTrig, sizeof(time_t) ) ) == NULL )
          {
                  logit( "e", "%s(%s): error allocating tvAlertTimes; exiting!\n",
                                 Argv0, MyModName );
                  ewthresh_free();
                  return( -1 );
           }
          if ( ( tvMsg = calloc( 1, TVMSG_HDR_LEN + TVMSG_SCNL_LEN * (1+tvNumTrig) ) ) == NULL )
          {
                  logit( "e", "%s(%s): error allocating tvMsg; exiting!\n",
                                 Argv0, MyModName );
                  ewthresh_free();
                  return( -1 );
           }
        }


   /* Attach to Input/Output shared memory ring
   ********************************************/
   tport_attach( &InRegion, InRingKey );
   tport_attach( &OutRegion, OutRingKey );

  /* Specify logos to get
  ***********************/
  nLogo = 1;
  if ( GetType( "TYPE_TRACEBUF2", &Type_Tracebuf2 ) != 0 ) {
     fprintf(stderr, "%s: Invalid message type <TYPE_TRACEBUF2>!\n", argv[0] );
     exit( -1 );
  }
  if ( GetType( "TYPE_THRESH_ALERT", &ThreshAlertType ) != 0 ) {
     fprintf(stderr, "%s: Invalid message type <TYPE_THRESH_ALERT>!\n", argv[0] );
     exit( -1 );
  }
  if ( GetModId( "MOD_WILDCARD", &ModWildcard ) != 0 ) {
     fprintf(stderr, "%s: Invalid moduleid <MOD_WILDCARD>!\n", argv[0] );
     exit( -1 );
  }
  if ( GetInst( "INST_WILDCARD", &InstWildcard ) != 0 ) {
     fprintf(stderr, "%s: Invalid instid <INST_WILDCARD>!\n", argv[0] );
     exit( -1 );
  }

  for( idx=0; idx<nLogo; idx++ ) {
      GetLogo[idx].instid = InstWildcard;
      GetLogo[idx].mod    = ModWildcard;
  }
  GetLogo[0].type = Type_Tracebuf2;
  
  if ( tvNumTrig > 0 )
          if ( GetType( "TYPE_TRIGLIST_SCNL", &TypeTrigList ) != 0 ) {
                 fprintf(stderr, "%s: Invalid message type <TYPE_TRIGLIST_SCNL>!\n", argv[0] );
                 exit( -1 );
          }

   /* step over all messages from transport ring
   *********************************************/
   /* As Lynn pointed out: if we're restarted by startstop after hanging,
      we should throw away any of our messages in the transport ring.
      Else we could end up re-sending a previously sent message, causing
      time to go backwards... */
   do
   {
     res = tport_getmsg( &InRegion, GetLogo, nLogo,
                         &reclogo, &recsize, MSrawmsg, MaxMsgSize );
   } while (res !=GET_NONE);

   /* One heartbeat to announce ourselves to statmgr
   ************************************************/
   ewthresh_status( TypeHeartBeat, 0, "" );
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

   /* Start main ewthresh service loop
   **********************************/
   while( tport_getflag( &InRegion ) != TERMINATE  &&
          tport_getflag( &InRegion ) != MyPid         )
   {
     /* Beat the heart into the transport ring
      ****************************************/
      time(&now);
      if (difftime(now,MyLastBeat) > (double)HeartBeatInt )
      {
          ewthresh_status( TypeHeartBeat, 0, "" );
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
   ewthresh_free();
   logit("t", "%s(%s): termination requested; exiting!\n",
          Argv0, MyModName );
   return( 0 );
}
/* *******************  end of main *******************************
 ******************************************************************/

/********************** Message Stacking Thread *******************
 *           Move messages from transport to memory queue         *
 ******************************************************************/
thr_ret MessageStacker( void *dummy )
{
   long          recsize;   /* size of retrieved message             */
   MSG_LOGO      reclogo;       /* logo of retrieved message             */
   int       res;
   TRACE2X_HEADER       *tbh2x = (TRACE2X_HEADER*)MSrawmsg;
   int                          *long_data  =  (int *)( MSrawmsg + sizeof(TRACE2X_HEADER) );
   short                        *short_data = (short *)( MSrawmsg + sizeof(TRACE2X_HEADER) );
   int                          data_size = 0;
   double                       convFactor;
   int                          sncl_idx, myidx;
   struct tm theTimeStruct;
   int              sum;
   int              meanval = 0;
   time_t theTime;
   int allthreshidx=0;
   
   /* Tell the main thread we're ok
   ********************************/
   MessageStackerStatus = MSGSTK_ALIVE;

   /* Start main export service loop for current connection
   ********************************************************/
   while( 1 )
   {
          long max = 0;
          int maxi = -1;
          double Amax[100];
          int triggerarray[100];
          double Amin;
          double Amin_coinci;
          int i;
          long lastThresh = -1;
          
      /* Get a message from transport ring
      ************************************/
      res = tport_getmsg( &InRegion, GetLogo, nLogo,
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
            ewthresh_status( TypeError, ERR_TOOBIG, errText );
            continue;
         }
         else if( res==GET_MISS )
         {
            sprintf( errText, "missed msg(s) i%d m%d t%d in %s",(int) reclogo.instid,
                    (int) reclogo.mod, (int)reclogo.type, InRing );
            ewthresh_status( TypeError, ERR_MISSMSG, errText );
         }
         else if( res==GET_NOTRACK )
         {
            sprintf( errText, "no tracking for logo i%d m%d t%d in %s",
                     (int) reclogo.instid, (int) reclogo.mod, (int)reclogo.type,
                     InRing );
            ewthresh_status( TypeError, ERR_NOTRACK, errText );
         }
      }

     /* Process retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK)
      ***********************************************************/

      /* First, localize & make sure it is a TRACEBUF2X message
      ***********************************************/
/*
      data_size = MakeLocal( tbh2x );
      if ( data_size == 0 )
         continue;
*/
      if ( WaveMsg2XMakeLocal(tbh2x) != 0) 
         continue;

      if ( tbh2x->datatype[1] == '2' )
         data_size = 2;
      else if ( tbh2x->datatype[1] == '4' )
         data_size = 4;

      convFactor = GET_TRACE2_CONVERSION_FACTOR(tbh2x);
      if ( convFactor == 0.0 )
        convFactor = 1.0;

      /* Next, see if it matches one of our SNCLs
      *********************************************/
      sncl_idx = -1;
      for ( i=0; i<nThreshDefs; i++ )
        if ( strcmp( threshDef[i].sta,  tbh2x->sta )  == 0 &&
                 strcmp( threshDef[i].net,  tbh2x->net )  == 0 ) {
            if ( strcmp( threshDef[i].chan, tbh2x->chan ) == 0 &&
                 strcmp( threshDef[i].loc,  tbh2x->loc )  == 0 ) {
                 sncl_idx = i;
                 if ( !NetStaWait ) {
                    lastThresh = (long)threshDef[i].lastSent;
                    break;
                }
            }
            if ( NetStaWait ) {
                if ( lastThresh < threshDef[i].lastSent )
                    lastThresh =  (long)threshDef[i].lastSent;
            }
        }
          
      if ( sncl_idx == -1 )
        continue;

      /* compute and remove mean value from data
      *******************************************/
      /*fprintf(stderr,"REMOVING MEAN FROM DATA\n");*/
          sum = 0;  
          if ( data_size == 2 )
          {
                /* Shorts */
                for ( i = 0; i < tbh2x->nsamp; i++ ) {

            sum += short_data[i];
                }
                meanval = sum/tbh2x->nsamp;
          }
          if ( data_size == 4 )
          {
                for ( i = 0; i < tbh2x->nsamp; i++ ) {

            sum += long_data[i];
                }
                meanval = sum/tbh2x->nsamp;
          }


      /* Now scan content for maximum value
      *******************************************/
          if ( data_size == 2 )
          {
                /* Shorts */
                for ( i = 0; i < tbh2x->nsamp; i++ ) {
                  short val = (short)abs(short_data[i]-meanval);
                  if ( val > max ){
                        max = val;
                        maxi = i;
                  }
                }
          }
          if ( data_size == 4 )
          {
                /* Local longs */
                for ( i = 0; i < tbh2x->nsamp; i++ ) {
                  long val = abs(long_data[i]-meanval);
                  if ( val > max ) {
                        max = val;
                        maxi = i;
                  }
                }
          }
 
          /* If converted max exceeds threshold, spit out a message
          ************************************************/

                

          if (threshDef[sncl_idx].conversion != 1.0) {
                  /* use the conversion facor from the ConvertedThreshold entry, not from tracebuf2x scale */
                  convFactor = threshDef[sncl_idx].conversion;
          }
                /* This block was moved down by ER (Q-con) 31.07.2015 to show the correct (converted) amplitudes */
          if ( DebugLevel > 1 ) {
                  theTime = (time_t)(tbh2x->starttime + maxi * (1.0/tbh2x->samprate));

                  gmtime_ew( &theTime, &theTimeStruct );
                  fprintf( stderr, "DEBUG: ewthresh: Raw info SNCL=%s.%s.%s.%s Thresh=%lf Current-Max-Value=%lf Time=%s%c",
                        tbh2x->sta, tbh2x->net, tbh2x->chan, tbh2x->loc,
                        threshDef[sncl_idx].threshold, max*convFactor, asctime( &theTimeStruct ), 0 );
          } 
                  Amax[sncl_idx] = max*convFactor;
                  /*fprintf(stderr,"amax:    %lu\n",Amax[sncl_idx]);*/
          
          if ( max*convFactor > threshDef[sncl_idx].threshold ) {
                  char msg[100];
                  time_t curTime;
                  
                  time(&curTime);

                allthreshidx += sncl_idx+1;

                  
                  theTime = (time_t)(tbh2x->starttime + maxi * (1.0/tbh2x->samprate));

                  gmtime_ew( &theTime, &theTimeStruct );

                
                  sprintf( msg, "SNCL=%s.%s.%s.%s Thresh=%lf Value=%lf Time=%s%c",
                        tbh2x->sta, tbh2x->net, tbh2x->chan, tbh2x->loc,
                        threshDef[sncl_idx].threshold, max*convFactor, asctime( &theTimeStruct ), 0 );
                  reclogo.type = ThreshAlertType;  /* note the new message type */
		  reclogo.mod    = MyModId;	 /* should be my module id */
		  
		          if ( curTime <= NoMessagesBefore ) {
		            logit("t", "ThreshAlert Message not sent (within WaitBeforeMessages ): %s\n", msg);
		          }
		          /* Make sure we haven't sent a message for this SNCL too recently */
                  else if ( curTime >= lastThresh + SecsBetweenThreshMsgs ) {

                      if ( tport_putmsg( &OutRegion, &reclogo, (long)strlen(msg), msg ) != PUT_OK ) {
                              logit("et", "%s:  Error writing threshold message to ring.\n",
                                              Argv0 );
                      }
                    
                        threshDef[sncl_idx].lastSent = curTime;
                        
                      if (DebugLevel > 0) {
                        logit("t", "ThreshAlert Message: %s\n", msg);
                      }
                  } else {
                        logit("t", "ThreshAlert Message not sent (within WaitBeforeNextThresh ): %s\n", msg);
                  }
                  if ( tvNumTrig > 0 && tvHoldVotingUntil <= theTime)
                  {
                    Amin =Amax[0];
					Amin_coinci = 9999999999999.;
					
                    fprintf(stderr,"\n###################################################################################################################\n ");
                    for ( myidx=0; myidx<nThreshDefs; myidx++ )
                    {
                        fprintf(stderr,"station     SNCL=%s.%s.%s.%s Thresh=%lf Value=%f Time=%s%c",
                               threshDef[myidx].sta, threshDef[myidx].net,threshDef[myidx].chan,threshDef[myidx].loc,
                               threshDef[myidx].threshold, Amax[myidx], asctime( &theTimeStruct ), 0 );
                        if(Amax[myidx]>=threshDef[myidx].threshold){
                            triggervalue +=1;
                            triggerarray[myidx] = 1;
                        }
                        else
                            triggerarray[myidx] = 0;

                    }
                    fprintf(stderr,"number of current coincident triggers = %i        number of needed triggers = %i\n",triggervalue,tvNumTrig);
                    fprintf(stderr,"###################################################################################################################\n\n");
                    if(triggervalue >= tvNumTrig)
                    {
                        fprintf(stderr,"\nTRIGGER observed!\n");
                        fprintf(stderr,"Trigger at station(s) ");
                            for ( myidx=0; myidx<nThreshDefs; myidx++ )
                            {
                                if(Amax[myidx]>=threshDef[myidx].threshold)
								{
                                    fprintf(stderr," %s",threshDef[myidx].sta);
									if(Amax[myidx]<Amin_coinci)
										Amin_coinci = Amax[myidx];
								}	
                                
                            }
                            fprintf(stderr,"\n \n");
                            
                        
                        fprintf(stderr,"Minimum coincident trigger amplitude: %f        trigger thresholds (y/r):   %f    %f   %s\n\n",Amin_coinci,Threshold_yellow,Threshold_red,alarmsound_red);
                        if ( Amin_coinci >= Threshold_red )
                        {
#ifdef _WINNT 
				play(alarmsound_red);
#endif
                                tvAddAlert( theTime );
                        }
                        else
                        {
                            if ( Amin_coinci >= Threshold_yellow )
                            {
#ifdef _WINNT 
				play(alarmsound_yellow);
#endif
                                tvAddAlert( theTime );
                            }
                        }

                    }
                    
                  }
          }
          allthreshidx = 0;
           triggervalue =0;
   } /* end of while */

   /* we're quitting
   *****************/
   MessageStackerStatus = MSGSTK_ERR; /* file a complaint to the main thread */
   KillSelfThread(); /* main thread will restart us */
   return THR_NULL_RET;
}

/*****************************************************************************
 *  ewthresh_config() processes command file(s) using kom.c functions;         *
 *                    exits if any errors are encountered.               *
 *****************************************************************************/
void ewthresh_config( char *configfile )
{
   int      ncommand;     /* # of required commands you expect to process   */
   char     init[10];     /* init flags, one byte for each required command */
   int      nmiss;        /* number of required commands that were missed   */
   char    *com;
   char     processor[20];
   int      nfiles;
   int      success;
   int      i;
   char*    str;
   long     NMB_delay = 0;

/* Set to zero one init flag for each required command
 *****************************************************/
   ncommand = 7;
   for( i=0; i<ncommand; i++ )  init[i] = 0;
   nLogo = 0;

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
            strcpy( processor, "ewthresh_config" );

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
  /* optional */     else if( k_its("DebugLevel") ) {
                DebugLevel = k_int();
            }


         /* Maximum size (bytes) for incoming/outgoing messages
          *****************************************************/
  /*5*/     else if( k_its("MaxMsgSize") ) {
                MaxMsgSize = k_long();
                init[5] = 1;
            }

         /* Add a SCNL to monitor & its threshold
          ********************************************************/
  /*6*/     else if( k_its("Threshold") ) {
                if ( nThreshDefs >= MAX_THRESH_DEFS-1 ) {
                        logit( "e", "%s: Too many Threshold commands; exiting!\n",
                        Argv0 );
                        exit(-1);
                }
                strcpy( threshDef[nThreshDefs].sta, k_str() );
                strcpy( threshDef[nThreshDefs].chan, k_str() );
                strcpy( threshDef[nThreshDefs].net, k_str() );
                strcpy( threshDef[nThreshDefs].loc, k_str() );
                threshDef[nThreshDefs].threshold = k_val();
                threshDef[nThreshDefs].conversion = 1.0;
                threshDef[nThreshDefs].lastSent = 0;
                nThreshDefs++;
                init[6] = 1;

            }
/*7*/     else if( k_its("Alarmsound_yellow") ) {
                        str=k_str();
            		strcpy(alarmsound_yellow,str);    /* myalarmsound = str; */
                        fprintf(stderr,"My 1st alarmsound is    %s\n",alarmsound_yellow);
            }
/*8*/     else if( k_its("Alarmsound_red") ) {
                        str=k_str();
            		strcpy(alarmsound_red,str);    /* myalarmsound = str; */
                        fprintf(stderr,"My 2nd alarmsound is    %s\n",alarmsound_red);
            }
/*9*/     else if( k_its("Threshold_yellow") ) {
                        Threshold_yellow = k_val();
                        fprintf(stderr,"My Yellow (1st) threshold is    %f\n",Threshold_yellow);
            }
/*10*/     else if( k_its("Threshold_red") ) {
                        Threshold_red = k_val();
                        fprintf(stderr,"My Red (2nd) threshold is    %f\n",Threshold_red);
            }
  /*alternate 6*/
              else if( k_its("ConvertedThreshold") ) {
                if ( nThreshDefs >= MAX_THRESH_DEFS-1 ) {
                        logit( "e", "%s: Too many ConvertedThreshold commands; exiting!\n",
                        Argv0 );
                        exit(-1);
                }
                strcpy( threshDef[nThreshDefs].sta, k_str() );
                strcpy( threshDef[nThreshDefs].chan, k_str() );
                strcpy( threshDef[nThreshDefs].net, k_str() );
                strcpy( threshDef[nThreshDefs].loc, k_str() );
                threshDef[nThreshDefs].threshold = k_val();
                threshDef[nThreshDefs].conversion = k_val();
                nThreshDefs++;
                init[6] = 1;

            }

         /* Conditions to fire off a triglist message
          ********************************************************/
         else if( k_its("ThreshVotes") ) {
                if ( tvNumTrig > 0) {
                        logit( "e", "%s: Multiple ThreshVotes commands; exiting!\n",
                        Argv0 );
                        exit(-1);
                }
                tvNumTrig = k_int();
                if ( tvNumTrig <= 0 ){
                        logit( "e", "%s: Non-positive NumTrig in ThreshVotes; exiting!\n",
                        Argv0 );
                        exit(-1);
                }
                tvTimeSpanSeconds = k_val();
                tvPreEventSeconds = k_val();
                tvNumSCNLs = k_int();
                if ( tvNumSCNLs > 0 )
                        tvSCNLs = (EW_THRESHOLD_DEF*)calloc( 1,  sizeof(EW_THRESHOLD_DEF) * tvNumSCNLs );
                for ( i=0; i<tvNumSCNLs; i++ ) {
                        char *nstr = k_str();
                        if ( nstr != NULL ) {
                                                strcpy( tvSCNLs[i].sta, nstr );
                                                nstr = k_str();
                                        }
                                        if ( nstr != NULL ) {
                                                strcpy( tvSCNLs[i].chan, nstr );
                                                nstr = k_str();
                                        }
                                        if ( nstr != NULL ) {
                                                strcpy( tvSCNLs[i].net, nstr );
                                                nstr = k_str();
                                        }
                                        if ( nstr != NULL )
                                                strcpy( tvSCNLs[i].loc, nstr );
                                        else {
                                                logit( "e", "%s: ThreshVotes is missing arguments (NumSCNLs=%d); exiting!\n", Argv0, tvNumSCNLs );
                                                exit(-1);
                                        }
                }
        }

        else if( k_its("WaitBeforeNextThresh") ) {
            SecsBetweenThreshMsgs = k_long();
            str = k_str();
            if ( str != NULL ) {
                if ( strcmp( str, "SameNetSta" ) ) {
                    logit( "e", "%s: Illegal option for WaitBeforeNextThresh: '%s'; exiting!\n", Argv0, str );
                    exit(-1);
                }
                NetStaWait = 1;
            } else {
                NetStaWait = 0;
                k_err();    // clear error
            }
            fprintf(stderr,"WaitBeforeNextThresh %s is    %ld\n", NetStaWait ? "(Net/Sta)" : "(SNCL)", SecsBetweenThreshMsgs);
        }

        else if( k_its("WaitBeforeMessages") ) {
            if ( NMB_delay > 0 ) {
                logit( "e", "%s: Multiple WaitBeforeMessages; exiting!\n", Argv0 );
                exit(-1);
            }
            NMB_delay = k_long();
            if ( NMB_delay <= 0 ) {
                logit( "e", "%s: Non-positive value for WaitBeforeMessages (%ld); exiting!\n", Argv0, NMB_delay );
                exit(-1);
            }
            NoMessagesBefore += NMB_delay;
            fprintf(stderr,"WaitBeforeMessages is    %ld\n", NMB_delay);
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
   for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
   if ( nmiss ) {
       logit( "e", "%s: ERROR, no ", Argv0 );
       if ( !init[0] )  logit( "e", "<LogFile> "      );
       if ( !init[1] )  logit( "e", "<MyModuleId> "   );
       if ( !init[2] )  logit( "e", "<InRing> "     );
       if ( !init[3] )  logit( "e", "<OutRing> "     );
       if ( !init[4] )  logit( "e", "<HeartBeatInt> " );
       if ( !init[5] )  logit( "e", "<MaxMsgSize> "  );
       if ( !init[6] )  logit( "e", "<Threshold> "   );
       logit( "e" , "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }
   if (DebugLevel > 1) {
        logit("t", "Config file %s parsed, %d threshold checks will be performed\n", configfile, nThreshDefs);
   }
   return;
}

/****************************************************************************
 *  ewthresh_lookup( )   Look up important info from earthworm.h tables       *
 ****************************************************************************/
void ewthresh_lookup( void )
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

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModName, &MyModId ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid module name <%s>; exiting!\n",
               Argv0, MyModName );
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
   return;
}

/***************************************************************************
 * ewthresh_status() builds a heartbeat or error message & puts it into      *
 *                 shared memory.  Writes errors to log file & screen.     *
 ***************************************************************************/
void ewthresh_status( unsigned char type, short ierr, char *note )
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
 * ewthresh_free()  free all previously allocated memory                     *
 ***************************************************************************/
void ewthresh_free( void )
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
int MakeLocal( TRACE2X_HEADER *tbh2x ) {
        int size = 1;
#if defined (_SPARC)
        if ( tbh2x->datatype[0] == 's' )
                size = -1;
        else if ( tbh2x->datatype[0] != 'i' ) {
                logit( "e", "ewthresh: unknown datatype: '%s'", tbh2x->datatype );
                return 0;
        }
#elif defined (_INTEL)
        if ( tbh2x->datatype[0] == 'i' )
                size = -1;
        else if ( tbh2x->datatype[0] != 's' ) {
                logit( "e", "ewthresh: unknown datatype: '%s'", tbh2x->datatype );
                return 0;
        }
#else
#error "_INTEL or _SPARC must be set before compiling"
#endif
        else {
                Swap( &tbh2x->starttime, sizeof(double) );
                Swap( &tbh2x->nsamp, sizeof(int) );
                Swap( &tbh2x->x.v21.conversion_factor, sizeof(float) );
        }
        if ( tbh2x->datatype[1] == '2' )
                return size * 2;
        else if ( tbh2x->datatype[1] == '4' )
                return size * 4;
        else {
                logit( "e", "ewthresh: unknown datatype size: '%s'", tbh2x->datatype );
                return 0;
        }
}


