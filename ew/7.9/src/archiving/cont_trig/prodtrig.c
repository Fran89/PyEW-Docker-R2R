
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: prodtrig.c,v 1.0  2010/06/10 18:00:00  JMS Exp $
 *
 *    Revision history:
 *     $Log: prodtrig.c,v $
 *     Revision 1.0  2010/06/10 18:00:00  JMS
 *     Initial revision
 *
 *
 */

/*
 * prodtrig.c: Produce a trigger message.
 *              1) Create the trigger message.
 *		2) Send the trigger message to the output ring.
 *
 * Modified 11/5/1998 for Y2K compliance: PNL				
 */

/*******							*********/
/*	Functions defined in this source file				*/
/*******							*********/

/*	Function: ProduceTrigger					*/
/*									*/
/*	Inputs:		Pointer to the Cont_Trig Network		*/
/*									*/
/*	Outputs:	Message sent to the output ring			*/
/*									*/
/*	Returns:	0 on success					*/
/*			Error code as defined in cont_trig.h on 	*/
/*			  failure					*/

/*******							*********/
/*	System Includes							*/
/*******							*********/
#include <stdio.h>
#include <string.h>	/* strcat, strcmp, strlen			*/
#include <sys/types.h>
#include <time.h>	/* time_t, tm					*/
#include <stdlib.h>	/* realloc					*/
#include <ctype.h>	/* isdigit					*/

/*******							*********/
/*	Earthworm Includes						*/
/*******							*********/
#include <earthworm.h>	/* logit					*/
#include <time_ew.h>	/* gmtime_ew					*/
#include <transport.h>	/* MSG_LOGO, SHM_INFO				*/

/*******							*********/
/*	Cont_Trig Includes						*/
/*******							*********/
#include "cont_trig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*	Function: ProduceTrigger					*/
int ProduceTriggerMessage( NETWORK* contNet )
{
  char		staLine[MAXLINELEN];	/* Station output line.		*/
  int		retVal = 0;	/* Return value for this function.	*/
  struct tm	tmTimeOn;	/* Event time on as a tm structure.	*/
  struct tm	tmStaOn;	/* Station on time as a tm structure	*/
  time_t	ttTimeOn;	/* Event time on as a time_t type.	*/
  time_t	ttStaOn;
  char          trigtime[30];   /* Trigger time string			*/
  int           ntrigsub = 0;   /* total number of triggered subnets    */
  int           overflow = 0;   /* does message overflow output buffer? */
  int		i, isub, jsta;
  FILE          *idfp;          /* ID file pointer			*/
  STATION*      station = contNet->stations;
  SUBNET*       subnet = contNet->subnets;

  /* Added by Eugene Lublinsky, 3/31/Y2K */
  int           singleSubnetNumber, namedFlag = 1;

  /*	Convert network onTime to a tm structure			*/
  ttTimeOn = (time_t) contNet->onTime;
  gmtime_ew( &ttTimeOn, &tmTimeOn );

  /* Initialize the beginning of the outgoing message		*/
  /* Year 2000 compliance Y2K					*/
  /* Changed by Eugene Lublinsky, 3/31/Y2K */
  /* There are two possible contents for the Author field now  */
  /* further changed by Carol Bryan 3/21/01 - author field goes
  ** back to its designed use and the subnet is carried at the 
  ** end of the first line of the TrigList msg                  */
  if (namedFlag) 
      sprintf( contNet->trigMsgBuf,
	       "%s EVENT DETECTED     %4d%02d%02d %02d:%02d:%02d.00 UTC "
	       "EVENT ID: %lu AUTHOR: %03u%03u%03u SUBNET: %-s\n\n",
	       CONT_VERSION, 
	       tmTimeOn.tm_year + 1900, tmTimeOn.tm_mon + 1,
	       tmTimeOn.tm_mday, tmTimeOn.tm_hour, tmTimeOn.tm_min,
	       tmTimeOn.tm_sec, contNet->eventID,
	       contNet->contEwh.typeTrigList, contNet->contEwh.myModId,
	       contNet->contEwh.myInstId, contNet->contParam.OriginName);
  else
      sprintf( contNet->trigMsgBuf, 
	       "%s EVENT DETECTED     %4d%02d%02d %02d:%02d:%02d.00 UTC "
	       "EVENT ID: %lu AUTHOR: %03u%03u%03u\n\n",
	       CONT_VERSION,
	       tmTimeOn.tm_year + 1900, tmTimeOn.tm_mon + 1,
	       tmTimeOn.tm_mday, tmTimeOn.tm_hour, tmTimeOn.tm_min, 
	       tmTimeOn.tm_sec, contNet->eventID,
	       contNet->contEwh.typeTrigList, contNet->contEwh.myModId,
	       contNet->contEwh.myInstId );
  if ( contNet->contParam.debug >= 1 )
      logit( "o", "%s", contNet->trigMsgBuf );
  strcat( contNet->trigMsgBuf, "Sta/Cmp/Net/Loc   Date   Time                       "
	  "start save       duration in sec.\n" );
  if ( contNet->contParam.debug >= 1 )
      logit( "o", "Sta/Cmp/Net/loc   Date   Time                       "
	 "start save       duration in sec.\n" );
  strcat( contNet->trigMsgBuf, "---------------   ------ --------------- "
	  "   -----------------------------------------\n" );
  if ( contNet->contParam.debug >= 1 )
      logit( "o", "---------------   ------ --------------- "
	 "   -----------------------------------------\n" );
      
  /* Format the time for saving data					*/
  ttTimeOn = contNet->onTime - contNet->PreEventTime;
  gmtime_ew( &ttTimeOn, &tmTimeOn );
      
  /* Step through each station					*/
  for ( i = 0; i < contNet->nSta; i++ ) {
      if ( contNet->stations[i].listMe ) {
	  if (contNet->stations[i].saveOnTime > 0.0) {
	      ttStaOn = ( time_t ) contNet->stations[i].saveOnTime;
	      gmtime_ew( &ttStaOn, &tmStaOn );
	      sprintf(trigtime, "%4d%02d%02d %02d:%02d:%02d.%02d UTC ",
		      tmStaOn.tm_year + 1900,
		       tmStaOn.tm_mon + 1, tmStaOn.tm_mday, tmStaOn.tm_hour,
		       tmStaOn.tm_min, tmStaOn.tm_sec,
		       (int) ( ( contNet->stations[i].saveOnTime - ttStaOn ) 
			       * 100 + 0.5 ));
	  }
	  else {
	      sprintf(trigtime, "%s", "00000000 00:00:00.00 UTC");
	  }
	      
	  /* Add the station specific information into the output message */
	  if ( contNet->compAsWild ) {
	      sprintf( staLine, " %s * %s %s P %s save: %4d%02d%02d %02d:%02d:%02d.00      %ld\n",
		       contNet->stations[i].staCode, 
		       contNet->stations[i].netCode, 
		       contNet->stations[i].locCode, trigtime,
		       tmTimeOn.tm_year + 1900, tmTimeOn.tm_mon + 1,
		       tmTimeOn.tm_mday, tmTimeOn.tm_hour, tmTimeOn.tm_min,
		       tmTimeOn.tm_sec, contNet->duration + contNet->PreEventTime );
	  } else {
	      sprintf( staLine, " %s %s %s %s P %s save: %4d%02d%02d %02d:%02d:%02d.00      %ld\n",
		       contNet->stations[i].staCode, contNet->stations[i].compCode,
		       contNet->stations[i].netCode, contNet->stations[i].locCode,
		       trigtime,
		       tmTimeOn.tm_year + 1900, tmTimeOn.tm_mon + 1,
		       tmTimeOn.tm_mday, tmTimeOn.tm_hour, tmTimeOn.tm_min,
		       tmTimeOn.tm_sec, contNet->duration + contNet->PreEventTime );
	  }
	      
	  /* Show that this station has been reported */
	  contNet->stations[i].listMe = 0;
	      
	  /* Check for buffer overflow				*/
	  if ( ( strlen( contNet->trigMsgBuf ) + strlen( staLine ) ) >= 
	       contNet->trigMsgBufLen ) {
	      overflow = 1;
	      retVal   =  ERR_SHORT_MSG;
	  }
	      
	  /* Append the station information to the output message	*/
	  if( !overflow ) strcat( contNet->trigMsgBuf, staLine );
	  if ( contNet->contParam.debug >= 1 )
		logit( "o", "%s", staLine );
      }
  }
      
  /* a blank line at the end for Lynn */
  if ( contNet->contParam.debug >= 1 )
      logit( "o", "\n");
      
  /* Send the output message, even if incomplete, to the output ring */
  if ( tport_putmsg( &(contNet->regionOut), &(contNet->trgLogo), 
		     strlen( contNet->trigMsgBuf ), contNet->trigMsgBuf )
       != PUT_OK ) {
      logit( "et", "cont_trig: Error sending a trigger message to ring.\n" );
      retVal = ERR_PROD_MSG;
  }
      
  /* Send an error to statmgr if there wasn't room for the whole output msg */
  if( overflow ) {
      sprintf( staLine, "incomplete triglist msg sent for eventid:%ld",
	       contNet->eventID );
      StatusReport( contNet, contNet->contEwh.typeError, ERR_SHORT_MSG, staLine );
  }
      
  /* Update the eventID and its file */
  contNet->eventID++;
  idfp = fopen( "cont_id.d", "w" );
  if ( idfp != (FILE *) NULL ) {
      fprintf( idfp,
	       "# Next available trigger sequence number:\n" );
      fprintf( idfp, "next_id %ld\n", contNet->eventID );
      fclose( idfp );
  }
      
  return ( retVal );
}
