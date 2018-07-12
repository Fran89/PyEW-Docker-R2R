 /************************************************************************
  * LOCQUAKE.C                                                           *    
  *                                                                      *
  * These functions locate earthquakes using the technique used at the   *
  * NTWC for many years (since the late 1970's).  There are two steps    *
  * to locating an earthquake with these functions.  First, an initial   *
  * guess at the location is arrived at through InitialLocator.  This    *
  * function returns either: the location of the first station which     *
  * picked up the earthquake (for local or regional quakes), a user input*
  * location, or a location computed from 4 of the total number of sites *
  * which recorded the quake.  The last option scribes 4 tripartites of  *
  * stations within the 4 station array.  For each tripartite, a location*
  * is computed based on the arrival azimuth (from P-time differences)   * 
  * and the distance (from incident angles).  This approach was written  *
  * by Whitmore in 1987.  The initial location is then passed to the     *
  * QuakeSolve function which determines a more exact location by using  *
  * all stations and minimizing the residuals (matrix solving).  This    *
  * function was set up by Sokolowski and others in the late 1970's/early*
  * 1980's. The HYPO structure is filled up by the functions.  IASPEI91  *
  * travel time tables are used tables can be used throughout.           *
  *                                                                      *
  * A function is also included which tests the residuals of the location*
  * and relocates if necessary by throwing out different combinations of *
  * stations. This is called FindBadPs.                                  *
  *                                                                      *
  *  September, 2013: Optimized FindDepth for Speed.                     *
  *  October, 2010: Merged PPICK and STATION structures                  *
  *  April, 2007: Removed JB location option                             *
  *  April, 2007: Combined PARRAY and QUAKELOC structures with Earthworm *
  *               structures                                             *
  *                                                                      *
  * Made into earthworm module 2/2001.                                   *
  *                                                                      *
  ************************************************************************/
  
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>  
#include <earthworm.h>
#include <transport.h>
#include "earlybirdlib.h"
#include "iaspei91.h"          /* Tau/P travel times (from iaspei91 model) */

      /******************************************************************
       *                   ComputePRTimeWindows()                       *
       *                                                                *
       * This function takes the origin time and epicenter and figures  *
       * the P-wave and Rayleigh wave time windows for an event.  The   *
       * P-wave window is from p-time to an amount up to MB_TIME after  *
       * the p-time.  The Rayleigh wave window is also computed (10s    *
       * before eta to MS_TIME after).  All times are in 1/1/70 s.      *
       *                                                                *
       *  Arguments:                                                    *
       *   pHypo            Computed hypocentral parameters             *
       *   pSta             Array of STATION structures                 *
       *   iNumStas         Number of items in this structure           *
       *   iFlag            1-Ms Window, Mm Window                      *
       *                                                                *
       ******************************************************************/
void ComputePRTimeWindows( HYPO *pHypo, STATION pSta[], int iNumStas, 
                           int iFlag )
{
   AZIDELT Azi;
   double  dDist2, dOver, dTemp1, dTemp2, dFracDep; /* Interpolated values */
   int     i, iTab, iDep;
   int     iDepth;                                  /* Temp. depth in km */
/* Rayleigh wave travel time in seconds */
   int     iRTravTime[181] = {0,32,63,95,127,159,190,222,254,285,317,/* 0-10 */
            349,381,412,441,475,507,539,571,603,634,           /* 11-20 */
            661,688,715,742,769,796,823,850,877,900,           /* 21-30 */
            930,960,990,1020,1050,1080,1110,1140,1170,1200,    /* 31-40 */
            1230,1260,1290,1320,1350,1380,1410,1440,1470,1500, /* 41-50 */
            1530,1560,1590,1620,1650,1680,1710,1740,1770,1800, /* 51-60 */
            1819,1838,1858,1877,1896,1915,1934,1953,1972,1992, /* 61-70 */
            2021,2049,2078,2106,2135,2163,2192,2220,2248,2277, /* 71-80 */
            2305,2334,2362,2391,2419,2448,2476,2505,2533,2562, /* 81-90 */
            2590,2618,2648,2675,2704,2732,2761,2789,2818,2846, /* 91-100 */
            2875,2903,2932,2960,2988,3017,3045,3074,3102,3131, /* 101-110 */
            3159,3188,3216,3245,3273,3302,3330,3358,3387,3415, /* 111-120 */
            3444,3472,3501,3529,3558,3586,3615,3643,3672,3700, /* 121-130 */
            3728,3757,3785,3814,3842,3871,3899,3928,3956,3985, /* 131-140 */
            4013,4042,4070,4098,4127,4155,4184,4212,4241,4269, /* 141-150 */
            4298,4326,4355,4383,4412,4440,4468,4497,4525,4554, /* 151-160 */
            4582,4611,4639,4668,4696,4725,4753,4782,4810,4838, /* 161-170 */
            4867,4895,4924,4952,4981,5009,5038,5066,5095,5123};/* 171-180 */
   LATLON  ll;
   float  *pfIAT;              /* Pointer to IASPEI time of interest */
        
/* First, find distance to stations */
   for ( i=0; i<iNumStas; i++ )
   {
      ll.dLat = pSta[i].dLat;
      ll.dLon = pSta[i].dLon;
      GeoCent( &ll );
      GetLatLonTrig( &ll );
      GetDistanceAz( (LATLON *) pHypo, &ll, &Azi );
      pSta[i].dDelta   = Azi.dDelta;
      pSta[i].dAzimuth = Azi.dAzimuth;
   }
   iDepth = (int) pHypo->dDepth;
   
/* Get P-time variables using IASPEI91 tables */
   if ( iDepth < 0 ) iDepth = 0;
   if ( iDepth >= (int) ((DEPTH_LEVELS_IASP-1)*IASP_DEPTH_INC) )  
      iDepth = (int) ((DEPTH_LEVELS_IASP-1)*IASP_DEPTH_INC);
   iDep     = (int) ((double) iDepth/IASP_DEPTH_INC) + 1;
   dFracDep = (double) iDepth/IASP_DEPTH_INC - 
              floor( (double) iDepth/IASP_DEPTH_INC );
/* Second, get P-wave travel times */
   for ( i=0; i<iNumStas; i++ )
   {
      dOver  = pSta[i].dDelta*(1./IASP_DIST_INC) - 
               floor( pSta[i].dDelta * (1./IASP_DIST_INC) );
      iTab   = (int) (pSta[i].dDelta * (1./IASP_DIST_INC));
/* Take depth into account in P-wave travel time */
      pfIAT  = fPP + iDep*IASP_NUM_PER_DEP + iTab;
      dTemp1 = *pfIAT + (*(pfIAT+1) - *pfIAT) * dOver;
      dTemp2 = *(pfIAT-IASP_NUM_PER_DEP) + 
               (*(pfIAT-IASP_NUM_PER_DEP+1) - *(pfIAT-IASP_NUM_PER_DEP))*dOver;
      pSta[i].dPTravTime = dTemp1*dFracDep + dTemp2*(1.-dFracDep);
   }
/* Third, get Rayleigh wave travel time */
   for ( i=0; i<iNumStas; i++ )
   {
      dDist2 = pSta[i].dDelta;
      dOver  = dDist2 - floor( dDist2 );
      if ( iFlag == 1 )
         pSta[i].dRTravTime = (double) iRTravTime[(int) dDist2] +
          ((double) (iRTravTime[(int) dDist2+1] - iRTravTime[(int) dDist2]) * 
          dOver);
      else if ( iFlag == 2 )
         pSta[i].dRTravTime = dDist2*DEGTOKM/4.3;
   }
/* Fourth, find beginning and end of P-wave window  */
   for ( i=0; i<iNumStas; i++ ) 
   {
      pSta[i].dPStartTime = pHypo->dOriginTime + pSta[i].dPTravTime;
      pSta[i].dPEndTime   = pHypo->dOriginTime + pSta[i].dPTravTime + MB_TIME;
   }
/* Fifth, find beginning and end of Rayleigh window  */
   for ( i=0; i<iNumStas; i++ )
   {
      pSta[i].dRStartTime = pHypo->dOriginTime + pSta[i].dRTravTime - 10.;
      if ( iFlag == 1 )
         pSta[i].dREndTime = pHypo->dOriginTime + pSta[i].dRTravTime + MS_TIME;
      else if ( iFlag == 2 )
         pSta[i].dREndTime = pHypo->dOriginTime + pSta[i].dRTravTime + MM_TIME;
   }
}

      /******************************************************************
       *                         FindBadPs()                            *
       *                                                                *
       * This function is used to search for P-times not associated with*
       * the majority of P-times in the P array.  It does this by       *
       * successively solving for the hypocenter parameters; throwing   *
       * out different combinations of stations and checking what the   *
       * residuals are for the quake without those stations.  The       *
       * solution with the lowest average residual is assumed to be the *
       * correct location.  The function will pick out a maximum of 3   *
       * unassociated P-times. First it tries eliminating just 1        *
       * station.  If that doesn't work, it tries for 2, then 3 if there*
       * are enough stations in the array.                              *
       *                                                                *
       * October, 2013: Removed second pass through the InitialLocator  *     
       *                tripartite solver.                              *       
       * February, 2011: Modified how many stations can be removed.     *
       * January, 2002: If iUseMe=2, do not try to remove it.           *
       *                                                                *
       *  Arguments:                                                    *
       *   iNum             Number of stations in STATION structure     *
       *   pSta             Array of STATION structures                 *
       *   pHypo            Computed hypocentral parameters             *
       *   dLatUser         Latitude specified by user (geographic)     *
       *   dLonUser         Longitude specified by user                 *
       *   iMinPs           Minimum Ps needed for soln                  *
       *   iDepthAvg        Average depths (km) for all lat/lons        *
       *   iDepthMax        Maximum depths (km) for all lat/lons        *
       *   iCnt             # stations to locate with                   *
       *                                                                *
       ******************************************************************/

void FindBadPs( int iNum, STATION pSta[], HYPO *pHypo, double dLatUser,
                double dLonUser, int iMinPs, int iDepthAvg[][360], 
                int iDepthMax[][360], int iCnt ) 
{
   double  dInitLat, dInitLon;  /* Lat/lon of station with first P time */
   double  dMinResid;     /* Min residual for the suite of locs. just done */
   double  dTemp;
   int     i, j, k;
   int     iMinIndex1, iMinIndex2, iMinIndex3;  /* Indices used for dMinResid */
   int     iType;         /* 1=InitLoc with lowest P timee; 2-Tripartite
   
   if ( MAX_TO_KO < 1 ) return;

/* Force the depth control to fixed at default */
   pHypo->dDepth = DEPTHKM;                                /* Set for default */
   pHypo->iDepthControl = 3;                                         /* Fixed */
   iMinIndex1 = 0;
   iMinIndex2 = 0;
   iMinIndex3 = 0;
   InitHypo( pHypo );		
/* Get the initial location - lat/lon of first P-pick */
   InitialLocator( iNum, 3, 1, pSta, pHypo, dLatUser, dLonUser );
   dInitLat = pHypo->dLat;
   dInitLon = pHypo->dLon;

/* If not enough stations to search through, just locate and return */
   if ( iCnt < 5 ) 
   {
      pHypo->dLat = dInitLat;
      pHypo->dLon = dInitLon;
      QuakeSolveIasp( iNum, pSta, pHypo, iDepthAvg, iDepthMax, 1 );
      IsItGoodSoln( iNum, pSta, pHypo, iMinPs );
/* If this is an auto-initial location, see if loc good */
      if ( pHypo->iGoodSoln != 3 )
/* If not, re-locate using tripartite initial locator */
      {
         InitHypo (pHypo);		
         InitialLocator (iNum, 3, 2, pSta, pHypo, dLatUser, dLonUser);
         QuakeSolveIasp( iNum, pSta, pHypo, iDepthAvg, iDepthMax, 1 );
         IsItGoodSoln( iNum, pSta, pHypo, iMinPs );
      }
      return;
   }

/* Try knocking out just one station */
   dMinResid = 1.e20;                               /* Any big number will do */
   iType = 1;
   for ( i=0; i<iNum; i++ )
      if ( pSta[i].iUseMe == 1 && pSta[i].dPTime > 0. )
      {
         pSta[i].iUseMe = 0;
         InitHypo( pHypo );		
         pHypo->dLat = dInitLat;
         pHypo->dLon = dInitLon;
         QuakeSolveIasp( iNum, pSta, pHypo, iDepthAvg, iDepthMax, 1 );
         IsItGoodSoln( iNum, pSta, pHypo, iMinPs );
         dTemp = pHypo->dAvgRes;
/* If this is not excellent solution, try tripartite initial locator */
         if ( pHypo->iGoodSoln != 3 )
         {              
            InitHypo( pHypo );		
            InitialLocator( iNum, 3, 2, pSta, pHypo, dLatUser, dLonUser );
            QuakeSolveIasp( iNum, pSta, pHypo, iDepthAvg, iDepthMax, 1 );
         }
/* Compare residual to minimum so far */
         if ( dTemp < dMinResid || pHypo->dAvgRes < dMinResid ) 
            if ( dTemp <= pHypo->dAvgRes )
            {                         
               dMinResid = dTemp;
               iMinIndex1 = i;
               iType = 1;
            }
            else
            {                         
               dMinResid = pHypo->dAvgRes;
               iMinIndex1 = i;
               iType = 2;
            }
         pSta[i].iUseMe = 1;
      }

/* Fill HYPO structure with hypo params from sol'n with lowest res. */
   pSta[iMinIndex1].iUseMe = 0;
   InitHypo( pHypo );		
   InitialLocator( iNum, 3, iType, pSta, pHypo, dLatUser, dLonUser );
   QuakeSolveIasp( iNum, pSta, pHypo, iDepthAvg, iDepthMax, 1 );
   IsItGoodSoln( iNum, pSta, pHypo, iMinPs );

/* If 6 P-times or less available, don't try to knock out 2 stations
   (or if more than X; it will take to long to compute) */
   if ( iCnt <= 6 || iCnt > 140 ) return;

/* If the Residuals are reasonable, return */
   if ( pHypo->iGoodSoln == 3 ) return;
   pSta[iMinIndex1].iUseMe = 1;
   
/* It wasn't a great solution, so try knocking out 2 stations at a time */
   if ( MAX_TO_KO < 2 ) return;
   dMinResid = 1.e20;                               /* Any big number will do */
   iType = 1;
   for ( i=0; i<iNum-1; i++ )
      if ( pSta[i].iUseMe == 1 && pSta[i].dPTime > 0. )
         for ( j=i+1; j<iNum; j++ )
    	    if ( pSta[j].iUseMe == 1 && pSta[j].dPTime > 0. )
            {
               pSta[i].iUseMe = 0;                   /* Turn off two stations */
               pSta[j].iUseMe = 0;
               InitHypo( pHypo ); 		
               pHypo->dLat = dInitLat;
               pHypo->dLon = dInitLon;
               QuakeSolveIasp( iNum, pSta, pHypo, iDepthAvg, iDepthMax, 1 );
               IsItGoodSoln( iNum, pSta, pHypo, iMinPs );
               dTemp = pHypo->dAvgRes;
/* If this is not excellent solution, try tripartite initial locator;
   but only use this if there is a limited # of stations. */
               if ( pHypo->iGoodSoln != 3 && iCnt < 14 )
               {
                  InitHypo( pHypo );		
                  InitialLocator( iNum, 3, 2, pSta, pHypo, dLatUser, dLonUser );
                  QuakeSolveIasp( iNum, pSta, pHypo, iDepthAvg, iDepthMax, 1 );
               }
/* Compare residual to minimum so far */
               if ( dTemp < dMinResid || pHypo->dAvgRes < dMinResid ) 
                  if ( dTemp <= pHypo->dAvgRes )
                  {                         
                     dMinResid = dTemp;
                     iMinIndex1 = i;
                     iMinIndex2 = j;
                     iType = 1;
                  }
                  else
                  {                         
                     dMinResid = pHypo->dAvgRes;
                     iMinIndex1 = i;
                     iMinIndex2 = j;
                     iType = 2;
                  }
               pSta[i].iUseMe = 1;
               pSta[j].iUseMe = 1;
            }
	    
/* Fill HYPO structure with hypo params from sol'n with lowest res. */
   pSta[iMinIndex1].iUseMe = 0;
   pSta[iMinIndex2].iUseMe = 0;
   InitHypo( pHypo );		
   InitialLocator( iNum, 3, iType, pSta, pHypo, dLatUser, dLonUser );
   QuakeSolveIasp( iNum, pSta, pHypo, iDepthAvg, iDepthMax, 1 );
   IsItGoodSoln( iNum, pSta, pHypo, iMinPs );
	
/* If 10 P-times or less available, don't try to knock out 3 stations
   (or if more than 25; it will take to long to compute) */
   if ( iCnt <= 10 || iCnt > 25 ) return;

/* If the Residuals are reasonable, return */
   if ( pHypo->iGoodSoln == 3 ) return;
   pSta[iMinIndex1].iUseMe = 1;
   pSta[iMinIndex2].iUseMe = 1;

/* It still wasn't a great solution, so try knocking out 3 stations at a time 
   (if we don't have too many stations; if we do, this takes forever) 
   Note: To save time, this part does not invoke the tripartite init loc. */
   if ( MAX_TO_KO < 3 ) return;
   dMinResid = 1.e20;                     /* Any big number will do */
   for ( i=0; i<iNum-2; i++ )
      if ( pSta[i].iUseMe == 1 && pSta[i].dPTime > 0. )
         for ( j=i+1; j<iNum-1; j++ )
            if ( pSta[j].iUseMe == 1 && pSta[j].dPTime > 0. )
               for ( k=j+1; k<iNum; k++ )
                  if ( pSta[k].iUseMe == 1 && pSta[k].dPTime > 0. )
                  {
                     pSta[i].iUseMe = 0;           /* Turn off three stations */
                     pSta[j].iUseMe = 0;
                     pSta[k].iUseMe = 0;
                     InitHypo( pHypo );			
                     pHypo->dLat = dInitLat;
                     pHypo->dLon = dInitLon;
                     QuakeSolveIasp( iNum, pSta, pHypo, iDepthAvg, iDepthMax, 
                                     1 );
                     if ( pHypo->dAvgRes < dMinResid )  
                     {                  /* Compare residual to minimum so far */
                        dMinResid = pHypo->dAvgRes;
                        iMinIndex1 = i;
                        iMinIndex2 = j;
                        iMinIndex3 = k;
                     }
                     pSta[i].iUseMe = 1;
                     pSta[j].iUseMe = 1;
                     pSta[k].iUseMe = 1;
                  }
		  
/* Fill HYPO structure with hypo params from sol'n with lowest res. */
   pSta[iMinIndex1].iUseMe = 0;
   pSta[iMinIndex2].iUseMe = 0;
   pSta[iMinIndex3].iUseMe = 0;
   InitHypo( pHypo );		
   pHypo->dLat = dInitLat;
   pHypo->dLon = dInitLon;
   QuakeSolveIasp( iNum, pSta, pHypo, iDepthAvg, iDepthMax, 1 );
   IsItGoodSoln( iNum, pSta, pHypo, iMinPs );
}

      /******************************************************************
       *                        FindDepthNew()                          *
       *                                                                *
       * Returns the best guess depth and maximum expected depth for a  *
       * certain location.  The best gues and max expected are based    *
       * on historic quakes over 5.                                     *
       *                                                                *
       *  Arguments:                                                    *
       *   dLatpass         Latitude to search (geo +/-)                *
       *   dLonpass         Longitude to search (geo +/-)               *
       *   piAvg            Average depth in km to return               *
       *   piMax            Maximum depth in km to return               *
       *   iDepthAvg        Array of average depths (km)                *
       *   iDepthMax        Array of maximum depths (km)                *
       *                                                                *
       *  Returns:                                                      *
       *   int              Index if location found, -1 otherwise       *
       *                                                                *
       ******************************************************************/
       
void FindDepthNew( double dLatpass, double dLonpass, int *piAvg, int *piMax,
                   int iDepthAvg[][360], int iDepthMax[][360] )
{
   int   i, j, iRndLat, iRndLon, iMax;
    
/* Round double to nearest int */
   iRndLon = Round( dLonpass );
   iRndLat = Round( dLatpass );
/* Convert to co-latitude and east longitude */
   if ( iRndLat >= 0   )    iRndLat = 90 - iRndLat;
   else if ( iRndLat <  0 ) iRndLat = abs( iRndLat ) + 90;
   if ( iRndLat <  0   )    iRndLat = 0;
   if ( iRndLat >= 180 )    iRndLat = 179;
   if ( iRndLon <  0   )    iRndLon += 360;
   if ( iRndLon <  0   )    iRndLon =  0;
   if ( iRndLon >= 360 )    iRndLon =  359;

/* Set average depth */ 
   *piAvg = iDepthAvg[iRndLat][iRndLon]; 

/* Search 5 degrees around lat/lon for max depth and use that */
   iMax = DEFAULT_MAXDEPTH;
   for ( i=iRndLat-2; i<=iRndLat+2; i++ ) 
      for ( j=iRndLon-2; j<=iRndLon+2; j++ ) 
      {
         if ( i < 0 || i > 179 || j < 0 || j > 359 ) continue;         
         if ( iDepthMax[i][j] > iMax ) iMax = iDepthMax[i][j];
      }

   *piMax = iMax; 
   return;
}

      /******************************************************************
       *                        ForceLocIasp()                          *
       *                                                                *
       * This function shows how well a location fits a group of travel *
       * times.  Residuals are then returned.  Travel times are taken   *
       * from the IASPEI91 travel time tables.                          *
       *                                                                *
       *  April, 2007: Updated QUAKELOC structure with HYPO structure   *
       *  September, 2005: Fixed so that O-time is set in calling       *
       *                   program.                                     *
       *                                                                *
       *  Arguments:                                                    *
       *   iNum             Number of stations in STATION structure     *
       *   pSta             Array of STATION structures                 *
       *   pHypo            Computed hypocentral parameters             *
       *                                                                *
       ******************************************************************/

void ForceLocIasp( int iNum, STATION pSta[], HYPO *pHypo )
{
   double  fih, fh, ptt1, ptt2, ptt;       /* Interpolation values */
   int     i, iCnt, ih, itab;              /* Counters, indices */
   float  *pfIA;                           /* Pointer to travel time table */

   pHypo->iNumPs    = 0;               
   pHypo->iNumBadPs = 0;
   for ( i=0; i<iNum; i++ )
      if ( pSta[i].iUseMe && pSta[i].dPTime > 0. ) pHypo->iNumPs++;
      else if ( pSta[i].dPTime > 0. )              pHypo->iNumBadPs++;
//      else                                         pHypo->iNumBadPs++;

/* Compute Distance/Az from specified epicenter */
   GetEpiAzDelta( iNum, pSta, pHypo );

/* Figure which depth level of the IASPEI91 travel time tables to use */
   pHypo->dAvgRes = 0.0;
   fih = pHypo->dDepth/IASP_DEPTH_INC;  /* 0, 10km, 20km; depth inc. in table */
   if ( fih <= 1./IASP_DEPTH_INC )      /* Force depth to 1km */
   {
      fih = 1./IASP_DEPTH_INC;					
      pHypo->dDepth = 1.;
   }
   ih = (int) (fih + 1.0);
   fh = fih - floor( fih );
   if ( ih >= DEPTH_LEVELS_IASP - 1 )   /* max depth 750km */
   {
      ih = DEPTH_LEVELS_IASP - 1;
      pHypo->dDepth = 750.;
      fh = 0.0;
   }
/* Get expected P-times (and residuals) for each station */
   iCnt = 0;	
   for ( i=0; i<iNum; i++ )
      if ( pSta[i].dPTime > 0. )
      {	/* First get index to use in IASPEI91 tables based on delta */
        /* (table spacing is IASP_DIST_INC degree) */
         itab = (int) (pSta[i].dDelta*(1./IASP_DIST_INC));
         if ( pSta[i].dFracDelta - IASP_DIST_INC >= 0.0 ) 
            pSta[i].dFracDelta -= IASP_DIST_INC;
         pfIA = fPP + ih*IASP_NUM_PER_DEP + itab;	// Ptr to IASPEI91 table
/* Four point interplolation */
         ptt1 = *pfIA + (*(pfIA+1)-*pfIA)*pSta[i].dFracDelta/IASP_DIST_INC;
         ptt2 = *(pfIA-IASP_NUM_PER_DEP) + (*(pfIA-IASP_NUM_PER_DEP+1)-
                *(pfIA-IASP_NUM_PER_DEP))*pSta[i].dFracDelta/IASP_DIST_INC;
         ptt = ptt1*fh + ptt2*(1.-fh);/* Depth adjust (EARTHRAD to normalize) */
         pSta[i].dRes = pSta[i].dPTime - ((pHypo->dOriginTime) + ptt) -
                        pSta[i].dElevation/EARTHRAD;
         if ( pSta[i].iUseMe )			
         {
            pHypo->dAvgRes += fabs (pSta[i].dRes);
            iCnt++;
         }
      }
   pHypo->dAvgRes = pHypo->dAvgRes/(double) iCnt;
}

      /******************************************************************
       *                        GetEpiAzDelta()                         *
       *                                                                *
       * This function calculates distance, azimuth, and other important*
       * location parameters for all stations used in locating an       *
       * earthquake.  Spherical trigonometric relations are used        *
       * throughout.                                                    *
       *                                                                *
       *  Arguments:                                                    *
       *   iNum             Number of stations in STATION structure     *
       *   pSta             Array of STATION structures                 *
       *   pHypo            Computed hypocentral parameters             *
       *                                                                *
       ******************************************************************/
       
void GetEpiAzDelta( int iNum, STATION *pSta, HYPO *pHypo )
{
   double  dDeltaCosine, dDeltaSine;
   double  dTempDelta, dTempAz;
   int     i;
   LATLON  ll;

   GetLatLonTrig( (LATLON *) pHypo );
   if ( pHypo->dSinlat == 0.0 ) pHypo->dSinlat = 0.01;

   for ( i=0; i<iNum; i++ )
   {
      ll.dLat = pSta[i].dLat;
      ll.dLon = pSta[i].dLon;
      GeoCent( &ll );
      GetLatLonTrig( &ll );
      dDeltaCosine = pHypo->dCoslat*pSta[i].dCoslat + pHypo->dSinlat*
                     ll.dSinlat*(pHypo->dCoslon*ll.dCoslon +
                     pHypo->dSinlon*ll.dSinlon);
      dDeltaSine = sqrt( 1. - dDeltaCosine*dDeltaCosine );
      if ( dDeltaSine == 0.0 )   dDeltaSine = 0.01;
      if ( dDeltaCosine == 0.0 ) dDeltaCosine = 0.01;
      dTempDelta =  atan( dDeltaSine / dDeltaCosine );
      pSta[i].dCooze = (ll.dCoslat - pHypo->dCoslat * dDeltaCosine) /
                       (pHypo->dSinlat * dDeltaSine);
      if ( pSta[i].dCooze == 0.0 ) pSta[i].dCooze = 0.0001;
      pSta[i].dSnooze = ll.dSinlat * (pHypo->dCoslon * ll.dSinlon -
                        pHypo->dSinlon * ll.dCoslon) / dDeltaSine;
      dTempAz = atan( pSta[i].dSnooze / pSta[i].dCooze );
      while ( dTempAz < 0.0 )       dTempAz += PI;
      if ( pSta[i].dSnooze <= 0. )  dTempAz += PI;
      while ( dTempDelta < 0.0 )    dTempDelta += PI;
      dTempDelta *= DEG;
      pSta[i].dFracDelta = dTempDelta - floor( dTempDelta );
      pSta[i].dDelta     = dTempDelta;
      pSta[i].dAzimuth   = dTempAz * DEG;
   }
}
	   
      /******************************************************************
       *                       InitialLocator()                         *
       *                                                                *
       * This function computes an initial epicentral location for a    *
       * group of stations with P-Times.  If the function is to compute *
       * its own starting location, four stations are selected from the *
       * group and an epicenter is computed for each of the four        *
       * inscribed tripartites.  The average lat and lon is then given  *
       * as the initial location.   For each tripartite, the intial     *
       * location is computed first by computing the azimuth from the   *
       * center of the tripartite and then computing the distance away  *
       * from the center.  The azimuth is computed from the difference  *
       * in arrival times between the stations.  The distance is        *
       * obtained from the apparent velocity of the quake which relates *
       * to the angle of incidence of the wave to the tripartite.  The  *
       * angle of incidence then relates to the distance of the quake   *
       * from the tripartite.  If the user has specified a location,    *
       * this is converted to geocentric and then returned.  If the     *
       * nearest station is used as the starting point, that station's  *
       * location is then returned.  The computed value is loaded into  *
       * the HYPO array.                                                *
       *                                                                *
       *  Arguments:                                                    *
       *   iNum             Number of data in station structure         *
       *   iArea            Starting location for solution              *
       *                    0 = Compute a starting location             *
       *                    1 = Location is station with lowest P-time  *
       *                    2 = User has specified a starting location  *
       *                    3 = The computer will decide between 0 and 1*
       *   iTry             1st or 2nd try when iArea = 3               *
       *   pSta             STATION array of structures                 *
       *   pHypo            Computed hypocentral parameters             *
       *   dLatUser         Latitude specified by user (geographic)     *
       *   dLonUser         Longitude specified by user                 *
       *                                                                *
       ******************************************************************/
       
void InitialLocator( int iNum, int iArea, int iTry, STATION *pSta, HYPO *pHypo,
                     double dLatUser, double dLonUser )
{
   AZIDELT azidelt[MAX_STATION_DATA];  /* Dist. to fixed center pt. for all */
   AZIDELT azidelt01, azidelt02, azidelt12;/* Azidelts within tripartite */
   AZIDELT azideltAB, azideltAC;/* Azimuth and dist. of bisectors of AB, AC */
   AZIDELT azideltB, azideltC;  /* Azimuth and dist. from corner A to B and C */
   AZIDELT azideltTemp;	        /* Temporary structure of distance/azimuth */
   double  dAng[3];             /* Interior angles of chosen tripartite */
   double  dAngIncident;        /* Incident angle of p-wave into tripartite */
   double  dAngle[3];           /* Ordered interior angles of chosen trip. */
   double  dApVel;              /* Apparent velocity over triangle leg */
   double  dAzEpiAB, dAzEpiAC;  /* Angles between epicentral az. and side az. */
   double  dAz;                 /* Calculated azimuth toward the epicenter */
   double  dCorrB, dCorrC;      /* Interpolation corrections for max p-times */
   double  dDelta;              /* Distance to Epicenter from tripartite */
   double  dHyp;                /* Hypotenuse of rt. trngl created with dx,dy */
   double  dTdAB, dTdAC;        /* Time difference between corners A,B & A,C */
   double  dTdABm, dTdACm;      /* Max. time diff. between corners A,B & A,C */
   double  dTemp1, dTemp2, dTempLon;
   double  dx, dy;              /* Legs of trngl pointing direction to quake */
   int     i1, i2, i3, i, j, k, iCnt, l, n, ia, ib, ic; 
         /* This array lists the incident angle (*10) expected
            for each distance (the first entry is 0 degrees,
            the last is 100 degrees). NOTE: the incident angle is
            measured from the normal to the earth's surface. */
   int     iDelt[] = {520,506,506,502,502,492,496,487,487,483,476,469,
            460,451,447,442,434,425,395,365,346,328,322,316,310,305,
            300,295,291,287,287,287,282,277,275,273,271,270,268,267,
            265,263,261,260,258,256,254,253,249,246,242,239,237,236,
            233,229,226,222,220,219,215,212,209,206,204,202,201,199,
            196,193,189,186,184,182,181,179,176,173,171,170,166,163,
            161,160,157,154,152,150,148,147,145,144,142,141,141,141,
            141,141,142,144,142,141,139,138};
   static  int  iDLev;          /* Index of 0 deg. in desired depth level */
   int     iIndices[4] = {0, 0, 0, 0}; /* Index of 4 stas closest to center */
   LATLON  ll, ll1, ll2, ll3;
   LATLON  llAKArray;           /* Point used to compare station locations */
   LATLON  llCen1;              /* Center of tripartite */
   LATLON  llCen2;              /* Center of tripartite leg used for distance */
   LATLON  llEpi;               /* Lat/lon of epicenter estimation */
   
/* Initialize some things */
   pHypo->iNumPs    = 0;
   pHypo->iNumBadPs = 0;
   for ( i=0; i<iNum; i++ )    /* Find number of stations to use */
      if ( pSta[i].iUseMe > 0 && pSta[i].dPTime > 0. ) pHypo->iNumPs++;
//      else                                             pHypo->iNumBadPs++;
      else if ( pSta[i].dPTime > 0. )                  pHypo->iNumBadPs++;
   dTempLon = PI / 2.;
   pHypo->dLat = 0.;
   pHypo->dLon = 0.;
   llAKArray.dLat = 0.47;        /* Near center of south-central Alaska array */
   llAKArray.dLon = 3.65;

/* Get depth level to use in travel time table */
   iDLev = (int) ((double) (DEPTHKM+0.01) / IASP_DEPTH_INC)*IASP_NUM_PER_DEP;
	    
/* If user is to specify location, take what was specified and convert */
   if ( iArea == 2 )  
   {
      pHypo->dLat = dLatUser;
      pHypo->dLon = dLonUser;
      GeoCent ((LATLON *) pHypo);
   }
   
/* Set initial location to station with lowest arrival time in certain cases */
   else if ( iArea == 1 || (iArea == 3 && iTry == 1) )
   {
      dTemp1 = 1.e20;
      l = 0;
      for ( i=0; i<iNum; i++ )
         if ( pSta[i].dPTime > 0. && pSta[i].iUseMe > 0 &&
              pSta[i].dPTime < dTemp1 )
         {
            dTemp1 = pSta[i].dPTime;
            l = i;
         }
      ll.dLat = pSta[l].dLat;
      ll.dLon = pSta[l].dLon;
      GeoCent( &ll );
      GetLatLonTrig( &ll );
      pHypo->dLat = ll.dLat + 0.02;
      pHypo->dLon = ll.dLon + 0.02;
   }
   
/* Compute initial location */   
   else if ( iArea == 0 || (iArea == 3 && iTry > 1) )
   {
/* Get distance from all stations to defined center point (this could be 
   improved by more intelligently pick the quadrapartite; such as by angles
   and distance between the stations !!!) */
      for ( i=0; i<iNum; i++ )
      {
         ll.dLat = pSta[i].dLat;
         ll.dLon = pSta[i].dLon;
         GeoCent( &ll );
         GetLatLonTrig( &ll );
         GetDistanceAz( &ll, &llAKArray, &azidelt[i] );
      }
      
/*  Choose four stations closest to defined center point */
      k = 0;
      for ( i=0; i<iNum; i++ )
         if ( pSta[i].iUseMe > 0 && pSta[i].dPTime > 0. )
         {
            iCnt = 0;
            for ( j=0; j<iNum; j++ )
               if ( azidelt[i].dDelta < azidelt[j].dDelta &&
                    pSta[j].iUseMe > 0 && pSta[j].dPTime > 0. ) iCnt++;
            if ( iCnt >= (pHypo->iNumPs-4) )
            {
               iIndices[k] = i;
               k++;
            }
         }
      if ( k < 3 )                 /* Not enough stations to do anything with */
      {
         dTemp1 = 1.e20;
         l = 0;
         for ( i=0; i<iNum; i++ )
            if ( pSta[i].dPTime > 0. && pSta[i].iUseMe > 0 &&
                 pSta[i].dPTime < dTemp1 )
            {
               dTemp1 = pSta[i].dPTime;
               l = i;
            }
         ll.dLat = pSta[l].dLat;
         ll.dLon = pSta[l].dLon;
         GeoCent( &ll );
         GetLatLonTrig( &ll );
         pHypo->dLat = ll.dLat + 0.02; /* Use nearest P-time */
         pHypo->dLon = ll.dLon + 0.02;
         return;
      }
   
/* Set up 4 station tripartite loop */
      for ( i3=0; i3<4; i3++ )
      {
         j = iIndices[i3];
         i1 = i3 + 1;
         if ( i3 == 3 ) i1 = 0;
         k = iIndices[i1];
         i2 = i3 + 2;
         if ( i3 == 2 )      i2 = 0;
         else if ( i3 == 3 ) i2 = 1;
         l = iIndices[i2];
/* Compute azimuths and distances between stations */
         ll1.dLat = pSta[j].dLat;
         ll1.dLon = pSta[j].dLon;
         GeoCent( &ll1 );
         GetLatLonTrig( &ll1 );
         ll2.dLat = pSta[k].dLat;
         ll2.dLon = pSta[k].dLon;
         GeoCent( &ll2 );
         GetLatLonTrig( &ll2 );
         ll3.dLat = pSta[l].dLat;
         ll3.dLon = pSta[l].dLon;
         GeoCent( &ll3 );
         GetLatLonTrig( &ll3 );
         GetDistanceAz( &ll1, &ll2, &azidelt01 );
         GetDistanceAz( &ll1, &ll3, &azidelt02 );
         GetDistanceAz( &ll2, &ll3, &azidelt12 );
/* Compute interior angles of tripartite (use half-angle formulas from
   spherical trig - !!! This is changed from original; was planar) */
         dTemp1 = 0.5*(azidelt01.dDelta + azidelt02.dDelta + azidelt12.dDelta);
         dTemp2 = sqrt( (sin ((dTemp1 - azidelt01.dDelta)*RAD) *
                         sin ((dTemp1 - azidelt02.dDelta)*RAD) *
                         sin ((dTemp1 - azidelt12.dDelta)*RAD)) / 
                         sin (dTemp1*RAD) );
         dAng[0] = 2.*atan( dTemp2/sin( (dTemp1-azidelt01.dDelta)*RAD ) )*DEG;
         dAng[1] = 2.*atan( dTemp2/sin( (dTemp1-azidelt02.dDelta)*RAD ) )*DEG;
         dAng[2] = 2.*atan( dTemp2/sin( (dTemp1-azidelt12.dDelta)*RAD ) )*DEG;
/* Compute geocentric center of tripartite */
         llCen1.dLat = (min( ll1.dLat, min (ll2.dLat, ll3.dLat) ) + 
                        max( ll1.dLat, max (ll2.dLat, ll3.dLat) )) / 2.0;
         llCen1.dLon = (min( ll1.dLon, min (ll2.dLon, ll3.dLon) ) + 
                        max( ll1.dLon, max (ll2.dLon, ll3.dLon) )) / 2.0;
/* Rename corners A,B,C (A closest to 60 deg, B so azimuth AB < az. AC */
         for ( i=0; i<3; i++ ) dAngle[i] = fabs( dAng[i] - 60.0 );
         dTemp1 = 10000;
         for ( i=0; i<3; i++ )
            if ( dAngle[i] < dTemp1 )     /* Get closest to 60 degrees (ia) */
            {
               dTemp1 = dAngle[i];
               if ( i == 0 ) { ia=j; ib=k; ic=l; }
               if ( i == 1 ) { ia=k; ib=l; ic=j; }
               if ( i == 2 ) { ia=l; ib=j; ic=k; }
            }
         ll1.dLat = pSta[ia].dLat;
         ll1.dLon = pSta[ia].dLon;
         GeoCent( &ll1 );
         GetLatLonTrig( &ll1 );
         ll2.dLat = pSta[ib].dLat;
         ll2.dLon = pSta[ib].dLon;
         GeoCent( &ll2 );
         GetLatLonTrig( &ll2 );
         ll3.dLat = pSta[ic].dLat;
         ll3.dLon = pSta[ic].dLon;
         GeoCent( &ll3 );
         GetLatLonTrig( &ll3 );	    
         GetDistanceAz( &ll1, &ll2, &azideltB );
         GetDistanceAz( &ll1, &ll3, &azideltC );
/* Reverse the indices if necessary */
         if (azideltC.dAzimuth <= azideltB.dAzimuth)
         {
            i = ib;
            ib = ic;
            ic = i;                    /* Recompute with new indices */
            ll1.dLat = pSta[ia].dLat;
            ll1.dLon = pSta[ia].dLon;
            GeoCent( &ll1 );
            GetLatLonTrig( &ll1 );
            ll2.dLat = pSta[ib].dLat;
            ll2.dLon = pSta[ib].dLon;
            GeoCent( &ll2 );
            GetLatLonTrig( &ll2 );
            ll3.dLat = pSta[ic].dLat;
            ll3.dLon = pSta[ic].dLon;
            GeoCent( &ll3 );
            GetLatLonTrig( &ll3 );	 	    
            GetDistanceAz( &ll1, &ll2, &azideltB);
            GetDistanceAz( &ll1, &ll3, &azideltC);
         }
/* Compute azimuth of bisectors */
         if ( (azideltC.dAzimuth - azideltB.dAzimuth) > 180. )
         {                             
            azideltAB.dAzimuth = azideltB.dAzimuth + 90.;
            azideltAC.dAzimuth = azideltC.dAzimuth - 90.;
         }
         else
         {
            azideltAB.dAzimuth = azideltB.dAzimuth - 90.;
            azideltAC.dAzimuth = azideltC.dAzimuth + 90.;
         }
         if ( azideltAB.dAzimuth < 0.0 ) azideltAB.dAzimuth += 360.;
         if ( azideltAC.dAzimuth < 0.0 ) azideltAC.dAzimuth += 360.;
/* Compute time difference between stations */
         dTdAB = pSta[ib].dPTime - pSta[ia].dPTime;
         dTdAC = pSta[ic].dPTime - pSta[ia].dPTime;
         if ( dTdAB == 0.0 ) dTdAB = 0.01;        /* Prevent divide by zero */
         if ( dTdAC == 0.0 ) dTdAC = 0.01;
/* Maximum time difference between stations from iaspei91 tables */
         i = (int) ((azideltB.dDelta)*(1./IASP_DIST_INC));
         dCorrB = azideltB.dDelta*(1./IASP_DIST_INC) - 
                  floor( azideltB.dDelta*(1./IASP_DIST_INC) );
         n = (int) ((azideltC.dDelta)*(1./IASP_DIST_INC)); 
         dCorrC = azideltC.dDelta*(1./IASP_DIST_INC) - 
                  floor( azideltC.dDelta*(1./IASP_DIST_INC) );
/* Use DEPTHKM in table */
         dTdABm = fPP[i+iDLev] + dCorrB*(fPP[(i+1)+iDLev] - fPP[i+iDLev]);
         dTdACm = fPP[n+iDLev] + dCorrC*(fPP[(n+1)+iDLev] - fPP[n+iDLev]);
/* Compute azimuth from center of tripartite to epicenter (trig method) */
         if ( dTdAB/dTdABm < 0.0 ) azideltAC.dAzimuth += 180.;
         if ( dTdAC/dTdACm < 0.0 ) azideltAB.dAzimuth += 180.;
         if ( azideltAB.dAzimuth >= 360. ) azideltAB.dAzimuth -= 360.;
         if ( azideltAC.dAzimuth >= 360. ) azideltAC.dAzimuth -= 360.;
         dx = fabs( dTdAB/dTdABm ) * sin( azideltAC.dAzimuth * RAD ) +
              fabs( dTdAC/dTdACm ) * sin( azideltAB.dAzimuth * RAD );
         dy = fabs( dTdAB/dTdABm ) * cos( azideltAC.dAzimuth * RAD ) +
              fabs( dTdAC/dTdACm ) * cos( azideltAB.dAzimuth * RAD );
         dHyp = sqrt( dx*dx + dy*dy );
         if ( dx > 0.0 )
         {
            if ( dy > 0.0 ) dAz = acos( dy/dHyp ) * DEG;
            else            dAz = acos( dx/dHyp ) * DEG + 90.;
         }
         else
         {
            if ( dy <= 0.0 ) dAz = acos( fabs( dy )/dHyp) * DEG + 180.;
            else             dAz = asin( dy/dHyp ) * DEG + 270.;
         }
/* Compute distance from center of AB and AC to epicenter. Assume surface
   velocity of 6 km/sec. */
         dAzEpiAB = fabs( dAz - azideltB.dAzimuth );
         dAzEpiAC = fabs( dAz - azideltC.dAzimuth );
         if ( dAzEpiAB > 180. ) dAzEpiAB -= 180.;
         if ( dAzEpiAC > 180. ) dAzEpiAC -= 180.;
/* Use the leg which is closest to the azimuth on which the epicenter lies.
   This will give more accurate results than looking at a leg which is
   perpendicular to the azimuth. */
         if ( fabs( cos( dAzEpiAB*RAD ) ) >= fabs( cos( dAzEpiAC*RAD ) ) )
         {
            dApVel = azideltB.dDelta*111./fabs( dTdAB );
            dTemp2 = fabs( cos( dAzEpiAB*RAD ) );
            if ( dTemp2 == 0.0 ) dTemp2 = 0.05; /* Prevent divide by 0 */
            dTemp1 = 6. / (dApVel*dTemp2);
            if ( dTemp1 >= 1.0 ) dTemp1 = 0.99; /* Prevent asin error */
            dAngIncident = asin( dTemp1 ) * DEG;
            ll1.dLat = pSta[ia].dLat;
            ll1.dLon = pSta[ia].dLon;
            GeoCent( &ll1 );
            GetLatLonTrig( &ll1 );
            ll2.dLat = pSta[ib].dLat;
            ll2.dLon = pSta[ib].dLon;
            GeoCent( &ll2 );
            GetLatLonTrig( &ll2 );
            llCen2.dLat = (ll1.dLat+ll2.dLat) / 2.0;
            llCen2.dLon = (ll1.dLon+ll2.dLon) / 2.0;
         }		 
         else
         {
            dApVel = azideltC.dDelta*111./fabs( dTdAC );
            dTemp2 = fabs( cos( dAzEpiAC*RAD ) );
            if ( dTemp2 == 0.0 ) dTemp2 = 0.05; /* Prevent divide by 0 */
            dTemp1 = 6. / (dApVel*dTemp2);
            if ( dTemp1 >= 1.0 ) dTemp1 = 0.99; /* Prevent asin error */
            dAngIncident = asin( dTemp1 ) * DEG;
            ll1.dLat = pSta[ia].dLat;
            ll1.dLon = pSta[ia].dLon;
            GeoCent( &ll1 );
            GetLatLonTrig( &ll1 );
            ll2.dLat = pSta[ib].dLat;
            ll2.dLon = pSta[ib].dLon;
            GeoCent( &ll2 );
            GetLatLonTrig( &ll2 );
            llCen2.dLat = (ll1.dLat+ll2.dLat) / 2.0;
            llCen2.dLon = (ll1.dLon+ll2.dLon) / 2.0;
         }
/* Pull the proper distance from the incident angle vs. distance table */
         for ( i=0; i<100; i++ )
            if ( (iDelt[i]*.1) < dAngIncident )
            {
               dDelta = (double) (i+1);
               break;
            }
/* Get distance and azimuth from center of tripartite leg to center of array
   and adjust computed distance */
         if ( llCen2.dLat != llCen1.dLat || llCen1.dLon != llCen2.dLon )
         {
            GetDistanceAz( &llCen1, &llCen2, &azideltTemp );
            if ( dDelta >= azideltTemp.dDelta )
               dDelta += azideltTemp.dDelta * cos( fabs(
	                 dAz-azideltTemp.dAzimuth ) * RAD );
         }
/* Compute geocentric lat/lon of inital guess */
         azideltTemp.dDelta = dDelta * RAD;
         azideltTemp.dAzimuth = dAz * RAD;
         PointToEpi( &llCen1, &azideltTemp, &llEpi );
         if ( pHypo->iNumPs > 3 )  /* Avg. loc over 4 tries */
         {                         /* Take 1/4 of each try and add 'em up */
            pHypo->dLat += llEpi.dLat * 0.25;
            if ( dTempLon < 0.8 && llEpi.dLon > 5.2 )/* Adj. near 0 deg. lon. */
               llEpi.dLon -= TWOPI; 
            else if ( dTempLon > 5.2 && llEpi.dLon < 0.8 ) 
               llEpi.dLon += TWOPI;
            pHypo->dLon += .25 * llEpi.dLon;
            dTempLon = pHypo->dLon * 4.0 / (double) (i3+1);
         }
         else                      /* This is the starting location */
         {
            pHypo->dLat = llEpi.dLat;
            pHypo->dLon = llEpi.dLon;
            break;                 /* Get out of loop and return location */
         }
      }
      while ( pHypo->dLon >= TWOPI ) pHypo->dLon -= TWOPI; /* Just in case */
      while ( pHypo->dLon < 0.0 )    pHypo->dLon += TWOPI;
      while ( pHypo->dLat >= PI )    pHypo->dLat -= PI;		
      while ( pHypo->dLon < 0.0 )    pHypo->dLon += PI;
   }
   else                      /* Should never happen */
   {
      pHypo->dLat = 63.0;    /* Use a spot in south-central Alaska */
      pHypo->dLon = -152.0;
      GeoCent( (LATLON *) pHypo );
   }
   GetLatLonTrig( (LATLON *) pHypo );
}

      /******************************************************************
       *                         IsItGoodSoln()                         *
       *                                                                *
       * This function determines (by the residuals) whether or not a   *
       * solution has converged or not.  It fills a variable in the     *
       * HYPO structure (iGoodSoln). The possible values of             *
       * iGoodSoln are: 0 - poor fit; 1 - ok fit; 2 - good fit; 3 -     *
       * very good fit.  The factors which determine the fit quality are*       
       * arbitrarily chosen.                                            *
       *                                                                *
       *  November, 2013: Modify level 2 parameters to sometimes allow  *
       *                   locs with < MinPs                             *
       *  August, 2011: Modify level 2 parameters to eliminate bad locs.*
       *  June, 2008: Compare nearest stn with freq.  If nearest is     *
       *              distant and high freq., probably not good.        *
       *  August, 2007: Not a good solution unless there is a near stn. *
       *  December, 2004: Added distance/azimuth control checks         *
       *                                                                *
       *  Arguments:                                                    *
       *   iNum             Number of Stations in the structure         *
       *   pSta             Station structure                           *
       *   pHypo            Computed hypocentral parameters             *
       *   iMinPs           Minimum # of Ps allowed for location        *
       *                                                                *
       ******************************************************************/
       
void IsItGoodSoln( int iNum, STATION *pSta, HYPO *pHypo, int iMinPs )
{
   double  dMin;
   int     i;
   int     iAnyOut;	    /* Flag indicating if any stations res. > 3 */
   int     iAnyWayOut;      /* Flag indicating if any stations res. > 6 */
   int     iIndex=0;          /* P array index of closest station */
   int     iOddLooking;     /* 1 if quake has odd dist/freq chars.; 0 if ok */
   int     iPUsed;          /* Number of Ps used in sol'n */  

/* See how many Ps */
   pHypo->iNumPs    = 0;
   pHypo->iNumBadPs = 0;
   for ( i=0; i<iNum; i++ )     /* Find number of stations to use */
      if ( pSta[i].iUseMe > 0 && pSta[i].dPTime > 0. ) pHypo->iNumPs++;
      else if ( pSta[i].dPTime > 0. )                  pHypo->iNumBadPs++;

/* Check each residual individually */
   iAnyOut    = 0;
   iAnyWayOut = 0;	
   iPUsed = 0;
   for ( i=0; i<iNum; i++ )
      if ( pSta[i].iUseMe > 0 && pSta[i].dPTime > 0. )
      {
         iPUsed++;
         if ( fabs( pSta[i].dRes ) > 10. ) iAnyWayOut++;
         if ( fabs( pSta[i].dRes ) > 5. ) iAnyOut++;
      }
/* Next, check the frequency of the nearest station */      
   dMin = 360.;
   iOddLooking = 0;
   for ( i=0; i<iNum; i++ )
      if ( pSta[i].iUseMe > 0 && pSta[i].dPTime > 0. )
         if ( pSta[i].dDelta < dMin )
         {
            dMin = pSta[i].dDelta;
            iIndex = i;
         }
   if ( pSta[iIndex].dFreq > FREQ_MIN && pSta[iIndex].dDelta > DELTA_TELE )
      iOddLooking = 1;
	 
/* Check the residuals, # stations, nearest distance, azimuthal
   control, and define a quality */
   if ( (iPUsed > iMinPs && pHypo->dAvgRes < 1.5 && pHypo->iAzm > 90 &&
         iPUsed/10 >= iAnyOut && pHypo->dNearestDist < DELTA_TELE && 
        (iOddLooking == 0 || (iOddLooking == 1 && iNum > 15)) &&         
        (pHypo->dNearestDist < 10. || pHypo->iAzm > 180)) ) 
      pHypo->iGoodSoln = 3;			/* Very good fit */
   else if ( (iPUsed >= iMinPs+2 && pHypo->dAvgRes < 2.5 &&
              iPUsed/10 >= iAnyWayOut && pHypo->dNearestDist < DELTA_TELE  &&
             (iOddLooking == 0  || (iOddLooking == 1 && iNum >= 10)) &&
              pHypo->iAzm > 30) ||
/* This section added in 5/14 */
            ((iPUsed >= iMinPs     && iPUsed < iMinPs+2) && 
              pHypo->dAvgRes < 2.5 && iAnyWayOut == 0    && 
              pHypo->dNearestDist  <  NEAR_STN_CUTOFF    &&
              iOddLooking == 0     && pHypo->iAzm > 30)  ||
/* This section added in 11/13 (modified 5/14) */
             (iPUsed == iMinPs-1 && pHypo->dAvgRes < 1.5 &&
              pHypo->dNearestDist < NEAR_STN_CUTOFF && iOddLooking == 0 &&
              pHypo->iAzm > 45) ) 
      pHypo->iGoodSoln = 2;			/* Good fit */
   else if ( pHypo->dAvgRes < 4. ) 
      pHypo->iGoodSoln = 1;			/* So-so fit */
   else
      pHypo->iGoodSoln = 0;			/* Poor fit */
}

      /******************************************************************
       *                         IsItSameQuake()                        *
       *                                                                *
       * This function compares two hypocenters and determines if they  *
       * are likely the same quake.                                     *
       *                                                                *
       * February, 2010: Included magnitude in comparison.              *
       *                                                                *
       *  Arguments:                                                    *
       *   pHypo1           One of the hypocenters                      *
       *   pHypo2           The other hypocenter                        *
       *                                                                *
       *  Returns:          1 if a match; 0 if not                      *
       *                                                                *
       ******************************************************************/
       
int IsItSameQuake( HYPO *pHypo1, HYPO *pHypo2 )
{
   AZIDELT azidelt;         /* Distance/azimuth between two hypocenters */

/* Get distance between two hypocenters in degrees */
   GetDistanceAz( (LATLON *) pHypo1, (LATLON *) pHypo2, &azidelt );
                       
/* Check origin time and location match */
   if ( fabs( pHypo1->dOriginTime-pHypo2->dOriginTime )     < 60. &&
        fabs( pHypo1->dPreferredMag-pHypo2->dPreferredMag ) < 1.5 &&
        azidelt.dDelta                                      < 5. ) return 1;
   return 0;
}

      /******************************************************************
       *                    IsItSameQuakeTight()                        *
       *                                                                *
       * This function compares two hypocenters and determines if they  *
       * are likely the same quake. This function uses much tighter     *
       * constraints than IsItWameQuake.                                *
       *                                                                *
       * February, 2010: Included magnitude in comparison.              *
       *                                                                *
       *  Arguments:                                                    *
       *   pHypo1           One of the hypocenters                      *
       *   pHypo2           The other hypocenter                        *
       *                                                                *
       *  Returns:          1 if a match; 0 if not                      *
       *                                                                *
       ******************************************************************/
       
int IsItSameQuakeTight( HYPO *pHypo1, HYPO *pHypo2 )
{
   AZIDELT azidelt;         /* Distance/azimuth between two hypocenters */

/* Get distance between two hypocenters in degrees */
   GetDistanceAz( (LATLON *) pHypo1, (LATLON *) pHypo2, &azidelt );
                       
/* Check origin time and location match */
   if ( fabs( pHypo1->dOriginTime-pHypo2->dOriginTime )     < 10. &&
        fabs( pHypo1->dPreferredMag-pHypo2->dPreferredMag ) < 0.2 &&
        azidelt.dDelta                                      < 1. ) return 1;
   return 0;
}

      /*********************************************************
       *                LoadEqDataNew()                        *
       *                                                       *
       * Loads average and max depth information into an       *
       * array (2d) with indices tied to lat/lon.              *
       *                                                       *
       * The array is ordered by iDepthAvg[co-latitude (0-180  *
       * from north to south)][east longitude (0-360)].  Two   *
       * variables are loaded: iDepthAvg and iDepthMax from the*
       * data file.                                            *
       *                                                       *
       * Arguments:                                            *
       *  pszFile        Input file with depth data            *
       *  iDepthAvg      Array of average depths (km) for that *
       *                 lat/lon                               *
       *  iDepthMax      Array of maximum depths (km) for that *
       *                 lat/lon                               *
       *                                                       *
       * Return:                                               *
       *  int            -1 if problem; 1 if ok                *
       *                                                       *
       *********************************************************/
       
int LoadEQDataNew( char *pszFile, int iDepthAvg[][360], int iDepthMax[][360] )
{
   FILE *hFile;         /* File handle */
   int   i, j;          /* Counters */
   int   iAveDepth;     /* Average depth for that l/l of all earthquakes */
   int   iLat;          /* Latitude read in from file (+/-) */
   int   iLon;          /* Longitude read in from file (+/-) */
   int   iMaxDepth;     /* Maximum depth for that l/l of all earthquakes */

/* Populate arrays with default values */
   for ( i=0; i<180; i++ )
      for ( j=0; j<360; j++ )
      {
         iDepthAvg[i][j] = DEFAULT_DEPTH;
         iDepthMax[i][j] = DEFAULT_MAXDEPTH;
      }   

/* Open the depth data file. */
   if ( (hFile = fopen( pszFile, "r" )) == NULL )
   {
      logit( "", "Error opening depth data file %s.\n", pszFile );
      return( -1 );
   }
/* Read entire file and fill array elements that are specified */
   while ( !feof( hFile ) )  
   {
      fscanf( hFile, "%d %d %d %d",  &iLat, &iLon, &iAveDepth, &iMaxDepth );
/* Convert from latitude to co-latitude and longitude to east longitude */
      if ( iLat >= 0   ) iLat =  90 - iLat;
      else if ( iLat <  0 ) iLat =  abs( iLat ) + 90;
      if ( iLat <  0   ) iLat =  0;
      if ( iLat >= 180 ) iLat =  179;
      if ( iLon <  0   ) iLon += 360; 
      if ( iLon <  0   ) iLon =  0; 
      if ( iLon >= 360 ) iLon =  359;
      iDepthAvg[iLat][iLon] = iAveDepth;
      iDepthMax[iLat][iLon] = iMaxDepth;
   }
   fclose( hFile );
   return( 1 );
} 

      /******************************************************************
       *                       QuakeAzimuthSort()                       *
       *                                                                *
       * This function sorts the quake azimuthal values.  This makes it *
       * easier to compute the azimuthal coverage of stations about the *
       * epicenter.                                                     *
       *                                                                *
       *  Arguments:                                                    *
       *   iNum             Number of stations in STATION structure     *
       *   pSta             Array of STATION structures                 *
       *                                                                *
       *  Return:                                                       *
       *   int - azimuthal coverage about epicenter                     *
       *                                                                *
       ******************************************************************/
       
int QuakeAzimuthSort( int iNum, STATION *pSta )
{
   double  dAz[MAX_STATION_DATA];  /* Azimuths of stations used in soln */
   double  dAzGap;
   double  dTemp, dTempAz;
   int     i, ii, j;
   int     iGoodPicks;             /* Number of stations used in loc. */

/* Put good azimuths into array */
   ii = 0;
   for ( i=0; i<iNum; i++ )
      if ( pSta[i].iUseMe > 0 && pSta[i].dPTime > 0. )
      {
         dAz[ii] = pSta[i].dAzimuth;
         ii++;
      }
   iGoodPicks = ii;

/* Sort the array */
   for ( i=0; i<iGoodPicks-1; i++ )
   {
      ii = i+1;
      for ( j=ii; j<iGoodPicks; j++ )
         if ( dAz[i] > dAz[j] )
         {
            dTemp = dAz[i];
            dAz[i] = dAz[j];
            dAz[j] = dTemp;
         }
   }
   
/* Compute coverage from sorted array */
   dAzGap = 0.0;
   ii = 0;
   for ( i=0; i<iGoodPicks-1; i++ )
   {
      dTempAz = dAz[i+1] - dAz[i];
      if ( dTempAz > dAzGap ) dAzGap = dTempAz;
   }
   
/* Return the coverage */
   dTempAz = 360. - (dAz[iGoodPicks-1] - dAz[0]);
   if (dTempAz > dAzGap) return( (int) (360.0 - dTempAz) );
   else                  return( (int) (360.0 - dAzGap) );
}

      /******************************************************************
       *                           QuakeDeta()                          *
       *                                                                *
       * Solve matrices.                                                *
       *                                                                *
       *  Arguments:                                                    *
       *   iDNum            Matrix order (3 fixed depth, 4 for float)   *
       *   dADet            Matrices                                    * 
       *                                                                *
       ******************************************************************/
       
double QuakeDeta( int iDNum, double dAdet[][4] )
{
   double  dProd, dSave[4];
   int     k, kk, i1, k1, k2, i, j, iCnt;

   dProd = 1.0;
   for ( k=0; k<(iDNum-1); k++ )
   {
      iCnt = 0;
      kk = k+1;
      while ( dAdet[k][k] == 0.0 )
      {
         if ( iCnt-(iDNum-1)+k+1 > 0 ) return 0.0;
         else
         {
            for ( i1=k; i1<iDNum; i1++ ) dSave[i1] = dAdet[i1][k];
            for ( k1=k; k1<iDNum; k1++ )
               for ( k2=k; k2<(iDNum-1); k2++ ) 
                  dAdet[k1][k2] = dAdet[k1][k2+1];
            for ( i1=k; i1<iDNum; i1++ ) dAdet[i1][k] = dSave[i1];
            iCnt++;
            dProd *= ((((iDNum-1)-k) % 2) ? -1.0 : 1.0);
         }
      }
      for ( i=kk; i<iDNum; i++ )
         for ( j=kk; j<iDNum; j++ )
            dAdet[j][i] -= dAdet[j][k] * dAdet[k][i] / dAdet[k][k];
      dProd *= dAdet[k][k];
   }
   dProd *= dAdet[iDNum-1][iDNum-1];
   return dProd;
}

      /******************************************************************
       *                           QuakeDets()                          *
       *                                                                *
       * This function uses determinates to get variable residuals.     *
       *                                                                *
       *  Arguments:                                                    *
       *   iDNum            Matrix order (3 fixed depth, 4 for float)   *
       *   dA, B, XDet      Matrices                                    * 
       *                                                                *
       ******************************************************************/
       
void QuakeDets( int iDNum, double dAdet[][4], double dBdet[], double dXdet[] )
{
   double  dA1det[4][4], dDenom, dAnum;
   int     i, j, k;

   for ( i=0; i<iDNum; i++ )
      for ( j=0; j<iDNum; j++ ) dA1det[i][j] = dAdet[i][j];

   dDenom = QuakeDeta( iDNum, dAdet );
   if ( dDenom < 1.E-13 )
   {
      dXdet[0] = 111.;
      return;
   }
   for ( k=0; k<iDNum; k++ )
   {
      for ( i=0; i<iDNum; i++ )
         for ( j=0; j<iDNum ; j++ ) dAdet[i][j] = dA1det[i][j];
      for ( i=0; i<iDNum; i++ ) dAdet[i][k] = dBdet[i];
      dAnum = QuakeDeta( iDNum, dAdet );
      dXdet[k] = dAnum / dDenom;
   }
}

      /******************************************************************
       *                       QuakeSolveIasp()                         *
       *                                                                *
       * This function determines the hypocenter of an event by         *
       * minimizing the set of time residuals of the observing stations.*
       * Up to QUAKE_ITER iterations are used to adjust the hypocenter  *
       * parameters.  On a first location attempt, the origin time is   *
       * calculated by subtracting the travel time from the earliest    *
       * station's arrival time and the depth is fixed.  The initial    *
       * location is taken from the HYPO array (this should have been   *
       * filled in InitialLocator). On subsequent iteration attempts,   *
       * the user may let the depth vary.  Depth is restricted to 750   *
       * km.  The travel times are taken from the IASPEI91 travel time  *
       * tables.                                                        *
       *                                                                *
       * Sept., 2013: Optimized FindDepth for speed                     *
       * March, 2006: Added call to find depth so that depth is fixed at*
       *              average for location when fixed is specified, and *
       *              that max is set by location when floated.         *
       *                                                                *
       *  Arguments:                                                    *
       *   iNum             Number of stns in STATION structure         *
       *   pSta             Array of STATION structures                 *
       *   pHypo            Computed hypocentral parameters             *
       *   iDepthAvg        Average depths (km) for all lat/lons        *
       *   iDepthMax        Maximum depths (km) for all lat/lons        *
       *   iFindDepth	    1->look up depth data, 0->don't             *
       *                                                                *
       ******************************************************************/
       
void QuakeSolveIasp( int iNum, STATION *pSta, HYPO *pHypo, int iDepthAvg[][360],
                     int iDepthMax[][360], int iFindDepth )
{
   static double  dAdet[4][4];  /* Matrices, etc., used to solve location */
   static double  dBdet[4];
   static double  dCoef[4*COEFFSIZE+MAX_STATION_DATA+10];
   double  dMin;
   static double  dXdet[4];
   double  dPTimeMin, dDeltaMin;/* Var. used on 1st pass thru for O-time */
   double  fih, fh, dadd, dadh, ptt1, ptt2, ptt; /* Interpolation values */
   int     i, j, k, l, mm, ih, itab; // , iIndex;     /* Counters, indices    */
   int     iAvg, iMax;          /* Average and Max depth returned from FindD */
   int     iIterCnt;            /* Iteration counter */
   static int iMaxDepthLevel;   /* Maximum expected IASPEI depth level */
   int     iPCnt;               /* # of P's used in this solution so far */
   LATLON  LLIn, LLOut;	        /* LATLON structures for conversion to
                                   geographic coords. */
   float   *pfIA;               /* Pointer to IASPEI91 travel time table */

   iIterCnt = 0;
   for ( i=0; i<QUAKE_ITER; i++ )
   {                     
      iPCnt = 0;
            
/* Compute distance, azimuth, etc. for all P stations from previous loc */
      for ( j=0; j<iNum; j++ )
         if ( pSta[j].dPTime > 0. )
            GetDistanceAz2( (LATLON *) pHypo, &pSta[j] );
      
/* First time through get origin time from first P-time and delta */
      if ( pHypo->dOriginTime == 0.0 )
      {     
         dMin = 1.e20;
         l = 0;
         for ( j=0; j<iNum; j++ )
            if ( pSta[j].dPTime > 0. && pSta[j].iUseMe > 0 &&
                 pSta[j].dPTime < dMin ) 
            {
               l = j;
               dMin = pSta[j].dPTime;
            }
	    
/* Use first pick included in sol'n */
         dPTimeMin = pSta[l].dPTime;                       /* l Figured above */
         dDeltaMin = pSta[l].dDelta;	          /* Filled in GetDistanceAz2 */
	 
/* I have no idea where these formulas come from (PW) */
         if ( dDeltaMin <= 20.0 )
            pHypo->dOriginTime = dPTimeMin - 3.67489 - 14.1561*dDeltaMin -
             0.0189237*dDeltaMin*dDeltaMin + 0.00267753*dDeltaMin*dDeltaMin*
             dDeltaMin;
         else
            pHypo->dOriginTime = dPTimeMin - 61.1089 - 11.4192*dDeltaMin +
             0.0410401*dDeltaMin*dDeltaMin - 0.0000301625*dDeltaMin*dDeltaMin*
             dDeltaMin;
      }
      pHypo->dAvgRes = 0.0;
    
/* Search the depth data base if desired */
      if (iFindDepth == 1)
      {
         iAvg = DEFAULT_DEPTH;
         iMax = DEFAULT_MAXDEPTH;
         LLIn.dLat = pHypo->dLat;
         LLIn.dLon = pHypo->dLon;
         GeoGraphic (&LLOut, &LLIn);
         if (LLOut.dLon > 180.) LLOut.dLon -= 360.;
         FindDepthNew( LLOut.dLat, LLOut.dLon, &iAvg, &iMax, iDepthAvg, 
                       iDepthMax );
         iMaxDepthLevel = (int) (iMax / (int) IASP_DEPTH_INC) + 5;
         if ( iMaxDepthLevel > DEPTH_LEVELS_IASP )
              iMaxDepthLevel = DEPTH_LEVELS_IASP;
         if (pHypo->iDepthControl == 3)                        /* Fixed depth */
            if ( iIterCnt == 0 || iIterCnt >= QUAKE_ITER-1 )
               pHypo->dDepth = (double) iAvg;             /* If 4 - it floats */
      }
      else
         iMaxDepthLevel = DEPTH_LEVELS_IASP;

      fih = pHypo->dDepth / IASP_DEPTH_INC;/* 0, 10km,... depth inc. in table */
      if ( fih <= 1./IASP_DEPTH_INC )                   /* Force depth to 1km */
      {
         fih = 1./IASP_DEPTH_INC;					
         pHypo->dDepth = 1.0;
      }
      ih = (int) (fih + 1.0);
      fh = fih - floor( fih );
      if ( ih >= iMaxDepthLevel - 1 )                  /* Max depth set above */
      {	  
         ih = iMaxDepthLevel - 1;
         pHypo->dDepth = (double) (iMaxDepthLevel*IASP_DEPTH_INC);
         fh = 0.0;
      }
      
/* Get expected P-times for each station */
      for ( j=0; j<iNum; j++ )
      {
         if ( pSta[j].dPTime > 0. )
         {
/* First get index to use in IASPEI91 tables based on delta (table spacing
   is IASP_DIST_INC degree) */
            itab = (int) (pSta[j].dDelta*(1./IASP_DIST_INC));
            while ( pSta[j].dFracDelta - IASP_DIST_INC >= 0.0 )  
                    pSta[j].dFracDelta -= IASP_DIST_INC;

            pfIA = fPP + ih*IASP_NUM_PER_DEP + itab; /* Ptr to IASPEI91 table */
            dadd = ((*(pfIA+1)-*pfIA)*fh + (*(pfIA-IASP_NUM_PER_DEP+1)-
                   *(pfIA-IASP_NUM_PER_DEP))*(1.-fh)) / IASP_DIST_INC;
            dadh = (*(pfIA-IASP_NUM_PER_DEP)-*pfIA) * 
                   (1.-pSta[j].dFracDelta/IASP_DIST_INC) + 
                   (*(pfIA-IASP_NUM_PER_DEP+1) - 
                   *(pfIA+1))*(pSta[j].dFracDelta/IASP_DIST_INC);
		   
/* Four point interplolation */
            ptt1 = *pfIA + (*(pfIA+1)-*pfIA)*pSta[j].dFracDelta/IASP_DIST_INC;
            ptt2 = *(pfIA-IASP_NUM_PER_DEP) + (*(pfIA-IASP_NUM_PER_DEP+1)-
                   *(pfIA-IASP_NUM_PER_DEP))*pSta[j].dFracDelta/IASP_DIST_INC;
            ptt = ptt1*fh + ptt2*(1.-fh);                 /* Depth adjustment */

            pSta[j].dRes = pSta[j].dPTime - (pHypo->dOriginTime + ptt) -
             pSta[j].dElevation/EARTHRAD;              /* Normalize Elevation */
            if ( pSta[j].iUseMe > 0 )                        /* Adjust matrix */
            {
               dCoef[COEFFSIZE+iPCnt] = dadd * pSta[j].dCooze;
               dCoef[2*COEFFSIZE+iPCnt] = dadd * pSta[j].dSnooze *
                                          pHypo->dSinlat * (-1.0);
               dCoef[3*COEFFSIZE+iPCnt] = -dadh * 0.1;                 /* ??? */
               dCoef[4*COEFFSIZE+iPCnt] = pSta[j].dRes;
               dCoef[iPCnt] = 1.0;
               pHypo->dAvgRes += fabs( pSta[j].dRes );
               if ( pHypo->iDepthControl == 3 )
                  dCoef[3*COEFFSIZE+iPCnt] = dCoef[4*COEFFSIZE+iPCnt];
               iPCnt++;
            } 
         }
         else
            pSta[j].dRes = 0.;
      }
	 
      if ( iIterCnt >= QUAKE_ITER-1 || pHypo->dAvgRes/(double) iPCnt < 0.2 )
         goto FunctionEnd;

      for ( j=0; j<pHypo->iDepthControl; j++ )
         for ( l=0; l<pHypo->iDepthControl; l++ )
         {
            dAdet[j][l] = 0.0;
            for ( k=0; k<iPCnt; k++ )
               dAdet[j][l] += dCoef[k+j*COEFFSIZE] * dCoef[k+l*COEFFSIZE];
         }
      mm = pHypo->iDepthControl;
      for ( j=0; j<pHypo->iDepthControl; j++ )
      {
         dBdet[j] = 0.0;
         for ( k=0; k<iPCnt; k++ )
            dBdet[j] += dCoef[k+j*COEFFSIZE] * dCoef[k+mm*COEFFSIZE];
      }
      
/* Solve matrix */
      QuakeDets( pHypo->iDepthControl, dAdet, dBdet, dXdet );
      if ( dXdet[0] == 111. ) goto FunctionEnd;
      if ( pHypo->iDepthControl > 3 ) pHypo->dDepth += dXdet[3];
      if ( pHypo->dDepth > 750. ) pHypo->dDepth = 750.;
      if ( pHypo->dDepth < 0.   ) pHypo->dDepth = 0.;
      pHypo->dOriginTime += dXdet[0];
      pHypo->dLat += dXdet[1]*RAD;
      pHypo->dLon += dXdet[2]*RAD;
      iIterCnt++;
   }
FunctionEnd:

/* Compute the average residual */
   pHypo->dAvgRes = pHypo->dAvgRes/(double) iPCnt;

/* Find azimuthal coverage around epicenter and dist to closest station */
   pHypo->iAzm = QuakeAzimuthSort( iNum, pSta );
   pHypo->dNearestDist = 200.;
   for ( j=0; j<iNum; j++ )
      if ( pSta[j].dPTime > 0. && pSta[j].iUseMe > 0 )
         if ( pSta[j].dDelta < pHypo->dNearestDist )
         {
            pHypo->dNearestDist = pSta[j].dDelta;
            pHypo->dFirstPTime = pSta[j].dPTime;
         }
	 
/* Adjust lat/lon to real earth values */
   if (pHypo->dLat < 0.)
   {
      pHypo->dLat = fabs( pHypo->dLat );
      pHypo->dLon -= PI;
   }
   while ( pHypo->dLat > TWOPI ) pHypo->dLat -= TWOPI;
   if ( pHypo->dLat > PI && pHypo->dLat <= TWOPI )
   {
      pHypo->dLat = TWOPI - pHypo->dLat;
      pHypo->dLon -= PI;
   }
   while ( pHypo->dLon > TWOPI ) pHypo->dLon -= TWOPI;
   while ( pHypo->dLon < 0.0 ) pHypo->dLon += TWOPI;
}

      /******************************************************************
       *                       Round()                                  *
       *                                                                *
       * Function Round will round double values to the nearest integer.*
       *                                                                *
       *  Arguments:                                                    *
       *   dInput        Input double value                             *
       *                                                                *
       *  Returns:                                                      *
       *   int           Rounded value                                  *
       *                                                                *
       ******************************************************************/
       
int Round( double dInput )
{
   int     iRound;
   double  dDec;
    
   if ( dInput >= 0. )
   {
      if ( modf( dInput, &dDec ) >= 0.5 )
      {
         iRound = (int) ceil( dInput );
      }
      else
         iRound = (int) floor( dInput );
   }
   else
      if ( modf( dInput, &dDec ) <= -0.5 )
      {
         iRound = (int) floor( dInput );
      }
      else
         iRound = (int) ceil( dInput );
	 
   return (iRound);	
}
