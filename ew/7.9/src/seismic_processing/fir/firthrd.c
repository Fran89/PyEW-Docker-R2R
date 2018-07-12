
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: firthrd.c 5770 2013-08-09 15:00:31Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2005/07/20 15:34:48  friberg
 *     added a #ifndef for Linux to not include synch.h
 *
 *     Revision 1.1  2000/02/14 17:27:23  lucky
 *     Initial revision
 *
 *
 */

/*
 * firthrd.c: Fir thread
 *              1) Allocates memory for input and output TRACE_BUF messages
 *              2) Retrieves TRACE_BUF messages from their queues
 *              3) Determines station index from the queue message
 *              4) Dispatches messages to FirFilter
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: FirThread                                             */
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
#ifndef _LINUX	/* synch.h not posix threads */
#include <synch.h>      /* mutex's                                      */
#endif
#endif
#endif

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>  /* logit, threads                               */

/*******                                                        *********/
/*      Fir Includes                                                    */
/*******                                                        *********/
#include "fir.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: FirThread                                             */
thr_ret FirThread (void* fir)
{
  WORLD         *pFir;
  int            ret;
  int            jSta;
  MSG_LOGO       reclogo;       /* logo of retrieved message     */
  char          *WaveBuf;       /* string to hold wave message   */
  long           WaveBufLen;    /* length of WaveBuf             */
  char          *outBuf;        /* string to hold comp message   */
  long           outBufLen;     /* length of outBuf              */

  pFir = ( WORLD *) fir;
  
  /* Allocate the waveform buffer */
  WaveBufLen = (MAX_TRACEBUF_SIZ + sizeof (int));
  WaveBuf = (char *) malloc ((size_t) WaveBufLen);

  if (WaveBuf == NULL)
  {
    logit ("e", "fir: Cannot allocate waveform buffer\n");
    pFir->FirStatus = -1;
    KillSelfThread();
  }

  /* Allocate the fird data buffer */
  outBufLen = MAX_TRACEBUF_SIZ;
  outBuf = (char *) malloc ((size_t) outBufLen);

  if (outBuf == NULL) 
  {
    logit ("e", "fir: Cannot allocate fird data buffer\n");
    pFir->FirStatus = -1;
    KillSelfThread();
  }

  /* Tell the main thread we're feeling ok */
  pFir->FirStatus = 0;

  while (1)
  {
    /* Get top message from the MsgQueue */
    RequestMutex ();
    ret = dequeue (&(pFir->MsgQueue), WaveBuf, &WaveBufLen, &reclogo);
    ReleaseMutex_ew ();
    
    if (pFir->firParam.debug)
      logit("","fir: dequeue returned %d\n", ret);
    
    if (ret < 0)
    {                                 /* empty queue */
      sleep_ew (500);
      continue;
    }

    /* Extract the SCN number; recall, it was pasted as an int on the front 
     * of the message by the main thread */
    jSta = *((int*) WaveBuf);
    
    if (FirFilter( pFir, WaveBuf + sizeof(int), jSta, outBuf) !=
        EW_SUCCESS)
    {
      logit("et", "fir: error from FirFilter; exitting\n");
      pFir->FirStatus = -1;
      KillSelfThread();
      return(NULL); /* should never reach here */
    }

  } /* while (1) - message dequeuing process */
  return(NULL); /* should never reach here */
}


