/*
 * initvar.c
 * Modified from initvar.c, Revision 1.1 in src/seismic_processing/pick_ew
 * 
 * This file is part of pick_FP, Earthworm module for the FilterPicker phase detector and picker.
 * 
 * (C) 2008-2012 Claudio Satriano <satriano@ipgp.fr>,
 * under the same license terms of the Earthworm software system. 
 */

#include <earthworm.h>
#include <transport.h>
#include "pick_FP.h"


   /*******************************************************************
    *                            InitVar()                            *
    *                                                                 *
    *       Initialize STATION variables to 0 for one station.        *
    *******************************************************************/

void InitVar( STATION *Sta )
{
	int i;
	//TODO: can we remove these two pointers?
	PICK *Pick = &Sta->Pick;         /* Pointer to pick structure */
	CODA *Coda = &Sta->Coda;         /* Pointer to coda structure */

	Sta->enddata    = 0L; /* Sample at end of previous message */
	Sta->endtime    = 0.; /* Time at end of previous message */
	Sta->first      = 1;  /* No messages with this channel have been detected */
	Sta->ns_restart = 0;  /* Restart sample count */

	Sta->phase_number = 0;

        if (Sta->mem != NULL) {
               free_FilterPicker5_Memory(&(Sta->mem));
               Sta->mem = NULL;
        }
	/* Pick variables*/
	Pick->time = 0.;         /* Pick time */

	for ( i = 0; i < 3; i++ )
		Pick->xpk[i] = 0.;    /* Absolute value of extrema after ipic */

	Pick->FirstMotion = '?'; /* First motion  ?=Not determined  U=Up  D=Down */
				 /* u=Questionably up  d=Questionably down */
	Pick->weight = 0;        /* Pick weight (0-3) */
	Pick->status = 0;

	/* Coda variables */
	for ( i = 0; i < 6; i++ )
		Coda->aav[i] = 0;     /* Average absolute value of preferred windows */

	Coda->len_sec = 0;       /* Coda length in seconds */
	Coda->len_out = 0;       /* Coda length in seconds (possibly * -1) */
	Coda->len_win = 0;       /* Coda length in number of windows */
	Coda->status  = 0;
}
