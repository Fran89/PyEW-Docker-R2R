
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: statrpt.c 5739 2013-08-07 14:33:42Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2007/02/26 13:44:40  paulf
 *     fixed heartbeat sprintf() to cast time_t as long
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
/*	CarlSubTrig Includes						*/
/*******							*********/
#include "carlsubtrig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*	Function: SubtusReport						*/
void StatusReport( NETWORK* csuNet, unsigned char type, short code, 
		   char* message )
{
  char		outMsg[MAXMESSAGELEN];	/* The outgoing message.	*/
  time_t	msgTime;	/* Time of the message.			*/

  /*	Validate the input parameters					*/
  if ( csuNet )
  {
    /*	Get the time of the message					*/
    time( &msgTime );

    /*	Build & process the message based on the type			*/
    if ( csuNet->csuEwh.typeHeartbeat == type )
    {
       sprintf( outMsg, "%ld %d\n", (long) msgTime, csuNet->MyPid );
 
      /*Write the message to the output region				*/
       if ( csuNet->csuParam.debug > 3 )
         logit( "t", "status: (i%u m%u t%u)\n", csuNet->hrtLogo.instid, 
  	        csuNet->hrtLogo.mod, csuNet->hrtLogo.type );
    
       if ( tport_putmsg( &(csuNet->regionOut), &(csuNet->hrtLogo), 
		       (long) strlen( outMsg ), outMsg ) != PUT_OK )
       {
         /*	Log an error message					*/
         logit( "et", "carlSubTrig: Failed to send a heartbeat message (%d).\n",
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
       if ( csuNet->csuParam.debug > 3 )
         logit( "t", "status: (i%u m%u t%u)\n", csuNet->errLogo.instid, 
  	        csuNet->errLogo.mod, csuNet->errLogo.type );
    
       if ( tport_putmsg( &(csuNet->regionOut), &(csuNet->errLogo), 
		       (long) strlen( outMsg ), outMsg ) != PUT_OK )
       {
         /*	Log an error message					*/
         logit( "et", "carlSubTrig: Failed to send an error message (%d).\n",
		code );
       }

    }
  }
}
