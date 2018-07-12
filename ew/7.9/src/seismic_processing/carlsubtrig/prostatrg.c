
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: prostatrg.c 1534 2004-05-31 17:59:31Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2004/05/31 17:59:31  lombard
 *     Removed version string from CARLSTATRIG_SCNL message.
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
 * prostatrg.c: Process station triggers
 *              1) Inserts station trigger messages in the station queues
 */

/*******							*********/
/*	Functions defined in this source file				*/
/*******							*********/

/*	Function: ProcessStationTrigger					*/
/*									*/
/*	Inputs:		Pointer to Network Structure			*/
/*									*/
/*	Outputs:	trigger message on station queue		*/
/*									*/
/*	Returns:	0 on success					*/
/*		       -1 on failure					*/

/*******							*********/
/*	System Includes							*/
/*******							*********/
#include <stdio.h>
#include <sys/types.h>

/*******							*********/
/*	Earthworm Includes						*/
/*******							*********/
#include <earthworm.h>	/* logit, threads				*/

/*******							*********/
/*	CarlSubTrig Includes						*/
/*******							*********/
#include "carlsubtrig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*	Function: ProcessStationTrigger					*/
int ProcessStationTrigger( NETWORK* csuNet, char* msg )
{
  STATION* 	station;		/* the station of interest	*/
  STATRIG*	trigger;		/* the current, empty slot	*/
  STATRIG*	lastTrigger;		/* last trigger put in the stack */
  char		staCode[TRACE2_STA_LEN];
  char		compCode[TRACE2_CHAN_LEN];
  char		netCode[TRACE2_NET_LEN];
  char		locCode[TRACE2_LOC_LEN];
  double	onTime, offTime;	/* trigger on and off times	*/
  double	onEta;			/* eta when the trigger went on	*/
  long		sequence;		/* trigger sequence number	*/
  int		nScan;			/* result of sscanf		*/
  int		nextNextIn;		/* the slot after nextIn	*/
  int		prevIn;
  int		retVal = 0;
  int           i;
  
  /***********************************************************************  
   * The station trigger stack is a first-in/first-out array of trigger	 *
   * structures. nextIn points to an empty slot that will be the next 	 *
   * one filled. nextOut points to the slot containing the next trigger  *
   * to be used by SubnetThread. The nextOut slot will be empty only if  *
   * there are no triggers in this stack; in this case nextIn == nextOut *
   * Since nextIn always points to an empty slot (except when the slot   *
   * is being filled), the stack is considered full when there is no     *
   * empty slot beyond nextIn.                                           *
   ***********************************************************************/

  nScan = sscanf( msg, "%s %s %s %s %lf %lf %ld %lf", staCode, compCode, 
		  netCode, locCode, &onTime, &offTime, &sequence, &onEta );
  
  if ( nScan != 8 )
  {
    logit( "et", "Error reading station trigger message (%d items read)\n", nScan );
    retVal = ERR_READ_MSG;
  } 
  else 
  {
    if ( csuNet->csuParam.debug > 3 )
    {
      logit( "t", "carlSubTrig: Processing station trigger message.\n" );
      logit( "", "\t<%s.%s.%s.%s> on: %8.4lf off: %8.4lf\n", staCode, compCode,
	     netCode, locCode, onTime, offTime );
      logit( "", "\t\t eta: %.2lf sequence: %ld\n", onEta, sequence );
    }
    
    /* Locate the station structure for this trigger message 		*/
    station = FindStation( staCode, compCode, netCode, locCode, csuNet );
    
    if ( station && station->triggers )
    {  /* Found the station for this message, and it has a trigger stack. */
      /* Lock the station stacks while we enter a new trigger message	*/
      RequestSpecificMutex( &(csuNet->stationMutex) );
      
      /* Set pointers to trigger structures for this station...		*/
      /* ... the next empty slot					*/
      trigger = &(station->triggers[ station->nextIn ]);
      /* ... the slot beyond that					*/
      nextNextIn = ( station->nextIn + 1 ) % csuNet->nSlots;
      /* ... the previously filled slot					*/
      prevIn = ( station->nextIn - 1 ) % csuNet->nSlots;
      if ( prevIn < 0 ) prevIn += csuNet->nSlots;
      lastTrigger = &(station->triggers[ prevIn ]);

      /* Check sequence of trigger messages				*/
      /* DANGER: sequence numbers get reset if carlStaTrig restarts	*/
      /*   so we have to rely on the ON time here.			*/
      if ( onTime < lastTrigger->onTime )
      {
	logit( "et", "Trigger message from <%s.%s.%s.%s> out of order!\n",
	       staCode, compCode, netCode, locCode );
	retVal = ERR_ORDER;
      }
      /* Are we safe using equality with doubles? May need to use	*/
      /*   near-equality ####						*/
      else if ( onTime == lastTrigger->onTime )
      {	/* this is the mate (OFF?) to a previous ON?	*/
	if ( offTime > 0.0 && lastTrigger->offTime == 0.0 )
	{  /* good: we already had a triggerON and we just got a triggerOFF */
	  lastTrigger->offTime = offTime;
	  if ( csuNet->csuParam.debug > 3 )
	    logit( "", "OFF matches an ON message; dur %lf\n", 
		   offTime - onTime );
	}
	else if ( offTime == 0.0 && lastTrigger->offTime > 0.0 )
	{  /* bad: we got the triggerOFF before the triggerON		*/
	  logit( "et", "triggerOFF before triggerON from <%s.%s.%s.%s>\n",
		 staCode, compCode, netCode, locCode );
	  retVal = ERR_ORDER;
	} 
	else /* real bad: both or neither triggers have off times 	*/
	{
	  logit( "et", "Bad OFF times from <%s.%s.%s.%s>: last: %8.4lf this: %8.4lf\n",
		 staCode, compCode, netCode, locCode, lastTrigger->offTime, offTime );
	  retVal = ERR_UNKNOWN;
	}
      }
      else	/* This is a new trigger message			*/
      {
	/* Make sure there is room: is the next slot empty?		*/
	if ( station->triggers[nextNextIn].sequence == -1 )
	{  /* there is room, so insert this trigger in the stack	*/
	  if ( csuNet->csuParam.debug > 3 )
	    logit( "", "New message in slot %d next\n", station->nextIn,
		   nextNextIn );
	  trigger->onTime = onTime;
	  trigger->offTime = offTime;
	  trigger->onEta = onEta;
	  trigger->sequence = sequence;
	  station->nextIn = nextNextIn;
	}
	else	/* no room; raise a ruckus				*/
	{
 	  /* maybe we can remove it if it's old enough 			*/
 	  /* Really this is SubnetThread's responsibility, but we'll
 	   * lend a helping hand in case something got overlooked there */
 	  time_t clockNow = time( NULL );
 	  time_t netNow = clockNow - ( time_t ) csuNet->latency;
 	  if ( ( station->triggers[nextNextIn].offTime != 0.0 &&
 		 netNow > station->triggers[nextNextIn].offTime ) ||
 	       ( station->triggers[nextNextIn].offTime == 0.0 &&
 		 netNow >= (long) station->triggers[nextNextIn].onTime + 
 		 csuNet->DefaultStationDur ) )
 	  {
 	    logit( "et", "Removing a stale trigger from <%s.%s.%s.%s>\n", 
 		   staCode, compCode, netCode, locCode );
 	    RemoveStaTrig( station, &(station->triggers[nextNextIn]),
 			   csuNet->nSlots );
 	  } 
 	  else /* trigger not old enough to remove */
 	  {
 	    logit( "et", "trigger stack full for <%s.%s.%s.%s>\n", staCode, 
 		   compCode, netCode, locCode );
 	    if ( csuNet->csuParam.debug > 4 )
 	    {
 	      logit("", "nextIn: %d nextnext %d nextSeq %d prev: %d\n", 
 		    station->nextIn, 
 		    nextNextIn, 
 		    station->triggers[nextNextIn].sequence, 
 		    prevIn );
 	      for ( i = 0; i < csuNet->nSlots; i++ )
 		logit("", "%d start: %8.4lf end: %8.4lf\n", i,
 		      station->triggers[i].onTime, 
 		      station->triggers[i].offTime );
 	    }
 	    retVal = ERR_FULLQUEUE;
 	  }
	}
      }
    } 
    else 
    {
      logit( "t", "can't find <%s.%s.%s.%s> in station list\n", staCode,
	     compCode, netCode, locCode );
      /* This doesn't seem like a big error, so we'll return a 		*/
      /*   good status							*/
    }
  }
  
  ReleaseSpecificMutex( &(csuNet->stationMutex) );
  return ( retVal );
}
