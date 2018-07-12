/******************************************************************************
 *                                GMEWHTMLEMAIL                                 *
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
 * to a set of recipients via email. In this case, gmewhtmlemail uses           *
 * sendmail, which is common with several linux distributions.                *
 * The code is a combination of seisan_report to retrieve hyp2000arc          *
 * messages, and gmew to access and retrieve data from a set of waveservers.  *
 * The code includes custom functions to generate the requests to the google  *
 * APIs and embeded these on the output html file.                            *
 * Besides the typical .d configuration file, gmewhtmlemail requires a css file *
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
#define VERSION_STR "1.4.2 - 2016-06-08"

#define MAX_STRING_SIZE 1024    /* used for many static string place holders */

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
//#include <rw_mag.h>
#include <rw_strongmotionII.h>
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
#include <sqlite3.h>
#include <time_ew.h>

char neic_id[30];


/* Defines
 *********/
#define MAX_STATIONS 200
#define MAX_GET_SAMP 2400       /* Number of samples in a google chart */
#define MAX_REQ_SAMP 600        /* Maximum number of samples per google request */
#define MAX_GET_CHAR 5000       /* Maximum number of characters in a google GET request */
#define MAX_POL_LEN 120000      /* Maximum number of samples in a trace gif */
#define MAX_WAVE_SERVERS 10
#define MAX_ADDRESS 80
#define MAX_PORT 6
#define MAX_EMAIL_RECIPIENTS 20
#define MAX_EMAIL_CHAR 160
#define DEFAULT_SUBJECT_PREFIX "EWalert"
#define DEFAULT_GOOGLE_STATIC_MAPTYPE "hybrid"
#define DEFAULT_MAPQUEST_STATIC_MAPTYPE "hyb"
#define OTHER_WORLD_DISTANCE_KM 100000000.0
#define DUMMY_MAG 9.0
#define KM2MILES 0.621371
#define MAX_SM_PER_EVENT    40


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
    char sendOnEQ;
    char sendOnSM;
    double min_magnitude;
    double max_distance;
} EMAILREC;

typedef struct HYPOARCSM2 {
    HypoArc ha;
    SM_INFO sm_arr[MAX_SM_PER_EVENT];
    int     sm_count;
    time_t  deliveryTime;
    char    fromArc;
    char    sncl[20];
} HypoArcSM2;

typedef struct site_order {
    double  key;
    int     idx;
} SiteOrder;

typedef struct dam_info {
    int    region_id;
    int    area_id;
    double lat;
    double lon;
    double dist;
    char   name[200];
    char   station[10];
} DamInfo;

char *region_names[] = {"",
    "Great Plains GP", 
    "Lower Colorado LC", 
    "Mid-Pacific MP",
    "Pacific Northwest PN", 
    "Upper Colorado UC" };

char *area_names[] = {
    "","DK","EC","MT","NK","OT","WY","LCD","PX","SC",
    "Y","CC","KB","LB","NC","SCC","LC","SR","UC","A",
    "P","WC"};

    
/* Functions in this file
 ************************/
void config(char*);
void lookup(void);
void status( unsigned char, short, char* );
int process_message( int arc_index ); //SM_INFO *sm ); //, MAG_INFO *mag, MAG_INFO *mw);
void MapRequest(char*, SITE**, int, double, double, HypoArcSM2*, SiteOrder*, SiteOrder* );
void GoogleMapRequest(char*, SITE**, int, double, double, HypoArcSM2*, SiteOrder*, SiteOrder* );
void MapQuestMapRequest(char*, SITE**, int, double, double, HypoArcSM2*, SiteOrder*, SiteOrder* );
//void GoogleChartRequest(char *request, SITE **sites, int nsites, double *distances );
//void GoogleChartRequest(char*, int*, double*, int, int, char, int);
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
void InsertShortHeaderTable(FILE *htmlfile, HypoArc *arc, char Quality);
void InsertHeaderTable(FILE *htmlfile, HypoArc *arc, char Quality, int showDM, char *);
unsigned char* base64_encode( size_t* output_length, const unsigned char *data, size_t input_length );
double deg2rad(double deg);
double rad2deg(double rad);
double distance(double lat1, double lon1, double lat2, double lon2, char unit);
void read_disclaimer( char* path );

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
static long       EmailDelay = 30;        // Time after reciept of trigger to send email
static int        UseBlat = 0;        // use blat syntaxi as the mail program instead of a UNIX like style
static int        UseRegionName = 0;        // put Region Name from FlynnEnghdal region into table at top, off by default
static char       BlatOptions[MAX_STRING_SIZE];      // apply any options needed after the file....-server -p profile etc...
static char       StaticMapType[MAX_STRING_SIZE];      // optional, specification of google map type
static int        UseUTC = 0;         // use UTC in the timestamps of the html page
static int        DontShowMd = 0;         // set to 1 to not show Md value
static int        DontUseMd = 0;         // set to 1 to not use Md value in MinMag decisions
static int        ShowDetail = 0;     // show more detail on event
static int        ShortHeader = 0;     // show a more compact header table
static char       MinQuality = 'D'; // by default send any email with quality equal to or greater than this
static char       DataCenter[MAX_STRING_SIZE];  // Datacenter name to be presented in the resume table.(optional)  
static int        SPfilter = 0;                       // Shortperiod filtering (optional)
static char       Cities[MAX_STRING_SIZE];          //cities filepath to be printed with gmt in the output map
static int        GMTmap=0;                     //Enable GMT map generation (Optional)
static int        StationNames=0;                 // Station names in the GMT map (Optional if GMTmap is enable)  (Optional)  
static char       MapLegend[MAX_STRING_SIZE];       // Map which contains the map legend to be drawn in the GMT map (optional)
static int        Mercator=1;                            // Mercator projection. This is the default map projection.                 
static int        Albers=0;                          // Albers projection. 
static int        MaxStationDist = 100;       // Maximum distance (in km) from origin for stations to appear in email
static int        MaxFacilityDist = 100;       // Maximum distance (in km) from origin for facilities to appear in email
static char       db_path[MAX_STRING_SIZE] = {0};                      // absolute path to sqlite DB file
static char       dbAvailable;              // 0 if quake ID trabnsaltion DB not available
static double     center_lat = 0.0;  // for distance check from center point
static double     center_lon = 0.0;  // for distance check from center point
static int        ShowMiles = 0;       // display distainces in miles as well as km
static int        MaxArcAge = 0;       // ignore Arc message more than this many secs old (0 means take all)
static char*      DisclaimerText = NULL;
static int        PlotAllSCNLs = 0;     // Plot all SCNLs; default is false (plot max SCNL for each SNL)
static char       MapQuestKey[MAX_STRING_SIZE] = {0};
static char       MQStaticMapType[MAX_STRING_SIZE];      // optional, specification of MapQuest map type
static int        facility_font_size = 3;
static int        ShowRegionAndArea = 0;
static int        showMax = 0;
static double     IgnoreHours = 0;

/* RSL: 2012.10.10 Gif options */
static int        UseGIF = 0;            // Use GIF files instead of google charts
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
static unsigned char      TypeTraceBuf2 = 0;
static unsigned char      TypeSTRONGMOTIONII = 0;
static unsigned char      TypeTHRESH = 0;


/* Error messages used by gmewhtmlemail
 ************************************/
#define  ERR_MISSGAP       0   // sequence gap in transport ring         
#define  ERR_MISSLAP       1   // missed messages in transport ring      
#define  ERR_TOOBIG        2   // retreived msg too large for buffer     
#define  ERR_NOTRACK       3   // msg retreived; tracking limit exceeded 
static char  Text[150];        // string for log/error messages          

#define MAX_QUAKES  100 // should make this configurable at some point
#define MAX_MAG_CHANS 1000  // this SHOULD be enough
#define MAX_DAMS  600 // number of facilities allowed in csv file, should be made config later

HypoArcSM2       *arc_list[MAX_QUAKES];     // arc_list[0..arc_counter-1] point to elements of arc_data
HypoArcSM2       arc_data[MAX_QUAKES];
long             arc_counter = 0;

static time_t next_delivery = -1;           // When to report on arc_list[next_arc]
static int next_arc = -1;

DamInfo dam_tbl[MAX_DAMS];
int dam_count = 0, dam_close_count = 0;

void free_arcsm2( HypoArcSM2 *p ) {
    if ( p->sm_count > 0 ) {
        //_free( p->sm_arr, "sm list" );        
    }
    if ( arc_list[arc_counter-1] != p ) {
        *p = arc_data[arc_counter-1];
    }
    //_free( p, "sm2" );
}

int read_dam_info( char *path ) {
    FILE *fp = fopen( path, "r" );
    if ( fp == NULL )
        return -1;
    char buffer[200];
    char *line = fgets(buffer, 180, fp);
    line = fgets(buffer, 180, fp);
    line = fgets(buffer, 180, fp);
    line = fgets(buffer, 180, fp);
    int lineno = 0;
    while ( line != NULL && dam_count < MAX_DAMS ) {
        char *word, *brkt;
        word = strtok_r(line, ",", &brkt);
        dam_tbl[dam_count].lon = atof( word );
        word = strtok_r(NULL, ",", &brkt);
        dam_tbl[dam_count].lat = atof( word );
        word = strtok_r(NULL, ",", &brkt);
        //strcpy( dam_tbl[dam_count].code, word );
        word = strtok_r(NULL, ",", &brkt);
        strcpy( dam_tbl[dam_count].name, word );
        lineno++;
        dam_count++;
        line = fgets(buffer, 180, fp);
    }
    if ( line != NULL ) {
        logit( "", "Maximum of %d facilites exceeded\n", MAX_DAMS );
    }
    fclose( fp );
    return 0;
}

int read_alldam_info( char *path ) {
    FILE *fp = fopen( path, "r" );
    if ( fp == NULL )
        return -1;
    char buffer[300];
    char *line = fgets(buffer, 290, fp);
    int lineno = 0;
    while ( line != NULL && dam_count < MAX_DAMS ) {
        char *word, *brkt, c;
        strtok_r(line, ",", &brkt); // skip column A
        word = strtok_r(NULL, ",", &brkt);
        dam_tbl[dam_count].lon = atof( word );
        word = strtok_r(NULL, ",", &brkt);
        dam_tbl[dam_count].lat = atof( word );
        // skip column D
        if ( *brkt==',' )
            brkt++;
        else
            strtok_r(NULL, ",", &brkt);
        word = strtok_r(NULL, ",", &brkt);
        strcpy( dam_tbl[dam_count].name, word );
        // skip columns F & G
        for ( c='f'; c<'h'; c++ )
            if ( *brkt==',' )
                brkt++;
            else
                strtok_r(NULL, ",", &brkt);
        if ( *brkt==',' ) {
            brkt++;
            dam_tbl[dam_count].station[0] = 0;
        } else {
            word = strtok_r(NULL, ",", &brkt);
            strcpy( dam_tbl[dam_count].station, word );
        }
        // skip columns I thru P
        for ( c='i'; c<'q'; c++ )
            if ( *brkt==',' )
                brkt++;
            else
                strtok_r(NULL, ",", &brkt);
        word = strtok_r(NULL, ",", &brkt);
        dam_tbl[dam_count].region_id = atoi(word);
        word = strtok_r(NULL, ",", &brkt);
        dam_tbl[dam_count].area_id = atoi(word);
        
//         printf("%f %f %s %s '%s' %s\n", dam_tbl[dam_count].lat,  dam_tbl[dam_count].lon,
//              region_names[dam_tbl[dam_count].region_id],  
//              area_names[dam_tbl[dam_count].area_id],
//               dam_tbl[dam_count].station,  dam_tbl[dam_count].name);

        lineno++;
        dam_count++;
        line = fgets(buffer, 290, fp);
    }
    if ( line != NULL ) {
        logit( "", "Maximum of %d facilites exceeded\n", MAX_DAMS );
    }
    fclose( fp );
    return 0;
}

char* assignMailingList() {
    char* str = k_str();
    if ( k_err() ) {
        // There was no modifier; assign to both lists
        emailrecipients[nemailrecipients].sendOnEQ = 1;
        emailrecipients[nemailrecipients].sendOnSM = 1;
    } else if ( strcmp( str, "EQK" ) == 0 ) {
        emailrecipients[nemailrecipients].sendOnEQ = 1;
        emailrecipients[nemailrecipients].sendOnSM = 0;
    } else if ( strcmp( str, "SM" ) == 0 ) {
        emailrecipients[nemailrecipients].sendOnEQ = 0;
        emailrecipients[nemailrecipients].sendOnSM = 1;
    } else {
        return str;
    }
    return NULL;
}

static char* monthNames[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"}; 

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
    HypoArcSM2    *arc;             // a hypoinverse ARC message
    
    /* Check command line arguments
    ******************************/
    if (argc != 2) 
    {
        fprintf(stderr, "Usage: gmewhtmlemail <configfile>\n");
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
    logit("", "gmewhtmlemail: Read command file <%s>\n", argv[1]);

    logit("t", "gmewhtmlemail: version %s\n", VERSION_STR);

    /* Get our own process ID for restart purposes
    *********************************************/
    if( (MyPid = getpid()) == -1 )
    {
      logit ("e", "gmewhtmlemail: Call to getpid failed. Exiting.\n");
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
    if ( !( msgbuf = (char *) calloc( 1, (size_t)MaxMessageSize+10 ) ) )
    {
      logit( "et",
         "gmewhtmlemail: failed to allocate %d bytes"
         " for message buffer; exiting!\n", MaxMessageSize+10 );
      free( GetLogo );
      exit( -1 );
    }
     
    /* Initialize DB for quake ID translation
     *****************************************/
    if ( (db_path[0]==0) || (SQLITE_OK != (res = sqlite3_initialize())) )
    {
        if ( db_path[0] )
            logit("et","Failed to initialize sqlite3 library: %d\n", res);
        else
            logit("t","sqlite3 library not specified\n");
        dbAvailable = 0;
    } else {
        dbAvailable = 1;
    }

    /* Attach to shared memory rings
    *******************************/
    tport_attach( &InRegion, InRingKey );
    logit( "", "gmewhtmlemail: Attached to public memory region: %ld\n",
      InRingKey );
        

    /* Force a heartbeat to be issued in first pass thru main loop
    *************************************************************/
    timeLastBeat = time(&timeNow) - HeartbeatInt - 1;
    

    /* Flush the incoming transport ring on startup
    **********************************************/
    while( tport_copyfrom(&InRegion, GetLogo, nLogo,  &reclogo,
        &recsize, msgbuf, MaxMessageSize, &seq ) != GET_NONE )
        ; 


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

        if ( next_delivery != -1 && timeNow >= next_delivery ) {
            if ( Debug )
                logit("","Process %s %d\n", arc_list[next_arc]->fromArc ? "arc" : "trigger", arc_list[next_arc]->ha.sum.qid);
            process_message( next_arc );
            free_arcsm2( arc_list[next_arc] );
            if ( arc_counter == 1 ) {
                next_delivery = -1;
                next_arc = -1;
                arc_list[0] = NULL;
            } else {
                if ( next_arc != arc_counter-1 ) 
                    arc_list[next_arc] = arc_list[arc_counter-1];
                arc_list[arc_counter-1] = NULL;
                next_delivery = arc_list[0]->deliveryTime;
                next_arc = 0;
                for ( i=1; i<arc_counter-1; i++ )
                    if ( arc_list[i]->deliveryTime < next_delivery ) {
                        next_delivery = arc_list[i]->deliveryTime;
                        next_arc = i;
                    }
            }
            arc_counter--;
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
            logit("t", "gmewhtmlemail: Received message, reclogo.type=%d\n",
                                                       (int)(reclogo.type));
        }
        
        if ( reclogo.type == TypeHYP2000ARC ) 
        {
            time_t delivery_time;
            if ( arc_counter == MAX_QUAKES-1 ) {
                logit( "et", "Maximum number of events (%d) exceeded; ignoring latest\n", MAX_QUAKES );
                continue;
            }
            arc = arc_data+arc_counter;
            if ( Debug )
                logit("t","Got a ARC msg: %s\n", msgbuf);
            if ( arc_counter == MAX_QUAKES-1 ) {
                logit( "et", "Maximum number of events (%d) exceeded; ignoring latest\n", MAX_QUAKES );
            } else {
                parse_arc_no_shdw(msgbuf, &(arc->ha));
                time(&delivery_time);
                if ( (MaxArcAge > 0) && (delivery_time - (arc->ha.sum.ot-GSEC1970) > MaxArcAge) ) {
                    logit("t","Ignoring ARC %ld; too old\n", arc->ha.sum.qid );
                    continue;
                }
                delivery_time += EmailDelay;
                for ( i=0; i<arc_counter; i++ )
                    if ( arc_list[i]->ha.sum.qid == arc->ha.sum.qid ) {
                        if ( Debug )
                            logit("","----Received duplicate ARC for %ld before original processed\n", arc->ha.sum.qid );
                        arc_list[i]->deliveryTime = delivery_time;
                        arc_list[i]->ha = arc->ha;
                        break;
                    }
                if ( i==arc_counter )  {
                     arc->deliveryTime = delivery_time;
                     arc->sm_count = 0;
                     arc_list[arc_counter] = arc;
                     arc_counter++;
                     if ( Debug )
                         logit("","Added arc #%ld lat=%lf lon=%lf depth=%lf %d phases\n", 
                            arc->ha.sum.qid, arc->ha.sum.lat, arc->ha.sum.lon, arc->ha.sum.z, arc->ha.sum.nph);
                     if ( arc_counter == 1 ) {
                        next_delivery = delivery_time;
                        next_arc = 0;
                    }
                    arc->fromArc = 1;
                } else {
                    if ( arc_counter > 1 ) {
                        next_arc = 0;
                        next_delivery = arc_list[0]->deliveryTime;
                        for ( i=1; i<arc_counter; i++ )
                            if ( arc_list[i]->deliveryTime < next_delivery ) {
                                next_delivery = arc_list[i]->deliveryTime;
                                next_arc = i;
                            }
                    }
                    if ( Debug && next_delivery != delivery_time )
                        logit( "", "----New head of queue: %ld\n", arc_list[next_arc]->ha.sum.qid );
                }
            }
        } else if ( reclogo.type == TypeTHRESH ) 
        {
            if ( Debug )
                logit("t","Got a THRESH message: %s\n", msgbuf );
            if ( arc_counter == MAX_QUAKES-1 ) {
                logit( "et", "Maximum number of events (%d) exceeded; ignoring latest\n", MAX_QUAKES );
            } else {
                 arc = arc_data+arc_counter; //_calloc(1, sizeof(HypoArcSM2), "sm2 for thresh");
                 if ( arc == NULL ) {
                    logit( "et", "Failed to allocated a HypoArcSM2\n" );
                } else {
                    //parse_arc_no_shdw(msgbuf, &(arc->ha));
                     arc->ha.sum.qid = 0;
                    int args, year, pos;
                    struct tm when_tm;
                    //time_t ot;
                    char month_name[20];
                    char sncl[4][20];
        
                   args = sscanf( msgbuf, "SNCL=%s Thresh=%*f Value=%*f Time=%*s %s %d %d:%d:%d %d", arc->sncl, month_name, &when_tm.tm_mday, &when_tm.tm_hour, &when_tm.tm_min, &when_tm.tm_sec, &year );
                    int i = 1;
                    for ( i=0; i<12; i++ )
                        if ( strcmp( monthNames[i], month_name ) == 0 ) {
                            when_tm.tm_mon = i;
                            break;
                        }
                    when_tm.tm_year = year - 1900;

                     //arc->ha.sum.ot = timegm_ew( &when_tm );
                     arc->ha.sum.ot = timegm( &when_tm );
                     arc->ha.sum.qid = -arc->ha.sum.ot;
                     arc->ha.sum.ot += GSEC1970;
            
                     int j = 0, k = 0;
                     for ( i=0; arc->sncl[i]; i++ )
                        if ( arc->sncl[i] == '.'  ) {
                            sncl[j][k] = 0;
                            j++;
                            k = 0;
                        } else 
                            sncl[j][k++] = arc->sncl[i];
                     sncl[j][k] = 0;
                     pos = site_index( sncl[0], sncl[1], sncl[2], sncl[3][0] ? sncl[3] : "--") ;
                     if ( pos != -1 ) {
                        arc->ha.sum.lat = Site[pos].lat;
                        arc->ha.sum.lon = Site[pos].lon;
                     } else {
                        arc->ha.sum.lat = arc->ha.sum.lon = -1;
                    }
                     arc->sm_count = 0;
                     time(&(arc->deliveryTime));
                     arc->deliveryTime += +EmailDelay;
                     arc_list[arc_counter] = arc;
                     arc_counter++;
                     if ( Debug )
                         logit("","Added thresh #%ld %s (%lf)\n", arc->ha.sum.qid, arc->sncl, arc->ha.sum.ot);
                     if ( arc_counter == 1 ) {
                        next_delivery = arc->deliveryTime;
                        next_arc = 0;
                    }
                    arc->fromArc = 0;
                }
            }
        } else if ( reclogo.type == TypeSTRONGMOTIONII ) 
        {
            char *ptr = msgbuf, gotOne = 0;
         
            while ( 1 ) {
                SM_INFO sm;
                long n_qid;
                char sncl[20];
                int rv = rd_strongmotionII( &ptr, &sm, 1 );
                if ( rv != 1 ) {
                    if ( !gotOne )
                        logit( "et", "Could not parse SM message; ignoring\n" );
                    break;
                }
                gotOne = 1;
                n_qid = atol( sm.qid );
                sprintf( sncl, "%s.%s.%s.%s", sm.sta, sm.net, sm.comp, sm.loc );
                for ( i=0; i<arc_counter; i++ )
                    if ( arc_list[i]->ha.sum.qid == n_qid )
                        break;
                if ( (i < arc_counter) && !arc_list[i]->fromArc ) {
                    if ( strcmp( sncl, arc_list[i]->sncl ) ) {
                        if ( Debug ) 
                            logit( "", "Ignoring SM from non-triggering station: %s\n", sncl);
                        continue;
                    }
                }
                if ( i < arc_counter ) {
                     int n = arc_list[i]->sm_count; 
                     if ( n>=MAX_SM_PER_EVENT ) {
                        logit( "et", "Failed to expand sm_arr\n" );
                     } else {
                         int j;
                         for ( j=0; j<n; j++ )
                            if ( !(strcmp( arc_list[i]->sm_arr[j].sta, sm.sta ) ||
                                    strcmp( arc_list[i]->sm_arr[j].net, sm.net ) ||
                                    strcmp( arc_list[i]->sm_arr[j].comp, sm.comp ) ||
                                    strcmp( arc_list[i]->sm_arr[j].loc, sm.loc )) ) 
                                break;
                         arc_list[i]->sm_arr[j] = sm;
                         if ( j==n ) {
                             arc_list[i]->sm_count++;
                             if (Debug)            
                                logit("t","Added sm2 %s to %s (#%d) pga=%lf pgv=%lf pgd=%lf %d rsa\n", 
                                    sncl, sm.qid, n+1, sm.pga, sm.pgv, sm.pgv, sm.nrsa);
                         }
                         else if ( Debug ) 
                                logit("t","Replaced sm2 %s for %s (#%d) pga=%lf pgv=%lf pgd=%lf %d rsa\n", 
                                    sncl, sm.qid, n+1, sm.pga, sm.pgv, sm.pgv, sm.nrsa);
                     }
                     //process_message( &sm );
                } else {
                    logit( "", "sm2 %d w/ no matching trigger:", n_qid );
                    for ( i=0; i<arc_counter; i++ )
                        logit( "", "%ld ", arc_list[i]->ha.sum.qid );
                    logit( "", "\n" );
                 }
            }
        } else {
            logit("","Unexpected message type: %d\n", reclogo.type );
        }

    }

    /* free allocated memory */
   free( GetLogo );
   free( msgbuf );
   free( Site );    // allocated by calling read_site()

    /* detach from shared memory */
   tport_detach( &InRegion );
    
    /* Shut down sqlite3, if in use */
   if ( dbAvailable )
        sqlite3_shutdown();

    /* write a termination msg to log file */
   logit( "t", "gmewhtmlemail: Termination requested; exiting!\n" );
   fflush( stdout );
    
   return( 0 );
}

/*****************************************************************************
 *  config() processes command file(s) using kom.c functions;                *
 *                    exits if any errors are encountered.                   *
 *****************************************************************************/
#define ncommand 10        /* # of required commands you expect to process */
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
   char path[200];
   int cfgNumLogo;
               
   BlatOptions[0]=0;
   KMLdir[0]=0;
   strcpy(SubjectPrefix, DEFAULT_SUBJECT_PREFIX);
   strcpy(StaticMapType, DEFAULT_GOOGLE_STATIC_MAPTYPE);
   strcpy(MQStaticMapType, DEFAULT_MAPQUEST_STATIC_MAPTYPE);

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
         "gmewhtmlemail: Error opening command file <%s>; exiting!\n",
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
                  "gmewhtmlemail: Error opening command file <%s>; exiting!\n",
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
                  "gmewhtmlemail: Invalid <LogFile> value %d; "
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
                      "gmewhtmlemail: Invalid module name <%s> "
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
                     "gmewhtmlemail: Invalid ring name <%s> "
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
               MSG_LOGO *tlogo = NULL;
               cfgNumLogo = nLogo + 10;
               tlogo = (MSG_LOGO *)
                             realloc(GetLogo, cfgNumLogo * sizeof (MSG_LOGO) );
               if (tlogo == NULL)
               {
                  logit("e", "gmewhtmlemail: GetLogo: error reallocing"
                     " %d bytes; exiting!\n",
                     cfgNumLogo * sizeof (MSG_LOGO));
                  exit(-1);
               }
               
               GetLogo = tlogo;

               if (GetInst(str, &GetLogo[nLogo].instid) != 0) 
               {
                  logit("e",
                     "gmewhtmlemail: Invalid installation name <%s>"
                     " in <GetLogo> cmd; exiting!\n", str);
                  exit(-1);
               }
               GetLogo[nLogo + 3].instid = GetLogo[nLogo + 2].instid = GetLogo[nLogo + 1].instid = GetLogo[nLogo].instid;
               if ((str = k_str()) != NULL) 
               {
                  if (GetModId(str, &GetLogo[nLogo].mod) != 0) 
                  {
                     logit("e",
                        "gmewhtmlemail: Invalid module name <%s>"
                        " in <GetLogo> cmd; exiting!\n", str);
                     exit(-1);
                  }
                  GetLogo[nLogo + 3].mod = GetLogo[nLogo + 2].mod = GetLogo[nLogo + 1].mod = GetLogo[nLogo].mod;
                  if (GetType("TYPE_HYP2000ARC", &GetLogo[nLogo++].type) != 0)
                  {
                     logit("e",
                        "gmewhtmlemail: Invalid message type <TYPE_HYP2000ARC>"
                        "; exiting!\n");
                     exit(-1);
                  }
                  if (GetType("TYPE_STRONGMOTIONII", &GetLogo[nLogo++].type) != 0)
                  {
                     logit("e",
                        "gmewhtmlemail: Invalid message type <TYPE_HYP2000ARC>"
                        "; exiting!\n");
                     exit(-1);
                  }
                  if (GetType("TYPE_THRESH_ALERT", &GetLogo[nLogo++].type) != 0)
                  {
                     logit("e",
                        "gmewhtmlemail: Invalid message type <TYPE_THRESH_ALERT>"
                        "; exiting!\n");
                     exit(-1);
                  }
               }
               else
               {
                  logit("e", "gmewhtmlemail: No module name "
                        "in <GetLogo> cmd; exiting\n");
                  exit(-1);
               }
            }
            else
            {
               logit("e", "gmewhtmlemail: No installation name "
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
               logit("e", "gmewhtmlemail: Excessive number of waveservers. Exiting.\n");
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
        if ( strcmp(StaticMapType,"hybrid")==0) 
            strcpy(MQStaticMapType,"hyb");
        else if (strcmp(StaticMapType,"terrain")==0||strcmp(StaticMapType,"roadmap")==0)
            strcpy(MQStaticMapType,"map");
        else if (strcmp(StaticMapType,"satellite")==0)
            strcpy(MQStaticMapType,"sat");
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
     else if ( k_its("EmailDelay") ) 
          {
             EmailDelay = k_val();
          }
     else if ( k_its("CenterPoint") ) 
          {
             center_lat = k_val();
             center_lon = k_val();
          }
     else if ( k_its("DataCenter") )
         {
        strcpy(DataCenter, strtok(k_str(),"\""));
     }
/*   else if (k_its("SPfilter")) 
         {
            SPfilter = 1;
         } */
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
     else if ( k_its("TraceWidth") ) 
         {
            TraceWidth = (int) k_int();
         }
/*7*/ else if ( k_its("site_file") ) 
         {
            strcpy(path, k_str());
            site_read ( path );
            init[7] = 1;            
         }
/*8*/ else if ( k_its("dam_file") ) 
         {
            strcpy(path, k_str());
            read_alldam_info ( path );
            init[8] = 1;            
         }
/*9*/ else if ( k_its("db_path") ) 
         {
            strcpy(db_path, k_str());
            init[9] = 1;            
         }
     else if ( k_its("MaxStationDist") ) 
         {
            MaxStationDist = (int) k_int();
         }
     else if ( k_its("MaxFacilityDist") ) 
         {
            MaxFacilityDist = (int) k_int();
         }
     else if ( k_its("IgnoreArcOlderThanSeconds") ) 
         {
            MaxArcAge = (int) k_int();
         }
         else if ( k_its("EmailRecipientWithMinMagAndDist") )
         {
            if (nemailrecipients<MAX_EMAIL_RECIPIENTS)
            {
                strcpy(emailrecipients[nemailrecipients].address, k_str());
                emailrecipients[nemailrecipients].min_magnitude = k_val();
                emailrecipients[nemailrecipients].max_distance = k_val();
                str = assignMailingList();
                if ( str )  {
                    logit("e","gmewhtmlemail: Illegal EmailRecipientWithMinMagAndDist mailing list: '%s'\n", str );
                    exit(-1);
                }
                nemailrecipients++;
            } 
            else
            {
                logit("e", "gmewhtmlemail: Excessive number of email recipients. Exiting.\n");
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
                str = assignMailingList();
                if ( str )  {
                    logit("e","gmewhtmlemail: Illegal EmailRecipientWithMinMag mailing list: '%s'\n", str );
                    exit(-1);
                }
                nemailrecipients++;
            } 
            else
            {
                logit("e", "gmewhtmlemail: Excessive number of email recipients. Exiting.\n");
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
                str = assignMailingList();
                if ( str )  {
                    logit("e","gmewhtmlemail: Illegal EmailRecipient mailing list: '%s'\n", str );
                    exit(-1);
                }
                nemailrecipients++;
            } 
            else
            {
                logit("e", "gmewhtmlemail: Excessive number of email recipients. Exiting.\n");
                        exit(-1);
            }
         }
         else if ( k_its("ShowMiles") )
         {
            ShowMiles = 1;
         }
         else if ( k_its("PlotAllSCNLs") )
         {
            PlotAllSCNLs = 1;
         }
         else if ( k_its("Disclaimer") )
         {
            char path[200];
            strcpy(path, k_str());
            read_disclaimer ( path );
         }
         else if ( k_its("MapQuestKey") )
         {
            strcpy(MapQuestKey, k_str());
         }
         else if ( k_its("ShowRegionAndArea") )
         {
            ShowRegionAndArea = 1;
            facility_font_size = 1;
         }
         else if ( k_its("IgnoreOlderThanHours") ) 
         {
            IgnoreHours = k_val();
         }
         /* Some commands may be processed by other functions
          ***************************************************/
         else if( site_com() )  strcpy( processor, "site_com" );
         /* Unknown command
          *****************/
         else 
         {
            logit("e", "gmewhtmlemail: <%s> Unknown command in <%s>.\n",
               com, configfile);
            continue;
         }

         /* See if there were any errors processing the command
          *****************************************************/
         if (k_err())
         {
            logit("e",
               "gmewhtmlemail: Bad <%s> command in <%s>; exiting!\n",
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
      logit("e", "gmewhtmlemail: ERROR, no ");
      if (!init[0]) logit("e", "<LogFile> ");
      if (!init[1]) logit("e", "<MyModuleId> ");
      if (!init[2]) logit("e", "<InRing> ");
      if (!init[3]) logit("e", "<HeartbeatInt> ");
      if (!init[4]) logit("e", "<GetLogo> ");
      if (!init[5]) logit("e", "<Debug> ");
      if (!init[6]) logit("e", "<HTMLFile> ");
      if (!init[7]) logit("e", "<site_file> ");
      if (!init[8]) logit("e", "<dam_file> ");
      if (!init[9]) logit("e", "<db_path> ");
      logit("e", "command(s) in <%s>; exiting!\n", configfile);
      exit(-1);
    }
   return;
}

/*********************************************************************
 *  lookup( )   Look up important info from earthworm.h tables       *
 *********************************************************************/
void lookup(void) 
 {
    /* Look up installations of interest
     *********************************/
    if (GetLocalInst(&InstId) != 0) {
        logit("e",
                "gmewhtmlemail: error getting local installation id; exiting!\n");
        exit(-1);
    }

    /* Look up message types of interest
     *********************************/
    if (GetType("TYPE_HEARTBEAT", &TypeHeartBeat) != 0) {
        logit("e",
                "gmewhtmlemail: Invalid message type <TYPE_HEARTBEAT>; exiting!\n");
        exit(-1);
    }
    if (GetType("TYPE_ERROR", &TypeError) != 0) {
        logit("e",
                "gmewhtmlemail: Invalid message type <TYPE_ERROR>; exiting!\n");
        exit(-1);
    }
    if (GetType("TYPE_HYP2000ARC", &TypeHYP2000ARC) != 0) {
        logit("e",
                "gmewhtmlemail: Invalid message type <TYPE_HYP2000ARC>; exiting!\n");
        exit(-1);
    }
    if (GetType("TYPE_TRACEBUF2", &TypeTraceBuf2) != 0) {
        logit("e",
                "gmewhtmlemail: Invalid message type <TYPE_TRACEBUF2>; exiting!\n");
        exit(-1);
    }
    if (GetType("TYPE_STRONGMOTIONII", &TypeSTRONGMOTIONII) != 0) {
        logit("e",
                "gmewhtmlemail: Invalid message type <TYPE_STRONGMOTIONII>; exiting!\n");
        exit(-1);
    }
    if (GetType("TYPE_THRESH_ALERT", &TypeTHRESH) != 0) {
        logit("e",
                "gmewhtmlemail: Invalid message type <TYPE_THRESH_ALERT>; exiting!\n");
        exit(-1);
    } else 
        logit( "t", "gmewhtmlemail: TYPE_THRESH_ALERT = %d\n", TypeTHRESH );
    return;
}

/******************************************************************************
 * status() builds a heartbeat or error message & puts it into                *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
void status(unsigned char type, short ierr, char *note) 
{
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
        logit("et", "gmewhtmlemail: %s\n", note);
    }

    size = strlen(msg); /* don't include the null byte in the message */

    /* Write the message to shared memory
     ************************************/
    if (tport_putmsg(&InRegion, &logo, size, msg) != PUT_OK) {
        if (type == TypeHeartBeat) {
            logit("et", "gmewhtmlemail:  Error sending heartbeat.\n");
        } else if (type == TypeError) {
            logit("et", "gmewhtmlemail:  Error sending error:%d.\n", ierr);
        }
    }

    return;
}













/* the magnitude type from hypoinverse: need to change if ML gets produced in future */
#define MAG_TYPE_STRING "Md"
#define MAG_MSG_TYPE_STRING "ML"
#define MAG_MSG_MWTYPE_STRING "Mw"

void InsertHeaderTable(FILE *htmlfile, HypoArc *arc, char Quality, int showDM, char *scnl ) 
{
    char        timestr[80], timestrUTC[80];                    /* Holds time messages */
    time_t      ot;
    //struct tm     *timeinfo;
    struct tm   mytimeinfo;
    char        *grname[36];          /* Flinn-Engdahl region name */
    int parity = 0;
    char        *bg[2] = {" bgcolor=\"DDDDFF\" class=\"alt\"", ""};
    char        *timeName;

        ot = ( time_t )( arc->sum.ot - GSEC1970 );
        //timeinfo = 
        localtime_r ( &ot, &mytimeinfo );
        //memcpy( &mytimeinfo, timeinfo, sizeof(mytimeinfo) );
        strftime( timestr, 80, "%Y.%m.%d %H:%M:%S", &mytimeinfo ); // Prepare origin time (local)

        //timeinfo = 
        gmtime_r ( &ot, &mytimeinfo );
        //memcpy( &mytimeinfo, timeinfo, sizeof(mytimeinfo) );
        strftime( timestrUTC, 80, "%Y.%m.%d %H:%M:%S", &mytimeinfo ); // Prepare origin time (UTC)
        
        // Table header
        fprintf( htmlfile, "<table id=\"DataTable\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n" );
        if(strlen(DataCenter) > 0 )
        {
             fprintf( htmlfile, "<tr bgcolor=\"000060\"><th><font size=\"3\" face=\"Sans-serif\" color=\"FFFFFF\">Data Center: %s</font><th><tr>\n", DataCenter );
            }
        if ( scnl ) {
            fprintf( htmlfile, "<tr bgcolor=\"000060\"><th><font size=\"3\" face=\"Sans-serif\" color=\"FFFFFF\">Threshold Trigger Exceedance</font><th><tr>\n" );

            fprintf( htmlfile, "<tr%s><td><font size=\"3\" face=\"Sans-serif\">Source:</font></td><td><font size=\"3\" face=\"Sans-serif\">%s</font></td><tr>\n", bg[parity], scnl );
            parity = 1-parity;
        } else {
            // Event ID
            fprintf( htmlfile, "<tr bgcolor=\"000060\"><th><font size=\"3\" face=\"Sans-serif\" color=\"FFFFFF\">EW Event ID: %ld</font><th><tr>\n", arc->sum.qid );
        
            // NEIC id
            if ( neic_id[0] ) {
                fprintf( htmlfile, "<tr%s><td><font size=\"3\" face=\"Sans-serif\">NEIC ID:</font></td><td><font size=\"3\" face=\"Sans-serif\"><a href=\"http://earthquake.usgs.gov/earthquakes/eventpage/%s\">%s</a></font></td><tr>\n", bg[parity], neic_id, neic_id );
                parity = 1-parity;
            }
        }

        timeName = ( neic_id[0] == 0 ) ? "Trigger" : "Origin";
        
        // Origin time (local)
        fprintf( htmlfile, "<tr%s><td><font size=\"3\" face=\"Sans-serif\">%s time (local):</font></td><td><font size=\"3\" face=\"Sans-serif\">%s</font></td><tr>\n",
                bg[parity], timeName, timestr );
        parity = 1-parity;

        // Origin time (UTC)
        fprintf( htmlfile, "<tr%s><td><font size=\"3\" face=\"Sans-serif\">%s time (UTC):</font></td><td><font size=\"3\" face=\"Sans-serif\">%s</font></td><tr>\n",
                bg[parity], timeName, timestrUTC );
        parity = 1-parity;

        // Seismic Region
        if (UseRegionName && ( arc->sum.lat != -1 || arc->sum.lon != -1 )) {
            FlEngLookup(arc->sum.lat, arc->sum.lon, grname, NULL);  
            fprintf( htmlfile, "<tr%s><td><font size=\"3\" face=\"Sans-serif\">Location:</font></td><td><font size=\"3\" face=\"Sans-serif\">%-36s</font></td><tr>\n",
                bg[parity], *grname );
            parity = 1-parity;
        }
                
        if ( arc->sum.lat != -1 || arc->sum.lon != -1 ) {
            // Latitude
            fprintf( htmlfile, "<tr%s><td><font size=\"3\" face=\"Sans-serif\">Latitude:</font></td><td><font size=\"3\" face=\"Sans-serif\">%7.4f</font></td><tr>\n", 
                bg[parity], arc->sum.lat );
            parity = 1-parity;
        
            // Longitude
            fprintf( htmlfile, "<tr%s><td><font size=\"3\" face=\"Sans-serif\">Longitude:</font></td><td><font size=\"3\" face=\"Sans-serif\">%8.4f</font></td><tr>\n",
                bg[parity],
                arc->sum.lon );
            parity = 1-parity;
        }
        
        if ( showDM ) {
            // Depth
            fprintf( htmlfile, "<tr%s><td><font size=\"3\" face=\"Sans-serif\">Depth:</font></td><td><font size=\"3\" face=\"Sans-serif\">%4.1f km</font></td><tr>\n", 
                bg[parity], arc->sum.z );
            parity = 1-parity;
        
            /*
                    if (arc->sum.mdwt == 0 && DontShowMd==0) 
                    {
            // Coda magnitude
                fprintf( htmlfile, "<tr bgcolor=\"DDDDFF\"><td><font size=\"3\" face=\"Sans-serif\">Coda Magnitude:</font></td>"
                    "<td><font size=\"3\" face=\"Sans-serif\">N/A %s nobs=None</font></td><tr>\n", 
                    MAG_TYPE_STRING );
                    } else if (DontShowMd == 0)
                    {
            // Coda magnitude
                fprintf( htmlfile, "<tr bgcolor=\"DDDDFF\"><td><font size=\"3\" face=\"Sans-serif\">Coda Magnitude:</font></td>"
                    "<td><font size=\"3\" face=\"Sans-serif\">%4.1f %s nobs=%d</font></td><tr>\n", 
                    arc->sum.Mpref, MAG_TYPE_STRING, (int) arc->sum.mdwt );
                    }
            */
            fprintf( htmlfile, "<tr%s><td><font size=\"3\" face=\"Sans-serif\">Magnitude:</font></td>"
                "<td><font size=\"3\" face=\"Sans-serif\">%4.1f</font></td><tr>\n", 
                bg[parity], arc->sum.Mpref );
            parity = 1-parity;
        }
        

        /* Event details
         ***************/
        if( ShowDetail )
        {
                
                // RMS
            fprintf( htmlfile, "<tr%s><td><font size=\"3\" face=\"Sans-serif\">RMS Error:</font></td><td><font size=\"3\" face=\"Sans-serif\">%5.2f s</font></td><tr>\n", 
                    bg[parity], arc->sum.rms);
            parity = 1-parity;
                    
            // Horizontal error
            fprintf( htmlfile, "<tr%s><td><font size=\"3\" face=\"Sans-serif\">Horizontal Error:"
                    "</font></td><td><font size=\"3\" face=\"Sans-serif\">%5.2f km</font></td><tr>\n", 
                    bg[parity], arc->sum.erh );
            parity = 1-parity;
                    
            // Vertical error
            fprintf( htmlfile, "<tr%s><td><font size=\"3\" face=\"Sans-serif\">Depth Error:</font></td><td><font size=\"3\" face=\"Sans-serif\">%5.2f km</font></td><tr>\n", 
                    bg[parity], arc->sum.erz );
            parity = 1-parity;
                    
            // Azimuthal gap
            fprintf( htmlfile, "<tr%s><td><font size=\"3\" face=\"Sans-serif\">Azimuthal Gap:</font></td>"
                    "<td><font size=\"3\" face=\"Sans-serif\">%d Degrees</font></td><tr>\n", 
                    bg[parity], arc->sum.gap );
            parity = 1-parity;
                    
            // Number of phases
            fprintf( htmlfile, "<tr%s><td><font size=\"3\" face=\"Sans-serif\">Total Phases:</font></td><td><font size=\"3\" face=\"Sans-serif\">%d</font></td><tr>\n",
                    bg[parity], arc->sum.nphtot);
            parity = 1-parity;
            
            // Used phases
            fprintf( htmlfile, "<tr%s><td><font size=\"3\" face=\"Sans-serif\">Total Phases Used:</font></td>"
                    "<td><font size=\"3\" face=\"Sans-serif\">%d</font></td><tr>\n", 
                    bg[parity], arc->sum.nph );
            parity = 1-parity;
            
            // Number of S phases
            fprintf( htmlfile, "<tr%s><td><font size=\"3\" face=\"Sans-serif\">Num S Phases Used:</font></td><td><font size=\"3\" face=\"Sans-serif\">%d</font></td><tr>\n",
                    bg[parity], arc->sum.nphS );
            parity = 1-parity;
                    
            // Average quality
            fprintf(htmlfile, "<tr%s><td><font size=\"3\" face=\"Sans-serif\">Quality:</font></td>"
                    "<td><font size=\"3\" face=\"Sans-serif\">%c</font></td><tr>\n", 
                    bg[parity], Quality);
            parity = 1-parity;
        }
        
        // Finish reference table
        fprintf( htmlfile, "</table><br/><br/>\n" );
}
         
/* this is a squished down table  for the header info*/
void InsertShortHeaderTable(FILE *htmlfile, HypoArc *arc,char Quality ) 
{
    char        timestr[80];                    /* Holds time messages */
    time_t      ot;
    struct tm   mytimeinfo; //, *timeinfo;
    char        *grname[36];          /* Flinn-Engdahl region name */
    struct tm * (*timefunc)(const time_t *, struct tm *);
    char        time_type[30];                  /* Time type UTC or local */
    int parity = 0;
    char        *bg[2] = {" bgcolor=\"DDDDFF\" class=\"alt\"", ""};

        timefunc = localtime_r;
        if( UseUTC )
        { 
            timefunc = gmtime_r;
            strcpy( time_type, "UTC" );
        }
        ot = ( time_t )( arc->sum.ot - GSEC1970 );
        //timeinfo = 
        timefunc ( &ot, &mytimeinfo );
        //memcpy( &mytimeinfo, timeinfo, sizeof(mytimeinfo) );
        strftime( timestr, 80, "%Y.%m.%d %H:%M:%S", &mytimeinfo ); // Prepare origin time
        
        // Table header
        fprintf( htmlfile, "<table id=\"DataTable\" border=\"0\" cellspacing=\"1\" cellpadding=\"0\" width=\"600px\">\n" );
        if(strlen(DataCenter) > 0 )
        {
             fprintf( htmlfile, 
                    "<tr bgcolor=\"000060\">"
                        "<th colspan=4><font size=\"3\" face=\"Sans-serif\" color=\"FFFFFF\">Data Center: %s</font></th>"
                    "</tr>\n", DataCenter );
            }
        // Event ID
        fprintf( htmlfile,
                "<tr bgcolor=\"000060\">"
                    "<th colspan=4><font size=\"3\" face=\"Sans-serif\" color=\"FFFFFF\">EW Event ID: %ld</font></th>"
                "</tr>\n", arc->sum.qid );
        // Seismic Region
        if (UseRegionName && ( arc->sum.lat != -1 || arc->sum.lon != -1 )) {
            FlEngLookup(arc->sum.lat, arc->sum.lon, grname, NULL);  
            fprintf( htmlfile, 
                "<tr%s>"
                    "<td colspan=2><font size=\"3\" face=\"Sans-serif\"><b>Location:</b></font></td>"
                    "<td colspan=2><font size=\"3\" face=\"Sans-serif\">%-36s</font></td>"
                "</tr>\n",
                bg[parity],
                *grname );
            parity = 1-parity;
        }
                
        
        // Origin time
        fprintf( htmlfile, 
                "<tr%s>"
                    "<td><font size=\"3\" face=\"Sans-serif\"><b>Origin time:</b></font></td>"
                    "<td><font size=\"3\" face=\"Sans-serif\">%s %s</font></td>\n",
                bg[parity], timestr, time_type );
        parity = 1-parity;
            // RMS
        fprintf( htmlfile, 
                "<td><font size=\"3\" face=\"Sans-serif\"><b>RMS Error:</b></font></td>"
                "<td><font size=\"3\" face=\"Sans-serif\">%5.2f s</font></td>"
                "</tr>\n", 
                arc->sum.rms);
        // Latitude
        // Longitude
        if ( arc->sum.lat != -1 || arc->sum.lon != -1 ) {
            fprintf( htmlfile, 
                "<tr%s>"
                    "<td><font size=\"3\" face=\"Sans-serif\"><b>Latitude, Longitude:</b></font></td>"
                    "<td><font size=\"3\" face=\"Sans-serif\">%7.4f, %8.4f</font></td>\n", 
                    bg[parity], arc->sum.lat, arc->sum.lon );
            parity = 1-parity;  
        }       
        // Horizontal error
        fprintf( htmlfile, 
                    "<td><font size=\"3\" face=\"Sans-serif\"><b>Horizontal Error:</b></font></td>"
                    "<td><font size=\"3\" face=\"Sans-serif\">%5.2f km</font></td>"
                    "</tr>\n", arc->sum.erh );
                
        // Depth
        fprintf( htmlfile, 
                "<tr%s>"
                    "<td><font size=\"3\" face=\"Sans-serif\"><b>Depth:</b></font></td>"
                    "<td><font size=\"3\" face=\"Sans-serif\">%4.1f km</font></td>", 
                    bg[parity], arc->sum.z );
        parity = 1-parity;
        // Vertical error
        fprintf( htmlfile, 
                    "<td><font size=\"3\" face=\"Sans-serif\"><b>Depth Error:</b></font></td>"
                    "<td><font size=\"3\" face=\"Sans-serif\">%5.2f km</font></td>"
                "</tr>\n", 
                arc->sum.erz );
        
        // Coda magnitude
        if (arc->sum.mdwt == 0 && DontShowMd == 0) {
            fprintf( htmlfile, 
                "<tr%s>"
                    "<td><font size=\"3\" face=\"Sans-serif\"><b>Coda Magnitude:</b></font></td>"
                    "<td><font size=\"3\" face=\"Sans-serif\">N/A %s</font></td>"
                    "<td><font size=\"3\" face=\"Sans-serif\">nobs=0</font></td>"
                "</tr>\n", 
                 bg[parity], MAG_TYPE_STRING );
            parity = 1-parity;
        } else if (DontShowMd == 0) {
            fprintf( htmlfile, 
                "<tr%s>"
                    "<td><font size=\"3\" face=\"Sans-serif\"><b>Coda Magnitude:</b></font></td>"
                    "<td><font size=\"3\" face=\"Sans-serif\">%4.1f %s</font></td>"
                    "<td><font size=\"3\" face=\"Sans-serif\">nobs=%d</font></td>"
                "</tr>\n", 
                bg[parity], arc->sum.Mpref, MAG_TYPE_STRING, (int) arc->sum.mdwt );
            parity = 1-parity;
        }   
                        
        // Average quality
        fprintf( htmlfile, 
                "<tr%s>"
                    "<td><font size=\"3\" face=\"Sans-serif\"><b>Quality:</b></font></td>"
                    "<td><font size=\"3\" face=\"Sans-serif\">%c</font></td>", bg[parity], Quality);
        parity = 1-parity;  
                
        // Azimuthal gap
        fprintf( htmlfile, 
                    "<td><font size=\"3\" face=\"Sans-serif\"><b>Azimuthal Gap:</b></font></td>"
                    "<td><font size=\"3\" face=\"Sans-serif\">%d Degrees</font></td>"
                "</tr>\n", arc->sum.gap );
                    
        // Number of phases
        fprintf( htmlfile, 
                "<tr%s>"
                    "<td><font size=\"3\" face=\"Sans-serif\"><b>Total Phases:</b></font></td>"
                    "<td><font size=\"3\" face=\"Sans-serif\">%d</font></td>", bg[parity], arc->sum.nphtot);
        parity = 1-parity;
            
        // Used phases
        fprintf( htmlfile, 
                    "<td><font size=\"3\" face=\"Sans-serif\"><b>Total Phases Used:</b></font></td>"
                    "<td><font size=\"3\" face=\"Sans-serif\">%d</font></td>"
                "</tr>\n", arc->sum.nph );
            
        // Number of S phases
        fprintf( htmlfile, 
                "<tr%s>"
                    "<td><font size=\"3\" face=\"Sans-serif\"><b>Num S Phases Used:</b></font></td>"
                    "<td><font size=\"3\" face=\"Sans-serif\">%d</font> </td>"
                "</tr>\n",
                bg[parity], arc->sum.nphS );
        parity = 1-parity;
                    
        
        // Finish reference table
        fprintf( htmlfile, "</table><br/><br/>\n" );         
}

int compareSiteOrder( const void*site_p_1, const void*site_p_2 ) 
{
    double diff = ((SiteOrder*)site_p_1)->key - ((SiteOrder*)site_p_2)->key;
    return diff<0 ? -1 : diff>0 ? 1 : ((SiteOrder*)site_p_1)->idx - ((SiteOrder*)site_p_2)->idx;
}

int compareDamOrder( const void*dam_p_1, const void*dam_p_2 ) 
{
    double diff = ((DamInfo*)dam_p_1)->dist - ((DamInfo*)dam_p_2)->dist;
    return diff<0 ? -1 : diff>0 ? 1 : 0;
}

int callback1(void* notUsed, int argc, char** argv, char** azColName  )
{
    //notUsed = 0;
    strcpy( neic_id, argv[0] );
    char *p = neic_id;
    while ( *p != 0 && *p != ' ' )
        p++;
    *p = 0;
    return 0;
}

/******************************************************************************
 * process_message() Processes a message to find if its a real event or not   *
 ******************************************************************************/
int process_message( int arc_index ) // SM_INFO *sm ) //, MAG_INFO *mag_l, MAG_INFO *mag_w) 
{
    HypoArc     *arc = NULL;
    HypoArcSM2  *arcsm;
    double      starttime = 0;                  /* Start time for all traces */
    double      endtime = 0;                    /* End time for all traces */
    double      dur;                            /* Duration of the traces */
    int         i, pos;                         /* Generic counters */
    int         gsamples[MAX_GET_SAMP];         /* Buffer for resampled traces */
    char        chartreq[50000];                    /* Buffer for chart requests or GIFs */     /* 2013.05.29 for base64 */
    SITE        *sites[MAX_STATIONS];           /* Selected sites */
    char        phases[MAX_STATIONS][3];            /* Phase types of each trace */
    double      arrivalTimes[MAX_STATIONS];     /* Arrival times of each trace */
    //double        residual[MAX_STATIONS];         /* phase residual */
    //int       coda_lengths[MAX_STATIONS];     /* Coda lengths of each trace */
    double      coda_mags[MAX_STATIONS];        /* station coda mags > 0 if used */
    int     coda_weights[MAX_STATIONS];         /* Coda Weight codes for each trace */
    //int       weights[MAX_STATIONS];          /* Weight codes for each trace */
    double      distances[MAX_STATIONS];        /* Distance from each station */
    SM_INFO *smForSite[MAX_STATIONS];
    SiteOrder   site_order[MAX_STATIONS];
    SITE        *sorted_sites[MAX_STATIONS];
    SITE        *waveSite, triggerSite;
    char        *cPtr, *phaseName;
    
    struct tm * (*timefunc)(const time_t *, struct tm *);
    char        time_type[30];                  /* Time type UTC or local */
//  int     codaLen;                        /* Holds the coda length absolute */
    int     nsites = 0;                     /* Number of stations in the arc msg */
    char        system_command[MAX_STRING_SIZE];/* System command to call email prog */
    char        kml_filename[MAX_STRING_SIZE];  /* Name of kml file to be generated */
//  FILE        *cssfile;                       /* CSS File to be embedded in email */
    FILE        *htmlfile;                      /* HTML file */
    FILE        *header_file;                   /* Email header file */
//  char        ch;
    time_t      ot, st;                         /* Times */
    struct tm   mytimeinfo; //, *timeinfo;
//        char            *basefilename;
    char        timestr[80];                    /* Holds time messages */
    char        fullFilename[250];              /* Full html file name */
    char        hdrFilename[250];               /* Full header file name */
    char        *buffer;                        /* Buffer for raw tracebuf2 messages */
    int     bsamplecount;                   /* Length of buffer */
    char        giffilename[256];               /* Name of GIF files */
    gdImagePtr  gif;                            /* Pointer for GIF image */
    FILE        *giffile;                       /* GIF file */
    int     BACKCOLOR;                      /* Background color of GIF images */
    int     TRACECOLOR;                     /* Trace color in GIF images */
    int     PICKCOLOR;                      /* Pick color in GIF images */
    char        Quality;        /* event quality */
    FILE*       gifHeader;                      /* Header for GIF attachments with sendmail */
    char        gifHeaderFileName[257];         /* GIF Header filename */
    unsigned char* gifbuffer;
        unsigned char*  base64buf;
        size_t  gifbuflen;
        size_t  base64buflen;
//      char    gifFileName[256];
        char    contentID[256];  
        int     rv = TRUE;
   char temp[MAX_GET_CHAR];
   char *request = chartreq;

    
    arcsm = arc_list[arc_index];
    arc = &(arcsm->ha);
    
    sqlite3 *db;
    char *err_msg = 0;
    
    if ( dbAvailable ) {
        rv = sqlite3_open(db_path, &db);
    
        if (rv != SQLITE_OK) {       
            logit("et", "Cannot open database: %s\n", sqlite3_errmsg(db));
        } else {
            char evt_sql[200];
            neic_id[0] = 0;
            sprintf( evt_sql, "SELECT PdlID FROM Events WHERE ROWID = %ld", arc->sum.qid );
            rv = sqlite3_exec(db, evt_sql, callback1, 0, &err_msg);
            sqlite3_close(db);
        }
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
    for (i=0; i<arcsm->sm_count; i++)
    {
    
        // Search this site on the sites file
        pos = site_index( arcsm->sm_arr[i].sta, 
                arcsm->sm_arr[i].net,
                arcsm->sm_arr[i].comp,
                arcsm->sm_arr[i].loc[0] ? arcsm->sm_arr[i].loc : "--") ;
        // If site is not found, continue to next phase
        if( pos == -1 )
        {
            logit( "et", "gmewhtmlemail: Unable to find %s.%s.%s.%s (%d) on the site file\n", 
                    arcsm->sm_arr[i].sta,
                    arcsm->sm_arr[i].comp,
                    arcsm->sm_arr[i].net,
                    arcsm->sm_arr[i].loc,
                    i
                 );
            continue;
        }
        
        
        // New station, store its pointer
        sites[nsites] = &Site[pos];     
        
        // Store epicentral distance
        site_order[nsites].key = distances[nsites] = distance( arc->sum.lat, arc->sum.lon, 
                                        Site[pos].lat, Site[pos].lon, 'K' );
        
        if ( distances[nsites] > MaxStationDist )
            continue;
        site_order[nsites].idx = nsites;
        smForSite[nsites] = arcsm->sm_arr+i;
                
        //distances[nsites] = arc->phases[i]->dist;
        
        
        // Store coda duration
        //coda_lengths[nsites] = arc->phases[i]->codalen;
        
        
        /* Md from channel for highest quality 
        if( arc->phases[i]->codawt < 4 )
        {
            coda_mags[nsites] = arc->phases[i]->Md;
            coda_weights[nsites] = arc->phases[i]->codawt;
        }
        else
        {
            coda_mags[nsites] = 0.0;
        } */

        // Increment nsites
        nsites++;
        
        
        /* Update endtime
         * Takes the last time instant of the latest event, defined as
         * the arrival time + coda length to set the overall endtime
         *************************************************************
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
        if( ( arc->phases[i]->Pat + codaLen ) > endtime )       //using P arrival or
            endtime = arc->phases[i]->Pat + codaLen;            //
        if( ( arc->phases[i]->Sat + codaLen ) > endtime )       //using S arrival
            endtime = arc->phases[i]->Sat + codaLen;
        */ 

        /* Check if the number of sites exceeds the
         * predefined maximum number of stations
         ******************************************/
        if( nsites >= MAX_STATIONS ) 
        {
            logit( "et", "gmewhtmlemail: More than %d stations in message\n",
                    MAX_STATIONS );
            break;
        }
    } // End of 'for' cycle for processing the phases in the arc message
    
    qsort( site_order, nsites, sizeof(SiteOrder), &compareSiteOrder );
    for ( i=0; i<nsites; i++ )
        sorted_sites[i] = sites[site_order[i].idx];
    
    /* Correct times for epoch 1970
     ******************************/
    endtime = starttime;
    starttime -= GSEC1970;      // Correct for epoch
    
    ot = time( NULL );
    
    if ( IgnoreHours>0 && (ot - starttime)/3600 > IgnoreHours ) {
        logit("o", "gmewhtmlemail: Ignoring event more than %1.2lf hours old (%1.2lf hours)\n", IgnoreHours, (ot - starttime)/3600 );
        return 0;
    } 
    
    //printf("Age: %lf sec (%lf hours, %lf days)\n", ot - starttime, (ot - starttime)/3600, (ot - starttime)/(3600*24) );

    starttime -= TimeMargin;    // Add time margin
    endtime -= GSEC1970;        // Correct for epoch
    endtime += TimeMargin;      // Add time margin

    /* Check maximum duration of the traces
     **************************************/
    dur = endtime - starttime;
    if( 1 ) // dur > ( double ) DurationMax ) 
    {
//        if( Debug ) logit( "ot", "Waveform Durations %d greater "
//              "than set by MaxDuration %d\n", 
//              (int) (endtime - starttime), (int)DurationMax);
        endtime = starttime+DurationMax;
        dur = DurationMax;  /* this is used later for header of waveform table */
    }
    
    /* Change to UTC time, if required
     *********************************/
    timefunc = localtime_r;
    if( UseUTC )
    { 
        timefunc = gmtime_r;
        strcpy( time_type, "UTC" );
    }
    
    /* Log debug info
     ****************/
    if( Debug )
    {
        logit("o", "Available channels (%d):\n", nsites);
        for( i = 0; i < nsites; i++ )
            logit( "o", "%5s.%3s.%2s.%2s\n",
                    sorted_sites[i]->name, sorted_sites[i]->comp,
                    sorted_sites[i]->net, sorted_sites[i]->loc);
            
        logit( "o", "Time margin: %f\n", TimeMargin );
        
        ot = ( time_t ) starttime;
        //timeinfo = 
        timefunc ( &ot, &mytimeinfo );
        //memcpy( &mytimeinfo, timeinfo, sizeof(mytimeinfo) );
        strftime( timestr, 80, "%Y.%m.%d %H:%M:%S", &mytimeinfo );
        logit( "o", "Waveform starttime: %s %s\n", timestr, time_type );
        
        ot = ( time_t ) endtime;
        //timeinfo = 
        timefunc ( &ot, &mytimeinfo );
        //memcpy( &mytimeinfo, timeinfo, sizeof(mytimeinfo) );
        strftime( timestr, 80, "%Y.%m.%d %H:%M:%S", &mytimeinfo );
        logit( "o", "Waveform endtime:   %s %s\n", timestr, time_type );
    }
    

    for ( i=0; i<dam_count; i++ )
        dam_tbl[i].dist = distance( arc->sum.lat, arc->sum.lon, 
                                    dam_tbl[i].lat, dam_tbl[i].lon, 'K' );
    qsort( dam_tbl, dam_count, sizeof(DamInfo), &compareDamOrder );
    dam_close_count = dam_count;
    for ( i=0; i<dam_count; i++ )
    {
        if ( dam_tbl[i].dist > MaxFacilityDist ) {
            dam_close_count = i;
            break;
        }
    }
    
    if ( dam_close_count == 0 ) {
        logit("t", "gmewhtmlemail: No facilities within %d km; aborting email for event id: %ld\n", MaxFacilityDist, arc->sum.qid);
        return 0;
    }
    
    /* At this point, we have retrieved the information required for the html email
     * traces, if there is any. Now, move forward to start producing the output files.
     *********************************************************************************/


    /* build a KML file
     ******************/
    if( KMLdir[0] != 0 )
    {
        logit( "ot", "gmewhtmlemail: writing KML file.\n" );
        kml_writer( KMLdir, &(arc->sum), KMLpreamble, kml_filename, sorted_sites, nsites );
    }
    
    
    /* Start html email file
     ***********************/
    if ( neic_id[0] )
        sprintf(fullFilename, "%s_%ld_%s.html", HTMLFile, arc->sum.qid, neic_id ); // Name of html file
    else
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
                    InsertShortHeaderTable(htmlfile, arc, Quality);
                    //InsertShortHeaderTable(htmlfile, arc, mag_l, mag_w, Quality);
        } else {
                    InsertHeaderTable(htmlfile, arc, Quality, arcsm->fromArc, arcsm->fromArc ? NULL : arcsm->sncl );
                    //InsertHeaderTable(htmlfile, arc, mag_l, mag_w, Quality);
        }
         
        fprintf( htmlfile, "<table id=\"DamTable\" border=\"0\" cellspacing=\"1\" cellpadding=\"3\" width=\"600px\">\n" ); 
        fprintf( htmlfile, "<tr bgcolor=\"000060\">");
        fprintf( htmlfile, "<th><font size=\"%d\" face=\"Sans-serif\" color=\"FFFFFF\">#</font></th>", facility_font_size);
        fprintf( htmlfile, "<th><font size=\"%d\" face=\"Sans-serif\" color=\"FFFFFF\">Facility</font></th>", facility_font_size);
        if ( ShowRegionAndArea ) {
            fprintf( htmlfile, "<th><font size=\"%d\" face=\"Sans-serif\" color=\"FFFFFF\">Region</font></th>", facility_font_size);
            fprintf( htmlfile, "<th><font size=\"%d\" face=\"Sans-serif\" color=\"FFFFFF\">Area</font></th>", facility_font_size);
        }
        fprintf( htmlfile, "<th><font size=\"%d\" face=\"Sans-serif\" color=\"FFFFFF\">Dist.(km)</font></th>", facility_font_size);
        if ( ShowMiles )
            fprintf( htmlfile, "<th><font size=\"%d\" face=\"Sans-serif\" color=\"FFFFFF\">Dist.(mi)</font></th>", facility_font_size);
        fprintf( htmlfile, "</tr>\n" );

        for ( i=0; i<dam_close_count; i++ )
        {
            fprintf( htmlfile,"<tr bgcolor=\"#%s\">", i%2==0 ? "DDDDFF" : "FFFFFF");  
            if ( MapQuestKey[0] == 0 && i < 26 )          
                fprintf( htmlfile, "<td><font size=\"%d\" face=\"Sans-serif\">%c</font></td>", facility_font_size, i+'A' );
            else
                fprintf( htmlfile, "<td><font size=\"%d\" face=\"Sans-serif\">%d</font></td>", facility_font_size, i+1 );
            fprintf( htmlfile, "<td><font size=\"%d\" face=\"Sans-serif\">%s</font></td>", facility_font_size, dam_tbl[i].name );
            if ( ShowRegionAndArea ) {
                fprintf( htmlfile, "<td><font size=\"%d\" face=\"Sans-serif\">%s</font></td>", facility_font_size, region_names[dam_tbl[i].region_id] );
                fprintf( htmlfile, "<td><font size=\"%d\" face=\"Sans-serif\">%s</font></td>", facility_font_size, area_names[dam_tbl[i].area_id] );
            }
            fprintf( htmlfile, "<td style=\"text-align:right\"><font size=\"%d\" face=\"Sans-serif\">%1.1lf</font></td>", facility_font_size, dam_tbl[i].dist);
            if ( ShowMiles )
                fprintf( htmlfile, "<td style=\"text-align:right\"><font size=\"%d\" face=\"Sans-serif\">%1.1lf</font></td>", facility_font_size, dam_tbl[i].dist*KM2MILES);
            fprintf( htmlfile, "</tr>\n" );
        }
        fprintf( htmlfile, "</table><br/><br/>\n" ); 

       int ix;
       double max_dist, max_pga, max_dist_pad, max_pga_pad;
       SiteOrder snl_order[MAX_STATIONS];
       int snl_sites = 1;
      
       if ( (arcsm->fromArc) && (nsites > 0) ) {
           int j;
           snl_order[0] = site_order[0];
           for ( i=1; i<nsites; i++ ) {
                if ( PlotAllSCNLs )
                    snl_order[snl_sites++] = site_order[i];
                else {
                    ix = site_order[i].idx;
                    for ( j=0; j<snl_sites; j++ ) {
                        int jx = snl_order[j].idx;
                        if ( !strcmp( smForSite[ix]->sta, smForSite[jx]->sta) &&
                            !strcmp( smForSite[ix]->net, smForSite[jx]->net) &&
                            !strcmp( smForSite[ix]->loc, smForSite[jx]->loc) ) {
                            if ( smForSite[ix]->pga > smForSite[jx]->pga ) {
printf("Moving %s.%s to head (%lf %lf)\n", smForSite[ix]->sta, smForSite[ix]->comp, smForSite[ix]->pga, smForSite[jx]->pga );
                                snl_order[j].idx = ix;
                            }
                            break;
                        }
                    }
                    if ( j == snl_sites ) {
printf("Adding %s.%s\n", smForSite[ix]->sta, smForSite[ix]->comp );
                        snl_order[snl_sites] = site_order[i];
                        snl_sites++;
                    }
                }
           }
        } else {
            for ( i=0; i<nsites; i++ )
                snl_order[i] = site_order[i];
        }

        if ( nsites > 0 ) {            
            fprintf( htmlfile, "<table id=\"StationTable\" border=\"0\" cellspacing=\"1\" cellpadding=\"3\" width=\"600px\">\n" ); 
            fprintf( htmlfile, "<tr bgcolor=\"000060\">");
            
                fprintf( htmlfile, "<th>#</th><th><font size=\"1\" face=\"Sans-serif\" color=\"FFFFFF\">Station</font></th>");
            if ( arcsm->fromArc ) {
                fprintf( htmlfile, "<th><font size=\"1\" face=\"Sans-serif\" color=\"FFFFFF\">Distance (km)</font></th>");
                if ( ShowMiles )
                     fprintf( htmlfile, "<th><font size=\"1\" face=\"Sans-serif\" color=\"FFFFFF\">Distance (mi)</font></th>");
            }
            fprintf( htmlfile, "<th><font size=\"1\" face=\"Sans-serif\" color=\"FFFFFF\">%%g</font></th>");
            fprintf( htmlfile, "<th><font size=\"1\" face=\"Sans-serif\" color=\"FFFFFF\">PGA (cm/s/s)</font></th>");
            fprintf( htmlfile, "<th><font size=\"1\" face=\"Sans-serif\" color=\"FFFFFF\">PGV (cm/s)</font></th>");
            fprintf( htmlfile, "<th><font size=\"1\" face=\"Sans-serif\" color=\"FFFFFF\">PGD (cm)</font></th>");
            fprintf( htmlfile, "<th><font size=\"1\" face=\"Sans-serif\" color=\"FFFFFF\">RSA 0.3s (cm/s/s)</font></th>");
            fprintf( htmlfile, "<th><font size=\"1\" face=\"Sans-serif\" color=\"FFFFFF\">RSA 1s (cm/s/s)</font></th>");
            fprintf( htmlfile, "<th><font size=\"1\" face=\"Sans-serif\" color=\"FFFFFF\">RSA 3s (cm/s/s)</font></th>");
            fprintf( htmlfile, "</tr>\n" );

            int snl_i = 0;
            for( i = 0; i < nsites; i++ ) 
            {
                int j;
                int ix = site_order[i].idx;
                fprintf( htmlfile,"<tr bgcolor=\"#%s\">", i%2==0 ? "DDDDFF" : "FFFFFF");
                if ( snl_order[snl_i].idx == ix ) {
                    snl_i++;
                    fprintf( htmlfile, "<th><font size=\"1\" face=\"Sans-serif\">%d</font></th>", snl_i );
                } else
                    fprintf( htmlfile, "<th><font size=\"1\" face=\"Sans-serif\"></font></th>" );
                fprintf( htmlfile, "<td><font size=\"1\" face=\"Sans-serif\">%s.%s.%s.%s</font></td>",
                        sorted_sites[i]->name, sorted_sites[i]->comp,
                        sorted_sites[i]->net, sorted_sites[i]->loc );
                if ( arcsm->fromArc ) {
                    fprintf( htmlfile, "<td style=\"text-align:right\"><font size=\"1\" face=\"Sans-serif\">%1.1lf</font></td>", distances[ix]);
                    if ( ShowMiles )
                        fprintf( htmlfile, "<td style=\"text-align:right\"><font size=\"1\" face=\"Sans-serif\">%1.1lf</font></td>", distances[ix]*KM2MILES);
                }
                fprintf( htmlfile, "<td style=\"text-align:right\"><font size=\"1\" face=\"Sans-serif\">%1.6lf</font></td>", 100*smForSite[ix]->pga/980);
                fprintf( htmlfile, "<td style=\"text-align:right\"><font size=\"1\" face=\"Sans-serif\">%1.6lf</font></td>", smForSite[ix]->pga);
                fprintf( htmlfile, "<td style=\"text-align:right\"><font size=\"1\" face=\"Sans-serif\">%1.6lf</font></td>", smForSite[ix]->pgv);
                fprintf( htmlfile, "<td style=\"text-align:right\"><font size=\"1\" face=\"Sans-serif\">%1.6lf</font></td>", smForSite[ix]->pgd);
                for ( j=0; j<3; j++ )
                    if ( j < smForSite[i]->nrsa)
                        fprintf( htmlfile, "<td style=\"text-align:right\"><font size=\"1\" face=\"Sans-serif\">%1.6lf</font></td>", smForSite[ix]->rsa[j]);
                    else
                        fprintf( htmlfile, "<td>&nbsp;</td>");
                fprintf( htmlfile, "</tr>\n" );
            }
            fprintf( htmlfile, "</table><br/><br/>\n" ); 
        } else  if ( arcsm->fromArc ) {
            fprintf( htmlfile, "<p>No sites for table</p><br/><br/>"); 
        }

        /* Produce google map with the stations and hypocenter
         *****************************************************/
         i=-1;
         if(GMTmap)
         {
             if( Debug ) logit( "ot", "Computing GMT map\n" );
             if((i=gmtmap(chartreq,sorted_sites,nsites,arc->sum.lat,arc->sum.lon,arc->sum.qid)) == 0)
                fprintf(htmlfile, "%s\n<br/><br/>", chartreq);
        }
        
        if(i==-1){
            if( Debug ) logit( "ot", "Computing Google map\n" );
            //if ( arcsm->fromArc ) 
                MapRequest( chartreq, sorted_sites, nsites, arc->sum.lat, arc->sum.lon, arcsm, site_order, snl_order );
            //else
            //    GoogleMapRequest( chartreq, sorted_sites, nsites, 0, 0, arcsm );
            fprintf(htmlfile, "%s\n<br/><br/>", chartreq);
        }

        //GoogleChartRequest( chartreq, sorted_sites, nsites, distances );

           //printf("%s: max pga=%lf, max dist=%lf\n", neic_id, max_pga, max_dist );
           
                    
           if ( (arcsm->fromArc) && (nsites > 0) ) {
               ix = snl_order[snl_sites-1].idx;
               max_dist = distances[ix];
               max_dist_pad = max_dist*1.1;
               max_pga = smForSite[snl_order[0].idx]->pga;
               max_pga_pad = max_pga*1.1;
               
    
               /* Base of the google static chart
                *********************************/
               snprintf(request, MAX_GET_CHAR, "<img src=\"http://chart.apis.google.com/chart?chs=600x400&cht=s&chts=000000,24&chtt=Peak+PGA+vs.+Distance" );
    
    
               /* Add distances (x axis)
                ************************/
               ix = snl_order[0].idx;
               snprintf( temp, MAX_GET_CHAR, "%s&chd=t:%lf", request, distances[ix]/max_dist_pad*100 );
               snprintf( request, MAX_GET_CHAR, "%s", temp );
               for( i = 1; i < snl_sites; i++ )
               {
                  ix = snl_order[i].idx;
                  snprintf( temp, MAX_GET_CHAR, "%s,%lf", request, distances[ix]/max_dist_pad*100 );
                  snprintf( request, MAX_GET_CHAR, "%s", temp );
                  if ( max_pga < smForSite[ix]->pga )
                    max_pga = smForSite[ix]->pga;
               }
               max_pga_pad = max_pga*1.1;
            
               /* Add PGAs (y axis)
                *******************/
               ix = snl_order[0].idx;
               snprintf( temp, MAX_GET_CHAR, "%s|%lf", request, smForSite[ix]->pga/max_pga_pad*100 );
               snprintf( request, MAX_GET_CHAR, "%s", temp );
               for( i = 1; i < snl_sites; i++ )
               {
                  ix = snl_order[i].idx;
                  snprintf( temp, MAX_GET_CHAR, "%s,%lf", request, smForSite[ix]->pga/max_pga_pad*100 );
                  snprintf( request, MAX_GET_CHAR, "%s", temp );
               }

               /* Add annotations
                *****************/
                snprintf( temp, MAX_GET_CHAR, "%s&chm=B,333333,0,0,7", request );
                snprintf( request, MAX_GET_CHAR, "%s", temp );
                for( i = 0; i < snl_sites; i++ ) {
                    ix = snl_order[i].idx;
                    snprintf( temp, MAX_GET_CHAR, "%s|A%s.%s.%s.%s,333333,0,%d,7", request,
                            smForSite[ix]->sta, smForSite[ix]->comp,
                            smForSite[ix]->net, smForSite[ix]->loc, i );
//                            sorted_sites[i]->name, sorted_sites[i]->comp,
//                            sorted_sites[i]->net, sorted_sites[i]->loc, i );
                    snprintf( request, MAX_GET_CHAR, "%s", temp );
                }
           
           
               /* Specify axes
                **************/
               snprintf( temp, MAX_GET_CHAR, "%s&chxt=x,y,x,y&chxr=0,0,%lf|1,0,%lf&chxl=2:|Distance+(km)|3:|PGA|(cm/s/s)&chxp=2,50|3,52,48&chg=100,100,1,0", request, max_dist_pad, max_pga_pad);
               snprintf( request, MAX_GET_CHAR, "%s", temp );

               //&chco=6699CC|CC33FF|CCCC33
               //&chdl=first+legend%7Csecond+legend%7Cthird+legend

               /* End of the request
                ********************/
               snprintf( temp, MAX_GET_CHAR, "%s\"/>", request );
               snprintf( request, MAX_GET_CHAR, "%s", temp );
                fprintf(htmlfile, "%s\n<br/><br/>", request);
            } else if ( arcsm->fromArc ) {
                snprintf( temp, MAX_GET_CHAR, "<p>No sites to plot</p>" );
                snprintf( request, MAX_GET_CHAR, "%s", temp );
                fprintf(htmlfile, "%s\n<br/><br/>", request);
            }
 
        

        /* Reserve memory buffer for raw tracebuf messages
         *************************************************/
         if ( !arcsm->fromArc || (nsites > 0) ) {
            buffer = ( char* ) malloc( MAX_SAMPLES * 4 * sizeof( char ) );
            if( buffer == NULL )
            {
                logit( "et", "gmewhtmlemail: Cannot allocate buffer for traces\n" );
                // return -1; - Event if there is no memory for traces, still try to send email
            }
            else
        {
            int ix;
        
            /* Produce station traces
             ************************/
            if( Debug ) logit("ot", "Computing Traces\n" );
            
            // Save header of traces table
            st = ( time_t )starttime;
            //timeinfo = 
            gmtime_r( &st, &mytimeinfo );
            //memcpy( &mytimeinfo, timeinfo, sizeof(mytimeinfo) );
            strftime( timestr, 80, "%Y.%m.%d %H:%M:%S", &mytimeinfo ); 
            fprintf( htmlfile, "<table id=\"WaveTable\" width=\"%d;\">\n",
                    TraceWidth + 8 ); 
            fprintf( htmlfile, "<thead>\n" );
            fprintf( htmlfile, "<tr bgcolor=\"000060\"><th><font size=\"1\" face=\"Sans-serif\" color=\"FFFFFF\">Waveform%s: (StartTime: %s UTC, Duration: %d"
                    " seconds)</font></th></tr>\n", arcsm->fromArc ? "s" : "", timestr, (int) dur );
            fprintf( htmlfile, "</thead>\n" );
            fprintf( htmlfile, "</tbody>\n" );
            
            
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
            
            /* Loop over the stations or, if a triggered event, over the triggering station */
            for( ix = 0; (ix==0 && !arcsm->fromArc) || (ix < nsites); ix++ )
            {
                if ( !arcsm->fromArc ) {
                    /* Fill triggerSite/waveSite with the triggering SCNL */
                    cPtr = arcsm->sncl;
                    i = 0;
                    while (*cPtr != '.')
                        triggerSite.name[i++] = *(cPtr++);
                    triggerSite.name[i++] = 0;
                    i = 0;
                    cPtr++;
                    while (*cPtr != '.')
                        triggerSite.net[i++] = *(cPtr++);
                    triggerSite.net[i++] = 0;
                    i = 0;
                    cPtr++;
                    while (*cPtr != '.')
                        triggerSite.comp[i++] = *(cPtr++);
                    triggerSite.comp[i++] = 0;
                    i = 0;
                    cPtr++;
                    while (*cPtr)
                        triggerSite.loc[i++] = *(cPtr++);
                    triggerSite.loc[i++] = 0;
                    waveSite = &triggerSite;
                    phaseName = "T";
                } else {
                    i = site_order[ix].idx;
                    waveSite = sites[i];
                    phaseName = phases[i];
                }
                //i = site_order[ix].idx;
         
                /* Load data from waveservers
                 ****************************/
                bsamplecount = MAX_SAMPLES * 4; //Sets number of samples back
                if( Debug ) logit( "o", "Loading data from %s.%s.%s.%s\n",
                        waveSite->name, waveSite->comp,
                        waveSite->net, waveSite->loc );
                if( getWStrbf( buffer, &bsamplecount, waveSite, starttime, endtime ) == -1 )
                {
                    logit( "t", "gmewhtmlemail: Unable to retrieve data from waveserver"
                            " for trace from %s.%s.%s.%s\n",
                            waveSite->name, waveSite->comp,
                            waveSite->net, waveSite->loc );
                    continue;
                }
                
                /* Find first maximum
                 ********************
                 {
                    TRACE2_HEADER   *trace_header;       // Pointer to the header of the current trace
                    char            *trptr;             // Pointer to the current sample
                    double          inrate;             // Initial sampling rate
                    int             i;                  // General purpose counter
                    double          s, s_max;                  // Sample, max
                    int             samppos = 0, pos_max = -1;        // Position of the sample in the array;

                    trace_header = (TRACE2_HEADER*)buffer;
                    trptr = buffer;
                    while( ( trace_header = ( TRACE2_HEADER* ) trptr ) < (TRACE2_HEADER*)(buffer + bsamplecount) ) {

                        // If necessary, swap bytes in tracebuf message
                        if ( WaveMsg2MakeLocal( trace_header ) < 0 )
                        {
                            logit( "et", "gmewhtmlemail(trbufresample): WaveMsg2MakeLocal error.\n" );
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
                            if ( s > s_max || pos_max == -1) {
                                s_max = s;
                                pos_max = samppos;
                            }
                        }
                    }
                    printf("Maximum %lf @ %d\n", s_max, pos_max );
                } */
                     
                 
                /* Produce traces GIF or google chart
                 ************************************/
                if( UseGIF )
                {
                
                    /* Produce GIF image
                     *******************/
                    gif = gdImageCreate( TraceWidth, TraceHeight ); // Creates base image
                
                    // Set colors for the trace and pick
                    BACKCOLOR = gdImageColorAllocate( gif, 255, 255, 255 ); // White
                    TRACECOLOR = gdImageColorAllocate( gif, 0, 0, 96 );     // Dark blue
                    PICKCOLOR = gdImageColorAllocate( gif, 96, 0, 0 );      // Dark red
                
                
                    /* Plot trace
                     ************/
                    if( trbf2gif( buffer, bsamplecount, gif,
                            starttime, endtime, TRACECOLOR, BACKCOLOR ) == 1 )
                    {
                        if( Debug ) logit( "o", "Created gif image for trace "
                                "from %s.%s.%s.%s\n",
                                waveSite->name, waveSite->comp,
                                waveSite->net, waveSite->loc );
                    }
                    else
                    {
                        logit( "e", "gmewhtmlemail: Unable to create gif image "
                                "for %s.%s.%s.%s\n",
                                waveSite->name, waveSite->comp,
                                waveSite->net, waveSite->loc );
                        continue;
                    }

            
                    /* Plot Pick
                     ***********
                    pick2gif( gif, arrivalTimes[i], phases[i], 
                            starttime, endtime, PICKCOLOR ); */
                    
                    
                        
                        
                        /* Open gif file */
                        sprintf( giffilename, "%s_%ld_%s.%s.%s.%s_%s.gif", 
                            HTMLFile, arc->sum.qid,
                            waveSite->name, waveSite->comp,
                            waveSite->net, waveSite->loc, phaseName );
                        giffile = fopen( giffilename, "wb+" );
                    if (giffile == NULL) {
                        logit( "e", "gmewhtmlemail: Unable to create gif image "
                                "for %s.%s.%s.%s as a file: %s\n",
                                waveSite->name, waveSite->comp,
                                waveSite->net, waveSite->loc, giffilename );
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
                            gifbuffer = ( unsigned char* ) malloc( sizeof( unsigned char ) * gifbuflen );
                        
                            /* Read gif from file */
                            gifbuflen = fread( gifbuffer, sizeof( unsigned char ),
                                    gifbuflen, giffile );
                        
                            /* Encode to base64 */
                            base64buf = base64_encode( (size_t *) &base64buflen,
                                    gifbuffer, gifbuflen );
                            base64buf[base64buflen] = 0;
                        
                            /* Free gif buffer */
                            free( gifbuffer );
                        


                            sprintf( chartreq, 
                                    "<img class=\"MapClass\" alt=\"Waveform\" src=\"data:image/gif;base64,%s\">",
                                    base64buf );


                            /* Compute a content ID for this image 
                            sprintf( contentID, "%s_%s_%s_%s_%s",
                                waveSite->name, waveSite->comp,
                                waveSite->net, waveSite->loc, phases[i] ); */
                        
                            /* Write base64 attachment to gif header file 
                            fprintf( gifHeader, "--FILEBOUNDARY\n"
                                    "Content-Type: image/gif\n"
                                "Content-Disposition: inline\n"
                                "Content-Transfer-Encoding: base64\n"
                                "Content-Id: <%s>\n\n%.*s\n",
                                contentID, (int) base64buflen, base64buf ); */
                        
                        /* Free base64 buffer */
                            free( base64buf );
                        
                            /* Create img request 
                            sprintf( chartreq, 
                                    "<img class=\"MapClass\" alt=\"\" src=\"cid:%s\">",
                                    contentID ); */
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
                            waveSite->name, waveSite->comp,
                            waveSite->net, waveSite->loc );
                    if( ( giffile = fopen( giffilename, "w" ) ) != NULL )
                    {
                        if( Debug ) logit( "o", "Saving gif file: %s\n", giffilename );
                                gdImageGif( gif, giffile );
                                fclose( giffile );
                    }
                    else
                    {
                        logit( "e", "gmewhtmlemail: Unable to save gif file: %s\n", 
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
//                  basefilename = giffilename;
//#else
//                  basefilename = (char*)basename( giffilename );
//#endif
/*                  if (strlen(basefilename) > MAX_GET_CHAR)
                                        {
                        logit( "et", "Warning, basefilename too large for <img> tag, only %d chars allowed and basefilename is %d chars\n" , MAX_GET_CHAR, strlen(basefilename));
                                        }
                    //sprintf( chartreq, 
                    //      "<img class=\"MapClass\" alt=\"\" src=\"%s\">", 
                    //      basefilename );
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
                        logit( "e", "gmewhtmlemail: Error resampling samples from "
                                "%s.%s.%s.%s\n",
                                waveSite->name, waveSite->comp,
                                waveSite->net, waveSite->loc );
                        continue;
                    }
             
                    /* Produce google charts request
                     *******************************/

                    if( Debug ) logit( "o", "Creating google chart request\n" );
                    if( makeGoogleChart( chartreq, gsamples, MAX_GET_SAMP, phaseName,
                            ((arrivalTimes[i]-starttime)/
                            (endtime-starttime)*100),
                            TraceWidth, TraceHeight ) == -1 )
                    {
                        logit( "e", "gmewhtmlemail: Error generating google chart for "
                                "%s.%s.%s.%s\n",
                                waveSite->name, waveSite->comp,
                                waveSite->net, waveSite->loc );
                        continue;
                    }
                    if( Debug ) logit( "o", "Produced google chart trace for %s.%s.%s.%s\n",
                            waveSite->name, waveSite->comp,
                            waveSite->net, waveSite->loc );
                } // End of decision to make GIF or google chart traces
         
         
                /* Add to request to html file
                 *****************************/
                ot = ( time_t ) arrivalTimes[i];
                //timeinfo = 
                timefunc ( &ot, &mytimeinfo );
                //memcpy( &mytimeinfo, timeinfo, sizeof(mytimeinfo) );
                strftime( timestr, 80,"%Y.%m.%d %H:%M:%S", &mytimeinfo );
                if ( arcsm->fromArc ) {
                    fprintf(htmlfile, "<tr bgcolor=\"DDDDFF\" class=\"WaveTableTextRowClass\"><td><font size=\"1\" face=\"Sans-serif\">%s : "
                        "%3s.%2s.%2s Distance=%5.1f km",
                        waveSite->name, waveSite->comp,
                        waveSite->net, waveSite->loc, 
                        //timestr, time_type, 
                        distances[i]);
    /*              fprintf(htmlfile, "<tr bgcolor=\"DDDDFF\" class=\"WaveTableTextRowClass\"><td><font size=\"1\" face=\"Sans-serif\">%s : "
                            "%5s.%3s.%2s.%2s %s %s  CodaDur.=%d s "
                            "PickQuality %d Residual=%6.2fs Distance=%5.1f km",
                            phases[i], waveSite->name, waveSite->comp,
                            waveSite->net, waveSite->loc, timestr,
                            time_type, coda_lengths[i], weights[i], residual[i], distances[i]); */
            
                    // Include coda magnitudes, if available
                    if( coda_mags[i] != 0.0 && DontShowMd == 0)
                    {
                        fprintf( htmlfile, " <b>Md=%3.1f wt=%d</b>", coda_mags[i], coda_weights[i] );
                    }
            
                    // Create the table row with the chart request
                    // Included font-size mandatory style for composite images
                    fprintf( htmlfile, "</font></td></tr>\n" );
                }
                fprintf( htmlfile, "<tr class=\"WaveTableTraceRowClass\" style=\"font-size:0px;\"><td><font size=\"1\" face=\"Sans-serif\">%s</font></td></tr>\n", 
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
            fprintf( htmlfile, "</tbody>\n" );
            fprintf(htmlfile, "</table><br/><br/>\n");
        
        } // End of Traces section
        }

        /* Footer for html file
         ******************/
        if ( DisclaimerText )
            fprintf(htmlfile,"<hr><div>\n%s\n</div><hr>\n",DisclaimerText);
        fprintf(htmlfile, "<p id=\"Footer\"><font size=\"1\" face=\"Sans-serif\">This report brought to you by Earthworm with gmewhtmlemail (version %s)</font></p>", VERSION_STR);
        
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
            logit( "ot", "gmewhtmlemail: processing  email alerts.\n" );
            for( i=0; i<nemailrecipients; i++ )// One email for each recipient
            {
                // Skip recipent if not on mailing list for this type of event
                if ( (arcsm->fromArc && !emailrecipients[i].sendOnEQ) ||
                    (!arcsm->fromArc && !emailrecipients[i].sendOnSM) ) 
                    continue;
                    
                if (1) /*if ( (emailrecipients[i].min_magnitude <= arc->sum.Mpref && DontUseMd == 0) &&
                (distance_from_center <= emailrecipients[i].max_distance) )*/
                {
                if( UseBlat )       // Use blat for sending email
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
                else                // Use sendmail
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
                    if ( arcsm->fromArc )
                        fprintf( header_file, "Subject: %s - EW Event ID: %ld\n", 
                                SubjectPrefix, arc->sum.qid );
                    else
                        fprintf( header_file, "Subject: %s - Trigger from %s\n", 
                                SubjectPrefix, arcsm->sncl );
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
                //printf( "Email command: '%s'\n", system_command );
                system(system_command);
                logit("et", "gmewhtmlemail: email sent to %s, passed tests\n", emailrecipients[i].address);
                if (DebugEmail) {
                    logit("et", "gmewhtmlemail: Debug; EmailCommand issued '%s'\n", system_command);
                }
                }
                else
                {
                /* this person doesn't get the email, Both Local (if used) or  Coda mag too low */
                if (emailrecipients[i].max_distance != OTHER_WORLD_DISTANCE_KM && 
                    emailrecipients[i].max_distance > distance_from_center) 
                { 
                    logit("et", "gmewhtmlemail: No email sent to %s because distance of event %5.1f lower than threshold %5.1f\n",
                    emailrecipients[i].address, distance_from_center, emailrecipients[i].max_distance);
                }
                }
            }
        }
        else
        {
            if (Quality>=MinQuality)
                logit("et", "gmewhtmlemail: No emails sent, quality %c is below MinQuailty %c.\n",
                    Quality, MinQuality);
        }
    }
    else
    {
        logit("et", "gmewhtmlemail: Unable to write html file: %s\n", fullFilename);
        rv = FALSE;
    }
    logit("et", "gmewhtmlemail: Completed processing of event id: %ld\n", arc->sum.qid);
    return rv;
}



/*******************************************************************************
 * MapRequest: Produce a google map request for a given set of stations        *
 *                   and a hypocenter                                          *
 ******************************************************************************/
void MapRequest(char *request, SITE **sites, int nsites,
    double hypLat, double hypLon, HypoArcSM2 *arcsm, 
   SiteOrder *site_order, SiteOrder *snl_order ) {
    if ( strlen(MapQuestKey) > 0 )
        MapQuestMapRequest( request, sites, nsites, hypLat, hypLon, arcsm, site_order, snl_order );
    else
        GoogleMapRequest( request, sites, nsites, hypLat, hypLon, arcsm, site_order, snl_order );
}

/*******************************************************************************
 * GoogleMapRequest: Produce a google map request for a given set of stations  *
 *                   and a hypocenter                                          *
 ******************************************************************************/
void GoogleMapRequest(char *request, SITE **sites, int nsites,
   double hypLat, double hypLon, HypoArcSM2 *arcsm, 
   SiteOrder *site_order, SiteOrder *snl_order )
{
   int i;
   char temp[MAX_GET_CHAR];

    /* Base of the google static map
    *******************************/
   snprintf(request, MAX_GET_CHAR, "<img class=\"MapClass\" alt=\"\" "
         "src=\"https://maps.googleapis.com/maps/api/staticmap?"
         "size=600x400&format=png8&maptype=%s&sensor=false", StaticMapType); 
    
    /* Icon for hypocenter 
    *********************/
   if (hypLat!=0.0 || hypLon!=0.0)
    {
      snprintf( temp, MAX_GET_CHAR, "%s&markers=icon:http:%%2F%%2Fmaps.google.com%%2Fmapfiles"
            "%%2Fkml%%2Fpaddle%%2Fgrn-stars-lv.png%%7Cshadow:true%%7C%f,%f", 
            request, hypLat, hypLon);
      snprintf( request, MAX_GET_CHAR, "%s", temp );
    }

    /* Add icons for stations
    ************************/
   snprintf( temp, MAX_GET_CHAR, "%s&markers=color:green%%7Cshadow:false", request );
   snprintf( request, MAX_GET_CHAR, "%s", temp );
   for( i = 0; i < nsites; i++ )
    {
      snprintf( temp, MAX_GET_CHAR, "%s%%7C%f,%f", request, sites[i]->lat, sites[i]->lon );
      snprintf( request, MAX_GET_CHAR, "%s", temp );
    }
    
    /* Add icons for facilities
    **************************/
   for ( i=0; i<dam_close_count && i<26; i++ )
    {
//       snprintf( temp, MAX_GET_CHAR, "%s&markers=color:blue%%7Clabel:%d%%7Cshadow:false%%7C%f,%f", 
//            request, i+1, dam_tbl[i].lat, dam_tbl[i].lon);
      snprintf( temp, MAX_GET_CHAR, "%s&markers=icon:http:%%2F%%2Fmaps.google.com%%2Fmapfiles"
            "%%2Fkml%%2Fpaddle%%2F%c.png%%7Cshadow:true%%7C%f,%f", 
            request, i+'A', dam_tbl[i].lat, dam_tbl[i].lon);
      snprintf( request, MAX_GET_CHAR, "%s", temp );
    }
/*
   if ( i < dam_close_count ) {
       snprintf( temp, MAX_GET_CHAR, "%s&markers=color:blue%%7Cshadow:false", request );
       snprintf( request, MAX_GET_CHAR, "%s", temp );
       for ( ; i<dam_close_count; i++ ) {
          snprintf( temp, MAX_GET_CHAR, "%s%%7C%f,%f", request, dam_tbl[i].lat, dam_tbl[i].lon );
          snprintf( request, MAX_GET_CHAR, "%s", temp );
        }
    } */

   for ( ; i<dam_close_count; i++ )
    {
      snprintf( temp, MAX_GET_CHAR, "%s&markers=icon:http:%%2F%%2Fmaps.google.com%%2Fmapfiles"
            "%%2Fkml%%2Fpaddle%%2Fred-circle.png%%7Cshadow:true%%7C%f,%f", 
            request, dam_tbl[i].lat, dam_tbl[i].lon);
      snprintf( request, MAX_GET_CHAR, "%s", temp );
    }
    /* End of the request
    ********************/
   snprintf( temp, MAX_GET_CHAR, "%s\"/>", request );
   snprintf( request, MAX_GET_CHAR, "%s", temp );
    
}


/*******************************************************************************
 * MapQuestMapRequest: Produce a MapQuest map request for a given set of       *
 *                   stations and a hypocenter                                 *
 ******************************************************************************/
void MapQuestMapRequest(char *request, SITE **sites, int nsites,
   double hypLat, double hypLon, HypoArcSM2 *arcsm, 
   SiteOrder *site_order, SiteOrder *snl_order )
{
   int i;

    /* Base of the google static map
    *******************************/
   snprintf(request, MAX_GET_CHAR, "<img alt=\"Center/Station/Facility Map\" "
         "src=\"http://www.mapquestapi.com/staticmap/v4/getmap?key=%s&declutter=true&"
         "size=600,400&imagetype=png&type=%s", MapQuestKey, MQStaticMapType); 
   request += strlen( request );
    
    /* Icon for hypocenter 
    *********************/
   if (hypLat!=0.0 || hypLon!=0.0)
    {
      //snprintf( request, MAX_GET_CHAR, "yellow_1,%f,%f|", 
      snprintf( request, MAX_GET_CHAR, "&xis=http://love.isti.com/~scott/LASSO/hypocenter.png,1,C,%f,%f",
            hypLat, hypLon);
      request += strlen( request );
    }
    
    snprintf( request, MAX_GET_CHAR, "&pois=" );
    request += strlen( request );

    /* Add icons for stations
    ************************/

   int snl_i = 0;
   for( i = 0; i < nsites; i++ )
    {
      int ix = site_order[i].idx;
      
      if ( snl_order[snl_i].idx == ix ) {
        snprintf( request, MAX_GET_CHAR, "%spink_1-%d,%f,%f|", request, snl_i+1, sites[i]->lat, sites[i]->lon );
        request += strlen( request );
        snl_i++;
      }
    }
    
    /* Add icons for facilities
    **************************/
   for ( i=0; i<dam_close_count; i++ )
    {
      snprintf( request, MAX_GET_CHAR, "%s%s_1-%d,%f,%f|", 
            request, dam_tbl[i].station[0] ? "blue" : "red",
            i+1, dam_tbl[i].lat, dam_tbl[i].lon);
      request += strlen( request );
    }
    /* End of the map
    ********************/
    snprintf( request, MAX_GET_CHAR, "%s\"/>\n", request );
    request += strlen( request );

    /* Add the legend
     ****************/
    snprintf( request, MAX_GET_CHAR, "<table border='1' cellspacing='1' cellpadding='3' width='600px'>"
        "<tr>"
        "<td><img src='http://love.isti.com/~scott/LASSO/hypocenter.png'> Event</td>"
        "<td><img src='http://www.mapquestapi.com/staticmap/geticon?uri=poi-red_1.png'> Uninstrumented Facility</td>"
        "<td><img src='http://www.mapquestapi.com/staticmap/geticon?uri=poi-blue_1.png'> Instrumented Facility</td>"
        "<td><img src='http://www.mapquestapi.com/staticmap/geticon?uri=poi-pink_1.png'> Station</td></tr>"
        "</table>" ); 
   request += strlen( request );
    
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
    double avg, max=0;
    char reqHeader[MAX_GET_CHAR];
    char reqSamples[MAX_GET_CHAR];
    int cursample;              /* Position of the current sample */
    int cursampcount;           /* Number of samples in the current image */
    int curimgwidth;            /* Width of the current image */
    int accimgwidth;            /* Accumulated width of the composite image */
    int ntraces;                /* Number of traces that will be required */
    double cursamppos, nxtsamppos;

    
    // Average value of the samples
    avg = 0;
    for( i = 0; i < samplecount; i++ ) {
        if ( i==0 || max < samples[i] )
            max = samples[i];
        avg += samples[i];
    }
    avg /= ( double ) samplecount;
    printf("Max = %lf\n", max );

    
    /* Instead of sending a single image, the trace will be split
     * in multiple images, each with its separate request and a 
     * maximum of MAX_REQ_SAMPLES samples
     ************************************************************/

    ntraces = ( ( int )( samplecount %  MAX_REQ_SAMP ) == 0 )?
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
        accimgwidth += curimgwidth;     // update accumulated image width


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
                    "chls=1&"               // Linewidth
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
                    "chco=000060,006000&"
                    "chma=0,0,0,0|0,006000,1,0&"
                    "chls=1|1&"               // Linewidth
                    "chd=s:",
                    curimgwidth + 4, traceheight );
        }

        /* Create samples
         ****************/
        for( i = 0; i < MAX_GET_CHAR; i++ ) reqSamples[i] = 0;
        for( i = cursample; i < ( cursample + cursampcount ); i++ )
            if ( showMax ) //samples[i] == max )
                reqSamples[i - cursample] = '_';
            else
                reqSamples[i - cursample] = simpleEncode( ( int )( ( double ) samples[i] - avg + 30.5 ) );
        int oldCurrSample = cursample;
        cursample += cursampcount;

        /* Add image and samples to final request
         ****************************************/
        strncat( chartreq, reqHeader, MAX_GET_CHAR );
        strncat( chartreq, reqSamples, MAX_GET_CHAR );
        
        if ( showMax ) {
            for( i = oldCurrSample; i < (oldCurrSample + cursampcount); i++ )
                if ( samples[i] != max )
                    reqSamples[i - oldCurrSample] = '_';
                else {
                    reqSamples[i - oldCurrSample] = simpleEncode( ( int )( ( double ) samples[i] - avg + 30.5 ) );
                    printf("Found a max!!!!\n");
                }
            strncat( chartreq, ",", MAX_GET_CHAR );
            strncat( chartreq, reqSamples, MAX_GET_CHAR );
        }

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
            "chls=1&"               // Linewidth
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
    WS_MENU_QUEUE_REC   menu_queue;
    TRACE_REQ           trace_req;
    int                 wsResponse;
    int                 atLeastOne;
    int                 i;
    char                WSErrorMsg[80];
    
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
            logit( "et", "gmewhtmlemail: Cannot contact waveserver %s:%s - ",
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
        logit( "et", "gmewhtmlemail: Unable to contact any waveserver.\n");
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
        logit( "et", "gmewhtmlemail: Error loading data from waveserver - %s\n",
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
    TRACE2_HEADER   *trace_header,*trace;       // Pointer to the header of the current trace
    char            *trptr;             // Pointer to the current sample
    double          inrate;             // Initial sampling rate
    double          outrate;            // Output rate
    int             i;                  // General purpose counter
    double          s;                  // Sample
    double          *fbuffer;           // Filter buffer
    double          nfbuffer;           // length of the filter buffer
    double          *outsig;            // Temporary array for output signal
    double          outavg;             // Compute average of output signal
    double          outmax;             // Compute maximum of output signal
    double          *samples;           // Array of samples to process
    int             nsamples = 0;       // Number of samples to process
    int             samppos = 0;        // Position of the sample in the array;
    double          sr;                     // buffer sample rate 
    int             isBroadband;        // Broadband=TRUE - ShortPeriod = FALSE
    int             flag;       
    double          y;              // temp int to hold the filtered sample 
    RECURSIVE_FILTER rfilter;     // recursive filter structure
    int             maxfilters=20;

    
    /* Reserve memory for temporary output signal
     ********************************************/
    outsig = ( double* ) malloc( sizeof( double ) * noutsig );
    if( outsig == NULL )
    {
        logit( "et", "gmewhtmlemail: Unable to reserve memory for resampled signal\n" );
        return -1;
    }
    
    
    /* Reserve memory for input samples
     **********************************/
    // RSL note: tried the serialized option but this is better
    samples = ( double* ) malloc( sizeof( double ) * MAX_SAMPLES );
    if( samples == NULL )
    {
        logit( "et", "gmewhtmlemail: Unable to allocate memory for sample buffer\n" );
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
            logit( "et", "gmewhtmlemail(trbufresample): WaveMsg2MakeLocal error.\n" );
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
            logit("et","gmewhtmlemail: initAllFilters() cannot allocate Filters; exiting.\n");
        }
        if( (flag=initTransferFn()) != EW_SUCCESS)
        {
            logit("et","gmewhtmlemail: initTransferFn() Could not allocate filterTF.\n");
        }
        if(flag==EW_SUCCESS)
        {
            switch (flag=initChannelFilter(sr, outavg/(double)nsamples,isBroadband,&rfilter, maxfilters))
            {
                 case  EW_SUCCESS:
                 
                        if (Debug)
                        logit("et","gmewhtmlemail: Pre-filter ready for channel %s:%s:%s:%s\n",
                                trace->sta,trace->chan,trace->net,trace->loc);
                        break;

                  case EW_WARNING:

                        logit("et","Unable to obtain a find an existing pre-filter or create a new (%s) pre-filter for sample rate %f; setting channel %s:%s:%s:%s bad\n",
                                (isBroadband ? "broadband" : "narrowband"),trace->samprate,
                                trace->sta,trace->chan,trace->net,trace->loc);
                        break;

                  case EW_FAILURE:
                  
                        logit("et",  "gmewhtmlemail Parameter passed in NULL; setting channel %s:%s:%s:%s bad\n",
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
            logit( "et", "gmewhtmlemail: Unable to allocate memory for filter buffer\n" );
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
    TRACE2_HEADER   *trace_header,*trace;   // Pointer to the header of the current trace
    char            *trptr;         // Pointer to the current sample
    double          sampavg;        // To compute average value of the samples
    int         nsampavg=0;     // Number of samples considered for the average
    double          sampmax;        // To compute maximum value
    int             i;              // General purpose counter
    gdPointPtr      pol;            // Polygon pointer
    int             npol;           // Number of samples in the polygon
    double          xscale;         // X-Axis scaling factor
    double          dt;             // Time interval between two consecutive samples
        double      sr;             // to hold the sample rate obtained through the trace_header struc.                        
    int             isBroadband;    // Broadband= TRUE - ShortPeriod = FALSE
    int             flag;       
    int             y;              // temp int to hold the filtered sample 
    RECURSIVE_FILTER rfilter;     // recursive filter structure
    int             maxfilters=20;      //Number of filters fixed to 20.
    
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
            logit("et","gmewhtmlemail: initAllFilters() cannot allocate Filters; exiting.\n");
        }
        if( (flag=initTransferFn()) != EW_SUCCESS)
        {
            logit("et","gmewhtmlemail: initTransferFn() Could not allocate filterTF.\n");
        }
        if(flag==EW_SUCCESS)
        {
            switch (flag=initChannelFilter(sr, sampavg/(double)nsampavg, isBroadband, &rfilter, maxfilters))
            {
                  case  EW_SUCCESS:
                 
                        if (Debug)
                        logit("et","gmewhtmlemail: Pre-filter ready for channel %s:%s:%s:%s\n",
                                trace->sta,trace->chan,trace->net,trace->loc);
                        break;

                  case EW_WARNING:

                        printf("Unable to obtain a find an existing pre-filter or create a new (%s) pre-filter for sample rate %f; setting channel %s:%s:%s:%s bad\n",
                                (isBroadband ? "broadband" : "narrowband"),trace->samprate,
                                trace->sta,trace->chan,trace->net,trace->loc);
                        break;

                  case EW_FAILURE:
                  
                        logit("et",  "gmewhtmlemail Parameter passed in NULL; setting channel %s:%s:%s:%s bad\n",
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
        //      ( int )( ( ( double ) pol[i].y - sampavg ) / sampmax * ( double ) gif->sy / 2 );
        
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
static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static int  mod_table[] = {0, 2, 1};


unsigned char* base64_encode( size_t* output_length,
        const unsigned char *data, size_t input_length )
{
    size_t i;
    int j;
#ifdef _WINNT
    typedef unsigned __int32 uint32_t;
#endif
    uint32_t    octet_a, octet_b, octet_c, triple;
    unsigned char* encoded_data;
    
    *output_length = 4 * ((input_length + 2) / 3);
    //*output_length = ( ( input_length - 1 ) / 3 ) * 4 + 4;
    
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


/*******************************************************************************
 * Yield distance between (lat1,lon1) and(lat2,lon2) in specified units:       *
 * (M)iles, (K)ilometers, or (N)?
 ******************************************************************************/
#define pi 3.14159265358979323846
double distance(double lat1, double lon1, double lat2, double lon2, char unit) {
  double theta, dist;
  theta = lon1 - lon2;
  dist = sin(deg2rad(lat1)) * sin(deg2rad(lat2)) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(theta));
  dist = acos(dist);
  dist = rad2deg(dist);
  dist = dist * 60 * 1.1515;
  switch(unit) {
    case 'M':
      break;
    case 'K':
      dist = dist * 1.609344;
      break;
    case 'N':
      dist = dist * 0.8684;
      break;
  }
  return (dist);
}

/********************************************************************
 *  This function converts decimal degrees to radians               *
 *******************************************************************/
double deg2rad(double deg) {
  return (deg * pi / 180);
}

/********************************************************************
 *  This function converts radians to decimal degrees               *
 *******************************************************************/
double rad2deg(double rad) {
  return (rad * 180 / pi);
}


/*******************************************************************************
 * Read disclaimer text from specified file, store in DisclaimerText           *
 ******************************************************************************/
void read_disclaimer( char* path ) {
    FILE *fp = fopen( path, "r" );
    long filesize, numread;
    if ( fp == NULL ) {
        logit( "et", "Could not open disclaimer file: %s\n", path );
        return;
    }
    fseek( fp, 0L, SEEK_END );
    filesize = ftell( fp );
    rewind( fp );
    
    DisclaimerText = malloc( filesize+5 );
    if ( DisclaimerText == NULL ) {
        logit( "et", "Could no allocate space for disclaimer\n" );
        fclose( fp );
        return;
    }
    
    numread = fread( DisclaimerText, filesize, 1, fp );
    if ( numread != 1 ) {
        logit( "et", "Could not read disclaimer from file: %s\n", path );
        free( DisclaimerText );
    }
    fclose( fp );
}
