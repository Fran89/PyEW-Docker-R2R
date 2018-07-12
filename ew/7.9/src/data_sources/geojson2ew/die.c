/*
	geojson2ew - geoJSON to earthworm 

	Copyright (c) 2014 California Institute of Technology.
	All rights reserved, November 6, 2014.
        This program is distributed WITHOUT ANY WARRANTY whatsoever.
        Do not redistribute this program without written permission.

	Authors: Kevin Frechette & Paul Friberg, ISTI.
*/
#include <stdio.h>
#include <stdlib.h>
#include "earthworm.h"
#include "transport.h"
#include "externs.h"
#include "heart.h"
#include "die.h"

/* errmap = an optional integer value that maps to a statmgr error 
	see the die.h and geojson2ew.desc
*/

void geojson2ew_die( int errmap, char * str ) {
        ShutMeDown = TRUE;
	if (errmap != -1 && errmap != GEOJSON2EW_DEATH_EW_CONFIG) {
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
}
