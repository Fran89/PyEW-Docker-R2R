/* THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU CHECKED IT OUT!
 *
 *  $Id: coda_aav.c 491 2009-11-11 00:31:48Z dietz $
 * 
 *  Revision history:
 *   $Log$
 *   Revision 1.2  2009/11/11 00:31:48  dietz
 *   Fixed bug in finding 2s bin boundary when double roundoff errors cause it
 *   to be missed between packets; changed to report a "null" aav=0 when a
 *   complete aav window was in a data gap (ie, no data to compute aav).
 *
 *   Revision 1.1  2009/11/09 19:15:28  dietz
 *   Initial version, may still contain bugs and debugging statements
 *
 */

      /*****************************************************************
       *                           coda_aav.c                          *
       *                                                               *
       *  This program produces 2s coda avg absolute values (aav).     * 
       *  The program was decoupled from pick_ew so that picks and     *
       *  coda can potentially be produced from differently-processed  *
       *  waveform streams of a given SCNL.                            *
       *                                                               *
       *  Coda_aav only processes SCNLs listed in the pick_ew station  *
       *  list with pick_flag=1, and it uses only one parameter:       *
       *                                                               *
       *  Old name   New name                                          *
       *  --------   --------                                          *
       *     c1       RawDataFilt                                      *
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
#include <swap.h>
#include <trheadconv.h>
#include <rw_coda_aav.h>
#include "coda_aav.h"

/* Function prototypes
   *******************/
int  GetConfig( char *, GPARM * );
void LogConfig( GPARM * );
int  caav_stalist( STATION **, int *, GPARM * );
void LogStaList( STATION *, int );
int  CompareSCNL( const void *, const void * );
int  GetEwh( EWH * );
int  ReportAAV( STATION *, SHM_INFO *, MSG_LOGO * );

#define MAX_SAMPLE_TBUF 2016  /* max# 2-byte samples in 4096 byte trace pkt */
#define BINDUR          2.0   /* duration (seconds) of coda amp bin         */
   
      /***********************************************************
       *              The main program starts here.              *
       *                                                         *
       *  Argument:                                              *
       *     argv[1] = Name of coda_aav configuration file       *
       ***********************************************************/

/* Versioning introduced May 27, 2014  v0.0.1 2014.05.27 */

#define VERSION "v0.0.1 2014.05.27"

int main( int argc, char **argv )
{
   int           i;                /* Loop counter */
   STATION       *StaArray = NULL; /* Station array */
   char          *TraceBuf;        /* Pointer to waveform buffer */
   TRACE_HEADER  *TraceHead;       /* Pointer to trace header w/o loc code */
   TRACE2_HEADER *Trace2Head;      /* Pointer to header with loc code */
   int           *TraceLong;       /* Long pointer to waveform data */
   short         *TraceShort;      /* Short pointer to waveform data */
   int           Sample[MAX_SAMPLE_TBUF]; /* for expanding 2-byte data */
   int           *pTraceData;      /* pointer to TraceData to process */
   double        tsample;          /* time of a single sample */
   long          MsgLen;           /* Size of retrieved message */
   MSG_LOGO      logo;             /* Logo of retrieved msg */
   MSG_LOGO      hrtlogo;          /* Logo of outgoing heartbeats */
   MSG_LOGO      caavlogo;         /* Logo of outgoing coda_aav msgs */
   int           Nsta = 0;         /* Number of stations in list */
   time_t        tlastbeat;        /* Previous heartbeat time */
   GPARM         Gparm;            /* Configuration file parameters */
   EWH           Ewh;              /* Parameters from earthworm.h */
   char          *configfile;      /* Pointer to name of config file */
   pid_t         myPid;            /* Process id of this process */
   unsigned char seq;        /* msg sequence number from tport_copyfrom() */

/* Check command line arguments
   ****************************/
   if( argc != 2 )
   {
      fprintf( stderr, "Usage: coda_aav <configfile>\n" );
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
      logit( "e", "coda_aav: GetConfig() failed. Exiting.\n" );
      logit( "e", "Version %s \n", VERSION );
      return -1;
   }

/* Look up info in the earthworm.h tables
   **************************************/
   if( GetEwh( &Ewh ) < 0 )
   {
      logit( "e", "coda_aav: GetEwh() failed. Exiting.\n" );
      return -1;
   }

/* Specify logos of incoming waveforms and outgoing heartbeats
   ***********************************************************/
   if( Gparm.nGetLogo == 0 ) 
   {
      Gparm.nGetLogo = 2;
      Gparm.GetLogo  = (MSG_LOGO *) calloc( Gparm.nGetLogo, sizeof(MSG_LOGO) );
      if( Gparm.GetLogo == NULL ) {
         logit( "e", "coda_aav: Error allocating space for GetLogo. Exiting\n" );
         return -1;
      }
      Gparm.GetLogo[0].instid = Ewh.InstIdWild;
      Gparm.GetLogo[0].mod    = Ewh.ModIdWild;
      Gparm.GetLogo[0].type   = Ewh.TypeTracebuf2;

      Gparm.GetLogo[1].instid = Ewh.InstIdWild;
      Gparm.GetLogo[1].mod    = Ewh.ModIdWild;
      Gparm.GetLogo[1].type   = Ewh.TypeTracebuf;
   }

   hrtlogo.instid  = Ewh.MyInstId;
   hrtlogo.mod     = Gparm.MyModId;
   hrtlogo.type    = Ewh.TypeHeartBeat;

   caavlogo.instid = Ewh.MyInstId;
   caavlogo.mod    = Gparm.MyModId;
   caavlogo.type   = Ewh.TypeCodaAAV;

/* Get our own pid for restart purposes
   ************************************/
   myPid = getpid();
   if( myPid == -1 )
   {
      logit( "e", "coda_aav: Can't get my pid. Exiting.\n" );
      free( Gparm.GetLogo );
      free( Gparm.StaFile );
      return -1;
   }

/* Log the configuration parameters
   ********************************/
   LogConfig( &Gparm );
   logit( "t", "Version %s \n", VERSION );

/* Allocate the waveform buffer
   ****************************/
   TraceBuf = (char *) malloc( (size_t) MAX_TRACEBUF_SIZ );
   if( TraceBuf == NULL )
   {
      logit( "et", "coda_aav: Cannot allocate waveform buffer\n" );
      free( Gparm.GetLogo );
      free( Gparm.StaFile );
      return -1;
   }

/* Point to header and data portions of waveform message
   *****************************************************/
   TraceHead  = (TRACE_HEADER *) TraceBuf;
   Trace2Head = (TRACE2_HEADER *)TraceBuf;
   TraceLong  = (int *)   (TraceBuf + sizeof(TRACE_HEADER));
   TraceShort = (short *) (TraceBuf + sizeof(TRACE_HEADER));

/* Read the station list and return the number of stations found.
   Allocate the station list array.
   *************************************************************/
   if( caav_stalist( &StaArray, &Nsta, &Gparm ) == -1 )
   {
      logit( "e", "coda_aav: caav_stalist() failed. Exiting.\n" );
      free( Gparm.GetLogo );
      free( Gparm.StaFile );
      free( StaArray );
      return -1;
   }

   if( Nsta == 0 )
   {
      logit( "et", "coda_aav: Empty station list(s). Exiting." );
      free( Gparm.GetLogo );
      free( Gparm.StaFile );
      free( StaArray );
      return -1;
   }

/* Sort the station list by SCNL
   *****************************/
   qsort( StaArray, Nsta, sizeof(STATION), CompareSCNL );

/* Log the station list
   ********************/
   LogStaList( StaArray, Nsta );

/* Attach to existing transport rings
   **********************************/
   if ( Gparm.OutKey != Gparm.InKey )
   {
      tport_attach( &Gparm.InRegion,  Gparm.InKey );
      tport_attach( &Gparm.OutRegion, Gparm.OutKey );
   }
   else
   {
      tport_attach( &Gparm.InRegion, Gparm.InKey );
      Gparm.OutRegion = Gparm.InRegion;
   }

/* Flush the input ring
   ********************/
   while( tport_getmsg( &Gparm.InRegion, Gparm.GetLogo, (short)Gparm.nGetLogo, 
                        &logo, &MsgLen, TraceBuf, MAX_TRACEBUF_SIZ) != 
                        GET_NONE );

/* Force heartbeat on first attempt.
   *********************************/
   tlastbeat = 0;

/* Loop to read waveform messages 
   ******************************/
   while( tport_getflag( &Gparm.InRegion ) != TERMINATE  &&
          tport_getflag( &Gparm.InRegion ) != myPid )
   {
      STATION  key;             /* Key for binary search */
      STATION *Sta;             /* Pointer to the station being processed */
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

         if( tport_putmsg( &Gparm.OutRegion, &hrtlogo, lineLen, line ) !=
             PUT_OK )
         {
            logit( "et", "coda_aav: Error sending heartbeat. Exiting." );
            break;
         }
         tlastbeat = now;
      }

   /* Get tracebuf or tracebuf2 message from ring
      *******************************************/
      rc = tport_copyfrom( &Gparm.InRegion, Gparm.GetLogo, (short)Gparm.nGetLogo, 
                           &logo, &MsgLen, TraceBuf, MAX_TRACEBUF_SIZ, &seq );

      if( rc == GET_NONE )
      {
         sleep_ew( 50 );
         continue;
      }

      if( rc == GET_NOTRACK )
         logit( "et", "coda_aav: Tracking error (NTRACK_GET exceeded)\n");

      if( rc == GET_MISS_LAPPED )
         logit( "et", "coda_aav: Missed msgs (lapped on ring) "
                "before i:%d m:%d t:%d seq:%d\n",
                (int)logo.instid, (int)logo.mod, (int)logo.type, (int)seq );

      if( rc == GET_MISS_SEQGAP )
         logit( "et", "coda_aav: Gap in sequence# before i:%d m:%d t:%d seq:%d\n",
                (int)logo.instid, (int)logo.mod, (int)logo.type, (int)seq );

      if( rc == GET_TOOBIG )
      {
         logit( "et", 
                "coda_aav: Retrieved msg is too big: i:%d m:%d t:%d len:%d\n",
                (int)logo.instid, (int)logo.mod, (int)logo.type, MsgLen );
         continue;
      }

   /* If necessary, swap bytes in tracebuf message
      ********************************************/
      if( logo.type == Ewh.TypeTracebuf2 )
      {
         if( WaveMsg2MakeLocal( Trace2Head ) < 0 )
         {
            logit( "et", "coda_aav: WaveMsg2MakeLocal() error.\n" );
            continue;
         }
      } else {
         if ( WaveMsgMakeLocal( TraceHead ) < 0 )
         {
            logit( "et", "coda_aav: WaveMsgMakeLocal error.\n" );
            continue;
         }
      }

   /* Convert TYPE_TRACEBUF messages to TYPE_TRACEBUF2
      ************************************************/
      if( logo.type == Ewh.TypeTracebuf ) Trace2Head = TrHeadConv( TraceHead );

   /* Look up SCNL number in the station list
      ***************************************/
      strncpy( key.sta,  Trace2Head->sta,  TRACE2_STA_LEN  );
      strncpy( key.chan, Trace2Head->chan, TRACE2_CHAN_LEN );
      strncpy( key.net,  Trace2Head->net,  TRACE2_NET_LEN  );
      strncpy( key.loc,  Trace2Head->loc,  TRACE2_LOC_LEN  );

      Sta = (STATION *) bsearch( &key, StaArray, Nsta, sizeof(STATION),
                                 CompareSCNL );

      if( Sta == NULL ) continue;  /* SCNL not found */
         
   /* Create working pointer to trace data, convert shorts to ints if necessary
      *************************************************************************/
      if( Trace2Head->datatype[1] == '2' )
      {
         for( i=0; i<Trace2Head->nsamp; i++ ) Sample[i] = (int)TraceShort[i];
         pTraceData = &Sample[0];
      } else {
         pTraceData = TraceLong; /* data's already 4-byte ints, point into msg */
      }

   /* Initialize things the first time we get a message with this SCNL
      ****************************************************************/
      if( !Sta->ready )
      {
         Sta->samprate   = Trace2Head->samprate;
         Sta->sampintrvl = 1.0/Trace2Head->samprate;
         Sta->nbin       = (int) (Sta->samprate*BINDUR + 0.5);
         Sta->told       = Trace2Head->starttime - Sta->sampintrvl;
         Sta->old_sample = pTraceData[0];  /* use very first sample as "old" */
         Sta->rdat       = 0.0;
         Sta->tstart     = 0.0;
         Sta->tend       = 0.0;
         Sta->rsrdat     = 0.0;
         Sta->ndat       = 0;
         Sta->ready      = 1;
      } 
   /* Otherwise, update info if sample rate changed (RARE) 
      ****************************************************/
      else if( Trace2Head->samprate != Sta->samprate ) 
      {
         Sta->samprate   = Trace2Head->samprate;
         Sta->sampintrvl = 1.0/Trace2Head->samprate;
         Sta->nbin       = (int) (Sta->samprate*BINDUR + 0.5);
      }         

   /* Process this packet of data 
      ***************************/
      for( i=0, tsample=Trace2Head->starttime;
           i < Trace2Head->nsamp; 
           i++, tsample+=Sta->sampintrvl )
      {
         const double small_double = 1.0e-10;  /* used to avoid underflow in rdat */
  
/*DEBUG*//*logit("","SCNL packet: %s.%s.%s.%s tsample:%.8lf bindur:%6lf fmod:%.8lf sampint:%.8lf\n", 
               key.sta,key.chan,key.net,key.loc,
               tsample, (double)BINDUR, fmod(tsample,BINDUR), Sta->sampintrvl);  */

      /* Skip overlapping data */
         if( tsample <= Sta->told ) continue; 
        
      /* Past end of current aav bin */
         else if( tsample-Sta->tstart > BINDUR  &&    
                  Sta->tstart != 0.0  )    
         {
         /* Finalize this 2s bin, but handle multi-bin gaps, too */
            while( tsample-Sta->tstart > BINDUR ) 
            {
               ReportAAV( Sta, &Gparm.OutRegion, &caavlogo );   
               Sta->tstart += BINDUR;
               Sta->tend    = Sta->tstart - Sta->sampintrvl + BINDUR;
               Sta->rsrdat  = 0.0;
               Sta->ndat    = 0;
            }
         }

      /* Place start of first aav bin on a 2s boundary */
         else if( Sta->tstart == 0.0 )
         {
            double modbin = fmod(tsample,(double)BINDUR);
            double fudge  = 0.25;
         /* Usual case */
            if( modbin < Sta->sampintrvl )                
            {
               if(Gparm.Debug) {
                  logit("","%s.%s.%s.%s start aav on 2s boundary, normal case; "
                        "tsample:%.8lf modbin:%.8lf sampint:%.8lf\n", 
                         Sta->sta,Sta->chan,Sta->net,Sta->loc,
                         tsample,modbin,Sta->sampintrvl ); 
               }
               Sta->tstart = floor(tsample);
               Sta->tend   = Sta->tstart - Sta->sampintrvl + BINDUR;
               Sta->rsrdat = 0.0;
               Sta->ndat   = 0;
            }
         /* Catch 2s bin-boundary in weird inter-pkt time-roundoff case */
            else if( (double)BINDUR-modbin < fudge*Sta->sampintrvl )
            { 
               if(Gparm.Debug) {                                              
                  logit("","%s.%s.%s.%s start aav on 2s boundary, weird roundoff case; "
                        "tsample:%.8lf modbin:%.8lf bindur-modbin:%.8lf sampint:%.8lf\n", 
                         Sta->sta,Sta->chan,Sta->net,Sta->loc,
                         tsample,modbin,(double)BINDUR-modbin,Sta->sampintrvl );
               } 
               Sta->tstart = floor(tsample+fudge*Sta->sampintrvl);
               Sta->tend   = Sta->tstart - Sta->sampintrvl + BINDUR;
               Sta->rsrdat = 0.0;
               Sta->ndat   = 0;
            }
         }

      /* Compute new value of filtered data */
         Sta->rdat = (Sta->rdat * Sta->RawDataFilt) +
                     (double) (pTraceData[i] - Sta->old_sample) + 
                     small_double;
         
      /* Update values for current aav bin */
         Sta->rsrdat += fabs( Sta->rdat );
         Sta->ndat++;

      /* Store current sample/time for comparison with next */
         Sta->old_sample = pTraceData[i];
         Sta->told       = tsample;

      } /*end for*/
   } /*end while*/

/* Detach from the ring buffers
   ****************************/
   if ( Gparm.OutKey != Gparm.InKey )
   {
      tport_detach( &Gparm.InRegion );
      tport_detach( &Gparm.OutRegion );
   }
   else
      tport_detach( &Gparm.InRegion );

   logit( "t", "Termination requested. Exiting.\n" );
   free( Gparm.GetLogo );
   free( Gparm.StaFile );
   free( StaArray );
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
      logit( "e", "coda_aav: Error getting MyInstId.\n" );
      return -1;
   }

   if ( GetInst( "INST_WILDCARD", &Ewh->InstIdWild ) != 0 )
   {
      logit( "e", "coda_aav: Error getting INST_WILDCARD.\n" );
      return -2;
   }
   if ( GetModId( "MOD_WILDCARD", &Ewh->ModIdWild ) != 0 )
   {
      logit( "e", "coda_aav: Error getting MOD_WILDCARD.\n" );
      return -3;
   }
   if ( GetType( "TYPE_HEARTBEAT", &Ewh->TypeHeartBeat ) != 0 )
   {
      logit( "e", "coda_aav: Error getting TYPE_HEARTBEAT.\n" );
      return -4;
   }
   if ( GetType( "TYPE_ERROR", &Ewh->TypeError ) != 0 )
   {
      logit( "e", "coda_aav: Error getting TYPE_ERROR.\n" );
      return -5;
   }
   if ( GetType( "TYPE_CODA_AAV", &Ewh->TypeCodaAAV ) != 0 )
   {
      logit( "e", "coda_aav: Error getting TYPE_CODA_AAV.\n" );
      return -7;
   }
   if ( GetType( "TYPE_TRACEBUF", &Ewh->TypeTracebuf ) != 0 )
   {
      logit( "e", "coda_aav: Error getting TYPE_TRACEBUF.\n" );
      return -8;
   }
   if ( GetType( "TYPE_TRACEBUF2", &Ewh->TypeTracebuf2 ) != 0 )
   {
      logit( "e", "coda_aav: Error getting TYPE_TRACEBUF2.\n" );
      return -9;
   }
   return 0;
}

int ReportAAV( STATION *Sta, SHM_INFO *outring, MSG_LOGO *caavlogo )
{
   SCNL_CAAV ch;
   char      msg[MAX_CODA_AAV_LEN];
   int       rc; 
   
   strcpy( ch.site, Sta->sta  );
   strcpy( ch.net,  Sta->net  );
   strcpy( ch.comp, Sta->chan );
   strcpy( ch.loc,  Sta->loc  );
   ch.caav.tstart            = Sta->tstart;
   ch.caav.tend              = Sta->tend;
   if(Sta->ndat) ch.caav.amp = (int)(Sta->rsrdat/Sta->ndat+0.5);
   else          ch.caav.amp = 0;
   ch.caav.completeness      = ((float)Sta->ndat)/Sta->nbin;

   rc = wr_coda_aav( msg, MAX_CODA_AAV_LEN, &ch );
   if( rc == 0 ) 
   {
     logit ("","coda_aav: error writing TYPE_CODA_AAV msg\n" );
     return( 0 );
   }

   if( tport_putmsg( outring, caavlogo, strlen(msg), msg ) != PUT_OK )
   {
      logit( "et", "coda_aav: Error sending TYPE_CODA_AAV msg." );
      return( 0 );
   }
   return( 1 );
}
             
