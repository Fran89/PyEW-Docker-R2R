
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: prodtrig.c 5739 2013-08-07 14:33:42Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.10  2009/08/28 17:48:36  paulf
 *     added TrigIdFilename option for pointing to trig_id.d
 *
 *     Revision 1.9  2004/07/14 20:34:39  lombard
 *     Cleaned up reporting of trigger time in trigger message for stations not in a
 *     triggered subnet: should be 0... instead of 1970...
 *
 *     Revision 1.8  2004/05/11 17:49:07  lombard
 *     Added support for location code, TYPE_CARLSTATRIG_SCNL and TYPE_TRIGLIST_SCNL
 *     messages.
 *     Removed OS2 support
 *
 *     Revision 1.7  2003/02/27 23:21:07  dietz
 *     Changed comments in TYPE_TRIGLIST2K msg and logfile from
 *     Sta/Net/Cmp to Sta/Cmp/Net to match the actual order of the fields.
 *
 *     Revision 1.6  2001/03/27 21:15:52  cjbryan
 *     cleaned up an error from the other day
 *
 *     Revision 1.5  2001/03/21 23:14:08  cjbryan
 *     integrated CVO subnet trigger info into triglist msg
 *
 *     Revision 1.4  2001/03/21 18:27:31  cjbryan
 *     incorporated CVO changes to carry subnet info through to
 *     output files
 *
 *     Revision 1.3  2000/11/01 22:33:08  dietz
 *     Removed leading zeros from format of EVENT ID in triglist msg
 *     at Nanometrics' request (now written as %lu, was %06lu)
 *
 *     Revision 1.2  2000/08/08 18:33:24  lucky
 *     Lint cleanup
 *
 *     Revision 1.1  2000/02/14 16:14:42  lucky
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
/*	Inputs:		Pointer to the CarlSubTrig Network		*/
/*									*/
/*	Outputs:	Message sent to the output ring			*/
/*									*/
/*	Returns:	0 on success					*/
/*			Error code as defined in carlsubtrig.h on 	*/
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
/*	CarlSubTrig Includes						*/
/*******							*********/
#include "carlsubtrig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/
int DoubleCmp( const void *, const void * );



/*	Function: ProduceTrigger					*/
int ProduceTriggerMessage( NETWORK* csuNet )
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
  STATION*      station = csuNet->stations;
  SUBNET*       subnet = csuNet->subnets;

  /* Added by Eugene Lublinsky, 3/31/Y2K */
  int           singleSubnetNumber, namedFlag = 1;
  char          subnetName[MAX_SUBNET_LEN];
  
  /*	Convert network onTime to a tm structure			*/
  ttTimeOn = (time_t) csuNet->onTime;
  gmtime_ew( &ttTimeOn, &tmTimeOn );

  /* Loop through all the subnets; count total number triggered         */
  /* Changed by Eugene Lublinsky, 3/31/Y2K */
  /* Also check all subnetCode(s) for forming the SUBNET field */
  
  for (isub = 0; isub < csuNet->nSub; isub++ ) {
      if (isdigit(subnet[isub].subnetCode[0])) namedFlag = 0; /* old style */
      if ( subnet[isub].Triggered ) {
	  ntrigsub++;
	  singleSubnetNumber = isub; 
      }      
  }

  /* added by T.Murray 4/1/00 if less than "all" subnets, use the name of */
  /* the last subnet in the list.  This means that order counts in the subnet */
  /* listing in the .d file */
  if ( ntrigsub < csuNet->numSubAll ) {
      strncpy(subnetName, subnet[singleSubnetNumber].subnetCode, MAX_SUBNET_LEN);
      if (strlen(subnetName) == MAX_SUBNET_LEN)
	  subnetName[MAX_SUBNET_LEN - 1] = '\0';
      
  }
  else strcpy(subnetName, "Regional_");
  
  /* End of changed code */
  
  if (csuNet->csuParam.debug )
      logit("t","Total of %d subnets triggered (%d coincidentally)\n", 
	    ntrigsub, csuNet->numSub );

  
  /* Maybe add some extra stations */
  if ( csuNet->listSubnets  &&  ntrigsub < csuNet->numSubAll )
      AddExtTrig( csuNet );
  
  
  if (csuNet->csuParam.debug > 1 ) {
      for (isub = 0; isub < csuNet->nSub; isub++ ) {
	  if ( subnet[isub].Triggered ) {
	      logit("", "   Subnet %s:", subnet[isub].subnetCode );
	      for ( jsta = 0; jsta < subnet[isub].nStas; jsta++ ) { 
		  /*if ( station[subnet[isub].stations[jsta]].listMe == 1 )*/
		  if ( station[subnet[isub].stations[jsta]].listMe % 2 == 1 ) {  
		      /* This station actually triggered */
		      logit("", " %s.%s.%s.%s",
			    station[subnet[isub].stations[jsta]].staCode,
			    station[subnet[isub].stations[jsta]].compCode,
			    station[subnet[isub].stations[jsta]].netCode,
			    station[subnet[isub].stations[jsta]].locCode);
		  }
	      }
	      logit("", "\n");
	  }
      }
  }
  /* WARNING, this is an experimental feature to be sure to eliminate telem glitches */
  if (csuNet->coincident_stas > 0 && ntrigsub <= csuNet->ignore_coincident) {
      double        ontimes[5000];
      int total_trig_count=0;
      double last_time;
      int coincident_count=0;
      for (isub = 0; isub < csuNet->nSub; isub++ ) {
	  if ( subnet[isub].Triggered ) {
	      for ( jsta = 0; jsta < subnet[isub].nStas; jsta++ ) { 
		  /*if ( station[subnet[isub].stations[jsta]].listMe == 1 )*/
		  if ( station[subnet[isub].stations[jsta]].listMe % 2 == 1 ) {  
		      /* This station actually triggered */
                      if (station[subnet[isub].stations[jsta]].coincident_check != 1)
                      {
                         ontimes[total_trig_count++] = station[subnet[isub].stations[jsta]].saveOnTime;
                         if (csuNet->csuParam.debug > 2)
                            logit("et", "DEBUG: Station %s trigger on at %f\n", 
		        	station[subnet[isub].stations[jsta]].staCode, station[subnet[isub].stations[jsta]].saveOnTime);
                         station[subnet[isub].stations[jsta]].coincident_check=1;
		      }
                      else
                      {
                         if (csuNet->csuParam.debug > 2)
                            logit("et", "DEBUG: Station %s already observed for coincident check, skipping another occurrence\n", 
		        	station[subnet[isub].stations[jsta]].staCode);
                      }
		  }
	      }
	  }
      }
      /* sort the times */
      qsort(ontimes,  total_trig_count, sizeof(double),  DoubleCmp);
      /* do a second pass to see if any have coincident time triggers that match exactly!!! (no tolerance since
       carlstatrig only measures times to the second.
      */
      last_time = ontimes[0];
      if (csuNet->csuParam.debug > 2)
          logit("et", "DEBUG: ontime check %d trigger on at %f\n", 0, ontimes[0]); 
      for ( jsta = 1; jsta < total_trig_count; jsta++ )
      {
                if (csuNet->csuParam.debug > 2)
                    logit("et", "DEBUG: ontime check %d trigger on at %f: coincident count before=%d ", jsta, ontimes[jsta], coincident_count); 
                if (last_time == ontimes[jsta])
                {
                    coincident_count++;
                    if (coincident_count==1) coincident_count++; /* increase it by 1 for first one to indicate 2 stations match */
                    if (csuNet->csuParam.debug > 2)
                        logit("et", "after=%d\n", coincident_count); 
                    if (coincident_count == csuNet->coincident_stas)
                    {
                        if ( csuNet->csuParam.debug > 2 )
                          logit("et", "DEBUG: %d coincident triggers at time %f across ALL SUBNETS triggered, rejecting message\n",
                                coincident_count, last_time);
                        break;   /* break out since we found N (or more coincident triggers) - possible GLITCH */
                    }
                }
                else
                {
                    if (csuNet->csuParam.debug > 2)
                        logit("et", "no match\n"); 
                    last_time = ontimes[jsta];
                    coincident_count = 0;	/* reset this */
                }
      }
      if (coincident_count == csuNet->coincident_stas)
      {
         return(0); /* SUCCESS, no message sent return value */
      }
  }
  
  if (csuNet->coincident_stas > 0 && 
      ntrigsub > csuNet->ignore_coincident && 
      csuNet->csuParam.debug) {
         logit("et", "IgnoreCoincident Check set at %d, too many subnets (%d) triggered\n", csuNet->ignore_coincident, ntrigsub); 
  }
  /* Maybe add some extra stations */
  if ( csuNet->listSubnets  &&  ntrigsub < csuNet->numSubAll )
      AddExtTrig( csuNet );
  
  /* Initialize the beginning of the outgoing message		*/
  /* Year 2000 compliance Y2K					*/
  /* Changed by Eugene Lublinsky, 3/31/Y2K */
  /* There are two possible contents for the Author field now  */
  /* further changed by Carol Bryan 3/21/01 - author field goes
  ** back to its designed use and the subnet is carried at the 
  ** end of the first line of the TrigList msg                  */
  if (namedFlag) 
      sprintf( csuNet->trigMsgBuf,
	       "%s EVENT DETECTED     %4d%02d%02d %02d:%02d:%02d.00 UTC "
	       "EVENT ID: %lu AUTHOR: %03u%03u%03u SUBNET: %-s\n\n",
	       CSU_VERSION, 
	       tmTimeOn.tm_year + 1900, tmTimeOn.tm_mon + 1,
	       tmTimeOn.tm_mday, tmTimeOn.tm_hour, tmTimeOn.tm_min,
	       tmTimeOn.tm_sec, csuNet->eventID,
	       csuNet->csuEwh.typeTrigList, csuNet->csuEwh.myModId,
	       csuNet->csuEwh.myInstId, subnetName);
  else
      sprintf( csuNet->trigMsgBuf, 
	       "%s EVENT DETECTED     %4d%02d%02d %02d:%02d:%02d.00 UTC "
	       "EVENT ID: %lu AUTHOR: %03u%03u%03u\n\n",
	       CSU_VERSION,
	       tmTimeOn.tm_year + 1900, tmTimeOn.tm_mon + 1,
	       tmTimeOn.tm_mday, tmTimeOn.tm_hour, tmTimeOn.tm_min, 
	       tmTimeOn.tm_sec, csuNet->eventID,
	       csuNet->csuEwh.typeTrigList, csuNet->csuEwh.myModId,
	       csuNet->csuEwh.myInstId );
  logit( "o", "%s", csuNet->trigMsgBuf );
  strcat( csuNet->trigMsgBuf, "Sta/Cmp/Net/Loc   Date   Time                       "
	  "start save       duration in sec.\n" );
  logit( "o", "Sta/Cmp/Net/loc   Date   Time                       "
	 "start save       duration in sec.\n" );
  strcat( csuNet->trigMsgBuf, "---------------   ------ --------------- "
	  "   -----------------------------------------\n" );
  logit( "o", "---------------   ------ --------------- "
	 "   -----------------------------------------\n" );
      
  /* Format the time for saving data					*/
  ttTimeOn = csuNet->onTime - csuNet->PreEventTime;
  gmtime_ew( &ttTimeOn, &tmTimeOn );
      
  /* Add a wildcard if this looks like a big event */
  if ( ntrigsub >= csuNet->numSubAll ) {
      sprintf( staLine, " * * * * P 00000000 00:00:00.00 UTC "
	       "save: %4d%02d%02d %02d:%02d:%02d.00      %ld\n",
	       tmTimeOn.tm_year + 1900, tmTimeOn.tm_mon + 1,
	       tmTimeOn.tm_mday, tmTimeOn.tm_hour, tmTimeOn.tm_min,
	       tmTimeOn.tm_sec, csuNet->duration + csuNet->PreEventTime );
	  
      /*	Check for buffer overflow				*/
      if ( ( strlen( csuNet->trigMsgBuf ) + strlen( staLine ) ) >= 
	   csuNet->trigMsgBufLen ) {
	  overflow = 1;
	  retVal   =  ERR_SHORT_MSG;
      }
	  
      /*	Append the station information to the output message	*/
      if( !overflow ) strcat( csuNet->trigMsgBuf, staLine );
      logit( "o", "%s", staLine );
  }

  /* Step through each station					*/
  for ( i = 0; i < csuNet->nSta; i++ ) {
      if ( csuNet->stations[i].listMe ) {
	  if (csuNet->stations[i].saveOnTime > 0.0) {
	      ttStaOn = ( time_t ) csuNet->stations[i].saveOnTime;
	      gmtime_ew( &ttStaOn, &tmStaOn );
	      sprintf(trigtime, "%4d%02d%02d %02d:%02d:%02d.%02d UTC ",
		      tmStaOn.tm_year + 1900,
		       tmStaOn.tm_mon + 1, tmStaOn.tm_mday, tmStaOn.tm_hour,
		       tmStaOn.tm_min, tmStaOn.tm_sec,
		       (int) ( ( csuNet->stations[i].saveOnTime - ttStaOn ) 
			       * 100 + 0.5 ));
	  }
	  else {
	      sprintf(trigtime, "%s", "00000000 00:00:00.00 UTC");
	  }
	      
	  /* Add the station specific information into the output message */
	  if ( csuNet->compAsWild ) {
	      sprintf( staLine, " %s * %s %s P %s save: %4d%02d%02d %02d:%02d:%02d.00      %ld\n",
		       csuNet->stations[i].staCode, 
		       csuNet->stations[i].netCode, 
		       csuNet->stations[i].locCode, trigtime,
		       tmTimeOn.tm_year + 1900, tmTimeOn.tm_mon + 1,
		       tmTimeOn.tm_mday, tmTimeOn.tm_hour, tmTimeOn.tm_min,
		       tmTimeOn.tm_sec, csuNet->duration + csuNet->PreEventTime );
	  } else {
	      sprintf( staLine, " %s %s %s %s P %s save: %4d%02d%02d %02d:%02d:%02d.00      %ld\n",
		       csuNet->stations[i].staCode, csuNet->stations[i].compCode,
		       csuNet->stations[i].netCode, csuNet->stations[i].locCode,
		       trigtime,
		       tmTimeOn.tm_year + 1900, tmTimeOn.tm_mon + 1,
		       tmTimeOn.tm_mday, tmTimeOn.tm_hour, tmTimeOn.tm_min,
		       tmTimeOn.tm_sec, csuNet->duration + csuNet->PreEventTime );
	  }
	      
	  /* Show that this station has been reported */
	  csuNet->stations[i].listMe = 0;
	  csuNet->stations[i].coincident_check = 0;
	      
	  /* Check for buffer overflow				*/
	  if ( ( strlen( csuNet->trigMsgBuf ) + strlen( staLine ) ) >= 
	       csuNet->trigMsgBufLen ) {
	      overflow = 1;
	      retVal   =  ERR_SHORT_MSG;
	  }
	      
	  /* Append the station information to the output message	*/
	  if( !overflow ) strcat( csuNet->trigMsgBuf, staLine );
	  logit( "o", "%s", staLine );
      }
  }
      
  /* Now list the non-seismic channels if any */
  for ( i = 0; i < csuNet->nChan; i++ ) {
      sprintf( staLine, " %s %s %s %s P 00000000 00:00:00.00 UTC "
	       "save: %4d%02d%02d %02d:%02d:%02d.00      %ld\n",
	       csuNet->channels[i].staCode, csuNet->channels[i].compCode,
	       csuNet->channels[i].netCode, csuNet->channels[i].locCode,
	       tmTimeOn.tm_year + 1900, tmTimeOn.tm_mon + 1,
	       tmTimeOn.tm_mday, tmTimeOn.tm_hour, tmTimeOn.tm_min,
	       tmTimeOn.tm_sec, csuNet->duration + csuNet->PreEventTime );
	  
      /*	Check for buffer overflow				*/
      if ( ( strlen( csuNet->trigMsgBuf ) + strlen( staLine ) ) >= 
	   csuNet->trigMsgBufLen ) {
	  overflow = 1;
	  retVal   = ERR_SHORT_MSG;
      }
	  
      /*	Append the station information to the output message	*/
      if( !overflow ) strcat( csuNet->trigMsgBuf, staLine );
      logit( "o", "%s", staLine );
  }
      
  /* a blank line at the end for Lynn */
  logit( "o", "\n");
      
  /* Send the output message, even if incomplete, to the output ring */
  if ( tport_putmsg( &(csuNet->regionOut), &(csuNet->trgLogo), 
		     strlen( csuNet->trigMsgBuf ), csuNet->trigMsgBuf )
       != PUT_OK ) {
      logit( "et", "carlsubtrig: Error sending a trigger message to ring.\n" );
      retVal = ERR_PROD_MSG;
  }
      
  /* Send an error to statmgr if there wasn't room for the whole output msg */
  if( overflow ) {
      sprintf( staLine, "incomplete triglist msg sent for eventid:%ld",
	       csuNet->eventID );
      StatusReport( csuNet, csuNet->csuEwh.typeError, ERR_SHORT_MSG, staLine );
  }
  
  
  // RSL: If this is the first message, do not update trigger  
  if ( csuNet->early_warning == 2 )
  {
    csuNet->early_warning = 1;
    return ( retVal );
  }
  
  /* Update the eventID and its file */
  csuNet->eventID++;
  if (csuNet->csuParam.trigIdFilename != NULL)
  {
     idfp = fopen( csuNet->csuParam.trigIdFilename, "w" );
  }
  else
  {
     idfp = fopen( "trig_id.d", "w" );
  }
  if ( idfp != (FILE *) NULL ) {
      fprintf( idfp,
	       "# Next available trigger sequence number:\n" );
      fprintf( idfp, "next_id %ld\n", csuNet->eventID );
      fclose( idfp );
  }
      
  return(retVal);
  
}
