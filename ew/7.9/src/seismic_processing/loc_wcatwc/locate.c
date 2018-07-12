  /***********************************************************************
  *                                                                      *    
  * LOCATE.C                                                             *
  *                                                                      *
  * These functions call location functions in libsrc.  The first job of *
  * these functions is to sort the incoming Ps into buffers.  The        *
  * functions attempt to put the Ps into buffers which contain only Ps   *
  * from the same event.  This is not an easy task for some station      *
  * geometries.                                                          *
  *                                                                      *
  * Ps are sorted based on times.  Before any locations are made within  *
  * the buffers, Ps are sorted based on the possibility of the times     *
  * being from the same quake.  If the time difference between the new P *
  * and any other P within the buffer is less than the maximum which     *
  * could be expected for the distance between the two, the P is placed  *
  * in that buffer.  A second check is also made: if the P time is       *
  * greater than a specified number of mifnutes from the last station in *
  * the buffer, it will not be included in that buffer.                  *
  *                                                                      *
  * When a set number of P-times are loaded into the buffer, functions   *
  * will be called to locate a quake based on those P-times.  From that  *
  * point on (assuming there was a good solution), Ps will be loaded into*
  * the buffer based on if the P-time fits the location within some      *
  * tolerance.  If a good solution was not arrived at, Ps are continually*
  * added with new locations attempted with every other P.  When over a  *
  * certain number of Ps are in the buffer, the highest residuals are    *
  * thrown out (and set into a different buffer). Also, when a good loc  *
  * is attained in a buffer, the other buffers are checked for Ps which  *
  * may fit that location.  Any Ps which fit are moved in.               *
  *                                                                      *
  * A common problem is that the same quake may be located in two        *
  * different buffers.  This can happen due to phases other than the P   *
  * (such as the pP or sP) being picked up as the P by the P-picker      *
  * instead of the real P.  A routine is called to check for similar     *
  * quakes in different buffers.  Only the buffer with the most Ps is    *
  * sent to the Hypo_Ring.                                               *
  *                                                                      *
  * Made into earthworm module 2/2001.                                   *
  *                                                                      *
  * October, 2010: Combined PPICK and STATION arrays.                    *
  *                                                                      *
  ************************************************************************/
  
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <earthworm.h>
#include <transport.h>
#include "loc_wcatwc.h"
#ifdef _WINNT
#include <mmsystem.h>		/* PlaySound */
#endif

/* fPP defined in iaspei91.h.  Must be defined as extern for compilation */
extern float fPP[27436];  
extern char  szStnRem[MAX_PBUFFS][MAX_STN_REM][TRACE_STA_LEN];/* Removed stns */

     /**************************************************************
      *                        AddInMwL()                          *
      *                                                            *
      * This function reads the Mw data file, determines if the    *
      * Mws are from the same quake as being located, and adds Mw  *
      * information to arrays.                                     *
      *                                                            *
      * Arguments:                                                 *
      *  pSta        Array of STATION structures to accept Mw.     *
      *  iNum        Number of data entries in pSta                *
      *  pszFile     File with Mw results                          *
      *  pHypo       Computed hypocentral parameters               * 
      *                                                            *
      **************************************************************/
      
void AddInMwL( STATION *pSta, int iNum, char *pszFile, HYPO *pHypo )
{
   double  dMw;                         /* Mw from data file */
   double  dMwSumMod;                   /* Mw summation */
   double  dDum;                        /* Temp for spectral info */
   FILE    *hFile;                      /* Mw File handle */
   int     i, j, iNumPers;
   int     iFoundStation;               /* Station from file in array ? */
   int     iMwCountMod;                 /* # Mws from file(within .6 of avg.) */
   HYPO    HypoT;                       /* Quake params. in Mw file */
   char    szNet[6], szStn[6], szChn[6], szLoc[6];/* Stn, Chan from Mw file */

/* Initialize Mw variables */
   pHypo->dMwAvg = 0.;
   pHypo->iNumMw = 0;
   iMwCountMod = 0;
   dMwSumMod = 0.;

/* Try to open the Mw file */
   if ( (hFile = (FILE *) fopen( pszFile, "r" )) == NULL )
   {
      logit( "t", "Couldn't open %s in AddInMwL\n", pszFile );
      return;
   }
                                        
/* Read Mw quake location */
   fscanf( hFile, "%lf\n", &HypoT.dLat );
   fscanf( hFile, "%lf\n", &HypoT.dLon );
   fscanf( hFile, "%lf\n", &HypoT.dOriginTime );  /* O-time in 1/1/70 seconds */
   fscanf( hFile, "%lf\n", &HypoT.dPreferredMag );               /* Magnitude */

/* Is the quake that produced the Mws the same as that located here? */
   if ( IsItSameQuake( &HypoT, pHypo ) )
   {
      while ( !feof( hFile ) )
      {                                                  /* Yes, so add in Mw */
         fscanf( hFile, "%s %s %s %s %lf %d \n", szStn, szChn, szNet, szLoc,
                 &dMw, &iNumPers );
         iFoundStation = 0;
         for ( i=0; i<iNum; i++ ) 
         {
            if ( !strcmp( pSta[i].szStation, szStn ) &&
                 !strcmp( pSta[i].szNetID,   szNet ) &&
                 !strcmp( pSta[i].szChannel, szChn ) )
            {	                                 /* Add in new Mw information */
               pSta[i].dMwMag = dMw;
               iFoundStation  = 1;
               break;
            }
         }                                                /* end for i < iNum */
         if ( iFoundStation == 1 )
         {
            pHypo->dMwAvg += dMw;
            pHypo->iNumMw += 1;
            pSta[i].iMwNumPers = iNumPers;
            for ( j=0; j<iNumPers; j++ ) 
               fscanf( hFile, "%lf %lf %lf %lf %lf\n", &pSta[i].dMwPerSp[j], 
                      &pSta[i].dMwMagSp[j],   &pSta[i].dMwAmpSp[j],
                      &pSta[i].dMwMagSpBG[j], &pSta[i].dMwAmpSpBG[j] );
         }
         else
            for ( j=0; j<iNumPers; j++ ) 
               fscanf( hFile, "%lf %lf %lf %lf %lf\n", 
                      &dDum, &dDum, &dDum, &dDum, &dDum );
      }

      if ( pHypo->iNumMw > 0 ) 
      {
         pHypo->dMwAvg /= (double) pHypo->iNumMw;
         for ( i=0; i<iNum; i++ )
            if ( pSta[i].iUseMe && pSta[i].dMwMag > 0.01 )
               if ( fabs( pHypo->dMwAvg-pSta[i].dMwMag ) < 0.6 )
               {
                  iMwCountMod++;
                  dMwSumMod += pSta[i].dMwMag;
               }
         if ( iMwCountMod ) pHypo->dMwAvg = dMwSumMod / (double) iMwCountMod;
         if ( iMwCountMod ) pHypo->iNumMw = iMwCountMod;
      }
   }
   fclose( hFile );                                  /* Close file and return */
   return;
}

     /**************************************************************
      *                     AddInThetaL()                          *
      *                                                            *
      * This function reads the theta data file, determines if the *
      * theta is from the same quake as being located, and adds    *
      * theta information to arrays.                               *
      *                                                            *
      * Arguments:                                                 *
      *  pSta        Array of STATION structures to accept theta.  *
      *  iNum        Number of data entries in pSta                *
      *  pszFile     File with theta results                       *
      *  pHypo       Computed hypocentral parameters               * 
      *                                                            *
      **************************************************************/
      
void AddInThetaL( STATION *pSta, int iNum, char *pszFile, HYPO *pHypo )
{
   double  dTheta;                      /* Theta from data file */
   double  dThetaAvg;                   /* Theta summation and average */
   double  dThetaStd;                   /* Theta Standard deviation */
   FILE    *hFile;                      /* Theta File handle */
   int     i;
   int     iFoundStation;               /* Station from file in array ? */
   int     iThetaCntSD;                 /* # theta used in averaging */
   HYPO    HypoT;                       /* Quake params. in theta file */
   char    szNet[8], szStn[8], szChn[8], szLoc[8];/* Stn, Chan from theta file*/

/* Initialize Theta variables */
   pHypo->dTheta    = 0.;
   pHypo->dThetaSD  = 0.;
   pHypo->iNumTheta = 0;
   iThetaCntSD      = 0;
   dThetaAvg        = 0.;
   dThetaStd        = 0.;
   for ( i=0; i<iNum; i++ ) 
   {
      pSta[i].dThetaEnergy = 0.;
      pSta[i].dThetaMoment = 0.;
      pSta[i].dTheta       = 0.;
   }

/* Try to open the Theta file */
   if ( (hFile = (FILE *) fopen( pszFile, "r" )) == NULL )
   {
      logit( "t", "Couldn't open %s in AddInThetaL\n", pszFile );
      return;
   }
                                        
/* Read Theta quake location */
   fscanf( hFile, "%lf\n", &HypoT.dLat );
   fscanf( hFile, "%lf\n", &HypoT.dLon );
   fscanf( hFile, "%lf\n", &HypoT.dOriginTime );  /* O-time in 1/1/70 seconds */
   fscanf( hFile, "%lf\n", &HypoT.dPreferredMag );               /* Magnitude */

/* Is the quake that produced the Mws the same as that located here? */
   if ( IsItSameQuake( &HypoT, pHypo ) )
   {
      while ( !feof( hFile ) )
      {                                               /* Yes, so add in theta */
         fscanf( hFile, "%s %s %s %s %lf\n", szStn, szChn, szNet, szLoc, 
                 &dTheta );
         iFoundStation = 0;
         for ( i=0; i<iNum; i++ ) 
         {
            if ( !strcmp( pSta[i].szStation, szStn ) &&
                 !strcmp( pSta[i].szNetID,   szNet ) &&
                 !strcmp( pSta[i].szChannel, szChn ) )
            {                                 /* Add in new theta information */
               pSta[i].dTheta = dTheta;
               iFoundStation = 1;
               break;
            }
         }                                                /* end for i < iNum */
         if ( iFoundStation == 1 )
         {
            dThetaAvg   += pSta[i].dTheta;
            iThetaCntSD += 1;
         }
      }
      if ( iThetaCntSD > 0 ) 
      {
         dThetaAvg /= (double) iThetaCntSD;
         for ( i=0; i<iNum; i++ )
            if ( pSta[i].iUseMe && fabs( pSta[i].dTheta ) > 0.01 )
               dThetaStd += ((pSta[i].dTheta-dThetaAvg) * 
                             (pSta[i].dTheta-dThetaAvg));
         dThetaStd   = sqrt( dThetaStd/(double) iThetaCntSD );
         iThetaCntSD = 0;
         dThetaAvg   = 0.;
         for ( i=0; i<iNum; i++ )
            if ( pSta[i].iUseMe && fabs( pSta[i].dTheta ) > 0.01 )
               if ( fabs( pSta[i].dTheta-dThetaAvg ) <= dThetaStd )
               {
                  dThetaAvg += pSta[i].dTheta;
                  iThetaCntSD++;
               }
         if ( iThetaCntSD > 0 )             /* Then fill Hypo array with info */
         {
            dThetaAvg /= (double) iThetaCntSD;
            pHypo->dTheta    = dThetaAvg;
            pHypo->iNumTheta = iThetaCntSD;
            pHypo->dThetaSD  = dThetaStd;
         }
      }
   }
   else
      logit( "", "Theta file does not match for this event\n" );
   fclose( hFile );                                  /* Close file and return */
   return;
}

     /**************************************************************
      *                     CheckPBuffTimes()                      *
      *                                                            *
      * After a solution has been made, see if any other P's in    *
      * other buffers go with this location.  If so, copy them into*
      * the buffer with the latest solution.                       *
      *                                                            *
      * Also, remove Ps that had high residuals and find a new     *
      * buffer for them.                                           *
      *                                                            *
      * June, 2007: Added code to better associate P-picks using   *
      *             nearest station approach.                      *
      * Sept., 2004: When picks are dropped due to bad residual    *
      *              (but not too bad - i.e. bad P-pick) don't move*
      *              to another buffer; just eliminate it.         *
      *                                                            *
      * Arguments:                                                 *
      *  pSta        STATION buffers to copy P info into           *
      *  iBufCnt     Number of P-picks in each buffer so far       *
      *  Hypo        Computed hypocenters for each buffer          *
      *  iActive     Last buffer to get a hypocenter solution      *
      *  Gparm       Configuration parameter structure             *
      *  iLastCnt    P buffer counter                              *
      *  iNumRem     Number of stations permanently removed        *
      *  pszPStnArray Array with nearest station lookup table      *
      *  iNumPStn    Number of stations in pick file               *
      *  iNumNearStn Number of near stations to compare            *
      *  iNumSta     Number of PPicks in buffer                    *
      *                                                            *
      **************************************************************/
	  
void CheckPBuffTimes( STATION **pSta, int iPBufCnt[], HYPO Hypo[], int iActive,
                      GPARM *Gparm, int iLastCnt[], int iNumRem[],
                      char pszPStnArray[][MAX_NUM_NEAR_STN][TRACE_STA_LEN],
                      int iNumPStn, int iNumNearStn, int iNumSta )
{
   AZIDELT azidelt;                /* Distance/azimuth between 2 points */
   double  dCorr;                  /* Distance correction for P tables */
   double  dMin;                   /* Minimum P-time in all buffers */
   double  dTimeExpected;          /* P-time for this station */
   double  dTTDif;                 /* P-time difference between 2 points */
   double  dTTDifMax;              /* Max P-time diff. given delta for 2 pts. */
   double  dTTDifQ;                /* Expected P time for this point */
   int     i, j, k, kk, iNum, iTemp;/* Index counters */
   int     iAllBad = 1;            /* All stations have high resid. - 1 */
   int     iAlreadyUsed;           /* 1->This buffer already has been ordered */
   int     iBuff[MAX_PBUFFS];      /* Order of buffers (Starting with buffer
                                      with the most picks, buffers with a good
                                      location are designated -2) */
   static  int  iDLev;             /* Index of 0 deg. in desired depth level */
   int     iMatch, iPCnt, iNearbyPMatch;/* Flags to indicate correct buffer */
   int     iMin;                   /* Index of buffer with minimum P-time */
   int     iStnIndex;              /* Index of this P in nearby station table */
   LATLON  ll, ll2;
   time_t  lTime;                  /* Preset 1/1/70 time in seconds */   
	    
/* First check to see if all stations have high resid., if so return */
   for ( k=0; k<iNumPStn; k++ )
      if ( pSta[iActive][k].dPTime       > 0. &&
           fabs( pSta[iActive][k].dRes ) < 180. ) iAllBad = 0;
   if ( iAllBad == 1 ) return;         

/* Remove picks with high residual. */	    
   if ( iPBufCnt[iActive] > Gparm->MinPs+1 )
      for ( k=0; k<iNumPStn; k++ )
      {
/* First, check to see if this looks like a bad P-pick for this event.
   That is, the pick should be associated with this event, but the P-time
   is off by 10-20 seconds (but the pick was not forced in by manual pick
   in hypo_display).  If it does seem like a bad P, remove it and send it
   nowhere so that it does not join up with other bad Ps to make
   a false location in another buffer. */   	  
         if ( atoi( pSta[iActive][k].szHypoID ) < 0 &&
              pSta[iActive][k].dPTime > 0. &&
              Hypo[iActive].iGoodSoln >= 2  &&
              Hypo[iActive].iNumPs    >= 10 &&
              fabs( pSta[iActive][k].dRes ) > 10. &&
              fabs( pSta[iActive][k].dRes ) < 20. )
         {
            logit( "", "Pick %s removed completely\n",
             pSta[iActive][k].szStation );
            strcpy( szStnRem[iActive][iNumRem[iActive]],
                    pSta[iActive][k].szStation );
            iNumRem[iActive]++;
            logit( "", "%s added to Buffer %d - %d removed stations\n",
             szStnRem[iActive][iNumRem[iActive]-1], iActive, iNumRem[iActive] );
            goto Remove;
         }
			  
/* Now check to see if this is way out and not likely just a bad P, but
   probably associated with another quake.  Then, try other buffers. */
         if (((((fabs( pSta[iActive][k].dRes ) > 10. &&
                   Hypo[iActive].iGoodSoln >= 1) ||
                 (Hypo[iActive].iNumPs >= 10 &&
                   Hypo[iActive].iGoodSoln >= 2 &&
                   fabs( pSta[iActive][k].dRes ) > 7.5))  &&
                   atoi( pSta[iActive][k].szHypoID ) < 0) ||
                 (fabs( pSta[iActive][k].dRes ) > 180.))  &&
                   pSta[iActive][k].dPTime > 0. )
         {
/* Find where to put pick */	 
/* Check all buffers to see if this fits in an existing solution */		 		 
            for ( i=0; i<Gparm->NumPBuffs; i++ ) iBuff[i] = -1;   
            for ( i=0; i<Gparm->NumPBuffs; i++ )
            {
               iTemp = iActive+i;
               if ( iTemp >= Gparm->NumPBuffs ) iTemp -= Gparm->NumPBuffs;
	  
/* Do we have a good sol'n for this buffer */
               if ( Hypo[iTemp].iNumPs >= Gparm->MinPs &&
                    Hypo[iTemp].iGoodSoln >= 2 && iTemp != iActive )
               {
                  iBuff[iTemp] = -2;

/* Get depth level to use in travel time table */
                  iDLev = (int) ( (Hypo[iTemp].dDepth+0.01) /
                          (double) IASP_DEPTH_INC) * IASP_NUM_PER_DEP;
	  
/* Check the P-time with what would be expected for this quake; if it
   fits keep it.  Otherwise, go to next buffer */	  
                  ll.dLat = pSta[iActive][k].dLat; 
                  ll.dLon = pSta[iActive][k].dLon; 
                  GeoCent( &ll );
                  GetLatLonTrig( &ll );
                  GetDistanceAz( (LATLON *) &Hypo[iTemp], &ll, &azidelt );
                  dTTDif = pSta[iActive][k].dPTime -
                           Hypo[iTemp].dOriginTime;
                  dCorr = azidelt.dDelta*(1./IASP_DIST_INC) - 
                   floor( azidelt.dDelta*(1./IASP_DIST_INC) );
                  dTTDifQ = fPP[(int) (azidelt.dDelta*(1./IASP_DIST_INC))+
                   iDLev] + dCorr*(fPP[(int) (azidelt.dDelta*
                   (1./IASP_DIST_INC))+1+iDLev] - fPP[(int)
                   (azidelt.dDelta*(1./IASP_DIST_INC))+iDLev]);				
				   
/* Is this station already in buffer? If it is, go to next buffer*/		 
                  for ( j=0; j<iNumPStn; j++ )
                     if ( !strcmp( pSta[iTemp][j].szStation,  
                           pSta[iActive][k].szStation )  &&
                          !strcmp( pSta[iTemp][j].szNetID,    
                           pSta[iActive][k].szNetID )    &&
                          !strcmp( pSta[iTemp][j].szLocation, 
                           pSta[iActive][k].szLocation ) &&
                          !strcmp( pSta[iTemp][j].szChannel,  
                           pSta[iActive][k].szChannel )  &&
                          pSta[iTemp][j].dPTime > 0. )
                        goto NextBuffer;
					
                  if ( fabs( dTTDifQ - dTTDif ) <= 5. )
                  {
                     for ( j=0; j<iNumPStn; j++ )
                        if ( !strcmp( pSta[iTemp][j].szStation,  
                              pSta[iActive][k].szStation )  &&
                             !strcmp( pSta[iTemp][j].szNetID,    
                              pSta[iActive][k].szNetID )    &&
                             !strcmp( pSta[iTemp][j].szLocation, 
                              pSta[iActive][k].szLocation ) &&
                             !strcmp( pSta[iTemp][j].szChannel,  
                              pSta[iActive][k].szChannel ) )
                        {
                           CopyPBuf( &pSta[iActive][k], &pSta[iTemp][j] );
                           iPBufCnt[iTemp]++;
                           pSta[iTemp][j].iPickCnt = iPBufCnt[iTemp];
                           if ( Gparm->Debug )
                              logit( "t", "Pick Added in Check (post7) %s %lf - %d\n",
                               pSta[iTemp][j].szStation,
                               pSta[iTemp][j].dPTime, iTemp );
                           goto Remove;
                        }
                     logit( "", "Couldn't find %s in buf %d\n",
                      pSta[iActive][k].szStation, iTemp );
                     goto Remove;
                  }
               }
NextBuffer:;}
	  
/* If we don't have a sol'n which matches this pick, try buffers to see which
   it could fit in. But first, order the buffers by Number of Ps. */
            for ( i=0; i<Gparm->NumPBuffs; i++ )
               if ( iBuff[i] != -2 )           /* Skip those that have a soln */
               {
                  iNum = 0;
                  for ( j=0; j<Gparm->NumPBuffs; j++ )
                     if ( iBuff[j] != -2 )
                     {
                        iAlreadyUsed = 0;
                        for ( kk=0; kk<i; kk++ )
                           if ( iBuff[kk] == j ) iAlreadyUsed = 1;
                        if ( iAlreadyUsed == 0 )
                           if ( iPBufCnt[j] > iNum )
                           {
                              iBuff[i] = j;
                              iNum = iPBufCnt[j];
                           }
                     }
               }
/* It didn't fit an existing solution, so see which buffer it can fit in */	  
            for ( i=0; i<Gparm->NumPBuffs; i++ )
            {
               iTemp = iBuff[i];
/* If we don't have a sol'n yet in buffer or there are not many picks */	  
               if ( iTemp >= 0 && iTemp != iActive )
               {              /* -2->good soln for buffer, -1->Buffer empty */
/* Here, check buffers by for a nearby station and P-time differences */	  
                  iMatch = 0;
                  iNearbyPMatch = 0;
                  iPCnt = 0;
/* Find the index of this new Ppick in nearby station lookup table */
                  for ( iStnIndex=0; iStnIndex<iNumPStn; iStnIndex++ )
                     if ( !strcmp( pSta[iActive][k].szStation, 
                                   pszPStnArray[iStnIndex][0] ) )
                        break;
                  if ( iStnIndex == iNumPStn ) iNearbyPMatch = 1;

/* See if any of the nearby stations are already in this buffer */	 
                  for ( j=0; j<iNumPStn; j++ )
                     if ( pSta[iTemp][j].dPTime > 10. )
                        for ( kk=1; kk<iNumNearStn; kk++ )
                           if ( !strcmp( pSta[iTemp][j].szStation,
                                         pszPStnArray[iStnIndex][kk] ) )
                              iNearbyPMatch = 1;   
/* Get depth level to use in travel time table */
                  iDLev = (int) ( ((double) DEPTHKM+0.01) /
                          (double) IASP_DEPTH_INC) * IASP_NUM_PER_DEP;
/* Check the P-times in the buffer to see if they are within theoretical
   limits.  But only check if there is a nearby station in this buffer.  Also
   check to see if this station is already in the buffer. */		  
                  if ( iNearbyPMatch == 1 )
                     for ( j=0; j<iNumPStn; j++ )
                        if ( pSta[iTemp][j].dPTime > 10. )
                        {
                           ll.dLat = pSta[iActive][k].dLat; 
                           ll.dLon = pSta[iActive][k].dLon; 
                           GeoCent( &ll );
                           GetLatLonTrig( &ll );
                           ll2.dLat = pSta[iTemp][j].dLat; 
                           ll2.dLon = pSta[iTemp][j].dLon; 
                           GeoCent( &ll2 );
                           GetLatLonTrig( &ll2 );
                           GetDistanceAz( &ll, &ll2, &azidelt );
                           dTTDif = fabs( pSta[iActive][k].dPTime -
                                          pSta[iTemp][j].dPTime );
                           dTTDifMax = fPP[(int) (azidelt.dDelta*
                                       (1./IASP_DIST_INC))+1+iDLev];
                           if ( dTTDif > dTTDifMax ) iPCnt++;
                           if ( !strcmp( pSta[iActive][k].szStation,  
                                 pSta[iTemp][j].szStation ) &&
                                !strcmp( pSta[iActive][k].szChannel,  
                                 pSta[iTemp][j].szChannel ) &&
                                !strcmp( pSta[iActive][k].szNetID,    
                                 pSta[iTemp][j].szNetID )   &&
                                !strcmp( pSta[iActive][k].szLocation, 
                                 pSta[iTemp][j].szLocation ) )
                              iMatch = 1;
                        }
/* Are we within the constraints to add the pick to this buffer */
                  if ( iMatch == 0 && iPCnt == 0 && iNearbyPMatch == 1 )	
/* We are within the constraints to add the pick to this buffer */
                  {
                     for ( j=0; j<iNumPStn; j++ )
                        if ( !strcmp( pSta[iTemp][j].szStation,  
                              pSta[iActive][k].szStation )  &&
                             !strcmp( pSta[iTemp][j].szNetID,    
                              pSta[iActive][k].szNetID )    &&
                             !strcmp( pSta[iTemp][j].szLocation, 
                              pSta[iActive][k].szLocation ) &&
                             !strcmp( pSta[iTemp][j].szChannel,  
                              pSta[iActive][k].szChannel ) )
                        {
                           CopyPBuf( &pSta[iActive][k], &pSta[iTemp][j] );
                           iPBufCnt[iTemp]++;
                           if ( Gparm->Debug )
                              logit( "t", "Pick Added in Check %s %lf - %d\n",
                               pSta[iTemp][j].szStation,
                               pSta[iTemp][j].dPTime, iTemp );
                           goto Remove;
                        }
                     logit( "", "Couldn't find %s in buf 2 %d\n",
                      pSta[iActive][k].szStation, iTemp );
                     goto Remove;
                  }                   		                               
               }
            }

/* If we get here, the pick doesn't fit in any existing buffers.  
   Try to find the first buffer available with no picks and put it there. */
            for ( i=0; i<Gparm->NumPBuffs; i++ )
            {
               iTemp = iActive+i;
               if ( iTemp >= Gparm->NumPBuffs ) iTemp -= Gparm->NumPBuffs;
               if ( iTemp != iActive )
                  if ( iPBufCnt[iTemp] == 0 )
                     for ( j=0; j<iNumPStn; j++ )
                        if ( !strcmp( pSta[iTemp][j].szStation,  pSta[iActive][k].szStation ) &&
                             !strcmp( pSta[iTemp][j].szNetID,    pSta[iActive][k].szNetID ) &&
                             !strcmp( pSta[iTemp][j].szLocation, pSta[iActive][k].szLocation ) &&
                             !strcmp( pSta[iTemp][j].szChannel,  pSta[iActive][k].szChannel ) )
                        {
                           CopyPBuf( &pSta[iActive][k], &pSta[iTemp][j] );
                           iPBufCnt[iTemp]++;
                           pSta[iTemp][j].iPickCnt = iPBufCnt[iTemp];
                           if ( Gparm->Debug )
                              logit( "t", "Pick Added in Check (new buf) %s %lf - %d\n",
                               pSta[iTemp][j].szStation,
                               pSta[iTemp][j].dPTime, iTemp );
                           goto Remove;
                        }
            }

/* If we get here, all P buffers must have some Ps in them, so one buffer must
   be cleared out. Find the one with the oldest P's and re-init that one. */
            iMin = 0;
            dMin = 1.E20;
            time( &lTime );
            for ( i=0; i<Gparm->NumPBuffs; i++ )
               if ( Hypo[i].dMSAvg == 0. ||
                  ((double) lTime-Hypo[i].dOriginTime) > 7200.)
                  if ( i != iActive )
                     for ( j=0; j<iNumPStn; j++ )
                        if ( pSta[i][j].dPTime < dMin &&
                             pSta[i][j].dPTime > 0.0 )
                        {
                           dMin = pSta[i][j].dPTime;
                           iMin = i;
                        }
            iPBufCnt[iMin] = 0;
            iNumRem[iMin]  = 0;
            iLastCnt[iMin] = 0;
            Hypo[iMin].iVersion     = 1;
            Hypo[iMin].iAlarmIssued = 0;
            iTemp = atoi( Hypo[iMin].szQuakeID );
            iTemp += Gparm->NumPBuffs;
            if ( iTemp >= 900000 ) iTemp -= 900000;
            itoaX( iTemp, Hypo[iMin].szQuakeID );
            PadZeroes( 6, Hypo[iMin].szQuakeID );
            for ( i=0; i<iNumSta; i++ ) InitP( &pSta[iMin][i] );
            InitHypo( &Hypo[iMin] );
            for ( j=0; j<iNumPStn; j++ )
               if ( !strcmp( pSta[iMin][j].szStation,  pSta[iActive][k].szStation ) &&
                    !strcmp( pSta[iMin][j].szNetID,    pSta[iActive][k].szNetID ) &&
                    !strcmp( pSta[iMin][j].szLocation, pSta[iActive][k].szLocation ) &&
                    !strcmp( pSta[iMin][j].szChannel,  pSta[iActive][k].szChannel ) )
               {
                  CopyPBuf( &pSta[iActive][k], &pSta[iMin][j] );
                  iPBufCnt[iMin]++;
                  pSta[iMin][j].iPickCnt = iPBufCnt[iMin];
                  if ( Gparm->Debug )
                     logit( "t", "Pick Added in Check (zeroed buf) %s %lf - %d\n",
                      pSta[iMin][j].szStation, pSta[iMin][j].dPTime, iMin );
               }
	    
/* We've found a home for the bad P, so remove it from the active buffer */
Remove:     RemoveP( &pSta[iActive][k] );
            iPBufCnt[iActive] = iPBufCnt[iActive]-1;
         }
      }

/* Next, bring in other picks if we're pretty sure this is a good location */
   if ( Hypo[iActive].iNumPs >= Gparm->MinPs && Hypo[iActive].iGoodSoln >= 2 )
   {
/* Get depth level to use in travel time table */
      iDLev = (int) ( (Hypo[iActive].dDepth+0.01) /
              (double) IASP_DEPTH_INC) * IASP_NUM_PER_DEP;
   
/* Loop through all other buffers */
      for ( i=0; i<Gparm->NumPBuffs; i++ )
         if ( i != iActive )
            for ( j=0; j<iNumPStn; j++ )
/* If pick already included in a good sol'n, don't move it around */			
               if ( ((Hypo[i].iGoodSoln >= 2 && pSta[i][j].iUseMe < 1) ||
                      Hypo[i].iGoodSoln < 2) && pSta[i][j].dPTime > 0. &&
                      Hypo[i].iNumPs < MAX_SCAVENGE )
               {				   
/* Is this station already in buffer? If it is, go to next buffer*/		 
                  for ( k=0; k<iNumPStn; k++ )
                     if ( !strcmp( pSta[i][j].szStation,  pSta[iActive][k].szStation ) &&
                          !strcmp( pSta[i][j].szLocation, pSta[iActive][k].szLocation ) &&
                          !strcmp( pSta[i][j].szNetID,    pSta[iActive][k].szNetID ) &&
                          !strcmp( pSta[i][j].szChannel,  pSta[iActive][k].szChannel ) )
                        goto NextP;
						
/* Compute what the P-time should be for this site from the last computed
   quake */			
                  ll.dLat = pSta[i][j].dLat; 
                  ll.dLon = pSta[i][j].dLon; 
                  GeoCent( &ll );
                  GetLatLonTrig( &ll );
                  GetDistanceAz( (LATLON *) &Hypo[iActive], &ll, &azidelt );
                  dCorr = azidelt.dDelta*(1./IASP_DIST_INC) - 
                   floor( azidelt.dDelta*(1./IASP_DIST_INC) );
                  dTimeExpected = Hypo[iActive].dOriginTime +
                   fPP[(int) (azidelt.dDelta*(1./IASP_DIST_INC))+iDLev] +
                   dCorr*(fPP[(int) (azidelt.dDelta*(1./IASP_DIST_INC))+1+iDLev]
                   - fPP[(int) (azidelt.dDelta*(1./IASP_DIST_INC))+iDLev]);				
/* If the P-time at this station fits what is expected, it maybe associated
   with this quake.  Add it to the Active buffer and remove from old. */				
                  if ( fabs( dTimeExpected-pSta[i][j].dPTime ) <= 5. )
                  {
                     for ( k=0; k<iNumPStn; k++ )
                        if ( !strcmp( pSta[i][j].szStation,  pSta[iActive][k].szStation ) &&
                             !strcmp( pSta[i][j].szLocation, pSta[iActive][k].szLocation ) &&
                             !strcmp( pSta[i][j].szNetID,    pSta[iActive][k].szNetID ) &&
                             !strcmp( pSta[i][j].szChannel,  pSta[iActive][k].szChannel ) )
                        {
                           CopyPBuf( &pSta[i][j], &pSta[iActive][k] );
                           iPBufCnt[iActive]++;
                           pSta[iActive][k].iPickCnt = iPBufCnt[iActive];
                           if ( Gparm->Debug )
                              logit( "o", "Moved %s from %d to %d; timedif=%lf\n",
                               pSta[i][j].szStation, i, iActive,
                               dTimeExpected-pSta[i][j].dPTime );
                           RemoveP( &pSta[i][j] );
                           iPBufCnt[i] = iPBufCnt[i]-1;
                        }
                     logit( "", "Time fit for move, but couldn't; %s from %d to %d \n",
                      pSta[i][j].szStation, i, iActive );
                  }
NextP:;        }
   }
}			

   /*******************************************************************
    *                     GetCityFile( )                              *
    *                                                                 *
    * This function picks which city to use in the voice output.      *
    *                                                                 *
    *  Arguments:                                                     *
    *    pCityDis      CITYDIS structure with location info           *
    *    pCity         Array of reference city locations              *
    *    iIndex        0 or 1 for pCity array (closer city)           *
    *    pszPath       Folder path for file of interest               *
    *                                                                 *
    *  Returns:        character string with city file to read        *
    *                                                                 *
    *******************************************************************/
char *GetCityFile( CITYDIS *pCityDis, CITY *pCity, int iIndex, char *pszPath )
{
   int    i;
   static char szFile[64];
   char   szTemp[35];

   for ( i=0; i<NUM_CITIES; i++ )
      if ( !strcmp( pCityDis->pszLoc[iIndex], (pCity+i)->szLoc ) ) 
      {
         strcpy( szFile, pszPath );
         strcat( szFile, "CITY" );
         _itoa( i, szTemp, 10 );
         PadZeroes( 2, szTemp );
         strcat( szFile, szTemp );
         strcat( szFile, ".WAV" );
         break;
      }
   return( szFile );
}

   /*******************************************************************
    *                     GetDirectionFile( )                         *
    *                                                                 *
    * This function picks which direction wave file to use with a     *
    * local location output.                                          *
    *                                                                 *
    *  Arguments:                                                     *
    *    pCityDis      CITYDIS structure with location info           *
    *    iIndex        0 or 1 for pCity array (closer city)           *
    *    pszPath       Folder path for file of interest               *
    *                                                                 *
    *  Returns:        character string with direction file to read   *
    *                                                                 *
    *******************************************************************/
char *GetDirectionFile( CITYDIS *pCityDis, int iIndex, char *pszPath )
{
   static char szFile[64];

   strcpy( szFile, pszPath );
   if ( !strcmp( pCityDis->pszDir[iIndex], "N" ) )
         strcat( szFile, "DIRN.WAV" );
   else if ( !strcmp( pCityDis->pszDir[iIndex], "NE" ) )
              strcat( szFile, "DIRNE.WAV" );
   else if ( !strcmp( pCityDis->pszDir[iIndex], "E" ) )
              strcat( szFile, "DIRE.WAV" );
   else if ( !strcmp( pCityDis->pszDir[iIndex], "SE" ) )
              strcat( szFile, "DIRSE.WAV" );
   else if ( !strcmp( pCityDis->pszDir[iIndex], "S" ) )
              strcat( szFile, "DIRS.WAV" );
   else if ( !strcmp( pCityDis->pszDir[iIndex], "SW" ) )
              strcat( szFile, "DIRSW.WAV" );
   else if ( !strcmp( pCityDis->pszDir[iIndex], "W" ) )
              strcat( szFile, "DIRW.WAV" );
   else if ( !strcmp( pCityDis->pszDir[iIndex], "NW" ) )
              strcat( szFile, "DIRNW.WAV" );
   return ( szFile );
}

   /*******************************************************************
    *                     GetDistanceFile( )                          *
    *                                                                 *
    * This function picks which numerical wave file (distance) to use *
    * with a local location output.                                   *
    *                                                                 *
    *  Arguments:                                                     *
    *    pCityDis      CITYDIS structure with location info           *
    *    iIndex        0 or 1 for pCity array (closer city)           *
    *    pszPath       Folder path for file of interest               *
    *                                                                 *
    *  Returns:        character string with distance file to read    *
    *                                                                 *
    *******************************************************************/
char *GetDistanceFile( CITYDIS *pCityDis, int iIndex, char *pszPath )
{
   static char szFile[64];

   strcpy( szFile, pszPath );
   if ( pCityDis->iDis[iIndex] == 0 ) strcat( szFile, "DIS000.WAV" );
   else if ( pCityDis->iDis[iIndex] == 5 ) strcat( szFile, "DIS005.WAV" );
   else if ( pCityDis->iDis[iIndex] == 10 ) strcat( szFile, "DIS010.WAV" );
   else if ( pCityDis->iDis[iIndex] == 15 ) strcat( szFile, "DIS015.WAV" );
   else if ( pCityDis->iDis[iIndex] == 20 ) strcat( szFile, "DIS020.WAV" );
   else if ( pCityDis->iDis[iIndex] == 25 ) strcat( szFile, "DIS025.WAV" );
   else if ( pCityDis->iDis[iIndex] == 30 ) strcat( szFile, "DIS030.WAV" );
   else if ( pCityDis->iDis[iIndex] == 35 ) strcat( szFile, "DIS035.WAV" );
   else if ( pCityDis->iDis[iIndex] == 40 ) strcat( szFile, "DIS040.WAV" );
   else if ( pCityDis->iDis[iIndex] == 45 ) strcat( szFile, "DIS045.WAV" );
   else if ( pCityDis->iDis[iIndex] == 50 ) strcat( szFile, "DIS050.WAV" );
   else if ( pCityDis->iDis[iIndex] == 55 ) strcat( szFile, "DIS055.WAV" );
   else if ( pCityDis->iDis[iIndex] == 60 ) strcat( szFile, "DIS060.WAV" );
   else if ( pCityDis->iDis[iIndex] == 65 ) strcat( szFile, "DIS065.WAV" );
   else if ( pCityDis->iDis[iIndex] == 70 ) strcat( szFile, "DIS070.WAV" );
   else if ( pCityDis->iDis[iIndex] == 75 ) strcat( szFile, "DIS075.WAV" );
   else if ( pCityDis->iDis[iIndex] == 80 ) strcat( szFile, "DIS080.WAV" );
   else if ( pCityDis->iDis[iIndex] == 85 ) strcat( szFile, "DIS085.WAV" );
   else if ( pCityDis->iDis[iIndex] == 90 ) strcat( szFile, "DIS090.WAV" );
   else if ( pCityDis->iDis[iIndex] == 95 ) strcat( szFile, "DIS095.WAV" );
   else if ( pCityDis->iDis[iIndex] == 100 ) strcat( szFile, "DIS100.WAV" );
   else if ( pCityDis->iDis[iIndex] == 105 ) strcat( szFile, "DIS105.WAV" );
   else if ( pCityDis->iDis[iIndex] == 110 ) strcat( szFile, "DIS110.WAV" );
   else if ( pCityDis->iDis[iIndex] == 115 ) strcat( szFile, "DIS115.WAV" );
   else if ( pCityDis->iDis[iIndex] == 120 ) strcat( szFile, "DIS120.WAV" );
   else if ( pCityDis->iDis[iIndex] == 125 ) strcat( szFile, "DIS125.WAV" );
   else if ( pCityDis->iDis[iIndex] == 130 ) strcat( szFile, "DIS130.WAV" );
   else if ( pCityDis->iDis[iIndex] == 135 ) strcat( szFile, "DIS135.WAV" );
   else if ( pCityDis->iDis[iIndex] == 140 ) strcat( szFile, "DIS140.WAV" );
   else if ( pCityDis->iDis[iIndex] == 145 ) strcat( szFile, "DIS145.WAV" );
   else if ( pCityDis->iDis[iIndex] == 150 ) strcat( szFile, "DIS150.WAV" );
   else if ( pCityDis->iDis[iIndex] == 155 ) strcat( szFile, "DIS155.WAV" );
   else if ( pCityDis->iDis[iIndex] == 160 ) strcat( szFile, "DIS160.WAV" );
   else if ( pCityDis->iDis[iIndex] == 165 ) strcat( szFile, "DIS165.WAV" );
   else if ( pCityDis->iDis[iIndex] == 170 ) strcat( szFile, "DIS170.WAV" );
   else if ( pCityDis->iDis[iIndex] == 175 ) strcat( szFile, "DIS175.WAV" );
   else if ( pCityDis->iDis[iIndex] == 180 ) strcat( szFile, "DIS180.WAV" );
   else if ( pCityDis->iDis[iIndex] == 185 ) strcat( szFile, "DIS185.WAV" );
   else if ( pCityDis->iDis[iIndex] == 190 ) strcat( szFile, "DIS190.WAV" );
   else if ( pCityDis->iDis[iIndex] == 195 ) strcat( szFile, "DIS195.WAV" );
   else if ( pCityDis->iDis[iIndex] == 200 ) strcat( szFile, "DIS200.WAV" );
   else if ( pCityDis->iDis[iIndex] == 205 ) strcat( szFile, "DIS205.WAV" );
   else if ( pCityDis->iDis[iIndex] == 210 ) strcat( szFile, "DIS210.WAV" );
   else if ( pCityDis->iDis[iIndex] == 215 ) strcat( szFile, "DIS215.WAV" );
   else if ( pCityDis->iDis[iIndex] == 220 ) strcat( szFile, "DIS220.WAV" );
   else if ( pCityDis->iDis[iIndex] == 225 ) strcat( szFile, "DIS225.WAV" );
   else if ( pCityDis->iDis[iIndex] == 230 ) strcat( szFile, "DIS230.WAV" );
   else if ( pCityDis->iDis[iIndex] == 235 ) strcat( szFile, "DIS235.WAV" );
   else if ( pCityDis->iDis[iIndex] == 240 ) strcat( szFile, "DIS240.WAV" );
   else if ( pCityDis->iDis[iIndex] == 245 ) strcat( szFile, "DIS245.WAV" );
   else if ( pCityDis->iDis[iIndex] == 250 ) strcat( szFile, "DIS250.WAV" );
   else if ( pCityDis->iDis[iIndex] == 255 ) strcat( szFile, "DIS255.WAV" );
   else if ( pCityDis->iDis[iIndex] == 260 ) strcat( szFile, "DIS260.WAV" );
   else if ( pCityDis->iDis[iIndex] == 265 ) strcat( szFile, "DIS265.WAV" );
   else if ( pCityDis->iDis[iIndex] == 270 ) strcat( szFile, "DIS270.WAV" );
   else if ( pCityDis->iDis[iIndex] == 275 ) strcat( szFile, "DIS275.WAV" );
   else if ( pCityDis->iDis[iIndex] == 280 ) strcat( szFile, "DIS280.WAV" );
   else if ( pCityDis->iDis[iIndex] == 285 ) strcat( szFile, "DIS285.WAV" );
   else if ( pCityDis->iDis[iIndex] == 290 ) strcat( szFile, "DIS290.WAV" );
   else if ( pCityDis->iDis[iIndex] == 295 ) strcat( szFile, "DIS295.WAV" );
   else if ( pCityDis->iDis[iIndex] == 300 ) strcat( szFile, "DIS300.WAV" );
   else if ( pCityDis->iDis[iIndex] == 305 ) strcat( szFile, "DIS305.WAV" );
   else if ( pCityDis->iDis[iIndex] == 310 ) strcat( szFile, "DIS310.WAV" );
   else if ( pCityDis->iDis[iIndex] == 315 ) strcat( szFile, "DIS315.WAV" );
   else if ( pCityDis->iDis[iIndex] == 320 ) strcat( szFile, "DIS320.WAV" );
   return ( szFile );                                      
}

   /*******************************************************************
    *                     GetFERegionFile( )                          *
    *                                                                 *
    * This function picks out the proper wave file to use for telling *
    * which region the earthquake was in.  Regions used here are the  *
    * Flinn-Engdahl geographic regions.                               *
    *                                                                 *
    *  Arguments:                                                     *
    *    iRegNumber    Flinn-Engdahl Region number                    *
    *    pszPath       Folder path for file of interest               *
    *                                                                 *
    *  Returns:        character string with FE file to read          *
    *                                                                 *
    *******************************************************************/
char *GetFERegionFile( int iRegNumber, char *pszPath )
{
   static char szFile[64];            /* FE Region WAV file name */
   char        szTemp[12];

   strcpy( szFile, pszPath );
   _itoa( iRegNumber, szTemp, 10 );
   PadZeroes( 3, szTemp );
   strcat( szFile, "REG" );
   strcat( szFile, szTemp );
   strcat( szFile, ".WAV" );
   return( szFile );                              
}

      /******************************************************************
       *                     LoadPagerString()                          *
       *                                                                *
       * This function builds a quake message to send to the alarm ring.*
       *                                                                *
       *  Arguments:                                                    *
       *   pHypo            Computed hypocentral parameters             *
       *   pszMsg           Pointer to string to hold message           *
       *   pcity            Array of reference city information         *
       *   pcityEC          Array of eastern reference city information *
       *   Gparm            Configuration parameter structure           *
       *                                                                *
       ******************************************************************/
       
void LoadPagerString( HYPO *pHypo, char *pszMsg, CITY *pcity, CITY *pcityEC,
                      GPARM *Gparm )
{
   CITYDIS CityDis;           /* Dist/dir from nearest cities (epicenter) */
   char    cNS, cEW;          /* 'N', 'S', etc. indicators. for lat/lon */
   int     iFERegion;         /* Flinn-Engdahl Region number */
   int     iRegion;           /* Procedural Region number */
   int     iTemp;
   LATLON  ll;                /* Geographic epicenter */
   long    lTime;             /* 1/1/70 time */
   char    *psz;              /* General region of epicenter */
   char    szTemp[64];        /* Littoral epicentral location */
   static  struct  tm    *tm; /* Origin time in tm structure */
   
/* Convert o-time from 1/1/70 time to tm structure */
   lTime = (long) (pHypo->dOriginTime+0.5);
   tm = TWCgmtime( lTime );
   
/* Get nearest cities to epicenter */
   GeoGraphic( &ll, (LATLON *) pHypo );
   if ( ll.dLon < 0 ) ll.dLon += 360.;
   iRegion = GetRegion( ll.dLat, ll.dLon );
   if ( ll.dLon > 180. ) ll.dLon -= 360.;
   cNS = 'N';
   if ( ll.dLat < 0. ) cNS = 'S';
   cEW = 'E';
   if ( ll.dLon < 0. ) cEW = 'W';
   
/* Compute distance to nearest cities */   
   if ( iRegion >= 10 )                                   /* NTWC Eastern AOR */
      NearestCitiesEC( (LATLON *) pHypo, pcityEC, &CityDis ); 
   else
      NearestCities( (LATLON *) pHypo, pcity, &CityDis );
  
/* Is there a city within 320 miles of epicenter? */
   if ( CityDis.iDis[1] < 320 || CityDis.iDis[0] < 320 )
   {
/* Yes, there is */
      if ( CityDis.iDis[1] < CityDis.iDis[0] ) iTemp = 1;
      else                                     iTemp = 0;
      sprintf( szTemp, "%d %s %s", CityDis.iDis[iTemp],
               CityDis.pszDir[iTemp], CityDis.pszLoc[iTemp] );
   }
   else                     /* There was no near-by city, so use general area */
   {
      psz = namnum( ll.dLat, ll.dLon, &iFERegion, Gparm->szIndexFile,
                    Gparm->szLatFile, Gparm->szNameFile );
      psz[strlen (psz)-1] = '\0';
      strcpy( szTemp, psz );
   }
   
/* Write the entire alarm message */
   sprintf( pszMsg, "%s  M%s %3.1lf  %d STN  %.1lf%c %.1lf%c  "
    "%d:%d:%dZ %d/%d  RES %.1lf  AZM %d ",
    szTemp, pHypo->szPMagType,
    pHypo->dPreferredMag, pHypo->iNumPs, fabs( ll.dLat ), cNS, fabs( ll.dLon ),
    cEW, tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_mon+1, tm->tm_mday,
    pHypo->dAvgRes, pHypo->iAzm );
}

     /**************************************************************
      *                         LoadUpPBuff()                      *
      *                                                            *
      * Load latest P-pick in the proper buffer.  If the pick is a *
      * repeat of a previous pick, replace the previous one (this  *
      * should have new magnitude information). Start with         *
      * iActiveBuffer and determine if the P-time fits in with the *
      * existing picks in that buffer.  If not, move to next       *
      * buffer. If it doesn't fit with any, move to the first      *
      * with no P-picks and start there.                           *
      *                                                            *
      * Two checks are made on the times before deciding which     *
      * buffer to use.  If the P-time is more than                 *
      * MaxTimeBetweenPicks from the last pick in the buffer, it   *
      * must go to the next.  Also, if it's time difference is     *
      * greater than possible (from the IASPEI91 P-time tables) to *
      * any pick in the buffer, it is moved to the next buffer.    *
      *                                                            *
      * June, 2014: Modifed phase check magnitude from 5.75 to 5   *
      * Apr., 2014: Added check for MaxDist as well as nearest     *
      *             station in P-pick association                  *
      * Dec., 2012: Instead of checking buffer locations for       *
      *             phases, it now uses the OldQuake file so that  *
      *             locs. made in other locators are included.     *
      * July, 2007: Added code to discriminate phases from other   *
      *             quakes which have already been located.        *
      * June, 2007: Added code to better associate P-picks using   *
      *             nearest station approach.                      *
      * Sept., 2004: Check to see that the station was not         *
      *              permanently removed from this buffer          *
      *                                                            *
      * Arguments:                                                 *
      *  pSta        Structure containing input P-pick information *
      *  pStaFull    STATION buffers to copy P into                *
      *  iBufCnt     Number of P-picks in each buffer so far       *
      *  Hypo        Computed hypocenters for each buffer          *
      *  piActive    Last buffer to put a P-pick into              *
      *  Gparm       Configuration parameter structure             *
      *  Ewh         Earthworm.d defines                           *
      *  city        reference city locations                      *
      *  iLastCnt    P buffer counter                              *
      *  iNumRem     Number of stations permanently removed        *
      *  cityEC      Eastern reference city locations              *
      *  iDepthAvg   Array of average depths for all lat/lon       *
      *  iDepthMax   Array of maximum depths for all lat/lon       *
      *  pszPStnArray Array with nearest station lookup table      *
      *  iNumPStn    Number of stations in structure               *
      *  iNumNearStn Number of near stations to compare            *
      *  dMaxDist    Max Distance for nearest station check        *
      *  iForceLoc   Flag to force re-location in LocateThread     *
      *  pszIndexFile FE Regions index file                        *
      *  pszLatFile  FE Regions Lat file                           *
      *  pszNameFile FE Regions Name file                          *
      *  pszPathDistances Path to voice distance files             *
      *  pszPathDirections Path to voice direction files           *
      *  pszPathCities    Path to voice cities files               *
      *  pszPathRegions   Path to voice region files               *
      *                                                            *
      **************************************************************/	  
void LoadUpPBuff( STATION *pSta, STATION **pStaFull, int iPBufCnt[],
                  HYPO Hypo[], int *piActive, GPARM *Gparm, EWH *Ewh,
                  CITY city[], int iLastCnt[], int iNumRem[], CITY cityEC[],
                  int iDepthAvg[][360], int iDepthMax[][360], 
                  char pszPStnArray[][MAX_NUM_NEAR_STN][TRACE_STA_LEN],
                  int iNumPStn, int iNumNearStn, double dMaxDist,
                  int iForceLoc[], char *pszIndexFile, char *pszLatFile, 
                  char *pszNameFile, char *pszPathDistances, 
                  char *pszPathDirections, char *pszPathCities, 
                  char *pszPathRegions )
{
   AZIDELT azidelt;                /* Distance/azimuth between 2 points */
   double  dCorr;                  /* Distance correction for P tables */
   double  dMin;                   /* Oldest P-time in any buffer */
   double  dTTDif;                 /* P-time difference between 2 points */
   double  dTTDifMax;              /* Max P-time diff. given delta for 2 pts. */
   double  dTTDifQ;                /* Expected P time for this point */
   FILE    *hFile;  
   HYPO    HypoL;                  /* Local Hypocenter structure */
   int     i, ii, j, k, iNum, iTemp;   /* Index counters */
   int     iAlreadyUsed;           /* 1->This buffer already has been ordered */
   int     iBuff[MAX_PBUFFS];      /* Order of buffers (Starting with buffer
                                      with the most picks, buffers with a good
                                      location are designated -2) */
   int     iIndex;                 /* Index of buffer with latest P-pick */
   static  int iMatch, iPCnt, iNearbyPMatch;/* Flags to indicate correct buffer */
   static  int iDLev;              /* Index of 0 deg. in desired depth level */
   int     iStnIndex;              /* Index of this P in nearby station table */
   LATLON  ll, ll2;
   time_t  lTime;                  /* Preset 1/1/70 time in seconds */  
   static  time_t lLastTime;       /* Last time Locate quake called */  
   static  OLDQUAKE OldQuake[MAX_QUAKES]; /* Previous auto-located quakes */
   static  PHASE Phase;            /* Phase data */

/* Save index of this station and ensure it is in lineup */
   for ( ii=0; ii<iNumPStn; ii++ )
      if ( !strcmp( pStaFull[0][ii].szStation,  pSta->szStation ) &&
           !strcmp( pStaFull[0][ii].szLocation, pSta->szLocation ) &&
           !strcmp( pStaFull[0][ii].szNetID,    pSta->szNetID ) &&
           !strcmp( pStaFull[0][ii].szChannel,  pSta->szChannel ) ) break;
   if ( ii == iNumPStn )
   {
      logit( "", "%s-%s-%s-%s is not in STATION array\n", pSta->szStation,
       pSta->szChannel, pSta->szNetID, pSta->szLocation );
      return;
   }
   else  /* Fill lat/lon for Get Phase */
   {
      pSta->dLat = pStaFull[0][ii].dLat; 
      pSta->dLon = pStaFull[0][ii].dLon; 
      pSta->dElevation = pStaFull[0][ii].dElevation; 
   } 

/* Is it an Ms? */
   if ( pSta->dPTime < 0.1 )   
      for ( i=0; i<Gparm->NumPBuffs; i++ )
         if ( atoi( Hypo[i].szQuakeID ) == pSta->lPickIndex )  /* Yes */
         {		 
/* Add MS information to structure */		 
            for ( j=0; j<iNumPStn; j++ )
               if ( !strcmp( pStaFull[i][j].szStation,  pSta->szStation ) &&
                    !strcmp( pStaFull[i][j].szLocation, pSta->szLocation ) &&
                    !strcmp( pStaFull[i][j].szNetID,    pSta->szNetID ) &&
                    !strcmp( pStaFull[i][j].szChannel,  pSta->szChannel ) )
               {
/* If there is a match, copy the new Ms info into the old pick structure */		 
                  pStaFull[i][j].dMSAmpGM = pSta->dMSAmpGM;
                  pStaFull[i][j].dMSPer   = pSta->dMSPer;
                  pStaFull[i][j].dMSTime  = pSta->dMSTime;
				  
/* Just re-compute the magnitudes and dump to TWC hypo ring every 10s */			
                  time( &lTime );
                  if ( lTime-lLastTime > 10 )
                  {
                     lLastTime = lTime;
                     if ( LocateQuake( iNumPStn, pStaFull[i], &iPBufCnt[i], 
                           Gparm, &Hypo[i], i, Ewh, city, 0, Hypo, iPBufCnt, 
                           cityEC, iDepthAvg, iDepthMax, pszIndexFile, 
                           pszLatFile, pszNameFile, pszPathDistances, 
                           pszPathDirections, pszPathCities, pszPathRegions ) < 0 )
                        logit( "et", "Problem in LocateQuake\n" );
                     if ( Gparm->Debug )
                        logit( "", "Ms Updated %s - %d\n",
                         pStaFull[i][j].szStation, i );
                  }
                  return;
               }
            logit( "", "No station match found for MS update %s - %d\n",
                   pSta->szStation, i );
            return;
         }

#ifndef _ALLOW_TANKPLAYBACK               /* For use with standard earthwuorm */
/* Ignore this pick if it is older than PPICK_TIMEOUT seconds */   
   time( &lTime );
   if ( lTime-(long) pSta->dPTime > PPICK_TIMEOUT )
   {
      logit( "t", "Old P-time - %s\n", pSta->szStation );
      return;
   }
#endif
   
/* If this pick came from hypo_display, it should have a QuakeID */
   for ( i=0; i<Gparm->NumPBuffs; i++ )
      if ( !strcmp( Hypo[i].szQuakeID, pSta->szHypoID ) ) 
      {                                    /* Then, force it into this buffer */
         for ( j=0; j<iNumPStn; j++ )
            if ( !strcmp( pStaFull[i][j].szStation,  pSta->szStation  ) &&
                 !strcmp( pStaFull[i][j].szLocation, pSta->szLocation ) &&
                 !strcmp( pStaFull[i][j].szNetID,    pSta->szNetID    ) &&
                 !strcmp( pStaFull[i][j].szChannel,  pSta->szChannel  ) )
            {
/* Update the Pick information in array */
               if ( pStaFull[i][j].dPTime < 10. ) 
               {
                  iPBufCnt[i] += 1;			
                  pStaFull[i][j].iPickCnt = iPBufCnt[i];
               }
               iForceLoc[i] = 1;
               CopyPBuf( pSta, &pStaFull[i][j] );
               if ( Gparm->Debug )
                  logit( "", "Pick Updated (in hypo_display) %s %lf - %d, iUseMe=%d\n",
                   pStaFull[i][j].szStation, pStaFull[i][j].dPTime, i, 
                   pStaFull[i][j].iUseMe );
               return;
            }
         logit( "", "No station match found for Hypo update %s - %d\n",
                pSta->szStation, i );
         return;
      }
   
/* Determine if this pick is a repeat of a previous pick */
   for ( i=0; i<Gparm->NumPBuffs; i++ )
      for ( j=0; j<iNumPStn; j++ )
         if ( (pStaFull[i][j].lPickIndex == pSta->lPickIndex &&
               fabs( pSta->dPTime-pStaFull[i][j].dPTime ) <
               Gparm->MaxTimeBetweenPicks*60.) ||
              (!strcmp( pStaFull[i][j].szStation, pSta->szStation ) &&
               !strcmp( pStaFull[i][j].szLocation, pSta->szLocation ) &&
               !strcmp( pStaFull[i][j].szNetID, pSta->szNetID ) &&
               !strcmp( pStaFull[i][j].szChannel, pSta->szChannel ) &&
               fabs( pSta->dPTime-pStaFull[i][j].dPTime ) <
              (double) BUFFER_TIMEOUT) )
         {
/* If it is, copy the new information into the old pick structure; UNLESS
   the Pick was manually adjusted by hypo_display (i.e., iUseMe=2)
   and this adjustment is from pick_wcatwc. */		 
            if ( pSta->iUseMe == 1 && (pStaFull[i][j].iUseMe == 2 || 
                 atoi( pStaFull[i][j].szHypoID ) >= 0) )
            {
               if ( Gparm->Debug )
                  logit( "", "Pick Update on %s rejected.\n",
                         pStaFull[i][j].szStation);
               return;
            }
// This should be done in some instances but not all !!!
//            if ( iPBufCnt[i] >= Gparm->MinPs ) iForceLoc[i] = 1;
            CopyPBuf( pSta, &pStaFull[i][j] );
            if ( Gparm->Debug )
               logit( "", "Pick Updated %s %lf - %d\n",
                pStaFull[i][j].szStation, pStaFull[i][j].dPTime, i );
            return;
         }

/* If we get here, no match was found for pick so find a buffer for it */
/* First, check all buffers to see if this fits in an existing solution */		 		 
   for ( i=0; i<Gparm->NumPBuffs; i++ ) iBuff[i] = -1;   
   for ( i=0; i<Gparm->NumPBuffs; i++ )
   {
      iTemp = *piActive+i;
      if ( iTemp >= Gparm->NumPBuffs ) iTemp -= Gparm->NumPBuffs;	  
/* Verify that this was not in the permanently removed list */	  
      for ( j=0; j<iNumRem[iTemp]; j++ )
         if ( !strcmp( szStnRem[iTemp][j], pSta->szStation ) ) 
         {
            logit( "", "%s was perm. removed from %d\n", pSta->szStation,
                   iTemp );
            return;
         }	                              
/* Do we have a good sol'n for this buffer */
/* Maybe iGoodSoln should be a 1 in the next line ??? - PW */
//      if ( Hypo[iTemp].iNumPs >= Gparm->MinPs && Hypo[iTemp].iGoodSoln >= 2 )
      if ( Hypo[iTemp].iNumPs >= Gparm->MinPs && Hypo[iTemp].iGoodSoln >= 1 )
      {	  
/* Get depth level to use in travel time table */
         iDLev = (int) ( (Hypo[iTemp].dDepth+0.01) /
                 (double) IASP_DEPTH_INC) * IASP_NUM_PER_DEP;
         iBuff[iTemp] = -2;
/* Check the P-time with what would be expected for this quake; if it
   fits keep it.  Otherwise, go to next buffer */
         ll.dLat = pStaFull[0][ii].dLat; 
         ll.dLon = pStaFull[0][ii].dLon; 
         GeoCent( &ll );
         GetLatLonTrig( &ll );
         GetDistanceAz( (LATLON *) &Hypo[iTemp], &ll, &azidelt );
         dTTDif = pSta->dPTime - Hypo[iTemp].dOriginTime;
         dCorr = azidelt.dDelta*(1./IASP_DIST_INC) - 
          floor( azidelt.dDelta*(1./IASP_DIST_INC) );
         dTTDifQ = fPP[(int) (azidelt.dDelta*(1./IASP_DIST_INC))+iDLev] +
          dCorr*(fPP[(int) (azidelt.dDelta*(1./IASP_DIST_INC))+1+iDLev] -
          fPP[(int) (azidelt.dDelta*(1./IASP_DIST_INC))+iDLev]);
         if ( pSta->dFreq < FREQ_MIN2 || azidelt.dDelta < DELTA_TELE ||
              Hypo[iTemp].dDepth > 100. )       
            if ( fabs( dTTDifQ - dTTDif ) <= 10. )
            {
               for ( j=0; j<iNumPStn; j++ )
                  if ( !strcmp( pStaFull[iTemp][j].szStation,  pSta->szStation ) &&
                       !strcmp( pStaFull[iTemp][j].szLocation, pSta->szLocation ) &&
                       !strcmp( pStaFull[iTemp][j].szNetID,    pSta->szNetID ) &&
                       !strcmp( pStaFull[iTemp][j].szChannel,  pSta->szChannel ) )
                  {
                     CopyPBuf( pSta, &pStaFull[iTemp][j] );
                     iPBufCnt[iTemp] += 1;
                     pStaFull[iTemp][j].iPickCnt = iPBufCnt[iTemp];
                     iForceLoc[iTemp] = 1;
                     if ( Gparm->Debug )
                        logit( "t", "Pick Added (existing) %s %lf - %d\n",
                         pStaFull[iTemp][j].szStation,
                         pStaFull[iTemp][j].dPTime, iTemp );
                     return;
                  }
            logit( "", "No station match found for hypo match %s - %d\n",
                   pSta->szStation, i );
            return;
            }                         
      }
   }
   
/* Check to see if this could be a phase from a strong quake in another
   buffer.  Phases are normally low frequency arrivals */   
   if ( pSta->dFreq < FREQ_MIN )
   {
/* First read in old quake data */
      j = 0;
      if ( (hFile = fopen( Gparm->szOldQuakes, "r" )) != NULL )
      {
         for ( j=0; j<MAX_QUAKES; j++ )
            if ( fscanf( hFile, "%lf %lf %lf %lf %d %s %d %s %d %d %lf "
                                "%lf %lf %d %lf %d %lf %d %lf %d %lf %d "
                                "%lf %d %lf %lf %d\n",
                &OldQuake[j].dOTime,       &OldQuake[j].dLat, &OldQuake[j].dLon,
                &OldQuake[j].dPreferredMag,&OldQuake[j].iNumPMags,
                (char *)(&OldQuake[j].szPMagType), &OldQuake[j].iDepth,
                (char *)(&OldQuake[j].szQuakeID),  &OldQuake[j].iVersion,
                &OldQuake[j].iNumPs,       &OldQuake[j].dAvgRes,
                &OldQuake[j].dAzm,      &OldQuake[j].dMbAvg, &OldQuake[j].iNumMb,
                &OldQuake[j].dMlAvg,    &OldQuake[j].iNumMl,
                &OldQuake[j].dMSAvg,    &OldQuake[j].iNumMS,
                &OldQuake[j].dMwpAvg,   &OldQuake[j].iNumMwp,
                &OldQuake[j].dMwAvg,    &OldQuake[j].iNumMw,
                &OldQuake[j].d1stPTime, &OldQuake[j].iGoodSoln,
                &OldQuake[j].dTheta,    &OldQuake[j].dThetaSD, 
                &OldQuake[j].iNumTheta ) != 27 ) break;
         fclose( hFile );     
      }

      for ( i=0; i<j; i++ )
      {  
/* Do we have a good sol'n for this event and is it a big quake which might
   produce phases */                   
         if ( OldQuake[i].iNumPs                >= Gparm->MinPs+10 &&
              OldQuake[i].dPreferredMag         >= 5.0  &&
              lTime - (long) OldQuake[i].dOTime <  5400 &&/* Within 1.5 hours */
              (long) OldQuake[i].dOTime         <  lTime )      /* For Player */
         {
/* Set up Hypo structure to get phase times */
            CopyOQToHypo( &OldQuake[i], &HypoL );
            GeoCent( (LATLON *) &HypoL );
            GetLatLonTrig( (LATLON *) &HypoL );
            GetPhaseTime( pSta, &HypoL, &Phase, 3, Gparm->szIasp91File );      
/* Check the P-time with phases that would be expected for this quake */
            dTTDif = pSta->dPTime;
            for ( j=0; j<Phase.iNumPhases; j++ )
            {                                      
               dTTDifQ = Phase.dPhaseTime[j];
               if ( fabs( dTTDifQ-dTTDif ) <= 20. )   
               {
                  if ( Gparm->Debug )
                     logit( "t", "Probable %s Phase for earlier quake "
                                 "(OldQuake=%d) - %s %lf - removed from locator\n",
                      Phase.szPhase[j], i, pSta->szStation,
                      pSta->dPTime );
                  return;
               }
            }                      
         }
      }             
   }
   
/* If we don't have a sol'n which matches this pick and it's not a phase, 
   try buffers to see which it could fit in. But first, order the buffers 
   by Number of Ps. */
   for ( i=0; i<Gparm->NumPBuffs; i++ )
      if ( iBuff[i] != -2 )       /* Skip those where there is already a soln */
      {
         iNum = 0;
         for ( j=0; j<Gparm->NumPBuffs; j++ )
            if ( iBuff[j] != -2 )
            {
               iAlreadyUsed = 0;
               for ( k=0; k<i; k++ )
                  if ( iBuff[k] == j ) iAlreadyUsed = 1;
               if ( iAlreadyUsed == 0 )
                  if ( iPBufCnt[j] > iNum )
                  {
                     iBuff[i] = j;
                     iNum = iPBufCnt[j];
                  }
            }
      }
   
/* Find the index of this new Ppick in nearby station lookup table */
   for ( iStnIndex=0; iStnIndex<iNumPStn; iStnIndex++ )
      if ( !strcmp( pSta->szStation, pszPStnArray[iStnIndex][0] ) )
         break;
   if ( iStnIndex == iNumPStn ) 
      logit( "", "%s is not in nearby station table\n", pSta->szStation );    
/* Now loop through buffers to see if this could be same quake */
   for ( i=0; i<Gparm->NumPBuffs; i++ )
   {
      iTemp = iBuff[i];
      if ( iTemp >= 0 )         /* -2->good soln for buffer, -1->Buffer empty */
      {	  
/* Here, check buffers for a nearby station and P-time differences */	  
         iMatch = 0;
         iNearbyPMatch = 0;
         iPCnt = 0;         
/* Go through checks to see if this station can fit in this buffer */	 
         for ( j=0; j<iNumPStn; j++ )
            if ( pStaFull[iTemp][j].dPTime > 10. )
            {                              /* Then this stn is in this buffer */
/* First check to see if station is in nearby station list to new P */
               if ( iNearbyPMatch == 0 && iStnIndex < iNumPStn )
               {
                  for ( k=1; k<iNumNearStn; k++ )
                     if ( !strcmp( pStaFull[iTemp][j].szStation,
                           pszPStnArray[iStnIndex][k] ) ) 
                     {
                        iNearbyPMatch = 1; 
                        if ( Gparm->Debug )
                           logit( "", "%s is nearby %s\n", 
                            pSta->szStation, pStaFull[iTemp][j].szStation );
                     }
               }
/* Next, check to see if station is within Max distance */
               ll.dLat = pStaFull[0][ii].dLat; 
               ll.dLon = pStaFull[0][ii].dLon; 
               GeoCent( &ll );
               GetLatLonTrig( &ll );
               ll2.dLat = pStaFull[iTemp][j].dLat; 
               ll2.dLon = pStaFull[iTemp][j].dLon; 
               GeoCent( &ll2 );
               GetLatLonTrig( &ll2 );
               GetDistanceAz( &ll, &ll2, &azidelt );
               if ( iNearbyPMatch == 0 )
               {
                  if ( azidelt.dDelta < dMaxDist ) 
                  {
                     iNearbyPMatch = 1;
                     if ( Gparm->Debug )
						 logit("", "%s is %lf from %s\n", pSta->szStation,
                         azidelt.dDelta, pStaFull[iTemp][j].szStation );
                  }
               }
/* Next, check to see if P-times are within theorteical limits */
               dTTDif = fabs( pSta->dPTime - pStaFull[iTemp][j].dPTime );
/* Assume surface source */
               dTTDifMax = fPP[(int) (azidelt.dDelta*(1./IASP_DIST_INC))+1+0];
               if ( dTTDif > (dTTDifMax+10.) && pStaFull[iTemp][j].iUseMe > 0 ) 
               { 
                  iPCnt++;
                  if ( Gparm->Debug )
                     logit( "", "%s Ptime is too far from %s\n", 
                      pSta->szStation, pStaFull[iTemp][j].szStation );
               }
/* Last, check to see if the station is already in this buffer */
               if ( !strcmp( pStaFull[iTemp][j].szStation,  pSta->szStation  ) &&
                    !strcmp( pStaFull[iTemp][j].szLocation, pSta->szLocation ) &&
                    !strcmp( pStaFull[iTemp][j].szNetID,    pSta->szNetID    ) &&
                    !strcmp( pStaFull[iTemp][j].szChannel,  pSta->szChannel  ) )
               {
                  iMatch = 1;
                  if ( Gparm->Debug )
                     logit( "", "%s already in buffer\n", pSta->szStation );
               }
            }
 
/* Are we within the constraints to add the pick to this buffer */
         if ( iMatch == 0 && iPCnt == 0 && iNearbyPMatch == 1 )
         {
            for ( j=0; j<iNumPStn; j++ )
               if ( !strcmp( pStaFull[iTemp][j].szStation,  pSta->szStation  ) &&
                    !strcmp( pStaFull[iTemp][j].szLocation, pSta->szLocation ) &&
                    !strcmp( pStaFull[iTemp][j].szNetID,    pSta->szNetID    ) &&
                    !strcmp( pStaFull[iTemp][j].szChannel,  pSta->szChannel  ) )
               {
                  CopyPBuf( pSta, &pStaFull[iTemp][j] );
                  iPBufCnt[iTemp]++;
                  pStaFull[iTemp][j].iPickCnt = iPBufCnt[iTemp];
                  if ( Gparm->Debug )
                     logit( "t", "Pick Added: %s:%s:%s:%s: %lf - %d\n",
                      pStaFull[iTemp][j].szStation,pStaFull[iTemp][j].szChannel,
                      pStaFull[iTemp][j].szNetID, pStaFull[iTemp][j].szLocation,
                      pStaFull[iTemp][j].dPTime, iTemp );
                  return;
               }
            logit( "", "No station match found for buffer update %s - %d\n",
                   pSta->szStation, i );
            return;
         }
      }
   }

/* If we get here, the pick doesn't fit in any existing buffers.  First, 
   try to find the first buffer available with no picks and put it there. */
   for ( i=0; i<Gparm->NumPBuffs; i++ )
   {
      iTemp = *piActive+i;
      if ( iTemp >= Gparm->NumPBuffs ) iTemp -= Gparm->NumPBuffs;
         if ( iPBufCnt[iTemp] == 0 )
            for ( j=0; j<iNumPStn; j++ )
               if ( !strcmp( pStaFull[iTemp][j].szStation,  pSta->szStation ) &&
                    !strcmp( pStaFull[iTemp][j].szLocation, pSta->szLocation ) &&
                    !strcmp( pStaFull[iTemp][j].szNetID,    pSta->szNetID ) &&
                    !strcmp( pStaFull[iTemp][j].szChannel,  pSta->szChannel ) )
               {
                  CopyPBuf( pSta, &pStaFull[iTemp][j] );
                  iPBufCnt[iTemp]++;
                  pStaFull[iTemp][j].iPickCnt = iPBufCnt[iTemp];
                  if ( Gparm->Debug )
                     logit( "t", "Pick Added (new buf) %s %lf - %d\n",
                      pStaFull[iTemp][j].szStation,
                      pStaFull[iTemp][j].dPTime, iTemp );
                  return;
               }
   }

/* If we get here, all buffers have some data, but this pick doesn't
   fit in any.  Find the buffer with the oldest data, zero it out,
   and put P there. (We really shouldn't get here, but sometimes we do.) */
   iIndex = 0;
   dMin = 1.E20;
   for ( i=0; i<Gparm->NumPBuffs; i++ )
/* Save this if MS could still be updating, or P's are still being added */
      if ( Hypo[i].dMSAvg == 0. ||
         ((double) lTime-Hypo[i].dOriginTime) > 5400. )
         if ( ((double) lTime-Hypo[i].dOriginTime) > 1260. || 
            Hypo[i].iNumPs < Gparm->MinPs || Hypo[i].iGoodSoln < 2 )
            for ( j=0; j<iNumPStn; j++ )
               if ( pStaFull[i][j].dPTime < dMin && 
                    pStaFull[i][j].dPTime > 0.0 )
               {
                  dMin = pStaFull[i][j].dPTime;
                  iIndex = i;
               }
   logit( "", "Re-init buffer in LoadUp %d\n", iIndex );
   iPBufCnt[iIndex] = 0;
   iNumRem[iIndex]  = 0;
   iLastCnt[iIndex] = 0;
   for ( i=0; i<iNumPStn; i++ ) InitP( &pStaFull[iIndex][i] );
   InitHypo( &Hypo[iIndex] );
   iTemp = atoi( Hypo[iIndex].szQuakeID );
   iTemp += Gparm->NumPBuffs;
   if ( iTemp >= 900000 ) iTemp -= 900000;
   itoaX( iTemp, Hypo[iIndex].szQuakeID );
   PadZeroes( 6, Hypo[iIndex].szQuakeID );
   Hypo[iIndex].iVersion = 1;
   Hypo[iIndex].iAlarmIssued = 0;
   for ( j=0; j<iNumPStn; j++ )
      if ( !strcmp( pStaFull[iIndex][j].szStation,  pSta->szStation ) &&
           !strcmp( pStaFull[iIndex][j].szLocation, pSta->szLocation ) &&
           !strcmp( pStaFull[iIndex][j].szNetID,    pSta->szNetID ) &&
           !strcmp( pStaFull[iIndex][j].szChannel,  pSta->szChannel ) )
      {
         CopyPBuf( pSta, &pStaFull[iIndex][j] );
         iPBufCnt[iIndex]++;
         pStaFull[iIndex][j].iPickCnt = iPBufCnt[iIndex];
         if ( Gparm->Debug )
            logit( "t", "Pick Added (latest) %s %lf - %d\n",
             pStaFull[iIndex][j].szStation, pStaFull[iIndex][j].dPTime, iIndex );
         return;
      }
   logit( "t", "No place to put %s-%s-%s-%s\n", pSta->szStation,
          pSta->szChannel, pSta->szNetID, pSta->szLocation );
   return;
}				  

      /******************************************************************
       *                          LocateQuake()                         *
       *                                                                *
       *  This function call the functions necessary to locate an       *
       *  earthquake given a set of P arrival times.  The hypocenter    *
       *  parameters are returned in a structure argument.              *
       *                                                                *
       *  November, 2015: Bug fix; Stations were being forced back in   *
       *                  after being manually eliminated in LatestHypo *
       *  November, 2015: Report alarm twice when alarm is announced    *
       *  March, 2015: Modified when locations are announced to reduce  *
       *               repetition                                       *
       *  August, 2014: Added direct call to SayLocation                *
       *  December, 2012: Re-ordered some of the processes, and re-wrote*
       *                  some of the logic.  Set up tests to compare   *
       *                  future changes with - logs in EB3\TestLogs.   *
       *  December, 2007: Add check to see if there was a recent large  *
       *                  quake and whether this has nearby stations.   *
       *  April, 2006: Added region of reliability to locator for local *
       *               versions of the locator.                         *
       *               Also, removed RESPOND due to 24x7 coverage.      *
       *                                                                *
       *  Arguments:                                                    *
       *   iNum             Number of STATION structures in pSta        *
       *   pSta             Station structure with Pdata                *
       *   piPBufCnt        Number of Ps in this structure              *
       *   Gparm            Configuration parameter structure           *
       *   pHypo            Computed hypocentral parameters             *
       *   iIndex           pSta index within array                     *
       *   Ewh              Earthworm.d defines                         *
       *   pcity            Array of reference city information         *
       *   iLoc             1=locate quake, 0=mags only                 *
       *   HypoStruct       Computed hypocenters for each buffer        *
       *   iBufCntStruct    Number of P-picks in each buffer so far     *
       *   pcityEC          Array of reference eastern city information *
       *   iDepthAvg        Array of average depths for all lat/lon     *
       *   iDepthMax        Array of maximum depths for all lat/lon     *
       *   pszIndexFile     FE Regions index file                       *
       *   pszLatFile       FE Regions Lat file                         *
       *   pszNameFile      FE Regions Name file                        *
       *   pszPathDistances Path to voice distance files                *
       *   pszPathDirections Path to voice direction files              *
       *   pszPathCities    Path to voice cities files                  *
       *   pszPathRegions   Path to voice region files                  *
       *                                                                *
       *  Return:                                                       *
       *   -1 if problem in location; otherwise ActiveBuffer index      *
       *                                                                *
       ******************************************************************/
	   
int LocateQuake( int iNum, STATION *pSta, int *piPBufCnt, GPARM *Gparm, 
                 HYPO *pHypo, int iIndex, EWH *Ewh, CITY *pcity, int iLoc, 
                 HYPO hypoStructArr[], int iBufCntStruct[], CITY *pcityEC, 
                 int iDepthAvg[][360], int iDepthMax[][360], 
                 char *pszIndexFile, char *pszLatFile, char *pszNameFile,
                 char *pszPathDistances, char *pszPathDirections, 
                 char *pszPathCities, char *pszPathRegions )
{
   double  dLonT;           /* Temp longitude */
   FILE    *hFile;          /* File handle */
   static  HYPO    HypoLast;/* Hypo parameters from last alarmed quake */
   int     i, j, iCnt, iFlag;
   int     iBadPIndices[16];/* Indices of stations tossed out by FindBadP */
   static int iInRegion;    /* 1->location within proper region; 0->otherwise */
   int     iFirst;          /* 1-> First time through locator */
   int     iMatch;          /* index of present quake in OldQuake File */										
   int     iNewAlarm;       /* 1->location/mag different than last alarm */
   int     iNumNewPs;       /* Number of new Ps since last location */
   int     iOrigGoodSoln;   /* This buffer's location quality before location */
   int     iPCnt = 0;       /* number of useable P-data points */										
   static int iRegion;      /* Quake epicenter region (NTWC AOR) */
   static int iRespond;     /* 1=>this quake was large enough to email out */
   int     iSendIt;         /* 1->Big enough to put on alarm, 0->Don't send */
   static  LATLON ll, ll2;  /* Quake lat/lon in geographic coords */
   static  OLDQUAKE OldQuake[MAX_QUAKES]; /* Previous auto-located quakes */
   STATION Sta;             /* Dummy variable for ReportAlarm call */
   char    szFile[64];      /* Created file name for P-time data */
   char    szPageMsg[128];  /* HYPO71SUM2K message for output */
   char    szMessage[256];  /* Pager message */
   char    szLastSta[8][8]; /* Latest station added to location */
   char    szTemp[16];      /* Temporary string */
   char    szTWCMsg[MAX_HYPO_SIZE]; /* TYPE_HYPOTWC message for output */
   
   iFirst = 1;
   if ( iLoc == 1 ) pHypo->iMagOnly = 0;
   else             pHypo->iMagOnly = 1;

/* Reset previously automatically eliminated stations */
   if ( iLoc == 1 )
   {
      for ( i=0; i<iNum; i++ )
         if ( pSta[i].dPTime > 0. ) 
/* If iUseMe = -1, then this was manually eliminated in Hypo - PW 11/2015 */
            if ( pSta[i].iUseMe >= 0 ) pSta[i].iUseMe = 1;

/* Save the most recent stations added in case a good solution goes bad */
      iOrigGoodSoln = pHypo->iGoodSoln;
      if ( (pHypo->iNumPs+pHypo->iNumBadPs) >= Gparm->MinPs )
      {
         iNumNewPs = *piPBufCnt - (pHypo->iNumPs+pHypo->iNumBadPs);
         if ( iNumNewPs > 8 ) iNumNewPs = 8;
         for ( i=0; i<iNumNewPs; i++ )
            for ( j=0; j<iNum; j++ )
               if ( pSta[j].iPickCnt == *piPBufCnt-i )
                  strcpy( szLastSta[i], pSta[j].szStation );
      } 

/* Are there are enough p-times to locate quake? */ 
      ReTry:            
      iPCnt = 0;
      for ( i=0; i<iNum; i++ )
         if ( pSta[i].iUseMe > 0 && pSta[i].dPTime > 0. ) iPCnt++;
      if ( iPCnt < Gparm->MinPs )
      {
         logit( "", "Not enough Ps; need %d, have %d; buffer %d\n",
                     Gparm->MinPs, iPCnt, iIndex );
         return -1;
      }
	  
/* Order Ps within array from first to last */	  
      qsort( pSta, iNum, sizeof (STATION), struct_cmp_by_EpicentralDistance );

/* First locate quake letting initial loc. be first P station */
      pHypo->iDepthControl = 3;                                      /* Fixed */
      pHypo->dDepth = DEPTHKM;                             /* Set for default */
      InitHypo( pHypo );                                    /* Init structure */
      InitialLocator( iNum, 3, 1, pSta, pHypo, 0., 0. );     /* Get start pt. */
      QuakeSolveIasp( iNum, pSta, pHypo, iDepthAvg, iDepthMax, 1 );/* Solve loc */
      IsItGoodSoln( iNum, pSta, pHypo, Gparm->MinPs );      /* Check Residual */
   
/* Try a depth float. */
      if ( pHypo->iGoodSoln != 3 )
      {
         pHypo->iDepthControl = 4;                                   /* Float */
         InitHypo( pHypo );                                 /* Init structure */
         InitialLocator( iNum, 3, 1, pSta, pHypo, 0., 0. );  /* Get start pt. */
         QuakeSolveIasp( iNum, pSta, pHypo, iDepthAvg, iDepthMax, 1 );
         IsItGoodSoln( iNum, pSta, pHypo, Gparm->MinPs );
      }
   
/* If this solution was not real good, try the Bad P finder */
      if ( pHypo->iGoodSoln != 3 && MAX_TO_KO > 0 )
      {
         InitHypo( pHypo );		
         FindBadPs( iNum, pSta, pHypo, 0., 0., Gparm->MinPs, iDepthAvg, 
                    iDepthMax, iPCnt );
      
/* See if there are many picks and good azimuthal control; let depth float. */
         if ( pHypo->iAzm >= 90 && pHypo->iNumPs >= Gparm->MinPs )
         {
            pHypo->iDepthControl = 4;                                /* Float */
/* Bring eliminated picks back in */
            iCnt = 0;
            for ( i=0; i<iNum; i++ )
               if ( pSta[i].iUseMe == 0 && pSta[i].dRes < 20. )
               {
                  pSta[i].iUseMe = 1;
                  iBadPIndices[iCnt] = i;
                  iCnt++;
                  if ( iCnt > 15 ) break;
               }
            QuakeSolveIasp( iNum, pSta, pHypo, iDepthAvg, iDepthMax, 1 );
            IsItGoodSoln( iNum, pSta, pHypo, Gparm->MinPs );
/* If this solution bombs, go back to fixed depth */
            if  ( pHypo->iGoodSoln < 2 )
            {
/* Remove originally eliminated picks */
               for ( i=0; i<iCnt; i++ )
                  pSta[iBadPIndices[i]].iUseMe = 0;
               pHypo->iDepthControl = 3;                             /* Fixed */
               pHypo->dDepth = DEPTHKM;                    /* Set for default */
               InitialLocator( iNum, 3, 1, pSta, pHypo, 0., 0. );
               QuakeSolveIasp( iNum, pSta, pHypo, iDepthAvg, 
                               iDepthMax, 1 );                  /* Solve loc. */
               IsItGoodSoln( iNum, pSta, pHypo, Gparm->MinPs );  /* Check Res */
            }
         }
      }
      
/* If this location was good last time and went bad this time around, 
   remove the last picks */      
      if ( (iOrigGoodSoln >= 2      && pHypo->iGoodSoln <= 1 &&
            iPCnt >= Gparm->MinPs+2 && iFirst           == 1 && 
            iNumNewPs > 0) ||
           (iOrigGoodSoln >= 2      && pHypo->iGoodSoln == 0 &&
            iFirst        == 1      && iNumNewPs > 0) )
      {
         for ( j=0; j<iNumNewPs; j++ )
            for ( i=0; i<iNum; i++ )
               if ( !strcmp( szLastSta[j], pSta[i].szStation ) )
               {
                  RemoveP( &pSta[i] );
                  iPCnt -= 1;
                  *piPBufCnt = *piPBufCnt-1;
                  logit( "", "Latest pick, %s, induced bad loc. and removed\n",
                         szLastSta[j] );
               }
         iFirst = 0;
         goto ReTry;
      } 
   }
   else                                         /* Zero res. for MS only data */
   {
      for ( i=0; i<iNum; i++ )
         if ( pSta[i].dPTime <= 1. ) pSta[i].dRes = 0.;
   }
   
/* We must have a solution now, good or not, so get magnitudes */   
   ZeroMagnitudes( pSta, iNum );
   AddInThetaL( pSta, iNum, Gparm->szThetaFile, pHypo );
   ComputeMagnitudes( iNum, pSta, pHypo );
   AddInMwL(    pSta, iNum, Gparm->szMwFile,    pHypo );
   GetPreferredMag( pHypo );
   
/* Is this the same quake as is being located in another buffer? */
   for ( i=0; i<Gparm->NumPBuffs; i++ )
      if ( i != iIndex && 
          (hypoStructArr[i].dLat != 0. || hypoStructArr[i].dLon != 0.) )	  
         if ( IsItSameQuake( pHypo, &hypoStructArr[i] ) == 1 )
            if ( iBufCntStruct[i] > iPCnt )            
            {                   /* If the other buffer has more picks, return */
               logit( "et", "Buffer match - %d\n", i );
               if ( iLoc == 1 )
               {
                  QuakeLog( iNum, pSta, pHypo, pcity, pcityEC, 
                            Gparm->szNameFile, Gparm->szNameFileLC, 
                            Gparm->szIndexFile, Gparm->szLatFile, 1, NULL );
                  QuakeLog2( Gparm->szQLogFile, pHypo );
                  pHypo->iVersion++;         
               }
               return( i );
            }
   
/* Has there been a recent large quake and does this location not have any
   near stations? If so, reset iGoodSoln to 1. */
   for ( i=0; i<Gparm->NumPBuffs; i++ )
      if ( i != iIndex && hypoStructArr[i].dPreferredMag > 5.8 &&
           pHypo->dNearestDist > DELTA_TELE && pHypo->iGoodSoln >= 2 &&
           pHypo->dOriginTime-hypoStructArr[i].dOriginTime < 3600. &&
           pHypo->dOriginTime-hypoStructArr[i].dOriginTime > 0. )	  
      {
         pHypo->iGoodSoln = 1;
         logit( "t", "Good Soln drop to 1 - recent large event %d\n", iIndex );
      }

/* Is the quake location within the region that this locator is solving? */
   iInRegion = 0;
   GeoGraphic( &ll, (LATLON *) pHypo );
   iRegion = GetRegion( ll.dLat, ll.dLon );                  /* For use later */
   if ( ll.dLon > 180. ) ll.dLon -= 360.;
   if ( Gparm->SouthernLat <= ll.dLat &&
        Gparm->NorthernLat >= ll.dLat &&
        Gparm->WesternLon  <= ll.dLon &&
        Gparm->EasternLon  >= ll.dLon ) iInRegion = 1;
   
/* Re-write the file monitored by SWD with time of first P */
   if ( iLoc == 1 && pHypo->iVersion == 1 && iInRegion == 1 )
   {
      if ( (hFile = fopen( Gparm->szAutoLoc, "w" )) != NULL )
      {
         fprintf( hFile, "%lf\n", pHypo->dFirstPTime );
         fclose( hFile );
      }
      else
         logit( "t", "Failed to open %s in AutoLoc\n", Gparm->szAutoLoc );
   }
   
/* Determine whether or not to REPORT this event outside of this module. */
   iRespond  = 0;
   iSendIt   = 0;
   iNewAlarm = 1; 	  
   if ( (pHypo->iGoodSoln    >= 2   && pHypo->iNumPs >= Gparm->MinPs-1   &&
         iInRegion           == 1)  ||
/* This next part allows REPORT of quakes with fair solutions but more
   picks.  Tried moving the min # picks from +7 to +4 in June, 2014.  May 
   be a bad idea. - PW */
        (pHypo->iGoodSoln    == 1   && pHypo->iNumPs >= (Gparm->MinPs+4) &&
         iInRegion           == 1   && pHypo->dPreferredMag >= 5.0       &&
         pHypo->dNearestDist <= 90. && pHypo->iAzm   >= 30) ) 
   {	
/* Update Old Quakes File */
      if ( (hFile = fopen( Gparm->szOldQuakes, "r" )) != NULL )
      {                                       /* First read in old quake data */
         for ( i=0; i<MAX_QUAKES; i++ )
            if ( fscanf( hFile, "%lf %lf %lf %lf %d %s %d %s %d %d %lf "
                         "%lf %lf %d %lf %d %lf %d %lf %d %lf %d "
                         "%lf %d %lf %lf %d\n",
             &OldQuake[i].dOTime, &OldQuake[i].dLat, &OldQuake[i].dLon,
             &OldQuake[i].dPreferredMag, &OldQuake[i].iNumPMags,
             (char *)(&OldQuake[i].szPMagType), &OldQuake[i].iDepth,
             (char *)(&OldQuake[i].szQuakeID), &OldQuake[i].iVersion,
             &OldQuake[i].iNumPs, &OldQuake[i].dAvgRes,
             &OldQuake[i].dAzm, &OldQuake[i].dMbAvg, &OldQuake[i].iNumMb,
             &OldQuake[i].dMlAvg, &OldQuake[i].iNumMl,
             &OldQuake[i].dMSAvg, &OldQuake[i].iNumMS,
             &OldQuake[i].dMwpAvg, &OldQuake[i].iNumMwp,
             &OldQuake[i].dMwAvg, &OldQuake[i].iNumMw,
             &OldQuake[i].d1stPTime, &OldQuake[i].iGoodSoln,
             &OldQuake[i].dTheta, &OldQuake[i].dThetaSD, 
             &OldQuake[i].iNumTheta ) != 27 ) break;
         fclose( hFile );		
/* Determine if this is the same quake as one already written to szOldQuake */
/* 9-14 - Added match based on location to synch up with Saved quakes in 
   EQCentral */
         iMatch = -1;
         for ( i=0; i<MAX_QUAKES; i++ )
		 {
            dLonT = OldQuake[i].dLon;
            if ( dLonT > 180. ) dLonT -= 360.;
            if ( (!strcmp( pHypo->szQuakeID, OldQuake[i].szQuakeID ) &&
                  pHypo->iVersion >= OldQuake[i].iVersion &&     /* Prob same */
                  pHypo->dOriginTime < OldQuake[i].dOTime + 1200. &&
                  pHypo->dOriginTime > OldQuake[i].dOTime - 1200.) ||
                 (fabs( ll.dLat-OldQuake[i].dLat ) < 0.5 &&
                  fabs( ll.dLon-dLonT            ) < 0.5 &&
                  fabs( pHypo->dOriginTime-OldQuake[i].dOTime ) < 60.) )
               iMatch = i;			
		 }
         logit( "", "iMatch = %ld\n", iMatch );
         if ( (hFile = fopen( Gparm->szOldQuakes, "w" )) == NULL ) 
            goto PastOQ;
         if ( iMatch == -1 )                 /* No Match - Put new one at top */
         {
            fprintf( hFile, "%lf %lf %lf %lf %d %s %d %s %d %d %lf "
                            "%lf %lf %d %lf %d %lf %d %lf %d %lf %d "
                            "%lf %d %lf %lf %d\n",
             pHypo->dOriginTime, ll.dLat, ll.dLon, pHypo->dPreferredMag,
             pHypo->iNumPMags, pHypo->szPMagType,
             (int) (pHypo->dDepth + 0.5), pHypo->szQuakeID, 
             pHypo->iVersion, pHypo->iNumPs, 
             pHypo->dAvgRes, (double) pHypo->iAzm, pHypo->dMbAvg,
             pHypo->iNumMb, pHypo->dMlAvg, pHypo->iNumMl, pHypo->dMSAvg,
             pHypo->iNumMS, pHypo->dMwpAvg, pHypo->iNumMwp,
             pHypo->dMwAvg, pHypo->iNumMw,
             pHypo->dFirstPTime, pHypo->iGoodSoln,
             pHypo->dTheta, pHypo->dThetaSD, pHypo->iNumTheta );
            for ( i=0; i<MAX_QUAKES-1; i++ )
               fprintf( hFile, "%lf %lf %lf %lf %d %s %d %s %d %d %lf "
                               "%lf %lf %d %lf %d %lf %d %lf %d %lf %d "
                               "%lf %d %lf %lf %d\n",
                OldQuake[i].dOTime, OldQuake[i].dLat, OldQuake[i].dLon,
                OldQuake[i].dPreferredMag, OldQuake[i].iNumPMags,
                OldQuake[i].szPMagType, OldQuake[i].iDepth,
                OldQuake[i].szQuakeID, OldQuake[i].iVersion,
                OldQuake[i].iNumPs, OldQuake[i].dAvgRes,
                OldQuake[i].dAzm, OldQuake[i].dMbAvg,OldQuake[i].iNumMb,
                OldQuake[i].dMlAvg, OldQuake[i].iNumMl,
                OldQuake[i].dMSAvg, OldQuake[i].iNumMS,
                OldQuake[i].dMwpAvg, OldQuake[i].iNumMwp,
                OldQuake[i].dMwAvg, OldQuake[i].iNumMw,
                OldQuake[i].d1stPTime, OldQuake[i].iGoodSoln,
                OldQuake[i].dTheta, OldQuake[i].dThetaSD, 
                OldQuake[i].iNumTheta  );
         }
         else                   /* Re-write file patching in updated location */
         {         		 
            for ( i=0; i<MAX_QUAKES; i++ )
               if ( i == iMatch )
                  fprintf( hFile, "%lf %lf %lf %lf %d %s %d %s %d %d %lf "
                   "%lf %lf %d %lf %d %lf %d %lf %d %lf %d "
                   "%lf %d %lf %lf %d\n",
                   pHypo->dOriginTime, ll.dLat, ll.dLon, 
                   pHypo->dPreferredMag,pHypo->iNumPMags, pHypo->szPMagType,
                   (int) (pHypo->dDepth + 0.5), pHypo->szQuakeID, 
                   pHypo->iVersion, pHypo->iNumPs, 
                   pHypo->dAvgRes, (double) pHypo->iAzm, pHypo->dMbAvg,
                   pHypo->iNumMb, pHypo->dMlAvg, pHypo->iNumMl, pHypo->dMSAvg,
                   pHypo->iNumMS, pHypo->dMwpAvg, pHypo->iNumMwp,
                   pHypo->dMwAvg, pHypo->iNumMw,
                   pHypo->dFirstPTime, pHypo->iGoodSoln,
                   pHypo->dTheta, pHypo->dThetaSD, pHypo->iNumTheta );
                else
                  fprintf( hFile, "%lf %lf %lf %lf %d %s %d %s %d %d %lf "
                           "%lf %lf %d %lf %d %lf %d %lf %d %lf %d "
                           "%lf %d %lf %lf %d\n",
                    OldQuake[i].dOTime, OldQuake[i].dLat, OldQuake[i].dLon,
                    OldQuake[i].dPreferredMag, OldQuake[i].iNumPMags,
                    OldQuake[i].szPMagType, OldQuake[i].iDepth,
                    OldQuake[i].szQuakeID, OldQuake[i].iVersion,
                    OldQuake[i].iNumPs, OldQuake[i].dAvgRes,
                    OldQuake[i].dAzm, OldQuake[i].dMbAvg, OldQuake[i].iNumMb,
                    OldQuake[i].dMlAvg, OldQuake[i].iNumMl,
                    OldQuake[i].dMSAvg, OldQuake[i].iNumMS,
                    OldQuake[i].dMwpAvg, OldQuake[i].iNumMwp,
                    OldQuake[i].dMwAvg, OldQuake[i].iNumMw,
                    OldQuake[i].d1stPTime, OldQuake[i].iGoodSoln,
                    OldQuake[i].dTheta, OldQuake[i].dThetaSD, 
                    OldQuake[i].iNumTheta  );
         }
         fclose( hFile );
      }
      else
         logit( "t", "Failed to open %s file in AutoLoc\n", 
                Gparm->szOldQuakes);
PastOQ:

/* Update P-time files */
      if ( iLoc == 1 ) WritePTimeFile( iNum, pSta, Gparm->szRTPFile, pHypo );      
      strcpy( szFile, Gparm->szPFilePath );      
      strcpy( szTemp, pHypo->szQuakeID );
      PadZeroes( 6, szTemp );
      strcat( szFile, szTemp );
      strcat( szFile, ".dat" );                
      WritePTimeFile( iNum, pSta, szFile, pHypo );      

/* Write to Dummy File */
      if ( iLoc == 1 && pHypo->iNumPs < 15 )
         WriteDummyData( pHypo, Gparm->szDummyFile, 1, 1 );

/* Send alarm over cell phones if warranted (i.e., Respond=1, and it is 
   different than last alarm) */
      if ( iLoc == 1 )
      {	
         if ( ((((iRegion <  5  || iRegion == 8) && pHypo->dPreferredMag >= 6.0) ||
                 (iRegion >= 10 && iRegion <= 14 && pHypo->dPreferredMag >  5.5) ||
                ((iRegion == 5  || iRegion == 9) && pHypo->dPreferredMag >= 6.0) ||
                 (iRegion >= 15                  && pHypo->dPreferredMag >= 6.0))&&
                  pHypo->iNumPs <  31            && (pHypo->iNumPs%6) == 0)      ||
                 (pHypo->dPreferredMag >= Gparm->MinMagToSend                    &&
                  pHypo->iAlarmIssued  == 0)     && pHypo->iNumPs >= Gparm->MinPs ) 
            iRespond = 1;
         if ( (((iRegion <  5  || iRegion == 8 ) && pHypo->dPreferredMag >= 3.4)  ||
               ((iRegion >= 10 && iRegion <= 14) && pHypo->dPreferredMag >= 3.4)  ||
               ((iRegion == 5  || iRegion == 9 ) && pHypo->dPreferredMag >= 5.0)  ||
                (iRegion >= 15 &&                   pHypo->dPreferredMag >= 5.0)) && 
                 pHypo->iNumPs < 50 ) iSendIt = 1;
         GeoGraphic( &ll2, (LATLON *) &HypoLast );
         while ( ll2.dLon > 180. ) ll2.dLon -= 360.;
         if ( fabs( ll2.dLat-ll.dLat ) < 0.5 &&
              fabs( ll2.dLon-ll.dLon ) < 0.5 &&
              fabs( HypoLast.dPreferredMag-pHypo->dPreferredMag ) <  0.2 &&
              fabs( HypoLast.dOriginTime-pHypo->dOriginTime     ) < 10.0 &&
              abs(  HypoLast.iNumPs-pHypo->iNumPs ) < 5 ) iNewAlarm = 0;
         if ( iSendIt == 1 && iNewAlarm == 1 )
         {  
            strcpy( szMessage, "\0" );
            LoadPagerString( pHypo, szMessage, pcity, pcityEC, Gparm );
            ReportAlarm( &Sta, Gparm->MyModId, Gparm->AlarmRegion,
             Ewh->TypeAlarm, Ewh->MyInstId, 4, szMessage, iRespond );
            HypoLast = *pHypo;
            logit( "t", "Should we announce?\n" );
            if ( pHypo->iAlarmIssued == 0 || pHypo->iAlarmIssued == 1 )
            {  
               logit( "t", "Announce this location\n" );
/* Check to see if any picks are force; if so no need to announce */
               iFlag = 0;
               for ( i=0; i<iNum; i++ )
                  if ( !strcmp( pHypo->szQuakeID, pSta[i].szHypoID ) ) 
                     iFlag = 1;
               if ( iFlag == 0 )
			   {
                  logit( "t", "Location announced\n" );
                  pHypo->iAlarmIssued += 1;
#ifdef _WINNT
                  SayLocation( ll.dLat, ll.dLon, pcity, pszIndexFile, pszLatFile, 
                               pszNameFile, pszPathDistances, pszPathDirections, 
                               pszPathCities, pszPathRegions );
/* Send to alarm log so it can show up in Summary */
#endif
                  strcpy( szMessage, "\0" );
                  LoadPagerString( pHypo, szMessage, pcity, pcityEC, Gparm );
                  ReportAlarm( &Sta, Gparm->MyModId, Gparm->AlarmRegion,
                   Ewh->TypeAlarm, Ewh->MyInstId, 4, szMessage, 2 );
			   }
            }
         }
      }

/* Report to Hypo Ring */
      MakeH71Msg( pHypo, szPageMsg );
      MakeTWCMsg( pHypo, szTWCMsg );
      ReportHypo( szPageMsg, szTWCMsg, Gparm->MyModId, Gparm->OutRegion,
                  Ewh->TypeHypoTWC, Ewh->TypeH71Sum2K, Ewh->MyInstId,
                  iLoc, pHypo->iNumPs );
   }
  
/* Log results and increment version */
   if ( iLoc == 1 )
   {
      QuakeLog( iNum, pSta, pHypo, pcity, pcityEC, Gparm->szNameFile,
                Gparm->szNameFileLC, Gparm->szIndexFile, Gparm->szLatFile, 1,
                NULL );
      QuakeLog2( Gparm->szQLogFile, pHypo );
      pHypo->iVersion++;   
   }

/* Return P buffer index */   
   return( iIndex );
}
                        
      /******************************************************************
       *                          MakeH71Msg()                          *
       *                                                                *
       * This function takes hypocenter parameters and puts them into a *
       * H71Sum2K message format.                                       *
       *                                                                *
       *  Arguments:                                                    *
       *   pHypo            Computed hypocentral parameters             *
       *   pszMsg           H71SUM2K message in proper format           *
       *                                                                *
       ******************************************************************/
       
void MakeH71Msg( HYPO *pHypo, char *pszMsg )
{
   double       dRes;                  /* Average residual */
   double       dTemp;                 /* Lat/Lon minutes */
   LATLON       ll;                    /* Lat/lon (geographic) array */
   long         lTime;                 /* 1/1/70 time */
   char         szTemp[20];
   struct tm    *tm;                   /* Origin time in tm structure */
/*
H71SUM2K SUMMARY FORMAT. From Lynn Dietz
1st Col  Length  Type           Description
0        4       int    %4d     Origin time year
4        2       int    %2d     Origin time month
6        2       int    %2d     Origin time day
8        1                      unused
9        2       int    %2d     Origin time hours
11       2       int    %2d     Origin time minutes
13       6       float  %6.2f   Origin time seconds
19       3       int    %3d     Latitude (degrees)
22       1       char   %c      S for south, blank for north
23       5       float  %5.2f   Latitude (decimal minutes)
28       4       int    %4d     Longitude (degrees)
32       1       char   %c      E for east, blank for west
33       5       float  %5.2f   Longitude (decimal minutes)
38       7       float  %7.2f   Depth (km)
45       1                      unused
46       1       char   %c      Magnitude type; D=duration, Z=low gain duration
47       5       float  %5.2f   Magnitude
52       3       int    %3d     # P&S times with weights > 0.1
55       4       int    %4d     Maximum azimuthal gap
59       5       float  %5.1f   Distance to nearest station
64       5       float  %5.2f   RMS travel time residual
69       5       float  %5.2f   Horizontal error (km)
74       5       float  %5.1f   Vertical error (km)
79       1       char   %c      Remark: Q if quarry blast
80       1       char   %c      Remark: Quality A-D
81       1       char   %c      Remark: Data source code
82       1                      unused
83       10      long   %10ld   Event ID number
93       1                      unused
94       1       char   %c      Version; 0=prelim, 1=Final w/ MD, 2=1+ML etc.
95       1       char   %c      newline character
*/

/* Clear message */
   strcpy( pszMsg, "\0" );

/* Convert o-time from 1/1/70 time to tm structure and fill in time fields */
   lTime = (long) (pHypo->dOriginTime);
   tm = TWCgmtime( lTime );
   itoaX( (int) tm->tm_year+1900, szTemp ); /* 0-3 year */
   PadZeroes( 4, szTemp );
   strcpy( pszMsg, szTemp );
   itoaX( (int) tm->tm_mon+1, szTemp );     /* 4-5 month */
   PadZeroes( 2, szTemp );
   strcat( pszMsg, szTemp );
   itoaX( (int) tm->tm_mday, szTemp );      /* 6-7 day */
   PadZeroes( 2, szTemp );             
   strcat( pszMsg, szTemp );
   strcat( pszMsg, " " );                   /* 8 blank */
   itoaX( (int) tm->tm_hour, szTemp );      /* 9-10 hour */
   PadZeroes( 2, szTemp );
   strcat( pszMsg, szTemp );
   itoaX( (int) tm->tm_min, szTemp );       /* 11-12 Minute */
   PadZeroes( 2, szTemp );
   strcat( pszMsg, szTemp );
   itoaX( (int) tm->tm_sec, szTemp );       /* 13-15 Second */
   PadZeroes( 3, szTemp );
   strcat( pszMsg, szTemp );
   strcat( pszMsg, "." );                   /* 16 . */
   itoaX( (int) ((pHypo->dOriginTime-floor( pHypo->dOriginTime ))*100.), szTemp );
   PadZeroes( 2, szTemp );                  /* 17-18 Hundredths Sec */
   strcat( pszMsg, szTemp );
   strcat( pszMsg, " " );                   /* 19 blank */
   
/* Convert lat/long from geocentric to geographic, then fill msg with loc. */
   GeoGraphic( &ll, (LATLON *) pHypo );
   itoaX( abs ((int) ll.dLat), szTemp );    /* 20-21 Latitude degrees */
   PadZeroes( 2, szTemp );
   strcat( pszMsg, szTemp );
   if ( ll.dLat > 0 ) strcpy( szTemp, " " );
   else               strcpy( szTemp, "S" );
   strcat( pszMsg, szTemp );               /* 22 S or blank for north */
   dTemp = 60. * fabs( ll.dLat - (int) ll.dLat );
   itoaX( (int) dTemp, szTemp );           /* 23-24 Latitude minutes */
   PadZeroes( 2, szTemp );
   strcat( pszMsg, szTemp );
   strcat( pszMsg, "." );                  /* 25 . */
   itoaX( (int) ((dTemp-(int)dTemp)*100.), szTemp );
   PadZeroes( 2, szTemp );                 /* 26-27 decimal minutes */
   strcat( pszMsg, szTemp );
   strcat( pszMsg, " " );                  /* 28 blank */
   while ( ll.dLon > 180. ) ll.dLon -= 360.;
   itoaX( abs( (int) ll.dLon ), szTemp );
   PadZeroes( 3, szTemp );                 /* 29-31 Longitude degrees */
   strcat( pszMsg, szTemp );
   if ( ll.dLon > 0 ) strcpy( szTemp, "E" );
   else               strcpy( szTemp, " " );
   strcat( pszMsg, szTemp );               /* 32 E or blank for W */
   dTemp = 60. * fabs( ll.dLon - (int) ll.dLon );
   itoaX( (int) dTemp, szTemp );           /* 33-34 Longitude minutes */
   PadZeroes( 2, szTemp );
   strcat( pszMsg, szTemp );
   strcat( pszMsg, "." );                  /* 35 . */
   itoaX( (int) ((dTemp-(int)dTemp)*100.), szTemp );
   PadZeroes( 2, szTemp );                 /* 36-37 decimal minutes */
   strcat( pszMsg, szTemp );
   strcat( pszMsg, " " );                  /* 38 blank */
   
/* Add depth to message (km) */   
   itoaX( (int) (pHypo->dDepth + 0.5), szTemp );
   PadZeroes( 3, szTemp );                 /* 39-41 Integer depth */
   strcat( pszMsg, szTemp );
   strcat( pszMsg, ".00" );                /* 42-44 Fill in expected format */
   strcat( pszMsg, " " );                  /* 45 blank */
   
/* Fill in magnitude type and magnitude */
   strncat( pszMsg, pHypo->szPMagType, 1 );
   strcat( pszMsg, "\0" );                 /* 46 magnitude type (b, w, etc.) */
   strcat( pszMsg, " " );                  /* 47 blank */
   itoaX( (int) pHypo->dPreferredMag, szTemp );
   strcat( pszMsg, szTemp );               /* 48 Integer mag. */
   strcat( pszMsg, "." );                  /* 49 . */
   itoaX( (int) ((pHypo->dPreferredMag-(int)pHypo->dPreferredMag)*100.),
                  szTemp );                /* 50-51 decimal mag. */
   PadZeroes( 2, szTemp );
   strcat( pszMsg, szTemp );
   
/* Number of P's used in location */
   itoaX( pHypo->iNumPs, szTemp );         /* 52-54 Number Ps used in loc */
   PadZeroes( 3, szTemp );
   strcat( pszMsg, szTemp );
   strcat( pszMsg, " " );                  /* 55 blank */
   
/* Azimuthal coverage of stations about the epicenter (converted to gap) */
   itoaX( (360-pHypo->iAzm), szTemp );
   PadZeroes( 3, szTemp );                 /* 56-58 Azimuthal gap */
   strcat( pszMsg, szTemp );
   
/* Nearest station's distance in degrees */
   itoaX( (int) pHypo->dNearestDist, szTemp );
   PadZeroes( 3, szTemp );                 /* 59-61 integer nearest distance */
   strcat( pszMsg, szTemp );
   strcat( pszMsg, "." );                  /* 62 . */
   itoaX( (int) ((pHypo->dNearestDist-(int)pHypo->dNearestDist)*10.),
                  szTemp );                /* 63 decimal distance */
   PadZeroes( 1, szTemp );
   strcat( pszMsg, szTemp );
   
/* Add Residual */
   dRes = pHypo->dAvgRes;
   if ( dRes > 99.99 ) dRes = 99.99;       /* Fit it in the space given */
   itoaX( (int) dRes, szTemp );            /* 64-65 integer average residual */
   PadZeroes( 2, szTemp );
   strcat( pszMsg, szTemp );
   strcat( pszMsg, "." );                  /* 66 . */
   itoaX( (int) ((dRes-(int)dRes)*100.), szTemp );
   PadZeroes( 2, szTemp );                 /* 67-68 decimal residual */
   strcat( pszMsg, szTemp );
   
/* Errors not computed by loc_wcatwc */
   strcat( pszMsg, "            " );       /* 69-80 blank */
   
/* Flags indicating regional center, quake id, and version */   
   strcat( pszMsg, "P" );                  /* 81 Data Source code (P=Palmer) */
   strcat( pszMsg, "  " );                 /* 82-83 blank */
   strcpy( szTemp, pHypo->szQuakeID );     /* 84-92 Quake ID */
   PadZeroes( 9, szTemp );
   strcat( pszMsg, szTemp );
   itoaX( pHypo->iVersion, szTemp);        /* 93-94 version */
   PadZeroes( 2, szTemp );
   strcat( pszMsg, szTemp );
   pszMsg[95] = '\n';
   pszMsg[96] = '\0';
}

      /******************************************************************
       *                          MakeTWCMsg()                          *
       *                                                                *
       * This function takes hypocenter parameters and puts them into a *
       * TYPE_HYPOTWC message format.                                   *
       *                                                                *
       *  Arguments:                                                    *
       *   pHypo            Computed hypocentral parameters             *
       *   pszMsg           HYPOTWC message in proper format            *
       *                                                                *
       ******************************************************************/
       
void MakeTWCMsg( HYPO *pHypo, char *pszMsg )
{
   char    szTemp[16];

/*
HYPOTWC SUMMARY FORMAT.
Type     Description
----     -----------
int      Quake ID
int      Version
double   Origin time (1/1/70 seconds)
double   Latitude (geocentric)
double   Longitude (geocentric)
int      Depth (km)
int      # P times used in location
int      Station Azimuthal coverage around epicenter
double   RMS travel time residual (absolute average)
int      Quality: 0=poor, 1=fair, 2=good, 3=very good (based on residual)
double   Preferred magnitude (modified average)
char     Type of preferred magnitude (b, l, S, w)
int      # stations used in preferred magnitude
double   MS magnitude (modified average)
int      # stations used in Ms
double   Mwp magnitude (modified average)
int      # stations used in Mwp
double   Mb magnitude (modified average)
int      # stations used in Mb
double   Ml magnitude (modified average)
int      # stations used in Ml
double   Mw magnitude (modified average)
int      # stations used in Mw
double   Theta (modified average)
int      # stations used in Theta
int      0->new location, 1->new magnitude only
*/

/* Clear message */
   strcpy( pszMsg, "\0" );

/* Pad quake ID with zeroes - PW 04/17/11 */
   strcpy( szTemp, pHypo->szQuakeID );    
   PadZeroes( 6, szTemp );
/* Fill message */
   sprintf( pszMsg, "%s %d %lf %lf %lf %lf %d %d %lf %d %lf %s %d %lf "
                    "%d %lf %d %lf %d %lf %d %lf %d %lf %d %d  ",
    szTemp, pHypo->iVersion, 
    pHypo->dOriginTime, pHypo->dLat, pHypo->dLon,
    pHypo->dDepth, pHypo->iNumPs, pHypo->iAzm,
    pHypo->dAvgRes, pHypo->iGoodSoln, 
    pHypo->dPreferredMag, pHypo->szPMagType, pHypo->iNumPMags,
    pHypo->dMSAvg, pHypo->iNumMS, pHypo->dMwpAvg, pHypo->iNumMwp, 
    pHypo->dMbAvg, pHypo->iNumMb, pHypo->dMlAvg, pHypo->iNumMl, pHypo->dMwAvg,
    pHypo->iNumMw, pHypo->dTheta, pHypo->iNumTheta, pHypo->iMagOnly );	
}

     /**************************************************************
      *                         RemoveP()                          *
      *                                                            *
      * Remove a STATION structure from a P buffer.                *
      *                                                            *
      * Arguments:                                                 *
      *  pSta        Station Array to remove pick from             *
      *                                                            *
      **************************************************************/
	  
void RemoveP( STATION *pSta )
{
   InitVar( pSta );
   InitP( pSta );
   pSta->iUseMe = 0;
}

#ifdef _WINNT
   /*******************************************************************
    *                    SayLocation()                                *
    *                                                                 *
    * This function calls the PlaySound function to announce the      *
    * littoral location of the earthquake over the speakers.  Erica   *
    * Dilley's voice (NTWC science aide, 1995) is used in the WAV     *
    * files. In 2/2000, the regions spoken were changed to the Flinn- *
    * Engdahl regions. These were recorded by NTWC science aide       *
    * Heather Ingram.                                                 *
    *                                                                 *
    *  Arguments:                                                     *
    *    dLat          Earthquake geographic (+/-) latitude           *
    *    dLon          Earthquake geographic (+/-) longitude          *
    *    pcity         Pointer to array of reference cities           *
    *    EVL           Configuration parameter structure              *
    *    pszIndexFile  FE Regions index file                          *
    *    pszLatFile    FE Regions Lat file                            *
    *    pszNameFile   FE Regions Name file                           *
    *    pszPathDistances Path to voice distance files                *
    *    pszPathDirections Path to voice direction files              *
    *    pszPathCities Path to voice cities files                     *
    *    pszPathRegions Path to voice region files                    *
    *                                                                 *
    *******************************************************************/
void SayLocation( double dLat, double dLon, CITY *pcity,  
                  char *pszIndexFile, char *pszLatFile, char *pszNameFile,
                  char *pszPathDistances, char *pszPathDirections, 
                  char *pszPathCities, char *pszPathRegions )
{
   CITYDIS CityDis;      /* Distance/azimuth from nearest cities to epi */
   int     iFERegion;    /* Flinn-Engdhal Region number */
   int     iCity;        /* Closest city index */
   LATLON  ll;
   char   *psz;          /* File name for WAV file */

   ll.dLat = dLat;       /* Put location in LATLON structure for future calls */
   ll.dLon = dLon;
   GeoCent( &ll );                /* Convert geographic lat/lon to geocentric */
   GetLatLonTrig( &ll );
/* Get distances to nearest cities */
   NearestCities( &ll, pcity, &CityDis );
   if ( CityDis.iDis[0] <= 200 || CityDis.iDis[1] <= 200 )  /* Use a city ref */
   {
      if ( CityDis.iDis[0] < CityDis.iDis[1] ) iCity = 0;
      else                                     iCity = 1;
/* First, say "The earthquake was located xxx miles" */
      psz = GetDistanceFile( &CityDis, iCity, pszPathDistances );
      PlaySound( psz, NULL, SND_NODEFAULT | SND_NOSTOP | SND_NOWAIT | 
                 SND_FILENAME | SND_SYNC );
/* Next, say "XX of" */
      psz = GetDirectionFile( &CityDis, iCity, pszPathDirections );
      PlaySound( psz, NULL, SND_NODEFAULT | SND_NOSTOP | SND_NOWAIT | 
                 SND_FILENAME | SND_SYNC );
/* Next, say "cityname" */
      psz = GetCityFile( &CityDis, pcity, iCity, pszPathCities );
      PlaySound( psz, NULL, SND_NODEFAULT | SND_NOSTOP | SND_NOWAIT | 
                 SND_FILENAME | SND_ASYNC );
   }
   else                                           /* Use a regional reference */
   {
      ll.dLat = dLat;         /* Reset LATLON structure to geographic coords. */
      ll.dLon = dLon;
      while ( ll.dLon > 180. ) ll.dLon -= 360.;
      psz = namnum( ll.dLat, ll.dLon, &iFERegion, pszIndexFile,
                    pszLatFile, pszNameFile );
      psz = GetFERegionFile( iFERegion, pszPathRegions );
/* Announce the location */
      PlaySound( psz, NULL, SND_NODEFAULT | SND_NOSTOP | SND_NOWAIT | 
                 SND_FILENAME | SND_ASYNC );
   }
}
#endif
