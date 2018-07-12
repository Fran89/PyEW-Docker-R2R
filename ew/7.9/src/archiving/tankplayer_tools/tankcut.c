/* tankcut is a quick and dirty utility to cut a specified time segment 
	out of a tank player tank. It was written to be used with 
	ring2tank for installations wishing to do complete reconstruction
	of the data leading up to an earthquake. The data from the tank
	can then be used in tankplayer.

	Paul Friberg ISTI
	
	August 22, 2007
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>

#include "earthworm.h"
#include "trace_buf.h"
#include "sachead.h"
#include "swap.h"
#include "time_ew.h"
#include "chron3.h"
#include "remux.h"

#define VERSION "v0.0.2 2011-06-10"

#ifndef SEC1970
#define SEC1970  11676096000.0
#endif

void usage() {

	fprintf(stdout, "tankcut version %s\n", VERSION);
	fprintf(stdout, "usage: tankcut -s StartTime [-e EndTime|-d Duration] intank outtank\n");
        fprintf(stdout, "\tall times for -s and -e options must be in YYYYMMDDHHMMSS format\n");
        fprintf(stdout, "\t-s StartTime - when to start including tracebufs from intank\n");
        fprintf(stdout, "\t-e EndTime - when to end including tracebufs from intank\n");
        fprintf(stdout, "\t-d Duration - Duration in seconds from start time when to end including tracebufs from intank\n");
        fprintf(stdout, "\t\t Default Duration is 600 seconds from start time\n");

}

#define MAX_TANK_NAME 512

main (int argc, char **argv) 
{
double start_epoch=0.0;
double end_epoch=0.0;
double duration=600.0;
char start_string[19];
char end_string[19];
char tank[2][MAX_TANK_NAME];
int tank_count =0;
int i;

  logit_init("tankcut", 0, 1024, 1);


        /*Parse command line args */
        for(i=1; i<argc; i++) {
                /*check switches */
                if(argv[i][0] == '-') {
                        switch(argv[i][1]) {
                                case 's':
 					i++;
					if (strlen(argv[i]) != 14) {
						fprintf(stderr, "Error: Start time must be YYYYMMDDHHMMSS format\n");
						usage();
						exit(2);
					}
					start_string[0]=0;
					strcpy(start_string, argv[i]);
					/* now tack on the fractional secs */
					strcat(start_string,".00");
					start_epoch = julsec17(start_string) - SEC1970;
					break;
                                case 'e':
 					i++;
					if (strlen(argv[i]) != 14) {
						fprintf(stderr, "Error: End time must be YYYYMMDDHHMMSS format\n");
						usage();
						exit(2);
					}
					end_string[0]=0;
					strcpy(end_string, argv[i]);
					/* now tack on the fractional secs */
					strcat(end_string,".00");
					end_epoch = julsec17(end_string) - SEC1970;
					break;
                                case 'd':
 					duration = atof(argv[i+1]);
					i++;
					break;
				default:
					usage();
					exit(1);
			} /* end of switch */
		} else {
			/* must be tanks */
			if (strlen(argv[i]) > MAX_TANK_NAME) {
				fprintf(stderr, "tank name %s too large, must be less than %d chars\n", argv[i], MAX_TANK_NAME);
				usage();
				exit(2);
			}
			strcpy(tank[tank_count++], argv[i]);
		}
	} /* end of command line args for loop */


/* check command line args */
	if (tank_count != 2) {
		fprintf(stderr, "Error, an input and output tank name must be provided\n");
		usage();
		exit(2);
	}

	if (start_epoch == 0.0) {
		fprintf(stderr, "Error, a start time must be provided, see -s argument\n");
		usage();
		exit(2);
	}
	if (end_epoch == 0.0 && duration == 0.0) {
		fprintf(stderr, "Error, an end time or duration must be provided, see -e or -d arguments\n");
		usage();
		exit(2);
	}

	if (end_epoch == 0.0) {
		end_epoch = start_epoch+duration;
	}

/* now lets get down to business and cut the data out of the tank */
	remux_tracebufs(tank[0], tank[1], start_epoch, end_epoch);

	exit(0);
}
