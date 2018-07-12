/*
	q2ew - quanterra two earthworm 

		(designed and tested using Q730 dataloggers)

	COPYRIGHT 1998, 1999: Instrumental Software Technologies, Inc.
	ALL RIGHTS RESERVED. Please contact the authors for use
	of this code. It is free to all Academic institutions and
	Federal Government Agencies.

	This code requires the COMSERV libraries of Quanterra Inc. and
	the QLIB2 libraries of Univ. California Berkeley, both of which
	may be obtained at the ftp site provided by Berkeley:
	ftp://quake.geo.berkeley.edu

	Authors: Paul Friberg & Sid Hellman 

	Contact: support@isti.com
	
	V1.0.0 September 14, 1998

	V1.0.1 May 20, 1999 - added a few DEBUG statements to main.c
	
	V1.0.2 June 18, 1999 - contributions from Doug Neuhauser
				- cs_off() used to clean up comserv shared mem
				- masked off SIGALARM signal in heart.c
				- turned off setting of Comseqbuf
				- removed some c++ comments that croaked the sun compiler
				

	COMMENTS:

		Note that this code is specially designed for the ATWC
		application. For instance, since ATWC is using Q730 dataloggers
		and only recent data are of interest to them, we set CSQ_LAST
		for the stations' sequence buffer delimiter. This may be set
		via the q2ew.d description file input on the command line.

	SEE: main.h for externs and all includes.

*/
#define MAIN
#define THREAD_STACK_SIZE 8192
#include "main.h"

int err_exit;				/* an q2ew_die() error number */

int main (int argc, char ** argv) {

STATIONS stations;			/* COMSERV stations struct */
int i, sta_index, scan_station;		/* indexes */
boolean alert, okay;			/* flags */
struct timespec poll_time;		/* nanosleep() to sleep */
struct timespec actual_poll;		/* nanosleep() actually slept */
int ret_val;				/* DEBUG return values */
void initiate_termination(int );	/* signal handler func */
char *ew_trace_buf;			/* earthworm trace buffer pointer */
long  ew_trace_len;			/* length in bytes of the trace buffer */
void logLOGchans();
pid_t mypid;

	/* init some locals */
	UseTraceBuf2 = 0 ; /* do not use TraceBuf 2 by default, turned on by InfoSCNL tags */
	alert = FALSE;
	okay = TRUE;
	poll_time.tv_sec = 0;		/* zero seconds */
	poll_time.tv_nsec = POLL_NANO;	/* nano seconds */
	err_exit = -1;
        mypid = getpid();       /* set it once on entry */


	/* init some globals */
	Verbose = FALSE;	/* not well used */
	ShutMeDown = FALSE;
	LOG2LogFile = 0;	/* do not log LOG chans to the LogFile */
	Region.mid = -1;	/* init so we know if an attach happened */
	TSLastCSData = 0;	/* set this to zero to start, see heart.c for details */
	
	/* handle some options, this exits if there are problems */
	handle_opts(argc, argv);
	
	/* deal with some signals */
	signal(SIGINT, initiate_termination);
	signal(SIGTERM, initiate_termination);

	/* setup stations struct for cs_gen, from /etc/stations.ini */
	cs_setup(&stations, client_name, server_name, TRUE, TRUE, 
		10, 5, DATA_MASK, COM_OUT_BUF_SIZE);

	/* start EARTHWORM logging */
	logit_init(Progname, (short) QModuleId, 256, LogFile);
	logit("e", "%s: Read in config file %s\n", Progname, Config);

	/* set up COMSERV shared memory region */
	shbuf = cs_gen(&stations);
	if (shbuf->maxstation == 0) {
		/* no stations setup in comserv, FATAL ERROR  die() */
		logit("e", "%s:no stations.ini setup in comserv\n", Progname);
		q2ew_die(-1, "no stations.ini setup in comserv" );
	}

	/* EARTHWORM init earthworm connection at this point, 
		this func() exits if there is a problem 
	*/
	tport_attach( &Region, RingKey );
	logit("e", "%s: Attached to Ring Key=%d\n", Progname, RingKey);

	/* EARTHWORM start a heartbeat thread */
	time(&TSLastBeat);	/* init the time we start */
	if ( (ret_val=StartThread(Heartbeat,(unsigned)THREAD_STACK_SIZE,&TidHB)) == -1) {
	    /* we have a problem with starting this thread */
	    fprintf(stderr, "%s: Fatal Error, could not start Hearbeat() thread %d\n",  
		Progname, ret_val);
	    logit("e", "%s: Heartbeat startup failed, retval=%d\n", Progname, ret_val);
	    q2ew_die(-1, "Heartbeat startup failure" );
	}

	/* sleep for 2 seconds to allow heart to beat so statmgr gets it 
		this helps statmgr see q2ew in case COMSERV is really DOA
		at startup.
	*/
	sleep_ew(2000);

	/* COMSERV loop through all stations and check status for 
		Fatal errors: dead comserv servers or 
		refusal of service; notify earthworm of
		refusals. 
	*/
	for (sta_index = 0 ; sta_index < shbuf->maxstation ; sta_index++) {
		sta = (PTR_STA_CLIENT) ((long) shbuf + shbuf->offsets[sta_index]);
		/* only get the new data , change this to CSQ_FIRST if
		you want the first available data (historical) */
		sta->seqdbuf = ComservSeqBuf;
		handle_cs_status(sta->status, sta->name.l);
	}

	/* main thread loop acquiring DATA and LOG MSGs from COMSERV */
	while (okay) {
	    /* COMSERV see if we have any data */
	    if ( (scan_station = cs_scan(shbuf, &alert)) != NOCLIENT) {
		/* station scan_station has new data */
		/* check to see if station_status has changed */
		sta = (PTR_STA_CLIENT)((long) shbuf + 
			shbuf->offsets[scan_station]);
		if (alert) {
		    handle_cs_status(sta->status, sta->name.l);
		    /* NEED to check if we should reset the seqdbuf to
			CSQ_LAST ---as this can get reset if the server
			changes invocations.
		     */
		 /* 1999.170 - TURN THIS OFF as per Doug Neuhauser's suggestion.
			That is, stay with CSQ_LAST for only the first run through
			and let COMSERV set it to CSQ_NEXT on the next loop 
		  */
		/* 1999.170 commented out!
		    if (sta->seqdbuf != ComservSeqBuf) {
			sta->seqdbuf = ComservSeqBuf;
		    }
		 */
		}
		if (sta->valdbuf) {
		    char sta_name[6];	/* seed header station name */
#ifdef DEBUG
		    fprintf(stderr, "START LOOP over station for %d bufs\n", sta->valdbuf);
#endif /* DEBUG */
		/* we have data, deal with it */
		    data = (PTR_DATA)((long) shbuf + sta->dbufoffset);
		    /* there could be more than one block in this data */
		    for(i=0; i < sta->valdbuf; i++) {
		        if (Verbose == TRUE) {
			    seed_record_header *mseed_hdr;
			    mseed_hdr = (void *) &data->data_bytes;
			    strncpy(sta_name,mseed_hdr->station_ID_call_letters,5);
			    sta_name[5] = '\0';
			    fprintf(stderr, "Received %s %s Time: %s\n", sta_name,
				seednamestring(&mseed_hdr->channel_id, 
					&mseed_hdr->location_id), 
				seedtimestring(mseed_hdr));
	 	        }	
			/* uncompress it here */
			if ((ew_trace_buf = convert_mseed_to_ewtrace((void *) &data->data_bytes, 
					MIN_SEED_BLK_SIZE, &ew_trace_len)) == NULL) {
				if (ew_trace_len != 0) {
					/* we have a LOG channel */
					if (LOG2LogFile == 1) {
						logLOGchans(data);
					}
				} else {
				    logit("et", "%s: Fatal Error decompressing trace via msunpack()\n",
					Progname);
				    /* SEND MESSAGE to statmgr of decompress */
				    message_send(TypeErr, 6, "qlib2 decompression error");
				}
			} else {
			    /* transport it off to EW */
		    	    time(&TSLastCSData);/* set the time since the lastCS data */
			    if ( tport_putmsg(&Region, &DataLogo, (long) ew_trace_len, ew_trace_buf) != PUT_OK) {
				logit("et", "%s: Fatal Error sending trace via tport_putmsg()\n",
					Progname);
				ShutMeDown = TRUE;
				err_exit = Q2EW_DEATH_EW_PUTMSG;
			    } else if ( Verbose==TRUE ) {
                                seed_record_header *mseed_hdr;
                                char sta_name[6];   /* seed header station name */
                                mseed_hdr = (void *) &data->data_bytes;
                                strncpy(sta_name,mseed_hdr->station_ID_call_letters,5);
                                sta_name[5] = '\0';
                                fprintf(stderr, "SENT to EARTHWORM %s %s Time: %s\n", sta_name,
                                        seednamestring(&mseed_hdr->channel_id,
                                                &mseed_hdr->location_id),
                                        seedtimestring(mseed_hdr));
                            }

			}
			/* increment pointer to next mseed data block */
			data = (PTR_DATA) ((long) data + sta->dbufsize);
		    } 	/* loop over all data for this station */
#ifdef DEBUG
	{
	 	fprintf(stderr, "END LOOP over station %s, %d bufs processed\n",
			sta_name, i);
	}
#endif /* DEBUG */
		} /* end of if for valid data buffers for this station */
	    } else {
#ifdef  DEBUG
		fputs ("sleeping...\n",stderr);
		fflush (stderr);
#endif
	    	nanosleep(&poll_time, &actual_poll);
#ifdef  DEBUG
		fputs ("awake...\n", stderr);
		fflush (stdout);
#endif
	    }
	    /* EARTHWORM see if we are being told to stop */
	    if ( tport_getflag( &Region ) == TERMINATE ||
                 tport_getflag( &Region ) == mypid        ) {
			q2ew_die(Q2EW_DEATH_EW_TERM,
				"Earthworm TERMINATE request");
	    }
	    /* see if we had a problem anywhere in processing the last data */
	    if (ShutMeDown == TRUE) {
			q2ew_die(err_exit, 
				"q2ew kill request or fatal EW error");
	    }
	}
	/* should never reach here! */
	q2ew_die( -1, "clean exit" );
	exit(0);
}

/************************************************************************/
/* signal handler that intiates a shutdown */
void initiate_termination(int sigval) {
    signal(sigval, initiate_termination);
    ShutMeDown = TRUE;
    err_exit = Q2EW_DEATH_SIG_TRAP;
    return;
}

/************************************************************************/
/* this should be moved to its own logLOGchans.c file eventually */
/* logLOGchans() sends LOG channel messages to the q2ew log file */

void logLOGchans(PTR_DATA data) {
seed_record_header *mseed_hdr;/* mseed_hdr comserv */
char *log_msg, *log_end, sta_name[6];

	mseed_hdr = (void *) &data->data_bytes;
	log_msg = (char *)mseed_hdr + mseed_hdr->first_data_byte;
	log_end = log_msg + mseed_hdr->samples_in_record - 2;
	*log_end = '\0' ;
	if (Verbose==TRUE)
		fprintf(stderr, "LOG message: %s\n", log_msg);
	strncpy(sta_name,mseed_hdr->station_ID_call_letters,5);
	sta_name[5] = '\0';
	logit("e", "%s %s %s Time: %s \n LOGMSG: %s\n", Progname,sta_name,
		seednamestring(&mseed_hdr->channel_id, 
		&mseed_hdr->location_id), seedtimestring(mseed_hdr), log_msg);
}
