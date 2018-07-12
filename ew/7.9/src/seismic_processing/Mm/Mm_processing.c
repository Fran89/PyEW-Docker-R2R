       /****************************************************************
       *  Mm_processing.c                                              *
       *                                                               *
       *  This file contains the ANSI Standard C processing routines   *
       *  used by module Mm.                                           *
       *                                                               *
       *  This code is based on routines used in                       *
       *  the original WCATWC Earlybird module Mm.                     *
       *                                                               *
       *  The original routines perform FFTs, filtering, and           *
       *  the various corrections associated with the computations of  *
       *  Mm.  The routines were provided to the WC/ATWC by Weinstein  *
       *  of PTWC, but originally were from Okal and were programmed   *
       *  in FORTRAN.  They were converted to C by Whitmore at WC/ATWC *
       *  in 2001.                                                     *   
       *                                                               *
       *  Some of these routines also are used by the threads to fill  *
       *  buffers.                                                     *
       *                                                               *
       *  2012: Paul Whitmore, NOAA-WCATWC - paul.whitmore@noaa.gov    *
       *                                                               *
       ****************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <transport.h>
#include <earthworm.h>
#include "Mm.h"

/* External Global Variables
   *************************/
extern double  dGroup[NUM_ATTEN][NUM_PATH_PER];           /* Group velocities */
extern double  dPeriod[NUM_PATH_PER];           /* Periods in path corr. file */
extern double  dQQ[NUM_ATTEN][NUM_PATH_PER];                  /* Attenuations */
extern int     iCrust[NUM_LAT_BOUNS][NUM_LON_BOUNS];      /* Crustal sections */

  /******************************************************************
   *                         CompMm()                               *
   *                                                                *
   *  This function orchestrates computation of Mm.  It calls       *
   *  several routines which perform FFT, response application, and *
   *  compute corrections necessary to produce an Mm.  This code    *
   *  needs regional path corrections supplied from elsewhere.      *
   *  Also, the trace data should be split out and supplied as      *
   *  an int32 array.                                                 *
   *                                                                *
   *  January, 2012: Removed MMSTUFF structure                      *
   *  January, 2005: Use frequencies up to 400s dependent on Mwp.   *
   *  July, 2003: Don't use frequencies below 170s.                 *
   *                                                                *
   *  Arguments:                                                    *
   *    lNSamps     Number of samples to evaluate                   *
   *    plData      Data array                                      *
   *    pSta        Pointer to STATION structure with all info      *
   *    dLat        Epicenter geocentric latitude                   *
   *    dLon        Epicenter geocentric longitude                  *
   *    dMag        Quake's magnitude to this point                 *
   *    iFlag       1->Compute Mm, 2->Compute background Mm         *
   *                                                                *
   *  Returns:                                                      *
   *    double      Mm magnitude                                    *
   *                                                                *
   ******************************************************************/

double CompMm( long lNSamps, int32_t *plData, STATION *pSta,
               double dLat, double dLon, double dMag, int iFlag )
{
   AZIDELT az;                      /* Distance/azimuth structure */
   double  dAbsVal;                 /* Displacement for each period */
   double  dAtten;                  /* Attenuation for path corr. */
   double  dAvgMm;                  /* Average Mm */
   double  dAx, dBx;                /* Path correction variables */
   double  dCdju, dP1, dP2;         /* Path correction factors */
   double  dDf;                     /* Temp FFT variable */
   double  dFqCnt;                  /* Freq increments for spectra */
   static  double  dFrMax = 1./50., dFrMin;    /* Freq. window (Hz) */
   double  dMm[MAX_FFT_PER];        /* Mm for each per */
   double  dMmMax;                  /* Max Mm over all periods */
   double  dMwTemp;                 /* Temp Mm for correction to Mw */
   double  dPathCorrT[MAX_FFT_PER]; /* Path correction for each per */
   double  dPathCorr[NUM_ATTEN];    /* Crustal zones travelled by surface
                                       waves from source to station */
   double  dPathInc;                /* Stepping increment for path correction*/
   double  dPer[MAX_FFT_PER];       /* Periods (s) examined */
   double  dResp;                   /* Response factor */
   double  dStdDev;                 /* Std Dev. of Mms */
   double  dSourceCorr[MAX_FFT_PER];/* Source correction for each per */
   static  double  dSPer0 = 1.8209, dSa0 = 3.7411, dSa1 = 0.42861,
                   dSa2 = -0.8332, dSa3 = 1.6163;/* Shallow source parameters */
   double  dSumMm;                  /* Mm summation */
   double  dSumMmStdDev;            /* Mm summation for std. dev.*/
   static  double  dTBeg = 0.05, dTEnd = 0.1;/* Taper length at start & end */
   double  dTheta;                  /* Source correction variable */
   double  dU;                      /* Velocity for this period */
   float   fData[MAX_FFT];          /* Data array converted to floats */
   int     i, j;
   int     iCrustType;              /* Average crust properties for 10x10
                                       degree sections of earth: 1-4=ocean,
                                       5=shiels, 6=mountains, 7=trench */
   int     iDist;                   /* Epicentral distance rounded up */
   int     iFFTOrder;               /* Order of FFT */
   int     iLat, iLon;              /* Indices relating to REGIONS file */
   int     iMaxPerIndex;            /* Period index of maximum Mm */
   int     iNFMax, iNFMin;          /* FFT bounding indices */
   int     iNk;                     /* Path correction variables */
   int     iNPath;                  /* # steps in path correction */
   int     iNumFFTPts;              /* # pts. in FFT (=2**iFFTOrder) */
   int     iTest;                   /* Error test */
   int     jFqCnt, ju, k;           /* Counters */
   LATLON  ll1, ll2, ll2GG;         /* Lat/lon structures */
   static  fcomplex z[MAX_FFT];     /* Data array converted to complex */
   fcomplex zres;                   /* Spectral amp. with response removed */

/* Set minimum frequency based on Mwp
   **********************************/
   if      ( dMag > 7.5 ) dFrMin = 1./410.;
   else if ( dMag < 6.6 ) dFrMin = 1./103.;
   else                   dFrMin = 1./171.;     
                                              
/* Make a few checks on the data
   *****************************/
   if ( (pSta->szChannel[0] != 'B'   &&
         pSta->szChannel[0] != 'H' ) ||
         pSta->szChannel[1] != 'H'   ||
         pSta->szChannel[2] != 'Z' )
   {
      logit( "", "%s %s wrong channel type for Mm\n", pSta->szStation, 
                                                      pSta->szChannel );
      return 0.0;
   }
   if ( (double) lNSamps/pSta->dSampRate < MM_MIN_TIME )
   {
      logit( "", "%s not enough samples - %ld\n", pSta->szStation, lNSamps );
      return 0.0;
   }
   if ( pSta->iNPole == 0 && pSta->iNZero == 0 )
   {
      logit( "", "%s no poles or zeros\n", pSta->szStation );
      return 0.0;
   }
   if ( pSta->dDelta < 15. )
   {
      logit( "", "%s too close for Mm - %lf\n",
             pSta->szStation, pSta->dDelta );
      return 0.0;
   }

/* Compute FFT order from number of points
   ***************************************/
   myfnd( lNSamps, &iNumFFTPts, &iFFTOrder );
   if ( iNumFFTPts == 0 || iFFTOrder == 0 )
   {
      logit( "", "%s FFT=0\n", pSta->szStation );
      return 0.0;
   }
   if ( iNumFFTPts > MAX_FFT )
   {
      logit( "", "%s FFT > MAX_FFT (%d)\n", pSta->szStation, iNumFFTPts );
      return 0.0;
   }
   
/* Convert data array to real (for compatibility with PTWC)
   ********************************************************/
   for ( j=0; j<lNSamps; j++ )
   {
      if ( j == MAX_FFT ) break;
      fData[j] = (float) plData[j];
   }
  
/* Attempt to remove any DC Offset
   *******************************/
   mydtr( fData, lNSamps );
	
/* Cosine taper the data array
   ***************************/
   mytpr( fData, lNSamps, dTBeg, dTEnd );
	
/* Fill in complex array; first initialize it
   ******************************************/
   for ( j=0; j<iNumFFTPts; j++ )
   {
      z[j].r = (float) 0.;
      z[j].i = (float) 0.;
   }
   for ( j=0; j<lNSamps; j++ )
   {
      z[j].r = fData[j];
      z[j].i = (float) 0.;
   }
	  
/* Perform FFT
   ***********/
   coolb( iFFTOrder, z, -1. );
   dDf = 1. / ((1./pSta->dSampRate)*(double) iNumFFTPts);
   if ( dDf <= 0. )
   {
      logit( "", "%s dDf <= 0., sr=%lf, iNumFFTPts=%d\n", pSta->szStation,
             pSta->dSampRate, iNumFFTPts );
      return 0.0;
   }

/* Defining bounding indices in FFT series
   ***************************************/
   iNFMin = (int) (dFrMin/dDf + 1.5);
   iNFMax = (int) (dFrMax/dDf + 1.5);
   
/* Compute the contribution of crustal sections to the path correction
   *******************************************************************/
   ll1.dLat = dLat;
   ll1.dLon = dLon;
   for ( i=0; i<NUM_ATTEN; i++ ) dPathCorr[i] = 0.;
   iDist = (int) (pSta->dDelta + 1.0);
   dPathInc = pSta->dDelta / (double) iDist;
   iNPath = iDist + 1;
   for ( j=1; j<iNPath+1; j++ )
   {
      az.dAzimuth = pSta->dAzimuth * RAD;
      az.dDelta = dPathInc * ((double) j-1) * RAD;
      PointToEpi( &ll1, &az, &ll2 );
      GeoGraphic( &ll2GG, &ll2 );
      while (ll2GG.dLon < 0.0) ll2GG.dLon += 360.0;
      iLon = (int) (ll2GG.dLon / 10.0);
      iLat = (int) ((90.0-ll2GG.dLat) / 10.0);
      iCrustType = iCrust[iLat][iLon];
      if ( j == 1 || j == iNPath ) dPathCorr[iCrustType-1] += .5*dPathInc;
      else                         dPathCorr[iCrustType-1] += dPathInc;
   }
	
/* Loop Over All Frequencies & compute Mm
   **************************************/
   dMmMax = 0.;
   ju = 0;
   dSumMm = 0.;
   dSumMmStdDev = 0.;                                                                
   iMaxPerIndex = -1;
   pSta->iMwNumPers = 0;
   for ( jFqCnt=iNFMin; jFqCnt<iNFMax+1; jFqCnt++ )
   {
      dFqCnt = ((float) (jFqCnt-1)) * dDf;
      if ( dFqCnt < dFrMin || dFqCnt > dFrMax ) continue; 
      dPer[ju] = 1./dFqCnt;
      dResp = 1.;
      resgeo( dFqCnt, pSta->dAmp0, pSta->iNZero, pSta->iNPole,
              pSta->zZeros, pSta->zPoles, &zres );
      dResp = (double) Cabs( zres );
      dResp /= 100.;        /* Convert from counts per meter to counts per cm */
      if ( dResp == 0. ) continue;
      dAbsVal = (double) (Cabs( z[jFqCnt-1] )) / dResp;
      dAbsVal = fabs( dAbsVal*(1./pSta->dSampRate) );
      if ( ju < MAX_SPECTRA )
      {
         if ( iFlag == 1 ) pSta->dMwAmpSp[ju]   =  dAbsVal;
         if ( iFlag == 2 ) pSta->dMwAmpSpBG[ju] =  dAbsVal;
         pSta->dMwPerSp[ju] =  dPer[ju];
         pSta->iMwNumPers   += 1;
      }
      dAbsVal *= 10000.;
      if ( dAbsVal <= 0. )
      {
         logit( "", "%s amplitude <= 0. (%lf) \n", pSta->szStation, dAbsVal );
         return 0.0;
      }
	  
/* Compute Source Correction
   *************************/
      if ( dPer[ju] <= 0. )
      {
         logit( "", "%s Period <= 0., ju=%d\n", pSta->szStation, ju );
         return 0.0;
      }
      dTheta = log10( dPer[ju] ) - dSPer0;
      dSourceCorr[ju] = dSa3*dTheta*dTheta*dTheta + dSa2*dTheta*dTheta;
      dSourceCorr[ju] = dSourceCorr[ju] + dSa1*dTheta + dSa0;
		
/* Compute Distance Correction
   ***************************/
      mysert( dPer[ju], dPeriod, NUM_PATH_PER, &dAx, &dBx, &iNk, &iTest );
      if ( iTest != 1 )
      {
         dAtten = 0.;
         for ( k=0; k<NUM_ATTEN; k++ )
         {
            dU = dGroup[k][iNk-1]*dAx + dGroup[k][iNk]*dBx;
            if ( dU == 0. || dQQ[k][iNk-1] == 0. || dQQ[k][iNk] == 0. )
               continue;
            dAtten += PI * dFqCnt * dPathCorr[k] * DEGTOKM *
                     (dAx/dQQ[k][iNk-1] + dBx/dQQ[k][iNk]) / dU;
         }		 
         dP1 = exp( dAtten );
         dP2 = pSta->dDelta * PI / 180.0;
         dP2 = (dP2 > 0.0) ? dP2 : -dP2;
         dCdju = sqrt( sin( dP2 ) ) * dP1;
         if ( dCdju <= 0. )
         {
            logit( "", "%s cDju <= 0.\n", pSta->szStation );
            return 0.0;
         }
         dPathCorrT[ju] = log10( dCdju );
		 
/* Compute Mm at period indexed ju
   *******************************/
         dMm[ju] = log10( dAbsVal ) + dPathCorrT[ju] + dSourceCorr[ju] - 0.90;
         dSumMm += dMm[ju];
         dSumMmStdDev = dSumMmStdDev + dMm[ju]*dMm[ju];
         if ( dMmMax < dMm[ju] ) iMaxPerIndex = ju;   /* Save maximums */
         if ( dMmMax < dMm[ju] ) dMmMax = dMm[ju];
         dMwTemp = dMm[ju]/1.5 + 2.6;
         if ( iFlag == 1 ) 
            { if ( ju < MAX_SPECTRA ) pSta->dMwMagSp[ju] =  dMwTemp; }
         if ( iFlag == 2 ) 
            { if ( ju < MAX_SPECTRA ) pSta->dMwMagSpBG[ju] =  dMwTemp; }
         ju++;
         continue;
      }
      ju++;
   }
   
/* Compute average Mm for this station
   ***********************************/   
   if ( ju > 0 )
   {
      dAvgMm = dSumMm / (double) ju;
      dStdDev = sqrt((dSumMmStdDev - ((double) ju*dAvgMm*dAvgMm)) / (double)ju);
   }
   else
   {
      dAvgMm = 0.;
      dStdDev = 0.;
   }
   if ( iMaxPerIndex >= 0 && iMaxPerIndex < MAX_FFT_PER)
      pSta->dPerMax = dPer[iMaxPerIndex];
   else pSta->dPerMax = 0.;
   logit( "", "%s Average Mm %lf +- %lf Maximum Mm %lf at T= %lf\n",
          pSta->szStation, dAvgMm, dStdDev, dMmMax, pSta->dPerMax);
	
/* Return the maximum Mm
   *********************/
   return dMmMax; 
}

/* FFT function */
void coolb( int nn, fcomplex zData[], double dSign )
{
   double  dTheta, dSinTheta, dSinR, dSinI, dWR, dWI;
   int     i, j, mm;       /* Based on 0 */
   int     iStep;
   int     m, n, mMax;     /* Based on 1 */
   fcomplex zTemp;
   
   n = (int) (pow( 2., (double) nn ) + 0.05);
   j = 0;
   for ( i=0; i<n; i++ )
   {
      if ( i-j < 0 )
      {
         zTemp.r = zData[j].r;   
         zTemp.i = zData[j].i;   
         zData[j].r = zData[i].r;
         zData[j].i = zData[i].i;
         zData[i].r = zTemp.r;   
         zData[i].i = zTemp.i;   
      }
      m = n/2;                      
Lp1:  if ( (j+1)-m > 0 )          
      {
         j = j - m;           
         m = m / 2;               
         if ( m-2 >= 0 ) goto Lp1;
      }
      j = j + m;              
   }
   mMax = 2;
TotalCheck:
   if ( (mMax/2 - n) < 0 )
   {
      iStep = 2*mMax;
      dTheta = dSign*TWOPI/(double) mMax;
      dSinTheta = sin( dTheta/2. );
      dSinR = -2.*dSinTheta*dSinTheta;
      dSinI = sin( dTheta );
      dWR = 1.;
      dWI = 0.;
      for ( mm=0; mm<mMax/2; mm++ )
      {
         for ( i=mm; i<n; i+=iStep/2 )
         {
            j = i + mMax/2;
            zTemp.r = (float) (dWR*zData[j].r - dWI*zData[j].i);
            zTemp.i = (float) (dWR*zData[j].i + dWI*zData[j].r);
            zData[j].r = zData[i].r - zTemp.r;
            zData[j].i = zData[i].i - zTemp.i;
            zData[i].r = zData[i].r + zTemp.r;
            zData[i].i = zData[i].i + zTemp.i;
         }
         zTemp.r = (float) dWR;
         dWR = dWR*dSinR - dWI*dSinI + dWR;
         dWI = dWI*dSinR + zTemp.r*dSinI + dWI;
      }
      mMax = iStep;
      goto TotalCheck;
   }
}

  /******************************************************************
   *                           FillBuff()                           *
   *                                                                *
   *  Fill up the array to send to CompMm.                          *
   *                                                                *
   *  Arguments:                                                    *
   *    plNSamps    Number of samples in this packet                *
   *    plBuff      Pointer to data array                           *
   *    pSta        Pointer to STATION structure with all info      *
   *                                                                *
   *  Return:                                                       *
   *    1 if buffer filled ok                                       *
   *    -1 if data gap observed                                     *
   *                                                                *
   ******************************************************************/
int FillBuff( long *plNSamps, int32_t *plBuff, STATION *pSta )
{
   int     i, k;
   int     lStart;    /* Starting buffer index in circbuff */
   int     lTIndex;   /* Temp index mapped to circ buff */

/* Compute Starting index in Circular buffer */
   lStart = pSta->lSampIndexR - (long) ((pSta->dEndTime-pSta->dRStartTime)*
            pSta->dSampRate);
   while ( lStart < 0 ) lStart += pSta->lRawCircSize;
   *plNSamps = (long) ((pSta->dREndTime-pSta->dRStartTime) * pSta->dSampRate);
   if ( *plNSamps > MMBUFFER_SIZE      ) *plNSamps = MMBUFFER_SIZE;
   if ( *plNSamps > pSta->lRawCircSize ) *plNSamps = pSta->lRawCircSize;
   for ( i=0; i<*plNSamps; i++ )
   {
      lTIndex = lStart + i;
      if ( lTIndex >= pSta->lRawCircSize )
           lTIndex -= pSta->lRawCircSize;
      plBuff[i] = pSta->plRawCircBuff[lTIndex];
/* Check to see if there are gaps */
      if ( plBuff[i] == 0 && i >= NUM_AT_ZERO )
         for ( k=i-NUM_AT_ZERO+1; k<=i; k++ )
            if ( plBuff[k] != 0 ) break;
            else if ( k == i )
/* Too large of a gap, so no Mm */
            {
               logit( "", "%s-Data gap in FillBuff\n", pSta->szStation);
               return -1;
            }
   }
   return 1;
}

  /******************************************************************
   *                           FillBuffBG()                         *
   *                                                                *
   *  Fill up the array to send to CompMm for background comps.     *
   *                                                                *
   *  Arguments:                                                    *
   *    plNSamps    Number of samples in this packet                *
   *    plBuff      Pointer to data array                           *
   *    pSta        Pointer to STATION structure with all info      *
   *    dOTime      Origin time in 1/1/70 seconds                   *
   *    iBGTime     Length of time (seconds) to use for background  *
   *                                                                *
   *  Return:                                                       *
   *    1 if buffer filled ok                                       *
   *    -1 if data gap observed                                     *
   *                                                                *
   ******************************************************************/
int FillBuffBG( long *plNSamps, int32_t *plBuff, STATION *pSta, double dOTime,
                int iBGTime )
{
   int     i, k;
   int     lStart;    /* Starting buffer index in circbuff */
   int     lTIndex;   /* Temp index mapped to circ buff */

/* Compute Starting index in Circular buffer */
   lStart = pSta->lSampIndexR - (long) 
    ((pSta->dEndTime-dOTime-(double) iBGTime)*pSta->dSampRate);
   while ( lStart < 0 ) lStart += pSta->lRawCircSize;
   *plNSamps = (long) ((double) iBGTime * pSta->dSampRate);
   if ( *plNSamps > MMBUFFER_SIZE      ) *plNSamps = MMBUFFER_SIZE;
   if ( *plNSamps > pSta->lRawCircSize ) *plNSamps = pSta->lRawCircSize;
   for ( i=0; i<*plNSamps; i++ )
   {
      lTIndex = lStart + i;
      if ( lTIndex >= pSta->lRawCircSize )
           lTIndex -= pSta->lRawCircSize;
      plBuff[i] = pSta->plRawCircBuff[lTIndex];
/* Check to see if there are gaps */
      if ( plBuff[i] == 0 && i >= NUM_AT_ZERO )
         for ( k=i-NUM_AT_ZERO+1; k<=i; k++ )
            if ( plBuff[k] != 0 ) break;
            else if ( k == i )
/* Too large of a gap, so no Mm */
            {
               logit( "", "%s-Data gap in FillBuffBG\n", pSta->szStation);
               return -1;
            }
   }
   return 1;
}

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
	  
/* Detrend array by suppressing average value */
void mydtr( float pfData[], int iNSamps )
{
   double  dSum;              /* Summation variable, later average */
   int     j;
   
   dSum = 0.;
   for ( j=0; j<iNSamps; j++ ) dSum = dSum + (double) pfData[j];
   dSum = dSum / (double) iNSamps;
   for ( j=0; j<iNSamps; j++ ) pfData[j] = pfData[j]-(float) dSum;
}

/* Given number iNSamps, Finds piFFTOrder such that  2**piFFTOrder is next
   smallest power of 2 (piFFTOrder is order of FFT to process iNSamps points).
   Also returns piNumFFTPts=2**piFFTOrder. */
void myfnd( int iNSamps, int *piNumFFTPts, int *piFFTOrder )
{
   double  dPower[MAX_FFT_ORDER], dTemp1, dTemp2;
   int     i, iTemp;
   
   dPower[0] = 1.;                                  
   for ( i=1; i<25; i++ ) dPower[i] = 2.*dPower[i-1];
   mysert( (double) iNSamps, dPower, MAX_FFT_ORDER, &dTemp1, &dTemp2,
            piFFTOrder, &iTemp );
   *piNumFFTPts = (int) (dPower[*piFFTOrder] + 0.5);
}
	  
/* Inserts dX into array dPower(iMax); returns index piIndex, and barycentral
   coeffs. pdAx, pdBx. piError =1 if screw-up.  dPower can be increasing or
   decreasing. Returns pdAx=dX-dPower(j+1); pdBx=dPower(j)-dX, so that  interp.
   goes dX=dA(piIndex)*pdAx+dPower(piIndex+1)*pdBx */
void mysert( double dX, double dPower[], int iMax, double *pdAx, double *pdBx,
             int *piIndex, int *piError )
{
   double  dCx;
   int     j;                         /* Based on 0 */

/* If array is increasing */
   *piError = 0;
   if ( dPower[iMax-1] >= dPower[0] )
   {
      if ( dX < dPower[0] )      
      {
         *piIndex = 0;
         goto Error;
      }
      if ( dX > dPower[iMax-1])  
      {
         *piIndex = iMax-1;
         goto Error;      }
      for ( j=0; j<iMax; j++ )
         if ( dX >= dPower[j] && dX <= dPower[j+1]) break;
      *piIndex = j+1;
      *pdBx = dPower[j]-dX;
      *pdAx = dX-dPower[j+1];
      dCx = *pdAx+*pdBx;
      *pdAx = *pdAx/dCx;
      *pdBx=*pdBx/dCx;
      return;
   }
   else                   /* Array is decreasing */
   {
      if ( dX > dPower[0] )
      {
         *piIndex = 0;
         goto Error;
      }
      if ( dX < dPower[iMax-1] )
      {
         *piIndex = iMax-1;
         goto Error;
      }
      for ( j=0; j<iMax; j++ )
         if ( dX < dPower[j] && dX >= dPower[j+1]) break;
      *piIndex = j+1;
      *pdBx = dPower[j]-dX;
      *pdAx = dX-dPower[j+1];
      dCx = *pdAx+*pdBx;
      *pdAx = *pdAx/dCx;
      *pdBx = *pdBx/dCx;
      return;
   }
   Error: 
   *piError = 1;
   *pdAx = 0.;
   *pdBx = 0.;
}

/* Cosine taper of array A of N points, by B at beginning and E at end */
void mytpr( float pfData[], int iNSamps, double dBeg, double dEnd )
{
   double  dAng, dCs;
   int     i;
   long    M1;          /* # samps to taper at beginning */
   long    M3;          /* # samps to taper at end */
   long    M4;          /* # starting sample for end taper (based on 0) */

   M1 = (long) ((double) iNSamps*dBeg + 0.5);
   if ( M1 > 0 )
   {
      dAng = PI / (double) M1;
      for ( i=0; i<M1; i++ )
      {
         dCs = (1. - cos( (double) (i+1)*dAng )) / 2.;
         pfData[i] = pfData[i] * (float) dCs;
      }
   }
   M3 = (long) ((double) iNSamps*dEnd + 0.5);
   M4 = iNSamps-M3;
   if ( M3 > 0 )
   {
      dAng = PI / (double) M3;
      for ( i=M4; i<iNSamps; i++ )
      {
         dCs = (1. - cos( (double) (i-iNSamps)*dAng ))/2.;
         pfData[i] = pfData[i] * (float) dCs;
      }
   }
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
   *                      PatchDummyWithMm()                        *
   *                                                                *
   *  Update the dummy file with the new average Mm (Mw).           *
   *                                                                *
   *  Arguments:                                                    *
   *    pHypo       Structure with hypocenter parameters            *
   *    pszDFile    Dummy File name                                 *
   *                                                                *
   ******************************************************************/
   
int PatchDummyWithMm( HYPO *pHypo, char *pszDFile )
{
   HYPO    HypoT;       /* Temporary hypocenter data structure */

/* Read in hypocenter information from dummy file */
   if ( ReadDummyData( &HypoT, pszDFile ) == 0 ) 
   {
      logit ("t", "Failed to open DummyFile in PatchDummyWithMm\n");
      return 0;
   }
    
/* Update Mw data */
   HypoT.dMwAvg = pHypo->dMwAvg;
   HypoT.iNumMw = pHypo->iNumMw;
   
/* Choose (and update) the preferred magnitude */
   GetPreferredMag( &HypoT );
   pHypo->iNumPMags = HypoT.iNumPMags;
   strcpy( pHypo->szPMagType, HypoT.szPMagType );
   pHypo->dPreferredMag = HypoT.dPreferredMag;

/* Update dummy file */
   if ( WriteDummyData( &HypoT, pszDFile, 0, 1 ) == 0 )
   {
      logit ("t", "Failed to open DummyFile in PatchDummyWithMm (2)\n");
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

/* zres returns T(s)=a0*(s-z(1))*...*(s-z(m))/(s-p(1))*...*(s-p(n)),
   where p, z, and s are complex.  s = cmplx(0.,2*pi*fqc).
   This is the regular IRIS convention */
void resgeo( double dFqc, double dA0, int iNumZero, int iNumPole,
             fcomplex zZero[], fcomplex zPole[], fcomplex *pzres )
{
   int     j;               /* Based on 0 */
   fcomplex zD, zS;

   *pzres = Complex ( 0.0, 0.0 );
   zD = Complex( 1., 0. );
   zS = Complex( 0., (2.*PI*dFqc) );
	
/* COMPUTE THE CONTRIBUTION OF THE ZEROES, IF ANY */
   if ( iNumZero > 0 )
      for ( j=0; j<iNumZero; j++ ) zD = Cmul( zD, Csub( zS, zZero[j] ) );
 
/* COMPUTE THE CONTRIBUTION OF THE POLES */
   if ( iNumPole > 0 )
   {
      for ( j=0; j<iNumPole; j++ ) zD = Cdiv( zD, Csub( zS, zPole[j] ) );
      *pzres = Cmul( Complex( dA0, 0. ), zD );           /* Displacement */
   }
}

  /******************************************************************
   *                       ReadRegion()                             *
   *                                                                *
   *  Read regionalized period, velocity, and attenuation file.     *
   *                                                                *
   *  Arguments:                                                    *
   *    pszRegionFile File name of regionalized path data           *
   *                                                                *
   *  Returns:                                                      *
   *    int         1=OK, 0=no good                                 *
   *                                                                *
   ******************************************************************/
   
int ReadRegion( char *pszRegionFile )
{
   FILE    *hFile;
   int     i, j, iTemp;

/* Open regional path correction data file */
   hFile = fopen( pszRegionFile, "r" );   
   if ( hFile == NULL )
   {
      logit( "", "%s file could not open\n", pszRegionFile );
      return 0;
   }
   
/* Read in periods with attenuations and group velocity */   
   for ( i=0; i<NUM_PATH_PER; i++ )
   {
      fscanf( hFile, " %d", &iTemp );
      dPeriod[i] = (double) iTemp;	  
   }

/* Read indexed crustal regions */   
   for ( i=0; i<NUM_LAT_BOUNS; i++ )
   {
      for ( j=0; j<NUM_LON_BOUNS; j++ )
         fscanf( hFile, " %d", &iCrust[i][j] );
      fscanf( hFile, "\n" );
   }
   
/* Read regionalized group vel. and Q for each period/region */
   for ( i=0; i<NUM_PATH_PER; i++ )
   {
      fscanf( hFile, " %d", &iTemp );
      for ( j=0; j<NUM_ATTEN; j++ )
      {	  
         fscanf( hFile, " %lf %d", &dGroup[j][i], &iTemp );
         dQQ[j][i] = (double) iTemp;	  
      }
      fscanf( hFile, "\n" );
   }
   fclose( hFile );
   return 1;
}
