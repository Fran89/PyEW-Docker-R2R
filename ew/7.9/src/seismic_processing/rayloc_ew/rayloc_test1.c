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
 *    $Id: rayloc_test1.c 1669 2004-08-05 04:15:11Z friberg $
 *
 *    Revision history:
 *     $Log$
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
                          rayloc1_main.c  -  description
                             -------------------
    begin                : Tue Jun 1 2004
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

#ifndef VERSION
#define VERSION "0.1"
#endif

#ifndef PACKAGE
#define PACKAGE "rayloc_test2"
#endif

void
	usage(void);

void
	usage(void)
	{
    fprintf(stdout, "VERSION %s\n", VERSION);
		fprintf(stdout, "%s -i token -d dir\n", PACKAGE);
		exit(0);
		
	}


int
	main(int argc, char **argv)
	{
		int c;
		int retVal;
		char *dirname = NULL;
		char *token = NULL;
		int i;
		while ( (c=getopt(argc,argv,"i:d:")) != -1)
		{
			switch(c)
			{
				case 'i':
					token = optarg;
					break;
				case 'd':
					dirname = optarg;
					break;
					case 'H':
					case '?':
					default:
					usage();
			}
		}
		if (!dirname)
			usage();
		if (!token)
			usage();
		for(i = 0; i < 30; i++)
		{
			retVal = lib_rayloc_stub(token, dirname);
			fprintf(stdout, "%d\n", retVal);
		}

		exit(0);
	}
