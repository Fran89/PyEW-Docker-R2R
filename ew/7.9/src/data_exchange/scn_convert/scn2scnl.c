/*
 *   This file is managed using Concurrent Versions System (CVS).
 *
 *    $Id: scn2scnl.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.11  2006/12/28 23:27:53  lombard
 *     Added version number, printed on startup.
 *     Revised scnl2scn to provide complete mapping from SCNL back to SCN, using
 *     configuration command similar to scn2scnl.
 *
 *     Revision 1.10  2006/03/17 20:02:36  dietz
 *     Removed poorly placed semi-colon in "if GET_MISS_SEQGAP" statement.
 *
 *     Revision 1.9  2006/03/17 18:37:58  dietz
 *     Shortened sleep_ew time interval and added message logo/sequence number
 *     information to transport error logging.
 *
 *     Revision 1.8  2005/05/18 17:55:26  dietz
 *     Changed tport_copyto to tport_putmsg to avoid "sequence gap" swarms
 *     in modules reading scn2scnl's output when scn2scnl is processing only
 *     a subset of available channels.
 *
 *     Revision 1.7  2004/10/21 20:24:54  dietz
 *     null terminated InBuf before writing to log
 *
 *     Revision 1.6  2004/10/21 19:13:36  dietz
 *     Modified return codes from to_pick_scnl and to_coda_scnl;
 *     added erro logging in main code.
 *
 *     Revision 1.5  2004/10/20 00:39:21  lombard
 *     Corrected nget parameter in tport calls.
 *
 *     Revision 1.4  2004/10/19 21:54:04  lombard
 *     Changes to support rules for renaming specific and wild-carded SCNs to
 *     configured SCNLs.
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
 *                           scn2scnl.c                          *
 *                                                               *
 *  Program to add location codes to old-style messages, ie      *
 *  TYPE_TRACEBUF messages are converted to TYPE_TRACEBUF2.      *
 *  TYPE_PICK2K messages are converted to TYPE_PICK_SCNL.        *
 *  TYPE_CODA2K messages are converted to TYPE_CODA_SCNL.        *
 *****************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <transport.h>
#include <earthworm.h>
#include <trace_buf.h>
#include "scn_convert.h"

/* fixed for mavericks and valgrind bug, care of Larry Baker */
#define VERSION "1.1.1 2016-05-27"

/*************************************************
 *        The main program starts here.          *
 *************************************************/

int main( int argc, char *argv[] )
{
    char          InBuf[MAX_TRACEBUF_SIZ+1];  /* The input buffer */
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
    unsigned char mod_scn2scnl;


    /* Check command line arguments
     ****************************/
    if ( argc != 2 ) {
	printf( "Usage: scn2scnl <configfile>\n" );
	printf( "Version: scn2scnl %s\n", VERSION );
	return -1;
    }

    /* Open log file
     *************/
    logit_init( argv[1], 0, 256, 1 );

    /* Get parameters from the configuration files.
       GetConfig() exits on error.
    *******************************************/
    GetConfig( argv[1], &Gparm );
    sort_scn();

    /* Log the configuration parameters
     ********************************/
    LogConfig( &Gparm, VERSION );

    /* Get process ID for heartbeat messages
     *************************************/
    myPid = getpid();
    if ( myPid == -1 ) {
	logit( "e","scn2scnl: Cannot get pid. Exiting.\n" );
	return -1;
    }

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
    if ( GetInst( "INST_WILDCARD", &inst_wildcard ) != 0 ) {
	logit( "et", "scn2scnl: Error getting INST_WILDCARD. Exiting.\n" );
	return -1;
    }
    if ( GetLocalInst( &inst_local ) != 0 ) {
	logit( "et", "scn2scnl: Error getting MyInstId.\n" );
	return -1;
    }
    if ( GetModId( Gparm.MyModName, &mod_scn2scnl ) != 0 ) {
	logit("", "scn2scnl: Error getting mod-id for %s. Exiting.\n", Gparm.MyModName );
	return -1;
    }
    if ( GetType( "TYPE_TRACEBUF", &type_tracebuf ) != 0 ) {
	logit( "et", "scn2scnl: Error getting <TYPE_TRACEBUF>. Exiting.\n" );
	return -1;
    }
    if ( GetType( "TYPE_TRACEBUF2", &type_tracebuf2 ) != 0 ) {
	logit( "et", "scn2scnl: Error getting <TYPE_TRACEBUF2>. Exiting.\n" );
	return -1;
    }
    if ( GetType( "TYPE_PICK2K", &type_pick2k ) != 0 ) {
	logit( "et", "scn2scnl: Error getting <TYPE_PICK2K>. Exiting.\n" );
	return -1;
    }
    if ( GetType( "TYPE_PICK_SCNL", &type_pick_scnl ) != 0 ) {
	logit( "et", "scn2scnl: Error getting <TYPE_PICK_SCNL>. Exiting.\n" );
	return -1;
    }
    if ( GetType( "TYPE_CODA2K", &type_coda2k ) != 0 ) {
	logit( "et", "scn2scnl: Error getting <TYPE_CODA2K>. Exiting.\n" );
	return -1;
    }
    if ( GetType( "TYPE_CODA_SCNL", &type_coda_scnl ) != 0 ) {
	logit( "et", "scn2scnl: Error getting <TYPE_CODA_SCNL>. Exiting.\n" );
	return -1;
    }
    if ( GetType( "TYPE_HEARTBEAT", &type_heartbeat ) != 0 ) {
	logit( "et", "scn2scnl: Error getting <TYPE_HEARTBEAT>. Exiting.\n" );
	return -1;
    }
    if ( GetType( "TYPE_ERROR", &type_error ) != 0 ) {
	logit( "et", "scn2scnl: Error getting <TYPE_ERROR>. Exiting.\n" );
	return -1;
    }
    if ( GetModId( "MOD_WILDCARD", &mod_wildcard ) != 0 ) {
	logit( "et", "scn2scnl: Error getting MOD_WILDCARD. Exiting.\n" );
	return -1;
    }

    /* Specify logos of incoming and outgoing messages
     ***********************************************/
    getLogo[0].instid = inst_wildcard;
    getLogo[0].type   = type_tracebuf;
    getLogo[0].mod    = mod_wildcard;

    getLogo[1].instid = inst_wildcard;
    getLogo[1].type   = type_pick2k;
    getLogo[1].mod    = mod_wildcard;

    getLogo[2].instid = inst_wildcard;
    getLogo[2].type   = type_coda2k;
    getLogo[2].mod    = mod_wildcard;

    hrtLogo.instid    = inst_local;
    hrtLogo.type      = type_heartbeat;
    hrtLogo.mod       = mod_scn2scnl;

    errLogo.instid    = inst_local;
    errLogo.type      = type_error;
    errLogo.mod       = mod_scn2scnl;

    /* Flush input transport ring on startup
     *************************************/
    while ( tport_getmsg( &inRegion, getLogo, 3, &gotLogo, &gotSize,
			  InBuf, MAX_TRACEBUF_SIZ ) != GET_NONE );

    /* Get the time we start reading messages
     **************************************/
    time( &startTime );
    prevHeartTime = startTime;

    /* See if termination flag has been set
     ************************************/
    while ( 1 ) {
	time_t now;      /* Current time */
       
	sleep_ew( 50 );  /* can't sleep long if you're processing waveforms! */
       
	if ( tport_getflag( &inRegion ) == TERMINATE ||
	     tport_getflag( &inRegion ) == myPid ) {
	    tport_detach( &inRegion );
	    logit( "t", "Termination flag detected. Program stopping.\n" );
	    return 0;
	}

	/* Send heartbeat to output ring
         *****************************/
	time( &now );
	if ( Gparm.HeartbeatInt > 0 ) {
	    if ( (now - prevHeartTime) >= Gparm.HeartbeatInt ) {
		int  lineLen;
		char line[40];
	      
		prevHeartTime = now;
		snprintf( line, sizeof(line), "%ld %d\n", (long)now, (int) myPid );
		line[sizeof(line)-1] = 0;
		lineLen = (int)strlen( line );
	      
		if ( tport_putmsg( &outRegion, &hrtLogo, lineLen, line ) !=
		     PUT_OK ) {
		    logit( "et", "scn2scnl: Error sending heartbeat. Exiting." );
		    return -1;
		}
	    }
	}
      
	/* Get all available interesting messages from input ring
         ******************************************************/
	while ( 1 ) {
	    int rc;
	    unsigned char seq;
	  
	    rc = tport_copyfrom( &inRegion, getLogo, 3, &gotLogo, &gotSize,
				 InBuf, MAX_TRACEBUF_SIZ, &seq );
	  
	    if ( rc == GET_NONE )
		break;
	  
	    if ( rc == GET_TOOBIG ) {
		logit( "et", "scn2scnl: Retrieved msg is too big: "
                       "i:%d m:%d t:%d len:%d\n", (int)gotLogo.instid, 
                       (int)gotLogo.mod, (int)gotLogo.type, gotSize );
		break;
	    }
	 
	    if ( rc == GET_MISS_LAPPED )
		logit( "et", "scn2scnl: Missed msgs (lapped on ring) "
                       "before i:%d m:%d t:%d seq:%d\n", (int)gotLogo.instid, 
                       (int)gotLogo.mod, (int)gotLogo.type, (int)seq );
	 
	    if ( rc == GET_MISS_SEQGAP )    
		logit( "et", "scn2scnl: Gap in sequence# before "
                       "i:%d m:%d t:%d seq:%d\n", (int)gotLogo.instid, 
                       (int)gotLogo.mod, (int)gotLogo.type, (int)seq );

	    if ( rc == GET_NOTRACK )
		logit( "et", "scn2scnl: Tracking error (NTRACK_GET exceeded)\n" );

	    /* We got a TYPE_TRACEBUF message.  Convert it to
	       TYPE_TRACEBUF2 and send it to the output ring.
     	    **********************************************/
	    if ( gotLogo.type == type_tracebuf ) {
		MSG_LOGO outLogo;
	     
		rc = to_trace_scnl( InBuf );
		if (rc != 0) continue;  /* skip chans not in config */
		outLogo.instid = gotLogo.instid;
		outLogo.mod    = gotLogo.mod;
		outLogo.type   = type_tracebuf2;
		tport_putmsg( &outRegion, &outLogo, gotSize, InBuf );
	    }

	    /* We got a TYPE_PICK2K message.  Convert it to
	       TYPE_PICK_SCNL and send it to the output ring.
     	    *********************************************/
	    else if ( gotLogo.type == type_pick2k ) {
		char     OutBuf[120];
		MSG_LOGO outLogo;

                InBuf[gotSize]='\0';
 
		rc = to_pick_scnl( InBuf, OutBuf, sizeof(OutBuf), type_pick_scnl );
                if( rc != 0 )
                {
		   if ( rc == -1 )
		      logit( "et", "scn2scnl: TYPE_PICK_SCNL too big for output buffer\n" );
		   else if ( rc == -2 )
		      logit( "et", "scn2scnl: error decoding TYPE_PICK2K msg:\n%s\n",
                              InBuf );
		   continue;
                }
		
		outLogo.instid = gotLogo.instid;
		outLogo.mod    = gotLogo.mod;
		outLogo.type   = type_pick_scnl;
		tport_putmsg( &outRegion, &outLogo, (long)strlen(OutBuf), OutBuf );
	    }

	    /* We got a TYPE_CODA2K message.  Convert it to
	       TYPE_CODA_SCNL and send it to the output ring.
     	    *********************************************/
	    else if ( gotLogo.type == type_coda2k ) {
		char     OutBuf[120];
		MSG_LOGO outLogo;
	     
                InBuf[gotSize]='\0';

		rc = to_coda_scnl( InBuf, OutBuf, sizeof(OutBuf), type_coda_scnl );
                if( rc != 0 )
                {
		   if ( rc == -1 )
		      logit( "et", "scn2scnl: TYPE_CODA_SCNL too big for output buffer\n" );
		   else if ( rc == -2 )
		      logit( "et", "scn2scnl: error decoding TYPE_CODA2K msg:\n%s\n",
                              InBuf );
		   continue;
                }
		
		outLogo.instid = gotLogo.instid;
		outLogo.mod    = gotLogo.mod;
		outLogo.type   = type_coda_scnl;
		tport_putmsg( &outRegion, &outLogo, (long)strlen(OutBuf), OutBuf );
	    }
	}
    }
}
