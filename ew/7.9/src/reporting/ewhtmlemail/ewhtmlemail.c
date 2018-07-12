/******************************************************************************
 *                                EWHTMLEMAIL                                 *
 *                                                                            *
 * Simplified graphical email alert for earthworm using google APIs.          *
 *                                                                            *
 *                                                                            *
 * Description:                                                               *
 * Produces an web file (html) containing graphical information on detected   *
 * earthquakes. The presented graphical information consists on a map of the  *
 * computed hypocenter and reporting stations as well as the seismic traces   *
 * retrieved from a WaveServer. The graphical information is produced using   * 
 * google maps and google charts APIs. As a consequence, correct visualization*
 * of the web file requires an internet connection. The web files may be sent *
 * to a set of recipients via email. In this case, ewhtmlemail uses           *
 * sendmail, which is common with several linux distributions.                *
 * The code is a combination of seisan_report to retrieve hyp2000arc          *
 * messages, and gmew to access and retrieve data from a set of waveservers.  *
 * The code includes custom functions to generate the requests to the google  *
 * APIs and embeded these on the output html file.                            *
 * Besides the typical .d configuration file, ewhtmlemail requires a css file *
 * with some basic style configurations to include in the output html.        *
 *                                                                            *
 * Written by R. Luis from CVARG in July, 2011                                *
 * Contributions and testing performed by Jean-Marie Saurel, from OVSM/IPGP   *
 * Later modifications for features and blat, by Paul Friberg                 *
 *
 * Two important URLs to follow: 
 *    https://developers.google.com/maps/documentation/staticmaps/
 *    https://developers.google.com/chart/interactive/docs/reference/
 *****************************************************************************/
#define VERSION_STR "1.0.78 - 2015-05-12"

#define MAX_STRING_SIZE 1024	/* used for many static string place holders */

/* Includes
 **********/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <geo.h>

#include <earthworm.h>
#include <transport.h>
#include <ws_clientII.h>
#include <trace_buf.h>
#include <swap.h>
#include <kom.h>
#include <read_arc.h>
#include <rw_mag.h>
#include <site.h>
#include <chron3.h>
#include <gd.h>
#include <gdfonts.h>
#include <fleng.h>
#include <ioc_filter.h> /*header containing filtering functions*/
#ifndef _WINNT
#include <unistd.h>
#endif
#include "kml_event.h"


/* Defines
 *********/
#define MAX_STATIONS 200
#define MAX_GET_SAMP 2400		/* Number of samples in a google chart */
#define MAX_REQ_SAMP 600		/* Maximum number of samples per google request */
#define MAX_GET_CHAR 5000		/* Maximum number of characters in a google GET request */
#define MAX_POL_LEN 120000		/* Maximum number of samples in a trace gif */
#define MAX_WAVE_SERVERS 10
#define MAX_ADDRESS 80
#define MAX_PORT 6
#define MAX_EMAIL_RECIPIENTS 20
#define MAX_EMAIL_CHAR 160
#define DEFAULT_SUBJECT_PREFIX "EWalert"
#define DEFAULT_GOOGLE_STATIC_MAPTYPE "hybrid"
#define OTHER_WORLD_DISTANCE_KM 100000000.0
#define DUMMY_MAG -9.0



/* Structures
 ************/
 
// Waveservers 
typedef struct {
   char wsIP[MAX_ADDRESS];
   char port[MAX_PORT];
} WAVESERV;

// Email Recipients
// TODO: Upgrade to have different parameters for each email recipient
typedef struct {
   char address[MAX_EMAIL_CHAR];
   double min_magnitude;
   double max_distance;
} EMAILREC;


/* Functions in this file
 ************************/
void config(char*);
void lookup(void);
void status( unsigned char, short, char* );
int process_message(HypoArc *arc, MAG_INFO *mag, MAG_INFO *mw);
void GoogleMapRequest(char*, SITE**, int, double, double);
void GoogleChartRequest(char*, int*, double*, int, int, char, int);
char simpleEncode(int);
int searchSite(char*, char*, char*, char*);
char* getWSErrorStr( int, char* );
int getWStrbf( char*, int*, SITE*, double, double );
int trbufresample( int*, int, char*, int, double, double, int );
double fbuffer_add( double*, int, double );
int makeGoogleChart( char*, int*, int, char *, double, int, int );
void gdImagePolyLine( gdImagePtr, gdPointPtr, int, int );
int trbf2gif( char*, int, gdImagePtr, double, double, int, int );
int pick2gif(gdImagePtr, double, char *, double, double, int);
int gmtmap(char *request,SITE **sites, int nsites,double hypLat, double hypLon,int idevt);
void InsertShortHeaderTable(FILE *htmlfile, HypoArc *arc, MAG_INFO *mag, MAG_INFO *mag_w, char Quality);
void InsertHeaderTable(FILE *htmlfile, HypoArc *arc, MAG_INFO *mag, MAG_INFO *mag_w, char Quality);
unsigned char* base64_encode( size_t* output_length, const unsigned char *data, size_t input_length );


/* Globals
 *********/
static SHM_INFO   InRegion; // shared memory region to use for input
static pid_t      MyPid;    // Our process id is sent with heartbeat

/* Things to read or derive from configuration file
 **************************************************/
static int        LogSwitch;              // 0 if no logfile should be written 
static long       HeartbeatInt;           // seconds between heartbeats        
static long       MaxMessageSize = 4096;  // size (bytes) of largest msg     
static int        Debug = 0;              // 0=no debug msgs, non-zero=debug    
static int        DebugEmail = 0;              // 0=no debug msgs, non-zero=debug    
static MSG_LOGO  *GetLogo = NULL;         // logo(s) to get from shared memory  
static short      nLogo = 0;              // # of different logos
static int        MAX_SAMPLES = 60000;    // Number of samples to get from ws 
static long       wstimeout = 5;          // Waveserver Timeout in Seconds  
static char       HTMLFile[MAX_STRING_SIZE];          // Base name of the out files
static char       EmailProgram[MAX_STRING_SIZE];      // Path to the email program
//static char       StyleFile[MAX_STRING_SIZE];         // Path to the style (css) file
static char       SubjectPrefix[MAX_STRING_SIZE];     // defaults to EWalert, settable now
static char       KMLdir[MAX_STRING_SIZE];          // where to put the kml files, dir must exist
static char       KMLpreamble[MAX_STRING_SIZE];          // where to find the KML preamble needed for this.
static int        nwaveservers = 0;       // Number of waveservers
static int        nemailrecipients = 0;   // Number of email recipients
static double     TimeMargin = 10;        // Margin to download samples.
static double     DurationMax = 144.;     // Maximum duration to show., truncate to this if greater (configurable)
static int     	  UseBlat = 0;        // use blat syntaxi as the mail program instead of a UNIX like style
static int     	  UseRegionName = 0;        // put Region Name from FlynnEnghdal region into table at top, off by default
static char       BlatOptions[MAX_STRING_SIZE];      // apply any options needed after the file....-server -p profile etc...
static char       StaticMapType[MAX_STRING_SIZE];      // optional, specification of google map type
static int        UseUTC = 0;         // use UTC in the timestamps of the html page
static int        DontShowMd = 0;         // set to 1 to not show Md value
static int        DontUseMd = 0;         // set to 1 to not use Md value in MinMag decisions
static int        ShowDetail = 0;     // show more detail on event
static int        ShortHeader = 0;     // show a more compact header table
static char       MinQuality = 'D';	// by default send any email with quality equal to or greater than this
static char       DataCenter[MAX_STRING_SIZE];  // Datacenter name to be presented in the resume table.(optional)  
static int			SPfilter = 0;						// Shortperiod filtering (optional)
static char       Cities[MAX_STRING_SIZE];			//cities filepath to be printed with gmt in the output map
static int        GMTmap=0;						//Enable GMT map generation (Optional)
static int			StationNames=0;					// Station names in the GMT map (Optional if GMTmap is enable)	(Optional)	
static char       MapLegend[MAX_STRING_SIZE];		// Map which contains the map legend to be drawn in the GMT map (optional)
static int       Mercator=1;							// Mercator projection. This is the default map projection.					
static int       Albers=0;							// Albers projection. 
/* the following are all related to ML use, later other mags? */
static int        UseML = 0;     // wait for ML before releasing event
static int        UseMW = 0;     // wait for MW before releasing event
static int        MagWaitSeconds = 5;     // wait for ML or Mw for this number of seconds and then release if it doesn't show up
static long       last_qid = 0;  	// last qid released
static time_t     last_qid_time = 0;  // last qid at time 
static int        qid_counter = 0;  // counter for qids in queue awaiting MLs

static double     center_lat = 0.0;  // for distance check from center point
static double     center_lon = 0.0;  // for distance check from center point


/* RSL: 2012.10.10 Gif options */
static int		  UseGIF = 0;            // Use GIF files instead of google charts
static int        TraceWidth = 600;        // Width of the GIF files
static int        TraceHeight = 61;        // Height of the GIF files

// Array of waveservers
static WAVESERV   waveservers[MAX_WAVE_SERVERS];
// Array of email recipients
static EMAILREC   emailrecipients[MAX_EMAIL_RECIPIENTS];


/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long               InRingKey = 0;    // key of transport ring for input
static unsigned char      InstId = 0;       // local installation id
static unsigned char      MyModId = 0;      // Module Id for this program
static unsigned char      TypeHeartBeat = 0;
static unsigned char      TypeError = 0;
static unsigned char      TypeHYP2000ARC = 0;
static unsigned char      TypeMagnitude = 0;
static unsigned char      TypeNoMagnitude = 0;
static unsigned char      TypeTraceBuf2 = 0;


/* Error messages used by ewhtmlemail
 ************************************/
#define  ERR_MISSGAP       0   // sequence gap in transport ring         
#define  ERR_MISSLAP       1   // missed messages in transport ring      
#define  ERR_TOOBIG        2   // retreived msg too large for buffer     
#define  ERR_NOTRACK       3   // msg retreived; tracking limit exceeded 
static char  Text[150];        // string for log/error messages          

#define MAX_QUAKES 	100	// should make this configurable at some point
#define MAX_MAG_CHANS 1000	// this SHOULD be enough

/* Main program starts here
 **************************/
int main(int argc, char **argv) 
{

   time_t        timeNow;          // current time               
   time_t        timeLastBeat;     // time last heartbeat was sent
   char         *msgbuf;           // buffer for msgs from ring    
   long          recsize;          // size of retrieved message    
   MSG_LOGO      reclogo;          // logo of retrieved message    
   unsigned char seq;
   int           i, res;
   HypoArc		 *arc;		   // a hypoinverse ARC message
   MAG_INFO	 mag_l;		   // a magnitude message containing ML, after ARC
   MAG_INFO	 mag_w;		   // a magnitude message containing Mw
   MAG_INFO	 mag_tmp;	   // a temp magnitude message containing Mw or ML
   HypoArc		 *arc_list[MAX_QUAKES];
   long qid_l=0;
   long qid_w=0;


   /* Check command line arguments
    ******************************/
   if (argc != 2) 
   {
      fprintf(stderr, "Usage: ewhtmlemail <configfile>\n");
      fprintf(stderr, "Version: %s\n", VERSION_STR );
      return EW_FAILURE;
   }
   
   /* Initialize name of log-file & open it
    ***************************************/
   logit_init(argv[1], 0, 256, 1);
   

   /* Read the configuration file(s)
    ********************************/
   config(argv[1]);
   

   /* Lookup important information from earthworm.d
    ***********************************************/
   lookup();
   

   /* Set logit to LogSwitch read from configfile
    *********************************************/
   logit_init(argv[1], 0, 256, LogSwitch);
   logit("", "ewhtmlemail: Read command file <%s>\n", argv[1]);

   logit("t", "ewhtmlemail: version %s\n", VERSION_STR);

   /* Get our own process ID for restart purposes
    *********************************************/
   if( (MyPid = getpid()) == -1 )
   {
      logit ("e", "ewhtmlemail: Call to getpid failed. Exiting.\n");
      free( GetLogo );
      exit( -1 );
   }
   
   /* zero out the arc_list messages - 
    takes care of condition where Mag may 
    come in but no ARC (eqfiltered out)
    ***********************************/
   for (i=0; i< MAX_QUAKES; i++) arc_list[i] = NULL;

   /* Allocate the message input buffer
    ***********************************/
   if ( !( msgbuf = (char *) malloc( (size_t)MaxMessageSize+1 ) ) )
   {
      logit( "et",
         "ewhtmlemail: failed to allocate %d bytes"
         " for message buffer; exiting!\n", MaxMessageSize+1 );
      free( GetLogo );
      exit( -1 );
   }
  
   /* allocate mag chans */
   if ( UseML && !( mag_l.pMagAux = (char *) malloc(sizeof(MAG_CHAN_INFO)*MAX_MAG_CHANS) ) )
   {
      logit( "et",
         "ewhtmlemail: failed to allocate %d bytes"
         " for ML buffer; exiting!\n", sizeof(MAG_CHAN_INFO)*MAX_MAG_CHANS  );
      exit( -1 );
   }
   if (UseML) 
   {
      mag_l.size_aux = sizeof(MAG_CHAN_INFO)*MAX_MAG_CHANS;
   }
   if ( UseMW && !( mag_w.pMagAux = (char *) malloc(sizeof(MAG_CHAN_INFO)*MAX_MAG_CHANS) ) )
   {
      logit( "et",
         "ewhtmlemail: failed to allocate %d bytes"
         " for MW buffer; exiting!\n", sizeof(MAG_CHAN_INFO)*MAX_MAG_CHANS  );
      exit( -1 );
   }
   if (UseMW) 
   {
      mag_w.size_aux = sizeof(MAG_CHAN_INFO)*MAX_MAG_CHANS;
   }
   if ( (UseMW || UseML) && !( mag_tmp.pMagAux = (char *) malloc(sizeof(MAG_CHAN_INFO)*MAX_MAG_CHANS) ) )
   {
      logit( "et",
         "ewhtmlemail: failed to allocate %d bytes"
         " for M temp buffer; exiting!\n", sizeof(MAG_CHAN_INFO)*MAX_MAG_CHANS  );
      exit( -1 );
   }
   if (UseMW||UseML) 
   {
      mag_tmp.size_aux = sizeof(MAG_CHAN_INFO)*MAX_MAG_CHANS;
   }
   

   /* Attach to shared memory rings
    *******************************/
   tport_attach( &InRegion, InRingKey );
   logit( "", "ewhtmlemail: Attached to public memory region: %ld\n",
      InRingKey );
      

   /* Force a heartbeat to be issued in first pass thru main loop
    *************************************************************/
   timeLastBeat = time(&timeNow) - HeartbeatInt - 1;
   

   /* Flush the incoming transport ring on startup
    **********************************************/
   while( tport_copyfrom(&InRegion, GetLogo, nLogo,  &reclogo,
      &recsize, msgbuf, MaxMessageSize, &seq ) != GET_NONE );


   /*-------------------- setup done; start main loop ------------------------*/


   while ( tport_getflag( &InRegion ) != TERMINATE  &&
      tport_getflag( &InRegion ) != MyPid ) 
   {
      /* send heartbeat
       ***************************/
      if( HeartbeatInt  &&  time(&timeNow)-timeLastBeat >= HeartbeatInt ) 
      {
         timeLastBeat = timeNow;
         status( TypeHeartBeat, 0, "" );
      }


      /* Get msg & check the return code from transport
       ************************************************/
      res = tport_copyfrom( &InRegion, GetLogo, nLogo, &reclogo,
                              &recsize, msgbuf, MaxMessageSize, &seq );
      switch( res )
      {
         case GET_OK:      /* got a message, no errors or warnings         */
            break;

         case GET_NONE:    /* no messages of interest, check again later   */
            if (UseML||UseMW) {
                time_t now;
                now = time(NULL);
                if (last_qid!= 0 && (now - last_qid_time) > MagWaitSeconds) {
                    /* release all qids since last_qid */
   	            logit( "t", "ewhtmlemail: NO Mags observed from mag engines for %d seconds, freeing them up!\n", MagWaitSeconds);
                    for (i=0; i< qid_counter; i++) {
                        arc = arc_list[(last_qid+i)%MAX_QUAKES];
			if (arc != NULL) {
				if (qid_l == qid_w && UseMW && UseML && qid_l == arc->sum.qid) {
                        	   process_message(arc, &mag_l, &mag_w);
                        	   qid_l = qid_w = 0;      /* reset qid trackers */
				} else if (UseML && qid_l == arc->sum.qid) {
                        	   process_message(arc, &mag_l, NULL);
                        	   qid_l = 0;         /* reset qid tracker */
				} else if (UseMW && qid_w == arc->sum.qid) {
                        	   process_message(arc, NULL, &mag_w);
                        	   qid_w = 0;         /* reset qid tracker */
				} else {
                        	   process_message(arc, NULL, NULL);
				}
                 		free_phases(arc);
                 		free(arc);
				arc_list[(last_qid+i)%MAX_QUAKES] = NULL;
			}
                    }
                    qid_counter = 0;
                    last_qid = 0;
                    last_qid_time = 0;
                }
            }
            sleep_ew(1000); /* milliseconds */
            continue;

         case GET_NOTRACK: /* got a msg, but can't tell if any were missed */
            sprintf( Text,
               "Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
               reclogo.instid, reclogo.mod, reclogo.type );
            status( TypeError, ERR_NOTRACK, Text );
            break;

         case GET_MISS_LAPPED:     /* got a msg, but also missed lots      */
            sprintf( Text,
               "Missed msg(s) from logo (i%u m%u t%u)",
               reclogo.instid, reclogo.mod, reclogo.type );
            status( TypeError, ERR_MISSLAP, Text );
            break;

         case GET_MISS_SEQGAP:     /* got a msg, but seq gap               */
            sprintf( Text,
               "Saw sequence# gap for logo (i%u m%u t%u s%u)",
               reclogo.instid, reclogo.mod, reclogo.type, seq );
            status( TypeError, ERR_MISSGAP, Text );
            break;

         case GET_TOOBIG:  /* next message was too big, resize buffer      */
            sprintf( Text,
               "Retrieved msg[%ld] (i%u m%u t%u) too big for msgbuf[%ld]",
               recsize, reclogo.instid, reclogo.mod, reclogo.type,
               MaxMessageSize );
            status( TypeError, ERR_TOOBIG, Text );
            continue;

         default:         /* Unknown result                                */
            sprintf( Text, "Unknown tport_copyfrom result:%d", res );
            status( TypeError, ERR_TOOBIG, Text );
            continue;
      }

      /* Received a message. Start processing
       **************************************/
      msgbuf[recsize] = '\0'; /* Null terminate for ease of printing */
      
      if (Debug) {
         logit("t", "ewhtmlemail: Received message, reclogo.type=%d\n",
                                                       (int)(reclogo.type));
      }
      
      if ( reclogo.type == TypeMagnitude && (UseML || UseMW)) {
	 long qid = 0;
	 if (rd_mag(msgbuf, recsize, &mag_tmp) != 0) {
	     logit( "et", "ewhtmlemail: Error parsing mag message: %s\n", msgbuf);
             continue;
	 }
         logit( "t", "ewhtmlemail: received magnitude type %s for qid=%s\n", MagNames[mag_tmp.imagtype], mag_tmp.qid );
         if (UseMW && mag_tmp.imagtype == MAGTYPE_MOMENT) {
	     rd_mag(msgbuf, recsize, &mag_w);
             qid = qid_w = atoi(mag_w.qid);
	 } else {
	     rd_mag(msgbuf, recsize, &mag_l);
             qid = qid_l = atoi(mag_l.qid);
	 }
         arc = arc_list[qid%MAX_QUAKES]; 
         /* check that qid actually matches summary qid because we may have a mag post-restart with no qid */
         if (arc != NULL && arc->sum.qid == qid) {
	    if (UseMW && arc->sum.qid == qid_w && UseML && arc->sum.qid == qid_l) {
		/* case 1, got them both and both are current to this arc */
                      process_message(arc, &mag_l, &mag_w);
                      qid_l = qid_w = 0;         /* reset qid trackers */
	    } else if (!UseML && UseMW && arc->sum.qid == qid_w) {
		        /* case 2 only care about Mw , got it */
                      process_message(arc, NULL, &mag_w);
                      qid_w = 0;                 /* reset qid tracker */
	    } else if (!UseMW && UseML && arc->sum.qid == qid_l) {
		        /* case 3 only care about ML, got it */
                      process_message(arc, &mag_l, NULL);
                      qid_l = 0;                 /* reset qid tracker */
	    } else {
			/* case 4 we want both, but only got 1 so far */
			continue;
	    }
	    /* if we reach here, we processed the message with the accompanying mags */
            free_phases(arc);
            free(arc);
            arc_list[qid%MAX_QUAKES] = NULL;
            if (last_qid == qid) {
                /* reset since this was the last one */
                last_qid = 0;
                last_qid_time = 0;
                qid_counter = 0;
            } 
         } else { 
	    logit( "et", "ewhtmlemail: NO matching hypocenter found for mag message with qid=%ld!\n", qid);
	 }
      } else if (reclogo.type == TypeNoMagnitude && (UseML || UseMW)) {
         long qid = 0;
         int magTypeIdx = 0;
         char qidStr[EVENTID_SIZE];
         if (rd_nomag_msg(msgbuf,qidStr,NULL,&magTypeIdx,EVENTID_SIZE) != 0) {
             logit( "et", "ewhtmlemail: Error parsing no-mag message: %s\n", msgbuf);
             continue;
         }
         logit( "t", "ewhtmlemail: received no-magnitude type %s msg for qid=%s\n",
                                             MagNames[magTypeIdx], qidStr );
         if (UseMW && magTypeIdx == MAGTYPE_MOMENT) {
             mag_w.imagtype = MAGTYPE_UNDEFINED;      /* indicate no magnitude */
             mag_w.szmagtype[0] = '\0';
             strncpy(mag_w.qid,qidStr,EVENTID_SIZE);
             mag_w.mag = 0.0;
             qid = qid_w = atoi(qidStr);
         } else {
             mag_l.imagtype = MAGTYPE_UNDEFINED;      /* indicate no magnitude */
             mag_l.szmagtype[0] = '\0';
             mag_l.mag = 0.0;
             strncpy(mag_l.qid,qidStr,EVENTID_SIZE);
             qid = qid_l = atoi(qidStr);
         }
         arc = arc_list[qid%MAX_QUAKES];
         /* check that qid actually matches summary qid because we may have a mag post-restart with no qid */
         if (arc != NULL && arc->sum.qid == qid) {
            if (UseMW && arc->sum.qid == qid_w && UseML && arc->sum.qid == qid_l) {
                /* case 1, got them both and both are current to this arc */
                      process_message(arc, &mag_l, &mag_w);
                      qid_l = qid_w = 0;         /* reset qid trackers */
            } else if (!UseML && UseMW && arc->sum.qid == qid_w) {
                        /* case 2 only care about Mw , got it */
                      process_message(arc, NULL, &mag_w);
                      qid_w = 0;                 /* reset qid tracker */
            } else if (!UseMW && UseML && arc->sum.qid == qid_l) {
                        /* case 3 only care about ML, got it */
                      process_message(arc, &mag_l, NULL);
                      qid_l = 0;                 /* reset qid tracker */
            } else {
                        /* case 4 we want both, but only got 1 so far */
                        continue;
            }
            /* if we reach here, we processed the message */
            free_phases(arc);
            free(arc);
            arc_list[qid%MAX_QUAKES] = NULL;
            if (last_qid == qid) {
                /* reset since this was the last one */
                last_qid = 0;
                last_qid_time = 0;
                qid_counter = 0;
            }
         } else {
            logit( "et", "ewhtmlemail: No matching hypocenter found for no-mag message with qid=%ld\n", qid);
         }
      } else if ( reclogo.type == TypeHYP2000ARC ) {
         arc = calloc(1, sizeof(HypoArc));
         parse_arc(msgbuf, arc);
         if (!UseML && !UseMW) {
             process_message(arc, NULL, NULL);
	     free(arc);
	     free_phases(arc);
         } else {
             /* store it for later use */
             arc_list[arc->sum.qid%MAX_QUAKES] = arc;
   	     logit( "t", "ewhtmlemail: storing hypocenter while waiting for possible ML, qid=%ld!\n", arc->sum.qid);
             if (last_qid == 0) {
                last_qid = arc->sum.qid;	/* set it to this one */
                last_qid_time = time(NULL);
                qid_counter = 1;
             } else {
                qid_counter++;
             }
         }
      }

   }

   /* free allocated memory */
   free( GetLogo );
   free( msgbuf  );

   /* detach from shared memory */
   tport_detach( &InRegion );

   /* write a termination msg to log file */
   logit( "t", "ewhtmlemail: Termination requested; exiting!\n" );
   fflush( stdout );
   return( 0 );
}

/*****************************************************************************
 *  config() processes command file(s) using kom.c functions;                *
 *                    exits if any errors are encountered.                   *
 *****************************************************************************/
#define ncommand 7        /* # of required commands you expect to process */
void config(char *configfile) 
{
   char init[ncommand]; /* init flags, one byte for each required command */
   int nmiss; /* number of required commands that were missed   */
   char *com;
   char *str;
   int nfiles;
   int success;
   char processor[15];
   int i;
   char cp[80];
   BlatOptions[0]=0;
   KMLdir[0]=0;
   strcpy(SubjectPrefix, DEFAULT_SUBJECT_PREFIX);
   strcpy(StaticMapType, DEFAULT_GOOGLE_STATIC_MAPTYPE);

   /* Set to zero one init flag for each required command
    *****************************************************/
   for (i = 0; i < ncommand; i++)
      init[i] = 0;
   nLogo = 0;

   /* Open the main configuration file
    **********************************/
   nfiles = k_open(configfile);
   if (nfiles == 0) 
   {
      logit("e",
         "ewhtmlemail: Error opening command file <%s>; exiting!\n",
         configfile);
      exit(-1);
   }

   /* Process all command files
    ***************************/
   while (nfiles > 0) /* While there are command files open */ 
   {
      while (k_rd()) /* Read next line from active file  */ 
      {
         com = k_str(); /* Get the first token from line */

         /* Ignore blank lines & comments
          *******************************/
         if (!com) continue;
         if (com[0] == '#') continue;

         /* Open a nested configuration file
          **********************************/
         if (com[0] == '@') {
            success = nfiles + 1;
            nfiles = k_open(&com[1]);
            if (nfiles != success) 
            {
               logit("e",
                  "ewhtmlemail: Error opening command file <%s>; exiting!\n",
                  &com[1]);
               exit(-1);
            }
            continue;
         }

         /* Process anything else as a command
          ************************************/
   /*0*/ if (k_its("LogFile")) 
         {
            LogSwitch = k_int();
            if (LogSwitch < 0 || LogSwitch > 2) 
            {
               logit("e",
                  "ewhtmlemail: Invalid <LogFile> value %d; "
                  "must = 0, 1 or 2; exiting!\n", LogSwitch);
               exit(-1);
            }
            init[0] = 1;
         } 
   /*1*/ else if (k_its("MyModuleId"))
         {
            if ((str = k_str()) != NULL) {
               if (GetModId(str, &MyModId) != 0) 
               {
                   logit("e",
                      "ewhtmlemail: Invalid module name <%s> "
                      "in <MyModuleId> command; exiting!\n", str);
                   exit(-1);
               }
            }
            init[1] = 1;
            }
   /*2*/ else if (k_its("InRing")) 
         {
            if ( (str = k_str()) != NULL) 
            {
               if ((InRingKey = GetKey(str)) == -1) 
               {
                  logit("e",
                     "ewhtmlemail: Invalid ring name <%s> "
                     "in <InRing> command; exiting!\n", str);
                  exit(-1);
               }
            }
            init[2] = 1;
         } 
   /*3*/ else if (k_its("HeartbeatInt"))
         {
            HeartbeatInt = k_long();
            init[3] = 1;
         }
   /*4*/ else if (k_its("GetLogo")) 
         {
            if ((str = k_str()) != NULL) 
            {
               unsigned char noMagType = 0;
               int cfgNumLogo;
               MSG_LOGO *tlogo = NULL;
                   /* get configured value for TYPE_NOMAGNITUDE (if any) */
               GetType("TYPE_NOMAGNITUDE", &noMagType);
                     /* set # of logos (one more if TYPE_NOMAGNITUDE avail) */
               cfgNumLogo = nLogo + ((noMagType != 0) ? 3 : 2);
               tlogo = (MSG_LOGO *)
                             realloc(GetLogo, cfgNumLogo * sizeof (MSG_LOGO));
               if (tlogo == NULL)
               {
                  logit("e", "ewhtmlemail: GetLogo: error reallocing"
                     " %d bytes; exiting!\n",
                     cfgNumLogo * sizeof (MSG_LOGO));
                  exit(-1);
               }
               GetLogo = tlogo;

               if (GetInst(str, &GetLogo[nLogo].instid) != 0) 
               {
                  logit("e",
                     "ewhtmlemail: Invalid installation name <%s>"
                     " in <GetLogo> cmd; exiting!\n", str);
                  exit(-1);
               }
               GetLogo[nLogo + 1].instid = GetLogo[nLogo].instid;
               if ((str = k_str()) != NULL) 
               {
                  if (GetModId(str, &GetLogo[nLogo].mod) != 0) 
                  {
                     logit("e",
                        "ewhtmlemail: Invalid module name <%s>"
                        " in <GetLogo> cmd; exiting!\n", str);
                     exit(-1);
                  }
                  GetLogo[nLogo + 1].mod = GetLogo[nLogo].mod;
                  if (GetType("TYPE_HYP2000ARC", &GetLogo[nLogo++].type) != 0)
                  {
                     logit("e",
                        "ewhtmlemail: Invalid message type <TYPE_HYP2000ARC>"
                        "; exiting!\n");
                     exit(-1);
                  }
                  if (GetType("TYPE_MAGNITUDE", &GetLogo[nLogo++].type) != 0)
                  {
                     logit("e",
                        "ewhtmlemail: Invalid message type <TYPE_MAGNITUDE>"
                        "; exiting!\n");
                     exit(-1);
                  }
                  if (noMagType != 0)
                  {  /* TYPE_NOMAGNITUDE is available; setup logo for it */
                      GetLogo[nLogo].instid = GetLogo[nLogo-1].instid;
                      GetLogo[nLogo].mod = GetLogo[nLogo-1].mod;
                      GetLogo[nLogo++].type = noMagType;
                  }
               }
               else
               {
                  logit("e", "ewhtmlemail: No module name "
                        "in <GetLogo> cmd; exiting\n");
                  exit(-1);
               }
            }
            else
            {
               logit("e", "ewhtmlemail: No installation name "
                          "in <GetLogo> cmd; exiting\n");
               exit(-1);
            }
            init[4] = 1;
         }
   /*5*/ else if (k_its("Debug")) 
         {
            Debug = k_int();
            init[5] = 1;
         }
         else if (k_its("DebugEmail")) DebugEmail = k_int();
         else if ( k_its("MaxMessageSize") ) 
         {
            MaxMessageSize = k_long();
         }
         else if ( k_its("MAX_SAMPLES") ) 
         {
            MAX_SAMPLES = k_int();
         }
         else if ( k_its("MinQuality") ) 
         {
            str = k_str();
	    MinQuality = str[0];
         }
         else if ( k_its("WSTimeout") ) 
         {
            wstimeout = k_int() * 1000;
         }
   /*6*/ else if ( k_its("HTMLFile") ) 
         {
            strcpy(HTMLFile, k_str());
            init[6] = 1;
         }
         else if ( k_its("EmailProgram") ) 
         {
            strcpy(EmailProgram, k_str());
         }
         else if ( k_its("KML") ) 
         {
            strcpy(KMLdir, k_str());
            strcpy(KMLpreamble, k_str());
         }
         else if ( k_its("StyleFile") ) 
         {
         	/* RSL: Removed styles */
            //strcpy(StyleFile, k_str());
         }
         else if ( k_its("SubjectPrefix") ) 
         {
            strcpy(SubjectPrefix, k_str());
         }
         else if ( k_its("TimeMargin") ) 
         {
            TimeMargin = k_val();
         }
         else if ( k_its("WaveServer") )
         {
            if (nwaveservers < MAX_WAVE_SERVERS)
            {
               strcpy(cp,k_str());
               strcpy(waveservers[nwaveservers].wsIP, strtok (cp, ":"));
               strcpy(waveservers[nwaveservers].port, strtok (NULL, ":"));
               nwaveservers++;
            }
            else
            {
               logit("e", "ewhtmlemail: Excessive number of waveservers. Exiting.\n");
               exit(-1);
            }
         }
         else if ( k_its("UseRegionName") )
         {
 		UseRegionName = 1;
	 }
         else if ( k_its("UseBlat") )
         {
 		UseBlat = 1;
	 }
         else if ( k_its("BlatOptions") )
         {
 		strcpy(BlatOptions, k_str());
	 }
         else if ( k_its("StaticMapType") )
         {
 		strcpy(StaticMapType, k_str());
	 }
         else if ( k_its("DontShowMd") )
         {
 		DontShowMd = 1;
	 }
         else if ( k_its("DontUseMd") )
         {
 		DontUseMd = 1;
	 }
         else if ( k_its("UseUTC") )
         {
 		UseUTC = 1;
	 }
	 else if ( k_its("MaxDuration") ) 
          {
             DurationMax = k_val();
          }
	 else if ( k_its("CenterPoint") ) 
          {
             center_lat = k_val();
             center_lon = k_val();
          }
         else if ( k_its("MagWaitSeconds") )
         {
                MagWaitSeconds = k_int();
	 } 
	 else if ( k_its("DataCenter") )
         {
 		strcpy(DataCenter, strtok(k_str(),"\""));
	 }
	 else if (k_its("SPfilter")) 
         {
            SPfilter = 1;
         }
         else if (k_its("GMTmap")) 
         {
            GMTmap = 1;
         }
         else if (k_its("Cities")) 
         {
            strcpy(Cities, k_str());
         }
         else if (k_its("StationNames")) 
         {
            StationNames = 1;
         }
         else if (k_its("MapLegend")) 
         {
            strcpy(MapLegend, k_str());
         }
         else if ( k_its("Mercator") )
         {
			Mercator = 1;
	     }
	     else if ( k_its("Albers") )
	     {
			Albers=1;
			Mercator=0;
		 }
         else if ( k_its("UseML") )
         {
 		UseML = 1;
	 }
         else if ( k_its("UseMW") )
         {
 		UseMW = 1;
	 }
         else if ( k_its("ShowDetail") )
         {
 		ShowDetail = 1;
	 }
         else if ( k_its("ShortHeader") )
         {
 		ShortHeader = 1;
	 }
	 else if ( k_its("UseGIF") )
         {
 		    UseGIF = 1;
	 }
	 else if ( k_its("TraceWidth") ) 
         {
            TraceWidth = (int) k_int();
         }
	 else if ( k_its("TraceHeight") ) 
         {
            TraceHeight = (int) k_int();
         }
         else if ( k_its("EmailRecipientWithMinMagAndDist") )
         {
		    if (nemailrecipients<MAX_EMAIL_RECIPIENTS)
			{
				strcpy(emailrecipients[nemailrecipients].address, k_str());
				emailrecipients[nemailrecipients].min_magnitude = k_val();
				emailrecipients[nemailrecipients].max_distance = k_val();
			 	nemailrecipients++;
			} 
			else
			{
				logit("e", "ewhtmlemail: Excessive number of email recipients. Exiting.\n");
                		exit(-1);
			}
         }
         else if ( k_its("EmailRecipientWithMinMag") )
         {
		    if (nemailrecipients<MAX_EMAIL_RECIPIENTS)
			{
				strcpy(emailrecipients[nemailrecipients].address, k_str());
				emailrecipients[nemailrecipients].min_magnitude = k_val();
				emailrecipients[nemailrecipients].max_distance  = OTHER_WORLD_DISTANCE_KM;
			 	nemailrecipients++;
			} 
			else
			{
				logit("e", "ewhtmlemail: Excessive number of email recipients. Exiting.\n");
                		exit(-1);
			}
         }
         else if ( k_its("EmailRecipient") )
         {
		    if (nemailrecipients<MAX_EMAIL_RECIPIENTS)
			{
				strcpy(emailrecipients[nemailrecipients].address, k_str());
				emailrecipients[nemailrecipients].min_magnitude = DUMMY_MAG;
				emailrecipients[nemailrecipients].max_distance  = OTHER_WORLD_DISTANCE_KM;
			 	nemailrecipients++;
			} 
			else
			{
				logit("e", "ewhtmlemail: Excessive number of email recipients. Exiting.\n");
                		exit(-1);
			}
         }
         /* Some commands may be processed by other functions
          ***************************************************/
         else if( site_com() )  strcpy( processor, "site_com" );
         /* Unknown command
          *****************/
         else 
         {
            logit("e", "ewhtmlemail: <%s> Unknown command in <%s>.\n",
               com, configfile);
            continue;
         }

         /* See if there were any errors processing the command
          *****************************************************/
         if (k_err())
         {
            logit("e",
               "ewhtmlemail: Bad <%s> command in <%s>; exiting!\n",
               com, configfile);
            exit(-1);
         }
      }
      nfiles = k_close();
   }

   /* After all files are closed, check init flags for missed commands
    ******************************************************************/
   nmiss = 0;
   for (i = 0; i < ncommand; i++) if (!init[i]) nmiss++;
   if (nmiss) 
   {
      logit("e", "ewhtmlemail: ERROR, no ");
      if (!init[0]) logit("e", "<LogFile> ");
      if (!init[1]) logit("e", "<MyModuleId> ");
      if (!init[2]) logit("e", "<InRing> ");
      if (!init[3]) logit("e", "<HeartbeatInt> ");
      if (!init[4]) logit("e", "<GetLogo> ");
      if (!init[5]) logit("e", "<Debug> ");
      if (!init[6]) logit("e", "<HTMLFile> ");
      logit("e", "command(s) in <%s>; exiting!\n", configfile);
      exit(-1);
   }
   return;
}

/*********************************************************************
 *  lookup( )   Look up important info from earthworm.h tables       *
 *********************************************************************/
void lookup(void) {
    /* Look up installations of interest
     *********************************/
    if (GetLocalInst(&InstId) != 0) {
        logit("e",
                "ewhtmlemail: error getting local installation id; exiting!\n");
        exit(-1);
    }

    /* Look up message types of interest
     *********************************/
    if (GetType("TYPE_HEARTBEAT", &TypeHeartBeat) != 0) {
        logit("e",
                "ewhtmlemail: Invalid message type <TYPE_HEARTBEAT>; exiting!\n");
        exit(-1);
    }
    if (GetType("TYPE_ERROR", &TypeError) != 0) {
        logit("e",
                "ewhtmlemail: Invalid message type <TYPE_ERROR>; exiting!\n");
        exit(-1);
    }
    if (GetType("TYPE_HYP2000ARC", &TypeHYP2000ARC) != 0) {
        logit("e",
                "ewhtmlemail: Invalid message type <TYPE_HYP2000ARC>; exiting!\n");
        exit(-1);
    }
    if (GetType("TYPE_MAGNITUDE", &TypeMagnitude) != 0) {
        logit("e",
                "ewhtmlemail: Invalid message type <TYPE_MAGNITUDE>; exiting!\n");
        exit(-1);
    }
    if (GetType("TYPE_TRACEBUF2", &TypeTraceBuf2) != 0) {
        logit("e",
                "ewhtmlemail: Invalid message type <TYPE_TRACEBUF2>; exiting!\n");
        exit(-1);
    }
    if (GetType("TYPE_NOMAGNITUDE", &TypeNoMagnitude) != 0) {
        logit("e",
                "ewhtmlemail: Warning: Msg type TYPE_NOMAGNITUDE not found\n");
    }
    return;
}

/******************************************************************************
 * status() builds a heartbeat or error message & puts it into                *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
void status(unsigned char type, short ierr, char *note) {
    MSG_LOGO logo;
    char msg[256];
    long size;
    time_t t;

    /* Build the message
     *******************/
    logo.instid = InstId;
    logo.mod = MyModId;
    logo.type = type;

    time(&t);

    if (type == TypeHeartBeat) {
        sprintf(msg, "%ld %ld\n", (long) t, (long) MyPid);
    } else if (type == TypeError) {
        sprintf(msg, "%ld %hd %s\n", (long) t, ierr, note);
        logit("et", "ewhtmlemail: %s\n", note);
    }

    size = strlen(msg); /* don't include the null byte in the message */

    /* Write the message to shared memory
     ************************************/
    if (tport_putmsg(&InRegion, &logo, size, msg) != PUT_OK) {
        if (type == TypeHeartBeat) {
            logit("et", "ewhtmlemail:  Error sending heartbeat.\n");
        } else if (type == TypeError) {
            logit("et", "ewhtmlemail:  Error sending error:%d.\n", ierr);
        }
    }

    return;
}













/* the magnitude type from hypoinverse: need to change if ML gets produced in future */
#define MAG_TYPE_STRING "Md"
#define MAG_MSG_TYPE_STRING "ML"
#define MAG_MSG_MWTYPE_STRING "Mw"

void InsertHeaderTable(FILE *htmlfile, HypoArc *arc, MAG_INFO *mag, MAG_INFO *mag_w, char Quality) {
	char		timestr[80];					/* Holds time messages */
	time_t 		ot;
	struct tm 	*timeinfo;
	char 		*grname[36];          /* Flinn-Engdahl region name */
	struct tm * (*timefunc)(const time_t *);
	char		time_type[30];					/* Time type UTC or local */

		timefunc = localtime;
		if( UseUTC )
		{ 
			timefunc = gmtime;
			strcpy( time_type, "UTC" );
		}
		ot = ( time_t )( arc->sum.ot - GSEC1970 );
		timeinfo = timefunc ( &ot );
		strftime( timestr, 80, "%Y.%m.%d %H:%M:%S", timeinfo ); // Prepare origin time
		
		// Table header
		fprintf( htmlfile, "<table id=\"DataTable\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n" );
		if(strlen(DataCenter) > 0 )
		{
		     fprintf( htmlfile, "<tr bgcolor=\"000060\"><th><font size=\"1\" face=\"Sans-serif\" color=\"FFFFFF\">Data Center: %s</font><th><tr>\n", DataCenter );
	        }
		// Event ID
		fprintf( htmlfile, "<tr bgcolor=\"000060\"><th><font size=\"1\" face=\"Sans-serif\" color=\"FFFFFF\">EW Event ID: %ld</font><th><tr>\n", arc->sum.qid );
		
		// Origin time
		fprintf( htmlfile, "<tr class=\"alt\" bgcolor=\"DDDDFF\"><td><font size=\"1\" face=\"Sans-serif\">Origin time:</font></td><td><font size=\"1\" face=\"Sans-serif\">%s %s</font></td><tr>\n",
				timestr, time_type );
		// Seismic Region
		if (UseRegionName) {
			FlEngLookup(arc->sum.lat, arc->sum.lon, grname, NULL);	
			fprintf( htmlfile, "<tr bgcolor=\"DDDDFF\"><td><font size=\"1\" face=\"Sans-serif\">Region:</font></td><td><font size=\"1\" face=\"Sans-serif\">%-36s</font></td><tr>\n",
				*grname );
		}
				
		// Latitude
		fprintf( htmlfile, "<tr class=\"alt\"><td><font size=\"1\" face=\"Sans-serif\">Latitude:</font></td><td><font size=\"1\" face=\"Sans-serif\">%7.4f</font></td><tr>\n", arc->sum.lat );
		
		// Longitude
		fprintf( htmlfile, "<tr bgcolor=\"DDDDFF\"><td><font size=\"1\" face=\"Sans-serif\">Longitude:</font></td><td><font size=\"1\" face=\"Sans-serif\">%8.4f</font></td><tr>\n",
				arc->sum.lon );
				
		// Depth
		fprintf( htmlfile, "<tr class=\"alt\"><td><font size=\"1\" face=\"Sans-serif\">Depth:</font></td><td><font size=\"1\" face=\"Sans-serif\">%4.1f km</font></td><tr>\n", arc->sum.z );
		
                if (arc->sum.mdwt == 0 && DontShowMd==0) 
                {
		// Coda magnitude
		    fprintf( htmlfile, "<tr bgcolor=\"DDDDFF\"><td><font size=\"1\" face=\"Sans-serif\">Coda Magnitude:</font></td>"
				"<td><font size=\"1\" face=\"Sans-serif\">N/A %s nobs=None</font></td><tr>\n", 
				MAG_TYPE_STRING );
                } else if (DontShowMd == 0)
                {
		// Coda magnitude
		    fprintf( htmlfile, "<tr bgcolor=\"DDDDFF\"><td><font size=\"1\" face=\"Sans-serif\">Coda Magnitude:</font></td>"
				"<td><font size=\"1\" face=\"Sans-serif\">%4.1f %s nobs=%d</font></td><tr>\n", 
				arc->sum.Mpref, MAG_TYPE_STRING, (int) arc->sum.mdwt );
                }
		// Optional amplitude magnitude
		if( UseML && mag != NULL )
		{
			fprintf( htmlfile, "<tr><td><font size=\"1\" face=\"Sans-serif\"><a href=\"#ML\">Local Magnitude:"
					"</a></font></td><td><font size=\"1\" face=\"Sans-serif\">%4.1f &plusmn;%3.1f %s nobs=%d</font></td><tr>\n", 
					mag->mag, mag->error, MAG_MSG_TYPE_STRING, mag->nchannels );
		}
		
		if( UseMW && mag_w != NULL )
		{
			fprintf( htmlfile, "<tr><td><font size=\"1\" face=\"Sans-serif\"><a href=\"#MW\">Moment Magnitude:"
					"</a></font></td><td><font size=\"1\" face=\"Sans-serif\">%4.1f &plusmn;%3.1f %s nobs=%d</font></td><tr>\n", 
					mag_w->mag, mag_w->error, MAG_MSG_MWTYPE_STRING, mag_w->nchannels );
		}
		/* Event details
		 ***************/
		if( ShowDetail )
		{
   			
   			// RMS
			fprintf( htmlfile, "<tr bgcolor=\"DDDDFF\" class=\"alt\"><td><font size=\"1\" face=\"Sans-serif\">RMS Error:</font></td><td><font size=\"1\" face=\"Sans-serif\">%5.2f s</font></td><tr>\n", 
					arc->sum.rms);
					
			// Horizontal error
			fprintf( htmlfile, "<tr><td><font size=\"1\" face=\"Sans-serif\">Horizontal Error:"
					"</font></td><td><font size=\"1\" face=\"Sans-serif\">%5.2f km</font></td><tr>\n", arc->sum.erh );
					
			// Vertical error
			fprintf( htmlfile, "<tr bgcolor=\"DDDDFF\" class=\"alt\"><td><font size=\"1\" face=\"Sans-serif\">Depth Error:</font></td><td><font size=\"1\" face=\"Sans-serif\">%5.2f km</font></td><tr>\n", 
					arc->sum.erz );
					
			// Azimuthal gap
			fprintf( htmlfile, "<tr><td><font size=\"1\" face=\"Sans-serif\">Azimuthal Gap:</font></td>"
					"<td><font size=\"1\" face=\"Sans-serif\">%d Degrees</font></td><tr>\n", arc->sum.gap );
					
			// Number of phases
			fprintf( htmlfile, "<tr bgcolor=\"DDDDFF\" class=\"alt\"><td><font size=\"1\" face=\"Sans-serif\">Total Phases:</font></td><td><font size=\"1\" face=\"Sans-serif\">%d</font></td><tr>\n",
					arc->sum.nphtot);
			
			// Used phases
			fprintf( htmlfile, "<tr><td><font size=\"1\" face=\"Sans-serif\">Total Phases Used:</font></td>"
					"<td><font size=\"1\" face=\"Sans-serif\">%d</font></td><tr>\n", arc->sum.nph );
			
			// Number of S phases
			fprintf( htmlfile, "<tr bgcolor=\"DDDDFF\" class=\"alt\"><td><font size=\"1\" face=\"Sans-serif\">Num S Phases Used:</font></td><td><font size=\"1\" face=\"Sans-serif\">%d</font></td><tr>\n",
					arc->sum.nphS );
					
			// Average quality
			fprintf(htmlfile, "<tr><td><font size=\"1\" face=\"Sans-serif\">Quality:</font></td>"
					"<td><font size=\"1\" face=\"Sans-serif\">%c</font></td><tr>\n", Quality);
		}
		
		// Finish reference table
		fprintf( htmlfile, "</table><br><br>\n" );
         
}
         
/* this is a squished down table  for the header info*/
void InsertShortHeaderTable(FILE *htmlfile, HypoArc *arc, MAG_INFO *mag, MAG_INFO *mag_w, char Quality) {
	char		timestr[80];					/* Holds time messages */
	time_t 		ot;
	struct tm 	*timeinfo;
	char 		*grname[36];          /* Flinn-Engdahl region name */
	struct tm * (*timefunc)(const time_t *);
	char		time_type[30];					/* Time type UTC or local */

		timefunc = localtime;
		if( UseUTC )
		{ 
			timefunc = gmtime;
			strcpy( time_type, "UTC" );
		}
		ot = ( time_t )( arc->sum.ot - GSEC1970 );
		timeinfo = timefunc ( &ot );
		strftime( timestr, 80, "%Y.%m.%d %H:%M:%S", timeinfo ); // Prepare origin time
		
		// Table header
		fprintf( htmlfile, "<table id=\"DataTable\" border=\"0\" cellspacing=\"1\" cellpadding=\"0\">\n" );
		if(strlen(DataCenter) > 0 )
		{
		     fprintf( htmlfile, 
		     		"<tr bgcolor=\"000060\">"
		     			"<th colspan=4><font size=\"1\" face=\"Sans-serif\" color=\"FFFFFF\">Data Center: %s</font></th>"
		     		"</tr>\n", DataCenter );
	        }
		// Event ID
		fprintf( htmlfile,
				"<tr bgcolor=\"000060\">"
					"<th colspan=4><font size=\"1\" face=\"Sans-serif\" color=\"FFFFFF\">EW Event ID: %ld</font></th>"
				"</tr>\n", arc->sum.qid );
		// Seismic Region
		if (UseRegionName) {
			FlEngLookup(arc->sum.lat, arc->sum.lon, grname, NULL);	
			fprintf( htmlfile, 
				"<tr bgcolor=\"DDDDFF\">"
					"<td colspan=2><font size=\"2\" face=\"Sans-serif\"><b>Region:</b></font></td>"
					"<td colspan=2><font size=\"2\" face=\"Sans-serif\">%-36s</font></td>"
				"</tr>\n",
				*grname );
		}
				
		
		// Origin time
		fprintf( htmlfile, 
				"<tr bgcolor=\"DDDDFF\" class=\"alt\">"
					"<td><font size=\"1\" face=\"Sans-serif\"><b>Origin time:</b></font></td>"
					"<td><font size=\"1\" face=\"Sans-serif\">%s %s</font></td>\n",
				timestr, time_type );
   		// RMS
		fprintf( htmlfile, 
				"<td><font size=\"1\" face=\"Sans-serif\"><b>RMS Error:</b></font></td>"
				"<td><font size=\"1\" face=\"Sans-serif\">%5.2f s</font></td>"
				"</tr>\n", 
				arc->sum.rms);
		// Latitude
		// Longitude
		fprintf( htmlfile, 
				"<tr>"
					"<td><font size=\"1\" face=\"Sans-serif\"><b>Latitude, Longitude:</b></font></td>"
					"<td><font size=\"1\" face=\"Sans-serif\">%7.4f, %8.4f</font></td>\n", arc->sum.lat, arc->sum.lon );
		// Horizontal error
		fprintf( htmlfile, 
					"<td><font size=\"1\" face=\"Sans-serif\"><b>Horizontal Error:</b></font></td>"
					"<td><font size=\"1\" face=\"Sans-serif\">%5.2f km</font></td>"
					"</tr>\n", arc->sum.erh );
				
		// Depth
		fprintf( htmlfile, 
				"<tr bgcolor=\"DDDDFF\" class=\"alt\">"
					"<td><font size=\"1\" face=\"Sans-serif\"><b>Depth:</b></font></td>"
					"<td><font size=\"1\" face=\"Sans-serif\">%4.1f km</font></td>", arc->sum.z );
		// Vertical error
		fprintf( htmlfile, 
					"<td><font size=\"1\" face=\"Sans-serif\"><b>Depth Error:</b></font></td>"
					"<td><font size=\"1\" face=\"Sans-serif\">%5.2f km</font></td>"
				"</tr>\n", 
				arc->sum.erz );
		
		// Coda magnitude
		if (arc->sum.mdwt == 0 && DontShowMd == 0) {
			fprintf( htmlfile, 
				"<tr>"
					"<td><font size=\"1\" face=\"Sans-serif\"><b>Coda Magnitude:</b></font></td>"
					"<td><font size=\"1\" face=\"Sans-serif\">N/A %s</font></td>"
					"<td><font size=\"1\" face=\"Sans-serif\">nobs=0</font></td>"
				"</tr>\n", 
				 MAG_TYPE_STRING );
		} else if (DontShowMd == 0) {
			fprintf( htmlfile, 
				"<tr>"
					"<td><font size=\"1\" face=\"Sans-serif\"><b>Coda Magnitude:</b></font></td>"
					"<td><font size=\"1\" face=\"Sans-serif\">%4.1f %s</font></td>"
					"<td><font size=\"1\" face=\"Sans-serif\">nobs=%d</font></td>"
				"</tr>\n", 
				arc->sum.Mpref, MAG_TYPE_STRING, (int) arc->sum.mdwt );
		}	
				
		// Optional amplitude magnitude
		if( UseML && mag != NULL )
		{
			fprintf( htmlfile, 
				"<tr>"
					"<td><font size=\"1\" face=\"Sans-serif\"><a href=\"#ML\"><b>Local Magnitude:</b>"
						"</a></font></td>"
					"<td><font size=\"1\" face=\"Sans-serif\">%4.1f %s</font></td>"
					"<td><font size=\"1\" face=\"Sans-serif\">&plusmn;%3.1f nobs=%d</font></td>"
				"</tr>\n", 
				mag->mag, MAG_MSG_TYPE_STRING, mag->error, mag->nchannels );
		}
		if( UseMW && mag_w != NULL )
		{
			fprintf( htmlfile, 
				"<tr>"
					"<td><font size=\"1\" face=\"Sans-serif\"><a href=\"#MW\"><b>Moment Magnitude:</b>"
						"</a></font></td>"
					"<td><font size=\"1\" face=\"Sans-serif\">%4.1f %s</font></td>"
					"<td><font size=\"1\" face=\"Sans-serif\">&plusmn;%3.1f nobs=%d</font></td>"
				"</tr>\n", 
				mag_w->mag, MAG_MSG_MWTYPE_STRING, mag_w->error, mag_w->nchannels );
		}
		
		// Average quality
		fprintf( htmlfile, 
				"<tr bgcolor=\"DDDDFF\" class=\"alt\">"
					"<td><font size=\"1\" face=\"Sans-serif\"><b>Quality:</b></font></td>"
					"<td><font size=\"1\" face=\"Sans-serif\">%c</font></td>", Quality);
					
		// Azimuthal gap
		fprintf( htmlfile, 
					"<td><font size=\"1\" face=\"Sans-serif\"><b>Azimuthal Gap:</b></font></td>"
					"<td><font size=\"1\" face=\"Sans-serif\">%d Degrees</font></td>"
				"</tr>\n", arc->sum.gap );
					
		// Number of phases
		fprintf( htmlfile, 
				"<tr>"
					"<td><font size=\"1\" face=\"Sans-serif\"><b>Total Phases:</b></font></td>"
					"<td><font size=\"1\" face=\"Sans-serif\">%d</font></td>", arc->sum.nphtot);
			
		// Used phases
		fprintf( htmlfile, 
					"<td><font size=\"1\" face=\"Sans-serif\"><b>Total Phases Used:</b></font></td>"
					"<td><font size=\"1\" face=\"Sans-serif\">%d</font></td>"
				"</tr>\n", arc->sum.nph );
			
		// Number of S phases
		fprintf( htmlfile, 
				"<tr bgcolor=\"DDDDFF\" class=\"alt\">"
					"<td><font size=\"1\" face=\"Sans-serif\"><b>Num S Phases Used:</b></font></td>"
					"<td><font size=\"1\" face=\"Sans-serif\">%d</font> </td>"
				"</tr>\n",
				arc->sum.nphS );
					
		
		// Finish reference table
		fprintf( htmlfile, "</table><br><br>\n" );
         
}

/******************************************************************************
 * process_message() Processes a message to find if its a real event or not   *
 ******************************************************************************/
int process_message(HypoArc * arc, MAG_INFO *mag_l, MAG_INFO *mag_w) {

	double		starttime = 0;					/* Start time for all traces */
	double		endtime = 0;					/* End time for all traces */
	double		dur;							/* Duration of the traces */
	int 		i, pos;							/* Generic counters */
	int 		gsamples[MAX_GET_SAMP];			/* Buffer for resampled traces */
	char		chartreq[50000];					/* Buffer for chart requests or GIFs */		/* 2013.05.29 for base64 */
	SITE		*sites[MAX_STATIONS];			/* Selected sites */
	char		phases[MAX_STATIONS][3];			/* Phase types of each trace */
	double		arrivalTimes[MAX_STATIONS];		/* Arrival times of each trace */
        double		residual[MAX_STATIONS]; 		/* phase residual */
	int		coda_lengths[MAX_STATIONS];		/* Coda lengths of each trace */
	double		coda_mags[MAX_STATIONS]; 		/* station coda mags > 0 if used */
	int		coda_weights[MAX_STATIONS];			/* Coda Weight codes for each trace */
	int		weights[MAX_STATIONS];			/* Weight codes for each trace */
	double		distances[MAX_STATIONS];		/* Distance from each station */
	struct tm * (*timefunc)(const time_t *);
	char		time_type[30];					/* Time type UTC or local */
	int		codaLen;						/* Holds the coda length absolute */
	int		nsites = 0;						/* Number of stations in the arc msg */
	char		system_command[MAX_STRING_SIZE];/* System command to call email prog */
	char		kml_filename[MAX_STRING_SIZE];	/* Name of kml file to be generated */
//	FILE		*cssfile;						/* CSS File to be embedded in email */
	FILE		*htmlfile;						/* HTML file */
	FILE		*header_file;					/* Email header file */
//	char		ch;
	time_t		ot, st;							/* Times */
	struct tm 	*timeinfo;
//        char            *basefilename;
	char		timestr[80];					/* Holds time messages */
	char		fullFilename[250];				/* Full html file name */
	char		hdrFilename[250];				/* Full header file name */
	char		*buffer;						/* Buffer for raw tracebuf2 messages */
	int		bsamplecount;					/* Length of buffer */
	char		giffilename[256];				/* Name of GIF files */
	gdImagePtr	gif;							/* Pointer for GIF image */
	FILE 		*giffile;						/* GIF file */
	int		BACKCOLOR;						/* Background color of GIF images */
	int		TRACECOLOR;						/* Trace color in GIF images */
	int		PICKCOLOR; 						/* Pick color in GIF images */
	char 		Quality;		/* event quality */
	FILE*		gifHeader;						/* Header for GIF attachments with sendmail */
	char		gifHeaderFileName[257];			/* GIF Header filename */
	unsigned char* gifbuffer;
   	unsigned char*	base64buf;
   	size_t	gifbuflen;
   	size_t	base64buflen;
//   	char	gifFileName[256];
   	char	contentID[256];   


	if (arc == NULL) {
		return FALSE;
        }

        /* clear ptr if magnitude type undefined for TYPE_NOMAGNITUDE msg */
	if (mag_l != NULL && mag_l->imagtype == MAGTYPE_UNDEFINED) {
                mag_l = NULL;
        }
        if (mag_w != NULL && mag_w->imagtype == MAGTYPE_UNDEFINED) {
                mag_w = NULL;
        }

	/* Initialize kml file name and time type
	 ****************************************/
	kml_filename[0] = 0;
	strcpy( time_type, "Local Time" );
	starttime = arc->sum.ot;
   

	/* Read phases using read_arc.c function
	 * For each phase, try to find the station in the sites file
	 * Then store phase data (arrival, type, coda length, etc)
	 * and update starttime and endtime to include all phases
	 * and corresponding coda lengths.
	 ***********************************************************/
	for (i=0; i< arc->num_phases; i++)
	{
		//If this phase does not have a site, continue to next
		if( strlen( arc->phases[i]->site ) == 0 )
			continue;
	
		// Search this site on the sites file
		pos = searchSite( arc->phases[i]->site, 
				arc->phases[i]->comp,
				arc->phases[i]->net,
				arc->phases[i]->loc) ;
		// If site is not found, continue to next phase
		if( pos == -1 )
		{
			logit( "et", "ewhtmlemail: Unable to find %s.%s.%s.%s on the site file\n", 
            		arc->phases[i]->site,
            		arc->phases[i]->comp,
            		arc->phases[i]->net,
            		arc->phases[i]->loc );
			continue;
		}      
      
      
		// New station, store its pointer
		sites[nsites] = &Site[pos];
		
		
		// Select phase type and arrival time
                //  example:SC24 SC  HHZ PnU2201408290349 4187-148 97    0   0   0      0 0  0   0   0 33513904  0     216  0  0  99   0W  -- 0
		if( arc->phases[i]->Plabel == 'P' || arc->phases[i]->Ponset == 'P' )
		{
			phases[nsites][0] = 'P'; // Store phase type
                        if (arc->phases[i]->Plabel == ' ' || arc->phases[i]->Plabel == 'P') {
			  phases[nsites][1] = '\0'; // Store phase type
                        } else {
			  phases[nsites][1] = arc->phases[i]->Plabel; // Store phase type
			  phases[nsites][2] = '\0'; // Store phase type
                        }
			arrivalTimes[nsites] = arc->phases[i]->Pat - GSEC1970; // Store arrival time
			weights[nsites] = arc->phases[i]->Pqual; // Store weight
			residual[nsites] = arc->phases[i]->Pres;
		}
		else // If not a P phase, assume its an S phase. Non P or S phases are assumed S, we are looking for Sn or Sg
		{
			phases[nsites][0] = 'S'; // Store phase type
                        if (arc->phases[i]->Slabel == ' ' || arc->phases[i]->Slabel == 'S') {
			  phases[nsites][1] = '\0'; // Store phase type
                        } else {
			  phases[nsites][1] = arc->phases[i]->Slabel; // Store phase type
			  phases[nsites][2] = '\0'; // Store phase type
                        }
			arrivalTimes[nsites] = arc->phases[i]->Sat - GSEC1970; // Store arrival time
			weights[nsites] = arc->phases[i]->Squal; // Store weight
			residual[nsites] = arc->phases[i]->Sres;
		}
		
		
		// Store epicentral distance
		distances[nsites] = arc->phases[i]->dist;
		
		
		// Store coda duration
		coda_lengths[nsites] = arc->phases[i]->codalen;
		
		
		/* Md from channel for highest quality */
		if( arc->phases[i]->codawt < 4 )
		{
			coda_mags[nsites] = arc->phases[i]->Md;
			coda_weights[nsites] = arc->phases[i]->codawt;
		}
		else
		{
			coda_mags[nsites] = 0.0;
		}

		// Increment nsites
		nsites++;
		
		
		/* Update endtime
		 * Takes the last time instant of the latest event, defined as
		 * the arrival time + coda length to set the overall endtime
		 *************************************************************/
		codaLen = arc->phases[i]->codalen;
		
		// Negative codas are made positive
		if (codaLen<0) codaLen = -codaLen;
		
		// For an unknown reason, sometimes the coda length is 599s
		// Hence it is necessary to limit the coda length to 144 seconds 
		// to avoid long traces
		codaLen = ( codaLen > 144 )?144:codaLen;
		if( Debug )
			logit( "o", "Coda length for %s.%s.%s.%s : %d\n", 
					arc->phases[i]->site,
					arc->phases[i]->comp,
					arc->phases[i]->net, 
					arc->phases[i]->loc,
					arc->phases[i]->codalen);
         
		// For the endtime definition, it is necessary to decide
		// which P or S phase will be used. In this case, the latest one.
		if( ( arc->phases[i]->Pat + codaLen ) > endtime )		//using P arrival or
			endtime = arc->phases[i]->Pat + codaLen;			//
		if( ( arc->phases[i]->Sat + codaLen ) > endtime )		//using S arrival
			endtime = arc->phases[i]->Sat + codaLen;
         

		/* Check if the number of sites exceeds the
		 * predefined maximum number of stations
		 ******************************************/
		if( nsites >= MAX_STATIONS ) 
		{
			logit( "et", "ewhtmlemail: More than %d stations in message\n",
					MAX_STATIONS );
			break;
		}
	} // End of 'for' cycle for processing the phases in the arc message

   
	/* Correct times for epoch 1970
	 ******************************/
	starttime -= GSEC1970;		// Correct for epoch
	starttime -= TimeMargin;	// Add time margin
	endtime -= GSEC1970;		// Correct for epoch
	endtime += TimeMargin;		// Add time margin

	/* Check maximum duration of the traces
	 **************************************/
	dur = endtime - starttime;
	if( dur > ( double ) DurationMax ) 
	{
        if( Debug ) logit( "ot", "Waveform Durations %d greater "
        		"than set by MaxDuration %d\n", 
				(int) (endtime - starttime), (int)DurationMax);
        endtime = starttime+DurationMax;
        dur = DurationMax;	/* this is used later for header of waveform table */
	}
	
	/* Change to UTC time, if required
	 *********************************/
	timefunc = localtime;
	if( UseUTC )
	{ 
		timefunc = gmtime;
		strcpy( time_type, "UTC" );
	}
	
	/* Log debug info
	 ****************/
	if( Debug )
	{
		logit("o", "Available channels:\n");
		for( i = 0; i < nsites; i++ )
			logit( "o", "%5s.%3s.%2s.%2s\n",
					sites[i]->name, sites[i]->comp,
					sites[i]->net, sites[i]->loc);
            
		logit( "o", "Time margin: %f\n", TimeMargin );
      
		ot = ( time_t ) starttime;
		timeinfo = timefunc ( &ot );
		strftime( timestr, 80, "%Y.%m.%d %H:%M:%S", timeinfo );
		logit( "o", "Waveform starttime: %s %s\n", timestr, time_type );
      
		ot = ( time_t ) endtime;
		timeinfo = timefunc ( &ot );
		strftime( timestr, 80, "%Y.%m.%d %H:%M:%S", timeinfo );
		logit( "o", "Waveform endtime:   %s %s\n", timestr, time_type );
	}
   
   
	/* At this point, we have retrieved the information required for the html email
	 * traces, if there is any. Now, move forward to start producing the output files.
	 *********************************************************************************/


	/* build a KML file
	 ******************/
	if( KMLdir[0] != 0 )
	{
		logit( "ot", "ewhtmlemail: writing KML file.\n" );
		kml_writer( KMLdir, &(arc->sum), KMLpreamble, kml_filename, sites, nsites );
	}
	
	
	/* Start html email file
	 ***********************/
	sprintf(fullFilename, "%s_%ld.html", HTMLFile, arc->sum.qid ); // Name of html file
	if( Debug ) logit( "ot", "Creating html file %s\n", fullFilename );

	// Open html file
	if( (htmlfile = fopen( fullFilename, "w" )) != NULL )
	{
		/* Save html header
		 ******************/
		if( Debug ) logit( "ot", "Writing html header\n", fullFilename );
		// Placing css in the html body instead of header, to bypass some email clients
		// blocking the style sheet
		fprintf( htmlfile, "<html>\n<header>\n<style type=\"text/css\">" );
         
		/* Copy content of css file to html file
		 ***************************************/   
		/*
		if( strlen( StyleFile ) > 0 ) // Check if there is a style file at all
		{  
			if( Debug ) logit( "ot", "Copying css file to html header: %s\n", StyleFile );
			if( cssfile = fopen( StyleFile, "r" ) )
			{
				while( !feof( cssfile ) ) 
				{
					ch = getc(cssfile);
					if( ferror( cssfile ) )
					{
						if( Debug ) logit( "e", "Read css error\n" );
						clearerr( cssfile );
						break;
					}
					else
					{
						if( !feof( cssfile ) ) putc( ch, htmlfile );
						if( ferror( htmlfile ) )
						{
							if( Debug ) logit( "e", "Write css error\n" );
							clearerr( htmlfile );
							break;
						}
					}
				}
				fclose(cssfile);
			}
			else
			{
				logit("et","Unable to open css file\n");
			}
		}
      	*/
      
		/* Close style header
		 ********************/
		fprintf( htmlfile, "\n</style>" );
		
		/* CLose HTML Header */
		fprintf( htmlfile, "</header>\n" );
		
		/* Open HTML body */
		fprintf( htmlfile, "<body>\n" );
		
		
      
		Quality = ComputeAverageQuality( arc->sum.rms, arc->sum.erh, arc->sum.erz, 
				arc->sum.z, (float) (1.0*arc->sum.dmin), arc->sum.nph, arc->sum.gap );
      
		/* Create table with reference information
		 *****************************************/
		if (ShortHeader) {
                	InsertShortHeaderTable(htmlfile, arc, mag_l, mag_w, Quality);
		} else {
                	InsertHeaderTable(htmlfile, arc, mag_l, mag_w, Quality);
		}
         
		/* Produce google map with the stations and hypocenter
		 *****************************************************/
		 i=-1;
		 if(GMTmap)
		 {
			 if( Debug ) logit( "ot", "Computing GMT map\n" );
			 if((i=gmtmap(chartreq,sites,nsites,arc->sum.lat,arc->sum.lon,arc->sum.qid)) == 0)
			 	fprintf(htmlfile, "%s\n<br><br>", chartreq);
		}
		
		if(i==-1){
			if( Debug ) logit( "ot", "Computing Google map\n" );
			GoogleMapRequest( chartreq, sites, nsites, arc->sum.lat, arc->sum.lon );
			fprintf(htmlfile, "%s\n<br><br>", chartreq);
		}
      
		/* Reserve memory buffer for raw tracebuf messages
         *************************************************/
		buffer = ( char* ) malloc( MAX_SAMPLES * 4 * sizeof( char ) );
		if( buffer == NULL )
		{
			logit( "et", "ewhtmlemail: Cannot allocate buffer for trace\n" );
			// return -1; - Event if there is no memory for traces, still try to send email
		}
		else
		{
		
		
			/* Produce station traces
			 ************************/
			if( Debug ) logit("ot", "Computing Traces\n" );
			
			// Save header of traces table
			st = ( time_t ) starttime;
			timeinfo = timefunc( &st );
			strftime( timestr, 80, "%Y.%m.%d %H:%M:%S", timeinfo ); 
			fprintf( htmlfile, "<table id=\"WaveTable\" width=\"%d;\">\n",
					TraceWidth + 8 ); 
			fprintf( htmlfile, "<tr bgcolor=\"000060\"><th><font size=\"1\" face=\"Sans-serif\" color=\"FFFFFF\">Waveforms: (StartTime: %s Duration: %d"
					" seconds)</font></th></tr>\n", timestr, (int) dur );
			
			
			/* Cycle for each station
			 *  - Try to retrieve waveform data from the wave servers
			 *  - Decide to produce GIF or google chart
			 *  - If GIF, call function to produce GIF file and the request is
			 *          the filename within <img> tags - Create GIF header
			 *  - If google char, resample the data and then produce the google chart
			 *          encoded GET request
			 ************************************************************************/
			
			if( UseGIF && !UseBlat )
			{
				/* If using gifs with sendmail, we need to start writing a header file 
				 * with the base64 data included
				 *******************************************************/
				sprintf( gifHeaderFileName, "%s_GifHeader", HTMLFile );
				gifHeader = fopen( gifHeaderFileName, "w" );
			}
			for( i = 0; i < nsites; i++ )
			{
         
				/* Load data from waveservers
				 ****************************/
				bsamplecount = MAX_SAMPLES * 4; //Sets number of samples back
				if( Debug ) logit( "o", "Loading data from %s.%s.%s.%s\n",
						sites[i]->name, sites[i]->comp,
						sites[i]->net, sites[i]->loc );
				if( getWStrbf( buffer, &bsamplecount, sites[i], starttime, endtime ) == -1 )
				{
					logit( "t", "ewhtmlemail: Unable to retrieve data from waveserver"
							" for trace from %s.%s.%s.%s\n",
							sites[i]->name, sites[i]->comp,
							sites[i]->net, sites[i]->loc );
					continue;
				}
				
				
				/* Produce traces GIF or google chart
				 ************************************/
				if( UseGIF )
				{
				
					/* Produce GIF image
					 *******************/
					gif = gdImageCreate( TraceWidth, TraceHeight ); // Creates base image
				
					// Set colors for the trace and pick
					BACKCOLOR = gdImageColorAllocate( gif, 255, 255, 255 );	// White
					TRACECOLOR = gdImageColorAllocate( gif, 0, 0, 96 );		// Dark blue
					PICKCOLOR = gdImageColorAllocate( gif, 96, 0, 0 );		// Dark red
   		    
   		    
					/* Plot trace
					 ************/
					if( trbf2gif( buffer, bsamplecount, gif,
							starttime, endtime, TRACECOLOR, BACKCOLOR ) == 1 )
					{
						if( Debug ) logit( "o", "Created gif image for trace "
								"from %s.%s.%s.%s\n",
								sites[i]->name, sites[i]->comp,
								sites[i]->net, sites[i]->loc );
					}
					else
					{
						logit( "e", "ewhtmlemail: Unable to create gif image "
								"for %s.%s.%s.%s\n",
								sites[i]->name, sites[i]->comp,
								sites[i]->net, sites[i]->loc );
						continue;
					}

            
					/* Plot Pick
					 ***********/
					pick2gif( gif, arrivalTimes[i], phases[i], 
							starttime, endtime, PICKCOLOR );
					
					
   		 			
   		 			
   		 			/* Open gif file */
   		 			sprintf( giffilename, "%s_%ld_%s.%s.%s.%s_%s.gif", 
							HTMLFile, arc->sum.qid,
							sites[i]->name, sites[i]->comp,
							sites[i]->net, sites[i]->loc, phases[i] );
   		 			giffile = fopen( giffilename, "wb+" );
 					if (giffile == NULL) {
						logit( "e", "ewhtmlemail: Unable to create gif image "
								"for %s.%s.%s.%s as a file: %s\n",
								sites[i]->name, sites[i]->comp,
								sites[i]->net, sites[i]->loc, giffilename );
						continue;
					}
   		 			
   		 			/* Save gif to file */
   		 			gdImageGif( gif, giffile );
   		 			
   		 			if( !UseBlat )
   		 			{
   		 				/* The gif must be converted to base64 */
   		 			
   		 				/* Rewind stream */
   		 				rewind( giffile );
   		 			
   		 				/* Allocate memory for reading from file */
   		 				gifbuflen = ( size_t )( gif->sx * gif->sy );
   		 				gifbuffer = ( unsigned char* ) malloc( 
   		 						sizeof( unsigned char ) * gifbuflen );
   		 			
   		 				/* Read gif from file */
   		 				gifbuflen = fread( gifbuffer, sizeof( unsigned char ),
   		 						gifbuflen, giffile );
   		 			
   		 				/* Encode to base64 */
   		 				base64buf = base64_encode( (size_t *) &base64buflen,
   		 						gifbuffer, gifbuflen );
   		 			
   		 				/* Free gif buffer */
   		 				free( gifbuffer );
   		 			
   		 				/* Compute a content ID for this image */
   		 				sprintf( contentID, "%s_%s_%s_%s_%s",
								sites[i]->name, sites[i]->comp,
								sites[i]->net, sites[i]->loc, phases[i] );
						
   		 				/* Write base64 attachment to gif header file */
   		 				fprintf( gifHeader, "--FILEBOUNDARY\n"
   		 						"Content-Type: image/gif\n"
								"Content-Disposition: inline\n"
								"Content-Transfer-Encoding: base64\n"
								"Content-Id: <%s>\n\n%.*s\n",
								contentID, (int) base64buflen, base64buf );
						
						/* Free base64 buffer */
   		 				free( base64buf );
   		 			
   		 				/* Create img request */
   		 				sprintf( chartreq, 
   		 						"<img class=\"MapClass\" alt=\"\" src=\"cid:%s\">",
   		 						contentID );
   		 			}
   		 			else
   		 			{
   		 				//TODO: Investigate blat attachment references
   		 				/* Create img request */
   		 				sprintf( chartreq, 
   		 						"<img class=\"MapClass\" alt=\"\" src=\"cid:%s\">",
   		 						contentID );
   		 			}
   		 			/* Close gif file */
   		 			fclose( giffile );
   		 			
   		 			
   		 			
   		 			
   		 
					/* Save gif file
					 ***************/
					/*
					sprintf( giffilename, "%s_%ld_%s.%s.%s.%s.gif", 
					HTMLFile, arc->sum.qid,
							sites[i]->name, sites[i]->comp,
							sites[i]->net, sites[i]->loc );
					if( ( giffile = fopen( giffilename, "w" ) ) != NULL )
					{
						if( Debug ) logit( "o", "Saving gif file: %s\n", giffilename );
								gdImageGif( gif, giffile );
								fclose( giffile );
					}
					else
					{
						logit( "e", "ewhtmlemail: Unable to save gif file: %s\n", 
								giffilename );
						gdImageDestroy(gif);
						continue;
					}
					*/
   		 
					// free gif from memory
					gdImageDestroy(gif);
				
   		    
					/* Create request for gif file
					 *****************************/
//#ifdef _WINNT
					/* we need a windows equivalent (basename() is POSIX only). */
//					basefilename = giffilename;
//#else
//					basefilename = (char*)basename( giffilename );
//#endif
/* 					if (strlen(basefilename) > MAX_GET_CHAR)
                                        {
						logit( "et", "Warning, basefilename too large for <img> tag, only %d chars allowed and basefilename is %d chars\n" , MAX_GET_CHAR, strlen(basefilename));
                                        }
					//sprintf( chartreq, 
					//		"<img class=\"MapClass\" alt=\"\" src=\"%s\">", 
					//		basefilename );
					*/
				}
				else
				{
			
					/* Produce Google trace
					 **********************/


					/* Resample trace
					 ****************/
					if( Debug ) logit( "o", "Resampling samples for google chart\n" );
					if( trbufresample( gsamples, MAX_GET_SAMP, buffer, bsamplecount, 
							starttime, endtime, 30 ) == -1 ) // The value 30 comes from google encoding
					{
						logit( "e", "ewhtmlemail: Error resampling samples from "
								"%s.%s.%s.%s\n",
								sites[i]->name, sites[i]->comp,
								sites[i]->net, sites[i]->loc );
						continue;
					}
      	     
					/* Produce google charts request
					 *******************************/
					if( Debug ) logit( "o", "Creating google chart request\n" );
					if( makeGoogleChart( chartreq, gsamples, MAX_GET_SAMP, phases[i],
							((arrivalTimes[i]-starttime)/
							(endtime-starttime)*100),
							TraceWidth, TraceHeight ) == -1 )
					{
						logit( "e", "ewhtmlemail: Error generating google chart for "
								"%s.%s.%s.%s\n",
								sites[i]->name, sites[i]->comp,
								sites[i]->net, sites[i]->loc );
						continue;
					}
					if( Debug ) logit( "o", "Produced google chart trace for %s.%s.%s.%s\n",
							sites[i]->name, sites[i]->comp,
							sites[i]->net, sites[i]->loc );
				} // End of decision to make GIF or google chart traces
         
         
				/* Add to request to html file
				 *****************************/
				ot = ( time_t ) arrivalTimes[i];
				timeinfo = timefunc ( &ot );
				strftime( timestr,80,"%Y.%m.%d %H:%M:%S", timeinfo );
				fprintf(htmlfile, "<tr bgcolor=\"DDDDFF\" class=\"WaveTableTextRowClass\"><td><font size=\"1\" face=\"Sans-serif\">%s : "
						"%5s.%3s.%2s.%2s %s %s  CodaDur.=%d s "
						"PickQuality %d Residual=%6.2fs Distance=%5.1f km",
						phases[i], sites[i]->name, sites[i]->comp,
						sites[i]->net, sites[i]->loc, timestr,
						time_type, coda_lengths[i], weights[i], residual[i], distances[i]);
			
				// Include coda magnitudes, if available
				if( coda_mags[i] != 0.0 && DontShowMd == 0)
				{
					fprintf( htmlfile, " <b>Md=%3.1f wt=%d</b>", coda_mags[i], coda_weights[i] );
				}
			
				// Create the table row with the chart request
				// Included font-size mandatory style for composite images
				fprintf( htmlfile, "</font></td></tr>\n"
						"<tr class=\"WaveTableTraceRowClass\" style=\"font-size:0px;\"><td><font size=\"1\" face=\"Sans-serif\">%s</font></td></tr>\n", 
						chartreq);
         
			} // End of trace for cycle
			
			/* Last boundary and close gif header file, if that is the case */
			if( UseGIF && !UseBlat )
			{
				fprintf( gifHeader, "--FILEBOUNDARY--\n" );
				fclose( gifHeader );
			}
      
			// Free buffer
			free( buffer );
		
			// Finish trace table
			fprintf(htmlfile, "</table>\n");
		
		} // End of Traces section



		/* print out all the ML TYPE_MAGNITUDE info 
		 ******************************************/
		if ( UseML && mag_l != NULL )
		{
			fprintf( htmlfile, "<hr><a name=\"ML\"></a><table border=\"0\" cellspacing=\"0\" cellpadding=\"0\" id=\"DataTable\">\n" );
			fprintf( htmlfile, 
					"<tr bgcolor=\"#000060\">"
						"<td colspan=3>"
							"<a name=\"ML\"><font face=\"Sans-serif\" size=\"1\" color=\"FFFFFF\">Local Magnitude:</a> %4.1f "
							"&plusmn;%3.1f %s nchans=%d nstas=%d</font>"
						"</td>"
					"<tr>\n", 
					mag_l->mag, mag_l->error, MAG_MSG_TYPE_STRING,
					mag_l->nchannels, mag_l->nstations );
			fprintf( htmlfile, "<tr><th><font size=\"1\" face=\"Sans-serif\">S.C.N.L</font></th>"
					"<th><font size=\"1\" face=\"Sans-serif\">Magnitude</font></th>"
					"<th><font size=\"1\" face=\"Sans-serif\">Distance(km)</font></th></tr>\n" );
			for( i = 0; i < mag_l->nchannels; i++ ) 
			{
				MAG_CHAN_INFO *pMagchan;
				pMagchan = (MAG_CHAN_INFO *) mag_l->pMagAux + i;
				if( i % 2 )
				{
					fprintf( htmlfile, "<tr>" );
				} 
				else 
				{
					fprintf( htmlfile, "<tr bgcolor=\"#DDDDFF\" class=\"alt\">" );
				}
				fprintf( htmlfile, "<td><font size=\"1\" face=\"Sans-serif\">%s.%s.%s.%s</font></td>", 
						pMagchan->sta, pMagchan->comp, 
						pMagchan->net, pMagchan->loc);
				fprintf( htmlfile, "<td align=\"center\"><font size=\"1\" face=\"Sans-serif\">%3.1f</font></td>", pMagchan->mag );
				fprintf( htmlfile, "<td align=\"center\"><font size=\"1\" face=\"Sans-serif\">%5.1f</font></td>", pMagchan->dist );
				fprintf( htmlfile, "</tr>\n" );
			}
			fprintf(htmlfile, "</table>\n");
		} // End of ML 'for' cycle
		if ( UseMW && mag_w != NULL )
		{
			fprintf( htmlfile, "<hr><a name=\"MW\"></a><table border=\"0\" cellspacing=\"0\" cellpadding=\"0\" id=\"DataTable\">\n" );
			fprintf( htmlfile, 
					"<tr bgcolor=\"#000060\">"
						"<td colspan=3>"
							"<a name=\"MW\"><font face=\"Sans-serif\" size=\"1\" color=\"FFFFFF\">Moment Magnitude:</a> %4.1f "
							"&plusmn;%3.1f %s nchans=%d nstas=%d</font>"
						"</td>"
					"<tr>\n", 
					mag_w->mag, mag_w->error, MAG_MSG_MWTYPE_STRING,
					mag_w->nchannels, mag_w->nstations );
			fprintf( htmlfile, "<tr><th><font size=\"1\" face=\"Sans-serif\">S.C.N.L</font></th>"
					"<th><font size=\"1\" face=\"Sans-serif\">Magnitude</font></th>"
					"<th><font size=\"1\" face=\"Sans-serif\">Distance(km)</font></th></tr>\n" );
			for( i = 0; i < mag_w->nchannels; i++ ) 
			{
				MAG_CHAN_INFO *pMagchan;
				pMagchan = (MAG_CHAN_INFO *) mag_w->pMagAux + i;
				if( i % 2 )
				{
					fprintf( htmlfile, "<tr>" );
				} 
				else 
				{
					fprintf( htmlfile, "<tr bgcolor=\"#DDDDFF\" class=\"alt\">" );
				}
				fprintf( htmlfile, "<td><font size=\"1\" face=\"Sans-serif\">%s.%s.%s.%s</font></td>", 
						pMagchan->sta, pMagchan->comp, 
						pMagchan->net, pMagchan->loc);
				fprintf( htmlfile, "<td align=\"center\"><font size=\"1\" face=\"Sans-serif\">%3.1f</font></td>", pMagchan->mag );
				fprintf( htmlfile, "<td align=\"center\"><font size=\"1\" face=\"Sans-serif\">%5.1f</font></td>", pMagchan->dist );
				fprintf( htmlfile, "</tr>\n" );
			}
			fprintf(htmlfile, "</table>\n");
		} // End of MW 'for' cycle
      
      
		/* Footer for html file
		 ******************/
        fprintf(htmlfile, "<p id=\"Footer\"><font size=\"1\" face=\"Sans-serif\">This report brought to you by Earthworm with ewhtmlemail (version %s)</font></p>", VERSION_STR);
      
		/* Finish html file
		 ******************/
		if( Debug ) logit( "ot", "Closing file\n" );
		fprintf(htmlfile,"</body>\n</html>\n");
		fclose(htmlfile);
      
      
		/* Send email
		 *****************/

		/* note minquality check is inverted because A, B, C, D in chars A has lower value than B and so on */
		if( strlen( EmailProgram ) > 0 && nemailrecipients > 0 && Quality <= MinQuality )
		{
			double distance_from_center, azm;
			geo_to_km(center_lat, center_lon, arc->sum.lat, arc->sum.lon, &distance_from_center, &azm);
			logit( "ot", "ewhtmlemail: processing  email alerts.\n" );
			for( i=0; i<nemailrecipients; i++ )// One email for each recipient
			{
		 	    if (((emailrecipients[i].min_magnitude <= arc->sum.Mpref && DontUseMd == 0) || 
				 (UseML && mag_l != NULL && emailrecipients[i].min_magnitude <= mag_l->mag ) ||
				 (UseML && mag_l == NULL && emailrecipients[i].min_magnitude == DUMMY_MAG))  &&
				(distance_from_center <= emailrecipients[i].max_distance) )
			    {
				if( UseBlat )		// Use blat for sending email
				{
					if( kml_filename[0] == 0 )
					{
						sprintf(system_command,"%s %s -html -to %s -subject \"%s "
								"- EW Event ID: %ld\" %s",
								EmailProgram, fullFilename, emailrecipients[i].address,
								SubjectPrefix, arc->sum.qid, BlatOptions);
					}
					else
					{
						/* the latest version of blat supports -attacht option for text file attachment, send the KML file */
						sprintf(system_command,"%s %s -html -to %s -subject \"%s "
								"- EW Event ID: %ld\" -attacht %s %s",
								EmailProgram, fullFilename, emailrecipients[i].address, 
								SubjectPrefix, arc->sum.qid, kml_filename, BlatOptions);
					}
				}
				else				// Use sendmail
				{
					/* this syntax below is for UNIX mail, which on most systems does 
					 * not handle HTML content type, may use that in future...
					 ****************************************************************/
					/*sprintf(system_command,"cat %s | %s -s '%s - EW Event ID: %ld' %s",
							fullFilename, EmailProgram, SubjectPrefix,
							arc->sum.qid, emailrecipients[i].address);
					*/

					/* sendmail -t handles in line To: and other header info */
				
					/* Create email header file
					 **************************/
					sprintf( hdrFilename, "%s_header.tmp", HTMLFile );
					header_file = fopen( hdrFilename, "w" );
					fprintf( header_file, "To: %s\n", emailrecipients[i].address );
					fprintf( header_file, "Subject: %s - EW Event ID: %ld\n", 
								SubjectPrefix, arc->sum.qid );
					if( UseGIF )
					{
						/* When using GIFs the header must be different */
						fprintf( header_file, "MIME-Version: 1.0\n"
								"Content-Type: multipart/mixed; boundary=\"FILEBOUNDARY\"\n\n"
								"--FILEBOUNDARY\n"
								"Content-Type: text/html\n\n" );
						fclose( header_file );
					
					/* System command for sendmail with attachments */
					sprintf(system_command,"cat %s %s %s | %s -t ", 
							hdrFilename, fullFilename, gifHeaderFileName, EmailProgram);
					}
					else
					{
						fprintf( header_file, "Content-Type: text/html\n\n" );
						fclose( header_file );
					
						/* System command for sendmail without attachments */
						sprintf(system_command,"cat %s %s | %s -t ", 
								hdrFilename, fullFilename, EmailProgram);
					}
					
				}
			
				/* Execute system command to send email
				 **************************************/
				system(system_command);
				logit("et", "ewhtmlemail: email sent to %s, passed tests\n", emailrecipients[i].address);
				if (DebugEmail) {
					logit("et", "ewhtmlemail: Debug; EmailCommand issued '%s'\n", system_command);
				}
			    }
			    else
			    {
				/* this person doesn't get the email, Both Local (if used) or  Coda mag too low */
				if (emailrecipients[i].max_distance != OTHER_WORLD_DISTANCE_KM && 
					emailrecipients[i].max_distance > distance_from_center) 
				{ 
					logit("et", "ewhtmlemail: No email sent to %s because distance of event %5.1f lower than threshold %5.1f\n",
					emailrecipients[i].address, distance_from_center, emailrecipients[i].max_distance);
				}
				else if (UseML && mag_l != NULL && emailrecipients[i].min_magnitude > mag_l->mag )  
 				{
					logit("et", "ewhtmlemail: No email sent to %s because ML %5.2f and Md %5.2f are  lower than requested threshold %4.1f\n",
					emailrecipients[i].address, mag_l->mag, arc->sum.Mpref, emailrecipients[i].min_magnitude);
				}
				else if (DontUseMd == 0 && emailrecipients[i].min_magnitude > arc->sum.Mpref)
 				{
					logit("et", "ewhtmlemail: No email sent to %s because Md %5.2f lower than requested threshold %4.1f\n",
					emailrecipients[i].address, arc->sum.Mpref, emailrecipients[i].min_magnitude);
				}
			    }
			}
		}
		else
		{
			if (Quality>=MinQuality)
				logit("et", "ewhtmlemail: No emails sent, quality %c is below MinQuailty %c.\n",
					Quality, MinQuality);
		}
	}
	else
	{
		logit("et", "ewhtmlemail: Unable to write html file: %s\n", fullFilename);
		return FALSE;
	}
	logit("et", "ewhtmlemail: Completed processing of event id: %ld\n", arc->sum.qid);
	return TRUE;
}













/*******************************************************************************
 * GoogleMapRequest: Produce a google map request for a given set of stations  *
 *                   and a hypocenter                                          *
 ******************************************************************************/
void GoogleMapRequest(char *request, SITE **sites, int nsites,
   double hypLat, double hypLon)
{
   int i;
   char temp[MAX_GET_CHAR];

   
   /* Base of the google static map
    *******************************/
   snprintf(request, MAX_GET_CHAR, "<img class=\"MapClass\" alt=\"\" "
         "src=\"http://maps.google.com/maps/api/staticmap?"
         "size=600x400&format=png8&maptype=%s&sensor=false", StaticMapType);   
   
   /* Icon for hypocenter 
    *********************/
   if (hypLat!=0.0 || hypLon!=0.0)
   {
      snprintf( temp, MAX_GET_CHAR, "%s&markers=icon:http:%%2F%%2Fmaps.google.com%%2"
            "Fmapfiles%%2Fkml%%2Fpaddle%%2Fylw-stars.png|shadow:true|%f,%f", 
            request, hypLat, hypLon);
      snprintf( request, MAX_GET_CHAR, "%s", temp );
   }

   /* Add icons for stations
    ************************/
   snprintf( temp, MAX_GET_CHAR, "%s&markers=icon:http:%%2F%%2Fmaps.google.com%%2Fmapfiles"
         "%%2Fkml%%2Fshapes%%2Fplacemark_circle.png|shadow:false", request );
   snprintf( request, MAX_GET_CHAR, "%s", temp );
   for( i = 0; i < nsites; i++ )
   {
      snprintf( temp, MAX_GET_CHAR, "%s|%f,%f", request, sites[i]->lat, sites[i]->lon );
      snprintf( request, MAX_GET_CHAR, "%s", temp );
   }
   
   /* End of the request
    ********************/
   snprintf( temp, MAX_GET_CHAR, "%s\"/>", request );
   snprintf( request, MAX_GET_CHAR, "%s", temp );
   
}
















/*******************************************************************************
 * makeGoogleChartRequest: Produce a google chart request for a given station  *
 ******************************************************************************/
 /* 
 * Input Parameters
 * chartreq:        String to save the request in 
 * samples:         Array with the samples
 * samplecount:     Number of samples
 * phaseName:       A character to indicate the name of the phase (P or S)
 * phasePos:        The relative position of the phase label 0%-100%
 * tracewidth:      Width of the google trace
 * traceheight:     Height of the google trace
 */
int makeGoogleChart( char* chartreq, int *samples, int samplecount, 
		char * phasename, double phasepos, int tracewidth, int traceheight )
{
	int i;
	double avg;
	char reqHeader[MAX_GET_CHAR];
	char reqSamples[MAX_GET_CHAR];
	int cursample;				/* Position of the current sample */
	int cursampcount;			/* Number of samples in the current image */
	int curimgwidth;			/* Width of the current image */
	int accimgwidth;			/* Accumulated width of the composite image */
	int ntraces;				/* Number of traces that will be required */
	double cursamppos, nxtsamppos;

	
	// Average value of the samples
	avg = 0;
	for( i = 0; i < samplecount; i++ )
		avg += samples[i];
	avg /= ( double ) samplecount;

	
	/* Instead of sending a single image, the trace will be split
	 * in multiple images, each with its separate request and a 
	 * maximum of MAX_REQ_SAMPLES samples
	 ************************************************************/

	ntraces = ( ( int )( samplecount % 	MAX_REQ_SAMP ) == 0 )?
			( ( int )( samplecount / MAX_REQ_SAMP ) ) :
			( ( int )( samplecount / MAX_REQ_SAMP ) + 1 );
	if( Debug ) 
		logit( "o", "ewhtmlemail: Splitting trace in %d images\n", ntraces );

	cursample = 0;
	accimgwidth = 0;
	chartreq[0] = '\0';
	while( cursample < samplecount )
	{
		
		/* Determine how many samples will be used in this image
		 *******************************************************/
		cursampcount = ( ( cursample + MAX_REQ_SAMP ) > samplecount )?
				( samplecount - cursample ) : MAX_REQ_SAMP;

		
		/* Compute width of the image
		 ****************************/
		curimgwidth = ( int )( ( double )( cursample + cursampcount ) /
				( double ) samplecount * ( double ) tracewidth ) - accimgwidth;
		accimgwidth += curimgwidth;		// update accumulated image width


		/* Create img request with cursampcount samples
		 *************************************************/
		cursamppos = ( double ) cursample / ( double ) samplecount * 100.0;
		nxtsamppos = ( double )( cursample + cursampcount ) / 
				( double ) samplecount * 100.0;
		if( phasepos >= cursamppos && phasepos < nxtsamppos )
		{
			/* Get image with phase label
			 ****************************/
			snprintf( reqHeader, MAX_GET_CHAR,
					"<div style=\"float:left;overflow:hidden\">\n"
					"\t<img alt=\"\" style=\"margin:0px -2px\""
					"src=\"http://chart.apis.google.com/chart?"
					"chs=%dx%d&"
					"cht=ls&"
					"chxt=x&"
					"chxl=0:|%s&"
					"chxp=0,%3.1f&"
					"chxtc=0,-600&"
					"chco=000060&"
					"chma=0,0,0,0&"
					"chls=1&"				// Linewidth
					"chd=s:",
					curimgwidth + 4, traceheight,
					phasename, ( phasepos - cursamppos ) /
					( nxtsamppos - cursamppos ) * 100.0 );
		}
		else
		{
			/* Get image with no label
			 *************************/
			snprintf( reqHeader, MAX_GET_CHAR,
					"<div style=\"float:left;overflow:hidden\">\n"
					"\t<img alt=\"\" style=\"margin:0px -2px\""
					"src=\"http://chart.apis.google.com/chart?"
					"chs=%dx%d&"
					"cht=ls&"
					"chxt=x&"
					"chxl=0:|&"
					"chxp=0,&"
					"chxtc=0,-600&"
					"chco=000060&"
					"chma=0,0,0,0&"
					"chls=1&"				// Linewidth
					"chd=s:",
					curimgwidth + 4, traceheight );
		}

		/* Create samples
		 ****************/
		for( i = 0; i < MAX_GET_CHAR; i++ ) reqSamples[i] = 0;
		for( i = cursample; i < ( cursample + cursampcount ); i++ )
			reqSamples[i - cursample] = simpleEncode( ( int )( ( double ) samples[i] - avg + 30.5 ) );
		cursample += cursampcount;

		/* Add image and samples to final request
		 ****************************************/
		strncat( chartreq, reqHeader, MAX_GET_CHAR );
		strncat( chartreq, reqSamples, MAX_GET_CHAR );

		/* Terminate img tag
		 *******************/
		strncat( chartreq, "\"/>\n</div>\n", MAX_GET_CHAR );

	}
	
	/* Create request header
	 ***********************/
	/*
	snprintf(reqHeader, MAX_GET_CHAR, "<img class=\"TraceClass\" alt=\"\" "
			"src=\"http://chart.apis.google.com/chart?"
			"chs=%dx%d&"
			"cht=ls&"
			"chxt=x&"
			"chxl=0:|%c&"
			"chxp=0,%3.1f&"
			"chxtc=0,-600&"
			"chco=000060&"
			"chma=0,0,0,0&"
			"chls=1&"				// Linewidth
			"chd=s:",
			tracewidth, traceheight, phasename, phasepos);
	*/
	
	/* Add samples to request
	 ************************/
	/*
	for( i = 0; i < samplecount  && i < ( MAX_GET_CHAR - 1 ); i++ )
		reqSamples[i] = simpleEncode( ( int )( ( double ) samples[i] - avg + 30.5 ) );
	reqSamples[samplecount] = '\0';
	*/

	/* Combine header and samples
	 ****************************/
	/*
	snprintf( chartreq, MAX_GET_CHAR, "%s%s\"/>", reqHeader, reqSamples );
	*/
    
	return 1;
}













/*******************************************************************************
 * simpleEncode: This is a simple integer encoder based on googles function    *
 *******************************************************************************/
char simpleEncode(int i)
{
   char base[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
   
   /* truncate input
    ****************/
   if (i<0)
      i=0;
   if (i>61)
      i=61;
      
   
   return base[i];
}












/*******************************************************************************
 * searchSite: To replace site index, which does not seem to work well         *
 *******************************************************************************/
int searchSite(char *S, char *C, char *N, char *L)
{
   int i;
   
   for (i=0; i<nSite; i++)
   {
      if (
         strcmp(S,Site[i].name)==0 &&
         strcmp(C,Site[i].comp)==0 &&
         strcmp(N,Site[i].net)==0 &&
         strcmp(L,Site[i].loc)==0 )
         return i;
      //printf("Testing: %s.%s.%s.%s <-> %s.%s.%s.%s\n",
      //   S,C,N,L,
      //   Site[i].name,Site[i].comp,Site[i].net,Site[i].loc);
   }

   return -1;
}













/****************************************************************************************
 * getWStrbf: Retrieve a set of samples from the waveserver and store the raw tracebuf2 *
 *            in a buffer                                                               *
 ****************************************************************************************/
int getWStrbf( char *buffer, int *buflen, SITE *site,
		double starttime, double endtime )
{
	WS_MENU_QUEUE_REC 	menu_queue;
	TRACE_REQ 			trace_req;
	int 				wsResponse;
	int					atLeastOne;
	int					i;
	char				WSErrorMsg[80];
	
	/* Initialize menu queue
	 ***********************/
	menu_queue.head = NULL;
	menu_queue.tail = NULL;
	
	atLeastOne = 0;
	
	/* Make menu request
    *******************/
	for( i = 0; i < nwaveservers; i++ )
	{
		wsResponse = wsAppendMenu(
				waveservers[i].wsIP, waveservers[i].port, 
				&menu_queue, wstimeout );
		if( wsResponse != WS_ERR_NONE )
		{
			logit( "et", "ewhtmlemail: Cannot contact waveserver %s:%s - ",
					waveservers[i].wsIP, waveservers[i].port, wsResponse );
			logit( "et", "%s\n",
					getWSErrorStr(wsResponse, WSErrorMsg ) );
			continue;
		}
		else
		{
			atLeastOne++;
		}
	}
	if( atLeastOne == 0 )
	{
		logit( "et", "ewhtmlemail: Unable to contact any waveserver.\n");
		return -1;
	}
	
	
	/* Make request structure
	************************/
	strcpy( trace_req.sta, site->name );
	strcpy( trace_req.chan, site->comp );
	strcpy( trace_req.net, site->net );
	strcpy( trace_req.loc, site->loc );
	trace_req.reqStarttime = starttime;
	trace_req.reqEndtime = endtime;
	trace_req.partial = 1;
	trace_req.pBuf = buffer;
	trace_req.bufLen = *buflen;
	trace_req.timeout = wstimeout;
	trace_req.fill = 0;
   
   
	/* Pull data from ws
	 *******************/
	wsResponse = wsGetTraceBinL( &trace_req, &menu_queue, wstimeout );
	if( wsResponse != WS_ERR_NONE )
	{
		logit( "et", "ewhtmlemail: Error loading data from waveserver - %s\n",
				getWSErrorStr(wsResponse, WSErrorMsg));
    		wsKillMenu(&menu_queue);
		return -1;
	}
	
	/* Update output number of bytes
	 *********************************/
	*buflen = (int)trace_req.actLen;
	
	
	/* Terminate connection
     **********************/
    wsKillMenu(&menu_queue);
    
    return 1;
}		












char* getWSErrorStr(int errorCode, char* msg)
{
   switch (errorCode)
   {
      case 1:
         strcpy(msg,"Reply flagged by server");
         return msg;
      case -1:
         strcpy(msg,"Faulty or missing input");
         return msg;
      case -2:
         strcpy(msg,"Unexpected empty menu");
         return msg;
      case -3:
         strcpy(msg,"Server should have been in menu");
         return msg;
      case -4:
         strcpy(msg,"SCNL not found in menu");
         return msg;
      case -5:
         strcpy(msg,"Reply truncated at buffer limit");
         return msg;
      case -6:
         strcpy(msg,"Couldn't allocate memory");
         return msg;
      case -7:
         strcpy(msg,"Couldn't parse server's reply");
         return msg;
      case -10:
         strcpy(msg,"Socket transaction timed out");
         return msg;
      case -11:
         strcpy(msg,"An open connection was broken");
         return msg;
      case -12:
         strcpy(msg,"Problem setting up socket");
         return msg;
      case -13:
         strcpy(msg,"Could not make connection");
         return msg;
   }
   //Unknown error
   return NULL;
}






/****************************************************************************************
 * trbufresample: Resample a trace buffer to a desired sample rate. Uses squared low    *
 *            filtering and oversampling                                                *
 ****************************************************************************************/
int trbufresample( int *outarray, int noutsig, char *buffer, int buflen,
		double starttime, double endtime, int outamp )
{
	TRACE2_HEADER 	*trace_header,*trace;		// Pointer to the header of the current trace
	char			*trptr;				// Pointer to the current sample
	double			inrate;				// Initial sampling rate
	double			outrate;			// Output rate
	int				i;					// General purpose counter
	double			s;					// Sample
	double			*fbuffer;			// Filter buffer
	double			nfbuffer;			// length of the filter buffer
	double			*outsig;			// Temporary array for output signal
	double			outavg;				// Compute average of output signal
	double			outmax;				// Compute maximum of output signal
	double			*samples;			// Array of samples to process
	int				nsamples = 0;		// Number of samples to process
	int				samppos = 0;		// Position of the sample in the array;
	double			sr;						// buffer sample rate 
	int 			isBroadband;		// Broadband=TRUE - ShortPeriod = FALSE
	int 			flag;		
	double			y;				// temp int to hold the filtered sample	
	RECURSIVE_FILTER rfilter;     // recursive filter structure
	int 			maxfilters=20;

	
	/* Reserve memory for temporary output signal
	 ********************************************/
	outsig = ( double* ) malloc( sizeof( double ) * noutsig );
	if( outsig == NULL )
	{
		logit( "et", "ewhtmlemail: Unable to reserve memory for resampled signal\n" );
    	return -1;
	}
	
	
	/* Reserve memory for input samples
	 **********************************/
	// RSL note: tried the serialized option but this is better
	samples = ( double* ) malloc( sizeof( double ) * MAX_SAMPLES );
	if( samples == NULL )
	{
		logit( "et", "ewhtmlemail: Unable to allocate memory for sample buffer\n" );
    	return -1;
	}
	for( i = 0; i < MAX_SAMPLES; i++ ) samples[i] = 0.0;
	
	/* Isolate input samples in a single buffer
	 ******************************************/
	trace_header = (TRACE2_HEADER*)buffer;
	trptr = buffer;
	outavg = 0; // Used to initialize the filter
	if(SPfilter)
	{
		trace = (TRACE2_HEADER*)buffer;
		sr =trace->samprate; /*passing the sample rate from buffer header*/
		/*check if this channel is a broadband one or SP
		 * This simply check out the first two channel characters in order to conclude 
		 * whether it is a broadband (BH) or shorperiod (SP)*/
		switch(trace->chan[0])
              {
                  case 'B':
                      isBroadband = TRUE;
                      break;
                  case 'H':
                      isBroadband = TRUE;
                      break;
                  default:
                      isBroadband = FALSE;
                      break;
              }
	}
	while( ( trace_header = ( TRACE2_HEADER* ) trptr ) < (TRACE2_HEADER*)(buffer + buflen) )
	{
		// If necessary, swap bytes in tracebuf message
    	if ( WaveMsg2MakeLocal( trace_header ) < 0 )
    	{
			logit( "et", "ewhtmlemail(trbufresample): WaveMsg2MakeLocal error.\n" );
    		return -1;
		}
		
		// Update sample rate
		inrate = trace_header->samprate;
		
		// Skip the trace header
		trptr += sizeof( TRACE2_HEADER );
		
		for( i = 0; i < trace_header->nsamp; i++ )
		{
			// Produce integer sample
			if( strcmp( trace_header->datatype, "i2" ) == 0 ||
					strcmp(trace_header->datatype, "s2")==0 )
			{
				s = ( double ) (*((short*)(trptr)));
				trptr += 2;
			}
			else
			{
				s = ( double )( *( ( int* )( trptr ) ) );
				trptr += 4;
			}
			
			// Compute position of the sample in the sample array
			samppos = ( int )( ( trace_header->starttime 
					+ ( double ) i / trace_header->samprate - starttime ) *
					trace_header->samprate );
					
			if( samppos < 0 || samppos >= MAX_SAMPLES )
				continue;
					
			// Store sample in array
			samples[samppos] = s;
			if( samppos > nsamples ) nsamples = samppos;
			outavg += s;
		}
	}	 	
	
	if(SPfilter && isBroadband)
	{
		if( (flag=initAllFilters(maxfilters)) != EW_SUCCESS)
		{
			logit("et","ewhtmlemail: initAllFilters() cannot allocate Filters; exiting.\n");
		}
		if( (flag=initTransferFn()) != EW_SUCCESS)
		{
			logit("et","ewhtmlemail: initTransferFn() Could not allocate filterTF.\n");
		}
		if(flag==EW_SUCCESS)
		{
			switch (flag=initChannelFilter(sr, outavg/(double)nsamples,isBroadband,&rfilter, maxfilters))
			{
				 case  EW_SUCCESS:
                 
                        if (Debug)
                        logit("et","ewhtmlemail: Pre-filter ready for channel %s:%s:%s:%s\n",
                                trace->sta,trace->chan,trace->net,trace->loc);
						break;

                  case EW_WARNING:

                        logit("et","Unable to obtain a find an existing pre-filter or create a new (%s) pre-filter for sample rate %f; setting channel %s:%s:%s:%s bad\n",
                                (isBroadband ? "broadband" : "narrowband"),trace->samprate,
                                trace->sta,trace->chan,trace->net,trace->loc);
                        break;

                  case EW_FAILURE:
                  
                        logit("et",  "ewhtmlemail Parameter passed in NULL; setting channel %s:%s:%s:%s bad\n",
                               trace->sta,trace->chan,trace->net,trace->loc);
                        break;

                  default:

                        logit("et","Unknown error in obtaining a pre-filter for sample rate %f; setting channel %s:%s:%s:%s bad\n",
                               trace->sta,trace->chan,trace->net,trace->loc);
                        break;
			}
		}
		if( flag == EW_SUCCESS ) 
			{/*The channel filters function went ok*/
			
				outavg = 0;
				for(i=0;i<nsamples;i++){
					y=filterTimeSeriesSample(samples[i],&rfilter);
					samples[i]=y;
					outavg += (double)samples[i];
				}
				// Remove average from samples and normalize
				outavg /= (double)nsamples;
			/*reset the transfer function*/
			freeTransferFn();
			}
			else
			{
			/*Something was wrong and it is not possible to filter the samples*/
			outavg /= nsamples;//move this part			
			}	
		
	}
	else
	{/*it was not selected SPfilter*/
		outavg /= nsamples;
	}
	
	/* Prepare filter buffer
	 ***********************/
	outrate = ( double ) noutsig / ( endtime - starttime ); // Output sample rate
	nfbuffer = inrate / outrate; // Length of the filter buffer
	
	/* Check if filtering is required
	 ********************************/
	if( nfbuffer > 1.5 )
	{
		int nb = ( int )( nfbuffer + 0.5 );
		if( Debug ) logit( "o", "Filtering is required\nFilter settings:  "
				"Input rate: %f    Output rate: %f    Buffer length: %f\n", 
				inrate, outrate, nfbuffer );
		fbuffer = ( double* ) malloc( sizeof( double ) * nb );
		if( fbuffer == NULL )
		{
			logit( "et", "ewhtmlemail: Unable to allocate memory for filter buffer\n" );
    		return -1;
		}
		for( i = 0; i < nb; i++ )
			fbuffer[i] = outavg; // Initialize filter with average of signal
		
		
		/* Filter signal with array buffer
	 	 *********************************/
	 	for( i = 0; i < nsamples; i++ )
	 		samples[i] = fbuffer_add( fbuffer, nb, samples[i] );
	 	
	 	/* Done with filter buffer
	 	 *************************/
	 	free( fbuffer );
	}
	else
	{
		if( Debug ) logit( "o", "Filtering is not required.\n" );
	}
	
	
	
	
	
	/* Resampling
	 * This will be based on finding the sample to place
	 * at each point of the output array. Allows also upsampling
	 * if necessary
	 ***********************************************************/
	if( Debug ) logit( "o", "Resampling at a rate of %f\n",
			inrate / outrate );
	outavg = 0;
	for( i = 0; i < noutsig; i++ )
	{
		/* for the i-th sample of the output signal, calculate
		 * the position at the input array
		 *****************************************************/
		samppos = ( int )( ( double ) i * inrate / outrate + 0.5 );
		
		
		/* check if position is valid
		 ****************************/
		if( samppos >= nsamples )
			continue; // Invalid position
		
		
		/* store sample
		 **************/
		outsig[i] = samples[samppos];
		outavg += outsig[i]; // update average
	}
	outavg /= noutsig;

	
	/* Remove average of output signal
	 *********************************/
	outmax = 0;
	for( i = 0; i < noutsig; i++ )
	{
		outsig[i] = outsig[i] - outavg; // remove average

		// Compute maximum value
		if( outsig[i] > outmax ) outmax = outsig[i];
		if( outsig[i] < -outmax ) outmax = -outsig[i];
	}

	for(i = 0; i < noutsig; i++ )
	{
		outarray[i] = ( int ) ( outsig[i] / outmax * ( double ) outamp + 0.5 );
	}
	
	
	
	// free memory
	free( outsig );
	free( samples );
	return 1;
	
}

/* Add a new sample to the buffer filter and compute output */
double fbuffer_add( double *fbuffer, int nfbuffer, double s )
{
	int i;
	double out = 0;
	
	// circulate buffer
	for( i = 1; i < nfbuffer; i++ )
		fbuffer[i - 1] = fbuffer[i];
	
	// add sample to buffer
	fbuffer[nfbuffer - 1] = s;
	
	// compute filter output by averaging buffer
	for( i = 0; i < nfbuffer; i++ )
		out += fbuffer[i];
	out /= ( double ) nfbuffer;
	
	return out;
}



/****************************************************************************************
 * trb2gif: Takes a buffer with tracebuf2 messages and produces a gif image with the    *
 *          corresponding trace                                                         *
 ****************************************************************************************/
int trbf2gif( char *buf, int buflen, gdImagePtr gif, double starttime, double endtime,
		int fcolor, int bcolor )
{
	TRACE2_HEADER 	*trace_header,*trace;	// Pointer to the header of the current trace
	char			*trptr;			// Pointer to the current sample
	double			sampavg;		// To compute average value of the samples
	int			nsampavg=0;		// Number of samples considered for the average
	double			sampmax;		// To compute maximum value
	int 			i;				// General purpose counter
	gdPointPtr 		pol;			// Polygon pointer
	int				npol;			// Number of samples in the polygon
	double			xscale;			// X-Axis scaling factor
	double			dt;				// Time interval between two consecutive samples
        double 		sr;		        // to hold the sample rate obtained through the trace_header struc.                        
	int 			isBroadband;    // Broadband= TRUE - ShortPeriod = FALSE
	int 			flag;		
	int				y;				// temp int to hold the filtered sample	
	RECURSIVE_FILTER rfilter;     // recursive filter structure
	int				maxfilters=20;		//Number of filters fixed to 20.
	
	// Reserve memory for polygon points //TODO Make this dynamic
	pol = ( gdPointPtr ) malloc( MAX_POL_LEN * sizeof( gdPoint ) );
	if( pol == NULL )
	{
		logit( "et", "trbf2gif: Unable to reserve memory for plotting trace.\n" );
    	return -1;
	}
	npol = 0;
	
	// Initialize trace pointer
	trace_header = (TRACE2_HEADER*)buf;
	trptr = buf; 
	
	// Initialize maximum counter to minimum integer
	sampmax = 0;
	sampavg = 0;
	
	// Compute x-axis scale factor in pixels/second
	xscale = ( double ) gif->sx / ( endtime - starttime );
	
	// Cycle to process each trace an draw it on the gif image
	while( ( trace_header = ( TRACE2_HEADER* ) trptr ) < (TRACE2_HEADER*)(buf + buflen) )
	{
		// Compute time between consecutive samples
		dt = ( trace_header->endtime - trace_header->starttime ) /
				( double ) trace_header->nsamp;
	
		// If necessary, swap bytes in tracebuf message
    	if ( WaveMsg2MakeLocal( trace_header ) < 0 )
    	{
			logit( "et", "trbf2gif(trb2gif): WaveMsg2MakeLocal error.\n" );
			free( pol );
    	    return -1;
		}
		
		// Store samples in polygon
		// This procedure saves all the samples of this trace on the polygon
		// It also stores the maximum value and average values of the samples
		// for correcting the polygon later
		
		// Skip the trace header
		trptr += sizeof( TRACE2_HEADER );
		if(SPfilter)
		{
		trace = (TRACE2_HEADER*)buf;
		sr = trace->samprate; /*passing the sample rate from buffer header*/
		/*check if this channel is a broadband one or SP
		 * This simply check out the first two channel characters in order to conclude 
		 * whether it is a broadband (BH) or shorperiod (SP)*/
		switch(trace->chan[0])
              {
                  case 'B':
                      isBroadband = TRUE;
                      break;
                  case 'H':
                      isBroadband = TRUE;
                      break;
                  default:
                      isBroadband = FALSE;
                      break;
              }
		}
		// Process samples
		if( strcmp( trace_header->datatype, "i2" ) == 0 ||
				strcmp(trace_header->datatype, "s2")==0 )
	    {   
	    	// Short samples
			for( i = 0; i < trace_header->nsamp; i++, npol++, trptr += 2 )
			{
				// Compute x coordinate
				pol[npol].x = ( int )( ( trace_header->starttime - starttime +
        				( double ) i * dt ) * xscale );
        		
        		// Compute raw y coordinate without scaling or de-averaging
				pol[npol].y = ( int ) (*((short*)(trptr)));
				
				// Add sample to average counter
				sampavg += ( double ) pol[npol].y;
				nsampavg++; // Increments number of samples to consider for average
				
				// Consider sample for maximum (absolute) value
				if( pol[npol].y > sampmax || pol[npol].y < -sampmax ) sampmax = pol[npol].y;
			}
		}
		else if( strcmp( trace_header->datatype, "i4" ) == 0 ||
				strcmp(trace_header->datatype, "s4")==0 )
		{
        	// Integer samples
			for( i = 0; i < trace_header->nsamp; i++, npol++, trptr += 4 )
			{
				// Compute x coordinate
				pol[npol].x = ( int )( ( trace_header->starttime - starttime +
        				( double ) i * dt ) * xscale );
        		
        		// Compute raw y coordinate without scaling or de-averaging
				pol[npol].y = *((int*)(trptr));
				
				// Add sample to average counter
				sampavg += ( double ) pol[npol].y;
				nsampavg++; // Increments number of samples to consider for average
				
				// Consider sample for maximum (non-absolute) value
				if( pol[npol].y > sampmax || pol[npol].y < -sampmax ) sampmax = pol[npol].y;
				
				//printf("%d ", pol[npol].y);
				//printf("samp: %d  avg: %f  max: %f\n",pol[npol].y,sampavg,sampmax);
			}
			//printf("\n");
		}
		else
		{
			// Unknown type of samples
			logit( "et", "trbf2gif: Unknown type of samples\n" );
			free( pol );
    	    return -1;
		}
		// At this point, the polygon is populated with samples from this trace
		// The sampmax, sampavg and nsampavg variables are also updated but have not
		// yet been applied to correct the polygon
	} // End of trace cycle
	
	/*Filtering part*/
	if(SPfilter && isBroadband)
	{
		/*Starting the FILTER structures handle by the ioc_filter lib*/
		if((flag=initAllFilters(maxfilters)) != EW_SUCCESS)
		{
			logit("et","ewhtmlemail: initAllFilters() cannot allocate Filters; exiting.\n");
		}
		if( (flag=initTransferFn()) != EW_SUCCESS)
		{
			logit("et","ewhtmlemail: initTransferFn() Could not allocate filterTF.\n");
		}
		if(flag==EW_SUCCESS)
		{
			switch (flag=initChannelFilter(sr, sampavg/(double)nsampavg, isBroadband, &rfilter, maxfilters))
			{
				  case  EW_SUCCESS:
                 
                        if (Debug)
                        logit("et","ewhtmlemail: Pre-filter ready for channel %s:%s:%s:%s\n",
                                trace->sta,trace->chan,trace->net,trace->loc);
						break;

                  case EW_WARNING:

                        printf("Unable to obtain a find an existing pre-filter or create a new (%s) pre-filter for sample rate %f; setting channel %s:%s:%s:%s bad\n",
                                (isBroadband ? "broadband" : "narrowband"),trace->samprate,
                                trace->sta,trace->chan,trace->net,trace->loc);
                        break;

                  case EW_FAILURE:
                  
                        logit("et",  "ewhtmlemail Parameter passed in NULL; setting channel %s:%s:%s:%s bad\n",
                               trace->sta,trace->chan,trace->net,trace->loc);
                        break;

                  default:

                        printf("Unknown error in obtaining a pre-filter for sample rate %f; setting channel %s:%s:%s:%s bad\n",
                               trace->samprate,trace->sta,trace->chan,trace->net,trace->loc);
                        break;
			}
		}
		if( flag == EW_SUCCESS ) 
			{/*The channel filters function went ok*/
				sampavg = 0;
				sampmax = 0;
				for(i=0;i<npol;i++){
					y=(int)(filterTimeSeriesSample((double)pol[i].y,&rfilter));
					pol[i].y=y;
					sampavg+=(double)pol[i].y;
					if( pol[i].y > sampmax || pol[i].y < -sampmax ) sampmax = pol[i].y;
				}
				// Remove average from samples and normalize
				sampavg /=(double)nsampavg;
				sampmax -= sampavg;
			/*freeing the transfer function*/
			freeTransferFn();
			}
			else
			{
			/*Something was wrong and it is not possible to filter the samples*/
			// Remove average from samples and normalize
			sampavg /= ( double ) nsampavg; // Compute final sample average
			sampmax -= sampavg; // Correct sample maximum with average;
			}
		}/*end of filter part*/
	else
	{/*it was not selected SPfilter*/
		sampavg /= ( double ) nsampavg; // Compute final sample average
		sampmax -= sampavg; // Correct sample maximum with average;
	}
	//printf("Buflen: %d   Avg: %f   Max: %f\n",buflen, sampavg,sampmax);
	
	/* Correct Average */
	sampavg = 0;
	nsampavg = 0;
	for( i = 0; i < npol; i++ )
	{
		sampavg += ( double ) pol[i].y;
		nsampavg++;
	}
	sampavg /= nsampavg;
	
	sampmax = 0;
	for( i = 0; i < npol; i++ ) {
		//pol[i].y = ( int ) ( gif->sy / 2 ) -
		//		( int )( ( ( double ) pol[i].y - sampavg ) / sampmax * ( double ) gif->sy / 2 );
		
		/* Correct Average */
		pol[i].y = ( int )( ( ( double ) pol[i].y - sampavg ) );
		
		/* Compute new maximum after removing average */
		if( ( double ) pol[i].y > sampmax )
		{
			sampmax = ( double ) pol[i].y;
		}
		else if( ( double ) ( -pol[i].y ) > sampmax )
		{
			sampmax = -( double ) pol[i].y;
		}
	}
	
	/* Scale image and shift to vertical middle */
	for( i = 0; i < npol; i++ )
	{
		pol[i].y =  ( int ) ( gif->sy / 2 ) - 
				( int ) ( ( double ) pol[i].y / sampmax * ( double ) gif->sy / 2 );
	}
				
	// Clear image
	gdImageFilledRectangle( gif, 0, 0, gif->sx, gif->sy, bcolor );
				
	// Draw poly line
	gdImagePolyLine( gif, pol, npol, fcolor );
	
	
	// Free polygon memory
	free( pol );
	return 1;
}





/****************************************************************************************
 * pick2gif: Plots a pick as a vertical line in the image. Includes the phase            *
 ****************************************************************************************/
int pick2gif(gdImagePtr gif, double picktime, char *pickphase, 
		double starttime, double endtime, int fcolor)
{
	// Draw vertical line
	int pickpos = ( int ) ( ( double ) gif->sx / ( endtime - starttime ) *
			( picktime - starttime ) + 0.5 );
	gdImageLine(gif, pickpos, 0, pickpos, gif->sy, fcolor);
	
	// Draw phase character
	gdImageChar(gif, gdFontSmall, pickpos - 8, 1, pickphase[0], fcolor);
	
	return 1;
}







/****************************************************************************************
 * Utility Functions                                                                    *
 ****************************************************************************************/

// Draw a poly line
void gdImagePolyLine( gdImagePtr im, gdPointPtr pol, int n, int c )
{
	int i;
	for( i = 0; i < ( n - 1 ); i++ )
		gdImageLine( im, pol[i].x, pol[i].y, pol[i + 1].x, pol[i + 1].y, c);
}
/***************************************************************************************
 * gmtmap is a function to create a map with GMT, the format of the image
 * is png. gmtmap returns the string with the complementary html code where is added 
 * the map. It is based in system calls.
 * Returns 0 if all is all right and -1 if a system call has failed
 * The map projection could be Mercator or Albers.
 * **************************************************************************************/
int gmtmap(char *request,SITE **sites, int nsites,
   double hypLat, double hypLon,int idevt)
{
	char command[MAX_GET_CHAR]; // Variable that contains the command lines to be executed through system() function. 
	int i,j;					
	/*apply some settings with gmtset for size font and others*/
	if(Debug) logit("ot","gmtmap(): gmtsets applying default fonts for event %d\n",idevt);
	if((j=system("gmtset ANNOT_FONT_SIZE_PRIMARY 8")) != 0)
	{
		logit("et","gmtmap(): Unable to set ANNOT_FONT_SIZE_PRIMARY for event %d\n",idevt);
		logit("et","gmtmap(): Please verify whether GMT is installed or the GMT bin PATH has been exported\n");
		return -1;
	}
	if((j=system("gmtset FRAME_WIDTH 0.2c")) != 0)
	{
		logit("et","gmtmap(): Unable to set FRAME_WIDTH for event %d\n",idevt);
		logit("et","gmtmap(): Please verify whether GMT is installed or the GMT bin PATH has been exported\n");
		return -1;
	}
	if((j=system("gmtset TICK_LENGTH 0.2c")) != 0)
	{	
		logit("et","gmtmap(): Unable to set TICK_LENGTH for event %d\n",idevt);
		logit("et","gmtmap(): Please verify whether GMT is installed or the GMT bin PATH has been exported\n");
		return -1;
	}
	if((j=system("gmtset TICK_PEN 0.5p")) != 0)
	{	
		logit("et","gmtmap(): Unable to set TICK_PEN for event %d\n",idevt);
		logit("et","gmtmap(): Please verify whether GMT is installed or the GMT bin PATH has been exported\n");
		return -1;
	 }
	/*Mercator Projection*/
	if( Mercator )
	{
		if(Debug)logit("ot","gmtmap(): Mercator Projection used for event %d\n",idevt);
		/*creating the map with the hypocentral information. The epicenter will be the center of this 
		* map and upper and  lower limits, for lat and lon, will be 4 degrees. Projection is Mercator*/
		if(Debug) logit("et","gmtmap(): creating the basemap for event %d\n",idevt);
		sprintf(command,"pscoast -R%f/%f/%f/%f -Jm%f/%f/0.5i -Df -G34/139/34 -B2 -P -N1/3/255 -N2/1/255/to"
		" -Slightblue -K > %s_%d.ps\n",hypLon-4.0,hypLon+4.0,hypLat-4.0,hypLat+4.0,hypLon,hypLat,HTMLFile,idevt);
		if( (j=system(command)) != 0)
		{	
			logit("et","gmtmap(): Unable to create the basemap for event %d\n",idevt);
			logit("et","gmtmap(): Please verify whether GMT is installed or the GMT bin PATH has been exported\n");
			return -1;
		}
	}
	/*Albers Projection*/
	if( Albers )
	{
		if(Debug)logit("ot","gmtmap(): Albers Projection used for event %d\n",idevt);
		/*creating the map with the hypocentral information. The epicenter will be the center of this 
		* map and upper and  lower limits, for lat and lon, will be 4 degrees. Albers Projection */
		if(Debug) logit("et","gmtmap(): creating the basemap for event %d\n",idevt);
		sprintf(command,"pscoast -R%f/%f/%f/%f -Jb%f/%f/%f/%f/0.5i -Df -G34/139/34 -B2 -P -N1/3/255 -N2/1/255/to"
		" -Slightblue -K > %s_%d.ps\n",hypLon-4.0,hypLon+4.0,hypLat-4.0,hypLat+4.0,hypLon,hypLat,hypLat-2,hypLat+2,HTMLFile,idevt);
		if( (j=system(command)) != 0)
		{	
			logit("et","gmtmap(): Unable to create the basemap for event %d\n",idevt);
			logit("et","gmtmap(): Please verify whether GMT is installed or the GMT bin PATH has been exported\n");
			return -1;
		}
	}
	 /*adding names (if StationNames = TRUE) and simbols for stations*/
   if(Debug) logit("et","gmtmap(): adding station location symbol\n");
   for(i=0;i<nsites;i++)
   {
	sprintf(command,"echo %f %f |psxy -R -J -St0.2 -W1/0 -Glightbrown -O -K >> %s_%d.ps",sites[i]->lon,sites[i]->lat,HTMLFile,idevt);
		if((j=system(command)) != 0)
		{	
			logit("et","gmtmap(): Unable to add station symbols for station %s\n",sites[i]->name);
			return -1;
		}
		if(StationNames)
		{
			sprintf(command,"echo %f %f 4 0 1 CM %s |pstext -R -J -O -K -Glightbrown >>%s_%d.ps",sites[i]->lon,sites[i]->lat-0.15,sites[i]->name,HTMLFile,idevt);
			if((j=system(command)) != 0)
			{	
				logit("et","gmtmap(): Unable to add station name for %s\n",sites[i]->name);
				return -1;
			}
		}
	}
	/*Adding the name of the cities listed in the file Cities*/
   if( strlen(Cities) >= 1 )
   {
	sprintf(command,"cat %s | awk '{print $1,$2-0.1}' | psxy -R -J -Sc0.05  -G255 -O -K >> %s_%d.ps",Cities,HTMLFile,idevt);
	if(Debug) logit("et","gmtmap(): adding a circle to symbolize a city \n");
		if( (j=system(command)) != 0 )
		{	
			logit("et","gmtmap(): Unable to set the symbols for cities: event %d\n",idevt);
			return -1;
		}
		sprintf(command,"cat %s | pstext -R -J -O -G255  -S2  -K >> %s_%d.ps",Cities,HTMLFile,idevt);
		if(Debug) logit("et","gmtmap(): adding the names of cities\n");
		if((j=system(command)) != 0)
		{	
			logit("et","gmtmap(): Unable to set the cities' names: event %d\n",idevt);
			return -1;
		}
	}   
	/*Map legend for a map with Mercator Projection.
	 * 
	 * The difference between this and the next if clause is the legend's size an position into the map*/
	if( strlen(MapLegend) > 1 && Mercator)
	{
		if(Debug) logit("et","gmtmap(): setting the size font to legend\n");
		sprintf(command,"gmtset ANNOT_FONT_SIZE_PRIMARY 6");
		system(command);
		if(Debug) logit("et","gmtmap(): Adding legend to map\n");
		sprintf(command,"pslegend -Dx0.48i/0.53i/0.9i/0.5i/TC -J -R -O -F %s -Glightyellow -K >> %s_%d.ps",MapLegend,HTMLFile,idevt);
		system(command);
	}
	
	if( strlen(MapLegend) > 1 && Albers )
	{
		if(Debug) logit("et","gmtmap(): setting the size font to legend\n");
		sprintf(command,"gmtset ANNOT_FONT_SIZE_PRIMARY 6.5");
		system(command);
		if(Debug) logit("et","gmtmap(): Adding legend to map\n");
		sprintf(command,"pslegend -Dx0.60i/0.60i/0.9i/0.55i/TC -J -R -O -F %s -Glightyellow -K >> %s_%d.ps",MapLegend,HTMLFile,idevt);
		system(command);
	}
	/*drawing the epicentral symbol. A red start is the symbol*/
	if(Debug) logit("et","gmtmap(): Adding symbol to the epicenter\n");
	sprintf(command,"echo %f %f |psxy -R -J -Sa0.3 -W1/255/0/0 -G255/0/0 -O >> %s_%d.ps",hypLon,hypLat,HTMLFile,idevt);
	
	if( (j=system(command)) != 0)
	{	
		logit("et","gmtmap(): Unable to set the epicentral symbol for event %d\n",idevt);
		return -1;
	}
	if(Debug) logit("et","gmtmap(): Converting ps to png for event %d\n",idevt);
	sprintf(command,"ps2raster %s_%d.ps -A -P -Tg",HTMLFile,idevt);
	if( (j=system(command)) != 0)
	{	
		logit("et","gmtmap(): Unable to convert from ps to png: event %d\n",idevt);
		return -1;
	}
	/*Resizing the png image to 500x500*/
	if(Debug) logit("et","gmtmap(): Changing the resolution of the image for event %d\n",idevt);
	sprintf(command,"convert -resize 500X500 %s_%d.png %s_%d.png",HTMLFile,idevt,HTMLFile,idevt);
	if( (j=system(command)) != 0)
	{	
		logit("et","gmtmap(): Unable to resize the output file: event %d\n",idevt);
	}

   /* Base of gmt map
    *******************************/
   snprintf(request, MAX_GET_CHAR, "<img class=\"MapClass\" alt=\"\" "
         "src=\"%s_%d.png\"/>",HTMLFile,idevt);   
	return 0;
}






/*******************************************************************************
 * Base64 encoder. Based on the suggestion presented on StackOverflow:         *
 * http://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c*
 ******************************************************************************/
static char	encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static int	mod_table[] = {0, 2, 1};


unsigned char* base64_encode( size_t* output_length,
		const unsigned char *data, size_t input_length )
{
	size_t i;
	int j;
#ifdef _WINNT
	typedef unsigned __int32 uint32_t;
#endif
	uint32_t 	octet_a, octet_b, octet_c, triple;
	unsigned char* encoded_data;
	
	*output_length = ( ( input_length - 1 ) / 3 ) * 4 + 4;
	
	encoded_data = ( unsigned char* ) malloc( 
			*output_length * sizeof( unsigned char ) );
	if( encoded_data == NULL ) return NULL;
	
	
	for( i = 0, j = 0; i < input_length; )
	{
		octet_a = (i < input_length) ? data[i++] : 0;
		octet_b = (i < input_length) ? data[i++] : 0;
		octet_c = (i < input_length) ? data[i++] : 0;

		triple = ( octet_a << 0x10 ) + ( octet_b << 0x08 ) + octet_c;

		encoded_data[j++] = encoding_table[( triple >> 3 * 6 ) & 0x3F];
		encoded_data[j++] = encoding_table[( triple >> 2 * 6 ) & 0x3F];
		encoded_data[j++] = encoding_table[( triple >> 1 * 6 ) & 0x3F];
		encoded_data[j++] = encoding_table[( triple >> 0 * 6 ) & 0x3F];
    }

	for ( j = 0; j < mod_table[input_length % 3]; j++ )
		encoded_data[ *output_length - 1 - j ] = '=';

	return encoded_data;
}

