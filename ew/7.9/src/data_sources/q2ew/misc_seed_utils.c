#include <stdio.h>
#include "comserv_incl.h"

/* This function returns a statically allocated string of the seed time 
	useful for printing out to a logfile or the like
	
	YYYY:JJJ:HH:MM:SS.SSSS
	    ^
	    Delim char
*/

#define SEED_TIME_STR_SIZE 24
#define SEED_TIME_STR_DELIM ':'
static char dummy_time_string[SEED_TIME_STR_SIZE];

char * seedtimestring( seed_record_header *hdr ) {
	
	sprintf(dummy_time_string, "%4d%c%03d%c%02d%c%02d%c%02d.%04d",
		hdr->starting_time.yr, SEED_TIME_STR_DELIM, 
		hdr->starting_time.jday, SEED_TIME_STR_DELIM,
		hdr->starting_time.hr, SEED_TIME_STR_DELIM,
		hdr->starting_time.minute, SEED_TIME_STR_DELIM,
		hdr->starting_time.seconds, hdr->starting_time.tenth_millisec);
	return dummy_time_string;
}

