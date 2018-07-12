/*
	geojson2ew - geoJSON to earthworm 

	Copyright (c) 2014 California Institute of Technology.
	All rights reserved, November 6, 2014.
        This program is distributed WITHOUT ANY WARRANTY whatsoever.
        Do not redistribute this program without written permission.

	Authors: Kevin Frechette & Paul Friberg, ISTI.
*/
#define MAIN
#define THREAD_STACK_SIZE 8192
#include "main.h"

volatile int err_exit = -1;		/* an geojson2ew_die() error number */
time_t  tsLastMsg; 			/* time stamp of last message */

/* signal handler that intiates a shutdown */
void initiate_termination(int sigval) {
    signal(sigval, initiate_termination);
    ShutMeDown = TRUE;
    err_exit = GEOJSON2EW_DEATH_SIG_TRAP;
    return;
}

void run() {
        int ret_val;			/* DEBUG return values */
	char *msg;
	time_t now;
	int process_error = 0;
	/* deal with some signals */
	signal(SIGINT, initiate_termination);
	signal(SIGTERM, initiate_termination);

	/* EARTHWORM init earthworm connection at this point, 
		this func() exits if there is a problem 
	*/
	tport_attach( &Region, RingKey );
	logit("e", "%s: Attached to Ring Key=%d\n", Progname, RingKey);

	/* EARTHWORM start a heartbeat thread */
	if ( (ret_val=StartThread(Heartbeat,(unsigned)THREAD_STACK_SIZE,&TidHB)) == -1) {
	    /* we have a problem with starting this thread */
	    logit("e", "%s: Heartbeat startup failed, retval=%d\n", Progname, ret_val);
	    geojson2ew_die(-1, "Heartbeat startup failure" );
	    return;
	}

	/* sleep for 2 seconds to allow heart to beat so statmgr gets it 
		this helps statmgr see geojson2ew in case server is really DOA
		at startup.
	*/
	sleep_ew(2000);

	if ( open_json_connection(&Conn_params) ) {
	    geojson2ew_die(-1, "JSON connection failure" );
	    return;
	}

	time(&tsLastMsg);  // set initial time to now
	while ( !ShutMeDown ) {
	    if ((msg = read_json_message()) != NULL) {
		 if ( Conn_params.data_timeout )
	            time(&tsLastMsg);  // update last time stamp
	         process_error = process_json_message(msg);
	         free_json_message();
	    } else if ( Conn_params.data_timeout ) {
	       time(&now);  // get current time stamp
               if (difftime(now, tsLastMsg) > Conn_params.data_timeout) {
		  err_exit = GEOJSON2EW_DEATH_SERVER_ERROR;
		  geojson2ew_die(err_exit, 
				"geojson2ew data timeout");
		  return;
	       }
            }

            if (process_error) {
               err_exit = process_error;
               ShutMeDown = TRUE;
            }
	    /* see if we had a problem anywhere in processing the last data */
	    if (ShutMeDown == TRUE) {
			geojson2ew_die(err_exit, 
				"geojson2ew kill request or fatal EW error");
			return;
	    }
	}
}

int main (int argc, char ** argv) {
	/* init some globals */
	MyPid = getpid();      /* set it once on entry */
	Verbose = 0;
	VerboseSncl = NULL;
	ShutMeDown = FALSE;
	Region.mid = -1;	/* init so we know if an attach happened */
	
	// handle some options
	handle_opts(argc, argv);
	if ( !ShutMeDown ) {
	   logit("e", "run start\n");
	   run();
	   logit("e", "run end\n");
	   close_json_connection();
	}
	exit(0);
}
