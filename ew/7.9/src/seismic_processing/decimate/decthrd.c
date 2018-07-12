
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: decthrd.c 5773 2013-08-09 15:04:21Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2005/07/20 15:28:00  friberg
 *     added a #ifndef for Linux to not include synch.h
 *
 *     Revision 1.3  2004/05/11 18:14:17  dietz
 *     Modified to work with either TYPE_TRACEBUF2 or TYPE_TRACEBUF msgs
 *
 *     Revision 1.2  2002/10/25 17:59:44  dietz
 *     fixed spelling mistakes.
 *
 *     Revision 1.1  2000/02/14 16:56:25  lucky
 *     Initial revision
 *
 *
 */

/*
 * decthrd.c: Decimator thread
 *              1) Allocates memory for input TRACE message
 *              2) Retrieves TRACE messages from their queues
 *              3) Determines station index from the queue message
 *              4) Dispatches messages to FilterDecimator
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: DecimateThread                                        */
/*                                                                      */
/*      Inputs:         Pointer to World Structure                      */
/*                                                                      */
/*      Outputs:        Message sent to the output ring                 */
/*                                                                      */
/*      Returns:        nothing                                         */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#ifdef _WINNT
#include <windows.h>
#define mutex_t HANDLE
#else
#ifdef _SOLARIS
#ifndef _LINUX
#include <synch.h>      /* mutex's                                      */
#endif
#endif
#endif

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>  /* logit, threads                               */

/*******                                                        *********/
/*      Decimate Includes                                               */
/*******                                                        *********/
#include "decimate.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: DecimateThread                                          */
thr_ret DecimateThread (void* dcm)
{
  WORLD         *pDcm;
  int            ret;
  int            jSta;
  MSG_LOGO       reclogo;       /* logo of retrieved message     */
  char          *InBuf;         /* string to hold input message  */
  long           InBufLen;      /* length of InBuf               */
  TracePacket   *WaveBuf;       /* pointer to raw trace message  */
  TracePacket    DecimBuf;      /* struct to hold decim message  */

  pDcm = ( WORLD *) dcm;
  
  /* Allocate the waveform buffer */
  InBufLen = (MAX_TRACEBUF_SIZ + sizeof (int));
  InBuf    = (char *) malloc ((size_t) InBufLen);
  WaveBuf  = (TracePacket *)(InBuf + sizeof(int));

  if (InBuf == NULL)
  {
    logit ("e", "decimate: Cannot allocate input buffer\n");
    pDcm->DecimatorStatus = -1;
    KillSelfThread();
  }

  /* Tell the main thread we're feeling ok */
  pDcm->DecimatorStatus = 0;

  while (1)
  {
    /* Get top message from the MsgQueue */
    RequestMutex ();
    ret = dequeue (&(pDcm->MsgQueue), InBuf, &InBufLen, &reclogo);
    ReleaseMutex_ew ();
    
    if (pDcm->dcmParam.debug)
      logit("","decthrd: dequeue returned %d\n", ret);
    
    if (ret < 0)
    {                                 /* empty queue */
      sleep_ew (500);
      continue;
    }

    /* Extract the SCNL number; recall, it was pasted as an int on the front 
     * of the message by the main thread */
    jSta = *((int*) InBuf);
    
    if (FilterDecimate( pDcm, WaveBuf, jSta, reclogo.type, &DecimBuf ) !=
        EW_SUCCESS)
    {
      logit("et", "decimate: error from FilterDecimate; exiting\n");
      pDcm->DecimatorStatus = -1;
      KillSelfThread();
      return(NULL); /* should never reach here */
    }

  } /* while (1) - message dequeuing process */
  return(NULL); /* should never reach here */

}


