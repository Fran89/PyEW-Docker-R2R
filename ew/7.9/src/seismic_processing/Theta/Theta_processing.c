       /****************************************************************
       *  Theta_processing.c                                           *
       *                                                               *
       *  This file contains the ANSI Standard C processing routines   *
       *  used by module Theta.                                        *
       *                                                               *
       *  This code is based on routines used in                       *
       *  the original Earlybird module EngMom.                        *
       *                                                               *
       *  The routines were provided by Richard Luckett of the BrGS.   *
       *                                                               *
       *  2012: Paul Whitmore, NOAA-WCATWC - paul.whitmore@noaa.gov    *
       *                                                               *
       ****************************************************************/

#include <stdio.h>
#include <string.h>                                   
#include <math.h>                                
#include <transport.h>                 
#include <earthworm.h>
#include "Theta.h"

  /******************************************************************
   *                       CompTheta()                              *
   *                                                                *
   *  This function orchestrates computation of Theta.  It calls    *
   *  routines which perform the necessary energy computations.     *
   *                                                                *
   *  May, 2012: Created this stand-alone function from original    *
   *             engmom source code.                                *
   *                                                                *
   *  Arguments:                                                    *
   *    pHypo       Pointer to Hypo structure with quake data       *
   *    pSta        Pointer to STATION structure with stn info      *
   *    iAuto       1-called from auto loc, 0-called from eqcentral *
   *    iNumSta     Number stations in pSta array                   *
   *    pszLocFilePath Path for P-data files from auto sol'ns       *
   *    iDebug      1 if write all debug statements                 *
   *    pszDiskPFile File to get P-data from based on manual loc.   *
   *    dMinDelta   Minimum distance of interest for Theta          *
   *    dMaxDelta   Maximum distance of interest for Theta          *
   *    iWindowLength Window length (sec) after P to get energy     *
   *    dFiltLo     Low end of bandpass filter                      *
   *    dFiltHi     High end of bandpass filter                     *
   *    iFromDisk   1-> this data is from a disk read.              *
   *                                                                *
   *  Returns:                                                      *
   *    int         1 if ok; 0 if problem                           *
   *                                                                *
   ******************************************************************/

int CompTheta( HYPO *pHypo, STATION pSta[], int iAuto, int iNumSta, 
               char *pszLocFilePath, int iDebug, char *pszDiskPFile, 
               double dMinDelta, double dMaxDelta, int iWindowLength, 
               double dFiltLo, double dFiltHi, int iFromDisk )
{
   AZIDELT azidelt;          /* Distance/azimuth structure */
   double  dData[MAX_THETABUFF_SIZE];   /* Time series */
   double  dEndTime;         /* Local end time - different for disk and buff */
   double  dOldestTime = 0.0;  /* 1/1/70 time of oldest data in buffer */
   HYPO    HypoT;            /* Temp structure */
   int     i, k;
   int     iNum;             /* Number of samples in dData */
   LATLON  ll2;              /* lat/lon structure */
   long    lStartSamp;       /* Starting index in buffer for window */
   long    lStartTime;       /* Start time of window used */
   long    lTIndex;          /* Temporary index */
   char    szFile[128];      /* File name to obtain P data from */

/* Get current picks and hypo information from loc file if from auto. */
   if ( iAuto == 1 )      /* Started automatically */
   {
      for ( i=0; i<iNumSta; i++ ) InitP( &pSta[i] );
      strcpy( szFile, pszLocFilePath );      
      strcat( szFile, pHypo->szQuakeID );
      strcat( szFile, ".dat" );
      if ( ReadPTimeFile( pSta, szFile, iNumSta, &HypoT ) == 0 ) 
      {
         logit( "", "Problem reading %s in CompTheta\n", szFile );
         return 0;        
      }
      if ( iDebug ) logit( "", "Read picks from %s\n", szFile );
   }
	
/* Otherwise, fill STATION array with info from disk
   file header (use o-time disk file). */
   else if ( iAuto == 0 )
   {
      logit( "t", "Theta started from EQCentral - hypo from dummy\n" );
      if ( ReadPTimeFile( pSta, pszDiskPFile, iNumSta, &HypoT ) == 0 )
      {
         logit( "", "Problem reading %s in CompTheta\n", pszDiskPFile );
         return 0;
      }
      if ( iDebug ) logit( "", "Read picks from %s\n", pszDiskPFile );
   }

/* Loop through all picks and set up data */
   for ( i=0; i<iNumSta; i++ )
   {
/* Find dEndtime.  If from a disk read, must use dDataEndTime. */
      if ( iFromDisk == 1 ) dEndTime = pSta[i].dDataEndTime;
      else                  dEndTime = pSta[i].dEndTime;

/* Only consider stations with Mwp and PPick (this discriminates low S:N) */
      if ( pSta[i].dMwpMag < 1.0 ) continue;
      if ( pSta[i].dPTime  < 1.0 ) continue;
      if ( iDebug )
         logit( "", "%s %s %s %s Pick; dMwpMag=%.1f\n", pSta[i].szStation, 
          pSta[i].szChannel, pSta[i].szNetID, pSta[i].szLocation, 
          pSta[i].dMwpMag );

/* Only want stations within right distance range */
      ll2.dLat = pSta[i].dLat;
      ll2.dLon = pSta[i].dLon;
      GeoCent( &ll2 );
      GetLatLonTrig( &ll2 );
      GetDistanceAz( (LATLON *) pHypo, &ll2, &azidelt );
      pSta[i].dDelta   = azidelt.dDelta;
      pSta[i].dAzimuth = azidelt.dAzimuth;
      if ( pSta[i].dDelta < dMinDelta || pSta[i].dDelta > dMaxDelta )
      {
         if ( iDebug ) logit( "", "%s delta=%lf not in range\n", 
                        pSta[i].szStation, pSta[i].dDelta );
         continue;
      }

/* Check sample rate is realistic */
      if ( pSta[i].dSampRate <= 0. || 
           pSta[i].dSampRate > (double) MAX_SAMPLE_RATE )
      {
         if ( iDebug )
            logit( "", "%s dodgy sample rate=%lf\n",
                   pSta[i].szStation, pSta[i].dSampRate );
         continue;
      }

/* See if we have response information */
      if ( pSta[i].iNPole == 0 && pSta[i].iNZero == 0 )
      {
         if ( iDebug )
            logit( "", "%s has no response information\n", pSta[i].szStation );
         continue;
      }

/* Compute time of oldest data in buffer */
      if ( iAuto == 1 )                   /* Real time data */
         dOldestTime = dEndTime + (1./pSta[i].dSampRate) -
                       ((double) pSta[i].lRawCircSize/pSta[i].dSampRate);
      else if ( iAuto == 0 )              /* Disk data */
         dOldestTime = pHypo->dOriginTime - (double)(iWindowLength*TAPER_WIDTH);	       

/* Skip if there is no data */
      if ( dOldestTime <= 0. || dEndTime <= 0. )
      {
         if ( iDebug ) logit( "", "%s has no data\n", pSta[i].szStation );
         continue;
      }

/* Start the window including the taper before P so we don't taper arrival */
      lStartTime = (long) (pSta[i].dPTime - iWindowLength*TAPER_WIDTH);

/* Skip if the data is already passed */
      if ( lStartTime < dOldestTime )
      {
         if ( iDebug )
            logit( "", "%s pick prior to bufferstart=%lf, dEndTime=%lf\n",
                   pSta[i].szStation, dOldestTime, dEndTime );
         continue;
      }

/* Is there data in this buffer covering the required window? */
      if ( pSta[i].dPTime + iWindowLength > dEndTime )
      {
         if ( iDebug ) logit( "", "%s not enough data\n", pSta[i].szStation);
         continue;
      }

/* Fill up the time series buffer */
      lStartSamp = (long) ((dEndTime-(double) lStartTime) * 
                            pSta[i].dSampRate);
      lStartSamp = pSta[i].lSampIndexR - lStartSamp;
      while ( lStartSamp < 0 ) lStartSamp += pSta[i].lRawCircSize;
/* Take post data taper into account for num samps */
      iNum = (int) (pSta[i].dSampRate*(double) iWindowLength * 
                   (1.+TAPER_WIDTH));
/* Don't let buffer overflow */
      if ( iNum >= MAX_THETABUFF_SIZE ) iNum = MAX_THETABUFF_SIZE;
      for ( k=0; k<iNum; k++ )
      {
         lTIndex = lStartSamp + k;
         if ( lTIndex >= pSta[i].lRawCircSize )
            lTIndex -= pSta[i].lRawCircSize;
         dData[k] = (double) pSta[i].plRawCircBuff[lTIndex];
         if ( k >= MAX_THETABUFF_SIZE ) break;   /* Don't let buffer overflow */
      }

/* Compute the energy */
      if ( GetEnergy( dFiltLo, dFiltHi, iNum, dData, lStartTime, &pSta[i] ) < 0 )
      {
         logit( "et", "%s problem in GetEnergy\n", pSta[i].szStation );
         continue;
      }

/* Calculate moment from magnitude */
      if ( !strcmp( pHypo->szPMagType, "w" ) )
      {
         pSta[i].dThetaMoment = pow( 10, (1.5*pHypo->dMwAvg + 9.1) );
         logit( "", "Use Mm for moment - %lf\n", pHypo->dMwAvg );
      }
      else                                 
      {
         if ( !strcmp( pHypo->szPMagType, "l" ) )
            pSta[i].dThetaMoment = pow( 10, (1.5*pSta[i].dMlMag + 9.1) );
         else if ( !strcmp( pHypo->szPMagType, "wp" ) )
            pSta[i].dThetaMoment = pow( 10, (1.5*pSta[i].dMwpMag + 9.1) );
         else if ( !strcmp( pHypo->szPMagType, "b" ) )
            pSta[i].dThetaMoment = pow( 10, (1.5*pSta[i].dMbMag + 9.1) );
         else if ( !strcmp( pHypo->szPMagType, "S" ) )
            pSta[i].dThetaMoment = pow( 10, (1.5*pSta[i].dMSMag + 9.1) );
         else 
            pSta[i].dThetaMoment = pow( 10, (1.5*pSta[i].dMwpMag + 9.1) );
      }

/* Calculate theta */
      pSta[i].dTheta = log10( pSta[i].dThetaEnergy/pSta[i].dThetaMoment );
      if ( iDebug )
         logit( "t","%s: use=%d mag=%.1f energy=%.2e Nm moment=%.2e Nm theta=%.1f\n",
          pSta[i].szStation, pSta[i].iUseMe, pSta[i].dMwpMag,
          pSta[i].dThetaEnergy, pSta[i].dThetaMoment, pSta[i].dTheta );
   }  /* End of loop over picks */
 
   return 1;
}

 /***********************************************************************
  *                           coolb()                                   *
  *                                                                     *
  *  FFT program.                                                       *
  *                                                                     *
  *    nn    - number of points in data as a power of 2                 *
  *    zData - data in array of complex numbers                         *
  *    dSign - sign of exponential -1 for forward fft 1 for inverse     *
  *                                                                     *
  *  NB: After taking inverse FFT divide output by 2**nn                *
  *                                                                     *
  ***********************************************************************/

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
         zTemp.r    = zData[j].r;
         zTemp.i    = zData[j].i;
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
      iStep     = 2*mMax;
      dTheta    = dSign*TWOPI/(double) mMax;
      dSinTheta = sin( dTheta/2. );
      dSinR = -2.*dSinTheta*dSinTheta;
      dSinI = sin( dTheta );
      dWR   = 1.;
      dWI   = 0.;
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
         dWR     = dWR*dSinR - dWI*dSinI + dWR;
         dWI     = dWI*dSinR + zTemp.r*dSinI + dWI;
      }
      mMax = iStep;
      goto TotalCheck;
   }
}

 /***********************************************************************
  *                          GetEnergy()                                *
  *                                                                     *
  *  Find the total seismic energy of a trace.                          *
  *                                                                     *
  *  Function Arguments:                                                *
  *   dFiltLo     Low end of bandpass filter                            *
  *   dFiltHi     High end of bandpass filter                           *
  *   iNum        Number of data points in dData                        *
  *   dData       Time series array                                     *
  *   lStartTime  Time of first sample in seconds since 1970            *
  *   pSta        STATION Structure of interest                         *
  *                                                                     *
  *  Returns:    0 if OK or -1 if problem                               *
  *                                                                     *
  ***********************************************************************/

int GetEnergy( double dFiltLo, double dFiltHi, int iNum, double *dData, 
               long lStartTime, STATION *pSta )
{
   double   dAbsVal;               /* Spectral amplitude */
   double   dFreq;                 /* Frequency in Hz */
   double   dFreqInt;              /* Interval between samples of FFT */
   double   dGenRadCoef;           /* F^g*P - generalized radiation pattern coefficient */
   double   dMean;                 /* Mean value of waveform removed as an offset */
   double   dSpreading;            /* R^2   - Geometrical spreading factor */
   double   dTstar;                /* t^*   - frequency dependent attenuation factor */
   int      i;
   int      iFFTOrder;             /* Power of 2 that iNfft corresponds to */
   int      iNfft;                 /* Number of points in fft */
   static fcomplex zCor[MAX_FFT];  /* Complex array for corrected data */
   static fcomplex zDat[MAX_FFT];  /* Complex array for raw data */
   fcomplex zRes;                  /* Complex response factor */

/* Constants taken from Lomax et al. 2007 */
   double dDensity     = 2600.; /* kg/m3 */
   double dPvel        = 5000.; /* m/s */
   double dMeanRadCoef = 4./15.;/* <(F^P)^2> - mean square radiation coef */
   double dPSpart      = 15.6;  /* q - partitioning of energy between P and S */

/* Remove offset and apply taper prior to fft */
   dMean = 0.;
   for ( i=0; i<iNum; i++ ) dMean += dData[i]/(double) iNum;
   for ( i=0; i<iNum; i++ ) dData[i] -= dMean;
   HanningTaper( dData, iNum, TAPER_WIDTH );
   
/* Compute FFT order and number of points */
   iFFTOrder = (int) (log10( iNum )/log10( 2 ) + 0.99);
   iNfft     = (int) pow( 2, iFFTOrder );
   if ( iNfft > MAX_FFT )
   {
      logit( "et", "Problem in GetEnergy: Timeseries too long for FFT "
                   "iNfft=%d\n", iNfft );
      return -1;
   }
   dFreqInt  = pSta->dSampRate/(double) iNfft;

/* Fill in complex arrays */
   for ( i=0; i<iNum; i++ )
   {
      zDat[i].r = (float) dData[i];
      zDat[i].i = (float) 0.;
      zCor[i].r = (float) 0.;
      zCor[i].i = (float) 0.;
   }
   for ( i=iNum; i<iNfft; i++ )
   {
      zDat[i].r = (float) 0.;
      zDat[i].i = (float) 0.;
      zCor[i].r = (float) 0.;
      zCor[i].i = (float) 0.;
   }

/* Perform FFT */
   coolb( iFFTOrder, zDat, -1. );

/* Loop over spectrum calculating energy flux */
   pSta->dThetaEnergy = 0.;
   for ( i=1; i<iNfft/2; i++ )
   {
/* Frequency in Hz of this point in spectrum */
      dFreq = i * dFreqInt;
      if ( dFreq <= dFiltLo ) continue;
      if ( dFreq >= dFiltHi ) break;

/* Frequency dependent attenuation from Newman and Okal 1998 */
      if ( dFreq < 0.1 )    dTstar = 0.9 - 0.1*log10( dFreq );
      else if ( dFreq < 1 ) dTstar = 0.5 - 0.5*log10( dFreq );
      else                  dTstar = 0.5 - 0.1*log10( dFreq );
	 
/* Apply velocity response */
      resgeo( dFreq, pSta, &zRes );
      zCor[i] = Cdiv( zDat[i], zRes );
	 
/* Get amplitude value for this frequency */
      dAbsVal = (double) (Cabs( zCor[i] ))/pSta->dSampRate;
	 
/* Add energy for this point, correcting for attenuation */
      pSta->dThetaEnergy += dAbsVal * dAbsVal * exp( 2*PI*dFreq*dTstar );
   }

/* Geometric spreading factor - distance in metres squared */
   dSpreading = DEGTOKM*pSta->dDelta*1000*DEGTOKM*pSta->dDelta*1000;

/* Distance dependent generalised radiation coefficient from Newman and 
   Okal 1998 */
   dGenRadCoef = 1.171 - 7.271e-3*pSta->dDelta + 6.009e-5*
                 pSta->dDelta*pSta->dDelta;

/* Get radiated energy - factor 1/4 to account for site amplification */
   pSta->dThetaEnergy *= (dDensity*dPvel*(1.+dPSpart)*dSpreading*
                         (dMeanRadCoef/dGenRadCoef));
   if ( pSta->dThetaEnergy <= 0. )
   {
      logit( "et", "Problem in GetEnergy: Energy=%lf\n", pSta->dThetaEnergy );
      return -1;
   }
   return 0;
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

 /***********************************************************************
  *                           HanningTaper()                            *
  *  Input:  dData  - array of samples.                                 *
  *          iNum   - number of samples.                                *
  *          dWidth - taper width.                                      *
  *                                                                     *
  *          Applies a Hanning taper to the data.                       *
  *                                                                     *
  ***********************************************************************/

void HanningTaper( double *dData, int iNum, double dWidth )
{
   double    f0, f1, omega, factor;
   int       i, ntaper;

   ntaper = (int) (iNum*dWidth);
   f0 = f1 = 0.5;
   omega   = PI/ntaper;
   for ( i=0; i<ntaper; i++ )
   {
      factor          = f0 - f1*cos( omega*i );
      dData[i]        = dData[i]*factor;
      dData[iNum-1-i] = dData[iNum-1-i]*factor;
   }
   return;
}

 /***********************************************************************
  *                               LogAlarm()                            *
  *                                                                     *
  * Log all abnormally low theta values to Alarm file for show in       *
  * Summary.                                                            *
  *                                                                     *
  * Arguments:                                                          *
  *  pszAlarmFile     File to write alarms                              *
  *                                                                     *
  ***********************************************************************/

void LogAlarm( char *pszAlarmFile )
{
   FILE *hFile;                                          /* Alarm File handle */

/* Try opening File
   ****************/
   if ( ( hFile = fopen (pszAlarmFile, "w" ) ) == NULL )     /* Wouldn't open */
      logit( "t", "%s could not be opened\n", pszAlarmFile );
   else                                                      /* Open OK       */
   {                                           /* Write alarm message to file */
      fprintf( hFile, "%s\n", "Theta Alarm" );
      logit( "t", "Log to %s - Theta Alarm\n", pszAlarmFile );
      fclose( hFile );
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
      plBuff[*plBuffCtr] = (long) dLDC;
      *plBuffCtr += 1;
      if ( *plBuffCtr == lBuffSize ) *plBuffCtr = 0;
   }	  
}

  /******************************************************************
   *                      PatchDummyWithTheta()                     *
   *                                                                *
   *  Update the dummy file with the new average Theta.             *
   *                                                                *
   *  Arguments:                                                    *
   *    pHypo       Structure with hypocenter parameters            *
   *    pszDFile    Dummy File name                                 *
   *                                                                *
   ******************************************************************/
   
int PatchDummyWithTheta( HYPO *pHypo, char *pszDFile )
{
   HYPO    HypoT;       /* Temporary hypocenter data structure */

/* Read in hypocenter information from dummy file */
   if ( ReadDummyData( &HypoT, pszDFile ) == 0 ) 
   {
      logit ("t", "Failed to open DummyFile in PatchDummyWithTheta\n");
      return 0;
   }
    
/* Update Theta data */
   HypoT.dTheta    = pHypo->dTheta;
   HypoT.dThetaSD  = pHypo->dThetaSD;
   HypoT.iNumTheta = pHypo->iNumTheta;

/* Update dummy file */
   if ( WriteDummyData( &HypoT, pszDFile, 0, 1 ) == 0 )
   {
      logit ("t", "Failed to open DummyFile in PatchDummyWithTheta (2)\n");
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

 /***********************************************************************
  *                          resgeo()                                   *
  *                                                                     *
  * Input: dFreq    - frequency want response for.                      *
  *        Sta      - Structure with station response etc               *
  *        pzRes    - pointer for returning response                    *
  *                                                                     *
  *  Sets zres T(s)=a0*(s-z(1))*...*(s-z(m))/(s-p(1))*...*(s-p(n))      *
  *  where; p, z, and s are complex.                                    *
  *  s = cmplx(0.,2*pi*fqc).                                            *
  *  This is the regular IRIS convention.                               *
  *                                                                     *
  ***********************************************************************/

void resgeo( double dFreq, STATION *Sta, fcomplex *pzRes )
{
   fcomplex zD, zS;
   int      j;

   *pzRes = Complex( 0.0, 0.0 );
   zD     = Complex( 1., 0. );
   zS     = Complex( 0., (2.*PI*dFreq) );

/* Compute the contribution of the zeroes, if any. */
   if ( Sta->iNZero > 0 )
      for ( j=0; j<Sta->iNZero; j++ )
         zD = Cmul( zD, Csub( zS, Sta->zZeros[j] ) );

/* Compute the contribution of the poles. */
   if ( Sta->iNPole > 0 )
   {
      for ( j=0; j<Sta->iNPole; j++ )
         zD = Cdiv( zD, Csub( zS, Sta->zPoles[j] ) );
      *pzRes = Cmul( Complex( Sta->dAmp0, 0. ), zD );
   }
}

