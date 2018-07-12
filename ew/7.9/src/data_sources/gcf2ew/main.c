/*

	COPYRIGHT 2002, 2003: Instrumental Software Technologies, Inc.
	ALL RIGHTS RESERVED. Please contact the authors for use
	of this code. It is free to all Academic institutions and
	Federal Government Agencies. Any commercial entity must
	license the program from ISTI.com

	Author: Paul Friberg

	Contact: support@isti.com
	
	started V0.0.1 	May 22, 2002

	COMMENTS:


	SEE: main.h for externs and all includes.
	SEE: misc.h for all details about versioning too!

*/
#define MAIN

#define UDP_SERVER_RENEW	200

#define THREAD_STACK_SIZE 8192
#define GCF_TIMEOUT 20000	/* milliseconds to wait for a packet (20secs) 
					before redoing the connection */
#define INIT_TRIES 4		/* how many times to try the initial socket connection before dieing*/
#define SLEEP_BETWEEN_TRIES 2000 /* milliseconds to sleep between each INITIAL conn try */

#define STARTUP_MSTIMEOUT   5000 /* msecs to wait for connect() call */


#define REPORT_INTERVAL 1000	/* report every X number of blocks to the log */
#include "main.h"

int err_exit;				/* an gcf2ew_die() error number */

unsigned long	blocks_read;
unsigned long	blocks_missed;
unsigned long   total_dropped_udp_packet_counter;
unsigned long   total_udp_packet_counter;
unsigned char 	input_type;		/* type of GCF input, see #defines */



int main (int argc, char ** argv) {

int alert, okay;			/* flags */
struct timespec poll_time;		/* nanosleep() to sleep */
struct timespec actual_poll;		/* nanosleep() actually slept */
int ret_val;				/* DEBUG return values */
void initiate_termination(int );	/* signal handler func */
char *ew_trace_buf;			/* earthworm trace buffer pointer */
long  ew_trace_len;			/* length in bytes of the trace buffer */
pid_t mypid;				/* DUH...the proc id of me */
GCFhdr hdr;				/* the gcf struct of a GCF packet */
char * serial_buf;			/* a pointer to serial data returned by gcf_socket_read() */
int packet_size;			/* the size of the packet */
int block_no;				/* the block number returned from the DM */
unsigned char  expected, last_block_no;		/* a 0-255  last block number from the DM */
SCN *scn;				/* an scn struct pointer  returned by getSCN() */
int first;				/* a flag  to indicate first packet received */
int initial_connect_tries;		/* how many times do we retry 
						a connection at startup */
int StartupMsTimeout = STARTUP_MSTIMEOUT;	/* how long to try a connect() 
							call  on the socket */
/* gcf_udp additions */
void *void_ptr;				/* used for return of packet from udp */
GCFscreamudp scream_packet;
unsigned short last_sequence = USHRT_MAX;	/* gcfserv udp packet seq */
int gcfcmd;					/* placeholder for gcfserv 
						   command returned by server */
int recv_item_type;	/* type of item in void_ptr returned */
unsigned long   renew_udp_packet_counter;
	

	UseTraceBuf2 = 0 ; /* do not use TraceBuf 2 by default, turned on by InfoSCNL tags */
	total_dropped_udp_packet_counter=0;
	input_type = 0;
	renew_udp_packet_counter = total_udp_packet_counter = 0;
	Port = DEFAULT_MSS_PORT;
	Host = NULL;
	GCFHost = NULL;
	initial_connect_tries = INIT_TRIES;	/* for MSS100, try a few times */
	gcf_socket_fd = -1;
	alert = FALSE;
	okay = TRUE;
	poll_time.tv_sec = 0;		/* zero seconds */
	poll_time.tv_nsec = POLL_NANO;	/* nano seconds */
	err_exit = -1;
        mypid = getpid();       /* set it once on entry */
	blocks_missed = blocks_read = 0;


	/* init some globals */
	Verbose = FALSE;	/* not well used */
	ShutMeDown = FALSE;
	SaveLOGS = 0;		/* do not log LOG chans to the LogFile */
	Region.mid = -1;	/* init so we know if an attach happened */
	TSLastGCFData = 0;	/* set this to zero to start, 
					see heart.c for details */
	
	/* handle some options, this exits if there are problems */
	handle_opts(argc, argv);
	
	/* deal with some signals */
	signal(SIGINT, initiate_termination);
	signal(SIGTERM, initiate_termination);

	/* start EARTHWORM logging */
	/* changed logit_init to use .d file  instead */
	logit_init(Config, (short) GModuleId, 1030, LogFile);
	logit("et", "%s: Startup of Version %s\n", Progname, VER_NO);
	logit("et", "%s: Read in config file %s\n", Progname, Config);
	LogConfig();	/* log the config params */


	/* need to start the heart beat before the connection attempt */
	/* EARTHWORM init earthworm connection at this point, 
		NB: this func() exits if there is a problem 
	*/
	tport_attach( &Region, RingKey );
	logit("et", "%s: Attached to Ring Key=%d\n", Progname, RingKey);

	/* EARTHWORM start a heartbeat thread */
	time(&TSLastBeat);	/* init the time we start */
	if ( (ret_val=StartThread(Heartbeat,(unsigned)THREAD_STACK_SIZE,&TidHB)) == -1) {
	    /* we have a problem with starting this thread */
	    fprintf(stderr, "%s: Fatal Error, could not start Hearbeat() thread %d\n",  
		Progname, ret_val);
	    logit("e", "%s: Heartbeat startup failed, retval=%d\n", Progname, ret_val);
	    gcf2ew_die(-1, "Heartbeat startup failure" );
	}

	/* set up GCF connection to data source*/
	if (Host != NULL) {
	    input_type = GCF_INPUT_MSS;
	    while ( initial_connect_tries > 0 && 
			(gcf_socket_fd = gcf_mss_client_init(Host, Port, StartupMsTimeout)) == -1) {
		initial_connect_tries--;
		/* we just try once at start up...the program can be restarted! by statmgr */
		logit("et", "%s: Could not connect with %s %d at startup on try %d for reason: %s\n", 
				Progname, Host, Port, 
				INIT_TRIES - initial_connect_tries, 
				gcf_getcomm_error());
		sleep_ew(SLEEP_BETWEEN_TRIES*(INIT_TRIES - initial_connect_tries));
	    }
	    if (initial_connect_tries == 0) {
		logit("et", "%s: Could not connect with MSS after %d attempts\n", 
				Progname, INIT_TRIES);
		gcf2ew_die(-1, "Initial Socket Connection Failure" );
	    }
	    logit("et", "%s: MSS Connect with %s %d\n", Progname, Host, Port);
	} else if (GCFHost != NULL) {
	        input_type = GCF_INPUT_GCFSERV;
		/* then we have a gcfserv client */
	
		if (Port == DEFAULT_MSS_PORT) {
			Port = DEFAULT_GCFSERV_PORT;
	    		logit("et", "%s: setting gcfserv port to default value of %d\n", Progname, Port);
			
		}
	
		/* connect up */
		if ( (gcf_socket_fd = gcf_udp_client_init(GCFHost, Port)) == -1) {
			logit("et", "%s: Could not connect with gcfserv at %s:%d at startup \n", 
				Progname, GCFHost, Port); 
			gcf2ew_die(-1, "Initial Socket Connection Failure" );
		}

		/* ping the server to see if it is responding */
		if (gcf_udp_ping(gcf_socket_fd) == 0) {
			logit("et", "%s: ping to gcfserv at %s:%d failed\n", 
				Progname, GCFHost, Port); 
			gcf2ew_die(-1, "Initial Socket Connection Failure" );
		}
		/* ask for data */
		gcf_udp_client_sendcmd(gcf_socket_fd, GCF_SEND_DATA);

	        logit("et", "%s: gcfserv Connect with %s:%d\n", Progname, GCFHost, Port);
	} else {
	    /* try and open up a serial port */
	    if ( (gcf_socket_fd = gcf_open(SerialPort, BaudRate)) == -1 ) {
		logit("et", "%s: Could not open serial port %s at %d at startup\n", 
				Progname, SerialPort, BaudRate);
		gcf2ew_die(-1, "Initial Serial Port Connection Failure" );
	    }
	    input_type = GCF_INPUT_SERIAL;
	}

	/* otherwise we have a connected file descriptor ! */
	gcf_turn_off_ACK_flush();


	/* main thread loop acquiring DATA and LOG MSGs from the GURALP using GCFTOOLS lib*/
	first = 0;
	last_block_no = 0;
	while (okay) {
	    /* EARTHWORM see if we are being told to stop */
	    if ( tport_getflag( &Region ) == TERMINATE ||
                 tport_getflag( &Region ) == mypid        ) {
			gcf2ew_die(GCF2EW_DEATH_EW_TERM,
				"Earthworm TERMINATE request");
	    }
	    /* see if we had a problem anywhere in processing the last data */
	    if (ShutMeDown == TRUE) {
			gcf2ew_die(err_exit, 
				"gcf2ew kill request or fatal EW error");
	    }

	    /* go get some GCF data */
	    switch (input_type) {
		case GCF_INPUT_SERIAL:
	    		block_no = gcf_read_s(gcf_socket_fd, 
					&serial_buf, &packet_size);
			break;
		case GCF_INPUT_MSS:
	    		block_no = gcf_socket_read(gcf_socket_fd, 	
					&serial_buf, &packet_size, GCF_TIMEOUT);
			break;
		case GCF_INPUT_GCFSERV:
			recv_item_type = gcf_udp_client_recv(gcf_socket_fd, 
					&void_ptr);
			break;
	    }


	    /* deal with the input data formatting issues */
	    switch (input_type) {
		case GCF_INPUT_SERIAL:
		case GCF_INPUT_MSS:
	    	    if (block_no != -1) {
			blocks_read++;
			if(blocks_read%REPORT_INTERVAL == 0) {
				logit("t", "Summary block report: %ld read, %ld missed\n", 
					blocks_read, blocks_missed);
			}
			expected = last_block_no+1;
			if (first==1 && block_no != expected) {
				/* we missed some packets for whatever reason and were not able
				   to recover them....
				
					issue some statistics too.
				*/
				int missed;
				missed = block_no  - expected;
				if (missed < 0) {
					missed = block_no + 256 - expected;
				}
				blocks_missed += missed;
				logit("et", "%s: missed %d packets got block_no %d and expected %d\n", 
					Progname, missed, block_no, expected);
				logit("et", "%s: read %ld blocks and lost approx %ld\n", 
					Progname, blocks_read, blocks_missed);
			}
			first=1;
			last_block_no = block_no;
			if (Verbose) {
				fprintf(stdout, "Block number %03d ", block_no);
			}

			if (gcfhdr_read_s(serial_buf, &hdr, GCFTP, packet_size) == -1) {
				/* packet error - bad data sent */
				logit("et", "%s: bad packet error: %s", Progname, gerror_str());
				continue;
			}
	    	    } else {
			/* we may need to remove this message or we will choke the logs */
			if (Host == NULL) {
				logit("et", "%s gcf_socket_read() fails, sleeping 2 secs and then retrying\n",  
					Progname);
				sleep_ew(2000);
				continue;
			} else {
				logit("et", "%s gcf_read_s() timesout and fails, retrying\n", Progname);
				continue;
			}
	    	}
		break;	/* end of case for processing SERIAL or MSS */
		case GCF_INPUT_GCFSERV:
			if (recv_item_type == -1) {
				/* nothing to recv() */
				sleep_ew(1000);
				continue;
			}
			renew_udp_packet_counter++;	
			total_udp_packet_counter++;
			if(total_udp_packet_counter%REPORT_INTERVAL == 0) {
				logit("t", "Summary udp packet report: %ld read, %ld missed\n", 
					total_udp_packet_counter, 
					total_dropped_udp_packet_counter
					);
			}
			if (renew_udp_packet_counter > UDP_SERVER_RENEW) {
				gcf_udp_client_sendcmd(gcf_socket_fd, 	
						GCF_SEND_DATA);
				renew_udp_packet_counter=0;	
			}
			switch (recv_item_type) {
				case GCF_SCREAM:
					gcf_decode_scream_packet(&scream_packet,
						 (char*) void_ptr);
					if (last_sequence!=USHRT_MAX &&
                                        	last_sequence+1 != scream_packet.sequence) {
                                        	total_dropped_udp_packet_counter += scream_packet.sequence - last_sequence + 2;
                                        	logit("et", "Warning, UDP sequence numbers out of sync: data out of order or being lost! %d total packets lost.\n Contact guralp.com and let them know you are using this tool.\n", total_dropped_udp_packet_counter);
                                        	logit("et", " TCP reconnect not implemented in this tool...yet.\n");
					}
					last_sequence = scream_packet.sequence;
					if ( gcfhdr_read ( scream_packet.gcf_block, &hdr, scream_packet.type ) == -1 ) {
                                        	logit("et", "gcf_get_data(): GCF Decoding error, from recv of SCREAM packet %s\n", gerror_str());
                                        	continue;
                                	}
					break;
				case GCF_CMD:
					gcfcmd=get_udpcmd_num((char *)void_ptr);
					switch(gcfcmd) {
					case GCF_SERVER_CLOSED:
						logit("et", "gcfserv server sent server shutdown command; Terminating.\n");
						ShutMeDown = TRUE;
						continue;
					case GCF_UDP_ACK:
						logit("et", "gcfserv server sent server ack command.\n");
						continue;
					default:
						logit("et", "Gcfserv server sent unknown command %d\n", recv_item_type);
						continue;
						break;
					}
					break;
				default:
					logit("et", "Gcfserv server sent unknown command/packet %d\n", recv_item_type);
					continue;
					break;
			}
	    }

	    /**************************/
	    /* GOOD GCF packet in hdr */
	
	    /* IF WE REACH HERE we have a good packet, see if we want it */
	    if (hdr.sample_rate == 0 && (SaveLOGS == 1 || InjectSOH ==1) ) {
		/* save the SOH message to a log or inject into ring */
		char *cptr;
		cptr= (char *)hdr.data;
		*(cptr+hdr.num_samps)='\0';
		if(Verbose) {
			fprintf(stdout, "Processing packet ");
				gcfhdr_print(&hdr, stdout);
		}
		if (SaveLOGS==1) {
			logit("et", "%s STATUS BLOCK: %s", Progname, cptr);
		}
		if (InjectSOH ==1) {
			if (getFirstSCN(hdr.system_id, &scn)) {
				char soh_msg[2048];
				int soh_len;

				logit("et", "InjectSOH: SOH processing for System_id %s as %s-%s\n", 
						hdr.system_id, scn->net, scn->site);
				sprintf(soh_msg, "%s-%s %ld\n%s", scn->net, scn->site, 
					gepoch2uepoch(&(hdr.epoch)), cptr);
				soh_len = strlen(soh_msg);
				if ( tport_putmsg(&Region, &SOHLogo, (long) soh_len, soh_msg) != PUT_OK) {
					logit("et", "%s: Fatal Error sending SOH via tport_putmsg()\n",
						Progname);
					ShutMeDown = TRUE;
					err_exit = GCF2EW_DEATH_EW_PUTMSG;
					continue;
			 	} 
				if (Verbose) {
					logit("et", "InjectSOH: SOH sent for System_id %s as %s-%s\n", 
						hdr.system_id, scn->net, scn->site);
				}
			} else {
				logit("et", "InjectSOH: SCN not found for System_id %s\n", hdr.system_id);
			}
		}
		continue;
	    }

	    /* see if this is an SCN we actually want */
	    if (getSCN(hdr.system_id, hdr.stream_id, &scn) != 1) {
		/* we don't want this packet */
		/* log it if in verbose mode */
		if(Verbose) {
			fprintf(stdout, "Discarding packet ");
				gcfhdr_print(&hdr, stdout);
		}
		continue;
		}
		if(Verbose) {
			fprintf(stdout, "Processing packet ");
			gcfhdr_print(&hdr, stdout);
			if (UseTraceBuf2)  {
				fprintf(stdout, " converting to SCNL %s.%s.%s.%s \n", scn->site, scn->chan, scn->net, scn->loc);
			} else {
				fprintf(stdout, " converting to SCN %s.%s.%s \n",  scn->site, scn->chan, scn->net);
			}
		}
			
		if ((ew_trace_buf = convert_gcf_to_ewtrace(&hdr, scn, &ew_trace_len)) == NULL) { 
			/* an error has occurred in the conversion */
			logit("et", "%s: Error converting gcf trace to ew packet in convert_gcf_to_ewtrace()\n", Progname);

		} else {
			    /* transport it off to EW */
			time(&TSLastGCFData); /* set the time since the last GCF data packet sent to Earthworm */
			if ( tport_putmsg(&Region, &DataLogo, (long) ew_trace_len, ew_trace_buf) != PUT_OK) {
				logit("et", "%s: Fatal Error sending trace via tport_putmsg()\n",
					Progname);
				ShutMeDown = TRUE;
				err_exit = GCF2EW_DEATH_EW_PUTMSG;
			 } 
		}
	}
	/* should never reach here! */
	gcf2ew_die( -1, "clean exit" );
	exit(0);
}

/************************************************************************/
/* signal handler that intiates a shutdown */
void initiate_termination(int sigval) {
    signal(sigval, initiate_termination);
    ShutMeDown = TRUE;
    err_exit = GCF2EW_DEATH_SIG_TRAP;

    if (GCFHost != NULL) {
	gcf_udp_client_sendcmd(gcf_socket_fd, GCF_STOP_DATA);
    }
    if (gcf_socket_fd != -1) {
    	close(gcf_socket_fd);
    }
    return;
}

