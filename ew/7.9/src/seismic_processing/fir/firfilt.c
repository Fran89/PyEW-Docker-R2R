
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: firfilt.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2007/02/14 19:57:19  luetgert
 *     Fixed problem when encountering a zero-length trcebuf.
 *     .
 *
 *     Revision 1.3  2006/12/31 00:05:02  stefan
 *     added outHead->version[0] = TRACE2_VERSION0 and outHead->version[1] = TRACE2_VERSION1 per David J Scott's discovery of issues with carlstatrig
 *
 *     Revision 1.2  2004/07/28 22:43:05  lombard
 *     Modified to handle SCNLs and TYPE_TRACEBUF2 (only!) messages.
 *
 *     Revision 1.1  2000/02/14 17:27:23  lucky
 *     Initial revision
 *
 *
 */

/*
 * firfilt.c: Applies FIR filter to trace data
 *              1) Filters TRACE2_BUF messages
 *              2) Fills in outgoing TRACE2_BUF messages
 *              3) Puts outgoing messages on transport ring
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: FirFilter                                        */
/*                                                                      */
/*      Inputs:         Pointer to World Structure                      */
/*                      Pointer to incoming TRACE2_BUF message           */
/*                      SCN index of incoming message                   */
/*                      Pointer to outgoing TRACE2_BUF buffer            */
/*                                                                      */
/*      Outputs:        Message sent to the output ring                 */
/*                                                                      */
/*      Returns:        EW_SUCCESS on success, else EW_FAILURE          */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <string.h>

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>  /* logit, threads                               */

/*******                                                        *********/
/*      Fir Includes                                               */
/*******                                                        *********/
#include "fir.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: FirFilter                                        */
int FirFilter (WORLD* pFir, char *inBuf, int jSta, char *outBuf)
{
  STATION *this;
  TRACE2_HEADER *waveHead, *outHead;
  short *waveShort, *outShort;
  int32_t *waveLong, *outLong;
  int ret, i;
  int datasize, outBufLen;
  
  /* Set some useful pointers */
  waveHead = (TRACE2_HEADER*) inBuf;
  waveShort = (short*)(inBuf + sizeof(TRACE2_HEADER));
  waveLong = (int32_t*)(inBuf + sizeof(TRACE2_HEADER));
  outHead = (TRACE2_HEADER*)outBuf;
  outShort = (short*)(outBuf + sizeof(TRACE2_HEADER));
  outLong = (int32_t*)(outBuf + sizeof(TRACE2_HEADER));
  this = &(pFir->stations[jSta]);
  
  if (pFir->firParam.debug)
    logit("t", "enter firfilt with <%s.%s.%s.%s> start: %lf\n", waveHead->sta,
          waveHead->chan, waveHead->net, waveHead->loc, waveHead->starttime );
  
  /* If we have previous data, check for data gap */
  if ( this->inEndtime != 0.0 )
  {
    if ( (waveHead->starttime - this->inEndtime) * waveHead->samprate > 
         pFir->firParam.maxGap )
    {
      logit("et", "fir: gap in data for <%s.%s.%s.%s>:\n"
            "\tlast end: %lf this start: %lf; resetting\n",
            this->inSta, this->inChan, this->inNet, this->inLoc, 
	    this->inEndtime, waveHead->starttime);
      ResetStation( this );
    }
    else if (waveHead->starttime < this->inEndtime)
    {
      logit("et", "fir: overlapping times for <%s.%s.%s.%s>:\n"
            "\tlast end: %lf this start: %lf; resetting\n",
            this->inSta, this->inChan, this->inNet, this->inLoc,
	    this->inEndtime, waveHead->starttime);
      ResetStation( this );
    }
  }
    
  /* Check for sufficient space for incoming data */
  if ( waveHead->nsamp > BUFFSIZE - this->inBuff.write )
  {
    logit("et", "fir: no room for data <%s.%s.%s.%s>; exitting.\n",
          this->inSta, this->inChan, this->inNet, this->inLoc);
    return EW_FAILURE;
  }
  
  /* Check for useable data types: we only handle shorts and longs for now */
  if ( waveHead->datatype[0] != 's' && waveHead->datatype[0] != 'i' )
  {
    logit("et", "fir: unusable datatype <%s> from SCNL <%s.%s.%s.%s>; skipping\n",
          waveHead->datatype, waveHead->sta, waveHead->chan, waveHead->net,
	  waveHead->loc);
    return EW_SUCCESS;
  }
  
  /* If stage buffer is empty, set its initial values */
  if ( this->inBuff.write == 0 )
  {
    this->inBuff.starttime = waveHead->starttime;
    this->inBuff.samplerate = waveHead->samprate;
  } 
  else
  {  /* Update its start time based on new data */
    this->inBuff.starttime = waveHead->starttime - this->inBuff.write 
      / waveHead->samprate;
  }
      
  /* Copy in new data */
  if ( waveHead->datatype[1] == '2' )
  {
    datasize = 2;
    for (i = 0; i < waveHead->nsamp; i++)
      this->inBuff.buffer[this->inBuff.write++] = (double) waveShort[i];
  }
  else if (waveHead->datatype[1] == '4' )
  {
    datasize = 4;
    for (i = 0; i < waveHead->nsamp; i++)
      this->inBuff.buffer[this->inBuff.write++] = (double) waveLong[i];
  }
  else
  {
    logit("et", "fir: unusable datatype <%s> from SCNL <%s.%s.%s.%s>; skipping\n",
          waveHead->datatype, waveHead->sta, waveHead->chan, waveHead->net,
	  waveHead->loc);
    return EW_SUCCESS;
  }

  this->inEndtime = waveHead->endtime;
  
  /* Do the actual filtering */
  ret = FiltOneSCNL( pFir, this );
  
  if ( ret > 0 )
  {  /* Something is coming out the far end of the pipe! */
    while (this->outBuff.write - this->outBuff.read >= waveHead->nsamp)
    {  /* Make the output packets the same length as the input ones. */

      /* Copy data into the outgoing TRACE_BUF packet */      
      memcpy(outHead->version, waveHead->version, 2); 
      outHead->pinno = waveHead->pinno;
      outHead->nsamp = waveHead->nsamp;
      outHead->starttime = this->outBuff.starttime
        + this->outBuff.read / waveHead->samprate;
      outHead->samprate = waveHead->samprate;
      outHead->endtime = (outHead->nsamp - 1) / outHead->samprate 
        + outHead->starttime;
      strcpy(outHead->sta, this->outSta);
      strcpy(outHead->chan, this->outChan);
      strcpy(outHead->net, this->outNet);
      strcpy(outHead->loc, this->outLoc);
      strncpy(outHead->datatype, waveHead->datatype,3);
      memcpy(outHead->quality, waveHead->quality, 2);
      memcpy(outHead->pad, waveHead->pad, 2);
      
      if (datasize == 2)
        for (i = 0; i < outHead->nsamp; i++)
          outShort[i] = (short) this->outBuff.buffer[i + this->outBuff.read];
      else if (datasize == 4)
        for (i = 0; i < outHead->nsamp; i++)
          outLong[i] = (int32_t) this->outBuff.buffer[i + this->outBuff.read];

      this->outBuff.read += outHead->nsamp;
      
      if (pFir->firParam.debug)
        logit("","firfilt: shipping <%s.%s.%s.%s>, start %lf, end %lf\n",
              outHead->sta, outHead->chan, outHead->net, outHead->loc,
	      outHead->starttime, outHead->endtime );
      
      /* Ship the packet out to the transport ring */
      outBufLen = outHead->nsamp * datasize + sizeof(TRACE2_HEADER);
      if (tport_putmsg (&(pFir->regionOut), &(pFir->trcLogo), outBufLen, 
                        outBuf) != PUT_OK)
      {
        logit ("et", "fir:  Error sending TRACE_BUF message.\n" );
        return EW_FAILURE;
      }
    } /* While... */

    if (this->outBuff.read > 0)
    {  /* Slide the old data out of the output buffer */
      memmove(this->outBuff.buffer, 
              &(this->outBuff.buffer[this->outBuff.read]),
              (this->outBuff.write - this->outBuff.read) * sizeof(double));
      this->outBuff.write -= this->outBuff.read;
      this->outBuff.starttime += this->outBuff.read / outHead->samprate;
      this->outBuff.read = 0;
    }
  }
  else if (ret < 0)
  {  /* An error occured in FiltOneSCN */
    switch (ret)
    {
    case FIR_NOROOM:
      logit("et", "fir: no room for data <%s.%s.%s.%s>; exitting.\n",
            this->inSta, this->inChan, this->inNet, this->inLoc);
      return EW_FAILURE;
      break;
    default:
      logit("et", "fir: unknown error return from FiltOneSCN: %d; exitting\n",
            ret);
      return EW_FAILURE;
    }
  }
  return EW_SUCCESS;
}
