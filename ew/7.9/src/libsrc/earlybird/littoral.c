 /************************************************************************
  * LITTORAL.C                                                           *
  *                                                                      *
  * These functions provide littoral location information based on a     *
  * lat/lon.  The littoral location is given as a direction and distance *
  * from NUM_CITIES reference cities.  Actually, the distance and        *
  * direction are given to two cities, one major and one minor.  In the  *
  * file CITIES.DAT the first NUM_MAJOR_CITIES are considered major      *
  * cities and the rest minor.                                           *
  *                                                                      *
  * January, 2005: Added East coast cities.                              *
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

      /******************************************************************
       *                          CityAziDelt()                         *
       *                                                                *
       *  This function calculates the distance and azimuth between an  *
       *  epicenter and city locations.                                 *
       *                                                                *
       *  Arguments:                                                    *
       *   pazidelt         Return structure with distance/azimuths     *
       *   pll              Epicentral location                         *
       *   pcity            Array of reference city lat/lons            *
       *   iNumCities       Number of cities in file                    *
       *                                                                *
       ******************************************************************/
	   
void CityAziDelt( AZIDELT *pazidelt, LATLON *pll, CITY *pcity, int iNumCities )
{
   int     i;

   for ( i=0; i<iNumCities; i++ )
   {
      GetDistanceAz( pll, (LATLON *) pcity, pazidelt );
      pazidelt++;
      pcity++;
   }
}

      /******************************************************************
       *                          LoadCities()                          *
       *                                                                *
       *  This function loads the city data to be used when getting the *
       *  location of the earthquake relative to cities.                *
       *                                                                *
       *  January, 2008: Cities files transferred in from calling prgrm.*
       *                                                                *
       *  Arguments:                                                    *
       *   pcity            Array of reference city lat/lons            *
       *   iFile            1->std., 2->lower case                      *
       *   pszFile1         Upper case city file                        *
       *   pszFile2         Lower case city file                        *
       *                                                                *
       *  Return:                                                       *
       *   -1 if problem in city load; 0 otherwise                      *
       *                                                                *
       ******************************************************************/
	   
int LoadCities( CITY *pCity, int iFile, char *pszFile1, char *pszFile2 )
{
   int     c;                        /* temporary char */
   FILE    *hFile;                   /* file handle */
   double  dLat, dLon;               /* temporary Lat, lon */
   int     i, j;                     /* counting indices */
   LATLON  ll;                       /* Structure for use in GeoCent */
   CITY    *pc;                      /* temporary CITY pointer */

/* Open the file with reference city name and locations */
   if ( iFile == 1 )
   {
      if ( (hFile = fopen( pszFile1, "r") ) == NULL ) 
      {
         logit( "t", "%s not opened in lodadcities\n", pszFile1 );
         return -1;
      }
   }
   else
   {
      if ( (hFile = fopen( pszFile2, "r") ) == NULL ) 
      {
         logit( "t", "%s not opened in lodadcities\n", pszFile2 );
         return -1;
      }
   }
   
/* File was found, so load up the data */
   for ( i=0; i<NUM_CITIES; i++ )
   {
      pc = pCity + i;
      if ( fscanf( hFile, " %lf %lf ", &dLat, &dLon ) == EOF )
      {
         fclose( hFile );
         logit( "t", "Problem reading %s\n", pszFile1 );
         return -1;
      }
      else
      {
         for ( j=0; j<32; j++ )
         {
            c = fgetc( hFile );
            if ( c != '\n' ) pc->szLoc[j] = (char) c;
            else
            {
               pc->szLoc[j] = '\0';
               break;
            }
         }
         ll.dLat = dLat;
         ll.dLon = dLon;
         GeoCent( &ll );
         pc->dLat = ll.dLat;
         pc->dLon = ll.dLon;
      }
   }
   fclose( hFile );
   return 0;
}

      /******************************************************************
       *                        LoadCitiesEC()                          *
       *                                                                *
       *  This function loads the city data to be used when getting the *
       *  location of the earthquake relative to eastern cities.        *
       *                                                                *
       *  January, 2008: Cities files transferred in from calling prgrm.*
       *                                                                *
       *  Arguments:                                                    *
       *   pcity            Array of reference city lat/lons            *
       *   iFile            1->std., 2->lower case                      *
       *   pszFile1         Upper case city file                        *
       *   pszFile2         Lower case city file                        *
       *                                                                *
       *  Return:                                                       *
       *   -1 if problem in city load; 0 otherwise                      *
       *                                                                *
       ******************************************************************/
	   
int LoadCitiesEC( CITY *pCity, int iFile, char *pszFile1, char *pszFile2 )
{
   int     c;                        /* temporary char */
   FILE    *hFile;                   /* file handle */
   double  dLat, dLon;               /* temporary Lat, lon */
   int     i, j;                     /* counting indices */
   LATLON  ll;                       /* Structure for use in GeoCent */
   CITY    *pc;                      /* temporary CITY pointer */

/* Open the file with reference city name and locations */
   if ( iFile == 1 )
   {
      if ( (hFile = fopen( pszFile1, "r") ) == NULL ) 
      {
         logit( "t", "%s not opened in lodadcities\n", pszFile1 );
         return -1;
      }
   }
   else
   {
      if ( (hFile = fopen( pszFile2, "r") ) == NULL ) 
      {
         logit( "t", "%s not opened in lodadcities\n", pszFile2 );
         return -1;
      }
   }

/* File was found, so load up the data */
   for ( i=0; i<NUM_CITIES_EC; i++ )
   {
      pc = pCity + i;
      if ( fscanf( hFile, " %lf %lf ", &dLat, &dLon ) == EOF )
      {
         fclose( hFile );
         logit( "t", "Problem reading %s\n", pszFile1 );
         return -1;
      }
      else
      {
         for ( j=0; j<29; j++ )
         {
            c = fgetc( hFile );
            if ( c != '\n' ) pc->szLoc[j] = (char) c;
            else
            {
               pc->szLoc[j] = '\0';
               break;
            }
         }
         ll.dLat = dLat;
         ll.dLon = dLon;
         GeoCent( &ll );
         pc->dLat = ll.dLat;
         pc->dLon = ll.dLon;
      }
   }
   fclose( hFile );
   return 0;
}

      /******************************************************************
       *                       NearestCities()                          *
       *                                                                *
       * This function calculates the distance from a location to the   *
       * nearest city.  There are NUM_CITIES cities on file.  The first *
       * NUM_MAJOR_CITIES are major and the remaining are minor cities. *
       * The nearest major and minor cities are found along with the    *
       * distance and direction to the city.  See the file CITIES.DAT.  *
       *                                                                *
       *  Arguments:                                                    *
       *   pll              Epicentral location (geocentric)            *
       *   pcity            Array of reference city lat/lons            *
       *   pcitydis         Distance/direction to nearest cities        *
       *                                                                *
       ******************************************************************/
	   
void NearestCities( LATLON *pll, CITY *pcity, CITYDIS *pcitydis )
{
   AZIDELT azidelt[NUM_CITIES];     /* pointer to azimuth, delta array */
   int     i, j, iIndex;            /* counting indices */
   double  dMin, dDir;
   int     iDir;
   char    *pszDir[] = { "S","SW","SW","SW","W","NW","NW","NW",
                         "N","NE","NE","NE","E","SE","SE","SE" };

/* Compute distance and azimuth from each city to epicenter */
   CityAziDelt( &azidelt[0], pll, pcity, NUM_CITIES );
   for ( i=0; i<NUM_CITIES; i++ )
      azidelt[i].dDelta *= 69.0933;     /* Miles per degree */
        
/* Choose the minor and major city with the closest distance */
   for ( j=0; j<2; j++ )
   {
      iIndex = 0;
      if ( j != 0 )                     /* First NUM_MAJOR_CITIES cities */
      {
         dMin = azidelt[0].dDelta;
         for ( i=0; i<NUM_MAJOR_CITIES; i++ )
            if ( dMin >= azidelt[i].dDelta )
            {
               dMin = azidelt[i].dDelta;
               iIndex = i;
            }
      }
      else                              /* Rest are minor cities */
      {
         dMin = azidelt[NUM_MAJOR_CITIES].dDelta;
         for ( i=NUM_MAJOR_CITIES; i<NUM_CITIES; i++ )
            if ( dMin >= azidelt[i].dDelta )
            {
               dMin = azidelt[i].dDelta;
               iIndex = i;
            }
      }
      dDir = azidelt[iIndex].dAzimuth;  /* Fill CITYDIS array */
      while ( dDir >= 348.75 ) dDir -= 360.;
      iDir = (int) ((dDir+33.75) / 22.5) - 1;
      pcitydis->pszLoc[j] = (pcity+iIndex)->szLoc;
      pcitydis->iDis[j] = ((int) ((azidelt[iIndex].dDelta+2.5)/5.)) * 5;
      pcitydis->pszDir[j] = pszDir[iDir];
   }
}

      /******************************************************************
       *                     NearestCitiesEC()                          *
       *                                                                *
       * This function calculates the distance from a location to the   *
       * nearest eastern city.  There are NUM_CITIES_EC cities on file. *
       * The 1st NUM_MAJOR_CITIES_EC are major and the remaining are    *
       * minor cities. The nearest major and minor cities are found     *
       * along with the distance and direction to the city.  See the    *
       * file CITIES-EC.DAT.                                            *
       *                                                                *
       *                                                                *
       *  Arguments:                                                    *
       *   pll              Epicentral location (geocentric)            *
       *   pcity            Array of reference city lat/lons            *
       *   pcitydis         Distance/direction to nearest cities        *
       *                                                                *
       ******************************************************************/
	   
void NearestCitiesEC( LATLON *pll, CITY *pcity, CITYDIS *pcitydis )
{
   AZIDELT azidelt[NUM_CITIES_EC];  /* pointer to azimuth, delta array */
   int     i, j, iIndex;            /* counting indices */
   double  dMin, dDir;
   int     iDir;
   char    *pszDir[] = { "S","SW","SW","SW","W","NW","NW","NW",
                         "N","NE","NE","NE","E","SE","SE","SE" };

/* Compute distance and azimuth from each city to epicenter */
   CityAziDelt( &azidelt[0], pll, pcity, NUM_CITIES_EC );
   for ( i=0; i<NUM_CITIES_EC; i++ )
      azidelt[i].dDelta *= 69.0933;     /* Miles per degree */
        
/* Choose the minor and major city with the closest distance */
   for ( j=0; j<2; j++ )
   {
      iIndex = 0;
      if ( j != 0 )                     /* First NUM_MAJOR_CITIES cities */
      {
         dMin = azidelt[0].dDelta;
         for ( i=0; i<NUM_MAJOR_CITIES_EC; i++ )
            if ( dMin >= azidelt[i].dDelta )
            {
               dMin = azidelt[i].dDelta;
               iIndex = i;
            }
      }
      else                              /* Rest are minor cities */
      {
         dMin = azidelt[NUM_MAJOR_CITIES_EC].dDelta;
         for ( i=NUM_MAJOR_CITIES_EC; i<NUM_CITIES_EC; i++ )
            if ( dMin >= azidelt[i].dDelta )
            {
               dMin = azidelt[i].dDelta;
               iIndex = i;
            }
      }
      dDir = azidelt[iIndex].dAzimuth;  /* Fill CITYDIS array */
      while ( dDir >= 348.75 ) dDir -= 360.;
      iDir = (int) ((dDir+33.75) / 22.5) - 1;
      pcitydis->pszLoc[j] = (pcity+iIndex)->szLoc;
      pcitydis->iDis[j] = ((int) ((azidelt[iIndex].dDelta+2.5)/5.)) * 5;
      pcitydis->pszDir[j] = pszDir[iDir];
   }
}
