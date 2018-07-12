/*************************************************************************
 *                                                                       *
 *  tideweb2ew - Tide guage data to earthworm ring                       *
 *                                                                       *
 *  This code uses an adaptation of Http Fetcher                         *
 *  Copyright (C) 2001, 2003, 2004                                       *
 *  Lyle Hanson (lhanson@users.sourceforge.net)                          *
 *                                                                       *
 *  Author: Scot Hunter, ISTI                                            * 
 *                                                                       *
 *  COMMENTS:                                                            *
 *                                                                       *
 *                                                                       *
 *************************************************************************/

#define MAIN
#define THREAD_STACK_SIZE 8192
#include "tideweb2ew.h"
#include "http_fetcher.h"
#include <math.h>

char   progname[] = "tideweb2ew";
char   NetworkCode[10];
char   StationCode[20];
char   *BaseFilePath;       /* base path for daily reports */
char   GaugeIpAddr[20];  
char   GaugeURL[100];
char   GaugeNetSta[20];       
char   InRingName[20];      /* name of transport ring for input */
char   OutRingName[20];     /* name of transport ring for output */
char   MyModName[MAX_MOD_STR];        /* speak as this module name/id      */

int err_exit;				/* an tideweb2ew_die() error number */
static unsigned char InstId = 255;
pid_t mypid;
//MSG_LOGO    OtherLogo, DataLogo, SnwSohLogo;

time_t MyLastInternalBeat;   /* time of last heartbeat into the local Earthworm ring */
unsigned  TidHeart;          /* thread id. was type thread_t on Solaris!             */

int     GaugePollSecs;
char    *FullFilePath, *FFP_Suffix;
char    HeaderLines[2][120];
static char* HeaderLines34 = "\n1_minute_update_via_GOES_secondary\nData Format: SampleTime(epochal 1/1/1970) WaterLevel SampleTime(yyyymmddhhmmss)\n";

#define   FILE_NAM_LEN 500

int Verbose;		/* spew messages to stderr */
char * Progname;		/* my name */
int ShutMeDown;		/* shut down flag */
char * Config;		/* config file name */
int offline = FALSE;


/****************************************************************/
/* EarthWorm global configs : all of these are REQUIRED from the config/desc
        file: All are set in the GetConfig() routine found in getconfig.c
*/
unsigned char TWModuleId;         /* module id for tideweb2ew */
long InRingKey;                    /* key to ring buffer tideweb2ew reads data */
long OutRingKey;                    /* key to ring buffer tideweb2ew dumps data */
int HeartBeatInt;                /* my heartbeat interval (secs) */
int LogFile;                     /* generate a logfile? */
int LOG2LogFile;                 /* put LOG channels in logfile? */

/* Optional config parameters
 ****************************/
static int    Debug = 0;
static int    Granulate = 0;
static double PWL_SCALE = 1;        // Optional multiplier for PWL values
static double BWL_SCALE = 1;        // Optional multiplier for BWL values
static char   PWL_CHAN[10] = "UTZ"; // Optional Channel code for PWL TRACEBUF2s
static char   BWL_CHAN[10] = "UTZ"; // Optional Channel code for BWL TRACEBUF2s
static char   PWL_LOC[10] = "00";   // Optional Location code for PWL TRACEBUF2s
static char   BWL_LOC[10] = "01";   // Optional Location code for BWL TRACEBUF2s

/* some globals for EW not settable in the .d file. */
SHM_INFO  Region;  
SHM_INFO  InRegion;  
SHM_INFO  OutRegion;  
MSG_LOGO DataLogo;               /* EW logo tag  for data */
MSG_LOGO SnwSohLogo;               /* EW logo tag  for SNW SOH messages */
MSG_LOGO OtherLogo;              /* EW logo tag  for err,log,heart */
unsigned char TypeTrace;         /* Trace EW type for logo */
unsigned char TypeTrace2;         /* Trace EW type for logo (supporting SCNL) */
unsigned char TypeSnwSoh;         /* SNWSOH EW type for logo */
unsigned char TypeHB;            /* HB=HeartBeat EW type for logo */
unsigned char TypeErr;           /* Error EW type for logo */

time_t	TSLastBeat;		/* time stamp since last heartbeat */
time_t	timeNow;		/* time stamp since last heartbeat */
unsigned TidHB;			/* ID for the hearbeat thread */

int numCodes = 0;       /* Number of codes on a webpage */
int reporting = 1;      /* how many Codes are being reported on? */
 
//typedef enum { PWL, BWL, LLUVIA, WT, BAT, BARO, AIRTEMP, HUMIDITY, MISSING} t_weblog;

//char *weblog_name[] = {"PWL", "BWL","LLUVIA","WT", "BAT", "BAROMETRO", "AIRTEMP", "HUMIDITY"};

typedef struct webDate {
	char mon[2];
	char sep1;
	char day[2];
	char sep2;
	char year[4];
} t_webDate;

typedef struct webTime {
	char hour[2];
	char sep1;
	char min[2];
	char sep2;
	char sec[2];
} t_webTime;

typedef struct sohDateTime {
	char year[4];
	char mon[2];
	char day[2];
	char hour[2];
	char min[2];
	char sec[2];
} t_sohDateTime;

typedef struct tdValue {
    char weblog[20];
    char sensorID[20];
    t_webTime timeStr;
    t_webDate dateStr;
    char quality[10];
    double dataVal;
    char dataUnits[5];
    time_t dateTime, oldDateTime;
    char dateTimeSOH[20], oldDateTimeSOH[20];
} t_tdValue;

typedef struct tideGauge {
    t_tdValue item[20];
} t_tideGauge;

t_tideGauge tg;
char firstReading = 1;


/* Functions in this source file
 *******************************/
int get_tg( t_tideGauge *tg, char** buffer  );
char* extract_TD( char *p, char *td );
char* extract_TD_Data( char *p, t_tdValue *v );
char* read_tdValue( t_tdValue *v, char *p );
char *trim( char* );

thr_ret Heartbeat( void * );
int tg_to_ring( t_tideGauge*, int, int );
void initiate_termination(int );	/* signal handler func */
void setuplogo(MSG_LOGO *);
void tideweb2ew_die( int errmap, char * str );
void message_send( unsigned char, short, char *);
int GetConfig( char *configfile );

/*************************************************************************
 *  main( int argc, char **argv )                                        *
 *************************************************************************/

int main( int argc, char **argv )
{
	int i;		/* indexes */
	unsigned char alert, okay;			/* flags */
    char    whoami[50];

    char* buffer = NULL;
    

	/* init some locals */
	alert = FALSE;
	okay  = TRUE;
	err_exit = -1;
    sprintf(whoami, "%s: %s: ", progname, "Main");
	mypid = getpid();       /* set it once on entry */
    if( mypid == -1 ) {
        fprintf( stderr,"%s Cannot get pid. Exiting.\n", whoami);
        return -1;
    }

	/* init some globals */
	Verbose = FALSE;	/* not well used */
	ShutMeDown = FALSE;
	Region.mid = -1;	/* init so we know if an attach happened */
	InRegion.mid = -1;	/* init so we know if an attach happened */
	OutRegion.mid = -1;	/* init so we know if an attach happened */

    /* Check command line arguments
     ******************************/
    if ( argc != 2 ) {
        fprintf( stderr, "Version: %s\nUsage: %s <configfile>\n", VER_NO, progname);
        exit( 0 );
    }

    /* Read the configuration file(s)
     ********************************/
	if (GetConfig(argv[1]) == -1) {
		tideweb2ew_die(TW2EW_DEATH_EW_CONFIG,"Too many tideweb2ew.d config problems");
		return 0;
	}
	
	/* deal with some signals */
	signal(SIGINT, initiate_termination);
	signal(SIGTERM, initiate_termination);

	/* start EARTHWORM logging */
	logit_init(argv[1], (short) TWModuleId, 256, LogFile);
	logit("t", "%s: Version %s\n", progname, VER_NO);
	logit("t", "%s: Read in config file %s\n", progname, argv[1]);
    logit("t", "%s: Primary = %s\n", progname, tg.item[0].weblog );
    if ( reporting > 1 )
       logit("t", "%s: Secondary = %s\n", progname, tg.item[1].weblog );
    logit( "t", "%s: Granulate = %d\n",   progname, Granulate );
    logit( "t", "%s: PWL_SCALE = %lf\n",  progname, PWL_SCALE );
    logit( "t", "%s: BWL_SCALE = %lf\n",  progname, BWL_SCALE );
    logit( "t", "%s: PWL_CHAN  = '%s'\n", progname, PWL_CHAN );
    logit( "t", "%s: BWL_CHAN  = '%s'\n", progname, BWL_CHAN );
    logit( "t", "%s: PWL_LOC   = '%s'\n", progname, PWL_LOC );
    logit( "t", "%s: BWL_LOC   = '%s'\n", progname, BWL_LOC );

	/* EARTHWORM init earthworm connection at this point, 
		this func() exits if there is a problem 
	*/

	/* Attach to Output shared memory ring
	 *************************************/
	tport_attach( &OutRegion, OutRingKey );
	logit( "t", "%s Attached to public memory region %s: %d\n",
	      whoami, OutRingName, OutRingKey );


   /* Start the heartbeat thread
   ****************************/
    time(&MyLastInternalBeat); /* initialize our last heartbeat time */
                          
    if ( StartThread( Heartbeat, (unsigned)THREAD_STACK_SIZE, &TidHeart ) == -1 ) {
        logit( "et","%s Error starting Heartbeat thread. Exiting.\n", whoami );
		tideweb2ew_die(err_exit, "Error starting Heartbeat thread");
        return -1;
    }

	/* sleep for 2 seconds to allow heart to beat so statmgr gets it.  */
	
	sleep_ew(2000);

	/* main thread loop acquiring DATA and LOG MSGs from COMSERV */
	while (okay) {


        if ( get_tg( &tg, &buffer ) == 0 ) {
            int newPvalue = strcmp( tg.item[0].dateTimeSOH, tg.item[0].oldDateTimeSOH );
            int newSvalue = strcmp( tg.item[1].dateTimeSOH, tg.item[1].oldDateTimeSOH );
            for ( i=0; i<numCodes; i++ )
                if ( strcmp( tg.item[i].dateTimeSOH, tg.item[i].oldDateTimeSOH ) ) {
                    tg_to_ring( &tg, newPvalue, newSvalue );
                    break;
                }
            if ( newPvalue || newSvalue ) {
            } else if ( Debug ) {
                logit( "t", "%s and %s on Webpage unchanged from %s time %s\n", tg.item[0].weblog, tg.item[1].weblog, tg.item[0].weblog, tg.item[0].dateTimeSOH );
            }
        }
        
        for ( i=0; i<GaugePollSecs; i++ ) {
            /* EARTHWORM see if we are being told to stop */
            if ( tport_getflag( &InRegion ) == TERMINATE  ) {
                tideweb2ew_die(TW2EW_DEATH_EW_TERM, "Earthworm global TERMINATE request");
                return -1;
            }
            if ( tport_getflag( &InRegion ) == mypid        ) {
                tideweb2ew_die(TW2EW_DEATH_EW_TERM, "Earthworm pid TERMINATE request");
                return -1;
            }
            /* see if we had a problem anywhere in processing the last data */
            if (ShutMeDown == TRUE) {
                tideweb2ew_die(err_exit, "tideweb2ew kill request or fatal EW error");
                return -1;
            }
            sleep_ew(1000);
        }
	}
	/* should never reach here! */
	    
	logit("t","Done for now.\n");
 
	tideweb2ew_die( -1, "clean exit" );
	
	//exit(0);
	return 0;
}

char *trim( char* s ) 
{
    int i = strlen(s)-1;
    while ( i>=0 && s[i] == ' ' )
        i--;
    if ( i>=0 )
        s[i+1] = 0;
    return s;
}

static int ff = -1;
int get_tg( t_tideGauge *tg, char** buffer  ) 
{
    char *p1;
    int i;
    t_tdValue spare, *target;
    
    if ( *buffer == NULL )
        *buffer = malloc(10000);
    ff = 0;
    //logit("t","Fetch (%d)\n",ff);
    int rv = http_fetch( ff/2==1 ? "http://1.2.3.4" : GaugeURL, buffer );
    if ( rv == -1 ) {
        if ( Debug || (offline==FALSE) ) {
            logit( "et", "Failed to fetch webpage from %s: %s\n", GaugeURL, (char*) http_strerror() );
            if ( !Debug )
                logit( "t", "Suspending error messages until re-connection\n" );
            offline = TRUE;
        }
        return -1;
    } else if ( offline ) {
        logit( "t", "Connection to %s restablished\n", GaugeURL );
        offline = FALSE;
    }
    p1 = strstr(*buffer, "</TR>");
    while ( p1 != NULL ) 
    {
        char kind[20];

        p1 = extract_TD( p1, kind );
        if ( p1 == NULL )
            break;
        for ( i=0; i<numCodes; i++ )
            if ( strcmp(kind,tg->item[i].weblog)==0 ) {
                p1 = read_tdValue( &tg->item[i], p1 );
                break;
            }
        if ( i==numCodes ) {
            if ( firstReading ) {
                strcpy( tg->item[numCodes].weblog, kind );
                numCodes++;
                target = &(tg->item[i]);
            } else  {
                // Parse the rest of the line, but ignore it
                target = &spare;
            }
            p1 = read_tdValue( target, p1 );
        }
    }
    firstReading = 0;
    return 0;
    
}

char* extract_TD( char *p, char *td ) 
{
    char *start, *finish;
    p = strstr( p, "<TD>" );
    if ( p == NULL ) 
        return NULL;
    while ( *p == ' ' )
        p++;
    start = p+4;
    p = strstr( start, "</TD>" );
    if ( p == NULL )
        return NULL;
    finish = p+5;
    if ( td != NULL ) {
       strncpy( td, start, p-start );  
       while ( td[p-start]==' ' )
            p--;
       td[p-start] = 0;  
    }
    return finish;
}

char* extract_TD_Data( char *p, t_tdValue *v ) 
{
    char td[20], *p2 = td;
    p = extract_TD( p, td );
    while ( *p2 != ' ' )
        p2++;
    *p2 = 0;
    v->dataVal = atof( td );
    strcpy( v->dataUnits, p2+1 );
    return p;
}

char* read_tdValue( t_tdValue *v, char *p ) {
    char *s;
    int len;
//     if ( p!=NULL )
//         p = extract_TD( p, NULL );
    if ( p!=NULL )
        p = extract_TD( p, v->sensorID );
    if ( p!=NULL )
        p = extract_TD( p, (char*)(&(v->timeStr)) );
    if ( p!=NULL )
        p = extract_TD( p, (char*)(&(v->dateStr)) );
    strcpy( v->oldDateTimeSOH, v->dateTimeSOH );
    t_sohDateTime *dt_string = (t_sohDateTime*)&(v->dateTimeSOH);
    struct tm time_bits;
    len = sizeof(dt_string->year);
    s = strncpy( dt_string->year, v->dateStr.year, len );
    s[len] = 0;
    time_bits.tm_year = atoi(dt_string->year) - 1900;
    len = sizeof(dt_string->mon);
    s = strncpy( dt_string->mon, v->dateStr.mon, len );
    s[len] = 0;
    time_bits.tm_mon = atoi(dt_string->mon)-1;
    len = sizeof(dt_string->day);
    s = strncpy( dt_string->day, v->dateStr.day, len );
    s[len] = 0;
    time_bits.tm_mday = atoi(dt_string->day);
    len = sizeof(dt_string->hour);
    s = strncpy( dt_string->hour, v->timeStr.hour, len );
    s[len] = 0;
    time_bits.tm_hour = atoi(dt_string->hour);
    len = sizeof(dt_string->min) ;
    s = strncpy( dt_string->min, v->timeStr.min, len );
    s[len] = 0;
    time_bits.tm_min = atoi(dt_string->min);
    len = sizeof(dt_string->sec);
    s = strncpy( dt_string->sec, v->timeStr.sec, len );
    s[len] = 0;
    time_bits.tm_sec = atoi(dt_string->sec);
    time_bits.tm_gmtoff=0;
    time_bits.tm_isdst = 0;
    time_bits.tm_zone = "GMT";
    v->oldDateTime = v->dateTime;
    //strcpy( v->oldDateTimeSOH, v->dateTimeSOH );
    v->dateTime = timegm_ew( &time_bits );
    p = extract_TD( p, v->quality );
    if ( p!=NULL )
        p = extract_TD_Data( p, v );
    return p;
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

static TracePacket trace_buffer;

/***********************************************************************
 *  tg_to_ring puts tide guage info in a TraceBuf2 and writes to ring *
 ***********************************************************************/

int tg_to_ring(t_tideGauge *tg_data, int sendPrim, int sendSec ) 
{
    TRACE2_HEADER   *trace2_hdr;		/* ew trace header */
    int              i, out_message_size;
    int32_t*         myTracePtr;
    char             sncl[20];

	trace2_hdr = (TRACE2_HEADER *) &trace_buffer.trh;
	memset((void*) trace2_hdr, 0, sizeof(TRACE2_HEADER));
	trace2_hdr->pinno = 0;		/* Unknown item */
	trace2_hdr->samprate = 60;  /* 1 per second */
	strcpy(trace2_hdr->sta,trim(StationCode));
	strcpy(trace2_hdr->net,trim(NetworkCode));
	
	strcpy(trace2_hdr->datatype, "i4");
    trace2_hdr->version[0]=TRACE2_VERSION0;
    trace2_hdr->version[1]=TRACE2_VERSION1;
	trace2_hdr->quality[1] = 0; //SBH: quality field?
	trace2_hdr->nsamp = 1;
	trace2_hdr->samprate = 1/60.0;
	myTracePtr = (int32_t*)(&trace_buffer.msg[0] + sizeof(TRACE2_HEADER));
	out_message_size = sizeof(TRACE2_HEADER)+sizeof(int)*trace2_hdr->nsamp;
	
	trace2_hdr->starttime = tg_data->item[0].dateTime;
	if ( Granulate )
	    trace2_hdr->starttime = round( trace2_hdr->starttime );
	trace2_hdr->endtime = trace2_hdr->starttime + 60;
    
    tg_data->item[0].dataVal *= PWL_SCALE;
    tg_data->item[1].dataVal *= BWL_SCALE;

    if ( sendPrim ) {
    	strcpy(trace2_hdr->chan,PWL_CHAN);
        strcpy(trace2_hdr->loc,PWL_LOC);
        sprintf(sncl, "%s_%s_%s_%s", trace2_hdr->sta,trace2_hdr->net,trace2_hdr->chan,trace2_hdr->loc);
        myTracePtr[0] = (tg_data->item[0].dataVal*1000)+0.5;
    
        if ( tport_putmsg(&OutRegion, &DataLogo, (long) out_message_size, (char*)&trace_buffer) != PUT_OK) {
            logit("et", "%s: Fatal Error sending PWL trace via tport_putmsg()\n", progname);
            ShutMeDown = TRUE;
            err_exit = TW2EW_DEATH_EW_PUTMSG;
        } else {
            if ( Verbose==TRUE )
                fprintf(stderr, "PWL SENT to EARTHWORM %s_%s_%s_%s %d Time: \n", 
                    trace2_hdr->sta, trace2_hdr->chan, 
                    trace2_hdr->net, trace2_hdr->loc, trace2_hdr->nsamp);
        }
    }
    if ( sendSec ) {
    	strcpy(trace2_hdr->chan,BWL_CHAN);
        strcpy(trace2_hdr->loc,BWL_LOC);
        sprintf(sncl, "%s_%s_%s_%s", trace2_hdr->sta,trace2_hdr->net,trace2_hdr->chan,trace2_hdr->loc);
        myTracePtr[0] = (tg_data->item[1].dataVal*1000)+0.5;

        if ( tport_putmsg(&OutRegion, &DataLogo, (long) out_message_size, (char*)&trace_buffer) != PUT_OK) {
            logit("et", "%s: Fatal Error sending BWL trace via tport_putmsg()\n", progname);
            ShutMeDown = TRUE;
            err_exit = TW2EW_DEATH_EW_PUTMSG;
        } else {
            if ( Verbose==TRUE )
                fprintf(stderr, "BWL SENT to EARTHWORM %s_%s_%s_%s %d Time: \n", 
                    trace2_hdr->sta, trace2_hdr->chan, 
                    trace2_hdr->net, trace2_hdr->loc, trace2_hdr->nsamp);
        }
    }            
    char soh_msg[200], *s = soh_msg;
    sprintf( soh_msg, "%s-%s:8", NetworkCode, StationCode );
    s += strlen( soh_msg );
    sprintf( s, ":%s=%lf",  tg_data->item[0].weblog, tg_data->item[0].dataVal );
    s += strlen( s );
    for ( i=1; i<numCodes; i++ ) {
        sprintf( s, ";%s=%lf", tg_data->item[i].weblog, tg_data->item[i].dataVal );
        s += strlen( s );
    }
    
    if ( tport_putmsg( &OutRegion, &SnwSohLogo, s - soh_msg, soh_msg ) != PUT_OK ) {
        logit("et", "%s: Fatal Error sending SNWSOH trace via tport_putmsg()\n", progname);
        ShutMeDown = TRUE;
        err_exit = TW2EW_DEATH_EW_PUTMSG;
    } else {
        FILE *fp;
        if ( Verbose==TRUE )
            fprintf(stderr, "SNWSOH SENT to EARTHWORM\n" );
        if ( strncmp( tg_data->item[0].dateTimeSOH, FFP_Suffix, 8 ) )
            memcpy( FFP_Suffix, tg_data->item[0].dateTimeSOH, 8 );                
        fp = fopen( FullFilePath, "a" );
        if ( ftell( fp ) == 0 ) {
            fwrite( HeaderLines[0], strlen(HeaderLines[0]), 1, fp );
            fwrite( "\n", 1, 1, fp );
            fwrite( HeaderLines[1], strlen(HeaderLines[1]), 1, fp );
            fwrite( HeaderLines34, strlen(HeaderLines34), 1, fp );
        }
        fprintf( fp, "%lu %1.6lf %s\n", (long)tg_data->item[0].dateTime, tg_data->item[0].dataVal, tg_data->item[0].dateTimeSOH );
        fclose( fp );
    }
    
	if(Debug) logit("t", "Done sending to EARTHWORM  \n"); 
         
	return 0;
} 

/************************************************************************/
/* signal handler that intiates a shutdown                              */
/************************************************************************/
void initiate_termination(int sigval) 
{
    signal(sigval, initiate_termination);
    ShutMeDown = TRUE;
    err_exit = TW2EW_DEATH_SIG_TRAP;
    return;
}





     /***************************************************************
      *                          GetConfig()                        *
      *         Processes command file using kom.c functions.       *
      *           Returns -1 if any errors are encountered.         *
      ***************************************************************/

#define NCOMMAND 11             /* Number of commands in the config file */

int GetConfig( char *configfile )
{
   const int ncommand = NCOMMAND;

   char     init[NCOMMAND] = {0}; /* Flags, one for each command */
   int      nmiss;                /* Number of commands that were missed */
   int      nfiles;
   int      i;

   tg.item[0].weblog[0] = tg.item[1].weblog[0] = 0;
   Debug = 0;

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
               strcpy( MyModName, str );
               if ( GetModId(str, &TWModuleId) == -1 ) {
                  fprintf( stderr, "%s: Invalid ModuleId <%s>. \n", 
				progname, str );
                  fprintf( stderr, "%s: Please Register ModuleId <%s> in earthworm.d!\n", 
				progname, str );
                  return -1;
               }
            }
            init[0] = 1;
         }
   /*1*/     
         else if ( k_its( "InRingName" ) ) {
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
         } 
  /*2*/     
         else if ( k_its( "OutRingName" ) ) {
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
         }
  /*3*/     
         else if ( k_its( "HeartBeatInt" ) ) {
            HeartBeatInt = k_int();
            init[3] = 1;
         }
  /*4*/     
         else if ( k_its( "LogFile" ) ) {
            LogFile = k_int();
            init[4] = 1;
         }
  /*5*/     
         else if ( k_its( "GaugeIpAddr" ) ) {
            if ( (str = k_str()) != NULL ) {
                strcpy( GaugeIpAddr, str );
            } else {
                fprintf( stderr, "%s: Missing addr in GaugeIpAddr.\n", progname );
                return -1;
            }
            sprintf( GaugeURL, "http://%s", GaugeIpAddr );
            init[5] = 1;
         }
  /*6*/     
         else if ( k_its( "GaugeNetSta" ) ) {
            if ( (str = k_str()) != NULL ) {
                int i = 0;
                while (*str!='.' && *str!=0 )
                    NetworkCode[i++] = *(str++);
                str++;
                i = 0;
                while (*str!='.' && *str!=0 )
                    StationCode[i++] = *(str++);
            } else {
                fprintf( stderr, "%s: Missing addr in GaugeNetSta.\n", progname );
                return -1;
            }
            init[6] = 1;
         }
  /*7*/     
         else if ( k_its( "GaugePollSecs" ) ) {
            GaugePollSecs = k_int();
            init[7] = 1;
         }
  /*8*/     
         else if ( k_its( "BaseFilePath" ) ) {
            if ( (str = k_str()) != NULL ) {
                BaseFilePath = malloc( strlen(str)+1 );
                FullFilePath = malloc( strlen(str)+FILE_NAM_LEN );
                if ( (BaseFilePath == NULL) || (FullFilePath==NULL) ) {
                    fprintf( stderr, "%s: Memory allocation failure (BaseFilePath); exiting!\n", progname );
                    return -1;
                }
                strcpy( BaseFilePath, str );
                strcpy( FullFilePath, str );
                FFP_Suffix = FullFilePath+strlen(str);
                strcpy( FFP_Suffix, "YYYYMMDD.data" );
            }
            init[8] = 1;
		} 
 /*9*/     
         else if ( k_its( "HeaderLine1" ) ) {
            if ( (str = k_str()) != NULL ) {
                strcpy( HeaderLines[0], str );
            } else {
                fprintf( stderr, "%s: Missing text in HeaderLine1.\n", progname );
                return -1;
            }
            init[9] = 1;
         }
 /*10*/     
         else if ( k_its( "HeaderLine2" ) ) {
            if ( (str = k_str()) != NULL ) {
                strcpy( HeaderLines[1], str );
            } else {
                fprintf( stderr, "%s: Missing text in HeaderLine2.\n", progname );
                return -1;
            }
            init[10] = 1;
         }
         else if ( k_its( "PrimaryCode" ) ) {
            if ( (str = k_str()) != NULL ) {
                strcpy( tg.item[0].weblog, str );
            } else {
                fprintf( stderr, "%s: Missing name in PrimaryCode.\n", progname );
                return -1;
            }
         }
         else if ( k_its( "SecondaryCode" ) ) {
            if ( (str = k_str()) != NULL ) {
                strcpy( tg.item[1].weblog, str );
            } else {
                fprintf( stderr, "%s: Missing name in PrimaryCode.\n", progname );
                return -1;
            }
         }
         else if ( k_its( "Granulate" ) ) {
            Granulate = 1;
         }
         else if ( k_its( "PWL_SCALE" ) ) {
            PWL_SCALE = k_val();
         }  
         else if ( k_its( "BWL_SCALE" ) ) {
            BWL_SCALE = k_val();
         }  
         else if ( k_its( "PWL_CHAN" ) ) {
            if ( (str = k_str()) == NULL ) {
                fprintf( stderr, "%s: Missing name in PWL_CHAN.\n", progname );
                return -1;
            } else if ( strlen(str) > 3 ) {
                fprintf( stderr, "%s: Name too long in PWL_CHAN.\n", progname );
                return -1;
            } else {
                strcpy( PWL_CHAN, str );
            }
         }  
         else if ( k_its( "BWL_CHAN" ) ) {
            if ( (str = k_str()) == NULL ) {
                fprintf( stderr, "%s: Missing name in BWL_CHAN.\n", progname );
                return -1;
            } else if ( strlen(str) > 3 ) {
                fprintf( stderr, "%s: Name too long in BWL_CHAN.\n", progname );
                return -1;
            } else {
                strcpy( BWL_CHAN, str );
            }
         }  
         else if ( k_its( "PWL_LOC" ) ) {
            if ( (str = k_str()) == NULL ) {
                fprintf( stderr, "%s: Missing name in PWL_LOC.\n", progname );
                return -1;
            } else if ( strlen(str) > 2 ) {
                fprintf( stderr, "%s: Name too long in PWL_LOC.\n", progname );
                return -1;
            } else {
                strcpy( PWL_LOC, str );
            }
         }  
         else if ( k_its( "BWL_LOC" ) ) {
            if ( (str = k_str()) == NULL ) {
                fprintf( stderr, "%s: Missing name in BWL_LOC.\n", progname );
                return -1;
            } else if ( strlen(str) > 2 ) {
                fprintf( stderr, "%s: Name too long in BWL_LOC.\n", progname );
                return -1;
            } else {
                strcpy( BWL_LOC, str );
            }
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
      if ( !init[5] ) fprintf(stderr, "<GaugeIpAddr> " );
      if ( !init[6] ) fprintf(stderr, "<GaugeNetSta> " );
      if ( !init[7] ) fprintf(stderr, "<GaugePollSecs> " );
      if ( !init[8] ) fprintf(stderr, "<BaseFilePath> " );
      if ( !init[9] ) fprintf(stderr, "<HeaderLine1> " );
      if ( !init[10] ) fprintf(stderr, "<HeaderLine2> " );
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
   if ( GetType( "TYPE_SNWSOH", &TypeSnwSoh ) != 0 ) {
      fprintf( stderr,
              "%s: Message type <TYPE_SNWSOH> not found in earthworm_global.d; exiting!\n", progname);
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
   setuplogo(&SnwSohLogo);
       SnwSohLogo.type=TypeSnwSoh;
   setuplogo(&OtherLogo);
   
   /* Wrap up Tide Gauge codes */
   if ( tg.item[0].weblog[0] == 0 ) {
    // Did not provide Primary; use PWL
    reporting = 2;
    strcpy( tg.item[0].weblog, "PWL" );
    if ( tg.item[1].weblog[0] == 0 ) {
        // Did not provide Secondary either; use BWL
        strcpy( tg.item[1].weblog, "BWL" );
    }
   } else {
        // Only use secondary if explicitly proivided
        reporting = ( tg.item[1].weblog[0] == 0 ) ? 1 : 2;
   }
   numCodes = reporting;
   
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
   logo->mod = TWModuleId;
   logo->instid = InstId;
}


/********************************************************************
 *  tideweb2ew_die attempts to gracefully die                          *
 ********************************************************************/

void tideweb2ew_die( int errmap, char * str ) 
{

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

	//exit(0);
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


