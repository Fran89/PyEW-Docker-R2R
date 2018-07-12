/* THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU CHECKED IT OUT!
 *
 *  $Id: coda_dur.c 499 2009-12-23 16:50:25Z dietz $
 * 
 *  Revision history:
 *   $Log$
 *   Revision 1.5  2009/12/23 16:50:25  dietz
 *   Commented out remaining DEBUG statements.
 *
 *   Revision 1.4  2009/12/17 19:07:23  dietz
 *   Modified to look at only newly-arrived CAAVs when scanning for coda
 *   termination value.
 *
 *   Revision 1.3  2009/12/08 18:46:21  dietz
 *   Removed MinCodaLen parameter (was never used).
 *   Added logic to stop coda processing for gappy or dead traces.
 *
 *   Revision 1.2  2009/11/13 17:35:22  dietz
 *   Modified some logging statements, fixed end-of-loop error in "magic aav"
 *   loop when loop hit firstcaav in list.
 *
 *   Revision 1.1  2009/11/09 19:16:15  dietz
 *   Initial version, may still contain bugs and debugging statements
 *
 */

      /*****************************************************************
       *                           coda_dur.c                          *
       *                                                               *
       *  This program reads picks and coda avg absolute values (aav)  * 
       *  and then determines the coda duration for each pick.         * 
       *  The program was decoupled from pick_ew so that picks and     *
       *  coda can potentially be produced from differently-processed  *
       *  waveform streams of a given SCNL.                            *
       *                                                               *
       *  Coda_dur only processes SCNLs listed in the pick_ew station  *
       *  list with pick_flag=1, and it uses these parameters:         *
       *                                                               *
       *  Old name   New name                                          *
       *  --------   --------                                          *
       *     c7      CodaTerm                                          *
       *     c8      AltCoda                                           *
       *     c9      PreEvent                                          *
       *                                                               *
       *****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>
#include <time.h>
#include <time_ew.h>
#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include <rdpickcoda.h>
#include <rw_coda_aav.h>
#include "coda_dur.h"

/* Function prototypes
   *******************/
int  GetConfig( char *, GPARM * );
void LogConfig( GPARM * );
int  cdur_stalist( STATION **, int *, GPARM * );
void LogStaList( STATION *, int );
int  CompareSCNL( const void *, const void * );
int  GetEwh( EWH * );
int  ReportCoda( STATION *, SHM_INFO *, MSG_LOGO * );
void cdur_free( void );
void cdur_processpicks( void );
void cdur_getpicks( void );

/* Define the different states for pick processing
 *************************************************/
#define PK_DONE   0   /* coda duration completed for this pick        */
#define PK_NEW    1   /* new pick, need to check pre-event conditions */
#define PK_ACTIVE 2   /* still active, use normal/quiet termination   */
#define PK_NOISY  3   /* still active, use noisy termination          */
#define PK_ERROR  4   /* giving up on this pick!                      */

char *PickStatus[] =  /* "state" strings for use in logging */
     { "PK_DONE",
       "PK_NEW",
       "PK_ACTIVE",
       "PK_NOISY",
       "PK_ERROR"  };

/* Define key processing parameters in coda determination
 ********************************************************/
#define NUM_PREEVENT_CAAV         2     /* # of caav's to average to get pre-pick */
                                        /*   amplitude (use in normal/noisy test) */
#define MAX_CODA_DURATION         144   /* if the caav has not decayed to the     */
                                        /*   termination amp by this many seconds */ 
                                        /*   after the pick, terminate it now and */
                                        /*   report the coda message              */
#define MAX_PROCESSING_TIME       180   /* maximum # of wall-clock seconds to     */
                                        /*   work on a given pick. If coda is not */
                                        /*   found, there must be a problem with  */
                                        /*   the trace.                           */
#define MIN_ACCEPTED_COMPLETENESS 0.25  /* minimum completeness level for caav    */ 
                                        /*   to consider for coda termination     */
#define NUM_BROKEN_TRACE          2     /* if trace has this many sequential      */
                                        /*   incomplete caavs, consider the trace */
                                        /*   "broken" and stop coda term search   */
                                        /*   No coda message will be produced     */

/* Coda amps to report in coda message.
 * In pick_ew, caav windows with specific post-P center times were saved 
 * during processing, and the last 6 caav amps were reported in the coda 
 * message. Here we store those "magic" center times. In coda_dur, we will
 * save the caavs of windows which contain the magic post-P times, but note
 * that these windows almost certainly won't be centered on that time.
 ***************************************************************************/
#define NUM_CAAV_REPORT   6         /* number of caavs to report in coda msg  */
#define NUM_MAGIC_CAAV   23         /* number of magic caav center times      */
double MagicCtime[NUM_MAGIC_CAAV] = /* magic post-P center times from pick_ew */
                    {   1.0,
                        3.0,
                        5.0,
                        7.0,
                        9.0,
                       11.0,
                       13.0,
                       15.0,
                       19.0,
                       23.0,
                       31.0,
                       39.0,
                       47.0,
                       55.0,
                       63.0,
                       71.0,
                       79.0,
                       87.0,
                       95.0,
                      111.0,
                      127.0,
                      139.0,
                      145.0  };

static STATION *StaArray = NULL;  /* Station array */ 
static STATION *Sta;              /* Pointer to the station being processed */
static STATION  key;              /* Key for binary search */
static int      Nsta = 0;         /* Number of stations in list */
static GPARM    Gparm;            /* Configuration file parameters */
static EWH      Ewh;              /* Parameters from earthworm.h    */

#define MAX_MSG_LEN 255
static char     Msg[MAX_MSG_LEN+1];    /* buffer for inbound transport msgs  */
static long     MsgLen;                /* actual length of retrieved msg     */
static MSG_LOGO Logo;                  /* Logo of retrieved msg              */
static char     OutMsg[MAX_MSG_LEN+1]; /* buffer for outbound transport msgs */
static MSG_LOGO OutLogo;               /* Logo of outbound msg               */

#define CODA_LOOP_PAUSE 100        /* max# coda aavs to process before going */
                                   /* back to looking for new picks */
   
      /***********************************************************
       *              The main program starts here.              *
       *                                                         *
       *  Argument:                                              *
       *     argv[1] = Name of coda_dur configuration file       *
       ***********************************************************/

/* Versioning introduced May 27, 2014

v0.0.1 2014-05-27
 */

#define VERSION "v0.0.1 2014-05-27"


int main( int argc, char **argv )
{
   int           i;                /* Loop counter */
   MSG_LOGO      hrtlogo;          /* Logo of outgoing heartbeats */
   SCNL_CAAV     scnlcaav;         /* struct for one CAAV msg */
   time_t        tlastbeat;        /* Previous heartbeat time */
   char         *configfile;       /* Pointer to name of config file */
   pid_t         myPid;            /* Process id of this process */
   int           ncodaloop;        /* number of caavs so far */
   unsigned char seq;              /* msg sequence number from tport_copyfrom() */

/* Check command line arguments
   ****************************/
   if( argc != 2 )
   {
      fprintf( stderr, "Usage: coda_dur <configfile>\n" );
      fprintf( stderr, "Version: %s\n", VERSION );
      return -1;
   }
   configfile = argv[1];

/* Initialize name of log-file & open it
   *************************************/
   logit_init( configfile, 0, 256, 1 );

/* Get parameters from the configuration files
   *******************************************/
   if( GetConfig( configfile, &Gparm ) == -1 )
   {
      logit( "e", "coda_dur: GetConfig() failed. Exiting.\n" );
      return -1;
   }

/* Look up info in the earthworm.h tables
   **************************************/
   if( GetEwh( &Ewh ) < 0 )
   {
      logit( "e", "coda_dur: GetEwh() failed. Exiting.\n" );
      return -1;
   }

/* Specify logos of incoming caav, picks and outgoing heartbeats
   *************************************************************/
   if( Gparm.nGetCaLogo == 0 ) 
   {
      Gparm.nGetCaLogo = 1;
      Gparm.GetCaLogo  = (MSG_LOGO *) calloc( Gparm.nGetCaLogo, sizeof(MSG_LOGO) );
      if( Gparm.GetCaLogo == NULL ) {
         logit( "e", "coda_dur: Error allocating space for GetCaLogo. Exiting\n" );
         return -1;
      }
      Gparm.GetCaLogo[0].instid = Ewh.InstIdWild;
      Gparm.GetCaLogo[0].mod    = Ewh.ModIdWild;
      Gparm.GetCaLogo[0].type   = Ewh.TypeCodaAAV;
   }
   if( Gparm.nGetPkLogo == 0 ) 
   {
      Gparm.nGetPkLogo = 1;
      Gparm.GetPkLogo  = (MSG_LOGO *) calloc( Gparm.nGetPkLogo, sizeof(MSG_LOGO) );
      if( Gparm.GetPkLogo == NULL ) {
         logit( "e", "coda_dur: Error allocating space for GetPkLogo. Exiting\n" );
         return -1;
      }
      Gparm.GetPkLogo[0].instid = Ewh.InstIdWild;
      Gparm.GetPkLogo[0].mod    = Ewh.ModIdWild;
      Gparm.GetPkLogo[0].type   = Ewh.TypePickSCNL;
   }

   hrtlogo.instid  = Ewh.MyInstId;
   hrtlogo.mod     = Gparm.MyModId;
   hrtlogo.type    = Ewh.TypeHeartBeat;

   OutLogo.instid  = Ewh.MyInstId;
   OutLogo.mod     = Gparm.MyModId;
   OutLogo.type    = Ewh.TypeCodaSCNL;

/* Get our own pid for restart purposes
   ************************************/
   myPid = getpid();
   if( myPid == -1 )
   {
      logit( "e", "coda_dur: Can't get my pid; exiting.\n" );
      cdur_free();
      return -1;
   }

/* Log the configuration parameters
   ********************************/
   LogConfig( &Gparm );
   logit( "t", "coda_dur: Version %s\n", VERSION);

/* Read the station list and return the number of stations found.
   Allocate the station list array.
   *************************************************************/
   if( cdur_stalist( &StaArray, &Nsta, &Gparm ) == -1 )
   {
      logit( "e", "coda_dur: cdur_stalist() failed; exiting.\n" );
      cdur_free();
      return -1;
   }

   if( Nsta == 0 )
   {
      logit( "et", "coda_dur: Empty station list(s); exiting." );
      cdur_free();
      return -1;
   }

/* Allocate per-station caav buffer and pick buffer
 **************************************************/
   for( i=0; i<Nsta; i++ )
   {
     Sta = &StaArray[i];
     Sta->caav = calloc( Gparm.mCaav, sizeof(CAAV) );
     if( Sta->caav == (CAAV *)NULL )
     {
        logit( "et", "coda_dur: Error allocating sta[%d] caav buffer; exiting.", i );
        cdur_free(); 
     }
     Sta->pk = calloc( Gparm.mPick, sizeof(PKINFO) );
     if( Sta->pk == (PKINFO *)NULL )
     {
        logit( "et", "coda_dur: Error allocating sta[%d] pick buffer; exiting.", i );
        cdur_free(); 
     }
   }

/* Sort the station list by SCNL
   *****************************/
   qsort( StaArray, Nsta, sizeof(STATION), CompareSCNL );

/* Log the station list
   ********************/
   LogStaList( StaArray, Nsta );

/* Attach to existing transport rings;
   CA region and Pk region need to be different!
   *********************************************/
   if( Gparm.CaKey == Gparm.PkKey )
   {
      logit("et","coda_dur: CodaAAVRing and PickRing must be different; exiting\n" );
      cdur_free();
      return -1;
   }
   tport_attach( &Gparm.CaRegion, Gparm.CaKey );
   tport_attach( &Gparm.PkRegion, Gparm.PkKey );

/* Flush CodaAAVRing, PickRing
   ***************************/
   while( tport_copyfrom( &Gparm.CaRegion, Gparm.GetCaLogo, (short)Gparm.nGetCaLogo, 
                          &Logo, &MsgLen, Msg, MAX_MSG_LEN, &seq ) != GET_NONE );
   while( tport_copyfrom( &Gparm.PkRegion, Gparm.GetPkLogo, (short)Gparm.nGetPkLogo, 
                          &Logo, &MsgLen, Msg, MAX_MSG_LEN, &seq ) != GET_NONE );

/* Force heartbeat on first attempt.
   *********************************/
   tlastbeat = 0;

/* Loop to read CodaAAV messages
   *****************************/
   ncodaloop = 0;
   while( tport_getflag( &Gparm.CaRegion ) != TERMINATE  &&
          tport_getflag( &Gparm.CaRegion ) != myPid )
   {
      int      icaav;
      int      rc;              /* Return code from tport_copyfrom() */
      time_t   now;             /* Current time */

   /* Send a heartbeat to the transport ring
      **************************************/
      time( &now );
      if( (now - tlastbeat) >= Gparm.HeartbeatInt )
      {
         int  lineLen;
         char line[40];

         sprintf( line, "%ld %d\n", (long) now, (int) myPid );
         lineLen = strlen( line );

         if( tport_putmsg( &Gparm.PkRegion, &hrtlogo, lineLen, line ) !=
             PUT_OK )
         {
            logit( "et", "coda_dur: Error sending heartbeat; exiting." );
            break;
         }
         tlastbeat = now;
      }

   /* Get coda_aav message from ring, process it
      ******************************************/
      rc = tport_copyfrom( &Gparm.CaRegion, 
                            Gparm.GetCaLogo, (short)Gparm.nGetCaLogo, 
                           &Logo, &MsgLen, Msg, MAX_MSG_LEN, &seq );

      if( rc == GET_NONE ) 
      {
         cdur_processpicks( );  /* process the picks we have */
         cdur_getpicks( );      /* look for new picks        */
         ncodaloop = 0;
         sleep_ew( 50 );
         continue;
      }

      if( rc == GET_NOTRACK )
         logit( "et", "coda_dur: CodaAAVRing tracking error (NTRACK_GET exceeded)\n");

      if( rc == GET_MISS_LAPPED )
         logit( "et", "coda_dur: CodaAAVRing missed msgs (lapped on ring) "
                "before i:%d m:%d t:%d seq:%d\n",
                (int)Logo.instid, (int)Logo.mod, (int)Logo.type, (int)seq );

      if( rc == GET_MISS_SEQGAP )
         logit( "et", "coda_dur: CodaAAVRing gap in sequence# before i:%d m:%d t:%d seq:%d\n",
                (int)Logo.instid, (int)Logo.mod, (int)Logo.type, (int)seq );

      if( rc == GET_TOOBIG )
      {
         logit( "et", 
                "coda_dur: CodaAAVRing retrieved msg is too big: i:%d m:%d t:%d len:%d\n",
                (int)Logo.instid, (int)Logo.mod, (int)Logo.type, MsgLen );
         continue;
      }

   /* Read the CAAV msg & stash it away
      *********************************/
      Msg[MsgLen]='\0'; /* null terminate for ease-of-handling */
      if( !rd_coda_aav( Msg, &scnlcaav ) ) 
      {
         logit( "et", "coda_dur: Error reading caav msg: %s\n", Msg );
         continue;
      }

   /* Look up SCNL number in the station list
      ***************************************/
      strncpy( key.sta,  scnlcaav.site,  TRACE2_STA_LEN  );
      strncpy( key.chan, scnlcaav.comp,  TRACE2_CHAN_LEN );
      strncpy( key.net,  scnlcaav.net,   TRACE2_NET_LEN  );
      strncpy( key.loc,  scnlcaav.loc,   TRACE2_LOC_LEN  );

      Sta = (STATION *) bsearch( &key, StaArray, Nsta, sizeof(STATION),
                                 CompareSCNL );

      if( Sta == NULL ) continue;  /* SCNL not found */
         
   /* Stash this CAAV msg away
      ************************/
      icaav = (int) (Sta->lcaav%Gparm.mCaav);
      memcpy( &(Sta->caav[icaav]), &(scnlcaav.caav), sizeof(CAAV) );  
      Sta->lcaav++;

   /* Time to pause from coda values and process picks
      ************************************************/
      if( ++ncodaloop >= CODA_LOOP_PAUSE ) {
         cdur_processpicks( );
         cdur_getpicks( );      /* look for new picks */
         ncodaloop = 0;
      }
 
   } /*end while*/

/* Detach from the ring buffers; free allocated memory
   ***************************************************/
   tport_detach( &Gparm.CaRegion );
   tport_detach( &Gparm.PkRegion ); 
   cdur_free();

   logit( "t", "Termination requested. Exiting.\n" );
   return 0;
}


      /*******************************************************
       *                      GetEwh()                       *
       *                                                     *
       *      Get parameters from the earthworm.h file.      *
       *******************************************************/

int GetEwh( EWH *Ewh )
{
   if ( GetLocalInst( &Ewh->MyInstId ) != 0 )
   {
      logit( "e", "coda_dur: Error getting MyInstId.\n" );
      return -1;
   }

   if ( GetInst( "INST_WILDCARD", &Ewh->InstIdWild ) != 0 )
   {
      logit( "e", "coda_dur: Error getting INST_WILDCARD.\n" );
      return -2;
   }
   if ( GetModId( "MOD_WILDCARD", &Ewh->ModIdWild ) != 0 )
   {
      logit( "e", "coda_dur: Error getting MOD_WILDCARD.\n" );
      return -3;
   }
   if ( GetType( "TYPE_HEARTBEAT", &Ewh->TypeHeartBeat ) != 0 )
   {
      logit( "e", "coda_dur: Error getting TYPE_HEARTBEAT.\n" );
      return -4;
   }
   if ( GetType( "TYPE_ERROR", &Ewh->TypeError ) != 0 )
   {
      logit( "e", "coda_dur: Error getting TYPE_ERROR.\n" );
      return -5;
   }
   if ( GetType( "TYPE_CODA_AAV", &Ewh->TypeCodaAAV ) != 0 )
   {
      logit( "e", "coda_dur: Error getting TYPE_CODA_AAV.\n" );
      return -7;
   }
   if ( GetType( "TYPE_PICK_SCNL", &Ewh->TypePickSCNL ) != 0 )
   {
      logit( "e", "coda_dur: Error getting TYPE_PICK_SCNL.\n" );
      return -8;
   }
   if ( GetType( "TYPE_CODA_SCNL", &Ewh->TypeCodaSCNL ) != 0 )
   {
      logit( "e", "coda_dur: Error getting TYPE_CODA_SCNL.\n" );
      return -9;
   }
   return 0;
}

             
/***************************************************************************
 * cdur_free()  free all previously allocated memory                       *
 ***************************************************************************/
void cdur_free( void )
{
   STATION *sta;
   int      i;

   free( Gparm.GetCaLogo );
   free( Gparm.GetPkLogo );
   free( Gparm.StaFile   );
   for( i=0, sta=StaArray; i<Nsta; i++, sta++ ) 
   {
      free( sta->caav );
      free( sta->pk   );
   }    
   free( StaArray );

   return;
}

         
/***************************************************************************
 * cdur_getpicks() read new picks from PickRing                            *
 ***************************************************************************/
void cdur_getpicks( void )
{
   EWPICK        pk;
   PKINFO       *pkinfo;
   unsigned char seq;   /* msg sequence number from tport_copyfrom() */
   int           rc;
   int           ipk;
   time_t        now;
 
   while( 1 )
   {

   /* Get pick message from ring, process it
      ***************************************/
      rc = tport_copyfrom( &Gparm.PkRegion, 
                            Gparm.GetPkLogo, (short)Gparm.nGetPkLogo, 
                           &Logo, &MsgLen, Msg, MAX_MSG_LEN, &seq );

      if( rc == GET_NONE ) break;  /* go back to coda_aav processing */

      if( rc == GET_NOTRACK )
         logit( "et", "coda_dur: Tracking error (NTRACK_GET exceeded)\n");

      if( rc == GET_MISS_LAPPED )
         logit( "et", "coda_dur: Missed msgs (lapped on ring) "
                "before i:%d m:%d t:%d seq:%d\n",
                (int)Logo.instid, (int)Logo.mod, (int)Logo.type, (int)seq );

      if( rc == GET_MISS_SEQGAP )
         logit( "et", "coda_dur: Gap in sequence# before i:%d m:%d t:%d seq:%d\n",
                (int)Logo.instid, (int)Logo.mod, (int)Logo.type, (int)seq );

      if( rc == GET_TOOBIG )
      {
        logit( "et", 
                "coda_dur: Retrieved msg is too big: i:%d m:%d t:%d len:%d\n",
                (int)Logo.instid, (int)Logo.mod, (int)Logo.type, MsgLen );
         continue;
      }

   /* Read the Pick msg & stash it away
      *********************************/
      Msg[MsgLen]='\0'; /* null terminate for ease-of-handling */

      if( Logo.type != Ewh.TypePickSCNL ) continue;
      if( rd_pick_scnl( Msg, MsgLen, &pk ) != EW_SUCCESS ) 
      {
         logit( "et", "coda_dur: Error reading pick msg: %s\n", Msg );
         continue;
      }

   /* Look up SCNL number in the station list
      ***************************************/
      strncpy( key.sta,  pk.site,  TRACE2_STA_LEN  );
      strncpy( key.chan, pk.comp,  TRACE2_CHAN_LEN );
      strncpy( key.net,  pk.net,   TRACE2_NET_LEN  );
      strncpy( key.loc,  pk.loc,   TRACE2_LOC_LEN  );

      Sta = (STATION *) bsearch( &key, StaArray, Nsta, sizeof(STATION),
                                 CompareSCNL );

      if( Sta == NULL ) continue;  /* SCNL not found */
         
   /* Stash this pick msg away
      ************************/
      ipk = (int) (Sta->lpk%Gparm.mPick);
      pkinfo = &(Sta->pk[ipk]);
      if( pkinfo->pkstat != PK_DONE  &&   /* complain if existing pick was not done */
          pkinfo->pkstat != PK_ERROR   )
      {
         logit("t","coda_dur: overwriting unfinished pick %s.%s.%s.%s tpick:%.3lf pkstat:%d\n",
                Sta->sta, Sta->chan, Sta->net, Sta->loc, pkinfo->tpick, pkinfo->pkstat );
      }
      pkinfo->msgtype = pk.msgtype;
      pkinfo->modid   = pk.modid;
      pkinfo->instid  = pk.instid;
      pkinfo->seq     = pk.seq;
      pkinfo->tpick   = pk.tpick;
      pkinfo->tstop   = time(&now)+MAX_PROCESSING_TIME;
      pkinfo->pkstat  = PK_NEW;
      pkinfo->nlow    = 0;
      pkinfo->icaav   = 0;                /* not found yet */
      pkinfo->lcaav   = 0;                /* not found yet */
      pkinfo->ecaav   = 0;                /* not found yet */
      pkinfo->cterm   = Sta->CodaTerm;    /* set to default at first */
      Sta->lpk++;

      if(Gparm.Debug) logit("t","GETPICKS: loaded %s.%s.%s.%s ip:%u as %s: %s", 
                             Sta->sta, Sta->chan, Sta->net, Sta->loc, 
                             Sta->lpk-1, PickStatus[pkinfo->pkstat], Msg );    

   } /*end while*/

   return;
}

/***************************************************************************
 * cdur_processpicks() check all picks for coda termination                *
 ***************************************************************************/
void cdur_processpicks( void )
{
   STATION       *sta;
   PKINFO        *pk;
   CAAV          *caav;
   double         precaav;
   time_t         now;
   int            reportcaav[NUM_CAAV_REPORT];
   unsigned long  firstpk;
   unsigned long  firstcaav;
   unsigned long  ip,ic;
   int            is;
   int            i,im,ireport;
   int            ncaav;
  
/* Loop over all stations 
 ************************/
   for( is=0; is<Nsta; is++ )
   {
      sta = &(StaArray[is]);
      firstpk = 0;
      if( sta->lpk > Gparm.mPick ) firstpk = sta->lpk - Gparm.mPick;
/*DEBUG*/ /*logit("","PROCPICK: is:%d  firstpk:%u  lpk:%u  mPick:%u\n",
                    is, firstpk, sta->lpk, Gparm.mPick );*/

   /* Loop over all picks for this station 
    **************************************/
      for( ip=firstpk; ip<sta->lpk; ip++ )
      {
         pk = &(sta->pk[ip%Gparm.mPick]);
         firstcaav = 0;
         if( sta->lcaav > Gparm.mCaav ) firstcaav = sta->lcaav - Gparm.mCaav;
/*DEBUG*/ /*logit("","PROCPICK: is:%d  ip:%u  %s firstcaav:%u  lcaav:%u  mCaav:u%\n",
                    is, ip, PickStatus[pk->pkstat], firstcaav, sta->lcaav, Gparm.mCaav );*/

      /* If PK_DONE, go to next pick  
       *****************************/
         if( pk->pkstat == PK_DONE  ) continue;
         if( pk->pkstat == PK_ERROR ) continue;

      /* Give up, regardless of pk_stat, if we've been working  
       * on this pick too long; its trace may be offline.
       *******************************************************/
         if( time(&now) > pk->tstop )
         {
            logit("t","%s: %s.%s.%s.%s ip:%u coda-processing clock expired (%ds); "
                      "possible dead trace, stop coda search.\n", PickStatus[pk->pkstat], 
                       sta->sta,sta->chan,sta->net,sta->loc, ip, MAX_PROCESSING_TIME );
            pk->pkstat = PK_ERROR;
            continue;
         }

      /* If PK_NEW, check pre-pick signal levels 
       *****************************************/
         if( pk->pkstat == PK_NEW ) 
         {
            unsigned long ibefore, iafter;  /* caav index before/after pick time */
            unsigned long lowcaav;
 
         /* Find/store index of caav containing P-time 
          ********************************************/
            ibefore   = 0;
            iafter    = sta->lcaav;
            pk->icaav = 0;
            for( ic=firstcaav; ic<sta->lcaav; ic++ ) 
            {
               caav = &(sta->caav[ic%Gparm.mCaav]);
               if( pk->tpick >= caav->tstart &&
                   pk->tpick <= caav->tend      ) /* P-time is in this caav window */
               {
                  pk->icaav = ic;
                  break;
               }
               if( pk->tpick > caav->tend   &&  ibefore < ic ) ibefore = ic;
               if( pk->tpick < caav->tstart &&  iafter  > ic ) iafter  = ic;
            }
            if( pk->icaav == 0 )                /* cannot find caav containing pick! */
            {
               if( ibefore == 0 )               /* pick's caav already fell off loop */
               {
                  pk->icaav = firstcaav;
                  if( firstcaav != 0 ) {
                     logit("t","WARNING is:%d %s.%s.%s.%s ip:%u firstcaav:%u lcaav:%u "
                            "caav containing pick time already lapped, using firstcaav\n",
                             is, sta->sta, sta->chan, sta->net, sta->loc, ip,
                             firstcaav, sta->lcaav );
                  } else {
                     logit("t","WARNING is:%d %s.%s.%s.%s ip:%u firstcaav:%u lcaav:%u "
                            "caav containing pick time occurred before startup, using firstcaav\n",
                             is, sta->sta, sta->chan, sta->net, sta->loc, ip,
                             firstcaav, sta->lcaav );
                  }
               }
               else if( iafter == sta->lcaav )  /* pick's caav hasn't arrived yet */
               { 
                  logit("t","WARNING is:%d %s.%s.%s.%s ip:%u firstcaav:%u lcaav:%u "
                            "no caav containing pick time, try later\n",
                             is, sta->sta, sta->chan, sta->net, sta->loc, ip,
                             firstcaav, sta->lcaav );
                  continue;
               } else {                        /* use 1st caav before P-time */               
                  pk->icaav = ibefore;
                  logit("t","WARNING is:%d %s.%s.%s.%s ip:%u firstcaav:%u lcaav:%u "
                            "no caav containing pick time, using ibefore:%u\n",
                             is, sta->sta, sta->chan, sta->net, sta->loc, ip,
                             firstcaav, sta->lcaav, ibefore );
               }               
            }
            pk->lcaav = pk->icaav;

         /* Find pre-P-time amplitude level 
          *********************************/
            lowcaav = 0;
            if( pk->icaav > NUM_PREEVENT_CAAV ) lowcaav = pk->icaav - NUM_PREEVENT_CAAV;
            ncaav   = 0;
            precaav = 0.0;
            for( ic=lowcaav; ic<pk->icaav; ic++ ) 
            {
               caav = &(sta->caav[ic%Gparm.mCaav]);
/*DEBUG*/    /*logit("","PK_NEW CAAV ic:%u   tstart:%.3lf  tend:%.3lf amp:%d comp:%.2f\n",
                      ic, caav->tstart, caav->tend, caav->amp, caav->completeness ); */
               precaav += (double)caav->amp;
               ncaav++;
            }
            if( ncaav != 0 ) precaav = precaav/ncaav;  /* average pre-pick caav value */

         /* Set termination value to noisy or normal 
          ******************************************/
            if( precaav >= sta->AltCoda*sta->CodaTerm )
            {
               pk->cterm  = sta->PreEvent*precaav;
               pk->pkstat = PK_NOISY;
            } else {
               pk->cterm  = sta->CodaTerm;
               pk->pkstat = PK_ACTIVE;
            }
            if(Gparm.Debug) 
            {
                caav = &(sta->caav[pk->icaav%Gparm.mCaav]);
                logit("t",
                      "PK_NEW: %s.%s.%s.%s ip:%u tpick:%.3lf icaav:%u %.3lf to %.3lf amp: %d comp: %.2f\n",
                       sta->sta, sta->chan, sta->net, sta->loc, ip, pk->tpick, pk->icaav,
                        caav->tstart, caav->tend, caav->amp, caav->completeness );
                logit("t",
                      "PK_NEW: %s.%s.%s.%s ip:%u pre-event: %.1lf codaterm: %.1lf new-status: %s\n",
                       sta->sta, sta->chan, sta->net, sta->loc, ip,
                       precaav, pk->cterm, PickStatus[pk->pkstat] );
            }

         } /* end if PK_NEW */

      /* If PK_NOISY or PK_ACTIVE, check caavs values and times 
       ********************************************************/
         if( pk->pkstat == PK_ACTIVE  ||  pk->pkstat == PK_NOISY )
         { 
            unsigned long lowcaav;        /* lowest caav to inspect */
            double        tmagic;
            int           termfound = 0;

         /* lowcaav = pk->icaav+1;*/ /* don't use pick's caav while finding codaterm */  
            lowcaav = pk->lcaav+1;   /* only look at newly arrived caavs & don't use */
                                     /* pick's caav while finding codaterm           */       
            if( lowcaav < firstcaav ) lowcaav = firstcaav;  /* lowcaav off caav-loop */

            for( ic=lowcaav; ic<sta->lcaav; ic++ )
            {
               caav = &(sta->caav[ic%Gparm.mCaav]);
               pk->lcaav = ic;

               if( caav->completeness >= MIN_ACCEPTED_COMPLETENESS ) pk->nlow=0;
               else                                                  pk->nlow++;

            /* If caav-value <= coda term, call it terminated 
             ************************************************/
               if( (double)caav->amp < pk->cterm )
               {
                  if( caav->completeness >= MIN_ACCEPTED_COMPLETENESS ) {
                     if(Gparm.Debug) {
                        logit("t","%s: %s.%s.%s.%s ip:%u termination amplitude reached ic:%u\n",
                               PickStatus[pk->pkstat],
                               sta->sta,sta->chan,sta->net,sta->loc, ip, ic );
                     }
                     termfound = 1;
                     break;
                  } else if( pk->nlow < NUM_BROKEN_TRACE ) {
                     logit("t","%s: %s.%s.%s.%s ip:%u termination amp found ic:%u, "
                               "but completeness:%.2lf too low; "
                               "nlow=%d, keep looking.\n",
                            PickStatus[pk->pkstat], sta->sta,sta->chan,sta->net,sta->loc, 
                            ip, ic, caav->completeness, pk->nlow );
                  } else {
                     logit("t","%s: %s.%s.%s.%s ip:%u termination amp found ic:%u, "
                               "but completeness:%.2lf too low; "
                               "nlow=%d, broken trace, stop coda search.\n",
                            PickStatus[pk->pkstat], sta->sta,sta->chan,sta->net,sta->loc, 
                            ip, ic, caav->completeness, pk->nlow );
                     pk->pkstat = PK_ERROR;
                     break;
                  }
               }
            /* If caav-time > MAX_CODA_DURATION, call it terminated 
             ******************************************************/
               if( caav->tend - pk->tpick  >=  (double)MAX_CODA_DURATION )
               {
                  if(Gparm.Debug) logit("t","%s: %s.%s.%s.%s ip:%u maximum duration reached ic:%u\n",
                                         PickStatus[pk->pkstat],      
                                         sta->sta,sta->chan,sta->net,sta->loc, ip, ic );
                  termfound = 1;
                  break;
               }
            } /* end for over all caavs looking for termination conditions */

         /* Found coda termination!
          *************************/
            if( termfound )
            {
               double cdur;     /* coda duration in decimal seconds */
               int    icdur;    /* coda duration in odd integer seconds */

               pk->ecaav = ic;  /* store ending caav for this pick */

            /* Determine the pick's official coda duration 
             *********************************************/
               cdur = caav->tend - pk->tpick;
               if( cdur >= MAX_CODA_DURATION ) {  /* set at max length coda, */
                  icdur = MAX_CODA_DURATION;      /* the only even duration  */
               } else {
                  icdur = (int) cdur;             /* set at nearest lower integer */
                  if( icdur%2 == 0 ) icdur--;     /* and make sure it's odd       */     
               }
               if( pk->pkstat == PK_NOISY ) icdur *= -1;  /* make all "noisy" durs negative */             
               if(Gparm.Debug) {
                  logit("","%s: %s.%s.%s.%s ip:%u TERM-CAAV ic:%u  tstart:%.3lf tend:%.3lf"
                             " amp: %d comp: %.2f  cdur: %.1lfs  icdur: %d\n",
                        PickStatus[pk->pkstat], sta->sta,sta->chan,sta->net,sta->loc, ip,
                        ic, caav->tstart, caav->tend, caav->amp, 
                        caav->completeness, cdur, icdur ); 
               }

            /* Gather magic caav windows to report in coda msg! 
             **************************************************/
               for(i=0; i<NUM_CAAV_REPORT; i++) reportcaav[i]=0;  
               reportcaav[0]= caav->amp;   /* always report terminating caav amp */
               ireport=1;

               im = NUM_MAGIC_CAAV-1;
               tmagic = pk->tpick + MagicCtime[im];
               for( ic=pk->ecaav-1; ic>=pk->icaav; ic-- )  /* loop backwards over pick's caavs */
               {
                  caav = &(sta->caav[ic%Gparm.mCaav]);
                  if(Gparm.Debug) {
                     logit("","MORE-CAAV ic:%u  tstart:%.3lf tend:%.3lf  amp:%d   comp:%.2f\n",
                           ic, caav->tstart, caav->tend, caav->amp, caav->completeness );
                  }
                  while( tmagic>caav->tend && im>0 )     /* skip magic bins later than caav */
                  {
                     im--;
                     tmagic = pk->tpick + MagicCtime[im];
                  }
                  if( tmagic>=caav->tstart )    /* see if it's one to report... */
                  {
                     if( tmagic<=caav->tend ) { /* tmagic is in this window, report it! */
                        reportcaav[ireport] = caav->amp;
                        if(Gparm.Debug) {
                           logit( "", 
                             "REPORT-CAAV[%d]: im:%d MagicCtime: %.1lf ic:%u amp:%d\n", 
                             ireport, im, MagicCtime[im], ic, caav->amp ); 
                        }  
                     } else {                   /* tmagic must've been in a gap, broken coda */
                        if(Gparm.Debug) {
                           logit("", 
                             "BROKEN-CODA: im:%d MagicCtime: %.1lf tmagic:%.3lf not in ic:%u\n", 
                              im, MagicCtime[im], tmagic, ic ); 
                        }  
                        break; 
                     } 
                     ireport++;
                     if( ireport == NUM_CAAV_REPORT ) break;
                     im--;
                     if( im < 0 ) break;
                     tmagic = pk->tpick + MagicCtime[im];
                  }
                  if( ic==0 ) break; /* no more caav bins to look at */
               } /* end for loop over caav to find magic bins */

            /* Build TYPE_CODA_SCNL message & write to ring 
             **********************************************/
               sprintf( OutMsg,"%d %d %d %d %s.%s.%s.%s",                        
                        (int) Ewh.TypeCodaSCNL,
                        (int) pk->modid,
                        (int) pk->instid,
                              pk->seq,
                              sta->sta,
                              sta->chan,
                              sta->net,
                              sta->loc );
               for( i=0; i<NUM_CAAV_REPORT; i++ ) {
                  sprintf( OutMsg+strlen(OutMsg), " %d", reportcaav[i] );
               }
  /*DEBUG*/ /* sprintf( OutMsg+strlen(OutMsg), " %d xx\n", icdur ); */  /* xx for debugging */
               sprintf( OutMsg+strlen(OutMsg), " %d\n",    icdur );     /* for production! */

               if( tport_putmsg( &Gparm.PkRegion, &OutLogo, strlen(OutMsg), OutMsg ) 
                                 != PUT_OK )
               {
                  logit( "et", 
                         "coda_dur: Error sending this TYPE_CODA_SCNL msg:\n%s", 
                          OutMsg );
               }
               pk->pkstat = PK_DONE;
               if( Gparm.Debug ) logit("t","%s: %s\n", PickStatus[pk->pkstat], OutMsg );
            } /* end if termination found */

         } /* end if PK_NOISY or PK_ACTIVE */

      } /* end for over picks at this station */

   } /* end for over stations */   

   return;
}
