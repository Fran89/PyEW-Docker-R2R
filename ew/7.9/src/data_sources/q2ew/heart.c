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

/* prototype for function resident herein */
void message_send( unsigned char, short, char *);

/* Heartbeat thread

	Sends a heartbeat to the transport ring buffer 
	This gets killed by the main thread if there is
	a problem.  
	
	HearbeatInt - governs the heartbeats interval (secs)
	
	It also kills itself if it does the main thread does
	not collect data after a certain time period. The
	time period is defined in the config file as:
	
	TimeoutNoSend - if no data received in this interval, die(),
			because something is wrong. If this value
			is set to ZERO, then never check this...Thus,
			the main thread will only die if comserv 
			croaks...via handle_cs_status().
	
	FOr the ISTI test configuration of HH? and HL? chans (6 only) 
	at 50 sps, with a block size of 256 bytes, comserv was passing 
	data every 2 seconds.
	Startup can be as large as 120 seconds. So, this TimeoutNoSend
	is only checked once the FIRST DATA PACKET ARRIVES. If no COMSERV data
	ARRIVES, this timeout will never be checked and another mechanism 
	will be needed to be hit for the process to die. This happens in
	cs_status.c when the comserv status changes.

	READ THE handle_cs_status() comments in the cs_status.c file
	to see how the heartbeat and whole process dies if comserv 
	closes down!...THis might wish to be re-thought.
	
*/

pid_t my_pid;	/* needed for restart message for heartbeat */
char heart_msg_str[256];

void *Heartbeat(void *unused_arg) {
time_t now;
sigset_t new;

	my_pid = getpid();	/* set it once on entry */
	message_send( TypeHB, 0, "");

	/* Mask out alarms so that comserv's SIGALRM is not delivered to this thread. */
	sigemptyset(&new);
	sigaddset(&new,SIGALRM);
#ifdef _SOLARIS
        thr_sigsetmask (SIG_BLOCK, &new, NULL);
#else
        pthread_sigmask (SIG_BLOCK, &new, NULL);
#endif


	while(!ShutMeDown) {
#ifdef	DEBUG
		fputs ("Heartbeat sleep...\n", stderr);
		fflush (stderr);
#endif
		sleep_ew(1000);
#ifdef	DEBUG
		fputs ("Heartbeat awake...\n", stderr);
		fflush (stderr);
#endif
		time(&now);
		if (difftime(now, TSLastBeat) > (double) HeartbeatInt) {
		    time(&TSLastBeat);
		    if (Verbose == TRUE) {
			char s[256];
#ifdef _SOLARIS
			cftime(s, (char*)0, &now);
#else
			char *ss = NULL;
			ss = (char *) calloc(1, 255);
			ss = ctime_r(&now, ss); /* IGD 2006/11/16 Replaced from cdtime to support Linux */
			strncpy(s, ss, strlen(ss) + 1);
			free(ss);
#endif
#ifndef _LINUX
			fprintf(stderr, "Heartbeat issued %s\n", s);
#endif
		    }
		    message_send( TypeHB, 0, "");
		}

		/* if TimeoutNoSend == 0 then ignore this check */
		if (TimeoutNoSend > 0 && TSLastCSData != 0 &&
			difftime(now, TSLastCSData) > (double) TimeoutNoSend) {
		    /* we should die now */
		    sprintf(heart_msg_str, 
			"Hearbeat() saw no COMSERV data for %d seconds\n", 
			TimeoutNoSend);
		    q2ew_die(Q2EW_DEATH_CS_TIMEOUT, heart_msg_str);
		}
	}
	return (void *)NULL;
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
       sprintf( message, "%ld %ld\n", (long) t, (long) my_pid);
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

   return;
}
