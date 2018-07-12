
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: filtdecim.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2004/05/11 18:14:18  dietz
 *     Modified to work with either TYPE_TRACEBUF2 or TYPE_TRACEBUF msgs
 *
 *     Revision 1.3  2003/05/08 16:56:40  dietz
 *     Changed the check for minimum number of samples from
 *     ret > minTraceBufLen  to  ret >= minTraceBufLen
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
 * filtdecim.c: Filter-Decimator
 *              1) Filters and decimates TRACE_BUF messages
 *              2) Fills in outgoing TRACE_BUF messages
 *              3) Puts outgoing messages on transport ring
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: FilterDecimate                                        */
/*                                                                      */
/*      Inputs:         Pointer to World Structure                      */
/*                      Pointer to incoming TRACE message               */
/*                      SCNL index of incoming message                  */
/*                      Pointer to outgoing TRACE buffer                */
/*                                                                      */
/*      Outputs:        Message sent to the output ring                 */
/*                                                                      */
/*      Returns:        EW_SUCCESS on success, else EW_FAILURE          */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>  /* logit, threads                               */
#include <trheadconv.h> /* trheadconv()                                 */

/*******                                                        *********/
/*      Decimate Includes                                               */
/*******                                                        *********/
#include "decimate.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: FilterDecimate                                        */
int FilterDecimate (WORLD* pDcm, TracePacket *inBuf, int jSta, 
                    unsigned char msgtype, TracePacket *outBuf)
{
  STATION *pSta;
  PSTAGE this, final;
  short *waveShort, *outShort;
  int32_t  *waveLong,  *outLong;
  int ret, i;
  int datasize, outBufLen;
  char logit_str[3];

  strcpy(logit_str, "et");
  if (pDcm->dcmParam.quiet) 
  {
    strcpy(logit_str, "t"); /* just logit to logfile, not to stderr */
  }
  
  /* Set some useful pointers */
  waveShort = (short*)(inBuf->msg + sizeof(TRACE2_HEADER));
  waveLong  = (int32_t*) (inBuf->msg + sizeof(TRACE2_HEADER));
  outShort  = (short*)(outBuf->msg + sizeof(TRACE2_HEADER));
  outLong   = (int32_t*) (outBuf->msg + sizeof(TRACE2_HEADER));
  pSta = &(pDcm->stations[jSta]);
  this = pSta->pStage;

  memset( outBuf->msg, 0, sizeof( TracePacket ) );

/* If it's tracebuf, make it look like a tracebuf2 message */
  if( msgtype == pDcm->dcmEWH.typeTrace ) TrHeadConv( &(inBuf->trh) );
  
  if (pDcm->dcmParam.debug)
    logit("t", "enter filtdec with <%s.%s.%s.%s> start: %lf\n", 
          inBuf->trh2.sta, inBuf->trh2.chan, inBuf->trh2.net, inBuf->trh2.loc,
          inBuf->trh2.starttime );
  
  /* If we have previous data, check for data gap */
  if ( pSta->inEndtime != 0.0 )
  {
    if ( (inBuf->trh2.starttime - pSta->inEndtime) * inBuf->trh2.samprate > 
         pDcm->dcmParam.maxGap )
    {
      logit(logit_str, "decimate: gap in data for <%s.%s.%s.%s>:\n"
            "\tlast end: %lf this start: %lf; resetting\n",
            pSta->inSta, pSta->inChan, pSta->inNet, pSta->inLoc, 
            pSta->inEndtime, inBuf->trh2.starttime);
      ResetStation( pSta );
    }
    else if (inBuf->trh2.starttime < pSta->inEndtime)
    {
      logit(logit_str, "decimate: overlapping times for <%s.%s.%s.%s>:\n"
            "\tlast end: %lf this start: %lf; resetting\n",
            pSta->inSta, pSta->inChan, pSta->inNet, pSta->inLoc, 
            pSta->inEndtime, inBuf->trh2.starttime);
      ResetStation( pSta );
    }
  }
    
  /* Check for sufficient space for incoming data */
  if ( inBuf->trh2.nsamp > BUFFSIZE - this->inBuff.write )
  {
    logit("et", "decimate: no room for data <%s.%s.%s.%s>; exiting.\n",
          pSta->inSta, pSta->inChan, pSta->inNet, pSta->inLoc);
    return EW_FAILURE;
  }
  
  /* Check for useable data types: we only handle shorts and longs for now */
  if ( inBuf->trh2.datatype[0] != 's' && inBuf->trh2.datatype[0] != 'i' )
  {
    logit("et", "decimate: unusable datatype <%s> from <%s.%s.%s.%s>; skipping\n",
          inBuf->trh2.datatype, inBuf->trh2.sta, inBuf->trh2.chan, 
          inBuf->trh2.net, inBuf->trh2.loc );
    return EW_SUCCESS;
  }
  
  /* If stage buffer is empty, set its initial values */
  if ( this->inBuff.write == 0 )
  {
    this->inBuff.starttime  = inBuf->trh2.starttime;
    this->inBuff.samplerate = inBuf->trh2.samprate;
  } 
  else
  {  /* Update its start time based on new data */
    this->inBuff.starttime = inBuf->trh2.starttime - 
                             this->inBuff.write / this->inBuff.samplerate;
  }
      
  /* Copy in new data */
  if ( inBuf->trh2.datatype[1] == '2' )
  {
    datasize = 2;
    for (i = 0; i < inBuf->trh2.nsamp; i++)
      this->inBuff.buffer[this->inBuff.write++] = (double) waveShort[i];
  }
  else if (inBuf->trh2.datatype[1] == '4' )
  {
    datasize = 4;
    for (i = 0; i < inBuf->trh2.nsamp; i++)
      this->inBuff.buffer[this->inBuff.write++] = (double) waveLong[i];
  }
  else
  {
    logit("et", "decimate: unusable datatype <%s> from <%s.%s.%s.%s>; skipping\n",
          inBuf->trh2.datatype, inBuf->trh2.sta, inBuf->trh2.chan, 
          inBuf->trh2.net, inBuf->trh2.loc);
    return EW_SUCCESS;
  }

  pSta->inEndtime = inBuf->trh2.endtime;
  
  /* Start the recursive trail of filter/decimators on this stage */
  ret = DoOneStage( this );
  
  if ( ret > 0 )
  {  /* Something is coming out the far end of the pipe! */
    if (ret >= pDcm->dcmParam.minTraceBufLen)
    {  /* Yipeee! We can send it */

      /* Find the final stage */
      final = this->next;
      while (final->next != (PSTAGE)NULL) final = final->next;

      /* Copy data into the outgoing TracePacket */     
      /* trh & trh2 are same except for loc & version fields */     
      outBuf->trh2.pinno     = inBuf->trh2.pinno;
      outBuf->trh2.nsamp     = ret;
      outBuf->trh2.starttime = final->inBuff.starttime;
      outBuf->trh2.samprate  = final->inBuff.samplerate;
      outBuf->trh2.endtime   = (double)(ret-1)/outBuf->trh2.samprate + 
                               outBuf->trh2.starttime;
      strcpy(outBuf->trh2.sta,  pSta->outSta);
      strcpy(outBuf->trh2.chan, pSta->outChan);
      strcpy(outBuf->trh2.net,  pSta->outNet);
      strncpy(outBuf->trh2.datatype, inBuf->trh2.datatype, 2);
      memcpy(outBuf->trh2.quality, inBuf->trh2.quality, 2);
      memcpy(outBuf->trh2.pad, inBuf->trh2.pad, 2);
      if( msgtype ==  pDcm->dcmEWH.typeTrace2 )
      {
        strcpy(outBuf->trh2.loc, pSta->outLoc);
        memcpy(outBuf->trh2.version, inBuf->trh2.version, 2);
      }
      
      if (datasize == 2)
        for (i = 0; i < ret; i++)
          outShort[i] = (short) final->inBuff.buffer[i + final->inBuff.read];
      else if (datasize == 4)
        for (i = 0; i < ret; i++)
          outLong[i] = (int32_t) final->inBuff.buffer[i + final->inBuff.read];

      /* we've copied everything out of the final buffer, so reset it. */
      final->inBuff.read  = 0;
      final->inBuff.write = 0;
      
      if (pDcm->dcmParam.debug) {
        if( msgtype == pDcm->dcmEWH.typeTrace2 ) {
          logit("","filtdec: shipping <%s.%s.%s.%s>, start %.2lf, end %.2lf\n",
                outBuf->trh2.sta, outBuf->trh2.chan, outBuf->trh2.net, 
                outBuf->trh2.loc, outBuf->trh2.starttime, outBuf->trh2.endtime );
        } else {
           logit("","filtdec: shipping <%s.%s.%s>, start %.2lf, end %.2lf\n",
                outBuf->trh2.sta, outBuf->trh2.chan, outBuf->trh2.net, 
                outBuf->trh2.starttime, outBuf->trh2.endtime );
        }
      }
      
      /* Ship the packet out to the transport ring */
      pDcm->trcLogo.type = msgtype;
      outBufLen = ret * datasize + sizeof(TRACE2_HEADER);
      if (tport_putmsg (&(pDcm->regionOut), &(pDcm->trcLogo), outBufLen, 
                        outBuf->msg) != PUT_OK)
      {
        logit ("et", "decimate: Error sending type:%d message.\n", msgtype );
        return EW_FAILURE;
      }
    } /* if ret >= minTraceBufLen */
  }
  else if (ret < 0)
  {  /* An error occured in DoOneStage */
    switch (ret)
    {
    case FD_NOROOM:
      if( msgtype == pDcm->dcmEWH.typeTrace2 ) {
        logit("et", "decimate: no room for data <%s.%s.%s.%s>; exiting.\n",
              pSta->inSta, pSta->inChan, pSta->inNet, pSta->inLoc);
      } else {
        logit("et", "decimate: no room for data <%s.%s.%s>; exiting.\n",
              pSta->inSta, pSta->inChan, pSta->inNet);
      } 
      return EW_FAILURE;
      break;
    default:
      logit("et", "decimate: unknown error return from DoOneStage: %d; exiting\n",
            ret);
      return EW_FAILURE;
    }
  }
  return EW_SUCCESS;
}
