
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *     Revision 1.1  2010/01/01 00:00:00  jmsaurel
 *     Initial revision
 *
 *
 */

  /********************************************************************
   *                    hypo2000_triglist_utilities                   *
   *                                                                  *
   *                                                                  *
   ********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <earthworm.h>
#include <trace_buf.h>
#include <chron3.h>
#include <math.h>
#include "statrig.h"

/* Define some simple "functions":
   *******************************/
#ifndef MAX
#define MAX(a,b) (((a)>(b)) ? (a) : (b))        /* Larger value   */
#endif
#ifndef MIN
#define MIN(a,b) (((a)<(b)) ? (a) : (b))        /* Smaller value  */
#endif
#define ABS(a) ((a) > 0 ? (a) : -(a))           /* Absolute value */

#define MAX_BYTES_PER_PUN 10000

/* externals */
extern float  UseLatitude;
extern float  UseLongitude;
extern float  UseDepth;

/* Functions in this source file:
 ********************************/

int	read_statrig_line( char *, STATRIG*);	/* reads a station line from triglist message and fills STATRIG structure */
void	write_hyp_staline( STATRIG *, char **);	/* writes a hyp2000 phase line and its shadow with infos from STATRIG structure */
void	write_hyp_header( STATRIG *, char **);	/* writes a hyp2000 header line and its shadow with infos from STATRIG structure */
void	write_hyp_term( STATRIG *, char **);		/* writes a hyp2000 terminator line and its shadow */

/****************************************************************************/
/*	read_statrig_line() reads a station line from triglist message	    */
/*		fills STATRIG structure with informations		    */
/*		returns 1 if station has triggered			    */
/*		returns 0 if station has not triggered			    */
/****************************************************************************/

int read_statrig_line(char *line, STATRIG* statrig)
{
	static	char terminators[] = " \t\n"; /* we accept space, newline, and tab as terminators */
	char*	nxttok;
	char	trigger_date[18];
	int	i;

	/* Initialize iteration over tokens *
	 ************************************/
	if ( ( nxttok=strtok(line, terminators) ) == NULL ) { /* first token should be station name */
		logit("et","read_statrig_line: Bad syntax in trigger message:"
			"Strange station line - no tokens in: \n.&s.\n",line);
		return(0);
	}
	/* Find SCN(L) names
	 *******************/
	if (nxttok ==NULL) { /* oops - should have been the station name */
		logit("et", "read_statrig_line: Bad syntax in trigger message."
			" Cant find station name in:\n.%s.\n", line);
		return(0);
	}
	strncpy( statrig->sta, nxttok, TRACE2_STA_LEN); 
	statrig->sta[TRACE2_STA_LEN-1] = '\0';  /* Ensure null termination */
   
	nxttok = strtok( (char*)NULL, terminators); /* should be the component */
	if (nxttok ==NULL) { /* oops - there was nothing after station name */
		logit("et", "read_statrig_line: Bad syntax in trigger message."
			" Cant find comp name in:\n.%s.\n", line);
		return(0);
	}
	if(!strcmp(nxttok,"*"))
		strcpy( statrig->chan, "   ");
	else
		strncpy( statrig->chan, nxttok, TRACE2_CHAN_LEN );
	statrig->chan[TRACE2_CHAN_LEN-1] = '\0'; /* Ensure null termination */
    
	nxttok = strtok( (char*)NULL, terminators); /* should be the net */
	if (nxttok ==NULL) { /* oops - there was nothing after component name */
		logit("et", "read_statrig_line: Bad syntax in trigger message."
			" Cant find net name in:\n.%s.\n", line);
		return(0);
	}
	strncpy( statrig->net, nxttok, TRACE2_NET_LEN );
	statrig->net[TRACE2_NET_LEN-1] = '\0';   
   
	nxttok = strtok( (char*)NULL, terminators); /* should be the net */
	if (nxttok ==NULL) { /* oops - there was nothing after network name */
		logit("et", "read_statrig_line: Bad syntax in trigger message."
			" Cant find loc name in:\n.%s.\n", line);
		return(0);
	}
	strncpy( statrig->loc, nxttok, TRACE2_LOC_LEN );
	statrig->loc[TRACE2_LOC_LEN-1] = '\0';   
	nxttok = strtok( (char*)NULL, terminators); /* should be the phase type, don't mind */
	/* Find trigger time
	 *******************/
	nxttok = strtok( (char*)NULL, terminators); /* should be the save start date */
	if (nxttok ==NULL) { /* oops - there was nothing after save: */
		logit("et", "read_statrig_line: Bad syntax in trigger message."
			" Cant find save date in:\n.%s.\n", line);
		return(0);
	}
	strcpy( trigger_date, nxttok ); 	/* put away the date string */
    
	nxttok = strtok( (char*)NULL, terminators); /* sould be the save start time-of-day */
	if (nxttok ==NULL) { /* oops - there was nothing after save: */
		logit("et", "read_statrig_line: Bad syntax in trigger message."
			" Cant find save time of day in:\n.%s.\n", line);
	return(0);
	}
	strncpy( trigger_date+8, nxttok, 2 ); 	/* put away the hour string */
	strncpy( trigger_date+10, nxttok+3, 2 ); 	/* put away the minute string */
	strncpy( trigger_date+12, nxttok+6, 5 ); 	/* put away the fractionnal second string */
	trigger_date[17] = '\0';		/* ensure NULL termination */
	statrig->Sta_time = julsec17(trigger_date);	/* Convert trigger time to julian date */
	if(statrig->Sta_time < 0)
		return(0);
	/* Find duration to save
	 ***********************/
	for( i=0;i<5;i++ ) {			/* there should be 5 field left, last being duration */
		nxttok = strtok( (char*)NULL, terminators); /* last one should be the duration */
		if (nxttok ==NULL) { /* oops - there was nothing */
			logit("et", "read_statrig_line: Bad syntax in trigger message."
				" Cant find duration time in:\n.%s.\n", line);
		statrig->duration = 0;
		return(1);
		}
	}
	statrig->duration = atol( nxttok );
	if ( statrig->duration <= 0 ) {
		logit("et", "read_statrig_line: Bad syntax in trigger message."
			" Bad duration value in:\n.%s.\n", line);
		statrig->duration = 0;
	}
	return(1);
}



/****************************************************************************/
/*	write_hyp_staline() writes a hyp2000arc station line		    */
/*		from STATRIG structure and its shadow			    */
/****************************************************************************/

void write_hyp_staline( STATRIG *statrig, char **outmsg_ptr)
{
	char	trigger_date[18];

	date17(statrig->Sta_time,trigger_date);
	sprintf(*outmsg_ptr,"%-5.5s%-2.2s  %-3.3s  P 4%12.12s %2.2s%2.2s",
		statrig->sta,statrig->net,statrig->chan,trigger_date,trigger_date+12,trigger_date+15);
	*outmsg_ptr=*outmsg_ptr+34;
	sprintf(*outmsg_ptr,"                                                     ");
	*outmsg_ptr=*outmsg_ptr+53;
	sprintf(*outmsg_ptr,"%4d                    %2.2s\n$\n",statrig->duration,statrig->loc);
	*outmsg_ptr=*outmsg_ptr+29;
	strcpy(*outmsg_ptr,"\0");
}

/****************************************************************************/
/*	write_hyp_header() writes a hyp2000arc header line		    */
/*		from STATRIG structure and its shadow			    */
/****************************************************************************/

void write_hyp_header( STATRIG *statrig, char **outmsg_ptr)
{
	float	deg, min;
	char	sign;
	char	trigger_date[18];
	int	i;

	date17(statrig->Sta_time,trigger_date);
	sprintf(*outmsg_ptr,"%14.14s%2.2s",trigger_date,trigger_date+15);
	*outmsg_ptr = *outmsg_ptr + 16;

	deg = floor( fabs(UseLatitude) );
	min = ( fabs(UseLatitude) - deg ) * 60 * 100;
	if (UseLatitude < 0)
		sign = 'S';
	else
		sign = ' ';
	sprintf(*outmsg_ptr,"%-2.0f%c%4.0f",deg,sign,min);
	*outmsg_ptr = *outmsg_ptr + 7;

	deg = floor( fabs(UseLongitude) );
	min = ( fabs(UseLongitude) - deg ) * 60 * 100;
	if (UseLongitude > 0)
		sign = 'E';
        else
		sign = ' ';
	sprintf(*outmsg_ptr,"%3.0f%c%4.0f",deg,sign,min);
        *outmsg_ptr = *outmsg_ptr + 8;

        sprintf(*outmsg_ptr,"%5.0f",UseDepth*100);
        *outmsg_ptr = *outmsg_ptr + 5;

	for (i=0; i<100; i++)
		sprintf(*outmsg_ptr+i," ");
	*outmsg_ptr = *outmsg_ptr + i;
	sprintf(*outmsg_ptr,"%10d                2 \n$1\n",statrig->Id);
	*outmsg_ptr = *outmsg_ptr + 32;
	strcpy(*outmsg_ptr,"\0");
}

/****************************************************************************/
/*	write_hyp_term() writes a hyp2000arc terminator line		    */
/*		 and its shadow						    */
/****************************************************************************/

void write_hyp_term( STATRIG *statrig, char **outmsg_ptr)
{
	int	i;

	for (i=0; i<62; i++)
		sprintf(*outmsg_ptr+i," ");
	*outmsg_ptr = *outmsg_ptr + i;
	sprintf(*outmsg_ptr,"%10d\n$",statrig->Id);
	*outmsg_ptr = *outmsg_ptr + 12;

	for (i=0; i<61; i++)
		sprintf(*outmsg_ptr+i," ");
	*outmsg_ptr = *outmsg_ptr + i;
	sprintf(*outmsg_ptr,"%10d\n",statrig->Id);
	*outmsg_ptr = *outmsg_ptr + 11;

	strcpy(*outmsg_ptr,"\0");
}
