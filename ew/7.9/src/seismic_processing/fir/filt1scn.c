
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: filt1scn.c 4477 2011-08-04 15:19:37Z kevin $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2004/07/28 22:43:05  lombard
 *     Modified to handle SCNLs and TYPE_TRACEBUF2 (only!) messages.
 *
 *     Revision 1.1  2000/02/14 17:27:23  lucky
 *     Initial revision
 *
 *
 */

/*
 * filt1scn.c: Filter one SCNL's buffer of data
 *              1) Report number of data points in output buffer
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: FiltOneSCNL                                            */
/*                                                                      */
/*      Inputs:         Pointer to stage ready to be processed          */
/*                                                                      */
/*      Outputs:        Filtered data                                   */
/*                                                                      */
/*      Returns:        On success, number of datapoints in output      */
/*                         buffer.                                      */
/*                      On failure, error status defined in fir.d       */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <string.h>

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>  

/*******                                                        *********/
/*      Fir Includes                                                    */
/*******                                                        *********/
#include "fir.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: FiltOneSCNL                                            */
int FiltOneSCNL ( WORLD *pFir, STATION *this )
{
  double sum;           /* Holder for the filter result                 */  
  int need;             /* number of datapoints needed in output buffer */
  int half_l;           /* number of duplicated coefficients in filter  */
  int read;             /* Local copy of the buffer read point          */
  int nodd;             /* =1 if filter length is odd; 0 otherwise      */
  int i;
  static int ret;
  double num;
  
  half_l = pFir->filter.Length / 2;
  nodd = pFir->filter.Length - half_l * 2;
  read = this->inBuff.read;
  
  /* Check for sufficient space in the output buffer.  */
  need = this->inBuff.write - read - pFir->filter.Length;
  if (need > BUFFSIZE - this->outBuff.write)
    return FIR_NOROOM;
  
  /* It the next buffer is empty, update its information */
  if (this->outBuff.write == 0)
  {
    /* Start time is time of newest sample that we'll read first        */
    this->outBuff.starttime = this->inBuff.starttime + 
#ifdef FIXTIMELAG
      (read + (pFir->filter.Length - 1) / 2.0) / this->inBuff.samplerate;
#else    
      (read + pFir->filter.Length - 1) / this->inBuff.samplerate;
#endif
  }
  
#ifdef DEBUG
  logit("", "filt1scn: read %d, write %d\n", this->inBuff.read, 
        this->inBuff.write);
#endif
  
  /* Process as much data as we can */
  while (read + pFir->filter.Length < this->inBuff.write)
  {
    if (nodd == 1)
      sum = pFir->filter.coef[half_l] * this->inBuff.buffer[read + half_l];
    else
      sum = 0.0;
  
    /* Filter is symmetric, so we make things a little more efficient */
    for (i = 0; i < half_l; i++)
      sum += pFir->filter.coef[i] 
        * (this->inBuff.buffer[read + i] + 
           this->inBuff.buffer[read + pFir->filter.Length - i - 1]);

    this->outBuff.buffer[this->outBuff.write++] = sum;
    read ++;
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
  
  return (this->outBuff.write - this->outBuff.read);
}

    
