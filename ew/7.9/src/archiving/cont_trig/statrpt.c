
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: statrpt.c,v 1.0  2010/06/10 18:00:00  JMS Exp $
 *
 *    Revision history:
 *     $Log: statrpt.c,v $
 *     Revision 1.0  2010/06/10 18:00:00  JMS
 *     Initial revision
 *
 *
 */

/*
 * statrpt.c: Produce an error or heartbeat message on the output ring.
 *              1) Construct the correct type of message.
 *              2) Send the message to the output ring.
 */

/*******							*********/
/*	Functions defined in this source file				*/
/*******							*********/

/*	Function: StatusReport						*/
/*									*/
/*	Inputs:		Pointer to the Network structure		*/
/*			Message type					*/
/*			Message id(code)				*/
/*			Pointer to a string(message)			*/
/*									*/
/*	Outputs:	Message structure sent to output ring		*/
/*									*/
/*	Returns:	Nothing						*/

/*******							*********/
/*	System Includes							*/
/*******							*********/
#include <stdio.h>
#include <sys/types.h>	/* time						*/
#include <time.h>	/* time						*/
#include <string.h>    /* strlen					*/

/*******							*********/
/*	Earthworm Includes						*/
/*******							*********/
#include <earthworm.h>	/* logit					*/
#include <transport.h>	/* MSG_LOGO, SHM_INFO, tport_putmsg		*/

/*******							*********/
/*	Cont_Trig Includes						*/
/*******							*********/
#include "cont_trig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*	Function: SubtusReport						*/
void StatusReport( NETWORK* contNet, unsigned char type, short code, 
		   char* message )
{
  char		outMsg[MAXMESSAGELEN];	/* The outgoing message.	*/
  time_t	msgTime;	/* Time of the message.			*/

  /*	Validate the input parameters					*/
  if ( contNet )
  {
    /*	Get the time of the message					*/
    time( &msgTime );

    /*	Build & process the message based on the type			*/
    if ( contNet->contEwh.typeHeartbeat == type )
    {
       sprintf( outMsg, "%ld %ld\n", (long) msgTime, (long) contNet->MyPid );
 
      /*Write the message to the output region				*/
       if ( contNet->contParam.debug > 3 )
         logit( "t", "status: (i%u m%u t%u)\n", contNet->hrtLogo.instid, 
  	        contNet->hrtLogo.mod, contNet->hrtLogo.type );
    
       if ( tport_putmsg( &(contNet->regionOut), &(contNet->hrtLogo), 
		       (long) strlen( outMsg ), outMsg ) != PUT_OK )
       {
         /*	Log an error message					*/
         logit( "et", "cont_trig: Failed to send a heartbeat message (%d).\n",
		code );
       }
    }
    else
    {
       if ( message ) {
  	 sprintf( outMsg, "%ld %hd %s\n", (long) msgTime, code, message );
         logit("t","Error:%d (%s)\n", code, message );
       }
       else {
	 sprintf( outMsg, "%ld %hd\n", (long) msgTime, code );
         logit("t","Error:%d (No description)\n", code );
       }

       /*Write the message to the output region				*/
       if ( contNet->contParam.debug > 3 )
         logit( "t", "status: (i%u m%u t%u)\n", contNet->errLogo.instid, 
  	        contNet->errLogo.mod, contNet->errLogo.type );
    
       if ( tport_putmsg( &(contNet->regionOut), &(contNet->errLogo), 
		       (long) strlen( outMsg ), outMsg ) != PUT_OK )
       {
         /*	Log an error message					*/
         logit( "et", "cont_trig: Failed to send an error message (%d).\n",
		code );
       }

    }
  }
}
