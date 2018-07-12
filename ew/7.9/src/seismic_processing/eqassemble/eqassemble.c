/*
 *    $Id: eqassemble.c 6803 2016-09-09 06:06:39Z et $
 */

/*
 * eqassemble.c: Assemble EVENT2K messages for earthquakes using the
 * 	         output of binder_ew (QUAKE2K and LINK messages) and 
 *               pick_ew (PICKSCNL and CODASCNL messages).
 *               This module is the result of merging eqprelim, 
 *               eqrapid (Caltech variant of eqprelim) and eqproc.
 * Pete Lombard, UC Berkeley 2006/08/10
 * V0.1.3 2012-07-10 - added in UseS option (only P phases used in rule counts otherwise)
 * V0.1.4 2012-10-09 - made UseS a boolean flag. Its presence now indicates S should be used in count
 * V0.1.5 2014-07-10 - moved check for Heartbeat and HYP inside packet processing loop
 * V0.1.6 2015-05-07 - paulf checked for return of eqas_link() to see if link was found in fifo, added Debug config
 * V0.1.7 2015-05-08 - paulf added tracking of phases found for links versus used
 * V0.1.8 2015-05-11 - paulf fixed an initialization error for actualPhases member of eqk hypo struct
 * V0.1.9 2015-05-12 - paulf modified actualPhases member of eqk hypo struct to use nph from first quake2k since
			binder_ew releases links BEFORE first quake2k message
 * v0.1.10 2016-01-05 - PL added spaces in two logit formats to keep the numbers from being smooshed together.
 * v0.1.11 2016-06-03 - ET Added casts to match parameters to 'sprintf' format strings (when 64 bit);
 *                        minor mods to squelch warnings.
 */

#define VERSION "0.1.11 2016-06-03"

#define X(lon) (facLon * ((lon) - orgLon))
#define Y(lat) (facLat * ((lat) - orgLat))
#define ew_hypot(x,y) (sqrt((x)*(x) + (y)*(y)))
#ifdef _WINNT
#define strcasecmp _stricmp 
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <chron3.h>
#include <earthworm.h>
#include <kom.h>
#include <rdpickcoda.h>
#include <site.h>
#include <tlay.h>
#include <transport.h>

static SHM_INFO  Region;          /* shared memory region to use for input  */

#define   MAXLOGO   10
MSG_LOGO  GetLogo[MAXLOGO];       /* array for requesting module,type,instid */
short     nLogo;

#define MAXSTRLEN             256
static char  Text[MAXSTRLEN];         /* string for log/error messages       */
static char *Arg0;                    /* pointer to my name                  */

enum EqStatus {eqKilled, eqVirgin, eqUnreported, eqPrelim, eqRapid, eqFinal};

#define MAX_CODA_WAIT 150;  /* number of seconds to wait for codas */
#define MAXHYP 100     /* default quake fifo length */
typedef struct {
    double  tRpt;     /* Event due for final report at this time. */
    double  tOrigin;  /* Event origin time */
    double  tDetect;  /* Time of receipt of furst quake2k message */
    double  tCodaWait;/* Time when codas due for maxPhasesPerEq event */
    double  lat;
    double  lon;
    double  z;
    long    id;       /* event ID assigned by binder */
    float   rms;      /* RMS traveltime residual */
    float   dmin;     /* Distance to nearest station */
    float   ravg;     /* Average epicentral distance of associated arrivals */
    float   gap;      /* Maximum azimuthal gap */
    int     nph;      /* Number of associated phases, this comes from quake2k message */
    int     actualPhases; /* number of assoc phases seen from LINK messages */
    int     maxPhasesForThisEq;
    enum EqStatus eqStatus;   
} RPT_HYP;
RPT_HYP *Hyp; /* alloc to length=maxHyp */

#define MAXPCK 1000   /* default pick fifo length */
long nPck = 0;
typedef struct {
    double          tArr;     /* phase arrival time */
    long            quake;
    int             id;
    unsigned char   src;
    unsigned char   instid;
    int             site;
    char            phase;
    char            ie;
    char            fm;
    char            wt;
    long            pamp[3];
    long            caav[6];
    short           clen;
    time_t          timeout;   /* end of time to wait for coda */
} RPT_PCK;
RPT_PCK *Pck; /* alloc to length=maxPck */

int nSrt;
typedef struct {
    double  tArr;
    int     ip;
} SRT;
SRT *Srt; /* alloc to length=maxPck */

/* Things to read from configuration file
****************************************/
static char RingName[MAX_RING_STR];  /* transport ring to read from         */
static char MyModName[MAX_MOD_STR];  /* speak as this module name/id        */
static int  LogSwitch;            /* 0 if no logging should be done to disk */
static int  ReportS;              /* 0 means don't send S to next process   */
static int  UseS = 0;             /* 0 means don't use S in phase count (default) */
static char NextProc[150];        /* actual command to start next program   */
static char DataSrc[2];           /* Source of data for picks               */
static int  Debug = 0;            /* 0 means don't log debug messages, for now a flag(default is off) */

/* Things to look up in the earthworm.h tables with getutil.c functions
**********************************************************************/
static long          RingKey;       /* key of ring for input         */
static unsigned char InstId;        /* local installation id         */
static unsigned char MyModId;       /* Module Id for this program    */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypeQuake2K;
static unsigned char TypeLink;
static unsigned char TypePickSCNL;
static unsigned char TypeCodaSCNL;
static unsigned char TypeEventSCNL;
static unsigned char TypeKill;
static unsigned char TypeCancelEvent;
static unsigned char CodaInst[MAXLOGO]; /* Inst ids that contribute codas */
int nCodaInst = 0;

static char  EqMsg[MAX_BYTES_PER_EQ];    /* char string to hold eventscnl message */

static double orgLat;  
static double orgLon;
static double facLat;
static double facLon;

size_t maxHyp = MAXHYP;         /* Quake FIFO length                    */
size_t maxPck = MAXPCK;         /* Pick  FIFO length                    */

int HypCheckInterval = 10;      /* Time Interval (s) to check quakes    */
int HeartbeatInt = 0;           /* Heartbeat interval (s)               */

int doPrelim = 0;               /* 1 = do the prelim rule               */
int prelimMinPhases = 0;        /* number of phases required for report */

int doRapid = 0;                /* 1 = do the rapid rule                */
int rapidDelay = 0;             /* seconds to wait before report        */
int rapidMinPhases = 0;		/* number of phases required for report */
enum Since {SinceOrigin, SinceDetection, SinceStable};
enum Since rapidDelaySince = SinceDetection; /* when rapid delay starts */

int doFinal = 0;                /* 1 = do the final rule                */
int finalDelay = 0;             /* seconds to wait before report        */
int finalMinPhases = 0;		/* number of phases required for report */
int finalIncludeCodas = 0;      /* Whether to wait for codas and include*
				 * them in the event message            */
double waifTolerance = 4.0;	/* Picks within this residual but not   *
				 * associated with any events are waifs */
enum Since finalDelaySince = SinceStable; /* when final delay starts    */
int maxPhasesPerEq = MAX_PHS_PER_EQ;
				/* maximum number of phases allowed in  *
				 * hypoinverse message.                 */

enum Version {vPrelim, vRapid, vFinal};
pid_t  MyPID=0;

/* Error messages used by eqassemble:
*******************************/
#define  ERR_MISSMSG            0
#define  ERR_TOOBIG             1
#define  ERR_NOTRACK            2
#define  ERR_PICKREAD           3
#define  ERR_CODAREAD           4
#define  ERR_QUAKEREAD          5
#define  ERR_LINKREAD           6
#define  ERR_UNKNOWNSTA         7
#define  ERR_TIMEDECODE         8
#define  ERR_FIFOLAP            9

/* Functions in this source file
*******************************/
void  eqas_config ( char * );
void  eqas_lookup ( void   );
int   eqas_pickscnl ( char * );
int   eqas_codascnl ( char * );
int   eqas_link   ( char * );
int   eqas_quake2k( char * );
int   eqas_compare( const void *, const void * );
void  eqas_earlyRules( int );
void  eqas_delayedRules  ( void );
void  eqas_status ( unsigned char, short, char * );
char *eqas_phscard( int, char *, int );
char *eqas_hypcard( int, char *, enum Version );
void  eqas_cancelevent( int );
void  eqas_assembleIfReady( int, enum EqStatus );

int main( int argc, char **argv )
{
    double     t;
    double     tcheck        = 0.0;
    double     NextHeartbeat = 0.0;
    char       rec[MAXSTRLEN];   /* actual retrieved message  */
    long       recsize;          /* size of retrieved message */
    MSG_LOGO   reclogo;          /* logo of retrieved message */
    int        res;
    int        iq;
    int        is;

    Arg0 = argv[0];
    strcpy(DataSrc, " ");

    /* Check command line arguments
     ******************************/
    if ( argc != 2 ) {
	fprintf( stderr, "Usage: %s <configfile>\n", Arg0 );
        fprintf( stderr, "%s version %s\n", Arg0, VERSION );
	exit( 0 );
    }

    /* Initialize name of log-file & open it
     ***************************************/
    logit_init( argv[1], 0, MAXSTRLEN*2, 1 );

    /* Read the configuration file(s)
     ********************************/
    eqas_config( argv[1] );
    logit("", "%s version %s\n", Arg0, VERSION);
    logit( "" , "%s: Read command file <%s>\n", Arg0, argv[1] );

    /* Look up important info from earthworm.h tables
     ************************************************/
    eqas_lookup();

    /* Store my own processid
     ************************/
    MyPID = getpid();

    /* Reinitialize logit to desired logging level
     *********************************************/
    logit_init( argv[1], 0, MAXSTRLEN*2, LogSwitch );

    /* Allocate space for Quake FIFO, Pick FIFO, sorting array 
     *********************************************************/
    Hyp = (RPT_HYP *) calloc( maxHyp, sizeof( RPT_HYP ) );
    if( Hyp == (RPT_HYP *) NULL ) {
	logit( "et","%s: Error allocating quake FIFO at length=%ld; "
	       "exiting!\n", Arg0, maxHyp );
	exit( -1 );
    }
    logit( "", "eqproc: Allocated quake fifo (length=%ld)\n", maxHyp );

    Pck = (RPT_PCK *) calloc( maxPck, sizeof( RPT_PCK ) );
    if( Pck == (RPT_PCK *) NULL ) {
	logit( "et","%s: Error allocating pick FIFO at length=%ld; "
	       "exiting!\n", Arg0, maxPck );
	free( Hyp );
	exit( -1 );
    }
    logit( "", "%s: Allocated pick fifo (length=%ld)\n", Arg0, maxPck );

    Srt = (SRT *) calloc( maxPck, sizeof( SRT ) );
    if( Srt == (SRT *) NULL ) {
	logit( "et","%s: Error allocating sort array at length=%ld; "
	       "exiting!\n", Arg0, maxPck );
	free( Hyp );
	free( Pck );
	exit( -1 );
    }

    /* Initializate some variables
     *****************************/
    nPck = 0;                    	/* no picks have been processed    */
    for(iq=0; iq<maxHyp; iq++) {	
        Hyp[iq].id  = 0;		/* set all hypocenter id's to zero */
	Hyp[iq].eqStatus = eqVirgin;  	/* no data in Hyp structure */
	Hyp[iq].actualPhases = 0; 	
    }

    /* Attach to PICK shared memory ring & flush out all old messages
     ****************************************************************/
    tport_attach( &Region, RingKey );
    logit( "", "eqproc: Attached to public memory region %s: %d\n",
	   RingName, RingKey );
    while( tport_getmsg( &Region, GetLogo, nLogo,
			 &reclogo, &recsize, rec, MAXSTRLEN ) != GET_NONE );

    /* Start the next processing program & open pipe to it
     *****************************************************/
    if( pipe_init( NextProc, 0 ) < 0 ) {
        logit( "et",
               "%s: Error opening pipe to %s; exiting!\n", Arg0, NextProc);
        tport_detach( &Region );
        free( Hyp );
        free( Pck );
        free( Srt );
        exit( -1 );
    }
    logit( "e", "%s: piping output to <%s>\n", Arg0, NextProc );

    /* Calculate network origin
     **************************/
    logit( "", "%s: nSite = %d\n", Arg0, nSite );
    orgLat = 0.0;
    orgLon = 0.0;
    for(is=0; is<nSite; is++) {
	orgLat += Site[is].lat;
	orgLon += Site[is].lon;
    }
    orgLat /= nSite;
    orgLon /= nSite;
    facLat = (double)(40000.0 / 360.0);
    facLon = facLat * cos(6.283185 * orgLat / 360.0);

    /*------------------- setup done; start main loop -------------------------*/

    while(1) {
       
	t = tnow();
	if(t < tcheck) {
	    sleep_ew(1000);
	}

       
	/* Process all new hypocenter, pick-time, pick-coda, & link messages
         *******************************************************************/
	do {
	    fflush( stdout );

	    t = tnow();
	    /* Send heartbeat if it's time
             *****************************/
	    if( t >= NextHeartbeat ) {
	        eqas_status( TypeHeartBeat, 0, "" );
	        NextHeartbeat = t + (double) HeartbeatInt;
	    }
	    /* Check the hypocenters for reporting if it's time
            **************************************************/
	    if( t >= tcheck ) {
	        eqas_delayedRules();  // check time-based rules for reporting
	        tcheck = t + (double) HypCheckInterval;
	    }

	    /* see if a termination has been requested
             *****************************************/
	    if ( pipe_error() || tport_getflag( &Region ) == TERMINATE  ||
		 tport_getflag( &Region ) == MyPID ) {
		/* send a kill message down the pipe */
		pipe_put( "", TypeKill );
		logit( "t","%s: sent TYPE_KILL msg to pipe.\n", Arg0);
		/* detach from shared memory */
		tport_detach( &Region );
		/* Free allocated memory */
		free( Hyp );
		free( Pck );
		free( Srt );
		/* write a few more things to log file and close it */
		if ( pipe_error() )
		    logit( "t", "%s: Output pipe error; exiting\n", Arg0 );
		else
		    logit( "t", "%s: Termination requested; exiting\n", Arg0 );
		sleep_ew( 500 );  /* give time for msg to get through pipe */
		/* close down communication to child */
		pipe_close();
		fflush( stdout );
		return 0;
	    }

	    /* Get & process the next message from shared memory
             ***************************************************/
	    res = tport_getmsg( &Region, GetLogo, nLogo,
				&reclogo, &recsize, rec, MAXSTRLEN-1 );
	    switch(res) {
            case GET_OK:      /* got a message just fine    */
                break;
	    case GET_MISS:    /* got a message; missed some */
		sprintf( Text,
			 "Missed msg(s)  i%u m%u t%u  region:%ld.",
			 reclogo.instid, reclogo.mod, reclogo.type, Region.key);
		eqas_status( TypeError, ERR_MISSMSG, Text );
		break;
	    case GET_NOTRACK: /* got a message; can't tell is any were missed */
		sprintf( Text,
			 "Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
			 reclogo.instid, reclogo.mod, reclogo.type );
		eqas_status( TypeError, ERR_NOTRACK, Text );
		break;
	    case GET_TOOBIG:  /* no message returned; it was too big */
                sprintf(Text,
                        "Retrieved msg[%ld] (i%u m%u t%u) too big for rec[%d]",
                        recsize, reclogo.instid, reclogo.mod, reclogo.type,
                        MAXSTRLEN-1 );
                eqas_status( TypeError, ERR_TOOBIG, Text );
		continue;
	    case GET_NONE:   /* no messages to process */
                continue;
	    }

            /* Process the message (GET_MISS, GETNOTRACK, GET_OK returns)
             ***********************************************************/
            rec[recsize] = '\0';      /*null terminate the message*/
            if (reclogo.type == TypePickSCNL) {
                eqas_pickscnl( rec );
            } else if ( reclogo.type == TypeCodaSCNL) {
                eqas_codascnl( rec );
            } else if (reclogo.type == TypeQuake2K) {
                eqas_quake2k( rec );
            } else if (reclogo.type == TypeLink) {
                if ( !eqas_link( rec ) ) {
		    logit( "t", "Warning TYPE_LINK pick_id not found in fifo, potential fifo lapping: link message=%s\n",  rec );
		}
            }   

        } while( res != GET_NONE );  /*end of message-processing-loop */
    }
    /*----------------------------end of main loop-------------------------------*/
}

/****************************************************************************/
/*  eqas_quake2k() processes a TYPE_QUAKE2K message from binder              */
/****************************************************************************/
int eqas_quake2k( char *msg )
{
    char       cdate[24];
    char       corg[24];
    char       timestr[20];
    double     tOrigin, tNow;
    double     lat, lon, z;
    float      rms, dmin, ravg, gap;
    long       qkseq;
    int        nph;
    int        iq;
    int        narg;

    /* Read info from ascii message
     ******************************/
    narg =  sscanf( msg,
		    "%*d %*d %ld %s %lf %lf %lf %f %f %f %f %d",
		    &qkseq, timestr, &lat, &lon, &z,
		    &rms, &dmin, &ravg, &gap, &nph );

    if ( narg < 10 ) {
	sprintf( Text, "eqas_quake2k: Error reading ascii quake msg: %s", msg );
	eqas_status( TypeError, ERR_QUAKEREAD, Text );
	return( 0 );
    }

    tNow = tnow();
    tOrigin = julsec17( timestr );
    if ( tOrigin == 0. ) {
	sprintf( Text, "eqas_quake2k: Error decoding quake time: %s", timestr );
	eqas_status( TypeError, ERR_TIMEDECODE, Text );
	return( 0 );
    }

    /* Store all hypo info in eqproc's RPT_HYP structure
     ***************************************************/
    iq = qkseq % maxHyp;

    if( qkseq != Hyp[iq].id ) {           /* initialize flags the 1st  */
                                          /* time a new quake id#      */
	Hyp[iq].eqStatus  = eqUnreported;
	Hyp[iq].id        = qkseq;
	Hyp[iq].tDetect   = tNow;
	Hyp[iq].tCodaWait = -1.0;
	Hyp[iq].maxPhasesForThisEq = maxPhasesPerEq;
	Hyp[iq].actualPhases = nph; /* put in the nph from nucleation phase (come before quake2k message in binder_ew) */
    }
    Hyp[iq].tOrigin = tOrigin;
    /* set time when event will be due for final report */
    switch (finalDelaySince) {
    case SinceStable:
	Hyp[iq].tRpt = tNow + finalDelay;
	break;
    case SinceOrigin:
	Hyp[iq].tRpt = tOrigin + finalDelay;
	break;
    case SinceDetection:
	Hyp[iq].tRpt = Hyp[iq].tDetect + finalDelay;
	break;
    default:
	logit("et", "eqas_quake2k: unknown SinceDetection value: %d\n",
	      finalDelaySince);
	Hyp[iq].tRpt = tNow + finalDelay;  /* eqproc behavior */
	break;
    }
    
    Hyp[iq].lat     = lat;
    Hyp[iq].lon     = lon;
    Hyp[iq].z       = z;
    Hyp[iq].rms     = rms;
    Hyp[iq].dmin    = dmin;
    Hyp[iq].ravg    = ravg;
    Hyp[iq].gap     = gap;
    Hyp[iq].nph     = nph;
    if (finalDelaySince != SinceStable && Hyp[iq].nph >= maxPhasesPerEq && 
	/* save the time we first get to maxPhasesPerEq */
	Hyp[iq].tCodaWait < 0.0) {
	Hyp[iq].tCodaWait = tNow + MAX_CODA_WAIT;
    }

    /* Write out the time-stamped hypocenter to the log file
     *******************************************************/
    date20( tNow, cdate );
    date20( Hyp[iq].tOrigin, corg );
    logit( "",
	   "%s:%8ld %s%9.4f%10.4f %6.2f %5.2f %5.1f %5.1f %4.0f %3d\n",
	   cdate+10, Hyp[iq].id, corg+10,
	   Hyp[iq].lat, Hyp[iq].lon, Hyp[iq].z,
	   Hyp[iq].rms, Hyp[iq].dmin, Hyp[iq].ravg,
	   Hyp[iq].gap, Hyp[iq].nph );

    
    /* See if this event is ready for reporting */
    if (Hyp[iq].nph  == 0 && Hyp[iq].eqStatus > eqUnreported) {
	Hyp[iq].eqStatus = eqKilled;         /* flag an event as killed   */
	eqas_cancelevent(iq);
    } else {
	eqas_earlyRules(iq);
    }
    
    return( 1 );
}

/****************************************************************************/
/*  eqas_link() processes a TYPE_LINK message                                */
/****************************************************************************/
int eqas_link( char *msg )
{
    long          lp1;
    long          lp2;
    long          lp;
    int           iq, is, ip, narg;
    long          qkseq;
    int           pkseq;
    unsigned char pksrc;
    unsigned char pkinstid;
    int           isrc, iinstid, iphs;

    narg  = sscanf( msg, "%ld %d %d %d %d",
		    &qkseq, &iinstid, &isrc, &pkseq, &iphs );

    if ( narg < 5 ) {
	sprintf( Text, "eqas_link: Error reading ascii link msg: %s", msg );
	eqas_status( TypeError, ERR_LINKREAD, Text );
	return( 0 );
    }
    pksrc    = (unsigned char) isrc;
    pkinstid = (unsigned char) iinstid;

    lp2 = nPck;
    lp1 = lp2 - (long)maxPck;
    if(lp1 < 0) lp1 = 0;

    iq = qkseq % maxHyp;

    for( lp=lp1; lp<lp2; lp++ ) {   /* loop from oldest to most recent */
	ip = lp % maxPck;
	if( pkseq    != Pck[ip].id )        continue;
	if( pksrc    != Pck[ip].src )       continue;
	if( pkinstid != Pck[ip].instid )    continue;
	Pck[ip].phase = (char) iphs;       /* identify its phase */
	if (qkseq != 0)  {
	     Hyp[iq].actualPhases += 1;  	/* we have a phase linked to this quake */
        } else {
	     /* this is a phase unlinking, quake id is set to zero for this pkseq, find what it was prior */
             iq = Pck[ip].quake % maxHyp;
	     Hyp[iq].actualPhases -= 1;  	/* phase now unlinked from this quake */
	}
        if (Debug) {
 	     is = Pck[ip].site;
	     if (qkseq != 0)  {
               logit("t", "Pick linkage quake=%ld pick=%d %s.%s.%s.%s phase=%s\n", qkseq, pkseq, 
                    Site[is].name, Site[is].comp, Site[is].net, Site[is].loc, Phs[iphs]);
	     } else {
               logit("t", "Pick unlinkage of pick=%d from quake=%ld  %s.%s.%s.%s\n", pkseq, Pck[ip].quake,
                    Site[is].name, Site[is].comp, Site[is].net, Site[is].loc);
	     }
        }
	Pck[ip].quake = qkseq;             /* link this pick to a quake, also unlink if qkseq==0 */
	return( 1 );
    }

    sprintf( Text, "eqas_link: fifo lapped, no pick found for link msg: %s", msg );
    eqas_status( TypeError, ERR_FIFOLAP, Text );

    return( 0 );
}

/****************************************************************************/
/*   eqas_pickscnl() decodes a TYPE_PICK_SCNL message from ascii to binary  */
/****************************************************************************/
int eqas_pickscnl( char *msg )
{
    EWPICK   pick;
    time_t   t_in;
    int      isite;
    int      lp, lp1, lp2, ip;
    int      i;

    /* Make note of current system time
     **********************************/
    time( &t_in );

    /* Read the pick into an EWPICK struct
     *************************************/
    if( rd_pick_scnl( msg, (int)strlen(msg), &pick ) != EW_SUCCESS ) {
	sprintf( Text, "eqas_pickscnl: Error reading pick: %s", msg );
	eqas_status( TypeError, ERR_PICKREAD, Text );
	return( 0 );
    } 

    /* Get site index (isite)
     ************************/
    isite = site_index( pick.site, pick.net, pick.comp, pick.loc );
    if( isite < 0 ) {
	sprintf( Text, "eqas_pickscnl: %s.%s.%s.%s - Not in station list.",
		 pick.site, pick.comp, pick.net, pick.loc );
	eqas_status( TypeError, ERR_UNKNOWNSTA, Text );
	return( 0 );
    }
 
    /* Try to find coda part in existing pick list
     *********************************************/
    lp2 = nPck;
    lp1 = lp2 - (int)maxPck;
    if( lp1 < 0 ) lp1 = 0;

    for( lp=lp2-1; lp>=lp1; lp-- ) { /* loop from most recent to oldest */
   	ip = lp % maxPck;
	if( pick.instid != Pck[ip].instid ) continue;
	if( pick.modid  != Pck[ip].src    ) continue;
	if( pick.seq    != Pck[ip].id     ) continue;
	Pck[ip].tArr     = pick.tpick + (double)GSEC1970; /* use gregorian time */
	Pck[ip].site    = isite;
	Pck[ip].phase   = 0;
	Pck[ip].fm      = pick.fm;
	Pck[ip].ie      = ' ';
	Pck[ip].wt      = pick.wt;
	Pck[ip].timeout = 0;
	for( i=0; i<3; i++ ) {
	    Pck[ip].pamp[i] = pick.pamp[i];
	}
	return( 1 );
    }

    /* Coda was not in list; load pick info into list
     ************************************************/
    ip  = nPck % maxPck;
    Pck[ip].instid  = pick.instid;
    Pck[ip].src     = pick.modid;
    Pck[ip].id      = pick.seq;
    Pck[ip].tArr    = pick.tpick + (double)GSEC1970; /* use gregorian time */
    Pck[ip].site    = isite;
    Pck[ip].phase   = 0;
    Pck[ip].fm      = pick.fm;
    Pck[ip].ie      = ' ';
    Pck[ip].wt      = pick.wt;
    for( i=0; i<3; i++ ) {
	Pck[ip].pamp[i] = pick.pamp[i];
    }
    Pck[ip].quake   = 0;
    Pck[ip].timeout = t_in + MAX_CODA_WAIT;
    nPck++;

    /* Coda was not in list; zero-out all coda info;
       it will be filled by TYPE_CODA_SCNL later.
    ***********************************************/
    for( i=0; i<6; i++ ) {
	Pck[ip].caav[i] = 0;
    }
    Pck[ip].clen   = 0;

    return ( 1 );
}

/****************************************************************************/
/*  eqas_codascnl() processes a TYPE_CODA_SCNL message                       */
/****************************************************************************/

int eqas_codascnl( char *msg )
{
    EWCODA  coda;
    long    lp, lp1, lp2;
    int     i, ip;

    /* Read the coda into an EWCODA struct
     *************************************/
    if( rd_coda_scnl( msg, (int)strlen(msg), &coda ) != EW_SUCCESS ) {
	sprintf( Text, "eqas_codascnl: Error reading coda: %s", msg );
	eqas_status( TypeError, ERR_CODAREAD, Text );
	return( 0 );
    }

    /* Try to find pick part in existing pick list
     *********************************************/
    lp2 = nPck;
    lp1 = lp2 - (long)maxPck;
    if( lp1 < 0 ) lp1 = 0;

    for( lp=lp2-1; lp>=lp1; lp-- ) { /* loop from most recent to oldest */
	ip = lp % maxPck;
	if( coda.instid != Pck[ip].instid )  continue;
	if( coda.modid  != Pck[ip].src )     continue;
	if( coda.seq    != Pck[ip].id )      continue;
	for( i=0; i<6; i++ ) {
	    Pck[ip].caav[i] = coda.caav[i];
	}
	Pck[ip].clen     = (short)coda.dur;
	Pck[ip].timeout  = 0;
	return( 1 );
    }

    /* Pick was not in list; load coda info into list
     ************************************************/
    ip  = nPck % maxPck;
    Pck[ip].instid = coda.instid;
    Pck[ip].src    = coda.modid;
    Pck[ip].id     = coda.seq;
    for( i=0; i<6; i++ ) {
	Pck[ip].caav[i] = coda.caav[i];
    }
    Pck[ip].clen   = (short)coda.dur;
    Pck[ip].quake  = 0;
    nPck++;

    /* Pick not in list; zero-out all pick info; will be filled by TYPE_PICK_SCNL
     *************************************************************************/
    Pck[ip].tArr   = 0.0;
    Pck[ip].site   = 0;
    Pck[ip].phase  = 0;
    Pck[ip].fm     = ' ';
    Pck[ip].ie     = ' ';
    Pck[ip].wt     = '9';
    for( i=0; i<3; i++ ) {
	Pck[ip].pamp[i] = (long) 0.;
    }
    Pck[ip].timeout  = 0;
    return( 1 );
}

/****************************************************************************
 * eqas_earlyRules() test if an event will pass any of the "early" rules    *
 *	The early rules are:	                                            *
 *	1. minimum number of associated P phases			    *
 *	2. event is sufficiently "old" to be ready for some further 	    *
 *	   processing. Definition if event age is configurable.		    *
 ***************************************************************************/
void  eqas_earlyRules( int iq )  /* iq is the index into Hyp    */
{
    double tNow, t = 0.0;
    
    tNow = tnow();
    
    if (doFinal && Hyp[iq].eqStatus < eqFinal) { 
	if (Hyp[iq].nph >= maxPhasesPerEq && ! finalIncludeCodas) {
	    /* Already have enough phases, don't have to wait for codas, *
	     * so we can report, even though binder hasn't stablized     *
	     * solution. */
	    eqas_assembleIfReady(iq, eqFinal);
	    if (Hyp[iq].eqStatus == eqFinal) {
		/* We did a final report, so we're done */
		return;
	    }
	}
    }
	
    if (doRapid && Hyp[iq].eqStatus < eqRapid) {
	switch (rapidDelaySince) {
	case SinceOrigin:
	    t = Hyp[iq].tOrigin;
	    break;
	case SinceDetection:
	    t = Hyp[iq].tDetect;
	    break;
	default:
	    logit("et", "eqas_earlyRules: unknown SinceDetection value: %d\n",
		  rapidDelaySince);
	    break;
	}
	if ((t > 0.0 && tNow - t > rapidDelay) ||
	    Hyp[iq].nph >= maxPhasesPerEq) {
	    /* Fill in the Srt array and report */
	    eqas_assembleIfReady(iq, eqRapid);
	    if (Hyp[iq].eqStatus == eqRapid) {
		/* We did a rapid report, so we're done */
		return;
	    }
	}
    }
	    
    if (doPrelim && Hyp[iq].eqStatus < eqPrelim &&
	Hyp[iq].nph >= prelimMinPhases) {
	eqas_assembleIfReady(iq, eqPrelim);
    }
    return;
}


/****************************************************************************
 * eqas_delayedRules() Check events against the time-based rules.           *
 * 	The rules are:                                                      *
 *	1. event is sufficiently "old" to be ready for some further 	    *
 *	   processing. Definition if event age is configurable.		    *
 *	2. event has stabilized long enough to be considered "Final"        *
 *	   This may optionally include a wait for coda messages.            *
 ****************************************************************************/
void eqas_delayedRules( )
{
    double tNow, t = 0.0;
    int iq;
    time_t     tsys;

    /*** Make note of current time ***/
    time( &tsys );   /* time as a time_t */
    tNow = tnow();   /* time as a double */
	
    if (doFinal) {
	/* Loop thru all hypocenters, see if it's time to report any as Final */
        for(iq=0; iq<maxHyp; iq++) {
	    /* Tests to see if eq is ready for reporting: *
	     * not ready if we don't have a binder solution */
	    if (Hyp[iq].eqStatus < eqUnreported) continue;

	    /* don't report again if final report already sent. */
	    if (Hyp[iq].eqStatus == eqFinal) continue;

	    /* don't report if event timer not due *
	     * and we don't have maxPhases yet.    */
	    if (Hyp[iq].tRpt > tNow && Hyp[iq].nph < maxPhasesPerEq) continue;

	    /* Did the SinceOrigin or SinceDetection timer expire?       *
	     * Set maxPhasesForThisEq to current nphs if not done before */
	    if (finalDelaySince != SinceStable && 
		Hyp[iq].maxPhasesForThisEq == maxPhasesPerEq) {
		Hyp[iq].maxPhasesForThisEq = Hyp[iq].nph;
	    }
	    
	    /* event is ready for final report except possibly for codas. *
	     * Let eqas_assembleIfReady do its tests and report if ready.        */
	    eqas_assembleIfReady(iq, eqFinal);
	}
    }
    if (doRapid) {
        for(iq=0; iq<maxHyp; iq++) {
	    if (Hyp[iq].eqStatus < eqUnreported || Hyp[iq].eqStatus > eqPrelim) continue;
	    switch (rapidDelaySince) {
	    case SinceOrigin:
		t = Hyp[iq].tOrigin;
		break;
	    case SinceDetection:
		t = Hyp[iq].tDetect;
		break;
	    default:
	         logit("et", "eqas_delayedRules: WARNING! unknown RapidRule 'delaySince' value: %d\n",
		  rapidDelaySince);
	         break;
	    }
	    if (t > 0.0 && tNow - t > rapidDelay) {
		/* Fill in the Srt array and report */
		eqas_assembleIfReady(iq, eqRapid);
	    }
	}
    }
}


/*************************************************************************
 * eqas_assembleIfReady: Maybe assemble the associated picks in preparation for *
 *    reporting. Event still must meet tests; if it does, it is reported *
 *    at the level specified by eqStatus.                                *
 *************************************************************************/
void eqas_assembleIfReady( int iq, enum EqStatus eqStatus )
{
    TPHASE treg[10];
    struct Greg  g;
    time_t twait, tsys;    
    long minute;
    long lp, lp1, lp2;  /* indices into pick structure  */
    int i, ip, is, iph, nph, includeCodas;
    int nphs_P, minPPhases;
    double xs, ys;
    double xq, yq, zq;
    double r, tNow;
    double dtdr, dtdz;
    double tres;
    char cdate[24];
    char corg[24];
    char carr[24];
    char cwaif[24];
    char txtpck[120];
    char *reportPrefix;
    char *prelimPrefix = "Preliminary ";
    char *rapidPrefix = "Rapid";
    char *finalPrefix = "Final";
    char        *eqmsg;           /* working pointer into EqMsg   */
    enum Version version;         /* version number to report     */
    

    /* See if this hypocenter passes the notification rule
     *****************************************************/
    if( Hyp[iq].eqStatus >= eqStatus ) return; /* already reported this version */

    /* Loop thru all picks, collecting associated picks for sorting 
     ***************************************************************/
    memset((void*)Srt, 0, sizeof(SRT) * maxPck);
    nSrt   = 0;
    nphs_P = 0;
    lp2    = nPck;
    lp1    = lp2 - (long)maxPck;
    if(lp1 < 0) lp1 = 0;
    for(lp=lp1; lp<lp2; lp++) {
	ip  = lp % maxPck;
	
	if(Pck[ip].quake == Hyp[iq].id) {
	    if(Pck[ip].phase%2 == 0) {     /* It's a P-phase... */
		nphs_P++;                  /* ...count them     */
	    } else {                         /* It's an S-phase... */
		if(ReportS == 0) continue; /* ...see if it should be skipped */
	    }
	    Srt[nSrt].tArr  = Pck[ip].tArr; /* load info for sorting  */
	    Srt[nSrt].ip = ip;
	    nSrt++;                         /* count total # phases   */
	}
    }
    
    switch (eqStatus) {
    case eqPrelim:
	reportPrefix = prelimPrefix;
	version = vPrelim;
	minPPhases = prelimMinPhases;
	includeCodas = 0;
	break;
    case eqRapid:
	reportPrefix = rapidPrefix;
	version = vRapid;
	minPPhases = rapidMinPhases;
	includeCodas = 0;
	break;
    case eqFinal:
	reportPrefix = finalPrefix;
	version = vFinal;
	minPPhases = (finalMinPhases < maxPhasesPerEq) ? finalMinPhases: maxPhasesPerEq;
	includeCodas = finalIncludeCodas;
	break;
    default:
	logit("et", "eqas_assembleIfReady: unexpected report level: %d\n", eqStatus);
	return;
    }
    
    time( &tsys );  /* time as a time_t */
    tNow = tnow();  /* time as a double */
    date20( tNow, cdate );

    if (UseS && nSrt < minPPhases) {
	if (nphs_P > 0) {
	    /* don't bother logging this if binder "killed" the event */
	    logit( "",
		   "%s:%8ld #### %s report delayed: %3d P-phs; %3d Total phases seen, %3d picks total in report\n",
		   cdate+10, Hyp[iq].id, reportPrefix, nphs_P, nSrt, Hyp[iq].nph );
	}
	return;
    } else if (!UseS && nphs_P < minPPhases) {
	if (nphs_P > 0) {
	    /* don't bother logging this if binder "killed" the event */
	    logit( "",
		   "%s:%8ld #### %s report delayed: %3d P-phs; %3d picks total\n",
		   cdate+10, Hyp[iq].id, reportPrefix, nphs_P, Hyp[iq].nph );
	}
	return;
    }

    /* sort picks by arrival time */
    qsort(Srt, nSrt, sizeof(SRT), eqas_compare);

    /* If we want codas, see if they are all here.           */
    /* We got here because we have more than enough phases   *
     * to report for a Final report, but the binder solution *
     * may not yet be stabilised. Look for codas associated  *
     * with P phases within earliest maxPhasesPerEq phases.  */
    if (includeCodas) {
	twait = 0;
	for (i = 0; i < nSrt; i++) {
	    if (i > Hyp[iq].maxPhasesForThisEq) break;
	    ip  = Srt[i].ip;
	    if (Pck[ip].phase%2 != 0) continue;  /* not a P; no coda */
	    if ( Pck[ip].instid != InstId ) {
		/* Pick isn't local; are we       *
		 * supposed to wait for its coda? */
		for (is = 0; is < nCodaInst; is++) {
		    if (Pck[ip].instid == CodaInst[is]) break;
		}
		if (is == nCodaInst) continue;
	    }
	    if ( Pck[ip].timeout > twait ) 
		twait = Pck[ip].timeout;
	}

	/* Need to wait for more codas to arrive                     *
	 * but we don't really want to wait for codas from stations  *
	 * with big telemetry delays: we want to get the report out! */
	if ( Hyp[iq].tCodaWait > 0 && Hyp[iq].tCodaWait < twait) {
	    date20((double)twait, cdate);
	    logit("", "wait for codas (%s) too long;", 
		  cdate+10);
	    date20(Hyp[iq].tCodaWait, cdate);
	    logit("", "wait shortened to %s", cdate+10);
	    twait = (time_t)Hyp[iq].tCodaWait;
	}
	
	if ( twait > tsys ) {
	    /* wait a little longer for codas */
	    Hyp[iq].tRpt += HypCheckInterval;
	    date20( (double)tsys, cdate );
	    logit( "",
		   "%s:%8ld waiting for codas.\n",
		   cdate+10, Hyp[iq].id);
	    return;
	}
    }
	    

    /* Prepare for residuals */
    xq = X(Hyp[iq].lon);
    yq = Y(Hyp[iq].lat);
    zq = Hyp[iq].z;

    /* Report this event
     *******************/
    minute = (long)(Hyp[iq].tOrigin / 60.0);
    grg(minute, &g);
    date20( Hyp[iq].tOrigin, corg );

    logit( "", "%s:%8ld #### %s report: %04d%02d%02d%02d%02d_%02d\n",
	   cdate+10, Hyp[iq].id, reportPrefix, g.year, g.month, g.day,
	   g.hour, g.minute, (int) (Hyp[iq].id % 100) );
    logit("", "%8ld %s%9.4f%10.4f %6.2f %5.2f %5.1f %5.1f %4.0f %3d\n",
	  Hyp[iq].id, corg,
	  Hyp[iq].lat, Hyp[iq].lon, Hyp[iq].z,
	  Hyp[iq].rms, Hyp[iq].dmin,
	  Hyp[iq].ravg, Hyp[iq].gap, Hyp[iq].nph);
    logit("", "%8ld phases_linked=%d, phases_remaining@report_time=%d\n", Hyp[iq].id, Hyp[iq].actualPhases, nSrt);

    /* Build an event message an write it to the pipe
     ************************************************/
    /* write binder's hypocenter to message */
    eqas_hypcard(iq, EqMsg, version);

    /* Add phases and maybe codas to message */
    for (i=0; i<nSrt; i++) {
	eqmsg = EqMsg + strlen(EqMsg);
	if (i < Hyp[iq].maxPhasesForThisEq) 
	    eqas_phscard(Srt[i].ip, eqmsg, includeCodas);

	/* log phases and residuals */
	ip  = Srt[i].ip;
	is  = Pck[ip].site;
	iph = Pck[ip].phase;
	date20(Pck[ip].tArr, carr);
	xs  = X(Site[is].lon);
	ys  = Y(Site[is].lat);
	r   = ew_hypot(xs - xq, ys - yq);
	tres = Pck[ip].tArr - Hyp[iq].tOrigin -
	    t_phase(iph, r, zq, &dtdr, &dtdz);
	logit("", "%-5s %-2s %-3s %-2s %s %-2s%c%c%6.1f%7.2f\n",
	      Site[is].name, Site[is].comp, Site[is].net, Site[is].loc,
	      carr, Phs[iph], Pck[ip].fm,
	      Pck[ip].wt, r, tres);
    }
    /* write event message to pipe */
    if ( pipe_put( EqMsg, TypeEventSCNL ) != 0 )
	logit("et","eqas_assembleIfReady: Error writing eq message to pipe.\n");

    /* Report waifs and phases with small residuals associated with *
     * other events.                                                */
    if (eqStatus == eqFinal) {
	lp2    = nPck;
	lp1    = lp2 - (long)maxPck;
	if(lp1 < 0) lp1 = 0;
	for(lp=lp1; lp<lp2; lp++)
	{
	    ip  = lp % maxPck;
	    if(Pck[ip].quake != Hyp[iq].id) {
		/* Skip picks that aren't close to origin time */
		if(Pck[ip].tArr < Hyp[iq].tOrigin - waifTolerance) continue;
		if(Pck[ip].tArr > Hyp[iq].tOrigin + 120.0)         continue;
		is = Pck[ip].site;
		
		xs  = X(Site[is].lon);
		ys  = Y(Site[is].lat);
		r   = ew_hypot(xs - xq, ys - yq);
		nph = t_region(r, zq, treg);
		/* Log waifs and picks close in time to this event *
		 * but associated with other events.               */
		strcpy(cwaif, "WAIF");
		if(Pck[ip].quake)
		    sprintf(cwaif, "#%ld",   Pck[ip].quake);
		for(i=0; i<nph; i++) {
		    tres = Pck[ip].tArr - Hyp[iq].tOrigin - treg[i].t;
		    if(fabs(tres) >  waifTolerance)   continue;
		    iph = treg[i].phase;
                    date20(Pck[ip].tArr, carr);
		    sprintf(txtpck,
			    "%-5s %-2s %-3s %-2s %s %-2s%c%c%6.1f%7.2f %s",
			    Site[is].name, Site[is].comp,
			    Site[is].net, Site[is].loc, 
			    carr, Phs[iph], Pck[ip].fm, Pck[ip].wt,
			    r, tres, cwaif);
		    logit( "", "%s\n", txtpck);
		}
	    }
	}
    }

    /* Flag event as reported for this level */
    Hyp[iq].eqStatus = eqStatus;

    return;
}


/*****************************************************************************/
/* eqas_phscard() builds a character-string phase-card from RPT_PCK structure*/
/*****************************************************************************/
char *eqas_phscard( int ip, char *phscard, int includeCodas )
{
    char   timestr[19];
    int    is, iph;
    int    incCoda;
    
    /*-------------------------------------------------------------------------
      Sample Earthworm format phase card (variable-length whitespace delimited):
      CMN VHZ NC -- U1 P 19950831183134.902 953 1113 968 23 201 276 289 0 0 7 W\n
      -----------------------------------------------------------------------*/

    /* Convert julian seconds character string 
     *****************************************/
    date18( Pck[ip].tArr, timestr );
    is  = Pck[ip].site;
    iph = Pck[ip].phase;
    /* Include coda information if requested, but only for P phases */
    incCoda = (includeCodas && Phs[iph][0] == 'P') ? 1 : 0;
    
    /* Write appropriate info to an Earthworm phase card
     ****************************************************/
    sprintf( phscard, 
	     "%s %s %s %s %c%c %s %s %ld %ld %ld %ld %ld %ld %ld %ld %ld %hd %s\n",
	     Site[is].name,
	     Site[is].comp,
	     Site[is].net,
	     Site[is].loc,
	     Pck[ip].fm,
	     Pck[ip].wt,
	     Phs[iph],
	     timestr,
	     Pck[ip].pamp[0],
	     Pck[ip].pamp[1],
	     Pck[ip].pamp[2],
	     (incCoda) ? Pck[ip].caav[0] : 0,
	     (incCoda) ? Pck[ip].caav[1] : 0,
	     (incCoda) ? Pck[ip].caav[2] : 0,
	     (incCoda) ? Pck[ip].caav[3] : 0,
	     (incCoda) ? Pck[ip].caav[4] : 0,
	     (incCoda) ? Pck[ip].caav[5] : 0,
	     (short) ((incCoda) ? Pck[ip].clen    : 0),
	     DataSrc );

    /*logit( "", "%s", phscard );*/ /*DEBUG*/
    return( phscard );
}

/*****************************************************************************/
/* eqas_hypcard() builds a character-string hypocenter card from             *
 *	 RPT_HYP struct   						     */
/*****************************************************************************/
char *eqas_hypcard( int iq, char *hypcard, enum Version version )

{
    char   timestr[19];

    /*-------------------------------------------------------------------------------
      Sample binder-based hypocenter as built below (whitespace delimited, variable length); 
      Event id from binder is added at end of card. 
      19920429011704.653 36.346578 -120.546932 8.51 27 78 19.8 0.16 10103 1\n
      --------------------------------------------------------------------------------------*/

    /* Convert julian seconds character string
     *****************************************/
    date18( Hyp[iq].tOrigin, timestr );

    /* Write all info to hypocenter card  
     ***********************************/
    sprintf( hypcard,
	     "%s %.6lf %.6lf %.2lf %d %.0f %.1f %.2f %ld %d\n",
	     timestr,
	     Hyp[iq].lat,
	     Hyp[iq].lon,
	     Hyp[iq].z,
	     Hyp[iq].nph,
	     Hyp[iq].gap,
	     Hyp[iq].dmin,
	     Hyp[iq].rms,
	     Hyp[iq].id,
	     version );

    /* logit( "", "%s", hypcard );*/ /*DEBUG*/
    return( hypcard );
}

/*****************************************************************************/
/*  eqas_compare() compare 2 times                                           */
/*****************************************************************************/
int eqas_compare( const void *p1, const void *p2 )
{
    SRT *srt1;
    SRT *srt2;

    srt1 = (SRT *) p1;
    srt2 = (SRT *) p2;
    if(srt1->tArr < srt2->tArr)   return -1;
    if(srt1->tArr > srt2->tArr)   return  1;
    return 0;
}

/******************************************************************************/
/* eqas_status() builds a heartbeat or error msg & puts it into shared memory  */
/******************************************************************************/
void eqas_status( unsigned char type, short ierr, char *note )
{
    char     msg[256];
    time_t   t;

    /* Build the message
     *******************/
    time( &t );

    if( type == TypeHeartBeat ) {
	sprintf( msg, "%ld %d\n", (long)t, MyPID );
    } else if( type == TypeError ) {
	sprintf( msg, "%ld %d %s\n", (long)t, ierr, note);
	logit( "et", "%s:  %s\n", Arg0, note );
    }

    /* Write the message to the pipe
     *******************************/
    if( pipe_put( msg, type ) != 0 )
	logit( "et", "%s:  Error sending msg to pipe.\n", Arg0);
   
    return;
}


/***************************************************************************
 * eqas_cancelevent() builds a cancelEvent msg & sends it down             *
 *                   the pipe to next module                               *
 ***************************************************************************/
void eqas_cancelevent( int iq )  /* iq is the index into Hyp    */
{
    double t;
    char cdate[24];
    char msg[100];
    
    t = tnow();
    date20( t, cdate );

    logit( "", "%s:%8ld #### event cancelled\n", cdate+10, Hyp[iq].id );

    /* Build an event message an write it to the pipe */
    sprintf( msg ,"%ld\n",Hyp[iq].id);
    
    /* write cancel event message to pipe */
    if ( pipe_put( msg , TypeCancelEvent ) != 0 ) {
	logit("et","eqas_cancelevent: Error writing CancelEvent message to pipe.\n");
    }
    
    return;
}


/*****************************************************************************/
/*     eqas_config() processes command file(s) using kom.c functions         */
/*                  exits if any errors are encountered                      */
/*****************************************************************************/
void eqas_config( char *configfile )
{
    int      ncommand;     /* # of required commands you expect to process   */
    char     init[10];     /* init flags, one byte for each required command */
    int      nmiss;        /* number of required commands that were missed   */
    char    *com;
    char    *str;
    char     processor[15];
    int      nfiles;
    int      success;
    int      i, tmp;
    int      rules = 0;

    /* Set to zero one init flag for each required command
     *****************************************************/
    ncommand = 8;
    for( i=0; i<ncommand; i++ )  init[i] = 0;
    nLogo = 0;

    /* Open the main configuration file
     **********************************/
    nfiles = k_open( configfile );
    if ( nfiles == 0 ) {
        logit( "e",
		 "%s: Error opening command file <%s>; exiting!\n",
                 Arg0, configfile );
        exit( -1 );
    }

    /* Process all command files
     ***************************/
    while(nfiles > 0) {  /* While there are command files open */
        while(k_rd()) {        /* Read next line from active file  */
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
		    logit( "e",
			     "%s: Error opening command file <%s>; exiting!\n",
			     Arg0, &com[1] );
		    exit( -1 );
		}
		continue;
            }

	    /* Process anything else as a command
             ************************************/
            strcpy( processor, "eqas_config" );

	    /* Numbered commands are required
             ********************************/
	    /*0*/     if( k_its("LogFile") ) {
                LogSwitch = k_int();
                init[0] = 1;
            }
	    /*1*/     else if( k_its("MyModuleId") ) {
                str = k_str();
                if(str) strcpy( MyModName, str );
                init[1] = 1;
            }
	    /*2*/     else if( k_its("RingName") ) {
                str = k_str();
                if(str) strcpy( RingName, str );
                init[2] = 1;
            }

            /*3*/ else if( k_its("HeartbeatInt") ) {
		HeartbeatInt = k_int();
		init[3] = 1;
	    }

	    /* Enter installation & module to get picks & codas from
             *******************************************************/
	    /*4*/     else if( k_its("GetPicksFrom") ) {
                if ( nLogo+1 >= MAXLOGO ) {
                    logit( "e",
			     "%s: Too many <Get*> commands in <%s>",
                             Arg0, configfile );
                    logit( "e", "; max=%d; exiting!\n", (int) MAXLOGO/2 );
                    exit( -1 );
                }
                if( ( str=k_str() ) != NULL ) {
		    if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
			logit( "e",
				 "%s: Invalid installation name <%s>", Arg0,
				 str );
			logit( "e", " in <GetPicksFrom> cmd; exiting!\n" );
			exit( -1 );
		    }
		    GetLogo[nLogo+1].instid = GetLogo[nLogo].instid;
                }
                if( ( str=k_str() ) != NULL ) {
		    if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
			logit( "e",
				 "%s: Invalid module name <%s>", Arg0, str );
			logit( "e", " in <GetPicksFrom> cmd; exiting!\n" );
			exit( -1 );
		    }
		    GetLogo[nLogo+1].mod = GetLogo[nLogo].mod;
                }
                if( GetType( "TYPE_PICK_SCNL", &GetLogo[nLogo].type ) != 0 ) {
                    logit( "e",
			     "%s: Invalid message type <TYPE_PICK_SCNL>",
			     Arg0);
                    logit( "e", "; exiting!\n" );
                    exit( -1 );
                }
                if( GetType( "TYPE_CODA_SCNL", &GetLogo[nLogo+1].type ) != 0 ) {
                    logit( "e",
			     "%s: Invalid message type <TYPE_CODA_SCNL>", 
			     Arg0);
                    logit( "e", "; exiting!\n" );
                    exit( -1 );
                }
                /*printf("GetLogo[%d] inst:%d module:%d type:%d\n",
		  nLogo, (int) GetLogo[nLogo].instid,
		  (int) GetLogo[nLogo].mod,
		  (int) GetLogo[nLogo].type ); */  /*DEBUG*/
                /*printf("GetLogo[%d] inst:%d module:%d type:%d\n",
		  nLogo+1, (int) GetLogo[nLogo+1].instid,
		  (int) GetLogo[nLogo+1].mod,
		  (int) GetLogo[nLogo+1].type ); */  /*DEBUG*/
                nLogo  += 2;
                init[4] = 1;
            }
	    /* Enter installation & module to get associations from
             ******************************************************/
	    /*5*/     else if( k_its("GetAssocFrom") ) {
                if ( nLogo+1 >= MAXLOGO ) {
                    logit( "e",
			     "%s: Too many <Get*From> commands in <%s>",
                             Arg0, configfile );
                    logit( "e", "; max=%d; exiting!\n", (int) MAXLOGO/2 );
                    exit( -1 );
                }
                if( ( str=k_str() ) != NULL ) {
		    if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
			logit( "e",
				 "%s: Invalid installation name <%s>", Arg0, 
				 str );
			logit( "e", " in <GetAssocFrom> cmd; exiting!\n" );
			exit( -1 );
		    }
		    GetLogo[nLogo+1].instid = GetLogo[nLogo].instid;
                }
                if( ( str=k_str() ) != NULL ) {
		    if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
			logit( "e",
				 "%s: Invalid module name <%s>", Arg0, str );
			logit( "e", " in <GetAssocFrom> cmd; exiting!\n" );
			exit( -1 );
		    }
		    GetLogo[nLogo+1].mod = GetLogo[nLogo].mod;
                }
                if( GetType( "TYPE_QUAKE2K", &GetLogo[nLogo].type ) != 0 ) {
                    logit( "e",
			     "%s: Invalid message type <TYPE_QUAKE2K>", Arg0 );
                    logit( "e", "; exiting!\n" );
                    exit( -1 );
                }
                if( GetType( "TYPE_LINK", &GetLogo[nLogo+1].type ) != 0 ) {
                    logit( "e",
			     "%s: Invalid message type <TYPE_LINK>", Arg0 );
                    logit( "e", "; exiting!\n" );
                    exit( -1 );
                }
                /*printf("GetLogo[%d] inst:%d module:%d type:%d\n",
		  nLogo, (int) GetLogo[nLogo].instid,
		  (int) GetLogo[nLogo].mod,
		  (int) GetLogo[nLogo].type ); */  /*DEBUG*/
                /*printf("GetLogo[%d] inst:%d module:%d type:%d\n",
		  nLogo+1, (int) GetLogo[nLogo+1].instid,
		  (int) GetLogo[nLogo+1].mod,
		  (int) GetLogo[nLogo+1].type ); */  /*DEBUG*/
                nLogo  += 2;
                init[5] = 1;
            }
	    /*6*/    else if( k_its("PipeTo") ) {
                str = k_str();
                if(str) strcpy( NextProc, str );
                init[6] = 1;
            }
	    /*7*/    else if( k_its("ReportS") ) {
                ReportS = k_int();
                init[7] = 1;
            }
	    /*OPTIONAL*/    else if( k_its("UseS") ) {
                UseS = 1;
            }
	    /*OPTIONAL*/    else if( k_its("Debug") ) {
                Debug = 1;
            }

	    /* At least one of these commands is required */
	    else if ( k_its("PrelimRule") ) {
		prelimMinPhases = k_int();
		doPrelim = 1;
		rules++;
	    }
	    else if ( k_its("RapidRule") ) {
		rapidMinPhases = k_int();
		rapidDelay = k_int();
		if ((str = k_str()) != NULL) {
		    if (strcasecmp(str, "SINCEORIGIN")==0 ) {
			rapidDelaySince = SinceOrigin;
		    }
		    else if (strcasecmp(str, "SINCEDETECTION")==0 ) {
			rapidDelaySince = SinceDetection;
		    } else {
			logit("e", "%s: Invalid RapidRule: %s\n", Arg0,
				k_com());
			exit(-1);
		    }
		}
		doRapid = 1;
		rules++;
	    }
	    else if (k_its("FinalRule") ) {
		finalMinPhases = k_int();
		finalDelay = k_int();
		if ( (str = k_str()) != NULL) {
		    if (strcasecmp(str, "SINCEORIGIN")==0 ) {
			finalDelaySince = SinceOrigin;
		    }
		    else if (strcasecmp(str, "SINCEDETECTION")==0 ) {
			finalDelaySince = SinceDetection;
		    }
		    else if (strcasecmp(str, "SINCESTABLE")==0 ) {
			finalDelaySince = SinceStable;
		    } 
		    else {
			logit("e", "%s: Invalid FinalRule: %s\n", Arg0,
				k_com());
			exit(-1);
		    }
		} else {
			logit("t", "%s: FinalRule:using default of SinceStable\n", Arg0);
		}
		if ( (str = k_str()) != NULL) {
		    if (strcasecmp(str, "WAITFORCODAS") == 0) {
			finalIncludeCodas = 1;
		    } else {
			logit("e", "%s: Invalid FinalRule: %s\n", Arg0,
				k_com());
			exit(-1);
		    }
		}
		doFinal = 1;
		rules++;
		continue;
	    }

	    /* These commands change default values; so are not required
             ***********************************************************/
            else if( k_its("HypCheckInterval") )
                HypCheckInterval = (int) k_val();

            else if( k_its("WaifTolerance") )
                waifTolerance= k_val();

            else if( k_its("pick_fifo_length") )
                maxPck = (size_t) k_int();

            else if( k_its("quake_fifo_length") )
                maxHyp = (size_t) k_int();

	    else if( k_its("CodaFromInst") ) {
		if (nCodaInst+1 >= MAXLOGO) {
                    logit( "e",
			     "%s: Too many <CodaFromInst> commands in <%s>",
                             Arg0, configfile );
                    logit( "e", "; max=%d; exiting!\n", MAXLOGO );
                    exit( -1 );
                }
                if( (str=k_str()) != NULL ) {
		    if( GetInst( str, &CodaInst[nCodaInst] ) != 0 ) {
			logit( "e",
				 "%s: Invalid installation name <%s>", Arg0,
				 str );
			logit( "e", " in <CodaFromInst> cmd; exiting!\n" );
			exit( -1 );
		    }
		    nCodaInst++;
                }
	    }

	    else if ( k_its("DataSrc") ) {
		str = k_str();
		if (str) {
		    if (strlen(str) > 1) {
			logit( "e",
			       "%s: DataSrc too long; max is one character",
			       Arg0);
			exit( -1 );
		    }
		    strcpy(DataSrc, str);
		}
	    }
	    
            else if( k_its("MaxPhasesPerEq") ) {
		tmp  = (size_t) k_int();
		if (tmp < MAX_PHS_PER_EQ && tmp > 0) 
		    maxPhasesPerEq = tmp;
		else {
		    logit( "e", 
			   "%s: MaxPhasesPerEq outside allowed range 1 - %d\n",
			   Arg0, MAX_PHS_PER_EQ);
		    exit( -1 );
		}
	    }

	    /* Some commands may be processed by other functions
             ***************************************************/
            else if( t_com()    )  strcpy( processor, "t_com"    );
            else if( site_com() )  strcpy( processor, "site_com" );
            else {
                logit( "e", "%s: <%s> Unknown command in <%s>.\n", Arg0,
                         com, configfile );
                continue;
            }

	    /* See if there were any errors processing the command
             *****************************************************/
            if( k_err() ) {
		logit( "e",
			 "%s: Bad <%s> command for %s() in <%s>; exiting!\n",
			 Arg0, com, processor, configfile );
		exit( -1 );
            }
        }
        nfiles = k_close();
    }

    /* After all files are closed, check init flags for missed commands
     ******************************************************************/
    nmiss = 0;
    for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
    if ( nmiss ) {
	logit( "e", "%s: ERROR, no ", Arg0 );
	if ( !init[0] )  logit( "e", "<LogFile> "      );
	if ( !init[1] )  logit( "e", "<MyModuleId> "   );
	if ( !init[2] )  logit( "e", "<RingName> "     );
	if ( !init[3] )  logit( "e", "<HeartbeatInt> " );
	if ( !init[4] )  logit( "e", "<GetPicksFrom> " );
	if ( !init[5] )  logit( "e", "<GetAssocFrom> " );
	if ( !init[6] )  logit( "e", "<PipeTo> "       );
	if ( !init[7] )  logit( "e", "<ReportS> "     );
	logit( "e", "command(s) in <%s>; exiting!\n", configfile );
	exit( -1 );
    }

    if (rules < 1) {
	logit( "e", "%s: no event rules given. \
		 At least one of PrelimRUle, RapidRule, FinalRule required\n",
		 Arg0);
	exit( -1 );
    }

    return;
}

/****************************************************************************/
/*  eqas_lookup( ) Look up important info from earthworm.h tables            */
/****************************************************************************/
void eqas_lookup( void )
{
    /* Look up keys to shared memory regions
     *************************************/
    if( ( RingKey = GetKey(RingName) ) == -1 ) {
        logit( "e",
		 "%s:  Invalid ring name <%s>; exiting!\n", Arg0, RingName);
        exit( -1 );
    }

    /* Look up installations of interest
     *********************************/
    if ( GetLocalInst( &InstId ) != 0 ) {
	logit( "e",
		 "%s: error getting local installation id; exiting!\n", Arg0 );
	exit( -1 );
    }

    /* Look up modules of interest
     ***************************/
    if ( GetModId( MyModName, &MyModId ) != 0 ) {
	logit( "e",
		 "%s: Invalid module name <%s>; exiting!\n", Arg0, MyModName );
	exit( -1 );
    }

    /* Look up message types of interest
     *********************************/
    if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
	logit( "e",
		 "%s: Invalid message type <TYPE_HEARTBEAT>; exiting!\n", Arg0 );
	exit( -1 );
    }
    if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
	logit( "e",
		 "%s: Invalid message type <TYPE_ERROR>; exiting!\n", Arg0 );
	exit( -1 );
    }
    if ( GetType( "TYPE_QUAKE2K", &TypeQuake2K ) != 0 ) {
	logit( "e",
		 "%s: Invalid message type <TYPE_QUAKE2K>; exiting!\n", Arg0 );
	exit( -1 );
    }
    if ( GetType( "TYPE_LINK", &TypeLink ) != 0 ) {
	logit( "e",
		 "%s: Invalid message type <TYPE_LINK>; exiting!\n", Arg0 );
	exit( -1 );
    }
    if ( GetType( "TYPE_PICK_SCNL", &TypePickSCNL ) != 0 ) {
	logit( "e",
		 "%s: Invalid message type <TYPE_PICK_SCNL>; exiting!\n", Arg0 );
	exit( -1 );
    }
    if ( GetType( "TYPE_CODA_SCNL", &TypeCodaSCNL ) != 0 ) {
	logit( "e",
		 "%s: Invalid message type <TYPE_CODA_SCNL>; exiting!\n", Arg0 );
	exit( -1 );
    }
    if ( GetType( "TYPE_EVENT_SCNL", &TypeEventSCNL ) != 0 ) {
	logit( "e",
		 "%s: Invalid message type <TYPE_EVENT_SCNL>; exiting!\n", 
		 Arg0 );
	exit( -1 );
    }
    if ( GetType( "TYPE_KILL", &TypeKill ) != 0 ) {
	logit( "e",
		 "%s: Invalid message type <TYPE_KILL>; exiting!\n", Arg0 );
	exit( -1 );
    }
    if ( GetType( "TYPE_CANCELEVENT", &TypeCancelEvent ) != 0 ) {
	logit( "e",
		 "%s: Invalid message type <TYPE_CANCELEVENT>; exiting!\n", 
		 Arg0 );
	exit( -1 );
    }
    return;
}

