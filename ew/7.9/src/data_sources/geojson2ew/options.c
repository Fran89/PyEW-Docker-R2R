/*
	geojson2ew - geoJSON to earthworm 

	Copyright (c) 2014 California Institute of Technology.
	All rights reserved, November 6, 2014.
        This program is distributed WITHOUT ANY WARRANTY whatsoever.
        Do not redistribute this program without written permission.

	Authors: Kevin Frechette & Paul Friberg, ISTI.
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "externs.h"
#include "earthworm.h"  /* need this for the logit() call */
#include "die.h"
#include "misc.h"
#include "options.h"

extern char *optarg;
extern int optind;


/* handle_opts() - handles any command line options.
	 	and sets the Progname extern to the
		base-name of the command.

	Returns: 
		TRUE if options are parsed okay.
		FALSE if there are bad or conflicting
		options.

*/

int GetConfig (char *);  /* defined in getconfig.c */

int handle_opts(int argc, char ** argv)  {

int opt_char;

	if ((Progname = (char *) strrchr(argv[0], '/')) == NULL) {
		Progname = argv[0];
	} else {
		Progname++;
	}
	if (optind >= argc || argv[optind][0] == '-') {
		usage();
		geojson2ew_die(GEOJSON2EW_DEATH_EW_CONFIG, "Improper geojson2ew usage");
		return FALSE;
        }
	Config = argv[optind];

        // Initialize name of log-file & open it
        logit_init(Config, 0, LOGIT_LEN, 1);

        logit("et", "%s: Startup of Version %s\n", Progname, VER_NO);

        while ((opt_char = getopt(argc, argv, "Hhv")) != -1) {
		switch (opt_char) {
		case 'v':
			Verbose = VERBOSE_GENERAL;
			break;
		case 'h':
		case 'H':
			usage();
			geojson2ew_die(-1,"Usage request");
			return FALSE;
		default:
                        logit("e", "%s: unknown option: %c\n", Progname, opt_char);
                        geojson2ew_die(-1,"Usage request");
			return FALSE;
		}
			
        }      
	/* NEED to read in the EarthWorm Config file here*/
	if (GetConfig(Config) == -1) {
                geojson2ew_die(GEOJSON2EW_DEATH_EW_CONFIG,"Too many geojson2ew.d config problems");
                return FALSE;
        }

        // Reinitialize logit do desired logging level
        logit_init(Config, (short) QModuleId, LOGIT_LEN, LogFile);
        logit("e", "%s: Read in config file %s\n", Progname, Config);
	return TRUE;
}

/* usage() - explains the options available to the user if the
		-h or -H (HELP!!!!! options are given).

*/
void usage() {
	fprintf(stderr, "%s: usage - Version %s\n", Progname, VER_NO);
	fprintf(stderr, "%s [-v][-hH] ew_config_file\n", Progname);
}
