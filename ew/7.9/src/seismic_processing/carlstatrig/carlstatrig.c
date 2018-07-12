
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: carlstatrig.c 6351 2015-05-13 00:30:04Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.7  2005/04/12 22:43:07  dietz
 *     Added optional command "GetWavesFrom <instid> <module_id>"
 *
 *     Revision 1.6  2004/06/02 22:37:06  dietz
 *     changed main() to int main()
 *
 *     Revision 1.5  2004/05/05 23:54:04  lombard
 *     Added location code: reads TYPE_TRACEBUF2 messages,
 *     writes TYPE_CARLSTATRIG_SCNL messages.
 *     Removed OS2 support.
 *
 *     Revision 1.4  2002/05/16 14:56:02  patton
 *     Made logit changes.
 *
 *     Revision 1.3  2001/05/09 18:16:52  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or cstWorld.MyPid.
 *
 *     Revision 1.2  2001/01/17 18:35:36  dietz
 *     minor logging changes
 *
 *     Revision 1.1  2000/02/14 16:12:07  lucky
 *     Initial revision
 *
 *
 */

/*
 * carlstatrig.c: The station trigger part of Carl Johnson's Coincidence 
 *              Trigger code as an earthworm module which:
 *              1) reads a configuration file using kom.c routines 
 *                 (ReadConfig).
 *              2) looks up shared memory keys, installation ids, 
 *                 module ids, message types from earthworm.h tables 
 *                 using getutil.c functions (ReadEWH).
 *              3) attaches to two public shared memory regions for
 *                 input and output using transport.c functions.
 *              4) processes hard-wired message types from configuration-
 *                 file-given installations & module ids (This source
 *                 code expects to process TYPE_TRACEBUF2 messages).
 *              5) produces TYPE_CARLSTATRIG messages.
 *              6) sends heartbeats and error messages back to the
 *                 shared memory region (StatusReport).
 *              7) writes to a log file using logit.c functions.
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: main                                                  */
/*                                                                      */
/*      Inputs:         Configuration File Name                         */
/*                                                                      */
/*      Outputs:        Log Messages                                    */
/*                                                                      */
/*      Returns:        0 on success                                    */
/*                      error code as defined in carlstatrig.h on failure */

/* version for logging and usage printing introduce in V2.0.1, not sent as part of msg */
/* 2.0.2 2015-04-26 - fixed allowing blank or nearly blank lines in station file */
/* 2.0.3 2015-05-12 - prevent divide by zero in UpdateStation */
#define VERSION "2.0.3 2015-05-12"
/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <stdio.h>
#include <stdlib.h>     /* exit, free, malloc, realloc                  */
#include <sys/types.h>  /* time                                         */
#include <time.h>       /* time                                         */

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>  /* logit                                        */

/*******                                                        *********/
/*      CarlStaTrig Includes                                            */
/*******                                                        *********/
#include "carlstatrig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: main                                                  */
int main( int argc, char* argv[] )
{
  char          msg[MAXMESSAGELEN];     /* Status or error text.        */
  char*         inBuf;          /* Pointer to the input message buffer. */
  CSTEWH        cstEwh;         /* Earthworm.[hd] configuration params. */
  int           result;         /* Result from function calls.          */
  long          inBufLen;       /* Maximum message size in bytes.       */
  long          sizeMsg;        /* Size of retrieved message.           */
  MSG_LOGO      logoMsg;        /* Logo of retrieved message.           */
  WORLD         cstWorld;       /* The entire network's information.    */
  SHM_INFO      regionIn;       /* Input shared memory region info.     */
  time_t        timeNow;        /* Current time.                        */
  time_t        timeLastBeat;   /* Time last heartbeat was sent.        */
  int           i;

  /*    Check command line arguments                                    */
  if ( 2 != argc )
  {
    fprintf( stderr, "Usage: %s <configfile>\n", argv[0] );
    fprintf( stderr, "Version: %s \n", VERSION );
    exit( ERR_USAGE );
  }
           
  /*    Initialize parameter structures to defaults                     */
  result = InitializeParameters( &cstEwh, &cstWorld );
  if ( CT_FAILED( result ) )
  {
    fprintf( stderr, "%s module initialization failed.\n", argv[0] );
    exit( ERR_INIT );
  }

  /*    Initialize pointer variables                                    */
  inBuf = NULL;

  /*    Initialize message logging                                      */
  logit_init( argv[1], 0, MAXMESSAGELEN, 1 );

  /*    Read the configuration file                                     */
  result = ReadConfig( argv[1], &cstWorld );
  if ( CT_FAILED( result ) )
  {
    logit( "e", "%s error in ReadConfig: %d!\n", argv[0], result );
    exit( result );
  }

  /*    Look up important info from earthworm.h tables                  */
  result = ReadEWH( &(cstWorld.cstParam), &cstEwh );
  if ( CT_FAILED( result ) )
  {
    logit( "e", "%s error in ReadEWH: %d!\n", argv[0], result );
    exit( result );
  }
 
   /*    Log a startup message                                           */
  if ( cstWorld.cstParam.debug ) {
      logit( "t", "%s initialized with configuration file '%s'\n", argv[0], argv[1]);
      logit( "t", "%s Version: '%s'\n", argv[0], VERSION );
      logit("", "producing %s TYPE_CARLSTATRIG_SCNL messages.\n",
	    CST_VERSION);
  }

  /*    Read the station file                                           */
  result = ReadStations( &cstWorld );
  if ( CT_FAILED( result ) )
    exit( result );

  /*    Allocate the message input buffer - use a default size to start */
  inBufLen = ( sizeof( long ) * ( 100 + cstWorld.maxGap - 1 ) ) + 64;
        /*sizeof( TRACE2_HEADER );*/
  if ( ! ( inBuf = (char *) malloc( (size_t) inBufLen ) ) )
  {
    logit( "et", "%s: Memory allocation failed - initial message buffer!\n",
        argv[0] );
    exit( ERR_MALLOC );
  }

  /*    Attach to Input and Output shared memory rings                  */
  tport_attach( &regionIn, cstWorld.cstParam.ringInKey );
  if ( cstWorld.cstParam.debug )
    logit( "t", "%s: Attached to public memory region %s:%d for input.\n", 
        argv[0], cstWorld.cstParam.ringIn, cstWorld.cstParam.ringInKey );

  tport_attach( &(cstWorld.regionOut), cstWorld.cstParam.ringOutKey );
  if ( cstWorld.cstParam.debug )
    logit( "t", "%s: Attached to public memory region %s:%d for output.\n", 
        argv[0], cstWorld.cstParam.ringOut, cstWorld.cstParam.ringOutKey );

  /*    Specify logos of incoming waveforms (if not in config file)     */
  if( cstWorld.cstParam.nGetLogo == 0 )
  {
    cstWorld.cstParam.nGetLogo = 1;
    cstWorld.cstParam.GetLogo  = (MSG_LOGO *) calloc( cstWorld.cstParam.nGetLogo, 
                                                      sizeof(MSG_LOGO) );
    if( cstWorld.cstParam.GetLogo == NULL ) {
      logit( "e", "%s: Error allocating space for GetLogo. Exiting\n",
             argv[0] );
      exit( ERR_MALLOC );
    }
    cstWorld.cstParam.GetLogo[0].instid = cstEwh.instWild;
    cstWorld.cstParam.GetLogo[0].mod    = cstEwh.modWild;
    cstWorld.cstParam.GetLogo[0].type   = cstEwh.typeWaveform;
  }
  else {
    for( i=0; i<cstWorld.cstParam.nGetLogo; i++ ) 
      cstWorld.cstParam.GetLogo[i].type = cstEwh.typeWaveform;
  }
 
  /*    Specify logos of outgoing messages                              */
  cstWorld.outLogo.instid = cstEwh.myInstId;
  cstWorld.outLogo.mod    = cstEwh.myModId;

  if ( cstWorld.cstParam.debug )
  {
    for( i=0; i<cstWorld.cstParam.nGetLogo; i++ ) { 
      logit( "t", "%s: Reading messages (i%u m%u t%u)\n", argv[0],
             cstWorld.cstParam.GetLogo[i].instid,
             cstWorld.cstParam.GetLogo[i].mod, 
             cstWorld.cstParam.GetLogo[i].type );
    }
    logit( "", "heartbeatint: %d\n", cstWorld.cstParam.heartbeatInt );
  }
  
  /*    Flush the input ring                                            */
  if ( cstWorld.cstParam.debug > 2 )
    logit( "t", "%s: Flushing input buffer...", argv[0] );
  while ( tport_getmsg( &regionIn, cstWorld.cstParam.GetLogo,
                        cstWorld.cstParam.nGetLogo, &logoMsg, &sizeMsg,
                        inBuf, inBufLen ) != GET_NONE );
  if ( cstWorld.cstParam.debug > 2 )
    logit( "t", "Done\n", argv[0] );

  /*    Force a heartbeat to be issued in first pass thru main loop     */
  timeLastBeat = time( &timeNow ) - cstWorld.cstParam.heartbeatInt - 1;

  /*    Main message processing loop                                    */
  while ( tport_getflag( &regionIn ) != TERMINATE  &&
          tport_getflag( &regionIn ) != cstWorld.MyPid )
  {
    /*  Check for need to send heartbeat message                        */
    if ( time( &timeNow ) - timeLastBeat >= cstWorld.cstParam.heartbeatInt )
    {
      timeLastBeat = timeNow;
      StatusReport( &cstWorld, cstEwh.typeHeartbeat, 0, "" ); 
      if ( cstWorld.cstParam.debug > 4 )
        logit( "t", "%s: Sent Heartbeat\n", argv[0] );
    }

    /*  Get next message                                                */
    result = tport_getmsg( &regionIn, cstWorld.cstParam.GetLogo,
                           cstWorld.cstParam.nGetLogo, 
                           &logoMsg, &sizeMsg, inBuf,inBufLen - 1 );

    /*  Acst on the return code from the transport function             */
    switch ( result )
    {
      case GET_OK:      /* got a message, no errors or warnings         */
        if ( cstWorld.cstParam.debug > 5 )
          logit( "t", "%s: Message received(i%u m%u t%u).\n", argv[0],
                logoMsg.instid, logoMsg.mod, logoMsg.type );
        break;
      case GET_NONE:    /* no messages of interest, check again later   */
        /* if ( cstWorld.cstParam.debug > 2 )
         *          logit( "t", "%s: No messages, sleeping.\n", argv[0] );
         */
        sleep_ew( 500 );        /* milliseconds                         */
        continue;
      case GET_NOTRACK: /* got a msg, but can't tell if any were missed */
        sprintf( msg, "%s: Message received(i%u m%u t%u); NTRACK_GET exceeded",
                argv[0], logoMsg.instid, logoMsg.mod, logoMsg.type );
        StatusReport( &cstWorld, cstEwh.typeError, ERR_NOTRACK, msg );
        break;
      case GET_MISS_LAPPED:     /* got a msg, but also missed lots      */
        sprintf( msg, "%s: Message received(i%u m%u t%u); missed lots",
                argv[0], logoMsg.instid, logoMsg.mod, logoMsg.type );
        StatusReport( &cstWorld, cstEwh.typeError, ERR_MISSLAPMSG,
                msg );
        break;
      case GET_MISS_SEQGAP:     /* got a msg, but seq gap               */
        sprintf( msg, "%s: Message received(i%u m%u t%u); sequence number gap",
                argv[0], logoMsg.instid, logoMsg.mod, logoMsg.type );
        StatusReport( &cstWorld, cstEwh.typeError, ERR_MISSGAPMSG,
                msg );
        break;
      case GET_MISS:    /* got a msg, but missed some                   */
        sprintf( msg, "%s: Message received(i%u m%u t%u); missed some",
                argv[0], logoMsg.instid, logoMsg.mod, logoMsg.type );
        StatusReport( &cstWorld, cstEwh.typeError, ERR_MISSMSG, msg );
        break;
      case GET_TOOBIG:  /* next message was too big, resize buffer      */
        if ( cstWorld.cstParam.debug )
          logit( "t",
                "%s: Resizing input buffer to hold message(i%u m%u t%u)\n",
                argv[0], logoMsg.instid, logoMsg.mod, logoMsg.type );
        if ( ! ( inBuf = (char *) realloc ( inBuf, sizeMsg + 1 ) ) )
        {
          sprintf( msg, "%s: Memory allocation failed - resize message buffer",
                argv[0] );
          StatusReport( &cstWorld, cstEwh.typeError, ERR_MALLOC, msg );
          exit( ERR_MALLOC );
        }
        /*      Successful reallocation of memory                       */
        inBufLen = sizeMsg + 1;
        continue;
      default:  /* Unknown result                                       */
        sprintf( msg, "%s: Unknown tport_getmsg result", argv[0] );
        StatusReport( &cstWorld, cstEwh.typeError, ERR_UNKNOWN, msg );
        continue;
    }

    /*  NULL terminate the message                                      */
    inBuf[sizeMsg] = '\0';

    if ( cstEwh.typeWaveform == logoMsg.type ) 
    {
      /*        Process the message                                     */
      result = ProcessTraceMsg( &cstWorld, inBuf );
      if ( CT_FAILED( result ) )
      {
        logit("et", "%s: exiting! ProcessTraceMsg failed, error code: %d\n", 
               argv[0], result);
        exit( result );
      }
    }
    else
      logit( "et", "%s: Unknown or unrequested message received!\n", argv[0] );

  /*    End of main processing loop                                     */
  }  

  /*    Termination has been requested                                  */
  if ( cstWorld.cstParam.debug )
    logit( "t", "%s: Termination requested; cleaning up...", argv[0] );

  /*    Detach from shared memory regions                               */
  tport_detach( &regionIn ); 
  tport_detach( &(cstWorld.regionOut) ); 

  /*    Free memory used                                                */
  free( cstWorld.cstParam.GetLogo );
  free( cstWorld.stations );
  free( inBuf );
  

  /*    Flush the output stream                                         */
  fflush( stdout );

  if ( cstWorld.cstParam.debug )
  {
    logit( "o", "carlStaTrig done.\n" );
    logit( "t", "%s: Exit.\n", argv[0] );
  }
/*      End of function main                                            */

  return ( 0 );
}
