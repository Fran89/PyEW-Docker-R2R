
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sgram.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.16  2010/02/01 20:53:14  stefan
 *     sncl to scnl
 *
 *     Revision 1.15  2008/12/02 00:02:52  luetgert
 *     Added separate heartbeat thread.
 *
 *     Revision 1.14  2008/04/28 19:57:50  luetgert
 *     *** empty log message ***
 *
 *     Revision 1.13  2008/03/12 17:18:42  luetgert
 *     Fixed error in IsDST.
 *     .
 *
 *     Revision 1.12  2007/08/03 05:20:22  luetgert
 *     Gravity changed from 978 to 981.
 *     .
 *
 *     Revision 1.11  2007/05/15 17:08:25  paulf
 *     cleaned up fgets calls with string size issue
 *
 *     Revision 1.9  2007/02/26 16:48:07  paulf
 *     made sure time_t are casted to long for heartbeat sprintf()
 *
 *     Revision 1.8  2007/02/20 22:15:45  luetgert
 *     Modified IsDST to accommodate changes in definition of DST.
 *     .
 *
 *     Revision 1.7  2005/01/28 20:48:33  luetgert
 *     Now location code compatible.
 *     Remote-copy eliminated.
 *     Uses binary wsclient routines.
 *     .
 *
 *     Revision 1.5  2002/05/15 17:35:32  patton
 *     Made Logit changes
 *
 *     Revision 1.4  2001/05/16 16:39:20  lucky
 *     removed queue_max_size.h -- it no longer exists, and this module does
 *     not appear to be in need of buffering.
 *
 *     Revision 1.3  2001/05/09 17:35:18  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPid.
 *
 *     Revision 1.2  2000/07/24 20:13:22  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.1  2000/02/14 19:17:10  lucky
 *     Initial revision
 *
 *
 */

/*************************************************************************
 *      sgram.c:                                                         *
 * On a time schedule, requests data from waveserver(s) and plots gif    *
 * files in a spectrogram format.  These files are then transferred to   *
 * webserver(s).                                                         *
 *                                                                       *
 *                                                                       *
 * Jim Luetgert 10/07/98                                                 *
 *************************************************************************/

/* versioning added on May 30, 2013 */
#define SGRAM_VERSION "0.0.4 2016-09-05"

#include <platform.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <chron3.h>
#include <string.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include "mem_circ_queue.h"
#include <swap.h>
#include <trace_buf.h>
/*
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wait.h>
*/

#include <ws_clientII.h>
#include "gd.h"
#include "gdfontt.h"   /*  6pt      */
#include "gdfonts.h"   /*  7pt      */
#include "gdfontmb.h"  /*  9pt Bold */
#include "gdfontl.h"   /* 13pt      */
#include "gdfontg.h"   /* 10pt Bold */

#include "sgram.h"

DATABUF  tracePtr;

/* Functions in this source file
 *******************************/
thr_ret Heartbeat( void * );
void SetUp(Global *);
void UpDate(Global *);
void Sort_Servers (Global *, double);
short Build_Axes(Global *, double, int, PltPar *);
void Make_Grid(Global *, PltPar *);
void Pallette(int i, gdImagePtr GIF, long color[]);
int Plot_Trace(Global *, double *, double);
void Make_Line(Global *, double *);
double Cabs(double zr, double zi);
void fit(double x, double *y, int ndata, double *a, double *b);
void taper(double *data, int npts, double width);
void set_n_2(int n, int *nadd, int *npower);
int check_2(int n, int *npower);
void cool(int nn, double *DATAI, int signi);
void CommentList(Global *);
void IndexList(Global *);
void IndexListUpdate(Global *);
void AddDay(Global *);
void Make_Date(char *);
void Save_Plot(Global *, PltPar *);
int Build_Menu (Global *);
int In_Menu_list (Global *);
int  RequestWave(Global *, int k, double *, char *, char *, char *, char *, double, double);
int  WSReqBin(Global *But, int k, TRACE_REQ *request, DATABUF *pTrace, char *SCNtxt);
int  WSReqBin2(Global *But, int k, TRACE_REQ *request, DATABUF *pTrace, char *SCNtxt);
int IsDST(int, int, int);
void Decode_Time( double, TStrct *);
void Encode_Time( double *, TStrct *);
void date22( double, char *);
void SetPlot(double, double);
int ixq(double);
int iyq(double);
void Get_Sta_Info(Global *);
int Put_Sta_Info(int, Global *);
void config_me( char *,  Global *);
void ewmod_status( unsigned char, short, char *);
void lookup_ew ( void );

/* Shared memory
 ******************************/
static  SHM_INFO  InRegion;          /* public shared memory for receiving arkive messages */

/* Things to lookup in the earthworm.h table with getutil.c functions
 **********************************************************************/
static long          InRingKey;      /* key of transport ring for input         */
static unsigned char InstId;         /* local installation id                   */
static unsigned char MyModId;        /* our module id                           */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;

/* Things to read from configuration file
 ****************************************/
static char InRingName[MAX_RING_STR];         /* name of transport ring for i/o */
static char MyModuleId[MAX_MOD_STR];          /* module id for this module      */
static int  LogSwitch;               /* 0 if no logging should be done to disk  */
static int  Transport;               /* 0 if no transport ring has been defined */
static time_t HeartBeatInterval = 5; /* seconds between heartbeats              */
static int  Debug = 1;               /* debug flag                              */
static int  EWmodule = 1;            /* Earthworm Module flag (0 for standalone)*/

/* Variables for talking to statmgr
 **********************************/
char      Text[150];
pid_t     MyPid;       /* Our own pid, sent with heartbeat for restart purposes */

#define THREAD_STACK_SIZE 8192
time_t MyLastInternalBeat;      /* time of last heartbeat into the local Earthworm ring    */
unsigned  TidHeart;             /* thread id. was type thread_t on Solaris! */

/* Error words used by this module
 *********************************/
#define   ERR_NETWORK       0
#define   ERR_NOMENU        1
#define   ERR_TIMETRACK     2
#define   ERR_FILEWRITE     3

static Global BStruct;               /* Private area for the threads            */
double  DataArr[MAXTRACELTH];        /* Trace array                             */
double  wind[MAXTRACELTH];           /* Trace array                             */
double  cdat[MAXTRACELTH][2];        /* Complex Trace array                     */
double  outArr[MAXTRACELTH];         /* Trace array                             */

/* Other globals
 ***************/
#define   XLMARGIN   0.7             /* Margin to left of axes                  */
#define   XRMARGIN   1.0             /* Margin to right of axes                 */
#define   TRACSIZE   1.0             /* Width of panel for wiggle trace         */
#define   YBMARGIN   1.0             /* Margin at bottom of axes                */
#define   YTMARGIN   0.9             /* Margin at top of axes plus size of logo */
double    YBMargin = 1.0;            /* Margin at bottom of axes                */
double    YTMargin = 0.7;            /* Margin at top of axes                   */
double    sec1970 = 11676096000.00;  /* # seconds between Carl Johnson's        */
                                     /* time 0 and 1970-01-01 00:00:00.0 GMT    */
char      module[512] = "sgram.d";
static double    xsize, ysize, plot_up;

/*************************************************************************
 *  main( int argc, char **argv )                                        *
 *************************************************************************/

int main( int argc, char **argv )
{
    char    whoami[512];
    char    *tmp;
    time_t  atime, now;
    int     i, j;

    /* Check command line arguments
     ******************************/
    if ( argc != 2 ) {
        fprintf( stderr, "Usage: %s <configfile>\n", argv[0]);
        fprintf( stderr, "Version: %s\n", SGRAM_VERSION);
        exit( 0 );
    }

    /* Zero the wave server arrays *
     *******************************/
    for (i=0; i< MAX_WAVESERVERS; i++) {
        memset( BStruct.wsIp[i],      0, MAX_ADRLEN);
        memset( BStruct.wsPort[i],    0, MAX_ADRLEN);
        memset( BStruct.wsComment[i], 0, MAX_ADRLEN);
    }
    BStruct.UpdateInt = 120;
    /* did not handle subdirectory usage in .d files */
#ifdef _WINNT
    if ( (tmp = strrchr(argv[1], '\\')) != NULL ) strcpy(module, tmp+1);
#else
    if ( (tmp = strrchr(argv[1], '/')) != NULL ) strcpy(module, tmp+1);
#endif
    else
      strcpy(module, argv[1]);
    for(i=0;i<(int)strlen(module);i++) if(module[i]=='.') module[i] = 0;
    strcpy(BStruct.mod, module);
    sprintf(whoami, " %s: %s: ", module, "main");
    BStruct.nPlots   = 0;

    /* Get our own Pid for restart purposes
    ***************************************/
    MyPid = getpid();
    if( MyPid == -1 ) {
        fprintf( stderr,"%s Cannot get pid. Exiting.\n", whoami);
        return -1;
    }

    /* Initialize name of log-file & open it
     ***************************************/
    logit_init( argv[1], 0, 256, 1 );

    /* Read the configuration file(s)
     ********************************/
    config_me( argv[1], &BStruct );
    logit( "" , "%s Read command file <%s>\n Starting Version %s", 
	whoami, argv[1], SGRAM_VERSION );

    /* Look up important info from earthworm.h tables
     ************************************************/
    if(EWmodule) lookup_ew();

    /* Reinitialize logit to the desired logging level
     *************************************************/
    LogSwitch = EWmodule? LogSwitch:0;
    logit_init( argv[1], (short) MyModId, 256, LogSwitch );

    /* DEBUG: dump variables to stdout
    **********************************/
    if(LogSwitch) {
        logit("","%s MyModuleId: %s  InRingName: %s\n",    whoami, MyModuleId, InRingName);
    }

    /* Attach to public HYPO shared memory ring
     ******************************************/
    Transport = 0;
    if(EWmodule) {
    tport_attach( &InRegion, InRingKey );
        Transport = 1;
    if (LogSwitch) logit( "", "%s Attached to public memory region <%s>: %ld.\n",
                              whoami, InRingName, InRegion.key );
#ifdef _SOLARIS                    /* SOLARIS ONLY:                         */
    if(BStruct.Debug) logit( "e", "%s Attached to public memory region <%s> key: %ld mid: %ld sid: %ld.\n",
                              whoami, InRingName, InRegion.key, InRegion.mid, InRegion.sid );
    if(BStruct.Debug) logit( "e", "%s nbytes: %ld keymax: %ld keyin: %ld keyold: %ld \n",
                              whoami, InRegion.addr->nbytes,
                              InRegion.addr->keymax, InRegion.addr->keyin,
                              InRegion.addr->keyold );
#endif                             /*                                       */
    }

   /* Start the heartbeat thread
   ****************************/
    time(&MyLastInternalBeat); /* initialize our last heartbeat time */

    if ( StartThread( Heartbeat, (unsigned)THREAD_STACK_SIZE, &TidHeart ) == -1 ) {
        logit( "et","%s Error starting Heartbeat thread. Exiting.\n", whoami );
        tport_detach( &InRegion );
        return -1;
    }

	/* sleep for 2 seconds to allow heart to beat so statmgr gets it.  */

	sleep_ew(2000);

    Get_Sta_Info(&BStruct);
    for(i=0;i<BStruct.nPlots;i++) Put_Sta_Info(i, &BStruct);

    CommentList(&BStruct);
    IndexList(&BStruct);

    if(!Build_Menu (&BStruct)) {
        logit("et", "%s No Menu! Just quit.\n", whoami);
        for(i=0;i<BStruct.nServer;i++) wsKillMenu(&(BStruct.menu_queue[i]));
        ewmod_status( TypeError, ERR_NOMENU, "" );
        exit(-1);
    }

    for(i=0;i<BStruct.nPlots;i++) {
        BStruct.Current_Plot = i;

        Put_Sta_Info(i, &BStruct);

        SetUp(&BStruct);
    }
  /* Kill the used menus */
    for(i=0;i<BStruct.nServer;i++) wsKillMenu(&(BStruct.menu_queue[i]));
    if(BStruct.OneDayOnly) {
        logit("et", "%s Exiting as result of flag.\n",  whoami);
        exit(0);
    }

    /* ------------------------ start working loop -------------------------*/
    while(1) {

        logit("et", "%s Updating Images.\n",  whoami);

        if(!Build_Menu (&BStruct)) {
            logit("e", "%s No Menu! Wait 5 minutes and try again.\n", whoami);
            for(j=0;j<60;j++) {
                sleep_ew( 5000 );       /* wait around */
            }
            BStruct.NoMenuCount += 1;
            if(BStruct.NoMenuCount > 5) {
                logit("et", "%s No menu in 5 consecutive trys. Exiting.\n", whoami);
                for(j=0;j<BStruct.nServer;j++) wsKillMenu(&(BStruct.menu_queue[j]));
                if(BStruct.Debug) {
                    sleep_ew( 500 );
                } else {
                    ewmod_status( TypeError, ERR_NOMENU, "" );
                    exit(-1);
                }
            }
        }
        else {
            if(BStruct.Debug) logit("e", "%s Got a Menu! \n", whoami);
            BStruct.NoMenuCount = 0;

            for(i=0;i<BStruct.nPlots;i++) {
                BStruct.Current_Plot = i;

                Put_Sta_Info(i, &BStruct);

                UpDate(&BStruct);
                /* see if a termination has been requested
                   ***************************************/
                if(Transport) {
	                if ( tport_getflag( &InRegion ) == TERMINATE ||
	                     tport_getflag( &InRegion ) == MyPid ) {      /* detach from shared memory regions*/
	                    sleep_ew( 500 );       /* wait around */
	                    tport_detach( &InRegion );
	                    logit("et", "%s Termination requested; exiting.\n", whoami);
	                    fflush(stdout);
	                    exit( 0 );
	                }
                }
            }
          /* Kill the used menus */
            for(i=0;i<BStruct.nServer;i++) wsKillMenu(&(BStruct.menu_queue[i]));

            time(&atime);
            do {
                /* see if a termination has been requested */
                /* *************************************** */
                if(Transport) {
	                if ( tport_getflag( &InRegion ) == TERMINATE ||
	                     tport_getflag( &InRegion ) == MyPid ) {      /* detach from shared memory regions*/
	                    sleep_ew( 500 );       /* wait around */
	                    tport_detach( &InRegion );
	                    logit("et", "%s Termination requested; exiting.\n", whoami);
	                    fflush(stdout);
	                    exit( 0 );
	                }
                }
                sleep_ew( 5000 );       /* wait around */
            } while(time(&now)-atime < BStruct.UpdateInt);
        }
    }   /*------------------------------end of working loop------------------------------*/
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
        if (difftime(now,MyLastInternalBeat) > (double)HeartBeatInterval) {
            ewmod_status( TypeHeartBeat, 0, "" );
            time(&MyLastInternalBeat);
        }
    }
}


/********************************************************************
 *  SetUp does the initial setup and first set of images            *
 ********************************************************************/

void SetUp(Global *But)
{
    char    whoami[512], time1[25], time2[25], sip[25], sport[25], sid[50];
    double  tankStarttime, tankEndtime, EndPlotTime, LocalTimeOffset;
    double  StartTime, EndTime, Duration, ZTime;
    time_t  current_time;
    int     i, j, k, jj, successful, server, ForceRebuild, hour1, hour2;
    int     first_gulp, UseLocal, LocalSecs, mins, secsPerGulp, secsPerStep;

    int     minPerStep, minOfDay;
    TStrct  t0, Time1, Time2;

    sprintf(whoami, " %s: %s: ", But->mod, "SetUp");
    first_gulp = 1;
    i = But->Current_Plot;

    UseLocal    = But->plt[i].UseLocal;
    LocalSecs   = But->plt[i].LocalSecs;
    mins        = But->plt[i].mins;
    secsPerGulp = But->plt[i].secsPerGulp;
    secsPerStep = But->plt[i].secsPerStep;
    ForceRebuild = But->BuildOnRestart;
    But->xsize = But->plt[i].xpix/72.0 + XLMARGIN + XRMARGIN;
    But->ysize = But->plt[i].ypix/72.0 + YBMARGIN + YTMargin;
    if(But->plt[i].Scale != 0.0) But->xsize += TRACSIZE;
    But->mins  = But->plt[i].mins;
    But->axexmax = But->plt[i].xpix/72.0;
    But->axeymax = But->plt[i].ypix/72.0;
    But->LinesPerHour = But->plt[i].LinesPerHour;
    But->HoursPerPlot = But->plt[i].HoursPerPlot;

    SetPlot(But->xsize, But->ysize);
    LocalTimeOffset = UseLocal? LocalSecs:0.0;

    StartTime = time(&current_time) - 120.0;
    Decode_Time(StartTime, &t0);
    if(IsDST(t0.Year,t0.Month,t0.Day) && (But->plt[i].UseDST || But->UseDST)) {
        LocalTimeOffset = LocalTimeOffset + 3600;
    }
/*    if(But->DaysAgo > 0) StartTime -= But->DaysAgo*60*60*But->HoursPerPlot;    */
    if(But->DaysAgo > 0) StartTime -= But->DaysAgo*60*60*24;
    Decode_Time(StartTime, &t0);
    t0.Sec = 0;
    minPerStep = secsPerStep/60;
    minOfDay = minPerStep*((t0.Hour*60 + t0.Min)/minPerStep) - minPerStep;
    if(minOfDay<0) minOfDay = 0;
    t0.Hour = minOfDay/60;
    t0.Min = minOfDay - t0.Hour*60;
    if(But->DaysAgo > 0) t0.Hour = t0.Min = 0;
    Encode_Time( &StartTime, &t0);    /* StartTime modulo step */

    /* If we are requesting older data, make sure we are on the current page.  */
    Decode_Time(StartTime + LocalTimeOffset, &Time1);
    hour1 = But->plt[i].HoursPerPlot*((24*Time1.Day + Time1.Hour)/But->plt[i].HoursPerPlot);
    StartTime = StartTime - But->plt[i].OldData*60*60;
    Decode_Time(StartTime + LocalTimeOffset, &Time2);
    hour2 = But->plt[i].HoursPerPlot*((24*Time2.Day + Time2.Hour)/But->plt[i].HoursPerPlot);

    if(hour1 != hour2) {
        Time1.Hour = But->plt[i].HoursPerPlot*(Time1.Hour/But->plt[i].HoursPerPlot);
        Time1.Min = 0;
        Time1.Sec = 0.0;
        Encode_Time( &StartTime, &Time1);
        StartTime = StartTime - LocalTimeOffset;
        ForceRebuild = 1;
    }

    if(In_Menu_list(But)) {

        /* Make sure that we aren't trying to get data that has been lapped  */
        for(j=0;j<But->nentries;j++) {
            k = But->index[j];
            But->TStime[k] += 300;
            Decode_Time( But->TStime[k], &t0);
            t0.Sec = 0;
            t0.Min = mins*(t0.Min/mins) + mins;
            Encode_Time( &But->TStime[k], &t0);
        }

        Sort_Servers(But, StartTime);
        if(But->nentries <= 0) goto quit;

        k = But->index[0];
        tankStarttime = But->TStime[k];
        tankEndtime   = But->TEtime[k];

        if(But->Debug) {
            date22 (StartTime, time1);
            logit("e", "%s Requested Start Time: %s. \n",
                  whoami, time1);
            date22 (tankStarttime, time1);
            date22 (tankEndtime,   time2);
            strcpy(sip,   But->wsIp[k]);
            strcpy(sport, But->wsPort[k]);
            strcpy(sid,   But->wsComment[k]);
            logit("e", "%s Got menu for: %s. %s %s %s %s <%s>\n",
                  whoami, But->plt[i].SCNtxt, time1, time2, sip, sport, sid);
            for(j=0;j<But->nentries;j++) {
                k = But->index[j];
                date22 (But->TStime[k], time1);
                date22 (But->TEtime[k], time2);
                logit("e", "            %d %d %s %s %s %s <%s>\n",
                      j, k, time1, time2, But->wsIp[k], But->wsPort[k], But->wsComment[k]);
            }
        }

        if(StartTime < tankStarttime) {
            StartTime = tankStarttime;
            Decode_Time(StartTime, &t0);
            t0.Sec = 0;
            minPerStep = secsPerStep/60;
            minOfDay = minPerStep*((t0.Hour*60 + t0.Min)/minPerStep);
            if(minOfDay<0) minOfDay = 0;
            t0.Hour = minOfDay/60;
            t0.Min = minOfDay - t0.Hour*60;
            Encode_Time( &StartTime, &t0);
        }

        Decode_Time(StartTime + LocalTimeOffset, &t0);
        t0.Min = 0;
        t0.Sec = 0.0;
        t0.Hour = But->plt[i].HoursPerPlot*(t0.Hour/But->plt[i].HoursPerPlot);
        But->plt[i].CurrentHour = t0.Hour;
        But->plt[i].CurrentDay  = t0.Day;
        But->plt[i].LastTime = StartTime;

        Duration = secsPerGulp;
        EndTime = StartTime + Duration;
        Encode_Time( &ZTime, &t0);

        if(Build_Axes(But, ZTime, ForceRebuild, &(But->plt[i]))) goto quit;
        AddDay(But);

        EndPlotTime = tankEndtime;
        while(EndTime < EndPlotTime) {
            if(But->Debug) {
                date22 (StartTime, time1);
                date22 (EndTime,   time2);
                logit("e", "%s Data for: %s. %s %s %s %s <%s>\n",
                     whoami, But->plt[i].SCNtxt, time1, time2, sip, sport, sid);
            }

            /* Try to get some data
            ***********************/
            for(jj=0;jj<But->nentries;jj++) {
                server = But->index[jj];
                successful = RequestWave(But, server, DataArr, But->plt[i].Site,
                    But->plt[i].Comp, But->plt[i].Net, But->plt[i].Loc, StartTime, Duration);
                if(successful == 1) {   /*    Plot this trace to memory. */
                    break;
                }
                else if(successful == 2) {
                    if(But->Debug) {
                        logit("e", "%s Data for: %s. RequestWave error 2\n",
                                whoami, But->plt[i].SCNtxt);
                    }
                    continue;
                }
                else if(successful == 3) {   /* Gap in data */
                    if(But->Debug) {
                        logit("e", "%s Data for: %s. RequestWave error 3\n",
                                whoami, But->plt[i].SCNtxt);
                    }

                }
            }

            if(successful == 1) {   /*    Plot this trace to memory. */
                if(first_gulp) {
                    But->plt[i].DCcorr = But->Mean;
                    first_gulp = 0;
                }
                Plot_Trace(But, DataArr, StartTime);
            }

            StartTime += secsPerStep;

            Decode_Time(StartTime + LocalTimeOffset, &t0);
            hour1 = But->plt[i].HoursPerPlot*(t0.Hour/But->plt[i].HoursPerPlot) + 24*t0.Day;
            hour2 = But->plt[i].CurrentHour + 24*But->plt[i].CurrentDay;
            if(hour1 != hour2) {
                if(But->OneDayOnly > 0) break;
                t0.Min = 0;
                t0.Sec = 0.0;
                t0.Hour = But->plt[i].HoursPerPlot*(t0.Hour/But->plt[i].HoursPerPlot);
                But->plt[i].CurrentHour = t0.Hour;
                But->plt[i].CurrentDay = t0.Day;
                Encode_Time( &ZTime, &t0);
                StartTime = ZTime - LocalTimeOffset;
                Save_Plot(But, &(But->plt[i]));

                if(Build_Axes(But, ZTime, 1, &(But->plt[i]))) goto quit;
                AddDay(But);
            }
            But->plt[i].LastTime = StartTime;
            Duration = secsPerGulp;
            EndTime = StartTime + Duration;
        }
        Save_Plot(But, &(But->plt[i]));
    } else {
        logit("e", "%s %s not in menu.\n", whoami, But->plt[i].SCNtxt);
    }
quit:
    sleep_ew(200);
}


/********************************************************************
 *  UpDate does the image updates                                   *
 *                                                                  *
 ********************************************************************/

#define SGRAM_STR_SIZE 512

void UpDate(Global *But)
{
    char    whoami[512], time1[25], time2[25], sip[25], sport[25], sid[50];
    char    string[SGRAM_STR_SIZE];
    double  tankStarttime, tankEndtime, EndPlotTime, LocalTimeOffset;
    double  StartTime, EndTime, Duration, ZTime;
    time_t  current_time;
    int     i, j, k, jj, successful = 0, server, ForceRebuild, hour1, hour2;
    int     UseLocal, LocalSecs, mins, secsPerGulp, secsPerStep;
    int     minPerStep, minOfDay;
    TStrct  t0, Time1;

    sprintf(whoami, " %s: %s: ", But->mod, "UpDate");
    i = But->Current_Plot;
    date22 (But->plt[i].LastTime, time1);

    UseLocal    = But->plt[i].UseLocal;
    LocalSecs   = But->plt[i].LocalSecs;
    mins        = But->plt[i].mins;
    secsPerGulp = But->plt[i].secsPerGulp;
    secsPerStep = But->plt[i].secsPerStep;
    ForceRebuild = 0;

    But->xsize = But->plt[i].xpix/72.0 + XLMARGIN + XRMARGIN;
    But->ysize = But->plt[i].ypix/72.0 + YBMARGIN + YTMargin;
    if(But->plt[i].Scale != 0.0) But->xsize += TRACSIZE;
    But->mins = But->plt[i].mins;
    But->axexmax = But->plt[i].xpix/72.0;
    But->axeymax = But->plt[i].ypix/72.0;
    But->LinesPerHour = But->plt[i].LinesPerHour;
    But->HoursPerPlot = But->plt[i].HoursPerPlot;

    SetPlot(But->xsize, But->ysize);
    LocalTimeOffset = UseLocal? LocalSecs:0.0;

    if(In_Menu_list(But)) {
        if(But->plt[i].LastTime==0) {
            logit("et", "%s %s LastTime came up 0! %s\n", whoami, But->plt[i].SCNtxt, time1);
            But->plt[i].LastTime = (double)time(&current_time);
            sprintf(string, "LastTime==0 %s", But->plt[i].SCNtxt);
         /*
            ewmod_status( TypeError, ERR_TIMETRACK, string );
         */
        }

        if(fabs(But->plt[i].LastTime-time(&current_time)) > 86400) {
            date22 (But->plt[i].LastTime, time1);
            logit("et", "%s %s LastTime came up %f %s!\n",
                whoami, But->plt[i].SCNtxt, But->plt[i].LastTime, time1);
            But->plt[i].LastTime = (double)time(&current_time);
            sprintf(string, "Last time too far away. Dead channel? %s", But->plt[i].SCNtxt);
         /*
            ewmod_status( TypeError, ERR_TIMETRACK, string );
         */
        }

        Decode_Time( But->plt[i].LastTime, &Time1);
        if(Time1.Year < 2002) {
            logit("et", "%s Bad year (%.4d) in start time for %s. Correct to current.\n",
                    whoami, Time1.Year, But->plt[i].SCNtxt);
            sprintf(string, "Update: bad year %s", But->plt[i].SCNtxt);
            ewmod_status( TypeError, ERR_TIMETRACK, string );
            But->plt[i].LastTime = (double)(time(&current_time) - But->secsPerStep);
        }
        StartTime = But->plt[i].LastTime;   /* Earliest time needed. */

        Sort_Servers(But, StartTime);
        if(But->nentries <= 0) goto quit;

        k = But->index[0];
        tankStarttime = But->TStime[k];
        tankEndtime   = But->TEtime[k];

        if(But->Debug) {
            date22 (tankStarttime, time1);
            date22 (tankEndtime,   time2);
            strcpy(sip,   But->wsIp[k]);
            strcpy(sport, But->wsPort[k]);
            strcpy(sid,   But->wsComment[k]);
            logit("e", "%s Got menu for: %s. %s %s %s %s <%s>\n",
                  whoami, But->plt[i].SCNtxt, time1, time2, sip, sport, sid);
            for(j=0;j<But->nentries;j++) {
                k = But->index[j];
                date22 (But->TStime[k], time1);
                date22 (But->TEtime[k], time2);
                logit("e", "            %d %d %s %s %s %s <%s>\n",
                      j, k, time1, time2, But->wsIp[k], But->wsPort[k], But->wsComment[k]);
            }
        }

        if(StartTime < tankStarttime) {
            StartTime = tankStarttime;
            Decode_Time(StartTime, &t0);
            t0.Sec = 0;
            minPerStep = secsPerStep/60;
            minOfDay = minPerStep*((t0.Hour*60 + t0.Min)/minPerStep);
            if(minOfDay<0) minOfDay = 0;
            t0.Hour = minOfDay/60;
            t0.Min = minOfDay - t0.Hour*60;
            Encode_Time( &StartTime, &t0);
        }

        Decode_Time( StartTime, &t0);
        if(IsDST(t0.Year,t0.Month,t0.Day) && (But->plt[i].UseDST || But->UseDST)) {
            LocalTimeOffset = LocalTimeOffset + 3600;
        }
        Decode_Time(StartTime + LocalTimeOffset, &t0);
        t0.Min = 0;
        t0.Sec = 0.0;
        t0.Hour = But->plt[i].HoursPerPlot*(t0.Hour/But->plt[i].HoursPerPlot);
        hour1 = But->plt[i].HoursPerPlot*(t0.Hour/But->plt[i].HoursPerPlot) + 24*t0.Day;
        hour2 = But->plt[i].CurrentHour + 24*But->plt[i].CurrentDay;
        if(hour1 != hour2) {
            But->plt[i].CurrentHour = t0.Hour;
            But->plt[i].CurrentDay  = t0.Day;
                Encode_Time( &ZTime, &t0);
            StartTime = ZTime - LocalTimeOffset;
            ForceRebuild = 1;
        }

        Duration = secsPerGulp;
        EndTime = StartTime + Duration;
        Encode_Time( &ZTime, &t0);

        if(Build_Axes(But, ZTime, ForceRebuild, &(But->plt[i]))) goto quit;
        AddDay(But);

        EndPlotTime = tankEndtime;
        while(EndTime < EndPlotTime) {
            if(But->Debug) {
                date22 (StartTime, time1);
                date22 (EndTime,   time2);
                logit("e", "%s Data for: %s. %s %s %s %s <%s>\n",
                     whoami, But->plt[i].SCNtxt, time1, time2, sip, sport, sid);
            }

            /* Try to get some data
            ***********************/
            for(jj=0;jj<But->nentries;jj++) {
                server = But->index[jj];
                successful = RequestWave(But, server, DataArr, But->plt[i].Site,
                    But->plt[i].Comp, But->plt[i].Net, But->plt[i].Loc, StartTime, Duration);
                if(successful == 1) {   /*    Plot this trace to memory. */
                    break;
                }
                else if(successful == 2) {
                    if(But->Debug) {
                        logit("e", "%s Data for: %s. RequestWave error 2\n",
                            whoami, But->plt[i].SCNtxt);
                    }
                   continue;
                }
                else if(successful == 3) {   /* Gap in data */
                    if(But->Debug) {
                        logit("e", "%s Data for: %s. RequestWave error 3\n",
                            whoami, But->plt[i].SCNtxt);
                    }
                }
            }

            if(successful == 1) {   /*    Plot this trace to memory. */
                Plot_Trace(But, DataArr, StartTime);
            }

            StartTime += secsPerStep;

            Decode_Time(StartTime + LocalTimeOffset, &t0);
            hour1 = But->plt[i].HoursPerPlot*(t0.Hour/But->plt[i].HoursPerPlot) + 24*t0.Day;
            hour2 = But->plt[i].CurrentHour + 24*But->plt[i].CurrentDay;
            if(hour1 != hour2) {
                t0.Min = 0;
                t0.Sec = 0.0;
                t0.Hour = But->plt[i].HoursPerPlot*(t0.Hour/But->plt[i].HoursPerPlot);
                But->plt[i].CurrentHour = t0.Hour;
                But->plt[i].CurrentDay = t0.Day;
                Encode_Time( &ZTime, &t0);
                StartTime = ZTime - LocalTimeOffset;
                Save_Plot(But, &(But->plt[i]));

                if(Build_Axes(But, ZTime, 1, &(But->plt[i]))) goto quit;
                AddDay(But);
            }
            But->plt[i].LastTime = StartTime;
            Duration = secsPerGulp;
            EndTime = StartTime + Duration;
        }
        Save_Plot(But, &(But->plt[i]));
    } else {
        logit("e", "%s %s not in menu.\n", whoami, But->plt[i].SCNtxt);
    }
quit:
    sleep_ew(200);
}


/*************************************************************************
 *   Sort_Servers                                                        *
 *      From the table of waveservers containing data for the current    *
 *      SCN, the table is re-sorted to provide an intelligent order of   *
 *      search for the data.                                             *
 *                                                                       *
 *      The strategy is to start by dividing the possible waveservers    *
 *      into those which contain the requested StartTime and those which *
 *      don't.  Those which do are retained in the order specified in    *
 *      the config file allowing us to specify a preference for certain  *
 *      waveservers.  Those waveservers which do not contain the         *
 *      requested StartTime are sorted such that the possible data       *
 *      retrieved is maximized.                                          *
 *************************************************************************/

void Sort_Servers (Global *But, double StartTime)
{
    char    whoami[512], c22[25];
    double  tdiff[MAX_WAVESERVERS*2];
    int     j, k, jj, last_jj, kk, hold, index[MAX_WAVESERVERS*2];

    sprintf(whoami, " %s: %s: ", But->mod, "Sort_Servers");
        /* Throw out servers with data too old. */
    j = 0;
    while(j<But->nentries) {
        k = But->index[j];
        if(StartTime > But->TEtime[k]) {
            if(But->Debug) {
                date22( StartTime, c22);
                logit("e","%s %d %d  %s", whoami, j, k, c22);
                    logit("e", " %s %s <%s>\n",
                          But->wsIp[k], But->wsPort[k], But->wsComment[k]);
                date22( But->TEtime[k], c22);
                logit("e","ends at: %s rejected.\n", c22);
            }
            But->nentries -= 1;
            for(jj=j;jj<But->nentries;jj++) {
                But->index[jj] = But->index[jj+1];
            }
        } else j++;
    }
    if(But->nentries <= 1) return;  /* nothing to sort */

    /* Calculate time differences between StartTime needed and tankStartTime */
    /* And copy positive values to the top of the list in the order given    */
    jj = 0;
    for(j=0;j<But->nentries;j++) {
        k = index[j] = But->index[j];
        tdiff[k] = StartTime - But->TStime[k];
        if(tdiff[k]>=0) {
            But->index[jj++] = index[j];
            tdiff[k] = -65000000; /* two years should be enough of a flag */
        }
    }
    last_jj = jj;

    /* Sort the index list copy in descending order */
    j = 0;
    do {
        k = index[j];
        for(jj=j+1;jj<But->nentries;jj++) {
            kk = index[jj];
            if(tdiff[kk]>tdiff[k]) {
                hold = index[j];
                index[j] = index[jj];
                index[jj] = hold;
            }
            k = index[j];
        }
        j += 1;
    } while(j < But->nentries);

    /* Then transfer the negatives */
    for(j=last_jj,k=0;j<But->nentries;j++,k++) {
        But->index[j] = index[k];
    }
}


/********************************************************************
 *    Build_Axes constructs the axes for the plot by drawing the    *
 *    GIF image in memory.                                          *
 *                                                                  *
 ********************************************************************/

short Build_Axes(Global *But, double Stime, int ForceRebuild, PltPar *plt)
{
    char    whoami[512], c22[30], cstr[150], LocalTimeID[4];
    double  xmax, ymax;
    int     zmins;
    int     step, label, tic, minutes, hours;
    int     mins, LinesPerHour, HoursPerPlot, UseLocal, ShowUTC, CurrentHour, LocalTime, Pal;
    int     ix, iy, i, j, k, kk;
    int     xgpix, ygpix, LocalSecs;
    long    black, must_create;
    FILE    *in;
    TStrct  Time1;
    gdImagePtr    im_in;

    sprintf(whoami, " %s: %s: ", But->mod, "Build_Axes");
    i = But->Current_Plot;
    xmax  = But->axexmax;
    ymax  = But->axeymax;
    mins  = But->mins;
    LinesPerHour = But->LinesPerHour;
    HoursPerPlot = But->HoursPerPlot;
    Pal = plt->Pallette;
    UseLocal = plt->UseLocal;
    ShowUTC = plt->ShowUTC;
    CurrentHour = plt->CurrentHour;
    LocalTime = plt->LocalTime;
    LocalSecs = plt->LocalSecs;
    strcpy(LocalTimeID, plt->LocalTimeID);

    Decode_Time( Stime, &Time1);
    if(Time1.Year < 2002) {
        logit("et", "%s Bad year (%.4d) in start time. Exiting.\n", whoami, Time1.Year);
        ewmod_status( TypeError, ERR_TIMETRACK, "Build_Axes lost time" );
        return 1;
    }
    if(IsDST(Time1.Year,Time1.Month,Time1.Day) && (plt->UseDST || But->UseDST)) {
        LocalTime = LocalTime + 1;
        LocalTimeID[1] = 'D';
    }
    sprintf(plt->Today, "%.4d%.2d%.2d", Time1.Year, Time1.Month, Time1.Day);
    sprintf(But->TmpName, "%s%s.%s%.2d", But->Prefix, plt->SCNnam, plt->Today, CurrentHour);
    sprintf(But->GifName, "%s%s", But->TmpName, ".gif");
    sprintf(But->LocalGif, "%s%s.gif", But->GifDir, plt->SCNnam);

    But->GifImage = 0L;
    if(ForceRebuild) {
        must_create = 1;
    } else {
        must_create = 0;
        in = fopen(But->LocalGif, "rb");
        if(!in) must_create = 1;
        if(in) {
            But->GifImage = gdImageCreateFromGif(in);
            fclose(in);
            if(!But->GifImage) must_create = 1;
            else {
                for(j=0;j<MAXCOLORS;j++) gdImageColorDeallocate(But->GifImage, j);
                Pallette(Pal, But->GifImage, But->gcolor);
            }
        }
    }

    if(must_create) {
        xgpix = (int)(But->xsize*72.0) + 8;
        ygpix = (int)(But->ysize*72.0) + 8;

        logit("et", "%s Building new axes for: %s %s %.2d\n",
            whoami, plt->SCNnam, plt->Today, CurrentHour);
        But->GifImage = gdImageCreate(xgpix, ygpix);
        if(But->GifImage==0) {
            logit("e", "%s Not enough memory! Reduce size of image or increase memory.\n\n", whoami);
            return 1;
        }
        if(But->GifImage->sx != xgpix) {
            logit("e", "%s Not enough memory for entire image! Reduce size of image or increase memory.\n",
                 whoami);
            return 1;
        }
        Pallette(Pal, But->GifImage, But->gcolor);
        AddDay(But);
        if(But->logo) {
            in = fopen(But->logoname, "rb");
            if(in) {
                im_in = gdImageCreateFromGif(in);
                fclose(in);
                gdImageCopy(But->GifImage, im_in, 0, 0, 0, 0, im_in->sx, im_in->sy);
                gdImageDestroy(im_in);
            }
        }
    }

    /* Plot the frame *
     ******************/
    Make_Grid(But, plt);

    if(plt->PixPerMin >= 5.0) {
        step = 1;    label = 10;
    } else if(plt->PixPerMin < 5.0 && plt->PixPerMin >= 3.0) {
        step = 10;    label = 10;
    } else if(plt->PixPerMin < 3.0 && plt->PixPerMin >= 1.0) {
        step = 10;    label = 60;
    } else {
        step = 60;    label = 360;
    }

    /* Put in the time tics and labels *
     ***********************************/
    black = But->gcolor[BLACK];
    zmins = Time1.Hour*60 + Time1.Min;
    for(j=0;j<HoursPerPlot*60;j++) {
        tic = 0;
        if(j-step*(j/step)==0) tic = 6;
        if(j-label*(j/label)==0) tic = 12;
        if(tic) {
            ix = ixq(0.0);
            if(plot_up)
                iy = iyq(0.0) - (int)(j*plt->PixPerMin);
            else
                iy = iyq(0.0) + (int)(j*plt->PixPerMin);
            gdImageLine(But->GifImage, ix, iy, ix-tic, iy, black);
            ix = ixq(xmax);
            gdImageLine(But->GifImage, ix, iy, ix+tic, iy, black);
        }
        if(tic==12) {
            minutes = zmins + j;
            hours = minutes/60;
            minutes = minutes - 60*hours;
            k = UseLocal?
                hours:
                hours + LocalTime;
            if(k <  0) k += 24;
            if(k > 23) k -= 24;
            kk = ShowUTC? k - LocalTime: k;
            if(kk <  0) kk += 24;
            if(kk > 23) kk -= 24;

            ix = 0;
            if(plot_up)
                iy = iyq(0.0) - (int)(j*plt->PixPerMin) - 6;
            else
                iy = iyq(0.0) + (int)(j*plt->PixPerMin) - 6;
            sprintf(cstr, "%02d:%02d", k, minutes);
            gdImageString(But->GifImage, gdFontMediumBold, ix, iy, cstr, black);
            ix = ixq(xmax) + tic + 3;
            sprintf(cstr, "%02d:%02d", kk, minutes);
            gdImageString(But->GifImage, gdFontMediumBold, ix, iy, cstr, black);
        }
    }

    iy = (int)(YTMargin*72.0) - 45;
    ix = ixq(-0.5);
    gdImageString(But->GifImage, gdFontMediumBold, ix, iy, LocalTimeID, black);
    ix = ixq(xmax + 0.05);
    if(ShowUTC) sprintf(cstr, "UTC") ;
    else         strcpy(cstr, LocalTimeID);
    gdImageString(But->GifImage, gdFontMediumBold, ix, iy, cstr, black);

    /* Write labels with the Date *
     ******************************/
    iy = (int)(YTMargin*72.0) - 30;
    Decode_Time( Stime, &Time1);
    Time1.Hour = Time1.Min = 0;
    Time1.Sec = 0.0;
    Encode_Time( &Time1.Time, &Time1);
    date22 (Time1.Time, c22);

    sprintf(cstr, "%.11s", c22) ;
    gdImageString(But->GifImage, gdFontMediumBold, ixq(0.0), iy-15, cstr, black);

    /* Write label with the Channel ID *
     ***********************************/
    iy = (int)(YTMargin*72.0) - 45;
    ix = ixq(0.0);
    if(plt->Comment[0]!=0) {
        sprintf(cstr, "(%s) ", plt->Comment);
        ix = ixq(xmax/2) - 6*(int)strlen(cstr)/2;
        gdImageString(But->GifImage, gdFontMediumBold, ix, iy, cstr, black);
        iy -= 15;
    }
    sprintf(cstr, "%s ", plt->SCNtxt);
    ix = ixq(xmax/2) - 6*(int)strlen(cstr)/2;
    gdImageString(But->GifImage, gdFontMediumBold, ix, iy, cstr, black);

    return 0;
}


/********************************************************************
 *    Make_Grid constructs the grid overlayed on the plot.          *
 *                                                                  *
 ********************************************************************/

void Make_Grid(Global *But, PltPar *plt)
{
    char    label[25], str[SGRAM_STR_SIZE];
    double  xmax, ymax, in_sec, tsize;
    long    isec, xp, yp0, yp1, yp2, j, k, black, ixmax;
    long    major_incr, minor_incr, tiny_incr;
    int     inx[]={0,1,2,2,2,3,4,4,4,4,5}, iny[]={2,3,2,1,0,1,1,0,2,3,3};

    /* Plot the frame *
     ******************/
    xmax  = But->axexmax;
    ymax  = But->axeymax;
    black = But->gcolor[BLACK];
    gdImageRectangle( But->GifImage, ixq(0.0), iyq(ymax), ixq(xmax),  iyq(0.0),  black);

    /* Make the x-axis ticks *
     *************************/
    major_incr = (plt->xpix/plt->fmax > 20)? 1:5;
    ixmax = (int)plt->fmax;
    in_sec = xmax / ixmax;
    minor_incr = (100*in_sec < 2.8)?  0: 1;
    tiny_incr = 0;

    yp0 = (int)((ysize - YBMARGIN)*72.0);   /* Bottom axis line */
    yp1 = yp0 + (int)(0.15*72.0);           /* Space below axis for label */
    for(isec=0;isec<=ixmax;isec++) {
        xp = ixq(isec*in_sec);
        if ((div(isec, major_incr)).rem == 0) {
            tsize = 0.15;    /* major ticks */
            sprintf(str, "%ld", isec);
            k = (int)strlen(str) * 3;
            gdImageString(But->GifImage, gdFontMediumBold, xp-k, yp1, str, black);
            gdImageLine(But->GifImage, xp, iyq(0.0), xp, iyq(ymax), But->gcolor[GREY]); /* make the minute mark */
        }
        else if ((div(isec, minor_incr)).rem == 0)
            tsize = 0.10;    /* minor ticks */
        else if(tiny_incr)
            tsize = 0.05;    /*  tiny ticks */
        else
            tsize = 0.0;     /*  no ticks   */

        if(tsize > 0.0) {
            yp2 = yp0 + (int)(tsize*72.0);
            gdImageLine(But->GifImage, xp, yp0, xp, yp2, black); /* make the tick */
        }
    }
    strcpy(label, "FREQUENCY (HZ)");
    yp1 = yp0 + (int)(0.3*72.0);           /* Space below axis for label */
    gdImageString(But->GifImage, gdFontMediumBold, ixq(xmax/2.0 - 0.5), yp1, label, black);

    /* Initial it *
     **************/
    j = (int)(But->ysize*72.0) - 5;
    for(k=0;k<11;k++) gdImageSetPixel(But->GifImage, inx[k]+2, j+iny[k], black);
}


/*******************************************************************************
 *    Pallette defines the pallete to be used for plotting.                    *
 *     PALCOLORS colors are defined.                                           *
 *                                                                             *
 *******************************************************************************/

void Pallette(int ColorFlag, gdImagePtr GIF, long color[])
{
    double  val;
    int     j, r, g, b;

    color[WHITE]  = gdImageColorAllocate(GIF, 255, 255, 255);
    color[BLACK]  = gdImageColorAllocate(GIF, 0,     0,   0);
    color[RED]    = gdImageColorAllocate(GIF, 255,   0,   0);
    color[BLUE]   = gdImageColorAllocate(GIF, 0,     0, 255);
    color[GREEN]  = gdImageColorAllocate(GIF, 0,   105,   0);
    color[GREY]   = gdImageColorAllocate(GIF, 125, 125, 125);
    color[YELLOW] = gdImageColorAllocate(GIF, 125, 125,   0);
    color[TURQ]   = gdImageColorAllocate(GIF, 0,   255, 255);
    color[PURPLE] = gdImageColorAllocate(GIF, 200,   0, 200);

    if(ColorFlag) {
        for(j=0;j<PALCOLORS;j++) {
            val = (float)j*1.0/(float)PALCOLORS;
            if(val <= 0.0) {
                b = (int)(0.625*255);
                color[j] = gdImageColorAllocate(GIF, 0, 0, b);
            }
            if(val > 0.0 && val <= 0.125) {
                b = (int)((val*4.0 + 0.5)*255);
                color[j] = gdImageColorAllocate(GIF, 0, 0, b);
            }
            if(val > 0.125 && val <= 0.375) {
                g = (int)((val*4.0 - 0.5)*255);
                color[j] = gdImageColorAllocate(GIF, 0, g, 255);
            }
            if(val > 0.375 && val <= 0.625) {
                r = (int)((val*4.0 - 1.5)*255);    b = (int)((-val*4.0 + 2.5)*255);
                color[j] = gdImageColorAllocate(GIF, r, 255, b);
            }
            if(val > 0.625 && val <= 0.875) {
                g = (int)((-val*4.0 + 3.5)*255);
                color[j] = gdImageColorAllocate(GIF, 255, g, 0);
            }
            if(val > 0.875 && val <= 1.000) {
                r = (int)((-val*4.0 + 4.5)*255);
                color[j] = gdImageColorAllocate(GIF, r, 0, 0);
            }
        }
        r = (int)(0.625*255);
        color[PALCOLORS] = gdImageColorAllocate(GIF, r, 0, 0);
    } else {
        for(j=0;j<PALCOLORS;j++) {
            color[j] = gdImageColorAllocate(GIF, j, j, j);
        }
        j = PALCOLORS;
        color[PALCOLORS] = gdImageColorAllocate(GIF, j, j, j);
    }
    gdImageColorTransparent(GIF, -1);
}

/*******************************************************************************
 *    Plot_Trace plots an individual trace (Data)  and stuffs it into          *
 *     the GIF image in memory.                                                *
 *                                                                             *
 *******************************************************************************/

int Plot_Trace(Global *But, double *Data, double Stime)
{
    char    whoami[512];
    char    DCFile[512];
    FILE    *out;
    double  x, atime;
    double  vmin, vmax, val = 0.0, dmin = 0.0, dmax, trace_max;
    double  axexmax, axeymax, Scale, DCcorr;
    double  ac[1000];
    double  datmin, datmax, wmean;
    int     mins, LinesPerHour, HoursPerPlot, CurrentHour, UseLocal, LocalSecs;
    int     i, j, k, kk, kkk, ix, iy, LineNumber, npts, ixmin, ixmax, sign;
    int     ii, iii, jj, mm, icenter, nstep;
    long    trace_clr = 0;
    TStrct  StartTime, Time1;

    sprintf(whoami, " %s: %s: ", But->mod, "Plot_Trace");
    i = But->Current_Plot;
    axexmax = But->axexmax;
    axeymax = But->axeymax;
    mins    = But->mins;
    LinesPerHour = But->LinesPerHour;
    HoursPerPlot = But->HoursPerPlot;
    Scale = 1.0;
    DCcorr = But->plt[i].DCcorr;
    CurrentHour  = But->plt[i].CurrentHour;
    UseLocal = But->plt[i].UseLocal;
    LocalSecs = But->plt[i].LocalSecs;
    Decode_Time( Stime, &Time1);
    if(IsDST(Time1.Year,Time1.Month,Time1.Day) && (But->plt[i].UseDST || But->UseDST)) {
        LocalSecs = LocalSecs + 3600;
    }

    atime =  UseLocal? Stime+LocalSecs:Stime;
    atime = atime - 3600.0*CurrentHour;
    Decode_Time( atime, &StartTime);

    LineNumber = LinesPerHour*StartTime.Hour + StartTime.Min/mins;

    /* Build the trace *
     *******************/
    Make_Line(But, Data);

    ix = ixq(0.0);
    if(plot_up) {
        iy = iyq(0.0) - (int)(LineNumber*But->plt[i].PixPerMin);
        j = iyq(But->axeymax);
        k = iyq(0.0);
        sign = -1;
    } else {
        iy = iyq(0.0) + (int)(LineNumber*But->plt[i].PixPerMin);
        j = iyq(0.0);
        k = iyq(But->axeymax);
        sign = +1;
    }

    if(iy > k || iy < j) {
        logit("et", "%s This line is off page: %d %d %d %d %d %d\n", whoami, j, iy, k, LineNumber, StartTime.Hour, StartTime.Min);
        return 1;
    }
    npts = (int)(But->axexmax*72.0);
    if(npts>MAXXPIX) npts = MAXXPIX;

    if(But->plt[i].dminhr > But->plt[i].dmin) But->plt[i].dminhr = But->plt[i].dmin;
    if(But->plt[i].dmaxhr < But->plt[i].dmax) But->plt[i].dmaxhr = But->plt[i].dmax;
    if(StartTime.Min == 0 || StartTime.Min == 15 || StartTime.Min == 30 || StartTime.Min == 45) {
        if(But->SaveDrifts) {
            sprintf(DCFile, "%s%s.dmax", But->GifDir, But->plt[i].SCNnam);
            out = fopen(DCFile, "a");
            if(out == 0L) {
                out = fopen(DCFile, "w");
                if(out != 0L) {
                    fprintf(out, "%.2d %.2d %.4d %.2d:%.2d  %7.0f  %7.0f \n",
                        StartTime.Day, StartTime.Month, StartTime.Year,
                        StartTime.Hour, StartTime.Min, But->plt[i].dminhr, But->plt[i].dmaxhr);
                    fclose(out);
                } else {
                    logit("et", "%s Unable to Open dmax file: %s\n", whoami, DCFile);
                }
            } else {
                fprintf(out, "%.2d %.2d %.4d %.2d:%.2d  %7.0f  %7.0f \n",
                    StartTime.Day, StartTime.Month, StartTime.Year,
                    StartTime.Hour, StartTime.Min, But->plt[i].dminhr, But->plt[i].dmaxhr);
                fclose(out);
            }
        }
        But->plt[i].dminhr = 500000.0;
        But->plt[i].dmaxhr = 0.0;
    }

    dmax = (But->plt[i].amax == 0.0)? But->plt[i].dmax:But->plt[i].amax;
    vmin =  100.0;
    vmax = -100.0;
    if(But->plt[i].nbw==4) {   /* Autocorrelation  */
        dmax = 0.0;
        for(j=1;j<npts;j++) {
            dmax = dmax + But->plt[i].oplot[j]*But->plt[i].oplot[j];
        }
        dmin = dmax;
        kkk = npts/2;
        for(j=1;j<npts;j++) {
            val = 0.0;
            kk = j;
            for(k=1;k<npts;k++) {
                if(kk>=npts) kk = 1;
                val = val + But->plt[i].oplot[k]*But->plt[i].oplot[kk];
                kk += 1;
            }
            kkk = j;
            ac[kkk] = val;
            if(dmin > ac[kkk]) dmin = ac[kkk];
            if(dmax < ac[kkk]) dmax = ac[kkk];
            if(+kkk>=npts) kkk = 1;
        }
    }
    if(But->plt[i].nbw==3) {
        dmax = 0.0;
        for(j=1;j<npts;j++) {
            But->plt[i].oplot[j] = But->plt[i].oplot[j] - But->plt[i].oplot[j-1];
            if(fabs(But->plt[i].oplot[j])>dmax) dmax = fabs(But->plt[i].oplot[j]);
        }
    }
    for(j=1;j<npts;j++) {
        if (j > But->plt[i].xpix) break;
        if(But->plt[i].nbw==1||But->plt[i].nbw==2||But->plt[i].nbw==3||But->plt[i].nbw==4) {
            if(But->plt[i].nbw==1) {
                val = fabs(But->plt[i].oplot[j]/dmax);
            }
            if(But->plt[i].nbw==2) {
                val = log10(But->plt[i].oplot[j]/dmax + 1.0) * 3.3;
                val = fabs(val);
            }
            if(But->plt[i].nbw==3) {
                val = log10(fabs(But->plt[i].oplot[j])/dmax + 1.0) * 3.3;
                val = fabs(val);
            }
            if(But->plt[i].nbw==4) {
                val = log10((ac[j]-dmin)/(dmax-dmin) + 1.0) * 3.3;
                val = log10(10*val);
                val = (ac[j]-dmin)/(dmax-dmin);
                val = fabs(val);
            }
            if(val < 0.0) val = 0.0;
            if(val > 1.0) val = 1.0;
            if(val < vmin) vmin = val;
            if(val > vmax) vmax = val;
            x = val*(PALCOLORS-1);
            k = (int)x;
            trace_clr = But->gcolor[k];
        }
        if(But->plt[i].PixPerLine > 1)
            gdImageLine(But->GifImage, ix+j, iy, ix+j, iy+sign*But->plt[i].PixPerLine, trace_clr);
        else
            gdImageSetPixel(But->GifImage, ix+j, iy, trace_clr);
    }

    trace_clr = But->gcolor[BLACK];
    Scale = 0.01*But->plt[i].Scale;
    trace_max = 0.5*TRACSIZE*72;
    icenter = ixq(But->xsize - 0.5*TRACSIZE - XLMARGIN);

    mm = 60*But->plt[i].mins*But->samp_sec;
    if(mm>But->Npts) mm = But->Npts;
    ii = mm/But->plt[i].PixPerLine;
    j = 0;
    nstep = 0;

    do {
        wmean = 0.0;
        datmin = datmax = Data[j];
        iii = ii;
        if(mm-j < iii) iii = mm-j;
        for(jj=0;jj<iii;jj++) {
            wmean += Data[j]/iii;
            if(Data[j] < datmin) datmin = Data[j];
            if(Data[j] > datmax) datmax = Data[j];
            j += 1;
        }
        datmin = datmin - wmean;
        datmax = datmax - wmean;
        x = datmin*Scale;
        if(x < -trace_max) x = -trace_max;
        if(x >  trace_max) x =  trace_max;
        ixmin = icenter + (int)x;
        x = datmax*Scale;
        if(x < -trace_max) x = -trace_max;
        if(x >  trace_max) x =  trace_max;
        ixmax = icenter + (int)x;
        gdImageLine(But->GifImage, ixmin, iy, ixmax, iy, trace_clr);
        iy += sign;
        nstep += 1;
    } while(j < mm);

    if(But->Debug) {
        logit("e", "%s %s %d %f %f %f %f\n", whoami, But->plt[i].SCNnam, nstep, dmin, dmax, vmin, vmax);
    }

    return 0;
}


/*******************************************************************************
 *    Make_Line builds an individual trace (Data)  and stuffs it into          *
 *     But->oplot in memory. This version breaks the sample into multiple      *
 *     shorter samples, takes FFT of subsamples, and averages in an attempt    *
 *     to provide  better smoothing.                                           *
 *                                                                             *
 *******************************************************************************/

void Make_Line(Global *But, double *Data)
{
    char    whoami[50];
    double  dt, delf, temp, a, b, dmin, dmax, tapr, fndiv, wmean;
    long    mpts, i, j, k, l, m, n;
    int     nwind, nadd, npower, nwind_fft, nxplot, ndiv, start, step, ffts, mute, nsmooth;

    sprintf(whoami, " %s: %s: ", But->mod, "Make_Line");
    i = But->Current_Plot;
    dt = (But->samp_sec==0)? 0.01:1.0/But->samp_sec;
    nwind = But->Npts;
    nwind = But->Npts/2;
    nwind = (int)(20.0/dt);
    nwind = (int)(10.0/dt);

    nxplot = (int)(But->axexmax*72.0);
    if(nxplot>MAXXPIX) nxplot = MAXXPIX;
    do {
        nwind += (int)(10.0/dt);
        set_n_2(nwind, &nadd, &npower);

        nwind_fft = nwind + nadd;
        delf = 1.0/nwind_fft/dt;
        mpts = (int)(But->plt[i].fmax/delf);
        ndiv = mpts/nxplot;
    } while (ndiv==0);
    ndiv += 1;
    fndiv = (But->plt[i].fmax/delf)/(float)nxplot;

    for(j=0;j<nxplot;j++) But->plt[i].oplot[j] = 0.0;

    nsmooth = 0;
    step  = nwind/6;
    step  = nwind/2;
    start = ffts = 0;
    while(start+nwind <= But->Npts) {
        wmean = 0.0;
        for(j=0;j<nwind;j++) {
            wind[j] = Data[j+start];
            wmean += Data[j+start]/nwind;
        }
        for(j=0;j<nwind;j++) wind[j] = wind[j] - wmean;
        for(j=nwind;j<nwind_fft;j++) wind[j] = 0.0;

        /* Removing trend */
        fit(dt, wind, nwind, &a, &b);
        for(j=0;j<nwind;j++) wind[j] = wind[j] - a - b*j*dt;

        /* Tapering */
        tapr = 0.05;
        taper(wind, nwind, tapr);

        /* Make Complex */
        for(j=0;j<nwind;j++) {
            cdat[j][0] = wind[j];
            cdat[j][1] = 0.0;
        }
         for(j=nwind;j<nwind_fft;j++) cdat[j][0] = cdat[j][1] = 0.0;

        /* FFT */
        cool(npower, &cdat[0][0], -1);
        for(j=0;j<nwind_fft/2;j++) outArr[j] = Cabs(cdat[j][0], cdat[j][1]);

        for(j=0;j<nxplot;j++) {
            m = (int)(j*fndiv);
            temp = outArr[m];
            n = 1;
            for(k=1;k<ndiv+nsmooth;k++) {
                l = m + k;
                temp += outArr[l];
                n += 1;
            }
            But->plt[i].oplot[j] += temp/n;
        }
        ffts += 1;
        start += step;
    }

    dmin = 50000000.0;
    dmax = 0.0;
    mute = (int)(nxplot*But->plt[i].fmute/But->plt[i].fmax);
    for(j=0;j<mute;j++) But->plt[i].oplot[j] = 0;
    for(j=mute;j<nxplot;j++) {
        if(But->plt[i].oplot[j] > dmax) dmax = But->plt[i].oplot[j];
        if(But->plt[i].oplot[j] < dmin) dmin = But->plt[i].oplot[j];
    }
    if(But->Debug) {
        logit("e", " %s %f %f %d %d %d %d %d %f %d\n",
        whoami, dmin, dmax, ffts, nwind, nwind_fft, nxplot, ndiv, fndiv, mpts);
    }

    But->plt[i].dmin = dmin;
    But->plt[i].dmax = dmax;

}


/********************************************************************
 * Cabs(z.r, z.i)                                                   *
 * Returns the absolute value (modulus) of a complex number.        *
*********************************************************************/

double Cabs(double zr, double zi)
{
    double    x, y, ans, temp;

    x = fabs(zr);
    y = fabs(zi);
    if(x == 0.0)        ans = y;
    else if(y == 0.0)    ans = x;
    else if(x > y) {
        temp = y/x;
        ans = x*sqrt(1.0+temp*temp);
    } else {
        temp = x/y;
        ans = y*sqrt(1.0+temp*temp);
    }
    return ans;
}

/********************************************************************
 *  fit defines the linear trend of the data                        *
 *      x[i] = i*dt                                                 *
 ********************************************************************/

void fit(double dt, double *y, int ndata, double *a, double *b)
{
    double    sx, sy, st2, ss, sxoss, t;
    int        i;

    sx = sy = st2 = *b = 0.0;
    for(i=0;i<ndata;i++) sx += i*dt;
    ss = (double)ndata;
    sxoss = sx/ss;
    for(i=0;i<ndata;i++) {
        t = i*dt - sxoss;
        st2 += t*t;
        *b  += t*y[i];
        sy  += y[i];
    }
    *b = *b/st2;
    *a = (sy-sx*(*b))/ss;
}

/********************************************************************
 *  taper applies a Hanning taper to the data                       *
 *                                                                  *
 ********************************************************************/

void taper(double *data, int npts, double width)
{
    double    pi, f0, f1, omega, factor;
    int        i, ntaper;

    ntaper = (int)(npts*width);
    pi = 3.14159265;
    f0 = f1 = 0.5;
    omega = pi/ntaper;
    for(i=0;i<ntaper;i++) {
        factor = f0 - f1*cos(omega*i);
        data[i] = data[i]*factor;
        data[npts-1-i] = data[npts-1-i]*factor;
    }
}

/********************************************************************
 *  set_n_2                                                         *
 *  Setting number of samples for the fft powers of 2, which is     *
 *  closest to the original n.                                      *
 *                                                                  *
 *  n      - (input) number of samples                              *
 *  nadd   - (output) number of samples to add                      *
 *  npower - (output) power of 2                                    *
 *                                                                  *
 *  n + nadd = 2**npower                                            *
 *                                                                  *
 ********************************************************************/

void set_n_2(int n, int *nadd, int *npower)
{
    int    i;

    *nadd = 0;
    if(check_2(n, npower)) return;
    for(i=0;i<n;i++) {
        if(check_2(n+i, npower)) return;
        *nadd += 1;
    }

    *nadd = 0;
    for(i=0;i<n;i++) {
        if(check_2(n-i, npower)) return;
        *nadd -= 1;
    }

    return;
}

/********************************************************************
 *  check_2                                                         *
 *  Checks whether or not n is a power of 2                         *
 *                                                                  *
 *  n      - (input) number of samples                              *
 *  npower - (output) power of 2                                    *
 *                                                                  *
 *  return - true for the right n, false for the wrong one          *
 *                                                                  *
 ********************************************************************/

int check_2(int n, int *npower)
{
    int    i;
    double    re, rint;

    if(n > MAXTRACELTH) return 0;
    i = n;
    *npower = 0;
    while(1) {
        re = i/2.0;
        i = i/2;
        rint = i*1.0;
        if(re == 1.0){
            *npower += 1;
            return 1;
        }
        if((re-rint) != 0) return 0;
        *npower += 1;
    }
}


/********************************************************************
 *  COOL fft                                                        *
 *                                                                  *
 ********************************************************************/

void cool(int nn, double *DATAI, int signi)
{
    int        i, j, m, n, mmax, istep;
    double    tempr, tempi, theta, sinth, wstpr, wstpi, wr, wi;

    n = (int)pow(2.0, (double)(nn+1));
    j = 0;

    for(i=0;i<n;i+=2) {
        if(i < j) {
            tempr = DATAI[j];
            tempi = DATAI[j+1];
            DATAI[j]   = DATAI[i];
            DATAI[j+1] = DATAI[i+1];
            DATAI[i]   = tempr;
            DATAI[i+1] = tempi;
        }
        m = n/2;
        do {
            if(j < m) break;
            j = j-m;
            m = m/2;
        } while(m >= 2);
        j += m;
    }

    mmax = 2;
    while(1) {
        if(mmax >= n) break;
        istep = 2*mmax;
        theta = signi*6.28318531/(float)(mmax);
        sinth = sin(theta/2.0);
        wstpr = -2.0  *sinth*sinth;
        wstpi =  sin(theta);
        wr = 1.0;
        wi = 0.0;
        for(m=0;m<mmax;m+=2) {
            for(i=m;i<n;i+=istep) {
                j = i + mmax;
                tempr = wr*DATAI[j]-wi*DATAI[j+1];
                tempi = wr*DATAI[j+1]+wi*DATAI[j];
                DATAI[j] = DATAI[i]-tempr;
                DATAI[j+1] = DATAI[i+1]-tempi;
                DATAI[i]   = DATAI[i]+tempr;
                DATAI[i+1] = DATAI[i+1]+tempi;
            }
            tempr = wr;
            wr = wr*wstpr - wi*wstpi    + wr;
            wi = wi*wstpr + tempr*wstpi + wi;
        }
        mmax = istep;
    }
}


/*********************************************************************
 *   CommentList()                                                   *
 *    Build and send a file relating SCNs to their comments.         *
 *********************************************************************/

void CommentList(Global *But)
{
    char    tname[200], fname[175], whoami[50];
    int     i, j, ierr, jerr;
    FILE    *out;

    sprintf(whoami, " %s: %s: ", module, "CommentList");
    sprintf(tname, "%sznamelist.dat", But->GifDir);

    for(j=0;j<But->nltargets;j++) {
        out = fopen(tname, "wb");
        if(out == 0L) {
            logit("e", "%s Unable to open NameList File: %s\n", whoami, tname);
        } else {
            for(i=0;i<But->nPlots;i++) {
                fprintf(out, "%s. %s.\n", But->plt[i].SCNnam, But->plt[i].Descrip);
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


/*********************************************************************
 *   IndexList()                                                     *
 *    Build the master .html page with pointers for each SCN         *
 *********************************************************************/

void IndexList(Global *But)
{
    char    FileName[200], whoami[50];
    int     i;
    FILE    *in;

    sprintf(whoami, " %s: %s: ", module, "IndexList");

    if(!But->Make_HTML) return;
    /* Make sure all needed history files are in place
     *************************************************/
    for(i=0;i<But->nPlots;i++) {
        sprintf(FileName, "%s%s.hist", But->GifDir, But->plt[i].SCNnam);
        in = fopen(FileName, "r");
        if(in == 0L) {
            in = fopen(FileName, "wb");
            if(in == 0L)
                logit("e", "%s Unable to open History File: %s\n", whoami, FileName);
            else {
                fprintf(in, "\n");
                fclose(in);
            }
        } else fclose(in);
    }
    IndexListUpdate(But);
}


/*********************************************************************
 *   IndexListUpdate()                                               *
 *    Update the master index.html with pointers for each SCN        *
 *********************************************************************/

void IndexListUpdate(Global *But)
{
    char    string[SGRAM_STR_SIZE], datetxt[30], FileName[200], tname[175], fname[175];
    char    History[200], temp[175], whoami[50];
    FILE    *in, *out, *hist;
    int     i, j, ierr, jerr;

    sprintf(whoami, " %s: %s: ", But->mod, "IndexListUpdate");
    /* Build the index.html file
       with the insertion of the SCNs along the way
     *********************************************************/

    sprintf( FileName, "%sindex.html", But->GifDir);   /* current copy of the full index.html file */
    out = fopen(FileName, "wb");
    if(out == 0L)
        logit("e", "%s Unable to open index.html File: %s\n", whoami, FileName);
    else {
        sprintf( temp, "%sindexa.html", But->GifDir);
        in  = fopen(temp, "r");
        if(in == 0L) {
            fprintf(out, "<HTML><HEAD><TITLE>\nRecent Spectrogram Displays\n</TITLE></HEAD>\n");
            fprintf(out, "<BODY BGCOLOR=#EEEEEE TEXT=#333333 vlink=purple>\n");
            fprintf(out, "<A NAME=\"top\"></A><CENTER><H2>\n");
      /*    fprintf(out, "<IMG SRC=\"smusgs.gif\" WIDTH=58 HEIGHT=35 ALT=\"Logo\" ALIGN=\"middle\">\n"); */
            fprintf(out, "<FONT COLOR=red>Recent Spectrogram Displays</FONT></H2><br>\n\n");
        } else {
            while(fgets(string, SGRAM_STR_SIZE-1, in)!=0L) fprintf(out, "%s", string);
            fclose(in);
        }

        fprintf(out, "<STRONG><P><font color=red>\n");
        fprintf(out, "             Here are the Days/Stations available for viewing<br>\n");
        fprintf(out, "</font></STRONG>\n\n");

        for(i=0;i<But->nPlots;i++) {
            fprintf(out, "<p><tr><th> | ");
            sprintf(History, "%s%s.hist", But->GifDir, But->plt[i].SCNnam);
            hist = fopen(History, "r");
            if(hist == 0L)
                logit("e", "%s Unable to read History File: %s for %s\n", whoami, History, But->plt[i].SCNnam);
            else {
                while(fgets(string, SGRAM_STR_SIZE-1, hist)!=0L) {
                    j = (int)strlen(string);
                    if(j>2 && string[0]!='#') {
                        string[strlen(string)-1] = '\0';
                        strcpy(datetxt, string);
                        Make_Date(datetxt);
                        fprintf(out, "<a href=\"%s%s.%s.gif\" > %s</a> | ",
                            But->Prefix, But->plt[i].SCNnam, string, datetxt);
                    }
                }
                fclose(hist);
            }
            fprintf(out, "%s | %s </th></p>\n\n", But->plt[i].SCNtxt, But->plt[i].Comment);
        }

        sprintf( temp, "%sindexb.html", But->GifDir);
        in  = fopen(temp, "r");
        if(in == 0L) {
            fprintf(out, "<P><HR><font color=red></font>\n");
            fprintf(out, "<P><A HREF=\"#top\">Top of this page</A>\n");
            fprintf(out, "</BODY></HTML>\n");
        } else {
            while(fgets(string, SGRAM_STR_SIZE-1, in)!=0L) fprintf(out, "%s", string);
            fclose(in);
        }
        fclose(out);

        /* Put the HTML file in output directory(s). *
         ********************************************/
        for(j=0;j<But->nltargets;j++) {
            sprintf(tname,  "%stemphtml.%s", But->GifDir, But->mod );
            out = fopen(tname, "wb");
            if(out == 0L) {
                logit("e", "%s Unable to write HTML File: %s\n", whoami, tname);
            } else {
                in = fopen(FileName, "r");
                if(in == 0L) {
                    logit("e", "%s Unable to open index.html File: %s\n", whoami, FileName);
                    fclose(out);
                } else {
                    while(fgets(string, SGRAM_STR_SIZE-1, in)!=0L) fprintf(out, "%s", string);
                    fclose(in);
                    fclose(out);
                    sprintf(fname,  "%s%s", But->loctarget[j], "index.html" );
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
    }
}


/*********************************************************************
 *   AddDay()                                                        *
 *    If Today is not in the history file for this SCN,              *
 *      1) Today is added to the history file.                       *
 *      2) An html wrapper file is built for the current GIF         *
 *         and sent to targets                                       *
 *      3) The Index list (index.html) is updated                    *
 *********************************************************************/

void AddDay(Global *But)
{
    char    whoami[50];
    char    string[SGRAM_STR_SIZE], time[200], FileName[100], DummyFileName[100];
    FILE    *in, *out;
    int     i, ierr, count;

    sprintf(whoami, " %s: %s: ", But->mod, "AddDay");
    i = But->Current_Plot;
    if(!But->Make_HTML) return;
    /* Test to see if we need to add this entry
     ******************************************/
    ierr = 1;
    sprintf(time, "%s%.2d", But->plt[i].Today, But->plt[i].CurrentHour);
    sprintf(FileName, "%s%s.hist", But->GifDir, But->plt[i].SCNnam);
    in = fopen(FileName, "r");
    if(in == 0L)
        logit("e", "%s Unable to read History File: %s\n", whoami, FileName);
    else {
        while(fgets(string, SGRAM_STR_SIZE-1, in)!=0L) {
       /*     if(strstr(string, But->plt[i].Today)) ierr = 0;    */
            if(strstr(string, time)) ierr = 0;
        }
        fclose(in);
    }

    if(ierr) {    /* Copy the History file to dummy with insertion of this entry */
        sprintf(DummyFileName, "%s%s", But->GifDir, "dummy");
        count = 0;
        out = fopen(DummyFileName, "wb");
        if(out == 0L)
            logit("e", "%s Unable to write dummy History File: %s for %s\n",
                    whoami, DummyFileName, But->plt[i].SCNnam);
        else {
            fprintf(out, "%s%.2d\n", But->plt[i].Today, But->plt[i].CurrentHour);
            count += 1;
            in = fopen(FileName, "r");
            if(in == 0L)
                logit("e", "%s Unable to read History File: %s for %s\n",
                        whoami, FileName, But->plt[i].SCNnam);
            else {
                while(fgets(string, SGRAM_STR_SIZE-1, in)!=0L) {
                    fprintf(out, "%s", string);
                    if((int)strlen(string)>2 && string[0]!='#') count += 1;
                    if(count >= But->Days2Save) break;
                }
                fclose(in);
            }
            fclose(out);
        }

        in = fopen(DummyFileName, "r");
        if(in == 0L)
            logit("e", "%s Unable to read dummy History File: %s for %s\n",
                    whoami, DummyFileName, But->plt[i].SCNnam);
        else {
            out = fopen(FileName, "wb");
            if(out == 0L)
                logit("e", "%s Unable to write History File: %s for %s\n",
                        whoami, FileName, But->plt[i].SCNnam);
            else {
                while(fgets(string, SGRAM_STR_SIZE-1, in)!=0L) {
                    fprintf(out, "%s", string);
                }
                fclose(out);
            }
            fclose(in);
        }

        IndexListUpdate(But);
    }
}


/*********************************************************************
 *   Make_Date()                                                     *
 *    Expands a string of form YYYYMMDD to DD/MM/YYYY                *
 *********************************************************************/

void Make_Date(char *date)
{
    char    y[5], m[3], d[3], h[5];
    int     i;

    for(i=0;i<4;i++) y[i] = date[i];
    for(i=0;i<2;i++) m[i] = date[i+4];
    for(i=0;i<2;i++) d[i] = date[i+6];
    for(i=0;i<2;i++) h[i] = date[i+8];

    for(i=0;i<2;i++) date[i] = m[i];
    date[2] = '/';
    for(i=0;i<2;i++) date[i+3] = d[i];
    date[5] = '/';
    for(i=0;i<4;i++) date[i+6] = y[i];
    date[10] = 0;
    if(h[0]!='0'&&h[1]!='0') {
        date[10] = ' ';
        for(i=0;i<2;i++) date[i+11] = h[i];
        date[13] = ':';
        date[14] = '0';
        date[15] = '0';
        date[16] = 0;
    }
}


/*********************************************************************
 *   Save_Plot()                                                     *
 *    Saves the current version of the GIF image and ships it out.   *
 *********************************************************************/

void Save_Plot(Global *But, PltPar *plt)
{
    char    tname[175], fname[175], whoami[50];
    FILE    *out;
    int     j, ierr, jerr;

    Make_Grid(But, plt);
    sprintf(whoami, " %s: %s: ", But->mod, "Save_Plot");

    /* Save the GIF file. *
     **********************/
    out = fopen(But->LocalGif, "wb");
    if(out == 0L) {
        logit("e", "%s Unable to write GIF File: %s\n", whoami, But->LocalGif);
        But->NoGIFCount += 1;
        if(But->NoGIFCount > 5) {
            logit("et", "%s Unable to write GIF in 5 consecutive trys. Exiting.\n", whoami);
            ewmod_status( TypeError, ERR_FILEWRITE, "Unable to write GIF" );
            exit(-1);
        }
    } else {
        But->NoGIFCount = 0;
        gdImageGif(But->GifImage, out);
        fclose(out);
    }

    /* Put the GIF file in output directory(s). *
     ********************************************/
    for(j=0;j<But->nltargets;j++) {
        sprintf(tname,  "%stempgif.%s", But->GifDir, But->mod );
        out = fopen(tname, "wb");
        if(out == 0L) {
            logit("e", "%s Unable to write GIF File: %s\n", whoami, tname);
            But->NoGIFCount += 1;
            if(But->NoGIFCount > 5) {
                logit("et", "%s Unable to write GIF in 5 consecutive trys. Exiting.\n", whoami);
                ewmod_status( TypeError, ERR_FILEWRITE, "Unable to write GIF" );
                exit(-1);
            }
        } else {
            But->NoGIFCount = 0;
            gdImageGif(But->GifImage, out);
            fclose(out);
            sprintf(fname,  "%s%s", But->loctarget[j], But->GifName );
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
    gdImageDestroy(But->GifImage);
}


/*************************************************************************
 *   Build_Menu ()                                                       *
 *      Builds the waveservers' menus                                    *
 *      Each waveserver has its own menu so that we can do intelligent   *
 *      searches for data.                                               *
 *************************************************************************/
int Build_Menu (Global *But)
{
    char    whoami[50], server[100];
    int     i, j, retry, ret, rc, got_a_menu;
    WS_PSCNL scnp;
    WS_MENU menu;

    sprintf(whoami, " %s: %s: ", But->mod, "Build_Menu");
    setWsClient_ewDebug(0);
    if(But->WSDebug) setWsClient_ewDebug(1);
    got_a_menu = 0;

    for(j=0;j<But->nServer;j++) But->index[j] = j;

    for (i=0;i< But->nServer; i++) {
        retry = 0;
        But->inmenu[i] = 0;
        sprintf(server, " %s:%s <%s>", But->wsIp[i], But->wsPort[i], But->wsComment[i]);
Append:
        ret = wsAppendMenu(But->wsIp[i], But->wsPort[i], &But->menu_queue[i], But->wsTimeout);

        if (ret == WS_ERR_NONE) {
            But->inmenu[i] = got_a_menu = 1;
        }
        else if (ret == WS_ERR_INPUT) {
            logit("e","%s Connection to %s input error\n", whoami, server);
            if(retry++ < But->RetryCount) goto Append;
        }
        else if (ret == WS_ERR_EMPTY_MENU)
            logit("e","%s Unexpected empty menu from %s\n", whoami, server);
        else if (ret == WS_ERR_BUFFER_OVERFLOW)
            logit("e","%s Buffer overflowed for %s\n", whoami, server);
        else if (ret == WS_ERR_MEMORY)
            logit("e","%s Waveserver %s out of memory.\n", whoami, server);
        else if (ret == WS_ERR_PARSE)
            logit("e","%s Parser failed for %s\n", whoami, server);
        else if (ret == WS_ERR_TIMEOUT) {
            logit("e","%s Connection to %s timed out during menu.\n", whoami, server);
            if(retry++ < But->RetryCount) goto Append;
        }
        else if (ret == WS_ERR_BROKEN_CONNECTION) {
            logit("e","%s Connection to %s broke during menu\n", whoami, server);
            if(retry++ < But->RetryCount) goto Append;
        }
        else if (ret == WS_ERR_SOCKET)
            logit("e","%s Could not create a socket for %s\n", whoami, server);
        else if (ret == WS_ERR_NO_CONNECTION) {
     /*       if(But->Debug) */
                logit("e","%s Could not get a connection to %s to get menu.\n", whoami, server);
        }
        else logit("e","%s Connection to %s returns error: %d\n", whoami, server, ret);
    }
    /* Let's make sure that servers in our server list have really connected.
       **********************************************************************/
    if(got_a_menu) {
	    for(j=0;j<But->nServer;j++) {
	        if ( But->inmenu[j]) {
	                rc = wsGetServerPSCNL( But->wsIp[j], But->wsPort[j], &scnp, &But->menu_queue[j]);
	            if ( rc == WS_ERR_EMPTY_MENU ) {
	                if(But->Debug) logit("e","%s Empty menu for %s:%s <%s> \n",
	                                    whoami, But->wsIp[j], But->wsPort[j], But->wsComment[j]);
	                But->inmenu[j] = 0;
	                continue;
	            }
	            if ( rc == WS_ERR_SERVER_NOT_IN_MENU ) {
	                if(But->Debug) logit("e","%s  %s:%s <%s> not in menu.\n",
	                                    whoami, But->wsIp[j], But->wsPort[j], But->wsComment[j]);
	                But->inmenu[j] = 0;
	                continue;
	            }
	        }
	    }
    }
    /* Now, detach 'em and let RequestWave attach only the ones it needs.
       **********************************************************************/
    for(j=0;j<But->nServer;j++) {
        if(But->inmenu[j]) {
            menu = But->menu_queue[j].head;
            if ( menu->sock > 0 ) {
                wsDetachServer( menu );
            }
        }
    }
    return got_a_menu;
}


/*************************************************************************
 *   In_Menu_list                                                        *
 *      Determines if the scn is in the waveservers' menu.               *
 *      If there, the tank starttime and endtime are returned.           *
 *      Also, the Server IP# and port are returned.                      *
 *************************************************************************/

int In_Menu_list (Global *But)
{
    char    whoami[50], server[100];
    int     i, j, rc;
    WS_PSCNL scnp;

    sprintf(whoami, " %s: %s: ", But->mod, "In_Menu_list");
    i = But->Current_Plot;
    But->nentries = 0;
    for(j=0;j<But->nServer;j++) {
        if(But->inmenu[j]) {
            sprintf(server, " %s:%s <%s>", But->wsIp[j], But->wsPort[j], But->wsComment[j]);
            rc = wsGetServerPSCNL( But->wsIp[j], But->wsPort[j], &scnp, &But->menu_queue[j]);
            if ( rc == WS_ERR_EMPTY_MENU ) {
                if(But->Debug) logit("e","%s Empty menu for %s \n", whoami, server);
                But->inmenu[j] = 0;
                continue;
            }
            if ( rc == WS_ERR_SERVER_NOT_IN_MENU ) {
                if(But->Debug) logit("e","%s  %s not in menu.\n", whoami, server);
                But->inmenu[j] = 0;
                continue;
            }

			But->wsLoc[j] = (strlen(scnp->loc))? 1:0;
            while ( 1 ) {
               if(strcmp(scnp->sta,  But->plt[i].Site)==0 &&
                  strcmp(scnp->chan, But->plt[i].Comp)==0 &&
                  strcmp(scnp->net,  But->plt[i].Net )==0 &&
                  (strcmp(scnp->loc,  But->plt[i].Loc )==0 || But->wsLoc[j]==0) ) {
                  But->TStime[j] = scnp->tankStarttime;
                  But->TEtime[j] = scnp->tankEndtime;
                  But->index[But->nentries]  = j;
                  But->nentries += 1;
               }
               if ( scnp->next == NULL )
                  break;
               else
                  scnp = scnp->next;
            }
        }
    }
    if(But->nentries>0) return 1;
    return 0;
}


/********************************************************************
 *  RequestWave                                                     *
 *   This is the binary version                                     *
 *   k - waveserver index                                           *
 ********************************************************************/
int RequestWave(Global *But, int k, double *Data,
            char *Site, char *Comp, char *Net, char *Loc, double Stime, double Duration)
{
    char     whoami[50], SCNtxt[17];
    int      i, ret, iEnd, npoints, gap0;
    double   mean;
    TRACE_REQ   request;
    GAP *pGap;

    sprintf(whoami, " %s: %s: ", But->mod, "RequestWave");
    strcpy(request.sta,  Site);
    strcpy(request.chan, Comp);
    strcpy(request.net,  Net );
    strcpy(request.loc,  Loc );
    request.waitSec = 0;
    request.pinno   = 0;
    request.reqStarttime = Stime;
    request.reqEndtime   = request.reqStarttime + Duration;
    request.partial = 1;
    request.pBuf    = But->TraceBuf;
    request.bufLen  = MAXTRACELTH*9;
    request.timeout = But->wsTimeout;
    request.fill    = 919191;
    sprintf(SCNtxt, "%s %s %s %s", Site, Comp, Net, Loc);

    /* Clear out the gap list */
    if(tracePtr.nGaps != 0) {
        while ( (pGap = tracePtr.gapList) != (GAP *)NULL) {
            tracePtr.gapList = pGap->next;
            free(pGap);
        }
    }
    tracePtr.nGaps = 0;

    if(But->wsLoc[k]==0) {
	    ret = WSReqBin(But, k, &request, &tracePtr, SCNtxt);
    } else {
	    ret = WSReqBin2(But, k, &request, &tracePtr, SCNtxt);
    }

  /* Find mean value of non-gap data */
    pGap = tracePtr.gapList;
    i = npoints = 0L;
    mean    = 0.0;
  /*
   * Loop over all the data, skipping any gaps. Note that a `gap' will not be declared
   * at the end of the data, so the counter `i' will always get to pTrace.nRaw.
   */
    do {
        iEnd = (pGap == (GAP *)NULL)? tracePtr.nRaw:pGap->firstSamp - 1;
        if (pGap != (GAP *)NULL) { /* Test for gap within peak-search window */
            gap0 = pGap->lastSamp - pGap->firstSamp + 1;
            if(Debug) logit("t", "trace from <%s> has %d point gap in window at %d\n", SCNtxt, gap0, pGap->firstSamp);
        }
        for (; i < iEnd; i++) {
            mean += tracePtr.rawData[i];
            npoints++;
        }
        if (pGap != (GAP *)NULL) {     /* Move the counter over this gap */
            i = pGap->lastSamp + 1;
            pGap = pGap->next;
        }
    } while (i < tracePtr.nRaw );

    mean /= (double)npoints;

  /* Now remove the mean, and set points inside gaps to zero */
    i = 0;
    do {
        iEnd = (pGap == (GAP *)NULL)? tracePtr.nRaw:pGap->firstSamp - 1;
        for (; i < iEnd; i++) tracePtr.rawData[i] -= mean;

        if (pGap != (GAP *)NULL) {    /* Fill in the gap with zeros */

            for ( ;i < pGap->lastSamp + 1; i++) tracePtr.rawData[i] = 0.0;

            pGap = pGap->next;
        }
    } while (i < tracePtr.nRaw );

    But->Npts = tracePtr.nRaw;
    if(But->Npts>MAXTRACELTH) {
        logit("e","%s Trace: %s Too many points: %d\n", whoami, SCNtxt, But->Npts);
        But->Npts = MAXTRACELTH;
    }
    for(i=0;i<But->Npts;i++) Data[i] = tracePtr.rawData[i];
    But->Mean = 0.0;
    if(tracePtr.nGaps > 500) ret = 4;

    return ret;
}

/********************************************************************
 *  WSReqBin                                                        *
 *                                                                  *
 *   k - waveserver index                                           *
 ********************************************************************/
int WSReqBin(Global *But, int k, TRACE_REQ *request, DATABUF *pTrace, char *SCNtxt)
{
    char     server[wsADRLEN*3], whoami[50];
    int      kk, io, retry;
    int      isamp, nsamp, success, ret, WSDebug = 0;
    WS_MENU  menu = NULL;
    WS_PSCNL  pscn = NULL;
    double   traceEnd, samprate;
    int32_t *longPtr;
    short   *shortPtr;

    TRACE_HEADER *pTH;
    TRACE_HEADER *pTH4;
    char tbuf[MAX_TRACEBUF_SIZ];
    GAP *pGap, *newGap;

    sprintf(whoami, " %s: %s: ", But->mod, "WSReqBin");
    WSDebug = But->WSDebug;
    success = retry = 0;

gettrace:
    menu = NULL;
    /*    Put out WaveServer request here and wait for response */
    /* Get the trace
     ***************/
    /* rummage through all the servers we've been told about */
    if ( (wsSearchSCN( request, &menu, &pscn, &But->menu_queue[k]  )) == WS_ERR_NONE ) {
        strcpy(server, menu->addr);
        strcat(server, "  ");
        strcat(server, menu->port);
    } else {
        strcpy(server, "unknown");
    }

/* initialize the global trace buffer, freeing old GAP structures. */
    pTrace->nRaw  = 0L;
    pTrace->delta     = 0.0;
    pTrace->starttime = 0.0;
    pTrace->endtime   = 0.0;
    pTrace->nGaps = 0;

    /* Clear out the gap list */
    pTrace->nGaps = 0;
    while ( (pGap = pTrace->gapList) != (GAP *)NULL) {
        pTrace->gapList = pGap->next;
        free(pGap);
    }

    if(WSDebug) {
        logit("e","\n%s Issuing request to wsGetTraceBin: server: %s Socket: %d.\n",
             whoami, server, menu->sock);
        logit("e","    %s %f %f %d\n", SCNtxt,
             request->reqStarttime, request->reqEndtime, request->timeout);
    }

    io = wsGetTraceBin(request, &But->menu_queue[k], But->wsTimeout);

    if (io == WS_ERR_NONE ) {
        if(WSDebug) {
            logit("e"," %s server: %s trace %s: went ok first time. Got %ld bytes\n",
                whoami, server, SCNtxt, request->actLen);
            logit("e","        actStarttime=%lf, actEndtime=%lf, actLen=%ld, samprate=%lf\n",
                  request->actStarttime, request->actEndtime, request->actLen, request->samprate);
        }
    }
    else {
        switch(io) {
        case WS_ERR_EMPTY_MENU:
        case WS_ERR_SCNL_NOT_IN_MENU:
        case WS_ERR_BUFFER_OVERFLOW:
            if (io == WS_ERR_EMPTY_MENU )
                logit("e"," %s server: %s No menu found.  We might as well quit.\n", whoami, server);
            if (io == WS_ERR_SCNL_NOT_IN_MENU )
                logit("e"," %s server: %s Trace %s not in menu\n", whoami, server, SCNtxt);
            if (io == WS_ERR_BUFFER_OVERFLOW )
                logit("e"," %s server: %s Trace %s overflowed buffer. Fatal.\n", whoami, server, SCNtxt);
            return 2;                /*   We might as well quit */

        case WS_ERR_PARSE:
        case WS_ERR_TIMEOUT:
        case WS_ERR_BROKEN_CONNECTION:
        case WS_ERR_NO_CONNECTION:
            sleep_ew(500);
            retry += 1;
            if (io == WS_ERR_PARSE )
                logit("e"," %s server: %s Trace %s: Couldn't parse server's reply. Try again.\n",
                        whoami, server, SCNtxt);
            if (io == WS_ERR_TIMEOUT )
                logit("e"," %s server: %s Trace %s: Timeout to wave server. Try again.\n", whoami, server, SCNtxt);
            if (io == WS_ERR_BROKEN_CONNECTION ) {
            if(WSDebug) logit("e"," %s server: %s Trace %s: Broken connection to wave server. Try again.\n",
                whoami, server, SCNtxt);
            }
            if (io == WS_ERR_NO_CONNECTION ) {
                if(WSDebug || retry>1)
                    logit("e"," %s server: %s Trace %s: No connection to wave server.\n", whoami, server, SCNtxt);
                if(WSDebug) logit("e"," %s server: %s: Socket: %d.\n", whoami, server, menu->sock);
            }
            ret = wsAttachServer( menu, But->wsTimeout );
            if(ret == WS_ERR_NONE && retry < But->RetryCount) goto gettrace;
            if(WSDebug) {
                switch(ret) {
                case WS_ERR_NO_CONNECTION:
                    logit("e"," %s server: %s: No connection to wave server.\n", whoami, server);
                    break;
                case WS_ERR_SOCKET:
                    logit("e"," %s server: %s: Socket error.\n", whoami, server);
                    break;
                case WS_ERR_INPUT:
                    logit("e"," %s server: %s: Menu missing.\n", whoami, server);
                    break;
                default:
                    logit("e"," %s server: %s: wsAttachServer error %d.\n", whoami, server, ret);
                }
            }
            return 2;                /*   We might as well quit */

        case WS_WRN_FLAGGED:
            if((int)strlen(&request->retFlag)>0) {
                if(WSDebug)
                logit("e","%s server: %s Trace %s: return flag from wsGetTraceBin: <%c>\n %.50s\n",
                        whoami, server, SCNtxt, request->retFlag, But->TraceBuf);
            }
            if(WSDebug) logit("e"," %s server: %s Trace %s: No trace available. Wave server returned %c\n",
                whoami, server, SCNtxt, request->retFlag);
            break;
      /*      return 2;                   We might as well quit */

        default:
            logit( "et","%s server: %s Failed.  io = %d\n", whoami, server, io );
        }
    }

    /* Transfer trace data from TRACE_BUF packets into Trace buffer */
    traceEnd = (request->actEndtime < request->reqEndtime) ?  request->actEndtime :
                                                              request->reqEndtime;
    pTH  = (TRACE_HEADER *)request->pBuf;
    pTH4 = (TRACE_HEADER *)tbuf;
    /*
    * Swap to local byte-order. Note that we will be calling this function
    * twice for the first TRACE_BUF packet; this is OK as the second call
    * will have no effect.
    */

    memcpy( pTH4, pTH, sizeof(TRACE_HEADER) );
    if(WSDebug) logit( "e","%s server: %s Make Local\n", whoami, server, io );
    if (WaveMsgMakeLocal(pTH4) == -1) {
        logit("et", "%s server: %s %s.%s.%s unknown datatype <%s>; skipping\n",
              whoami, server, pTH4->sta, pTH4->chan, pTH4->net, pTH4->datatype);
        return 5;
    }
    if (WaveMsgMakeLocal(pTH4) == -2) {
        logit("et", "%s server: %s %s.%s.%s found funky packet with suspect header values!! <%s>; skipping\n",
              whoami, server, pTH4->sta, pTH4->chan, pTH4->net, pTH4->datatype);
    /*  return 5;  */
    }

    if(WSDebug) logit("e"," %s server: %s Trace %s: Data has samprate %g.\n",
                     whoami, server, SCNtxt, pTH4->samprate);
    if (pTH4->samprate < 0.1) {
        logit("et", "%s server: %s %s.%s.%s (%s) has zero samplerate (%g); skipping trace\n",
              whoami, server, pTH4->sta, pTH4->chan, pTH4->net, SCNtxt, pTH4->samprate);
        return 5;
    }
    But->samp_sec = pTH4->samprate<=0? 100L:(long)pTH4->samprate;

    pTrace->delta = 1.0/But->samp_sec;
    samprate = But->samp_sec;   /* Save rate of first packet to compare with later packets */
    pTrace->starttime = request->reqStarttime;
    /* Set Trace endtime so it can be used to test for gap at start of data */
    pTrace->endtime = ( (pTH4->starttime < request->reqStarttime) ?
                        pTH4->starttime : request->reqStarttime) - 0.5*pTrace->delta ;
    if(WSDebug) logit("e"," pTH->starttime: %f request->reqStarttime: %f delta: %f endtime: %f.\n",
                     pTH4->starttime, request->reqStarttime, pTrace->delta, pTrace->endtime);

  /* Look at all the retrieved TRACE_BUF packets
   * Note that we must copy each tracebuf from the big character buffer
   * to the TRACE_BUF structure pTH4.  This is because of the occasionally
   * seen case of a channel putting an odd number of i2 samples into
   * its tracebufs!  */
    kk = 0;
    while( pTH < (TRACE_HEADER *)(request->pBuf + request->actLen) ) {
        memcpy( pTH4, pTH, sizeof(TRACE_HEADER) );
         /* Swap bytes to local order */
	    if (WaveMsgMakeLocal(pTH4) == -1) {
	        logit("et", "%s server: %s %s.%s.%s unknown datatype <%s>; skipping\n",
	              whoami, server, pTH4->sta, pTH4->chan, pTH4->net, pTH4->datatype);
	        return 5;
	    }
	    if (WaveMsgMakeLocal(pTH4) == -2) {
	        logit("et", "%s server: %s %s.%s.%s found funky packet with suspect header values!! <%s>; skipping\n",
	              whoami, server, pTH4->sta, pTH4->chan, pTH4->net, pTH4->datatype);
	    /*  return 5;  */
        }

        nsamp = pTH4->nsamp;
        memcpy( pTH4, pTH, sizeof(TRACE_HEADER) + nsamp*4 );
         /* Swap bytes to local order */
	    if (WaveMsgMakeLocal(pTH4) == -1) {
	        logit("et", "%s server: %s %s.%s.%s unknown datatype <%s>; skipping\n",
	              whoami, server, pTH4->sta, pTH4->chan, pTH4->net, pTH4->datatype);
	        return 5;
	    }
	    if (WaveMsgMakeLocal(pTH4) == -2) {
	        logit("et", "%s server: %s %s.%s.%s found funky packet with suspect header values!! <%s>; skipping\n",
	              whoami, server, pTH4->sta, pTH4->chan, pTH4->net, pTH4->datatype);
	    /*  return 5;  */
        }

        if ( fabs(pTH4->samprate - samprate) > 1.0) {
            logit("et", "%s <%s.%s.%s samplerate change: %f - %f; discarding trace\n",
                whoami, pTH4->sta, pTH4->chan, pTH4->net, samprate, pTH4->samprate);
            return 5;
        }

    /* Check for gap */
        if (pTrace->endtime + 1.5 * pTrace->delta < pTH4->starttime) {
            if(WSDebug) logit("e"," %s server: %s Trace %s: Gap detected.\n",
                        whoami, server, SCNtxt);
            if ( (newGap = (GAP *)calloc(1, sizeof(GAP))) == (GAP *)NULL) {
                logit("et", "getTraceFromWS: out of memory for GAP struct\n");
                return -1;
            }
            newGap->starttime = pTrace->endtime + pTrace->delta;
            newGap->gapLen = pTH4->starttime - newGap->starttime;
            newGap->firstSamp = pTrace->nRaw;
            newGap->lastSamp  = pTrace->nRaw + (long)( (newGap->gapLen * samprate) - 0.5);
            if(WSDebug) logit("e"," starttime: %f gaplen: %f firstSamp: %d lastSamp: %d.\n",
                newGap->starttime, newGap->gapLen, newGap->firstSamp, newGap->lastSamp);
            /* Put GAP struct on list, earliest gap first */
            if (pTrace->gapList == (GAP *)NULL)
                pTrace->gapList = newGap;
            else
                pGap->next = newGap;
            pGap = newGap;  /* leave pGap pointing at the last GAP on the list */
            pTrace->nGaps++;

            /* Advance the Trace pointers past the gap; maybe gap will get filled */
            pTrace->nRaw = newGap->lastSamp + 1;
            pTrace->endtime += newGap->gapLen;
        }

        isamp = (pTrace->starttime > pTH4->starttime)?
                (int)( 0.5 + (pTrace->starttime - pTH4->starttime) * samprate):0;

        if (request->reqEndtime < pTH4->endtime) {
            nsamp = pTH4->nsamp - (int)( 0.5 * (pTH4->endtime - request->reqEndtime) * samprate);
            pTrace->endtime = request->reqEndtime;
        }
        else {
            nsamp = pTH4->nsamp;
            pTrace->endtime = pTH4->endtime;
        }

    /* Assume trace data is integer valued here, int32 or short */
        if (pTH4->datatype[1] == '4') {
            longPtr=(int32_t*) ((char*)pTH4 + sizeof(TRACE_HEADER) + isamp * 4);
            for ( ;isamp < nsamp; isamp++) {
                pTrace->rawData[pTrace->nRaw] = (double) *longPtr;
                longPtr++;
                pTrace->nRaw++;
                if(pTrace->nRaw >= MAXTRACELTH*5) break;
            }
            /* Advance pTH to the next TRACE_BUF message */
            pTH = (TRACE_HEADER *)((char *)pTH + sizeof(TRACE_HEADER) + pTH4->nsamp * 4);
        }
        else {   /* pTH->datatype[1] == 2, we assume */
            shortPtr=(short*) ((char*)pTH4 + sizeof(TRACE_HEADER) + isamp * 2);
            for ( ;isamp < nsamp; isamp++) {
                pTrace->rawData[pTrace->nRaw] = (double) *shortPtr;
                shortPtr++;
                pTrace->nRaw++;
                if(pTrace->nRaw >= MAXTRACELTH*5) break;
            }
            /* Advance pTH to the next TRACE_BUF packets */
            pTH = (TRACE_HEADER *)((char *)pTH + sizeof(TRACE_HEADER) + pTH4->nsamp * 2);
        }
    }  /* End of loop over TRACE_BUF packets */


    if (io == WS_ERR_NONE ) success = 1;

    return success;
}


/********************************************************************
 *  WSReqBin2                                                       *
 *                                                                  *
 *  Retrieves binary data from location code enabled waveservers.   *
 *   k - waveserver index                                           *
 ********************************************************************/
int WSReqBin2(Global *But, int k, TRACE_REQ *request, DATABUF *pTrace, char *SCNtxt)
{
    char     server[wsADRLEN*3], whoami[50];
    int      kk, io, retry;
    int      isamp, nsamp, success, ret, WSDebug = 0;
    WS_MENU  menu = NULL;
    WS_PSCNL  pscn = NULL;
    double   traceEnd, samprate;
    int32_t *longPtr;
    short   *shortPtr;

    TRACE2_HEADER *pTH;
    TRACE2_HEADER *pTH4;
    char tbuf[MAX_TRACEBUF_SIZ];
    GAP *pGap, *newGap;

    sprintf(whoami, " %s: %s: ", But->mod, "WSReqBin2");
    WSDebug = But->WSDebug;
    success = retry = 0;

gettrace:
    menu = NULL;
    /*    Put out WaveServer request here and wait for response */
    /* Get the trace
     ***************/
    /* rummage through all the servers we've been told about */
    if ( (wsSearchSCNL( request, &menu, &pscn, &But->menu_queue[k]  )) == WS_ERR_NONE ) {
        strcpy(server, menu->addr);
        strcat(server, "  ");
        strcat(server, menu->port);
    } else {
        strcpy(server, "unknown");
    }

/* initialize the global trace buffer, freeing old GAP structures. */
    pTrace->nRaw  = 0L;
    pTrace->delta     = 0.0;
    pTrace->starttime = 0.0;
    pTrace->endtime   = 0.0;
    pTrace->nGaps = 0;

    /* Clear out the gap list */
    pTrace->nGaps = 0;
    while ( (pGap = pTrace->gapList) != (GAP *)NULL) {
        pTrace->gapList = pGap->next;
        free(pGap);
    }

    if(WSDebug) {
        logit("e","\n%s Issuing request to wsGetTraceBinL: server: %s Socket: %d.\n",
             whoami, server, menu->sock);
        logit("e","    %s %f %f %d\n", SCNtxt,
             request->reqStarttime, request->reqEndtime, request->timeout);
    }

    io = wsGetTraceBinL(request, &But->menu_queue[k], But->wsTimeout);

    if (io == WS_ERR_NONE ) {
        if(WSDebug) {
            logit("e"," %s server: %s trace %s: went ok first time. Got %ld bytes\n",
                whoami, server, SCNtxt, request->actLen);
            logit("e","        actStarttime=%lf, actEndtime=%lf, actLen=%ld, samprate=%lf\n",
                  request->actStarttime, request->actEndtime, request->actLen, request->samprate);
        }
    }
    else {
        switch(io) {
        case WS_ERR_EMPTY_MENU:
        case WS_ERR_SCNL_NOT_IN_MENU:
        case WS_ERR_BUFFER_OVERFLOW:
            if (io == WS_ERR_EMPTY_MENU )
                logit("e"," %s server: %s No menu found.  We might as well quit.\n", whoami, server);
            if (io == WS_ERR_SCNL_NOT_IN_MENU )
                logit("e"," %s server: %s Trace %s not in menu\n", whoami, server, SCNtxt);
            if (io == WS_ERR_BUFFER_OVERFLOW )
                logit("e"," %s server: %s Trace %s overflowed buffer. Fatal.\n", whoami, server, SCNtxt);
            return 2;                /*   We might as well quit */

        case WS_ERR_PARSE:
        case WS_ERR_TIMEOUT:
        case WS_ERR_BROKEN_CONNECTION:
        case WS_ERR_NO_CONNECTION:
            sleep_ew(500);
            retry += 1;
            if (io == WS_ERR_PARSE )
                logit("e"," %s server: %s Trace %s: Couldn't parse server's reply. Try again.\n",
                        whoami, server, SCNtxt);
            if (io == WS_ERR_TIMEOUT )
                logit("e"," %s server: %s Trace %s: Timeout to wave server. Try again.\n", whoami, server, SCNtxt);
            if (io == WS_ERR_BROKEN_CONNECTION ) {
            if(WSDebug) logit("e"," %s server: %s Trace %s: Broken connection to wave server. Try again.\n",
                whoami, server, SCNtxt);
            }
            if (io == WS_ERR_NO_CONNECTION ) {
                if(WSDebug || retry>1)
                    logit("e"," %s server: %s Trace %s: No connection to wave server.\n", whoami, server, SCNtxt);
                if(WSDebug) logit("e"," %s server: %s: Socket: %d.\n", whoami, server, menu->sock);
            }
            ret = wsAttachServer( menu, But->wsTimeout );
            if(ret == WS_ERR_NONE && retry < But->RetryCount) goto gettrace;
            if(WSDebug) {
                switch(ret) {
                case WS_ERR_NO_CONNECTION:
                    logit("e"," %s server: %s: No connection to wave server.\n", whoami, server);
                    break;
                case WS_ERR_SOCKET:
                    logit("e"," %s server: %s: Socket error.\n", whoami, server);
                    break;
                case WS_ERR_INPUT:
                    logit("e"," %s server: %s: Menu missing.\n", whoami, server);
                    break;
                default:
                    logit("e"," %s server: %s: wsAttachServer error %d.\n", whoami, server, ret);
                }
            }
            return 2;                /*   We might as well quit */

        case WS_WRN_FLAGGED:
            if((int)strlen(&request->retFlag)>0) {
                if(WSDebug)
                logit("e","%s server: %s Trace %s: return flag from wsGetTraceBin: <%c>\n %.50s\n",
                        whoami, server, SCNtxt, request->retFlag, But->TraceBuf);
            }
            if(WSDebug) logit("e"," %s server: %s Trace %s: No trace available. Wave server returned %c\n",
                whoami, server, SCNtxt, request->retFlag);
            break;
      /*      return 2;                   We might as well quit */

        default:
            logit( "et","%s server: %s Failed.  io = %d\n", whoami, server, io );
        }
    }

    /* Transfer trace data from TRACE_BUF packets into Trace buffer */
    traceEnd = (request->actEndtime < request->reqEndtime) ?  request->actEndtime :
                                                              request->reqEndtime;
    pTH  = (TRACE2_HEADER *)request->pBuf;
    pTH4 = (TRACE2_HEADER *)tbuf;
    /*
    * Swap to local byte-order. Note that we will be calling this function
    * twice for the first TRACE_BUF packet; this is OK as the second call
    * will have no effect.
    */

    memcpy( pTH4, pTH, sizeof(TRACE2_HEADER) );
    if(WSDebug) logit( "e","%s server: %s Make Local\n", whoami, server, io );
    if (WaveMsg2MakeLocal(pTH4) == -1) {
        logit("et", "%s server: %s %s.%s.%s.%s unknown datatype <%s>; skipping\n",
              whoami, server, pTH4->sta, pTH4->chan, pTH4->net, pTH4->loc, pTH4->datatype);
        return 5;
    }
    if (WaveMsg2MakeLocal(pTH4) == -2) {
        logit("et", "%s server: %s %s.%s.%s.%s found funky packet with suspect header values!! <%s>; skipping\n",
              whoami, server, pTH4->sta, pTH4->chan, pTH4->net, pTH4->loc, pTH4->datatype);
    /*  return 5;  */
    }

    if(WSDebug) logit("e"," %s server: %s Trace %s: Data has samprate %g.\n",
                     whoami, server, SCNtxt, pTH4->samprate);
    if (pTH4->samprate < 0.1) {
        logit("et", "%s server: %s %s.%s.%s.%s (%s) has zero samplerate (%g); skipping trace\n",
              whoami, server, pTH4->sta, pTH4->chan, pTH4->net, pTH4->loc, SCNtxt, pTH4->samprate);
        return 5;
    }
    But->samp_sec = pTH4->samprate<=0? 100L:(long)pTH4->samprate;

    pTrace->delta = 1.0/But->samp_sec;
    samprate = But->samp_sec;   /* Save rate of first packet to compare with later packets */
    pTrace->starttime = request->reqStarttime;
    /* Set Trace endtime so it can be used to test for gap at start of data */
    pTrace->endtime = ( (pTH4->starttime < request->reqStarttime) ?
                        pTH4->starttime : request->reqStarttime) - 0.5*pTrace->delta ;
    if(WSDebug) logit("e"," pTH->starttime: %f request->reqStarttime: %f delta: %f endtime: %f.\n",
                     pTH4->starttime, request->reqStarttime, pTrace->delta, pTrace->endtime);

  /* Look at all the retrieved TRACE_BUF packets
   * Note that we must copy each tracebuf from the big character buffer
   * to the TRACE_BUF structure pTH4.  This is because of the occasionally
   * seen case of a channel putting an odd number of i2 samples into
   * its tracebufs!  */
    kk = 0;
    while( pTH < (TRACE2_HEADER *)(request->pBuf + request->actLen) ) {
        memcpy( pTH4, pTH, sizeof(TRACE2_HEADER) );
         /* Swap bytes to local order */
	    if (WaveMsg2MakeLocal(pTH4) == -1) {
	        logit("et", "%s server: %s %s.%s.%s.%s unknown datatype <%s>; skipping\n",
	              whoami, server, pTH4->sta, pTH4->chan, pTH4->net, pTH4->loc, pTH4->datatype);
	        return 5;
	    }
	    if (WaveMsg2MakeLocal(pTH4) == -2) {
	        logit("et", "%s server: %s %s.%s.%s.%s found funky packet with suspect header values!! <%s>; skipping\n",
	              whoami, server, pTH4->sta, pTH4->chan, pTH4->net, pTH4->loc, pTH4->datatype);
	    /*  return 5;  */
        }

        nsamp = pTH4->nsamp;
        memcpy( pTH4, pTH, sizeof(TRACE2_HEADER) + nsamp*4 );
         /* Swap bytes to local order */
	    if (WaveMsg2MakeLocal(pTH4) == -1) {
	        logit("et", "%s server: %s %s.%s.%s.%s unknown datatype <%s>; skipping\n",
	              whoami, server, pTH4->sta, pTH4->chan, pTH4->net, pTH4->loc, pTH4->datatype);
	        return 5;
	    }
	    if (WaveMsg2MakeLocal(pTH4) == -2) {
	        logit("et", "%s server: %s %s.%s.%s.%s found funky packet with suspect header values!! <%s>; skipping\n",
	              whoami, server, pTH4->sta, pTH4->chan, pTH4->net, pTH4->loc, pTH4->datatype);
	    /*  return 5;  */
        }

        if ( fabs(pTH4->samprate - samprate) > 1.0) {
            logit("et", "%s <%s.%s.%s.%s samplerate change: %f - %f; discarding trace\n",
                whoami, pTH4->sta, pTH4->chan, pTH4->net, pTH4->loc, samprate, pTH4->samprate);
            return 5;
        }

    /* Check for gap */
        if (pTrace->endtime + 1.5 * pTrace->delta < pTH4->starttime) {
            if(WSDebug) logit("e"," %s server: %s Trace %s: Gap detected.\n",
                        whoami, server, SCNtxt);
            if ( (newGap = (GAP *)calloc(1, sizeof(GAP))) == (GAP *)NULL) {
                logit("et", "getTraceFromWS: out of memory for GAP struct\n");
                return -1;
            }
            newGap->starttime = pTrace->endtime + pTrace->delta;
            newGap->gapLen = pTH4->starttime - newGap->starttime;
            newGap->firstSamp = pTrace->nRaw;
            newGap->lastSamp  = pTrace->nRaw + (long)( (newGap->gapLen * samprate) - 0.5);
            if(WSDebug) logit("e"," starttime: %f gaplen: %f firstSamp: %d lastSamp: %d.\n",
                newGap->starttime, newGap->gapLen, newGap->firstSamp, newGap->lastSamp);
            /* Put GAP struct on list, earliest gap first */
            if (pTrace->gapList == (GAP *)NULL)
                pTrace->gapList = newGap;
            else
                pGap->next = newGap;
            pGap = newGap;  /* leave pGap pointing at the last GAP on the list */
            pTrace->nGaps++;

            /* Advance the Trace pointers past the gap; maybe gap will get filled */
            pTrace->nRaw = newGap->lastSamp + 1;
            pTrace->endtime += newGap->gapLen;
        }

        isamp = (pTrace->starttime > pTH4->starttime)?
                (int)( 0.5 + (pTrace->starttime - pTH4->starttime) * samprate):0;

        if (request->reqEndtime < pTH4->endtime) {
            nsamp = pTH4->nsamp - (int)( 0.5 * (pTH4->endtime - request->reqEndtime) * samprate);
            pTrace->endtime = request->reqEndtime;
        }
        else {
            nsamp = pTH4->nsamp;
            pTrace->endtime = pTH4->endtime;
        }

    /* Assume trace data is integer valued here, int32 or short */
        if (pTH4->datatype[1] == '4') {
            longPtr=(int32_t*) ((char*)pTH4 + sizeof(TRACE2_HEADER) + isamp * 4);
            for ( ;isamp < nsamp; isamp++) {
                pTrace->rawData[pTrace->nRaw] = (double) *longPtr;
                longPtr++;
                pTrace->nRaw++;
                if(pTrace->nRaw >= MAXTRACELTH*5) break;
            }
            /* Advance pTH to the next TRACE_BUF message */
            pTH = (TRACE2_HEADER *)((char *)pTH + sizeof(TRACE2_HEADER) + pTH4->nsamp * 4);
        }
        else {   /* pTH->datatype[1] == 2, we assume */
            shortPtr=(short*) ((char*)pTH4 + sizeof(TRACE2_HEADER) + isamp * 2);
            for ( ;isamp < nsamp; isamp++) {
                pTrace->rawData[pTrace->nRaw] = (double) *shortPtr;
                shortPtr++;
                pTrace->nRaw++;
                if(pTrace->nRaw >= MAXTRACELTH*5) break;
            }
            /* Advance pTH to the next TRACE_BUF packets */
            pTH = (TRACE2_HEADER *)((char *)pTH + sizeof(TRACE2_HEADER) + pTH4->nsamp * 2);
        }
    }  /* End of loop over TRACE_BUF packets */


    if (io == WS_ERR_NONE ) success = 1;

    return success;
}


/**********************************************************************
 * IsDST : Determine if we are using daylight savings time.           *
 *         This is a valid function for US and Canada thru 31/12/2099 *
 *                                                                    *
 *         Modified 02/16/07 to reflect political changes. JHL        *
 *         Fixed 03/05/08 to run correctly. JHL                       *
 *                                                                    *
 **********************************************************************/
int IsDST(int year, int month, int day)
{
    int     i, leapyr, day1, day2, num, jd, jd1, jd2, jd3, jd4;
    int     dpm[] = {31,28,31,30,31,30,31,31,30,31,30,31};

    leapyr = 0;
    if((year-4*(year/4))==0 && (year-100*(year/100))!=0) leapyr = 1;
    if((year-400*(year/400))==0) leapyr = 1;

    num = ((year-1900)*5)/4 + 5;
    num = num - 7*(num/7);       /* day # of 1 March                  */
    	day1 = num-1;      
    	if(day1<=0) day1 = day1 + 7;  /* day # (-1) of 1 March             */
    	day1 = 8 - day1;              /* day # of 1st Sunday in March      */
    	jd3 = 59 + 7 + day1 + leapyr; /* Julian day of 2nd Sunday in March */
    day1 = num + 2;
    if(day1>7) day1 = day1 - 7;  /* day # (-1) of 1 April             */
    day1 = 8 - day1;             /* date of 1st Sunday in April       */
    jd1 = 90 + day1 + leapyr;    /* Julian day of 1st Sunday in April */
    day2 = num + 3;
    if(day2>7) day2 = day2 - 7;  /* day # (-1) of 1 Oct               */
    day2 = 8 - day2;             /* date of 1st Sunday in Oct         */
    while(day2<=31) day2 += 7;   /* date of 1st Sunday in Nov         */
    	jd4 = 273 + leapyr + day2;   /* Julian day of 1st Sunday in Nov  */
    day2 -= 7;                   /* date of last Sunday in Oct        */
    jd2 = 273 + day2 + leapyr;   /* Julian day of last Sunday in Oct  */

    jd = day;
    for(i=0;i<month-1;i++) jd += dpm[i];
    if(month>2) jd += leapyr;

    if(jd>=jd1 && jd<jd2) return 1;  
    if(jd>=jd3 && jd<jd4) return 1;

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
    secs += sec1970;
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
    *secs    = 60.0 * (double) julmin(&g) + Time->Sec - sec1970;
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

    secs += sec1970;
    minute = (long) (secs / 60.0);
    sex = secs - 60.0 * minute;
    grg(minute, &g);
    sprintf(c22, "%3s%2d,%4d %.2d:%.2d:%05.2f",
            cmo[g.month-1], g.day, g.year, g.hour, g.minute, sex);
}


/*************************************************************************
 *   SetPlot sets globals for the current plot.                          *
 *                                                                       *
 *************************************************************************/
void SetPlot(double Xsize, double Ysize)
{
    xsize = Xsize;
    ysize = Ysize;
}


/*************************************************************************
 *   ixq calculates the x pixel location.                                *
 *   a is the distance in inches from the left margin.                   *
 *************************************************************************/
int ixq(double a)
{
    double   val;
    int      i;

    val  = (a + XLMARGIN);
    if(val > xsize) val = xsize;
    if(val < 0.0)   val = 0.0;
    i = (int)(val*72.0);
    return i;
}


/*************************************************************************
 *   iyq calculates the y pixel location.                                *
 *   a is the distance in inches up from the bottom margin.              *
 *                      <or>                                             *
 *   a is the distance in inches down from the top margin.               *
 *************************************************************************/
int iyq(double a)
{
    double   val;
    int      i;

    val = plot_up?
         (ysize - YBMARGIN - a) : /* time increases up from bottom */
                  YTMargin + a;   /* times increases down from top */

    if(val > ysize) val = ysize;
    if(val < 0.0)   val = 0.0;
    i = (int)(val*72.0);
    return i;
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

    sprintf(whoami, "%s: %s: ", But->mod, "Get_Sta_Info");

    ns = 'N';
    ew = 'W';
    But->NSCN = 0;
    for(k=0;k<But->nStaDB;k++) {
            /* Open the main station file
             ****************************/
        nfiles = k_open( But->stationList[k] );
        if(nfiles == 0) {
            logit("e", "%s Error opening command file <%s>; exiting!\n", whoami, But->stationList[k] );
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
                        logit("e", "%s Error opening command file <%s>; exiting!\n", whoami, &com[1] );
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
                if(But->stationListType[k] == 1) {
	                str = k_str();
	                if(str) strncpy( But->Chan[j].Loc, str, 2);
	                for(i=0;i<2;i++) if(But->Chan[j].Loc[i]==' ')  But->Chan[j].Loc[i]  = 0;
	                But->Chan[j].Loc[2] = 0;
                }


                    /* Lat Lon Elev */
                if(But->stationListType[k] == 0) {
	                dlat = k_int();
	                mlat = k_val();

	                dlon = k_int();
	                mlon = k_val();

	                But->Chan[j].Elev = k_val();

	                    /* convert to decimal degrees */
	                if ( dlat < 0 ) dlat = -dlat;
	                if ( dlon < 0 ) dlon = -dlon;
	                But->Chan[j].Lat = dlat + (mlat/60.0);
	                But->Chan[j].Lon = dlon + (mlon/60.0);
	                    /* make south-latitudes and west-longitudes negative */
	                if ( ns=='s' || ns=='S' )               But->Chan[j].Lat = -But->Chan[j].Lat;
	                if ( ew=='w' || ew=='W' || ew==' ' )    But->Chan[j].Lon = -But->Chan[j].Lon;
                } else if(But->stationListType[k] == 1) {
	                But->Chan[j].Lat  = k_val();
	                But->Chan[j].Lon  = k_val();
	                But->Chan[j].Elev = k_val();
                }

         /*       str = k_str();      Blow past the subnet */

                But->Chan[j].Inst_type = k_int();
                But->Chan[j].Inst_gain = k_val();
                But->Chan[j].GainFudge = k_val();
                But->Chan[j].Sens_type = k_int();
                But->Chan[j].Sens_unit = k_int();
                But->Chan[j].Sens_gain = k_val();
                But->Chan[j].SiteCorr  = k_val();

                if(But->Chan[j].Sens_unit == 3) But->Chan[j].Sens_gain /= 981.0;

                But->Chan[j].sensitivity = (1000000.0*But->Chan[j].Sens_gain/But->Chan[j].Inst_gain)*But->Chan[j].GainFudge;    /*    sensitivity counts/units        */

                if(But->stationListType[k] == 1) But->Chan[j].ShkQual = k_int();

                if (k_err()) {
                    logit("e", "%s Error decoding line in station file\n%s\n  exiting!\n", whoami, k_get() );
                    exit( -1 );
                }
         /*>Comment<*/
                str = k_str();
                if( str != NULL && str[0]!='#')  strcpy( But->Chan[j].SiteName, str );

                str = k_str();
                if( str != NULL && str[0]!='#')  strcpy( But->Chan[j].Descript, str );

                But->NSCN++;
            }
            nfiles = k_close();
        }
    }
}


/*************************************************************************
 *  Put_Sta_Info(Global *But);                                           *
 *  Retrieve all the information available about station i               *
 *  and put it into an internal structure for reference.                 *
 *  This should eventually be a call to the database; for now we must    *
 *  supply an ascii file with all the info.                              *
 *************************************************************************/
int Put_Sta_Info(int i, Global *But)
{
    char    whoami[50];
    short   j;

    sprintf(whoami, " %s: %s: ", But->mod, "Put_Sta_Info");

    for(j=0;j<But->NSCN;j++) {
        if(strcmp(But->Chan[j].Site, But->plt[i].Site )==0 &&
           strcmp(But->Chan[j].Comp, But->plt[i].Comp)==0 &&
           strcmp(But->Chan[j].Net,  But->plt[i].Net )==0) {
            if(strcmp("**",  But->plt[i].Loc )==0) {
            	strcpy(But->plt[i].Loc, But->Chan[j].Loc);
                sprintf(But->plt[i].SCNnam, "%s_%s_%s_%s_00", But->plt[i].Site, But->plt[i].Comp, But->plt[i].Net, But->plt[i].Loc);
                sprintf(But->plt[i].SCNtxt, "%s %s %s %s", But->plt[i].Site, But->plt[i].Comp, But->plt[i].Net, But->plt[i].Loc);
                sprintf(But->plt[i].SCN, "%s%s%s%s", But->plt[i].Site, But->plt[i].Comp, But->plt[i].Net, But->plt[i].Loc);
            }
            if(strcmp(But->Chan[j].Loc,  But->plt[i].Loc )==0) {
            strcpy(But->plt[i].Comment, But->Chan[j].SiteName);
            But->plt[i].Inst_type  = But->Chan[j].Inst_type;
            But->plt[i].GainFudge  = But->Chan[j].GainFudge;
            But->plt[i].Sens_type  = But->Chan[j].Sens_type;
            But->plt[i].Sens_gain  = But->Chan[j].Sens_gain;
            But->plt[i].Sens_unit  = But->Chan[j].Sens_unit;
            But->plt[i].sensitivity  = But->Chan[j].sensitivity;
            if(But->plt[i].Sens_unit<=1) But->plt[i].Scaler = But->plt[i].Scale*1000.0;
            if(But->plt[i].Sens_unit==2) But->plt[i].Scaler = But->plt[i].Scale*10000.0;
            if(But->plt[i].Sens_unit==3) But->plt[i].Scaler = But->plt[i].Scale*10.0;
        if(But->Debug) {
            logit("e", "\n%s %s. %d %d %d %f %f %f %f %s %s\n\n",
                whoami, But->plt[i].SCNtxt, But->plt[i].Inst_type, But->plt[i].Sens_type,
                But->plt[i].Sens_unit, But->plt[i].Sens_gain, But->plt[i].GainFudge, But->plt[i].sensitivity,
                But->plt[i].Scale, But->plt[i].Comment, But->plt[i].Descrip);
        }
            return 1;
        }
    }
    }
    strcpy(But->plt[i].Comment, "");
    But->plt[i].Inst_type  = 0;
    But->plt[i].GainFudge  = 1;
    But->plt[i].Sens_type  = 0;
    But->plt[i].Sens_gain  = 1.0;
    But->plt[i].Sens_unit  = 0;
    But->plt[i].sensitivity  = 1000000.0;    /* Default: 1 volt/unit & 1 microvolt/count */
    But->plt[i].Scaler = But->plt[i].Scale*1000.0;
        if(But->Debug) {
            logit("e", "\n%s %s. %d %d %d %f %f %f %f %s\n\n",
                whoami, But->plt[i].SCNtxt, But->plt[i].Inst_type, But->plt[i].Sens_type,
                But->plt[i].Sens_unit, But->plt[i].Sens_gain, But->plt[i].GainFudge, But->plt[i].sensitivity,
                But->plt[i].Scale, But->plt[i].Comment);
        }
    return 0;
}


/****************************************************************************
 *      config_me() process command file using kom.c functions              *
 *                       exits if any errors are encountered                *
 ****************************************************************************/

void config_me( char* configfile, Global *But )
{
    char    whoami[50], *com, *str = NULL;
    char    init[20];       /* init flags, one byte for each required command */
    int     ncommand;       /* # of required commands you expect              */
    int     nmiss;          /* number of required commands that were missed   */
    int     n, nPlots, nfiles, success, i;
    double  val;
    FILE    *in;
    gdImagePtr    im_in;

    sprintf(whoami, " %s: %s: ", But->mod, "config_me");
        /* Set one init flag to zero for each required command
         *****************************************************/
    ncommand = 8;
    for(i=0; i<ncommand; i++ )  init[i] = 0;
    But->nltargets = 0;
    But->Debug = Debug = But->WSDebug = But->logo = But->logox = But->logoy = 0;
    But->SaveDrifts = 0;
    But->Make_HTML = 0;
    But->Days2Save = 7;
    But->BuildOnRestart = 0;
    But->DaysAgo = But->OneDayOnly = 0;
    But->RetryCount = 2;
    But->UpdateInt =   120;
    But->NoMenuCount = 0;
    But->NoGIFCount  = 0;
    strcpy(But->Prefix, "");
    plot_up = 1;
    EWmodule = 1;

        /* Open the main configuration file
         **********************************/
    nfiles = k_open( configfile );
    if(nfiles == 0) {
        logit("e", "%s Error opening command file <%s>; exiting!\n", whoami, configfile );
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
                    logit("e", "%s Error opening command file <%s>; exiting!\n", whoami, &com[1] );
                    exit( -1 );
                }
                continue;
            }

                /* Process anything else as a command
                 ************************************/
/*0*/
            if( k_its("LogSwitch") ) {
                LogSwitch = k_int();
                init[0] = 1;
            }
/*1*/
            else if( k_its("MyModuleId") ) {
                str = k_str();
                if(str) strcpy( MyModuleId , str );
                init[1] = 1;
            }
/*2*/
            else if( k_its("RingName") ) {
                str = k_str();
                if(str) strcpy( InRingName, str );
                init[2] = 1;
            }
/*3*/
            else if( k_its("HeartBeatInt") ) {
                HeartBeatInterval = k_long();
                init[3] = 1;
            }

/*4*/
            else if( k_its("wsTimeout") ) { /* timeout interval in seconds */
                But->wsTimeout = k_int()*1000;
                init[4] = 1;
            }

         /* wave server addresses and port numbers to get trace snippets from
          *******************************************************************/
/*5*/
            else if( k_its("WaveServer") ) {
                if ( But->nServer >= MAX_WAVESERVERS ) {
                    logit("e", "%s Too many <WaveServer> commands in <%s>",
                             whoami, configfile );
                    logit("e", "; max=%d; exiting!\n", (int) MAX_WAVESERVERS );
                    return;
                }
                if( (str=k_str()) != NULL )  strcpy(But->wsIp[But->nServer],str);
                if( (str=k_str()) != NULL )  strcpy(But->wsPort[But->nServer],str);
                str = k_str();
                if( str != NULL && str[0]!='#')  strcpy(But->wsComment[But->nServer],str);
                But->nServer++;
                init[5]=1;
            }

        /* get Gif directory path/name
        *****************************/
/*6*/
            else if( k_its("GifDir") ) {
                str = k_str();
                if( (int)strlen(str) >= GDIRSZ) {
                    logit("e", "%s Fatal error. Gif directory name %s greater than %d char.\n",
                        whoami, str, GDIRSZ);
                    return;
                }
                if(str) strcpy( But->GifDir , str );
                init[6] = 1;
            }


          /* get plot parameters
        ************************************/
/*7*/
            else if( k_its("Plot") ) {
                if( But->nPlots >= MAXPLOTS ) {
                    logit("e", "%s Too many <Plot> commands in <%s>", whoami, configfile );
                    logit("e", "; max=%d; exiting!\n", (int) MAXPLOTS );
                    return;
                }
                nPlots = But->nPlots;
                But->plt[nPlots].DCcorr = But->Mean = 0.0;
                But->plt[nPlots].UseDST = 0;
                But->plt[nPlots].DCremoved   = 0;
                But->plt[nPlots].fmax  = 25.0;
                But->plt[nPlots].fmute =  0.0;
                But->plt[nPlots].amax  =  0.0;
                But->plt[nPlots].nbw   = 2;
                But->plt[nPlots].Pallette = 1;
                But->plt[nPlots].dminhr   =  500000.0;
                But->plt[nPlots].dmaxhr   =  0.0;
                But->plt[nPlots].Scale    =  1.0;

         /*>Site<*/
                str = k_str();
                if(str) strcpy( But->plt[nPlots].Site , str );
         /*>Comp<*/
                str = k_str();
                if(str) strcpy( But->plt[nPlots].Comp , str );
         /*>Net<*/
                str = k_str();
                if(str) strcpy( But->plt[nPlots].Net , str );
         /*>Loc<*/
                strcpy( But->plt[nPlots].Loc , "**" );
                strcpy( But->plt[nPlots].Loc , "--" );
                sprintf(But->plt[nPlots].SCNnam, "%s_%s_%s", But->plt[nPlots].Site, But->plt[nPlots].Comp, But->plt[nPlots].Net);
                sprintf(But->plt[nPlots].SCNnam, "%s_%s_%s_%s_00", But->plt[nPlots].Site, But->plt[nPlots].Comp, But->plt[nPlots].Net, But->plt[nPlots].Loc);
                sprintf(But->plt[nPlots].SCNtxt, "%s %s %s %s", But->plt[nPlots].Site, But->plt[nPlots].Comp, But->plt[nPlots].Net, But->plt[nPlots].Loc);
                sprintf(But->plt[nPlots].SCN, "%s%s%s%s", But->plt[nPlots].Site, But->plt[nPlots].Comp, But->plt[nPlots].Net, But->plt[nPlots].Loc);
         /*>04<*/
                But->plt[nPlots].HoursPerPlot = k_int();   /*  # of hours per gif image */
                if(But->plt[nPlots].HoursPerPlot <  1) But->plt[nPlots].HoursPerPlot =  1;
                if(But->plt[nPlots].HoursPerPlot > 24) But->plt[nPlots].HoursPerPlot = 24;
                But->plt[nPlots].HoursPerPlot  = 24/(24/But->plt[nPlots].HoursPerPlot);
         /*>05<*/
                But->plt[nPlots].OldData = k_int();        /* Number of previous hours to retrieve */
                if(But->plt[nPlots].OldData < 0 || But->plt[nPlots].OldData > 168) {
                    But->plt[nPlots].OldData = 0;
                }
         /*>06<*/
                But->plt[nPlots].LocalTime = k_int();    /* Hour offset of local time from UTC */
                if(But->plt[nPlots].LocalTime < -24 || But->plt[nPlots].LocalTime > 24) {
                    But->plt[nPlots].LocalTime = 0;
                }
                But->plt[nPlots].LocalSecs = But->plt[nPlots].LocalTime*3600;
         /*>07<*/
                str = k_str();    /* Local Time ID e.g. PST */
                strncpy(But->plt[nPlots].LocalTimeID, str, (size_t)3);
                But->plt[nPlots].LocalTimeID[3] = '\0';
         /*>08<*/
                But->plt[nPlots].ShowUTC  = k_int();    /* Flag to show UTC time */
         /*>09<*/
                But->plt[nPlots].UseLocal = k_int();    /* Flag to reference plot to local midnight */
                But->plt[nPlots].LocalTimeOffset = But->plt[nPlots].UseLocal? But->plt[nPlots].LocalSecs:0;
         /*>10<*/
                val = k_val();
                if(val >= 100.0) {            /* x-size of data plot in pixels */
                    But->plt[nPlots].xpix = (int)val;
                } else {
                    But->plt[nPlots].xpix = (int)(val*72.0);
                }
         /*>11<*/
                But->plt[nPlots].PixPerLine = 1;
                But->plt[nPlots].PixPerLine = (int)k_val();         /*  Number of vertical pixels per display line */
                if(But->plt[nPlots].PixPerLine >= 100.0) But->plt[nPlots].PixPerLine = 100;
                if(But->plt[nPlots].PixPerLine <=   1.0) But->plt[nPlots].PixPerLine =   1;
         /*>12<*/
                But->plt[nPlots].mins = k_int();            /* # of minutes/line to display */
                if(But->plt[nPlots].mins < 1 || But->plt[nPlots].mins > 60) {
                    But->plt[nPlots].mins = 15;
                }
                But->plt[nPlots].mins  = 60/(60/But->plt[nPlots].mins);
                But->plt[nPlots].LinesPerHour = 60/But->plt[nPlots].mins;
                But->plt[nPlots].secsPerStep  = But->plt[nPlots].mins*60;
                But->plt[nPlots].PixPerMin = (double)But->plt[nPlots].PixPerLine/(double)But->plt[nPlots].mins;

         /*>13<*/
                But->plt[nPlots].secsPerGulp = k_int();    /* # of seconds/line to FFT */
                if(But->plt[nPlots].secsPerGulp < 10 || But->plt[nPlots].secsPerGulp > 60*MAXMINUTES) {
                    But->plt[nPlots].secsPerGulp = 60;
                }

                But->plt[nPlots].ypix = But->plt[nPlots].PixPerLine*But->plt[nPlots].LinesPerHour*But->plt[nPlots].HoursPerPlot;
         /*>14<*/
                But->plt[nPlots].fmax = k_val();    /* Maximum frequency */
                if(But->plt[nPlots].fmax < 0.0 || But->plt[nPlots].fmax > 200) {
                    But->plt[nPlots].fmax = 100.0;
                }
         /*>15<*/
                But->plt[nPlots].fmute = k_val();    /* # Hz to mute at lo end */
                if(But->plt[nPlots].fmute < 0.0 || But->plt[nPlots].fmute > But->plt[nPlots].fmax) {
                    But->plt[nPlots].fmute = 0.0;
                }
         /*>16<*/
                But->plt[nPlots].nbw = k_int();    /* Scaling type */
                if(But->plt[nPlots].nbw < 0) {
                    But->plt[nPlots].Pallette = 0;
                    But->plt[nPlots].nbw = -But->plt[nPlots].nbw;
                }
                if(But->plt[nPlots].nbw < 1 || But->plt[nPlots].nbw > 4) {
                    But->plt[nPlots].nbw = 1;
                }
         /*>17<*/
                But->plt[nPlots].amax = k_val();    /* Maximum amplitude */
         /*>18<*/
                But->plt[nPlots].Scale = k_val();    /* Wiggle-line trace scaling */
         /*>19<*/
                str = k_str();    /* Comment */
                if(str) strcpy( But->plt[nPlots].Descrip , str );

                But->nPlots++;
                init[7] = 1;
            }

          /* get SCNL parameters
        ************************************/
            else if( k_its("SCNL") ) {
                if( But->nPlots >= MAXPLOTS ) {
                    logit("e", "%s Too many <Plot> commands in <%s>", whoami, configfile );
                    logit("e", "; max=%d; exiting!\n", (int) MAXPLOTS );
                    return;
                }
                nPlots = But->nPlots;
                But->plt[nPlots].DCcorr = But->Mean = 0.0;
                But->plt[nPlots].UseDST = 0;
                But->plt[nPlots].DCremoved   = 0;
                But->plt[nPlots].fmax  = 25.0;
                But->plt[nPlots].fmute =  0.0;
                But->plt[nPlots].amax  =  0.0;
                But->plt[nPlots].nbw   = 2;
                But->plt[nPlots].Pallette = 1;
                But->plt[nPlots].dminhr   =  500000.0;
                But->plt[nPlots].dmaxhr   =  0.0;
                But->plt[nPlots].Scale    =  1.0;

         /*>Site<*/
                str = k_str();
                if(str) strcpy( But->plt[nPlots].Site , str );
         /*>Comp<*/
                str = k_str();
                if(str) strcpy( But->plt[nPlots].Comp , str );
         /*>Net<*/
                str = k_str();
                if(str) strcpy( But->plt[nPlots].Net , str );
         /*>Loc<*/
                str = k_str();
                if(str) strcpy( But->plt[nPlots].Loc , str );
                sprintf(But->plt[nPlots].SCNnam, "%s_%s_%s", But->plt[nPlots].Site, But->plt[nPlots].Comp, But->plt[nPlots].Net);
                sprintf(But->plt[nPlots].SCNnam, "%s_%s_%s_%s_00", But->plt[nPlots].Site, But->plt[nPlots].Comp, But->plt[nPlots].Net, But->plt[nPlots].Loc);
                sprintf(But->plt[nPlots].SCNtxt, "%s %s %s %s", But->plt[nPlots].Site, But->plt[nPlots].Comp, But->plt[nPlots].Net, But->plt[nPlots].Loc);
                sprintf(But->plt[nPlots].SCN, "%s%s%s%s", But->plt[nPlots].Site, But->plt[nPlots].Comp, But->plt[nPlots].Net, But->plt[nPlots].Loc);
         /*>04<*/
                But->plt[nPlots].HoursPerPlot = k_int();   /*  # of hours per gif image */
                if(But->plt[nPlots].HoursPerPlot <  1) But->plt[nPlots].HoursPerPlot =  1;
                if(But->plt[nPlots].HoursPerPlot > 24) But->plt[nPlots].HoursPerPlot = 24;
                But->plt[nPlots].HoursPerPlot  = 24/(24/But->plt[nPlots].HoursPerPlot);
         /*>05<*/
                But->plt[nPlots].OldData = k_int();        /* Number of previous hours to retrieve */
                if(But->plt[nPlots].OldData < 0 || But->plt[nPlots].OldData > 168) {
                    But->plt[nPlots].OldData = 0;
                }
         /*>06<*/
                But->plt[nPlots].LocalTime = k_int();    /* Hour offset of local time from UTC */
                if(But->plt[nPlots].LocalTime < -24 || But->plt[nPlots].LocalTime > 24) {
                    But->plt[nPlots].LocalTime = 0;
                }
                But->plt[nPlots].LocalSecs = But->plt[nPlots].LocalTime*3600;
         /*>07<*/
                str = k_str();    /* Local Time ID e.g. PST */
                strncpy(But->plt[nPlots].LocalTimeID, str, (size_t)3);
                But->plt[nPlots].LocalTimeID[3] = '\0';
         /*>08<*/
                But->plt[nPlots].ShowUTC  = k_int();    /* Flag to show UTC time */
         /*>09<*/
                But->plt[nPlots].UseLocal = k_int();    /* Flag to reference plot to local midnight */
                But->plt[nPlots].LocalTimeOffset = But->plt[nPlots].UseLocal? But->plt[nPlots].LocalSecs:0;
         /*>10<*/
                val = k_val();
                if(val >= 100.0) {            /* x-size of data plot in pixels */
                    But->plt[nPlots].xpix = (int)val;
                } else {
                    But->plt[nPlots].xpix = (int)(val*72.0);
                }
         /*>11<*/
                But->plt[nPlots].PixPerLine = 1;
                But->plt[nPlots].PixPerLine = (int)k_val();         /*  Number of vertical pixels per display line */
                if(But->plt[nPlots].PixPerLine >= 100.0) But->plt[nPlots].PixPerLine = 100;
                if(But->plt[nPlots].PixPerLine <=   1.0) But->plt[nPlots].PixPerLine =   1;
         /*>12<*/
                But->plt[nPlots].mins = k_int();            /* # of minutes/line to display */
                if(But->plt[nPlots].mins < 1 || But->plt[nPlots].mins > 60) {
                    But->plt[nPlots].mins = 15;
                }
                But->plt[nPlots].mins  = 60/(60/But->plt[nPlots].mins);
                But->plt[nPlots].LinesPerHour = 60/But->plt[nPlots].mins;
                But->plt[nPlots].secsPerStep  = But->plt[nPlots].mins*60;
                But->plt[nPlots].PixPerMin = (double)But->plt[nPlots].PixPerLine/(double)But->plt[nPlots].mins;

         /*>13<*/
                But->plt[nPlots].secsPerGulp = k_int();    /* # of seconds/line to FFT */
                if(But->plt[nPlots].secsPerGulp < 10 || But->plt[nPlots].secsPerGulp > 60*MAXMINUTES) {
                    But->plt[nPlots].secsPerGulp = 60;
                }

                But->plt[nPlots].ypix = But->plt[nPlots].PixPerLine*But->plt[nPlots].LinesPerHour*But->plt[nPlots].HoursPerPlot;
         /*>14<*/
                But->plt[nPlots].fmax = k_val();    /* Maximum frequency */
                if(But->plt[nPlots].fmax < 0.0 || But->plt[nPlots].fmax > 250.0) {
                    logit("e", "%s Fmax (%f) too large. Reset to 20.0",
                             whoami, But->plt[nPlots].fmax );
                    But->plt[nPlots].fmax = 20.0;
                }
         /*>15<*/
                But->plt[nPlots].fmute = k_val();    /* # Hz to mute at lo end */
                if(But->plt[nPlots].fmute < 0.0 || But->plt[nPlots].fmute > But->plt[nPlots].fmax) {
                    But->plt[nPlots].fmute = 0.0;
                }
         /*>16<*/
                But->plt[nPlots].nbw = k_int();    /* Scaling type */
                if(But->plt[nPlots].nbw < 0) {
                    But->plt[nPlots].Pallette = 0;
                    But->plt[nPlots].nbw = -But->plt[nPlots].nbw;
                }
                if(But->plt[nPlots].nbw < 1 || But->plt[nPlots].nbw > 4) {
                    But->plt[nPlots].nbw = 1;
                }
         /*>17<*/
                But->plt[nPlots].amax = k_val();    /* Maximum amplitude */
         /*>18<*/
                But->plt[nPlots].Scale = k_val();    /* Wiggle-line trace scaling */
         /*>19<*/
                str = k_str();    /* Comment */
                if(str) strcpy( But->plt[nPlots].Descrip , str );

                But->nPlots++;
                init[7] = 1;
            }


               /* optional commands */

                /* get station list path/name
                *****************************/

            else if( k_its("StationList") ) {
                if ( But->nStaDB >= MAX_STADBS ) {
                    logit("e", "%s Too many <StationList> commands in <%s>",
                             whoami, configfile );
                    logit("e", "; max=%d; exiting!\n", (int) MAX_STADBS );
                    return;
                }
                str = k_str();
                if( (int)strlen(str) >= STALIST_SIZ) {
                    logit("e", "%s Fatal error. Station list name %s greater than %d char.\n",
                            whoami, str, STALIST_SIZ);
                    exit(-1);
                }
                if(str) strcpy( But->stationList[But->nStaDB] , str );
                But->stationListType[But->nStaDB] = 0;
                But->nStaDB++;
            }
            else if( k_its("StationList1") ) {
                if ( But->nStaDB >= MAX_STADBS ) {
                    logit("e", "%s Too many <StationList> commands in <%s>",
                             whoami, configfile );
                    logit("e", "; max=%d; exiting!\n", (int) MAX_STADBS );
                    return;
                }
                str = k_str();
                if( (int)strlen(str) >= STALIST_SIZ) {
                    logit("e", "%s Fatal error. Station list name %s greater than %d char.\n",
                            whoami, str, STALIST_SIZ);
                    exit(-1);
                }
                if(str) strcpy( But->stationList[But->nStaDB] , str );
                But->stationListType[But->nStaDB] = 1;
                But->nStaDB++;
            }


        /* get Prefix for deletable files on the target
        ***********************************************/
            else if( k_its("Prefix") ) {
                str = k_str();
                if( (int)strlen(str) >= PRFXSZ) {
                    logit("e", "%s Fatal error. File prefix %s greater than %d char.\n",
                        whoami, str, PRFXSZ);
                    return;
                }
                if(str) {
                    strcpy(But->Prefix, str );
                    strcat(But->Prefix, ".");
                }
            }

         /* get the local target directory(s)
        ************************************/
            else if( k_its("LocalTarget") ) {
                if ( But->nltargets >= MAX_TARGETS ) {
                    logit("e", "%s Too many <LocalTarget> commands in <%s>",
                             whoami, configfile );
                    logit("e", "; max=%d; exiting!\n", (int) MAX_TARGETS );
                    return;
                }
                if( (str=k_str()) != NULL )  {
                    n = (int)strlen(str);   /* Make sure directory name has proper ending! */
                    if( str[n-1] != '/' ) strcat(str, "/");
                    strcpy(But->loctarget[But->nltargets], str);
                }
                But->nltargets += 1;
            }

            else if( k_its("Debug") )          But->Debug = 1;            /* optional commands */

            else if( k_its("WSDebug") )        But->WSDebug = 1;          /* optional commands */

            else if( k_its("SaveDrifts") )     But->SaveDrifts = 1;       /* optional commands */

            else if( k_its("Make_HTML") )      But->Make_HTML = 1;        /* optional commands */

            else if( k_its("UseDST") )         But->UseDST = 1;           /* optional commands */

            else if( k_its("BuildOnRestart") ) But->BuildOnRestart = 1;   /* optional commands */

            else if( k_its("DaysAgo") ) {   /* optional commands */
                But->BuildOnRestart = 1;
                But->OneDayOnly = 1;
                But->DaysAgo = k_int();
                if(But->DaysAgo>300) But->DaysAgo = 300;
                if(But->DaysAgo< 1) But->DaysAgo =  0;
            }

            else if( k_its("StandAlone") )     EWmodule = 0;              /* optional command */

            else if( k_its("PlotDown") )       plot_up = 0;               /* optional command */

            else if( k_its("PlotUp") )         plot_up = 1;               /* optional command */

            else if( k_its("RetryCount") ) {                              /*optional command*/
                But->RetryCount = k_int();
                if(But->RetryCount > 20) But->RetryCount = 20;
                if(But->RetryCount <  2) But->RetryCount =  2;
            }

            else if( k_its("UpdateInt") ) {                               /*optional command*/
                But->UpdateInt = k_int()*60;
                if(But->UpdateInt>10000) But->UpdateInt = 10000;
                if(But->UpdateInt<  1) But->UpdateInt =   120;
            }

            else if( k_its("Logo") ) {                                    /*optional command*/
                str = k_str();
                if(str) {
                    strcpy( But->logoname, str );
                    But->logo = 1;
                }
            }

            else if( k_its("Days2Save") ) {                               /*optional command*/
                But->Days2Save = k_int();
                if(But->Days2Save>14) But->Days2Save = 14;
                if(But->Days2Save< 1) But->Days2Save =  1;
            }

                /* At this point we give up. Unknown thing.
                *******************************************/
            else {
                logit("e", "%s <%s> Unknown command in <%s>.\n",
                         whoami, com, configfile );
                continue;
            }

                /* See if there were any errors processing the command
                 *****************************************************/
            if( k_err() ) {
               logit("e", "%s Bad <%s> command  in <%s>; exiting!\n",
                             whoami, com, configfile );
               exit( -1 );
            }
        }
        nfiles = k_close();
   }

        /* After all files are closed, check init flags for missed commands
         ******************************************************************/
    nmiss = 0;
    for(i=0;i<ncommand;i++)  if( !init[i] ) nmiss++;
    if ( nmiss ) {
        logit("e", "%s ERROR, no ", whoami);
        if ( !init[0] )  logit("e", "<LogSwitch> "    );
        if ( !init[1] )  logit("e", "<MyModuleId> "   );
        if ( !init[2] )  logit("e", "<RingName> "     );
        if ( !init[3] )  logit("e", "<HeartBeatInt> " );
        if ( !init[4] )  logit("e", "<wsTimeout> "    );
        if ( !init[5] )  logit("e", "<WaveServer> "   );
        if ( !init[6] )  logit("e", "<GifDir> "       );
        if ( !init[7] )  logit("e", "<Plot> "         );
        logit("e", "command(s) in <%s>; exiting!\n", configfile );
        exit( -1 );
    }
    YTMargin = YTMARGIN;
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
    for(i=0;i<But->nPlots;i++)
        But->plt[i].UseDST = But->UseDST;
}


/**********************************************************************************
 * ewmod_status() builds a heartbeat or error msg & puts it into shared memory    *
 **********************************************************************************/
void ewmod_status( unsigned char type,  short ierr,  char *note )
{
    char        subname[] = "ewmod_status";
    MSG_LOGO    logo;
    char        msg[256];
    long        size;
    time_t      t;

    logo.instid = InstId;
    logo.mod    = MyModId;
    logo.type   = type;

    time( &t );
    if( type == TypeHeartBeat ) {
        sprintf( msg, "%ld %ld\n", (long) t,(long) MyPid);
    } else
    if( type == TypeError ) {
        sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note );
        logit( "t", "%s: %s:  %s\n",  module, subname, note );
    }

    size = (long)strlen( msg );   /* don't include the null byte in the message */
    if(Transport) {
    if( tport_putmsg( &InRegion, &logo, size, msg ) != PUT_OK ) {
        if( type == TypeHeartBeat ) {
            logit("et","%s: %s:  Error sending heartbeat.\n",  module, subname );
        } else
        if( type == TypeError ) {
            logit("et","%s: %s:  Error sending error:%d.\n",  module, subname, ierr );
        }
    }
}
}


/****************************************************************************
 *  lookup_ew( ) Look up important info from earthworm.h tables             *
 ****************************************************************************/
void lookup_ew( )
{
    char    subname[] = "lookup_ew";

        /* Look up keys to shared memory regions
         ***************************************/
    if( (InRingKey = GetKey(InRingName)) == -1 ) {
        fprintf( stderr,
                "%s: %s: Invalid ring name <%s>; exiting!\n",  module, subname, InRingName );
        exit( -1 );
    }

        /* Look up installation Id
         *************************/
    if( GetLocalInst( &InstId ) != 0 ) {
        fprintf( stderr,
                "%s: %s: error getting local installation id; exiting!\n",  module, subname );
        exit( -1 );
    }

        /* Look up modules of interest
         *****************************/
    if( GetModId( MyModuleId, &MyModId ) != 0 ) {
        fprintf( stderr,
                "%s: %s: Invalid module name <%s>; exiting!\n",  module, subname, MyModuleId );
        exit( -1 );
    }

        /* Look up message types of interest
         ***********************************/
    if( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
        fprintf( stderr,
                "%s: %s: Invalid message type <TYPE_HEARTBEAT>; exiting!\n",  module, subname );
        exit( -1 );
    }
    if( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
        fprintf( stderr,
                "%s: %s: Invalid message type <TYPE_ERROR>; exiting!\n",  module, subname );
        exit( -1 );
    }
}


