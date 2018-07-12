/***************************************************************************
 *  This code is a part of rayloc_ew / USGS EarthWorm module               *
 *                                                                         *
 *  It is written by ISTI (Instrumental Software Technologies, Inc.)       *
 *          as a part of a contract with CERI USGS.                        *
 * For support contact info@isti.com                                       *
 *   Ilya Dricker (i.dricker@isti.com)                                     *
 *                                                   Aug 2004              *
 **************************************************************************/

/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: rayloc_unused_phases.c 1669 2004-08-05 04:15:11Z friberg $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2004/08/05 04:15:11  friberg
 *     First commit of rayloc_ew in EW-CENTRAL CVS
 *
 *     Revision 1.2  2004/08/04 19:27:54  ilya
 *     Towards version 1.0
 *
 *     Revision 1.1.1.1  2004/06/22 21:12:06  ilya
 *     initial import into CVS
 *
 */

/***************************************************************************
                          rayloc_unused_phases.c  -  description
                             -------------------
    begin                : Thu Jun 3 2004
    copyright            : (C) 2004 by Ilya Dricker, ISTI
    email                : i.dricker@isti.com
 ***************************************************************************/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <stdlib.h>
#include <string.h>
#include "rayloc1.h"
 


RAYLOC_PHASES *
	rayloc_new_phase_list(void)
	{
		
		RAYLOC_PHASES *new_list = NULL;
		new_list = (RAYLOC_PHASES *)calloc(1, sizeof(RAYLOC_PHASES));
		new_list->max_num = sizeof(new_list->phases)/(RAYLOC_MAX_PHASE_NAME_LEN + 1);
		
		return new_list;
	}

int
	rayloc_add_phase(RAYLOC_PHASES *phase_list, const char *phase)
	{
		if (strlen(phase) > (RAYLOC_MAX_PHASE_NAME_LEN -1))
			return RAYLOC_FAILED;
		if (phase_list->num >= phase_list->max_num)
			return RAYLOC_FAILED;
		strncpy(phase_list->phases[phase_list->num], phase, strlen(phase));
		(phase_list->num)++;
		return RAYLOC_SUCCESS;
	}

void
	rayloc_destroy_phase_list(RAYLOC_PHASES *phase_list)
	{
		if (phase_list)
			free(phase_list);
		return;
	}

int
	rayloc_if_in_the_phase_list(RAYLOC_PHASES *phase_list, const char *phase)
	{
		int i;
		for (i = 0; i < phase_list->num; i++)
		{
		if (0 == strncmp(phase_list->phases[i], phase, strlen(phase)))
			return RAYLOC_TRUE;
		}
		return RAYLOC_FALSE;
	}

 
