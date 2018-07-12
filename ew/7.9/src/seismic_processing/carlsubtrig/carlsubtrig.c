
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: carlsubtrig.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.5  2004/05/11 17:49:07  lombard
 *     Added support for location code, TYPE_CARLSTATRIG_SCNL and TYPE_TRIGLIST_SCNL
 *     messages.
 *     Removed OS2 support
 *
 *     Revision 1.4  2002/06/05 14:52:32  patton
 *     Made logit changes.
 *
 *     Revision 1.3  2001/05/09 18:19:28  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or csuNet.MyPid.
 *
 *     Revision 1.2  2000/08/08 18:33:24  lucky
 *     Lint cleanup
 *
 *     Revision 1.1  2000/02/14 16:14:42  lucky
 *     Initial revision
 *
 *
 */

/*
 * carlsubtrig.c: Subnet part of Carl Johnson's Coincidence Trigger code 
 *		as an earthworm module which:
 *              1) reads a configuration file using kom.c routines 
 *                 (ReadConfig).
 *              2) looks up shared memory keys, installation ids, 
 *                 module ids, message types from earthworm.h tables 
 *                 using getutil.c functions (ReadEWH).
 *              3) attaches to two public shared memory regions for
 *                 input and output using transport.c functions.
 *              4) processes hard-wired message types from configuration-
 *                 file-given installations & module ids (This source
 *                 code expects to process TYPE_CARLSTATRIG messages).
 *              5) produces TYPE_TRIGLIST2K messages.
 *              6) sends heartbeats and error messages back to the
 *                 shared memory region (StatusReport).
 *              7) writes to a log file using logit.c functions.
 */
/* changes: 
  Lombard: 11/19/98: V4.0 changes: 
     0) changed TRIGLIST message to TYPE_TRIGLIST2K
     1) changed argument of logit_init to the config file name.
     2) process ID in heartbeat message: already done
     3) flush input transport ring
     4) add `restartMe' to .desc file
     5) multi-threaded logit
*/
#define CARLSUB_VERSION "1.0.9 - 2016-06-03"
/*******							*********/
/*	Functions defined in this source file				*/
/*******							*********/

/*	Function: main							*/
/*									*/
/*	Inputs:		Configuration File Name				*/
/*									*/
/*	Outputs:	Log Messages					*/
/*									*/
/*	Returns:	0 on success					*/
/*			error code as defined in carlsubtrig.h on failure	*/

/*******							*********/
/*	System Includes							*/
/*******							*********/
#include <stdio.h>
#include <stdlib.h>	/* exit, free, malloc, realloc			*/
#include <sys/types.h>	/* time						*/
#include <time.h>	/* time						*/

/*******							*********/
/*	Earthworm Includes						*/
/*******							*********/
#include <earthworm.h>	/* logit					*/
#include <transport.h>	/* MSG_LOGO, SHM_INFO, tport_attach,		*/
/*   tport_detach, tport_getflag, tport_getmsg	*/
#define THREAD_STACK	1024
/*******							*********/
/*	CarlSubTrig Includes						*/
/*******							*********/
#include "carlsubtrig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*******                                                        *********/
/*      Global variable declarations                                    */
/*******                                                        *********/
volatile time_t SubnetThreadHeart;     /* value changed in sbntthrd.c,  */
                                       /* monitored in the main thread  */

/*	Function: main							*/
int main( int argc, char* argv[] )
{
  char		msg[MAXMESSAGELEN];	/* Status or error text.	*/
  char		inBuf[MAXLINELEN];	/* Pointer to the input message */
                                        /*   buffer.			*/
  unsigned int	tidSubnet;	/* id of Subnet thread			*/
  int		result;		/* Result from function calls.		*/
  long		sizeMsg;	/* Size of retrieved message.		*/
  MSG_LOGO	logoMsg;	/* Logo of retrieved message.		*/
  MSG_LOGO	logoStaTrig;	/* Logo of requested station triggers.	*/
  NETWORK	csuNet;		/* The entire network's information.	*/
  SHM_INFO	regionIn;	/* Input shared memory region info.	*/
  time_t	timeNow;	/* Current time.			*/
  time_t	timeLastBeat;	/* Time last heartbeat was sent.	*/

  /*	Check command line arguments					*/
  if ( 2 != argc )
  {
    fprintf( stderr, "Usage: %s <configfile>\n", argv[0] );
    fprintf( stderr, "Version: %s\n", CARLSUB_VERSION );
    exit( ERR_USAGE );
  }
           
  /*	Initialize parameter structures to defaults			*/
  result = InitializeParameters( &csuNet );
  if ( CT_FAILED( result ) )
  {
    fprintf( stderr, "%s module initialization failed.\n", argv[0] );
    exit( ERR_INIT );
  }

  /*	Initialize message logging 					*/
  logit_init( argv[1], 0, MAXMESSAGELEN, 1 );

  /*	Read the configuration file					*/
  result = ReadConfig( argv[1], &csuNet );
  if ( CT_FAILED( result ) )
  {
    fprintf( stderr, "%s error in ReadConfig: %d!\n", argv[0], result );
    exit( result );
  }

  /*	Look up important info from earthworm.h tables			*/
  result = ReadEWH( &csuNet );
  if ( CT_FAILED( result ) )
  {
    fprintf( stderr, "%s error in ReadEWH: %d!\n", argv[0], result );
    exit( result );
  }
 
  /*	Log a startup message						*/
  if ( csuNet.csuParam.debug ) {
      logit( "t", "%s initialized with configuration file '%s'\n", argv[0],
	     argv[1] );
      logit( "", "carlsubtrig version: %s\n",
	     CARLSUB_VERSION);
      logit( "", "accepting %s TYPE_CARLSTATRIG_SCNL messages\n",
	     CST_VERSION);
  }

  /*    Allocate some space for the output message */
  csuNet.trigMsgBuf = calloc( 1, csuNet.trigMsgBufLen );
  if ( csuNet.trigMsgBuf == ( char* ) NULL )
  {
     logit( "e", 
            "carlSubTrig: Cannot allocate %d bytes for trigger message buffer\n",
             csuNet.trigMsgBufLen );
     exit( ERR_INIT );
  }
  
  /*	Read the station file						*/
  result = ReadStations( &csuNet );
  if ( CT_FAILED( result ) )
    exit( result );
  
  /*	Read the subnet file						*/
  result = ReadSubnets( &csuNet );
  if ( CT_FAILED( result ) )
    exit( result );
  
  /* Start the subnet thread						*/
  SubnetThreadHeart = time( NULL );
  CreateSpecificMutex( &(csuNet.stationMutex) );
  if ( StartThreadWithArg( SubnetThread, ( void *) &csuNet, 
   			   ( unsigned ) THREAD_STACK, &tidSubnet ) == -1 )
  {
    logit( "e", "carlSubTrig: error starting subnet thread. Exitting.\n");
    exit( -1 );
  }
  
  /*	Attach to Input and Output shared memory rings			*/
  tport_attach( &regionIn, csuNet.csuParam.ringInKey );
  if ( csuNet.csuParam.debug )
    logit( "t", "%s: Attached to public memory region %s:%d for input.\n", 
	   argv[0], csuNet.csuParam.ringIn, csuNet.csuParam.ringInKey );

  tport_attach( &(csuNet.regionOut), csuNet.csuParam.ringOutKey );
  if ( csuNet.csuParam.debug )
    logit( "t", "%s: Attached to public memory region %s:%d for output.\n", 
	   argv[0], csuNet.csuParam.ringOut, csuNet.csuParam.ringOutKey );

  /*	Specify logos of incoming station triggers			*/
  logoStaTrig.instid = csuNet.csuEwh.readInstId;
  logoStaTrig.mod    = csuNet.csuEwh.readModId;
  logoStaTrig.type   = csuNet.csuEwh.typeCarlStaTrig;
  
  /*	Specify logos of outgoing messages				*/
  csuNet.hrtLogo.instid = csuNet.csuEwh.myInstId;
  csuNet.hrtLogo.mod    = csuNet.csuEwh.myModId;
  csuNet.hrtLogo.type   = csuNet.csuEwh.typeHeartbeat;

  csuNet.errLogo.instid = csuNet.csuEwh.myInstId;
  csuNet.errLogo.mod    = csuNet.csuEwh.myModId;
  csuNet.errLogo.type   = csuNet.csuEwh.typeError;

  csuNet.trgLogo.instid = csuNet.csuEwh.myInstId;
  csuNet.trgLogo.mod    = csuNet.csuEwh.myModId;
  csuNet.trgLogo.type   = csuNet.csuEwh.typeTrigList;

  if ( csuNet.csuParam.debug > 3 )
    logit( "t", "%s: Reading messages (i%u m%u t%u)\n", argv[0],
	   logoStaTrig.instid, logoStaTrig.mod, logoStaTrig.type );
 
  /*	Flush the input ring						*/
  if ( csuNet.csuParam.debug > 2 )
    logit( "t", "%s: Flushing input buffer...", argv[0] );
  while ( tport_getmsg( &regionIn, &logoStaTrig, (short)1, &logoMsg, &sizeMsg,
			inBuf, MAXLINELEN - 1 ) == GET_OK );
  if ( csuNet.csuParam.debug > 2 )
    logit( "", "Done\n", argv[0] );

  /*	Force a heartbeat to be issued in first pass thru main loop	*/
  timeLastBeat = time( &timeNow ) - csuNet.csuParam.heartbeatInt - 1;

  /*	Main message processing loop					*/
  while ( tport_getflag( &regionIn ) != TERMINATE  && 
          tport_getflag( &regionIn ) != csuNet.MyPid  )
  {
    /*	Check for need to send heartbeat message			*/
    if ( time( &timeNow ) - timeLastBeat >= csuNet.csuParam.heartbeatInt )
    {
    /* Beat heart only if SubnetThread is still looping */
      if( timeNow - SubnetThreadHeart < csuNet.csuParam.heartbeatInt/2 )
      {
        StatusReport( &csuNet, csuNet.csuEwh.typeHeartbeat, 0, "" );
        if ( csuNet.csuParam.debug > 3 )
          logit( "t", "%s: Sent Heartbeat\n", argv[0] );
      }
    /* SubnetThread is stuck; don't beat heart; say why */
      else  
      {
        sprintf( msg, "%s: Skipping Heartbeat; no activity in "
                      " SubnetThread for %ld sec", 
                 argv[0], (long)(timeNow - SubnetThreadHeart) );
        StatusReport( &csuNet, csuNet.csuEwh.typeError, ERR_THRDSTUCK, msg );
      }
      timeLastBeat = timeNow;
    }
    
    /*	Get next message						*/
    result = tport_getmsg( &regionIn, &logoStaTrig, (short)1, &logoMsg, 
			   &sizeMsg, inBuf, MAXLINELEN - 1 );

    /*	Act on the return code from the transport function		*/
    switch ( result )
    {
    case GET_OK:	/* got a message, no errors or warnings		*/
      if ( csuNet.csuParam.debug > 3 )
	logit( "t", "%s: Message received(i%u m%u t%u).\n", argv[0],
	       logoMsg.instid, logoMsg.mod, logoMsg.type );
      break;
    case GET_NONE:	/* no messages of interest, check again later	*/
      sleep_ew( 500 );	/* milliseconds				*/
      continue;
    case GET_NOTRACK:	/* got a msg, but can't tell if any were missed	*/
      sprintf( msg, "%s: Message received(i%u m%u t%u); NTRACK_GET exceeded",
	       argv[0], logoMsg.instid, logoMsg.mod, logoMsg.type );
      StatusReport( &csuNet, csuNet.csuEwh.typeError, ERR_NOTRACK, msg );
      break;
    case GET_MISS_LAPPED:	/* got a msg, but also missed lots	*/
      sprintf( msg, "%s: Message received(i%u m%u t%u); missed lots",
	       argv[0], logoMsg.instid, logoMsg.mod, logoMsg.type );
      StatusReport( &csuNet, csuNet.csuEwh.typeError, ERR_MISSLAPMSG,
		    msg );
      break;
    case GET_MISS_SEQGAP:	/* got a msg, but seq gap		*/
      sprintf( msg, "%s: Message received(i%u m%u t%u); sequence number gap",
	       argv[0], logoMsg.instid, logoMsg.mod, logoMsg.type );
      StatusReport( &csuNet, csuNet.csuEwh.typeError, ERR_MISSGAPMSG,
		    msg );
      break;
    case GET_MISS:	/* got a msg, but missed some			*/
      sprintf( msg, "%s: Message received(i%u m%u t%u); missed some",
	       argv[0], logoMsg.instid, logoMsg.mod, logoMsg.type );
      StatusReport( &csuNet, csuNet.csuEwh.typeError, ERR_MISSMSG, msg );
      break;
    case GET_TOOBIG:	/* next message was too big			*/
      sprintf( msg, "%s: Message too big\n",
	       argv[0] );
      StatusReport( &csuNet, csuNet.csuEwh.typeError, ERR_TOOBIG, msg );
      break;
    default:	/* Unknown result					*/
      sprintf( msg, "%s: Unknown tport_getmsg result", argv[0] );
      StatusReport( &csuNet, csuNet.csuEwh.typeError, ERR_UNKNOWN, msg );
      continue;
    }

    /*	NULL terminate the message					*/
    inBuf[sizeMsg] = '\0';
    if ( csuNet.csuParam.debug > 3 )
      logit( "t", "len: %d msg: %s\n", sizeMsg, inBuf );
    
    if ( csuNet.csuEwh.typeCarlStaTrig == logoMsg.type ) 
    {
      /*	Process the message					*/
      result = ProcessStationTrigger( &csuNet, inBuf );
      /*  ### What errors should make us quit?				*/
      /* Maybe none; press on, regardless; condition is already logged.	*/
    }
    else
      logit( "et", "%s: Unknown or unrequested message received!\n", argv[0] );

    /*	End of main processing loop					*/
  }  

  /*	Termination has been requested					*/
  csuNet.terminate = 1;

  if ( csuNet.csuParam.debug )
    logit( "t", "%s: Termination requested; cleaning up...", argv[0] );

  /*	Detach from shared memory regions				*/
  tport_detach( &regionIn ); 
  tport_detach( &(csuNet.regionOut) ); 

  /*	Flush the output stream						*/
  fflush( stdout );

  if ( csuNet.csuParam.debug )
  {
    logit( "o", "Done.\n" );
    logit( "t", "%s: Exit.\n", argv[0] );
  }
  /*	End of function main						*/

  return ( 0 );
}
