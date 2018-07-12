/************************************************************************
  * GET_PICK.C                                                           *
  *                                                                      *
  * This is a group of functions which provide tools for                 *
  * making P-picks and determining magnitudes from the P data.           *
  *                                                                      *
  * Made into earthworm module 3/2001.                                   *
  *                                                                      *
  ************************************************************************/
  
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <earthworm.h>
#include <transport.h>
#include "earlybirdlib.h"
#ifdef _MAT_LAB_
#include <mat.h>
#include <libNeuralNet.h>
#endif

/* "Global" variables */
time_t tAlarm;        /* Time of iAlarmCnt=1 Alarm */

/*******************************************************************************
 *                              AICPick()                                      *
 *                                                                             *
 *  This function is based on the Akaike Information Criterion (AIC). Data     *
 *  that preceed an initial LTA/STA pick are selected and passed to the AIC    *
 *  algorthim. Within this windowed data, it is assumed that the seismogram    *
 *  is divided into two locally stationary segments that may be modeled        *
 *  as Autoregressive(AR) processes. The intervals before and after the onset  *
 *  time are two different stationary processes (Sleeman et al, 1999).         *
 *  The order and the value of the AR coefficients change when the             *
 *  characteristic of the current segment of seismogram is different from      *
 *  before.                                                                    *
 *                                                                             * 
 *  March, 2009:                                                               *
 *                                                                             *
 *       Contributors:       David Nyland                                      *
 *       OS:                 Windows XP SP3                                    *
 *       Compiler:           Microsoft Visual Studio.NET 2003                  *
 *                                                                             *
 *  Arguments:                                                                 *
 *         dData           Array of windowed data containing                   *
 *	                       the first arrival.                                  *
 *         lNumAICSamps    Number of samples in the dData array                *
 *         dSampRate       Sample rate - samples per seconds                   *
 *                                                                             *
 *  Return Type:           Returns a long variable containing the index of     *
 *                         the new pick                                        *
 *                                                                             *
 *  	                                                                       *
 ******************************************************************************/
long  AICPick( double dData[], long lNumAICSamps, double dSampRate ) 
{
   int     i, k, j; 
   double  eps, dSum, dSum2, dSum3, dSum4, dAvg, dAvg2, dVar, dVar2;
   double  aic[MAX_NN_SAMPS];   
   double  dMinValue;
   long    lMinIndex;


/* Initialize variables */
   dAvg  = 0;
   dAvg2 = 0;
   dSum  = 0;
   dSum2 = 0;
   dSum3 = 0;
   dSum4 = 0;
   dVar  = 0;
   dVar2 = 0;
   aic[0]= 0.0;
   eps   = 2.2204e-16; 

   for( k=1; k<=lNumAICSamps-1; k++ ) 
   {     
      for ( i=0; i<k; i++ ) dSum += dData[i];
      dAvg = dSum / (double) k;	 
/* Calculate the variance of the first gate */
      for ( i=0; i<k; i++ ) dSum2 += ((dAvg-dData[i]) * (dAvg-dData[i]));
      dVar = dSum2 / (double) (k-1+eps);  			
      for ( j=k; j<lNumAICSamps; j++ ) dSum3 += dData[j];
      dAvg2 = dSum3 / (double) (lNumAICSamps-k);

/* Calculate the variance of the second gate */
      for ( j=k; j<lNumAICSamps; j++ )
         dSum4 += ((dAvg2-dData[j]) * (dAvg2-dData[j]));
      dVar2 = dSum4 / (double) (lNumAICSamps-1-k+eps);

      aic[k] = (double) k * log10( dVar+eps ) + ((double) lNumAICSamps -
               (double) k-1) * log10( dVar2+eps );     
      dSum  = 0;
      dSum2 = 0;
      dSum3 = 0;
      dSum4 = 0;
      dAvg  = 0;
      dAvg2 = 0;
   }                /* end for k */

/* Find the index for the global minimum in the aic array */
   dMinValue = aic[6]; /* Initialize dMinValue to an arbitrary value */
   lMinIndex = 6;      /* Initialize dMinIndex */

   for ( i=1; i<lNumAICSamps-1; i++ )
   {	
      if ( aic[i] < dMinValue )
      {
         dMinValue = aic[i];
         lMinIndex = (long) i; 
      }
   }
   return lMinIndex;
}

 /***********************************************************************
  *                             CheckForAlarm()                         *
  *                                                                     *
  *  Check for multi station regional alarm.  This logic was patterned  *
  *  after a PTWC program which basically does the same thing. Send     *
  *  alarm if necessary.                                                *
  *                                                                     *
  *  August, 2014: Added return value and iSendAlarm - PW.              *
  *  May, 2008: Split out to libsrc from pick_wcatwc.                   *
  *  May, 2005: Add timeout feature to alarm (i.e., if pick is over x   *
  *             minutes old, ignore it.                                 *
  *                                                                     *
  *  Arguments:                                                         *
  *     Sta              Station data structure                         *
  *     pAS              ALARMSTRUCT structure                          *
  *     iNumReg          Number of Alarm regions                        *
  *     ucMyModID        Calling module ID                              *
  *     siAlarmRegion    Calling module Alarm Ring                      *
  *     ucEWHTypeAlarm   Earthworm alarm message number                 *
  *     ucEWHMyInstID    Earthworm Institute ID                         *
  *     iSendAlarm       1=Report Alarm; 0=Don't                        *
  *                                                                     *
  *  Returns:                                                           *
  *     int              1 if Alarm; else 0                             *
  *                                                                     *
  ***********************************************************************/
int CheckForAlarm( STATION *Sta, ALARMSTRUCT pAS[], int iNumReg, 
                   unsigned char ucMyModID, SHM_INFO siAlarmRegion,
                   unsigned char ucEWHTypeAlarm, 
                   unsigned char ucEWHMyInstID, int iSendAlarm )
{
   int   i, j, k;
   int   iAlarm=0;             /* 1 if an Alarm is announced */
   
   for ( i=0; i<iNumReg; i++ )
      for ( j=0; j<pAS[i].iNumStnInReg; j++ ) /* Is it an alarm station */
	     if ( !strcmp( pAS[i].szStation[j], Sta->szStation ) )
         {                                    /* Yes, it is */
            logit( "", "Alarm Stn found-%s in %s, SN=%lf\n", Sta->szStation, 
                   pAS[i].szRegionName, Sta->dPStrength );
            if ( Sta->dPStrength > pAS[i].dThresh )/* Is it strong enough? */
            {
               if ( (Sta->dTrigTime-Sta->dTimeCorrection)-pAS[i].dLastTime <
                  pAS[i].dMaxTime )                   /* Is it within time? */
               {  /* First, see if this station is already alarmed */
                  for ( k=0; k<pAS[i].iNumPicksCnt; k++ )
                     if ( !strcmp( Sta->szStation, pAS[i].szStnAlarm[k] ) )
                     {
                        logit( "", "%s already alarmed\n", Sta->szStation );
	                goto LoopEnd;
                     }
                  strcpy( pAS[i].szStnAlarm[pAS[i].iNumPicksCnt], Sta->szStation );
                  pAS[i].iNumPicksCnt++;
                  logit( "", "%s has %d alarm stations\n", pAS[i].szRegionName,
                         pAS[i].iNumPicksCnt );
                  if ( pAS[i].iNumPicksCnt >= pAS[i].iAlarmThresh )
                  {
                     iAlarm = 1;
                     if ( iSendAlarm == 1 )
                        ReportAlarm( Sta, ucMyModID, siAlarmRegion,
                         ucEWHTypeAlarm, ucEWHMyInstID, 2, pAS[i].szRegionName,
                         0 );
                     pAS[i].iNumPicksCnt = 0;
                     pAS[i].dLastTime = 0.;
                  }
               }
               else
               {
                  logit( "", "Reset alarm vars. in %s, %s\n", pAS[i].szRegionName,
                         Sta->szStation );
                  strcpy( pAS[i].szStnAlarm[0], Sta->szStation );
                  pAS[i].iNumPicksCnt = 1;
                  pAS[i].dLastTime = Sta->dTrigTime-Sta->dTimeCorrection;
               }
            }
         }
   LoopEnd:;
   return iAlarm;
}

/***********************************************************************
  *                           CheckForLPAlarmSWD()                      *
  *                                                                     *
  *  Check for multi LP station alarms with a given area.               *
  *                                                                     *
  *  August, 2014: Added return value and iSendAlarm - PW.              *
  *  January, 2012: Moved nearest array into STATION structure - PW.    *
  *  12 Nov, 2009: Created new function - JFP.                          *
  *                                                                     *
  *  Arguments:                                                         *
  *     Sta              Station data structure                         *
  *     nSta             Number of stations in Sta array                *
  *     ucMyModID        Calling module ID                              *
  *     siAlarmRegion    Calling module Alarm Ring                      *
  *     ucEWHTypeAlarm   Earthworm alarm message number                 *
  *     ucEWHMyInstID    Earthworm Institute ID                         *
  *     iSendAlarm       1=Report Alarm; 0=Don't                        *
  *                                                                     *
  *  Returns:                                                           *
  *     int              1 if Alarm; else 0                             *
  *                                                                     *
  ***********************************************************************/
int CheckForLPAlarmSWD( STATION *Sta, int nSta, unsigned char ucMyModID, 
                        SHM_INFO siAlarmRegion, unsigned char ucEWHTypeAlarm, 
                        unsigned char ucEWHMyInstID, int iSendAlarm )
{
   char     alarmString[128];
   int      i=0, j;
   int      iAlarm=0;             /* 1 if an Alarm is announced */
   STATION  StationTemp;

/* March thru all of the stations */
/* ****************************** */
   for ( i=0; i<nSta; i++ )
   {
/* Reset if sent */
/* ************* */
      if ( Sta[i].iAlarmStatus < 3 ) Sta[i].iLPAlarmSent = 0;
      
/* If we have an alarm */
/* ******************* */
      if ( Sta[i].iAlarmStatus == 3 )
         for ( j=1; j<5; j++ )
/* See if one of the Nearby Stations is triggered too. */
/* *************************************************** */
            if ( Sta[Sta[i].iNearbyStnArray[j]].iAlarmStatus == 3 )
               if ( Sta[i].iLPAlarmSent == 0 )
               {
                  Sta[i].iLPAlarmSent = 1;
/* O.K. Then send it out. */
/* ********************** */
                  iAlarm = 1;
                  if ( iSendAlarm == 1 )
				  {
                     memcpy( &StationTemp, &Sta[i], sizeof(STATION) );
                     strcpy( alarmString, Sta[i].szStation );
                     ReportAlarm( &StationTemp, ucMyModID, siAlarmRegion, 
                      ucEWHTypeAlarm, ucEWHMyInstID, 3, alarmString, 1 );
                  }
                  break;
               }
   } /* end for i */
   return iAlarm;
}
                 
  /**************************************************************************
   *                             ComputeDC()                                *
   *                                                                        *
   * This function computes the background DC offset for the raw and        *
   * filtered data.                                                         *
   *                                                                        *
   *  Arguments:                                                            *
   *     pSta             STATION structure full of data                    *
   *     lNumForDC        Number of samples to use for DC                   *
   *                                                                        *
   **************************************************************************/
int ComputeDC( STATION *pSta, long lNumForDC )
{
   double  dTotal, dTotalF;         /* Adder for average computations */
   long    lSamp;                   /* Sample of interest in large array */
   long    lTemp, lTemp2, i, j;

/* Get the DC offset and scaling for broadband, SP, and LP data */
   dTotal  = 0.;
   dTotalF = 0.;
   lSamp   = pSta->lIndex;
   
/* Check to see where the data actually starts (it may be all 0's) */
   for ( j=0; j<pSta->lRawCircSize-lNumForDC; j++ )
   {
      lTemp2 = lSamp + j;
      if ( lTemp2 >= pSta->lRawCircSize ) lTemp2 -= pSta->lRawCircSize;
      if ( pSta->plRawCircBuff[lTemp2] != 0 )            /* We have some data */
      {                          
         pSta->dDataStartTime = pSta->dStartTime +
                                ((double) j/pSta->dSampRate);
         for ( i=0; i<lNumForDC; i++ )      /* Add up background noise levels */
         {
            lTemp = lTemp2 + i;        /* Sample index within circular buffer */
            if ( lTemp >= pSta->lRawCircSize ) lTemp -= pSta->lRawCircSize;
            dTotal  += (double) pSta->plRawCircBuff[lTemp];
            dTotalF += (double) pSta->plFiltCircBuff[lTemp];
         }
         goto OutOfFor;
      }
   }
   
/* Compute DC Offsets */
   OutOfFor:
   pSta->dAveLDCRaw = dTotal  / (double) lNumForDC;
   pSta->dAveLDC    = dTotalF / (double) lNumForDC; 
   return 0;
}

     /**************************************************************
      *                         CopyPBuf()                         *
      *                                                            *
      * Copy one the P-Pick info from one structure to another.    *
      *                                                            *
      * October, 2010: Now use STATION structure instead of PPICK  *
      *                                                            *
      * Arguments:                                                 *
      *  pStaIn      Input STATION structure                       *
      *  pStaOut     Output STATION structure                      *
      *                                                            *
      **************************************************************/

void CopyPBuf( STATION *pStaIn, STATION *pStaOut )
{
   int i;

   strcpy( pStaOut->szPhase,   pStaIn->szPhase );
   pStaOut->lPickIndex       = pStaIn->lPickIndex;
   pStaOut->iPickCnt         = pStaIn->iPickCnt;
   pStaOut->iBin             = pStaIn->iBin;
   pStaOut->iUseMe           = pStaIn->iUseMe;
   pStaOut->iClipIt          = pStaIn->iClipIt;
   pStaOut->dPTime           = pStaIn->dPTime;
   pStaOut->dExpectedPTime   = pStaIn->dExpectedPTime;
   pStaOut->dPhaseTime       = pStaIn->dPhaseTime;
   strcpy( pStaOut->szHypoID,  pStaIn->szHypoID );
   pStaOut->stPTime.wYear    = pStaIn->stPTime.wYear;
   pStaOut->stPTime.wMonth   = pStaIn->stPTime.wMonth;
   pStaOut->stPTime.wDayOfWeek = pStaIn->stPTime.wDayOfWeek;
   pStaOut->stPTime.wDay     = pStaIn->stPTime.wDay;
   pStaOut->stPTime.wHour    = pStaIn->stPTime.wHour;
   pStaOut->stPTime.wMinute  = pStaIn->stPTime.wMinute;
   pStaOut->stPTime.wSecond  = pStaIn->stPTime.wSecond;
   pStaOut->stPTime.wMilliseconds = pStaIn->stPTime.wMilliseconds;
   pStaOut->cFirstMotion     = pStaIn->cFirstMotion;
   pStaOut->dMbAmpGM         = pStaIn->dMbAmpGM;
   pStaOut->dMbPer           = pStaIn->dMbPer;
   pStaOut->dMbTime          = pStaIn->dMbTime;
   pStaOut->dMbMag           = pStaIn->dMbMag;
   pStaOut->iMbClip          = pStaIn->iMbClip;
   pStaOut->dMlAmpGM         = pStaIn->dMlAmpGM;
   pStaOut->dMlPer           = pStaIn->dMlPer;
   pStaOut->dMlTime          = pStaIn->dMlTime;
   pStaOut->dMlMag           = pStaIn->dMlMag;
   pStaOut->iMlClip          = pStaIn->iMlClip;
   pStaOut->dMSAmpGM         = pStaIn->dMSAmpGM;
   pStaOut->dMSPer           = pStaIn->dMSPer;
   pStaOut->dMSTime          = pStaIn->dMSTime;
   pStaOut->dMSMag           = pStaIn->dMSMag;
   pStaOut->iMSClip          = pStaIn->iMSClip;
   pStaOut->dMwpIntDisp      = pStaIn->dMwpIntDisp;
   pStaOut->dMwpTime         = pStaIn->dMwpTime;
   pStaOut->dMwpMag          = pStaIn->dMwpMag;
   pStaOut->dMwAmpGM         = pStaIn->dMwAmpGM;
   pStaOut->dMwPer           = pStaIn->dMwPer;
   pStaOut->dMwTime          = pStaIn->dMwTime;
   pStaOut->dMwMag           = pStaIn->dMwMag;
   pStaOut->dMwMagBG         = pStaIn->dMwMagBG;
   pStaOut->iMwClip          = pStaIn->iMwClip;
   pStaOut->dPerMax          = pStaIn->dPerMax;
   pStaOut->dPTravTime       = pStaIn->dPTravTime; 
   pStaOut->dPStartTime      = pStaIn->dPStartTime;
   pStaOut->dPEndTime        = pStaIn->dPEndTime;
   pStaOut->dRTravTime       = pStaIn->dRTravTime;
   pStaOut->dRStartTime      = pStaIn->dRStartTime;
   pStaOut->dREndTime        = pStaIn->dREndTime;
   for ( i=0; i<MAX_SPECTRA; i++ ) pStaOut->dMwAmpSp[i]   = pStaIn->dMwAmpSp[i];
   for ( i=0; i<MAX_SPECTRA; i++ ) pStaOut->dMwPerSp[i]   = pStaIn->dMwPerSp[i];
   for ( i=0; i<MAX_SPECTRA; i++ ) pStaOut->dMwAmpSpBG[i] = pStaIn->dMwAmpSpBG[i];
   for ( i=0; i<MAX_SPECTRA; i++ ) pStaOut->dMwMagSp[i]   = pStaIn->dMwMagSp[i];
   for ( i=0; i<MAX_SPECTRA; i++ ) pStaOut->dMwMagSpBG[i] = pStaIn->dMwMagSpBG[i];
   pStaOut->dFreq            = pStaIn->dFreq;
   pStaOut->dPStrength       = pStaIn->dPStrength;
   pStaOut->dRes             = pStaIn->dRes;
   pStaOut->dDelta           = pStaIn->dDelta;
   pStaOut->dAzimuth         = pStaIn->dAzimuth;
   pStaOut->dFracDelta       = pStaIn->dFracDelta;
   pStaOut->dSnooze          = pStaIn->dSnooze;
   pStaOut->dCooze           = pStaIn->dCooze;
   pStaOut->dThetaEnergy     = pStaIn->dThetaEnergy;
   pStaOut->dThetaMoment     = pStaIn->dThetaMoment;
   pStaOut->dTheta           = pStaIn->dTheta;
}

  /**************************************************************
   *                   FindDataEndSWD()                         *
   *                                                            *
   * This function finds where data ends within the circular    *
   * buffer.                                                    *
   *                                                            *
   * Arguments:                                                 *
   *  pSta        Array of STATION structures                   *
   *                                                            *
   **************************************************************/      

void FindDataEndSWD( STATION *pSta )
{
   long    lLastNonZero;            /* Latest index with data != 0 */
   long    lNumSamp;                /* Number of data samples */
   long    lStartIndex;             /* Index where data starts */
   long    lTemp, i;                /* Counters */

/* See if any data was on this trace */
   lLastNonZero = 0;
   if ( pSta->dDataStartTime > 0.1 )     /* If yes, at what time does it end? */
   {               /* Find index where data actually starts (probably lIndex) */
      lStartIndex = (long) ((pSta->dDataStartTime-pSta->dStartTime) *
                     pSta->dSampRate) + pSta->lIndex;
      while ( lStartIndex < 0 ) lStartIndex += pSta->lRawCircSize;
      while ( lStartIndex >= pSta->lRawCircSize )
         lStartIndex -= pSta->lRawCircSize;

/* Find the number of samples of data read in */
      lNumSamp = (long) ((pSta->dEndTime-pSta->dStartTime)*pSta->dSampRate) + 1;

/* Loop through these to see if data stops before last sample */
      for ( i=0; i<lNumSamp; i++ )
      {
         lTemp = i + lStartIndex;
         if ( lTemp >= pSta->lRawCircSize ) lTemp -= pSta->lRawCircSize;
         if ( pSta->plRawCircBuff[lTemp] != 0 )        /* Possibly stops here */
            lLastNonZero = i;
      }
/* If it's close to end, assume it is to end */
      if ( lNumSamp-lLastNonZero < NUM_AT_ZERO ) lLastNonZero = lNumSamp;
/* Convert index into MJS time */
      pSta->dDataEndTime = pSta->dDataStartTime +
                           ((double) (lLastNonZero-1)/pSta->dSampRate);
   }                               
}

      /******************************************************************
       *                            InitP()                             *
       *                                                                *
       *  This function initializes a Pdata in a STATION structure.     *
       *                                                                *
       *  October, 2010: Adjust to modify Station structure.            *
       *                                                                *
       *  Arguments:                                                    *
       *   pP               Station structure                           *
       *                                                                *
       ******************************************************************/
	   
void InitP( STATION *pSta )
{
   int     i;
   LATLON  ll;

   ll.dLat = pSta->dLat;
   ll.dLon = pSta->dLon;
   GeoCent( &ll );
   GetLatLonTrig( &ll );
   pSta->dCoslat = ll.dCoslat;
   pSta->dSinlat = ll.dSinlat;
   pSta->dCoslon = ll.dCoslon;
   pSta->dSinlon = ll.dSinlon;
   strcpy( pSta->szPhase, "e" );
   pSta->iClipIt            = 1;
   pSta->iAhead             = 0;
   pSta->iPickCnt           = 0;
   pSta->lPickIndex         = -1;
   pSta->dPTime             = 0.;
   pSta->dPhaseTime         = 0.;
   pSta->dExpectedPTime     = 0.;
   pSta->stPTime.wYear      = 0;
   pSta->stPTime.wMonth     = 0;
   pSta->stPTime.wDayOfWeek = 0;
   pSta->stPTime.wDay       = 0;
   pSta->stPTime.wHour      = 0;
   pSta->stPTime.wMinute    = 0;
   pSta->stPTime.wSecond    = 0;
   pSta->stPTime.wMilliseconds = 0;   
   pSta->cFirstMotion       = '?';
   pSta->dMbAmpGM           = 0.;
   pSta->dMbPer             = 0.;
   pSta->dMbMag             = 0.;
   pSta->dMbTime            = 0.;
   pSta->iMbClip            = 0;
   pSta->dMlAmpGM           = 0.;
   pSta->dMlPer             = 0.;
   pSta->dMlTime            = 0.;
   pSta->dMlMag             = 0.;
   pSta->iMlClip            = 0;
   pSta->dMSAmpGM           = 0.;
   pSta->dMSPer             = 0.;
   pSta->dMSTime            = 0.;
   pSta->dMSMag             = 0.;
   pSta->iMSClip            = 0;
   pSta->dMwpIntDisp        = 0.;
   pSta->dMwpTime           = 0.;
   pSta->dMwpMag            = 0.;
   pSta->dMwAmpGM           = 0.;
   pSta->dMwPer             = 0.;
   pSta->dMwMag             = 0.;
   pSta->dMwMagBG           = 0.;
   pSta->dMwTime            = 0.;
   pSta->iMwClip            = 0;
   pSta->dPerMax            = 0.;        
   for ( i=0; i<MAX_SPECTRA; i++ ) pSta->dMwAmpSp[i]   = 0.;
   for ( i=0; i<MAX_SPECTRA; i++ ) pSta->dMwAmpSpBG[i] = 0.;
   for ( i=0; i<MAX_SPECTRA; i++ ) pSta->dMwPerSp[i]   = 0.;
   for ( i=0; i<MAX_SPECTRA; i++ ) pSta->dMwMagSp[i]   = 0.;
   for ( i=0; i<MAX_SPECTRA; i++ ) pSta->dMwMagSpBG[i] = 0.;
   pSta->dPTravTime   = 0.;    
   pSta->dPStartTime  = 0.;    
   pSta->dPEndTime    = 0.;    
   pSta->dRTravTime   = 0.;    
   pSta->dRStartTime  = 0.;    
   pSta->dREndTime    = 0.;    
   pSta->dRes         = 0.;
   pSta->dFracDelta   = 0.;
   pSta->dSnooze      = 0.;
   pSta->dCooze       = 0.;
   pSta->dThetaEnergy = 0.;
   pSta->dThetaMoment = 0.;
   pSta->dTheta       = 0.;
   pSta->dPStrength   = 0.;
   pSta->iUseMe       = 1;
   pSta->iBin         = 0;
//   pSta->iLPAlarmSent = 0;
   pSta->iTrigger     = 0;
   strcpy( pSta->szHypoID, "-1" );
}

      /******************************************************************
       *                         InitPSmall()                           *
       *                                                                *
       *  This function initializes certain Pdata in a STATION          *
       *  structure.                                                    *
       *                                                                *
       *  Arguments:                                                    *
       *   pP               Station structure                           *
       *                                                                *
       ******************************************************************/
	   
void InitPSmall( STATION *pSta )
{
   int     i;

   strcpy( pSta->szPhase, "e" );
   pSta->lPickIndex         = -1;
   pSta->dPTime             = 0.;
   pSta->dPhaseTime         = 0.;
   pSta->stPTime.wYear      = 0;
   pSta->stPTime.wMonth     = 0;
   pSta->stPTime.wDayOfWeek = 0;                         
   pSta->stPTime.wDay       = 0;
   pSta->stPTime.wHour      = 0;
   pSta->stPTime.wMinute    = 0;
   pSta->stPTime.wSecond    = 0;
   pSta->stPTime.wMilliseconds = 0;   
   pSta->cFirstMotion       = '?';
   pSta->dMbAmpGM           = 0.;
   pSta->dMbPer             = 0.;
   pSta->dMbMag             = 0.;
   pSta->dMbTime            = 0.;
   pSta->iMbClip            = 0;
   pSta->dMlAmpGM           = 0.;
   pSta->dMlPer             = 0.;
   pSta->dMlTime            = 0.;
   pSta->dMlMag             = 0.;
   pSta->iMlClip            = 0;
   pSta->dMSAmpGM           = 0.;
   pSta->dMSPer             = 0.;
   pSta->dMSTime            = 0.;
   pSta->dMSMag             = 0.;
   pSta->iMSClip            = 0;
   pSta->dMwpIntDisp        = 0.;
   pSta->dMwpTime           = 0.;
   pSta->dMwpMag            = 0.;
   pSta->dMwAmpGM           = 0.;
   pSta->dMwPer             = 0.;
   pSta->dMwMag             = 0.;
   pSta->dMwMagBG           = 0.;
   pSta->dMwTime            = 0.;
   pSta->iMwClip            = 0;
   pSta->dPerMax            = 0.;        
   for ( i=0; i<MAX_SPECTRA; i++ ) pSta->dMwAmpSp[i]   = 0.;
   for ( i=0; i<MAX_SPECTRA; i++ ) pSta->dMwAmpSpBG[i] = 0.;
   for ( i=0; i<MAX_SPECTRA; i++ ) pSta->dMwPerSp[i]   = 0.;
   for ( i=0; i<MAX_SPECTRA; i++ ) pSta->dMwMagSp[i]   = 0.;
   for ( i=0; i<MAX_SPECTRA; i++ ) pSta->dMwMagSpBG[i] = 0.;
   pSta->dPTravTime   = 0.;    
   pSta->dPStartTime  = 0.;    
   pSta->dPEndTime    = 0.;    
   pSta->dRTravTime   = 0.;    
   pSta->dRStartTime  = 0.;    
   pSta->dREndTime    = 0.;    
   pSta->dRes         = 0.;
   pSta->dFracDelta   = 0.;
   pSta->dSnooze      = 0.;
   pSta->dCooze       = 0.;
   pSta->dThetaEnergy = 0.;
   pSta->dThetaMoment = 0.;
   pSta->dTheta       = 0.;
   pSta->dPStrength   = 0.;
   pSta->iUseMe       = 1;
   pSta->iBin         = 0;
//   pSta->iLPAlarmSent = 0;
   strcpy( pSta->szHypoID, "-1" );
}

  /******************************************************************
   *                           MovingAvg()                          *
   *                                                                *
   *  Determine and update moving averages of absolute signal value *
   *  (called LTA here) and differential function (called MDF). Peak*
   *  noise (unfiltered) is also noted here for each LTASamps.      *
   *  NOTE: Incoming data must be short-period or filtered.         *
   *                                                                *
   *  March, 2004: Changed background noise computation to RMS for  *
   *               Mwp computations                                 *
   *                                                                *
   *  Arguments:                                                    *
   *    LongSample  One waveform data sample                        *
   *    Sta         Station data array                              *
   *    lLTASamps   # of samples per moving avg block               *
   *    RawSample   Un-filtered waveform data sample                *
   *    lNumConsec  Maximum samples to add up MDF (based on MinFreq)*
   *                                                                *
   ******************************************************************/

void MovingAvg( long LongSample, STATION *Sta, long lLTASamps,  
                long RawSample, long lNumConsec )
{
   static long    lHigh, lLow; /* Peak/trough signal values for each interval */

/* Copy new sample to structure and compute DF */
   Sta->lSampNew = (int32_t)LongSample;
   Sta->lSampRaw = (int32_t)RawSample;
   Sta->lMDFNew = Sta->lSampNew - Sta->lSampOld;

/* Add last sample and MDF to running totals and noise levels */
   if ( Sta->lLTACtr < lLTASamps )
   {
      if ( Sta->iPickStatus == 1 )
      {
         Sta->dSumLDC      += (double) Sta->lSampOld;
         Sta->dSumLDCRaw   += (double) Sta->lSampRaw;
         Sta->dSumLTA      += (double) (labs( Sta->lSampOld ));
         Sta->dSumRawNoise += (((double)Sta->lSampRaw/Sta->dSens) *
                               ((double)Sta->lSampRaw/Sta->dSens));
      }
      else
      {
         Sta->dSumLDC      += ((double) (Sta->lSampOld) - Sta->dAveLDC);
         Sta->dSumLDCRaw   += ((double) (Sta->lSampRaw) - Sta->dAveLDCRaw);
         Sta->dSumLTA      += (fabs( (double) Sta->lSampOld-Sta->dAveLDC ) -
                               Sta->dAveLTA);
         Sta->dSumRawNoise += (((double) Sta->lSampRaw-Sta->dAveLDCRaw)/Sta->dSens *
                               ((double) Sta->lSampRaw-Sta->dAveLDCRaw)/Sta->dSens);
      }
      if ( Sta->lSampRaw > lHigh ) lHigh = Sta->lSampRaw;
      if ( Sta->lSampRaw < lLow  ) lLow  = Sta->lSampRaw;
   }
   else               /* Compute new LTAs and noise level */
   {
      if ( Sta->iPickStatus == 1 )
      {
         Sta->dAveMDF    = Sta->dSumMDF    / (double) Sta->lCycCntLTA;
         Sta->dAveLDC    = Sta->dSumLDC    / (double) (lLTASamps-1);
         Sta->dAveLDCRaw = Sta->dSumLDCRaw / (double) (lLTASamps-1);
         Sta->dAveLTA    = Sta->dSumLTA    / (double) (lLTASamps-1);
         Sta->dAveRawNoise = sqrt( Sta->dSumRawNoise /
                                  (double) (lLTASamps-1) );
         Sta->iPickStatus = 2;
      }
      else
      {
         Sta->dAveMDF    += (0.5*Sta->dSumMDF    / (double) Sta->lCycCntLTA);
         Sta->dAveLDC    += (0.5*Sta->dSumLDC    / (double) (lLTASamps));
         Sta->dAveLDCRaw += (0.5*Sta->dSumLDCRaw / (double) (lLTASamps));
         Sta->dAveLTA    += (0.5*Sta->dSumLTA    / (double) (lLTASamps));
         Sta->dAveRawNoise = 0.9*Sta->dAveRawNoise +
                             0.1*sqrt( Sta->dSumRawNoise /
                                       (double) (lLTASamps) );
      }
      Sta->lRawNoise = lHigh - lLow;
      if ( Sta->lRawNoise == 0 ) Sta->lRawNoise = 1;
	  
/* Reset summation variables and compute thresholds */	  
      lHigh             = -10000000;
      lLow              =  10000000;
      Sta->dSumMDF      =  0.;
      Sta->dSumLDC      =  0.;
      Sta->dSumLDCRaw   =  0.;
      Sta->dSumRawNoise =  0.;
      Sta->dSumLTA      =  0.;
      Sta->lLTACtr      =  0;
      Sta->lCycCntLTA   =  0;
      Sta->dMDFThresh   =  0.5  * (double) Sta->iSignalToNoise * Sta->dAveMDF;
      Sta->dLTAThresh   =  1.57 * (double) Sta->iSignalToNoise * Sta->dAveLTA;
   }   
   
/* Check for cycle changes (convert DF to MDF) */
   if ( (Sta->lMDFOld <  0 && Sta->lMDFNew <  0) ||
        (Sta->lMDFOld >= 0 && Sta->lMDFNew >= 0) )
   {
         /* No changes, continuing adding up MDF */
      Sta->lSampsPerCycLTA++;
      if ( Sta->lSampsPerCycLTA < lNumConsec )
         Sta->lMDFRunningLTA += Sta->lMDFNew;
      else
      {
         if ( Sta->iPickStatus == 1 )
            Sta->dSumMDF += (double) (labs( Sta->lMDFRunningLTA ));
         else
            Sta->dSumMDF += ((double) (labs( Sta->lMDFRunningLTA )) -
                             Sta->dAveMDF);
         Sta->lMDFRunningLTA = Sta->lMDFNew;
         Sta->lCycCntLTA++;
         Sta->lSampsPerCycLTA = 0;
      }
   }
   else  /* Cycle has changed sign, start anew */
   {
      if ( Sta->iPickStatus == 1 )
         Sta->dSumMDF += (double) (labs( Sta->lMDFRunningLTA ));
      else
         Sta->dSumMDF += ((double) (labs( Sta->lMDFRunningLTA )) -
                          Sta->dAveMDF);
      Sta->lMDFRunningLTA = Sta->lMDFNew;
      Sta->lCycCntLTA++;
      Sta->lSampsPerCycLTA = 0;
   }   
   
/* Update old with new value and increment averages counter (MDF updated
   elsewhere) */   
   Sta->lSampOld = Sta->lSampNew;
   Sta->lLTACtr++;           /* LTA counter */
}

 /***********************************************************************
  *                              PickV()                                *
  *                                                                     *
  *      Evaluate one demultiplexed message with the Veith P-picker     *
  *                                                                     *
  *  The P-pick detection algorithm used here was developed by Veith in *
  *  1978 and is described in Technical Note 1/78, "Seismic Signal      *
  *  Detection Algorithm" by Karl F. Veith, Teledyne GeoTech.           *
  *  The signal must go through four processing stages before           *
  *  a pick is declared.  The first stage is a simple test which looks  *
  *  for higher than normal signal amplitudes.  Actually, it is not the *
  *  amplitude that is tested but the accumulated difference between    *
  *  samples (the MDF). This is compared to the background MDF every    *
  *  sample.  The background MDF is computed with a moving averages     *
  *  technique.  Every LTASECONDS the average MDF is computed and this  *
  *  is averaged with the existing MDF to produce a new MDF.            *
  *                                                                     *
  *  The second phase of signal processing consists of two tests.  Test *
  *  1 checks that the MDF exceeds a trigger threshold for lNumConsec   *
  *  samples after Phase 1 is passed twice, but before 3*lNumConsec     *
  *  samples are processed. Test 2 states that the amplitude must exceed*
  *  the signal-to-noise ratio times the LTA (*1.57) at some time during*
  *  Phase 2.  If the Phase 1 trigger was not exceeded more than it was *
  *  at any time during Phase 2, the phase fails.                       *
  *                                                                     *
  *  Phase 3 consists of three tests.  Test 1 requires MDF greater than *
  *  the trigger threshold at least 6 times in opposing directions      *
  *  (i.e. it must see three full cycles of signal).  Test 2 requires   *
  *  the avg. frequency of the above oscillations to be greater than    *
  *  FMINFREQ/2.  Test 3 requires the MDF to be above the trigger for at*
  *  least half the time it takes to pass the above two tests.  If      *
  *  these three Phases are passed, a P-pick is declared.               *
  *                                                                     *
  *  Another check was added in 2007.  If a pick is high frequency,     *
  *  it also must be above a set signal-to-noise ratio.  If not, the    *
  *  pick is rejected.  This helps prevent spurious noise picks.        *
  *                                                                     *
  *  In 2009, the pick time is adjusted using the AIC picker function.  *
  *  Normally, the adjustment moves the pick time back in time.  In     *
  *  general, the pick is more accurate with the AIC adjustment.        *
  *                                                                     *
  *  At this point, the time that Phase 1 was passed is saved as the    *
  *  event's P-time.  The picker continues to evaluate the signal for   *
  *  magnitude data.  Mb, Mwp, and Ml's are computed here. After the    *
  *  magnitude processing has finished, evaluation on this channel      *
  *  ceases and all variables are reinitialized.                        *
  *                                                                     *
  *  The picker evaluates data at any sample rate. The P-picker works   *
  *  best on the short period filtered data from broadband signal.  The *
  *  broadband is used for Mwp processing.  This P-picker did not work  *
  *  well when tried on broadband signal.                               *
  *                                                                     *
  *  August, 2014: Added limit to Regional Alarm announcements.         *
  *  July, 2014: Added option to pick on acceleration data.             *
  *  April, 2009: Added adjustment Neural net check to verify pick.     *
  *  March, 2009: Added adjustment of pick based on AIC picker.         *
  *  May, 2008: Combined with pick software in Analyze.                 *
  *  May, 2008: Removed sine wave cal discriminator.                    *
  *  July, 2007: Added fourth phase which compares frequency and signal *
  *              strength.                                              *
  *  December, 2004: Added multi-station alarm and signal strength      *
  *                  determination.                                     *
  *                                                                     *
  *  Arguments:                                                         *
  *     Sta              Pointer to station being processed             *
  *     dStartTime       Start time (1/1/70 seconds) of the packet      *
  *     iAlarmOn         1->Alarm function enabled, 0->Disabled         *
  *     i2StnAlarmOn     1->multo station alarm on, 0->off              *
  *     dAlarmTimeout    Time (sec) to re-start alarm after trig        *
  *     dMinFreq         Minimum P frequency (hz) of interest           *
  *     dLTASeconds      Moving average length of time (seconds)        *
  *     iMwpSeconds      Max # seconds to evaluate P for Mwp            *
  *     dMwpSigNoise     Auto-Mwp necessary signal-to-noise ratio       *
  *     iLGSeconds       Seconds after P in which max LG can be         *
  *                       computed for Ml (excluding 1st MbCycles)      *
  *     iMbCycles        # 1/2 cycles after P Mb can be computed        *
  *     dSNLocal         S:N which must be exceeded for local P-picks   *
  *     dMinFLoc         Frequency to identify potential local events   *
  *     ucMyModID        Calling module ID                              *
  *     siAlarmRegion    Calling module Alarm Ring                      *
  *     siPRegion        Calling module P Ring                          *
  *     ucEWHTypeAlarm   Earthworm alarm message number                 *
  *     ucEWHTypePickTWC Earthworm pick message number                  *
  *     ucEWHMyInstID    Earthworm Institute ID                         *
  *     WaveRaw          Array of unfiltered signal                     *
  *     WaveLong         Pointer to array of filtered data              *
  *     pAS              ALARMSTRUCT array with regional alarm data     *
  *     iNumReg          Number of regions in ALARMSTRUCT               *
  *     iRT              1->called from pick_wcatwc; 0->not real-time   *
  *     piSaveP          For Analyze; 1->Good pick; 0->No pick          *
  *     iNN              1->Run pick thorugh Neural Net; 0->Dont        *
  *     iPickAcc         1->Run pick thorugh acceleration data; 0->Dont *
  *     WaveAcc          Pointer to array of acceleration data          *
  *                                                                     *
  ***********************************************************************/

void PickV( STATION *Sta, double dStartTime, int iAlarmOn, int i2StnAlarmOn,
            double dAlarmTimeout, double dMinFreq, double dLTASeconds,
            int iMwpSeconds, double dMwpSigNoise, int iLGSeconds,
            int iMbCycles, double dSNLocal, double dMinFLoc, 
            unsigned char ucMyModID, SHM_INFO siAlarmRegion, SHM_INFO siPRegion,
            unsigned char ucEWHTypeAlarm, unsigned char ucEWHTypePickTWC,
            unsigned char ucEWHMyInstID, int32_t WaveRaw [], int32_t *WaveLong,
            ALARMSTRUCT pAS[], int iNumReg, int iRT, int *piSaveP, int iNN,
            int iPickAcc, int32_t *WaveAcc )
{
   double  dData[MAX_NN_SAMPS]; /* Temp data array for AIC pick */
   int     i, ii;
   static int iAlarmCnt;        /* Number of alarms per alarm period */
   int     iSendAlarm;          /* 0=Don't send; 1=Send */
   long    lCnt;                /* Index counter for transfer of raw data 
                                   circular buffer to Mwp buffer */
   int32_t lData[MAX_NN_SAMPS]; /* Temp data array for AIC pick */
   static  long lLTASamps;      /* # samps / moving avg block */
   long    lNewPIndex;          /* AIC adjusted pick index */
   long    lNumAICSamps;        /* Number of samples to send to AIC picker */
   long    lNumConsec;          /* Max. # samps. to evaluate in 1/2 cycle */
   long    lNumNNSamps;         /* Number of samples to send to Neural net */
   long    lNumSamp;            /* Number of samples to evaluate */
   static  long lPickCounter=0; /* Pick Counter */
   long    lTemp;
   double  dOriginalPickTime;
   time_t  now;                 /* Present 1/1/1970 time */
#ifdef _MAT_LAB_
   mxArray *pInputData;         /* input data array to Neural Net */
   mxArray *pPickStatus = NULL; /* return value from the Neural Net */
   double  *dyNN, dRet;
   int iRet;
#endif

/* Temp buffer for compatibility with AutoMwp */
   *piSaveP = 0;
  
/* Compute number of samples */
   lNumSamp = (long) ((Sta->dDataEndTime-dStartTime)*Sta->dSampRate +
                       0.01) + 1;

/* Process data through alarm function if set in .sta file (and recent data) */
   time( &now );
   if ( iRT == 1 )
      if ( now-Sta->dDataEndTime < RECENT_ALARM_TIME )
         if ( iAlarmOn )
            if ( Sta->iAlarmStatus >= 1 )
               SeismicAlarm( Sta, lNumSamp, WaveLong, 1, ucMyModID, 
                siAlarmRegion, ucEWHTypeAlarm, ucEWHMyInstID, dAlarmTimeout,
                dStartTime, 1 );

/* Compute Maximum number of samples to allow per cycle */
   lNumConsec = (long) (Sta->dSampRate / (2.*dMinFreq) + 0.0001);

/* Compute number of samples in the moving average block */
   lLTASamps = (long) (Sta->dSampRate*dLTASeconds + 0.0001);

/* Loop over all samples in packet */
   for ( i=0; i<lNumSamp; i++ )
   {
/* First, update averages */
      if ( iPickAcc == 1 )
         MovingAvg( WaveAcc[i],  Sta, lLTASamps, WaveRaw[i], lNumConsec );
	  else
         MovingAvg( WaveLong[i], Sta, lLTASamps, WaveRaw[i], lNumConsec );
      
/* Next, update pick buffer - !!! */
      if ( iPickAcc == 1 )
         Sta->plPickBuf[Sta->lPickBufIndex] = WaveAcc[i];      
	  else
         Sta->plPickBuf[Sta->lPickBufIndex] = WaveLong[i];      
      Sta->lPickBufIndex++;
      if ( Sta->lPickBufIndex >= MAX_NN_SAMPS || Sta->lPickBufIndex < 0 )
         Sta->lPickBufIndex = 0;
      
/* Update neural net buffer - This is new !!!*/
      Sta->plPickBufRaw[Sta->lPickBufRawIndex] = WaveRaw[i];
      Sta->lPickBufRawIndex++;
      if ( Sta->lPickBufRawIndex >= MAX_NN_SAMPS || Sta->lPickBufRawIndex < 0 )
         Sta->lPickBufRawIndex = 0;

/* If Station passed 1st initialization or just had pick, proceed with picker */
      if ( Sta->iPickStatus >= 2 )
      {
/* Add to Mwp array if Phase 1 passed */  
         if ( Sta->lMwpCtr >  0 &&
              Sta->lMwpCtr <= (long) (Sta->dSampRate*iMwpSeconds+0.001)-2 &&
              Sta->lMwpCtr <= MAXMWPARRAY-2 ) Sta->lMwpCtr++;
         if ( Sta->dSens > 0.0 && Sta->iComputeMwp && Sta->lMwpCtr > 0 )
         {
            Sta->plRawData[Sta->lMwpCtr] = (int32_t) ((double) Sta->lSampRaw -
                                           Sta->dAveLDCRawOrig);
            if ( Sta->lMwpCtr >= (long) (Sta->dSampRate*iMwpSeconds+0.001)-1 )
                 Sta->lMwpCtr =  (long) (Sta->dSampRate*iMwpSeconds+0.001)-1;
            if ( Sta->lMwpCtr >= MAXMWPARRAY-1 )
                 Sta->lMwpCtr =  MAXMWPARRAY-1;

/* Compute Mwp when array is full or every 50 seconds */
            if ( Sta->lMwpCtr == (long) (Sta->dSampRate*iMwpSeconds+0.1) ||
                (Sta->lMwpCtr %  (long) (Sta->dSampRate*50.)) == 0 )
            {
               AutoMwp( Sta, dMwpSigNoise, iMwpSeconds, 0, 2 ); 

/* If S:N was great enough to compute an Mwp, report the pick to PICK_RING */
               if ( Sta->dMwpIntDisp > 0. )
               {	
                  if ( iRT == 1 )
                     ReportPick( Sta, ucMyModID, siPRegion,
                                 ucEWHTypePickTWC, ucEWHMyInstID, 4 );
                  else if ( iRT == 0 )
                  {
                     *piSaveP = 1;
                     if ( Sta->lMwpCtr ==
                         (long) (Sta->dSampRate*iMwpSeconds+0.1)-1 ||
                          Sta->lMwpCtr == MAXMWPARRAY-1 ) return;
                  }
               }
            }
         }

/* If we are done with Lg and Mwp processing, restart computations with new
   averages */
         if ( Sta->iPickStatus == 3 && 
              dStartTime+(double)i/Sta->dSampRate-Sta->dTrigTime >=
               (double) iMwpSeconds &&
              dStartTime+(double)i/Sta->dSampRate-Sta->dTrigTime >=
               (double) iLGSeconds ) 
         {                            
            Sta->iPickStatus = 1;
            if ( iRT == 1 )
            {
               InitVar( Sta );
               Reset( Sta );
            }
            if ( iRT == 0 ) return;
            goto EndOfLoop;
         }

/* Back to the picker, first check for cycle changes */
         if ( (Sta->lMDFOld <  0 && Sta->lMDFNew <  0) ||
              (Sta->lMDFOld >= 0 && Sta->lMDFNew >= 0) )
         {     /* No changes, continuing adding up MDF */
            Sta->lSampsPerCyc++;
            if ( Sta->lSampsPerCyc < lNumConsec )
               Sta->lMDFRunning += Sta->lMDFNew;
            else
            {
               Sta->lMDFRunning = Sta->lMDFNew;
               Sta->lSampsPerCyc = 0;            
            }
         }
         else  /* Cycle has changed sign, get mags and start anew */
         {
            if ( Sta->lPhase1 == 1 )
            {
               Sta->lMDFTotal += labs( Sta->lMDFRunning );
               Sta->lMDFCnt++;
               if ( GetMbMl( Sta, i, ucMyModID, siPRegion, ucEWHTypePickTWC,
                             ucEWHMyInstID, iMbCycles, iRT, dStartTime ) < 0 )
                  goto EndOfLoop;     /* Sine-wave cal must be over */
            }
            Sta->lMDFRunning = Sta->lMDFNew;
            Sta->lSampsPerCyc = 0;
         }
		 
/* If this is a wc/atwc sine wave cal, skip further processing */		 
         if ( Sta->iCal ) goto EndOfLoop;
		 
/* Check first motion if we are in first few samples of pick */
         if ( Sta->lFirstMotionCtr >= 1 &&
              Sta->lFirstMotionCtr < FIRST_MOTION_SAMPS )
         {
            if ( (Sta->lMDFRunning < 0 && Sta->lMDFOld < 0) ||
                 (Sta->lMDFRunning >= 0 && Sta->lMDFOld >= 0) )
               Sta->lFirstMotionCtr++;
            else         /* There was a reveral so 1st motion is questionable */
            {
               Sta->cFirstMotion = '?';
               Sta->lFirstMotionCtr = 0;
            }
         }         /* If we've checked enough samples, assume 1st mo. is good */
         if ( Sta->lFirstMotionCtr == FIRST_MOTION_SAMPS )
            Sta->lFirstMotionCtr = 0;

/* If the station is picked, no need to do anything more */
         if ( Sta->iPickStatus == 3 ) goto EndOfLoop;

/* If Phase3 has been passed, wait 3s before declaring pick */
         if ( Sta->lPhase3 == 1 )
         {   
            Sta->l13sCnt++;
            if ( Sta->l13sCnt < (long) (Sta->dSampRate * 3. + 0.01))
                  goto EndOfLoop;
            else  goto PickCounter;
         }

/* Phase 1: Has MDF trigger threshold been surpassed? */
         if ( ( !Sta->lPhase1 && (double) (labs( Sta->lMDFRunning )) >=
                 Sta->dMDFThresh ) ||
              (  Sta->lPhase1 && (double) (labs( Sta->lMDFRunning )) >=
                 Sta->dMDFThreshOrig ) )
         {
            Sta->lTrigFlag = 1;
            if ( Sta->lPhase1 == 0 )
            {   /* Set phase1 passage here */  
               Sta->lPhase1 = 1;
			   
/* Save existing moving averages and thresholds */
               Sta->dMDFThreshOrig   = Sta->dMDFThresh;
               Sta->dLTAThreshOrig   = Sta->dLTAThresh;
               Sta->dAveLDCRawOrig   = Sta->dAveLDCRaw;
               Sta->dAveRawNoiseOrig = Sta->dAveRawNoise;
               Sta->dAveMDFOrig      = Sta->dAveMDF;
               Sta->lRawNoiseOrig    = Sta->lRawNoise;
			   
/* Look for first motion */                         
               Sta->lFirstMotionCtr = 1;
               if (Sta->lMDFRunning > 0) Sta->cFirstMotion = 'U';
               else                      Sta->cFirstMotion = 'D';

/* If this station is used for Mwp calculations, start Counter and fill buffer*/
               Sta->lMwpCtr = Sta->lSampsPerCyc;
               if ( Sta->lMwpCtr >= (long) (Sta->dSampRate*iMwpSeconds+0.1)-1 ) 
                  Sta->lMwpCtr = (long) (Sta->dSampRate*iMwpSeconds+0.1)-1;
               if ( Sta->lMwpCtr >= MAXMWPARRAY-1 )Sta->lMwpCtr = MAXMWPARRAY-1;
               if ( Sta->lMwpCtr == 0 ) Sta->lMwpCtr = 1;       
               if ( Sta->dSens > 0.0 && Sta->iComputeMwp )
               {
                  lCnt = Sta->lPickBufRawIndex - Sta->lSampsPerCyc - 1;
                  if ( lCnt < 0 ) lCnt += MAX_NN_SAMPS;
                  for ( ii=0; ii<Sta->lSampsPerCyc; ii++ )
                  {
                     Sta->plRawData[ii] = (int32_t) ((double)
                      Sta->plPickBufRaw[lCnt] - Sta->dAveLDCRawOrig);
                     lCnt++;
                     if ( lCnt >= MAX_NN_SAMPS ) lCnt -= MAX_NN_SAMPS;
                  }		  
               }

/* Save P-time (# seconds since 1/1/1970) */
               Sta->dTrigTime = dStartTime +
                (double) (i-Sta->lSampsPerCyc)/Sta->dSampRate;
            }
         }
         if ( Sta->lPhase2 == 1 ) goto Phase3;
         if ( Sta->lPhase1 == 0 ) goto Phase4;

/* Phase 2: P-Phase processing */
         Sta->lPhase2Cnt++;
         if ( Sta->lPhase2Cnt > 3*lNumConsec ) goto Reset;

/* Count trigger passes versus misses */
         if ( Sta->lTrigFlag == 1 )
         {
            Sta->lHit++;
            if ( Sta->lHit == lNumConsec ) Sta->lTest1 = 1;
         }
         else
         {
            Sta->lMis++;

/* NOTE: By Veith's paper, this test should be performed after the passing 
   of tests 1 and 2.  The picks are much better, though, if it is done 
   sample-by-sample. */
            if ( Sta->lMis > Sta->lHit ) goto Reset; // Fail test 3
         }

/* Test2 in Phase2 - Must exceed following amp. sometime in phase2 */
         if ( fabs ((double) Sta->lSampNew-Sta->dAveLDC) > Sta->dLTAThreshOrig )
            Sta->lTest2 = 1;

/* See if Phase 2 has passed */
         if ( Sta->lTest1+Sta->lTest2 != 2 ) goto Phase4;

/* Otherwise, Phase 2 has passed. So, get ready for Phase 3 */
         Sta->lPhase2 = 1;
         Sta->lNumOsc = 0;
         Sta->lHit = 0;
         Sta->lMis = 0;
         Sta->lLastSign = 0;
         if ( Sta->lMDFRunning < 0 ) Sta->lLastSign = 1;

/* Phase 3: Look for oscillatory motion */
Phase3:  Sta->lPhase3Cnt++;

/* Below is time limit for passing Phase 3 (test 2) */
         if ( Sta->lPhase3Cnt > 12*lNumConsec ) goto Reset;

/* Check to see if trigger MDF was exceeded */
         if ( Sta->lTrigFlag == 1 ) Sta->lHit++;
         else                       Sta->lMis++;
         if ( Sta->lTrigFlag == 0 ) goto Phase4;

/* Next, check for oscillations */
         Sta->lCurSign = 0;
         if ( Sta->lMDFRunning < 0 ) Sta->lCurSign = 1;
         if ( Sta->lLastSign != Sta->lCurSign ) Sta->lNumOsc++;

/* Phase3, test3 is passed when 6 reverses are noted */
         if ( Sta->lNumOsc < 6 )
         {
            Sta->lLastSign = Sta->lCurSign;
            goto Phase4;
         }

/* If MDF < trigger value more than not and for more than 4s, fail test 3 */
         if ( Sta->lMis > Sta->lHit && Sta->lMis > (long)(4.*Sta->dSampRate) ) 
            goto Reset;
	    
/* Figure out the frequency of this signal */
         if ( Sta->lMwpCtr == 0 ) Sta->lMwpCtr = 1;
         Sta->dFreq = 1. / (((double) Sta->lMwpCtr/Sta->dSampRate) / 6.);

/* Otherwise, phase 3 was passed */
         Sta->lPhase3 = 1;
         goto EndOfLoop;

/* Declare and report the pick */
PickCounter: 
         Sta->iPickStatus = 3;
         lPickCounter++;
         if ( lPickCounter >= 9999 ) lPickCounter = 1;
         Sta->lPickIndex = lPickCounter;    
         if ( fabs( Sta->dMDFThreshOrig ) < 0.0001 ) Sta->dMDFThreshOrig = 1.;
         Sta->dPStrength = ((double) Sta->lMDFTotal/(double) Sta->lMDFCnt) /
                                     Sta->dMDFThreshOrig;

/* Load local array for AIC picker adjustment - use 2 seconds after P-time
   as the end of the AIC window  - !!!*/
         lNumAICSamps = 0;
         lTemp = Sta->lPickBufIndex -
          (long) ((dStartTime + (double) (i)/Sta->dSampRate - Sta->dTrigTime)*
                   Sta->dSampRate + 0.0001) - 
          (long) ((AIC_NUM_SEC-2)*Sta->dSampRate + 0.0001) - 1;
         while ( lTemp < 0 ) lTemp += MAX_NN_SAMPS;
         while ( lTemp >= MAX_NN_SAMPS ) lTemp -= MAX_NN_SAMPS;	 

         for ( ii=0; ii<(long) ((AIC_NUM_SEC)*Sta->dSampRate + 0.0001); ii++ )
         {
            lData[ii] = Sta->plPickBuf[lTemp];
            dData[ii] = (double) lData[ii];
            lNumAICSamps++;
            lTemp++;
            if ( lTemp >= MAX_NN_SAMPS ) lTemp = 0;
         }

         logit( "t", "before- %s lNumAICSamps=%ld, PTime=%lf\n", Sta->szStation,
          lNumAICSamps, Sta->dTrigTime );

/* Determine adjusted pick index based on AIC routine !!! */	 
         lNewPIndex = 0;
         lNewPIndex = AICPick( dData, lNumAICSamps, Sta->dSampRate ); 
         dOriginalPickTime = Sta->dTrigTime;                           
												        
/* Adjust pick time based on AIC adjustment !!! */	 
         Sta->dTrigTime = (Sta->dTrigTime - AIC_NUM_SEC + 2.) +
                          (double) lNewPIndex/Sta->dSampRate; 
         logit( "t", "%s AIC PTime=%lf\n", Sta->szStation, Sta->dTrigTime );
         if ( fabs( dOriginalPickTime - Sta->dTrigTime) > (double) AIC_NUM_SEC-5. )
         {
            Sta->dTrigTime = dOriginalPickTime;
            logit( "t", "%s Pick time used=%lf\n", Sta->szStation, Sta->dTrigTime );
         }

/* This is a fourth stage that the pick must pass to be declared.  It is there
   to filter out high frequency-low strength P-picks.  This was added by PH and
   PW in June, 2007 */
         if ( iNN == 0 )  				     
         {
            if ( Sta->dPStrength <= dSNLocal && Sta->dFreq >= dMinFLoc )
            {
               logit( "t", "4th stage rejection %s F-%lf SN-%lf\n",
                      Sta->szStation, Sta->dFreq, Sta->dPStrength );
               goto Reset;                /* Did not pass phase 4 */
            }
         }   

/* Load local array for neural net check  */
         lNumNNSamps = 0;
         lTemp = Sta->lPickBufIndex - i - (long) (((dStartTime -
          Sta->dTrigTime) + (double) NN_NUM_SEC/2.)*
          Sta->dSampRate + 0.0001) - 1;
         while ( lTemp < 0 ) lTemp += MAX_NN_SAMPS;
         while ( lTemp >= MAX_NN_SAMPS ) lTemp -= MAX_NN_SAMPS;
         for ( ii=0; ii<(long) ((NN_NUM_SEC)*Sta->dSampRate + 0.0001)+1; ii++ )
         {
            dData[ii] = (double) Sta->plPickBuf[lTemp];
            lNumNNSamps++;
            lTemp++;
            if ( lTemp >= MAX_NN_SAMPS ) lTemp = 0;
         }
         logit( "t", "%s NN Array Size=%ld\n", Sta->szStation, lNumNNSamps );	

#ifdef _MAT_LAB_
/* Now, run the signal through a neural net checker to verify good pick */
         if ( iNN == 1 )
         {
            pInputData = mxCreateDoubleMatrix( lNumNNSamps, 1, mxREAL );
            memcpy( mxGetPr( pInputData ), dData, lNumNNSamps*sizeof(double) );
            mlfNNClassifer( 1, &pPickStatus, pInputData );
            dyNN = mxGetPr( pPickStatus );
            dRet = *dyNN;
            iRet = (int) dRet; 
            mxDestroyArray( pInputData );
            logit( "t", "%s, to Neural Network; NNreturn=%d\n", Sta->szStation,
                   iRet );
            if ( iRet  == 2 )           // Then pick no good 
               goto Reset;
         }
#endif

/* If we get here, this is very likely a good pick with an adjusted P-time based
   on the AIC picker; so, reload Mwp array based on the new pick time. */
         if ( Sta->lMwpCtr > 0 )        /* This station has an Mwp started */
         {
            lCnt = (long) ((dStartTime + i*(1./Sta->dSampRate) -
                            Sta->dTrigTime)*Sta->dSampRate + 0.0001) + 1;
            lTemp = Sta->lPickBufRawIndex - (long) ((dStartTime +
             i*(1./Sta->dSampRate) - Sta->dTrigTime)*Sta->dSampRate +
             0.0001) - 1;
            while ( lTemp < 0 ) lTemp += MAX_NN_SAMPS;
            while ( lTemp >= MAX_NN_SAMPS ) lTemp -= MAX_NN_SAMPS;
            for ( ii=0; ii<lCnt; ii++ )
            {
               Sta->plRawData[ii] = (int32_t) ((double)
                Sta->plPickBufRaw[lTemp] - Sta->dAveLDCRawOrig);
               lTemp++;
               if ( lTemp >= MAX_NN_SAMPS ) lTemp = 0;
               if ( ii >= (long) (Sta->dSampRate*iMwpSeconds+0.1)-1 ) break;
               if ( ii >= MAXMWPARRAY-1 ) break;
               Sta->lMwpCtr = ii;
            }
         }            
   
/* Now report the pick */
         if ( iRT == 1 )
            ReportPick( Sta, ucMyModID, siPRegion, ucEWHTypePickTWC,
                        ucEWHMyInstID, 4 );
         else if ( iRT == 0 ) *piSaveP = 1;

/* Check for multiple pick alarm */
         time( &now );	                  /* See that it's not old data */
         if ( now-Sta->dTrigTime < RECENT_ALARM_TIME )
            if ( i2StnAlarmOn == 1 && iRT == 1 )
            {
/* Only allow one alarm every 22 minutes so PKP does not trigger 
   multiple alarms for same event */
               if ( (double) (now-tAlarm) > 22.*60. ) 
               {                /* This limits # of Alarm Announcements */
                  logit( "t", "Reset tAlarm, now=%ld, tAlarm=%ld\n", 
                   (long)now, (long)tAlarm );
                  tAlarm    = now;
                  iAlarmCnt = 0;
               }                     
               if ( iAlarmCnt == 0 ) iSendAlarm = 1;
               else                  iSendAlarm = 0;
               if ( CheckForAlarm( Sta, pAS, iNumReg, ucMyModID,
                     siAlarmRegion, ucEWHTypeAlarm, ucEWHMyInstID, 
                     iSendAlarm ) == 1 ) 
               {
                  iAlarmCnt++;
                  logit( "t", "Reg. Alarm on %s; Send=%d; Cnt=%d\n", 
                   Sta->szStation, iSendAlarm, iAlarmCnt );
               }
            }
         goto Phase4;

/* Reset some picker variables, one of the tests failed */
Reset:   Reset( Sta );
                        
/* Start of Phase 4 (Phase 4 is mainly skipped here.  Events are terminated 
   when magnitude information has been computed). */
Phase4:  Sta->lTrigFlag = 0;
EndOfLoop:;
      }
      Sta->lMDFOld = Sta->lMDFNew;
   }
}

     /**************************************************************
      *                         PPickStruct()                      *
      *                                                            *
      * Fill in PPICK structure from PickTWC message.              *
      *                                                            *
      * October, 2010: Now use STATION structure instead of PPICK. *
      *                                                            *
      * Arguments:                                                 *
      *  PIn         PickTWC message from ring                     *
      *  pSta        Temp Station data structure                   *
      *  TypePickTWC Earthworm message type expected               *
      *                                                            *
      * Return - 0 if OK, -1 if wrong message type                 *
      **************************************************************/

int PPickStruct( char *PIn, STATION *pSta, unsigned char TypePickTWC )
{
   int      iMessageType, iModId, iInst;  /* Incoming logo */

/* Break up incoming message
   *************************/
   sscanf( PIn, "%d %d %d %s %s %s %s %ld %d %lf %c %s %lf %lf %lf %lf "
                "%lf %lf %lf %lf %lf %lE %lf %s %lf %lf",
           &iMessageType, &iModId, &iInst, pSta->szStation, pSta->szChannel,
            pSta->szNetID, pSta->szLocation, &pSta->lPickIndex,
           &pSta->iUseMe, &pSta->dPTime, &pSta->cFirstMotion, pSta->szPhase,
           &pSta->dMbAmpGM, &pSta->dMbPer, &pSta->dMbTime,
           &pSta->dMlAmpGM, &pSta->dMlPer, &pSta->dMlTime,
           &pSta->dMSAmpGM, &pSta->dMSPer, &pSta->dMSTime,
           &pSta->dMwpIntDisp, &pSta->dMwpTime, pSta->szHypoID,
           &pSta->dPStrength, &pSta->dFreq );

   if ( iMessageType == TypePickTWC )
      return 0;
   else
   {
      logit( "te", "Incoming message type %d; must be PickTWC\n",
             iMessageType );
      return -1;
   }
}

 /***********************************************************************
  *                              Reset()                                *
  *      Reset some picker variables when a phase/test has failed       *
  *                                                                     *
  *  Arguments:                                                         *
  *     Sta              Pointer to station being processed             *
  *                                                                     *
  ***********************************************************************/

void Reset( STATION *Sta )
{
   Sta->iClipIt = 1;
   Sta->lPhase1 = 0;
   Sta->lPhase2 = 0;
   Sta->lPhase3 = 0;               
   Sta->lTest1 = 0;
   Sta->lTest2 = 0;
   Sta->lHit = 0;
   Sta->lMis = 0;
   Sta->dMaxPk = 0.;
   Sta->lCycCnt = 0;
   Sta->lPer = 0;
   Sta->lMlPer = 0;
   Sta->dMlTime = 0.;
   Sta->lMbPer = 0;
   Sta->dMbTime = 0.;
   Sta->dMlAmpGM = 0.;
   Sta->dMbAmpGM = 0.;
   Sta->lMwpCtr = 0;
   Sta->iCal = 0;
   Sta->cFirstMotion = '?';
   Sta->lFirstMotionCtr = 0;
   Sta->lMagAmp = 0;
   Sta->dAvAmp = 0.;
   Sta->lSWSim = 0;
   Sta->lPhase2Cnt = 0;
   Sta->lPhase3Cnt = 0;
   Sta->dTrigTime = 0.;
   Sta->dMDFThreshOrig = 0.;
   Sta->dLTAThreshOrig = 0.;
   Sta->dAveLDCRawOrig = 0.;
   Sta->dAveRawNoiseOrig = 0.;
   Sta->dAveMDFOrig = 0.;
   Sta->lRawNoiseOrig = 0;
   Sta->lMDFTotal = 0;
   Sta->lMDFCnt = 0;
   Sta->dPStrength = 0.;
   Sta->dFreq = 0.;
}

 /***********************************************************************
  *                          SeismicAlarm()                             *
  *                                                                     *
  *  Check signal for large, cyclical motion.  Send alarm if noted.     *
  *                                                                     *
  *  August, 2014: Added return value - PW.                             *
  *                                                                     *
  *  Arguments:                                                         *
  *     Sta              Pointer to station being processed             *
  *     lNumSamp         Number of samples to evaluate                  *
  *     WaveLong         Pointer to array of filtered data              *
  *     iType            Alarm Type - 1=SP Alarm, 3=LP Alarm            *
  *     ucMyModID        Calling module ID                              *
  *     siAlarmRegion    Calling module Alarm Ring                      *
  *     ucEWHTypeAlarm   Earthworm alarm message number                 *
  *     ucEWHMyInstID    Earthworm Institute ID                         *
  *     dAlarmTimeout    Time till station can be re-alarmed (s)        *
  *     dStartTime       Start time (1/1/70 seconds) of the packet      *
  *     iSendReport      If equal to 1 then send Report, otherwise      *
  *                      do not send                                    *
  *                                                                     *
  *  Returns:                                                           *
  *     int              1 if Alarm; else 0                             *
  *                                                                     *
  ***********************************************************************/

int SeismicAlarm( STATION *Sta, long lNumSamp,
                  int32_t *WaveLong, int iType, unsigned char ucMyModID,
                  SHM_INFO siAlarmRegion, unsigned char ucEWHTypeAlarm,
                  unsigned char ucEWHMyInstID, double dAlarmTimeout,
                  double dStartTime, int iSendReport )
{
   int  i;
   int  iAlarm = 0;        /* 1 if Alarm is determined */
   long lNumConsecA;       /* Max # samples which can pass without an alarm
                              being declared. Reset variables when no alarm. */
   
/* Compute lNumConsecA */
   lNumConsecA = (long) (Sta->dSampRate / (2.*Sta->dAlarmMinFreq) + 0.0001);

/* Initialize SeismicAlarm variables if necessary */
   if ( Sta->iAlarmStatus == 1 )
   {
      Sta->lAlarmP1 = 0;
      Sta->lAlarmCycs = 0;
      Sta->lAlarmSamps = 0;		     
      Sta->iAlarmStatus = 2;
   }

/* Loop through each sample in data buffer */   
   if ( Sta->iAlarmStatus >= 2 )
      for ( i=0; i<lNumSamp; i++ )
      {       	  
/* Reset Alarm status and variables if timeout has passed */
         if ( Sta->iAlarmStatus == 3 )
         {
            if ( (dStartTime + (double) i/Sta->dSampRate) >
                  Sta->dAlarmLastTriggerTime+dAlarmTimeout )
            {
               logit( "t", "Reset seis alarm from 3 to 2 in SA - %s\n",
                      Sta->szStation );
               Sta->iAlarmStatus = 2;
               goto ResetA;
            }
            goto EndOfLoop;
         }
  	   
/* Convert SP (filtered) data to approximate value in m/s */	
         Sta->dAlarmSamp = (double) WaveLong[i] / Sta->dSens;

/* Does this value exceed the alarm amplitude threshold? */
         if ( (fabs( Sta->dAlarmSamp ) > Sta->dAlarmAmp) && !Sta->lAlarmP1 )
         {
            Sta->lAlarmP1 = 1;
            Sta->dAlarmLastSamp = Sta->dAlarmSamp;
            Sta->lAlarmCycs = 0;
            Sta->lAlarmSamps = 0;
         }
		 
/* If Phase 1 is passed, look for strong, cyclical motion */		 
         if ( Sta->lAlarmP1 )
         {
            Sta->lAlarmSamps++;
            Sta->lAlarmCycs++;

/* Signal has lNumConsecA samples to reverse and exceed the threshold, or
   processing will start over */			
            if ( Sta->lAlarmCycs > lNumConsecA ) goto ResetA;
			
/* Has the sample's sign changed and is it over the threshold */
            if ( Sta->dAlarmSamp*Sta->dAlarmLastSamp < 0. &&
                 fabs (Sta->dAlarmSamp) > Sta->dAlarmAmp )
            {
               Sta->lAlarmCycs = 0;
               Sta->dAlarmLastSamp = Sta->dAlarmSamp;
            }

/* Has the signal stayed in the threshold range for long enough
   without having a timeout to declare an alarm? */
            if ( (double) Sta->lAlarmSamps > Sta->dSampRate*Sta->dAlarmDur )
            {     /* If yes, report the alarm to ring */
               Sta->iAlarmStatus = 3;
               iAlarm = 1;
               Sta->dAlarmLastTriggerTime = dStartTime +
                                            (double) i/Sta->dSampRate;
               if ( iSendReport == 1 ) 
               {
                  ReportAlarm( Sta, ucMyModID, siAlarmRegion,
                   ucEWHTypeAlarm, ucEWHMyInstID, iType, Sta->szStation, 0 );
               }
            }
         }
      goto EndOfLoop;
	  
/* Reset variables when alarm threshold can not be sustained for duration */	  
ResetA:		      
      Sta->lAlarmP1 = 0;
      Sta->lAlarmCycs = 0;
      Sta->lAlarmSamps = 0;		     
EndOfLoop:;
      }
   return iAlarm;
}  

     /**************************************************************
      *                       ShortCopyPBuf()                      *
      *                                                            *
      * Update magnitudes in main PBuffer.                         *
      *                                                            *
      * October, 2010: Now use STATION structure instead of PPICK  *
      *                                                            *
      * Arguments:                                                 *
      *  pStaIn      Input STATION structure                       *
      *  pStaOut     Output STATION structure                      *
      *                                                            *
      **************************************************************/

void ShortCopyPBuf( STATION *pStaIn, STATION *pStaOut )
{
   int i;

   pStaOut->iClipIt          = pStaIn->iClipIt;
   pStaOut->dMbAmpGM         = pStaIn->dMbAmpGM;
   pStaOut->dMbPer           = pStaIn->dMbPer;
   pStaOut->dMbTime          = pStaIn->dMbTime;
   pStaOut->dMbMag           = pStaIn->dMbMag;
   pStaOut->iMbClip          = pStaIn->iMbClip;
   pStaOut->dMlAmpGM         = pStaIn->dMlAmpGM;
   pStaOut->dMlPer           = pStaIn->dMlPer;
   pStaOut->dMlTime          = pStaIn->dMlTime;
   pStaOut->dMlMag           = pStaIn->dMlMag;
   pStaOut->iMlClip          = pStaIn->iMlClip;
   pStaOut->dMSAmpGM         = pStaIn->dMSAmpGM;
   pStaOut->dMSPer           = pStaIn->dMSPer;
   pStaOut->dMSTime          = pStaIn->dMSTime;
   pStaOut->dMSMag           = pStaIn->dMSMag;
   pStaOut->iMSClip          = pStaIn->iMSClip;
   pStaOut->dMwpIntDisp      = pStaIn->dMwpIntDisp;
   pStaOut->dMwpTime         = pStaIn->dMwpTime;
   pStaOut->dMwpMag          = pStaIn->dMwpMag;
   pStaOut->dMwAmpGM         = pStaIn->dMwAmpGM;
   pStaOut->dMwPer           = pStaIn->dMwPer;
   pStaOut->dMwTime          = pStaIn->dMwTime;
   pStaOut->dMwMag           = pStaIn->dMwMag;
   pStaOut->dMwMagBG         = pStaIn->dMwMagBG;
   pStaOut->iMwClip          = pStaIn->iMwClip;
   pStaOut->dPerMax          = pStaIn->dPerMax;
   pStaOut->dPTravTime       = pStaIn->dPTravTime;
   pStaOut->dPStartTime      = pStaIn->dPStartTime;
   pStaOut->dPEndTime        = pStaIn->dPEndTime;
   pStaOut->dRTravTime       = pStaIn->dRTravTime;
   pStaOut->dRStartTime      = pStaIn->dRStartTime;
   pStaOut->dREndTime        = pStaIn->dREndTime;
   for ( i=0; i<MAX_SPECTRA; i++ ) pStaOut->dMwAmpSp[i]   = pStaIn->dMwAmpSp[i];
   for ( i=0; i<MAX_SPECTRA; i++ ) pStaOut->dMwAmpSpBG[i] = pStaIn->dMwAmpSpBG[i];
   for ( i=0; i<MAX_SPECTRA; i++ ) pStaOut->dMwPerSp[i]   = pStaIn->dMwPerSp[i];
   for ( i=0; i<MAX_SPECTRA; i++ ) pStaOut->dMwMagSp[i]   = pStaIn->dMwMagSp[i];
   for ( i=0; i<MAX_SPECTRA; i++ ) pStaOut->dMwMagSpBG[i] = pStaIn->dMwMagSpBG[i];
   pStaOut->dRes             = pStaIn->dRes;
   pStaOut->dDelta           = pStaIn->dDelta;
   pStaOut->dAzimuth         = pStaIn->dAzimuth;
   pStaOut->dFracDelta       = pStaIn->dFracDelta;
   pStaOut->dSnooze          = pStaIn->dSnooze;
   pStaOut->dCooze           = pStaIn->dCooze;
}

     /**************************************************************
      *                     VeryShortCopyPBuf()                    *
      *                                                            *
      * Update Ms magnitudes in main structure.                    *
      *                                                            *
      * Arguments:                                                 *
      *  pStaIn      Input STATION structure                       *
      *  pStaOut     Output STATION structure                      *
      *                                                            *
      **************************************************************/

void VeryShortCopyPBuf( STATION *pStaIn, STATION *pStaOut )
{
   int    i;

   pStaOut->dMSAmpGM         = pStaIn->dMSAmpGM;
   pStaOut->dMSPer           = pStaIn->dMSPer;
   pStaOut->dMSTime          = pStaIn->dMSTime;
   pStaOut->dMSMag           = pStaIn->dMSMag;
   pStaOut->iMSClip          = pStaIn->iMSClip;
   pStaOut->dMwPer           = pStaIn->dMwPer;
   pStaOut->dMwTime          = pStaIn->dMwTime;
   pStaOut->dMwMag           = pStaIn->dMwMag;
   pStaOut->dMwMagBG         = pStaIn->dMwMagBG;
   pStaOut->iMwClip          = pStaIn->iMwClip;
   pStaOut->dPerMax          = pStaIn->dPerMax;
   pStaOut->dPTravTime       = pStaIn->dPTravTime;
   pStaOut->dPStartTime      = pStaIn->dPStartTime;
   pStaOut->dPEndTime        = pStaIn->dPEndTime;
   pStaOut->dRTravTime       = pStaIn->dRTravTime;
   pStaOut->dRStartTime      = pStaIn->dRStartTime;
   pStaOut->dREndTime        = pStaIn->dREndTime;
   for ( i=0; i<MAX_SPECTRA; i++ ) pStaOut->dMwAmpSp[i]   = pStaIn->dMwAmpSp[i];
   for ( i=0; i<MAX_SPECTRA; i++ ) pStaOut->dMwAmpSpBG[i] = pStaIn->dMwAmpSpBG[i];
   for ( i=0; i<MAX_SPECTRA; i++ ) pStaOut->dMwPerSp[i]   = pStaIn->dMwPerSp[i];
   for ( i=0; i<MAX_SPECTRA; i++ ) pStaOut->dMwMagSp[i]   = pStaIn->dMwMagSp[i];
   for ( i=0; i<MAX_SPECTRA; i++ ) pStaOut->dMwMagSpBG[i] = pStaIn->dMwMagSpBG[i];
}
