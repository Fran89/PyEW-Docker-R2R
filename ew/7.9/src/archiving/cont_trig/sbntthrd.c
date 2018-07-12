
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sbntthrd.c,v 1.0 2010/06/10 18:00:00  JMS Exp $
 *
 *    Revision history:
 *     $Log: sbntthrd.c,v $
 *     Revision 1.0  2010/06/10 18:00:00  JMS
 *     Initial revision
 *
 *
 */

/*
 * sbntthrd.c: Subnet thread
 *              1) Initializes some values
 *		2) Wait the trigger time added with the latency
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
/*	Cont_Trig Includes						*/
/*******							*********/
#include "cont_trig.h"

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
  NETWORK* contNet;
  STATION*	sta;		/* current station			*/
  time_t 	clockNow;       /* Wall clock time, seconds since epoch */
  double	hrClockNow;	/* high resolution wall clock		*/
  unsigned	snooze;		/* sleep interval in milliseconds	*/
  static time_t	netNow;		/* Network's idea of current time	*/
  char		msg[MAXMESSAGELEN];	/* status message buffer	*/
  int		i;
  long		newTrig;
  
  contNet = ( NETWORK* ) net;
  
  /*	Set the network clock when we start up				*/
  clockNow = time( NULL );
  netNow = clockNow - ( time_t ) contNet->latency;
  SubnetThreadHeart = clockNow;

  /*	Initialize trigger duration and enable all stations		*/
  contNet->duration = contNet->TrigDur;
  for ( i = 0; i < contNet->nSta; i++ )
    contNet->stations[i].listMe = 1;  

  /*	Initialize first time to save data				*/
  /*	Chooses a round second, minute, hour, depending on input	*/
  newTrig = (long)( clockNow / contNet->duration + 1 ) * contNet->duration;
  if ( contNet->contParam.debug > 3 )
	logit( "", "NowEpoch = %ld NextTrigEpoch : %ld\n", (long)clockNow,
		newTrig );
 
  /* 	Main Subnet loop, forever					*/
  while( 1 )
  {
    /* 	Check the network clock against the wall clock			*/
    clockNow = time( NULL );
    SubnetThreadHeart = clockNow;
    /* We shouldn't get behind, since the sleep time, at end of this 	*/
    /*  loop is adjusted to keep us only <latency> seconds behind.	*/
    if ( netNow < clockNow - ( time_t ) contNet->latency )
    {
      sprintf( msg, "Cont_Trig: Network clock %ld seconds slow", 
	       (long)(clockNow - netNow - contNet->latency) );
      StatusReport( contNet, contNet->contEwh.typeError, ERR_LATE, msg );
    }
    if ( contNet->contParam.debug > 3 )
      logit( "", "subnet thread clocks: net %ld wall %ld newTrig %ld\n", (long)netNow,
	     (long)clockNow, (long)newTrig );
    
    if ( netNow >= newTrig )
    {
	if ( netNow > newTrig )
	{
		sprintf( msg, "Cont_Trig: Trigger is %ld seconds late", 
			(long)(clockNow - netNow - contNet->latency) );
		StatusReport( contNet, contNet->contEwh.typeError, ERR_LATE, msg );
	}
	contNet->onTime = newTrig - contNet->TrigDur;
	for ( i = 0; i < contNet->nSta; i++ )
	{ 
		sta = &(contNet->stations[i]);
		if ( sta->listMe == 0 )
		{
			sta->saveOnTime = newTrig- contNet->TrigDur;
			sta->saveOnEta = newTrig- contNet->TrigDur;
			sta->listMe = 1;
		}
	}
	ProduceTriggerMessage( contNet );
	/* reset the saved ON time and eta after they're reported	*/
	/*   This may already be done by ProduceTriggerMessage()        */
	for ( i = 0; i < contNet->nSta; i++ )
	{
	  contNet->stations[i].saveOnTime = 0.0;
	  contNet->stations[i].saveOnEta = 0.0;
	}
	newTrig = newTrig + contNet->TrigDur;
    }

    if ( contNet->terminate )
      break;	/* All done, say goodby					*/

    /* Step the network clock ahead one second				*/
    netNow++;

    /* Take a short nap until the next second rolls around		*/
    hrtime_ew( &hrClockNow );
    if ( hrClockNow < ( double ) ( netNow + contNet->latency ) )
      {
	snooze = (unsigned) ( ( netNow + contNet->latency - hrClockNow ) * 
			      1000.0 );
	sleep_ew( snooze );
      }
  }	/* end of main loop						*/
  
  return THR_NULL_RET;
}

  
