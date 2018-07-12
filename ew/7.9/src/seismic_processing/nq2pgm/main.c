/*************************************************************************
 *                                                                       *
 *  nq2pgm - NetQuakes MiniSEED to strong ground motion                  *
 *                                                                       *
 *  Calculates peak ground motions and spectral amplitudes using the     *
 *  method of Nigam and Jennings.                                        *
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

#define   DEBUG

#define   FILE_NAM_LEN 500
#define THREAD_STACK_SIZE 8192
#include "main.h"

#define ABS(X) (((X) >= 0) ? (X) : -(X))

char   progname[] = "nq2pgm";
char   NetworkName[TRACE_NET_LEN];     /* Network name. Constant from trace_buf.h */
char   NQFilesInDir[FILE_NAM_LEN];     /* directory from whence cometh the */
                                       /* NQ input files */
char   RingName[20];                   /* name of transport ring for input  */

int Debug;
int err_exit;                          /* an nq2pgm_die() error number */
static unsigned char InstId = 255;
pid_t mypid;
int ShutMeDown;		/* shut down flag */

time_t MyLastInternalBeat;      /* time of last heartbeat into the local Earthworm ring    */
unsigned  TidHeart;             /* thread id. was type thread_t on Solaris! */

/* Other globals
 ***************/
unsigned char QModuleId;         /* module id for nq2pgm */
long InRingKey;                  /* key to ring buffer nq2pgm dumps data */
int HeartBeatInt;                /* my heartbeat interval (secs) */
int LogFile;                     /* generate a logfile? */

/* some globals for EW not settable in the .d file. */
SHM_INFO  Region;  
MSG_LOGO DataLogo;               /* EW logo tag  for data */
MSG_LOGO OtherLogo;              /* EW logo tag  for err,log,heart */
unsigned char TypeTrace;         /* Trace EW type for logo */
unsigned char TypeTrace2;        /* Trace EW type for logo (supporting SCNL) */
unsigned char TypeHB;            /* HB=HeartBeat EW type for logo */
unsigned char TypeErr;           /* Error EW type for logo */

#define BUFLEN  65000           /* define maximum size for an event msg  */
static char MsgBuf[BUFLEN];     /* char string to hold output message    */
static unsigned char TypeSM2;   /* message type we'll produce            */
static unsigned char TypeSM3;   /* message type we'll produce            */
static int       LogOutgoingMsg = 1;  /* if non-zero, write each outgoing  */
                                      /*  msg to the daily log file        */
static char *sNullDate = "0000/00/00 00:00:00.000";

/* Functions in this source file
 *******************************/
int  wr_strongmotionIII( SM_INFO *sm, char *buf, int buflen );

static int   strappend( char *s1, int s1max, char *s2 );
static int   tokenlength( char *begtok, char c );
static char *datestr24( double t, char *pbuf, int len );
static int   addtimestr( char *buf, int buflen, double t );

thr_ret Heartbeat( void * );
int mseed_audit(int *bytes, DATA_HDR *mseed_hdr, Global *But);
int set_parms(Global *But); 

void initiate_termination(int );	/* signal handler func */
void Get_Sta_Info(Global *);
int GetConfig( char *configfile, Global *But );
void setuplogo(MSG_LOGO *);
void nq2pgm_die( int errmap, char * str );
void message_send( unsigned char, short, char *);
int file2ew_ship( unsigned char type, unsigned char iid,
                  char *msg, size_t msglen );
void file2ew_logmsg(char *msg, int msglen );
int write_xml(Global *But, char *fname);
int print_xml(Global *But, char *fname);
int xmltime(double time, char *xmlt);
void Decode_Time( double, TStrct *);
void Encode_Time( double *secs, TStrct *Time);
void date22( double, char *);
void date11( double, char *);

int peak_ground(float *Data, int npts, int itype, double dt, SM_INFO *sm);
void demean(float *A, int N);
void locut(float *s, int nd, float fcut, float delt, int nroll, int icaus);
void rdrvaa(float *acc, int na, float omega, float damp, float dt,
            float *rd, float *rv, float *aa, int *maxtime);
void amaxper(int npts, float dt, float *fc, float *amaxmm, 
					float *aminmm, float *pmax, int *imin, int *imax);

/*************************************************************************
 *  main( int argc, char **argv )                                        *
 *************************************************************************/

int main( int argc, char **argv )
{
	int i, j, sta_index, scan_station;   /* indexes */
	int ret_val;                         /* DEBUG return values */
    int     FileOK, rc;
	unsigned char alert, okay;           /* flags */
	char *ew_trace_buf;                  /* earthworm trace buffer pointer */
	long  ew_trace_len;                  /* length in bytes of the trace buffer */
    char    whoami[50], subname[] = "Main";
    FILE   *fp;
    char    fname[FILE_NAM_LEN];         /* name of ascii version of evt file */
    char    outFname[FILE_NAM_LEN];      /* Full name of output file */
    char    errbuf[100];                 /* Buffer for error messages */
    DATA_HDR   mseed_hdr, *mseed_hdr_ptr;
    int        max_num_points, msdata[MAXTRACELTH], idata[MAXTRACELTH][MAXCHAN], npts[MAXCHAN];
    float      Data[MAXTRACELTH], dt;
    time_t     current_time;
    SM_INFO    sm[MAXCHAN];
    Global     BStruct;  

	/* init some locals */
	alert = FALSE;
	okay = TRUE;
	err_exit = -1;
	mypid = getpid();       /* set it once on entry */
    if( mypid == -1 ) {
        fprintf( stderr,"%s Cannot get pid. Exiting.\n", whoami);
        return -1;
    }
    strcpy(BStruct.mod, progname);
    sprintf(whoami, "%s: %s: ", progname, "Main");

	/* init some globals */
	ShutMeDown = FALSE;
	Region.mid = -1;	/* init so we know if an attach happened */

    /* Check command line arguments
     ******************************/
    if ( argc != 2 ) {
        fprintf( stderr, "Usage: %s <configfile>\n", progname);
        exit( 0 );
    }

    /* Read the configuration file(s)
     ********************************/
	if (GetConfig(argv[1], &BStruct ) == -1) 
		nq2pgm_die(Q2EW_DEATH_EW_CONFIG,"Too many q2ew.d config problems");
	
	/* deal with some signals */
	signal(SIGINT, initiate_termination);
	signal(SIGTERM, initiate_termination);

	/* start EARTHWORM logging */
	logit_init(argv[1], (short) QModuleId, 512, LogFile);
	logit("e", "%s: Read in config file %s\n", progname, argv[1]);

	/* EARTHWORM init earthworm connection at this point, 
		this func() exits if there is a problem 
	*/

	/* Attach to Input shared memory ring
	 ************************************/
	tport_attach( &Region, InRingKey );
	logit( "e", "%s Attached to public memory region %s: %d\n",
	      whoami, RingName, InRingKey );

   /* Start the heartbeat thread
   ****************************/
    time(&MyLastInternalBeat); /* initialize our last heartbeat time */
                          
    if ( StartThread( Heartbeat, (unsigned)THREAD_STACK_SIZE, &TidHeart ) == -1 ) {
        logit( "et","%s Error starting Heartbeat thread. Exiting.\n", whoami );
		nq2pgm_die(err_exit, "Error starting Heartbeat thread");
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
		
		Get_Sta_Info(&BStruct);
		
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

			sleep_ew(2000);

	        if ((fp = fopen(fname, "rb" )) == NULL) {
	            logit ("e", "Could not open file <%s>\n", fname);
	            continue;
	        }
			logit("et","Process file %s\n",fname);

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
				npts[i] = 0;
			}
	        
	        /* Read the mseed file to get time and channel limits
	        *****************************************************/
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
	        /* Calculate Strong Ground Motion parameters and ship them.
	        ***********************************************************/
	        	
	        	if(set_parms(&BStruct) == 0) {
	        
					for(i=0;i<BStruct.Sta.Nchan;i++) {
						strcpy(BStruct.Sta.SNCL[i].sm.sta,  BStruct.Sta.SNCL[i].Site);
						strcpy(BStruct.Sta.SNCL[i].sm.net,  BStruct.Sta.SNCL[i].Net);
						strcpy(BStruct.Sta.SNCL[i].sm.comp, BStruct.Sta.SNCL[i].Comp);
						strcpy(BStruct.Sta.SNCL[i].sm.loc,  BStruct.Sta.SNCL[i].Loc);
						strcpy(BStruct.Sta.SNCL[i].sm.qid, "");
						strcpy(BStruct.Sta.SNCL[i].sm.qauthor, "");
						BStruct.Sta.SNCL[i].sm.t = BStruct.Sta.SNCL[i].mintime;
						BStruct.Sta.SNCL[i].sm.talt = time(&current_time);
						peak_ground(BStruct.Sta.SNCL[i].fdata, BStruct.Sta.SNCL[i].npts, 
									BStruct.Sta.SNCL[i].Sens_unit, BStruct.Sta.SNCL[i].dt, &BStruct.Sta.SNCL[i].sm);
						BStruct.Sta.SNCL[i].sm.talt = BStruct.Sta.SNCL[i].sm.tpga;
				
						/* Build TYPE_SM2 msg from last SM_DATA structure and ship it
						***********************************************************/
						if (wr_strongmotionII( &BStruct.Sta.SNCL[i].sm, MsgBuf, BUFLEN ) != 0 ) {
							logit("e","nq2pgm: Error building TYPE_STRONGMOTIONII msg\n");
							break;
						}
					
						if (file2ew_ship( TypeSM2, 0, MsgBuf, strlen(MsgBuf) ) != 0 ) {
							logit("e","nq2pgm: file2ew_ship error\n");
							break;
						}
				
						/* Build TYPE_SM3 msg from last SM_DATA structure and ship it
						***********************************************************/
						if (wr_strongmotionIII( &BStruct.Sta.SNCL[i].sm, MsgBuf, BUFLEN ) != 0 ) {
							logit("e","nq2pgm: Error building TYPE_STRONGMOTIONIII msg\n");
							break;
						}
					
						/*
						*/
						if(Debug) logit("et","TYPE_STRONGMOTIONIII \n%s\n",MsgBuf);
						if (file2ew_ship( TypeSM3, 0, MsgBuf, strlen(MsgBuf) ) != 0 ) {
							logit("e","nq2pgm: file2ew_ship error\n");
							break;
						}
					}
					
					write_xml(&BStruct, fname);
					print_xml(&BStruct, fname);
					
					logit("et","Done with file %s\n",fname);
	        	}
	        }
	        else {      /* something blew up on this file. Log the error */
	            logit("e","Error processing file %s\n",fname);

	         /*   continue; */
	        }
			
			/* Dispose of file
			*********************/
			if( remove( fname ) != 0) {
				logit("et", "%s Cannot delete MiniSEED file <%s>; exiting!", whoami, fname );
				break;
			}
		}
	    
	    /* EARTHWORM see if we are being told to stop */
	    if ( tport_getflag( &Region ) == TERMINATE  ) {
			nq2pgm_die(Q2EW_DEATH_EW_TERM, "Earthworm global TERMINATE request");
	    }
	    if ( tport_getflag( &Region ) == mypid        ) {
			nq2pgm_die(Q2EW_DEATH_EW_TERM, "Earthworm pid TERMINATE request");
	    }
	    /* see if we had a problem anywhere in processing the last data */
	    if (ShutMeDown == TRUE) {
			nq2pgm_die(err_exit, "q2ew kill request or fatal EW error");
	    }
		sleep_ew(2000);
	}
	/* should never reach here! */
	    
	logit("et","Done for now.\n");
 
	nq2pgm_die( -1, "clean exit" );
	
	exit(0);
}

/********************************************************************
 * wr_strongmotionIII()                                             *
 * Reads a SM_INFO structure and writes an ascii TYPE_STRONGMOTION3 *
 * message (null terminated)                                        *
 * Returns 0 on success, -1 on failure (buffer overflow)            *
 ********************************************************************/
int wr_strongmotionIII( SM_INFO *sm, char *buf, int buflen )
{
   char     tmp[256]; /* working buffer */
   char    *qid;
   char    *qauthor;
   int      i;
    time_t     current_time;

   memset( buf, 0, (size_t)buflen );    /* zero output buffer */

/* channel codes */
   sprintf( buf, "SCNL: %s.%s.%s.%s", 
            sm->sta, sm->comp, sm->net, sm->loc );

/* start of record time */
   if( strappend( buf, buflen, "\nWINDOW: " ) ) return ( -1 );
   if( addtimestr( buf, buflen, sm->t ) ) return ( -1 );
   sprintf( tmp, " LENGTH: %.3lf ", (sm->length!=SM_NULL ? ABS(sm->length) : sm->length) );
   if( strappend( buf, buflen, tmp ) ) return( -1 );

/* Print peak acceleration value & time */
   sprintf( tmp, "\nPGA: %.6lf TPGA: ", (sm->pga!=SM_NULL ? ABS(sm->pga) : sm->pga) );
   if( strappend( buf, buflen, tmp ) ) return( -1 );
   if( addtimestr( buf, buflen, sm->tpga ) ) return ( -1 );
   if( strappend( buf, buflen, " METH: MS" ) ) return( -1 );
      
/* Print peak velocity value & time */
   sprintf( tmp, "\nPGV: %.6lf TPGV: ", (sm->pgv!=SM_NULL ? ABS(sm->pgv) : sm->pgv) );
   if( strappend( buf, buflen, tmp ) ) return( -1 );
   if( addtimestr( buf, buflen, sm->tpgv ) ) return ( -1 );
   if( strappend( buf, buflen, " METH: IT 5.9" ) ) return( -1 );
      
/* Print peak displacement value & time */
   sprintf( tmp, "\nPGD: %.6lf TPGD: ", (sm->pgd!=SM_NULL ? ABS(sm->pgd) : sm->pgd) );
   if( strappend( buf, buflen, tmp ) ) return( -1 );
   if( addtimestr( buf, buflen, sm->tpgd ) ) return ( -1 );
   if( strappend( buf, buflen, " METH: IT 5.9" ) ) return( -1 );

/* Print spectral amplitude value & time for period = 0.3 */
   sprintf( tmp, "\nSA: 0.3 %.6lf TSA: ", (sm->rsa[0]!=SM_NULL ? ABS(sm->rsa[0]) : sm->rsa[0]) );
   if( strappend( buf, buflen, tmp ) ) return( -1 );
   if( addtimestr( buf, buflen, sm->trsa[0] ) ) return ( -1 );
   if( strappend( buf, buflen, " METH: NJ" ) ) return( -1 );

/* Print spectral amplitude value & time for period = 1.0 */
   sprintf( tmp, "\nSA: 1.0 %.6lf TSA: ", (sm->rsa[1]!=SM_NULL ? ABS(sm->rsa[1]) : sm->rsa[1]) );
   if( strappend( buf, buflen, tmp ) ) return( -1 );
   if( addtimestr( buf, buflen, sm->trsa[1] ) ) return ( -1 );
   if( strappend( buf, buflen, " METH: NJ" ) ) return( -1 );

/* Print spectral amplitude value & time for period = 3.0 */
   sprintf( tmp, "\nSA: 3.0 %.6lf TSA: ", (sm->rsa[2]!=SM_NULL ? ABS(sm->rsa[2]) : sm->rsa[2]) );
   if( strappend( buf, buflen, tmp ) ) return( -1 );
   if( addtimestr( buf, buflen, sm->trsa[2] ) ) return ( -1 );
   if( strappend( buf, buflen, " METH: NJ" ) ) return( -1 );

/* Print the eventid & event author */
   
   sprintf( tmp, "\nEVID: - -");
   if( strappend( buf, buflen, tmp ) ) return( -1 );

   sprintf( tmp, "\nAUTH: %s ", sm->net );
   if( strappend( buf, buflen, tmp ) ) return( -1 );
   if( addtimestr( buf, buflen, time(&current_time) ) ) return ( -1 );
   if( strappend( buf, buflen, "\n" ) ) return( -1 );

   return( 0 );
}


/**********************************************************
 * Converts time (double, seconds since 1970:01:01) to    *
 * a 23-character, null-terminated string in the form of  *
 *            yyyy/mm/dd hh:mm:ss.sss                     *
 * Time is displayed in UTC                               *
 * Target buffer must be 24-chars long to have room for   *
 * null-character                                         *
 **********************************************************/
char *datestr24( double t, char *pbuf, int len )
{
   time_t    tt;       /* time as time_t                  */
   struct tm stm;      /* time as struct tm               */
   int       t_msec;   /* milli-seconds part of time      */

/* Make sure target is big enough
 ********************************/
   if( len < 24 ) return( (char *)NULL );

/* Convert double time to other formats
 **************************************/
   t += 0.0005;  /* prepare to round to the nearest 1000th */
   tt     = (time_t) t;
   t_msec = (int)( (t - tt) * 1000. );
   gmtime_ew( &tt, &stm );

/* Build character string
 ************************/
   sprintf( pbuf,
           "%04d/%02d/%02d %02d:%02d:%02d.%03d",
            stm.tm_year+1900,
            stm.tm_mon+1,
            stm.tm_mday,
            stm.tm_hour,
            stm.tm_min,
            stm.tm_sec,
            t_msec );

   return( pbuf );
}


/********************************************************************
 * addtimestr() append a date string to the end of existing string  *
 *   Return -1 if result would overflow the target,                 *
 *           0 if everything went OK                                *
 ********************************************************************/
int addtimestr( char *buf, int buflen, double t )
{
   char tmp[30];

   if( t == 0.0 )
   {
     if( strappend( buf, buflen, sNullDate ) ) return( -1 );
   } else {
     datestr24( t, tmp, 30 );  
     if( strappend( buf, buflen, tmp ) ) return( -1 );
   }
   return( 0 );
}

/********************************************************************
 * strappend() append second null-terminated character string to    *
 * the first as long as there's enough room in the target buffer    * 
 * for both strings and the null-byte                               *
 ********************************************************************/
int strappend( char *s1, int s1max, char *s2 )
{
   if( (int)strlen(s1)+(int)strlen(s2)+1 > s1max ) return( -1 );
   strcat( s1, s2 );
   return( 0 );
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
    int    i, j, k;
    char   SNCL[20];
    char   Site[6], Net[5], Comp[5], Loc[5];     
    int    this, minv, maxv, val;
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
		else {logit("e", "mseed_audit: Too many channels (%d) in this file!\n", But->Sta.Nchan); this = But->Sta.Nchan;}
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
	k = 0;
	if(But->Sta.SNCL[this].npts < MAXTRACELTH) {
		for(i=0;i<mseed_hdr->num_samples;i++) { 
			val = (int)bytes[i];
			j = But->Sta.SNCL[this].npts + i;
			if(But->Sta.SNCL[this].minval > val) But->Sta.SNCL[this].minval = val;
			if(But->Sta.SNCL[this].maxval < val) But->Sta.SNCL[this].maxval = val;
			if(j < MAXTRACELTH) {
				But->Sta.SNCL[this].sigma += val;
				But->Sta.SNCL[this].fdata[j] = val;
				k++;
			} else {
				break;
			}
		}
	}
	But->Sta.SNCL[this].npts += k;
	But->Sta.SNCL[this].samprate = sps_rate(mseed_hdr->sample_rate,mseed_hdr->sample_rate_mult);
	if(But->Sta.SNCL[this].samprate > 0.0) But->Sta.SNCL[this].dt = 1.0/But->Sta.SNCL[this].samprate;
	    
	if(Debug) {
		logit("e", "mseed_audit: S_C_N_L: %s  nsamp: %d samprate: %f sigma: %f k: %d\n", 
				But->Sta.SNCL[this].SNCLnam, But->Sta.SNCL[this].npts, 
				But->Sta.SNCL[this].samprate, But->Sta.SNCL[this].sigma, k );
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
    int              i, j, k;
    
	But->Sta.mintime = But->Sta.maxtime = But->Sta.SNCL[0].mintime;
	for(i=0;i<But->Sta.Nchan;i++) {
		if(But->Sta.mintime > But->Sta.SNCL[i].mintime) But->Sta.mintime = But->Sta.SNCL[i].mintime;
		if(But->Sta.maxtime < But->Sta.SNCL[i].maxtime) But->Sta.maxtime = But->Sta.SNCL[i].maxtime;
		if(But->Sta.minval > But->Sta.SNCL[i].minval)   But->Sta.minval = But->Sta.SNCL[i].minval;
		if(But->Sta.maxval < But->Sta.SNCL[i].maxval)   But->Sta.maxval = But->Sta.SNCL[i].maxval;
		k = -1;
		for(j=0;j<But->NSCN;j++) {
			if(strcmp(But->Chan[j].SCNnam, But->Sta.SNCL[i].SNCLnam)==0) k = j;
		}
		if(k >= 0) {
			But->Sta.SNCL[i].meta = k;
			But->Sta.SNCL[i].Sens_unit = But->Chan[k].Sens_unit;
			for(j=0;j<But->Sta.SNCL[i].npts;j++) {
				But->Sta.SNCL[i].fdata[j] /= But->Chan[k].sensitivity;
			}
		} else {
			logit("e", "%s_%s_%s undefined scaling error. Ignore. \n", But->Sta.Site, But->Sta.Net, But->Sta.SNCL[i].Comp);
			return 1;
		}
		if(Debug) {
			logit("e", "%s_%s_%s %d %f \n", But->Sta.Site, But->Sta.Net, But->Sta.SNCL[i].Comp, k, But->Chan[k].sensitivity);
		}
		But->Sta.SNCL[i].mean = But->Sta.SNCL[i].sigma/But->Sta.SNCL[i].npts;
		But->Sta.SNCL[i].sm.length = But->Sta.SNCL[i].maxtime - But->Sta.SNCL[i].mintime;
	}
	
	But->T0 = floor(But->Sta.mintime) - 1.0;
	But->Tn =  ceil(But->Sta.maxtime) + 1.0;
	if((But->Tn - But->T0) <  10.0) But->Tn = But->T0 +  10.0;
	if((But->Tn - But->T0) > 120.0) But->Tn = But->T0 + 120.0;
	
	if(Debug) {
		logit("e", "%s_%s %15.2f %15.2f %15.2f \n", But->Sta.Site, But->Sta.Net, But->Sta.mintime, But->Sta.maxtime, But->Sta.maxtime - But->Sta.mintime);
		logit("e", "%s_%s %15.2f %15.2f %15.2f \n", But->Sta.Site, But->Sta.Net, But->T0, But->Tn, But->Tn - But->T0);
		logit("e", "%d %d \n", But->Sta.minval, But->Sta.maxval);
	}
	
	for(i=0;i<But->Sta.Nchan;i++) {
		if(Debug) {
			logit("e", "mean: %f meta: %d \n", But->Sta.SNCL[i].mean, But->Sta.SNCL[i].meta);
			logit("e", "mean: %f min: %d max: %d \n", But->Sta.SNCL[i].mean, But->Sta.SNCL[i].minval, But->Sta.SNCL[i].maxval);
		}
	}
	
    Decode_Time(But->T0, &But->Stime);
	
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
    int     i, j, k, nfiles, success;
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
            fprintf( stderr, "%s Error opening command file <%s>; exiting!\n", whoami, But->stationList[k] );
            exit( -1 );
        }

            /* Process all command files
             ***************************/
        while(nfiles > 0) {  /* While there are command files open */
            while(k_rd())  {      /* Read next line from active file  */
                com = k_str();         /* Get the first token from line */

                    /* Ignore blank lines & comments
                     *******************************/
                if( !com )           continue;
                if( com[0] == '#' )  continue;

                    /* Open a nested configuration file
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
                        
                if(But->Chan[j].Sens_unit == 3) But->Chan[j].Sens_gain /= 978.0;
               
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
            }
            nfiles = k_close();
        }
    }
}



     /***************************************************************
      *                          GetConfig()                        *
      *         Processes command file using kom.c functions.       *
      *           Returns -1 if any errors are encountered.         *
      ***************************************************************/

#define NCOMMAND 7             /* Number of commands in the config file */

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

    sprintf(whoami, " %s: %s: ", progname, "GetConfig");

/* Set to zero one init flag for each required command
   ***************************************************/
   for ( i = 0; i < ncommand; i++ ) init[i] = 0;
   Debug = 0;

/* Open the main configuration file
   ********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
      fprintf(stderr, "%s: Error opening configuration file <%s>\n", progname, configfile );
      return -1;
   }

/* Process all nested configuration files
   **************************************/
	while ( nfiles > 0 ) {        /* While there are config files open */
		while ( k_rd() ) {         /* Read next line from active file  */
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
					  fprintf( stderr, "%s: Invalid ModuleId <%s>. \n", progname, str );
					  fprintf( stderr, "%s: Please Register ModuleId <%s> in earthworm.d!\n", progname, str );
					  return -1;
					}
				}
				init[0] = 1;
   /*1*/     
			} else if ( k_its( "RingName" ) ) {
				if ( (str = k_str()) != NULL ) {
					if(str) strcpy( RingName, str );
					if ( (InRingKey = GetKey(str)) == -1 ) {
						fprintf( stderr, "%s: Invalid RingName <%s>. \n", progname, str );
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
			}
  /*4*/     
			else if( k_its("XMLDir") ) {
                str = k_str();
                if(str) strcpy( But->xmldir, str );
                init[4] = 1;
            }
  /*5*/     
			else if( k_its("NQFilesInDir") ) {
                str = k_str();
                if(str) strcpy( NQFilesInDir, str );
                init[5] = 1;
            }

                /* get station list path/name
                *****************************/
  /*6*/
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
                init[6] = 1;
            }
            

			else if ( k_its( "Debug" ) ) {
				But->Debug = Debug = 1;
				/* turn on the LogFile too! */
				LogFile = 1;
			}  


		else {	    /* An unknown parameter was encountered */
            fprintf( stderr, "%s: <%s> unknown parameter in <%s>\n", whoami,com, configfile );
            return -1;
         }

/* See if there were any errors processing the command
   ***************************************************/
         if ( k_err() ) {
            fprintf( stderr, "%s: Bad <%s> command in <%s>.\n", progname, com, configfile );
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
      if ( !init[0] ) fprintf(stderr, "<ModuleId> "     );
      if ( !init[1] ) fprintf(stderr, "<RingName> "     );
      if ( !init[2] ) fprintf(stderr, "<LogFile> "      );
      if ( !init[3] ) fprintf(stderr, "<HeartBeatInt> " );
      if ( !init[4] ) fprintf(stderr, "<XMLDir> "       );
      if ( !init[5] ) fprintf(stderr, "<NQFilesInDir> " );
      if ( !init[6] ) fprintf(stderr, "<StationList> "  );
      fprintf(stderr, "command(s) in <%s>.\n", configfile );
      return -1;
   }
	
   if ( GetType( "TYPE_HEARTBEAT", &TypeHB ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid message type <TYPE_HEARTBEAT>\n",progname);
      return( -1 );
   }
   if ( GetType( "TYPE_STRONGMOTIONII", &TypeSM2 ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid message type <TYPE_STRONGMOTIONII>\n",progname);
      return( -1 );
   }
   if ( GetType( "TYPE_STRONGMOTIONIII", &TypeSM3 ) != 0 ) {
      fprintf( stderr,
              "%s: Invalid message type <TYPE_STRONGMOTIONIII>\n",progname);
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
 *  nq2pgm_die attempts to gracefully die                          *
 ********************************************************************/

void nq2pgm_die( int errmap, char * str ) {

	if (errmap != -1) {
		/* use the statmgr reporting to notify of this death */
#ifdef DEBUG
		fprintf(stderr, "SENDING MESSAGE to statmgr: %d %s\n", errmap, str);
#endif DEBUG
		message_send(TypeErr, errmap, str);
	}
	
	/* this next bit must come after the possible tport_putmsg() above!! */
	if (Region.mid != -1) {
		/* we attached to an EW ring buffer */
		logit("e", "%s: exiting because %s\n", progname, str);
		tport_detach( &Region );
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
       sprintf( message, "%ld %ld\n", (long)t, (long)mypid);
    } else if( type == TypeErr ) {
       sprintf( message, "%ld %hd %s\n", (long)t, ierr, note);
       logit( "et", "%s: %s\n", progname, note );
    }
    len = strlen( message );   /* don't include the null byte in the message */

#ifdef DEBUG
		fprintf(stderr, "message_send: %ld %s\n", len, message);
#endif /* DEBUG */
   /* write the message to shared memory */
    if( tport_putmsg( &Region, &OtherLogo, len, message ) != PUT_OK ) {
        if( type == TypeHB ) {
           logit("et","%s:  Error sending heartbeat.\n", progname );
        }
        else if( type == TypeErr ) {
           logit("et","%s:  Error sending error:%d.\n", progname, ierr );
        }
    }

   return;
}


/******************************************************************************
 * file2ew_ship() Drop a message created elsewhere into the transport ring    * 
 *                using given message type and installation id.               *
 *                If given instid=0, use local installation id.               *
 ******************************************************************************/
int file2ew_ship( unsigned char type, unsigned char iid,
                  char *msg, size_t msglen )
{
   MSG_LOGO    logo;
   int         rc;

/* Fill in the logo
 *******************/ 
   if( iid==0 ) logo.instid = InstId;
   else         logo.instid = iid;
   logo.mod  = QModuleId;
   logo.type = type;

   if( LogOutgoingMsg ) file2ew_logmsg( msg, msglen );

   rc = tport_putmsg( &Region, &logo, msglen, msg );
   if( rc != PUT_OK ){
      logit("et","%s: Error putting msg in ring.\n",
             progname );
      return( -1 );
   }
   return( 0 );
}

/******************************************************************************
 * file2ew_logmsg() logs an Earthworm message in chunks that are small enough *
 *                  that they won't overflow logit's buffer                   *
 ******************************************************************************/
void file2ew_logmsg(char *msg, int msglen )
{
   char  chunk[LOGIT_LEN];
   char *next     = msg;
   char *endmsg   = msg+msglen;
   int   chunklen = LOGIT_LEN-10;

   logit( "", "\n" );
   while( next < endmsg )
   {
      strncpy( chunk, next, chunklen );
      chunk[chunklen]='\0';
      logit( "", "%s", chunk );
      next += chunklen;
   }
   return;
}


/********************************************************************
 *  write_xml writes a strong motion XML file                       *
 ********************************************************************/
int write_xml(Global *But, char *fname)
{
    char    whoami[50], string[50], xname[DIRSIZ], fnew[DIRSIZ], *cptr;
    int     i, ind;
    FILE   *fx;

    sprintf(whoami, "%s: %s: ", progname, "write_xml");
    
	strcpy(xname, fname);
	cptr = strstr(xname, ".msd");
	*cptr = 0;
	strcat(xname, ".xml");
	sprintf(fnew,"%s/%s",But->xmldir, xname );
	fx = fopen( fnew, "wb" );
	if( fx == NULL ) {
		logit( "e", "%s: trouble opening XML file: %s\n", whoami, fnew );
		return( 1 );
	} 
	else {
		fprintf(fx, "<?xml version=\"1.0\" encoding=\"US-ASCII\" standalone=\"yes\"?> \n");
		fprintf(fx, "<amplitudes agency=\"NCSN\"> \n");
		fprintf(fx, "<record> \n");
		fprintf(fx, "<timing> \n");
		fprintf(fx, "  <reference zone=\"GMT\" quality=\"0.5\"> \n");

		xmltime(But->Sta.SNCL[0].sm.t, string);

		fprintf(fx, "    <PGMTime>%s</PGMTime> \n",   string);
		fprintf(fx, "  </reference> \n");
		fprintf(fx, "  <trigger value=\"0\"/> \n");
		fprintf(fx, "</timing> \n");

		for(i = 0; i < But->Sta.Nchan; i++) {
			ind = But->Sta.SNCL[i].meta;
			fprintf(fx, "<station code=\"%s\" net=\"%s\" lat=\"%7.3f\" lon=\"%8.3f\" name=\"%s\"> \n",
				But->Chan[ind].Site, But->Chan[ind].Net, But->Chan[ind].Lat, But->Chan[ind].Lon, But->Chan[ind].SiteName);
			fprintf(fx, "<component name=\"%s\" loc=\"%s\" qual=\"%d\"> \n", But->Chan[ind].Comp, But->Chan[ind].Loc, But->Chan[ind].ShkQual);
			xmltime(But->Sta.SNCL[i].sm.tpga, string);
			fprintf(fx, "  <pga value=\"%.4f\"  units=\"cm/s/s\" datetime=\"%s\"/> \n", But->Sta.SNCL[i].sm.pga, string);
			xmltime(But->Sta.SNCL[i].sm.tpgv, string);
			fprintf(fx, "  <pgv value=\"%.5f\"  units=\"cm/s\"   datetime=\"%s\"/> \n", But->Sta.SNCL[i].sm.pgv, string);
			xmltime(But->Sta.SNCL[i].sm.tpgd, string);
			fprintf(fx, "  <pgd value=\"%.6f\"  units=\"cm\"     datetime=\"%s\"/> \n", But->Sta.SNCL[i].sm.pgd, string);
			xmltime(But->Sta.SNCL[i].sm.trsa[0], string);
			fprintf(fx, "  <sa period=\"0.3\" value=\"%f\" units=\"cm/s/s\" datetime=\"%s\"/> \n", 
												But->Sta.SNCL[i].sm.rsa[0], string);
			xmltime(But->Sta.SNCL[i].sm.trsa[1], string);
			fprintf(fx, "  <sa period=\"1.0\" value=\"%f\" units=\"cm/s/s\" datetime=\"%s\"/> \n", 
												But->Sta.SNCL[i].sm.rsa[1], string);
			xmltime(But->Sta.SNCL[i].sm.trsa[2], string);
			fprintf(fx, "  <sa period=\"3.0\" value=\"%f\" units=\"cm/s/s\" datetime=\"%s\"/> \n", 
												But->Sta.SNCL[i].sm.rsa[2], string);
			fprintf(fx, "</component> \n");
			fprintf(fx, "</station> \n\n");
		}
		fprintf(fx, "</record> \n");
		fprintf(fx, "</amplitudes> \n");
		fclose(fx);
	}
	return 0;
}

/********************************************************************
 *  print_xml logs a record of the pgm values                       *
 ********************************************************************/
int print_xml(Global *But, char *fname)
{
    char    whoami[50], string[50];
    int     i, ind;

    sprintf(whoami, "%s: %s: ", progname, "print_xml");
    
		logit( "e", "<?xml version=\"1.0\" encoding=\"US-ASCII\" standalone=\"yes\"?> \n");
		logit( "e", "<amplitudes agency=\"NCSN\"> \n");
		logit( "e", "<record> \n");
		logit( "e", "<timing> \n");
		logit( "e", "  <reference zone=\"GMT\" quality=\"0.5\"> \n");

		xmltime(But->Sta.SNCL[0].sm.t, string);

		logit( "e", "    <PGMTime>%s</PGMTime> \n",   string);
		logit( "e", "  </reference> \n");
		logit( "e", "  <trigger value=\"0\"/> \n");
		logit( "e", "</timing> \n");

		for(i = 0; i < But->Sta.Nchan; i++) {
			ind = But->Sta.SNCL[i].meta;
			logit( "e", "<station code=\"%s\" net=\"%s\" lat=\"%7.3f\" lon=\"%8.3f\" name=\"%s\"> \n",
				But->Chan[ind].Site, But->Chan[ind].Net, But->Chan[ind].Lat, But->Chan[ind].Lon, But->Chan[ind].SiteName);
			logit( "e", "<component name=\"%s\" loc=\"%s\" qual=\"%d\"> \n", But->Chan[ind].Comp, But->Chan[ind].Loc, But->Chan[ind].ShkQual);
			xmltime(But->Sta.SNCL[i].sm.tpga, string);
			logit( "e", "  <pga value=\"%.4f\"  units=\"cm/s/s\" datetime=\"%s\"/> \n", But->Sta.SNCL[i].sm.pga, string);
			xmltime(But->Sta.SNCL[i].sm.tpgv, string);
			logit( "e", "  <pgv value=\"%.5f\"  units=\"cm/s\"   datetime=\"%s\"/> \n", But->Sta.SNCL[i].sm.pgv, string);
			xmltime(But->Sta.SNCL[i].sm.tpgd, string);
			logit( "e", "  <pgd value=\"%.6f\"  units=\"cm\"     datetime=\"%s\"/> \n", But->Sta.SNCL[i].sm.pgd, string);
			xmltime(But->Sta.SNCL[i].sm.trsa[0], string);
			logit( "e", "  <sa period=\"0.3\" value=\"%.4f\" units=\"cm/s/s\" datetime=\"%s\"/> \n", 
												But->Sta.SNCL[i].sm.rsa[0], string);
			xmltime(But->Sta.SNCL[i].sm.trsa[1], string);
			logit( "e", "  <sa period=\"1.0\" value=\"%.4f\" units=\"cm/s/s\" datetime=\"%s\"/> \n", 
												But->Sta.SNCL[i].sm.rsa[1], string);
			xmltime(But->Sta.SNCL[i].sm.trsa[2], string);
			logit( "e", "  <sa period=\"3.0\" value=\"%.4f\" units=\"cm/s/s\" datetime=\"%s\"/> \n", 
												But->Sta.SNCL[i].sm.rsa[2], string);
			logit( "e", "</component> \n");
			logit( "e", "</station> \n\n");
		}
		logit( "e", "</record> \n");
		logit( "e", "</amplitudes> \n");
		
	
	return 0;
}

/********************************************************************
 *  xmltime converts time in secs to xml format                     *
 ********************************************************************/
int xmltime(double time, char *xmlt)
{
    long     minute;
    double   secs, sex;
    double   sec1970 = 11676096000.00;  /* # seconds between Carl Johnson's        */
                                        /* time 0 and 1970-01-01 00:00:00.0 GMT    */
	struct Greg  g;

	secs = time;
	secs += sec1970;
	minute = (long) (secs / 60.0);
	sex = secs - 60.0 * minute;

	grg(minute, &g);
	sprintf(xmlt, "%04d-%02d-%02dT%02d:%02d:%06.3fZ", g.year, g.month, g.day, g.hour, g.minute, sex);

	return 0;
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



/******************************************************************************
 *   subroutine for estimation of ground motion                               *
 *                                                                            *
 *   input:                                                                   *
 *           Data   - data array                                              *
 *           npts   - number of points in timeseries                          *
 *           itype  - units for timeseries. 1=disp 2=vel 3=acc                *
 *           dt     - sample spacing in sec                                   *
 *   return:                                                                  *
 *           0      - All OK                                                  *
 *           1      - error                                                   *
 *                                                                            *
 ******************************************************************************/
int peak_ground(float *Data, int npts, int itype, double dt, SM_INFO *sm)
{
    int     imax_acc, imax_vel, imax_dsp, imax_raw;
    int     imin_acc, imin_vel, imin_dsp, imin_raw;
    int     ii, kk, kpts, id[4], npd[4][2], icaus, maxtime;
    float  totint, a, tpi, omega, damp, rd, rv, aa;
    float  amax_acc, amin_acc, pmax_acc;
    float  amax_vel, amin_vel, pmax_vel;
    float  amax_dsp, amin_dsp, pmax_dsp;
    float  amax_raw, amin_raw, pmax_raw, gd[4], sd[4];

    /* Made these float arrays static because Solaris was Segfaulting on an
     * allocation this big from the stack.  These currently are 3.2 MB each 
     * DK 20030108
     ************************************************************************/
	  static float    d1[MAXTRACELTH];
	  static float    d2[MAXTRACELTH];
	  static float    d3[MAXTRACELTH];

    gd[0] = 0.05;
    gd[1] = 0.10;
    gd[2] = 0.20;
    gd[3] = 0.50;
    icaus = 1;

    tpi  = 8.0*atan(1.0);

/* Find the raw maximum and its period
 *************************************/

    demean(Data, npts);
    amaxper(npts, dt, Data, &amin_raw, &amax_raw, &pmax_raw, &imin_raw, &imax_raw);

    if(itype == 1) {  /* input data is displacement  */
        for(kk=0;kk<npts;kk++) d1[kk] = Data[kk];
        locut(d1, npts, 0.17, dt, 2, icaus);
        for(kk=1;kk<npts;kk++) {
            d2[kk] = (d1[kk] - d1[kk-1])/dt;
        }
        d2[0] = d2[1];
        demean(d2, npts);
        for(kk=1;kk<npts;kk++) {
            d3[kk] = (d2[kk] - d2[kk-1])/dt;
        }
        d3[0] = d3[1];
        demean(d3, npts);
    } else
    if(itype == 2) {  /* input data is velocity      */
        for(kk=0;kk<npts;kk++) d2[kk] = Data[kk];
        locut(d2, npts, 0.17, dt, 2, icaus);
        for(kk=1;kk<npts;kk++) {
            d3[kk] = (d2[kk] - d2[kk-1])/dt;
        }
        d3[0] = d3[1];
        demean(d3, npts);

        totint = 0.0;
        for(kk=0;kk<npts-1;kk++) {
            totint = totint + (d2[kk] + d2[kk+1])*0.5*dt;
            d1[kk] = totint;
        }
        d1[npts-1] = d1[npts-2];
        demean(d1, npts);

    } else
    if(itype == 3) {  /* input data is acceleration  */
        for(kk=0;kk<npts;kk++) d3[kk] = Data[kk];
        locut(d3, npts, 0.17, dt, 2, icaus);

        totint = 0.0;
        for(kk=0;kk<npts-1;kk++) {
            totint = totint + (d3[kk] + d3[kk+1])*0.5*dt;
            d2[kk] = totint;
        }
        d2[npts-1] = d2[npts-2];
        demean(d2, npts);

        totint = 0.0;
        for(kk=0;kk<npts-1;kk++) {
            totint = totint + (d2[kk] + d2[kk+1])*0.5*dt;
            d1[kk] = totint;
        }
        d1[npts-1] = d1[npts-2];
        demean(d1, npts);
    } else {
        return 1;
    }

/* Find the displacement(cm), velocity(cm/s), & acceleration(cm/s/s) maxima  and their periods
 *********************************************************************************************/

    amaxper(npts, dt, &d1[0], &amin_dsp, &amax_dsp, &pmax_dsp, &imin_dsp, &imax_dsp);
    amaxper(npts, dt, &d2[0], &amin_vel, &amax_vel, &pmax_vel, &imin_vel, &imax_vel);
    amaxper(npts, dt, &d3[0], &amin_acc, &amax_acc, &pmax_acc, &imin_acc, &imax_acc);

/* Find the spectral response
 ****************************/

    damp = 0.05;
    kk = 0;
    sm->pdrsa[kk] = 0.3;
    omega = tpi/sm->pdrsa[kk];
    rdrvaa(&d3[0], npts-1, omega, damp, dt, &rd, &rv, &aa, &maxtime);
    sm->rsa[kk] = aa;
    sm->trsa[kk] = sm->t + dt*maxtime;
    kk += 1;

    sm->pdrsa[kk] = 1.0;
    omega = tpi/sm->pdrsa[kk];
    rdrvaa(&d3[0], npts-1, omega, damp, dt, &rd, &rv, &aa, &maxtime);
    sm->rsa[kk] = aa;
    sm->trsa[kk] = sm->t + dt*maxtime;
    kk += 1;

    sm->pdrsa[kk] = 3.0;
    omega = tpi/sm->pdrsa[kk];
    rdrvaa(&d3[0], npts-1, omega, damp, dt, &rd, &rv, &aa, &maxtime);
    sm->rsa[kk] = aa;
    sm->trsa[kk] = sm->t + dt*maxtime;
    kk += 1;

    sm->nrsa = kk;

/* Since we are here, determine the duration of strong shaking
 *************************************************************/

    for(kk=0;kk<4;kk++) {
        id[kk] = npd[kk][1] = npd[kk][2] = 0;
        for(ii=1;ii<=npts-1;ii++) {
            a = fabs(d3[ii]/GRAVITY);
            if (a >= gd[kk]) {
                id[kk] = id[kk] + 1;
                if (id[kk] == 1) npd[kk][1] = ii;
                npd[kk][2] = ii;
            }
        }
        if (id[kk] != 0) {
            kpts = npd[kk][2] - npd[kk][1] + 1;
            sd[kk] = kpts*dt;
        } else {
            sd[kk] = 0.0;
        }
    }

    sm->pgd = fabs(amin_dsp)>fabs(amax_dsp)? fabs(amin_dsp):fabs(amax_dsp);
    sm->pgv = fabs(amin_vel)>fabs(amax_vel)? fabs(amin_vel):fabs(amax_vel);
    sm->pga = fabs(amin_acc)>fabs(amax_acc)? fabs(amin_acc):fabs(amax_acc);

    sm->tpgd = fabs(amin_dsp)>fabs(amax_dsp)? sm->t + dt*imin_dsp:sm->t + dt*imax_dsp;
    sm->tpgv = fabs(amin_vel)>fabs(amax_vel)? sm->t + dt*imin_vel:sm->t + dt*imax_vel;
    sm->tpga = fabs(amin_acc)>fabs(amax_acc)? sm->t + dt*imin_acc:sm->t + dt*imax_acc;

    return 0;
}


/******************************************************************************
 *  demean removes the mean from the n point series stored in array A.        *
 *                                                                            *
 ******************************************************************************/
void demean(float *A, int n)
{
    int       i;
    float    xm;

    xm = 0.0;
    for(i=0;i<n;i++) xm = xm + A[i];
    xm = xm/n;
    for(i=0;i<n;i++) A[i] = A[i] - xm;
}


/******************************************************************************
 *  Butterworth locut filter order 2*nroll (nroll<=8)                         *
 *   (see Kanasewich, Time Sequence Analysis in Geophysics,                   *
 *   Third Edition, University of Alberta Press, 1981)                        *
 *  written by W. B. Joyner 01/07/97                                          *
 *                                                                            *
 *  s[j] input = the time series to be filtered                               *
 *      output = the filtered series                                          *
 *      dimension of s[j] must be at least as large as                        *
 *        nd+3.0*float(nroll)/(fcut*delt)                                     *
 *  nd    = the number of points in the time series                           *
 *  fcut  = the cutoff frequency                                              *
 *  delt  = the timestep                                                      *
 *  nroll = filter order                                                      *
 *  causal if icaus.eq.1 - zero phase shift otherwise                         *
 *                                                                            *
 * The response is given by eq. 15.8-6 in Kanasewich:                         *
 *  Y = sqrt((f/fcut)**(2*n)/(1+(f/fcut)**(2*n))),                            *
 *                 where n = 2*nroll                                          *
 *                                                                            *
 * Dates: 01/07/97 - Written by Bill Joyner                                   *
 *        12/17/99 - D. Boore added check for fcut = 0.0, in which case       *
 *                   no filter is applied.  He also cleaned up the            *
 *                   appearance of the code (indented statements in           *
 *                   loops, etc.)                                             *
 *        02/04/00 - Changed "n" to "nroll" to eliminate confusion with       *
 *                   Kanesewich, who uses "n" as the order (=2*nroll)         *
 *        03/01/00 - Ported to C by Jim Luetgert                              *
 *                                                                            *
 ******************************************************************************/
void locut(float *s, int nd, float fcut, float delt, int nroll, int icaus)
{
    float    fact[8], b1[8], b2[8];
    float    pi, w0, w1, w2, w3, w4, w5, xp, yp, x1, x2, y1, y2;
    int       j, k, np2, npad;

    if (fcut == 0.0) return;       /* Added by DMB  */

    pi = 4.0*atan(1.0);
    w0 = 2.0*pi*fcut;
    w1 = 2.0*tan(w0*delt/2.0);
    w2 = (w1/2.0)*(w1/2.0);
    w3 = (w1*w1)/2.0 - 2.0;
    w4 = 0.25*pi/nroll;

    for(k=0;k<nroll;k++) {
        w5 = w4*(2.0*k + 1.0);
        fact[k] = 1.0/(1.0+sin(w5)*w1 + w2);
        b1[k] = w3*fact[k];
        b2[k] = (1.0-sin(w5)*w1 + w2)*fact[k];
    }

    np2 = nd;

    if(icaus != 1) {
        npad = 3.0*nroll/(fcut*delt);
        np2 = nd+npad;
        for(j=nd;j<np2;j++) s[j] = 0.0;
    }

    for(k=0;k<nroll;k++) {
        x1 = x2 = y1 = y2 = 0.0;
        for(j=0;j<np2;j++) {
            xp = s[j];
            yp = fact[k]*(xp-2.0*x1+x2) - b1[k]*y1 - b2[k]*y2;
            s[j] = yp;
            y2 = y1;
            y1 = yp;
            x2 = x1;
            x1 = xp;
        }
    }

    if(icaus != 1) {
        for(k=0;k<nroll;k++) {
            x1 = x2 = y1 = y2 = 0.0;
            for(j=0;j<np2;j++) {
                xp = s[np2-j-1];
                yp = fact[k]*(xp-2.0*x1+x2) - b1[k]*y1 - b2[k]*y2;
                s[np2-j-1] = yp;
                y2 = y1;
                y1 = yp;
                x2 = x1;
                x1 = xp;
            }
        }
    }

    return;
}


/******************************************************************************
 * rdrvaa                                                                     *
 *                                                                            *
 * This is a modified version of "Quake.For", originally                      *
 * written by J.M. Roesset in 1971 and modified by                            *
 * Stavros A. Anagnostopoulos, Oct. 1986.  The formulation is                 *
 * that of Nigam and Jennings (BSSA, v. 59, 909-922, 1969).                   *
 * Dates: 02/11/00 - Modified by David M. Boore, based on RD_CALC             *
 *        03/01/00 - Ported to C by Jim Luetgert                              *
 *                                                                            *
 *     acc = acceleration time series                                         *
 *      na = length of time series                                            *
 *   omega = 2*pi/per                                                         *
 *    damp = fractional damping (e.g., 0.05)                                  *
 *      dt = time spacing of input                                            *
 *      rd = relative displacement of oscillator                              *
 *      rv = relative velocity of oscillator                                  *
 *      aa = absolute acceleration of oscillator                              *
 * maxtime = time of max aa                                                   *
 ******************************************************************************/
void rdrvaa(float *acc, int na, float omega, float damp, float dt,
            float *rd, float *rv, float *aa, int *maxtime)
{
    float    omt, d2, bom, d3, omd, om2, omdt, c1, c2, c3, c4, cc, ee;
    float    s1, s2, s3, s4, s5, a11, a12, a21, a22, b11, b12, b21, b22;
    float    y, ydot, y1, z, z1, z2, ra;
    int      i;

    omt  = omega*dt;
    d2   = sqrt(1.0-damp*damp);
    bom  = damp*omega;
    d3   = 2.0*bom;
    omd  = omega*d2;
    om2  = omega*omega;
    omdt = omd*dt;
    c1 = 1.0/om2;
    c2 = 2.0*damp/(om2*omt);
    c3 = c1+c2;
    c4 = 1.0/(omega*omt);
    ee = exp(-damp*omt);
    cc = cos(omdt)*ee;
    s1 = sin(omdt)*ee/omd;
    s2 = s1*bom;
    s3 = s2 + cc;
    s4 = c4*(1.0-s3);
    s5 = s1*c4 + c2;

    a11 =  s3;          a12 = s1;
    a21 = -om2*s1;      a22 = cc - s2;

    b11 =  s3*c3 - s5;  b12 = -c2*s3 + s5 - c1;
    b21 = -s1+s4;       b22 = -s4;

    y = ydot = *rd = *rv = *aa = 0.0;
    *maxtime = -1;
    for(i=0;i<na-1;i++) {
        y1   = a11*y + a12*ydot + b11*acc[i] + b12*acc[i+1];
        ydot = a21*y + a22*ydot + b21*acc[i] + b22*acc[i+1];
        y = y1;    /* y is the oscillator output at time corresponding to index i   */
        z = fabs(y);
        if (z > *rd) *rd = z;
        z1 = fabs(ydot);
        if (z1 > *rv) *rv = z1;
        ra = -d3*ydot -om2*y1;
        z2 = fabs(ra);
        if (z2 > *aa) {
        	*aa = z2;
        	*maxtime = i;
        }
    }
}

/******************************************************************************
 *   compute maximum amplitude and its associated period                      *
 *                                                                            *
 *   input:                                                                   *
 *           npts   - number of points in timeseries                          *
 *           dt     - sample spacing in sec                                   *
 *           fc     - input timeseries                                        *
 *   output:                                                                  *
 *           amaxmm - raw maximum                                             *
 *           pmax   - period of maximum                                       *
 *           imax   - index of maxmimum point                                 *
 *                                                                            *
 ******************************************************************************/
void amaxper(int npts, float dt, float *fc, float *aminmm, float *amaxmm,
                       float *pmax, int *imin, int *imax)
{
    float    amin, amax, pp, pm, mean, frac;
    int       i, j, jmin, jmax;

    *imax = jmax = *imin = jmin = 0;
    amax = amin = *amaxmm = *aminmm = fc[0];
    *aminmm = *pmax = mean = 0.0;
    for(i=0;i<npts;i++) {
        mean = mean + fc[i]/npts;
        if (fc[i] > amax) { jmax = i; amax = fc[i]; }
        if (fc[i] < amin) { jmin = i; amin = fc[i]; }
    }

/*     compute period of maximum    */

    pp = pm = 0.0;
    if (fc[jmax] > mean) {
        j = jmax+1;
        while(fc[j] > mean && j < npts) {
            pp += dt;
            j  += 1;
        }
        frac = dt*(mean-fc[j-1])/(fc[j]-fc[j-1]);
        frac = 0.0;
        pp = pp + frac;
        j = jmax-1;
        while(fc[j] > mean && j >= 0) {
            pm += dt;
            j  -= 1;
        }
        frac = dt*(mean-fc[j+1])/(fc[j]-fc[j+1]);
        frac = 0.0;
        pm = pm + frac;
    } else {
        j = jmax+1;
        if(fc[j] < mean && j < npts) {
            pp += dt;
            j  += 1;
        }
        frac = dt*(mean-fc[j-1])/(fc[j]-fc[j-1]);
        frac = 0.0;
        pp = pp + frac;
        j = jmax-1;
        if(fc[j] < mean && j >= 0) {
            pm += dt;
            j  -= 1;
        }
        frac = dt*(mean-fc[j+1])/(fc[j]-fc[j+1]);
        frac = 0.0;
        pm = pm + frac;
    }

    *imin = jmin;
    *imax = jmax;
    *pmax = 2.0*(pm+pp);
    *aminmm = amin;
    *amaxmm = amax;

    return;
}


