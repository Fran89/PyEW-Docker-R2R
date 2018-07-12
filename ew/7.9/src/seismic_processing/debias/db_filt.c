
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
 * db_filt.c: Filter for Debiasor
 *              1) Filters and debiases TRACE_BUF messages
 *              2) Fills in outgoing TRACE_BUF messages
 *              3) Puts outgoing messages on transport ring
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: FilterXfrm                                            */
/*                                                                      */
/*      Inputs:         Pointer to World Structure                      */
/*                      Pointer to incoming TRACE message               */
/*                      SCNL index of incoming message                  */
/*                      Pointer to outgoing TRACE buffer                */
/*                                                                      */
/*      Outputs:        Message(s) sent to the output ring              */
/*                                                                      */
/*      Returns:        EW_SUCCESS on success, else EW_FAILURE          */
/* ---------------------------------------------------------------------*/
/*      Function: XfrmResetSCNL                                         */
/*                                                                      */
/*      Inputs:         Pointer to SCNL Structure                       */
/*                      Pointer to incoming TRACE message               */
/*                      Pointer to World Structure                      */
/*                                                                      */
/*      Outputs:        Changes to SCNL Structure                       */
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
#include "debias.h"

/*******                                                        *********/
/*      Structure definitions                                           */
/*******                                                        *********/


/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

         /*******************************************************
          *                   XfrmResetSCNL                     *
          *                                                     *
          *  A new span has started; (re-)initialize            *
          * information about its running average               *
          *******************************************************/
int XfrmResetSCNL( XFRMSCNL *xSCNL, TracePacket *inBuf, XFRMWORLD* xWorld )
{
  DBSCNL *pSCNL = (DBSCNL*)xSCNL;
  DBWORLD* pDb = (DBWORLD*)xWorld;
  int bytesNeeded;           /* size of window in bytes */
  int samples;               /* nbr of samples in window */
  int numTBheaders;          /* nbr of packets cached when computing initial average */
  
  /* Calc nbr samples needed for length of window, and estimate nbr of packets 
     for that many samples */
  samples = (int)(inBuf->trh2.samprate * pDb->xfrmParam->avgWindowSecs);
  if ( samples < 2 ) 
  {
    logit( "e", "debias: WARNING: AvgWindow too low to hold more than 1 sample; lengthened to hold 2\n" );
    samples = 2;
  }
  numTBheaders = (samples+inBuf->trh2.nsamp-1) / inBuf->trh2.nsamp;

  if ( samples == 0 ) 
  {
    logit( "e", "debias: packet wants window with 0 samples!" );
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
  pSCNL->awDataSize = inBuf->trh2.datatype[1]-'0';
  bytesNeeded = samples * (int)pSCNL->awDataSize;
  if ( bytesNeeded > pSCNL->awBytesAllocated ) 
  {
    if ( pSCNL->awBytesAllocated == 0 )
      pSCNL->avgWindow = (void*)calloc( samples, pSCNL->awDataSize);
    else
      pSCNL->avgWindow = (void*)realloc( pSCNL->avgWindow, bytesNeeded );
    if ( pSCNL->avgWindow == NULL )
    {
      logit( "e", "debias: failed to allocate averaging window\n" );
      return EW_FAILURE;
    }
    pSCNL->awBytesAllocated = bytesNeeded;
  }
  
  /* Reset counters/flags */
  pSCNL->awPosn = pSCNL->awFull = pSCNL->nCachedOutbufs = 0;
  pSCNL->sum = 0.0;
  
  return EW_SUCCESS;
}

int XfrmInitSCNL( XFRMWORLD* xDb, XFRMSCNL* xSCNL )
{
  DBSCNL *pSCNL = (DBSCNL*)xSCNL;
  
  pSCNL->outBufHdr = pSCNL->avgWindow = NULL;
  pSCNL->obhAllocated = pSCNL->nCachedOutbufs = pSCNL->awPosn 
      = pSCNL->awBytesAllocated = pSCNL->awSamples = pSCNL->awFull 
      = (int)(pSCNL->awDataSize = 0);
  pSCNL->sum = 0.0;

  return EW_SUCCESS;   
}

         /*******************************************************
          *                    FilterXfrm                       *
          *                                                     *
          *  A new packet to be processed has arrived; include  *
          * it in the rolling-average calculation, writing a    *
          * debiased version out once enough samples to compute *
          * the average have been processed.                    *
          *******************************************************/
int FilterXfrm (XFRMWORLD* xDb, TracePacket *inBuf, int jSta, 
                    unsigned char msgtype, TracePacket *outBuf)
{
  DBSCNL *pSCNL;
  short *waveShort, *outShort, *awShort = NULL;
  int32_t  *waveLong,  *outLong, *awLong = NULL;
  int ret, i, j, k;
  int datasize, outBufLen;
  int posn;
  DBWORLD *pDb = (DBWORLD*)xDb;
  
  ret = BaseFilterXfrm( xDb, inBuf, jSta, msgtype, outBuf );
  if ( ret != EW_SUCCESS )
  {
    return ret;
  } else 
    if (pDb->xfrmParam->debug) logit("","Back from BaseFilter\n");
  
  /* Set some useful pointers */
  waveShort = (short*)(inBuf->msg + sizeof(TRACE2_HEADER));
  waveLong  = (int32_t*) (inBuf->msg + sizeof(TRACE2_HEADER));
  outShort  = (short*)(outBuf->msg + sizeof(TRACE2_HEADER));
  outLong   = (int32_t*) (outBuf->msg + sizeof(TRACE2_HEADER));
  pSCNL = (DBSCNL*)(pDb->scnls + jSta*(pDb->scnlRecSize)); 
  
  /* Local copies of various values */
  posn = pSCNL->awPosn;
  datasize = (int)pSCNL->awDataSize;

  /* Get properly-typed pointer to window */
  if ( datasize==2 )
    awShort = (short*)(pSCNL->avgWindow);
  else
    awLong = (int32_t*)(pSCNL->avgWindow);

  for ( i=0; i<inBuf->trh2.nsamp; i++ )
  {
    /* If window in full, remove the oldest entry from sum */
    if ( pSCNL->awFull )
    {
//      int oldest_posn = (posn==pSCNL->awSamples ? 0 : posn);
      pSCNL->sum -= (datasize==2 ? awShort[posn] : awLong[posn]);
    }

    /* Add newest entry to sum & window */
    pSCNL->sum += (datasize==2 ? waveShort[i] : waveLong[i]);
    if ( datasize==2 )
    { 
      awShort[posn++] = waveShort[i];
    }
    else
    {
      awLong[posn++] = waveLong[i];
    }

    /* If window is full, remove bias from latest value */
    if ( pSCNL->awFull )
    {
      if ( datasize==2 )
      {
        short bias = (short)(pSCNL->sum / pSCNL->awSamples + 0.5);
        outShort[i] = waveShort[i] - bias;
      } else {
        int32_t bias = (int32_t)(pSCNL->sum / pSCNL->awSamples + 0.5);
        outLong[i] = waveLong[i] - bias;
      }
      if ( posn == pSCNL->awSamples )
        posn = 0;
    }
    /* If we filled window for first time, flush the cache after debiasing it */
    else if ( posn == pSCNL->awSamples )
    {
      double bias = (pSCNL->sum / pSCNL->awSamples + 0.5);
      if (pDb->xfrmParam->debug)
        logit( "", "first flush of cache (bias=%f=%f/%d)\n", bias,pSCNL->sum,pSCNL->awSamples);

      if ( pSCNL->awFull == 0 )
      {
        pSCNL->awFull = 1;
        posn = 0;
        
        /* Flush each of the caches outbufs */
        for ( k=0; k<pSCNL->nCachedOutbufs; k++ )
        {
          if (pDb->xfrmParam->debug)
            logit("","Flushing outbuf #%d (%d samples)\n", k, pSCNL->outBufHdr[k].nsamp );
          outBuf->trh2 = pSCNL->outBufHdr[k];
          for ( j=0; j<pSCNL->outBufHdr[k].nsamp; j++ )
          {
            if ( datasize==2 )
            {
              outShort[j] = (short)(awShort[posn++] - bias + 0.5);
            } else {
              outLong[j] = (int32_t)(awLong[posn++] - bias + 0.5);
            }
          }
          
          /* Ship the packet out to the transport ring */
          pDb->trcLogo.type = msgtype;
          outBufLen = pSCNL->outBufHdr[k].nsamp * datasize + sizeof(TRACE2_HEADER);
          if (tport_putmsg (&(pDb->regionOut), &(pDb->trcLogo), outBufLen, 
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
            outShort[j] = (short)(waveShort[j] - bias + 0.5);
          } else {
            outLong[j] = (int32_t)(waveLong[j] - bias + 0.5);
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

      ret = XfrmWritePacket( xDb, (XFRMSCNL*)pSCNL, msgtype, outBuf, outBufLen );
      
      if ( ret != EW_SUCCESS )
      {
        return ret;
      }
  }
  else
  {
    if (pDb->xfrmParam->debug)
      logit( "", "packet didn't fill window (%d vs %d)\n", pSCNL->awPosn, pSCNL->awSamples );
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
