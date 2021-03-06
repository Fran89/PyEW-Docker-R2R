#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "externs.h"
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

        while ((opt_char = getopt(argc, argv, "Hhv")) != -1) {
		switch (opt_char) {
		case 'v':
			Verbose = TRUE;
			break;
		case 'h':
		case 'H':
			usage();
			q2ew_die(-1,"Usage request");
			break;
		default:
                        fprintf(stderr, "%s: unknown option: %c\n", Progname, opt_char);
                        q2ew_die(-1,"Usage request");
			return FALSE;
		}
			
        }      
	/* NEED to read in the EarthWorm Config file here*/
	if (optind < argc) {
		Config = argv[optind];
		if (GetConfig(Config) == -1) 
			q2ew_die(Q2EW_DEATH_EW_CONFIG,"Too many q2ew.d config problems");
	} else {
		usage();
		q2ew_die(Q2EW_DEATH_EW_CONFIG, "Improper q2ew usage");
	}
	return TRUE;
}

/* usage() - explains the options available to the user if the
		-h or -H (HELP!!!!! options are given).

*/
void usage() {
	fprintf(stderr, "%s: usage - Version %s\n", Progname, VER_NO);
	fprintf(stderr, "%s [-v][-hH] ew_config_file\n", Progname);
}
