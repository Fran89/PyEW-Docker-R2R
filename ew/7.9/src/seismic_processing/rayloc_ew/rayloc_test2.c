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
 *    $Id: rayloc_test2.c 2055 2006-01-19 19:04:55Z friberg $
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
 *     Revision 1.1  2004/08/03 17:51:47  ilya
 *     Finalizing the project: using EW globals
 *
 *     Revision 1.1.1.1  2004/06/22 21:12:06  ilya
 *     initial import into CVS
 *
 */

/***************************************************************************
                          rayloc_main1.c  -  description
                             -------------------
    begin                : Wed Jun 2 2004
    copyright            : (C) 2004 by Ilya Dricker, ISTI
    email                : i.dricker@isti.com
 ***************************************************************************/



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "rayloc1.h"
#include <rayloc_message_rw.h>

#ifndef VERSION
#define VERSION "0.1"
#endif

#ifndef PACKAGE
#define PACKAGE "rayloc_test3"
#endif

void
	usage(void);

void
	usage(void)
	{
    fprintf(stdout, "VERSION %s\n", VERSION);
		fprintf(stdout, "%s -m in message_file -o out_file -i token -d dir -s station_file\n", PACKAGE);
		exit(0);
	}


int
	main(int argc, char **argv)
	{
		int c;
		int retVal;
		char *dirname = NULL;
		char *token = NULL;
		char *inMesFileName = NULL;
		char *outMesFileName = NULL;
		char *inStr = NULL;
		char *outStr = NULL;
		char *errStr = NULL;
		char *station_fname = NULL;
		int in_mesLen;
		int errLines;
		RAYLOC_PROC_FLAGS *flags = NULL;
		RAYLOC_PHASES *unused_list = NULL;
		RAYLOC_STATIONS *stns;

		while ( (c=getopt(argc,argv,"s:i:d:m:o:H?")) != -1)
		{
			switch(c)
			{
				case 'i':
					token = optarg;
					break;
				case 'd':
					dirname = optarg;
					break;
				case 'm':
					inMesFileName = optarg;
					break;
				case 'o':
					outMesFileName = optarg;
					break;
				case 's':
					station_fname = optarg;
					break;
					
				case 'H':
				case '?':
				default:
					usage();
			}
		}

		
		if(!inMesFileName)
			usage();
		if(!outMesFileName)
			usage();
		if (!dirname)
			usage();
		if (!token)
			usage();
		if (!station_fname)
			usage();

		retVal = file2string(inMesFileName, &inStr);
		if (retVal < 0)
		{
			fprintf(stderr, "Failed to convert file %s to string\n", inMesFileName);
			exit(-1);
		}
		in_mesLen = retVal;

		unused_list = rayloc_new_phase_list();
		(void)rayloc_add_phase(unused_list, "?");

		stns = rayloc_stations_from_file(station_fname, &retVal, &errLines);
		
		flags = (RAYLOC_PROC_FLAGS *)calloc(1, sizeof(RAYLOC_PROC_FLAGS));
		retVal = lib_rayloc(inStr, in_mesLen, unused_list, flags, token, dirname, 232, 4, 13, 233, 4, 13, &outStr, &errStr, stns);

		/* Clean-up operations */
		if (flags)
			free(flags);
		rayloc_destroy_phase_list(unused_list);
		rayloc_destroy_stations(stns);
		if (outStr)
			free(outStr);

		if (inStr)
			free(inStr);
		exit(0);
	}
 
