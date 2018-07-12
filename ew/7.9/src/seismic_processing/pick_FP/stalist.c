/*
 * stalist.c
 * Modified from stalist.c, Revision 1.6 in src/seismic_processing/pick_ew
 *
 * This file is part of pick_FP, Earthworm module for the FilterPicker phase detector and picker.
 *
 * (C) 2008-2012 Claudio Satriano <satriano@ipgp.fr>,
 * under the same license terms of the Earthworm software system. 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <earthworm.h>
#include <transport.h>
#include "pick_FP.h"

#define MAX_LEN_STRING_STALIST 512

/* Function prototype
   ******************/
void InitVar( STATION * );
int  IsComment( char [] );


  /***************************************************************
   *                         GetStaList()                        *
   *                                                             *
   *                     Read the station list                   *
   *                                                             *
   *  Returns -1 if an error is encountered.                     *
   ***************************************************************/

int GetStaList( STATION **Sta, int *Nsta, GPARM *Gparm )
{
   char    string[MAX_LEN_STRING_STALIST];
   int     i,ifile;
   int     nstanew;
   STATION *sta;
   FILE    *fp;

/* Loop thru the station list file(s)
   **********************************/
   for( ifile=0; ifile<Gparm->nStaFile; ifile++ )
   {
      if ( ( fp = fopen( Gparm->StaFile[ifile].name, "r") ) == NULL )
      {
         logit( "et", "pick_FP: Error opening station list file <%s>.\n",
                Gparm->StaFile[ifile].name );
         return -1;
      }

   /* Count channels in the station file.
      Ignore comment lines and lines consisting of all whitespace.
      ***********************************************************/
      nstanew = 0;
      while ( fgets( string, MAX_LEN_STRING_STALIST, fp ) != NULL )
         if ( !IsComment( string ) ) nstanew++;

      rewind( fp );

   /* Re-allocate the station list
      ****************************/
      sta = (STATION *) realloc( *Sta, (*Nsta+nstanew)*sizeof(STATION) );
      if ( sta == NULL )
      {
         logit( "et", "pick_FP: Cannot reallocate the station array\n" );
         return -1;
      }
      *Sta = sta;           /* point to newly realloc'd space */
      sta  = *Sta + *Nsta;  /* point to next empty slot */

   /* Initialize internal variables in station list
      *********************************************/
      for ( i = 0; i < nstanew; i++ ) {
         sta[i].mem = NULL;
         InitVar( &sta[i] );
      }

   /* Read stations from the station list file into the station
      array, including parameters used by the picking algorithm
      *********************************************************/
      i = 0;
      while ( fgets( string, MAX_LEN_STRING_STALIST, fp ) != NULL )
      {
         int ndecoded;
         int pickflag;
         int pin;

         if ( IsComment( string ) ) continue;
         ndecoded = sscanf( string, "%d %d %s %s %s %s %lf %lf %lf %lf %lf", 
                 &pickflag,
                 &pin,
                  sta[i].sta,
                  sta[i].chan,
                  sta[i].net,
                  sta[i].loc,
                 &sta[i].Parm.filterWindow,
                 &sta[i].Parm.longTermWindow,
                 &sta[i].Parm.threshold1,
                 &sta[i].Parm.threshold2,
                 &sta[i].Parm.tUpEvent
                 );
         if ( ndecoded < 11 )
         {
            logit( "et", "pick_FP: Error decoding station file.\n" );
            logit( "e", "ndecoded: %d\n", ndecoded );
            logit( "e", "Offending line:\n" );
            logit( "e", "%s\n", string );
            return -1;
         }
         if ( pickflag == 0 ) continue;
         i++;
      }
      fclose( fp );
      logit( "", "pick_FP: Loaded %d channels from station list file:  %s\n",
             i, Gparm->StaFile[ifile].name);
      Gparm->StaFile[ifile].nsta = i;
      *Nsta += i;
   } /* end for over all StaFiles */
   return 0;
}


 /***********************************************************************
  *                             LogStaList()                            *
  *                                                                     *
  *                         Log the station list                        *
  ***********************************************************************/

void LogStaList( STATION *Sta, int Nsta )
{
   int i;

   logit( "", "\nPicking %d channel", Nsta );
   if ( Nsta != 1 ) logit( "", "s" );
   logit( "", " total:\n" );

   for ( i = 0; i < Nsta; i++ )
   {
      logit( "", "%-5s",     Sta[i].sta );
      logit( "", " %-3s",    Sta[i].chan );
      logit( "", " %-2s",    Sta[i].net );
      logit( "", " %-2s",    Sta[i].loc );
      logit( "", "  %5.3lf",    Sta[i].Parm.filterWindow );
      logit( "", "  %5.3lf",    Sta[i].Parm.longTermWindow );
      logit( "", "  %5.3lf",    Sta[i].Parm.threshold1 );
      logit( "", "  %5.3lf",    Sta[i].Parm.threshold2 );
      logit( "", "  %5.3lf",    Sta[i].Parm.tUpEvent );
      logit( "", "\n" );
   }
   logit( "", "\n" );
}


    /*********************************************************************
     *                             IsComment()                           *
     *                                                                   *
     *  Accepts: String containing one line from a pick_FP station list  *
     *  Returns: 1 if it's a comment line                                *
     *           0 if it's not a comment line                            *
     *********************************************************************/

int IsComment( char string[] )
{
   int i;

   for ( i = 0; i < (int)strlen( string ); i++ )
   {
      char test = string[i];

      if ( test!=' ' && test!='\t' && test!='\n' )
      {
         if ( test == '#'  )
            return 1;          /* It's a comment line */
         else
            return 0;          /* It's not a comment line */
      }
   }
   return 1;                   /* It contains only whitespace */
}
