
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: fir.c 4013 2010-08-18 20:59:05Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2008/02/12 18:47:36  withers
 *     moved nsamp test to after byte swap
 *
 *     Revision 1.5  2007/02/26 19:52:50  paulf
 *     fixed a ton of windows warnings related to time_t again
 *
 *     Revision 1.4  2007/02/14 19:57:19  luetgert
 *     Fixed problem when encountering a zero-length trcebuf.
 *     .
 *
 *     Revision 1.3  2004/07/28 22:43:05  lombard
 *     Modified to handle SCNLs and TYPE_TRACEBUF2 (only!) messages.
 *
 *     Revision 1.2  2001/05/09 22:31:16  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or Fir.MyPid.
 *
 *     Revision 1.1  2000/02/14 17:27:23  lucky
 *     Initial revision
 *
 *
 */

/*
 * fir.c:  
 *
 */

#include <earthworm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <kom.h>
#include <swap.h>
#include "fir.h"

main (int argc, char **argv)
{
  WORLD Fir;                       /* Our main data structure       */
  time_t timeNow;                  /* current time                  */ 
  time_t timeLastBeat;             /* time last heartbeat was sent  */
  long  sizeMsg;                   /* size of retrieved message     */
  SHM_INFO  regionIn;              /* Input shared memory region info.     */
  MSG_LOGO  logoWave;              /* Logo of requested waveforms.  */
  MSG_LOGO  logoMsg;               /* logo of retrieved message     */
  unsigned  tidFir;                /* Filter thread id              */
  char *inBuf;                     /* Pointer to the input message buffer. */
  int inBufLen;                    /* Size of input message buffer  */
  TRACE2_HEADER  *WaveHeader;
  int   ret;
  char  msgText[MAXMESSAGELEN];    /* string for log/error messages    */

  /* Check command line arguments 
         ******************************/
  if (argc != 2)
  {
    fprintf (stderr, "Usage: fir <configfile>\n");
    exit (EW_FAILURE);
  }

  /* Read config file and configure the decimator */
  if (Configure(&Fir, argv) != EW_SUCCESS)
  {
    fprintf (stderr, "fir: configure() failed \n");
    exit (EW_FAILURE);
  }

  if ( Fir.firParam.testMode )
  {
    logit("e", "Fir terminating normally for test mode; see log for results\n");
    exit (EW_SUCCESS);
  }
  
  /* We will put the station index ine front of the trace message, so we   *
   * don't have to look up the SCNL again at the other end of the queue. */
  inBufLen = MAX_TRACEBUF_SIZ + sizeof( int );
  if ( ! ( inBuf = (char *) malloc( (size_t) inBufLen ) ) )
  {
    logit( "e", "%s: Memory allocation failed - initial message buffer!\n",
        argv[0] );
    exit( EW_FAILURE );
  }
  WaveHeader = (TRACE2_HEADER *) (inBuf + sizeof(int));
  
  /* Attach to Input shared memory ring 
         *******************************************/
  tport_attach (&regionIn, Fir.firEWH.ringInKey);
  if (Fir.firParam.debug)
    logit ("", "Attached to public memory region %s: %d\n", 
           Fir.firParam.ringIn, Fir.firEWH.ringInKey);

  /* Attach to Output shared memory ring 
         *******************************************/
  if (Fir.firEWH.ringOutKey == Fir.firEWH.ringInKey)
    Fir.regionOut = regionIn;
  else
    tport_attach (&(Fir.regionOut), Fir.firEWH.ringOutKey);

  if (Fir.firParam.debug)
    logit ("", "Attached to public memory region %s: %d\n", 
         Fir.firParam.ringOut, Fir.firEWH.ringOutKey);

 /*    Specify logos of incoming waveforms                             */
  logoWave.instid = Fir.firEWH.readInstId;
  logoWave.mod    = Fir.firEWH.readModId;
  logoWave.type   = Fir.firEWH.typeWaveform;

  /*    Specify logos of outgoing messages                              */
  Fir.hrtLogo.instid = Fir.firEWH.myInstId;
  Fir.hrtLogo.mod    = Fir.firEWH.myModId;
  Fir.hrtLogo.type   = Fir.firEWH.typeHeartbeat;

  Fir.errLogo.instid = Fir.firEWH.myInstId;
  Fir.errLogo.mod    = Fir.firEWH.myModId;
  Fir.errLogo.type   = Fir.firEWH.typeError;

  Fir.trcLogo.instid = Fir.firEWH.myInstId;
  Fir.trcLogo.mod    = Fir.firEWH.myModId;
  Fir.trcLogo.type   = Fir.firEWH.typeWaveform;

  /* Force a heartbeat to be issued in first pass thru main loop  */
  timeLastBeat = time (&timeNow) - Fir.firParam.heartbeatInt - 1;


  /* Flush the incoming transport ring */
  while (tport_getmsg (&regionIn, &logoWave, 1, &logoMsg,
                       &sizeMsg, inBuf, inBufLen) != GET_NONE);

    /* Create MsgQueue mutex */
  CreateMutex_ew();

  /* Allocate the message Queue */
  initqueue (&(Fir.MsgQueue), Fir.firParam.QueueSize, inBufLen);
  

  /* Start decimator thread which will read messages from   *
   * the Queue, fir them and write them to the OutRing */
  if (StartThreadWithArg (FirThread, (void *) &Fir, (unsigned) THREAD_STACK, 
                          &tidFir) == -1)
  {
    logit( "e", 
           "fir: Error starting Fir thread.  Exitting.\n");
    tport_detach (&regionIn);
    tport_detach (&(Fir.regionOut));
    exit (EW_FAILURE);
  }

  Fir.FirStatus = 0; /*assume the best*/


/*--------------------- setup done; start main loop -------------------------*/

  while (tport_getflag (&regionIn) != TERMINATE  &&
         tport_getflag (&regionIn) != Fir.MyPid )
  {
    /* send fir' heartbeat */
    if (time (&timeNow) - timeLastBeat >= Fir.firParam.heartbeatInt) 
    {
      timeLastBeat = timeNow;
      StatusReport (&Fir, Fir.firEWH.typeHeartbeat, 0, ""); 
    }

    if (Fir.FirStatus < 0)
    {
      logit ("et", 
             "fir: Filter thread died. Exitting\n");
      exit (EW_FAILURE);
    }

    ret = tport_getmsg (&regionIn, &logoWave, 1, &logoMsg,
                        &sizeMsg, inBuf + sizeof(int), inBufLen - sizeof(int));

    /* Check return code; report errors */
    if (ret != GET_OK)
    {
      if (ret == GET_TOOBIG)
      {
        sprintf (msgText, "msg[%ld] i%d m%d t%d too long for target",
                 sizeMsg, (int) logoMsg.instid,
                 (int) logoMsg.mod, (int)logoMsg.type);
        StatusReport (&Fir, Fir.firEWH.typeError, ERR_TOOBIG, msgText);
        continue;
      }
      else if (ret == GET_MISS)
      {
        sprintf (msgText, "missed msg(s) i%d m%d t%d in %s",
                 (int) logoMsg.instid, (int) logoMsg.mod, 
                 (int)logoMsg.type, Fir.firParam.ringIn);
        StatusReport (&Fir, Fir.firEWH.typeError, ERR_MISSMSG, msgText);
      }
      else if (ret == GET_NOTRACK)
      {
        sprintf (msgText, "no tracking for logo i%d m%d t%d in %s",
                 (int) logoMsg.instid, (int) logoMsg.mod, 
                 (int)logoMsg.type, Fir.firParam.ringIn);
        StatusReport (&Fir, Fir.firEWH.typeError, ERR_NOTRACK, msgText);
      }
      else if (ret == GET_NONE)
      {
        sleep_ew(Fir.firParam.SleepMilliSeconds);
        continue;
      }

    }

    /* Check to see if msg's SCNL code is desired. Note that we don't need *
     * to do byte-swapping before we can read the SCNL.                    */
    if ((ret = matchSCNL(WaveHeader, &Fir )) < -1 )
    {
      logit ("et", "Fir: Call to matchSCNL failed; exitting.\n");
      exit (EW_FAILURE);
    }
    else if ( ret == -1 )
      /* Not an SCNL we want */
      continue;
       
    /* stick the SCNL number as an int at the front of the message */
    *((int*)inBuf) = ret; 

    /* If necessary, swap bytes in the wave message */
    if (WaveMsg2MakeLocal (WaveHeader) < 0)
    {
      logit ("et", "Fir: Unknown waveform type.\n");
      continue;
    }
    if(WaveHeader->nsamp <= 0) {
       logit ("et", "Fir:  Zero length tracebuf. %s_%s_%s_%s\n",  
              WaveHeader->sta, WaveHeader->chan, WaveHeader->net, WaveHeader->loc);
       continue;
    }
    /* Queue retrieved msg */
    RequestMutex ();
    ret = enqueue (&(Fir.MsgQueue), inBuf, sizeMsg + sizeof(int), logoMsg); 
    ReleaseMutex_ew ();

    if (ret != 0)
    {
      if (ret == -1)
      {
        sprintf (msgText, 
                 "Message too large for queue; Lost message.");
        StatusReport (&Fir, Fir.firEWH.typeError, ERR_QUEUE, msgText);
        continue;
      }
      if (ret == -3) 
      {
        sprintf (msgText, "Queue full. Old messages lost.");
        StatusReport (&Fir, Fir.firEWH.typeError, ERR_QUEUE, msgText);
        continue;
      }
    } /* problem from enqueue */

  } /* wait until TERMINATE is raised  */  

  /* Termination has been requested */
  tport_detach (&regionIn);
  tport_detach (&(Fir.regionOut));
  logit ("t", "Termination requested; exitting!\n" );
  exit (EW_SUCCESS);

}

