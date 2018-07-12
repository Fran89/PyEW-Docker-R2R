/*
 *   This file is managed using Concurrent Versions System (CVS).
 *
 *    $Id: scnl2scn.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.10  2007/01/17 19:25:25  lombard
 *     Added call to sort_scnl() in scnl2scn; didn't work very well without it!
 *     Added optional Debug command to log configured mappings as well as
 *     the handling of each packet - very verbose, but helpful!
 *
 *     Revision 1.9  2007/01/11 23:26:45  dietz
 *     cleaned up Windows compile warnings/errors after Lombard's full SCNL->SCN
 *     remapping modifications to scnl2scn.
 *
 *     Revision 1.8  2006/12/28 23:27:53  lombard
 *     Added version number, printed on startup.
 *     Revised scnl2scn to provide complete mapping from SCNL back to SCN, using
 *     configuration command similar to scn2scnl.
 *
 *     Revision 1.7  2004/10/21 20:24:54  dietz
 *     null terminated InBuf before writing to log
 *
 *     Revision 1.6  2004/10/21 19:20:43  dietz
 *     Changed to log/skip TYPE_PICK_SCNL and TYPE_CODA_SCNL messages that
 *     it has trouble decoding.
 *
 *     Revision 1.5  2004/10/19 21:54:04  lombard
 *     Changes to support rules for renaming specific and wild-carded SCNs to
 *     configured SCNLs.
 *
 *     Revision 1.4  2004/07/14 16:43:03  dietz
 *     set tracebuf2 loc and version fields to nulls when converting to tracebuf
 *
 *     Revision 1.3  2004/05/27 20:00:55  kohler
 *     Bug fixes.
 *
 *     Revision 1.2  2004/05/26 15:36:18  kohler
 *     Added support for pick and coda conversion.
 *
 *     Revision 1.1  2004/05/18 22:57:30  kohler
 *     Initial entry into CVS.
 *
 *     Revision 1.1.1.1  2004/05/18 16:33:18  kohler
 *     initial import into CVS
 *
 */

  /*****************************************************************
   *                           scnl2scn.c                          *
   *                                                               *
   *  Program to remove location codes from new-style messages, ie *
   *  TYPE_TRACEBUF2 messages are converted to TYPE_TRACEBUF.      *
   *****************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <transport.h>
#include <earthworm.h>
#include <trace_buf.h>
#include "scnl2scn.h"
#include "scnl_convert.h"

#define VERSION "1.2.1 2016-05-27"
int Debug = 0;


         /*************************************************
          *        The main program starts here.          *
          *************************************************/

/* external func */
void log_s2s_config();

int main( int argc, char *argv[] )
{
   char          *InBuf;          /* The input buffer */
   TRACE2_HEADER *Trace2Head;     /* The tracebuf2 header */
   SHM_INFO      inRegion;        /* Input shared memory region */
   SHM_INFO      outRegion;       /* Output shared memory region */
   MSG_LOGO      getLogo[3];
   MSG_LOGO      gotLogo;
   MSG_LOGO      hrtLogo;         /* Logo of outgoing heartbeats */
   MSG_LOGO      errLogo;         /* Logo of outgoing errors */
   GPARM         Gparm;           /* Configuration file parameters */
   long          gotSize;
   time_t        startTime;
   time_t        prevHeartTime;
   pid_t         myPid;           /* for restarts by startstop */
   unsigned char inst_wildcard;
   unsigned char inst_local;
   unsigned char type_tracebuf;
   unsigned char type_tracebuf2;
   unsigned char type_pick2k;
   unsigned char type_coda2k;
   unsigned char type_pick_scnl;
   unsigned char type_coda_scnl;
   unsigned char type_heartbeat;
   unsigned char type_error;
   unsigned char mod_wildcard;
   unsigned char mod_scnl2scn;

   int InBufSize = MAX_TRACEBUF_SIZ+1;

/* Check command line arguments
   ****************************/
   if ( argc != 2 )
   {
      printf( "Usage: scnl2scn <configfile>\n" );
      printf( "Version: scnl2scn %s\n", VERSION );
      return -1;
   }

/* Open log file
   *************/
   logit_init( argv[1], 0, 512, 1 );

/* Get parameters from the configuration files.
   GetConfig() exits on error.
   *******************************************/
   GetConfig( argv[1], &Gparm );
   sort_scnl();

/* Log the configuration parameters
   ********************************/
   LogConfig( &Gparm, VERSION );
   if (Debug) 
       log_s2s_config();

/* Get process ID for heartbeat messages
   *************************************/
   myPid = getpid();
   if ( myPid == -1 )
   {
     logit( "e","scnl2scn: Cannot get pid. Exiting.\n" );
     return -1;
   }

/* Allocate the input buffer
   *************************/
   InBuf = (char *) malloc( InBufSize );
   if ( InBuf == NULL )
   {
      logit( "t", "Error allocating input buffer.  Exiting.\n" );
      return -1;
   }
   Trace2Head = (TRACE2_HEADER *)InBuf;

/* Attach to the input and output transport rings.
   The two rings may actually be the same.
   **********************************************/
   tport_attach( &inRegion, Gparm.InKey );
   if ( Gparm.InKey == Gparm.OutKey )
      outRegion = inRegion;
   else
      tport_attach( &outRegion, Gparm.OutKey );

/* Get logos to be used later
   **************************/
   if ( GetInst( "INST_WILDCARD", &inst_wildcard ) != 0 )
   {
      logit( "et", "scnl2scn: Error getting INST_WILDCARD. Exiting.\n" );
      return -1;
   }
   if ( GetLocalInst( &inst_local ) != 0 )
   {
      logit( "et", "scnl2scn: Error getting MyInstId.\n" );
      return -1;
   }
   if ( GetModId( Gparm.MyModName, &mod_scnl2scn ) != 0 )
   {
      logit("", "scnl2scn: Error getting mod-id for %s. Exiting.\n", Gparm.MyModName );
      return -1;
   }
   if ( GetType( "TYPE_TRACEBUF", &type_tracebuf ) != 0 )
   {
      logit( "et", "scnl2scn: Error getting <TYPE_TRACEBUF>. Exiting.\n" );
      return -1;
   }
   if ( GetType( "TYPE_TRACEBUF2", &type_tracebuf2 ) != 0 )
   {
      logit( "et", "scnl2scn: Error getting <TYPE_TRACEBUF2>. Exiting.\n" );
      return -1;
   }
   if ( GetType( "TYPE_PICK2K", &type_pick2k ) != 0 )
   {
      logit( "et", "scnl2scn: Error getting <TYPE_PICK2K>. Exiting.\n" );
      return -1;
   }
   if ( GetType( "TYPE_PICK_SCNL", &type_pick_scnl ) != 0 )
   {
      logit( "et", "scnl2scn: Error getting <TYPE_PICK_SCNL>. Exiting.\n" );
      return -1;
   }
   if ( GetType( "TYPE_CODA2K", &type_coda2k ) != 0 )
   {
      logit( "et", "scnl2scn: Error getting <TYPE_CODA2K>. Exiting.\n" );
      return -1;
   }
   if ( GetType( "TYPE_CODA_SCNL", &type_coda_scnl ) != 0 )
   {
      logit( "et", "scnl2scn: Error getting <TYPE_CODA_SCNL>. Exiting.\n" );
      return -1;
   }
   if ( GetType( "TYPE_HEARTBEAT", &type_heartbeat ) != 0 )
   {
      logit( "et", "scnl2scn: Error getting <TYPE_HEARTBEAT>. Exiting.\n" );
      return -1;
   }
   if ( GetType( "TYPE_ERROR", &type_error ) != 0 )
   {
      logit( "et", "scnl2scn: Error getting <TYPE_ERROR>. Exiting.\n" );
      return -1;
   }
   if ( GetModId( "MOD_WILDCARD", &mod_wildcard ) != 0 )
   {
      logit( "et", "scnl2scn: Error getting MOD_WILDCARD. Exiting.\n" );
      return -1;
   }

/* Specify logos of incoming and outgoing messages
   ***********************************************/
   getLogo[0].instid = inst_wildcard;
   getLogo[0].type   = type_tracebuf2;
   getLogo[0].mod    = mod_wildcard;

   getLogo[1].instid = inst_wildcard;
   getLogo[1].type   = type_pick_scnl;
   getLogo[1].mod    = mod_wildcard;

   getLogo[2].instid = inst_wildcard;
   getLogo[2].type   = type_coda_scnl;
   getLogo[2].mod    = mod_wildcard;

   hrtLogo.instid    = inst_local;
   hrtLogo.type      = type_heartbeat;
   hrtLogo.mod       = mod_scnl2scn;

   errLogo.instid    = inst_local;
   errLogo.type      = type_error;
   errLogo.mod       = mod_scnl2scn;

/* Flush input transport ring on startup
   *************************************/
   while ( tport_getmsg( &inRegion, getLogo, 3, &gotLogo, &gotSize,
                         InBuf, InBufSize ) != GET_NONE );

/* Get the time we start reading messages
   **************************************/
   time( &startTime );
   prevHeartTime = startTime;

/* See if termination flag has been set
   ************************************/
   while ( 1 )
   {
      time_t now;              /* Current time */

      sleep_ew( 200 );

      if ( tport_getflag( &inRegion ) == TERMINATE ||
           tport_getflag( &inRegion ) == myPid )
      {
         tport_detach( &inRegion );
         logit( "t", "Termination flag detected. Program stopping.\n" );
         return 0;
      }

/* Send heartbeat to output ring
   *****************************/
      time( &now );
      if ( Gparm.HeartbeatInt > 0 )
      {
         if ( (now - prevHeartTime) >= Gparm.HeartbeatInt )
         {
            int  lineLen;
            char line[40];

            prevHeartTime = now;
            snprintf( line, sizeof(line), "%ld %d\n", (long)now, myPid );
            line[sizeof(line)-1] = 0;
            lineLen = (int)strlen( line );

            if ( tport_putmsg( &outRegion, &hrtLogo, lineLen, line ) !=
                 PUT_OK )
            {
               logit( "et", "scnl2scn: Error sending heartbeat. Exiting." );
               return -1;
            }
         }
      }

/* Get all available interesting messages from input ring
   ******************************************************/
      while ( 1 )
      {
         int fromRes;
         unsigned char seq;

         fromRes = tport_copyfrom( &inRegion, getLogo, 3, &gotLogo, &gotSize,
                                   InBuf, InBufSize-1, &seq );

         if ( fromRes == GET_NONE )
            break;

         if ( fromRes == GET_TOOBIG )
         {
            logit( "et", "scnl2scn: Retrieved message is too big (%d)\n", gotSize );
            break;
         }

         if ( fromRes == GET_NOTRACK )
            logit( "et", "scnl2scn: NTRACK_GET exceeded.\n" );

         if ( fromRes == GET_MISS_LAPPED )
            logit( "et", "scnl2scn: GET_MISS_LAPPED error.\n" );

         if ( fromRes == GET_MISS_SEQGAP );     /* Do nothing */

/* We got a TYPE_TRACEBUF2 message.  Convert it to
   TYPE_TRACEBUF and send it to the output ring.
   ****************************************************************/
         if ( gotLogo.type == type_tracebuf2 )
         {
	     MSG_LOGO outLogo;
	     int rc;
	     
	     rc = to_trace_scn( InBuf );
	     if (rc != 0) continue;  /* skip chans not in config */

	     outLogo.instid = gotLogo.instid;
	     outLogo.mod    = gotLogo.mod;
	     outLogo.type   = type_tracebuf;
	     tport_putmsg( &outRegion, &outLogo, gotSize, InBuf );
         }

/* We got a TYPE_PICK_SCNL message.  Convert it to
   TYPE_PICK2K and send it to the output ring.
   ***********************************************/
         if ( gotLogo.type == type_pick_scnl )
         {
            char     OutBuf[120];
            MSG_LOGO outLogo;
	    int rc;
	    
            InBuf[gotSize] = '\0';

            rc = to_pick2k( InBuf, OutBuf, type_pick2k );
	    if (rc != 0) continue;  /* skip chans not in config */

            outLogo.instid = gotLogo.instid;
            outLogo.mod    = gotLogo.mod;
            outLogo.type   = type_pick2k;
            tport_copyto( &outRegion, &outLogo, (long)strlen(OutBuf), OutBuf, seq );
         }

/* We got a TYPE_CODA_SCNL message.  Convert it to
   TYPE_CODA2K and send it to the output ring.
   ***********************************************/
         if ( gotLogo.type == type_coda_scnl )
         {
            char     OutBuf[120];
            MSG_LOGO outLogo;
	    int rc;
	    
            InBuf[gotSize] = '\0';

            rc = to_coda2k( InBuf, OutBuf, type_coda2k );
	    if (rc != 0) continue;  /* skip chans not in config */

            outLogo.instid = gotLogo.instid;
            outLogo.mod    = gotLogo.mod;
            outLogo.type   = type_coda2k;
            tport_copyto( &outRegion, &outLogo, (long)strlen(OutBuf), OutBuf, seq );
         }
      }
   }
}
