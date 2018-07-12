
 /************************************************************************
  * GET_HYPO.C                                                           *
  *                                                                      *
  * This is a group of functions which provide tools for                 *
  * reading hypocenters from a ring and loading them in a structure.     *
  *                                                                      *
  * Made into earthworm module 4/2001.                                   *
  *                                                                      *
  ************************************************************************/
  
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <earthworm.h>
#include <transport.h>
#include "earlybirdlib.h"

     /**************************************************************
      *                  CopyHypo                                  *
      *                                                            *
      * Copy hypocenter structure into main hypocenter array.      *
      *                                                            *
      * Arguments:                                                 *
      *  HStruct     Hypocenter from ring                          *
      *  Hypo        Array of Hypocenter data structures           *
      *                                                            *
      * Returns:     Index of updated quake                        *
      *                                                            *
      **************************************************************/
int CopyHypo( HYPO *HStruct, HYPO Hypo[] )
{
   int     i;
   int     iMatch;             /* Flag to set if match found */

/* Check if this is a new version of an old quake */
   iMatch = -1;
   for ( i=0; i<MAX_QUAKES; i++ )
      if ( !strcmp( HStruct->szQuakeID, Hypo[i].szQuakeID ) &&
           HStruct->iVersion >= Hypo[i].iVersion &&
           HStruct->dOriginTime < Hypo[i].dOriginTime + 1200. &&
           HStruct->dOriginTime > Hypo[i].dOriginTime - 1200. )
         iMatch = i;

/* Shift array if this is a new quake (move each entry down one) */
   if ( iMatch < 0 )
   {
      for ( i=MAX_QUAKES-2; i>=0; i-- )
      {
         strcpy( Hypo[i+1].szPMagType, Hypo[i].szPMagType );
         strcpy( Hypo[i+1].szQuakeID,  Hypo[i].szQuakeID );
         Hypo[i+1].iVersion      = Hypo[i].iVersion;
         Hypo[i+1].dLat          = Hypo[i].dLat;
         Hypo[i+1].dLon          = Hypo[i].dLon;
         Hypo[i+1].dCoslat       = Hypo[i].dCoslat;
         Hypo[i+1].dSinlat       = Hypo[i].dSinlat;
         Hypo[i+1].dCoslon       = Hypo[i].dCoslon;
         Hypo[i+1].dSinlon       = Hypo[i].dSinlon;
         Hypo[i+1].dOriginTime   = Hypo[i].dOriginTime;
         Hypo[i+1].dDepth        = Hypo[i].dDepth;
         Hypo[i+1].iNumPs        = Hypo[i].iNumPs;
         Hypo[i+1].iAzm          = Hypo[i].iAzm;
         Hypo[i+1].dAvgRes       = Hypo[i].dAvgRes;
         Hypo[i+1].iGoodSoln     = Hypo[i].iGoodSoln;
         Hypo[i+1].dPreferredMag = Hypo[i].dPreferredMag;
         Hypo[i+1].iNumPMags     = Hypo[i].iNumPMags;
         Hypo[i+1].dMSAvg        = Hypo[i].dMSAvg;
         Hypo[i+1].iNumMS        = Hypo[i].iNumMS;
         Hypo[i+1].dMbAvg        = Hypo[i].dMbAvg;
         Hypo[i+1].iNumMb        = Hypo[i].iNumMb;
         Hypo[i+1].dMlAvg        = Hypo[i].dMlAvg;
         Hypo[i+1].iNumMl        = Hypo[i].iNumMl;
         Hypo[i+1].dMwpAvg       = Hypo[i].dMwpAvg;
         Hypo[i+1].iNumMwp       = Hypo[i].iNumMwp;      
         Hypo[i+1].dMwAvg        = Hypo[i].dMwAvg;
         Hypo[i+1].iNumMw        = Hypo[i].iNumMw;      
         Hypo[i+1].iMagOnly      = Hypo[i].iMagOnly;      
         Hypo[i+1].dTheta        = Hypo[i].dTheta;      
         Hypo[i+1].dThetaSD      = Hypo[i].dThetaSD;      
         Hypo[i+1].iNumTheta     = Hypo[i].iNumTheta;      
      }
      iMatch = 0;
   }

/* Now, copy (or update) the new hypo into the array */   
   strcpy( Hypo[iMatch].szPMagType, HStruct->szPMagType );
   strcpy( Hypo[iMatch].szQuakeID,  HStruct->szQuakeID );
   Hypo[iMatch].iVersion      = HStruct->iVersion;
   Hypo[iMatch].dLat          = HStruct->dLat;
   Hypo[iMatch].dLon          = HStruct->dLon;
   Hypo[iMatch].dCoslat       = HStruct->dCoslat;
   Hypo[iMatch].dSinlat       = HStruct->dSinlat;
   Hypo[iMatch].dCoslon       = HStruct->dCoslon;
   Hypo[iMatch].dSinlon       = HStruct->dSinlon;
   Hypo[iMatch].dOriginTime   = HStruct->dOriginTime;
   Hypo[iMatch].dDepth        = HStruct->dDepth;
   Hypo[iMatch].iNumPs        = HStruct->iNumPs;
   Hypo[iMatch].iAzm          = HStruct->iAzm;
   Hypo[iMatch].dAvgRes       = HStruct->dAvgRes;
   Hypo[iMatch].iGoodSoln     = HStruct->iGoodSoln;
   Hypo[iMatch].dPreferredMag = HStruct->dPreferredMag;
   Hypo[iMatch].iNumPMags     = HStruct->iNumPMags;
   Hypo[iMatch].dMSAvg        = HStruct->dMSAvg;
   Hypo[iMatch].iNumMS        = HStruct->iNumMS;
   Hypo[iMatch].dMbAvg        = HStruct->dMbAvg;
   Hypo[iMatch].iNumMb        = HStruct->iNumMb;
   Hypo[iMatch].dMlAvg        = HStruct->dMlAvg;
   Hypo[iMatch].iNumMl        = HStruct->iNumMl;
   Hypo[iMatch].dMwpAvg       = HStruct->dMwpAvg;
   Hypo[iMatch].iNumMwp       = HStruct->iNumMwp;
   Hypo[iMatch].dMwAvg        = HStruct->dMwAvg;
   Hypo[iMatch].iNumMw        = HStruct->iNumMw;
   Hypo[iMatch].iMagOnly      = HStruct->iMagOnly;
   Hypo[iMatch].dTheta        = HStruct->dTheta;
   Hypo[iMatch].dThetaSD      = HStruct->dThetaSD;
   Hypo[iMatch].iNumTheta     = HStruct->iNumTheta;

   return iMatch;      
}

     /**************************************************************
      *                  CopyHypoNoCheck                           *
      *                                                            *
      * Same as CopyHypo, but no check on for same quake is        *
      * performed.                                                 *
      *                                                            *
      * Arguments:                                                 *
      *  HStruct     Hypocenter from ring                          *
      *  Hypo        Array of Hypocenter data structures           *
      *                                                            *
      **************************************************************/
void CopyHypoNoCheck( HYPO *HStruct, HYPO Hypo[] )
{
   int     i;

/* Shift array (move each entry down one) */
   for ( i=MAX_QUAKES-2; i>=0; i-- )
   {
      strcpy( Hypo[i+1].szPMagType, Hypo[i].szPMagType );
      strcpy( Hypo[i+1].szQuakeID,  Hypo[i].szQuakeID );
      Hypo[i+1].iVersion      = Hypo[i].iVersion;
      Hypo[i+1].dLat          = Hypo[i].dLat;
      Hypo[i+1].dLon          = Hypo[i].dLon;
      Hypo[i+1].dCoslat       = Hypo[i].dCoslat;
      Hypo[i+1].dSinlat       = Hypo[i].dSinlat;
      Hypo[i+1].dCoslon       = Hypo[i].dCoslon;
      Hypo[i+1].dSinlon       = Hypo[i].dSinlon;
      Hypo[i+1].dOriginTime   = Hypo[i].dOriginTime;
      Hypo[i+1].dDepth        = Hypo[i].dDepth;
      Hypo[i+1].iNumPs        = Hypo[i].iNumPs;
      Hypo[i+1].iAzm          = Hypo[i].iAzm;
      Hypo[i+1].dAvgRes       = Hypo[i].dAvgRes;
      Hypo[i+1].iGoodSoln     = Hypo[i].iGoodSoln;
      Hypo[i+1].dPreferredMag = Hypo[i].dPreferredMag;
      Hypo[i+1].iNumPMags     = Hypo[i].iNumPMags;
      Hypo[i+1].dMSAvg        = Hypo[i].dMSAvg;
      Hypo[i+1].iNumMS        = Hypo[i].iNumMS;
      Hypo[i+1].dMbAvg        = Hypo[i].dMbAvg;
      Hypo[i+1].iNumMb        = Hypo[i].iNumMb;
      Hypo[i+1].dMlAvg        = Hypo[i].dMlAvg;
      Hypo[i+1].iNumMl        = Hypo[i].iNumMl;
      Hypo[i+1].dMwpAvg       = Hypo[i].dMwpAvg;
      Hypo[i+1].iNumMwp       = Hypo[i].iNumMwp;      
      Hypo[i+1].dMwAvg        = Hypo[i].dMwAvg;
      Hypo[i+1].iNumMw        = Hypo[i].iNumMw;      
      Hypo[i+1].iMagOnly      = Hypo[i].iMagOnly;      
      Hypo[i+1].dTheta        = Hypo[i].dTheta;      
      Hypo[i+1].dThetaSD      = Hypo[i].dThetaSD;      
      Hypo[i+1].iNumTheta     = Hypo[i].iNumTheta;      
   }

/* Now, copy the new hypo into the array */   
   strcpy( Hypo[0].szPMagType, HStruct->szPMagType );
   strcpy( Hypo[0].szQuakeID,  HStruct->szQuakeID );
   Hypo[0].iVersion      = HStruct->iVersion;
   Hypo[0].dLat          = HStruct->dLat;
   Hypo[0].dLon          = HStruct->dLon;
   Hypo[0].dCoslat       = HStruct->dCoslat;
   Hypo[0].dSinlat       = HStruct->dSinlat;
   Hypo[0].dCoslon       = HStruct->dCoslon;
   Hypo[0].dSinlon       = HStruct->dSinlon;
   Hypo[0].dOriginTime   = HStruct->dOriginTime;
   Hypo[0].dDepth        = HStruct->dDepth;
   Hypo[0].iNumPs        = HStruct->iNumPs;
   Hypo[0].iAzm          = HStruct->iAzm;
   Hypo[0].dAvgRes       = HStruct->dAvgRes;
   Hypo[0].iGoodSoln     = HStruct->iGoodSoln;
   Hypo[0].dPreferredMag = HStruct->dPreferredMag;
   Hypo[0].iNumPMags     = HStruct->iNumPMags;
   Hypo[0].dMSAvg        = HStruct->dMSAvg;
   Hypo[0].iNumMS        = HStruct->iNumMS;
   Hypo[0].dMbAvg        = HStruct->dMbAvg;
   Hypo[0].iNumMb        = HStruct->iNumMb;
   Hypo[0].dMlAvg        = HStruct->dMlAvg;
   Hypo[0].iNumMl        = HStruct->iNumMl;
   Hypo[0].dMwpAvg       = HStruct->dMwpAvg;
   Hypo[0].iNumMwp       = HStruct->iNumMwp;
   Hypo[0].dMwAvg        = HStruct->dMwAvg;
   Hypo[0].iNumMw        = HStruct->iNumMw;
   Hypo[0].iMagOnly      = HStruct->iMagOnly;
   Hypo[0].dTheta        = HStruct->dTheta;
   Hypo[0].dThetaSD      = HStruct->dThetaSD;
   Hypo[0].iNumTheta     = HStruct->iNumTheta;

   return;      
}

     /**************************************************************
      *                  CopyOQToHypo                              *
      *                                                            *
      * Copy OLDQUAKE structure into hypocenter structure.         *
      *                                                            *
      * Arguments:                                                 *
      *  pOQ         Pointer to OLDQUAKE structure                 *
      *  pHypo       Pointer to Hypocenter structure               *
      *                                                            *
      * Returns:     1                                             *
      *                                                            *
      **************************************************************/
int CopyOQToHypo( OLDQUAKE *pOQ, HYPO *pHypo )
{
   strcpy( pHypo->szPMagType, pOQ->szPMagType ) ;
   pHypo->dPreferredMag = pOQ->dPreferredMag;
   pHypo->iNumPMags     = pOQ->iNumPMags;
   pHypo->iAzm          = (int) pOQ->dAzm;
   pHypo->dAvgRes       = pOQ->dAvgRes;
   pHypo->iNumPs        = pOQ->iNumPs;
   strcpy( pHypo->szQuakeID, pOQ->szQuakeID );
   pHypo->iVersion      = pOQ->iVersion;
   pHypo->iGoodSoln     = pOQ->iGoodSoln;
   pHypo->dOriginTime   = pOQ->dOTime;
   pHypo->dDepth        = (double) pOQ->iDepth;
   pHypo->llEpiGG.dLat  = pOQ->dLat;
   pHypo->llEpiGG.dLon  = pOQ->dLon;
   pHypo->dLat          = pOQ->dLat;       /* Must be followed with GeoCent */
   pHypo->dLon          = pOQ->dLon;
   pHypo->dFirstPTime   = pOQ->d1stPTime;
   pHypo->dMSAvg        = pOQ->dMSAvg;
   pHypo->iNumMS        = pOQ->iNumMS;
   pHypo->dMbAvg        = pOQ->dMbAvg;
   pHypo->iNumMb        = pOQ->iNumMb;
   pHypo->dMlAvg        = pOQ->dMlAvg;
   pHypo->iNumMl        = pOQ->iNumMl;
   pHypo->dMwpAvg       = pOQ->dMwpAvg;
   pHypo->iNumMwp       = pOQ->iNumMwp;
   pHypo->dMwAvg        = pOQ->dMwAvg;
   pHypo->iNumMw        = pOQ->iNumMw;
   pHypo->dTheta        = pOQ->dTheta;
   pHypo->dThetaSD      = pOQ->dThetaSD;
   pHypo->iNumTheta     = pOQ->iNumTheta;
   return 1;      
}

     /**************************************************************
      *                HypoStruct()                                *
      *                                                            *
      * Fill in HYPO structure from HYPOTWC message.               *
      *                                                          7  *
      * Arguments:                                                 *
      *  HIn         HypoTWC message from ring                     *
      *  pHypo       Hypocenter data structure                     *
      *                                                            *
      * Return - 0 if OK, -1 if copy problem                       *
      **************************************************************/

int HypoStruct( char *HIn, HYPO *pHypo )
{
/* Break up incoming message
   *************************/
   if ( sscanf( HIn, "%s %d %lf %lf %lf %lf %d %d %lf %d %lf %s %d "
                     "%lf %d %lf %d %lf %d %lf %d %lf %d %lf %d %d",
     pHypo->szQuakeID, &pHypo->iVersion, &pHypo->dOriginTime,
    &pHypo->dLat, &pHypo->dLon, &pHypo->dDepth, &pHypo->iNumPs, &pHypo->iAzm,
    &pHypo->dAvgRes, &pHypo->iGoodSoln, &pHypo->dPreferredMag,
     pHypo->szPMagType, &pHypo->iNumPMags,
    &pHypo->dMSAvg, &pHypo->iNumMS, &pHypo->dMwpAvg, &pHypo->iNumMwp, 
    &pHypo->dMbAvg, &pHypo->iNumMb, &pHypo->dMlAvg, &pHypo->iNumMl,
    &pHypo->dMwAvg, &pHypo->iNumMw, &pHypo->dTheta, &pHypo->iNumTheta, 
    &pHypo->iMagOnly  ) != 26 )
   {
      logit( "t", "Not correct # fields in message: %s\n", HIn );
      return -1;
   }    
   GetLatLonTrig( (LATLON *) pHypo );  /* Get sin/cos of lat and lon */
   return 0;
}

      /******************************************************************
       *                            InitHypo()                          *
       *                                                                *
       *  This function initializes a HYPO structure.                   *
       *                                                                *
       *  Arguments:                                                    *
       *   Hypo             Quake hypocenter parameter structure        *
       *                                                                *
       ******************************************************************/
	
void InitHypo( HYPO *pHypo )
{
   pHypo->dLat          = 0.;
   pHypo->dLon          = 0.;
   pHypo->dCoslat       = 0.;
   pHypo->dSinlat       = 0.;
   pHypo->dCoslon       = 0.;
   pHypo->dSinlon       = 0.;
   pHypo->dDepth        = 0.0;
   pHypo->dPreferredMag = 0.0;
   pHypo->dOriginTime   = 0;
   pHypo->iNumPMags     = 0;
   pHypo->iDepthControl = 3;
   pHypo->iNumPs        = 0;
   pHypo->iNumBadPs     = 0;
   pHypo->dAvgRes       = 0.;
   pHypo->iAzm          = 0;
   pHypo->dNearestDist  = 0.;
   pHypo->dFirstPTime   = 0.;
   pHypo->iGoodSoln     = 0;
   pHypo->dMbAvg        = 0.0;
   pHypo->dMlAvg        = 0.0;
   pHypo->dMSAvg        = 0.0;
   pHypo->dMwAvg        = 0.0;
   pHypo->dMwpAvg       = 0.0;
   pHypo->dTheta        = 0.0;
   pHypo->dThetaSD      = 0.0;
   pHypo->iNumMb        = 0;
   pHypo->iNumMl        = 0;
   pHypo->iNumMS        = 0;
   pHypo->iNumMw        = 0;
   pHypo->iNumMwp       = 0;
   pHypo->iNumTheta     = 0;
   pHypo->iNumMbClip    = 0;
   pHypo->iNumMlClip    = 0;
   pHypo->iNumMSClip    = 0;
   pHypo->iNumMwpClip   = 0;
   pHypo->iNumMwClip    = 0;
   pHypo->iMagOnly      = 0;
   pHypo->iUpdateMap    = 0;
   strcpy( pHypo->szLat, "\0" );
   strcpy( pHypo->szLon, "\0" );
   strcpy( pHypo->szOTimeRnd, "\0" );
}

     /**************************************************************
      *                  LoadHypo()                                *
      *                                                            *
      * Fill in HYPO structure from QuakeFile.                     *
      *                                                            *
      * Arguments:                                                 *
      *  pszQFile    Quake file name                               *
      *  Hypo        Hypocenter data structure                     *
      *  iMaxQuakes  Max number to read into HYPO array            *
      *                                                            *
      **************************************************************/

void LoadHypo( char *pszQFile, HYPO Hypo[], int iMaxQuakes )	  
{
   char    cLat, cLon;
   double  dAzm;                        /* Azimuthal coverage */
   double  dTemp;                       /* Dummy variable */
   FILE    *hFile;                      /* File handle */
   int     i;
   int     iOTimeRnd;                   /* O-time rounded to nearest minute */
   LATLON  ll, ll2;                     /* Input location */

/* Open quake file */
   if ( (hFile = fopen( pszQFile, "r" )) == NULL )
   {
      logit ("t", "Quake file, %s, not opened.\n", pszQFile);
      return;
   }
   
/* Read in quake information */   
   for ( i=0; i<iMaxQuakes; i++ )
   {
      if ( fscanf( hFile, "%lf %lf %lf %lf %d %s %lf %s %d %d %lf"
                          " %lf %lf %d %lf %d %lf %d %lf %d %lf %d"
                          " %lf %d %lf %lf %d\n",
                   &Hypo[i].dOriginTime, &ll.dLat, &ll.dLon,
                   &Hypo[i].dPreferredMag, &Hypo[i].iNumPMags,
                    Hypo[i].szPMagType, &Hypo[i].dDepth,
                    Hypo[i].szQuakeID, &Hypo[i].iVersion,
                   &Hypo[i].iNumPs, &Hypo[i].dAvgRes, &dAzm,
                   &Hypo[i].dMbAvg, &Hypo[i].iNumMb,
                   &Hypo[i].dMlAvg, &Hypo[i].iNumMl,
                   &Hypo[i].dMSAvg, &Hypo[i].iNumMS,
                   &Hypo[i].dMwpAvg, &Hypo[i].iNumMwp,
                   &Hypo[i].dMwAvg, &Hypo[i].iNumMw,
                   &dTemp, &Hypo[i].iGoodSoln, &Hypo[i].dTheta, 
                   &Hypo[i].dThetaSD, &Hypo[i].iNumTheta ) != 27 ) break;
      if ( Hypo[i].dOriginTime == 0. ) break;
      memcpy( &ll2, &ll, sizeof( LATLON ) );
      GeoCent( &ll );
      Hypo[i].dLat     = ll.dLat;
      Hypo[i].dLon     = ll.dLon;
      Hypo[i].dCoslat  = cos( Hypo[i].dLat );
      Hypo[i].dSinlat  = sin( Hypo[i].dLat );
      Hypo[i].dCoslon  = cos( Hypo[i].dLon );
      Hypo[i].dSinlon  = sin( Hypo[i].dLon );
      Hypo[i].iAzm     = (int) dAzm;
      Hypo[i].iMagOnly = 0;
   
/* Put lat/lon in geographic form */
      Hypo[i].llEpiGG.dLat = ll2.dLat;
      Hypo[i].llEpiGG.dLon = ll2.dLon;
      if ( Hypo[i].llEpiGG.dLon > 180. ) Hypo[i].llEpiGG.dLon -= 360.;

/* Convert location to character coords */
      ConvertLoc( &Hypo[i].llEpiGG, &ll, &cLat, &cLon );
      sprintf( Hypo[i].szLat, "%5.1lf%c", ll.dLat, cLat );
      sprintf( Hypo[i].szLon, "%6.1lf%c", ll.dLon, cLon ); 

/* Get O-time rounded to minute for messages */
      NewDateFromModSec( &Hypo[i].stOTime, Hypo[i].dOriginTime+3506630400. );
      NewDateFromModSecRounded( &Hypo[i].stOTimeRnd,
                                 Hypo[i].dOriginTime+3506630400. );
      iOTimeRnd = Hypo[i].stOTimeRnd.wHour*100 + Hypo[i].stOTimeRnd.wMinute;
      itoaX( iOTimeRnd, Hypo[i].szOTimeRnd ); 
      PadZeroes( 4, Hypo[i].szOTimeRnd );                /* Add leading zeros */
   }
   fclose( hFile );
}   

