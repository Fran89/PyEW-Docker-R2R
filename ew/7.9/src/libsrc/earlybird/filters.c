  /**********************************************************************
   *                              filters.c                             *
   *                                                                    *
   * These functions provide bandpass filtering capabilities using an   *
   * IIR filter.  These were provided by Bob Cessaro of PTWC and        *
   * converted to c at the WC&ATWC by Whitmore in 1997.  The main       *
   * function to call from another program is FilterPacket.             *
   * This is set up to filter real-time data coming in in packets.      *
   * This code was converted to use in Earthworm for the pick_wcatwc    *
   * module in 12/2000 by Whitmore.                                     *
   *                                                                    *
   **********************************************************************/
                                                       
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <earthworm.h>
#include <transport.h>
#include "earlybirdlib.h"
#define  XNUM 3


     /**************************************************************
      *                        apiir()                             *
      *                                                            *
      * Apply IIR filter to data sequence.  The filter is assumed  *
      * to be stored as second order sections.  Filtering is in    *
      * place.  Zero-phase filtering (forward and reverse) is an   *
      * option. X1, etc. values retained for sectional filtering.  *
      * Since there may be several sectional filters calling this  *
      * function at various times, up to XNUM versions of x1, ...  *
      * are saved.  The passed value iNum tells this function      *
      * which version of x1... to use.                             *
      *                                                            *
      * Arguments:                                                 *
      *  WaveLong        Data array (longs)                        *
      *  nsamp           Number of samples in packet to filter     *
      *  iFRFilter       0=forward only, 1=forward and reverse     *
      *  Sta             Station data structure                    *
      *  sn              Filter numerator coefs (zeroes)           *
      *  sd              Filter denominator coefs (poles)          *
      *  iNumSects       Number of IIR filter sections             *
      *                                                            *
      **************************************************************/
void apiir( int32_t *WaveLong, long nsamp, int iFRFilter, STATION *Sta,
            double sn[], double sd[], int iNumSects )
{
   int     j, jptr;
   double  input, output;
   int32_t i;

/* Apply filter sample-by-sample */
   for ( i=0; i<nsamp; i++ )
   {
      jptr = 0;
      input = (double) WaveLong[i];
      for ( j=0; j<iNumSects; j++ )
      {
         secord( &output, &Sta->dFiltY1[j], &Sta->dFiltY2[j], 
                 sd[jptr+1], sd[jptr+2], input, &Sta->dFiltX1[j], 
                 &Sta->dFiltX2[j], sn[jptr], sn[jptr+1], sn[jptr+2] );
         jptr += 3;
         input = output;
      }
      WaveLong[i] = (int32_t) output;
      Sta->lFiltSamps += 1;      /* Keep track of samples evaluated for taper */
      if ( Sta->lFiltSamps == 1000000000 )
         Sta->lFiltSamps = 10000;                  /* Avoid integer overflows */
   }

/* The filter can be applied forward and reverse.  Forward only should be
   used in real-time packet processing */   
   if ( iFRFilter )
      for ( i=nsamp-1; i>=0; i-- )
      {
         jptr = 0;
         input = (double) WaveLong[i];
         for ( j=0; j<iNumSects; j++ )
         {
            secord (&output, &Sta->dFiltY1[j], &Sta->dFiltY2[j], 
                    sd[jptr+1], sd[jptr+2], input, &Sta->dFiltX1[j], 
                    &Sta->dFiltX2[j], sn[jptr], sn[jptr+1], sn[jptr+2]);
            jptr += 3;
            input = output;
         }
         WaveLong[i] = (int32_t) output;
      }
}

     /**************************************************************
      *                        apiirG()                            *
      *                                                            *
      * Apply IIR filter to data sequence.  The filter is assumed  *
      * to be stored as second order sections.  Filtering is in    *
      * place.  Zero-phase filtering (forward and reverse) is an   *
      * option. X1, etc. values retained for sectional filtering.  *
      * Since there may be several sectional filters calling this  *
      * function at various times, up to XNUM versions of x1, ...  *
      * are saved.  The passed value iNum tells this function      *
      * which version of x1... to use.                             *
      *                                                            *
      * January, 2013: Modified code so that some pre-data is      *
      *  examined when scrolling through data.                     *
      *                                                            *
      * Arguments:                                                 *
      *  lData           Data array (longs)                        *
      *  lNSamps         Number of samples in packet to filter     *
      *  iFRFilter       0=forward only, 1=forward and reverse     *
      *  lIndex          lData index to start filter               *
      *  sn              Filter numerator coefs (zeroes)           *
      *  sd              Filter denominator coefs (poles)          *
      *  iNumSects       Number of IIR filter sections             *
      *  iStaIndex       Station index                             *
      *  iZero           1->zero out arrays, 0->don't              *
      *  lMaxIndex       Max size of buffer                        *
      *  iLP             1->LP; 0->BB                              *
      *  Sta             Station data structure                    *
      *  lDataB          BB Data array (longs)                     *
      *  lPreSamps       # samps to evaluate before time of intrest*
      *                                                            *
      **************************************************************/
void apiirG( int32_t lData[], long lNSamps, int iFRFilter, long lIndex,
             double sn[], double sd[], int iNumSects, int iStaIndex, int iZero,
             long lMaxIndex, int iLP, STATION *Sta, int32_t lDataB[],
             long lPreSamps  )
{
   int     j, jptr;
   double  input, output;
   long    lTemp, i, ir;

   if ( iZero ) 
   {
      if ( iLP == 0 )
      {
         zero( Sta->dFiltX1, iNumSects );
         zero( Sta->dFiltX2, iNumSects );
         zero( Sta->dFiltY1, iNumSects );
         zero( Sta->dFiltY2, iNumSects );
      }
      else if ( iLP == 1 )
      {
         zero( Sta->dFiltX1LP, iNumSects );
         zero( Sta->dFiltX2LP, iNumSects );
         zero( Sta->dFiltY1LP, iNumSects );
         zero( Sta->dFiltY2LP, iNumSects );
      }
   }
   for ( i=0; i<lNSamps+lPreSamps; i++ )
   {
      jptr = 0;
      lTemp = i + lIndex - lPreSamps;
      if ( lTemp <  0 )         lTemp += lMaxIndex;
      if ( lTemp >= lMaxIndex ) lTemp -= lMaxIndex;
      if ( lPreSamps == 0 ) input = (double) lData[lTemp];
      else                  input = (double) lDataB[lTemp];
      for ( j=0; j<iNumSects; j++ )
      {
         if ( iLP == 0 )
            secord( &output, &Sta->dFiltY1[j], &Sta->dFiltY2[j], 
                    sd[jptr+1], sd[jptr+2], input, &Sta->dFiltX1[j], 
                    &Sta->dFiltX2[j], sn[jptr], sn[jptr+1], sn[jptr+2] );
         else if ( iLP == 1 )
            secord( &output, &Sta->dFiltY1LP[j], &Sta->dFiltY2LP[j], 
                    sd[jptr+1], sd[jptr+2], input, &Sta->dFiltX1LP[j], 
                    &Sta->dFiltX2LP[j], sn[jptr], sn[jptr+1], sn[jptr+2] );
         jptr += 3;
         input = output;
      }
      if ( i >= lPreSamps ) 
         lData[lTemp] = (int32_t) output;
   }
   if ( iFRFilter )/*iFRFilter should be FALSE when filter applied in sections*/
   {
      if ( iLP == 0 )
      {
         zero( Sta->dFiltX1, iNumSects );
         zero( Sta->dFiltX2, iNumSects );
         zero( Sta->dFiltY1, iNumSects );
         zero( Sta->dFiltY2, iNumSects );
      }
      else if ( iLP == 1 )
      {
         zero( Sta->dFiltX1LP, iNumSects );
         zero( Sta->dFiltX2LP, iNumSects );
         zero( Sta->dFiltY1LP, iNumSects );
         zero( Sta->dFiltY2LP, iNumSects );
      }
      for ( i=0; i<lNSamps; i++ )
      {
         jptr = 0;
         ir = lNSamps - 1 - i;
         lTemp = ir + lIndex;
         if ( lTemp >= lMaxIndex ) lTemp -= lMaxIndex;
         if ( lTemp < 0 ) lTemp += lMaxIndex;
         input = (double) lData[lTemp];
         for ( j=0; j<iNumSects; j++ )
         {
            if ( iLP == 0 )
               secord( &output, &Sta->dFiltY1[j], &Sta->dFiltY2[j], 
                       sd[jptr+1], sd[jptr+2], input, &Sta->dFiltX1[j], 
                       &Sta->dFiltX2[j], sn[jptr], sn[jptr+1], sn[jptr+2] );
            else if ( iLP == 1 )
               secord( &output, &Sta->dFiltY1LP[j], &Sta->dFiltY2LP[j], 
                       sd[jptr+1], sd[jptr+2], input, &Sta->dFiltX1LP[j], 
                       &Sta->dFiltX2LP[j], sn[jptr], sn[jptr+1], sn[jptr+2] );
            jptr += 3;
            input = output;
         }
      lData[lTemp] = (int32_t) output;
      }
   }
}

     /**************************************************************
       *                        bilin2()                            *
      *                                                            *
      * Transforms an analog filter to a digital filter via the    *
      * bilinear transformation.  Assumes both are stored as second*
      * order sections.  The transformation is done in place.      *
      *                                                            *
      * Arguments:                                                 *
      *  sn              Filter numerator coefs (zeroes)           *
      *  sd              Filter denominator coefs (poles)          *
      *  iNumSects       Number of IIR filter sections             *
      *                                                            *
      **************************************************************/
	  
void bilin2( double sn[], double sd[], int iNumSects )
{
   double  a0, a1, a2, scale;
   int     i, iptr;

   iptr = 0;
   for ( i=0; i<iNumSects; i++ )
   {
      a0 = sd[iptr];
      a1 = sd[iptr+1];
      a2 = sd[iptr+2];
      scale = a2+a1+a0;
      sd[iptr] = 1.;
      sd[iptr+1] = (2. * (a0-a2)) / scale;
      sd[iptr+2] = (a2 - a1 + a0) / scale;

      a0 = sn[iptr];
      a1 = sn[iptr+1];
      a2 = sn[iptr+2];
      sn[iptr] = (a2+a1+a0) / scale;
      sn[iptr+1] = (2. * (a0-a2)) / scale;
      sn[iptr+2] = (a2 - a1 + a0) / scale;
      iptr += 3;
   }
}

     /**************************************************************
      *                        bupoles()                           *
      *                                                            *
      * Compute Butterworth poles for low pass filter.             *
      *                                                            *
      * Arguments:                                                 *
      *  p               Poles - contains 1 from each complex      *
      *                   conjugate pair and all real poles        *
      *  type            S - single real                           *
      *                  C - complex conjugate pair                *
      *  nps             number of second order sections           *
      *  iOrder          Number of desired poles                   *
      *                                                            *
      **************************************************************/
void bupoles( fcomplex p[], char type[], int *nps, int iOrder )
{
   double  angle;
   int     half, k;

   half = (int) ((double) iOrder / 2.);

/* Test for odd order and add pole at -1 */
   *nps = 0;
   if ( 2*half < iOrder )
   {
      p[0].r = -1.;
      p[0].i = 0.;
      type[0] = 'S';
      *nps = 1;
   }

   for ( k=0; k<half; k++ )
   {
      angle = PI * (0.5 + (double) (2*(k+1)-1) / (double) (2*iOrder));
      *nps += 1;
      p[*nps-1].r = (float) cos( angle );
      p[*nps-1].i = (float) sin( angle );
      type[*nps-1] = 'C';
   }
}

     /**************************************************************
      *                       FilterPacket()                       *
      *                                                            *
      * This function takes an incoming packet of data and filters *
      * it based on the cutoffs passed in as arguments.  This      *
      * function is set up for real-time packets.  A value for     *
      * tapering is also passed to this function.                  *
      *                                                            *
      * April, 2012: Added Nyquist check.                          *                                                           *
      *                                                            *
      * Arguments:                                                 *
      *  WaveLong        Array of int32_t data values              *
      *  Sta             Station data array                        *
      *  lNumSamp        Number of samples                         *
      *  dSampRate       Signal sample rate in samples per second  *
      *  Gparm           Configuration parameter structure         *
      *  dFiltLow        Low end of pass band (Hz)                 *
      *  dFiltHigh       High end of pass band (Hz)                *
      *  dTaperTime      Time (sec) to taper signal                *
      *                                                            *
      **************************************************************/

void FilterPacket( int32_t *WaveLong, STATION *Sta, long lNumSamp,double dSampRate,
                   double dFiltLow, double dFiltHigh, double dTaperTime )
{
   double  dFiltHighLocal;        /* Temp value for function */
   int	   iNumSects;             /* Number of IIR filter sections */
   long    lNumToTaper;           /* Number of samples in tapering */
   double  sn[MAX_FILT_SECTIONS*FILTER_ORDER],
           sd[MAX_FILT_SECTIONS*FILTER_ORDER]; 

/* Adjust High Cut Filter if at or beyond Nyquist */
   dFiltHighLocal = dFiltHigh;
   if ( Sta->dSampRate / dFiltHighLocal <= 2. ) 
      dFiltHighLocal = (Sta->dSampRate/2.) * 0.95;

/* Compute number of samples to taper for this signal */	  
   lNumToTaper = (long) (dSampRate*dTaperTime + 0.0001);
   if ( lNumToTaper < 10 ) lNumToTaper = 10;
	  
/* Apply taper if within first lNumToTaper samples */
   if ( Sta->lFiltSamps < lNumToTaper )
      Taper( WaveLong, Sta, lNumSamp, 0, lNumToTaper );
		   
/* Get proper coefficients */
   GetFilterCoefs( dSampRate, dFiltLow, dFiltHighLocal, sd, sn, &iNumSects );

/* Remove DC from signal */
//   for ( l=lStartThisTime; l<(lStartThisTime+j); l++ )
//   {
//      lTemp = l;
//      if ( lTemp >= pStaArray->lRawCircSize ) 
//           lTemp -= pStaArray->lRawCircSize;
//      pStaArray->plFiltCircBuff[lTemp] -= (int32_t) pStaArray->dAveLDCRaw;
//   }
				
/* Apply filter */
   apiir( WaveLong, lNumSamp, 0, Sta, sn, sd, iNumSects );
}

     /**************************************************************
      *                       FilterPacketG()                      *
      *                                                            *
      * This function takes an incoming packet of data and filters *
      * it based on the cutoffs passed in as arguments.  This      *
      * function is set up for non-real-time packets.  A value for *
      * tapering is also passed to this function.  When 0, no      *
      * tapering is performed.                                     *
      *                                                            *
      * January, 2013: Updated so that when old data is patched    *
      *  in array, only the patched data is filtered and not       *
      *  entire array.                                             *
      * April, 2012: Added Nyquist check.                          *    
      * January, 2012: Replaced CIRC_BUFF_SIZE with lRawCircSize   *    
      *                                                            *
      * Arguments:                                                 *
      *  pStaArray       Station data array                        *
      *  iIndex          Index of STATION structure to filter      *
      *  dFiltLow        Low end of pass band (Hz)                 *
      *  dFiltHigh       High end of pass band (Hz)                *
      *  dTaperTime      Time (sec) to taper signal                *
      *  iLP             0->SP Filter; 1->LP Filter                *
      *  iFlag           1->Use full array; 2->Use partial array   *
      *                                                            *
      **************************************************************/
int FilterPacketG( STATION *pStaArray, int iIndex, double dFiltLow, double
                   dFiltHigh, double dTaperTime, int iLP, int iFlag )
{
   double  dFiltHighLocal;      /* Temp value for function */
   long    i, j, k, iCnt, l;
   static  int  iNumSects;      /* Number of IIR filter sections */
   long    lNumToFilter;        /* Number of samples to run thru filter */
   long    lNumToTaper;         /* Number of samples in tapering */
   long    lSamp;               /* Starting sample in array to filter */
   long    lStartThisTime;      /* Index to start tapering/filtering at */
   long    lTemp, lTemp2;
   static  double sn[MAX_FILT_SECTIONS*3], sd[MAX_FILT_SECTIONS*3]; 

/* Adjust High Cut Filter if at or beyond Nyquist */
   dFiltHighLocal = dFiltHigh;
   if ( pStaArray->dSampRate / dFiltHighLocal <= 2. ) 
      dFiltHighLocal = (pStaArray->dSampRate/2.) * 0.95;
/* Starting index to filter and taper */
   if ( iFlag == 1 ) lSamp = pStaArray->lIndex;
   else              lSamp = pStaArray->lStartFilterIndex;
/* Compute number of samples to filter */
   if ( iFlag == 1 )
      lNumToFilter = (long) ((pStaArray->dEndTime - pStaArray->dStartTime +
                      0.0001)*pStaArray->dSampRate) + 1;
   else 
      lNumToFilter = pStaArray->lNumToFilter;
   if ( lNumToFilter > pStaArray->lRawCircSize ) 
      lNumToFilter = pStaArray->lRawCircSize;
      
/* Get proper filter coefficients */
   GetFilterCoefs( pStaArray->dSampRate, dFiltLow, dFiltHighLocal, sd, sn, 
                   &iNumSects );
/* How many points do we taper? */
   lNumToTaper = (long) (pStaArray->dSampRate*dTaperTime + 0.0001);
   if ( lNumToTaper < 10 ) lNumToTaper = 10;
/* Loop over entire data array.  The data may have gaps due to the */
/* nature of its transmission.  Each gap must be tapered to avoid */
/* filter impulse response at the start and end of the data. */
   i = 0;
   while ( i < lNumToFilter )
   {    /* First, find where data starts; then go into loop to see where */
        /*  it ends. */
      lTemp = lSamp + i;
      i++;
      if ( lTemp >= pStaArray->lRawCircSize ) lTemp -= pStaArray->lRawCircSize;
      if ( pStaArray->plFiltCircBuff[lTemp] != 0 )      /* We have some data! */
      {
         lStartThisTime = lTemp;
/* Now, see if the data ends before end of array */
         iCnt = i - 1;
         j = 1;
         while ( j < lNumToFilter-iCnt )
         {
            lTemp = lStartThisTime + j;
            j++;
            if ( j == lNumToFilter-iCnt ) break;
            if ( lTemp >= pStaArray->lRawCircSize ) 
               lTemp -= pStaArray->lRawCircSize;
            if ( pStaArray->plFiltCircBuff[lTemp] == 0 )   /* Possibly at end */
            {    /* Check next NUM_AT_ZERO samples to see if all 0; */
                 /* If they are, we hit a gap or are at end of data */
               for ( k=1; k<=NUM_AT_ZERO; k++ )
               {
                  lTemp2 = lTemp + k;
                  j++;
                  if ( j == lNumToFilter-iCnt ) break;
                  if ( lTemp2 >= pStaArray->lRawCircSize ) 
                     lTemp2 -= pStaArray->lRawCircSize;
                  if ( pStaArray->plFiltCircBuff[lTemp2] != 0 ) /* Not at end */
                     goto CarryOn;
               }
/* If we get to this point, we have hit a gap or the end of the data. So, */
/* taper and filter it. Apply taper to first and last lNumToTaper samples */
/* Use the following if DC is to be taken out before filtering */
               for ( l=lStartThisTime;l<(lStartThisTime+j-(NUM_AT_ZERO+1));l++ )
               {
                  lTemp = l;
                  if ( lTemp >= pStaArray->lRawCircSize ) 
                     lTemp -= pStaArray->lRawCircSize;
                  pStaArray->plFiltCircBuff[lTemp] -= 
                     (int32_t) pStaArray->dAveLDCRaw;
               }
               if ( iFlag == 1 || (iFlag == 2 && (lStartThisTime == 0 || 
                    lStartThisTime != pStaArray->lStartFilterIndex)) )
               {
                  taperG( pStaArray->plFiltCircBuff, j-(NUM_AT_ZERO+1),
                          lStartThisTime, TRUE, lNumToTaper, 
                          pStaArray->lRawCircSize, 1 );	    
/* Apply filter */
                  apiirG( pStaArray->plFiltCircBuff, j-(NUM_AT_ZERO+1), 0,
                          lStartThisTime, sn, sd, iNumSects, iIndex, 1, 
                          pStaArray->lRawCircSize, iLP, pStaArray, 
                          pStaArray->plRawCircBuff, 0 );
               }
               else
               {
                  apiirG( pStaArray->plFiltCircBuff, j-(NUM_AT_ZERO+1), 0,
                          lStartThisTime, sn, sd, iNumSects, iIndex, 1, 
                          pStaArray->lRawCircSize, iLP, pStaArray, 
                          pStaArray->plRawCircBuff, lNumToTaper );
               }
               i += (j-1);
               goto WhileEnd;
            }
CarryOn:    ;
         }
         i += (j-1);
/* Force filter at end of array if we are in data */
         if ( i >= lNumToFilter )
         {     /* Apply taper to first lNumToTaper samples */
               /* Use the following if DC is to be taken out before filtering */             
            for ( l=lStartThisTime; l<(lStartThisTime+j); l++ )
            {
               lTemp = l;
               if ( lTemp >= pStaArray->lRawCircSize ) 
                    lTemp -= pStaArray->lRawCircSize;
               pStaArray->plFiltCircBuff[lTemp] -= (int32_t) pStaArray->dAveLDCRaw;
            }
            if ( iFlag == 1 )
            {
               taperG( pStaArray->plFiltCircBuff, j, lStartThisTime, TRUE,
                       lNumToTaper, pStaArray->lRawCircSize, 1 );	    
               apiirG( pStaArray->plFiltCircBuff, j, 0, lStartThisTime, sn, sd,
                       iNumSects, iIndex, 1, pStaArray->lRawCircSize, iLP, 
                       pStaArray, pStaArray->plRawCircBuff, 0 );
            }
            else if ( iFlag == 2 && 
                      lStartThisTime != pStaArray->lStartFilterIndex )
            {
               taperG( pStaArray->plFiltCircBuff, j, lStartThisTime, FALSE,
                       lNumToTaper, pStaArray->lRawCircSize, 1 );	    
               apiirG( pStaArray->plFiltCircBuff, j, 0, lStartThisTime, sn, sd,
                       iNumSects, iIndex, 1, pStaArray->lRawCircSize, iLP, 
                       pStaArray, pStaArray->plRawCircBuff, 0 );
            }
            else 
               apiirG( pStaArray->plFiltCircBuff, j, 0, lStartThisTime, sn, sd,
                       iNumSects, iIndex, 1, pStaArray->lRawCircSize, iLP, 
                       pStaArray, pStaArray->plRawCircBuff, lNumToTaper );
         }
      }
WhileEnd:;
   }
return 0;
}

     /**************************************************************
      *                     GetFilterCoefs()                       *
      *                                                            *
      * This function gets IIR filter coefficients.                *
      * This function is a rewrite of iirdes in the original code. *
      *                                                            *
      * Arguments:                                                 *
      *  dSampRate       Incoming data smple rate (samp/s)         *
      *  dFiltLow        Low end of pass band                      *
      *  dFiltHigh       High end of pass band                     *
      *  sd              Filter denominator coefs (poles)          *
      *  sn              Filter numerator coefs (zeroes)           *
      *  piNumSects      Number of IIR filter sections             *
      *                                                            *
      **************************************************************/
void GetFilterCoefs( double dSampRate, double dFiltLow, double dFiltHigh,
		             double sd[], double sn[], int *piNumSects )
{
   double   dLow, dHigh;       /* Low/high ends of filter (rel. to sr) */
   double   dLowW, dHighW;     /* Warped (?) ends of filter */
   int      nps;               /* Number of second order sections of filter */
   fcomplex p[MAX_FILT_SECTIONS];      /* Poles of filter */
   char     ptype[MAX_FILT_SECTIONS];  /* S=single read, C=cmplx pair */

/* Get IIR filter coefficients */				
   bupoles (p, ptype, &nps, FILTER_ORDER);  /* Compute Butterworth poles */
   dLow = dFiltLow / 2. / dSampRate;        /* Relate frequency to samp rate */
   dHigh = dFiltHigh / 2. / dSampRate;
   dLowW = warp (dLow, 2.);                 /* Apply tangent freq warping */
   dHighW = warp (dHigh, 2.);               /* (2. used to make warp work) */
   lptbpa (p, ptype, nps, dLowW, dHighW, sn, sd, piNumSects);
   bilin2 (sn, sd, *piNumSects);            /* Bilinear transformation */
}

   /*******************************************************************
    *                            InitVar()                            *
    *                                                                 *
    *          Initialize STATION variables for one station.          *
    *******************************************************************/

void InitVar( STATION *Sta )
{
   Sta->iClipIt             = 1;    /* 1 -> Clip signal on display */
   Sta->iLPAlarmSent        = 0;    /* 1 -> LP Alarm sent for this station */
   Sta->iCal                = 0;    /* TRUE -> pick looks like calibration */
   Sta->cFirstMotion        = '?';  /* ?=unknown, U=up, D=down */
   Sta->dAvAmp              = 0.;   /* Average signal amp per 12 cycle */
   Sta->dAveLDC             = 0.;   /* Moving avg of filtered DC signal level */
   Sta->dAveLDCRaw          = 0.;   /* Moving avg of unfiltered DC sgnl level */
   Sta->dAveLDCRawOrig      = 0.;   /* Moving avg of unfiltered DC sgnl level
                                       when Phase1 passed */
   Sta->dAveRawNoise        = 0.;   /* Moving avg of rms noise level */
   Sta->dAveRawNoiseOrig    = 0.;   /* Moving avg of rms noise level when phase
                                       1 passed*/
   Sta->dAveLTA             = 0.;   /* Moving average of average amplitude */
   Sta->dAveMDF             = 0.;   /* Moving average of MDF */
   Sta->dAveMDFOrig         = 0.;   /* Moving avg of MDF when Phase 1 passed*/
   Sta->dLTAThresh          = 0.;   /* Phase 2 ampltude threshold */
   Sta->dLTAThreshOrig      = 0.;   /* dLTAThresh when Phase 1 first passed */
   Sta->dMbAmpGM            = 0.;   /* Mb amplitude (ground motion in nm)
                                       highest amp in 1st MbCycles */
   Sta->dMbTime             = 0.;   /* 1/1/70 time (sec) at end of Mb T/A */
   Sta->dMlAmpGM            = 0.;   /* Ml amplitude (ground motion in nm) 
                                       highest amp in 1st LGSeconds, this
                                       sets limit on how far out Ml ok. */
   Sta->dMlTime             = 0.;   /* 1/1/70 time (sec) at end of Ml T/A */
   Sta->dMDFThresh          = 0.;   /* MDF to exceed to pass Phase 1 */
   Sta->dMDFThreshOrig      = 0.;   /* dMDFThresh when Phase 1 first passed */
   Sta->dMwpIntDisp         = 0.;   /* Max integrated disp. peak-to-peak amp */
   Sta->dMwpTime            = 0.;   /* Mwp window time in seconds */
   Sta->dPStrength          = 0.;   /* P strength ratio */
   Sta->dSumLDC             = 0.;   /* Accumulator for average amplitude */
   Sta->dSumLDCRaw          = 0.;   /* Accumulator for unfiltered, avg amp */
   Sta->dSumRawNoise        = 0.;   /* Accumulator for unfiltered, rms amp */
   Sta->dSumLTA             = 0.;   /* Accumulator for DC offset */
   Sta->dSumMDF             = 0.;   /* Accumulator for MDF summation */
   Sta->dTrigTime           = 0.;   /* Time (1/1/70-sec) Phase1 was passed */
   Sta->lCurSign            = 0;    /* Sign of current MDF for Phase 3 */
   Sta->lCycCnt             = 0;    /* Cycle ctr (if T/A in first MbCycles, 
                                       this is associated with Mb magnitude) */
   Sta->lCycCntLTA          = 0;    /* Cycle counter for LTAs */
   Sta->lFirstMotionCtr     = 0;    /* Number of 1st motion samples checked */										   
   Sta->lHit                = 0;    /* # of hits counter for Phases 2 & 3 */
   Sta->lLastSign           = 0;    /* Sign of last MDF for Phase 3 */
   Sta->lSampIndexF         = 0;    /* Live sample counter - filtered array*/
   Sta->lSampIndexR         = 0;    /* Live sample counter - raw array */
   Sta->lLTACtr             = 0;    /* Long term averages counter */
   Sta->lMagAmp             = 0;    /* Summation of 1/2 cycle amplitudes */
   Sta->dMaxPk              = 0.;   /* Amp (p-p nm) for use in mag comp. */
   Sta->lMbPer              = 0;    /* Mb Per data, per of dMbAmpGM doubled */
   Sta->lMDFCnt             = 0;    /* Number of cycles in MDFTotal */
   Sta->lMDFNew             = 0;    /* Present MDF value */
   Sta->lMDFOld             = 0;    /* Last MDF value */
   Sta->lMDFRunning         = 0;    /* Running total of sample differences */
   Sta->lMDFRunningLTA      = 0;    /* Running total of sample differences */
   Sta->lMDFTotal           = 0;    /* Running total of MDFs over several cyc */
   Sta->lMis                = 0;    /* Num. of misses count for Phases 2 & 3 */
   Sta->lMlPer              = 0;    /* Ml Per data*10, per of dMlAmpGM doubled */
   Sta->lMwpCtr             = 0;    /* Index which counts samp from P for Mwp */
   Sta->lNumOsc             = 0;    /* # of osc. counter for Phase 3 */
   Sta->lPer                = 0;    /* Temporary period array*10 */
   Sta->lPhase1             = 0;    /* Phase 1 passed flag */
   Sta->lPhase2             = 0;    /* Phase 2 passed flag */
   Sta->lPhase3             = 0;    /* Phase 3 passed flag */
   Sta->lPhase2Cnt          = 0;    /* Sample counter for timing Phase 2 */
   Sta->lPhase3Cnt          = 0;    /* Sample counter for timing Phase 3 */
   Sta->lRawCircCtr         = 0;    /* Small, raw-data circ buffer counter */
   Sta->lRawNoise           = 0;    /* Max peak/trough signal difference */
   Sta->lRawNoiseOrig       = 0;    /* Max peak/trough signal difference
                                       when Phase1 passed */
   Sta->lSampNew            = 0;    /* Present sample */
   Sta->lSampOld            = 0;    /* Last sample */
   Sta->lSampRaw            = 0;    /* Un-filter present sample */
   Sta->lSampsPerCyc        = 0;    /* Number of samples per half cycle */
   Sta->lSampsPerCycLTA     = 0;    /* Number of samples per half cycle in LTA*/
   Sta->lSWSim              = 0;    /* Count of similarities to sin wave cal */
   Sta->lTest1              = 0;    /* Phase 2 Test 1 passed */
   Sta->lTest2              = 0;    /* Phase 2 Test 2 passed */
   Sta->lTrigFlag           = 0;    /* Flag -> samp has passed MDF threshold */
   Sta->l13sCnt             = 0;    /* 13s counter; this delays p-picks 
                                       by 13 seconds after pick for Neural net*/
   strcpy( Sta->szPhase, "eP" );    /* Assume only Ps picked */										   
}

     /**************************************************************
      *                          lptbpa()                          *
      *                                                            *
      * Convert all pole low-pass filter to band pass filter via   *
      * the analog polynomial transform.  The lowpass filter is    *
      * described in terms of its poles (as input to this routine).*
      * The output consists of the parameters for second order     *
      * sections.
      *                                                            *
      * Arguments:                                                 *
      *  p               Poles - contains 1 from each complex      *
      *                   conjugate pair and all real poles        *
      *  type            S - single real                           *
      *                  C - complex conjugate pair                *
      *  nps             number of second order sections           *
      *  dLowW           Low Frequency cutoff                      *
      *  dHighW          High Frequency cutoff                     *
      *  sn              Filter numerator coefs (zeroes)           *
      *  sd              Filter denominator coefs (poles)          *
      *  piNumSects      Number of IIR filter sections             *
      *                                                            *
      **************************************************************/
void lptbpa( fcomplex p[], char type[], int nps, double dLowW, 
             double dHighW, double sn[], double sd[], int *piNumSects )
{
   fcomplex  ctemp, ctemp2, p1, p2, ctemp3, ctemp4, ctemp5;
   double    a, b;
   int       i, iptr;

   a = TWO_PI*TWO_PI*dLowW*dHighW;
   b = TWO_PI * (dHighW-dLowW);
   *piNumSects = 0;
   iptr = 0;
   for ( i=0; i<nps; i++ )
      if ( type[i] == 'C' )
      {
         ctemp = RCmul (b, p[i]);
         ctemp = Cmul (ctemp, ctemp);
         ctemp.r -= (float) (4.*a);
         ctemp = Csqrt (ctemp);
         ctemp2 = RCmul (b, p[i]);
         ctemp3 = Cadd (ctemp2, ctemp);
         ctemp4 = Csub (ctemp2, ctemp);
         p1 = RCmul (0.5, ctemp3);
         p2 = RCmul (0.5, ctemp4);
         sn[iptr] = 0.;
         sn[iptr+1] = b;
         sn[iptr+2] = 0.;
         ctemp5 = Cmul (p1, Conjg (p1));
         sd[iptr] = ctemp5.r;
         sd[iptr+1] = -2. * p1.r;
         sd[iptr+2] = 1.;
         iptr += 3;
         sn[iptr] = 0.;
         sn[iptr+1] = b;
         sn[iptr+2] = 0.;
         ctemp5 = Cmul (p2, Conjg (p2));
         sd[iptr] = ctemp5.r;
         sd[iptr+1] = -2. * p2.r;
         sd[iptr+2] = 1.;
         iptr += 3;
         *piNumSects += 2;
      }
      else if ( type[i] == 'S' )
      {
         sn[iptr] = 0.;
         sn[iptr+1] = b;
         sn[iptr+2] = 0.;
         sd[iptr] = a;
         sd[iptr+1] = -b * p[i].r;
         sd[iptr+2] = 1.;
         iptr += 3;
         *piNumSects += 1;
      }
}

     /**************************************************************
      *                      ResetFilter()                         *
      *                                                            *
      * Utility fuinction to reset needed filter variables.        *
      *                                                            *
      * Arguments:                                                 *
      *  Sta             Station data structure                    *
      *                                                            *
      **************************************************************/

void ResetFilter( STATION *Sta ) 
{
   int i;

   Sta->lFiltSamps = 0;
   for ( i=0; i<MAX_FILT_SECTIONS; i++ )
   {
      Sta->dFiltX1[i] = 0.;
      Sta->dFiltX2[i] = 0.;
      Sta->dFiltY1[i] = 0.;
      Sta->dFiltY2[i] = 0.;
   }
}

     /**************************************************************
      *                          secord()                          *
      *                                                            *
      * Implement one time-step of a second order IIR section.     *
      * This uses the recursion equation: y = -a1y1 - a2y2 + bx +  *
      * b1x1 + b2x2.                                               *
      *                                                            *
      * Arguments:                                                 *
      *  output          Current filtered value                    *
      *  dY1, dY2        Previous output samples                   *
      *  dX1, dX2        Previous input samples                    *
      *  input           Current value to filter                   *
      *  a1, a2          denominator coefficients                  *
      *  b, b1, b2       numerator coefficients                    *
      *                                                            *
      **************************************************************/
	  
void secord( double *output, double *dY1, double *dY2, double a1, double a2, 
             double input, double *dX1, double *dX2, double b, double b1,
             double b2 )
{
   *output = b*input + b1*(*dX1) + b2*(*dX2) - (a1*(*dY1) + a2*(*dY2));
   *dY2 = *dY1;
   *dY1 = *output;
   *dX2 = *dX1;
   *dX1 = input;
}

     /**************************************************************
      *                          Taper()                           *
      *                                                            *
      * Windows beginning and end of signal with cosine taper.     *
      * Changed original so that data can be                       *
      * tapered in consecutive packets.  (Just taper front end of  *
      * data when data comes in consecutive packets).              *
      *                                                            *
      * Arguments:                                                 *
      *  WaveLong        Array of long data values                 *
      *  Sta             Station data array                        *
      *  lNumSamp        Number of samples                         *
      *  iFETaper        0->only taper front end; 1->both ends     *
      *  lNumToTaper	 Total number of samples to taper          *
      *                                                            *
      **************************************************************/	  
void Taper( int32_t *WaveLong, STATION *Sta, long lNumSamp,
            int iFETaper, long lNumToTaper )
{
   int     i;
   int     iSampCnt;            /* Local value for Sta->iFiltSamps */
   long    lNumLocal;           /* lNumToTaper may be higher than packet size */
   double  ang, xi, cs;         /* Taper values */

/* Use a local counter for samples processed */
   iSampCnt = Sta->lFiltSamps;
   
/* Reset lNumToTaper based on number of samples in packet */   
   lNumLocal = lNumToTaper;
   if ( lNumLocal > lNumSamp ) lNumLocal = lNumSamp;
   
/* Compute sample weight */   
   ang = acos (-1.) / (double) lNumToTaper;
   
/* Taper through packet or till lNumToTaper; whichever comes first */
   while ( iSampCnt < lNumLocal )
   {
      iSampCnt++;
      xi = (double) iSampCnt;
      cs = (1.0 - cos (xi*ang)) / 2.0;
      WaveLong[iSampCnt-1] = (int32_t) ((double) WaveLong[iSampCnt-1] * cs);
   }

/* Taper back end is optional if data is not near-real-time in packets */
   if ( iFETaper )   
   {
      for ( i=lNumSamp-lNumToTaper; i<lNumSamp; i++ )
      {
         xi = (double) (i - lNumSamp);
         cs = (1.0 - cos (xi*ang)) / 2.0;
         WaveLong[i] = (int32_t) ((double) WaveLong[i] * cs);
      }
   }
}

     /**************************************************************
      *                          taperG()                          *
      *                                                            *
      * Windows beginning and end of signal with cosine taper.     *
      * Changed original so that data can be                       *
      * tapered in consecutive packets.  (Just taper front end of  *
      * data when data comes in consecutive packets).              *
      *                                                            *
      * Arguments:                                                 *
      *  lData           Array of int32_t data values              *
      *  lNumPts         Number of data points in array            *
      *  lIndex          Index of starting point in array to taper *
      *  iZP             0->front end taper only                   *
      *  lNumToTaper	 Total number of samples to taper          *
      *  lMaxIndex       Max size of buffer                        *
      *  iFETaper        0->only taper front end; 1->both ends     *
      *                                                            *
      **************************************************************/
void taperG( int32_t lData[], long lNumPts, long lIndex, int iZP, long lNumToTaper,
             long lMaxIndex, int iFETaper )
{
   double  dTemp, ang, xi, cs;
   long    m1, m3, m4, m5;
   long    i, lTemp;

   dTemp = acos( -1. );
   if ( iFETaper == 1 )
   {
      m1 = lNumToTaper;
      if ( m1 > lNumPts ) m1 = lNumPts;
      if ( m1 > 0 && lNumToTaper > 0 )
      {
         ang = dTemp / (double) lNumToTaper;
         for ( i=1; i<=m1; i++ )
         {
            xi = (double) i;
            cs = (1.0 - cos( xi*ang )) / 2.0;
            lTemp = i + lIndex - 1;
            if ( lTemp >= lMaxIndex ) lTemp -= lMaxIndex;
            if ( lTemp < 0 ) lTemp += lMaxIndex;
            lData[lTemp] = (int32_t) ((double) lData[lTemp] * cs);
         }
      }
   }
   if ( iZP )  // Only taper back end if data is not real-time (or near real) */
   {
      m3 = lNumToTaper;
      m5 = lNumPts - m3;
      m4 = m5 + 1;
      if ( m3 > 0 ) 
      {
         ang = dTemp / (double) lNumToTaper;
         for ( i=m4; i<=lNumPts; i++ )
         {
            xi = (double) (i - lNumPts - 1);
            cs = (1.0 - cos( xi*ang )) / 2.0;
            lTemp = i + lIndex - 1;
            if ( lTemp >= lMaxIndex ) lTemp -= lMaxIndex;
            if ( lTemp < 0 ) lTemp += lMaxIndex;
            lData[lTemp] = (int32_t) ((double) lData[lTemp] * cs);
         }
      }
   }
}

     /**************************************************************
      *                          warp()                            *
      *                                                            *
      * Applies tangent frequency warping to compensate for        *
      * bilinear analog -> digital transformation.                 *
      *                                                            *
      * Arguments:                                                 *
      *  f               Frequency related to sample rate          *
      *  t               Sample interval (here, forced to 2 so     *
      *                   function works)                          *
      * Return:          Adjusted frequency                        *
      *                                                            *
      **************************************************************/
double warp( double f, double t )
{
   double  angle, result;

   angle = TWO_PI*f*t / 2.;
   result = 2. * tan (angle) / t;
   result /= TWO_PI;
   return result;
}

     /**************************************************************
      *                          zero()                            *
      *                                                            *
      * Utility function to zero an array.                         *
      *                                                            *
      * Arguments:                                                 *
      *  x               input array to be zeroed                  *
      *  n               number of integers in array               *
      *                                                            *
      **************************************************************/
void zero( double x[], int n )
{
   int     i;

   for ( i=0; i<n; i++ ) x[i] = 0.;
}
