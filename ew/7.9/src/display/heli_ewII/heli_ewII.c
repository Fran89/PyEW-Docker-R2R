

/* This is heli_ewII */

/*
History:
Jim Luetgert created the original heli 10/7/98. He did all the hard work.
It fell on Pete to clean and debug the code. This Pete did, creating two
versions: heli_standalone, which was a free-standing program (although
using earthworm functions), and heli_ew, which ran as a Module, thus providing
error processing and restarts.
Meanwhile, Jim Luetgert and others proceeded to develop and enhance similar
programs. We're aware of six similar efforts. What to do. Given Pete's efforts 
to stabilize heli_standalone, and its proven track-record for reliability, and
that we're to provide a mission-critical system, as well as a community effort,
we're producing heli_ewII as the staid, reliable, supported product. We will 
carry fancier products as contributed to us by others in the Contrib section.
From time to time, we may adopt and adapt some of these more advanced products
into the core earthworm software.
*/
/* Mar 18, 2002 JMP:
Fixed a bug where heli_ewII, while running as a module, was not beating its
heartbeat while in startup mode or while updateing the gifs.
*/
/* Sept 19, 2000 Alex: 
Created heli_ewII from heli_ew and heli_standalone. Purpose:
1. to make it run on NT.
2. to allow it to optionally attach to a ring and do the heartbeat and restart 
stuff.
3. remove the remote copy option. Files are to be moved to other
machines by shared disk schemes or by Will's SendFile.
*/    
  
/*   2000/05/13 19:01:20  lombard
 *     Fixed tracebuf length. This bug prevented aquisition of 200 SPS data
 *     at update intervals of MAXMINUTES.
 */

/* Changes 24 November 1999: PNL
 * Local and remote Gif file names are now the same, so they match what's in
 *  the index file even for less than 24 hours per plot.
 * Index file now can list more than one gif per day if HoursPerPlot is < 24.
 * Index file name is configurable; defaults to "index.html".
 * Added error checking for too-long strings in config file.
 * Fixed bug in WSDEBUG logic.
 */

/* Bug fix: 9 June 1999: PNL
 * Corrected bug in Sort_Servers(): 24 hours after a station stopped in the
 * wave_server, none of the stations following that one in heli's config list
 * would be updated since the servers entire menu would be removed by
 * Sort_Servers().
 */

/* Bug fix: 2 April, 1999; PNL
 * Corrected bugs in RequestWave() to properly handle error returns from
 * wsGetTraceAscii() before calling wsAttachServer.
 */

/* Modified 10/17/98 PNL 
 * 1. Change to allow time to increase downward; configurable at compile time
 *    by adjusting iyq() below.
 * 2. Added option for setting clip value for traces
 * 3. Made "Target" an optional command, for when you don't want to use it.
 * 4. Added date to gif file name in LocalGif
 * 5. Removed all mutex calls; they weren't being used.
 * 6. Removed all transport calls and heartbeats.
 * 7. Changed logit() calls to fprintf()
 */

/* More changes 12/1/98 PNL:
 * 1. changed error checking after ws_client calls to look for success first,
 *    use switch instead if if/ifelse.
 * 2. Removed some redundant parts such as calculating mean twice
 * 3. Fixed bug in Sort_Servers
 * 4. Pulled Build_Menu out of gif loop
 */

/* More changes 1/4/13 TGZ:
 * 1. Make optional MaxSampRate config parameter and make default 500
 */

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <earthworm.h>
#include <transport.h>
#include <chron3.h>
#include <kom.h>
#include <ws_clientII.h>
#include "gd.h"
#include "gdfontt.h"   /*  6pt      */
#include "gdfonts.h"   /*  7pt      */
#include "gdfontmb.h"  /*  9pt Bold */
#include "gdfontl.h"   /* 13pt      */
#include "gdfontg.h"   /* 10pt Bold */

#include "heli_ewII.h" 

/* Functions in this source file
 *******************************/
void SetUp(Butler *);
void UpDate(Butler *);
void Sort_Servers (Butler *, double StartTime);
short Build_Axes(Butler *, double, int);
void Make_Grid(Butler *);
void Pallette(int i, gdImagePtr GIF, long color[]);
void Plot_Trace(Butler *, double *, double);
void Find_Mean(Butler *, double *Data, int, int index, double Stime, int);
void CommentList(Butler *);
void IndexList(Butler *);
void AddDay(Butler *);
void IndexListUpdate(Butler *);
void Make_Date(char *);
void Save_Plot(Butler *);
void Build_Menu (Butler *);
int In_Menu_list (Butler *);
short RequestWave(Butler *, int k, double *, char *, char *, char *, char *, double, double);
void Decode_Time( double secs, TStrct *Time);
void Encode_Time( double *secs, TStrct *Time);
void date22( double, char *);
void SetPlot(double, double);
int ixq(double);
int iyq(double);
void config_me( char *,  Butler *); /* reads config (.d) file via Carl's routines  */
void ewmod_status( unsigned char, short, char *); /* sends heartbeats and errors into ring   */
void lookup_ew ( void );  /* Goes from symbolic names to numeric values, via earthworm.h  */
void DoModuleStuff();    /* beats the heart and chekcs for shutdown reqeuest*/

/* Ew Moudle stuff
******************/
static  SHM_INFO  InRegion;         /* public shared memory for receiving arkive messages */
static long          InRingKey;     /* key of transport ring for input         */
static unsigned char InstId;        /* local installation id                   */
static unsigned char MyModId;       /* our Module id                           */
static unsigned char TypeHeartBeat; /* message type */
static unsigned char TypeError;     /* message type */
static char InRingName[MAX_RING_STR]; /* name of transport ring for i/o          */
static char MyModuleId[MAX_MOD_STR];/* Module id for this Module               */
static int  LogSwitch;              /* 0 if no logging should be done to disk  */
static time_t HeartBeatInterval;    /* seconds between heartbeats              */
static time_t    HeartBeatInterval, TimeLastBeat;    /* current time & time of last heartbeat   */
static pid_t     MyPid;        /* Our own pid, sent with heartbeat for restart purposes */
static int ImAModule=0;        /* 1 if we're running as a module, 0 if standalone */
                  /* set if we find any Earthworm-related parameters in the
                  config file */

/* EW error codes used by this Module
 ***********************************/
#define   ERR_MISSMSG       0
#define   ERR_TOOBIG        1
#define   ERR_NOTRACK       2
#define   ERR_INTERNAL      3
#define   ERR_QUEUE         4

static  Butler BStruct;              /* Private area for the threads            */
double  *Data;           /* Trace array     size: MAXTRACELTH                       */

/* Other globals
 ***************/
double    XLMargin = 0.7;            /* Margin to left of axes                  */
double    YBMargin = 0.5;            /* Margin at bottom of axes                */
double    XRMargin = 1.0;            /* Margin to right of axes                 */
double    YTMargin = 0.7;            /* Margin at top of axes                   */
double    sec1970 = 11676096000.00;  /* # seconds between Carl Johnson's        */
/* static int  Debug = 1; */         /* debug output flag   */
                                     /* time 0 and 1970-01-01 00:00:00.0 GMT    */
char      string[20];
char      Module[] = "heli_ewII";
static double    xsize, ysize, xoff, yoff;
/* int MAXSAMPRATE = MAXSAMPRATE_DEFAULT; */ /* Now a global int instead of a define */
size_t MAXTRACELTH = MAXMINUTES*60*MAXSAMPRATE_DEFAULT;
size_t MAXTRACEBUF;




/*************************************************************************
 *  main( int argc, char **argv )                                        *
 *************************************************************************/

#define VERSION_NUM_STR "1.0.5 2015-03-17"

int
main( int argc, char **argv )
{
  char    whoami[NAMELEN];
  int     i, j;
  time_t  atime, now;
  
  /* Check command line arguments
  ******************************/
  if ( argc != 2 ) {
    fprintf( stderr, "Usage: %s <configfile>\n", Module);
    fprintf( stderr, "Version: %s\n", VERSION_NUM_STR);
    exit( 0 );
  }
    
  /* Zero the wave server arrays *
  *******************************/
  for (i=0; i< MAX_WAVESERVERS; i++) {
    memset( BStruct.wsIp[i],   0, MAX_ADRLEN);
    memset( BStruct.wsPort[i], 0, MAX_ADRLEN);
    memset( BStruct.wsComment[i], 0, MAX_ADRLEN);
  }
  
  /* construct program and routine name
  *************************************/
  strcpy(BStruct.mod, Module);
  sprintf(whoami, "%s (%s): ", Module, "Main");
  
  /* Read the configuration file(s)
  ********************************/
  config_me( argv[1], &BStruct );
  
  /* Set data sizes */
  MAXTRACELTH = MAXMINUTES*60*BStruct.maxSampRate;
  MAXTRACEBUF = MAXTRACELTH*13;
  Data = calloc(MAXTRACELTH, sizeof(double));
  if (Data == NULL)
  {
    printf("Error allocating memory for Data, exiting");
    exit(-1);
  }
  BStruct.TraceBuf = calloc(MAXTRACELTH*10, sizeof(char));
  if (BStruct.TraceBuf == NULL)
  {
    printf("Error allocating memory for BStruct.TraceBuf, exiting");
    exit(-1);
  }
  
  /* Start logging
  ****************/
  /* We're using the Ew logit() routines. If we're running standlaone,
  we specify no log file, and write to stderr only */
  if(!ImAModule) LogSwitch=0; /* Don't try to write a Ew log file if we're running standlone*/
  logit_init( argv[1], (short) MyModId, 256, LogSwitch );
  logit( "et" , "%s Version: %s\n", whoami, VERSION_NUM_STR);
  logit( "et" , "%s Read command file <%s>\n", whoami, argv[1] );
  
  /* Get our own Pid for restart purposes
  ***************************************/
  MyPid = getpid();
  if( MyPid == -1 ) {
    logit( "et", "%s Cannot get pid. Exiting.\n", whoami);
    return -1;
  }

  /* If we're a module, do the Earthwormy things
  **********************************************/
  if(ImAModule) {  /* this is set in config_me() by the presence of module paramters in the config file*/
    if(HeartBeatInterval<=SLEEP_SEC){
      logit("et","%s Heartbeat Interval=%d. Shorter than minimum %d. Exiting\n", whoami, HeartBeatInterval, SLEEP_SEC);
      return(-1);
    }
                sleep_ew(500);  /* delay a bit to let EW and modules start */
    lookup_ew();  /* Look up numeric values from earthworm.h tables*/
    tport_attach( &InRegion, InRingKey ); /* attach to ew ring */
    time(&TimeLastBeat); /* Send first heartbeat */
    ewmod_status( TypeHeartBeat, 0, "" );
  }
  
  CommentList(&BStruct);
  if(BStruct.Make_HTML) IndexList(&BStruct);

  i = 0;
  while(1) {
    /* See what our wave servers have to offer */
    Build_Menu (&BStruct);
    if(BStruct.got_a_menu)        /* if success then       */
      break;                      /* exit loop and move on */
    logit( "et", "%s Unable to build menus from wave servers\n", whoami);
    for(j=0; j<BStruct.nServer; j++)        /* Kill Menu */
      wsKillMenu(&(BStruct.menu_queue[j]));
    if(++i > 9) {       /* abort if too many failures */
      logit( "et", "%s Too many failures; aborting program\n", whoami);
      sleep_ew(200);
      exit(-1);
      return(-1);
    }
    sleep_ew(1000);
    time(&TimeLastBeat);     /* send heartbeats while retrying */
    ewmod_status( TypeHeartBeat, 0, "" );
  }
    
  /* Do first run through plots */
  logit("et","%s Starting plots\n",whoami);
  for(i=0; i<BStruct.nPlots; i++) {
    BStruct.Current_Plot = i;
    SetUp(&BStruct);
  }
  /* Kill the used menus */    
  for(j=0; j<BStruct.nServer; j++) 
    wsKillMenu(&(BStruct.menu_queue[j]));
  
  /* ------------------------ start working loop -------------------------*/
  while(1) {
        
    logit( "et", "%s Updating Helicorders.\n", whoami);
    DoModuleStuff(); /* beat the heart and check for shutdown*/
    Build_Menu (&BStruct);
    if (!BStruct.got_a_menu) { 
      /*yipes! - no menues at all*/
      logit( "e", "%s No Menu! skipping update.\n", whoami);
      BStruct.NoMenuCount += 1;
      if(BStruct.NoMenuCount > 5) { 
        logit( "e", "%s No menu in 5 consecutive tries. Exitting.\n", whoami);
        /* Kill Menue lists */
        for(j=0; j<BStruct.nServer; j++) wsKillMenu(&(BStruct.menu_queue[j]));
        sleep_ew(200);
        exit(-1);
      }
    } else { 
      /* Got a menu; now update the plots. */
      BStruct.NoMenuCount = 0; 
      for(i=0; i<BStruct.nPlots; i++) {
        DoModuleStuff(); /* beat the heart and check for shutdown*/
        BStruct.Current_Plot = i;
        UpDate(&BStruct);
      }
    }
    /* Kill the used menus */    
    for(j=0; j<BStruct.nServer; j++) wsKillMenu(&(BStruct.menu_queue[j]));
    
        /*Now sleep till its time to update the plots again.
    But keep doing heart beats and check for shutdown requests*/
    time(&atime);
        do {
      DoModuleStuff();
            sleep_ew( SLEEP_SEC*1000 );       /* wait around */
        } while(time(&now)-atime < BStruct.UpdateInt);
  }   /*-----------------------end of working loop-------------------------*/
}


/********************************************************************
 *  SetUp does the initial setup and first set of images            *
 ********************************************************************/

 void SetUp(Butler *But)
 {
   char    whoami[NAMELEN], time1[25], time2[25], sip[25], sport[25], sid[NAMELEN];
   double  tankStarttime, tankEndtime, LocalTimeOffset;
   double  StartTime, EndTime, Duration, ZTime;
   time_t  current_time;
   int     i, j, k, jj, successful, server, hour1, hour2;
   int     minPerStep, minOfDay;
   TStrct  t0, Time1, Time2;    
   
   sprintf(whoami, " %s: %s: ", But->mod, "SetUp");
   
   i = But->Current_Plot;
   SetPlot(But->plt[i].xsize, But->plt[i].ysize);
   LocalTimeOffset = But->plt[i].UseLocal? But->plt[i].LocalSecs:0.0;
   
   /* Figure out a good time to start the plot, so they look nice */
   StartTime = (double) (time(&current_time) - 120);
   Decode_Time(StartTime, &t0);
   t0.Sec = 0;
   minPerStep = But->plt[i].secsPerStep/60;
   minOfDay = minPerStep*((t0.Hour*60 + t0.Min)/minPerStep) - minPerStep;
   if(minOfDay<0) minOfDay = 0;
   t0.Hour = minOfDay/60;
   t0.Min = minOfDay - t0.Hour*60;
   Encode_Time( &StartTime, &t0);    /* StartTime modulo step */
   
   /* If we are requesting older data, make sure we are on the current page.  */
   Decode_Time(StartTime + LocalTimeOffset, &Time1);
   hour1 = (int) (But->plt[i].HoursPerPlot*((24*Time1.Day + Time1.Hour)/But->plt[i].HoursPerPlot));
   StartTime = StartTime - But->plt[i].OldData*60*60;
   Decode_Time(StartTime + LocalTimeOffset, &Time2);
   hour2 = But->plt[i].HoursPerPlot*((24*Time2.Day + Time2.Hour)/But->plt[i].HoursPerPlot);
   if(hour1 != hour2) {
     Time1.Hour = But->plt[i].HoursPerPlot*(Time1.Hour/But->plt[i].HoursPerPlot);
     Time1.Min = 0;
     Time1.Sec = 0.0;
     Encode_Time( &StartTime, &Time1);
     StartTime = StartTime - LocalTimeOffset;
   }
   
   if(In_Menu_list(But)) {
     
     /* Make sure that we aren't trying to get data that isn't about to be lapped  */
     for(j=0;j<But->plt[i].nentries;j++) {
       k = But->plt[i].index[j];
       But->plt[i].TStime[k] += 300;     
       Decode_Time( But->plt[i].TStime[k], &t0);
       t0.Sec = 0;
       t0.Min = But->plt[i].mins*(t0.Min/But->plt[i].mins) + But->plt[i].mins;
       Encode_Time( &But->plt[i].TStime[k], &t0);
     }
     
     Sort_Servers(But, StartTime);
     if(But->plt[i].nentries <= 0) {  /* nothing to process */
       return;
     }
     
     k = But->plt[i].index[0];
     tankStarttime = But->plt[i].TStime[k];
     tankEndtime   = But->plt[i].TEtime[k];
     
     if(But->Debug) {
       date22 (tankStarttime, time1);
       date22 (tankEndtime, time2);
       strcpy(sip,   But->wsIp[k]);
       strcpy(sport, But->wsPort[k]);
       strcpy(sid,   But->wsComment[k]);
       logit( "e", "%s Got menu for: %s. %s %s %s %s <%s>\n", 
         whoami, But->plt[i].SCNLtxt, time1, time2, sip, sport, sid);
       for(j=0;j<But->plt[i].nentries;j++) {
         k = But->plt[i].index[j];
         date22 (But->plt[i].TStime[k], time1);
         date22 (But->plt[i].TEtime[k], time2);
         logit( "e", "            %d %d %s %s %s %s <%s>\n", 
           j, k, time1, time2, But->wsIp[k], But->wsPort[k], But->wsComment[k]);
       }
     }
     
     /* Adjust plot start time bassed on what the wave servers can provide */
     if(StartTime < tankStarttime) {
       StartTime = tankStarttime;
       Decode_Time(StartTime, &t0);
       t0.Sec = 0;
       minPerStep = But->plt[i].secsPerStep/60;
       minOfDay = minPerStep*((t0.Hour*60 + t0.Min)/minPerStep);
       if(minOfDay<0) minOfDay = 0;
       t0.Hour = minOfDay/60;
       t0.Min = minOfDay - t0.Hour*60;
       Encode_Time( &StartTime, &t0);
     }
     
     Duration = But->plt[i].secsPerGulp;         /* Seconds to get */
     EndTime = StartTime + Duration;      
     But->plt[i].LastTime = StartTime;
     
     Decode_Time(StartTime + LocalTimeOffset, &t0);
     t0.Min = 0;
     t0.Sec = 0.0;
     t0.Hour = But->plt[i].HoursPerPlot*(t0.Hour/But->plt[i].HoursPerPlot);
     But->plt[i].CurrentHour = t0.Hour;
     But->plt[i].CurrentDay  = t0.Day;
     Encode_Time( &ZTime, &t0);
     
     /* Time to start making gifs */
     if ( Build_Axes(But, ZTime, But->BuildOnRestart)) 
       return;
     
     while(EndTime < tankEndtime) {
       if(But->Debug) {
         date22 (StartTime, time1);
         date22 (EndTime,   time2);
         logit( "e", "%s Getting data for: %s. %s %s %s %s <%s>\n", 
           whoami, But->plt[i].SCNLtxt, time1, time2, sip, sport, sid);
       }

       DoModuleStuff(); /* beat the heart and check for shutdown*/
       
       /* Try to get some data
       ***********************/
       for(jj=0;jj<But->plt[i].nentries;jj++) {
         server = But->plt[i].index[jj];
         successful = RequestWave(But, server, Data, But->plt[i].Site, 
           But->plt[i].Comp, But->plt[i].Net, But->plt[i].Loc, StartTime, Duration);
         if(successful == 1) {   /*    Plot this trace to memory. */
           break;
         }
         else if(successful == 2) {
           continue;
         }
         else if(successful == 3) {   /* Gap in data */
           
         }
       }
       
       if(successful == 1) {   /*    Plot this trace to memory. */
         Plot_Trace(But, Data, StartTime);
       }
       
       StartTime += But->plt[i].secsPerStep;
       
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
         Save_Plot(But);
         
         if ( Build_Axes( But, ZTime, 1)) 
           return;
         
       }
       But->plt[i].LastTime = StartTime;
       Duration = But->plt[i].secsPerGulp;
       EndTime = StartTime + Duration;      
     }
     Save_Plot(But);
  } else {
    logit( "e", "%s %s not in menu.\n", whoami, But->plt[i].SCNLtxt);
  }
  return;
}


/********************************************************************
 *  UpDate does the image updates                                   *
 *                                                                  *
 ********************************************************************/
 
 void UpDate(Butler *But)
 {
   char    whoami[NAMELEN], time1[25], time2[25], sip[25], sport[25], sid[NAMELEN];
   double  tankStarttime, tankEndtime, LocalTimeOffset;
   double  StartTime, EndTime, Duration, ZTime;
   time_t  current_time;
   int     i, j, k, jj, successful, server, ForceRebuild, hour1, hour2;
   TStrct  t0;    
   
   sprintf(whoami, " %s: %s: ", But->mod, "UpDate");
   i = But->Current_Plot;
   
   if(In_Menu_list(But)) {    
     if(But->plt[i].LastTime==0) {
       logit( "e", "%s LastTime came up 0!\n", whoami); 
       But->plt[i].LastTime = (double) time(&current_time);
     }
     if(fabs(But->plt[i].LastTime-time(&current_time)) > 86400) {
       date22 (But->plt[i].LastTime, time1);
       logit( "e", "%s LastTime came up %f %s!\n", whoami, But->plt[i].LastTime, time1);
       But->plt[i].LastTime = (double) time(&current_time);
     }
     StartTime = But->plt[i].LastTime;   /* Earliest time needed. */
     
     Sort_Servers(But, StartTime);
     if(But->plt[i].nentries <= 0) return;  /* nothing to process */
     
     SetPlot(But->plt[i].xsize, But->plt[i].ysize);
     k = But->plt[i].index[0];
     tankStarttime = But->plt[i].TStime[k];
     tankEndtime   = But->plt[i].TEtime[k];
     if(StartTime < tankStarttime) StartTime = tankStarttime;
     
     if(But->Debug) {
       date22 (tankStarttime, time1);
       date22 (tankEndtime,   time2);
       strcpy(sip,   But->wsIp[k]);
       strcpy(sport, But->wsPort[k]);
       strcpy(sid,   But->wsComment[k]);
       logit( "e", "%s Got menu for: %s. %s %s %s %s <%s>\n", 
         whoami, But->plt[i].SCNLtxt, time1, time2, sip, sport, sid);
       for(j=0;j<But->plt[i].nentries;j++) {
         k = But->plt[i].index[j];
         date22 (But->plt[i].TStime[k], time1);
         date22 (But->plt[i].TEtime[k], time2);
         logit( "e", "            %d %d %s %s %s %s <%s>\n", 
           j, k, time1, time2, But->wsIp[k], But->wsPort[k], But->wsComment[k]);
       }
     }
     
     LocalTimeOffset = But->plt[i].UseLocal? But->plt[i].LocalSecs:0.0;
     ForceRebuild = 0;
     Decode_Time(StartTime + LocalTimeOffset, &t0);
     t0.Min = 0;
     t0.Sec = 0.0;
     t0.Hour = But->plt[i].HoursPerPlot*(t0.Hour/But->plt[i].HoursPerPlot);
     Encode_Time( &ZTime, &t0);
     hour1 = But->plt[i].HoursPerPlot*(t0.Hour/But->plt[i].HoursPerPlot) + 24*t0.Day;
     hour2 = But->plt[i].CurrentHour + 24*But->plt[i].CurrentDay;
     if(hour1 != hour2) {
       But->plt[i].CurrentHour = t0.Hour;
       But->plt[i].CurrentDay  = t0.Day;
       StartTime = ZTime - LocalTimeOffset;
       ForceRebuild = 1;
     }
     Duration = But->plt[i].secsPerGulp;
     EndTime = StartTime + Duration;      
     
     if ( Build_Axes( But, ZTime, ForceRebuild)) 
       return;
     
     while(EndTime < tankEndtime) {
       if(But->Debug) {
         date22 (StartTime, time1);
         date22 (EndTime,   time2);
         logit( "e", "%s Getting data for: %s. %s %s %s %s <%s>\n", 
           whoami, But->plt[i].SCNLtxt, time1, time2, sip, sport, sid);
       }

       DoModuleStuff(); /* beat the heart and check for shutdown*/

       /* Try to get some data
       ***********************/
       for(jj=0;jj<But->plt[i].nentries;jj++) {
         server = But->plt[i].index[jj];
         successful = RequestWave(But, server, Data, But->plt[i].Site, 
           But->plt[i].Comp, But->plt[i].Net, But->plt[i].Loc, StartTime, Duration);
         if(successful == 1) {   /*    Plot this trace to memory. */
           break;
         }
         else if(successful == 2) {
           continue;
         }
         else if(successful == 3) {   /* Gap in data */
           
         }
       }
       
       if(successful == 1) {   /*    Plot this trace to memory. */
         Plot_Trace(But, Data, StartTime);
       }
       
       StartTime += But->plt[i].secsPerStep;
       
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
         Save_Plot(But);
         
         if ( Build_Axes(But, ZTime, 1))
           return;
         
       }
       But->plt[i].LastTime = StartTime;
       Duration = But->plt[i].secsPerGulp;
       EndTime = StartTime + Duration;      
     }
     Save_Plot(But);
                 if (ForceRebuild == 0) IndexListUpdate(But); /** rebuild index when plot is updated 2013-06-06 -aww */
  } else 
  {
    logit( "e", "%s %s not in menu.\n", whoami, But->plt[i].SCNLtxt);
  }
return;
}


/*************************************************************************
 *   Sort_Servers                                                        *
 *      From the table of waveservers containing data for the current    *
 *      SCNL, the table is re-sorted to provide an intelligent order of   *
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

void Sort_Servers (Butler *But, double StartTime)
{
  char    whoami[NAMELEN];
  char    c22[25];
  double  tdiff[MAX_WAVESERVERS*2];
  int     i, j, k, jj, last_jj, kk, hold, index[MAX_WAVESERVERS*2];

    
  sprintf(whoami, " %s: %s: ", But->mod, "Sort_Servers");
  i = But->Current_Plot;
  /* Throw out servers with data too old. */
  j = 0;
  while(j<But->plt[i].nentries) {
    k = But->plt[i].index[j];
    if(StartTime > But->plt[i].TEtime[k]) {
      if(But->Debug) {
        date22( StartTime, c22);
        logit( "e","%s %d %d  %s", whoami, j, k, c22);
        logit( "e", " %s %s <%s>\n", 
                But->wsIp[k], But->wsPort[k], But->wsComment[k]);
        date22( But->plt[i].TEtime[k], c22);
        logit( "e","ends at: %s rejected.\n", c22);
      }
      /* Following commented out 9 June, 1999, PNL; see note at top of this
       * file. */
      /* But->inmenu[k] = 0;    */
      But->plt[i].nentries -= 1;
      for(jj=j;jj<But->plt[i].nentries;jj++) {
        But->plt[i].index[jj] = But->plt[i].index[jj+1];
      }
    } else j++;
  }
  if(But->plt[i].nentries <= 1) return;  /* nothing to sort */
            
  /* Calculate time differences between StartTime needed and tankStartTime */
  /* And copy positive values to the top of the list in the order given    */
  jj = 0;
  for(j=0;j<But->plt[i].nentries;j++) {
    k = index[j] = But->plt[i].index[j];
    tdiff[k] = StartTime - But->plt[i].TStime[k];
    if(tdiff[k]>=0) {
      But->plt[i].index[jj++] = index[j];
      tdiff[k] = -65000000; /* two years should be enough of a flag */
    }
  }
  last_jj = jj;
  
  /* Sort the index list copy in descending order */
  j = 0;
  do {
    k = index[j];
    for(jj=j+1;jj<But->plt[i].nentries;jj++) {
      kk = index[jj];
      if(tdiff[kk]>tdiff[k]) {
        hold = index[j];
        index[j] = index[jj];
        index[jj] = hold;
      }
      k = index[j];
    }
    j += 1;
  } while(j < But->plt[i].nentries);
    
  /* Then transfer the negatives */
  for( j=last_jj, k=0; j < But->plt[i].nentries; j++, k++) {
    But->plt[i].index[j] = index[k];
  }
}    


/********************************************************************
 *    Build_Axes constructs the axes for the plot by drawing the    *
 *    GIF image in memory.                                          *
 *                                                                  *
 ********************************************************************/

short Build_Axes(Butler *But, double Stime, int ForceRebuild)
{
  char    whoami[NAMELEN], c22[30], cstr[150];
  double  xmax, ymax;
  double  trace_size, tsize, yp;
  int     ntrace;
  int     ix, iy, i, j, k, kk;
  long    black, must_create;
  FILE    *in;
  TStrct  Time1;
  gdImagePtr    im_in;

  sprintf(whoami, " %s: %s: ", But->mod, "Build_Axes");
  i = But->Current_Plot;
  Decode_Time( Stime, &Time1);
  sprintf(But->plt[i].Today, "%.4d%.2d%.2d", Time1.Year, Time1.Month, 
          Time1.Day);
  sprintf(But->plt[i].GifName, "%s.%s%.2d.gif", But->plt[i].SCNLnam, 
          But->plt[i].Today, But->plt[i].CurrentHour);
  if (strlen(But->GifDir) + strlen(But->plt[i].GifName) > MAX_PATH)
  {
    logit( "e","Build_Axes: LocalGif name too long (internal error)\n");
    exit( 1 );
  }
  sprintf(But->plt[i].LocalGif, "%s%s", But->GifDir, But->plt[i].GifName );
  
  But->plt[i].GifImage = 0L;
  if(ForceRebuild) {
    must_create = 1;
  } else {
    must_create = 0;
    in = fopen(But->plt[i].LocalGif, "rb");
    if(!in) must_create = 1;
    if(in) {
      But->plt[i].GifImage = gdImageCreateFromGif(in);
      fclose(in);
      if(!But->plt[i].GifImage) must_create = 1;
      else {
        for(j=0;j<MAXCOLORS;j++) gdImageColorDeallocate(But->plt[i].GifImage, j);
        Pallette(But->plt[i].Pallette, But->plt[i].GifImage, But->plt[i].gcolor);
      }
    }
  }
  if(But->Debug) logit( "e", "%s xpix: %d ypix: %d xgpix: %d ygpix: %d axexmax: %f axeymax %f xsize: %f ysize: %f.\n\n", 
                         whoami, But->plt[i].xpix, But->plt[i].ypix, But->plt[i].xgpix, But->plt[i].ygpix, 
                         But->plt[i].axexmax, But->plt[i].axeymax, But->plt[i].xsize, But->plt[i].ysize);
                 
  if(must_create) {
    But->plt[i].GifImage = gdImageCreate(But->plt[i].xgpix, But->plt[i].ygpix);
    if(But->plt[i].GifImage==0) {
      logit( "e", "%s Not enough memory! Reduce size of image or increase memory.\n\n", whoami);
      return 1;
    }
    if(But->plt[i].GifImage->sx != But->plt[i].xgpix) {
      logit( "e", "%s Not enough memory for entire image! Reduce size of image or increase memory.\n", 
              whoami);
      logit( "e", "%s xpix: %d ypix: %d xgpix: %d ygpix: %d axexmax: %f axeymax %f xsize: %f ysize: %f.\n\n", 
              whoami, But->plt[i].xpix, But->plt[i].ypix, But->plt[i].xgpix, But->plt[i].ygpix, 
              But->plt[i].axexmax, But->plt[i].axeymax, But->plt[i].xsize, But->plt[i].ysize);
      return 1;
    }
    Pallette(But->plt[i].Pallette, But->plt[i].GifImage, But->plt[i].gcolor);
    if(But->Make_HTML) AddDay(But);
    if(But->logo) {
      in = fopen(But->logoname, "rb");
      if(in) {
        im_in = gdImageCreateFromGif(in);
        fclose(in);
        gdImageCopy(But->plt[i].GifImage, im_in, 0, 0, 0, 0, im_in->sx, im_in->sy);
        gdImageDestroy(im_in);
      }
    }
  }

  /* Plot the frame *
  ******************/
  Make_Grid(But);
    
  /* Put in the vertical axis time tics and labels * 
  ***********************************/
  xmax  = But->plt[i].axexmax;
  ymax  = But->plt[i].axeymax;
  black = But->plt[i].gcolor[BLACK];
  ntrace   =  But->plt[i].LinesPerHour*But->plt[i].HoursPerPlot;
  trace_size =  But->plt[i].axeymax/ntrace; /* height of one trace [Data] */

  /* center lines for each trace */
  /*    for(j=0;j<ntrace;j++) {
   *        iy = iyq(((float)(j)+0.5)*trace_size);
   *        gdImageLine(But->plt[i].GifImage, ixq(0.0), iy, ixq(xmax), iy, black);
   *    } */

  /* Left and right axis labels */
  for (j=0; j<But->plt[i].HoursPerPlot; j++ ) {
    /* hours on left axis */
    k = (But->plt[i].UseLocal)? 
      j + But->plt[i].CurrentHour:
      j + But->plt[i].CurrentHour + But->plt[i].LocalTime;
    if(k <  0) k += 24;
    if(k > 23) k -= 24;
    /* hours on right axis */
    kk = (But->plt[i].ShowUTC)? k - But->plt[i].LocalTime: k;
    if(kk <  0) kk += 24;
    if(kk > 23) kk -= 24;
        
    yp =  trace_size*j*But->plt[i].LinesPerHour + 0.5*trace_size;
    ix = ixq(-0.5); 
    iy = iyq(yp) - 5;  
    /* don't print too close to the top */
    if ( iy < 72.0 * YTMargin + 5 ) continue;
    
    sprintf(cstr, "%02d:00", k);
    gdImageString(But->plt[i].GifImage, gdFontMediumBold, ix, iy, cstr, black);
    ix = ixq(xmax + 0.05); 
    if ( But->plt[i].mins < 60 )
      sprintf(cstr, "%02d:%02d", kk, But->plt[i].mins);
    else  /* maximum of 60 minutes per line */
    {
      kk += 1;
      if(kk > 23) kk -= 24;
      sprintf(cstr, "%02d:00", kk);
    }
    gdImageString(But->plt[i].GifImage, gdFontMediumBold, ix, iy, cstr, black);
  }
    
  /* Units for the axis labels */
  iy = (int) (72.0 * YTMargin);
  gdImageString(But->plt[i].GifImage, gdFontMediumBold, ixq(-XLMargin), iy, 
                But->plt[i].LocalTimeID, black);  
  ix = ixq(xmax) + 15;   
  if(But->plt[i].ShowUTC) sprintf(cstr, "UTC") ;
  else strcpy(cstr, But->plt[i].LocalTimeID);
  gdImageString(But->plt[i].GifImage, gdFontMediumBold, ix, iy, cstr, black);  
      
  if(But->plt[i].DCremoved) {
    ix = ixq(xmax + 0.55);   
    sprintf(cstr, "   DC") ;
    gdImageString(But->plt[i].GifImage, gdFontTiny, ix, iy, cstr, black);
  }
    
  /* Write label with the Channel ID, Date * 
  *****************************************/
  Decode_Time( Stime, &Time1);
  Time1.Hour = Time1.Min = 0;
  Time1.Sec = 0.0;
  Encode_Time( &Time1.Time, &Time1);
  date22 (Time1.Time, c22);
     
  iy = iy-15;
  if(But->plt[i].Comment[0]!=0) {
    sprintf(cstr, "(%s) ", But->plt[i].Comment) ;
    gdImageString(But->plt[i].GifImage, gdFontMediumBold, ixq(0.0), iy, cstr, black);
    iy = iy-15;
  }
  sprintf(cstr, "%s ", But->plt[i].SCNLtxt) ;
  gdImageString(But->plt[i].GifImage, gdFontMediumBold, ixq(0.0), iy, cstr, black);    
  sprintf(cstr, "%.11s", c22) ;
  gdImageString(But->plt[i].GifImage, gdFontMediumBold, ixq(0.0), iy-15, cstr, black);    

  /* Measure it, at the bottom */
  j = (int) (But->plt[i].ysize*72 - 5);
  tsize = 1.0/(But->plt[i].Scale);
  sprintf(cstr, "Each Vertical Division = %7.2f microvolts", tsize) ;
  gdImageString(But->plt[i].GifImage, gdFontTiny, 20, j-5, cstr, black);    
    
  /* Label for optional clipping */
  if ( But->Clip ) {
    sprintf(cstr, "Traces clipped at plus/minus %d vertical divisions",
            But->Clip );
    gdImageString(But->plt[i].GifImage, gdFontTiny, ixq(xmax/2.0 + 1.0), j-5,
                  cstr, black );
  }
    
  return 0;
}


/********************************************************************
 *    Make_Grid constructs the grid overlayed on the plot.          *
 *                                                                  *
 ********************************************************************/

void Make_Grid(Butler *But)
{
  char    string[150];
  double  xmax, ymax, y0, in_sec, tsize;
  long    isec, xp, i, j, k, black, major_incr, minor_incr, tiny_incr;
  int     inx[]={0,1,2,2,2,3,4,4,4,4,5}, iny[]={2,3,2,1,0,1,1,0,2,3,3};

  i = But->Current_Plot;
  /* Plot the frame *
  ******************/
  xmax  = But->plt[i].axexmax;
  ymax  = But->plt[i].axeymax;
  black = But->plt[i].gcolor[BLACK];
  gdImageRectangle( But->plt[i].GifImage, ixq(0.0), iyq(ymax), ixq(xmax),
                    iyq(0.0), black);

  /* Make the x-axis ticks * 
  *************************/
  in_sec = xmax / (But->plt[i].mins*60);  /* Actually, inches per minute */
  /* Adjust spacing of major and minor ticks. Based on xmax of 10 inches */
  tiny_incr = 0;                /* no `second' marks */
  if ( in_sec < 0.005 )                /* for 1 hour per line */
  {
    major_incr = 300;
    minor_incr = 60;
  }
  else if (in_sec < 0.05 )        /* for 30 min or less per line */
  {
    major_incr = 60;
    minor_incr = 10;
  } 
  else                        /* for 5 min or less per line with xmax = 20in */
  {
    major_incr = 60;
    minor_incr = 10;
    tiny_incr = 1;                /* turn on `second' marks */
  }
  y0 = ysize - YBMargin;  /* The bottom axis */
  for ( isec=0; isec<=But->plt[i].mins*60; isec++) {
    xp = ixq(isec*in_sec);
    if ((div(isec, major_incr)).rem == 0) 
    {
      tsize = 0.15;                    /* major ticks */
      sprintf(string, "%.2ld", isec / 60);        /* label */
      gdImageString(But->plt[i].GifImage, gdFontMediumBold, xp-6, 
                    (long)(72.0 * (y0 + 0.15)), string, black);
      gdImageLine(But->plt[i].GifImage, xp, iyq(0.0), xp, iyq(ymax), 
                  But->plt[i].gcolor[GREY]); /* make the minute mark */
    }
    else if ((div(isec, minor_incr)).rem == 0) 
      tsize = 0.10;                    /* minor ticks */
    else if (tiny_incr)
      tsize = 0.05;                    /*  1 sec ticks */
    else
      tsize = 0.0;                /* no ticks */
    if ( tsize > 0.0 )                /* make the tick */
      gdImageLine(But->plt[i].GifImage, xp, (long)(72.0*y0), xp, 
                  (long)(72.0*(y0+tsize)), black); 
  }
  gdImageString(But->plt[i].GifImage, gdFontMediumBold, ixq(xmax/2.0 - 0.5),
                (long)(72.0*(y0+0.3)), "TIME (MINUTES)", black);


  /* Initial it *
  **************/
  j = (int) (But->plt[i].ysize*72 - 5);
  for(k=0;k<11;k++) gdImageSetPixel(But->plt[i].GifImage, inx[k]+2, j+iny[k], 
                                    black);
}


/*******************************************************************************
 *    Pallette defines the pallete to be used for plotting.                    *
 *     PALCOLORS colors are defined.                                           *
 *                                                                             *
 *******************************************************************************/

void Pallette(int ColorFlag, gdImagePtr GIF, long color[])
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
    
  gdImageColorTransparent(GIF, -1 );
}

/*******************************************************************************
 *    Plot_Trace plots an individual trace (Data)  and stuffs it into          *
 *     the GIF image in memory.                                                *
 *                                                                             *
 *******************************************************************************/

void Plot_Trace(Butler *But, double *Data, double Stime)
{
  char    whoami[NAMELEN];
  double  x, y, xinc, Middle, samp_pix, atime;
  double  in_sec, ycenter, sf, value, trace_size;
  int     i, j, k, ix, iy, LineNumber, npts;
  int     lastx, lasty, decimation, acquired, minutes;
  long    trace_clr;
  TStrct  StartTime;    

  sprintf(whoami, " %s: %s: ", But->mod, "Plot_Trace");
  i = But->Current_Plot;
    
  atime =  But->plt[i].UseLocal? Stime+But->plt[i].LocalSecs:Stime;
  atime = atime - 3600.0*But->plt[i].CurrentHour;
  Decode_Time( atime, &StartTime);
    
  LineNumber = But->plt[i].LinesPerHour*StartTime.Hour + StartTime.Min/But->plt[i].mins;
    
  trace_size =  But->plt[i].axeymax/(But->plt[i].LinesPerHour*But->plt[i].HoursPerPlot); /* height of one trace [Data] */
  ycenter =  trace_size*LineNumber + 0.5*trace_size;
  npts = But->plt[i].Npts;
    
  sf = trace_size*But->plt[i].Scale;
  in_sec     = But->plt[i].axexmax / (But->plt[i].mins*60.0);
  decimation = 2;
  samp_pix = But->plt[i].samp_sec / in_sec / 72.0;
  decimation = (int) ((samp_pix/4 < 1)? 1:samp_pix/4);
  xinc = decimation * in_sec / But->plt[i].samp_sec;       /* decimation */
  if(But->Debug) {
    logit( "e", "%s %s. sf: %f in_sec: %f samp_pix: %f samp_sec: %d decimation: %d xinc: %f\n", 
            whoami, But->plt[i].SCNLtxt, sf, in_sec, samp_pix, But->plt[i].samp_sec, decimation, xinc);
  }

  /* Plot the trace *
  ******************/
  Decode_Time( Stime, &StartTime);
  minutes = 60*StartTime.Hour + StartTime.Min;
  minutes = div(minutes,But->plt[i].mins).rem;
  if ( But->plt[i].first_gulp) {
    But->plt[i].first_gulp = 0;
    Find_Mean(But, Data, npts, 0, Stime, LineNumber);
  } else if ( minutes==0 ) 
    Find_Mean(But, Data, npts, 0, Stime, LineNumber);
  x = in_sec*60.0*minutes;
    
  trace_clr = But->plt[i].gcolor[BLACK];
  k = div(LineNumber, 4).rem + 1;
  trace_clr = But->plt[i].gcolor[k];
  Middle = But->plt[i].DCremoved?  But->plt[i].DCcorr:0.0;
  acquired = 0;
  for(j=0;j<npts;j+=decimation) {
    x += xinc;
    if (x > But->plt[i].axexmax) {
      if(j+2*decimation > npts) break;
      LineNumber += 1;
      ycenter += trace_size;
      if (ycenter > But->plt[i].axeymax) break;
      x = 0.0;
      k = div(LineNumber, But->plt[i].LinesPerHour).rem;
      k = div(k, 4).rem + 1;
      trace_clr = But->plt[i].gcolor[k];
      atime = Stime + (float)j/But->plt[i].samp_sec;
      Find_Mean(But, Data, npts, j, atime, LineNumber);
      Middle = But->plt[i].DCremoved?  But->plt[i].DCcorr:0.0;
      acquired = 0;  
    }
    if(Data[j] != 919191) {
      value = (Data[j] - Middle)*But->plt[i].Gain *sf;
      if (But->Clip) {
        if ( value > But->Clip * trace_size ) 
          value = But->Clip * trace_size;
        else if ( -value > But->Clip * trace_size )
          value = - But->Clip * trace_size;
      }
      y = ycenter + value;
      ix = ixq(x);    iy = iyq(y);
      if(acquired) {
        gdImageLine(But->plt[i].GifImage, ix, iy, lastx, lasty, trace_clr);
      }
      lastx = ix;  lasty = iy;
      acquired = 1;
    }
  } 
  sleep_ew( 100 );   /* give the wave_server a break */
}


/*******************************************************************************
 *    Find_Mean finds the mean and rms for an individual trace (Data) for the  *
 *     current minute, plots them at end of line and stuffs them into a file   *
 *     if requested.                                                           *
 *                                                                             *
 *******************************************************************************/

void Find_Mean(Butler *But, double *Data, int maxpts, int index, double Stime, int LineNumber)
{
  char    DCFile[NAMELEN], string[30], whoami[NAMELEN];
  double  in_sec, samp_pix, mpts, rms, ycenter, max, min, val, trace_size;
  int     i, j, ix, iy, decimation;
  long    npts;
  TStrct  StartTime;    
  FILE    *out;

  sprintf(whoami, " %s: %s: ", But->mod, "Find_Mean");
  i = But->Current_Plot;
  npts = (long) 60 * (long) But->plt[i].samp_sec;
  if(npts  > maxpts) npts  = maxpts;
  if(index > maxpts) index = maxpts - npts;
    
  in_sec     = But->plt[i].axexmax / (But->plt[i].mins*60.0);
  samp_pix = But->plt[i].samp_sec / in_sec / 72.0;
  decimation = (int) ((samp_pix/4 < 1)? 1:samp_pix/4);

  mpts = But->plt[i].Mean = rms = 0.0;
  for(j=0;j<npts;j+=decimation) {
    if(Data[j+index]!=919191) {
      mpts += 1;
      But->plt[i].Mean += Data[j+index];
    }
  }
  But->plt[i].Mean = But->plt[i].Mean/mpts;
  min =  5000000.0;
  max = -5000000.0;
  for(j=0;j<npts;j+=decimation) {
    if(Data[j+index]!=919191) {
      val = Data[j+index]-But->plt[i].Mean;
      rms += val*val/mpts;
      if(min > val) min = val;
      if(max < val) max = val;
    }
  }
  rms = sqrt(rms);
  But->plt[i].DCcorr = But->plt[i].Mean;
    
  if(But->plt[i].DCremoved) {
    trace_size =  But->plt[i].axeymax/(But->plt[i].LinesPerHour*But->plt[i].HoursPerPlot); /* height of one trace [Data] */
    Decode_Time( Stime, &StartTime);
    ycenter =  trace_size*LineNumber + 0.5*trace_size;
    ix = ixq(But->plt[i].axexmax + 0.55); 
    iy = iyq(ycenter) - 3;
    sprintf(string, "%7.0f", But->plt[i].DCcorr);
    gdImageString(But->plt[i].GifImage, gdFontTiny, ix, iy, string, But->plt[i].gcolor[BLACK]);
  }
    
  if(But->SaveDrifts) {
    sprintf(DCFile, "%s%s.drft", But->GifDir, But->plt[i].SCNL);
    out = fopen(DCFile, "a");
    if(out == 0L) {
      out = fopen(DCFile, "w");
      if(out != 0L) {
        fprintf(out, "%.2d %.2d %.4d %.2d:%.2d  %7.0f %7.0f %7.0f %7.0f\n", 
                StartTime.Day, StartTime.Month, StartTime.Year, 
                StartTime.Hour, StartTime.Min, But->plt[i].DCcorr, rms,
                min, max);
        fclose(out);
      } else {
        logit( "e", "%s Unable to Open drift file: %s\n", whoami, DCFile);
      }
    } else {
      fprintf(out, "%.2d %.2d %.4d %.2d:%.2d  %7.0f %7.0f %7.0f %7.0f\n", 
              StartTime.Day, StartTime.Month, StartTime.Year, 
              StartTime.Hour, StartTime.Min, But->plt[i].DCcorr, rms,
              min, max);
      fclose(out);
    }
  }
}


/*********************************************************************
 *   CommentList()                                                   *
 *    Build and send a file relating SCNLs to their comments.         *
 *********************************************************************/

void CommentList(Butler *But)
{
  char    FileName[200], whoami[NAMELEN];
  int     i;
  FILE    *out;
    
  sprintf(whoami, " %s: %s: ", Module, "CommentList");
  sprintf(FileName, "%sznamelist.dat", But->GifDir);
  out = fopen(FileName, "wb");
  if(out == 0L) {
    logit( "e", "%s Unable to open NameList File: %s\n", whoami, FileName);    
  } else {
    for(i=0;i<But->nPlots;i++) {
      fprintf(out, "%s.%s\n", But->plt[i].SCNLnam, But->plt[i].Comment);
    }
    fclose(out);
  }
    
            

}


/*********************************************************************
 *   IndexList()                                                     *
 *    Build the master .html page with pointers for each SCNL         *
 *********************************************************************/

void IndexList(Butler *But)
{
  char    FileName[200], whoami[NAMELEN];
  int     i;
  FILE    *in;
    
  sprintf(whoami, " %s: %s: ", Module, "IndexList");
  /* Make sure all needed history files are in place
  *************************************************/   
  for(i=0;i<But->nPlots;i++) {
    sprintf(FileName, "%s%s.hist", But->GifDir, But->plt[i].SCNLnam);
    in = fopen(FileName, "r");
    if(in == 0L) {
      in = fopen(FileName, "wb");
      if(in == 0L) 
        logit( "e", "%s Unable to open History File: %s\n", whoami, FileName);    
      else {
        fprintf(in, "\n");
        fclose(in);
      }
    } else fclose(in);
  }
  IndexListUpdate(But);   
}


/*********************************************************************
 *   AddDay()                                                        *
 *    If Today is not in the history file for this SCNL,              *
 *      1) Today is added to the history file.                       *
 *      2) An html wrapper file is built for the current GIF         *
 *         and sent to targets                                       *
 *      3) The Index list (welcome.html) is updated                  *
 *********************************************************************/

void AddDay(Butler *But)
{
  char    whoami[NAMELEN], today[NAMELEN];
  char    string[200], FileName[100], DummyFileName[100];
  FILE    *in, *out;
  int     i, ierr, count;
    
  sprintf(whoami, " %s: %s: ", But->mod, "AddDay");
  i = But->Current_Plot;
  /* Test to see if we need to add this entry
  ******************************************/   
  ierr = 1;     
  sprintf(FileName, "%s%s.hist", But->GifDir, But->plt[i].SCNLnam);
  sprintf(today, "%s%.2d", But->plt[i].Today, But->plt[i].CurrentHour);
  in = fopen(FileName, "r");
  if(in == 0L) 
    logit( "e", "%s Unable to read History File: %s\n", whoami, FileName);    
  else {
    while(fgets(string, 128, in)!=0L) {
      if(strstr(string, today)) ierr = 0;
    } 
    fclose(in);
  }
    
  if(ierr) {    /* Copy the History file to dummy with insertion of this entry */        
    sprintf(DummyFileName, "%s%s", But->GifDir, "dummy");
    count = 0;
    out = fopen(DummyFileName, "wb");
    if(out == 0L) 
      logit( "e", "%s Unable to write dummy History File: %s for %s\n", 
              whoami, DummyFileName, But->plt[i].SCNLnam);
    else {        
      fprintf(out, "%s%.2d\n", But->plt[i].Today, But->plt[i].CurrentHour);
      count += 1;
      in = fopen(FileName, "r");
      if(in == 0L) 
        logit( "e", "%s Unable to read History File: %s for %s\n", 
                whoami, FileName, But->plt[i].SCNLnam);    
      else {
        while(fgets(string, 128, in)!=0L) {
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
      logit( "e", "%s Unable to read dummy History File: %s for %s\n", 
              whoami, DummyFileName, But->plt[i].SCNLnam);
    else {        
      out = fopen(FileName, "wb");
      if(out == 0L) 
        logit( "e", "%s Unable to write History File: %s for %s\n", 
                whoami, FileName, But->plt[i].SCNLnam);    
      else {
        while(fgets(string, 128, in)!=0L) {
          fprintf(out, "%s", string);
        } 
        fclose(out);
      } 
      fclose(in);
    }
        
  }
  IndexListUpdate(But);   /* moved this outside the check if today is there */
}


/*********************************************************************
 *   IndexListUpdate()                                               *
 *    Update the master welcome.html with pointers for each SCNL      *
 *********************************************************************/
#define NUM_READ_CHARS 1000
void IndexListUpdate(Butler *But)
{
  char    string[NUM_READ_CHARS], datetxt[20], FileName[512]; 
  char    History[512], temp[512], whoami[NAMELEN];
  char    hour[10];
  FILE    *in, *out, *hist;
  int     i, j, nday;
  time_t  now;
    
  sprintf(whoami, " %s: %s: ", But->mod, "IndexListUpdate");
  /* Build the welcome.html file 
     with the insertion of the SCNLs along the way 
     *********************************************************/    

  if (But->IndexFile[0] == 0) {
    /* welcome.html is the default value */
    sprintf( FileName, "%s/welcome.html", But->GifDir);   /* current copy of the full welcome.html file */
  } else {
    sprintf( FileName, "%s/%s", But->GifDir, But->IndexFile);   
  }
  out = fopen(FileName, "wb");
  if(out == 0L) 
    logit( "e", "%s Unable to open welcome.html File: %s\n", whoami, FileName);    
  else {
    sprintf( temp, "%sindexa.html", But->GifDir); 
    in  = fopen(temp, "r");
    if(in == 0L) {
      fprintf(out, "<HTML><HEAD><META http-equiv=\"refresh\" content=\"%d\">\n", BStruct.UpdateInt);
      fprintf(out, "<TITLE>\nRecent Helicorder Displays\n</TITLE></HEAD>\n");
      fprintf(out, "<BODY BGCOLOR=#EEEEEE TEXT=#333333 vlink=purple>\n");
      fprintf(out, "<A NAME=\"top\"></A><CENTER><H2>\n");
      /*    fprintf(out, "<IMG SRC=\"smusgs.gif\" WIDTH=58 HEIGHT=35 ALT=\"Logo\" ALIGN=\"middle\">\n"); */
      fprintf(out, "<FONT COLOR=red>Recent Helicorder Displays</FONT></H2><br>\n\n");
    } else {
      while(fgets(string, NUM_READ_CHARS, in)!=0L) fprintf(out, "%s", string);
      fclose(in);
    }
        
    for(i=0;i<But->nPlots;i++) {
      sprintf(History, "%s%s.hist", But->GifDir, But->plt[i].SCNLnam);
      hist = fopen(History, "r");
      if(hist == 0L) 
        logit( "e", "%s Unable to read History File: %s for %s\n", whoami, History, But->plt[i].SCNLnam);    
      else {
        fprintf(out, "<p><CENTER> %s : %s </CENTER>\n", But->plt[i].SCNLtxt,
                But->plt[i].Comment);
        fprintf(out, "<CENTER>");
        nday = 0;
        while(fgets(string, 128, hist)!=0L) {
          j = strlen(string);
          if(j>2 && string[0]!='#') {
            nday++;
            if (j > 9 && j < 12 )
            { /* String includes more than YYYYMMDD\n, probably hour as HH */
              strcpy(hour, string+8);
              hour[2] = '\0';
            }
            else
              hour[0] = '\0';

            string[j-1] = '\0';
            strcpy(datetxt, string);
            Make_Date(datetxt);
            if (hour[0] != '\0')
            {
              j = strlen(datetxt);
              sprintf(&(datetxt[j]), " (%s)", hour);
            }
            now = time(NULL);
            if ( nday < But->Days2Save ) 
              fprintf(out, "<a href=\"%s.%s.gif?v=%d\" > %s</a> | ", 
                      But->plt[i].SCNLnam, string, (int) now, datetxt);
            else
              fprintf(out, "<a href=\"%s.%s.gif?v=%d\" > %s</a>", 
                      But->plt[i].SCNLnam, string, (int) now, datetxt);
          }
        } 
        fclose(hist);
        fprintf(out, "</CENTER>\n\n");
      }
    }
    sprintf( temp, "%sindexb.html", But->GifDir); 
    in  = fopen(temp, "r");
    if(in == 0L) {
          fprintf(out, "<P><HR><font color=red></font>\n");
      fprintf(out, "<P><A HREF=\"#top\">Top of this page\n</A>\n");
          fprintf(out, "<CENTER><font size=-2>\n\n Generated with gd, by Thomas Boutell\n\n</font>");   
      fprintf(out, "</BODY></HTML>\n");
    } else {
      while(fgets(string, NUM_READ_CHARS, in)!=0L) fprintf(out, "%s", string);
      fclose(in);
    } 
    fclose(out);
            

  }
}


/*********************************************************************
 *   Make_Date()                                                     *
 *    Expands a string of form YYYYMMDD to MM/DD/YYYY                *
 *********************************************************************/

void Make_Date(char *date)
{
  char    y[5], m[3], d[3];
  int     i;
    
  for(i=0;i<4;i++) y[i] = date[i];
  for(i=0;i<2;i++) m[i] = date[i+4];
  for(i=0;i<2;i++) d[i] = date[i+6];
    
  for(i=0;i<2;i++) date[i] = m[i];
  date[2] = '/';
  for(i=0;i<2;i++) date[i+3] = d[i];
  date[5] = '/';
  for(i=0;i<4;i++) date[i+6] = y[i];
  date[10] = 0;
}    


/*********************************************************************
 *   Save_Plot()                                                     *
 *    Saves the current version of the GIF image and ships it out.   *
 *********************************************************************/

void Save_Plot(Butler *But)
{
  char    whoami[NAMELEN];
  FILE    *out;
  int     i, ierr;
    
  sprintf(whoami, " %s: %s: ", But->mod, "Save_Plot");
  i = But->Current_Plot;
  Make_Grid(But);
  /* Make the GIF file. *
  **********************/        
  ierr = 0;
  out = fopen(But->plt[i].LocalGif, "wb");
  if(out == 0L) 
  {
    logit( "e", "%s Unable to write GIF File: %s\n", whoami, 
            But->plt[i].LocalGif); 
    But->NoGIFCount += 1;
    if(But->NoGIFCount > 5) 
    {
      logit( "e", "%s Unable to write GIF in 5 consecutive trys. Exiting.\n", whoami);
      exit(-1);
    }
  } 
  else 
  {
    But->NoGIFCount = 0;
    gdImageGif(But->plt[i].GifImage, out);
    fclose(out);
    ierr = 1;
  }
  gdImageDestroy(But->plt[i].GifImage);
  if(ierr) 
  {
    if(But->Debug) 
      logit( "e", "%s Writing GIF File: %s\n", whoami, 
              But->plt[i].GifName);

  }
}


/*************************************************************************
 *   Build_Menu ()                                                       *
 *      Builds the waveservers' menus                                    *
 *      Each waveserver has its own menu so that we can do intelligent   *
 *      searches for data.                                               *
 *************************************************************************/

void Build_Menu (Butler *But)
{
  char    whoami[NAMELEN], server[100];
  long    i, ii, retry, ret;
 
  sprintf(whoami, " %s: %s: ", But->mod, "Build_Menu");
  ii = But->Current_Plot;
  setWsClient_ewDebug(0);  
  if(But->WSDebug) setWsClient_ewDebug(1);  
  But->got_a_menu = 0;
    
  for (i=0;i< But->nServer; i++) {
    retry = 0;
    But->inmenu[i] = 0;
    sprintf(server, " %s:%s <%s>", But->wsIp[i], But->wsPort[i], But->wsComment[i]);
  Append:
        
    ret = wsAppendMenu(But->wsIp[i], But->wsPort[i], &But->menu_queue[i], But->wsTimeout);
        
    if (ret == WS_ERR_NONE) {
      But->inmenu[i] = But->got_a_menu = 1;
    }
    else 
      switch ( ret ) { 
      case WS_ERR_NO_CONNECTION:
        if(But->Debug) 
          logit( "e","%s Could not get a connection to %s to get menu.\n", 
                  whoami, server);
        break;
      case WS_ERR_SOCKET:
        logit( "e","%s Could not create a socket for %s\n", 
                whoami, server);
        break;
      case WS_ERR_BROKEN_CONNECTION:
        logit( "e","%s Connection to %s broke during menu\n", 
                whoami, server);
        if (retry++ < But->RetryCount) goto Append;
        break;
      case WS_ERR_TIMEOUT:
        logit( "e","%s Connection to %s timed out during menu.\n", 
                whoami, server);
        if (retry++ < But->RetryCount) goto Append;
        break;
      case WS_ERR_MEMORY:
        logit( "e","%s: error allocating memory during menu.\n", whoami);
        break;
      case WS_ERR_INPUT:
        logit( "e","%s bad/empty inputs to menu\n", whoami);
        break;
      case WS_ERR_PARSE:
        logit( "e","%s Parser failed for %s\n", whoami, server);
        break;
      case WS_ERR_BUFFER_OVERFLOW:
        logit( "e","%s Buffer overflowed for %s\n", whoami, server);
        break;
      case WS_ERR_EMPTY_MENU:
        logit( "e","%s Unexpected empty menu from %s\n", whoami, server);
        break;
      default: 
        logit( "e","%s Connection to %s returns error: %d\n", whoami, 
                server, ret);
      }
  }
  
}


/*************************************************************************
 *   In_Menu_list                                                        *
 *      Determines if the scn is in the waveservers' menu.               *
 *      If there, the tank starttime and endtime are returned.           *
 *      Also, the Server IP# and port are returned.                      *
 *************************************************************************/

int In_Menu_list (Butler *But)
{
  char    whoami[NAMELEN], server[100];
  int     i, j, rc;
  WS_PSCNL scnlp;
    
  sprintf(whoami, " %s: %s: ", But->mod, "In_Menu_list");
  i = But->Current_Plot;
  But->plt[i].nentries = 0;
  for(j=0;j<But->nServer;j++) {
    if(But->inmenu[j]) {
      sprintf(server, " %s:%s <%s>", But->wsIp[j], But->wsPort[j], But->wsComment[j]);
      rc = wsGetServerPSCNL( But->wsIp[j], But->wsPort[j], &scnlp, &But->menu_queue[j]);    
      if ( rc == WS_ERR_EMPTY_MENU ) {
        if(But->Debug) logit( "e","%s Empty menu for %s \n", whoami, server);
        But->inmenu[j] = 0;    
        continue;
      }
      if ( rc == WS_ERR_SERVER_NOT_IN_MENU ) {
        if(But->Debug) logit( "e","%s  %s not in menu.\n", whoami, server);
        But->inmenu[j] = 0;    
        continue;
      }

      while ( 1 ) {
        if(strcmp(scnlp->sta,  But->plt[i].Site)==0 && 
           strcmp(scnlp->chan, But->plt[i].Comp)==0 && 
           strcmp(scnlp->loc, But->plt[i].Loc)==0 && 
           strcmp(scnlp->net,  But->plt[i].Net )==0) {
          But->plt[i].TStime[j] = scnlp->tankStarttime;
          But->plt[i].TEtime[j] = scnlp->tankEndtime;
          But->plt[i].index[But->plt[i].nentries]  = j;
          But->plt[i].nentries += 1;
          break;  /* no need to search once we found it */
        }
        if ( scnlp->next == NULL )
          break;
        else
          scnlp = scnlp->next;
      }
    }
  }
  if(But->plt[i].nentries>0) return 1;
  return 0;
}


/********************************************************************
 *  RequestWave                                                     *
 *                                                                  *
 *   i - plot index                                                 *
 *   k - waveserver index                                           *
 * It seems like what's done  here is to get ascii data, endlessly  *
 * deal with error cases, then convert the ascii data to float's,   *
 * and finally decimate it (Alex)                                        *
 ********************************************************************/
short RequestWave(Butler *But, int k, double *Data, 
      char *Site, char *Comp, char *Net, char *Loc, double Stime, double Duration)
{
  char     *token, server[wsADRLEN*3], whoami[NAMELEN], SCNLtxt[19];
  float    samprate, sample;
  int      i, j, nsamp, nbytes, io, retry;
  double   temp, temp1;           
  int      success, ret, WSDebug = 0;          
  TRACE_REQ   request;
  WS_MENU  menu = NULL;
  WS_PSCNL  pscnl = NULL;
    
  sprintf(whoami, " %s: %s: ", But->mod, "RequestWave");
  i = But->Current_Plot;
  WSDebug = But->WSDebug;
  success = retry = 0;

  if(WSDebug) logit( "e"," %s gettrace.\n", whoami); 
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
  request.bufLen  = MAXTRACEBUF;
  request.timeout = But->wsTimeout;
  request.fill    = 919191;
  sprintf(SCNLtxt, "%s %s %s %s", Site, Comp, Net, Loc);
    
  strcpy(server, But->menu_queue[k].head->addr);
  strcat(server, "  ");
  strcat(server, But->menu_queue[k].head->port);
  
  /* Get the trace
  ***************/
  if(WSDebug) 
  {   
    logit( "e","\n%s Issuing request to wsGetTraceAsciiL: server: %s\n", whoami, server);
    logit( "e","    %s %f %f %f %d\n", SCNLtxt, But->plt[i].LastTime, Stime, request.reqEndtime, request.timeout);
    logit( "e"," %s server: %s\n", whoami, server); 
  }
  gettrace:    
  io = wsGetTraceAsciiL(&request, &But->menu_queue[k], But->wsTimeout);

  if (io == WS_ERR_NONE )
  {
    if(WSDebug) 
    {
      logit( "e"," %s server: %s trace %s: went ok first time. Got %ld bytes\n", whoami, server, SCNLtxt, request.actLen); 
      logit( "e","%s server: %s Return from wsGetTraceAsciiL: %d\n", whoami, server, io);
      logit( "e","        actStarttime=%lf, actEndtime=%lf, actLen=%ld, samprate=%lf\n",
                request.actStarttime,
                request.actEndtime,
                request.actLen,
                request.samprate);
    }
  } 
  else                      /* @@@ start of else block "some problem occurred" @@@ */
  {
    switch ( io ) 
    {                      /* **** start of switch( io) statment */
      case WS_ERR_EMPTY_MENU:
      /**********************/
        logit( "e"," %s server: %s No menu found.  We might as well quit.\n", whoami, server); 
        return 2;
        break;

      case WS_WRN_FLAGGED:
      /*******************/
        if ( (int)strlen( &request.retFlag) > 0) 
        {
          if(WSDebug) logit( "e","%s server: %s Trace %s: return flag from wsGetTraceAscii: <%c>\n %.80s\n", 
                whoami, server, SCNLtxt, request.retFlag, But->TraceBuf);
          if(request.retFlag == 'L') return 3;
          if(request.retFlag == 'R') return 3;
          if(request.retFlag == 'G') return 3;
        }
        logit("e", " %s server: %s Trace %s: No trace available. Wave server returned %c\n", 
              whoami, server, SCNLtxt, request.retFlag); 
        return 2;

      case WS_ERR_SCNL_NOT_IN_MENU:
      /**************************/
        logit( "e"," %s server: %s Trace %s not in menu\n", whoami, server, SCNLtxt);
        return 2;
      
      case WS_ERR_BUFFER_OVERFLOW:
      /**************************/
        logit( "e"," %s server: %s Trace %s overflowed buffer. Fatal.\n", whoami, server, SCNLtxt); 
        return 2;
      
      case WS_ERR_BROKEN_CONNECTION:
      /****************************/
        sleep_ew(500);
        retry += 1;
        if(WSDebug) logit( "e"," %s server: %s Trace %s: Broken connection to wave server. Try again.\n", whoami, server, SCNLtxt); 
        if ( (ret = wsSearchSCNL( &request, &menu, &pscnl, &But->menu_queue[k] )) != WS_ERR_NONE ) return 2;
        ret = wsAttachServer( menu, But->wsTimeout );
        if ( ret != WS_ERR_NONE ) 
        {
          if(WSDebug) 
          {
            switch ( ret ) 
            {
              case WS_ERR_NO_CONNECTION:
              logit( "e"," %s server: %s: No connection to wave server.\n", whoami, server);
              break;
              case WS_ERR_SOCKET:
              logit( "e"," %s server: %s: Socket error.\n", whoami, server); 
              break;
              case WS_ERR_INPUT:
              logit( "e"," %s server: %s: Menu missing.\n", whoami, server); 
              break;
              default:
              logit( "e"," %s server: %s: wsAttachServer error %d.\n", whoami, server, ret); 
            }
          }
          return 2;
        }
        if(retry < But->RetryCount) goto gettrace;
        return 2;
        break;

      case WS_ERR_TIMEOUT:
      /******************/
        sleep_ew(500);
        retry += 1;
        logit( "e"," %s server: %s Trace %s: Timeout to wave server. Try again.\n", whoami, server, SCNLtxt); 
        if ( (ret = wsSearchSCNL( &request, &menu, &pscnl, &But->menu_queue[k] )) != WS_ERR_NONE ) return 2;
        ret = wsAttachServer( menu, But->wsTimeout );
        if ( ret != WS_ERR_NONE ) 
        {
          if(WSDebug) 
          {   
            switch ( ret ) 
            {
              case WS_ERR_NO_CONNECTION:
              logit( "e"," %s server: %s: No connection to wave server.\n", whoami, server);
              break;
              case WS_ERR_SOCKET:
              logit( "e"," %s server: %s: Socket error.\n", whoami, server);
              break;
              case WS_ERR_INPUT:
              logit( "e"," %s server: %s: Menu missing.\n", whoami, server); 
              break;
              default:
              logit( "e"," %s server: %s: wsAttachServer error %d.\n", whoami, server, ret); 
            }
          }
          return 2;
        }
        if(retry < But->RetryCount) goto gettrace;
        return 2;

      case WS_ERR_NO_CONNECTION:
      /************************/
        logit( "e"," %s server: %s Trace %s: No connection to wave server.\n", whoami, server, SCNLtxt); 
        sleep_ew(500);
        retry += 1;
        if(WSDebug) logit( "e"," %s server: %s: Socket: %d.\n", whoami, server, menu->sock); 
        ret = wsAttachServer( menu, But->wsTimeout );
        if(WSDebug)  logit( "e"," %s server: %s: Socket: %d.\n", whoami, server, menu->sock); 
        if ( ret != WS_ERR_NONE ) 
        {
          if(WSDebug) 
          {   
            switch ( ret ) 
            {
              case WS_ERR_NO_CONNECTION:
              logit( "e"," %s server: %s: No connection to wave server.\n", whoami, server);
              break;
              case WS_ERR_SOCKET:
              logit( "e"," %s server: %s: Socket error.\n", whoami, server);
              break;
              case WS_ERR_INPUT:
              logit( "e"," %s server: %s: Menu missing.\n", whoami, server); 
              break;
              default:
              logit( "e"," %s server: %s: wsAttachServer error %d.\n", whoami, server, ret);
            }
          }
          return 2;
        }
        if(retry < But->RetryCount) goto gettrace;
        return 2;
        default:
        logit( "e","%s server: %s Failed.  io = %d\n", whoami, server, io );
        return 2;
    }              /* **** end of case(io) statment *** */
  }                /* @@@@ end of else block "some problem occurred" @@@@@ */

  /* trace data retrieved; now deal with it 
  *****************************************/
  But->plt[i].actStime = request.actStarttime;
  But->plt[i].actDuration = request.actEndtime - request.actStarttime;
  nbytes = request.actLen;
  But->plt[i].samp_sec = (float) (request.samprate<=0? 100:request.samprate);
  samprate = (float) request.samprate;
    
  /* decode (ascii to float) and decimate the data 
  ************************************************/
  nsamp = (int) (Duration*samprate);
  if(nsamp > MAXTRACELTH) nsamp = MAXTRACELTH;
  strtok(But->TraceBuf, " ,");
  j = 0;
  token = strtok(0L, " ,");
    
  nsamp = 0;
  while(token!=0L && j<MAXTRACELTH) 
  {
    if(strcmp(token, "919191")==0) 
    {
      Data[j++] = 919191;
    } 
    else 
    {
      sscanf( token, "%f", &sample);
      Data[j] = sample;
      j += 1;
      nsamp += 1;
    }
    token = strtok(0L, " ,");
  }
        
  But->plt[i].Npts = j;
  temp = j/samprate + 1.0;
  temp1 = request.reqEndtime - request.reqStarttime;
  if(temp < Duration) 
  {
    if (WSDebug) logit( "e","%s trace %s incomplete: %lf  %f %f\n", whoami, SCNLtxt, Duration, temp, temp1);
    sleep_ew(200);
    retry += 1;
    if(retry < But->RetryCount) goto gettrace;
  }   
  if (io == WS_ERR_NONE ) return 1;

  return 0;
}

/**********************************************************************
 * Decode_Time : Decode time from seconds since 1970                  *
 *                                                                    *
 **********************************************************************/
void Decode_Time( double secs, TStrct *Time)
{
  struct Greg    g;
  long    minute;
  double    sex;

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
  struct Greg    g;
  long    minute;
  double    sex;

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
  xoff  = XLMargin;
  yoff  = YBMargin;   
}


/*************************************************************************
 *   ixq calculates the x pixel location.                                *
 *   a is the distance in inches from the left margin.                   *
 *************************************************************************/
int ixq(double a)
{
  double   val;
  int      i;
    
  val  = (a + xoff);
  if(val > xsize) val = xsize;
  if(val<0.0) val = 0.0;
  i = (int) (val*72);
  return i;
}


/*************************************************************************
 *   iyq calculates the y pixel location.                                *
 *   a is the distance in inches up from the top (bottom) margin.        *
 *************************************************************************/
int iyq(double a)
{
  double   val;
  int      i;
    
  /*    val = (ysize - a - yoff); */ /* time increases up from bottom */
  val = a + YTMargin;   /* times increases down from top */
  if(val > ysize) val = ysize;
  if(val<0.0) val = 0.0;
  i = (int) (val*72);
  return i;
}

/****************************************************************************
 *      config_me() process command file using kom.c functions              *
 *                       exits if any errors are encountered                *
 ****************************************************************************/

void config_me( char* configfile, Butler *But )
{
  char whoami[NAMELEN];
  char *com, *str;
  char init[20];       /* init flags, one byte for each required command */
  int  ncommand;       /* # of required commands you expect              */
  int  nmiss;          /* number of required commands that were missed   */
  int  nPlots, nfiles, success, i, j;
  FILE *in;
  gdImagePtr    im_in;

  sprintf(whoami, " %s: %s: ", But->mod, "config_me");
  /* Set to zero one init flag for each required command
  *****************************************************/
  ncommand = 4;
  for(i=0; i<ncommand; i++ )  init[i] = 0;
  But->Debug = But->WSDebug = But->logo = But->logox = But->logoy = 0;
  BStruct.UpdateInt = 120;
  BStruct.nPlots   = 0;
  But->Clip = 0;
  But->Days2Save = 7;
  But->SaveDrifts = 0;
  But->Make_HTML = 0;
  But->BuildOnRestart = 0;
  But->RetryCount = 2;
  But->NoMenuCount = 0;
  But->NoGIFCount  = 0;
  But->IndexFile[0] = 0;
  But->maxSampRate = MAXSAMPRATE_DEFAULT;
  
  /* Open the main configuration file
  **********************************/
  nfiles = k_open( configfile );
  if(nfiles == 0) {
    fprintf( stderr, "%s Error opening command file <%s>; exiting!\n", whoami, configfile );
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

      /* Process anything else as a command
      ************************************/
      /*0*/
      else if( k_its("wsTimeout") ) { /* timeout interval in seconds */
        But->wsTimeout = k_int()*1000;
        init[0] = 1;
      }
            
      /* wave server addresses and port numbers to get trace snippets from
      *******************************************************************/
      /*1*/          
      else if( k_its("WaveServer") ) {
        if ( But->nServer >= MAX_WAVESERVERS ) {
          fprintf( stderr, "%s Too many <WaveServer> commands in <%s>", 
                   whoami, configfile );
          fprintf( stderr, "; max=%d; exiting!\n", (int) MAX_WAVESERVERS );
          return;
        }
        if( (long)(str=k_str()) != 0 )
        {
          if (strlen(str) > MAX_ADRLEN - 1)
          {
            fprintf(stderr, "server address length too long, max is %d\n",
                    MAX_ADRLEN - 1);
            exit( 1 );
          }
          strcpy(But->wsIp[But->nServer],str);
        }
        
        if( (long)(str=k_str()) != 0 )
        {
          if (strlen(str) > MAX_ADRLEN)
          {
            fprintf(stderr, "server port number too long, max is %d\n",
                    MAX_ADRLEN - 1);
            exit( 1 );
          }
          strcpy(But->wsPort[But->nServer],str);
        }

        str=k_str();
        if( (long)(str) != 0 && str[0]!='#') 
        {
          if (strlen(str) > NAMELEN - 1)
          {
            fprintf(stderr, "server comment too long, max is %d\n",
                    NAMELEN - 1);
            exit( 1 );
          }
          strcpy(But->wsComment[But->nServer],str);
        }
        
        But->nServer++;
        init[1]=1;
      }


      /* get Gif directory path/name
      *****************************/
      /*2*/
      else if( k_its("GifDir") ) {
        str = k_str();
        if( (int)strlen(str) >= GDIRSZ - 1) {
          fprintf( stderr, "%s Fatal error. Gif directory name %s greater than %d char.\n",
                   whoami, str, GDIRSZ - 1);
          return;
        }
        if(str) strcpy( But->GifDir , str );
        if (str[strlen(str) - 1] != '/') 
          strcat(But->GifDir, "/");
        init[2] = 1;
      }


      /* get plot parameters
      ************************************/
      /*3*/
      else if( k_its("Plot") ) {
        if ( But->nPlots >= MAXPLOTS ) {
          fprintf( stderr, "%s Too many <Plot> commands in <%s>", 
                   whoami, configfile );
          fprintf( stderr, "; max=%d; exiting!\n", (int) MAXPLOTS );
          return;
        }
        nPlots = But->nPlots;
        But->plt[nPlots].DCcorr   =  But->plt[nPlots].Mean = 0.0;
        But->plt[nPlots].first_gulp = 1;
                
        /*>01<*/
        str = k_str();
        if(str) strcpy( But->plt[nPlots].Site , str );
        /*>02<*/
        str = k_str();
        if(str) strcpy( But->plt[nPlots].Comp , str );
        /*>03<*/
        str = k_str();
        if(str) strcpy( But->plt[nPlots].Net , str );
        /*>03a - location added in<*/
        str = k_str();
        if(str) strcpy( But->plt[nPlots].Loc, str );
        sprintf(But->plt[nPlots].SCNLnam, "%s_%s_%s_%s", But->plt[nPlots].Site, 
                But->plt[nPlots].Comp, But->plt[nPlots].Net, But->plt[nPlots].Loc);
        sprintf(But->plt[nPlots].SCNLtxt, "%s %s %s %s", But->plt[nPlots].Site, 
                But->plt[nPlots].Comp, But->plt[nPlots].Net, But->plt[nPlots].Loc);
        sprintf(But->plt[nPlots].SCNL, "%s%s%s%s", But->plt[nPlots].Site, 
                But->plt[nPlots].Comp, But->plt[nPlots].Net, But->plt[nPlots].Loc);
        /*>04<*/
        But->plt[nPlots].HoursPerPlot = k_int();   /*  # of hours per gif image */
        if(But->plt[nPlots].HoursPerPlot <  1) 
          But->plt[nPlots].HoursPerPlot =  1;
        if(But->plt[nPlots].HoursPerPlot > 24)
          But->plt[nPlots].HoursPerPlot = 24;
        But->plt[nPlots].HoursPerPlot  = 24/(24/But->plt[nPlots].HoursPerPlot);
        /*>05<*/
        But->plt[nPlots].OldData = k_int();  /* Number of previous hours to retrieve */
        if(But->plt[nPlots].OldData < 0 || But->plt[nPlots].OldData > 168)
          But->plt[nPlots].OldData = 0;

        /*>06<*/
        But->plt[nPlots].LocalTime = k_int();    /* Hour offset of local time from UTC */
        if(But->plt[nPlots].LocalTime < -24 || But->plt[nPlots].LocalTime > 24)
          But->plt[nPlots].LocalTime = 0;
        But->plt[nPlots].LocalSecs = (int) (But->plt[nPlots].LocalTime*3600.0);

        /*>07<*/
        str = k_str();    /* Local Time ID e.g. PST */
        strncpy(But->plt[nPlots].LocalTimeID, str, (size_t)3);
        But->plt[nPlots].LocalTimeID[3] = '\0';

        /*>08<*/
        But->plt[nPlots].ShowUTC  = k_int();    /* Flag to show UTC time */

        /*>09<*/
        But->plt[nPlots].UseLocal = k_int();    /* Flag to reference plot to local midnight */

        /*>10<*/
        But->plt[nPlots].xsize = k_val();
        if(But->plt[nPlots].xsize >= 100.0) {            /* x-size of data plot in pixels */
          But->plt[nPlots].xpix = (int) But->plt[nPlots].xsize;
        } else {
          But->plt[nPlots].xpix = (int) (72.0*But->plt[nPlots].xsize);
        }
        But->plt[nPlots].axexmax  =  But->plt[nPlots].xpix/72.0;
        But->plt[nPlots].xsize = But->plt[nPlots].axexmax + XLMargin + XRMargin;  /* Overall size of plot in pixels */
        But->plt[nPlots].xgpix = (int) (72.0*But->plt[nPlots].xsize + 8);

        /*>11<*/
        But->plt[nPlots].ysize = k_val();
        if(But->plt[nPlots].ysize >= 100.0) {            /* y-size of data plot in pixels */
          But->plt[nPlots].ypix = (int) But->plt[nPlots].ysize;
        } else {
          But->plt[nPlots].ypix = (int) (72.0*But->plt[nPlots].ysize);
        }
        But->plt[nPlots].axeymax  =  But->plt[nPlots].ypix/72.0;
        But->plt[nPlots].ysize = But->plt[nPlots].axeymax + YBMargin + YTMargin;  /* Overall size of plot in pixels */
        But->plt[nPlots].ygpix = (int) (72.0*But->plt[nPlots].ysize + 8);

        /*>12<*/
        But->plt[nPlots].mins = k_int();            /* # of minutes/line to display */
        if(But->plt[nPlots].mins < 1 || But->plt[nPlots].mins > 60)
          But->plt[nPlots].mins = 15;

        But->plt[nPlots].mins  = 60/(60/But->plt[nPlots].mins);
        But->plt[nPlots].LinesPerHour = 60/But->plt[nPlots].mins;
        But->plt[nPlots].secsPerGulp = But->plt[nPlots].secsPerStep 
          = 60*(60/(60/MAXMINUTES));
        But->plt[nPlots].PixPerLine = But->plt[nPlots].ypix
          /((60*But->plt[nPlots].HoursPerPlot)/But->plt[nPlots].mins);

        /*>13<*/
        But->plt[nPlots].Gain = k_val();     /* Gain factor */

        /*>14<*/
        But->plt[nPlots].Scale = k_val()*0.01;    /* Scale factor */

        /*>15<*/
        But->plt[nPlots].DCremoved = k_int();

        /*>16<*/
        str = k_str();
        if(str) strcpy( But->plt[nPlots].Comment , str );

        But->nPlots++;
        init[3] = 1;
      }
    
      /* optional commands 
          ********************/

          /*Earthworm stuff*/
      else if( k_its("LogSwitch") ) {
        LogSwitch = k_int();
        ImAModule=1; /* we're to run as a module */
      }
      else if( k_its("MyModuleId") ) {
        str = k_str();
        if(str) strcpy( MyModuleId , str );
          ImAModule=1; /* we're to run as a module */
      }
      else if( k_its("RingName") ) {
        str = k_str();
        if(str) strcpy( InRingName, str );
          ImAModule=1; /* we're to run as a module */
      }
      else if( k_its("HeartBeatInt") ) {
        HeartBeatInterval = k_long();
        ImAModule=1; /* we're to run as a module */
      }

      else if( k_its("Debug") ) But->Debug = 1;   /* optional commands */

      else if( k_its("WSDebug") ) But->WSDebug = 1;   /* optional commands */

      else if( k_its("RetryCount") ) {  /*optional command*/
        But->RetryCount = k_int();
        if(But->RetryCount > 20) But->RetryCount = 20;
        if(But->RetryCount <  1) But->RetryCount =  1;
      }

      else if( k_its("Days2Save") ) {  /*optional command*/
        But->Days2Save = k_int();
        if(But->Days2Save>14) But->Days2Save = 14;
        if(But->Days2Save< 1) But->Days2Save =  1;
      }

      else if( k_its("SaveDrifts") ) 
        But->SaveDrifts = 1;   /* optional commands */

      else if( k_its("Make_HTML") ) 
        But->Make_HTML = 1;   /* optional commands */

      else if( k_its("IndexFile") ) {  /* optional commands */
        str = k_str();
        if (str)
        {
          if (strlen(str) > NAMELEN - 1)
          {
            fprintf(stderr, "IndexFile name too long, max is %d\n", 
                    NAMELEN - 1);
            exit( 1 );
          }
          strcpy(But->IndexFile, str);
        }
      }
      
      else if( k_its("BuildOnRestart") ) 
        But->BuildOnRestart = 1;   /* optional commands */

      else if( k_its("Logo") ) {  /*optional command*/
        str = k_str();
        if(str) {
          if (strlen(str) > NAMELEN - 1)
          {
            fprintf(stderr, "Logo name too long, max is %d\n", NAMELEN - 1);
            exit( 1 );
          }
          strcpy( But->logoname, str );
          But->logo = 1;
        }
      }

      else if( k_its("UpdateInt") ) {  /*optional command*/
        But->UpdateInt = k_int()*60;
        if(But->UpdateInt>10000) But->UpdateInt = 10000; 
        if(But->UpdateInt<  1) But->UpdateInt =   120; 
      }
            
      else if( k_its("Clip") ) {   /* optional command */
        But->Clip = k_int();
      }
      
      else if( k_its("MaxSampRate") ) {   /* optional command */
        But->maxSampRate = k_int();
      }
            
      /* At this point we give up. Unknown thing.
      *******************************************/
      else {
        fprintf(stderr, "%s <%s> Unknown command in <%s>.\n",
                whoami, com, configfile );
        continue;
      }

      /* See if there were any errors processing the command
      *****************************************************/
      if( k_err() ) {
        fprintf( stderr, "%s Bad <%s> command  in <%s>; exiting!\n",
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
    fprintf( stderr, "%s ERROR, no ", whoami);
    if ( !init[0] )  fprintf( stderr, "<wsTimeout> "    );
    if ( !init[1] )  fprintf( stderr, "<WaveServer> "   );
    if ( !init[2] )  fprintf( stderr, "<GifDir> "       );
    if ( !init[3] )  fprintf( stderr, "<Plot> "         );
    fprintf( stderr, "command(s) in <%s>; exiting!\n", configfile );
    exit( -1 );
  }
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
      for(i=0;i<But->nPlots;i++) {
        But->plt[i].ysize = But->plt[i].axeymax + YBMargin + YTMargin;  /* Overall size of plot in pixels */
        But->plt[i].ygpix = (int) (72.0*But->plt[i].ysize + 8);
      }
      gdImageDestroy(im_in);
    } else {
      fprintf(stderr, "Error opening Logo file <%s>\n", But->logoname);
      exit( 1 );
    }
  }
  j = (MAXMINUTES*60 < But->UpdateInt)? MAXMINUTES*60:But->UpdateInt;
  for(i=0;i<But->nPlots;i++) {
    But->plt[i].secsPerGulp = But->plt[i].secsPerStep = j;
  }
}

/****************************************************************************
 *  lookup_ew( ) Look up important info from earthworm.h tables           *
 ****************************************************************************/
void lookup_ew( )
{
    char    subname[] = "lookup_ew";
    
        /* Look up keys to shared memory regions
         ***************************************/
    if( (InRingKey = GetKey(InRingName)) == -1 ) {
        fprintf( stderr,
                "%s: %s: Invalid ring name <%s>; exiting!\n",  Module, subname, InRingName );
        exit( -1 );
    }

        /* Look up installation Id
         *************************/
    if( GetLocalInst( &InstId ) != 0 ) {
        fprintf( stderr,
                "%s: %s: error getting local installation id; exiting!\n",  Module, subname );
        exit( -1 );
    }

        /* Look up modules of interest
         *****************************/
    if( GetModId( MyModuleId, &MyModId ) != 0 ) {
        fprintf( stderr,
                "%s: %s: Invalid Module name <%s>; exiting!\n",  Module, subname, MyModuleId );
        exit( -1 );
    }

        /* Look up message types of interest
         ***********************************/
    if( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
        fprintf( stderr,
                "%s: %s: Invalid message type <TYPE_HEARTBEAT>; exiting!\n",  Module, subname );
        exit( -1 );
    }
    if( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
        fprintf( stderr,
                "%s: %s: Invalid message type <TYPE_ERROR>; exiting!\n",  Module, subname );
        exit( -1 );
    }
}


/**********************************************************************************
 * ewmod_status() builds a heartbeat or error msg & puts it into shared memory     *
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
        logit( "t", "%s: %s:  %s\n",  Module, subname, note );
    }

    size = strlen( msg );   /* don't include the null byte in the message */
    if( tport_putmsg( &InRegion, &logo, size, msg ) != PUT_OK ) {
        if( type == TypeHeartBeat ) {
            logit("et","%s: %s:  Error sending heartbeat.\n",  Module, subname );
        } else
        if( type == TypeError ) {
            logit("et","%s: %s:  Error sending error:%d.\n",  Module, subname, ierr );
        }
    }
}

/****************************************************************
 *  DoModuleStuff() beat the heart, check for shutdown request  *
 ****************************************************************/
void DoModuleStuff()
{
  time_t now;
  if(!ImAModule) return;
  if ( tport_getflag( &InRegion ) == TERMINATE ||
       tport_getflag( &InRegion ) == MyPid ) {      /* detach from shared memory regions*/
    sleep_ew( 500 );       /* wait around */
    tport_detach( &InRegion );
    logit("et", "%s Termination requested; exiting.\n", Module);
    fflush(stdout);
    exit( 0 );
  }
  time(&now);
  if ( now - TimeLastBeat  >=  HeartBeatInterval ) {
      TimeLastBeat = now;
      ewmod_status( TypeHeartBeat, 0, "" );
  }
  return;
}
