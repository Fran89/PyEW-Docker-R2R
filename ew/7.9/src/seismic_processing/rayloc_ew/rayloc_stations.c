/***************************************************************************
 *  This code is a part of rayloc_ew / USGS EarthWorm module               *
 *                                                                         *
 *  It is written by ISTI (Instrumental Software Technologies, Inc.)       *
 *          as a part of a contract with CERI USGS.                        *
 * For support contact info@isti.com                                       *
 *   Ilya Dricker (i.dricker@isti.com)                                     *
 *                                                   Aug 2004              *
 ***************************************************************************/

/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: rayloc_stations.c 2055 2006-01-19 19:04:55Z friberg $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2006/01/19 19:04:55  friberg
 *     rayloc upgraded to use a library version of rayloc_message_rw.c
 *
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
                          rayloc_stations.c  -  description
                             -------------------
    begin                : Fri Jun 4 2004
    copyright            : (C) 2004 by Ilya Dricker, ISTI
    email                : i.dricker@isti.com
 ***************************************************************************/

 
/* Note : this code deals with station data file in the following format
**************************************************************
BR31  39.8535N  32.7608E 1243 IM Belbasi Array Site 31, Turkey
BRNJ  40.6828N  74.5660W   50 LD Basking Ridge, New Jersey, USA
BRVK  53.0581N  70.2828E  315 II Borovoye, Kazakhstan
**************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <errno.h>
#include <locale.h>


#include "rayloc1.h"
#include <rayloc_message_rw.h> 

RAYLOC_ONE_STATION *
	rayloc_new_empty_single_station(void);

void
	rayloc_destroy_single_stations(RAYLOC_ONE_STATION *sta);


RAYLOC_STATIONS *
	rayloc_new_empty_stations(void);

int
	rayloc_add_station_to_list(RAYLOC_STATIONS *stations, const char *station_string);



RAYLOC_STATIONS *
	rayloc_stations_from_file(const char *fname, int *errNO, int *errNum)
	{
		RAYLOC_STATIONS         *stns = NULL;
		FILE                    *fd;
		char tmp[10000];
		int retVal;

		*errNum = 0;
		*errNO = RAYLOC_SUCCESS;
		
		fd = fopen(fname, "r");
		if (!fd)
		{
			*errNO = RAYLOC_FAILED_TO_OPEN_FILE;
			return NULL;
		}

		stns = rayloc_new_empty_stations();
		if (!stns)
		{
			fclose(fd);
			*errNO = RAYLOC_ALLOCATION_FAILED;
			return NULL;
		}
		while (fgets (tmp, 10000, fd))
		{
			retVal = rayloc_add_station_to_list(stns, tmp);
			if (RAYLOC_SUCCESS != retVal)
			{
				(*errNum)++;
			}
		}

		fclose(fd);
		return stns;
	}

RAYLOC_STATIONS *
	rayloc_new_empty_stations(void)
	{
		RAYLOC_STATIONS *stns = NULL;
		stns = (RAYLOC_STATIONS *) calloc(1, sizeof(RAYLOC_STATIONS));
		stns->num = 0;
		stns->max_num = sizeof(stns->sta)/sizeof(int);
		stns->use_36_base_digest = RAYLOC_TRUE;
		return stns;
	}

int
	rayloc_add_station_to_list(RAYLOC_STATIONS *stations, const char *station_string)
	{
		RAYLOC_ONE_STATION      *sta;
		char                    token[5][1000];
		double lat;
		double lon;
		int elev_int;
		double elev;
		short polarity;
		int num_token;
		char *endptr;

		/* Check if we still have a slot */
		if (stations->num >= stations->max_num)
			return RAYLOC_NO_MORE_STATION_SLOTS;

		/* String should contain at least 5 tokens */	
		num_token = rayloc_how_many_tokens(station_string);
		if (num_token < 5)
			return RAYLOC_NOT_STATION_STRING;

		sscanf(station_string, "%s %s %s %s %s", token[0], token[1], token[2], token[3], token[4]);

		/* Now we do some rudimentary tests of those tokens
		 * to make sure we deal with real station string
		 */
		/* TOKEN 0 is a station: 6 chars is a max */
		if (strlen(token[0]) > 6)
			return RAYLOC_NOT_STATION_STRING;

		/* TOKEN 1 is a latitude: 8 chars is a maximum; N or S is the last char */
		/* If TOKEN 1 is valid, convert it to signed double */
		if (strlen(token[1]) > 8)
			return RAYLOC_NOT_STATION_STRING;
		if ('S' != token[1][strlen(token[1]) - 1])
		{
			if ('N' != token[1][strlen(token[1]) - 1])
				return RAYLOC_NOT_STATION_STRING;
			else
			{
				polarity = 1;
			}
		}
		else
		{
			polarity = -1;
		}
		token[1][strlen(token[1]) - 1] = '\0';
		lat = atof(token[1]);
		if (-1 == polarity)
			lat = -lat;

		/* TOKEN 2 is a longitude: 9 chars is a maximum; E or W is the last char */
		/* If TOKEN 2 is valid, convert it to signed double */
		if (strlen(token[2]) > 9)
			return RAYLOC_NOT_STATION_STRING;
		if ('E' != token[2][strlen(token[2]) - 1])
		{
			if ('W' != token[2][strlen(token[2]) - 1])
				return RAYLOC_NOT_STATION_STRING;
			else
			{
				polarity = 1;
			}
		}
		else
		{
			polarity = -1;
		}
		token[2][strlen(token[2]) - 1] = '\0';
		lon = atof(token[2]);
		if (-1 == polarity)
			lon = -lon;
		
		/* TOKEN 3 is a elevation in meters: 4 chars max */
		/* If TOKEN 3 is valid, convert it to signed double in kilometers */
		if (strlen(token[3]) > 4)
			return RAYLOC_NOT_STATION_STRING;

		elev_int = atoi (token[3]);
		if (elev_int > 9999)
			return RAYLOC_NOT_STATION_STRING;

		elev = (double)elev_int/1000.;

		/* TOKEN 4 is a network code: 2 chars max */
		if (strlen(token[4]) > 2)
			return RAYLOC_NOT_STATION_STRING;

		/* IF WE ARE HERE OUR TESTS ARE OK */
		
		sta = rayloc_new_empty_single_station();
		if (!sta)
			return RAYLOC_ALLOCATION_FAILED;

		/* FILL sta structure */
		strncpy(sta->net, token[4], strlen(token[4]));
		strncpy(sta->sta, token[0], strlen(token[0]));
		sta->lat = lat;
		sta->lon = lon;
		sta->elev = elev;

    /* Try to build 36base digest */
		errno =0;
		sta->base36_net = strtoll(sta->net, &endptr, 36);
		if (0 == sta->base36_net)
			stations->use_36_base_digest = RAYLOC_FALSE;
		if (errno != 0)
			stations->use_36_base_digest = RAYLOC_FALSE;

		errno =0;
		sta->base36_sta = strtoll(sta->sta, &endptr, 36);
		if (0 == sta->base36_net)
			stations->use_36_base_digest = RAYLOC_FALSE;
		if (errno != 0)
			stations->use_36_base_digest = RAYLOC_FALSE;

		stations->sta[stations->num] = sta;
		(stations->num)++;

		return RAYLOC_SUCCESS;
	}

void
	rayloc_destroy_stations(RAYLOC_STATIONS *stns)
	{
		int i;
		if (!stns)
			return;

		for (i = 0; i<stns->num; i++)
			rayloc_destroy_single_stations(stns->sta[i]);

		free(stns);
		
		return;
	}

int
	rayloc_if_in_the_station_list(RAYLOC_STATIONS *stations, const char *net, const char *sta)
	{
		long long             base36_net;
		long long             base36_sta;
		int                   use36_digest = RAYLOC_TRUE;
		int                   errno;
		char                  *endptr;
		int                   i;
		RAYLOC_ONE_STATION    *sta_struct;
		
		/* See if we can use 36-based digest */
		if (RAYLOC_TRUE == stations->use_36_base_digest)
		{
			/* Get base36 digest for station and net */
			errno =0;
			base36_net = strtoll(net, &endptr, 36);
			if (0 == base36_net)
				use36_digest = RAYLOC_FALSE;
			if (errno != 0)
				use36_digest = RAYLOC_FALSE;

			errno =0;
				base36_sta = strtoll(sta, &endptr, 36);
			if (0 == base36_net)
				use36_digest = RAYLOC_FALSE;
			if (errno != 0)
				use36_digest = RAYLOC_FALSE;
		}
		else
			use36_digest = RAYLOC_FALSE;

		for (i = 0; i < stations->num; i++)
		{
			sta_struct = stations->sta[i];
			if (RAYLOC_TRUE == use36_digest)
			{
				if (base36_net == sta_struct->base36_net)
				{
					if (base36_sta == sta_struct->base36_sta)
						return i;
				}
			}
			else /* digest prohibited */
			{
				if (0 == strncasecmp(net, sta_struct->net, strlen(sta)))
				{
					if (0 == strncasecmp(sta, sta_struct->sta, strlen(sta)))
						return i;
				}
			}
		}
		return RAYLOC_FAILED;
	}

double
	rayloc_get_station_lat(RAYLOC_STATIONS *stations, int num_in_list)
	{
		RAYLOC_ONE_STATION    *sta_struct;
		sta_struct = stations->sta[num_in_list];
		if (!sta_struct)
			return 0.0;
		return (sta_struct->lat);
	}

double
	rayloc_get_station_lon(RAYLOC_STATIONS *stations, int num_in_list)
	{
		RAYLOC_ONE_STATION    *sta_struct;
		sta_struct = stations->sta[num_in_list];
		if (!sta_struct)
			return 0.0;
		return (sta_struct->lon);
	}

double
	rayloc_get_station_elev(RAYLOC_STATIONS *stations, int num_in_list)
	{
		RAYLOC_ONE_STATION    *sta_struct;
		sta_struct = stations->sta[num_in_list];
		if (!sta_struct)
			return 0.0;
		return (sta_struct->elev);
	}

void
	rayloc_destroy_single_stations(RAYLOC_ONE_STATION *sta)
	{
		if (sta)
			free(sta);
		return;
	}

RAYLOC_ONE_STATION *
	rayloc_new_empty_single_station(void)
	{
		RAYLOC_ONE_STATION      *sta;
		sta = (RAYLOC_ONE_STATION *)calloc(1, sizeof(RAYLOC_ONE_STATION));
		return sta;
	}


