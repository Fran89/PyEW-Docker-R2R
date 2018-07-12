
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: do1stg.c 1459 2004-05-11 18:14:18Z dietz $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2004/05/11 18:14:17  dietz
 *     Modified to work with either TYPE_TRACEBUF2 or TYPE_TRACEBUF msgs
 *
 *     Revision 1.1  2000/02/14 16:56:25  lucky
 *     Initial revision
 *
 *
 */

/*
 * do1stg.c: Do one filter/decimation stage for one SCNL
 *              1) Perform one stage of filter and decimator
 *              2) Proceed to following stage recursively
 *              3) Report number of data points at final stage
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: DoOneSage                                             */
/*                                                                      */
/*      Inputs:         Pointer to stage ready to be processed          */
/*                                                                      */
/*      Outputs:        Filtered and decimated data for next stage      */
/*                                                                      */
/*      Returns:        On success, number of datapoints from final     */
/*                         stage.                                       */
/*                      On failure, error status defined in decimate.d  */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>  

/*******                                                        *********/
/*      Decimate Includes                                               */
/*******                                                        *********/
#include "decimate.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: DoOneStage                                            */
int DoOneStage ( PSTAGE this )
{
  PSTAGE next;
  double sum;           /* Holder for the filter result                 */  
  int need;             /* number of datapoints needed in output buffer */
  int length;
  int half_l;           /* Number of duplicated coefficients in filter  */
  int read;             /* Local copy of the buffer read point          */
  int nodd;             /* =1 if filter length is odd; 0 otherwise      */
  int i;
  static int ret;
  
  next = this->next;
  length = this->pFilt->Length;
  half_l = length / 2;
  nodd = length - half_l * 2;
  read = this->inBuff.read;
  
  /* Check for sufficient space in the next stage's buffer, our ouput.  */
  need = (this->inBuff.write - read - length) / this->pFilt->decRate;
  if (need > BUFFSIZE - next->inBuff.write)
    return FD_NOROOM;
  
  /* It the next buffer is empty, update its information */
  if (next->inBuff.write == 0)
  {
    /* Start time is time of newest sample that we'll read first        */
    next->inBuff.starttime = this->inBuff.starttime + 
#ifdef FIXTIMELAG
      (read + (length - 1) / 2.0) / this->inBuff.samplerate;
#else    
      (read + length - 1) / this->inBuff.samplerate;
#endif
    next->inBuff.samplerate = this->inBuff.samplerate / this->pFilt->decRate;
  }
  
  /* Process as much data as we can */
  while (read + length < this->inBuff.write)
  {
   if (nodd == 1)
      sum = this->pFilt->coef[half_l] * this->inBuff.buffer[read + half_l];
    else
      sum = 0.0;

    /* Filter is symmetric, so we make things a little more efficient */
    for (i = 0; i < half_l; i++)
      sum += this->pFilt->coef[i] * 
        (this->inBuff.buffer[read + i] +
         this->inBuff.buffer[read + length - i - 1]);
    next->inBuff.buffer[next->inBuff.write++] = sum;

    /* Advance the read point by the decimation rate: this is how       *
     * we decimate the data.                                            */
    read += this->pFilt->decRate;
  }
  
  /* Update the current buffer: slide the old data out, update pointers */
  if (read > 0)
  {
    memmove(this->inBuff.buffer, &(this->inBuff.buffer[read]), 
            (this->inBuff.write - read) * sizeof(double));
    this->inBuff.write -= read;
    this->inBuff.starttime += read / this->inBuff.samplerate;
    read = 0;
  }
  this->inBuff.read = read;
  
  /* Maybe go on to the next stage */
  if (next->next == (PSTAGE) NULL)
  { /* There is no next stage: report how much is in final buffer */
    return (next->inBuff.write - next->inBuff.read);
  } 
  else 
  {
    ret = DoOneStage(next);
    return ret;
  }
}

    
