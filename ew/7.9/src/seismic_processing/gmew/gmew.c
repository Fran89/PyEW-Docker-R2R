/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: gmew.c 6487 2016-04-18 18:51:32Z scott $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.13  2009/08/26 13:28:54  paulf
 *     fixed version output and gmew.d
 *
 *     Revision 1.12  2009/08/26 00:38:17  paulf
 *     forgot to up the version number...duh
 *
 *     Revision 1.11  2009/08/25 19:55:12  paulf
 *     modified logit for extraDelay to only report to log
 *
 *     Revision 1.10  2009/08/25 00:10:40  paulf
 *     added extraDelay parameter go gmew
 *
 *     Revision 1.9  2007/02/26 13:44:40  paulf
 *     fixed heartbeat sprintf() to cast time_t as long
 *
 *     Revision 1.8  2007/02/23 17:00:42  paulf
 *     fixed long warning for time_t
 *
 *     Revision 1.7  2006/03/15 14:21:54  paulf
 *     SCNL version of gmew v0.2.0
 *
 *     Revision 1.6  2001/07/18 20:10:35  lombard
 *     removed more debug lines.
 *
 *     Revision 1.5  2001/07/18 20:00:06  lombard
 *     Removed some debug lines that were causing problems
 *
 *     Revision 1.4  2001/07/18 19:18:25  lucky
 *     *** empty log message ***
 *
 *     Revision 1.3  2001/06/10 21:27:36  lombard
 *     Changed single transport ring to input and output rings.
 *     Added ability to handle multiple getEventsFrom commands.
 *     Fixed handling of waveservers in config file.
 *
 *     Revision 1.2  2001/05/09 22:33:50  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or pEW->myPid.
 *
 *     Revision 1.1  2001/03/30 19:14:25  lombard
 *     Initial revision
 *
 *
 *
 */
/*
 * gmew.c: gmew, the earthworm ground-motion module.
 *
 * Pete Lombard; Earthworm Engineering; February 2001
 */


#define VERSION "v0.3.1 2016-04-18"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <earthworm.h>
#include <mem_circ_queue.h>
#include <rw_strongmotionII.h>
#include <transport.h>
#include <time_ew.h>
#include <read_arc.h>
#include "gm.h"
#include "gm_util.h"
#include "gm_sac.h"
#include "gm_ws.h"
#include "../localmag/lm_misc.h"
#include "ew_spectra_io.h"

/* Error messages used by gmew */
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_ARCFILE       3   /* error creating/writing arc file        */
#define  ERR_QUEUE         4   

#define IN_QUEUE_SIZE   100     /* Number of arc summary lines to hold  */
#define THREAD_STACK    8192    /* How big is our thread stack          */

/* Internal function prototypes */
static void setUpThread( GMPARAMS * );
static thr_ret StackerThread( void * );
static void gm_status( MSG_LOGO *, short, char *, GMEW *);

/* Used to parse textual date */
static char* monthNames[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"}; 

int main(int argc, char **argv)
{
  static EVENT event;
  static GMPARAMS gmParams;
  static GMEW gmEW;
  int result;
  long msgLen;
  MSG_LOGO recLogo;
  char msg[MAX_BYTES_PER_EQ+1];
  
  gmParams.pEW = &gmEW;

  if (Configure(&gmParams, argc, argv, &event, VERSION) < 0)
  {
    logit("et", "gmew: Version %s\n", VERSION);
    logit("et", "gmew: configuration failed; exitting\n");
    return 1;
  }
  
  logit("t", "gmew: Version %s\n", VERSION);

  /* Start up our message stacker thread */
  setUpThread( &gmParams );
 
  /* Main loop; do this until we die */
  while ( !gmEW.terminate )
  {
    /* Grap the next arc summary line from the input queue */
    RequestSpecificMutex(&gmEW.Qmutex);
    result = dequeue (&gmEW.msgQ, msg, &msgLen, &recLogo);
    ReleaseSpecificMutex(&gmEW.Qmutex);
    
    if (result < 0) /* Nothing in queue; take a break */
      goto Next;
    
    /* Parse event data from the new message */
    if ( gmEW.ha2kLogo.type != 0 && recLogo.type == gmEW.ha2kLogo.type ) {
    	/* Process the hypoarc message */
    	
		if ( (result = procArc( msg, &event, recLogo, gmEW.gmLogo )) < 0)
		{
		  logit("et", "gmew: error processing ARC message; exiting\n");
		  break;
		}
	
		if (gmParams.waitTime != 0) {
		  logit("t", "gmew: sleeping for %d seconds before processing event\n", gmParams.waitTime);
		  sleep_ew(1000*gmParams.waitTime);
		}
		  
		/* Process the event */
		if ( (result = getGMFromTrace(&event, &gmParams)) < 0)
		{
		  logit("et", "gmew: error analyzing ground motion; exiting\n");
		  break;
		}
	
		endEvent( &event, &gmParams);
	} else if ( gmEW.amLogo.type != 0 && recLogo.type == gmEW.amLogo.type ) {
		/* Process the activate_module message */
		char date[100];
		int modID, args;
		time_t now;
		
		event.numSta = 0;
		args = sscanf( msg, "%d %s", &modID, date );
		if ( args == 1 || args == 2 ) {
			if ( modID != gmEW.hrtLogo.mod )
				continue;
			event.eventId[0] = 0;  /* Indicate this isn't a real event */
			if ( args == 2 ) {
				if (atoi(date) > 0) {
					EWSConvertTime (date, &event.origin_time);
                                } else {
 					/* handle negative times as time BEFORE NOW */
					time(&now);
					event.origin_time = now + atoi(date);
 				}
			} else {
				/* handle no second argument as NOW */
				time(&now);
				event.origin_time = now;
			}
			
		} else {
			logit( "w", "gmew: Improperly-formatted ACTIVATE_MODULE message read: '%s'\n",
				msg );
		}
		
		if (gmParams.waitTime != 0) {
		  logit("t", "gmew: sleeping for %d seconds before processing request\n", gmParams.waitTime);
		  sleep_ew(1000*gmParams.waitTime);
		}
		  
		/* Process the event */
		if ( (result = getGMFromTrace(&event, &gmParams)) < 0)
		{
		  logit("et", "gmew: error processing request; exiting\n");
		  break;
		}
		
	} else if ( gmEW.threshLogo.type != 0 && recLogo.type == gmEW.threshLogo.type ) {
	    //logit("t", "gmew: saw the THRESH message: %s\n", msg );
		/* Process the ew threshold message */
		int args, year;
		struct tm when_tm;
		char month_name[20];
		
		event.numSta = 0;
		args = sscanf( msg, "%*s Thresh=%*f Value=%*f Time=%*s %s %d %d:%d:%d %d", month_name, &when_tm.tm_mday, &when_tm.tm_hour, &when_tm.tm_min, &when_tm.tm_sec, &year );
		if ( args == 6 ) {
		    int i = 1;
		    for ( i=0; i<12; i++ )
		        if ( strcmp( monthNames[i], month_name ) == 0 ) {
		            when_tm.tm_mon = i;
		            break;
		        }
			when_tm.tm_year = year - 1900;
			event.origin_time = timegm_ew( &when_tm );
			sprintf( event.eventId, "%1.0f", -event.origin_time );  /* Indicate this isn't a real event */
		} else {
			logit( "w", "gmew: Improperly-formatted EWTHRESH message read: '%s'\n",
				msg );
		}
		
		if (gmParams.waitTime != 0) {
		  logit("t", "gmew: sleeping for %d seconds before processing request\n", gmParams.waitTime);
		  sleep_ew(1000*gmParams.waitTime);
		}
		  
		/* Process the event */
		if ( (result = getGMFromTrace(&event, &gmParams)) < 0)
		{
		  logit("et", "gmew: error processing request; exiting\n");
		  break;
		}
		
	}
    /* If all went well, don't pause; just get another message */
    continue;
  
  Next:
    sleep_ew (1000);
    continue;
  }
  /* We've terminated for some reason; make sure our StackerThread knows */
  gmEW.terminate = 1;
  logit("et", "gmew: termination requested\n");

  sleep_ew(500); /* Give death a chance...*/
  tport_detach( &gmEW.InRegion ); 
  if (gmEW.RingInKey != gmEW.RingOutKey)
    tport_detach( &gmEW.OutRegion ); 

  exit(0);
  return 0;  /* Not really, but it keeps the compiler happy */
}

  
/*
 * setUpThread: Set up the internal queue between the StackerThread and
 *              the main thread; start up transport, and then start
 *              the StackerThread.
 */
static void setUpThread( GMPARAMS *pgmParams )
{
  GMEW *pEW = pgmParams->pEW;
  unsigned int tidStacker;

  /* Set up a queue; start the message-stacking thread to feed it */
  initqueue(&pEW->msgQ, IN_QUEUE_SIZE, MAX_BYTES_PER_EQ+1);
  CreateSpecificMutex(&pEW->Qmutex);
    
  /* look up my PID for the heartbeat message */
  pEW->myPid = getpid();
  if ( pEW->myPid == -1 )
  {
    logit( "e", "gmew: Cannot get pid. Exitting.\n");
    exit( -1 );
  }

  /* Attach to Input/Output shared memory ring */
  if (pEW->RingInKey != pEW->RingOutKey)
  {
    tport_attach( &pEW->InRegion, pEW->RingInKey );
    tport_attach( &pEW->OutRegion, pEW->RingOutKey );
  }
  else
  {
    tport_attach( &pEW->InRegion, pEW->RingInKey );
    pEW->OutRegion = pEW->InRegion;
  }

  logit( "", "gmew: Attached to public memory regions %d and %d\n", 
         pEW->RingInKey, pEW->RingOutKey );

  pEW->terminate = 0;
  if (StartThreadWithArg (StackerThread, (void *) pgmParams, 
                          (unsigned) THREAD_STACK, &tidStacker) == -1)
  {
    logit( "e", 
           "gmew: Error starting Stacker thread.  Exitting.\n");
    pEW->terminate = 1;
    KillSelfThread();
  }
  return;
}

/*
 * StackerThread: This is the thread that is the entrance to gmew.
 *                It reads TYPE_HYP2KARC messages from transport,
 *                places the first (summary) line from the message into
 *                an internal queue, and it manages heartbeats for gmew.
 */
static thr_ret StackerThread( void *pgm )
{
  GMPARAMS *pgmParams;
  GMEW *pEW;
  MSG_LOGO  recLogo;          /* logo of retrieved message       */
  static char  eqmsg[MAX_BYTES_PER_EQ];  /* array to hold event message     */
  static char line[MAX_BYTES_PER_EQ+1], *cr;
  static char  Text[GM_MAXTXT];    /* string for log/error messages         */
  long recsize = 0;      /* size of retrieved message       */
  time_t timeNow;          /* current time                    */       
  time_t timeLastBeat;     /* time last heartbeat was sent    */
  int result;
  struct Hsum arcSum;
  int flagAddToQueue = 0;

  pgmParams = (GMPARAMS *)pgm;  
  pEW = pgmParams->pEW;
  
  /* Flush the input buffer on startup */
  while ( tport_getmsg( &pEW->InRegion, pEW->GetLogo, pEW->nGetLogo, 
                        &recLogo, &recsize, eqmsg, MAX_BYTES_PER_EQ-1 ) != 
          GET_NONE );
  
  /* Force a heartbeat to be issued in first pass thru main loop */
  timeLastBeat = time(&timeNow) - pgmParams->HeartBeatInterval - 1;
  
  /* setup done; start main loop */
  while( tport_getflag(&pEW->InRegion) != TERMINATE  &&
         tport_getflag(&pEW->InRegion) != pEW->myPid  )
  {
    /* send gmew's heartbeat */
    if  ( time(&timeNow) - timeLastBeat  >=  pgmParams->HeartBeatInterval ) 
    {
      timeLastBeat = timeNow;
      gm_status( &pEW->hrtLogo, 0, "", pEW ); 
    }
    
    /* Process all new hypoinverse archive msgs */
    do  /* Keep doing this as long as there are msgs in the transport ring */
    {
      /* Get the next message from shared memory */
      result = tport_getmsg( &pEW->InRegion, pEW->GetLogo, pEW->nGetLogo, 
                             &recLogo, &recsize, eqmsg, MAX_BYTES_PER_EQ );
      /* Check return code; report errors if necessary */
      if( result != GET_OK )
      {
        if( result == GET_NONE ) 
          break;
        else if( result == GET_TOOBIG ) 
        {
          sprintf( Text, 
                   "Retrieved msg[%ld] (i%u m%u t%u) too big for eqmsg[%d]",
                   recsize, recLogo.instid, recLogo.mod, recLogo.type, 
                   MAX_BYTES_PER_EQ-1 );
          gm_status( &pEW->errLogo, ERR_TOOBIG, Text, pEW );
          continue;
        }
        else if( result == GET_MISS ) 
        {
          sprintf( Text,
                   "Missed msg(s)  i%u m%u t%u",
                   recLogo.instid, recLogo.mod, recLogo.type);
          gm_status( &pEW->errLogo, ERR_MISSMSG, Text, pEW );
        }
        else if( result == GET_NOTRACK ) 
        {
          sprintf( Text,
                   "Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
                   recLogo.instid, recLogo.mod, recLogo.type );
          gm_status( &pEW->errLogo, ERR_NOTRACK, Text, pEW );
        }
      }

      /* Copy the first (summary) line into a buffer for the queue */
      memset(line, 0, MAX_BYTES_PER_EQ+1);
      if ( (cr = (char *)memchr(eqmsg, '\n', MAX_BYTES_PER_EQ)) == NULL)
        strncpy(line, eqmsg, MAX_BYTES_PER_EQ);
      else
        strncpy(line, eqmsg, (size_t)(cr - eqmsg));
      
      /* Initialize flag for adding arc message to the queue */
      flagAddToQueue = 0;

      /* Check for LookAtVersion */
      if( pgmParams->LookAtVersion == vAll) {
	flagAddToQueue = 1;
      } else {
	if (read_hyp (eqmsg, NULL, &arcSum) < 0) {
	  snprintf( Text, GM_MAXTXT - 1, "Error reading arc summary: %s\n", eqmsg );
	  logit("e", "%s", Text);
	} else {
	  if (arcSum.version ==  pgmParams->LookAtVersion ) {
	    flagAddToQueue = 1;
	    snprintf( Text, GM_MAXTXT - 1, "Enqueued arc message %ld.%ld\n", arcSum.qid, arcSum.version );
	    logit("t", "%s", Text);

	  } else {
	    snprintf( Text, GM_MAXTXT - 1, "Not enqueued arc message %ld.%ld\n", arcSum.qid, arcSum.version );
	    logit("t", "%s", Text);
	  }
	}
      }

      /* Check for adding (or not) the arc message to the queue */
      if(flagAddToQueue) {
	/* Queue retrieved msg */
	RequestSpecificMutex(&pEW->Qmutex);
	result = enqueue (&pEW->msgQ, line, MAX_BYTES_PER_EQ, recLogo); 
	ReleaseSpecificMutex(&pEW->Qmutex);
      } else {
	result = GET_NONE;
      }
      
      if (result != 0)
      {
        if (result == -1)
        {
          sprintf (Text, "Message too large for queue; Lost message.");
          gm_status (&pEW->errLogo, ERR_QUEUE, Text, pEW );
          continue;
        }
        if (result == -3) 
        {
          sprintf (Text, "Queue full. Old messages lost.");
          gm_status (&pEW->errLogo, ERR_QUEUE, Text, pEW);
          continue;
        }
      } /* problem from enqueue */
      
    } while( result != GET_NONE );  /*end of message-processing-loop */

    /* No more msgs in transport ring; take a break */
    sleep_ew( 1000 );  /* no more messages; wait for new ones to arrive */
  }
  /* end of main loop */

  /* Termination has been requested; make sure our main thread knows */
  pEW->terminate = 1;
  KillSelfThread();
  exit(0);  /* not really */
}



/*
 * gm_status() builds a heartbeat or error message & puts it into
 *             shared memory.  Writes errors to log file & screen.
 */
static void gm_status( MSG_LOGO *pLogo, short ierr, char *note, GMEW *pEW )
{
  char         msg[256];
  long         size;
  time_t        t;
 
  /* Build the message */ 
  time( &t );

  if( pLogo->type == pEW->hrtLogo.type )
    sprintf( msg, "%ld %d\n", (long) t, (int) pEW->myPid );
  else if( pLogo->type == pEW->errLogo.type )
  {
    sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note);
    logit( "et", "gmew: %s\n", msg );
  }


  /* Write the message to shared memory */
  if (pLogo->type == pEW->gmLogo.type)
  {
    size = strlen(note);
    if( tport_putmsg( &pEW->OutRegion, pLogo, size, note ) != PUT_OK )
      logit("et", "gmew: Error sending LOCALMAG message\n");
  }
  else
  {
    size = strlen( msg );   /* don't include the null byte in the message */
    if( tport_putmsg( &pEW->OutRegion, pLogo, size, msg ) != PUT_OK )
    {
      if( pLogo->type == pEW->hrtLogo.type ) 
        logit("et","gmew:  Error sending heartbeat.\n" );
      else if( pLogo->type == pEW->errLogo.type ) 
        logit("et","gmew:  Error sending error:%d.\n", ierr );
    }
  }
  return;
}

/*
 * send_gm: send the ground-motion observations (as a StrongMotion message)
 *          to transport.
 *     returns: 0 on success
 *             -1 on failure (out of space in message or transport error)
 */
int send_gm( SM_INFO *pSM, GMEW *pEW )
{
  char message[GM_SM_LEN];
  int rc, len;
  
  if ( (rc = wr_strongmotionII( pSM, message, GM_SM_LEN)) < 0)
    return rc;
  
  len = strlen(message);

  logit("t", "Strong motion message with length=%d is being issued:\n", len);
  logit("", "%s\n", message);
  
  
  if( tport_putmsg( &pEW->OutRegion, &pEW->gmLogo, len, message ) != PUT_OK )
  {
    logit("et","gmew:  Error sending strong_motion message.\n" );
    return -1;
  }
  
  return 0;
}
