#include <stdio.h>
#include "die.h"
#include "misc.h"
#include "externs.h"
#include "earthworm_incl.h"
#include "comserv_incl.h"

/* what the defs  CSCR_ correspond to in english,
        found in comserv/include/service.h
*/
CHAR23 stats[13] = {    "Good",
                        "Enqueue Timeout",
                        "Service Timeout",
                        "Init Error",
                        "Attach Refused",
                        "No Data",
                        "Server Busy",
                        "Invalid Command",
                        "Server Dead",
                        "Server Changed",
                        "Segment Error",
                        "Command Size",
                        "Privileged Command"
};


/* 

NOTE, this is where Q2EW could bite the big one if comserv disappears!

This is a tough call. DO you rely on netmon to restart comserv, or if comserv
disappears, do you rely on the Earthworm Statmgr to restart the q2ew process...

For now, since this is an Earthworm CENTRIC process, if the status CSCR_DIED is
encountered, the q2ew process dies  a complete death...

*/

int
handle_cs_status (int status, long station) {
		switch (status) {
			case CSCR_GOOD:
			/* good status */
				if (Verbose == TRUE)
					fprintf (stderr, "%s: Station %s has status=%s\n", Progname, long_str(station), stats[status]);
				logit("e", "Station %s has status=%s\n", long_str(station), stats[status]);
				break;
			case CSCR_INIT:
			case CSCR_TIMEOUT:
			case CSCR_BUSY:
			case CSCR_CHANGE:
			case CSCR_DIED:	/* SHOULD THIS BE a fatal status??? */
				if (Verbose == TRUE)
				    fprintf (stderr, 
					"%s: Station %s has recoverable status=%s\n",
					Progname ,long_str(station), stats[status]);
				logit("e", "Station %s has  recoverable status=%s\n",long_str(station), stats[status]);
				break;
			/* fatal status */
			case CSCR_ENQUEUE:
			case CSCR_REFUSE:
			case CSCR_INVALID:
			case CSCR_PRIVATE:
			case CSCR_SIZE:
			case CSCR_PRIVILEGE:
				if (Verbose == TRUE)
				    fprintf (stderr, "Station %s has FATAL status=%s\n exiting....\n", long_str(station), stats[status]);
				logit("e", "Station %s has FATAL status=%s\n exiting....\n", long_str(station), stats[status]);
				q2ew_die(Q2EW_DEATH_CS_PROBLEM, " bad COMSERV status ");
				break;
		}
	return	 TRUE;
}
