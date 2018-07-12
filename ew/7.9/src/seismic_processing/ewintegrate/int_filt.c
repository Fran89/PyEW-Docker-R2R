
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: filtdb.c,v 1.4 2004/05/11 18:14:18 dietz Exp $
 *
 *    Revision history:
 *     $Log: filtdb.c,v $
 *
 *
 */

/*
 * int_filt.c: Integrate-filter
 *              1) Filters and debiases TRACE_BUF messages
 *              2) Fills in outgoing TRACE_BUF messages
 *              3) Puts outgoing messages on transport ring
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: FilterIntegrate                                       */
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

/*******                                                        *********/
/*      Debias Includes                                                 */
/*******                                                        *********/
#include "ewintegrate.h"

/*******                                                        *********/
/*      Structure definitions                                           */
/*******                                                        *********/


/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/
#include <assert.h>
         /*******************************************************
          *                     ResetSCNL                       *
          *                                                     *
          *  A new span has started; (re-)initialize            *
          * information about its running average               *
          *******************************************************/
int XfrmResetSCNL( XFRMSCNL *xSCNL, TracePacket *inBuf, XFRMWORLD* xWorld )
{
  int i;
  INTSCNL *pSCNL = (INTSCNL*)xSCNL;
  INTWORLD *pWorld = (INTWORLD*)xWorld;
  
  pSCNL->awDataSize = inBuf->trh2.datatype[1]-'0';

  if ( pWorld->xfrmParam->avgWindowSecs ) 
  {
      size_t bytesNeeded;        /* size of window in bytes */
      int samples;               /* nbr of samples in window */
      int numTBheaders;          /* nbr of packets cached when computing initial average */
      
      /* Calc nbr samples needed for length of window, and estimate nbr of packets 
         for that many samples */
      samples = (int)(inBuf->trh2.samprate * pWorld->xfrmParam->avgWindowSecs);
      if ( samples < 2 ) 
      {
        logit( "e", "ewintegrate: WARNING: AvgWindow too low to hold more than 1 sample; lengthened to hold 2\n" );
        samples = 2;
      }
      numTBheaders = (samples+inBuf->trh2.nsamp-1) / inBuf->trh2.nsamp;
    
      if ( samples == 0 ) 
      {
        logit( "e", "ewintegrate: packet wants window with 0 samples!" );
        return EW_FAILURE;
      }  
      
      
      /* (Re-)Allocation of space for headers of the initial packets */
      if ( numTBheaders > pSCNL->obhAllocated )
      {
        if ( pSCNL->obhAllocated == 0 )
          pSCNL->outBufHdr = (TRACE2_HEADER*)calloc( numTBheaders, sizeof(TRACE2_HEADER) );
        else
          pSCNL->outBufHdr = (TRACE2_HEADER*)realloc( pSCNL->outBufHdr, numTBheaders * sizeof(TRACE2_HEADER) );
    
        pSCNL->obhAllocated = numTBheaders;
      } 
      
      /* (Re-)allocation of space for window of samples */
      pSCNL->awSamples = samples;
      bytesNeeded = samples * pSCNL->awDataSize;
      if ( bytesNeeded > pSCNL->awBytesAllocated ) 
      {
        if ( pSCNL->awBytesAllocated == 0 )
          pSCNL->avgWindow = (void*)calloc( samples, pSCNL->awDataSize);
        else
          pSCNL->avgWindow = (void*)realloc( pSCNL->avgWindow, bytesNeeded );
        if ( pSCNL->avgWindow == NULL )
        {
          logit( "e", "ewintegrate: failed to allocate averaging window\n" );
          return EW_FAILURE;
        }
        pSCNL->awBytesAllocated = (int)bytesNeeded;
      }
      
      /* Reset counters/flags */
      pSCNL->awPosn = pSCNL->awFull = pSCNL->nCachedOutbufs = 0;
      pSCNL->sum = 0.0;
      pSCNL->awEmpty = 1;
  }
  
  pSCNL->period = 1.0 / inBuf->trh2.samprate;

  if ( pSCNL->nHPLevels > 0 ) 
  {
    complex *poles = pSCNL->poles;

    highpass( pSCNL->hpFreq, pSCNL->period, pSCNL->hpOrder, poles, &pSCNL->hp_gain );
  
    for ( i=0; i<pSCNL->nHPLevels; i++ ) 
    {
      pSCNL->hpLevel[i].d1 = pSCNL->hpLevel[i].d2 = 0.0;
      pSCNL->hpLevel[i].a1 = -2.0 * poles[i+i].real;
      pSCNL->hpLevel[i].a2 = poles[i+i].real*poles[i+i].real + poles[i+i].imag*poles[i+i].imag;
      pSCNL->hpLevel[i].b1 = -2.0;
      pSCNL->hpLevel[i].b2 = 1.0;
    }
  }
  
  return EW_SUCCESS;
} 

int XfrmInitSCNL( XFRMWORLD* xEwi, XFRMSCNL* xSCNL )
{
  int i;
  INTSCNL *pSCNL = (INTSCNL*)xSCNL;
  INTWORLD *pEwi = (INTWORLD*)xEwi;
  complex *poles;

  /* base setting */
  pSCNL->inEndtime =0.0;

  /* Debias fields */
  pSCNL->outBufHdr = pSCNL->avgWindow = NULL;
  pSCNL->obhAllocated = pSCNL->nCachedOutbufs = pSCNL->awPosn 
      = pSCNL->awBytesAllocated = pSCNL->awSamples = pSCNL->awFull 
      = (int)(pSCNL->awDataSize = 0);
  pSCNL->awEmpty = 1;
  pSCNL->sum = 0.0;
  pSCNL->hpFreq = pEwi->xfrmParam->hpFreqCutoff;
  pSCNL->hpOrder = pEwi->xfrmParam->hpOrder;


  if ( pEwi->xfrmParam->avgWindowSecs == 0 ) 
    pSCNL->awSamples = pSCNL-> awFull = 1;
    
  /* Integrate & filter fields */
  pSCNL->outBuf = NULL;
  pSCNL->hpLevel = NULL;
  pSCNL->nHPLevels = (pEwi->xfrmParam->hpOrder+1)/2;
  if ( pSCNL->nHPLevels > 0 )
  {            
    HP_STAGE* stages = (HP_STAGE*)calloc( sizeof(HP_STAGE), pSCNL->nHPLevels);
    if ( stages == NULL ) 
    {
      logit ("et", "ewintegrate: Failed to allocate SCNL info stages; exiting.\n" );
      return EW_FAILURE;
    }
    else
      pSCNL->hpLevel = stages;
    poles = (complex*)calloc( sizeof(complex), pEwi->xfrmParam->hpOrder);
    if ( poles == NULL )
    {
      logit ("t", "ewintegrate: Failed to allocate SCNL info poles; exiting.\n" );
      free( stages );
      return EW_FAILURE;
    }
    else
      pSCNL->poles = poles;
      
    for ( i=0; i<pSCNL->nHPLevels; i++ )
      stages[i].d1 = stages[i].d2 = stages[i].f = 0; 
  }
  return EW_SUCCESS;   
}

         /*******************************************************
          *                      IntFilt                        *
          *                                                     *
          * Integrate datum (using the previous acc) if         *
          * integrating; if filtering, apply that afterward.    *
          * Yield the result of the processing                  *
          *******************************************************/
static double IntFilt (INTSCNL *pSCNL, double datum, INTWORLD *pEwi)
{
  int i;
  double out;
  double coef = 1.0;
  double prev_vel, next_out;  

  if ( pEwi->xfrmParam->doIntegration ) {
      if(pSCNL->awEmpty)
      {
        pSCNL->prev_vel = out = 0.0;
        pSCNL->awEmpty = 0;
      } else {
        prev_vel = (1.0+coef)*(datum+pSCNL->prev_acc)/4.0 + (double)coef*pSCNL->prev_vel;
        pSCNL->prev_vel = prev_vel;
        out = prev_vel * pSCNL->period;
      }
      pSCNL->prev_acc = datum;
  } else
    out = datum;
  for ( i=0; i<pSCNL->nHPLevels; i++ )
  {
    HP_STAGE *lvl = pSCNL->hpLevel+i;
    next_out = out + lvl->d1;
	lvl->d1 = lvl->b1*out - lvl->a1*next_out + lvl->d2 ;
	lvl->d2 = lvl->b2*out - lvl->a2*next_out ;
	out = next_out;
  }
  return pSCNL->nHPLevels>0 ? (out*pSCNL->hp_gain) : out;
}


         /*******************************************************
          *                    FilterXfrm                       *
          *                                                     *
          *  A new packet to be processed has arrived; process  *
          * and write to output ring as necessary.              *
          *******************************************************/
int FilterXfrm (XFRMWORLD* arg, TracePacket *inBuf, int jSta, 
                    unsigned char msgtype, TracePacket *outBuf)
{
  INTSCNL *pSCNL;
  short *waveShort = NULL, *outShort = NULL, *awShort = NULL;
  int32_t  *waveLong = NULL,  *outLong = NULL, *awLong = NULL;
  int ret, i, j, k;
  int datasize, outBufLen;
  int posn = 0;
  INTWORLD *pEwi = (INTWORLD*)arg;
  char debiasing = (pEwi->xfrmParam->avgWindowSecs != 0);
  double datum;
  
  pSCNL = ((INTSCNL*)(pEwi->scnls))+jSta;

/*
  fprintf(stderr, "DEBUG: Processing %s.%s.%s.%s from SCNLs array element %d\n",
	pSCNL->inSta, pSCNL->inChan, pSCNL->inNet, pSCNL->inLoc, jSta);
*/

  ret = BaseFilterXfrm( arg, inBuf, jSta, msgtype, outBuf );
/*
  fprintf(stdout, "after BaseFilterXfrm().... SUCCESS\n");
*/

  if ( ret != EW_SUCCESS )
  {
    return ret;
  }
  
  
  /* Local copies of various values */
  datasize = (int)pSCNL->awDataSize;

  /* Set some useful pointers */
  if ( datasize==2 ) 
  {
    waveShort = (short*)(inBuf->msg + sizeof(TRACE2_HEADER));
    outShort  = (short*)(outBuf->msg + sizeof(TRACE2_HEADER));
  } else {
    waveLong  = (int32_t*) (inBuf->msg + sizeof(TRACE2_HEADER));
    outLong   = (int32_t*) (outBuf->msg + sizeof(TRACE2_HEADER));
  }

  if ( debiasing ) 
  {
      posn = pSCNL->awPosn;
      /* Get properly-typed pointer to window */
      if ( datasize==2 )
        awShort = (short*)(pSCNL->avgWindow);
      else
        awLong = (int32_t*)(pSCNL->avgWindow);
  }

  for ( i=0; i<inBuf->trh2.nsamp; i++ )
  {
    if ( debiasing )
    {
        /* If window in full, remove the oldest entry from sum */
        if ( pSCNL->awFull )
        {
          int oldest_posn = (posn==pSCNL->awSamples ? 0 : posn);
          pSCNL->sum -= (datasize==2 ? awShort[oldest_posn] : awLong[oldest_posn]);
        }
    
        /* Add newest entry to sum & window */
        if ( datasize==2 )
        { 
          pSCNL->sum += (datum = waveShort[i]);
          awShort[posn++] = waveShort[i];
        }
        else
        {
          pSCNL->sum += (datum = waveLong[i]);
          awLong[posn++] = waveLong[i];
        }
    } else
    {
        datum = (datasize==2 ? waveShort[i] : waveLong[i]);
    }
    
    /* If window is full, remove bias from latest value */
    if ( pSCNL->awFull )
    {
      datum = IntFilt (pSCNL, datum - (pSCNL->sum / pSCNL->awSamples), pEwi );
      /*logit("","N[%d]: %f\n", i, datum );*/
      if ( datasize==2 )
        outShort[i] = (short)(datum + 0.5);
      else
        outLong[i] = (int32_t)(datum + 0.5);
      if ( posn == pSCNL->awSamples )
        posn = 0;
    }
    /* If we filled window for first time, flush the cache after debiasing it */
    else if ( posn == pSCNL->awSamples )
    {
      double bias = pSCNL->sum / pSCNL->awSamples;
      if (pEwi->xfrmParam->debug)
        logit( "et", "first flush of cache (bias=%f=%f/%d)\n", bias,pSCNL->sum,pSCNL->awSamples);

      if ( pSCNL->awFull == 0 )
      {
        pSCNL->awFull = 1;
        posn = 0;
        
        /* Flush each of the caches outbufs */
        for ( k=0; k<pSCNL->nCachedOutbufs; k++ )
        {
          if (pEwi->xfrmParam->debug)
            logit("et","Flushing outbuf #%d (%d samples)\n", k, pSCNL->outBufHdr[k].nsamp );
          outBuf->trh2 = pSCNL->outBufHdr[k];
          for ( j=0; j<pSCNL->outBufHdr[k].nsamp; j++ )
          {
            double dVal = (datasize==2 ? awShort[posn++] : awLong[posn++]);
            dVal = IntFilt (pSCNL, dVal - (pSCNL->sum / pSCNL->awSamples), pEwi );
            if ( datasize==2 )
              outShort[j] = (short)(dVal + 0.5);
            else
              outLong[j] = (int32_t)(dVal + 0.5);
          }
          
          /* Ship the packet out to the transport ring */
          pEwi->trcLogo.type = msgtype;
          outBufLen = pSCNL->outBufHdr[k].nsamp * datasize + sizeof(TRACE2_HEADER);
          if (tport_putmsg (&(pEwi->regionOut), &(pEwi->trcLogo), outBufLen, 
                            outBuf->msg) != PUT_OK)
          {
            logit ("et","debias: Error sending type:%d message.\n", msgtype );
            return EW_FAILURE;
          }
          
        }
        
        /* Fill current outbuf w/ unbiased previously read values */
        for ( j=0; j<i; j++ )
        {
          if ( datasize==2 )
          {
            outShort[j] = waveShort[j] - (short)bias;
          } else {
            outLong[j] = waveLong[j] - (int32_t)bias;
          }
        }

      }
      posn = 0;
    }
  }
  pSCNL->awPosn = posn;
  if ( pSCNL->awFull )
  {
      /* Ship the packet out to the transport ring */
      outBuf->trh2 = inBuf->trh2;
      outBufLen = inBuf->trh2.nsamp * datasize + sizeof(TRACE2_HEADER);
      ret = XfrmWritePacket( arg, (XFRMSCNL*)pSCNL, msgtype, outBuf, outBufLen );      
      if ( ret != EW_SUCCESS )
      {
        return ret;
      }
  }
  else
  {
    if (pEwi->xfrmParam->debug)
      logit( "et", "packet didn't fill window (%d vs %d)\n", pSCNL->awPosn, pSCNL->awSamples );
    /* Add this packet to cached list */
    if ( pSCNL->nCachedOutbufs >= pSCNL->obhAllocated )
    {
      pSCNL->obhAllocated = pSCNL->nCachedOutbufs + 5;
      pSCNL->outBufHdr = (TRACE2_HEADER*)realloc( pSCNL->outBufHdr, (size_t)(pSCNL->obhAllocated * sizeof(TRACE_HEADER)) );
    }
    pSCNL->outBufHdr[pSCNL->nCachedOutbufs++] = inBuf->trh2;    
    outBuf->trh2.version[0] = TRACE2_VERSION0;
    outBuf->trh2.version[1] = TRACE2_VERSION1;

    /* Copy the new SCNL into the cached header */
    strcpy( pSCNL->outBufHdr[pSCNL->nCachedOutbufs-1].sta, pSCNL->outSta );
    strcpy( pSCNL->outBufHdr[pSCNL->nCachedOutbufs-1].net, pSCNL->outNet );
    strcpy( pSCNL->outBufHdr[pSCNL->nCachedOutbufs-1].chan, pSCNL->outChan );
    strcpy( pSCNL->outBufHdr[pSCNL->nCachedOutbufs-1].loc, pSCNL->outLoc );    
  }
  
  return EW_SUCCESS;
}
