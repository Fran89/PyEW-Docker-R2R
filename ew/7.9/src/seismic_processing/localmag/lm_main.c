/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: lm_main.c 6366 2015-05-28 03:24:01Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.37  2010/03/15 16:22:50  paulf
 *     merged from Matteos branch for adding version to directory naming
 *
 *     Revision 1.36.2.1  2010/03/15 15:15:55  quintiliani
 *     Changed output file and directory name
 *     Detailed description in ticket 22 from
 *     http://bigboy.isti.com/trac/earthworm/ticket/22
 *
 *     Revision 1.36  2007/03/30 14:14:05  paulf
 *     added saveXMLdir option
 *
 *     Revision 1.35  2007/03/29 20:38:04  paulf
 *     another minor fix, lm_xml_event returns -1 on error, not 0!
 *
 *     Revision 1.34  2007/03/29 20:36:30  paulf
 *     minor fix to option
 *
 *     Revision 1.33  2007/03/29 20:09:50  paulf
 *     added eventXML option from INGV. This option allows writing the Shakemap style event information out as XML in the SAC out dir
 *
 *     Revision 1.32  2007/02/27 14:12:16  paulf
 *     fixed long to time_t conversions
 *
 *     Revision 1.31  2007/02/26 13:44:40  paulf
 *     fixed heartbeat sprintf() to cast time_t as long
 *
 *     Revision 1.30  2006/03/10 13:03:28  paulf
 *     upgraded to SCNL version 2.1.0, tested at Utah
 *
 *     Revision 1.29  2005/08/23 01:56:28  friberg
 *     removed debugging statements and changed z2pThresh pre-event noise calculation
 *
 *     Revision 1.28  2005/08/15 20:58:04  friberg
 *     minor fixes and debugging statements for 2.0.4
 *
 *     Revision 1.27  2005/08/15 15:30:54  friberg
 *     version 2.0.3, added in notUsed flag to PCOMP1 to indicate that the
 *     channels from this component set were not used. This can only
 *     happen currently because of the require2Horizontals configuration
 *     parameter.
 *
 *     Revision 1.26  2005/08/12 17:07:47  friberg
 *     patched minStationsMl check to only use stations with reported magnitudes
 *
 *     Revision 1.25  2005/08/08 18:38:14  friberg
 *
 *     Fixed bug from last version that had station corrections added in twice.
 *     Added in new directive require2Horizontals to require 2 components for a station Ml
 *     	example:   require2Horizontals 1
 *     	Note needs the 1 after the directive to be used
 *     Added in new directive useMedian
 *     	example:   useMedian
 *     	Note for this one, no flag is needed after the directive
 *     Also updated the Doc files.
 *
 *     Revision 1.24  2005/07/27 16:34:51  friberg
 *     Reverted lm_main.c back to version 1.17 before all of the global stuff
 *     was added in. I included a few minor changes from the 1.23 release but
 *     scrapped the majority of them. I also added in a new parameter called
 *     minStationsMl for the minimum number of stations needed for a Ml avg.
 *
 *
 *     Reverted lm_main.c back to version 1.17 before all of the global stuff
 *     was added in. I included a few minor changes from the 1.23 release but
 *     scrapped the majority of them.
 *
 *     Revision 1.17  2002/03/17 18:21:04  lombard
 *     When running as an earthworm module, localmag now waits for the
 *        selected trace interval (trace start to trace end including taper)
 *        to propagate to the maximum station distance before processing
 *        the event.
 *     Cleaned up rules for including channels in the Mag message: if
 *        the p2p amps are present, report them regardless of the value
 *        of slideLength; otherwise report the z2p amps if it is present.
 *
 *     Revision 1.16  2002/01/15 21:23:03  lucky
 *     *** empty log message ***
 *
 *     Revision 1.15  2001/08/30 18:29:40  lucky
 *     Made to work under NT
 *
 *     Revision 1.14  2001/06/21 21:22:22  lucky
 *     Modified the code to support the new amp pick format: there can be one
 *     or two picks, each consisting of time, amplitude, and period. This is
 *     reflected in the new TYPE_MAGNITUDE message, as well as the SAC header
 *     fields that get filled in.
 *     Also, the labels for SAC fields were shortened to comply with the K_LEN
 *     limitation from sachead.h
 *
 *     Revision 1.13  2001/06/10 21:21:46  lombard
 *     Changed single transport ring to two rings, added allowance
 *     for multiple getEventsFrom commands.
 *     These changes necessitated several changes in the way config
 *     and earthworm*.d files were handled.
 *
 *     Revision 1.12  2001/05/31 17:41:13  lucky
 *     Added support for outputFormat = File. This option works only in
 *     standalone mode. It writes TYPE_MAGNITUDE message to a specified file.
 *     We need this for review.
 *
 *     Revision 1.11  2001/05/26 21:02:01  lombard
 *     Changed ML_INFO struct to MAG_CHAN_INFO struct to work
 *     with modified rw_mag.[ch]
 *
 *     Revision 1.10  2001/05/09 22:37:34  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or myPid.
 *
 *     Revision 1.9  2001/05/02 20:59:46  alex
 *     *** empty log message ***
 *
 *     Revision 1.8  2001/04/29 00:08:59  alex
 *     Alex: fix for mag type integer and string in message
 *
 *     Revision 1.7  2001/03/24 05:33:41  lombard
 *     Changed TYPE_LOCALMAG to TYPE_MAGNITUDE
 *
 *     Revision 1.6  2001/03/01 05:25:44  lombard
 *     changed FFT package to fft99; fixed bugs in handling of SCNPars;
 *     changed output to Magnitude message using rw_mag.c
 *
 *     Revision 1.5  2001/01/15 03:55:55  lombard
 *     bug fixes, change of main loop, addition of stacker thread;
 *     moved fft_prep, transfer and sing to libsrc/util.
 *
 *     Revision 1.4  2000/12/31 21:31:05  lombard
 *     Bug fixes in transport loop
 *
 *     Revision 1.3  2000/12/31 17:27:25  lombard
 *     More bug fixes and cleanup.
 *
 *     Revision 1.2  2000/12/25 22:14:39  lombard
 *     bug fixes and development
 *
 *     Revision 1.1  2000/12/19 18:31:25  lombard
 *     Initial revision
 *
 *
 *
 */

/*
 * Localmag, a standalone Earthworm program for estimating local magnitude.
 *
 * Written by: Pete Lombard, Earthworm Engineering  September 2000
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <earthworm.h>
#include <rw_mag.h>
#include <transport.h>
#include <mem_circ_queue.h>
#include <time_ew.h>
#include <read_arc.h>
#include "lm.h"
#include "lm_config.h"
#include "lm_misc.h"
#include "lm_sac.h"
#include "lm_util.h"
#include "lm_ws.h"
#include "lm_xml_event.h"
#include "lm_version.h"
#ifdef UW
#include "lm_uw.h"
#endif

/* Error messages used by localmag */
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_ARCFILE       3   /* error creating/writing arc file        */
#define  ERR_QUEUE         4   /* Internal queue error                   */
#define  ERR_TRUNC         5   /* Magnitude message is truncated         */
#define  ERR_NOMAG         6   /* No local magnitude available           */

/* Number of arc summary lines to hold  */
#define IN_QUEUE_SIZE   100
/* How big is our thread stack          */
#define THREAD_STACK    8192
static QUEUE msgQ;
static mutex_t Qmutex;
static pid_t myPid;
static int gTerminate = 0;

/* Internal function prototypes */
static void setUpThread( LMPARAMS * );
static thr_ret StackerThread( void * );
static void lm_loop( EVENT *, LMPARAMS *);
static void lm_status( MSG_LOGO *, short, char *, LMEW * );
static int doOneEvent( LMPARAMS *, EVENT *, char * );
static void wrLM( EVENT *, LMPARAMS *);

static MAG_INFO MI;
static int nML = 0;

/* How many (on average) channels per station we expect */
#define MAX_CHAN_PER_STA 6

int main(int argc, char **argv)
{
  static EVENT event;
  static LMPARAMS lmParams;
  static LMEW lmEW;

  lmParams.pEW = &lmEW;  
  
  if (Configure(&lmParams, argc, argv, &event) < 0)
  {
    logit("e", "localmag: configuration failed; exiting\n");
    return 1;
  }
  logit("e", "localmag: version %s\n", LOCALMAG_VERSION);
  if ( (MI.pMagAux = (char *)malloc(lmParams.maxSta * MAX_CHAN_PER_STA *
                                    sizeof(MAG_CHAN_INFO))) == NULL)
  {
    logit("et", "localmag: out of memory for MAG_CHAN_INFO; exiting\n");
    return( 1 );
  }
  MI.size_aux = lmParams.maxSta * MAX_CHAN_PER_STA * sizeof(MAG_CHAN_INFO);
  nML = lmParams.maxSta * MAX_CHAN_PER_STA;

  if (lmParams.fEWTP == 1)
  {
    setUpThread( &lmParams );
    lm_loop( &event, &lmParams);
  }
  else
  {
    event.author[0] = '\0';
    if (doOneEvent(&lmParams, &event, NULL) < 0)
      goto Abort;
  }
  return 0;
  
  Abort:
  logit("et", "localmag: terminated on fatal error\n");
  return 1;
}
  

static int doOneEvent(LMPARAMS *plmParams, EVENT *pEvt, char *eqmsg)
{
  int rc;
  
  /* Get initial event info from those sources that can provide it */
  if (plmParams->fEWTP == TRUE)
  {
    if ( (rc = procArc( eqmsg, pEvt )) < 0)
      return rc;   /* parse failed; try the next one */
  }
  else
  {
    if ( (rc = getEvent(pEvt, plmParams)) < 0)
      return rc;
  }
  
  if (plmParams->fGetAmpFromSource == TRUE)
    rc = getAmpFromSource(pEvt, plmParams);
  else
    rc = getAmpFromTrace(pEvt, plmParams);
  if (rc < 0)
    return rc;
  
  getMagFromAmp(pEvt, plmParams);
  
  /* Sort the STA array by order of distance; nearest first */
  qsort(pEvt->Sta, pEvt->numSta, sizeof(STA), CompareStaDist);
  
  /* Preliminary output routine; need support of EWDB and UW */
  wrLM(pEvt, plmParams);

   /* Write xml event file */
   if(plmParams->eventXML==1) {
     if (xml_event_write(pEvt, plmParams) == -1) {
           logit( "t", "xml_event_write: error writing xml event file\n" );
     }
   }
  
  endEvent(pEvt, plmParams);
  
  return 0;
}

/*
 * setUpThread:
 */
static void setUpThread( LMPARAMS *plmParams )
{
  unsigned int tidStacker;
  LMEW *pEW = plmParams->pEW;
  
  /* Set up a queue; start the message-stacking thread to feed it */
  /* maximum size (bytes) for input msgs MAX_BYTES_PER_EQ from earthworm_defs.h */
  initqueue(&msgQ, IN_QUEUE_SIZE, MAX_BYTES_PER_EQ+1);
  CreateSpecificMutex(&Qmutex);
    
  /* look up my PID for the heartbeat message */
  myPid = getpid();
  if ( myPid == -1 )
  {
    logit( "e", "localmag: Cannot get pid. Exiting.\n");
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

  logit( "", "localmag: Attached to public memory regions %d and %d\n", 
         pEW->RingInKey, pEW->RingOutKey );

  gTerminate = 0;
  if (StartThreadWithArg (StackerThread, (void *) plmParams, 
                          (unsigned) THREAD_STACK, &tidStacker) == -1)
  {
    logit( "e", "localmag: Error starting Stacker thread.  Exiting.\n");
    gTerminate = 1;
    KillSelfThread();
  }
  return;
}


static thr_ret StackerThread( void *plm )
{
  LMPARAMS *plmParams;
  LMEW *pEW;
  MSG_LOGO  recLogo;          /* logo of retrieved message       */
  static char  eqmsg[MAX_BYTES_PER_EQ+1];  /* array to hold event message     */
  static char  Text[LM_MAXTXT];    /* string for log/error messages         */
  long recsize = 0;      /* size of retrieved message       */
  time_t timeNow;          /* current time                    */       
  time_t timeLastBeat;     /* time last heartbeat was sent    */
  int result;
  struct Hsum arcSum;
  int flagAddToQueue = 0;

  plmParams = (LMPARAMS *)plm;  
  pEW = plmParams->pEW;
  
  /* Flush the input buffer on startup */
  while ( tport_getmsg( &pEW->InRegion, pEW->GetLogo, (short) pEW->nGetLogo, &recLogo, 
                        &recsize, eqmsg, MAX_BYTES_PER_EQ ) != GET_NONE );
  
  /* Force a heartbeat to be issued in first pass thru main loop */
  timeLastBeat = time(&timeNow) - plmParams->HeartBeatInterval - 1;
  
  /* setup done; start main loop */
  while( tport_getflag(&pEW->InRegion) != TERMINATE  &&
         tport_getflag(&pEW->InRegion) != myPid )
  {
    /* send localmag's heartbeat */
    if  ( time(&timeNow) - timeLastBeat  >=  plmParams->HeartBeatInterval ) 
    {
      timeLastBeat = timeNow;
      lm_status( &pEW->hrtLogo, 0, "", pEW ); 
    }
    
    /* Process all new hypoinverse archive msgs */
    do  /* Keep doing this as long as there are msgs in the transport ring */
    {
      /* Get the next message from shared memory */
      result = tport_getmsg( &pEW->InRegion, pEW->GetLogo, (short) pEW->nGetLogo,
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
          lm_status( &pEW->errLogo, ERR_TOOBIG, Text, pEW );
          continue;
        }
        else if( result == GET_MISS ) 
        {
          sprintf( Text,
                   "Missed msg(s)  i%u m%u t%u.",
                   recLogo.instid, recLogo.mod, recLogo.type );
          lm_status( &pEW->errLogo, ERR_MISSMSG, Text, pEW );
        }
        else if( result == GET_NOTRACK ) 
        {
          sprintf( Text,
                   "Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
                   recLogo.instid, recLogo.mod, recLogo.type );
          lm_status( &pEW->errLogo, ERR_NOTRACK, Text, pEW );
        }
      }

      /* Copy the first (summary) line into a buffer for the queue */
      /* memset(line, 0, MAX_BYTES_PER_EQ+1);
      if ( (cr = (char *)memchr(eqmsg, '\n', MAX_BYTES_PER_EQ)) == NULL) 
        strncpy(line, eqmsg, MAX_BYTES_PER_EQ);
      else
        strncpy(line, eqmsg, (size_t)(cr - eqmsg));
	*/
      
      /* Initialize flag for adding arc message to the queue */
      flagAddToQueue = 0;

      /* Check for LookAtVersion */
      if( plmParams->LookAtVersion == vAll) {
	  flagAddToQueue = 1;
      } else {
	  if (read_hyp (eqmsg, NULL, &arcSum) < 0) {
	      snprintf( Text, LM_MAXTXT - 1, "Error reading arc summary: %s\n", eqmsg );
	      logit("e", "%s", Text);
	  } else {
	      if (arcSum.version ==  plmParams->LookAtVersion ) {
		  flagAddToQueue = 1;
		  snprintf( Text, LM_MAXTXT - 1, "Enqueued arc message %ld.%ld\n", arcSum.qid, arcSum.version );
		  logit("t", "%s", Text);

	      } else {
		  snprintf( Text, LM_MAXTXT - 1, "Not enqueued arc message %ld.%ld\n", arcSum.qid, arcSum.version );
		  logit("t", "%s", Text);
	      }
	  }
      }

      /* Check for adding (or not) the arc message to the queue */
      if(flagAddToQueue) {
	  /* Queue retrieved msg */
	  RequestSpecificMutex(&Qmutex);
	  result = enqueue (&msgQ, eqmsg, recsize, recLogo); 
	  ReleaseSpecificMutex(&Qmutex);
      } else {
	  result = GET_NONE;
      }
      
      if (result != 0)
      {
        if (result == -1)
        {
          sprintf (Text, "Message too large for queue; Lost message.");
          lm_status (&pEW->errLogo, ERR_QUEUE, Text, pEW);
          continue;
        }
        if (result == -3) 
        {
          sprintf (Text, "Queue full. Old messages lost.");
          lm_status (&pEW->errLogo, ERR_QUEUE, Text, pEW);
          continue;
        }
      } /* problem from enqueue */
      
    } while( result != GET_NONE );  /*end of message-processing-loop */
    /* No more msgs in transport ring; take a break */
    sleep_ew( 1000 );  /* no more messages; wait for new ones to arrive */
  }
  /* end of main loop */

  /* Termination has been requested */
  gTerminate = 1;

  /* write a termination msg to log file */
  logit( "et", "localmag: Termination requested; exiting!\n" );
  KillSelfThread();
  exit(0);  /* not really */
}



/*
 * lm_loop: the dequeue loop for the transport-connected side of localmag.
 *          Once we enter this routine, we don't return until we
 *          get an earthworm terminate message or we hit a fatal error.
 */
static void lm_loop( EVENT *pEvt, LMPARAMS *plmParams)
{
  char line[MAX_BYTES_PER_EQ+1];
  char *lp;
  int result, rc;
  long len;
  MSG_LOGO recLogo;
  time_t now, sleep_time;
  
  while ( !gTerminate )
  {
    RequestSpecificMutex(&Qmutex);
    /* fetch next line and logo from queue */
    rc = dequeue(&msgQ, line, &len, &recLogo);
    ReleaseSpecificMutex(&Qmutex);

    if (rc < 0)
    {                                 /* empty queue */
      sleep_ew (1000);
      continue;
    }
    line[len] = '\0';        /* null terminate data line */
    
    /* is it time to process this event yet? */
    if ( (rc = procArc( line, pEvt )) < 0) {
      /* failed to parse; terminate */
      gTerminate = 1;
      break;
    }
    now = time( NULL );
    if (pEvt->origin_time + plmParams->waitTime > (double) now && plmParams->waitNow == 0) {
      sleep_time = (time_t)(pEvt->origin_time + plmParams->waitTime) - now;
      logit("t", "event %s: waiting %d seconds before processing\n",
            pEvt->eventId, sleep_time);
      sleep_ew( 1000 * (unsigned int) sleep_time);
    } else if (plmParams->waitTime>0.0 && plmParams->waitNow == 1) {
      sleep_ew( (unsigned int) (1000 * plmParams->waitTime) );
    }

    if (gTerminate == 1)
      break;
    
    /* Process new message */
    sprintf(pEvt->author, "%03d%03d%03d:%03d%03d%03d", recLogo.type, 
            recLogo.mod, recLogo.instid, plmParams->pEW->magLogo.type, 
            plmParams->pEW->magLogo.mod, plmParams->pEW->magLogo.instid);
    rc = doOneEvent(plmParams, pEvt, line);

    if (rc < 0)
    {
      gTerminate = 1;
      break;
    }
  }

  /* detach from shared memory */
  tport_detach( &plmParams->pEW->InRegion ); 
  if (plmParams->pEW->RingInKey != plmParams->pEW->RingOutKey)
    tport_detach( &plmParams->pEW->OutRegion ); 
  return;
}



/*
 * lm_status() builds a heartbeat or error message & puts it into
 *             shared memory.  Writes errors to log file & screen.
 */
static void lm_status( MSG_LOGO *pLogo, short ierr, char *note, LMEW *pEW )
{
  char         msg[256];
  long         size;
  time_t        t;
 
  /* Build the message */ 
  time( &t );

  /* Write the message to shared memory */
  if (pLogo->type == pEW->magLogo.type)
  {
    size = strlen(note);
    if( tport_putmsg( &pEW->OutRegion, pLogo, size, note ) != PUT_OK )
      logit("et", "localmag: Error sending MAGNITUDE message\n");
  }
  else
  {
    if( pLogo->type == pEW->hrtLogo.type )
      sprintf( msg, "%ld %d\n", (long)t, (int)myPid );
    else if( pLogo->type == pEW->errLogo.type ||
                                        pLogo->type == pEW->noMagLogo.type )
    {
      sprintf( msg, "%ld %hd %s\n", (long)t, ierr, note);
      logit( "et", "localmag: %s\n", note );
    }
    else
      return;

    size = strlen( msg );   /* don't include the null byte in the message */
    if( tport_putmsg( &pEW->OutRegion, pLogo, size, msg ) != PUT_OK )
    {
      if( pLogo->type == pEW->hrtLogo.type ) 
        logit("et","localmag:  Error sending heartbeat.\n" );
      else if( pLogo->type == pEW->errLogo.type ||
                                        pLogo->type == pEW->noMagLogo.type )
      {
        logit("et","localmag:  Error sending error:%d.\n", ierr );
      }
    }
  }
  return;
}

/*
 * wrLM: log the results of all our efforts;
 *       optionally, send the message to stdout for our manager to 
 *       pick up and deliver as an earthworm message to a transport ring.
 */
static void wrLM( EVENT *pEvt, LMPARAMS *plmParams)
{
  char buf[MAX_BYTES_PER_EQ];
  int i, dir, rc, nchan = 0, nstaUsed=0, last_nchan=0;
  double mindist = -1.0;
  STA *pSta;
  COMP1 *pC1;
  COMP3 *pComp;
  MAG_CHAN_INFO *pMci = (MAG_CHAN_INFO *)MI.pMagAux;

  char  outfile_Mlmsg[500];
  FILE *f_Mlmsg;
 
  if (pEvt->mag == NO_MAG)
  {      /* create "No local magnitude available" message with eventId, */
         /*  magnitude type (ML) and magnitude-type index (1)  */
    wr_nomag_msg(buf, pEvt->eventId, MagNames[1], 1);
    lm_status(&plmParams->pEW->noMagLogo, ERR_NOMAG, buf, plmParams->pEW);
    return;
  }

  memset(buf, 0, MAX_BYTES_PER_EQ);

  /* Copy information into MAG_CHAN_INFO structure */
  strncpy(MI.qid, pEvt->eventId, EVENTID_SIZE);
  strncpy(MI.qauthor, pEvt->author, AUTHOR_FIELD_SIZE);
  MI.error = pEvt->sdev;
  MI.quality = -1.0;
  MI.mindist = -1.0;
  MI.azimuth = -1;
  MI.origin_version = pEvt->origin_version;
  MI.qdds_version = pEvt->qdds_version;
  MI.nstations = pEvt->numSta; /* I believe this is wrong, some components may not even be used */
  strcpy(MI.szmagtype, MagNames[1]); /* alex 4/25/1: from rw_mag.h */
  MI.imagtype = 1; /* alex 4/25/1 */
  if (plmParams->useMedian == 1)
  {
    /* new to this version */
    MI.mag = pEvt->magMed;
    strcpy(MI.algorithm, "MED");
  }
  else
  {
    MI.mag = pEvt->mag;
    strcpy(MI.algorithm, "AVG");
  }

  for (i = 0; i < pEvt->numSta; i++)
  {
    pSta = &pEvt->Sta[i];
    pC1 = pSta->comp;
    while( pC1 != (COMP1 *)NULL)
    {
      /* see if this component group is not used, if not, 
		then don't even report on the comps 
      */
      if (pC1->notUsed == 1 && plmParams->require2Horizontals) 
      {
        logit("t", "Channel group %2s for station %2s.%s not used, require2Horizontals was set\n",
		pC1->n2, pSta->net, pSta->sta);
        pC1 = pC1->next;
        continue;
      }
      else if (pC1->notUsed == 1)
      {
        logit("t", "Channel group %2s for station %2s.%s not used.\n",
		pC1->n2, pSta->net, pSta->sta);
        pC1 = pC1->next;
        continue;
      }
	
      for (dir = 0; dir < 3; dir++)
      {
        pComp = &pC1->c3[dir];
        if (pComp->name[0] == 0)
          continue;  /* Skip empty component slots */
        if (pComp->mag != NO_MAG)
        { 
          if (nchan > nML)
          {
            sprintf(buf, "localmag: ran out of channels in MAG_INFO struct"
                  " TYPE_MAGNITUDE message is truncated\n");
            lm_status (&plmParams->pEW->errLogo, ERR_TRUNC, buf, 
                       plmParams->pEW);
            logit("", buf);
            goto Done;
          }
          /* Don't report clipped or low-snr channels */
          if (pComp->BadBitmap) continue;
          
          strcpy(pMci[nchan].sta, pSta->sta);
          strcpy(pMci[nchan].comp, pComp->name);
          strcpy(pMci[nchan].net, pSta->net);
          strcpy(pMci[nchan].loc, pComp->loc);
          pMci[nchan].mag = pComp->mag;
          pMci[nchan].dist = pSta->dist;
	  if(mindist < 0.0  ||  pMci[nchan].dist < mindist) {
	      mindist = pMci[nchan].dist;
	  }
          pMci[nchan].corr = ( (pComp->pSCNLPar == (SCNLPAR *)NULL) ? 0.0 : 
                              pComp->pSCNLPar->magCorr);

          if (pComp->p2pAmp > 0.0)
          {
            pMci[nchan].Amp1 = (float)pComp->p2pMin;
            pMci[nchan].Time1 = pComp->p2pMinTime;
            pMci[nchan].Period1 = MAG_NULL;   /* Not used */
            
            pMci[nchan].Amp2 = (float)pComp->p2pMax;
            pMci[nchan].Time2 = pComp->p2pMaxTime;
            pMci[nchan].Period2 = MAG_NULL;   /* Not used */
          }
          else
          {
            pMci[nchan].Amp1 = (float)pComp->z2pAmp;
            pMci[nchan].Time1 = pComp->z2pTime;
            pMci[nchan].Period1 = MAG_NULL;   /* Not used */
            
            pMci[nchan].Amp2 = MAG_NULL;
            pMci[nchan].Time2 = MAG_NULL;
            pMci[nchan].Period2 = MAG_NULL;
          }
          
          nchan++;
        }
      }
      pC1 = pC1->next;
    }
    /* now get a real count on number of stations used by counting components */
    if (last_nchan != nchan)  nstaUsed++;
    last_nchan = nchan;
  }
  MI.nstations = nstaUsed;
  MI.mindist = mindist;
  /* Compute the quality of the magnitude when it is required and only for
   * value of magnitude greater than 1.0. Otherwise quality is -1. */
  if(MI.mag > 1.0  &&  plmParams->MLQpar1 > 0.0) {
      MI.quality = nchan /( MI.mag * plmParams->MLQpar1);
      MI.quality = (MI.quality > 1.0)? 1.0 : MI.quality;
  }

 Done:
  MI.nchannels = nchan;
  
  if ( plmParams->saveSCNL ) {
     rc = wr_mag_scnl( &MI, buf, MAX_BYTES_PER_EQ);
  } else {
     rc = wr_mag( &MI, buf, MAX_BYTES_PER_EQ);
  }

  logit("", "Magnitude message: \n%s", buf);

   if ( plmParams->MlmsgOutDir ) {
        sprintf(outfile_Mlmsg, "%s/%s_%d.lm", plmParams->MlmsgOutDir, pEvt->eventId, pEvt->origin_version);
        if ( !(f_Mlmsg = fopen( outfile_Mlmsg, "wt")) )
        {
                logit("et", "Unable to open %s file for writing: \n", outfile_Mlmsg );
                return;
        }
	else {
               fprintf(f_Mlmsg,"Magnitude message: \n%s", buf);
               fclose(f_Mlmsg);
             }
  }
  /* paulf - 2005/07/20 changes for Utah - new minStationsML parameter in .d file 
	fixed in 2.0.1 !
   */
  if (pEvt->nMags < plmParams->minStationsMl)
  {      /* create "No local magnitude available" message with eventId, */
         /*  magnitude type (ML) and magnitude-type index (1)  */
    wr_nomag_msg(buf, pEvt->eventId, MagNames[1], 1);
         /* append extra information to message */
    sprintf(&buf[strlen(buf)],
            " (min num of sta for ML not met, only %d avail)", pEvt->nMags);
    lm_status(&plmParams->pEW->noMagLogo, ERR_NOMAG, buf, plmParams->pEW);
    return;
  }
  
  if (plmParams->outputFormat == LM_OM_LM)
    lm_status(&plmParams->pEW->magLogo, 0, buf, plmParams->pEW);
  else if (plmParams->outputFormat == LM_OM_FL)
  {
    FILE *outfp;
    if ((outfp = fopen (plmParams->outputFile, "wt")) == NULL)
    {
      logit ("e", "Could not open output file: %s.\n", plmParams->outputFile);
      return;
    }
    
    fprintf (outfp, "%s", buf); 
    fclose (outfp);
  }

  if (rc < 0)
  {
    sprintf(buf, "localmag: ran out of space in message buffer"
            " TYPE_MAGNITUDE message is truncated\n");
    lm_status (&plmParams->pEW->errLogo, ERR_TRUNC, buf, plmParams->pEW);
    logit("et", buf);
  }

  return;
}
