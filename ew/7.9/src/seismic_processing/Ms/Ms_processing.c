       /****************************************************************
       *  Ms_processing.c                                              *
       *                                                               *
       *  This file contains the ANSI Standard C processing routines   *
       *  used by module Ms.                                           *
       *                                                               *
       *  This code is based on routines used in                       *
       *  the WCATWC Earlybird modules lpproc.                         *
       *                                                               *
       *  2011: Paul Whitmore, NOAA-WCATWC - paul.whitmore@noaa.gov    *
       *                                                               *
       ****************************************************************/

#include <stdio.h>
#include <string.h>
#include <transport.h>
#include <earthworm.h>
#include "Ms.h"

  /******************************************************************
   *                           GetLDC()                             *
   *                                                                *
   *  Determine and update moving averages of signal value          *
   *  (called LDC here).                                            *
   *                                                                *
   *  Arguments:                                                    *
   *    lNSamps     Number of samples in this packet                *
   *    WaveLong    Pointer to data array                           *
   *    pdLDC       Pointer to long term DC average                 *
   *    lFiltSamp   # samples through filter                        *
   *                                                                *
   ******************************************************************/

void GetLDC( long lNSamps, int32_t *WaveLong, double *pdLDC, long lFiltSamp )
{
   double  dSumLDC;     /* Summation of all values in this packet */
   int     i;
   
   dSumLDC = 0.;
   
/* Sum up total for this packet */
   for ( i=0; i<lNSamps; i++ )
   {
      if ( lFiltSamp == 0 ) dSumLDC  += ((double) WaveLong[i]);
      else                  dSumLDC  += ((double) WaveLong[i] - *pdLDC);
   }
	  
/* Compute new LTAs (first time through, just take average.  Adjust average
   from there */
   if ( lFiltSamp == 0 ) *pdLDC =  (dSumLDC/(double) lNSamps);
   else                  *pdLDC += (0.5 * dSumLDC/(double) lNSamps);
}

 /***********************************************************************
  *                             GetMs()                                 *
  *                                                                     *
  *  Loop through data to get MS parameters.                            *
  *                                                                     *
  *  NOTE: Only call this function if some R-window within buffer.      *                                                               *
  *                                                                     *
  *  Arguments:                                                         *
  *     Sta              Pointer to station being processed             *
  *     iNumSamp         Num samples in this packet (0 if iInit=1)      *
  *     pHStruct         Hypocenter data structure                      *
  *     Gparm            Configuration parameter structure              *
  *     iInit            0=just process last packet, 1=start anew  (be  *
  *                      careful with this, lEndData must be known if   *
  *                      iInit = 0).                                    *
  *                                                                     *
  *  Returns:                                                           *
  *     1 if new highest amplitude found, 0 otherwise                   *                                                                   *
  *                                                                     *
  ***********************************************************************/
int GetMs( STATION *Sta, int iNumSamp, HYPO *pHStruct, GPARM *Gparm, int iInit )
{
   AZIDELT Azi;
   double  dPer;          /* Period in seconds */
   double  dTime;         /* 1/1/70 time at i=0 */
   int     iReturn;       /* 0=no new mag params, 1=new mag params */
   long    i, lIndex;
   LATLON  ll;
   long    lAmp2;         /* MS Amplitude in counts */
   long    lBNum;         /* Number of samples to evaluate for MDF */
   long    lNum;          /* Number of samples to evaluate */
   long    lOldIndex;     /* Index of oldest data in array */
   long    lRIndex;       /* Buffer index of R-wave start */

   iReturn = 0;
   
/* Is this a calibrated station? */
   if ( Sta->dSens <= 0. ) return 0;

/* If this is a not a vertical, velocity station, return */
   if ( (Sta->szChannel[0] != 'B' && Sta->szChannel[0] != 'H' &&
         Sta->szChannel[0] != 'L' && Sta->szChannel[0] != 'M' ) ||
         Sta->szChannel[1] != 'H' || Sta->szChannel[2] != 'Z' ) return 0;

/* Initialize some things */
   if ( iInit == 1 )          /* Go through buffer to catch up */
   {         /* Be careful that iInit is only set to 0 when lEndData is known */
/* What is starting buffer index of the R-wave start time and oldest time? */   
      if ( Sta->dStartTime < Sta->dRStartTime ) 
      {
         lRIndex   = Sta->lSampIndexF - (long) ((Sta->dEndTime-
                     Sta->dRStartTime)*Sta->dSampRate) - 1;
/* Note that oldest time includes LP taper in case it is at start of signal */
         lOldIndex = Sta->lSampIndexF - (long) ((Sta->dEndTime-
                     Sta->dStartTime)*Sta->dSampRate) - 1 + 
                     (long) (3.*(1./Gparm->dLowCutFilter)*Sta->dSampRate);
      }
      else 
      {
         lRIndex   = Sta->lSampIndexF - Sta->lRawCircSize + 1;
         lOldIndex = Sta->lSampIndexF - Sta->lRawCircSize + 1;
      }
      while ( lRIndex   <  0 )                 lRIndex   += Sta->lRawCircSize;
      while ( lRIndex   >= Sta->lRawCircSize ) lRIndex   -= Sta->lRawCircSize; 
      while ( lOldIndex <  0 )                 lOldIndex += Sta->lRawCircSize;
      while ( lOldIndex >= Sta->lRawCircSize ) lOldIndex -= Sta->lRawCircSize; 
	  
      Sta->dAveLDCRawOrig = Sta->dAveLDC;  /* Here RawOrig = filtered */
      Sta->lSampOld       = Sta->plFiltCircBuff[lRIndex-1] -
                            (int32_t) Sta->dAveLDCRawOrig;
      Sta->lSampsPerCyc   = 0;
      Sta->lMagAmp        = 0;
      Sta->lPhase1        = 0;        /* Here, lPhase1 is the previous amp */
      Sta->lPhase2        = 0;        /* Here, lPhase2 is the highest amp */
      Sta->lPer           = 0;
   
/* How many points to evaluate? */   
      lNum = (long) ((Sta->dREndTime-Sta->dRStartTime) * Sta->dSampRate);
      dTime = Sta->dRStartTime;
      if ( Sta->dStartTime > Sta->dRStartTime )
      {
         lNum -= (long) ((Sta->dStartTime-Sta->dRStartTime) * Sta->dSampRate);
         dTime = Sta->dStartTime;
      }
      if ( Sta->dEndTime < Sta->dREndTime )
         lNum -= (long) ((Sta->dREndTime-Sta->dEndTime) * Sta->dSampRate);
	  
/* Compute the background peak-trough noise level-get index of buffer start */   
      lBNum   = (long) ((double) MS_BACKGROUND_TIME * Sta->dSampRate);
      GetMDFFilt( lOldIndex, lBNum, Sta );
   }
   else  /* Just analyze last packet */
   {     /* Be careful that iInit is only set to 0 when lEndData is known */
      lRIndex = Sta->lEndData;
      lNum    = iNumSamp;  
      dTime   = Sta->dEndTime - (double) lNum*Sta->dSampRate;
   }

/* Set i us so that MS will be computed on new packets, if needed */
   if ( Sta->dEndTime < Sta->dREndTime ) Sta->iPickStatus = 1;
      
/* Loop through data to get magnitude */
   for ( i=0; i<lNum; i++ )
   {
      lIndex = i + lRIndex;
      if ( lIndex >= Sta->lRawCircSize ) lIndex -= Sta->lRawCircSize; 
      Sta->lSampNew = Sta->plFiltCircBuff[lIndex] - (int32_t) Sta->dAveLDCRawOrig;
      
/* Check for cycle changes (at the zero crossings) */
      if ( (Sta->lSampNew <  0 && Sta->lSampOld <  0) || 
           (Sta->lSampNew >= 0 && Sta->lSampOld >= 0) )
      {                               /* No zero crossing */
         Sta->lSampsPerCyc += 1;
         if ( labs( Sta->lSampNew ) > Sta->lMagAmp ) 
             Sta->lMagAmp = labs( Sta->lSampNew );
      }
      else                            /* Zero has been crossed */
      {
         Sta->lSampsPerCyc += 1;
         dPer = (double) (Sta->lSampsPerCyc+Sta->lPer) / Sta->dSampRate;
         if ( dPer <= 1.0  ) dPer = 1.0;      /* Filter response constraint */
         if ( dPer >= 29.0 ) dPer = 29.0;
         Sta->lPer = Sta->lSampsPerCyc;
         Sta->lSampsPerCyc = 0;
         lAmp2 = Sta->lMagAmp + Sta->lPhase1;
		 
/* Should we report this cycle to PICK_RING? */		 
         if ( lAmp2 > Sta->lPhase2 &&           /* Bigger than previous? */
              dPer >= 17. && dPer <= 23. &&     /* Right Period Range? */
                                                /* High enough S:N? */
              lAmp2 > (long) (Sta->dAveMDF*Gparm->dSigNoise) &&
              Sta->iUseMe >= 0 )                /* Manually removed? */
         {       /* Convert to ground motion and save in structure */
            if ( Gparm->iDebug == 1 )
               logit( "", "%s - Report new Ms; lAmp2=%ld, S:N=%ld\n", 
                Sta->szStation, lAmp2, (long) (Sta->dAveMDF*Gparm->dSigNoise) );
            Sta->lPickIndex = atoi( pHStruct->szQuakeID );
            strcpy( Sta->szPhase, "R" );
            Sta->cFirstMotion = '?';
            Sta->dPTime = 0.;
            Sta->iUseMe = 1;
            Sta->dMSPer = dPer;
            Sta->iMSClip = 0;	    
            if ( lAmp2 > Sta->dClipLevel ) Sta->iMSClip = 1;
            Sta->dMSAmpGM = MsGroundMotion( Sta->szChannel, Sta->dSens,
                                            (long) (Sta->dMSPer+0.1), lAmp2 );
            Sta->dMSTime = dTime + (double) i*Sta->dSampRate;	    
            Sta->dMSMag = ComputeMSMag( Sta->szChannel, 0., Sta->dMSAmpGM, 
                                        Sta->dMSPer, Sta->dDelta );
            Sta->lPhase2 = lAmp2;
            ll.dLat = Sta->dLat;
            ll.dLon = Sta->dLon;
            GeoCent( &ll );
            GetLatLonTrig( &ll );
            GetDistanceAz( (LATLON *) pHStruct, &ll, &Azi );
            Sta->dDelta   = Azi.dDelta;
            Sta->dAzimuth = Azi.dAzimuth;
            iReturn = 1;
         }
         Sta->lPhase1 = Sta->lMagAmp;     /* Reset for next 1/2 cycle */
         Sta->lMagAmp = Sta->lSampNew;
      }  
      Sta->lSampOld = Sta->lSampNew;
   }
   return iReturn;
}

  /******************************************************************
   *                        PadBuffer()                             *
   *                                                                *
   *  Fill in gaps in the data with DC offset.                      *
   *  Update last data value index.                                 *
   *                                                                *
   *  Arguments:                                                    *
   *    lGapSize    Number of values to pad (+1)                    *
   *    dLDC        DC average                                      *
   *    plBuffCtr   Index tracker in main buffer                    *
   *    plBuff      Pointer to main circular buffer                 *
   *    lBuffSize   # samples in main circular buffer               *
   *                                                                *
   ******************************************************************/

void PadBuffer( long lGapSize, double dLDC, long *plBuffCtr, 
                int32_t *plBuff, long lBuffSize )
{
   long    i;
   
/* Pad the data buffer over the gap interval */
   for ( i=0; i<lGapSize-1; i++ )
   {
      plBuff[*plBuffCtr] = (int32_t) dLDC;
      *plBuffCtr += 1;
      if ( *plBuffCtr == lBuffSize ) *plBuffCtr = 0;
   }	  
}

  /******************************************************************
   *                      PatchDummyWithLP()                        *
   *                                                                *
   *  Update the dummy file with the new average Ms.                *
   *                                                                *
   *  Arguments:                                                    *
   *    pHypo       Structure with hypocenter parameters            *
   *    pszDFile    Dummy File name                                 *
   *                                                                *
   ******************************************************************/
   
int PatchDummyWithLP( HYPO *pHypo, char *pszDFile )
{
   HYPO    HypoT;       /* Temporary hypocenter data structure */

/* Read in hypocenter information from dummy file */
   if ( ReadDummyData( &HypoT, pszDFile ) == 0 ) 
   {
      logit ("t", "Failed to open DummyFile in PatchDummyWithLP\n");
      return 0;
   }
    
/* Update MS data */
   HypoT.dMSAvg = pHypo->dMSAvg;
   HypoT.iNumMS = pHypo->iNumMS;
   
/* Choose (and update) the preferred magnitude */
   GetPreferredMag( &HypoT );
   pHypo->iNumPMags = HypoT.iNumPMags;
   strcpy( pHypo->szPMagType, HypoT.szPMagType );
   pHypo->dPreferredMag = HypoT.dPreferredMag;

/* Update dummy file */
   if ( WriteDummyData( &HypoT, pszDFile, 0, 1 ) == 0 )
   {
      logit ("t", "Failed to open DummyFile in PatchDummyWithLP (2)\n");
      return 0;
   }
   return 1;
}

  /******************************************************************
   *                         PutDataInBuffer()                      *
   *                                                                *
   *  Load the data from the input buffer to the large circular     *
   *  buffer.  Update last data value index.                        *
   *                                                                *
   *  Arguments:                                                    *
   *    WaveHead    Pointer to data buffer header                   *
   *    WaveLong    Pointer to data array                           *
   *    plBuff      Pointer to main circular buffer                 *
   *    plBuffCtr   Index tracker in main buffer                    *
   *    lBuffSize   # samples in main circular buffer               *
   *                                                                *
   ******************************************************************/
void PutDataInBuffer( TRACE2_HEADER *WaveHead, int32_t *WaveLong,
                      int32_t *plBuff, long *plBuffCtr, long lBuffSize )
{
   int     i;
   
/* Copy the data buffer */
   for ( i=0; i<WaveHead->nsamp; i++ )
   {
      plBuff[*plBuffCtr] = WaveLong[i];
      *plBuffCtr += 1;
      if ( *plBuffCtr == lBuffSize ) *plBuffCtr = 0;
   }	  
}
