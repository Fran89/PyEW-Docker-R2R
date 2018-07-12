
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: evanstrig.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.7  2007/03/28 18:31:06  paulf
 *     removed malloc.h since it is now in platform.h and not used on some platforms
 *
 *     Revision 1.6  2007/02/26 13:44:40  paulf
 *     fixed heartbeat sprintf() to cast time_t as long
 *
 *     Revision 1.5  2004/05/21 22:32:23  dietz
 *     added location code; inputs TYPE_TRACEBUF2, outputs TYPE_LPTRIG_SCNL
 *
 *     Revision 1.4  2002/05/16 16:11:44  patton
 *     Made logit changes.
 *
 *     Revision 1.3  2001/05/09 20:07:45  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPid.
 *
 *     Revision 1.2  2000/07/09 18:39:14  lombard
 *     Fixed an sprintf() missing several arguments; removed unused variable `first'
 *
 *     Revision 1.1  2000/02/14 17:17:36  lucky
 *     Initial revision
 *
 *
 */

      /*****************************************************************
       *                         evanstrig.c                           *
       *                                                               *
       *           John Evans' long-period trigger program,            *
       *             encapsulated in an Earthworm module.              *
       *                                                               *
       *  This program is designed to work on PC systems only.         *
       *  On other systems, there may be serious round-off errors in   *
       *  the function to_clock() in file mteltrg.c                    *
       *                                                               *
       *  Evans' code was originally encapsulated in the Earthworm     *
       *  module, lptrig, by Will Kohler in 1996.  lptrig works on     *
       *  multiplexed ad_buf messages created by the original DOS      *
       *  Earthworm digitizer.                                         *
       *                                                               *
       *  This program (evanstrig) uses demultiplexed waveform data    *
       *  (trace_buf messages) that are created by the NT Earthworm    *
       *  digitizer. This program can be used with analog or digital   *
       *  data sources.  May, 1998 LDD                                 *
       *                                                               *
       *  Evanstrig base code which interacts with Earthworm and feeds *
       *  the long-period trigger functions is based on pick_ew's base *
       *  code which was written by Will Kohler, January, 1997         *
       *  Modified to use SCNs instead of pin numbers. 3/20/98 WMK     *
       *                                                               *
       *****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <math.h>
#include <earthworm.h>
#include <kom.h>
#include <swap.h>
#include <transport.h>
#include "mconst.h"
#include "mteltrg.h"
#include "mutils.h"
#include "evanstrig.h"

/* Error Codes Defined
 *********************/
#define  ERR_MISSMSG    0   /* message missed in transport ring      */
#define  ERR_TOOBIG     1   /* retreived msg too large for buffer    */
#define  ERR_NOTRACK    2   /* transport tracking limit exceeded     */
#define  ERR_RESTART    3   /* data gap caused re-initialization     */
#define  ERR_SAMPRATE   4   /* invalid sampling rate; cannot process */

           /*********************************************
             *          Public Global Variables          *
             *********************************************/

/* These parameters are for Earthworm purposes */
SHM_INFO      InRegion;         /* Info structure for input memory region   */
SHM_INFO      OutRegion;        /* Info structure for output memory region  */
unsigned char MyInstId;         /* Local installation                       */
unsigned char MyModuleId;       /* Our module id                            */
unsigned char GetThisInstId;    /* Get messages from this installation id   */
unsigned char GetThisModuleId;  /* Get messages only from this guy          */
pid_t         MyPid;            /* My process id, sent with heartbeat for
                                      restart purposes                      */
int           Log_triggers;     /* If 1, log to screen only
                                      2, log to log file only
                                      3, log to screen and log file         */
int           Debug_enabled;    /* Set to 1 for extra printing              */

/* These parameters are straight from John Evan's Tdetect code */
double        Drate;            /* Required input sample rate               */
double        Decim;            /* Decimation factor (whole number)         */
double        Ctlta;            /* seconds (1/e time)                       */
double        Ctsta;            /* seconds (1/e time)                       */
double        Ctstav;           /* seconds (1/e time)                       */
double        Tth1;             /* db ((tsta-tstav)/(tlta-tminen))          */
double        Tth2;             /* db ((tsta-tstav)/(tlta-tminen))          */
long          Tminen;           /* digitizer counts                         */
double        Mndur1;           /* seconds                                  */
double        Mndur2;           /* seconds                                  */
double        Cmlta;            /* seconds (1/e time)                       */
double        Cmsta;            /* seconds (1/e time)                       */
double        Cmstav;           /* seconds (1/e time)                       */
double        Mth;              /* db ((msta-mstav)/(mlta-mminen))          */
long          Mminen;           /* digitizer counts                         */
double        Aset;             /* Microseism caution window - sec          */
double        Bset;             /* Pre-event quiet - sec                    */
double        Cset;             /* Post-event + pre-event quiet - sec       */
double        Eset;             /* Microseism caution start delay - sec     */
double        Sset;             /* Settling period - sec                    */
double        DCcoef;           /* 1/e time for DC (hipass) filter - sec    */
int           Dflt_len;         /* Decimation filter length                 */
long          *Dflt;            /* Decimation filter coefficients           */
int           Lflt_len;
long          *Lflt;
double        Quiet;            /* Used to avoid floating-pt underflow      */

            /*********************************************
             *           Static Global Variables         *
             *********************************************/
static int            LogSwitch;    /* If 0, no logging will be done to disk */
static long           InKey;        /* Key to ring where waveforms live      */
static long           OutKey;       /* Key to ring where triggers will live  */
static int            HeartbeatInt; /* Heartbeat interval in seconds         */
static unsigned char  TypeTraceBuf2;
static unsigned char  TypeLptrigSCNL;
static unsigned char  TypeHeartBeat;
static unsigned char  TypeError;
static STATION       *StaArray = NULL;   /* List of stations to process      */
static int            Nsta    = 0;       /* actual # of stations to process  */
static int            MaxSCNL = MAX_SCNL; /* default max # SCNL's to process */
static int            InitSta = 1;       /* Initialization flag for StaArray */
static char          *trbuf;             /* Pointer to trace data buffer     */

/* the following config parameter is from pick_ew's chassis */
static int    MaxGap;     /* Maximum gap (in # of sample intervals) to    */
                          /*   interpolate thru without a restart.  The   */
                          /*   number of samples to "create" is MaxGap-1  */


            /*********************************************
             *            Function declarations          *
             *********************************************/
int  evanstrig_dotrig( STATION *, char *, int );
int  evanstrig_reporttrig( STATION *, double );
void evanstrig_freemem( void );
void evanstrig_config( char * );
void evanstrig_lookup( void );
void evanstrig_status( unsigned char, short, char * );
int  CompareSCNLs( const void *, const void * );
void Interpolate( char *, int, int32_t  );

      /***********************************************************
       *              The main program starts here               *
       *                                                         *
       * Argument: configfile = Name of the configuration file   *
       ***********************************************************/

int main( int argc, char **argv )
{
   TRACE2_HEADER *trhd;            /* Pointer to trace data header */
   int32_t      *trlong;           /* Long pointer to trace data   */
   short        *trshort;          /* Short pointer to trace data  */
   MSG_LOGO      getlogo;          /* Requested logo(s)            */
   MSG_LOGO      logo;             /* Type, module, instid of retrieved msg */
   int           buflength;        /* length of buffer for a tracbuf msg */
   int           restart;          /* 1 if there is a break in time series */
   time_t        timelastbeat = 0; /* time last heartbeat was sent */
   time_t        now;              /* current time */
   int           rc;               /* Return code  */
   long          length;
   char          errmsg[80];
   int           i;

/* Check command line arguments
   ****************************/
   if ( argc != 2 )
   {
      printf( "Usage: evanstrig <configfile>\n" );
      return -1;
   }
/* Initialize name of log-file & open it
   *************************************/
   logit_init( argv[1], 0, 512, 1 );

/* Read configuration parameters
   *****************************/
   evanstrig_config( argv[1] );

/* Look up stuff in earthworm.h tables
   ***********************************/
   evanstrig_lookup();

/* Reinitialize logit to desired logging level
   *******************************************/
   logit_init( argv[1], MyModuleId, 512, LogSwitch );

/* Get our own process id for restart purposes
   *******************************************/
   MyPid = getpid();
   if( MyPid == -1 )
   {
      logit("e","evanstrig%d: Cannot get pid; exiting!\n", MyModuleId );
      evanstrig_freemem();
      return -1;
   }

/* Allocate the input message buffer & initialize pointers.
   We'll make the buffer big enough to hold the maximum sized
   tracebuf message, input as 2-byte data, converted to 4-byte,
   with space for prepending any interpolated samples to the msg
   *************************************************************/
   buflength = MAX_TRACEBUF_SIZ*2 + sizeof(int32_t)*(MaxGap-1);
   trbuf = (char *) malloc( (size_t)buflength );
   if ( trbuf == (char *)NULL )
   {
      logit( "et", "evanstrig%d: Error allocating input buffer; Exiting!\n",
             MyModuleId );
      evanstrig_freemem();
      return -1;
   }
   trhd    = (TRACE2_HEADER *)trbuf;
   trlong  = (int32_t *)  (trbuf + sizeof(TRACE2_HEADER));
   trshort = (short *) (trbuf + sizeof(TRACE2_HEADER));

/* Log configuration file parameters
   *********************************/
   logit( "", "\nConfiguration file parameters:\n" );
   logit( "", "   Local installation id:            %u\n",    MyInstId );
   logit( "", "   Module id of this program:        %u\n",    MyModuleId );
   logit( "", "   Receive waveforms from inst id:   %u\n",    GetThisInstId );
   logit( "", "   Receive waveforms from module id: %u\n",  GetThisModuleId );
   logit( "", "   Maximum # channels to trigger on: %d\n",    MaxSCNL );
   logit( "", "   Maximum Gap to interpolate thru:  %d\n",    MaxGap );
   logit( "", "   Heartbeat interval (seconds):     %d\n",    HeartbeatInt );
   logit( "", "   Log_triggers:                     %d\n",    Log_triggers );
   logit( "", "   Debug_enabled:                    %d\n",    Debug_enabled );
   logit( "", "   Required sampling rate (Drate):   %.2lf\n", Drate );
   logit( "", "   Decimation factor (Decim):        %.0lf\n", Decim );
   logit( "", "   Ctlta:                            %.2lf\n", Ctlta );
   logit( "", "   Ctsta:                            %.2lf\n", Ctsta );
   logit( "", "   Ctstav:                           %.2lf\n", Ctstav );
   logit( "", "   Tth1:                             %.2lf\n", Tth1 );
   logit( "", "   Tth2:                             %.2lf\n", Tth2 );
   logit( "", "   Tminen:                           %ld\n",   Tminen );
   logit( "", "   Mndur1:                           %.2lf\n", Mndur1 );
   logit( "", "   Mndur2:                           %.2lf\n", Mndur2 );
   logit( "", "   Cmlta:                            %.2lf\n", Cmlta );
   logit( "", "   Cmsta:                            %.2lf\n", Cmsta );
   logit( "", "   CMstav:                           %.2lf\n", Cmstav );
   logit( "", "   Mth:                              %.2lf\n", Mth );
   logit( "", "   Mminen:                           %ld\n",   Mminen );
   logit( "", "   Aset:                             %.2lf\n", Aset );
   logit( "", "   Bset:                             %.2lf\n", Bset );
   logit( "", "   Cset:                             %.2lf\n", Cset );
   logit( "", "   Eset:                             %.2lf\n", Eset );
   logit( "", "   Sset:                             %.2lf\n", Sset );
   logit( "", "   DCcoef:                           %.2lf\n", DCcoef );
   logit( "", "   Dflt_len:                         %d\n",    Dflt_len );
   logit( "", "   Lflt_len:                         %d\n",    Lflt_len );
   logit( "", "   Quiet:                            %.2e\n",  Quiet );
   logit( "", "\n" );

/* Sort the station list by SCNL & log the list
   *******************************************/
   qsort( StaArray, Nsta, sizeof(STATION), CompareSCNLs );

   logit("", "   Triggering on %d stations:\n", Nsta );
   for( i=0; i<Nsta; i++ )
   {
      logit( "", "      %s %s %s %s\n",
             StaArray[i].sta, StaArray[i].chan, StaArray[i].net, StaArray[i].loc);
   }
   logit( "", "\n" );

/* Attach to the transport rings (Must already exist)
   **************************************************/
   if ( OutKey != InKey )
   {
      tport_attach( &InRegion, InKey );
      tport_attach( &OutRegion, OutKey );
   }
   else
   {
      tport_attach( &InRegion, InKey );
      OutRegion = InRegion;
   }

/* Specify logo to get from the transport ring
   *******************************************/
   getlogo.type   = TypeTraceBuf2;
   getlogo.mod    = GetThisModuleId;
   getlogo.instid = GetThisInstId;

/* Flush the input ring
   ********************/
   while( tport_getmsg( &InRegion, &getlogo, 1, &logo, &length,
                        trbuf, (long)MAX_TRACEBUF_SIZ ) != GET_NONE );

/* Initialize trigger parameters
   *****************************/
   tel_initialize_params();

/* Loop to read from the transport ring
   ************************************/
   while ( 1 )
   {
      STATION key;          /* Key for binary search                    */
      STATION *sta;         /* Pointer to the station being processed   */
      double  GapSizeD;     /* # sample intervals between msgs (double) */
      long    GapSize;      /* # sample intervals between msgs (integer)*/

      if ( tport_getflag( &InRegion ) == TERMINATE  ||
           tport_getflag( &InRegion ) == MyPid )
         break;

      if ( tport_getflag( &OutRegion ) == TERMINATE  ||
           tport_getflag( &OutRegion ) == MyPid )
         break;

   /* Get a tracebuf message from transport region
      ********************************************/
      rc = tport_getmsg( &InRegion, &getlogo, 1,
                         &logo, &length, trbuf, (long)MAX_TRACEBUF_SIZ );

      if ( rc == GET_NONE )
      {
         sleep_ew( 100 );
         continue;
      }
      else if ( rc == GET_TOOBIG )
      {
         sprintf( errmsg, "Retrieved msg too big (%ld) for buffer\n",
                  length );
         evanstrig_status( TypeError, ERR_TOOBIG, errmsg );
         continue;
      }
      else if ( rc == GET_NOTRACK )
      {
         sprintf( errmsg, "No sequence tracking for logo i:%d m:%d t:%d\n",
                  (int)logo.instid, (int)logo.mod, (int)logo.type);
         evanstrig_status( TypeError, ERR_NOTRACK, errmsg );
      }
      else if ( rc == GET_MISS )
      {
         sprintf( errmsg, "Missed msg(s) of logo i:%d m:%d t:%d\n",
                (int)logo.instid, (int)logo.mod, (int)logo.type );
         evanstrig_status( TypeError, ERR_MISSMSG, errmsg );
      }

   /* Try to find SCNL in the station list
      ***********************************/
      strncpy( key.sta, trhd->sta, TRACE2_STA_LEN );
      key.sta[TRACE2_STA_LEN-1]= '\0';

      strncpy( key.chan, trhd->chan, TRACE2_CHAN_LEN );
      key.chan[TRACE2_CHAN_LEN-1] = '\0';

      strncpy( key.net, trhd->net, TRACE2_NET_LEN );
      key.net[TRACE2_NET_LEN-1] = '\0';

      strncpy( key.loc, trhd->loc, TRACE2_LOC_LEN );
      key.loc[TRACE2_LOC_LEN-1] = '\0';

      sta = (STATION *) bsearch( &key, StaArray, Nsta, sizeof(STATION),
                                 CompareSCNLs );

      if ( sta == NULL  ) continue; /* SCNL not in "to-be-triggered" list */
      if ( !sta->okrate ) continue; /* this SCNL has been black-listed    */

   /* If necessary, swap bytes in the message
      ***************************************/
      if ( WaveMsg2MakeLocal( trhd ) < 0 )
      {
         logit( "et", "evanstrig: Unknown datatype in tracebuf2 msg.\n" );
         continue;
      }

   /* Initialize stuff the first time we get a message with this SCNL
      ***************************************************************/
      restart = 0;

      if ( sta->first == 1 )
      {
         restart      = 1;
         sta->endtime = trhd->starttime - (1.0/trhd->samprate);
         sta->pin     = trhd->pinno;
         sta->first   = 0;

      /* Make sure this trace has the required sample rate */
         if( trhd->samprate != Drate )
         {
            sta->okrate = 0;
            sprintf(errmsg,"Cannot trigger on %s %s %s %s; "
                           "samprate=%.1lf required=%.1lf\n",
                    trhd->sta, trhd->chan, trhd->net, trhd->loc, 
                    trhd->samprate, Drate );
            evanstrig_status( TypeError, ERR_SAMPRATE, errmsg );
            continue;
         }
      }

   /* If the samples are shorts, make them longs.
      We allocated enough space to do this for
      the largest possible tracebuf message!
      ******************************************/
      if ( trhd->datatype[1] == '2' )
      {
         int j;
         for ( j = trhd->nsamp - 1; j > -1; j-- )
            trlong[j] = (int32_t) trshort[j];
         trhd->datatype[1] = '4';
      }

   /* Compute # sample intervals since the end of the previous message.
      If (GapSize == 1), no data has been lost between messages.
      If (1 < GapSize <= MaxGap), data will be interpolated.
      If (GapSize > MaxGap), the evanstrig will go into restart mode.
      *******************************************************************/
      GapSizeD = trhd->samprate * (trhd->starttime - sta->endtime);

      if ( GapSizeD < 0. )    /* Invalid. Time going backwards. */
      {
         GapSize = 0;
         restart = 1;
      }
      else
         GapSize = (long) (GapSizeD + 0.5);

   /* Interpolate missing samples and prepend them to the current message.
      We allocated enough space to do this for MaxGap-1 data points!
      *******************************************************************/
      if ( (GapSize > 1) && (GapSize <= MaxGap) )
      {
         Interpolate( trbuf, GapSize, sta->enddata );
         if(Debug_enabled)
             logit("t","evanstrig: Interpolated %d samples for %s %s %s %s\n",
                    GapSize, sta->sta, sta->chan, sta->net, sta->loc );
      }

   /* Announce large sample gaps
      **************************/
      else if ( GapSize > MaxGap )
      {
         sprintf( errmsg,
                 "Found %4ld sample gap; trigger restart on %s %s %s %s\n",
                  GapSize, sta->sta, sta->chan, sta->net, sta->loc );
         evanstrig_status( TypeError, ERR_RESTART, errmsg );
         restart = 1;
      }

   /* Invoke the trigger
      ******************/
      if ( evanstrig_dotrig( sta, trbuf, restart ) < 0 )
      {
         logit( "et", "evanstrig%d: DoTrig failed; Exiting!", MyModuleId );
         return -1;
      }

   /* Save time and amplitude of the end of the current message
      *********************************************************/
      sta->enddata = trlong[trhd->nsamp - 1];
      sta->endtime = trhd->endtime;

   /* Send the heartbeat to the transport ring
      ****************************************/
      if( HeartbeatInt!=0  &&  (time(&now)-timelastbeat)>=HeartbeatInt )
      {
         evanstrig_status( TypeHeartBeat, 0, "" );
         timelastbeat = now;
      }
   } /*end main while loop*/

/* Detach from the ring buffers
   ****************************/
   if ( OutKey != InKey )
   {
      tport_detach( &InRegion );
      tport_detach( &OutRegion );
   }
   else
      tport_detach( &InRegion );

/* Free any allocated memory
   *************************/
   evanstrig_freemem();

   logit( "t", "Termination requested; Exiting.\n" );
   return 0;
}

   /******************************************************************
    *                     evanstrig_dotrig()                         *
    *                                                                *
    *                  Process one tracedata buffer                  *
    *                                                                *
    * Arguments:                                                     *
    *    Sta                Pointer to station being processed       *
    *    msg                Pointer to trace data buffer             *
    *    restart            1 if program is starting or data gap     *
    ******************************************************************/

int evanstrig_dotrig( STATION *sta, char *msg, int restart )
{
   long          index;
   TRACE_HEADER *thd     = (TRACE_HEADER *) msg;
   int32_t      *data    = (int32_t *) &msg[sizeof(TRACE_HEADER)];
   int           nsample = thd->nsamp;

/* On restart, initialize trigger variables for this SCNL
   *****************************************************/
   if ( restart )
   {
      sta->qch.pin = sta->pin;
      tel_initialize_channel( &sta->qch );
    /*return 0;*/ /* lptrig's muxtrig function returned here. If we do
                     this, we won't process any data from this buffer.
                     I'm going to try to continue on... LDD */
   }

/* Analyze this SCNL; sta->qch contains its current parameter values
   ****************************************************************/
   index = tel_triggered_on_tracebuf( &sta->qch, data, nsample );
   if ( index != -1L )
   {
      int rc;
      double trigger_time;

      trigger_time = thd->starttime + (double)index / Drate;
      rc = evanstrig_reporttrig( sta, trigger_time );
      if ( rc == -1 )
         logit( "et", "evanstrig%d: Error sending trigger to output ring!\n",
                 (int)MyModuleId );
   }

   return 0;
}


   /*********************************************************************
    *                   evanstrig_reporttrig()                          *
    *      Called by evanstrig_dotrig when a trigger is detected.       *
    *                                                                   *
    *  Arguments:                                                       *
    *     staPtr        Pointer to station structure for this channel   *
    *     trigger_time  Start time of message containing the trigger    *
    *                                                                   *
    *  Returns:                                                         *
    *     0 if all ok                                                   *
    *    -1 if error writing to trigger to output ring                  *
    *********************************************************************/

int evanstrig_reporttrig( STATION *staPtr, double trigger_time )
{
   MSG_LOGO     outlogo;        /* Logo of message to output */
   static char  line[100];
   int          lineLen;

/* Encode trigger into space-delimited character string
   ****************************************************/
   sprintf( line, "%d %d %d %d %s %s %s %s %.3lf %c\n",
           (int) TypeLptrigSCNL, (int) MyModuleId, (int) MyInstId,
           staPtr->pin, staPtr->sta, staPtr->chan, staPtr->net, staPtr->loc,
           trigger_time, staPtr->qch.info.event_type );
   lineLen = (int)strlen(line);

/* Log the trigger
   ***************/
   if ( Log_triggers == 1 )
      printf( "%s", line );

   if ( Log_triggers == 2 )
      logit( "", "%s", line );

   if ( Log_triggers == 3 )
      logit( "e", "%s", line );

/* Send the trigger to the output ring
   ***********************************/
   outlogo.type   = TypeLptrigSCNL;
   outlogo.mod    = MyModuleId;
   outlogo.instid = MyInstId;

   if ( tport_putmsg( &OutRegion, &outlogo, lineLen, line ) != PUT_OK )
      return -1;
   return 0;
}

 /***********************************************************************
  * evanstrig_freemem()  Free all memory that was malloc'd or calloc'd  *
  ***********************************************************************/
void evanstrig_freemem( void )
{
   free( trbuf );
   free( Dflt  );
   free( Lflt  );
   free( StaArray );
   return;
}

 /***********************************************************************
  *                          evanstrig_config()                         *
  *             Processes command file using kom.c functions.           *
  *                  Exits if any errors are encountered.               *
  ***********************************************************************/
#define N_COMMAND 34                /* Number of commands to process */

void evanstrig_config( char *configfile )
{
   const int ncommand = N_COMMAND;  /* Number of commands to process       */
   char     init[N_COMMAND];        /* Init flags, one for each command    */
   int      nmiss;                  /* Number of commands that were missed */
   char    *com;
   char    *str;
   int      nfiles;
   int      success;
   int      i;
   int      nDfltExpected = 0;   /* Number of coef expected in config file */
   int      nDflt = 0;           /* Number of coef found in config file    */
   int      nLfltExpected = 0;
   int      nLflt = 0;

/* Set to zero one init flag for each required command
   ***************************************************/
   for ( i = 0; i < ncommand; i++ ) init[i] = 0;

/* Open the main configuration file
   ********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 )
   {
        logit("e", "evanstrig: Error opening configuration file <%s>; Exiting!\n",
                 configfile );
        exit( -1 );
   }

/* Process all command files
   *************************/
   while ( nfiles > 0 )            /* While there are command files open */
   {
        while ( k_rd() )           /* Read next line from active file  */
        {
            com = k_str();         /* Get the first token from line */

/* Ignore blank lines & comments
   *****************************/
            if( !com )           continue;
            if( com[0] == '#' )  continue;

/* Open a nested configuration file
   ********************************/
            if( com[0] == '@' )
            {
               success = nfiles + 1;
               nfiles  = k_open( &com[1] );
               if ( nfiles != success ) {
                  logit("e", "evanstrig: Error opening configuration file <%s>; Exiting!\n",
                           &com[1] );
                  exit( -1 );
               }
               continue;
            }

/* Process anything else as a command
   **********************************/
            if( k_its( "GetWavesFrom" ) )
            {
               if ( ( str = k_str() ) != NULL ) {
                  if ( GetInst( str, &GetThisInstId ) != 0 )
                  {
                      logit("e", "evanstrig: Invalid GetWavesFrom installation <%s>",
                               str );
                      logit("e", " in <%s>; Exiting!\n", configfile );
                      exit( -1 );
                  }
               }
               if ( ( str = k_str() ) != NULL ) {
                  if ( GetModId( str, &GetThisModuleId ) != 0 )
                  {
                      logit("e", "evanstrig: Invalid GetWavesFrom module <%s>", str );
                      logit("e", " in <%s>; Exiting!\n", configfile );
                      exit( -1 );
                  }
               }
               init[0] = 1;
            }

            else if( k_its( "MyModuleId" ) )
            {
               str = k_str();
               if ( str != NULL ) {
                  if ( GetModId( str, &MyModuleId ) != 0 )
                  {
                     logit("e", "evanstrig: Invalid MyModuleId <%s> in <%s>; Exiting!\n",
                              str , configfile );
                     exit( -1 );
                  }
               }
               init[1] = 1;
            }

            else if ( k_its( "HeartbeatInt" ) )
               HeartbeatInt = k_int(), init[2] = 1;

            else if ( k_its( "TriggerOn" ) )
            {
               if( InitSta ) /* initialize station array */
               {
                  StaArray = (STATION *)calloc((size_t)MaxSCNL, sizeof(STATION));
                  if( StaArray == (STATION *) NULL )
                  {
                     logit("e", "evanstrig: Error allocating station array for "
                             "MaxSCNL=%d; exiting\n", MaxSCNL );
                     exit( -1 );
                  }
                  InitSta = 0;
               }
               if( Nsta >= MaxSCNL )  /* no room for another */
               {
                   logit("e", "evanstrig: Too many <TriggerOn> commands in <%s>,"
                           " limit=%d, increase with <MaxSCNL> command; exiting!\n",
                             configfile, MaxSCNL );
                   exit( -1 );
               }
               str = k_str();   /* read station site code */
               if( str && strlen(str)<TRACE2_STA_LEN ) {
                   strcpy( StaArray[Nsta].sta, str );
               } else {
                   logit("e", "evanstrig: error in <TriggerOn> command in <%s>:\n",
                           configfile );
                   logit("e", "           bad station code (field 1); exiting!\n" );
                   exit( -1 );
               }
               str = k_str();   /* read component code */
               if( str && strlen(str)<TRACE2_CHAN_LEN  ) {
                   strcpy( StaArray[Nsta].chan, str );
               } else {
                   logit("e", "evanstrig: error in <TriggerOn> command in <%s>:\n",
                           configfile );
                   logit("e", "           bad component code (field 2); exiting!\n" );
                   exit( -1 );
               }
               str = k_str();  /* read network code */
               if( str && strlen(str)<TRACE2_NET_LEN ) {
                   strcpy( StaArray[Nsta].net, str );
               } else {
                   logit("e", "evanstrig: error in <TriggerOn> command in <%s>:\n",
                           configfile );
                   logit("e", "           bad network code (field 3); exiting!\n" );
                   exit( -1 );
               }
               str = k_str();  /* read location code */
               if( str && strlen(str)<TRACE2_LOC_LEN ) {
                   strcpy( StaArray[Nsta].loc, str );
               } else {
                   logit("e", "evanstrig: error in <TriggerOn> command in <%s>:\n",
                           configfile );
                   logit("e", "           bad location code (field 4); exiting!\n" );
                   exit( -1 );
               }
               StaArray[Nsta].pin     = 0;    /* initialize other stuff */
               StaArray[Nsta].first   = 1;
               StaArray[Nsta].okrate  = 1;
               StaArray[Nsta].enddata = 0;
               StaArray[Nsta].endtime = 0.0;
               Nsta++;
               init[3] = 1;
            }
            else if ( k_its( "InRing" ) )
            {
               if ( ( str=k_str() ) != NULL ) {
                  if ( (InKey = GetKey(str)) == -1 )
                  {
                     logit("e", "evanstrig: Invalid InRing name <%s>; Exiting!\n",
                              str );
                     exit( -1 );
                  }
               }
               init[4] = 1;
            }

            else if ( k_its( "OutRing" ) )
            {
               if ( ( str=k_str() ) != NULL ) {
                  if ( (OutKey = GetKey(str)) == -1 )
                  {
                     logit("e", "evanstrig: Invalid OutRing name <%s>; Exiting!\n",
                              str );
                     exit( -1 );
                  }
               }
               init[5] = 1;
            }

            else if( k_its("LogFile") )
               LogSwitch = k_int(), init[6] = 1;

            else if ( k_its( "Log_triggers" ) )
               Log_triggers = k_int(), init[7] = 1;

            else if ( k_its( "Debug_enabled" ) )
               Debug_enabled = k_int(), init[8] = 1;

            else if ( k_its( "MaxGap" ) )
               MaxGap = k_int(), init[9] = 1;

            else if ( k_its( "Drate" ) )
            {
               Drate = k_val();
               if( Drate != 100. ) {
                  logit("e", "evanstrig: Only works properly with <Drate 100.0>!"
                          " Exiting!\n" );
                  exit( -1 );
               }
               init[10] = 1;
            }
            else if ( k_its( "Decim" ) )
               Decim = k_val(), init[11] = 1;

            else if ( k_its( "Ctlta" ) )
               Ctlta = k_val(), init[12] = 1;

            else if ( k_its( "Ctsta" ) )
               Ctsta = k_val(), init[13] = 1;

            else if ( k_its( "Ctstav" ) )
               Ctstav = k_val(), init[14] = 1;

            else if ( k_its( "Tth1" ) )
               Tth1 = k_val(), init[15] = 1;

            else if ( k_its( "Tth2" ) )
               Tth2 = k_val(), init[16] = 1;

            else if ( k_its( "Tminen" ) )
               Tminen = k_long(), init[17] = 1;

            else if ( k_its( "Mndur1" ) )
               Mndur1 = k_val(), init[18] = 1;

            else if ( k_its( "Mndur2" ) )
               Mndur2 = k_val(), init[19] = 1;

            else if ( k_its( "Cmlta" ) )
               Cmlta = k_val(), init[20] = 1;

            else if ( k_its( "Cmsta" ) )
               Cmsta = k_val(), init[21] = 1;

            else if ( k_its( "Cmstav" ) )
               Cmstav = k_val(), init[22] = 1;

            else if ( k_its( "Mth" ) )
               Mth = k_val(), init[23] = 1;

            else if ( k_its( "Mminen" ) )
               Mminen = k_long(), init[24] = 1;

            else if ( k_its( "Aset" ) )
               Aset = k_val(), init[25] = 1;

            else if ( k_its( "Bset" ) )
               Bset = k_val(), init[26] = 1;

            else if ( k_its( "Cset" ) )
               Cset = k_val(), init[27] = 1;

            else if ( k_its( "Eset" ) )
               Eset = k_val(), init[28] = 1;

            else if ( k_its( "Sset" ) )
               Sset = k_val(), init[29] = 1;

            else if ( k_its( "DCcoef" ) )
               DCcoef = k_val(), init[30] = 1;

            else if ( k_its( "Quiet" ) )
               Quiet = k_val(), init[31] = 1;

            else if ( k_its( "Dflt_len" ) )
            {
                Dflt_len = k_int();
                nDfltExpected = (Dflt_len + 1) / 2;
                Dflt = (long *) calloc( nDfltExpected, sizeof(long) );
                if ( Dflt == NULL )
                {
                   logit("e", "evanstrig: Error allocating Dflt array. Exiting.\n" );
                   exit( -1 );
                }
                init[32] = 1;
            }

            else if( k_its( "Lflt_len" ) )
            {
                Lflt_len = k_int();
                nLfltExpected = (Lflt_len + 1) / 2;
                Lflt = (long *) calloc( nLfltExpected, sizeof(long) );
                if ( Lflt == NULL )
                {
                   logit("e", "evanstrig: Error allocating Lflt array. Exiting.\n" );
                   exit( -1 );
                }
                init[33] = 1;
            }

            else if ( k_its( "Dflt" ) )
            {
                if ( nDflt == nDfltExpected )
                {
                   logit("e", "evanstrig: Too many Dflt filter coefficients\n" );
                   logit("e", "        specified in the config file. Exiting!\n" );
                   exit( -1 );
                }
                Dflt[nDflt++] = k_long();
            }

            else if ( k_its( "Lflt" ) )
            {
                if ( nLflt == nLfltExpected )
                {
                   logit("e", "evanstrig: Too many Lflt filter coefficients\n" );
                   logit("e", "        specified in the config file. Exiting!\n" );
                   exit( -1 );
                }
                Lflt[nLflt++] = k_long();
            }

            else if ( k_its( "MaxSCNL" ) )
            {
               MaxSCNL = k_int();
               if( InitSta )
               {
                  StaArray = (STATION *)calloc((size_t)MaxSCNL, sizeof(STATION));
                  if( StaArray == (STATION *) NULL )
                  {
                     logit("e", "evanstrig: Error allocating station array for "
                             "MaxSCNL=%d; exiting\n", MaxSCNL );
                     exit( -1 );
                  }
                  InitSta = 0;
               }
               else
               {
                  logit("e", "evanstrig: <MaxSCNL> command must precede all "
                          "<TriggerOn> commands in <%s>; Exiting!\n",
                           configfile );
                  evanstrig_freemem();
                  exit( -1 );
               }
            }


         /* Unknown command
            ***************/
            else {
                logit("e", "evanstrig: <%s> unknown command in <%s>\n",
                        com, configfile );
                continue;
            }

         /* See if there were any errors processing the command
            ***************************************************/
            if( k_err() ) {
               logit("e", "evanstrig: Bad <%s> command in <%s>; Exiting!\n",
                        com, configfile );
               exit( -1 );
            }
        }
        nfiles = k_close();
   }

/* After all files are closed, check init flags for missed commands
   ****************************************************************/
   nmiss = 0;
   for ( i = 0; i < ncommand; i++ )
      if( !init[i] ) nmiss++;
   if ( nmiss )
   {
       logit("e", "evanstrig: ERROR, no " );
       if ( !init[0]  ) logit("e", "<GetWavesFrom> "  );
       if ( !init[1]  ) logit("e", "<MyModuleId> "    );
       if ( !init[2]  ) logit("e", "<HeartbeatInt> "  );
       if ( !init[3]  ) logit("e", "<TriggerOn> "     );
       if ( !init[4]  ) logit("e", "<InRing> "        );
       if ( !init[5]  ) logit("e", "<OutRing> "       );
       if ( !init[6]  ) logit("e", "<LogFile> "       );
       if ( !init[7]  ) logit("e", "<Log_triggers> "  );
       if ( !init[8]  ) logit("e", "<Debug_enabled> " );
       if ( !init[9]  ) logit("e", "<MaxGap> " );
       if ( !init[10] ) logit("e", "<Drate> "  );
       if ( !init[11] ) logit("e", "<Decim> "  );
       if ( !init[12] ) logit("e", "<Ctlta> "  );
       if ( !init[13] ) logit("e", "<Ctsta> "  );
       if ( !init[14] ) logit("e", "<Ctstav> " );
       if ( !init[15] ) logit("e", "<Tth1> "   );
       if ( !init[16] ) logit("e", "<Tth2> "   );
       if ( !init[17] ) logit("e", "<Tminen> " );
       if ( !init[18] ) logit("e", "<Mndur1> " );
       if ( !init[19] ) logit("e", "<Mndur2> " );
       if ( !init[20] ) logit("e", "<Cmlta> "  );
       if ( !init[21] ) logit("e", "<Cmsta> "  );
       if ( !init[22] ) logit("e", "<Cmstav> " );
       if ( !init[23] ) logit("e", "<Mth> "    );
       if ( !init[24] ) logit("e", "<Mminen> " );
       if ( !init[25] ) logit("e", "<Aset> "   );
       if ( !init[26] ) logit("e", "<Bset> "   );
       if ( !init[27] ) logit("e", "<Cset> "   );
       if ( !init[28] ) logit("e", "<Eset> "   );
       if ( !init[29] ) logit("e", "<Sset> "   );
       if ( !init[30] ) logit("e", "<DCcoef> " );
       if ( !init[31] ) logit("e", "<Quiet> "  );
       if ( !init[32] ) logit("e", "<Dflt_len> " );
       if ( !init[33] ) logit("e", "<Lflt_len> " );
       logit("e", "command(s) in <%s>; Exiting!\n", configfile );
       exit( -1 );
   }

   if ( nDflt < nDfltExpected )
   {
       logit("e", "evanstrig: Not enough Dflt filter coefficients\n" );
       logit("e", "           specified in the config file. Exiting!\n" );
       exit( -1 );
   }

   if ( nLflt < nLfltExpected )
   {
       logit("e", "evanstrig: Not enough Lflt filter coefficients\n" );
       logit("e", "           specified in the config file. Exiting!\n" );
       exit( -1 );
   }
   return;
}


   /************************************************************
    *                    evanstrig_lookup()                    *
    *     Look up important info from earthworm.h tables       *
    ************************************************************/

void evanstrig_lookup( void )
{
/* Find local installation id
   **************************/
   if ( GetLocalInst( &MyInstId ) != 0 ) {
      printf( "evanstrig: error getting local installation id; Exiting!\n" );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      printf( "evanstrig: Invalid message type <TYPE_HEARTBEAT>; Exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      printf( "evanstrig: Invalid message type <TYPE_ERROR>; Exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_LPTRIG_SCNL", &TypeLptrigSCNL ) != 0 ) {
      printf( "evanstrig: Invalid message type <TYPE_LPTRIG_SCNL>; Exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_TRACEBUF2", &TypeTraceBuf2 ) != 0 ) {
      printf( "evanstrig: Invalid message type <TYPE_TRACEBUF2>; Exiting!\n" );
      exit( -1 );
   }
   return;
}

/******************************************************************************
 * evanstrig_status()  builds a heartbeat or error message & puts it into     *
 *                     shared memory.  Writes errors to log file & screen.    *
 ******************************************************************************/
void evanstrig_status( unsigned char type, short ierr, char *note )
{
   MSG_LOGO    logo;
   char        msg[256];
   long        size;
   time_t        t;

/* Build the message
 *******************/
   logo.instid = MyInstId;
   logo.mod    = MyModuleId;
   logo.type   = type;

   time( &t );

   if( type == TypeHeartBeat )
   {
        sprintf( msg, "%ld %d\n", (long) t, (int)MyPid );
   }
   else if( type == TypeError )
   {
        sprintf( msg, "%ld %hd %s\n", (long) t, ierr, note);
        logit( "et", "evanstrig%d: %s\n", (int)MyModuleId, note );
   }

   size = (int)strlen( msg );   /* don't include the null byte in the message */

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &OutRegion, &logo, size, msg ) != PUT_OK )
   {
        if( type == TypeHeartBeat ) {
           logit("et","evanstrig%d:  Error sending heartbeat.\n", MyModuleId );
        }
        else if( type == TypeError ) {
           logit("et","evanstrig%d:  Error sending error:%d.\n",
           MyModuleId, ierr );
        }
   }

   return;
}

