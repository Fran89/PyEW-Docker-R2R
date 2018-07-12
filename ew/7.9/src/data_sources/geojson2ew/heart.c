/*
	geojson2ew - geoJSON to earthworm 

	Copyright (c) 2014 California Institute of Technology.
	All rights reserved, November 6, 2014.
        This program is distributed WITHOUT ANY WARRANTY whatsoever.
        Do not redistribute this program without written permission.

	Authors: Kevin Frechette & Paul Friberg, ISTI.
*/
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include "earthworm_incl.h"
#include "externs.h"
#include "die.h"
#include "misc.h"
#include "heart.h"

/* prototype for function resident herein */
void message_send( unsigned char, short, char *);

/* Heartbeat thread

	Sends a heartbeat to the transport ring buffer 
	This gets killed by the main thread if there is
	a problem.  
	
	HearbeatInt - governs the heartbeats interval (secs)
*/

time_t	tsLastBeat;		/* time stamp since last heartbeat */
char heart_msg_str[256];

void *Heartbeat(void * UNUSED(arg)) {
        time_t now;

	message_send( TypeHB, 0, "");

	while(!ShutMeDown) {
		sleep_ew(1000);

		if (check_stop()) break;

		time(&now);
		if (difftime(now, tsLastBeat) > (double) HeartbeatInt) {
		    time(&tsLastBeat);
		    message_send( TypeHB, 0, "");
		}
	}
        sleep_ew(5000); // wait for main thread to shut down
        logit("e", "geojson2ew main thread did not terminate, heartbeat thread will now exit\n");
        exit(0);
	return (void *)NULL;
}

int check_stop() {
   if ( !ShutMeDown) {
      /* EARTHWORM see if we are being told to stop */
      int flag = tport_getflag( &Region );
      if ( flag == TERMINATE || flag == MyPid ) {
         geojson2ew_die(GEOJSON2EW_DEATH_EW_TERM,
                        "Earthworm TERMINATE request");
      }
   }
   return ShutMeDown;
}

/***************************************************************************
 message_send() builds a heartbeat or error message & puts it into
                  shared memory.  Writes errors to log file.
 
*/
void message_send( unsigned char type, short ierr, char *note )
{
    time_t t;
    char message[256];
    long len;

    OtherLogo.type  = type;

    time( &t );
    /* put the message together */
    if( type == TypeHB ) {
       sprintf( message, "%ld %ld\n", (long) t, (long) MyPid);
    } else if( type == TypeErr ) {
       sprintf( message, "%ld %hd %s\n", (long) t, ierr, note);
       logit( "et", "%s: %s\n", Progname, note );
    }
    len = strlen( message );   /* don't include the null byte in the message */

   /* write the message to shared memory */
    if( tport_putmsg( &Region, &OtherLogo, len, message ) != PUT_OK ) {
        if( type == TypeHB ) {
           logit("et","%s:  Error sending heartbeat.\n", Progname );
        }
        else if( type == TypeErr ) {
           logit("et","%s:  Error sending error:%d.\n", Progname, ierr );
        }
    }
}
