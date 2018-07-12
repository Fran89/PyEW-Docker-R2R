
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
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
	
*/

pid_t my_pid;	/* needed for restart message for heartbeat */
char heart_msg_str[256];

void *Heartbeat(void *unused_arg) {
time_t now;
struct tm *tm_ptr;
sigset_t new;

	my_pid = getpid();	/* set it once on entry */
	message_send( TypeHB, 0, "");

	/* Mask out alarms so that comserv's SIGALRM is not delivered to this thread. */
	sigemptyset(&new);
	sigaddset(&new,SIGALRM);
#ifdef SOLARIS
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
/*
		    if (Verbose == TRUE) {
			char s[1024];
			tm_ptr = gmtime(&now);
			strftime(s, 255, (char*)"%F_%T", tm_ptr);
			fprintf(stderr, "Heartbeat issued at %s\n", s);
		    }
*/
		    message_send( TypeHB, 0, "");
		}

		/* if TimeoutNoSend == 0 then ignore this check */
		if (TimeoutNoSend > 0 && TSLastGCFData != 0 &&
			difftime(now, TSLastGCFData) > (double) TimeoutNoSend) {
		    /* we should die now */
		    sprintf(heart_msg_str, 
			"Hearbeat() saw no GCF data for %d seconds\n", 
			TimeoutNoSend);
		    gcf2ew_die(GCF2EW_DEATH_GCF_TIMEOUT, heart_msg_str);
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
       sprintf( message, "%ld %ld\n", t, my_pid);
    } else if( type == TypeErr ) {
       sprintf( message, "%ld %hd %s\n", t, ierr, note);
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
