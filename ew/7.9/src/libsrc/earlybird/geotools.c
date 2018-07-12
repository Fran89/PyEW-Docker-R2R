 /************************************************************************
  * GEOTOOLS.C                                                           *
  *                                                                      *
  * This is a group of functions which provide tools for converting      *
  * geocentric lat/lons to geographic and vice-versa and for performing  *
  * distance/azimuth computations on a sphere (the earth).               *
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
#ifdef _WINNT                                                  
 #include <crtdbg.h>
#endif

#include "earlybirdlib.h"
#include "iasplib.h" 

#ifndef _WINNT
 char* _itoa(int value, char* result, int base) 
 {
 /* check that the base if valid */
    if ( base < 2 || base > 36 ) { *result = '\0'; return result; }
        
    char *ptr  = result; 
    char *ptr1 = result;
    char  tmp_char;
    int   tmp_value;
 
    do 
    {
       tmp_value = value;
       value /= base;
       *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );
        
 /* Apply negative sign */
    if (tmp_value < 0)  
    {
       *ptr++ = '-';
    }
    *ptr-- = '\0';
    while(ptr1 < ptr) 
    {
       tmp_char = *ptr;
       *ptr--= *ptr1;
       *ptr1++ = tmp_char;
    }
    return result;
 }
#endif


      /******************************************************************
       *                         ConvertLoc()                           *
       *                                                                *
       *  This function converts a lat/lon pair expressed in +/- form to*
       *  an alphabetic output such as 45.2N 124.2W.                    *
       *                                                                *
       *  Arguments:                                                    *
       *   pll1             Input lat/lon pair                          *
       *   pll2             Converted lat/lon pair                      *
       *   pcLat            S or N for south or north                   *
       *   pcLon            E or W for east or west                     *
       *                                                                *
       ******************************************************************/
       
void ConvertLoc( LATLON *pll1, LATLON *pll2, char *pcLat, char *pcLon )
{
   *pcLat = 'N';
   *pcLon = 'E';
   if ( pll1->dLon > 180. ) pll1->dLon -= 360.;
   if ( pll1->dLat < 0. ) *pcLat = 'S';
   if ( pll1->dLon < 0. ) *pcLon = 'W';
   pll2->dLat = fabs( pll1->dLat );
   pll2->dLon = fabs( pll1->dLon );
}

      /******************************************************************
       *                            GeoCent()                           *
       *                                                                *
       *  Converts to geocentric latitude and longitude . Latitude runs *
       *  from 0 degrees at North to 180 degrees.  Longitude runs 0 to  *
       *  360 degrees. West longitudes are negative. Corrected for earth*
       *  shape.  Returned values are in radians.                       *
       *                                                                *
       *  Arguments:                                                    *
       *   pll              On Input: geographic lat/lon                *
       *                    On Output: geocentric lat/lon               *
       *                                                                *
       ******************************************************************/
	   
void GeoCent( LATLON *pll )
{
   double  dTemp;

   while ( pll->dLat < -90.0 ) pll->dLat += 90.0;
   while ( pll->dLat > 90.0 )  pll->dLat -= 90.0;
   if ( pll->dLat < 0. )
   {
      dTemp = 1.;
      pll->dLat = fabs( pll->dLat );
   }
   else dTemp = -1.;
   pll->dLat *= RAD;
   pll->dLat = atan( 0.993277 * tan( pll->dLat ) );
   pll->dLat = PI/2. + pll->dLat*dTemp;

   while ( pll->dLon < 0.0 )    pll->dLon += 360.0;
   while ( pll->dLon >= 360.0 ) pll->dLon -= 360.0;
   pll->dLon *= RAD;
}

      /******************************************************************
       *                          GeoGraphic()                          *
       *                                                                *
       *  This function converts geocentric lat/lon to geographic       *
       *  lat/lon.  Output longitude goes from 0 to 360.                *
       *                                                                *
       *  Arguments:                                                    *
       *   pllOut           Converted geographic lat/lon                *
       *   pllIn            Input geocentric lat/lon                    *
       *                                                                *
       ******************************************************************/
	   
void GeoGraphic( LATLON *pllOut, LATLON *pllIn )
{
   while ( pllIn->dLon > TWOPI) pllIn->dLon -= TWOPI;
   while ( pllIn->dLon < 0.0)   pllIn->dLon += TWOPI;
   pllOut->dLon = pllIn->dLon * DEG;
   pllOut->dLat = DEG * atan( (cos( pllIn->dLat ) / sin( pllIn->dLat )) /
                  0.993277 );
}

      /******************************************************************
       *                     GetDistanceAz()                            *
       *                                                                *
       *  This function calculates the distance and azimuth between two *
       *  points. Spherical trigonometric relations are used.  The      *
       *  azimuth is from ll1 to ll2.  NOTE: An explanation of the math *
       *  used here can be found in "Modern Trigonometry", by Kaj L.    *
       *  Nielsen, 1966, Barnes and Noble (a brief explanation exists in*
       *  the CRC Math Tables).                                         *
       *                                                                *
       *  July, 2011: Changed so that azidelt is an argument created    *
       *              elsewhere.                                        *
       *                                                                *
       *  Arguments:                                                    *
       *   pll1             From point                                  *
       *   pll2             To point                                    *
       *   pazidelt         Structure with an azimuth and distance (180 *
       *                    degrees returned if a problem)              *     
       *                                                                *
       *  Returns:                                                      *
       *   iFlag   1=Normal; -1=Bad input value                         *
       ******************************************************************/
            
int GetDistanceAz( LATLON *pll1, LATLON *pll2, AZIDELT *pazidelt )
{
   double  dAdd;        /* 1/2 (pll1 azimuth + pll2 azimuth) */
   double  dDelLat;     /* Difference in latitudes */
   double  dDelLon;     /* Angle at north pole in radians */
   double  dSub;        /* 1/2 (pll1 azimuth - pll2 azimuth) */
   int     iFlag;       /* Return Value */
  
/* Check inputs, if they could cause floating point error, send back zeroes */
   iFlag = 1;
   if ( pll1->dLat <= 0. )
   {
      logit( "t", "1-Bad pll1->dLat - %lf\n", pll1->dLat );
      pll1->dLat = 0.;
      iFlag = -1;
   }
   if ( pll1->dLat > PI )
   {
      logit( "t", "2-Bad pll1->dLat - %lf\n", pll1->dLat );
      pll1->dLat = PI;
      iFlag = -1;
   }
   if ( pll2->dLat <= 0. )
   {
      logit( "t", "3-Bad pll2->dLat - %lf\n", pll2->dLat );
      pll2->dLat = 0.;
      iFlag = -1;
   }
   if ( pll2->dLat > PI )

   {
      logit( "t", "4-Bad pll2->dLat - %lf\n", pll2->dLat );
      pll2->dLat = PI;
      iFlag = -1;
   }
   if ( pll1->dLon < 0. )
   {
      logit( "t", "5-Bad pll1->dLon - %lf\n", pll1->dLon );
      pll1->dLon = 0.;
      iFlag = -1;
   }
   if ( pll1->dLon > TWOPI )
   {
      logit( "t", "6-Bad pll1->dLon - %lf\n", pll1->dLon );
      pll1->dLon = TWOPI;
      iFlag = -1;
   }
   if ( pll2->dLon < 0. )
   {
      logit( "t", "7-Bad pll2->dLon - %lf\n", pll2->dLon );
      pll2->dLon = 0.;
      iFlag = -1;
   }
   if ( pll2->dLon > TWOPI )
   {
      logit( "t", "8-Bad pll2->dLon - %lf\n", pll2->dLon );
      pll2->dLon = TWOPI;
      iFlag = -1;
   }

   dDelLon = fabs( pll1->dLon - pll2->dLon );   /* Polar angle */
   while ( dDelLon >= TWOPI ) dDelLon -= TWOPI; /* Not likely */
   if ( dDelLon >= PI ) dDelLon = TWOPI - dDelLon; /* Force it between 0 and PI */
   if ( dDelLon == PI ) dDelLon -= 0.000000001; /* So tan doesn't explode */
   if ( dDelLon == 0. ) dDelLon += 0.000000001;	/* Prevent divide by zero */
   dDelLat = pll1->dLat - pll2->dLat;
   if ( dDelLat == PI ) dDelLat -= 0.000000001;
   if ( dDelLat == 0. ) dDelLat += 0.000000001;
/* Use Napier analogies to get internal angles (azimuths) at pll1 and 2 */
   dAdd = atan( (cos( 0.5 * dDelLat )) / (tan( 0.5*dDelLon ) * 
                 cos( 0.5 * (pll1->dLat+pll2->dLat) )) );
   dSub = atan( (sin( 0.5 * dDelLat )) / (tan( 0.5*dDelLon ) * 
                 sin( 0.5 * (pll1->dLat+pll2->dLat) )) );
/* Subtract two to get pll1 to pll2 azimuth (add the two together if the
   azimuth from pll2 to pll1 is needed) */
   pazidelt->dAzimuth = dAdd - dSub;
/* Get azimuth clockwise from north, account for crossing prime meridian */
   if ( pazidelt->dAzimuth < 0. ) /* co-azimuth */
   {
      if ( pll2->dLon < pll1->dLon || pll2->dLon > pll1->dLon+PI ) /* sta. west */
         pazidelt->dAzimuth = PI - pazidelt->dAzimuth;                 /* of epi. */
      else                      /* Sta. east of epi. */
         pazidelt->dAzimuth = PI + pazidelt->dAzimuth;
   }
   else                         /* actual azimuth */
   {
      if ( pll2->dLon < pll1->dLon || pll2->dLon > pll1->dLon+PI ) 
           pazidelt->dAzimuth = TWOPI - pazidelt->dAzimuth;
   }
/* Check to see that computed azimuth makes sense */
   if ( pazidelt->dAzimuth < 0. || pazidelt->dAzimuth > TWOPI )
      logit( "t", "Bad az in GetDistanceAz: %lf\nlat1=%lf, lon1=%lf, "
             "lat2=%lf, lon2=%lf\n", pazidelt->dAzimuth, pll1->dLat, pll1->dLon,
             pll2->dLat, pll2->dLon );
/* Use another one of Napier's analogies to get distance from pll1 to 2 */
   if ( dSub == 0. || dSub == PI ) dSub += 0.0000001; /* Prevent divide by zero */
   pazidelt->dDelta = fabs( 2.*atan( (tan( 0.5*dDelLat ) *
                    sin( dAdd )) / sin( dSub ) ) );
/* Check to see that computed distance makes sense */
   if ( pazidelt->dDelta < 0. || pazidelt->dDelta > PI )
      logit ( "t", "Bad dist in GetDistanceAz: %lf\nlat1=%lf, lon1=%lf, "
              "lat2=%lf, lon2=%lf\n", pazidelt->dDelta, pll1->dLat, pll1->dLon,
              pll2->dLat, pll2->dLon );
/* Convert radian angles to degrees */
   pazidelt->dDelta *= DEG;
   pazidelt->dAzimuth *= DEG;
   return iFlag;
}

      /******************************************************************
       *                     GetDistanceAz2()                           *
       *                                                                *
       *  This function calculates the distance and azimuth between two *
       *  points. Spherical trigonometric relations are used.  The      *
       *  azimuth is from ll1 to ll2.  NOTE: An explanation of the math *
       *  used here can be found in "Modern Trigonometry", by Kaj L.    *
       *  Nielsen, 1966, Barnes and Noble (a brief explanation exists in*
       *  the CRC Math Tables).                                         *
       *                                                                *
       *  July, 2011: Changed so that azidelt is an argument created    *
       *              elsewhere.                                        *
       *                                                                *
       *  Arguments:                                                    *
       *   pll1             From point                                  *
       *   pll2             To point                                    *
       *                                                                *
       *  Returns:                                                      *
       *   always 1                                                     *
       ******************************************************************/
            
int GetDistanceAz2( LATLON *pll1, STATION *pll2 )
{
   double  dDeltaCosine, dDeltaSine;
   double  dTempDelta, dTempAz;
   LATLON  ll;

   GetLatLonTrig( (LATLON *) pll1 );
   if ( pll1->dSinlat == 0.0 ) pll1->dSinlat = 0.01;

   ll.dLat = pll2->dLat;
   ll.dLon = pll2->dLon;
   GeoCent( &ll );
   GetLatLonTrig( &ll );
   dDeltaCosine = pll1->dCoslat*pll2->dCoslat + pll1->dSinlat*
                  ll.dSinlat*(pll1->dCoslon*ll.dCoslon +
                  pll1->dSinlon*ll.dSinlon);
   dDeltaSine = sqrt( 1. - dDeltaCosine*dDeltaCosine );
   if ( dDeltaSine == 0.0 )   dDeltaSine = 0.01;
   if ( dDeltaCosine == 0.0 ) dDeltaCosine = 0.01;
   dTempDelta =  atan( dDeltaSine / dDeltaCosine );
   pll2->dCooze = (ll.dCoslat - pll1->dCoslat * dDeltaCosine) /
                    (pll1->dSinlat * dDeltaSine);
   if ( pll2->dCooze == 0.0 ) pll2->dCooze = 0.0001;
   pll2->dSnooze = ll.dSinlat * (pll1->dCoslon * ll.dSinlon -
                     pll1->dSinlon * ll.dCoslon) / dDeltaSine;
   dTempAz = atan( pll2->dSnooze / pll2->dCooze );
   while ( dTempAz < 0.0 )       dTempAz += PI;
   if ( pll2->dSnooze <= 0. )  dTempAz += PI;
   while ( dTempDelta < 0.0 )    dTempDelta += PI;
   dTempDelta *= DEG;
   pll2->dFracDelta = dTempDelta - floor( dTempDelta );
   pll2->dDelta     = dTempDelta;
   pll2->dAzimuth   = dTempAz * DEG;
   return( 1 );
}         

      /******************************************************************
       *                        GetLatLonTrig()                         *
       *                                                                *
       *  This function computes the sine and cos of the geocentric     *
       *  lat/lon.                                                      *
       *                                                                *
       *  Arguments:                                                    *
       *   pll              Input geocentric lat/lon                    *
       *                                                                *
       ******************************************************************/
	   
void GetLatLonTrig( LATLON *pll )
{
   pll->dCoslat = cos( pll->dLat );
   pll->dSinlat = sin( pll->dLat );
   pll->dCoslon = cos( pll->dLon );
   pll->dSinlon = sin( pll->dLon );
}

      /******************************************************************
       *                       GetOpAgency ()                           *
       *                                                                *
       *  This function takes an input Network ID and returns a         *
       *  descriptive string describing the agency.                     *
       *                                                                *
       *  Arguments:                                                    *
       *   pszNetID         Seismometer Network ID                      *
       *                                                                *
       *  Return:                                                       *
       *   String naming the operating agency.                          *
       *                                                                *
       ******************************************************************/
       
char *GetOpAgency( char *pszNetID )
{
   if ( !strcmp( pszNetID, "AT" ) )
      return( "West Coast/Alaska Tsunami Warning Center" );
   else if ( !strcmp( pszNetID, "AK" ) )
      return( "Alaska Earthquake Information Center" );
   else if ( !strcmp( pszNetID, "II" ) )
      return( "IRIS/IDA Project" );
   else if ( !strcmp( pszNetID, "IU" ) )
      return( "IRIS/ASL Project" );
   else if ( !strcmp( pszNetID, "GT" ) )
      return( "Global Telemetry Seismic Network" );
   else if ( !strcmp( pszNetID, "IC" ) )
      return( "China Siesmographic Bureau" );
   else if ( !strcmp( pszNetID, "US" ) )
      return( "US National Seismic Network" );
   else if ( !strcmp( pszNetID, "BK" ) )
      return( "UC - Berkeley" );
   else if ( !strcmp( pszNetID, "CN" ) )
      return( "Pacific Geosciences Center" );
   else if ( !strcmp( pszNetID, "UW" ) )
      return( "University of Washington" );
   else if ( !strcmp( pszNetID, "CI" ) )
      return( "Cal Tech" );
   else if ( !strcmp( pszNetID, "PT" ) )
      return( "Pacific Tsunami Warning Center" );
   else if ( !strcmp( pszNetID, "HV" ) )
      return( "Hawaii Volcanoes Observatory" );
   else if ( !strcmp( pszNetID, "LD" ) )
      return( "Lamont-Doherty Observatory" );
   else if ( !strcmp( pszNetID, "UO" ) )
      return( "University of Oregon" );
   else if ( !strcmp( pszNetID, "UU" ) )
      return( "University of Utah" );
   else if ( !strcmp( pszNetID, "NN" ) )
      return( "Nevada Seismic Network" );
   else if ( !strcmp( pszNetID, "NE" ) )
      return( "Boston College, Weston Obs." );
   else if ( !strcmp( pszNetID, "PR" ) )
      return( "Puerto Rico Seismic Network" );
   else return( "" );
}

      /******************************************************************
       *                       GetPhaseTime ()                          *
       *                                                                *
       *  This function obtains body wave phase times from the IASPEI91 *
       *  tables.                                                       *
       *                                                                *
       *  Arguments:                                                    *
       *   pSta             Incoming STATION structure                  *
       *   pHypo            Hypocenter to compare p-pick with           *
       *   pPhase           Expected phase times for this comparison    *
       *   iPhaseToGet      1=P only; 2=P=Depth phases; 3=all basic     *
       *   pszIasp91File    File with phase info                        *
       *                                                                *
       *  Return:                                                       *
       *   1 -> Phases returned ok; 0 -> problem                        *
       *                                                                *
       ******************************************************************/
int GetPhaseTime( STATION *pSta, HYPO *pHypo, PHASE *pPhase, int iPhaseToGet,
                  char *pszIasp91File )
{
   AZIDELT azidelt;             /* Distance/azimuth between 2 points */
   int     iPrint[3] = {0, 0, 1};
   double  dDDdp[MAX_PHASES];
   double  dDTdd[MAX_PHASES];
   double  dDTdh[MAX_PHASES];
   double  dTT[MAX_PHASES];
   double  dUsrC[2];
   FILE   *hFileTbl;            /* iaspei91 tau/p file */
   int     j;
   LATLON  ll;
   int     n;                   /* # of phases returned for this station */
   int     iNo = 0;             /* # of keywords (or segments) to brnset */
   /* char    szModnam[] = {"iasp91"}; */
   char    szPhase[MAX_PHASES][PHASE_LENGTH], szPhaseLst[10][PHASE_LENGTH];

/* Figure distance to station from this hypocenter */
   ll.dLat = pSta->dLat;
   ll.dLon = pSta->dLon;
   GeoCent( &ll );
   GetLatLonTrig( &ll );
   GetDistanceAz( (LATLON *) pHypo, &ll, &azidelt );
   
/* Query taulib for phases */
   if ( iPhaseToGet == 1 )                                   /* Just P phases */
   {
      iNo = 1;
      strcpy( szPhaseLst[0], "P" );
   }
   else if ( iPhaseToGet == 2 )                     /* P phase + depth phases */
   {
      iNo = 3;
      strcpy( szPhaseLst[0], "P" );
      strcpy( szPhaseLst[1], "pP" );
      strcpy( szPhaseLst[2], "sP" );
   }
   else if ( iPhaseToGet == 3 )                           /* all basic phases */
   {
      iNo = 1;
//      strcpy( szPhaseLst[0], "basic" );
      strcpy( szPhaseLst[0], "all" );
   }
/* Read in iaspei91 tau/p file */
   if ( tabin( &hFileTbl, pszIasp91File ) == 0 ) return( 0 );   
   brnset( iNo, szPhaseLst, iPrint );                /* Set up phases desired */
   depset( pHypo->dDepth, dUsrC, hFileTbl );              /* Depth correction */
/* Compute phase times */
   trtm( azidelt.dDelta, &n, dTT, dDTdd, dDTdh, dDDdp, szPhase );
   pPhase->iNumPhases = n;               
   for ( j=0; j<n; j++ )                               /* Fill up PHASE array */
   {                       /* Odd way to do elevation correction but it works */
      pPhase->dPhaseTT[j]   = dTT[j] + pSta->dElevation/EARTHRAD;
      pPhase->dPhaseTime[j] = dTT[j] + pHypo->dOriginTime +
                              pSta->dElevation/EARTHRAD;
      strcpy( pPhase->szPhase[j], szPhase[j] );
   }
   fclose( hFileTbl );
   return( 1 );
}                            

      /******************************************************************
       *                         GetQuakeID()                           *
       *                                                                *
       *  This function creates a quake ID based on the origin time.    *
       *  The Quake ID is a 6 character alphanumeric code based on a    *
       *  base 36 conversion from the OTime in 1/1/70 seconds.          *
       *                                                                *
       *  Arguments:                                                    *
       *   dOTime           Quake origin time in 1/1/70 seconds         *
       *   pszQuakeID       Output quake ID                             *
       *                                                                *
       ******************************************************************/
       
void GetQuakeID( double dOTime, char *pszQuakeID )
{
   _itoa( (int) dOTime, pszQuakeID, 36 );
   PadZeroes( 6, pszQuakeID );    /* Add leading zeros */
   return;
}
	                   
      /******************************************************************
       *                           GetRegion()                          *
       *                                                                *
       * This function computes and returns the region which the quake  *
       * is located in.  These regions key specific procedures at the   *
       * West Coast/Alaska Tsunami Warning Center.  Regions are:        *
       *     0 - U.S. west coast                                        *
       *     1 - British Columbia                                       *
       *     2 - Alaska/Alaska Peninsula/Aleutian                       *
       *     3 - Bering Sea-shallow                                     *
       *     4 - Bering Sea-deep                                        *
       *     5 - Pacific Basin (non WC&ATWC AOR)                        *
       *     8 - Arctic Ocean (AOR)                                     *
       *     9 - Arctic Ocean (non-AOR)                                 *
       *     10 - US East Coast                                         *
       *     11 - Gulf of Mexico                                        *
       *     12 - Eastern Canada                                        *
       *     13 - Gulf of St. Lawrence                                  *
       *     14 - Puerto Rico                                           *
       *     15 - Caribbean Sea                                         *
       *     16 - Mid-Atlantic Ridge                                    *
       *     17 - Atlantic Basin	                                    *
       *     20 - None of the above                                     *
       *                                                                *
       *  May, 2012: Updated E Coast/Canada regions                     *
       *  February, 2011: Updated PR/VI region                          *
       *  December, 2007: Adjusted Bering Sea shallow region near Nome. *
       *  January, 2005: Added Arctic and Bering Sea-shallow region     *
       *  December, 2005: Tightened up west coast and Alaska.           *
       *  November, 2005: Added Atlantic regions.                       *
       *  January, 2005: Added Atlantic regions.                        *
       *  November, 2004: Added a Caribbean region and adjusted Bering  *
       *                  Sea region.                                   *
       *                                                                *
       *  Arguments:                                                    *
       *   dLat             Epicentral latitude (geographic)            *
       *   dLon             Epicentral longitude                        *
       *                                                                *
       *  Return:                                                       *
       *   Region number (see above)                                    *
       *                                                                *
       ******************************************************************/
       
int GetRegion( double dLat, double dLon )
{
   double  dLatT, dLonT;        /* lat/lon with lon from 0-360 */
   int     iMsgRegion;          /* Quake region (see above) */

/* Adjust epicenter in case longitude comes in negative */
   dLatT = dLat;
   dLonT = dLon;
   if ( dLonT < 0. ) dLonT += 360.;

/* Get the region */
   if ( dLatT >= 53. && dLatT < 53.5 && dLonT >= 174. && dLonT <= 188. )
      iMsgRegion = 4;                 /* Bering Sea-deep */
   else if ( dLatT >= 53.5 && dLatT < 54. && dLonT >= 174. && dLonT <= 191. )
      iMsgRegion = 4;                 /* Bering Sea-deep */
   else if ( dLatT >= 54. && dLatT < 54.5 && dLonT >= 170. && dLonT <= 192. )
      iMsgRegion = 4;                 /* Bering Sea-deep */
   else if ( dLatT >= 54.5 && dLatT < 55. && dLonT >= 170. && dLonT <= 194. )
      iMsgRegion = 4;                 /* Bering Sea-deep */
   else if ( dLatT >= 55. && dLatT < 55.5 && dLonT >= 168. && dLonT <= 194. )
      iMsgRegion = 4;                 /* Bering Sea-deep */
   else if ( dLatT >= 55.5 && dLatT < 56. && dLonT >= 168. && dLonT <= 193. )
      iMsgRegion = 4;                 /* Bering Sea-deep */
   else if ( dLatT >= 56. && dLatT < 57. && dLonT >= 165. && dLonT <= 193. )
      iMsgRegion = 4;                 /* Bering Sea-deep */
   else if ( dLatT >= 57. && dLatT < 58. && dLonT >= 162. && dLonT <= 190. )
      iMsgRegion = 4;                 /* Bering Sea-deep */
   else if ( dLatT >= 58. && dLatT < 59. && dLonT >= 162. && dLonT <= 188. )
      iMsgRegion = 4;                 /* Bering Sea-deep */
   else if ( dLatT >= 59. && dLatT < 60. && dLonT >= 163. && dLonT <= 186. )
      iMsgRegion = 4;                 /* Bering Sea-deep */
   else if ( dLatT >= 60. && dLatT < 61. && dLonT >= 165. && dLonT <= 184. )
      iMsgRegion = 4;                 /* Bering Sea-deep */
   else if ( dLatT >= 61. && dLatT < 62. && dLonT >= 172. && dLonT <= 183. )
      iMsgRegion = 4;                 /* Bering Sea-deep */
   else if ( dLatT >= 62. && dLatT < 63. && dLonT >= 175. && dLonT <= 180. )
      iMsgRegion = 4;                 /* Bering Sea-deep */
   else if ( dLatT >= 55.5 && dLatT < 56. && dLonT >= 193. && dLonT <= 197. )
      iMsgRegion = 3;                 /* Bering Sea-shallow */             
   else if ( dLatT >= 56. && dLatT < 56.5 && dLonT >= 193. && dLonT <= 199. )
      iMsgRegion = 3;                 /* Bering Sea-shallow */
   else if ( dLatT >= 56.5 && dLatT < 57. && dLonT >= 193. && dLonT <= 200. )
      iMsgRegion = 3;                 /* Bering Sea-shallow */
   else if ( dLatT >= 57. && dLatT < 57.5 && dLonT >= 190. && dLonT <= 201. )
      iMsgRegion = 3;                 /* Bering Sea-shallow */
   else if ( dLatT >= 57.5 && dLatT < 58. && dLonT >= 190. && dLonT <= 203. )
      iMsgRegion = 3;                 /* Bering Sea-shallow */
   else if ( dLatT >= 58. && dLatT < 59. && dLonT >= 188. && dLonT <= 203. )
      iMsgRegion = 3;                 /* Bering Sea-shallow */
   else if ( dLatT >= 59. && dLatT < 59.5 && dLonT >= 186. && dLonT <= 203. )
      iMsgRegion = 3;                 /* Bering Sea-shallow */
   else if ( dLatT >= 59.5 && dLatT < 60. && dLonT >= 186. && dLonT <= 200. )
      iMsgRegion = 3;                 /* Bering Sea-shallow */
   else if ( dLatT >= 60. && dLatT < 60.5 && dLonT >= 184. && dLonT <= 200. )
      iMsgRegion = 3;                 /* Bering Sea-shallow */
   else if ( dLatT >= 60.5 && dLatT < 61. && dLonT >= 184. && dLonT <= 200. )
      iMsgRegion = 3;                 /* Bering Sea-shallow */
   else if ( dLatT >= 61. && dLatT < 62. && dLonT >= 183. && dLonT <= 200. )
      iMsgRegion = 3;                 /* Bering Sea-shallow */
   else if ( dLatT >= 62. && dLatT < 63. && dLonT >= 180. && dLonT <= 200. )
      iMsgRegion = 3;                 /* Bering Sea-shallow */
   else if ( dLatT >= 63. && dLatT < 65. && dLonT >= 178. && dLonT <= 200. )
      iMsgRegion = 3;                 /* Bering Sea-shallow */
   else if ( dLatT >= 65. && dLatT < 66. && dLonT >= 178. && dLonT <= 200. )
      iMsgRegion = 3;                 /* Bering Sea-shallow */
   else if ( dLatT >= 66. && dLatT < 75. && dLonT >= 186. && dLonT <= 205. )
      iMsgRegion = 8;                 /* Arctic O. AOR */
   else if ( dLatT >= 67. && dLatT < 79. && dLonT >= 200. && dLonT <= 220. )
      iMsgRegion = 8;                 /* Arctic O. AOR */
   else if ( dLatT >= 67. && dLatT < 83. && dLonT >= 220. && dLonT <= 240. )
      iMsgRegion = 8;                 /* Arctic O. AOR */
   else if ( dLatT >= 67. && dLatT < 87. && dLonT >= 240. && dLonT <= 276. )
      iMsgRegion = 8;                 /* Arctic O. AOR */
   else if ( dLatT >= 81. && dLatT < 87. && dLonT >= 276. && dLonT <= 300. )
      iMsgRegion = 8;                 /* Arctic O. AOR */
   else if ( dLatT >= 45. && dLatT < 53.0 && dLonT >= 170. && dLonT < 195. )
      iMsgRegion = 2;                 /* Alaska/AK Pen./Aleutians */
   else if ( dLatT >= 53. && dLatT < 54.0 && dLonT >= 170. && dLonT < 174. )
      iMsgRegion = 2;                 /* Alaska/AK Pen./Aleutians */
   else if ( dLatT >= 53. && dLatT < 53.5 && dLonT >= 188. && dLonT < 195. )
      iMsgRegion = 2;                 /* Alaska/AK Pen./Aleutians */
   else if ( dLatT >= 53.5 && dLatT < 54. && dLonT >= 191. && dLonT < 195. )
      iMsgRegion = 2;                 /* Alaska/AK Pen./Aleutians */
   else if ( dLatT >= 54. && dLatT < 54.5 && dLonT >= 192. && dLonT < 195. )
      iMsgRegion = 2;                 /* Alaska/AK Pen./Aleutians */
   else if ( dLatT >= 49. && dLatT < 54.5 && dLonT >= 195. && dLonT < 210. )
      iMsgRegion = 2;                 /* Alaska/AK Pen./Aleutians */
   else if ( dLatT >= 54.5 && dLatT < 55.5 && dLonT >= 194. && dLonT < 210. )
      iMsgRegion = 2;                 /* Alaska/AK Pen./Aleutians */
   else if ( dLatT >= 55.5 && dLatT < 56. && dLonT >= 197. && dLonT < 210. )
      iMsgRegion = 2;                 /* Alaska/AK Pen./Aleutians */
   else if ( dLatT >= 56. && dLatT < 56.5 && dLonT >= 199. && dLonT < 210. )
      iMsgRegion = 2;                 /* Alaska/AK Pen./Aleutians */
   else if ( dLatT >= 56.5 && dLatT < 57. && dLonT >= 200. && dLonT < 210. )
      iMsgRegion = 2;                 /* Alaska/AK Pen./Aleutians */
   else if ( dLatT >= 57. && dLatT < 57.5 && dLonT >= 201. && dLonT < 210. )
      iMsgRegion = 2;                 /* Alaska/AK Pen./Aleutians */
   else if ( dLatT >= 57.5 && dLatT < 59.5 && dLonT >= 203. && dLonT < 210. )
      iMsgRegion = 2;                 /* Alaska/AK Pen./Aleutians */
   else if ( dLatT >= 59.5 && dLatT < 60.5 && dLonT >= 200. && dLonT < 210. )
      iMsgRegion = 2;                 /* Alaska/AK Pen./Aleutians */
   else if ( dLatT >= 60.5 && dLatT < 63. && dLonT >= 200. && dLonT < 210. )
      iMsgRegion = 2;                 /* Alaska/AK Pen./Aleutians */
   else if ( dLatT >= 63. && dLatT < 66. && dLonT >= 200. && dLonT < 210. )
      iMsgRegion = 2;                 /* Alaska/AK Pen./Aleutians */
   else if ( dLatT >= 66. && dLatT < 67. && dLonT >= 205. && dLonT < 210. )
      iMsgRegion = 2;                 /* Alaska/AK Pen./Aleutians */
   else if ( dLatT >= 52. && dLatT < 67.0 && dLonT >= 210. && dLonT < 220. )
      iMsgRegion = 2;                 /* Alaska/AK Pen./Aleutians */
   else if ( dLatT >= 54.5 && dLatT < 67. && dLonT >= 220. && dLonT < 225. )
      iMsgRegion = 2;                 /* Alaska/AK Pen./Aleutians */
   else if ( dLatT >= 54.5 && dLatT < 63.5 && dLonT >= 225. && dLonT < 230. )
      iMsgRegion = 2;                 /* Alaska/AK Pen./Aleutians */
   else if ( dLatT >= 54.5 && dLatT < 60. && dLonT >= 230. && dLonT < 235. )
      iMsgRegion = 2;                 /* Alaska/AK Pen./Aleutians */
   else if ( dLatT > 48.5 && dLatT <= 50.0 && dLonT >= 223. && dLonT < 242. )
      iMsgRegion = 1;                 /* British Columbia */
   else if ( dLatT > 50.0 && dLatT <= 51.0 && dLonT >= 222. && dLonT < 242. )
      iMsgRegion = 1;                 /* British Columbia */
   else if ( dLatT > 51.0 && dLatT <= 52.0 && dLonT >= 221. && dLonT < 242. )
      iMsgRegion = 1;                 /* British Columbia */
   else if ( dLatT > 52.0 && dLatT <= 54.5 && dLonT >= 220. && dLonT < 240. )
      iMsgRegion = 1;                 /* British Columbia */
   else if ( dLatT >= 42.0 && dLatT <= 48.5 && dLonT >= 226. && dLonT < 242. )
      iMsgRegion = 0;                 /* U.S. West Coast */
   else if ( dLatT >= 38.0 && dLatT <= 42.0 && dLonT >= 226. && dLonT < 240. )
      iMsgRegion = 0;                 /* U.S. West Coast */
   else if ( dLatT >= 36.0 && dLatT <= 38.0 && dLonT >= 228. && dLonT < 243. )
      iMsgRegion = 0;                 /* U.S. West Coast */
   else if ( dLatT >= 34.0 && dLatT <= 36.0 && dLonT >= 230. && dLonT < 246. )
      iMsgRegion = 0;                 /* U.S. West Coast */
   else if ( dLatT >= 30.0 && dLatT <= 32.0 && dLonT >= 231. && dLonT < 245. )
      iMsgRegion = 0;                 /* U.S. West Coast */
   else if ( dLatT >= 32.0 && dLatT <= 34.0 && dLonT >= 231. && dLonT < 246. )
      iMsgRegion = 0;                 /* U.S. West Coast */
   else if ( dLatT >= 51.0 && dLatT < 52.0 && dLonT >= 300. && dLonT < 303.5 )
      iMsgRegion = 13;                /* Gulf of St. Lawrence */
   else if ( dLatT >= 50.0 && dLatT < 51.0 && dLonT >= 290. && dLonT < 303. )
      iMsgRegion = 13;                /* Gulf of St. Lawrence */
   else if ( dLatT >= 48.0 && dLatT < 50.0 && dLonT >= 290. && dLonT < 302.5 )
      iMsgRegion = 13;                /* Gulf of St. Lawrence */
   else if ( dLatT >= 47.0 && dLatT < 48.0 && dLonT >= 293. && dLonT < 300. )
      iMsgRegion = 13;                /* Gulf of St. Lawrence */
   else if ( dLatT >= 46.0 && dLatT < 47.0 && dLonT >= 294. && dLonT < 299. )
      iMsgRegion = 13;                /* Gulf of St. Lawrence */
   else if ( dLatT >= 45.5 && dLatT < 46.0 && dLonT >= 296. && dLonT < 298.5 )
      iMsgRegion = 13;                /* Gulf of St. Lawrence */
   else if ( dLatT >= 22.0 && dLatT < 24.0 && dLonT >= 279. && dLonT < 284. )
      iMsgRegion = 10;                /* U.S. East Coast */
   else if ( dLatT >= 24.0 && dLatT < 27.0 && dLonT >= 279. && dLonT < 287. )
      iMsgRegion = 10;                /* U.S. East Coast */
   else if ( dLatT >= 27.0 && dLatT < 28.0 && dLonT >= 278.5 && dLonT < 287. )
      iMsgRegion = 10;                /* U.S. East Coast */
   else if ( dLatT >= 28.0 && dLatT < 30.0 && dLonT >= 278. && dLonT < 287. )
      iMsgRegion = 10;                /* U.S. East Coast */
   else if ( dLatT >= 30.0 && dLatT < 31.0 && dLonT >= 277. && dLonT < 287. )
      iMsgRegion = 10;                /* U.S. East Coast */
   else if ( dLatT >= 31.0 && dLatT < 32.0 && dLonT >= 277. && dLonT < 287. )
      iMsgRegion = 10;                /* U.S. East Coast */
   else if ( dLatT >= 32.0 && dLatT < 33.0 && dLonT >= 276. && dLonT < 288. )
      iMsgRegion = 10;                /* U.S. East Coast */
   else if ( dLatT >= 33.0 && dLatT < 34.0 && dLonT >= 276. && dLonT < 289. )
      iMsgRegion = 10;                /* U.S. East Coast */
   else if ( dLatT >= 34.0 && dLatT < 36.0 && dLonT >= 276. && dLonT < 290. )
      iMsgRegion = 10;                /* U.S. East Coast */
   else if ( dLatT >= 36.0 && dLatT < 38.0 && dLonT >= 278. && dLonT < 291. )
      iMsgRegion = 10;                /* U.S. East Coast */
   else if ( dLatT >= 38.0 && dLatT < 39.0 && dLonT >= 278. && dLonT < 294. )
      iMsgRegion = 10;                /* U.S. East Coast */
   else if ( dLatT >= 39.0 && dLatT < 40.0 && dLonT >= 278. && dLonT < 295. )
      iMsgRegion = 10;                /* U.S. East Coast */
   else if ( dLatT >= 40.0 && dLatT < 42.0 && dLonT >= 280. && dLonT < 297. )
      iMsgRegion = 10;                /* U.S. East Coast */
   else if ( dLatT >= 42.0 && dLatT < 45.0 && dLonT >= 280. && dLonT < 294. )
      iMsgRegion = 10;                /* U.S. East Coast */
   else if ( dLatT >= 45.0 && dLatT < 48.0 && dLonT >= 288. && dLonT < 293. )
      iMsgRegion = 10;                /* U.S. East Coast */
   else if ( dLatT >= 18.0 && dLatT < 20.0 && dLonT >= 263. && dLonT < 271. )
      iMsgRegion = 11;                /* Gulf of Mexico */
   else if ( dLatT >= 20.0 && dLatT < 21.0 && dLonT >= 261. && dLonT < 271. )
      iMsgRegion = 11;                /* Gulf of Mexico */
   else if ( dLatT >= 21.0 && dLatT < 22.0 && dLonT >= 261. && dLonT < 274. )
      iMsgRegion = 11;                /* Gulf of Mexico */
   else if ( dLatT >= 22.0 && dLatT < 22.5 && dLonT >= 261. && dLonT < 276. )
      iMsgRegion = 11;                /* Gulf of Mexico */
   else if ( dLatT >= 22.5 && dLatT < 23.0 && dLonT >= 261. && dLonT < 277. )
      iMsgRegion = 11;                /* Gulf of Mexico */
   else if ( dLatT >= 23.0 && dLatT < 27.0 && dLonT >= 261. && dLonT < 279. )
      iMsgRegion = 11;                /* Gulf of Mexico */
   else if ( dLatT >= 27.0 && dLatT < 28.0 && dLonT >= 261. && dLonT < 278.5 )
      iMsgRegion = 11;                /* Gulf of Mexico */
   else if ( dLatT >= 28.0 && dLatT < 30.0 && dLonT >= 261. && dLonT < 278. )
      iMsgRegion = 11;                /* Gulf of Mexico */
   else if ( dLatT >= 30.0 && dLatT < 31.0 && dLonT >= 261. && dLonT < 277. )
      iMsgRegion = 11;                /* Gulf of Mexico */
   else if ( dLatT >= 31.0 && dLatT < 32.0 && dLonT >= 263. && dLonT < 277. )
      iMsgRegion = 11;                /* Gulf of Mexico */
   else if ( dLatT >= 32.0 && dLatT < 34.0 && dLonT >= 264. && dLonT < 276. )
      iMsgRegion = 11;                /* Gulf of Mexico */
   else if ( dLatT >= 40.0 && dLatT < 42.0 && dLonT >= 297. && dLonT < 315. )
      iMsgRegion = 12;                /* Eastern Canada */
   else if ( dLatT >= 42.0 && dLatT < 45.0 && dLonT >= 294. && dLonT < 315. )
      iMsgRegion = 12;                /* Eastern Canada */
   else if ( dLatT >= 45.0 && dLatT < 48.0 && dLonT >= 285. && dLonT < 288. )
      iMsgRegion = 12;                /* Eastern Canada */
   else if ( dLatT >= 48.0 && dLatT < 52.0 && dLonT >= 285. && dLonT < 320. )
      iMsgRegion = 12;                /* Eastern Canada */
   else if ( dLatT >= 45.0 && dLatT < 48.0 && dLonT >= 293. && dLonT < 320. )
      iMsgRegion = 12;                /* Eastern Canada */
   else if ( dLatT >= 52.0 && dLatT < 58.0 && dLonT >= 285. && dLonT < 315. )
      iMsgRegion = 12;                /* Eastern Canada */
   else if ( dLatT >= 58.0 && dLatT < 62.0 && dLonT >= 285. && dLonT < 307. )
      iMsgRegion = 12;                /* Eastern Canada */
   else if ( dLatT >= 17.0 && dLatT < 21.0 && dLonT >= 291.75 && dLonT < 296.5 )
      iMsgRegion = 14;                /* Puerto Rico */
   else if ( dLatT >= 8.0 && dLatT < 9.2 && dLonT >= 282. && dLonT < 305. )
      iMsgRegion = 15;                /* Caribbean */
   else if ( dLatT >= 8.5 && dLatT < 9.0 && dLonT >= 278. && dLonT < 280. )
      iMsgRegion = 15;                /* Caribbean */
   else if ( dLatT >= 9.0 && dLatT < 9.2 && dLonT >= 276.5 && dLonT < 280. )
      iMsgRegion = 15;                /* Caribbean */
   else if ( dLatT >= 9.2 && dLatT < 10.0 && dLonT >= 276.5 && dLonT < 305. )
      iMsgRegion = 15;                /* Caribbean */
   else if ( dLatT >= 10.0 && dLatT < 11.0 && dLonT >= 276. && dLonT < 305. )
      iMsgRegion = 15;                /* Caribbean */
   else if ( dLatT >= 11.0 && dLatT < 15.0 && dLonT >= 275. && dLonT < 305. )
      iMsgRegion = 15;                /* Caribbean */
   else if ( dLatT >= 15.0 && dLatT < 21.0 && dLonT >= 271. && dLonT < 305. )
      iMsgRegion = 15;                /* Caribbean */
   else if ( dLatT >= 21.0 && dLatT < 22.0 && dLonT >= 274. && dLonT < 305. )
      iMsgRegion = 15;                /* Caribbean */
   else if ( dLatT >= 22.0 && dLatT < 22.5 && dLonT >= 276. && dLonT < 279. )
      iMsgRegion = 15;                /* Caribbean */
   else if ( dLatT >= 22.5 && dLatT < 23.0 && dLonT >= 277. && dLonT < 279. )
      iMsgRegion = 15;                /* Caribbean */
   else if ( dLatT >= 22.0 && dLatT < 24.0 && dLonT >= 284. && dLonT < 305. )
      iMsgRegion = 15;                /* Caribbean */
   else if ( dLatT >= -52.0 && dLatT < -50.0 && dLonT >= 346. && dLonT < 358. )
      iMsgRegion = 16;                /* Atlantic Ridge */
   else if ( dLatT >= -50.0 && dLatT < -45.0 && dLonT >= 340. && dLonT < 358. )
      iMsgRegion = 16;                /* Atlantic Ridge */
   else if ( dLatT >= -45.0 && dLatT < -6.0 && dLonT >= 340. && dLonT < 352. )
      iMsgRegion = 16;                /* Atlantic Ridge */
   else if ( dLatT >= -6.0 && dLatT < -2.0 && dLonT >= 330. && dLonT < 352. )
      iMsgRegion = 16;                /* Atlantic Ridge */
   else if ( dLatT >= -2.0 && dLatT < 2.0 && dLonT >= 326. && dLonT < 352. )
      iMsgRegion = 16;                /* Atlantic Ridge */
   else if ( dLatT >= 2.0 && dLatT < 3.0 && dLonT >= 326. && dLonT < 345. )
      iMsgRegion = 16;                /* Atlantic Ridge */
   else if ( dLatT >= 3.0 && dLatT < 5.0 && dLonT >= 320. && dLonT < 345. )
      iMsgRegion = 16;                /* Atlantic Ridge */
   else if ( dLatT >= 5.0 && dLatT < 7.0 && dLonT >= 320. && dLonT < 332. )
      iMsgRegion = 16;                /* Atlantic Ridge */
   else if ( dLatT >= 7.0 && dLatT < 10.0 && dLonT >= 314. && dLonT < 332. )
      iMsgRegion = 16;                /* Atlantic Ridge */
   else if ( dLatT >= 10.0 && dLatT < 12.0 && dLonT >= 310. && dLonT < 332. )
      iMsgRegion = 16;                /* Atlantic Ridge */
   else if ( dLatT >= 12.0 && dLatT < 28.0 && dLonT >= 310. && dLonT < 321. )
      iMsgRegion = 16;                /* Atlantic Ridge */
   else if ( dLatT >= 28.0 && dLatT < 30.0 && dLonT >= 310. && dLonT < 327. )
      iMsgRegion = 16;                /* Atlantic Ridge */
   else if ( dLatT >= 30.0 && dLatT < 35.0 && dLonT >= 313. && dLonT < 327. )
      iMsgRegion = 16;                /* Atlantic Ridge */
   else if ( dLatT >= 35.0 && dLatT < 40.0 && dLonT >= 320. && dLonT < 331. )
      iMsgRegion = 16;                /* Atlantic Ridge */
   else if ( dLatT >= 40.0 && dLatT < 61.0 && dLonT >= 320. && dLonT < 335. )
      iMsgRegion = 16;                /* Atlantic Ridge */
   else if ( dLatT >= 45. && dLatT < 66. && dLonT > 130. && dLonT < 240. )
      iMsgRegion = 5;                 /* Pacific event non-WC&ATWC AOR */
   else if ( dLatT >= 30. && dLatT < 45. && dLonT > 110. && dLonT < 240. )
      iMsgRegion = 5;                 /* Pacific event non-WC&ATWC AOR */
   else if ( dLatT >= 30. && dLatT < 32. && dLonT > 240. && dLonT < 255. )
      iMsgRegion = 5;                 /* Pacific event non-WC&ATWC AOR */
   else if ( dLatT >= 25. && dLatT < 30. && dLonT > 110. && dLonT < 255. )
      iMsgRegion = 5;                 /* Pacific event non-WC&ATWC AOR */
   else if ( dLatT >= 20. && dLatT < 25. && dLonT > 99. && dLonT < 261. )
      iMsgRegion = 5;                 /* Pacific event non-WC&ATWC AOR */
   else if ( dLatT >= 18.0 && dLatT < 20. && dLonT > 99. && dLonT < 263. )
      iMsgRegion = 5;                 /* Pacific event non-WC&ATWC AOR */
   else if ( dLatT >= 15. && dLatT < 18.0 && dLonT > 99. && dLonT < 271. )
      iMsgRegion = 5;                 /* Pacific event non-WC&ATWC AOR */
   else if ( dLatT >= 11. && dLatT < 15. && dLonT > 99. && dLonT < 275. )
      iMsgRegion = 5;                 /* Pacific event non-WC&ATWC AOR */
   else if ( dLatT >= 10. && dLatT < 11. && dLonT > 99. && dLonT < 276. )
      iMsgRegion = 5;                 /* Pacific event non-WC&ATWC AOR */
   else if ( dLatT >= 9. && dLatT < 10. && dLonT > 99. && dLonT < 276.5 )
      iMsgRegion = 5;                 /* Pacific event non-WC&ATWC AOR */
   else if ( dLatT >= 8.5 && dLatT < 9. && dLonT > 99. && dLonT < 278. )
      iMsgRegion = 5;                 /* Pacific event non-WC&ATWC AOR */
   else if ( dLatT >= 8.5 && dLatT < 9.2 && dLonT > 280. && dLonT < 282. )
      iMsgRegion = 5;                 /* Pacific event non-WC&ATWC AOR */
   else if ( dLatT >= 8.0 && dLatT < 8.5 && dLonT > 99. && dLonT < 282. )
      iMsgRegion = 5;                 /* Pacific event non-WC&ATWC AOR */
   else if ( dLatT >= -4. && dLatT < 8. && dLonT > 102. && dLonT < 285. )
      iMsgRegion = 5;                 /* Pacific event non-WC&ATWC AOR */
   else if ( dLatT >= -7. && dLatT < -4. && dLonT > 108. && dLonT < 285. )
      iMsgRegion = 5;                 /* Pacific event non-WC&ATWC AOR */
   else if ( dLatT >= -13. && dLatT < -7. && dLonT > 132. && dLonT < 285. )
      iMsgRegion = 5;                 /* Pacific event non-WC&ATWC AOR */
   else if ( dLatT >= -30. && dLatT < -13. && dLonT > 132. && dLonT < 295. )
      iMsgRegion = 5;                 /* Pacific event non-WC&ATWC AOR */
   else if ( dLatT >= -40. && dLatT < -30. && dLonT > 132. && dLonT < 292. )
      iMsgRegion = 5;                 /* Pacific event non-WC&ATWC AOR */
   else if ( dLatT >= -60. && dLatT < -40. && dLonT > 132. && dLonT < 290. )
      iMsgRegion = 5;                 /* Pacific event non-WC&ATWC AOR */
   else if ( dLatT >= -80. && dLatT < -60. && dLonT > 132. && dLonT < 295. )
      iMsgRegion = 5;                 /* Pacific event non-WC&ATWC AOR */
   else if ( dLatT >= -80.0 && dLatT < -20.0 && dLonT >= 0. && dLonT < 23. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= -20.0 && dLatT < 10.0 && dLonT >= 0. && dLonT < 20. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= 41.0 && dLatT < 48.0 && dLonT >= 0. && dLonT < 3. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= 48.0 && dLatT < 62.0 && dLonT >= 0. && dLonT < 10. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= 62.0 && dLatT < 78.0 && dLonT >= 0. && dLonT < 25. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= -80.0 && dLatT < -60.0 && dLonT >= 295. && dLonT <= 360. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= -60.0 && dLatT < -43.0 && dLonT >= 290. && dLonT <= 360. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= -43.0 && dLatT < -36.0 && dLonT >= 292. && dLonT <= 360. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= -36.0 && dLatT < -28.0 && dLonT >= 298. && dLonT <= 360. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= -28.0 && dLatT < -20.0 && dLonT >= 305. && dLonT <= 360. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= -20.0 && dLatT < -15.0 && dLonT >= 310. && dLonT <= 360. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= -15.0 && dLatT < -5.0 && dLonT >= 315. && dLonT <= 360. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= -5.0 && dLatT < 1.0 && dLonT >= 305. && dLonT <= 360. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= 1.0 && dLatT < 8.0 && dLonT >= 296. && dLonT <= 360. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= 9.2 && dLatT < 10.0 && dLonT >= 276.5 && dLonT < 282. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= 9.0 && dLatT < 9.2 && dLonT >= 276.5 && dLonT < 280. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= 8.5 && dLatT < 9.0 && dLonT >= 278. && dLonT < 280. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= 8.0 && dLatT < 10.0 && dLonT >= 282. && dLonT <= 360. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= 10.0 && dLatT < 11.0 && dLonT >= 276. && dLonT < 350. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= 11.0 && dLatT < 15.0 && dLonT >= 275. && dLonT < 350. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= 15.0 && dLatT < 18.0 && dLonT >= 271. && dLonT < 350. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= 18.0 && dLatT < 20.0 && dLonT >= 263. && dLonT < 350. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= 20.0 && dLatT < 27.0 && dLonT >= 261. && dLonT < 350. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= 27.0 && dLatT < 35.0 && dLonT >= 261. && dLonT < 355. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= 35.0 && dLatT < 41.0 && dLonT >= 275. && dLonT < 355. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= 41.0 && dLatT < 42.0 && dLonT >= 275. && dLonT <= 360. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= 42.0 && dLatT < 50.0 && dLonT >= 280. && dLonT <= 360. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= 50.0 && dLatT < 67.0 && dLonT >= 265. && dLonT <= 360. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= 67.0 && dLatT < 81.0 && dLonT >= 277. && dLonT <= 360. )
      iMsgRegion = 17;                 /* Atlantic Basin */
   else if ( dLatT >= 66.0 )
      iMsgRegion = 9;                  /* Arctic Ocean */
   else
      iMsgRegion = 20;                 /* Not in any of the above */
   return iMsgRegion;
}

      /******************************************************************
       *                       GetSeisInfo ()                           *
       *                                                                *
       *  This function takes an input seismometer type indicator and   *
       *  returns the type in a string and flat response length.        *
       *                                                                *
       *  Arguments:                                                    *
       *   pszNetID         Seismometer Network ID                      *
       *                                                                *
       *  Return:                                                       *
       *   String naming the operating agency.                          *
       *                                                                *
       ******************************************************************/
       
char *GetSeisInfo( int iSeisType, double *dFlatResp )
{
   *dFlatResp = 1.;
   if ( iSeisType == 103 )
      return( "Short period/Low gain" );
   else if ( iSeisType == 102 )
      return( "Short period/Medium gain" );
   else if ( iSeisType == 101 )
      return( "Short period/High gain" );
   else if ( iSeisType == 100 )
      return( "Short period/Uncalibrated" );
   else if ( iSeisType == 52 )
      return( "Long period/Low gain" );
   else if ( iSeisType == 51 )
      return( "Long period/High gain" );
   else if ( iSeisType == 50 )
      return( "Long period/Uncalibrated" );
   else if ( iSeisType == 30 )
      return( "Accelerometer/FBA ES-T" );
   else if ( iSeisType == 20 )
      return( "Broadband/Unknown Type" );
   else if ( iSeisType == 1 )
   {
      *dFlatResp = 360.;
      return( "Streckheisen STS-1" );
   }
   else if ( iSeisType == 2 )
   {
      *dFlatResp = 130.;
      return( "Streckheisen STS-2" );
   }
   else if ( iSeisType == 3 )
   {
      *dFlatResp = 30.;
      return( "Guralp CMG-3NSN" );
   }
   else if ( iSeisType == 4 )
   {
      *dFlatResp = 100.;
      return( "Guralp CMG-3T - 120" );
   }   
   else if ( iSeisType == 5 )
   {
      *dFlatResp = 360.;
      return( "Geotech KS360i" );
   }     
   else if ( iSeisType == 6 )
   {
      *dFlatResp = 360.;
      return( "Geotech KS54000" );
   }     
   else if ( iSeisType == 7 )
   {
      *dFlatResp = 30.;
      return( "Guralp CMG-3" );
   } 
   else if ( iSeisType == 8 )
   {
      *dFlatResp = 30.;
      return( "Guralp CMG-40T" );
   } 
   else if ( iSeisType == 9 )
   {
      *dFlatResp = 30.;
      return( "Guralp CMG-3TNSN" );
   } 
   else if ( iSeisType == 10 )
   {
      *dFlatResp = 20.;
      return( "Geotech KS-10" );
   }
   else if ( iSeisType == 11 )
   {
      *dFlatResp = 30.;
      return( "Guralp CMG-3ESP_30" );
   }
   else if ( iSeisType == 12 )
   {
      *dFlatResp = 60.;
      return( "Guralp CMG-3ESP_60" );
   } 
   else if ( iSeisType == 13 )
   {
      *dFlatResp = 40.;
      return( "Trillium" );
   } 
   else if ( iSeisType == 14 )
   {
      *dFlatResp = 120.;
      return( "Guralp CMG-3ESP_120" );
   } 
   else if ( iSeisType == 15 )
   {
      *dFlatResp = 20.;
      return( "Guralp CMG40T_20" );
   } 
   else if ( iSeisType == 16 )
   {
      *dFlatResp = 360.;
      return( "Guralp CMG3T_360" );
   } 
   else if ( iSeisType == 17 )
   {
      *dFlatResp = 120.;
      return( "Geotech 2000_120" );
   } 
   else if ( iSeisType == 18 )
   {
      *dFlatResp = 30.;
      return( "Guralp 6TD" );
   } 
   else if ( iSeisType == 19 )
   {
      *dFlatResp = 120.;
      return( "Trillium 120" );
   } 
   else if ( iSeisType == 20 )
   {
      *dFlatResp = 240.;
      return( "Trillim 240" );
   } 
   else return( "" );
}

      /******************************************************************
       *                     IsItDangerous()                            *
       *                                                                *
       * This function determines whether or not an earthquake occurred *
       * in an area of the Pacific which has historically triggerred    *
       * tsunamis potentially dangerous to the WC&ATWC AOR (i.e. N.     *
       * Honshu, Kurils, Kamchatka, Komandorski?, Chile, S. Peru, or    *
       * Hawaii; WC&ATWC AOR not included; this is assumed to be        *
       * dangerous).                                                    *
       *                                                                *
       *  Arguments:                                                    *
       *   dLat             Latitude +=N, -=S                           *
       *   dLon             Longitude +=E, -=W                          *
       *                                                                *
       *  Return:                                                       *
       *   1 if quake in potential danger area for AOR, 0 if not        *
       *                                                                *
       ******************************************************************/
int IsItDangerous (double dLat, double dLon)
{
if (dLat >= 30. && dLat <= 35. && dLon >= 138. && dLon <= 146.) return 1;
if (dLat >= 36. && dLat <= 38. && dLon >= 141. && dLon <= 150.) return 1;
if (dLat >= 38. && dLat <= 40. && dLon >= 142. && dLon <= 150.) return 1;
if (dLat >= 40. && dLat <= 42. && dLon >= 142. && dLon <= 154.) return 1;
if (dLat >= 42. && dLat <= 43. && dLon >= 143. && dLon <= 155.) return 1;
if (dLat >= 43. && dLat <= 44. && dLon >= 145. && dLon <= 158.) return 1;
if (dLat >= 44. && dLat <= 45. && dLon >= 147. && dLon <= 160.) return 1;
if (dLat >= 45. && dLat <= 46. && dLon >= 148. && dLon <= 162.) return 1;
if (dLat >= 46. && dLat <= 47. && dLon >= 151. && dLon <= 164.) return 1;
if (dLat >= 47. && dLat <= 48. && dLon >= 152. && dLon <= 165.) return 1;
if (dLat >= 48. && dLat <= 49. && dLon >= 153. && dLon <= 167.) return 1;
if (dLat >= 49. && dLat <= 50. && dLon >= 154. && dLon <= 170.) return 1;
if (dLat >= 50. && dLat <= 51. && dLon >= 155. && dLon <= 170.) return 1;
if (dLat >= 51. && dLat <= 52. && dLon >= 156. && dLon <= 170.) return 1;
if (dLat >= 52. && dLat <= 53. && dLon >= 157. && dLon <= 170.) return 1;
if (dLat >= 53. && dLat <= 54. && dLon >= 158. && dLon <= 170.) return 1;
if (dLat >= 54. && dLat <= 55. && dLon >= 160. && dLon <= 170.) return 1;
if (dLat >= 55. && dLat <= 56. && dLon >= 161. && dLon <= 170.) return 1;
if (dLat >= 56. && dLat <= 60. && dLon >= 161. && dLon <= 170.) return 1;
if (dLat >= 60. && dLat <= 61. && dLon >= 165. && dLon <= 170.) return 1;
if (dLat >=-18. && dLat <=-16. && dLon >= -80. && dLon <= -71.) return 1;
if (dLat >=-23. && dLat <=-18. && dLon >= -78. && dLon <= -69.) return 1;
if (dLat >=-27. && dLat <=-23. && dLon >= -76. && dLon <= -70.) return 1;
if (dLat >=-35. && dLat <=-27. && dLon >= -77. && dLon <= -71.) return 1;
if (dLat >=-37. && dLat <=-35. && dLon >= -78. && dLon <= -72.) return 1;
if (dLat >=-42. && dLat <=-37. && dLon >= -79. && dLon <= -73.) return 1;
if (dLat >=-50. && dLat <=-42. && dLon >= -80. && dLon <= -74.) return 1;
if (dLat >= 18. && dLat <= 22. && dLon >=-158. && dLon <=-153.) return 1;
return 0;
}

      /******************************************************************
       *                     IsItInHawaii()                             *
       *                                                                *
       *  Determines if a quake location is near the Hawaiian Islands.  *
       *                                                                *
       *  Arguments:                                                    *
       *   dLat             Latitude +=N, -=S                           *
       *   dLon             Longitude +=E, -=W                          *
       *                                                                *
       *  Return:                                                       *
       *   1 if quake near Hawaii, 0 if not                             *
       *                                                                *
       ******************************************************************/

int IsItInHawaii( double dLat, double dLon )
{
   if ( dLat < 30. && dLat > 15. && dLon > -170. && dLon < -150. ) return 1;
   else                                                            return 0;
}

      /******************************************************************
       *                            itoaX()                             *
       *                                                                *
       *  Convert integer to character string.                          *
       *                                                                *
       *  From Stu at PTWC. (Added for compatibility with Solaris.)     *
       *                                                                *
       *  May, 2013: Update from Larry Baker.                           *
       *  July, 2008: Added code to account for negative #s (Whitmore). *
       *  June, 2007: Updated with new code from Stu.                   *
       *                                                                *
       *  Arguments:                                                    *
       *   iNum             Integer to convert                          *
       *   pszNum           String pointer with int converted to char   *
       *                                                                *
       *  Return:                                                       *
       *   Pointer to character string (pszNum)                         *
       *                                                                *
       ******************************************************************/

char *itoaX( int iNum, char *pszNum )
{
   int   i;
//   int   iDigit;      /* Values to convert */
//   int   iNumAbs;     /* Absolute value of iNum */
//   int   iPower;      /* Number of characters necessary in converted string */
//   int   iStart;
   char *s;
 
   if ( pszNum == (char *) NULL )
   {
      logit( "", "No memory allocated for pszNum in itoaX\n");
      return NULL;
   }
 
   s = pszNum;
   if ( iNum == 0 )
   {
      *s++ = '0';
      *s = '\0';
   }
   else
   {
      if ( iNum < 0 )
      {
         *s++ = '-';
         iNum = -iNum;
      }
      for ( i=iNum; i>0; i/=10 ) s++;
      *s-- = '\0';
      for ( ; iNum>0; iNum/=10 ) *s-- = '0' + (iNum % 10);          
   }
   return pszNum;

//   iNumAbs = labs( iNum );
//   iPower = (int) log10( (double) iNumAbs );  
 
//   if ( iNumAbs )
//   {
//      iPower = (int) log10( (double) iNumAbs );
//      if ( iNum < 0 ) iPower += 1;
//      iStart = 0;
//      if ( iNum < 0 ) iStart = 1;
//      if ( iNum < 0 ) pszNum[0] = '-';
//      for ( i=iStart; i<=iPower; i++ )
//      {
//         iDigit = iNumAbs%10;
//         pszNum[iPower-i+iStart]= iDigit + 48;
//         iNumAbs=iNumAbs/10;
//      }
//   }
//   else
//   {
//      iPower = 0;
//      pszNum[iPower] = '0';
//   }
 
//  pszNum[iPower+1] = '\0';
//  return pszNum; 
}

      /******************************************************************
       *                          LLConv()                              *
       *                                                                *
       *  This function converts a character string to a double         *
       *  precision floating point value and accounts for south and     *
       *  west lat/lons.                                                *
       *                                                                *
       *  Arguments:                                                    *
       *   psz              Input lat/lon string with n,s,...           *
       *                                                                *	   
       *  Returns:          Converted lat or lon (double)               *
       *                                                                *
       ******************************************************************/
	   
double LLConv( char *psz )
{
   double  dTemp;

   dTemp = atof( psz );
   if ( strchr( psz, 's' ) || strchr( psz, 'S' ) || strchr( psz, 'w' ) ||
        strchr( psz, 'W' ) ) dTemp *= -1.0;
   return dTemp;
}

      /******************************************************************
       *                      LoadTTT( )                                *
       *                                                                *
       *  This function reads in warning point data files and fills the *
       *  Tsunami Travel Time array.                                    *
       *                                                                *
       *  Jan., 2014: Now use new text file format output from database.*
       *                                                                *
       *  Arguments:                                                    *
       *   pszFile          Tsunami Travel Time warning points file     *
       *   ttt              Tsunami travel time warning point array     *
       *   iNumSites        Number of sites in file                     *
       *   iFormat          Flag showing what fields this file contains *
       *                    1 - PTWC AOR file format                    *	   
       *                    2 - Arctic file format                      *	   
       *                    3 - NTWC Pac/Atl AOR file format            *	   
       *                                                                *	   
       *  Returns:          1 if ok; 0 if problem                       *
       *                                                                *
       ******************************************************************/	   
int LoadTTT( char *pszFile, TSUTRAVTIME *ttt, int iNumSites, int iFormat )
{
   FILE   *hFile;                               /* Warning pt. file handle */             
   int     i, j, jLast=0, jCnt=0;
   char    szLine[512];                         /* Line from ttt site file */
   char    szRelLoc[64];                        /* Relative loc in file */
   char    szSiteType[4], szLat[16], szLon[16]; /* For reading ttt site files */
    
/* Load up Tsunami Travel Time array from data file. */
   hFile = fopen( pszFile, "r" );
   if ( hFile == NULL )
   {
      logit( "t", "TTT file not opened - %s\n", pszFile );
      //      MessageBox( NULL, "Warning point data file read failed\n", NULL, 0 );
      return 0;
   }
   else
   {
      for ( i=0; i<iNumSites; i++ )                 /* Loop through all sites */
      {
         fgets( &szLine[0], 456, hFile );
/* For parsing, pad szLine with blanks (-1 for EOL char) */
         for ( j=(int)strlen( szLine )-1; j<456; j++ ) szLine[j] = ' ';
         szLine[456] = '\0';           
         if ( iFormat == 1 || iFormat == 2 )
         {
            jLast = -1;                /* Used to set end of new string to \0 */
            jCnt  =  0;          /* Keeps track of where in string to extract */
            for ( j=0; j<32; j++ )                       /* Extract Site Name */
            {
               ttt[i].szSiteName[j] = szLine[j+jCnt];
               if ( ttt[i].szSiteName[j] != ' ' ) jLast = j;         
            }
            ttt[i].szSiteName[jLast+1] = '\0';
            jCnt += 33;
            jLast = -1;
            for ( j=0; j<32; j++ )                    /* Extract Site Country */
            {
               ttt[i].szSiteCountry[j] = szLine[j+jCnt];
               if ( ttt[i].szSiteCountry[j] != ' ' ) jLast = j;         
            }
            ttt[i].szSiteCountry[jLast+1] = '\0';
            jCnt += 33;
            jLast = -1;
            for ( j=0; j<2; j++ )                /* Extract Site Country Code */
            {
               ttt[i].szSiteCountryCode[j] = szLine[j+jCnt];
               if ( ttt[i].szSiteCountryCode[j] != ' ' ) jLast = j;         
            }
            ttt[i].szSiteCountryCode[jLast+1] = '\0';
            jCnt += 3;
            jLast = -1;
            if ( iFormat >= 2 )
            { 
               for ( j=0; j<32; j++ )                   /* Extract Site State */
               {
                  ttt[i].szSiteState[j] = szLine[j+jCnt];
                  if ( ttt[i].szSiteState[j] != ' ' ) jLast = j;         
               }
               ttt[i].szSiteState[jLast+1] = '\0';
               jCnt += 33;
               jLast = -1;
            }
            if ( iFormat >= 2 )
            { 
               for ( j=0; j<2; j++ )               /* Extract Site State Code */
               {
                  ttt[i].szSiteStateCode[j] = szLine[j+jCnt];
                  if ( ttt[i].szSiteStateCode[j] != ' ' ) jLast = j;         
               }
               ttt[i].szSiteStateCode[jLast+1] = '\0';
               jCnt += 3;
               jLast = -1;
            }
         }
         else if ( iFormat == 3 )
         {
            jLast = -1;                /* Used to set end of new string to \0 */
            jCnt  =  0;          /* Keeps track of where in string to extract */
            for ( j=0; j<24; j++ )                       /* Extract Site Name */
            {
               ttt[i].szSiteName[j] = szLine[j+jCnt];
               if ( ttt[i].szSiteName[j] != ' ' ) jLast = j;         
            }
            ttt[i].szSiteName[jLast+1] = '\0';
            jCnt += 24;
            jLast = -1;
            for ( j=0; j<18; j++ )                    /* Extract Site Country */
            {
               ttt[i].szSiteCountry[j] = szLine[j+jCnt];
               if ( ttt[i].szSiteCountry[j] != ' ' ) jLast = j;         
            }
            ttt[i].szSiteCountry[jLast+1] = '\0';
            jCnt += 18;
            jLast = -1;
            for ( j=0; j<2; j++ )                /* Extract Site Country Code */
            {
               ttt[i].szSiteCountryCode[j] = szLine[j+jCnt];
               if ( ttt[i].szSiteCountryCode[j] != ' ' ) jLast = j;         
            }
            ttt[i].szSiteCountryCode[jLast+1] = '\0';
            jCnt += 3;
            jLast = -1;
            if ( iFormat >= 2 )
            { 
               for ( j=0; j<18; j++ )                   /* Extract Site State */
               {
                  ttt[i].szSiteState[j] = szLine[j+jCnt];
                  if ( ttt[i].szSiteState[j] != ' ' ) jLast = j;         
               }
               ttt[i].szSiteState[jLast+1] = '\0';
               jCnt += 18;
               jLast = -1;
            }
            if ( iFormat >= 2 )
            { 
               for ( j=0; j<2; j++ )               /* Extract Site State Code */
               {
                  ttt[i].szSiteStateCode[j] = szLine[j+jCnt];
                  if ( ttt[i].szSiteStateCode[j] != ' ' ) jLast = j;         
               }
               ttt[i].szSiteStateCode[jLast+1] = '\0';
               jCnt += 3;
               jLast = -1;
            }
         }
         for ( j=0; j<4; j++ )                           /* Extract Site Code */
         {
            ttt[i].szSiteCode[j] = szLine[j+jCnt];
            if ( ttt[i].szSiteCode[j] != ' ' ) jLast = j;         
         }
         ttt[i].szSiteCode[jLast+1] = '\0';
         jCnt += 5;
         jLast = -1;
         ttt[i].cOcean = szLine[jCnt];                  /* Extract Ocean Code */
         jCnt += 2;
         for ( j=0; j<2; j++ )                           /* Extract Site Type */
         {
            szSiteType[j] = szLine[j+jCnt];
            if ( szSiteType[j] != ' ' ) jLast = j;         
         }
         szSiteType[jLast+1] = '\0';
         ttt[i].sSiteType = (short) atoi( szSiteType );
         jCnt += 3;
         jLast = -1;
         for ( j=0; j<5; j++ )                /* Extract Site TG Abbreviation */
         {
            ttt[i].szSiteTGAbbr[j] = szLine[j+jCnt];
            if ( ttt[i].szSiteTGAbbr[j] != ' ' ) jLast = j;         
         }
         ttt[i].szSiteTGAbbr[jLast+1] = '\0';
         jCnt += 6;
         jLast = -1;
         for ( j=0; j<8; j++ )                       /* Extract Site Latitude */
         {
            szLat[j] = szLine[j+jCnt];
            if ( szLat[j] != ' ' ) jLast = j;         
         }
         szLat[jLast+1] = '\0';
         ttt[i].fSiteLatLon[1] = (float) atof( szLat );
         jCnt += 9;
         jLast = -1;
         for ( j=0; j<9; j++ )                      /* Extract Site Longitude */
         {
            szLon[j] = szLine[j+jCnt];
            if ( szLon[j] != ' ' ) jLast = j;         
         }
         szLon[jLast+1] = '\0';
         ttt[i].fSiteLatLon[0] = (float) atof( szLon );
         if ( ttt[i].fSiteLatLon[0] < 0. ) ttt[i].fSiteLatLon[0] += 360.;
         jCnt += 10;
         jLast = -1;
         if ( iFormat >= 3 )
         { 
            for ( j=0; j<1; j++ )                 /* Extract Break Point Type */
            {
               szSiteType[j] = szLine[j+jCnt];
               if ( szSiteType[j] != ' ' ) jLast = j;         
            }
            szSiteType[jLast+1] = '\0';
            ttt[i].sBreakPoint = (short) atoi( szSiteType );
            jCnt += 2;
            jLast = -1;
         }
         if ( iFormat >= 3 )
         { 
            for ( j=0; j<6; j++ )    /* Extract Relative Geographic Indicator */
            {
               szLon[j] = szLine[j+jCnt];
               if ( szLon[j] != ' ' ) jLast = j;         
            }
            szLon[jLast+1] = '\0';
            ttt[i].fPlaceRelBkPts = (float) atof( szLon );
            jCnt += 7;
            jLast = -1;
         }
         if ( iFormat >= 3 )
         { 
            for ( j=0; j<38; j++ )                    /* Extract Marine Codes */
            {
               ttt[i].szMarineZones[j] = szLine[j+jCnt];
               if ( ttt[i].szMarineZones[j] != ' ' ) jLast = j;         
            }
            ttt[i].szMarineZones[jLast+1] = '\0';
            jCnt += 38;
            jLast = -1;
         }
         if ( iFormat >= 3 )
         { 
            for ( j=0; j<56; j++ )                    /* Extract Public Zones */
            {
               ttt[i].szGeoCodes[j] = szLine[j+jCnt];
               if ( ttt[i].szGeoCodes[j] != ' ' ) jLast = j;         
            }
            ttt[i].szGeoCodes[jLast+1] = '\0';
            jCnt += 56;
            jLast = -1;
         }
         if ( iFormat >= 3 )
         { 
            for ( j=0; j<32; j++ )               /* Extract relative location */
            {
               szRelLoc[j] = szLine[j+jCnt];
               if ( szRelLoc[j] != ' ' ) jLast = j;         
            }
            szRelLoc[jLast+1] = '\0';
         }
         if ( iFormat >= 3 )          /* Create location strings for messages */
         { 
/* Create long name in CAPS */
            jCnt = 0;
            for ( j=0; j<(int) strlen( ttt[i].szSiteName ); j++ )
            {                                                    /* Site Name */
               ttt[i].szSiteNameCapLong[jCnt] = (char)toupper( ttt[i].szSiteName[j] );
               jCnt++;
            }
            ttt[i].szSiteNameCapLong[jCnt] = ' ';
            jCnt++;
            for ( j=0; j<(int) strlen( ttt[i].szSiteState ); j++ )
            {                                                   /* Site State */
               ttt[i].szSiteNameCapLong[jCnt] = (char)toupper( ttt[i].szSiteState[j] );
               jCnt++;
            }
            if ( (int) strlen( szRelLoc ) > 0 )
            {
               sprintf( &ttt[i].szSiteNameCapLong[jCnt], " /WHICH IS LOCATED " );
               jCnt += 19;
            }
            for ( j=0; j<(int) strlen( szRelLoc ); j++ )
            {                           
               ttt[i].szSiteNameCapLong[jCnt] = (char)toupper( szRelLoc[j] );
               jCnt++;
            }
            if ( (int) strlen( szRelLoc ) > 0 )
            {
               ttt[i].szSiteNameCapLong[jCnt] = '/';
               jCnt++;
            }
            ttt[i].szSiteNameCapLong[jCnt] = '\0';
/* Create long name in lower case */
            jCnt = 0;
            for ( j=0; j<(int) strlen( ttt[i].szSiteName ); j++ )
            {                                                    /* Site Name */
               ttt[i].szSiteNameLowLong[jCnt] = ttt[i].szSiteName[j];
               jCnt++;
            }
            if ( (int) strlen( ttt[i].szSiteState ) > 0 )
            {
               ttt[i].szSiteNameLowLong[jCnt] = ',';
               jCnt++;
            }
            ttt[i].szSiteNameLowLong[jCnt] = ' ';
            jCnt++;
            for ( j=0; j<(int) strlen( ttt[i].szSiteState ); j++ )
            {                                                   /* Site State */
               ttt[i].szSiteNameLowLong[jCnt] = ttt[i].szSiteState[j];
               jCnt++;
            }
            if ( (int) strlen( szRelLoc ) > 0 )
            {
               sprintf( &ttt[i].szSiteNameLowLong[jCnt], " (" );
               jCnt += 2;
            }
            for ( j=0; j<(int) strlen( szRelLoc ); j++ )
            {                                            /* Relative Location */
               ttt[i].szSiteNameLowLong[jCnt] = szRelLoc[j];
               jCnt++;
            }
            if ( (int) strlen( szRelLoc ) > 0 )
            {
               ttt[i].szSiteNameLowLong[jCnt] = ')';
               jCnt++;
            }
            ttt[i].szSiteNameLowLong[jCnt] = '\0';
         }
      }
      logit( "t", "%d sites read in for %s\n", i, pszFile );
      fclose( hFile );
   }   
   return 1;    
}

      /******************************************************************
       *                          PadBlandsL()                          *
       *                                                                *
       *  This function adds leading blanks to a character  string.     *
       *                                                                *
       *  Arguments:                                                    *
       *   iNumDigits       # digits desired in output                  *
       *   pszString        String to be padded with blanks             *
       *                                                                *
       ******************************************************************/

void PadBlanksL( int iNumDigits, char *pszString )
{
   int     i, iStringLen;
   char    szTemp1[128], szTemp2[128];
   
   strcpy( szTemp1, pszString );
   strcpy( szTemp2, szTemp1 );
   iStringLen = (int)strlen( szTemp1 );
   if ( iNumDigits-iStringLen > 0 )                /* Number of blanks to add */
   {
      for ( i=0; i<iNumDigits-iStringLen; i++ ) szTemp1[i] = ' ';
      for ( i=iNumDigits-iStringLen; i<iNumDigits; i++ )
         szTemp1[i] = szTemp2[i-(iNumDigits-iStringLen)];
      szTemp1[iNumDigits] = '\0';                /* Add NULL character to end */
      strcpy( pszString, szTemp1 );
   }
}

      /******************************************************************
       *                           PadZeroes()                          *
       *                                                                *
       *  This function adds leading zeroes to a numeric character      *
       *  string. (10 characters maximum in string).                    *
       *                                                                *
       *  Arguments:                                                    *
       *   iNumDigits       # digits desired in output                  *
       *   pszString        String to be padded with zeroes             *
       *                                                                *
       ******************************************************************/
	   
void PadZeroes( int iNumDigits, char *pszString )
{
   int     i, iStringLen;
   char    szTemp1[128], szTemp2[128];

   strcpy( szTemp1, pszString );
   strcpy( szTemp2, szTemp1 );
   iStringLen = (int)strlen( szTemp1 );
   if ( iNumDigits-iStringLen > 0 )     /* Number of zeroes to add */
   {
      for ( i=0; i<iNumDigits-iStringLen; i++ ) szTemp1[i] = '0';
      for ( i=iNumDigits-iStringLen; i<iNumDigits; i++ ) 
         szTemp1[i] = szTemp2[i-(iNumDigits-iStringLen)];
      szTemp1[iNumDigits] = '\0';       /* Add NULL character to end */
      strcpy( pszString, szTemp1 );
   }
}

      /******************************************************************
       *                           PadZeroesR()                          *
       *                                                                *
       *  This function adds leading zeroes to a numeric character      *
       *  string. (10 characters maximum in string).                    *
       *                                                                *
       *  Arguments:                                                    *
       *   iNumDigits       # digits desired in output                  *
       *   pszString        String to be padded with zeroes             *
       *                                                                *
       ******************************************************************/
void PadZeroesR( int iNumDigits, char *pszString )
{
   int     i, iStringLen;    
   char    szTemp1[128];

   strcpy( szTemp1, pszString );
   iStringLen = (int)strlen( szTemp1 );
   if ( iNumDigits-iStringLen > 0 ) /* Number of zeroes to add */
   {
      for ( i=iStringLen; i<iNumDigits; i++ ) szTemp1[i] = '0';
      szTemp1[iNumDigits] = '\0';   /* Add NULL character to end */
      strcpy( pszString, szTemp1 );
   }
}

      /******************************************************************
       *                          PointToEpi()                          *
       *                                                                *
       *  This function returns a latitude and longitude given a point, *
       *  an azimuth and distance from that point.  The calculated lat  *
       *  and lon is delta degrees away from input lat and lon.         *
       *  Spherical trigonometric relations are used in the             *
       *  computations.                                                 *
       *                                                                *
       *  July, 2011: Modified code so that argument pointer is used    *
       *              instead of structure return.                      *
       *                                                                *
       *  Arguments:                                                    *
       *   pll1             Input geocentric lat/lon (on az, from pt.)  *
       *   pazidelt         Distance/az from point to output point      *
       *                                                                *
       *  Return:                                                       *
       *   LATLON structure geocentric location of new point.           *
       *                                                                *
       ******************************************************************/
	   
void PointToEpi( LATLON *pll1, AZIDELT *pazidelt, LATLON *pll2 )
{
   double   dAz;                  /* Azimuth from input point */
   double   dSinamb, dSinapb, dCosamb, dCosapb, dA, dB;

   dAz = pazidelt->dAzimuth;      /* Make this local as it may change */
   if ( dAz > PI ) dAz = TWOPI - dAz;
   dSinamb = sin( (pll1->dLat - pazidelt->dDelta) / 2.0 );
   dSinapb = sin( (pll1->dLat + pazidelt->dDelta) / 2.0 );
   dCosamb = cos( (pll1->dLat - pazidelt->dDelta) / 2.0 );
   dCosapb = cos( (pll1->dLat + pazidelt->dDelta) / 2.0 );
   dA = atan( dSinamb / (dSinapb*tan( dAz/2.0 )) ) + 
        atan( dCosamb / (dCosapb*tan( dAz/2.0 )) );
   if (dA < 0.) dA += PI;  
   dB = 2.0 * atan( dCosamb / (dCosapb*tan( dAz / 2.0 )) ) - dA;
   pll2->dLat = acos( (cos( pazidelt->dDelta ) * cos( pll1->dLat )) +
              (sin( pazidelt->dDelta ) * sin( pll1->dLat ) * cos( dAz )) );
   if ( pazidelt->dAzimuth < PI ) pll2->dLon = pll1->dLon + dB;
   else                           pll2->dLon = pll1->dLon - dB;
   while ( pll2->dLon >= TWOPI ) pll2->dLon -= TWOPI;  /* Just a check */
   while ( pll2->dLon < 0.0 )    pll2->dLon += TWOPI;
   while ( pll2->dLat >= PI )    pll2->dLat -= PI;
   while ( pll2->dLat < 0.0 )    pll2->dLat += PI;
   return;
}   

      /******************************************************************
       *                      ReadThreatFile()                          *
       *                                                                *
       * Read threat data base text file and determine if this          *
       * hypocenter is included in the file.  If so, read in            *
       * appropriate information (headlines, UGCs, etc.).               *
       *                                                                *
       * June, 2010: Added depth to threat file                         *
       *                                                                *
       *  Arguments:                                                    *
       *   pszThreatFile    File with threat information                *
       *   pHypo            Hypocenter parameters                       *
       *   pThreat          Text from threat data file                  *
       *   piType           w/w/a type (iWWType)                        *
       *   piDetails        Text inluded in file                        *
       *   piInitial        1=initial w/w/a; 2=final                    *
       *                                                                *
       *  Return:                                                       *
       *   int - 1->w/w/a info in this file; 0->hypo not found in file  *
       *                                                                *
       ******************************************************************/

int ReadThreatFile( char *pszThreatFile, HYPO *pHypo, THREAT *pThreat,
                    int *piType, int *piDetails, int *piInitial, int iWarn1[],
                    int iWarn2[], int iAdv1[], int iAdv2[], int iWatch1[],
                    int iWatch2[] )
{
   FILE   *hFile;
   double  dLat, dLon;                               /* Hypo. loc.-geographic */
   double  dLatN, dLatS, dLonW, dLonE, dMagH, dMagL, dDep; /* Special params */
   int     i, iNum, iTemp1, iTemp2, iTemp, iTemp3;
   int     iInFile;        /* 1->This event is in the file; 0->no */
   char    szTemp[256];
   
/* Put hypo. location into local variables (and make 0->360) */
   iInFile = 0;
   dLat = pHypo->llEpiGG.dLat;
   dLon = pHypo->llEpiGG.dLon;
   if ( dLon < 0. ) dLon += 360.;
   
/* Initialize Threat structure strings */
   for ( i=0; i<MAX_NUM_WWA_SEGS; i++ )
      memset( pThreat->szWarnUGC[i], 0, sizeof( pThreat->szWarnUGC[i] ) );
   for ( i=0; i<MAX_NUM_WWA_SEGS; i++ )
      memset( pThreat->szWarnHeadLineStd[i], 0, sizeof( pThreat->szWarnHeadLineStd[i] ) );
   for ( i=0; i<MAX_NUM_WWA_SEGS; i++ )
      memset( pThreat->szWarnHeadLineSpn[i], 0, sizeof( pThreat->szWarnHeadLineSpn[i] ) );
   for ( i=0; i<MAX_NUM_WWA_SEGS; i++ )
      memset( pThreat->szWarnSegArea[i], 0, sizeof( pThreat->szWarnSegArea[i] ) );
   for ( i=0; i<MAX_NUM_WWA_SEGS; i++ )
      memset( pThreat->szWarnPager[i], 0, sizeof( pThreat->szWarnPager[i] ) );
   for ( i=0; i<MAX_NUM_WWA_SEGS; i++ )
      memset( pThreat->szAdvUGC[i], 0, sizeof( pThreat->szAdvUGC[i] ) );
   for ( i=0; i<MAX_NUM_WWA_SEGS; i++ )
      memset( pThreat->szAdvHeadLineStd[i], 0, sizeof( pThreat->szAdvHeadLineStd[i] ) );
   for ( i=0; i<MAX_NUM_WWA_SEGS; i++ )
      memset( pThreat->szAdvHeadLineSpn[i], 0, sizeof( pThreat->szAdvHeadLineSpn[i] ) );
   for ( i=0; i<MAX_NUM_WWA_SEGS; i++ )
      memset( pThreat->szAdvSegArea[i], 0, sizeof( pThreat->szAdvSegArea[i] ) );
   for ( i=0; i<MAX_NUM_WWA_SEGS; i++ )
      memset( pThreat->szAdvPager[i], 0, sizeof( pThreat->szAdvPager[i] ) );
   for ( i=0; i<MAX_NUM_WWA_SEGS; i++ )
      memset( pThreat->szWatchUGC[i], 0, sizeof( pThreat->szWatchUGC[i] ) );
   for ( i=0; i<MAX_NUM_WWA_SEGS; i++ )
      memset( pThreat->szWatchHeadLineStd[i], 0, sizeof( pThreat->szWatchHeadLineStd[i] ) );
   for ( i=0; i<MAX_NUM_WWA_SEGS; i++ )
      memset( pThreat->szWatchHeadLineSpn[i], 0, sizeof( pThreat->szWatchHeadLineSpn[i] ) );
   for ( i=0; i<MAX_NUM_WWA_SEGS; i++ )
      memset( pThreat->szWatchSegArea[i], 0, sizeof( pThreat->szWatchSegArea[i] ) );
   for ( i=0; i<MAX_NUM_WWA_SEGS; i++ )
      memset( pThreat->szWatchPager[i], 0, sizeof( pThreat->szWatchPager[i] ) );
   for ( i=0; i<MAX_NUM_INFO_SEGS; i++ )
      memset( pThreat->szInfoAreaStd[i], 0, sizeof( pThreat->szInfoAreaStd[i] ) );
   for ( i=0; i<MAX_NUM_INFO_SEGS; i++ )
      memset( pThreat->szInfoAreaSpn[i], 0, sizeof( pThreat->szInfoAreaSpn[i] ) );
   memset( pThreat->szCancelUGC, 0, sizeof( pThreat->szCancelUGC ) );
   memset( pThreat->szCancelHeadLineStd, 0, sizeof( pThreat->szCancelHeadLineStd ) );
   memset( pThreat->szCancelHeadLineSpn, 0, sizeof( pThreat->szCancelHeadLineSpn ) );
   memset( pThreat->szCancelSegArea, 0, sizeof( pThreat->szCancelSegArea ) );
   memset( pThreat->szSpecArea, 0, sizeof( pThreat->szSpecArea ) );

/* Read threat file to see if this event fits in any special categories or
   if pre-computed w/w/a regions were defined */
   if ( (hFile = fopen( pszThreatFile, "r" )) != NULL )
   {
      while ( !feof( hFile ) )
      {
         fscanf( hFile, "%d\n", &iNum );
         for ( i=0; i<iNum; i++ )
         {
            fscanf( hFile, "%lf %lf %lf %lf %lf %lf %lf\n",
             &dLatN, &dLatS, &dLonW, &dLonE, &dMagL, &dMagH, &dDep);
            if ( dLonW < 0. ) dLonW += 360.;
            if ( dLonE < 0. ) dLonE += 360.;
            if ( dLat <= dLatN && dLat >= dLatS &&
                 dLon <= dLonE && dLon >= dLonW &&
                 pHypo->dPreferredMag <= dMagH  &&
                 pHypo->dPreferredMag >= dMagL  &&
                 pHypo->dDepth <= dDep )
               iInFile = 1;
         }						    
         if ( iInFile == 1 )
         {        /* Hypo in this region, so read into structure and break */
            fscanf( hFile, "%d\n", piDetails );
            fscanf( hFile, "%d\n", piInitial );
            fscanf( hFile, "%d\n", &iNum );       /* # Warning Regions */
            if ( iNum > MAX_NUM_WWA_SEGS )
            {
               logit( "t", "Too many warning regs in threat file-%d\n", iNum );
               fclose( hFile );
               return( 0 ) ;
            }
            for ( i=0; i<iNum; i++ )
            {
               if ( *piDetails == 1 ) fscanf( hFile, "%s\n", pThreat->szWarnUGC[i] );
               if ( i == 0 ) fscanf( hFile, "%d %d\n", &iWarn1[0], &iWarn1[1] );
               if ( i == 1 ) fscanf( hFile, "%d %d\n", &iWarn2[0], &iWarn2[1] );
               if ( *piDetails == 1 ) 
               {
                  fgets( pThreat->szWarnHeadLineStd[i], 256, hFile );                                             
                  fgets( pThreat->szWarnHeadLineSpn[i], 512, hFile );                                             
                  fgets( pThreat->szWarnSegArea[i], 256, hFile );                                             
                  fgets( pThreat->szWarnPager[i], 128, hFile );                                             
                  pThreat->szWarnHeadLineStd[i][strlen( pThreat->szWarnHeadLineStd[i] )-1] = '\0';
                  pThreat->szWarnHeadLineSpn[i][strlen( pThreat->szWarnHeadLineSpn[i] )-1] = '\0';
                  pThreat->szWarnSegArea[i][strlen( pThreat->szWarnSegArea[i] )-1] = '\0';
                  pThreat->szWarnPager[i][strlen( pThreat->szWarnPager[i] )-1] = '\0';
               }
            }
            fscanf( hFile, "%d\n", &iNum );       /* # Advisory Regions */
            if ( iNum > MAX_NUM_WWA_SEGS )
            {
               logit( "t", "Too many advisory regs in threat file-%d\n", iNum );
               fclose( hFile );
               return( 0 ) ;
            }
            for ( i=0; i<iNum; i++ )
            {
               if ( *piDetails == 1 ) fscanf( hFile, "%s\n", pThreat->szAdvUGC[i] );
               if ( i == 0 ) fscanf( hFile, "%d %d\n", &iAdv1[0], &iAdv1[1] );
               if ( i == 1 ) fscanf( hFile, "%d %d\n", &iAdv2[0], &iAdv2[1] );
               if ( *piDetails == 1 ) 
               {
                  fgets( pThreat->szAdvHeadLineStd[i], 256, hFile );                                             
                  fgets( pThreat->szAdvHeadLineSpn[i], 512, hFile );                                             
                  fgets( pThreat->szAdvSegArea[i], 256, hFile );                                             
                  fgets( pThreat->szAdvPager[i], 128, hFile );                                             
                  pThreat->szAdvHeadLineStd[i][strlen( pThreat->szAdvHeadLineStd[i] )-1] = '\0';
                  pThreat->szAdvHeadLineSpn[i][strlen( pThreat->szAdvHeadLineSpn[i] )-1] = '\0';
                  pThreat->szAdvSegArea[i][strlen( pThreat->szAdvSegArea[i] )-1] = '\0';
                  pThreat->szAdvPager[i][strlen( pThreat->szAdvPager[i] )-1] = '\0';
               }
            }
            fscanf( hFile, "%d\n", &iNum );       /* # Watch Regions */
            if ( iNum > MAX_NUM_WWA_SEGS )
            {
               logit( "t", "Too many watch regs in threat file-%d\n", iNum );
               fclose( hFile );
               return( 0 ) ;
            }
            for ( i=0; i<iNum; i++ )
            {
               if ( *piDetails == 1 ) fscanf( hFile, "%s\n", pThreat->szWatchUGC[i] );
               if ( i == 0 ) fscanf( hFile, "%d %d\n", &iWatch1[0], &iWatch1[1] );
               if ( i == 1 ) fscanf( hFile, "%d %d\n", &iWatch2[0], &iWatch2[1] );
               if ( *piDetails == 1 ) 
               {
                  fgets( pThreat->szWatchHeadLineStd[i], 256, hFile );                                             
                  fgets( pThreat->szWatchHeadLineSpn[i], 512, hFile );                                             
                  fgets( pThreat->szWatchSegArea[i], 256, hFile );                                             
                  fgets( pThreat->szWatchPager[i], 128, hFile );                                             
                  pThreat->szWatchHeadLineStd[i][strlen( pThreat->szWatchHeadLineStd[i] )-1] = '\0';
                  pThreat->szWatchHeadLineSpn[i][strlen( pThreat->szWatchHeadLineSpn[i] )-1] = '\0';
                  pThreat->szWatchSegArea[i][strlen( pThreat->szWatchSegArea[i] )-1] = '\0';
                  pThreat->szWatchPager[i][strlen( pThreat->szWatchPager[i] )-1] = '\0';
               }
            }
            fscanf( hFile, "%d\n", &iNum );       /* # Info Regions */
            if ( iNum > MAX_NUM_INFO_SEGS )
            {
               logit( "t", "Too many info regs in threat file-%d\n", iNum );
               fclose( hFile );
               return( 0 ) ;
            }
            for ( i=0; i<iNum; i++ )
            {
               if ( *piDetails == 1 ) fgets( pThreat->szInfoAreaStd[i], 256, hFile );                                             
               if ( *piDetails == 1 ) pThreat->szInfoAreaStd[i][strlen( pThreat->szInfoAreaStd[i] )-1] = '\0';
            }
            fscanf( hFile, "%d\n", &iNum );       /* # Info Regions - web */
            if ( iNum > MAX_NUM_INFO_SEGS )
            {
               logit( "t", "Too many web info regs in threat file-%d\n", iNum );
               fclose( hFile );
               return( 0 ) ;
            }
            for ( i=0; i<iNum; i++ )
            {
               if ( *piDetails == 1 ) fgets( pThreat->szInfoAreaSpn[i], 256, hFile );                                             
               if ( *piDetails == 1 ) pThreat->szInfoAreaSpn[i][strlen( pThreat->szInfoAreaSpn[i] )-1] = '\0';
            }
            fscanf( hFile, "%d\n", &iNum );       /* # Cancel Regions */
            if ( iNum > 1 ) iNum = 1;
            for ( i=0; i<iNum; i++ )
            {
               if ( *piDetails == 1 ) 
               {
                  fscanf( hFile, "%s\n", pThreat->szCancelUGC );
                  fgets( pThreat->szCancelHeadLineStd, 256, hFile );                                             
                  fgets( pThreat->szCancelHeadLineSpn, 512, hFile );                                             
                  fgets( pThreat->szCancelSegArea, 256, hFile );                                             
                  pThreat->szCancelHeadLineStd[strlen( pThreat->szCancelHeadLineStd )-1] = '\0';
                  pThreat->szCancelHeadLineSpn[strlen( pThreat->szCancelHeadLineSpn )-1] = '\0';
                  pThreat->szCancelSegArea[strlen( pThreat->szCancelSegArea )-1] = '\0';
               }
            }
            fgets( pThreat->szSpecArea, 128, hFile );

            fclose( hFile );
/* Compute iWWType here */         
            if ( iWarn1[0] <  0 && iAdv1[0] <  0 && iWatch1[0] <  0 ) *piType=0;
            if ( iWarn1[0] <  0 && iAdv1[0] >= 0 && iWatch1[0] <  0 ) *piType=1;
            if ( iWarn1[0] <  0 && iAdv1[0] <  0 && iWatch1[0] >= 0 ) *piType=2;
            if ( iWarn1[0] <  0 && iAdv1[0] >= 0 && iWatch1[0] >= 0 ) *piType=3;
            if ( iWarn1[0] >= 0 && iAdv1[0] <  0 && iWatch1[0] <  0 ) *piType=4;
            if ( iWarn1[0] >= 0 && iAdv1[0] <  0 && iWatch1[0] >= 0 ) *piType=5;
            if ( iWarn1[0] >= 0 && iAdv1[0] >= 0 && iWatch1[0] <  0 ) *piType=6;
            if ( iWarn1[0] >= 0 && iAdv1[0] >= 0 && iWatch1[0] >= 0 ) *piType=7;
            return( iInFile );
         }
         else     /* Hypo was not in this group, so just read into temp vars. */
         {
            fscanf( hFile, "%d\n", &iTemp );
            fscanf( hFile, "%d\n", &iTemp3 );
            fscanf( hFile, "%d\n", &iNum );       /* # Warning Regions */
            for ( i=0; i<iNum; i++ )
            {
               if ( iTemp == 1 ) fscanf( hFile, "%s\n", szTemp );
               fscanf( hFile, "%d %d\n", &iTemp1, &iTemp2 );
               if ( iTemp == 1 ) fgets( szTemp, 256, hFile );                                             
               if ( iTemp == 1 ) fgets( szTemp, 512, hFile );                                             
               if ( iTemp == 1 ) fgets( szTemp, 256, hFile );                                             
               if ( iTemp == 1 ) fgets( szTemp, 128, hFile );                                             
            }
            fscanf( hFile, "%d\n", &iNum );       /* # Advisory Regions */
            for ( i=0; i<iNum; i++ )
            {
               if ( iTemp == 1 ) fscanf( hFile, "%s\n", szTemp );
               fscanf( hFile, "%d %d\n", &iTemp1, &iTemp2 );
               if ( iTemp == 1 ) fgets( szTemp, 256, hFile );                                             
               if ( iTemp == 1 ) fgets( szTemp, 512, hFile );                                             
               if ( iTemp == 1 ) fgets( szTemp, 256, hFile );                                             
               if ( iTemp == 1 ) fgets( szTemp, 128, hFile );                                             
            }
            fscanf( hFile, "%d\n", &iNum );       /* # Watch Regions */
            for ( i=0; i<iNum; i++ )
            {
               if ( iTemp == 1 ) fscanf( hFile, "%s\n", szTemp );
               fscanf( hFile, "%d %d\n", &iTemp1, &iTemp2 );
               if ( iTemp == 1 ) fgets( szTemp, 256, hFile );                                             
               if ( iTemp == 1 ) fgets( szTemp, 512, hFile );                                             
               if ( iTemp == 1 ) fgets( szTemp, 256, hFile );                                             
               if ( iTemp == 1 ) fgets( szTemp, 128, hFile );                                             
            }
            fscanf( hFile, "%d\n", &iNum );       /* # Info Regions */
            for ( i=0; i<iNum; i++ )
               if ( iTemp == 1 ) fgets( szTemp, 256, hFile );                                             
            fscanf( hFile, "%d\n", &iNum );       /* # Info Regions */
            for ( i=0; i<iNum; i++ )
               if ( iTemp == 1 ) fgets( szTemp, 256, hFile );                                             
            fscanf( hFile, "%d\n", &iNum );       /* # Cancel Regions */
            if ( iNum > 1 ) iNum = 1;
            for ( i=0; i<iNum; i++ )
            {
               if ( iTemp == 1 ) fscanf( hFile, "%s\n", szTemp );
               if ( iTemp == 1 ) fgets( szTemp, 256, hFile );                                             
               if ( iTemp == 1 ) fgets( szTemp, 256, hFile );                                             
               if ( iTemp == 1 ) fgets( szTemp, 256, hFile );                                             
            }
            fgets( szTemp, 128, hFile );                                             
            fscanf( hFile, "\n" );
         }
      }
      fclose( hFile );
   }
   else
   {
      logit( "t", "Couldn't open %s\n", pszThreatFile );
      return( iInFile );
   }
   return( iInFile );
}
	                   
      /******************************************************************
       *                      WarnOrAdvisory()                          *
       *                                                                *
       * This function returns a flag which indicates if a warning or   *
       * or advisory should be called for this region.  It is only      *
       * called if the the region is 0, 1, or 2 (U.S. west coast, BC,   *
       * or Alaska) and the magnitude is less than 7.6.  For larger     *
       * magnitudes, a warning is automatic.                            *
       *                                                                *
       *  Arguments:                                                    *
       *   dLat             Epicentral latitude (geographic)            *
       *   dLon             Epicentral longitude                        *
       *                                                                *
       *  Return:                                                       *
       *   int - 1->Warning; 0->Advisory                                *
       *                                                                *
       ******************************************************************/
       
int WarnOrAdvisory( double dLat, double dLon )
{
   double  dLatT, dLonT;        /* lat/lon with lon from 0-360 */
   int     iFlag;               /* 1-> Warning; 0-> Advisory */

/* Adjust epicenter in case longitude comes in negative */
   dLatT = dLat;
   dLonT = dLon;
   if ( dLonT < 0. ) dLonT += 360.;

   iFlag = 1;
/* West Coast eliminate by east/west check */
   if ( dLatT < 31.5 ) iFlag = 0;
   if ( dLatT >= 31.5 && dLatT < 32.0 && dLonT <= 240.0 ) iFlag = 0;
   if ( dLatT >= 32.0 && dLatT < 33.0 && dLonT <= 239.3 ) iFlag = 0;
   if ( dLatT >= 33.0 && dLatT < 34.0 && dLonT <= 238.6 ) iFlag = 0;
   if ( dLatT >= 34.0 && dLatT < 35.0 && dLonT <= 237.8 ) iFlag = 0;
   if ( dLatT >= 35.0 && dLatT < 36.0 && dLonT <= 237.2 ) iFlag = 0;
   if ( dLatT >= 36.0 && dLatT < 37.0 && dLonT <= 236.5 ) iFlag = 0;
   if ( dLatT >= 37.0 && dLatT < 38.0 && dLonT <= 236.0 ) iFlag = 0;
   if ( dLatT >= 38.0 && dLatT < 39.0 && dLonT <= 235.5 ) iFlag = 0;
   if ( dLatT >= 39.0 && dLatT < 40.0 && dLonT <= 235.0 ) iFlag = 0;
   if ( dLatT >= 40.0 && dLatT < 43.0 && dLonT <= 234.6 ) iFlag = 0;
   if ( dLatT >= 43.0 && dLatT < 45.0 && dLonT <= 234.3 ) iFlag = 0;
   if ( dLatT >= 45.0 && dLatT < 46.0 && dLonT <= 234.0 ) iFlag = 0;
   if ( dLatT >= 46.0 && dLatT < 47.0 && dLonT <= 233.5 ) iFlag = 0;
   if ( dLatT >= 47.0 && dLatT < 48.0 && dLonT <= 233.2 ) iFlag = 0;
   if ( dLatT >= 48.0 && dLatT < 49.0 && dLonT <= 232.5 ) iFlag = 0;
   if ( dLatT >= 49.0 && dLatT < 49.4 && dLonT <= 232.2 ) iFlag = 0;
/* BC/Alaska eliminate by north/south check */
   if ( dLatT < 51.9 && dLonT >= 170.0 && dLonT < 171.0 ) iFlag = 0;
   if ( dLatT < 51.5 && dLonT >= 171.0 && dLonT < 172.0 ) iFlag = 0;
   if ( dLatT < 51.3 && dLonT >= 172.0 && dLonT < 173.0 ) iFlag = 0;
   if ( dLatT < 51.0 && dLonT >= 173.0 && dLonT < 174.0 ) iFlag = 0;
   if ( dLatT < 50.7 && dLonT >= 174.0 && dLonT < 175.5 ) iFlag = 0;
   if ( dLatT < 50.4 && dLonT >= 175.5 && dLonT < 176.5 ) iFlag = 0;
   if ( dLatT < 50.2 && dLonT >= 176.5 && dLonT < 178.0 ) iFlag = 0;
   if ( dLatT < 50.0 && dLonT >= 178.0 && dLonT < 180.0 ) iFlag = 0;
   if ( dLatT < 49.8 && dLonT >= 180.0 && dLonT < 184.0 ) iFlag = 0;
   if ( dLatT < 50.0 && dLonT >= 184.0 && dLonT < 186.0 ) iFlag = 0;
   if ( dLatT < 50.2 && dLonT >= 186.0 && dLonT < 188.0 ) iFlag = 0;
   if ( dLatT < 50.5 && dLonT >= 188.0 && dLonT < 190.0 ) iFlag = 0;
   if ( dLatT < 50.7 && dLonT >= 190.0 && dLonT < 191.5 ) iFlag = 0;
   if ( dLatT < 51.0 && dLonT >= 191.5 && dLonT < 192.5 ) iFlag = 0;
   if ( dLatT < 51.3 && dLonT >= 192.5 && dLonT < 193.5 ) iFlag = 0;
   if ( dLatT < 51.6 && dLonT >= 193.5 && dLonT < 195.0 ) iFlag = 0;
   if ( dLatT < 52.0 && dLonT >= 195.0 && dLonT < 196.5 ) iFlag = 0;
   if ( dLatT < 52.5 && dLonT >= 196.5 && dLonT < 198.0 ) iFlag = 0;
   if ( dLatT < 52.7 && dLonT >= 198.0 && dLonT < 200.0 ) iFlag = 0;
   if ( dLatT < 53.3 && dLonT >= 200.0 && dLonT < 202.0 ) iFlag = 0;
   if ( dLatT < 53.7 && dLonT >= 202.0 && dLonT < 203.5 ) iFlag = 0;
   if ( dLatT < 54.2 && dLonT >= 203.5 && dLonT < 205.0 ) iFlag = 0;
   if ( dLatT < 54.6 && dLonT >= 205.0 && dLonT < 207.0 ) iFlag = 0;
   if ( dLatT < 55.2 && dLonT >= 207.0 && dLonT < 208.5 ) iFlag = 0;
   if ( dLatT < 55.8 && dLonT >= 208.5 && dLonT < 210.5 ) iFlag = 0;
   if ( dLatT < 56.7 && dLonT >= 210.5 && dLonT < 212.0 ) iFlag = 0;
   if ( dLatT < 57.2 && dLonT >= 212.0 && dLonT < 213.0 ) iFlag = 0;
   if ( dLatT < 57.7 && dLonT >= 213.0 && dLonT < 214.0 ) iFlag = 0;
   if ( dLatT < 58.5 && dLonT >= 214.0 && dLonT < 215.0 ) iFlag = 0;
   if ( dLatT < 59.1 && dLonT >= 215.0 && dLonT < 217.0 ) iFlag = 0;
   if ( dLatT < 58.8 && dLonT >= 217.0 && dLonT < 217.8 ) iFlag = 0;
   if ( dLatT < 58.4 && dLonT >= 217.8 && dLonT < 219.0 ) iFlag = 0;
   if ( dLatT < 58.1 && dLonT >= 219.0 && dLonT < 220.0 ) iFlag = 0;
   if ( dLatT < 57.8 && dLonT >= 220.0 && dLonT < 221.3 ) iFlag = 0;
   if ( dLatT < 57.5 && dLonT >= 221.3 && dLonT < 222.0 ) iFlag = 0;
   if ( dLatT < 57.0 && dLonT >= 222.0 && dLonT < 222.7 ) iFlag = 0;
   if ( dLatT < 56.0 && dLonT >= 222.7 && dLonT < 223.7 ) iFlag = 0;
   if ( dLatT < 55.0 && dLonT >= 223.7 && dLonT < 224.7 ) iFlag = 0;
   if ( dLatT < 54.0 && dLonT >= 224.7 && dLonT < 225.3 ) iFlag = 0;
   if ( dLatT < 53.0 && dLonT >= 225.3 && dLonT < 226.0 ) iFlag = 0;
   if ( dLatT < 52.5 && dLonT >= 226.0 && dLonT < 227.2 ) iFlag = 0;
   if ( dLatT < 51.7 && dLonT >= 227.2 && dLonT < 228.0 ) iFlag = 0;
   if ( dLatT < 51.3 && dLonT >= 228.0 && dLonT < 229.0 ) iFlag = 0;
   if ( dLatT < 51.0 && dLonT >= 229.0 && dLonT < 230.0 ) iFlag = 0;
   if ( dLatT < 50.5 && dLonT >= 230.0 && dLonT < 230.8 ) iFlag = 0;
   if ( dLatT < 49.8 && dLonT >= 230.8 && dLonT < 231.7 ) iFlag = 0;
   if ( dLatT < 49.4 && dLonT >= 231.7 && dLonT < 232.2 ) iFlag = 0;
   return iFlag;
}

