
 /************************************************************************
  * FEREG.C                                                              *
  *                                                                      *
  * These functions obtain the Flinn-Engdahl geographical region name and*
  * number given a +/- geographical latitude and longitude.  The FORTRAN *
  * code for these functions was taken from the NEIC Public FTP site and *
  * converted to C at the WC/ATWC by Whitmore in October, 1999.          *
  *                                                                      *
  * Made into earthworm module 2/2001.                                   *
  *                                                                      *
  ************************************************************************/
  
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <earthworm.h>
#include <transport.h>             
#ifdef _WINNT
 #include <crtdbg.h>
#endif

#include "earlybirdlib.h"

/*	subroutine getnam (nbr, name)

c	PROCEDURE IS CALLED BY namnum

c	ARGUMENTS:
c		nbr - the Flinn - Engdahl geographical region number
c		name - the corresponding F-E geographical region name

c	FILES ACCESSED:
c		names.fer

c	A GEOGRAPHICAL REGION NAME IS NO MORE THAN 32 CHARACTERS IN LENGTH.

c	Written by G.J. Dunphy
c	modified August 29, 1991 by BPresgrave to call subroutine gtunit
c	to open the next available unit number. This is needed because many
c	programs have already opened another file as unit 1 long before this
c	subroutine is called.

Converted to C 10/99 at wc&atwc by Whitmore

*/

char *getnam( int iFENum, char *pszNameFile )
{
   int     i;
   FILE    *InFile;                    /* File handles */
   static  char  szRegName[36];        /* Flinn-Engdahl region name */

   strcpy( szRegName, "\0" );
   if ( iFENum == 0 ) return( (char *) NULL );
   if ( (InFile = fopen( pszNameFile, "r" )) == NULL )
   {
      logit( "t", "%s file not opened\n", pszNameFile );
      return( (char *) NULL );
   }
   for ( i=0; i<iFENum; i++ )          /* Read name from file */
      fgets( szRegName, 34, InFile );  /* Get string (34 to allow for CR/LF) */
   fclose( InFile );
   return( szRegName );
}

/*	subroutine getnamLC (nbr, name)

c	PROCEDURE IS CALLED BY namnumLC

c	ARGUMENTS:
c		nbr - the Flinn - Engdahl geographical region number
c		name - the corresponding F-E geographical region name

c	FILES ACCESSED:
c		nameslc.fer

c	Written by G.J. Dunphy
c	modified August 29, 1991 by BPresgrave to call subroutine gtunit
c	to open the next available unit number. This is needed because many
c	programs have already opened another file as unit 1 long before this
c	subroutine is called.

Converted to C 10/99 at wc&atwc by Whitmore, and added with calls to the lower
case data file in 9/05.

*/

char *getnamLC( int iFENum, char *pszNameFileLC )
{
   int    i;
   FILE  *InFile;               /* File handles */
   static char szRegName[64];   /* Flinn-Engdahl region name */

   strcpy( szRegName, "\0" );
   if ( iFENum == 0 ) return( (char *) NULL );
   if ( (InFile = fopen( pszNameFileLC, "r" )) == NULL )
   {
      logit( "t", "%s file not opened\n", pszNameFileLC );
      return( (char *) NULL );
   }
   for ( i=0; i<iFENum; i++ )  /* Read name from file */
      fgets( szRegName, 64, InFile );	
   fclose( InFile );			
   return( szRegName );
}

/*	subroutine getnum (lat, lon, nbr)

c	PROCEDURE IS CALLED BY namnum

c	ARGUMENTS:
c		lat - geographic latitude in decimal degrees
c		lon - geographic longitude in decimal degrees
c		nbr - the Flinn - Engdahl geographical region number

c	FILES ACCESSED:
c			llindx.fer
c			lattiers.fer

c	A GEOGRAPHIC REGION CONTAINS ITS LOWER BOUNDARIES BUT NOT
c	ITS UPPER. (HERE UPPER AND LOWER ARE DEFINED IN TERMS
c	OF THEIR ABSOLUTE VALUES).  THE EQUATOR BELONGS TO THE NORTHERN
c	HEMISPHERE & THE GREENWICH MERIDIAN BELONGS TO THE EASTERN HEMISPHERE IN
c	THIS IMPLEMENTATION.  180.000 DEGREES LONGITUDE IS ASSIGNED TO THE
c	EASTERN HEMISPHERE.


c	COORDINATES ARE TRUNCATED TO WHOLE DEGREES BEFORE DETERMINING
c	NUMBER.

c	Note:  There are a maximum of 35 boundary-number-pairs/tier in file
c	       lattiers.fer.  The record length chosen (512 bytes) corresponds
c	       to a page size on the VAX/VMS system.
c	Written by G.J. Dunphy

c	Modified August 29, 1991 by B Presgrave to call library function
c	gtunit to determine the next available unit number for the open
c	statements. This is needed because many programs have long ago opened
c	another file on unit 1 or 2 before this routine is called. The close
c 	at the end then closes the other file, leaving the programmer scratching
c	his head about why adding a subroutine call causes his output to
c 	disappear completely!

	parameter (RECSIZ = 128)     ! Recordsize in longwords
	parameter (RSTMS2 = 256)     ! Twice RECSIZ
	parameter (RSPLS1 = 129)     ! RECSIZ + 1
	parameter (RSMNS1 = 127)     ! RECSIZ - 1
	parameter (RSMNS2 = 126)     ! RECSIZ - 2
	parameter (LASTRC =  46)     ! (TOTREC / RECSIZ) + 1 (TOTLEN = 5796)

c	BIBLIOGRAPHIC REFERENCES

c		Flinn, E.A. and E.R. Engdahl (1964). A proposed basis for geo-
c			graphical and seismic regionalization, Seismic Data
c			Laboratory Report No. 101, Earth Sciences Division,
c			United Electrodynamics, Inc. Alexandria, Va.

c		Flinn, E.A. and E.R. Engdahl (1965). A proposed basis for geo-
c			graphical and seismic regionalization, Rev. Geophys 3,
c			123 - 149.

c		Flinn, E.A., E.R. Engdahl and A.R. Hill (1974). Seismic and geo-
c			graphical regionalization, BSSA 64, 771 - 992.

Converted to C 10/99 at wc&atwc by Whitmore

*/

int getnum( double dLat, double dLon, char *pszIndexFile, char *pszLatFile )
{
   double  dALat, dALon;       /* Absolute values of lat/lon */
   double  dLng;               /* Adjusted longitude */
   int     i;
   int     iFENum = 0;         /* Flinn/Engdahl region for this lat/lon */
   int     iLastFENum;         /* FE region from last read */
   int     iLat, iLon;         /* Truncated lat/lon */
   FILE    *InFile1, *InFile2; /* Input file handles */
   int     iNumRegs;           /* # fe regions for this lat */
   int     iQuadOn;            /* Record where this quadrant starts */
   int     iTierLon;           /* Longitude read in from lattiers file */
   int     iTierOn;            /* Record # where fe nums. start for this lat */

   if ( (InFile1 = fopen( pszIndexFile, "r" )) == NULL )
   {
      logit( "t", "%s file not opened\n", pszIndexFile );
      return 0;
   }
   if ( (InFile2 = fopen( pszLatFile, "r" ))
                          == NULL )
   {
      logit( "t", "%s file not opened\n", pszLatFile );
      fclose( InFile1 );
      return 0;
   }
	
   dLng = dLon;
   if ( dLng == -180. ) dLng = 180.0;
   dALat = fabs( dLat );                 /* Get absolute values of lat/lon */
   dALon = fabs( dLng );                 /*  and check quality of lat/lon */
   if ( dALat > 90.001 || dALon > 180.001 )
   {                                     /* Return if lat or lon no good */
      fclose( InFile1 );
      fclose( InFile2 );
      return 0;
   }

/* GET ONSET OF QUADRANT INFO IN llindx.fer */
   if ( dLat < 0. )
   {    /* NOTE: Both +0.0 & -0.0 will belong to the No. Hemisphere */
      if ( dLng < 0. ) iQuadOn = 273;    /* quadrant onset */
      else             iQuadOn = 182;
   }
   else
   {
      if ( dLng < 0. ) iQuadOn = 91;
      else             iQuadOn = 0;
   }

/* TRUNCATE ABSOLUTE VALUES OF COORDINATES */
   iLat = (int) dALat;
   iLon = (int) dALon;

/* Read (save last) latitude of interest */
   for ( i=0; i<=iLat+iQuadOn; i++ )
      fscanf( InFile1, "%d %d", &iTierOn, &iNumRegs );

/* Read records for this latitude and see where longitude fits */
   for ( i=0; i<iTierOn-1; i++ )
      fscanf( InFile2, "%d %d\n", &iTierLon, &iFENum );
   for ( i=iTierOn-1; i<=iTierOn+iNumRegs-2; i++ )
   {
      fscanf( InFile2, "%d %d\n", &iTierLon, &iFENum );
      if ( iLon < iTierLon )
      {
         iFENum = iLastFENum;
         break;
      }
      if ( i == iTierOn+iNumRegs-2 ) break;
      iLastFENum = iFENum;
   }
   fclose( InFile1 );	
   fclose( InFile2 );	
   return( iFENum );
}

/*	subroutine namnum (geoltd, geolnd, name, number)

c	Given the geographical coordinates in decimal degrees, returns
c	the Flinn - Engdahl geographical region number (I3) and
c	name (character*32).

c	entry point NAMNBR accepts coordinates in radians.

c	ARGUMENTS:
c		1.  geographical latitude in degrees (geoltd)  -   real
c		2.  geographical longitude in degrees (geolnd)  -  real
c		3.  geographical region name			-  character*32
c		4.  F-E geographical region number		-  integer

c	Note:  Subroutines getnum & getnam require logical units 1 & 2.  To
c	       minimize the number of logical units required, these units are
c	       opened & closed with each call.  This could proove grossly
c	       inefficient for certain applications.
c	Note:  Getnum and getnam now call gtunit to get the next available
c	       unit number, to prevent conflicts with programs which may have
c	       already opened other files using units 1 and 2. (BWP 8/29/91)

Converted to C 10/99 at wc&atwc by Whitmore

*/

char *namnum( double dLat, double dLon, int *piFENum, char *pszIndexFile,
              char *pszLatFile, char *pszNameFile )
{
/* Get FE Region # given lat/lon */
   *piFENum = getnum( dLat, dLon, pszIndexFile, pszLatFile ); 
   return( getnam( *piFENum, pszNameFile ) );    /* Return FE Region name */
}

/* Add a duplicate function which calls getnamLC for lower case, readable
   messages */
char *namnumLC( double dLat, double dLon, int *piFENum, char *pszIndexFile,
                char *pszLatFile, char *pszNameFileLC )
{
/* Get FE Region # given lat/lon */
   *piFENum = getnum( dLat, dLon, pszIndexFile, pszLatFile ); 
/* Return FE Region name (lower case) */
   return( getnamLC (*piFENum, pszNameFileLC) ); 
}	
