/*
 * report.c
 * Modified from report.c, Revision 1.6 in src/seismic_processing/pick_ew
 *
 * This file is part of pick_FP, Earthworm module for the FilterPicker phase detector and picker.
 *
 * (C) 2008-2012 - Claudio Satriano <satriano@ipgp.fr>
 *          with contributions from Luca Elia <elia@fisica.unina.it>,
 * under the same license terms of the Earthworm software system. 
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <earthworm.h>
#include <trace_buf.h>
#include <chron3.h>
#include <transport.h>

#include "pick_FP.h"

int GetPickIndex (unsigned char modid, char *dir);  /* function in index.c */

extern int offline_mode;

static char line[LINELEN];

void reportPick (PickData* pick_list, TRACE2_HEADER* TraceHead, STATION* Sta, GPARM* Gparm, EWH* Ewh)
{
	MSG_LOGO logo;      /* Logo of message to send to output ring */

	double secs;
	struct Greg gtime;

	double uncertainty;

	double delta;

	int lineLen;

	int tsec, thun;
	int PickIndex;
	char first_motion;
	int weight;


	PickIndex = GetPickIndex (Gparm->MyModId, Gparm->PickIndexDir);

	delta = 1.0/TraceHead->samprate;

	secs = (double) TraceHead->starttime + GSEC1970;
	secs += delta * (pick_list->indices[0] + pick_list->indices[1]) / 2.0;
	datime(secs, &gtime);
	tsec = (int)floor( (double) gtime.second );
	thun = (int)((100.*(gtime.second - tsec)) + 0.5);
	if ( thun == 100 )
		tsec++, thun = 0;

	// pick quality
        // set uncertainty to half width between right and left indices
	uncertainty = delta * fabs(pick_list->indices[1] - pick_list->indices[0]) / 2.0;
	for (weight=0; weight<=3; weight++)
		if(uncertainty <= Gparm->WeightTable[weight]) break;


	// first motion
	first_motion = '?';
	if (pick_list->polarity == POLARITY_POS)
		first_motion = 'U';
	if (pick_list->polarity == POLARITY_NEG)
		first_motion = 'D';


	/* 
	 * Convert pick to space-delimited text string.
	 * This is a bit risky, since the buffer could overflow.
	 * Milliseconds are always set to zero.
	 */
	sprintf (line,              "%d",  (int) Ewh->TypePickScnl);
	sprintf (line+strlen(line), " %d", (int) Gparm->MyModId);
	sprintf (line+strlen(line), " %d", (int) Ewh->MyInstId);
	sprintf (line+strlen(line), " %d", PickIndex);
	sprintf (line+strlen(line), " %s", Sta->sta);
	sprintf (line+strlen(line), ".%s", Sta->chan);
	sprintf (line+strlen(line), ".%s", Sta->net);
	sprintf (line+strlen(line), ".%s", Sta->loc);

	sprintf (line+strlen(line), " %c%d", first_motion, weight);

	sprintf (line+strlen(line), " %4d%02d%02d%02d%02d%02d.%02d0",
		 gtime.year, gtime.month, gtime.day, gtime.hour,
		 gtime.minute, tsec, thun);

	sprintf (line+strlen(line), " %d", (int)(pick_list->amplitude + 0.5));
	sprintf (line+strlen(line), " %d", 0);
	sprintf (line+strlen(line), " %d", 0);
	strcat (line, "\n");
	lineLen = strlen(line);

	if (offline_mode) {
		/* Print the pick */
		//#if defined(_OS2) || defined(_WINNT)
		printf( "%s", line );
		//#endif
	} else {
		/* Send the pick to the output ring */
		logo.type   = Ewh->TypePickScnl;
		logo.mod    = Gparm->MyModId;
		logo.instid = Ewh->MyInstId;

		if ( tport_putmsg( &Gparm->OutRegion, &logo, lineLen, line ) != PUT_OK )
			logit( "et", "pick_FP: Error sending pick to output ring.\n" );
		else
			;
			//logit( "t", "PICK %s\n", line); //luca
	}

	return;
}
