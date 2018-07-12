
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
 * ewdrift_filt.c: drift-filter
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
#include "ewdrift.h"

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
  return EW_SUCCESS;
} 

int XfrmInitSCNL( XFRMWORLD* xEwi, XFRMSCNL* xSCNL )
{
  INTSCNL *pSCNL = (INTSCNL*)xSCNL;

  /* base setting */
  pSCNL->inEndtime =0.0;

  return EW_SUCCESS;   
}

/*
 * Get the INTSCNL for the station index
 * In:  pEwi pointer to the INTWORLD structure
 *      jSta the station index
 * Returns the INTSCNL or NULL if none
 */
static INTSCNL* getINTSCNL(INTWORLD *pEwi, int jSta) {
	if (jSta < 0 || jSta >= pEwi->nSCNL) {
		return NULL;
	}
	return ((INTSCNL*)(pEwi->scnls))+jSta;
}

/*
 * Compare the SCNLs.
 * In:  scnl1 the first scnl
 *      scnl2 the second scnl
 * Return 0 if the SCNLs match, non-zero otherwise.
 */
static int CompareSCNL(INTSCNL* scnl1, INTSCNL* scnl2) {
	return BaseCompareSCNL((XFRMSCNL*)scnl1, (XFRMSCNL*)scnl2);
}

/*
 * Write the packet
 * In:  arg the XFRMWORLD
 *      scnl1 the first scnl
 *      scnl2 the second scnl
 *      msgtype the message type
 *      outBuf the output buffer
 * Returns EW_SUCCESS if successful
 */
static int WritePacket(XFRMWORLD* arg, INTSCNL* scnl1, INTSCNL* scnl2, unsigned char msgtype, TracePacket* outBuf) {
	int i, outBufLen;
	TracePacket* inBuf1 = GetInBufForMsgBuf(scnl1->inMsgBuf);
	TracePacket* inBuf2 = GetInBufForMsgBuf(scnl2->inMsgBuf);
	int datasize = GetDataSize(inBuf1);

	if (datasize == 2)
	{
		short* waveShort1 = (short*)GetWave(inBuf1);
		short* waveShort2 = (short*)GetWave(inBuf2);
		short* outShort = (short*)GetWave(outBuf);

		for (i = 0; i < inBuf1->trh2.nsamp; i++)
		{
			outShort[i] = waveShort2[i] - waveShort1[i];
		}

	} else {
		int32_t* waveLong1 = (int32_t*)GetWave(inBuf1);
		int32_t* waveLong2 = (int32_t*)GetWave(inBuf2);
		int32_t* outLong = (int32_t*)GetWave(outBuf);

		for (i = 0; i < inBuf1->trh2.nsamp; i++)
		{
			outLong[i] = waveLong2[i] - waveLong1[i];
		}
	}

	/* Ship the packet out to the transport ring */
	outBuf->trh2 = inBuf1->trh2;
	outBufLen = inBuf1->trh2.nsamp * datasize + sizeof(TRACE2_HEADER);
	return XfrmWritePacket(arg, (XFRMSCNL*)scnl1, msgtype, outBuf, outBufLen);
}

/*
 * Write the packet if needed
 * In:  arg the XFRMWORLD
 *      scnl1 the first scnl
 *      scnl2 the second scnl
 *      msgtype the message type
 *      outBuf the output buffer
 * Returns EW_SUCCESS if successful
 */
static int WritePacketIfNeeded(XFRMWORLD* arg, INTSCNL* scnl1, INTSCNL* scnl2, unsigned char msgtype, TracePacket* outBuf) {
	  int ret;

	  if (scnl1 == NULL || scnl2 == NULL) {
		  return EW_SUCCESS;
	  }

	  if (scnl1->inMsgBuf == scnl2->inMsgBuf) {
		  INTWORLD *pEwi = (INTWORLD*)arg;
		  logit("e", "%s FilterXfrm: both input message buffers are the same\n", pEwi->mod_name);
		  return EW_FAILURE;
	  }

	  if (CompareSCNL(scnl1, scnl2) != 0)  // if SCNLs do not match
	  {
		  return EW_SUCCESS;
	  }

	  ret = WritePacket(arg, scnl1, scnl2, msgtype, outBuf);

	  if ( ret != EW_SUCCESS )
	  {
		  return ret;
	  }
	  return EW_SUCCESS;
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
  int ret;
  INTWORLD *pEwi = (INTWORLD*)arg;
  INTSCNL *pSCNL = ((INTSCNL*)(pEwi->scnls))+jSta;
  INTSCNL *scnl1, *scnl2;

/*
  fprintf(stderr, "DEBUG: Processing %s.%s.%s.%s from SCNLs array element %d\n",
	pSCNL->inSta, pSCNL->inChan, pSCNL->inNet, pSCNL->inLoc, jSta );
*/
  ret = BaseFilterXfrm( arg, inBuf, jSta, msgtype, outBuf );

  if ( ret != EW_SUCCESS )
  {
    return ret;
  }
  
  scnl1 = pSCNL;
  scnl2 = getINTSCNL(pEwi, pSCNL->nextSta);

  ret = WritePacketIfNeeded(arg, scnl1, scnl2, msgtype, outBuf);
  if ( ret != EW_SUCCESS )
  {
    return ret;
  }

  scnl1 = getINTSCNL(pEwi, pSCNL->prevSta);
  scnl2 = pSCNL;

  ret = WritePacketIfNeeded(arg, scnl1, scnl2, msgtype, outBuf);
  if ( ret != EW_SUCCESS )
  {
    return ret;
  }
  
  return EW_SUCCESS;
}
