/*************************************************************************
 *                                                                       *
 *  nq2ring - NetQuakes MiniSEED to earthworm ring                       *
 *                                                                       *
 *  This code requires the QLIB2 library of Univ. California Berkeley,   *
 *  which may be obtained at the ftp site provided by Berkeley:          *
 *  ftp://quake.geo.berkeley.edu                                         *
 *                                                                       *
 *  Author: Jim Luetgert                                                 * 
 *                                                                       *
 *  COMMENTS:                                                            *
 *                                                                       *
 *                                                                       *
 *  SEE: main.h for externs and all includes.                            *
 *                                                                       *
 *************************************************************************/

#define MAIN
#define THREAD_STACK_SIZE 8192
#include "main.h"

char   progname[] = "nq2ring";
char   NetworkName[TRACE_NET_LEN];     /* Network name. Constant from trace_buf.h   */
char   NQFilesInDir[FILE_NAM_LEN];     /* directory from whence cometh the          */
                                       /* evt input files */
char   NQFilesOutDir[FILE_NAM_LEN];    /* directory to which we put processed files */
char   NQFilesErrorDir[FILE_NAM_LEN];  /* directory to which we put problem files   */
char  InRingName[20];  /* name of transport ring for input  */
char  OutRingName[20]; /* name of transport ring for output */

static int Debug = 0;
int err_exit;				/* an nq2ring_die() error number */
static unsigned char InstId = 255;
pid_t mypid;
MSG_LOGO    OtherLogo, DataLogo;

time_t MyLastInternalBeat;   /* time of last heartbeat into the local Earthworm ring */
unsigned  TidHeart;          /* thread id. was type thread_t on Solaris!             */

int granulated;              /* if = 1, tracebufs always start on integer seconds    */
int delay;
int nchans;
DATABUF chanbuf[MAXCHANS];

/* Functions in this source file
 *******************************/
thr_ret Heartbeat( void * );
int mseed_to_ring_int(int *bytes, DATA_HDR *mseed_hdr, int out_data_size); 
int mseed_to_ring(int *bytes, DATA_HDR *mseed_hdr, int out_data_size); 
void initiate_termination(int );	/* signal handler func */
void setuplogo(MSG_LOGO *);
void nq2ring_die( int errmap, char * str );
void message_send( unsigned char, short, char *);
int GetConfig( char *configfile );

/*************************************************************************
 *  main( int argc, char **argv )                                        *
 *************************************************************************/

int main( int argc, char **argv )
{
	int i, sta_index, scan_station;		/* indexes */
	unsigned char alert, okay;			/* flags */
	int ret_val;                        /* DEBUG return values */
	char *ew_trace_buf;	                /* earthworm trace buffer pointer */
	long  ew_trace_len;                 /* length in bytes of the trace buffer */
    char    whoami[50], subname[] = "Main";
    FILE   *fp;
    char    fname[FILE_NAM_LEN];         /* name of ascii version of evt file */
    char    outFname[FILE_NAM_LEN];      /* Full name of output file */
    char    errbuf[100];                 /* Buffer for error messages */
    int     FileOK, rc;
    DATA_HDR    mseed_hdr, *mseed_hdr_ptr;
    int         max_num_points, msdata[20000];

	/* init some locals */
	alert = FALSE;
	okay  = TRUE;
	err_exit = -1;
	mypid = getpid();       /* set it once on entry */
    if( mypid == -1 ) {
        fprintf( stderr,"%s Cannot get pid. Exiting.\n", whoami);
        return -1;
    }
    sprintf(whoami, "%s: %s: ", progname, "Main");

	/* init some globals */
	Verbose = FALSE;	/* not well used */
	ShutMeDown = FALSE;
	Region.mid = -1;	/* init so we know if an attach happened */
	InRegion.mid = -1;	/* init so we know if an attach happened */
	OutRegion.mid = -1;	/* init so we know if an attach happened */

    /* Check command line arguments
     ******************************/
    if ( argc != 2 ) {
        fprintf( stderr, "Usage: %s <configfile>\n", progname);
        exit( 0 );
    }

    /* Read the configuration file(s)
     ********************************/
	if (GetConfig(argv[1]) == -1) 
		nq2ring_die(Q2EW_DEATH_EW_CONFIG,"Too many q2ew.d config problems");
	
	/* deal with some signals */
	signal(SIGINT, initiate_termination);
	signal(SIGTERM, initiate_termination);

	/* start EARTHWORM logging */
	logit_init(argv[1], (short) QModuleId, 256, LogFile);
	logit("e", "%s: Read in config file %s\n", progname, argv[1]);

	/* EARTHWORM init earthworm connection at this point, 
		this func() exits if there is a problem 
	*/

	/* Attach to Input shared memory ring
	 ************************************/
	tport_attach( &InRegion, InRingKey );
	logit( "e", "%s Attached to public memory region %s: %d\n",
	      whoami, InRingName, InRingKey );

	/* Attach to Output shared memory ring
	 *************************************/
	tport_attach( &OutRegion, OutRingKey );
	logit( "e", "%s Attached to public memory region %s: %d\n",
	      whoami, OutRingName, OutRingKey );


   /* Start the heartbeat thread
   ****************************/
    time(&MyLastInternalBeat); /* initialize our last heartbeat time */
                          
    if ( StartThread( Heartbeat, (unsigned)THREAD_STACK_SIZE, &TidHeart ) == -1 ) {
        logit( "et","%s Error starting Heartbeat thread. Exiting.\n", whoami );
		nq2ring_die(err_exit, "Error starting Heartbeat thread");
        return -1;
    }

	/* sleep for 2 seconds to allow heart to beat so statmgr gets it.  */
	
	sleep_ew(2000);

    /* Change working directory to "NQFilesInDir" - where the files should be
     ************************************************************************/
	if ( chdir_ew( NQFilesInDir ) == -1 ) {
		logit( "e", "Error. Can't change working directory to %s\n Exiting.", NQFilesInDir );
		return -1;
	}

	/* main thread loop acquiring DATA and LOG MSGs from COMSERV */
	while (okay) {
	    /* Start of working loop over files.
	    ************************************/
	    while ((GetFileName (fname)) != 1 && okay) {    
	        /* Open the file for reading only.  Since the file name
	        is in the directory we know the file exists, but it
	        may be in use by another process.  If so, fopen_excl()
	        will return NULL.  In this case, wait a second for the
	        other process to finish and try opening the file again.
	        ******************************************************/
			if( strcmp(fname,"core")==0 ) {
				logit("et", "%s Deleting core file <%s>; \n", whoami, fname );
				if( remove( fname ) != 0) {
					logit("et", "%s Cannot delete core file <%s>; exiting!", whoami, fname );
					break;
				}
				continue;
			}
	        if ((fp = fopen(fname, "rb" )) == NULL) {
	            logit ("e", "Could not open file <%s>\n", fname);
	            continue;
	        }
			else {
				if(Debug) {
				}
				logit("et", "\n\n ************************************************************ \n");
				logit("et", "\n *** Begin Processing file %s *** \n", fname);
				logit("et", "\n ************************************************************ \n");
				
			}

	        /* Prime the data buffers
	        *************************/
	        nchans = 0;
	        for(i=0;i<MAXCHANS;i++) {
	        	chanbuf[i].bufptr = 0;
	        	strcpy(chanbuf[i].sncl, " ");
	        }

	        /* Read the mseed file
	        ************************/
			FileOK = TRUE;
	        max_num_points = 10000;
	        mseed_hdr_ptr = &mseed_hdr;
	        while ((rc=read_ms (&mseed_hdr_ptr,  msdata, max_num_points, fp )) != EOF) {
	            if(rc >= 0) {
					if(Debug) {
						logit("et", "\n *** Processing file %s, Station %s *** \n", 
											fname, mseed_hdr_ptr->station_id);
						logit("e", "rc: %d seq_no: %d S_C_N_L: %s_%s_%s_%s  nsamp: %d %d %d %c\n\n", 
								rc, mseed_hdr_ptr->seq_no, mseed_hdr_ptr->station_id, mseed_hdr_ptr->channel_id, 
								mseed_hdr_ptr->network_id, mseed_hdr_ptr->location_id, mseed_hdr_ptr->num_samples, 
								mseed_hdr_ptr->sample_rate, mseed_hdr_ptr->data_type, mseed_hdr_ptr->record_type );
	                }
					if (rc == 0) continue;	/* No data in record */
	                if(granulated) {
						if(mseed_to_ring_int(msdata, mseed_hdr_ptr, 1000) == 0) {
						
						}
	                }
	                else {
						if(mseed_to_ring(msdata, mseed_hdr_ptr, 1000) == 0) {
						
						}
	                }
	            }
	            else {
					strcpy(errbuf, " ");
					if(rc == -2) strcpy(errbuf, "(MiniSEED error)");
					if(rc == -3) strcpy(errbuf, "(Malloc error)");
					if(rc == -4) strcpy(errbuf, "(Time error)");
					logit("e", "\n *** Error Processing file %s, Error: %d %s ***\n", 
										fname, rc, errbuf);
					FileOK = FALSE;
					if(rc < -1) break;
	            }
	        } 

	        
	        fclose (fp);
	        
	        /* Dispose of file
	        *********************/
	        if (FileOK) {
	            /* all ok; move the file to the output directory */

	            sprintf (outFname, "%s/%s", NQFilesOutDir, fname);
	            if ( rename( fname, outFname ) != 0 ) {
	                logit( "e", "Error moving %s: %s\n", fname, strerror(errno));
	                return -1;
	            }
	        }
	        else {      /* something blew up on this file. Preserve the evidence */
	            logit("e","Error processing file %s\n",fname);

	            /* move the file to the error directory */
	            sprintf (outFname, "%s/%s", NQFilesErrorDir, fname);

	            if (rename (fname, outFname) != 0 ) {
	                logit( "e", "Fatal: Error moving %s: %s\n", fname, strerror(errno));
	                return(-1);
	            } 
	            continue;
	        }
	        
			logit("et","Done with file %s\n",fname);
		}
	        
	    
	    /* EARTHWORM see if we are being told to stop */
	    if ( tport_getflag( &InRegion ) == TERMINATE  ) {
			nq2ring_die(Q2EW_DEATH_EW_TERM, "Earthworm global TERMINATE request");
	    }
	    if ( tport_getflag( &InRegion ) == mypid        ) {
			nq2ring_die(Q2EW_DEATH_EW_TERM, "Earthworm pid TERMINATE request");
	    }
	    /* see if we had a problem anywhere in processing the last data */
	    if (ShutMeDown == TRUE) {
			nq2ring_die(err_exit, "q2ew kill request or fatal EW error");
	    }
		sleep_ew(2000);
	}
	/* should never reach here! */
	    
	logit("et","Done for now.\n");
 
	nq2ring_die( -1, "clean exit" );
	
	exit(0);
}

double    endtime;

/***************************** Heartbeat **************************
 *           Send a heartbeat to the transport ring buffer        *
 ******************************************************************/

thr_ret Heartbeat( void *dummy )
{
    time_t now;

   /* once a second, do the rounds.  */
    while ( 1 ) {
        sleep_ew(1000);
        time(&now);

        /* Beat our heart (into the local Earthworm) if it's time
        ********************************************************/
        if (difftime(now,MyLastInternalBeat) > (double)HeartBeatInt) {
            message_send( TypeHB, 0, "" );
            time(&MyLastInternalBeat);
        }
    }
}

/********************************************************************
 *  mseed_to_ring_int converts MiniSEED to TraceBuf2 and            *
 *  writes to ring with tracebufs starting on integer seconds.      *
 ********************************************************************/

static TracePacket trace_buffer;

int mseed_to_ring_int(int *bytes, DATA_HDR *mseed_hdr, int out_data_size) 
{
    TRACE2_HEADER   *trace2_hdr;		/* ew trace header */
    int              i, j, k, offset, out_message_size;
    int*             inpTracePtr;
    char*            myTracePtr;
    char             sncl[20];

	/*** If we have a log channel, return! ***/
	if (mseed_hdr->sample_rate == 0 || 
	    mseed_hdr->sample_rate_mult == 0 ||
	    mseed_hdr->num_samples == 0)  {
		return 1;
	}

	/*** Pre-fill some of the tracebuf header ***/
	trace2_hdr = (TRACE2_HEADER *) &trace_buffer.trh;
	memset((void*) trace2_hdr, 0, sizeof(TRACE2_HEADER));
	trace2_hdr->pinno = 0;		/* Unknown item */
	trace2_hdr->samprate = sps_rate(mseed_hdr->sample_rate,mseed_hdr->sample_rate_mult);
	strcpy(trace2_hdr->sta,trim(mseed_hdr->station_id));
	strcpy(trace2_hdr->net,trim(mseed_hdr->network_id));
	strcpy(trace2_hdr->chan,trim(mseed_hdr->channel_id));
	strcpy(trace2_hdr->loc,trim(mseed_hdr->location_id));
	if (0 == strncmp(trace2_hdr->loc, "  ", 2) || 0 == memcmp(trace2_hdr->loc, "\000\000", 2))
		strcpy(trace2_hdr->loc,"--");
	sprintf(sncl, "%s_%s_%s_%s", trace2_hdr->sta,trace2_hdr->net,trace2_hdr->chan,trace2_hdr->loc);
	
	strcpy(trace2_hdr->datatype,(my_wordorder == SEED_BIG_ENDIAN) ? "s4" : "i4");
			trace2_hdr->version[0]=TRACE2_VERSION0;
			trace2_hdr->version[1]=TRACE2_VERSION1;
	trace2_hdr->quality[1] = (char)mseed_hdr->data_quality_flags;
	
	/*** Move the data into the floating buffer. ***/
	k = -1;
	for(i=0;i<nchans;i++) {
		if(strcmp(chanbuf[i].sncl, sncl) == 0) k = i;
	}
	if(k == -1) {
		/*** New channel. Start floating buffer at next integer second. ***/
		k = nchans++;
		strcpy(chanbuf[k].sncl, sncl);
		offset = ((USECS_PER_SEC - mseed_hdr->begtime.usec)*trace2_hdr->samprate)/USECS_PER_SEC;
		for(i=offset,j=0;i<mseed_hdr->num_samples;i++,j++) chanbuf[k].buf[j] = bytes[i];
		chanbuf[k].bufptr = j;
		chanbuf[k].starttime = (double)unix_time_from_int_time(mseed_hdr->begtime) + 1;
		chanbuf[k].endtime   = chanbuf[k].starttime + (double)(trace2_hdr->nsamp - 1)/trace2_hdr->samprate;
		chanbuf[k].endtime   = chanbuf[k].endtime - 1;
	} else {
		for(i=0,j=chanbuf[k].bufptr;i<mseed_hdr->num_samples;i++,j++) chanbuf[k].buf[j] = bytes[i];
		chanbuf[k].bufptr = j;
	}
	
	if(Debug) {
		trace2_hdr->starttime = (double)unix_time_from_int_time(mseed_hdr->begtime) +
					((double)(mseed_hdr->begtime.usec)/USECS_PER_SEC);
		trace2_hdr->endtime = (double)unix_time_from_int_time(mseed_hdr->endtime) +
					((double)(mseed_hdr->endtime.usec)/USECS_PER_SEC);
		logit("et", "SENT to EARTHWORM %s_%s_%s_%s %d Time: %f  %f \n", 
				trace2_hdr->sta, trace2_hdr->chan, trace2_hdr->net, trace2_hdr->loc, 
				mseed_hdr->num_samples, 
				trace2_hdr->starttime, trace2_hdr->endtime);
	}
	
	while(chanbuf[k].bufptr > trace2_hdr->samprate) {
		
		trace2_hdr->nsamp     = trace2_hdr->samprate;
		trace2_hdr->starttime = chanbuf[k].starttime;
		endtime = chanbuf[k].endtime;
		chanbuf[k].endtime    = chanbuf[k].starttime + (double)(trace2_hdr->nsamp - 1)/trace2_hdr->samprate;
		trace2_hdr->endtime   = chanbuf[k].endtime;

		myTracePtr = &trace_buffer.msg[0] + sizeof(TRACE2_HEADER);        
		inpTracePtr = &chanbuf[k].buf[0];        
	
		/* Move the trace */
		memcpy( (void*)myTracePtr, (void*)inpTracePtr, trace2_hdr->nsamp*sizeof(int32_t) );
    	
		out_message_size = sizeof(TRACE2_HEADER)+sizeof(int)*trace2_hdr->nsamp;
		
		if(Debug) {
			logit("et", "SENT to EARTHWORM %s_%s_%s_%s %d Time: %f  %f %f %f %d %d %d\n", 
					trace2_hdr->sta, trace2_hdr->chan, 
					trace2_hdr->net, trace2_hdr->loc, trace2_hdr->nsamp, 
					trace2_hdr->starttime, trace2_hdr->endtime, trace2_hdr->endtime-trace2_hdr->starttime, 
					trace2_hdr->starttime-endtime, out_message_size, chanbuf[k].bufptr, j-i);
		}
		/* transport it off to EW */
		if ( tport_putmsg(&OutRegion, &DataLogo, (long) out_message_size, (char*)&trace_buffer) != PUT_OK) {
			logit("et", "%s: Fatal Error sending trace via tport_putmsg()\n", progname);
			ShutMeDown = TRUE;
			err_exit = Q2EW_DEATH_EW_PUTMSG;
		} else if ( Verbose==TRUE ) {
			fprintf(stderr, "SENT to EARTHWORM %s_%s_%s_%s %d Time: \n", 
					trace2_hdr->sta, trace2_hdr->chan, 
					trace2_hdr->net, trace2_hdr->loc, trace2_hdr->nsamp);
		}
		
		/* Reset the floating buffer */
		for(i=0,j=trace2_hdr->nsamp;j<chanbuf[k].bufptr;i++,j++) chanbuf[k].buf[i] = chanbuf[k].buf[j];
		chanbuf[k].bufptr -= trace2_hdr->nsamp;
		chanbuf[k].starttime += 1;
		
		/* sleep for a while so that we don't overwhelm the wave ring.
		*/
		sleep_ew(delay);

    }
    
	if(Debug) logit("et", "Done sending to EARTHWORM  \n"); 
         
	return 0;
} 

/********************************************************************
 *  mseed_to_ring converts MiniSEED to TraceBuf2 and writes to ring *
 ********************************************************************/

int mseed_to_ring(int *bytes, DATA_HDR *mseed_hdr, int out_data_size) 
{
    TRACE2_HEADER   *trace2_hdr;		/* ew trace header */
    int              i, j, k, offset, out_message_size;
    int*             inpTracePtr;
    char*            myTracePtr;
    char             sncl[20];

	/* check to see if we have a log channel! */
	if (mseed_hdr->sample_rate == 0 || 
	    mseed_hdr->sample_rate_mult == 0 ||
	    mseed_hdr->num_samples == 0)  {
		return 1;
	}

	trace2_hdr = (TRACE2_HEADER *) &trace_buffer.trh;
	memset((void*) trace2_hdr, 0, sizeof(TRACE2_HEADER));
	trace2_hdr->pinno = 0;		/* Unknown item */
	trace2_hdr->samprate = sps_rate(mseed_hdr->sample_rate,mseed_hdr->sample_rate_mult);
	strcpy(trace2_hdr->sta,trim(mseed_hdr->station_id));
	strcpy(trace2_hdr->net,trim(mseed_hdr->network_id));
	strcpy(trace2_hdr->chan,trim(mseed_hdr->channel_id));
	strcpy(trace2_hdr->loc,trim(mseed_hdr->location_id));
	if (0 == strncmp(trace2_hdr->loc, "  ", 2) || 0 == memcmp(trace2_hdr->loc, "\000\000", 2))
		strcpy(trace2_hdr->loc,"--");
	sprintf(sncl, "%s_%s_%s_%s", trace2_hdr->sta,trace2_hdr->net,trace2_hdr->chan,trace2_hdr->loc);
	
	strcpy(trace2_hdr->datatype,(my_wordorder == SEED_BIG_ENDIAN) ? "s4" : "i4");
			trace2_hdr->version[0]=TRACE2_VERSION0;
			trace2_hdr->version[1]=TRACE2_VERSION1;
	trace2_hdr->quality[1] = (char)mseed_hdr->data_quality_flags;
	
	k = -1;
	for(i=0;i<nchans;i++) {
		if(strcmp(chanbuf[i].sncl, sncl) == 0) k = i;
	}
	if(k == -1) {
		k = nchans++;
		strcpy(chanbuf[k].sncl, sncl);
		offset = ((USECS_PER_SEC - mseed_hdr->begtime.usec)*trace2_hdr->samprate)/USECS_PER_SEC;
		for(i=offset,j=0;i<mseed_hdr->num_samples;i++,j++) chanbuf[k].buf[j] = bytes[i];
		chanbuf[k].bufptr = j;
	} else {
		for(i=0,j=chanbuf[k].bufptr;i<mseed_hdr->num_samples;i++,j++) chanbuf[k].buf[j] = bytes[i];
		chanbuf[k].bufptr = j;
	}
	
	if(Debug) {
		trace2_hdr->starttime = (double)unix_time_from_int_time(mseed_hdr->begtime) +
					((double)(mseed_hdr->begtime.usec)/USECS_PER_SEC);
		trace2_hdr->endtime = (double)unix_time_from_int_time(mseed_hdr->endtime) +
					((double)(mseed_hdr->endtime.usec)/USECS_PER_SEC);
		logit("et", "SENT to EARTHWORM %s_%s_%s_%s %d Time: %f  %f \n", 
				trace2_hdr->sta, trace2_hdr->chan, trace2_hdr->net, trace2_hdr->loc, 
				mseed_hdr->num_samples, 
				trace2_hdr->starttime, trace2_hdr->endtime);
	}
	
	i = 0;
	while(i < mseed_hdr->num_samples) {
		j = i + out_data_size;
		if(j > mseed_hdr->num_samples) j = mseed_hdr->num_samples;
		trace2_hdr->nsamp = j - i;
		/* note that unix_time_from_int_time() does not handle leap_seconds secs=60 should
			this miraculously occur on the start time of a data packet... 
		*/
		trace2_hdr->starttime = (double)unix_time_from_int_time(mseed_hdr->begtime) +
					((double)(mseed_hdr->begtime.usec)/USECS_PER_SEC) + 
					(double)i/trace2_hdr->samprate;
		trace2_hdr->endtime = (double)unix_time_from_int_time(mseed_hdr->begtime) +
					((double)(mseed_hdr->begtime.usec)/USECS_PER_SEC) + 
					(double)(j-1)/trace2_hdr->samprate;
		
		myTracePtr = &trace_buffer.msg[0] + sizeof(TRACE2_HEADER);        
		inpTracePtr = &bytes[0] + i;        
	
		/* Move the trace */
		memcpy( (void*)myTracePtr, (void*)inpTracePtr, trace2_hdr->nsamp*sizeof(int32_t) );
    	
		out_message_size = sizeof(TRACE2_HEADER)+sizeof(int)*trace2_hdr->nsamp;
		
		if(Debug) {
			logit("et", "SENT to EARTHWORM %s_%s_%s_%s %d Time: %f  %f %f %f %d %d %d\n", 
					trace2_hdr->sta, trace2_hdr->chan, 
					trace2_hdr->net, trace2_hdr->loc, trace2_hdr->nsamp, 
					trace2_hdr->starttime, trace2_hdr->endtime, trace2_hdr->endtime-trace2_hdr->starttime, 
					trace2_hdr->starttime-endtime, out_message_size, i, j-i);
			endtime = trace2_hdr->endtime;
		}
		/* transport it off to EW */
		if ( tport_putmsg(&OutRegion, &DataLogo, (long) out_message_size, (char*)&trace_buffer) != PUT_OK) {
			logit("et", "%s: Fatal Error sending trace via tport_putmsg()\n", progname);
			ShutMeDown = TRUE;
			err_exit = Q2EW_DEATH_EW_PUTMSG;
		} else if ( Verbose==TRUE ) {
			fprintf(stderr, "SENT to EARTHWORM %s_%s_%s_%s %d Time: \n", 
					trace2_hdr->sta, trace2_hdr->chan, 
					trace2_hdr->net, trace2_hdr->loc, trace2_hdr->nsamp);
		}
    	i = j;

		/* sleep for a while so that we don't overwhelm
			the wave ring.
		*/
		sleep_ew(delay);

    }
	/* Reset the floating buffer */
	chanbuf[k].bufptr = 0;
    
	if(Debug) logit("et", "Done sending to EARTHWORM  \n"); 
         
	return 0;
} 

/************************************************************************/
/* signal handler that intiates a shutdown                              */
/************************************************************************/
void initiate_termination(int sigval) 
{
    signal(sigval, initiate_termination);
    ShutMeDown = TRUE;
    err_exit = Q2EW_DEATH_SIG_TRAP;
    return;
}





     /***************************************************************
      *                          GetConfig()                        *
      *         Processes command file using kom.c functions.       *
      *           Returns -1 if any errors are encountered.         *
      ***************************************************************/

#define NCOMMAND 8             /* Number of commands in the config file */

int GetConfig( char *configfile )
{
   const int ncommand = NCOMMAND;

   char     init[NCOMMAND];     /* Flags, one for each command */
   int      nmiss;              /* Number of commands that were missed */
   int      nfiles;
   int      i;

/* Set to zero one init flag for each required command
   ***************************************************/
   for ( i = 0; i < ncommand; i++ )
      init[i] = 0;
   Debug = 0;
   granulated = 0;
   delay = 200;

/* Open the main configuration file
   ********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 )
   {
      fprintf(stderr, "%s: Error opening configuration file <%s>\n", progname, configfile );
      return -1;
   }

/* Process all nested configuration files
   **************************************/
   while ( nfiles > 0 )          /* While there are config files open */
   {
      while ( k_rd() )           /* Read next line from active file  */
      {
         int  success;
         char *com;
         char *str;

         com = k_str();          /* Get the first token from line */

         if ( !com ) continue;             /* Ignore blank lines */
         if ( com[0] == '#' ) continue;    /* Ignore comments */

/* Open another configuration file
   *******************************/
         if ( com[0] == '@' ) {
            success = nfiles + 1;
            nfiles  = k_open( &com[1] );
            if ( nfiles != success ) {
               fprintf(stderr, "%s: Error opening command file <%s>.\n", progname, &com[1] );
               return -1;
            }
            continue;
         }

/* Read configuration parameters
   *****************************/
  /*0*/     
         if ( k_its( "ModuleId" ) ) {
            if ( (str = k_str()) ) {
               if ( GetModId(str, &QModuleId) == -1 ) {
                  fprintf( stderr, "%s: Invalid ModuleId <%s>. \n", 
				progname, str );
                  fprintf( stderr, "%s: Please Register ModuleId <%s> in earthworm.d!\n", 
				progname, str );
                  return -1;
               }
            }
            init[0] = 1;
   /*1*/     
         } else if ( k_its( "InRingName" ) ) {
            if ( (str = k_str()) != NULL ) {
                if(str) strcpy( InRingName, str );
               if ( (InRingKey = GetKey(str)) == -1 )
               {
                  fprintf( stderr, "%s: Invalid RingName <%s>. \n", 
			progname, str );
                  return -1;
               }
            }
            init[1] = 1;
  /*2*/     
         } else if ( k_its( "OutRingName" ) ) {
            if ( (str = k_str()) != NULL ) {
                if(str) strcpy( OutRingName, str );
               if ( (OutRingKey = GetKey(str)) == -1 )
               {
                  fprintf( stderr, "%s: Invalid RingName <%s>. \n", 
			progname, str );
                  return -1;
               }
            }
            init[2] = 1;
  /*3*/     
         } else if ( k_its( "HeartBeatInt" ) ) {
            HeartBeatInt = k_int();
            init[3] = 1;
  /*4*/     
         } else if ( k_its( "LogFile" ) ) {
            LogFile = k_int();
            init[4] = 1;
            }
  /*5*/     
			else if( k_its("NQFilesInDir") ) {
                str = k_str();
                if(str) strcpy( NQFilesInDir, str );
                init[5] = 1;
            }
    /*6*/     
			else if( k_its("NQFilesOutDir") ) {
                str = k_str();
                if(str) strcpy( NQFilesOutDir, str );
                init[6] = 1;
            }
  /*7*/     
			else if( k_its("NQFilesErrorDir") ) {
                str = k_str();
                if(str) strcpy( NQFilesErrorDir, str );
                init[7] = 1;
            }
			else if ( k_its( "Granulate" ) ) {
				granulated = 1;
			 }  
			else if ( k_its( "Delay" ) ) {
				delay = k_int();
				if(delay <    5) delay =    5;
				if(delay > 1000) delay = 1000;
			 }  
			else if ( k_its( "Debug" ) ) {
				Debug = 1;
				/* turn on the LogFile too! */
				LogFile = 1;
			 }  
		else {
	    /* An unknown parameter was encountered */
            fprintf( stderr, "%s: <%s> unknown parameter in <%s>\n", 
		progname,com, configfile );
            return -1;
         }

/* See if there were any errors processing the command
   ***************************************************/
         if ( k_err() ) {
            fprintf( stderr, "%s: Bad <%s> command in <%s>.\n", 
		progname, com, configfile );
            return -1;
         }
      }
      nfiles = k_close();
   }

/* After all files are closed, check flags for missed commands
   ***********************************************************/
   nmiss = 0;
	/* note the last argument is optional Debug, hence
	the ncommand-1 in the for loop and not simply ncommand */
   for ( i = 0; i < ncommand-1; i++ )
      if ( !init[i] )
         nmiss++;

   if ( nmiss > 0 ) {
      fprintf( stderr,"%s: ERROR, no ", progname );
      if ( !init[0]  ) fprintf(stderr, "<ModuleId> " );
      if ( !init[1]  ) fprintf(stderr, "<InRingName> " );
      if ( !init[2]  ) fprintf(stderr, "<OutRingName> " );
      if ( !init[3] ) fprintf(stderr, "<HeartBeatInt> " );
      if ( !init[4] ) fprintf(stderr, "<LogFile> " );
      if ( !init[5] ) fprintf(stderr, "<NQFilesInDir> " );
      if ( !init[6] ) fprintf(stderr, "<NQFilesOutDir> " );
      if ( !init[7] ) fprintf(stderr, "<NQFilesErrorDir> " );
      fprintf(stderr, "command(s) in <%s>.\n", configfile );
      return -1;
   }
	
   if ( GetType( "TYPE_HEARTBEAT", &TypeHB ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid message type <TYPE_HEARTBEAT>\n",progname);
      return( -1 );
   }
   if ( GetType( "TYPE_TRACEBUF", &TypeTrace ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid message type <TYPE_TRACEBUF>; exiting!\n", progname);
        return(-1);
   }
   if ( GetType( "TYPE_TRACEBUF2", &TypeTrace2 ) != 0 ) {
      fprintf( stderr,
              "%s: Message type <TYPE_TRACEBUF2> not found in earthworm_global.d; exiting!\n", progname);
        return(-1);
   } 
   if ( GetType( "TYPE_ERROR", &TypeErr ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid message type <TYPE_ERROR>\n", progname);
      return( -1 );
   }
/* build the datalogo */
   setuplogo(&DataLogo);
       DataLogo.type=TypeTrace2;
   setuplogo(&OtherLogo);

   return 0;
}


/********************************************************************
 *  setuplogo initializes logos                                     *
 ********************************************************************/

void setuplogo(MSG_LOGO *logo) 
{
   /* only get the InstId once */
   if (InstId == 255  && GetLocalInst(&InstId) != 0) {
      fprintf( stderr,
              "%s: Invalid Installation code; exiting!\n", progname);
      exit(-1);
   }
   logo->mod = QModuleId;
   logo->instid = InstId;
}


/********************************************************************
 *  nq2ring_die attempts to gracefully die                          *
 ********************************************************************/

void nq2ring_die( int errmap, char * str ) {

	if (errmap != -1) {
		/* use the statmgr reporting to notify of this death */
#ifdef DEBUG
		fprintf(stderr, "SENDING MESSAGE to statmgr: %d %s\n", errmap, str);
#endif 
		message_send(TypeErr, errmap, str);
	}
	
	/* this next bit must come after the possible tport_putmsg() above!! */
	if (InRegion.mid != -1) {
		/* we attached to an EW ring buffer */
		logit("e", "%s: exiting because %s\n", progname, str);
		tport_detach( &InRegion );
	}
	if (OutRegion.mid != -1) {
		/* we attached to an EW ring buffer */
		logit("e", "%s: exiting because %s\n", progname, str);
		tport_detach( &OutRegion );
	}

	exit(0);
}


/********************************************************************
 *  message_send() builds a heartbeat or error message & puts it    *
 *               into shared memory.  Writes errors to log file.    *
 ********************************************************************/
 
void message_send( unsigned char type, short ierr, char *note )
{
    time_t t;
    char message[256];
    long len;

    OtherLogo.type  = type;

    time( &t );
    /* put the message together */
    if( type == TypeHB ) {
       sprintf( message, "%ld %ld\n", t, (long)mypid);
    } else if( type == TypeErr ) {
       sprintf( message, "%ld %hd %s\n", t, ierr, note);
       logit( "et", "%s: %s\n", progname, note );
    }
    len = strlen( message );   /* don't include the null byte in the message */

#ifdef DEBUG
		fprintf(stderr, "message_send: %ld %s\n", len, message);
#endif 
   /* write the message to shared memory */
    if( tport_putmsg( &OutRegion, &OtherLogo, len, message ) != PUT_OK ) {
        if( type == TypeHB ) {
           logit("et","%s:  Error sending heartbeat.\n", progname );
        }
        else if( type == TypeErr ) {
           logit("et","%s:  Error sending error:%d.\n", progname, ierr );
        }
    }

   return;
}


