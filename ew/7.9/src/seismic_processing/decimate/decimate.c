
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: decimate.c 6325 2015-05-01 00:44:09Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2007/02/26 19:42:20  paulf
 *     yet another time_t fix
 *
 *     Revision 1.5  2004/05/11 18:14:17  dietz
 *     Modified to work with either TYPE_TRACEBUF2 or TYPE_TRACEBUF msgs
 *
 *     Revision 1.4  2002/10/25 19:47:48  dietz
 *     added #include <time.h>
 *
 *     Revision 1.3  2002/10/25 17:59:44  dietz
 *     Added support for multiple GetWavesFrom commands
 *
 *     Revision 1.2  2001/05/09 18:32:05  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or Dcm.MyPid.
 *
 *     Revision 1.1  2000/02/14 16:56:25  lucky
 *     Initial revision
 *
 *
 */

/*
 * decimate.c:  
 *
 *  Implements filter and decimate routines supplied by Dave Harris
 *  to process wave data (TYPE_TRACEBUF) coming from the InRing defined 
 *  in decimate.d. Decimated trace messages are written to the OutRing 
 *  with their SCNLs changed according to the rules defined in decimate.d
 * 
 *  Initial version:
 *  Lucky Vidmar (lucky@Colorado.EDU) - Tue Jul 28 15:48:29 MDT 1998
 *
 *  Wed Nov 11 16:43:14 MST 1998 lucky 
 *   Part of Y2K compliance project:
 *    1) name of the config file passed to logit_init()
 *    2) incomming transport ring flushed at startup 
 *    3) Process ID sent on with the heartbeat msgs for restart
 *       purposes.      
 *      
 *   Made decimate multithreaded in the finest tradition of earthworm
 *
 *  Major rewrite: Pete Lombard, 18 October, 1999
 *    Filtering and decimation is now done in stages as configured.
 *    The filter coefficients are determined by the decimation rate.
 *    Since several buffers must be allocated for each station, each
 *      SCNL must be listed in the config file; no wildcards are allowed.
 *    This made Lucky's fine rewrite rules unnecessary, sad to say.
 *      
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <earthworm.h>
#include <kom.h>
#include <swap.h>
#include "decimate.h"

#define PROGRAM_VERSION "1.8.0 2015-04-30"

int main (int argc, char **argv)
{
  WORLD     Dcm;                   /* Our main data structure              */
  time_t    timeNow;               /* current time                         */ 
  time_t    timeLastBeat;          /* time last heartbeat was sent         */
  long      sizeMsg;               /* size of retrieved message            */
  SHM_INFO  regionIn;              /* Input shared memory region info.     */
  MSG_LOGO  logoWave[MAX_LOGO];    /* Logo(s) of requested waveforms.      */
  int       ilogo;                 /* working counter                      */
  MSG_LOGO  logoMsg;               /* logo of retrieved message            */
  unsigned  tidDecimator;          /* Decimator thread id                  */
  char     *inBuf;                 /* Pointer to the input message buffer. */
  int       inBufLen;              /* Size of input message buffer         */
  TracePacket  *TracePkt;
  int       ret;
  char  msgText[MAXMESSAGELEN];    /* string for log/error messages    */

  /* Check command line arguments 
   ******************************/
  if (argc != 2)
  {
    fprintf (stderr, "Usage: decimate <configfile>\n");
    exit (EW_FAILURE);
  }

  /* Read config file and configure the decimator */
  if (Configure(&Dcm, argv, PROGRAM_VERSION) != EW_SUCCESS)
  {
    logit("e", "decimate: configure() failed \n");
    exit (EW_FAILURE);
  }

  if ( Dcm.dcmParam.testMode )
  {
    logit("e", "Decimator terminating normally for test mode\n");
    exit (EW_SUCCESS);
  }
  
  /* We will put the station index in front of the trace message, so we  *
   * don't have to look up the SCNL again at the other end of the queue. */
  inBufLen = MAX_TRACEBUF_SIZ + sizeof( int );
  if ( ! ( inBuf = (char *) malloc( (size_t) inBufLen ) ) )
  {
    logit( "e", "%s: Memory allocation failed - initial message buffer!\n",
        argv[0] );
    exit( EW_FAILURE );
  }
  TracePkt = (TracePacket *) (inBuf + sizeof(int));
  
  /* Attach to Input shared memory ring 
   ************************************/
  tport_attach (&regionIn, Dcm.dcmEWH.ringInKey);
  if (Dcm.dcmParam.debug) {
    logit ("", "Attached to public memory region %s: %d\n", 
           Dcm.dcmParam.ringIn, Dcm.dcmEWH.ringInKey);
  }

  /* Attach to Output shared memory ring 
   *************************************/
  if (Dcm.dcmEWH.ringOutKey == Dcm.dcmEWH.ringInKey) {
    Dcm.regionOut = regionIn;
  } else {
    tport_attach (&(Dcm.regionOut), Dcm.dcmEWH.ringOutKey);
    if (Dcm.dcmParam.debug)
      logit ("", "Attached to public memory region %s: %d\n", 
             Dcm.dcmParam.ringOut, Dcm.dcmEWH.ringOutKey);
  }

 /* Specify logos of incoming waveforms 
  *************************************/
  for( ilogo=0; ilogo<Dcm.dcmParam.nlogo; ilogo++ ) {
    logoWave[ilogo].instid = Dcm.dcmEWH.readInstId[ilogo];
    logoWave[ilogo].mod    = Dcm.dcmEWH.readModId[ilogo];
    logoWave[ilogo].type   = Dcm.dcmEWH.readMsgType[ilogo];
  }

  /* Specify logos of outgoing messages 
   ************************************/
  Dcm.hrtLogo.instid = Dcm.dcmEWH.myInstId;
  Dcm.hrtLogo.mod    = Dcm.dcmEWH.myModId;
  Dcm.hrtLogo.type   = Dcm.dcmEWH.typeHeartbeat;

  Dcm.errLogo.instid = Dcm.dcmEWH.myInstId;
  Dcm.errLogo.mod    = Dcm.dcmEWH.myModId;
  Dcm.errLogo.type   = Dcm.dcmEWH.typeError;

  Dcm.trcLogo.instid = Dcm.dcmEWH.myInstId;
  Dcm.trcLogo.mod    = Dcm.dcmEWH.myModId;
  Dcm.trcLogo.type   = Dcm.dcmEWH.typeTrace2;

  /* Force a heartbeat to be issued in first pass thru main loop  */
  timeLastBeat = time (&timeNow) - Dcm.dcmParam.heartbeatInt - 1;

  /* Flush the incoming transport ring */
  while (tport_getmsg (&regionIn, logoWave, (short)Dcm.dcmParam.nlogo, &logoMsg,
                       &sizeMsg, inBuf, inBufLen) != GET_NONE);

  /* Create MsgQueue mutex */
  CreateMutex_ew();

  /* Allocate the message Queue */
  initqueue (&(Dcm.MsgQueue), QUEUE_SIZE, inBufLen);
  

  /* Start decimator thread which will read messages from   *
   * the Queue, decimate them and write them to the OutRing */
  if (StartThreadWithArg (DecimateThread, (void *) &Dcm, (unsigned) 
                          THREAD_STACK, &tidDecimator) == -1)
  {
    logit( "e", 
           "decimate: Error starting Decimator thread.  Exiting.\n");
    tport_detach (&regionIn);
    
    if (Dcm.dcmEWH.ringOutKey != Dcm.dcmEWH.ringInKey) {
       tport_detach (&(Dcm.regionOut)); 
    }
    free( inBuf );
    free( Dcm.stations );
    exit( EW_FAILURE );
  }

  Dcm.DecimatorStatus = 0; /*assume the best*/


/*--------------------- setup done; start main loop -------------------------*/

  while (tport_getflag (&regionIn) != TERMINATE  &&
         tport_getflag (&regionIn) != Dcm.MyPid )
  {
    /* send decimate' heartbeat */
    if (time (&timeNow) - timeLastBeat >= Dcm.dcmParam.heartbeatInt) 
    {
      timeLastBeat = timeNow;
      StatusReport (&Dcm, Dcm.dcmEWH.typeHeartbeat, 0, ""); 
    }

    if (Dcm.DecimatorStatus < 0)
    {
      logit ("et", 
             "decimate: Decimator thread died. Exiting\n");
      exit (EW_FAILURE);
    }

    ret = tport_getmsg (&regionIn, logoWave, (short)Dcm.dcmParam.nlogo, 
                        &logoMsg, &sizeMsg, TracePkt->msg, MAX_TRACEBUF_SIZ);

    /* Check return code; report errors */
    if (ret != GET_OK)
    {
      if (ret == GET_TOOBIG)
      {
        sprintf (msgText, "msg[%ld] i%d m%d t%d too long for target",
                 sizeMsg, (int) logoMsg.instid,
                 (int) logoMsg.mod, (int)logoMsg.type);
        StatusReport (&Dcm, Dcm.dcmEWH.typeError, ERR_TOOBIG, msgText);
        continue;
      }
      else if (ret == GET_MISS)
      {
        sprintf (msgText, "missed msg(s) i%d m%d t%d in %s",
                 (int) logoMsg.instid, (int) logoMsg.mod, 
                 (int)logoMsg.type, Dcm.dcmParam.ringIn);
        StatusReport (&Dcm, Dcm.dcmEWH.typeError, ERR_MISSMSG, msgText);
      }
      else if (ret == GET_NOTRACK)
      {
        sprintf (msgText, "no tracking for logo i%d m%d t%d in %s",
                 (int) logoMsg.instid, (int) logoMsg.mod, 
                 (int)logoMsg.type, Dcm.dcmParam.ringIn);
        StatusReport (&Dcm, Dcm.dcmEWH.typeError, ERR_NOTRACK, msgText);
      }
      else if (ret == GET_NONE)
      {
        sleep_ew(500);
        continue;
      }

    }

    /* Check to see if msg's SCNL code is desired. Note that we don't need *
     * to do byte-swapping before we can read the SCNL.                    */

    if ((ret = matchSCNL( TracePkt, logoMsg.type, &Dcm )) < -1 )
    {
      logit ("et", "Decimate: Call to matchSCNL failed; exiting.\n");
      exit (EW_FAILURE);
    }
    else if ( ret == -1 )
      /* Not an SCNL we want */
      continue;
    
    /* stick the SCNL number as an int at the front of the message */
    *((int*)inBuf) = ret; 

    /* If necessary, swap bytes in the wave message */
    if( logoMsg.type == Dcm.dcmEWH.typeTrace2 )
        ret = WaveMsg2MakeLocal( &(TracePkt->trh2) );
    else if( logoMsg.type == Dcm.dcmEWH.typeTrace )  
        ret = WaveMsgMakeLocal( &(TracePkt->trh) );

    if(ret < 0)
    {
      logit ("et", "Decimate: Unknown datatype.\n");
      continue;
    }

    /* Queue retrieved msg */
    RequestMutex ();
    ret = enqueue (&(Dcm.MsgQueue), inBuf, sizeMsg + sizeof(int), logoMsg); 
    ReleaseMutex_ew ();

    if (ret != 0)
    {
      if (ret == -1)
      {
        sprintf (msgText, 
                 "Message too large for queue; Lost message.");
        StatusReport (&Dcm, Dcm.dcmEWH.typeError, ERR_QUEUE, msgText);
        continue;
      }
      if (ret == -3) 
      {
        sprintf (msgText, "Queue full. Old messages lost.");
        StatusReport (&Dcm, Dcm.dcmEWH.typeError, ERR_QUEUE, msgText);
        continue;
      }
    } /* problem from enqueue */

  } /* wait until TERMINATE is raised  */  

  /* Termination has been requested */
  tport_detach (&regionIn);
  if (Dcm.dcmEWH.ringOutKey != Dcm.dcmEWH.ringInKey) {
    tport_detach (&(Dcm.regionOut));
  }
  free( inBuf );
  free( Dcm.stations );
  logit ("t", "Termination requested; exiting!\n" );
  exit (EW_SUCCESS);

}

