// 
// ======================================================================
// Copyright (C) 2000-2003 Instrumental Software Technologies, Inc. (ISTI)
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. If modifications are performed to this code, please enter your own 
// copyright, name and organization after that of ISTI.
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in
// the documentation and/or other materials provided with the
// distribution.
// 3. All advertising materials mentioning features or use of this
// software must display the following acknowledgment:
// "This product includes software developed by Instrumental
// Software Technologies, Inc. (http://www.isti.com)"
// 4. If the software is provided with, or as part of a commercial
// product, or is used in other commercial software products the
// customer must be informed that "This product includes software
// developed by Instrumental Software Technologies, Inc.
// (http://www.isti.com)"
// 5. The names "Instrumental Software Technologies, Inc." and "ISTI"
// must not be used to endorse or promote products derived from
// this software without prior written permission. For written
// permission, please contact "info@isti.com".
// 6. Products derived from this software may not be called "ISTI"
// nor may "ISTI" appear in their names without prior written
// permission of Instrumental Software Technologies, Inc.
// 7. Redistributions of any form whatsoever must retain the following
// acknowledgment:
// "This product includes software developed by Instrumental
// Software Technologies, Inc. (http://www.isti.com/)."
// 8. Redistributions of source code, or portions of this source code,
// must retain the above copyright notice, this list of conditions
// and the following disclaimer.
// THIS SOFTWARE IS PROVIDED BY INSTRUMENTAL SOFTWARE
// TECHNOLOGIES, INC. "AS IS" AND ANY EXPRESSED OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.  IN NO EVENT SHALL INSTRUMENTAL SOFTWARE TECHNOLOGIES,
// INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
// 
#include "q3302ew.h"

#ifndef _WINNT
#include <unistd.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include "heart.h"
#include "config.h"
#include "externs.h"

#include "transport.h"
#include "earthworm_simple_funcs.h"
#include "earthworm.h"
#include <time.h>

time_t	Heartbeat_lastBeatTime;		/* time stamp since last heartbeat */
unsigned Heartbeat_threadID;			/* ID for the hearbeat thread */

/* Heartbeat thread

	Sends a heartbeat to the transport ring buffer 
	This gets killed by the main thread if there is
	a problem.  
	
	HearbeatInt - governs the heartbeats interval (secs)
	
*/

pid_t MyPid;	/* needed for restart message for heartbeat */
char heart_msg_str[256];

void *Heartbeat(void *unused_arg) {
	time_t now;
	MyPid = getpid();	/* set it once on entry */
	message_send( TypeHB, 0, "");

	while(!ShutMeDown) {

	  sleep_ew(1000);

	  time(&now);
	  if (difftime(now, Heartbeat_lastBeatTime) > (double) gConfig.HeartbeatInt) {
	    time(&Heartbeat_lastBeatTime);
	    message_send( TypeHB, 0, "");
	  }
		
	}
	return (void *)NULL;
}

/***************************************************************************
 message_send() builds a heartbeat or error message & puts it into
                  shared memory.  Writes errors to log file.
 
*/
void message_send( unsigned char type, short ierr, char *note ) {
    time_t t;
    char message[256];
    long len;

    OtherLogo.type  = type;

    time( &t );
    /* put the message together */
    if( type == TypeHB ) {
       sprintf( message, "%ld %d\n", (long)t, MyPid);
    } else if( type == TypeErr ) {
       sprintf( message, "%ld %hd %s\n", (long)t, ierr, note);
       logit( "et", "%s: %s\n", Q3302EW_NAME, note );
    }
    len = strlen( message );   /* don't include the null byte in the message */

   /* write the message to shared memory */
    if( tport_putmsg( &Region, &OtherLogo, len, message ) != PUT_OK ) {
        if( type == TypeHB ) {
           logit("et","%s:  Error sending heartbeat.\n", Q3302EW_NAME );
        }
        else if( type == TypeErr ) {
           logit("et","%s:  Error sending error:%d.\n", Q3302EW_NAME, ierr );
        }
    }

   return;
}
