
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: cont_trig.c,v 1.0  2010/06/10 18:00:00  JMS Exp $
 *
 *    Revision history:
 *     $Log: cont_trig.c,v $
 *     Revision 1.0  2010/06/10 18:00:00  JMS
 *     Initial revision
 *
 *
 */

/*
 * cont_trig.c: modified Subnet part of Carl Johnson's Coincidence Trigger code 
 *		as an earthworm module which:
 *              1) reads a configuration file using kom.c routines 
 *                 (ReadConfig).
 *              2) looks up shared memory keys, installation ids, 
 *                 module ids, message types from earthworm.h tables 
 *                 using getutil.c functions (ReadEWH).
 *              3) attaches to one public shared memory region for
 *                 input and output using transport.c functions.
 *              4) processes hard-wired message types from configuration-
 *                 file-given installations & module ids
 *              5) produces TYPE_TRIGLIST2K messages.
 *              6) sends heartbeats and error messages back to the
 *                 shared memory region (StatusReport).
 *              7) writes to a log file using logit.c functions.
 */

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
/*			error code as defined in cont_trig.h on failure	*/

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
/*	Cont_Trig Includes						*/
/*******							*********/
#include "cont_trig.h"

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
//  char		inBuf[MAXLINELEN];	/* Pointer to the input message */
                                        /*   buffer.			*/
  unsigned int	tidSubnet;	/* id of Subnet thread			*/
  int		result;		/* Result from function calls.		*/
//  long		sizeMsg;	/* Size of retrieved message.		*/
//  MSG_LOGO	logoMsg;	/* Logo of retrieved message.		*/
  NETWORK	contNet;		/* The entire network's information.	*/
//  SHM_INFO	regionOut;	/* Input shared memory region info.	*/
  time_t	timeNow;	/* Current time.			*/
  time_t	timeLastBeat;	/* Time last heartbeat was sent.	*/

  /*	Check command line arguments					*/
  if ( 2 != argc )
  {
    fprintf( stderr, "Usage: %s <configfile>\n", argv[0] );
    exit( ERR_USAGE );
  }
           
  /*	Initialize parameter structures to defaults			*/
  result = InitializeParameters( &contNet );
  if ( CT_FAILED( result ) )
  {
    fprintf( stderr, "%s module initialization failed.\n", argv[0] );
    exit( ERR_INIT );
  }

  /*	Initialize message logging 					*/
  logit_init( argv[1], 0, MAXMESSAGELEN, 1 );

  /*	Read the configuration file					*/
  result = ReadConfig( argv[1], &contNet );
  if ( CT_FAILED( result ) )
  {
    fprintf( stderr, "%s error in ReadConfig: %d!\n", argv[0], result );
    exit( result );
  }

  /*	Look up important info from earthworm.h tables			*/
  result = ReadEWH( &contNet );
  if ( CT_FAILED( result ) )
  {
    fprintf( stderr, "%s error in ReadEWH: %d!\n", argv[0], result );
    exit( result );
  }
 
  /*	Log a startup message						*/
  if ( contNet.contParam.debug ) {
      logit( "t", "%s initialized with configuration file '%s'\n", argv[0],
	     argv[1] );
  }

  /*    Allocate some space for the output message */
  contNet.trigMsgBuf = calloc( 1, contNet.trigMsgBufLen );
  if ( contNet.trigMsgBuf == ( char* ) NULL )
  {
     logit( "e", 
            "cont_trig: Cannot allocate %d bytes for trigger message buffer\n",
             contNet.trigMsgBufLen );
     exit( ERR_INIT );
  }
  
  /*	Read the station file						*/
  result = ReadStations( &contNet );
  if ( CT_FAILED( result ) )
    exit( result );
  
  /* Start the subnet thread						*/
  SubnetThreadHeart = time( NULL );
  CreateSpecificMutex( &(contNet.stationMutex) );
  if ( StartThreadWithArg( SubnetThread, ( void *) &contNet, 
   			   ( unsigned ) THREAD_STACK, &tidSubnet ) == -1 )
  {
    logit( "e", "cont_trig: error starting subnet thread. Exitting.\n");
    exit( -1 );
  }
  
  /*	Attach to Input and Output shared memory rings			*/
  tport_attach( &(contNet.regionOut), contNet.contParam.ringKey );
  if ( contNet.contParam.debug )
    logit( "t", "%s: Attached to public memory region %s:%d for output.\n", 
	   argv[0], contNet.contParam.ring, contNet.contParam.ringKey );

  /*	Specify logos of outgoing messages				*/
  contNet.hrtLogo.instid = contNet.contEwh.myInstId;
  contNet.hrtLogo.mod    = contNet.contEwh.myModId;
  contNet.hrtLogo.type   = contNet.contEwh.typeHeartbeat;

  contNet.errLogo.instid = contNet.contEwh.myInstId;
  contNet.errLogo.mod    = contNet.contEwh.myModId;
  contNet.errLogo.type   = contNet.contEwh.typeError;

  contNet.trgLogo.instid = contNet.contEwh.myInstId;
  contNet.trgLogo.mod    = contNet.contEwh.myModId;
  contNet.trgLogo.type   = contNet.contEwh.typeTrigList;

  /*	Force a heartbeat to be issued in first pass thru main loop	*/
  timeLastBeat = time( &timeNow ) - contNet.contParam.heartbeatInt - 1;

  /*	Main message processing loop					*/
  while ( tport_getflag( &(contNet.regionOut) ) != TERMINATE  && 
          tport_getflag( &(contNet.regionOut) ) != contNet.MyPid  )
  {
    /*	Check for need to send heartbeat message			*/
    if ( time( &timeNow ) - timeLastBeat >= contNet.contParam.heartbeatInt )
    {
    /* Beat heart only if SubnetThread is still looping */
      if( timeNow - SubnetThreadHeart < contNet.contParam.heartbeatInt/2 )
      {
        StatusReport( &contNet, contNet.contEwh.typeHeartbeat, 0, "" );
        if ( contNet.contParam.debug > 3 )
          logit( "t", "%s: Sent Heartbeat\n", argv[0] );
      }
    /* SubnetThread is stuck; don't beat heart; say why */
      else  
      {
        sprintf( msg, "%s: Skipping Heartbeat; no activity in "
                      " SubnetThread for %ld sec", 
                 argv[0], (long)(timeNow - SubnetThreadHeart) );
        StatusReport( &contNet, contNet.contEwh.typeError, ERR_THRDSTUCK, msg );
      }
      timeLastBeat = timeNow;
    }
    sleep_ew( 500 );    
    /*	End of main processing loop					*/
  }  

  /*	Termination has been requested					*/
  contNet.terminate = 1;

  if ( contNet.contParam.debug )
    logit( "t", "%s: Termination requested; cleaning up...", argv[0] );

  /*	Detach from shared memory regions				*/
  tport_detach( &(contNet.regionOut) ); 

  /*	Flush the output stream						*/
  fflush( stdout );

  if ( contNet.contParam.debug )
  {
    logit( "o", "Done.\n" );
    logit( "t", "%s: Exit.\n", argv[0] );
  }
  /*	End of function main						*/

  return ( 0 );
}
