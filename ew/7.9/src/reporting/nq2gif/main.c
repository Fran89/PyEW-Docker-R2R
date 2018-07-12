/*************************************************************************
 *                                                                       *
 *  nq2gif - NetQuakes MiniSEED to gif images                            *
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

char   progname[] = "nq2gif";
char   NetworkName[TRACE_NET_LEN];     /* Network name. Constant from trace_buf.h */
char   NQFilesInDir[FILE_NAM_LEN];     /* directory from whence cometh the */
                                       /* evt input files */
char   NQFilesOutDir[FILE_NAM_LEN];    /* directory to which we put processed files */
char   NQFilesErrorDir[FILE_NAM_LEN];  /* directory to which we put problem files */
char  InRingName[20];                  /* name of transport ring for input  */

int Debug;
int err_exit;				/* an nq2gif_die() error number */
static unsigned char InstId = 255;
pid_t mypid;
MSG_LOGO    OtherLogo;

time_t MyLastInternalBeat;      /* time of last heartbeat into the local Earthworm ring    */
unsigned  TidHeart;             /* thread id. was type thread_t on Solaris! */

/* Other globals
 ***************/
#define   XLMARGIN   1.2             /* Margin to left of axes                  */
#define   XRMARGIN   1.0             /* Margin to right of axes                 */
#define   YBMARGIN   0.8             /* Margin at bottom of axes                */
#define   YTMARGIN   0.9             /* Margin at top of axes plus size of logo */
double    YBMargin = 0.8;            /* Margin at bottom of axes                */
double    YTMargin = 0.7;            /* Margin at top of axes                   */


/* Functions in this source file
 *******************************/
thr_ret Heartbeat( void * );
int mseed_audit(int *bytes, DATA_HDR *mseed_hdr, Global *But);
int set_parms(Global *But); 
int Build_Axes(Global *But); 
void Pallette(gdImagePtr GIF, long color[]);
int mseed_plot(int *bytes, DATA_HDR *mseed_hdr, Global *But); 
int Plot_Trace(Global *But, int this, int *Data, double mint);

void Save_Plot(Global *But);
void CommentList(Global *But);
void initiate_termination(int );	/* signal handler func */
void Get_Sta_Info(Global *);
int GetConfig( char *configfile, Global *But );
void setuplogo(MSG_LOGO *);
void nq2gif_die( int errmap, char * str );
void message_send( unsigned char, short, char *);
void Decode_Time( double, TStrct *);
void Encode_Time( double *secs, TStrct *Time);
void date22( double, char *);
void date11( double, char *);

int         max_num_points, msdata[20000];

/*************************************************************************
 *  main( int argc, char **argv )                                        *
 *************************************************************************/

int main( int argc, char **argv )
{
	int i, j, sta_index, scan_station;	/* indexes */
	unsigned char alert, okay;			/* flags */
	int ret_val;				        /* DEBUG return values */
	char *ew_trace_buf;			        /* earthworm trace buffer pointer */
	long  ew_trace_len;			        /* length in bytes of the trace buffer */
    char    whoami[50], subname[] = "Main";
    FILE   *fp;
    char    fname[FILE_NAM_LEN];        /* name of ascii version of evt file */
    char    outFname[FILE_NAM_LEN];     /* Full name of output file          */
    char    errbuf[100];                /* Buffer for error messages         */
    int     FileOK, rc;
    DATA_HDR    mseed_hdr, *mseed_hdr_ptr;
    Global BStruct;  

	/* init some locals */
	alert = FALSE;
	okay  = TRUE;
	err_exit = -1;
	mypid = getpid();       /* set it once on entry */
    if( mypid == -1 ) {
        fprintf( stderr,"%s Cannot get pid. Exiting.\n", whoami);
        return -1;
    }
    strcpy(BStruct.mod, progname);
    sprintf(whoami, "%s: %s: ", progname, "Main");

	/* init some globals */
	Verbose    = FALSE;	/* not well used */
	ShutMeDown = FALSE;
	Region.mid   = -1;	/* init so we know if an attach happened */
	InRegion.mid = -1;	/* init so we know if an attach happened */

    /* Check command line arguments
     ******************************/
    if ( argc != 2 ) {
        fprintf( stderr, "Usage: %s <configfile>\n", progname);
        exit( 0 );
    }

    /* Read the configuration file(s)
     ********************************/
	if (GetConfig(argv[1], &BStruct ) == -1) 
		nq2gif_die(Q2EW_DEATH_EW_CONFIG,"Too many q2ew.d config problems");
	
	/* deal with some signals */
	signal(SIGINT, initiate_termination);
	signal(SIGTERM, initiate_termination);

	/* start EARTHWORM logging */
	logit_init(argv[1], (short) QModuleId, 256, LogFile);
	logit("e", "%s: Read in config file %s\n", progname, argv[1]);

	/* Attach to Input shared memory ring
	 ************************************/
	tport_attach( &InRegion, InRingKey );
	logit( "e", "%s Attached to public memory region %s: %d\n",
	      whoami, InRingName, InRingKey );


    /* Change working directory to "NQFilesInDir" - where the files should be
     ************************************************************************/
	if ( chdir_ew( NQFilesInDir ) == -1 ) {
		logit( "e", "Error. Can't change working directory to %s\n Exiting.", NQFilesInDir );
		return -1;
	}

   /* Start the heartbeat thread
   ****************************/
    time(&MyLastInternalBeat); /* initialize our last heartbeat time */
                          
    if ( StartThread( Heartbeat, (unsigned)THREAD_STACK_SIZE, &TidHeart ) == -1 ) {
        logit( "et","%s Error starting Heartbeat thread. Exiting.\n", whoami );
		nq2gif_die(err_exit, "Error starting Heartbeat thread");
        return -1;
    }

	/* sleep for 2 seconds to allow heart to beat so statmgr gets it.  */
	
	sleep_ew(2000);

	/* main thread loop acquiring DATA and LOG MSGs from COMSERV */
	while (okay) {
		
		Get_Sta_Info(&BStruct);
		
	    /* Start of working loop over files.
	    ************************************/
	    while ((GetFileName (fname)) != 1 && okay) {    

			if(Debug) logit("et", "\n *** Processing file %s *** \n", fname);
			
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
			FileOK = TRUE;
	        max_num_points = 10000;
	        mseed_hdr_ptr = &mseed_hdr;
	        BStruct.Sta.Nchan = 0;
			for(i=0;i<MAXCHAN;i++) {
				BStruct.Sta.SNCL[i].mintime =0.0;
				BStruct.Sta.SNCL[i].maxtime =0.0;
				BStruct.Sta.SNCL[i].sigma =0.0;
				BStruct.Sta.SNCL[i].meta =-1;
				BStruct.Sta.SNCL[i].npts = 0;
				sprintf(BStruct.Sta.SNCL[i].SNCLnam, "xxx_xx_xxx_xx");
			}
	        
	        /* Read the mseed file to get time and channel limits
	        *****************************************************/
	        while ((rc=read_ms (&mseed_hdr_ptr,  msdata, max_num_points, fp )) != EOF) {
	            if(rc >= 0) {
					if(Debug==2) {
					logit("et", "\n *** Processing file %s, Station %s *** \n", 
										fname, mseed_hdr_ptr->station_id);
					logit("e", "rc: %d seq_no: %d S_C_N_L: %s_%s_%s_%s  nsamp: %d %d %d %c\n\n", 
							rc, mseed_hdr_ptr->seq_no, mseed_hdr_ptr->station_id, mseed_hdr_ptr->channel_id, 
							mseed_hdr_ptr->network_id, mseed_hdr_ptr->location_id, mseed_hdr_ptr->num_samples, 
							mseed_hdr_ptr->sample_rate, mseed_hdr_ptr->data_type, mseed_hdr_ptr->record_type );
	                }
					if (rc == 0) continue;	/* No data in record */
	                if(mseed_audit(msdata, mseed_hdr_ptr, &BStruct) == 0) {
	                
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
	        
	        if (FileOK) {
	        /* Build the axes, labels, etc.
	        *****************************************************/
	         
				strncpy(BStruct.FileType, fname, (size_t)3);
				BStruct.FileType[3]  = '\0'; 
  	
	        	set_parms(&BStruct);
	        	
		        if(Build_Axes(&BStruct)) {
		            logit("e", "%s Build_axes croaked for: %s_%s. \n", whoami, BStruct.Sta.Site, BStruct.Sta.Net);
		            break;
		        }
	        
	        /* Read the mseed file again and plot the data
	        *****************************************************/
				if ((fp = fopen(fname, "rb" )) == NULL) {
					logit ("e", "Could not open file <%s>\n", fname);
					continue;
				}
				
				if(Debug) {
				}
				while ((rc=read_ms (&mseed_hdr_ptr,  msdata, max_num_points, fp )) != EOF) {
					if(rc >= 0) {
						if(Debug==2) {
						logit("e", "\n *** Processing file %s, Station %s *** \n", 
											fname, mseed_hdr_ptr->station_id);
						logit("e", "rc: %d seq_no: %d S_C_N_L: %s_%s_%s_%s  nsamp: %d %d %d %c\n\n", 
								rc, mseed_hdr_ptr->seq_no, mseed_hdr_ptr->station_id, mseed_hdr_ptr->channel_id, 
								mseed_hdr_ptr->network_id, mseed_hdr_ptr->location_id, mseed_hdr_ptr->num_samples, 
								mseed_hdr_ptr->sample_rate, mseed_hdr_ptr->data_type, mseed_hdr_ptr->record_type );
						}
						/**/
						if (rc == 0) continue;	/* No data in record */
						if(mseed_plot(msdata, mseed_hdr_ptr, &BStruct) == 0) {
						
						}
						
					}
					else {
						logit("e", "\n *** Error Processing file %s, Error: %d ***\n", fname, rc);
						FileOK = FALSE;
						if(rc < -1) break;
					}
				} 
				fclose (fp);
	        
	        /* Close out the gif image and put it in the output directory
	        *****************************************************/
	        
	        	Save_Plot(&BStruct);
			
	        /* Dispose of file
	        *********************/
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
			
		if( strcmp(fname,"core")==0 ) {
			logit("et", "%s Deleting core file <%s>; \n", whoami, fname );
			if( remove( fname ) != 0) {
				logit("et", "%s Cannot delete core file <%s>; exiting!", whoami, fname );
				okay = FALSE;
				break;
			}
		}
	    
	    /* EARTHWORM see if we are being told to stop */
	    if ( tport_getflag( &InRegion ) == TERMINATE ) {
			nq2gif_die(Q2EW_DEATH_EW_TERM, "Earthworm global TERMINATE request");
	    }
	    if ( tport_getflag( &InRegion ) == mypid        ) {
			nq2gif_die(Q2EW_DEATH_EW_TERM, "Earthworm pid TERMINATE request");
	    }
	    /* see if we had a problem anywhere in processing the last data */
	    if (ShutMeDown == TRUE) {
			nq2gif_die(err_exit, "q2ew kill request or fatal EW error");
	    }
		sleep_ew(2000);
	}
	/* should never reach here! */
	    
	logit("et","Done for now.\n");
 
	nq2gif_die( -1, "clean exit" );
	
	exit(0);
}


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
 *  mseed_audit previews MiniSEED file before plotting              *
 ********************************************************************/

int mseed_audit(int *bytes, DATA_HDR *mseed_hdr, Global *But) 
{
    int    i, j, this, minv, maxv, val;
    char   SNCL[20], Site[6], Net[5], Comp[5], Loc[5];     
    double mint, maxt;

	/* check to see if we have a log channel! */
	if (mseed_hdr->sample_rate == 0 || 
	    mseed_hdr->sample_rate_mult == 0 ||
	    mseed_hdr->num_samples == 0)  {
		return 1;
	}

	strcpy(Site,trim(mseed_hdr->station_id));
	strcpy(Net, trim(mseed_hdr->network_id));
	strcpy(Comp,trim(mseed_hdr->channel_id));
	strcpy(Loc, trim(mseed_hdr->location_id));
	if (0 == strncmp(Loc, "  ", 2) || 0 == memcmp(Loc, "\000\000", 2)) strcpy(Loc,"--");
		
	strcpy(But->Sta.Site,Site);
	strcpy(But->Sta.Net,Net);
	strcpy(But->Sta.Loc,Loc);
	sprintf(SNCL, "%s_%s_%s_%s", Site, Net, Comp, Loc);
	mint = (double)unix_time_from_int_time(mseed_hdr->begtime) +
					((double)(mseed_hdr->begtime.usec)/USECS_PER_SEC);
	maxt = (double)unix_time_from_int_time(mseed_hdr->endtime) +
					((double)(mseed_hdr->endtime.usec)/USECS_PER_SEC);
	this = -1;
	for(i=0;i<But->Sta.Nchan;i++) {
		if(strcmp(SNCL, But->Sta.SNCL[i].SNCLnam)==0) {this = i; break;}
	}
	if(this<0) {
		if(But->Sta.Nchan < MAXCHAN) this = But->Sta.Nchan++;
		else {logit("e", "mseed_audit: Too many channels in this file!\n"); this = But->Sta.Nchan;}
		strcpy(But->Sta.SNCL[this].Site,Site);
		strcpy(But->Sta.SNCL[this].Net,Net);
		strcpy(But->Sta.SNCL[this].Comp,Comp);
		strcpy(But->Sta.SNCL[this].Loc,Loc);
		sprintf(But->Sta.SNCL[this].SNCLnam, "%s_%s_%s_%s", Site, Net, Comp, Loc);
		val = (int)bytes[0];
		But->Sta.SNCL[this].npts   =   0;
		But->Sta.SNCL[this].minval = val;
		But->Sta.SNCL[this].maxval = val;
		But->Sta.SNCL[this].sigma  = 0.0;
		But->Sta.SNCL[this].mintime = mint;
		But->Sta.SNCL[this].maxtime = maxt;
	}
	
	if(mint < But->Sta.SNCL[this].mintime || But->Sta.SNCL[this].mintime == 0.0) 
				But->Sta.SNCL[this].mintime = mint;
	if(maxt > But->Sta.SNCL[this].maxtime || But->Sta.SNCL[this].maxtime == 0.0) 
				But->Sta.SNCL[this].maxtime = maxt;
	
	But->Sta.SNCL[this].samprate = sps_rate(mseed_hdr->sample_rate,mseed_hdr->sample_rate_mult);
	
	
	But->Sta.SNCL[this].npts += mseed_hdr->num_samples;
	for(i=0;i<mseed_hdr->num_samples;i++) {
		val = (int)bytes[i];
		if(But->Sta.SNCL[this].minval > val) But->Sta.SNCL[this].minval = val;
		if(But->Sta.SNCL[this].maxval < val) But->Sta.SNCL[this].maxval = val;
		But->Sta.SNCL[this].sigma += val;
	}
	    
	if(Debug == 2) {
		logit("e", "mseed_audit: S_C_N_L: %s  nsamp: %d samprate: %f sigma: %f\n", 
				But->Sta.SNCL[this].SNCLnam, But->Sta.SNCL[this].npts, 
				But->Sta.SNCL[this].samprate, But->Sta.SNCL[this].sigma );
		logit("e", "mseed_audit: mintime: %f maxtime: %f \n             minval: %d maxval: %d Mean: %f\n\n", 
				But->Sta.SNCL[this].mintime, But->Sta.SNCL[this].maxtime, 
				But->Sta.SNCL[this].minval, But->Sta.SNCL[this].maxval,
				But->Sta.SNCL[this].sigma/But->Sta.SNCL[this].npts );
	}
         
	return 0;
} 

/********************************************************************
 *  set_parms sets up values needed for plotting                    *
 ********************************************************************/

int set_parms(Global *But) 
{
    int    i, j, maxp, maxm;
    
	But->Sta.mintime = But->Sta.maxtime = But->Sta.SNCL[0].mintime;
	But->Sta.minval  = But->Sta.maxval  = But->Sta.SNCL[0].minval;
	But->Sta.range   = But->Sta.SNCL[0].maxval-But->Sta.SNCL[0].minval;
	But->Sta.maxoff   = abs(But->Sta.SNCL[0].maxval-But->Sta.SNCL[0].sigma/But->Sta.SNCL[0].npts);
	for(i=0;i<But->Sta.Nchan;i++) {
		But->Sta.SNCL[i].mean = But->Sta.SNCL[i].sigma/But->Sta.SNCL[i].npts;
		if(But->Sta.mintime > But->Sta.SNCL[i].mintime) 
					But->Sta.mintime = But->Sta.SNCL[i].mintime;
		if(But->Sta.maxtime < But->Sta.SNCL[i].maxtime) 
					But->Sta.maxtime = But->Sta.SNCL[i].maxtime;

		maxp = abs(But->Sta.SNCL[i].maxval-But->Sta.SNCL[i].mean);
		maxm = abs(But->Sta.SNCL[i].minval-But->Sta.SNCL[i].mean);
		if(But->Sta.maxoff < maxp) But->Sta.maxoff = maxp;
		if(But->Sta.maxoff < maxm) But->Sta.maxoff = maxm;
		
		But->Sta.SNCL[i].range = But->Sta.SNCL[i].maxval-But->Sta.SNCL[i].minval;
		if(But->Sta.range < But->Sta.SNCL[i].range) 
					But->Sta.range = But->Sta.SNCL[i].range;
		if(But->Sta.minval > But->Sta.SNCL[i].minval) 
					But->Sta.minval = But->Sta.SNCL[i].minval;
		if(But->Sta.maxval < But->Sta.SNCL[i].maxval) 
					But->Sta.maxval = But->Sta.SNCL[i].maxval;
		for(j=0;j<But->NSCN;j++) {
			if(strcmp(But->Chan[j].SCNnam, But->Sta.SNCL[i].SNCLnam)==0) 
					But->Sta.SNCL[i].meta = j;
		}
	}
	But->Sta.range = 2*But->Sta.maxoff;
	
	But->T0 = floor(But->Sta.mintime) - 1.0;
	But->Tn =  ceil(But->Sta.maxtime) + 1.0;
	if((But->Tn - But->T0) <  10.0) But->Tn = But->T0 +  10.0;
	if((But->Tn - But->T0) > 120.0) But->Tn = But->T0 + 120.0;
	
	
	But->xsf = But->xpix/(But->Tn - But->T0);
	But->ysf = (double)But->ypix/(double)(But->Sta.maxval - But->Sta.minval);
	But->ysf = (double)But->ypix/(double)(But->Sta.range);
	
	if(Debug) {
		logit("e", "%s_%s %15.2f %15.2f %15.2f \n", But->Sta.Site, But->Sta.Net, But->Sta.mintime, But->Sta.maxtime, But->Sta.maxtime - But->Sta.mintime);
		logit("e", "%s_%s %15.2f %15.2f %15.2f \n", But->Sta.Site, But->Sta.Net, But->T0, But->Tn, But->Tn - But->T0);
		logit("e", "%d %d \n", But->Sta.minval, But->Sta.maxval);
		logit("e", "%f %f \n", But->xsf, But->ysf);
	}
	
	for(i=0;i<But->Sta.Nchan;i++) {
		But->Sta.SNCL[i].page.x1 = 0;
		But->Sta.SNCL[i].page.x2 = XLMARGIN*72;
		But->Sta.SNCL[i].page.x3 = XLMARGIN*72 + But->xpix;
		But->Sta.SNCL[i].page.x4 = XLMARGIN*72 + But->xpix + XRMARGIN*72;
		But->Sta.SNCL[i].page.y1 = 0 + i*(YTMargin*72 + But->ypix + YBMargin*72);
		But->Sta.SNCL[i].page.y2 = YTMargin*72 + i*(YTMargin*72 + But->ypix + YBMargin*72);
		But->Sta.SNCL[i].page.y3 = YTMargin*72 + But->ypix + i*(YTMargin*72 + But->ypix + YBMargin*72);
		But->Sta.SNCL[i].page.y4 = YTMargin*72 + But->ypix + YBMargin*72 + i*(YTMargin*72 + But->ypix + YBMargin*72);
		But->Sta.SNCL[i].page.ycen = But->Sta.SNCL[i].page.y2 + But->ypix/2;
		But->Sta.SNCL[i].ysf = (double)But->ypix/(double)(But->Sta.SNCL[i].maxval - But->Sta.SNCL[i].minval);
		if(Debug) {
			logit("e", "x: %d %d %d %d\n", But->Sta.SNCL[i].page.x1, But->Sta.SNCL[i].page.x2, But->Sta.SNCL[i].page.x3, But->Sta.SNCL[i].page.x4);
			logit("e", "y: %d %d %d %d %d\n", But->Sta.SNCL[i].page.y1, But->Sta.SNCL[i].page.y2, But->Sta.SNCL[i].page.ycen, But->Sta.SNCL[i].page.y3, But->Sta.SNCL[i].page.y4);
			logit("e", "mean: %f meta: %d min: %d max: %d \n", But->Sta.SNCL[i].mean, But->Sta.SNCL[i].meta, But->Sta.SNCL[i].minval, But->Sta.SNCL[i].maxval);
			logit("e", "ysf: %f ysf: %f \n", But->Sta.SNCL[i].ysf, But->ysf);
		}
	}
	
    Decode_Time(But->T0, &But->Stime);
	
	return 0;
}


/********************************************************************
 *    Build_Axes constructs the axes for the plot by drawing the    *
 *    GIF image in memory.                                          *
 *                                                                  *
 ********************************************************************/

int Build_Axes(Global *But) 
{
    char    whoami[90], c22[30], cstr[150];
    int     i, j;
    int     xgpix, ygpix, black, ix, iy, iyc, iy2, metindex;
    double  time, value, sensitivity;
    TStrct  Ts;
    gdImagePtr    im_in;

    sprintf(whoami, " %s: %s: ", progname, "Build_Axes");
    
    But->GifImage = 0L;
                 
    xgpix = 72.0*But->xsize + 8;
    ygpix = 72.0*But->ysize + 8;
    xgpix = But->Sta.SNCL[0].page.x4;
    ygpix = But->Sta.SNCL[But->Sta.Nchan-1].page.y4;

    But->GifImage = gdImageCreate(xgpix, ygpix);
    if(But->GifImage==0) {
        logit("e", "%s Not enough memory! Reduce size of image or increase memory.\n\n", whoami);
        return 2;
    }
    if(But->GifImage->sx != xgpix) {
        logit("e", "%s Not enough memory for entire image! Reduce size of image or increase memory.\n", 
             whoami);
        return 2;
    }
    Pallette(But->GifImage, But->gcolor);
    
    black = But->gcolor[BLACK];
    for(i=0;i<But->Sta.Nchan;i++) {
		gdImageRectangle( But->GifImage, But->Sta.SNCL[i].page.x2, But->Sta.SNCL[i].page.y2, 
										 But->Sta.SNCL[i].page.x3, But->Sta.SNCL[i].page.y3,  black);
		gdImageString(But->GifImage, gdFontMediumBold, But->Sta.SNCL[i].page.x2+100, But->Sta.SNCL[i].page.y2 - 15, But->Sta.SNCL[i].SNCLnam, black);    
		date22 (But->Sta.mintime, c22);
		sprintf(cstr, "%.11s", c22) ;
		gdImageString(But->GifImage, gdFontMediumBold, But->Sta.SNCL[i].page.x2+220, But->Sta.SNCL[i].page.y2 - 15, cstr, black);    
		iy = But->Sta.SNCL[i].page.y2;
		iyc = But->Sta.SNCL[i].page.ycen;
		iy2 = But->Sta.SNCL[i].page.y3;
		time = But->T0;
		while(time <= But->Tn) {
			Decode_Time( time, &Ts);
			sprintf(cstr, "%02d:%02d:%02.0f", Ts.Hour, Ts.Min, Ts.Sec);
			ix = But->Sta.SNCL[i].page.x2 + (time - But->T0)*But->xsf;
			gdImageLine(But->GifImage, ix, iy, ix, iy+5, black);
			gdImageLine(But->GifImage, ix, iy2, ix, iy2-5, black);
			if(fmod(time,10)==0.0) {
				gdImageString(But->GifImage, gdFontMediumBold, ix-25, iy2+15, cstr, black);
				gdImageLine(But->GifImage, ix, iy2-10, ix, iy2+10, black);    
			}
			time += 1.0;
		}
		metindex = But->Sta.SNCL[i].meta;
		ix = But->Sta.SNCL[i].page.x2;
		gdImageLine(But->GifImage, ix-10, iy, ix, iy, black);
		if(But->stype == 0)
        	value = (double)(But->Sta.range)/2.0;
		if(But->stype == 1)
        	value = (double)(But->Sta.SNCL[i].maxval-But->Sta.SNCL[i].minval)/2.0;
		if(metindex>=0) {
			value = (value)/But->Chan[metindex].sensitivity;  
		} else {         /* Use default */
			sensitivity = 2563.62;
			sensitivity = 1000000.0*3.331/1.3245/981.0;
			value = (value)/sensitivity;  
		}
		sprintf(cstr, "%8.3f", value);
		gdImageString(But->GifImage, gdFontMediumBold, ix-80, iy-5, cstr, black);
		
		gdImageLine(But->GifImage, ix-10, iyc, ix, iyc, black);
		sprintf(cstr, "%8.0f", 0.0);
		gdImageString(But->GifImage, gdFontMediumBold, ix-80, iyc-5, cstr, black);
		
		gdImageLine(But->GifImage, ix-10, iy2, ix, iy2, black);
		sprintf(cstr, "%8.3f", -value);
		gdImageString(But->GifImage, gdFontMediumBold, ix-80, iy2-5, cstr, black);
		sprintf(cstr, "counts");
		if(metindex>=0) {
			if(But->Chan[metindex].Sens_unit == 3) sprintf(cstr, "cm/sec/sec");
			if(But->Chan[metindex].Sens_unit == 2) sprintf(cstr, "cm/sec");
		} else {         /* Use default */
			sprintf(cstr, "cm/sec/sec");  
		}
		gdImageString(But->GifImage, gdFontMediumBold, ix-80, iyc-25, cstr, black);
	}
	
	return 0;
}


/*******************************************************************************
 *    Pallette defines the pallete to be used for plotting.                    *
 *     PALCOLORS colors are defined.                                           *
 *                                                                             *
 *******************************************************************************/

void Pallette(gdImagePtr GIF, long color[])
{
    color[WHITE]  = gdImageColorAllocate(GIF, 255, 255, 255);
    color[BLACK]  = gdImageColorAllocate(GIF, 0,     0,   0);
    color[RED]    = gdImageColorAllocate(GIF, 255,   0,   0);
    color[BLUE]   = gdImageColorAllocate(GIF, 0,     0, 255);
    color[GREEN]  = gdImageColorAllocate(GIF, 0,   105,   0);
    color[GREY]   = gdImageColorAllocate(GIF, 125, 125, 125);
    color[YELLOW] = gdImageColorAllocate(GIF, 125, 125,   0);
    color[TURQ]   = gdImageColorAllocate(GIF, 0,   255, 255);
    color[PURPLE] = gdImageColorAllocate(GIF, 200,   0, 200);    
    
    gdImageColorTransparent(GIF, -1);
}


/********************************************************************
 *  mseed_plot plots data traces from MiniSEED file                 *
 ********************************************************************/

int mseed_plot(int *bytes, DATA_HDR *mseed_hdr, Global *But) 
{
    int    i, j, this, minv, maxv, val;
    char   SNCL[20], Site[6], Net[5], Comp[5], Loc[5];     
    double mint, maxt;

	/* check to see if we have a log channel! */
	if (mseed_hdr->sample_rate == 0 || 
	    mseed_hdr->sample_rate_mult == 0 ||
	    mseed_hdr->num_samples == 0)  {
		return 1;
	}

	strcpy(Site,trim(mseed_hdr->station_id));
	strcpy(Net,trim(mseed_hdr->network_id));
	strcpy(Comp,trim(mseed_hdr->channel_id));
	strcpy(Loc,trim(mseed_hdr->location_id));
	if (0 == strncmp(Loc, "  ", 2) || 0 == memcmp(Loc, "\000\000", 2)) strcpy(Loc,"--");
		
	sprintf(SNCL, "%s_%s_%s_%s", Site, Net, Comp, Loc);
	this = -1;
	for(i=0;i<But->Sta.Nchan;i++) {
		if(strcmp(SNCL, But->Sta.SNCL[i].SNCLnam)==0) {this = i; break;}
	}
	if(this<0) return 1;
	
	mint = (double)unix_time_from_int_time(mseed_hdr->begtime) +
					((double)(mseed_hdr->begtime.usec)/USECS_PER_SEC);
	maxt = (double)unix_time_from_int_time(mseed_hdr->endtime) +
					((double)(mseed_hdr->endtime.usec)/USECS_PER_SEC);
	But->Sta.SNCL[this].mpts = mseed_hdr->num_samples;
	
	Plot_Trace(But, this, bytes, mint);
	         
	return 0;
} 

/*******************************************************************************
 *    Plot_Trace plots an individual trace (Data)  and stuffs it into          *
 *     the GIF image in memory.                                                *
 *                                                                             *
 *******************************************************************************/

int Plot_Trace(Global *But, int this, int *Data, double mint)
{
    char    whoami[90], string[160];
    double  x0, x, y, xinc, samp_pix, tsize, time;
    double  in_sec, xsf, ycenter, sf, mpts, rms, value, trace_size;
    double  max, min, fudge, Sens_gain;
    int     i, j, jj, k, ix, iy, LineNumber, secs, metindex;
    int     lastx, lasty, decimation, acquired, ixmin, ixmax, imin, imax, idmin, idmax;
    long    black, color, trace_clr;

    sprintf(whoami, " %s: %s: ", progname, "Plot_Trace");
    
    metindex = But->Sta.SNCL[this].meta;
    i = 0;
    secs = But->SecsPerPlot;
    decimation = 1;
    
    imin = imax = But->Sta.SNCL[this].page.ycen - Data[0]*But->ysf;
    idmin = idmax = Data[0];
    ixmin =  99999999;
    ixmax = -99999999;
    
    trace_clr = But->gcolor[BLACK];
    acquired = 0;
    for(j=0;j<But->Sta.SNCL[this].mpts;j+=decimation) {
        time = mint + j/But->Sta.SNCL[this].samprate;
        
        if (time > But->Tn) return 0;
        
        ix = But->Sta.SNCL[this].page.x2 + (time - But->T0)*But->xsf;
        if(ix > But->Sta.SNCL[this].page.x3) return 0;
        if(But->stype == 0)
        	iy = But->Sta.SNCL[this].page.ycen - (Data[j]-But->Sta.SNCL[this].mean)*But->ysf;
        if(But->stype == 1)
        	iy = But->Sta.SNCL[this].page.ycen - (Data[j]-But->Sta.SNCL[this].mean)*But->Sta.SNCL[this].ysf;
		if(acquired) {
			gdImageLine(But->GifImage, ix, iy, lastx, lasty, trace_clr);
		}
		lastx = ix;  lasty = iy;
		acquired = 1;
		if(imin > iy) imin = iy;
		if(imax < iy) imax = iy;
		if(ixmin > ix) ixmin = ix;
		if(ixmax < ix) ixmax = ix;
		if(idmin > Data[j]) idmin = Data[j];
		if(idmax < Data[j]) idmax = Data[j];
    } 
	if(Debug) 
		logit("e", "%s %s %12d %12d %5d %5d %5d %8d %8d %15.2f %15.2f\n", 
		whoami, But->Sta.SNCL[this].SNCLnam, ixmin, ixmax, imin, imax, But->Sta.SNCL[this].mpts, 
		idmin, idmax, But->T0, mint);
    
    return 0;
}

/*********************************************************************
 *   Save_Plot()                                                     *
 *    Saves the current version of the GIF image and ships it out.   *
 *********************************************************************/

void Save_Plot(Global *But)
{
    char    tname[175], string[200], whoami[90];
    FILE    *out;
    int     j, ierr, retry;
    
    sprintf(whoami, " %s: %s: ", progname, "Save_Plot");
    /* Make the GIF file. *
     **********************/        
    sprintf(But->TmpName, "%s_%s_%s.%4d%02d%02d.%02d%02d%02.0f.%s", But->Sta.Site, But->Sta.Net, But->Sta.Loc, 
             But->Stime.Year, But->Stime.Month, But->Stime.Day, 
             But->Stime.Hour, But->Stime.Min,   But->Stime.Sec, But->FileType);
    sprintf(But->GifName, "%s%s", But->TmpName, ".gif");

    for(j=0;j<But->nltargets;j++) {
		sprintf(But->LocalGif, "%s%s.%d", But->loctarget[j], But->TmpName, But->Stime.Min);
		out = fopen(But->LocalGif, "wb");
		if(out == 0L) {
			logit("e", "%s Unable to write GIF File: %s\n", whoami, But->LocalGif); 
		} else {
			gdImageGif(But->GifImage, out);
			fclose(out);
			sprintf(tname,  "%s%s", But->loctarget[j], But->GifName );
			ierr = rename(But->LocalGif, tname);
			if(ierr) {
				if(But->Debug) logit("e", "%s Error Renaming GIF File %d\n", whoami, ierr);
			} else {
				if(But->Debug) logit("e", "%s GIF File %s renamed %s.\n", whoami, But->LocalGif, tname);
			}
		}
    }
}


/*********************************************************************
 *   CommentList()                                                   *
 *    Build and send a file relating SCNs to their comments.         *
 *********************************************************************/

void CommentList(Global *But)
{
    char    tname[200], fname[175], whoami[50];
    char    stanet[20];
    int     i, j, k, n, ierr, jerr;
    FILE    *out;
    
    sprintf(whoami, " %s: %s: ", progname, "CommentList");

	n = 0;
    for(j=0;j<But->NSCN;j++) {
    	k = 1;
    	for(i=0;i<n;i++) {
			if(strcmp(But->Chan[j].StaNet, But->zzSta[i].StaNet)==0) k = 0;
    	}
    	if(k) {
    		strcpy(But->zzSta[n].StaNet, But->Chan[j].StaNet);
    		strcpy(But->zzSta[n].SiteName, But->Chan[j].SiteName);
    		n++;
    	}
    }
	But->Nsta = n;
    sprintf(tname, "%sznamelist.dat", But->GifDir);
    
    for(j=0;j<But->nltargets;j++) {
        out = fopen(tname, "wb");
        if(out == 0L) {
            logit("e", "%s Unable to open NameList File: %s\n", whoami, tname);    
        } else {
            for(i=0;i<But->Nsta;i++) {
                fprintf(out, "%s. %s.\n", But->zzSta[i].StaNet, But->zzSta[i].SiteName);
            }
            fclose(out);
            sprintf(fname,  "%sznamelist.dat", But->loctarget[j] );
            ierr = rename(tname, fname); 
            /* The following silliness is necessary to be Windows compatible */ 
            if( ierr != 0 ) {
                if(Debug) logit( "e", "Error moving file %s to %s; ierr = %d\n", tname, fname, ierr );
                if( remove( fname ) != 0 ) {
                    logit("e","error deleting file %s\n", fname);
                } else  {
                    if(Debug) logit("e","deleted file %s.\n", fname);
                    jerr = rename( tname, fname );
                    if( jerr != 0 ) {
                        logit( "e", "error moving file %s to %s; ierr = %d\n", tname, fname, ierr );
                    } else {
                        if(Debug) logit("e","%s moved to %s\n", tname, fname );
                    }
                }
            } else {
                if(Debug) logit("e","%s moved to %s\n", tname, fname );
            }
        }
    }
}

/************************************************************************/
/* signal handler that initiates a shutdown                              */
/************************************************************************/
void initiate_termination(int sigval) 
{
    signal(sigval, initiate_termination);
    ShutMeDown = TRUE;
    err_exit = Q2EW_DEATH_SIG_TRAP;
    return;
}

/****************************************************************************
 *  Get_Sta_Info(Global *But);                                              *
 *  Retrieve all the information available about the network stations       *
 *  and put it into an internal structure for reference.                    *
 *  This should eventually be a call to the database; for now we must       *
 *  supply an ascii file with all the info.                                 *
 *     process station file using kom.c functions                           *
 *                       exits if any errors are encountered                *
 ****************************************************************************/
void Get_Sta_Info(Global *But)
{
    char    whoami[50], *com, *str, ns, ew;
    int     i, j, k, nfiles, success, count;
    double  dlat, mlat, dlon, mlon;

    sprintf(whoami, "%s: %s: ", progname, "Get_Sta_Info");
    
    ns = 'N';
    ew = 'W';
    But->NSCN = 0;
    for(k=0;k<But->nStaDB;k++) {
            /* Open the main station file
             ****************************/
        nfiles = k_open( But->stationList[k] );
        if(nfiles == 0) {
            fprintf( stderr, "%s Error opening station file <%s>; exiting!\n", whoami, But->stationList[k] );
            exit( -1 );
        }

            /* Process all station files
             ***************************/
        count = 0;
        while(nfiles > 0) {  /* While there are station files open */
            while(k_rd())  {      /* Read next line from active file  */
                com = k_str();         /* Get the first token from line */

                    /* Ignore blank lines & comments
                     *******************************/
                if( !com )           continue;
                if( com[0] == '#' )  continue;

                    /* Open a nested station file
                     **********************************/
                if( com[0] == '@' ) {
                    success = nfiles+1;
                    nfiles  = k_open(&com[1]);
                    if ( nfiles != success ) {
                        fprintf( stderr, "%s Error opening command file <%s>; exiting!\n", whoami, &com[1] );
                        exit( -1 );
                    }
                    continue;
                }

                /* Process anything else as a channel descriptor
                 ***********************************************/

                if( But->NSCN >= MAXCHANNELS ) {
                    fprintf(stderr, "%s Too many channel entries in <%s>", 
                             whoami, But->stationList[k] );
                    fprintf(stderr, "; max=%d; exiting!\n", (int) MAXCHANNELS );
                    exit( -1 );
                }
                j = But->NSCN;
                
                    /* S C N */
                strncpy( But->Chan[j].Site, com,  6);
                str = k_str();
                if(str) strncpy( But->Chan[j].Net,  str,  2);
                str = k_str();
                if(str) strncpy( But->Chan[j].Comp, str, 3);
                for(i=0;i<6;i++) if(But->Chan[j].Site[i]==' ') But->Chan[j].Site[i] = 0;
                for(i=0;i<2;i++) if(But->Chan[j].Net[i]==' ')  But->Chan[j].Net[i]  = 0;
                for(i=0;i<3;i++) if(But->Chan[j].Comp[i]==' ') But->Chan[j].Comp[i] = 0;
                But->Chan[j].Comp[3] = But->Chan[j].Net[2] = But->Chan[j].Site[5] = 0;
                
				str = k_str();
				if(str) strncpy( But->Chan[j].Loc, str, 2);
				for(i=0;i<2;i++) if(But->Chan[j].Loc[i]==' ')  But->Chan[j].Loc[i]  = 0;
				But->Chan[j].Loc[2] = 0;
                
				sprintf(But->Chan[j].SCNnam, "%s_%s_%s_%s", But->Chan[j].Site, But->Chan[j].Net, But->Chan[j].Comp, But->Chan[j].Loc);
				sprintf(But->Chan[j].StaNet, "%s_%s", But->Chan[j].Site, But->Chan[j].Net);


				/* Lat Lon Elev */
				But->Chan[j].Lat  = k_val();
				But->Chan[j].Lon  = k_val();
				But->Chan[j].Elev = k_val();
			

                But->Chan[j].Inst_type = k_int();
                But->Chan[j].Inst_gain = k_val();
                But->Chan[j].GainFudge = k_val();
                But->Chan[j].Sens_type = k_int();
                But->Chan[j].Sens_unit = k_int();
                But->Chan[j].Sens_gain = k_val();
                But->Chan[j].SiteCorr  = k_val();
                        
                if(But->Chan[j].Sens_unit == 3) But->Chan[j].Sens_gain /= 981.0;
               
                But->Chan[j].sensitivity = (1000000.0*But->Chan[j].Sens_gain/But->Chan[j].Inst_gain)*But->Chan[j].GainFudge;    /*    sensitivity counts/units        */
                
                But->Chan[j].ShkQual = k_int();
                
                if (k_err()) {
                    logit("e", "%s Error decoding line %d in station file\n%s\n  exiting!\n", whoami, j, k_get() );
                    logit("e", "%s Previous line was %s\n  exiting!\n", whoami, But->Chan[j-1].SCNnam );
                    exit( -1 );
                }
         /*>Comment<*/
                str = k_str();
                if( (long)(str) != 0 && str[0]!='#')  strcpy( But->Chan[j].SiteName, str );
                    
                str = k_str();
                if( (long)(str) != 0 && str[0]!='#')  strcpy( But->Chan[j].Descript, str );
                    
                But->NSCN++;
                count++;
            }
            nfiles = k_close();
        }
		if(Debug) logit("et", "Station file %s has info about %d stations. \n", But->stationList[k], count);
    }

	if(Debug) logit("et", "Get_Sta_Info has info about %d stations. \n", But->NSCN);
			
}



     /***************************************************************
      *                          GetConfig()                        *
      *         Processes command file using kom.c functions.       *
      *           Returns -1 if any errors are encountered.         *
      ***************************************************************/

#define NCOMMAND 12             /* Number of commands in the config file */

int GetConfig( char *configfile, Global *But )
{
	const int ncommand = NCOMMAND;
	
	char    whoami[50], str[50];
	char     init[NCOMMAND];     /* Flags, one for each command */
	int      nmiss;              /* Number of commands that were missed */
	int      nfiles;
	int      i, n;
    double  val;
    FILE    *in;
    gdImagePtr    im_in;

    sprintf(whoami, " %s: %s: ", progname, "GetConfig");

/* Set to zero one init flag for each required command
   ***************************************************/
   for ( i = 0; i < ncommand; i++ ) init[i] = 0;
   Debug = 0;
   But->stype     = 0;
   But->nltargets = 0;
   But->nStaDB    = 0;

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
         } else if ( k_its( "LogFile" ) ) {
            LogFile = k_int();
            init[2] = 1;
  /*3*/     
         } else if ( k_its( "HeartBeatInt" ) ) {
            HeartBeatInt = k_int();
            init[3] = 1;
  /*4*/     
            }
            else if( k_its("GifDir") ) {
                str = k_str();
                if( (int)strlen(str) >= GDIRSZ) {
                    fprintf( stderr, "%s Fatal error. Gif directory name %s greater than %d char.\n",
                        whoami, str, GDIRSZ);
                    return (-1);
                }
                if(str) strcpy( But->GifDir , str );
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

          /* get display parameters
        ************************************/
/*8*/
            else if( k_its("Display") ) {
         /*>01<*/
                But->SecsPerPlot = k_int();   /*  # of seconds per gif image */
                if(But->SecsPerPlot <  10) But->SecsPerPlot =  10;
                if(But->SecsPerPlot > 240) But->SecsPerPlot = 240;
         /*>02<*/
                val = k_val();            /* x-size of data plot in pixels */
                But->xpix = (val >= 100.0)? (int)val:(int)(72.0*val);
                
         /*>03<*/
                val = k_val();            /* y-size of data plot in pixels */
                But->ypix = (val >= 100.0)? (int)val:(int)(72.0*val);
                
                init[8] = 1;
            }

          /* get filter parameters
        ************************************/
/*9*/
            else if( k_its("Filter") ) {
         /*>01<*/
                But->flo = k_val();   /*  Lower filter corner */
         /*>02<*/
                But->fhi = k_val();   /*  Upper filter corner */
                
         /*>03<*/
                But->ftype = k_int(); /* Filter type */
                
                if(But->flo <   0.0) But->flo =   0.0;
                if(But->fhi > 100.0) But->fhi = 100.0;
                init[9] = 1;
            }

                /* get station list path/name
                *****************************/
/*10*/
            else if( k_its("StationList") ) {
                if ( But->nStaDB >= MAX_STADBS ) {
                    fprintf( stderr, "%s Too many <StationList> commands in <%s>", 
                             whoami, configfile );
                    fprintf( stderr, "; max=%d; exiting!\n", (int) MAX_STADBS );
                    return (-1);
                }
                str = k_str();
                if( (int)strlen(str) >= STALIST_SIZ) {
                    fprintf( stderr, "%s Fatal error. Station list name %s greater than %d char.\n", 
                            whoami, str, STALIST_SIZ);
                    exit(-1);
                }
                if(str) strcpy( But->stationList[But->nStaDB] , str );
                But->nStaDB++;
                init[10] = 1;
            }
            
        /* get the local target directory(s)
        ************************************/
/*11*/
            else if( k_its("LocalTarget") ) {
                if ( But->nltargets >= MAX_TARGETS ) {
                    logit("e", "%s Too many <LocalTarget> commands in <%s>", 
                             whoami, configfile );
                    logit("e", "; max=%d; exiting!\n", (int) MAX_TARGETS );
                    return (-1);
                }
                if( (long)(str=k_str()) != 0 )  {
                    n = strlen(str);   /* Make sure directory name has proper ending! */
                    if( str[n-1] != '/' ) strcat(str, "/");
                    strcpy(But->loctarget[But->nltargets], str);
                }
                But->nltargets += 1;
                init[11] = 1;
            }
	/*optional commands*/
            else if( k_its("Logo") ) {        
                str = k_str();
                if(str) {
                    strcpy( But->logoname, str );
                    But->logo = 1;
                }
            }

			else if ( k_its( "Debug" ) ) {
				But->Debug = Debug = 1;
				/* turn on the LogFile too! */
				LogFile = 1;
			 }  

			else if ( k_its( "YScale" ) ) {
				But->stype = k_int();
			 }  

            else if( k_its("UseDST") )         But->UseDST = 1;         /* optional command */

		else {
	    /* An unknown parameter was encountered */
            fprintf( stderr, "%s: <%s> unknown parameter in <%s>\n", 
		whoami,com, configfile );
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
      if ( !init[2]  ) fprintf(stderr, "<LogFile> " );
      if ( !init[3] ) fprintf(stderr, "<HeartBeatInt> " );
      if ( !init[4] ) fprintf(stderr, "<GifDir> " );
      if ( !init[5] ) fprintf(stderr, "<NQFilesInDir> " );
      if ( !init[6] ) fprintf(stderr, "<NQFilesOutDir> " );
      if ( !init[7] ) fprintf(stderr, "<NQFilesErrorDir> " );
      if ( !init[8] ) fprintf(stderr, "<Display> " );
      if ( !init[9] ) fprintf(stderr, "<Filter> " );
      if ( !init[10] ) fprintf(stderr, "<StationList> " );
      if ( !init[11] ) fprintf(stderr, "<LocalTarget> " );
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
/*   setuplogo(&DataLogo);
       DataLogo.type=TypeTrace2; */
   setuplogo(&OtherLogo);

    if(But->logo) {
        strcpy(str, But->logoname);
        strcpy(But->logoname, But->GifDir);
        strcat(But->logoname, str);
        in = fopen(But->logoname, "rb");
        if(in) {
            im_in = gdImageCreateFromGif(in);
            fclose(in);
            But->logox = im_in->sx;
            But->logoy = im_in->sy;
            YTMargin += im_in->sy/72.0 + 0.1;
            gdImageDestroy(im_in);
        }
    }

    But->axexmax = But->xpix/72.0;
    But->axeymax = But->ypix/72.0;
    But->xsize = But->axexmax + XLMARGIN + XRMARGIN;
    But->ysize = But->axeymax + YBMARGIN + YTMargin;

   return 0;
}


/********************************************************************
 *  setuplogo initializes logos                                     *
 ********************************************************************/

void setuplogo(MSG_LOGO *logo) 
{
   /* only get the InstId once */
   if (InstId == 255  && GetLocalInst(&InstId) != 0) {
      fprintf( stderr, "%s: Invalid Installation code; exiting!\n", progname);
      exit(-1);
   }
   logo->mod = QModuleId;
   logo->instid = InstId;
}


/********************************************************************
 *  nq2gif_die attempts to gracefully die                          *
 ********************************************************************/

void nq2gif_die( int errmap, char * str ) {

	if (errmap != -1) {
		/* use the statmgr reporting to notify of this death */
#ifdef DEBUG
		fprintf(stderr, "SENDING MESSAGE to statmgr: %d %s\n", errmap, str);
#endif /* DEBUG */
		message_send(TypeErr, errmap, str);
	}
	
	/* this next bit must come after the possible tport_putmsg() above!! */
	if (InRegion.mid != -1) {
		/* we attached to an EW ring buffer */
		logit("e", "%s: exiting because %s\n", progname, str);
		tport_detach( &InRegion );
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

    OtherLogo.instid  = InstId;
    OtherLogo.mod  = QModuleId;
    OtherLogo.type  = type;

    time( &t );
    /* put the message together */
    if( type == TypeHB ) {
       sprintf( message, "%ld %ld\n", t, (long) mypid);
    } else if( type == TypeErr ) {
       sprintf( message, "%ld %hd %s\n", t, ierr, note);
       logit( "et", "%s: %s\n", progname, note );
    }
    len = strlen( message );   /* don't include the null byte in the message */

#ifdef DEBUG
		fprintf(stderr, "message_send: %ld %s\n", len, message);
#endif /* DEBUG */
   /* write the message to shared memory */
    if( tport_putmsg( &InRegion, &OtherLogo, len, message ) != PUT_OK ) {
        if( type == TypeHB ) {
           logit("et","%s:  Error sending heartbeat.\n", progname );
        }
        else if( type == TypeErr ) {
           logit("et","%s:  Error sending error:%d.\n", progname, ierr );
        }
    }

   return;
}

/**********************************************************************
 * Decode_Time : Decode time from seconds since 1970                  *
 *                                                                    *
 **********************************************************************/
void Decode_Time( double secs, TStrct *Time)
{
    struct Greg  g;
    long    minute;
    double  sex;

    Time->Time = secs;
    secs += GSEC1970;
    Time->Time1600 = secs;
    minute = (long) (secs / 60.0);
    sex = secs - 60.0 * minute;
    grg(minute, &g);
    Time->Year  = g.year;
    Time->Month = g.month;
    Time->Day   = g.day;
    Time->Hour  = g.hour;
    Time->Min   = g.minute;
    Time->Sec   = sex;
}


/**********************************************************************
 * Encode_Time : Encode time to seconds since 1970                    *
 *                                                                    *
 **********************************************************************/
void Encode_Time( double *secs, TStrct *Time)
{
    struct Greg    g;

    g.year   = Time->Year;
    g.month  = Time->Month;
    g.day    = Time->Day;
    g.hour   = Time->Hour;
    g.minute = Time->Min;
    *secs    = 60.0 * (double) julmin(&g) + Time->Sec - GSEC1970;
}


/**********************************************************************
 * date22 : Calculate 22 char date in the form Jan23,1988 12:34 12.21 *
 *          from the julian seconds.  Remember to leave space for the *
 *          string termination (NUL).                                 *
 **********************************************************************/
void date22( double secs, char *c22)
{
    char *cmo[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    struct Greg  g;
    long    minute;
    double  sex;

    secs += GSEC1970;
    minute = (long) (secs / 60.0);
    sex = secs - 60.0 * minute;
    grg(minute, &g);
    sprintf(c22, "%3s%2d,%4d %.2d:%.2d:%05.2f",
            cmo[g.month-1], g.day, g.year, g.hour, g.minute, sex);
}


/***********************************************************************
 * date11 : Calculate 11 char h:m:s time in the form 12:34:12.21       *
 *          from the daily seconds.  Remember to leave space for the   *
 *          string termination (NUL).                                  *
 *          This is a special version used for labelling the time axis *
 ***********************************************************************/
void date11( double secs, char *c11)
{
    long    hour, minute, wholesex;
    double  sex, fracsex;

    if(secs < 86400.0) secs = secs - 86400; /* make negative times negative */
    minute = (long) (secs / 60.0);
    sex    = secs - 60.0 * minute;
    wholesex = (long) sex;
    fracsex  = sex - wholesex;
    hour   = (long) (minute / 60.0);
    minute = minute - 60 * hour;
    while(hour>=24) hour -= 24;
    
    if(hour != 0) {
        sprintf(c11, "%ld:%.2ld:%05.2f", hour, minute, sex);
        if(fracsex >= 0.01) sprintf(c11, "%.2ld:%.2ld:%05.2f", hour, minute, sex);
        else sprintf(c11, "%.2ld:%.2ld:%.2ld", hour, minute, wholesex);
    }
    else if(minute != 0) {
        sprintf(c11, "%ld:%05.2f", minute, sex);
        if(fracsex >= 0.01) sprintf(c11, "%.2ld:%05.2f", minute, sex);
        else sprintf(c11, "%.2ld:%.2ld", minute, wholesex);
    }
    else if(sex >= 10.0) {
        if(fracsex >= 0.01) sprintf(c11, "%05.2f", sex);
        else sprintf(c11, "%.2ld", wholesex);
    }
    else {
        if(fracsex >= 0.01) sprintf(c11, "%4.2f", sex);
        else sprintf(c11, "%.1ld", wholesex);
    }
}



