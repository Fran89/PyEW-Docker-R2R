#include <stdio.h>
#include <stdlib.h>
#include "earthworm.h"
#include "transport.h"
#include "externs.h"

#include "comserv_incl.h"
extern PTR_CLIENT       shbuf;


/* errmap = an optional integer value that maps to a statmgr error 
	see the die.h and q2ew.desc
*/

void q2ew_die( int errmap, char * str ) {

/* prototype for message_send() from heart.c */
void message_send( unsigned char, short, char *);

	if (errmap != -1) {
		/* use the statmgr reporting to notify of this death */
#ifdef DEBUG
		fprintf(stderr, "SENDING MESSAGE to statmgr: %d %s\n",
			errmap, str);
#endif /*DEBUG*/
		message_send(TypeErr, errmap, str);
	}
	
	/* this next bit must come after the possible tport_putmsg()
		above!! */
	if (Region.mid != -1) {
		/* we attached to an EW ring buffer */
		logit("e", "%s: exiting because %s\n", Progname, str);
		tport_detach( &Region );
	}

	/* Perform final cs_scan for 0 records to ack previous records.	*/
	/* Detach from all stations and delete my segment.			*/
	if (shbuf != NULL) {
	    int j;
	    pclient_station this;
	    boolean alert;
	    for (j=0; j< shbuf->maxstation; j++) {
		this = (pclient_station) ((long) shbuf + shbuf->offsets[0]);
		this->reqdbuf = 0;
	    }
	    cs_scan (shbuf, &alert);
	    cs_off (shbuf);
	}

	exit(0);
}
