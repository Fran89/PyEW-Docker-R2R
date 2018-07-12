/*
 * call_FilterPicker.c
 * Bridge routine between Earthworm and FilterPicker
 *
 * This file is part of pick_FP, Earthworm module for the FilterPicker phase detector and picker.
 *
 * Copyright (C) 2008-2012 Claudio Satriano <satriano@ipgp.fr> and Anthony Lomax <anthony@alomax.net>
 *                         with the contribution of Luca Elia <elia@fisica.unina.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

//Earthworm stuff
#include <earthworm.h>
#include <trace_buf.h>
#include <chron3.h>
#include <transport.h>

#include "pick_FP.h"

#define MAX_TRACEBUF_SAMPLES 2016

static float sample[MAX_TRACEBUF_SAMPLES];


void reportPick (PickData* pick_list, TRACE2_HEADER* TraceHead, STATION* Sta, GPARM* Gparm, EWH* Ewh);

extern int offline_mode;

void call_FilterPicker (STATION *Sta, char *TraceBuf, GPARM *Gparm, EWH *Ewh)
{
	TRACE2_HEADER *TraceHead = (TRACE2_HEADER *) TraceBuf;
   	int32_t *TraceSample = (int32_t *) (TraceBuf + sizeof(TRACE2_HEADER));

	// useMemory: set to TRUE_INT=1 if function is called for packets of data in sequence, FALSE_INT = 0 otherwise
	BOOLEAN_INT useMemory = TRUE_INT; 

	int i, n;

	int length; //packet size

	// picker parameters 
	// SEE: _DOC_ in FilterPicker.c for more details on the picker parameters
	// defaults
	// filp_test filtw 4.0 ltw 10.0 thres1 8.0 thres2 8.0 tupevt 0.2 res PICKS...
	double filterWindow = Sta->Parm.filterWindow;	// NOTE: auto set below
	double longTermWindow = Sta->Parm.longTermWindow;// NOTE: auto set below
	double threshold1 = Sta->Parm.threshold1;
	double threshold2 = Sta->Parm.threshold2;
	double tUpEvent = Sta->Parm.tUpEvent;	// NOTE: auto set below

	// integer version of parameters
	long iFilterWindow;
	long ilongTermWindow;
	long itUpEvent;

	double delta; //sampling step
	double dt; //sampling step (parameter)

	PickData** pick_list = NULL; //pick_list is allocated by the Pick() function
	int num_picks = 0;

	char channel_id[20];

#ifdef DEBUG
	printf( "pinno: %d\n nsamp: %d\n starttime: %lf\n endtime: %lf\n samprate: %lf\n sta: %s\n net: %s\n chan: %s\n loc: %s\n version: %c%c\n datatype: %s\n quality: %c%c\npad: %c%c\n",
        TraceHead->pinno,                 /* Pin number */
        TraceHead->nsamp,                 /* Number of samples in packet */
        TraceHead->starttime,             /* time of first sample in epoch seconds
                                          (seconds since midnight 1/1/1970) */
        TraceHead->endtime,               /* Time of last sample in epoch seconds */
        TraceHead->samprate,              /* Sample rate; nominal */
        TraceHead->sta,   		  /* Site name (NULL-terminated) */
        TraceHead->net,		          /* Network name (NULL-terminated) */
        TraceHead->chan, 		  /* Component/channel code (NULL-terminated)*/
        TraceHead->loc,			  /* Location code (NULL-terminated) */
        TraceHead->version[0], TraceHead->version[1],            /* version field */
        TraceHead->datatype,	           /* Data format code (NULL-terminated) */
        TraceHead->quality[0], TraceHead->quality[1],           /* Data-quality field */
        TraceHead->pad[0], TraceHead->pad[1]			  /* padding */
);
#endif



	// allocate array for data
	length = (int) TraceHead->nsamp;
	for (i=0; i<=length; i++)
		sample[i] = (float) TraceSample[i];


	//
        // auto set values
        // get dt
	delta = 1.0/TraceHead->samprate;
	dt = delta;
	dt = dt < 0.02 ? 0.02 : dt;     // avoid too-small values for high sample rate data
	//
	
	if (filterWindow < 0) //autoset if negative
		filterWindow = 300.0 * dt;
	iFilterWindow = (long) (0.5 + filterWindow * 1000.0);
	if (iFilterWindow > 1)
		filterWindow = (double) iFilterWindow / 1000.0;
	//
	if (longTermWindow < 0) //autoset if negative
		longTermWindow = 500.0 * dt;  // seconds
	ilongTermWindow = (long) (0.5 + longTermWindow * 1000.0);
	if (ilongTermWindow > 1)
		longTermWindow = (double) ilongTermWindow / 1000.0;
	//
	if (tUpEvent < 0) //autoset if negative
		tUpEvent = 20.0 * dt;   // time window to take integral of charFunct version
	itUpEvent = (long) (0.5 + tUpEvent * 1000.0);
	if (itUpEvent > 1)
		tUpEvent = (double) itUpEvent / 1000.0;
	//
#ifdef DEBUG
	printf("picker_func_test: filp_test filtw %f ltw %f thres1 %f thres2 %f tupevt %f res PICKS\n",
	       filterWindow, longTermWindow, threshold1, threshold2, tUpEvent);
#endif

	sprintf(channel_id, "%s.%s.%s.%s", TraceHead->sta, TraceHead->net, TraceHead->chan, TraceHead->loc);
	Pick(
		delta,
		sample,
		length,
		filterWindow,
		longTermWindow,
		threshold1,
		threshold2,
		tUpEvent,
		&(Sta->mem),
		useMemory,
		&pick_list,
		&num_picks,
		channel_id
	);

	for (n = 0; n < num_picks; n++) {
		reportPick (*(pick_list+n), TraceHead, Sta, Gparm, Ewh);
	}

	// clean up
	free_PickList(pick_list, num_picks);	// PickData objects freed here

	return ;
}
