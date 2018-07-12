
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sbntthrd.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2007/03/28 18:28:34  paulf
 *     fixed for MAC OS X
 *
 *     Revision 1.3  2005/07/20 15:26:18  friberg
 *     added a #ifndef for Linux to not include synch.h
 *
 *     Revision 1.2  2004/05/11 17:49:07  lombard
 *     Added support for location code, TYPE_CARLSTATRIG_SCNL and TYPE_TRIGLIST_SCNL
 *     messages.
 *     Removed OS2 support
 *
 *     Revision 1.1  2000/02/14 16:14:42  lucky
 *     Initial revision
 *
 *
 */

/*
 * sbntthrd.c: Subnet thread
 *              1) Retrieves station trigger messages from their queues
 *		2) Manages station and network trigger countdown timers
 *		3) Maintains network clock
 *		4) Initiates network trigger messages
 */

/*******							*********/
/*	Functions defined in this source file				*/
/*******							*********/

/*	Function: SubnetThread						*/
/*									*/
/*	Inputs:		Pointer to Network Structure			*/
/*									*/
/*	Outputs:	Message sent to the output ring			*/
/*									*/
/*	Returns:	0 on success					*/
/*		       -1 on failure					*/

/*******							*********/
/*	System Includes							*/
/*******							*********/
#include <stdio.h>
#include <sys/types.h>
#include <time.h>	/* time_t, tm					*/
#ifdef _WINNT
#include <windows.h>
#define mutex_t HANDLE
#else
#if defined(_SOLARIS)
	/* this include not needed in Linux or Mac OS X where pthreads are used! */
#include <synch.h>	/* mutex's					*/
#endif
#endif

/*******							*********/
/*	Earthworm Includes						*/
/*******							*********/
#include <earthworm.h>	/* logit, threads				*/
#include <transport.h>	/* MSG_LOGO, SHM_INFO				*/
#include <time_ew.h>	/* hrtime_ew					*/

/*******							*********/
/*	CarlSubTrig Includes						*/
/*******							*********/
#include "carlsubtrig.h"

#define NEARLY_ZERO 1.0

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********//*******                                                        *********/
/*      Global variable declarations                                    */
/*******                                                        *********/
extern volatile time_t SubnetThreadHeart; /* changes in sbntthrd.c,     */
                                          /* monitored in main thread   */

/*	Function: SubnetThread						*/
thr_ret SubnetThread( void* net )
{
  NETWORK* csuNet;
  STATION*	sta;		/* current station			*/
  STATRIG*	trig;		/* the trigger message of interest	*/
  time_t 	clockNow;       /* Wall clock time, seconds since epoch */
  double	hrClockNow;	/* high resolution wall clock		*/
  unsigned	snooze;		/* sleep interval in milliseconds	*/
  static time_t	netNow;		/* Network's idea of current time	*/
  char		msg[MAXMESSAGELEN];	/* status message buffer	*/
  int		i, j;
  int		lastOut;	/* Index to the one past the last 	*/
  				/*   station trigger			*/
  long		newTrig;
  static long	oldTrig;	/* The network countdown timer		*/
  int      dataTimeOffset = 0;  /* secs from wall clock we set netNow to */
  
  csuNet = ( NETWORK* ) net;
  
  /*	Set the network clock when we start up				*/
  clockNow = time( NULL );
  netNow = clockNow - ( time_t ) csuNet->latency;
  SubnetThreadHeart = clockNow;
  if (csuNet->useDataTime) netNow = -1;
 
  /* 	Main Subnet loop, forever					*/
  while( 1 )
  {
    /* 	Check the network clock against the wall clock			*/
    clockNow = time( NULL );
    SubnetThreadHeart = clockNow;
    /* We shouldn't get behind, since the sleep time, at end of this 	*/
    /*  loop is adjusted to keep us only <latency> seconds behind.	*/
    if ( !csuNet->useDataTime && (netNow < clockNow - ( time_t ) csuNet->latency) )
    {
      sprintf( msg, "CarlSubTrig: Network clock %ld seconds slow", 
	       (long)(clockNow - netNow - csuNet->latency) );
      StatusReport( csuNet, csuNet->csuEwh.typeError, ERR_LATE, msg );
    }
    if ( csuNet->csuParam.debug > 3 )
      logit( "", "subnet thread clocks: net %ld wall %ld\n", netNow, 
	     clockNow );
    
    /*	Lock the station message stack while we peruse it		*/
    RequestSpecificMutex( &(csuNet->stationMutex) );
    
    /* Look at every station for updates				*/
    for ( i = 0; i < csuNet->nSta; i++ )
    {
      sta = &(csuNet->stations[i]);
      /* Decrement the active station trigger timers			*/
      if ( sta->countDown ) 
	{
	  sta->countDown--;
	  if ( sta->countDown == 0 )
	    { /* station timed out; clear out its memory, except for	*/
	      /*   saveOnTime, which is set when network triggers and 	*/
	      /*   cleared when network trigger message is sent.	*/
	      sta->onTime = 0.0;
	      sta->onEta = 0.0;
	    }
	}

      /* Look for new station trigger messages in the station stacks	*/
      lastOut = sta->nextIn;
      
      if ( csuNet->csuParam.debug > 4 )
	logit( "", "%s In %d out %d\n", sta->staCode, sta->nextIn,
	       sta->nextOut );
      
      if ( lastOut == sta->nextOut )	/* No messages for this station	*/
	continue;			/* go on to the next		*/
      
      if ( lastOut < sta->nextOut )
	lastOut += csuNet->nSlots;	/* messages wrap around stack	*/
      
      for ( j = sta->nextOut; j < lastOut; j++ )
      {
	trig = &(sta->triggers[j % csuNet->nSlots]);
	if ( csuNet->csuParam.debug > 4 && trig->onTime > 0.0 )
	  logit( "", "<%s.%s.%s.%s> %d on %lf off %lf\n", sta->staCode, sta->compCode,
		 sta->netCode, sta->locCode, j, trig->onTime, trig->offTime );
	
        if ( netNow == -1 && trig->onTime > 0.0) {
          netNow = (time_t)(trig->onTime + 2); /* arbitrarily set it to first trigger on time plus 2 secs */
          dataTimeOffset = (int)(clockNow - netNow);
	  logit( "", "PLAYBACK MODE netNow set to %lf of statrig <%s.%s.%s.%s>\n", trig->onTime, sta->staCode, sta->compCode,
		 sta->netCode, sta->locCode);
        }
	if ( netNow < ( time_t ) trig->onTime )   
	  /* this and all other triggers are too young	*/
	  break;
	
	/* netNow >= trig->onTime: trigger could be of interest		*/
	if ( netNow < ( time_t ) trig->offTime )
	{	/* station is currently triggered			*/
	  sta->countDown = sta->timeToLive;
	  if ( sta->onTime == 0.0 )
	  {	/* first time for this station	*/
	    sta->onTime = trig->onTime;
	    sta->onEta = trig->onEta;
	    if ( csuNet->csuParam.debug > 3 )
	      logit( "", "station %d ON at %lf\n", i, sta->onTime );
	  }
	}
	else if ( trig->offTime > NEARLY_ZERO )
	{	/* This trigger message is old 	*/
	  if ( sta->countDown > sta->timeToLive )
	  {	/* This triggerOFF message just arrived, so fix the 	*/
	    	/*   countdown timer, previously set to the default 	*/
	    	/*   duration.						*/
	    sta->countDown = sta->timeToLive - ( int )( netNow - trig->offTime );
	    if ( sta->countDown < 0 )
	      sta->countDown = 0;
	  }
	  RemoveStaTrig( sta, trig, csuNet->nSlots );
	  if ( csuNet->csuParam.debug > 3 )
	    logit("", "Removed OFF trigger %d for station %d; next: %d\n", 
		  j % csuNet->nSlots, i, sta->nextOut );
	} 
	else /* trig->offTime == 0.0; still no triggerOFF message	      	*/
	{
	  if ( netNow >= ( long ) trig->onTime + csuNet->DefaultStationDur )
	  {	/* trigger is definitely old; get rid of it		*/
	    RemoveStaTrig( sta, trig, csuNet->nSlots );
	  if ( csuNet->csuParam.debug > 3 )
	    logit("", "Removed ON trigger %d for station %d; next: %d\n", 
		  j % csuNet->nSlots, i, sta->nextOut );
	  } 
	  else /* Allow this trigger to last the  default duration	*/
	  {
	    sta->countDown = sta->timeToLive;
	    if ( sta->onTime == 0.0 )
	    {	/* first time for this station	*/
	      sta->onTime = trig->onTime;
	      sta->onEta = trig->onEta;
	    }
	  }
	}
      }		/* for each trigger message	*/
    }		/* for each station		*/
    
    /* All done with the station trigger stacks; give back their lock	*/
    ReleaseSpecificMutex( &(csuNet->stationMutex) );

    /*	Now look at the trigger status of each subnet			*/
    if ( (newTrig = GetSubnet( csuNet )) != 0 )
    {	/*  network is now triggered	*/
      
      if ( ! oldTrig )
      {	/*  This is a new network trigger	*/
	csuNet->onTime = netNow;
	csuNet->duration = 0;
	logit("t", "Trigger ON at %ld; initial duration: %d\n", 
	      csuNet->onTime, newTrig );
	
	// RSL: If early warning, produce message now
	if ( csuNet->early_warning > 0 )
	{
	
	  for ( i = 0; i < csuNet->nSta; i++ )
          { 
	    sta = &(csuNet->stations[i]);
	    if ( sta->listMe == 0 )
	    {
	      sta->saveOnTime = sta->onTime;
	      sta->saveOnEta = sta->onEta;
	      if (sta->onTime > 0.0 )
	        sta->listMe = 1;
	    }
          }
          
          // Set early_warning trigger to 2, indicating that this is the first message
          csuNet->early_warning = 2; // It should be set back to one after this message
          
          // Send early warning message
	  ProduceTriggerMessage( csuNet );
	}
      }
    }
    else	/* newTrig == 0; no subnets triggered	*/
    {
      newTrig = oldTrig;
    }
    /* set the network countdown timer to what comes out of GetSubnets	*/
    /*   unless that is zero						*/
    /*oldTrig = newTrig; */ /* original code; commented out 990309:LDD  */

    /* set the network countdown timer to what comes out of GetSubnets  */
    /*   unless that is smaller than the current countdown value.       */
    /*   Replaces original code (above):  LDD 990309                    */ 
    if( newTrig > oldTrig ) oldTrig = newTrig;

    
    if ( oldTrig )
    {	/* Network is currently active	*/
      oldTrig--;	/* Count down one second */

      /* Save the first station trigger-on times of an event for the	*/
      /*   trigger message. These times will be zeroed in		*/
      /*   ProduceTriggerMessage().					*/
      for ( i = 0; i < csuNet->nSta; i++ )
      { 
	sta = &(csuNet->stations[i]);
	if ( sta->listMe == 0 )
	{
	  sta->saveOnTime = sta->onTime;
	  sta->saveOnEta = sta->onEta;
	  if (sta->onTime > 0.0 )
	    sta->listMe = 1;
	}
      }
      
      /* See if the trigger is done					*/
      if ( oldTrig == 0 || csuNet->duration >= csuNet->MaxDur )
      {
	oldTrig = 0;	/* in case it was duration that shut us off	*/
	logit("t", "Trigger OFF at %ld; duration: %ld\n", netNow, 
	      csuNet->duration);

	/* Tell the world...						*/
	ProduceTriggerMessage( csuNet );
	
	/* reset the saved ON time and eta after they're reported	*/
	/*   This may already be done by ProduceTriggerMessage()        */
	for ( i = 0; i < csuNet->nSta; i++ )
	{
	  csuNet->stations[i].saveOnTime = 0.0;
	  csuNet->stations[i].saveOnEta = 0.0;
	}
		
	/* Reset all the subnet trigger flags				*/
	for ( j = 0; j < csuNet->nSub; j++ )
	  csuNet->subnets[j].Triggered = 0;
	csuNet->numSub = 0;

      }
      csuNet->duration++;	/* Keep track of how long we're triggered */
    }
    if ( csuNet->terminate )
      break;	/* All done, say goodby					*/

    if (netNow == -1) {
      sleep_ew(1000); /* we are in playback mode */
      continue; /* still waiting for a trigger to set netNow */
    }
    /* Step the network clock ahead one second				*/
    netNow++;

    /* Take a short nap until the next second rolls around		*/
    hrtime_ew( &hrClockNow );
    if ( hrClockNow < ( double ) ( netNow + csuNet->latency ) )
      {
	snooze = (unsigned) ( ( netNow + csuNet->latency - hrClockNow ) * 
			      1000.0 );
	sleep_ew( snooze );
      }
    else if ( csuNet->useDataTime && hrClockNow < ( double ) ( netNow + dataTimeOffset ) ) 
      {
	snooze = (unsigned) ( ( netNow + dataTimeOffset - hrClockNow ) * 
			      1000.0 );
	sleep_ew( snooze );
      }
  }	/* end of main loop						*/
  
  return THR_NULL_RET;
}

  
