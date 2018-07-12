/* THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU CHECKED IT OUT!
 *
 *  $Id: cdur_stalist.c 497 2009-12-08 18:46:21Z dietz $
 * 
 *  Revision history:
 *   $Log$
 *   Revision 1.2  2009/12/08 18:46:21  dietz
 *   Removed MinCodaLen parameter (was never used).
 *   Added logic to stop coda processing for gappy or dead traces.
 *
 *   Revision 1.1  2009/11/09 19:16:15  dietz
 *   Initial version, may still contain bugs and debugging statements
 *
 */

/*  cdur_stalist.c  reads parameters-of-interest from the pick_ew station 
 *                  lists
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <earthworm.h>
#include <transport.h>
#include "coda_dur.h"

/* Function prototype
   ******************/
int  IsComment( char [] );


  /***************************************************************
   *                         cdur_stalist()                      *
   *                                                             *
   *  Read interesting parameters from pick_ew station list      * 
   *  Returns -1 if an error is encountered.                     *
   ***************************************************************/

int cdur_stalist( STATION **Sta, int *Nsta, GPARM *Gparm )
{
   char    string[200];
   int     i,ifile;
   int     nstanew;
   STATION *sta;
   FILE    *fp;

/* Loop thru the station list file(s)
   **********************************/
   for( ifile=0; ifile<Gparm->nStaFile; ifile++ )
   {
      if( (fp = fopen(Gparm->StaFile[ifile].name,"r")) == NULL )
      {
         logit( "et", "coda_dur: Error opening station list file <%s>.\n",
                Gparm->StaFile[ifile].name );
         return -1;
      }

   /* Count channels in the station file.
      Ignore comment lines and lines consisting of all whitespace.
      ***********************************************************/
      nstanew = 0;
      while( fgets( string, 200, fp ) != NULL )
         if( !IsComment( string ) ) nstanew++;

      rewind( fp );

   /* Re-allocate the station list
      ****************************/
      sta = (STATION *) realloc( *Sta, (*Nsta+nstanew)*sizeof(STATION) );
      if( sta == NULL )
      {
         logit( "et", "coda_dur: Cannot reallocate the station array\n" );
         return -1;
      }
      *Sta = sta;           /* point to newly realloc'd space */
      sta  = *Sta + *Nsta;  /* point to next empty slot */

   /* Initialize internal variables in station list
      *********************************************/
      for( i=0; i<nstanew; i++ ) memset( &sta[i], 0, sizeof(STATION) );

   /* Read stations from the station list file into the station
      array, including parameters used by the picking algorithm
      *********************************************************/
      i = 0;
      while( fgets( string, 200, fp ) != NULL )
      {
         int ndecoded;
         int pickflag;

         if( IsComment( string ) ) continue;
         ndecoded = sscanf( string,
                    "%d%*d%s%s%s%s%*d%*d%*d%*d%*d%*d%*f%*f%*f%*f%*f%*f%*f%lf%lf%lf%*f",
                 &pickflag,              
             /*  &pin,                  */
                  sta[i].sta,
                  sta[i].chan,
                  sta[i].net,
                  sta[i].loc,
             /*  &sta[i].Itr1,          */
             /*  &sta[i].MinSmallZC,    */
             /*  &sta[i].MinBigZC,      */
             /*  &sta[i].MinPeakSize,   */
             /*  &sta[i].MaxMint,       */
             /*  &sta[i].MinCodaLen,    */ 
             /*  &sta[i].RawDataFilt,   */
             /*  &sta[i].CharFuncFilt,  */
             /*  &sta[i].StaFilt,       */
             /*  &sta[i].LtaFilt,       */
             /*  &sta[i].EventThresh,   */
             /*  &sta[i].RmavFilt,      */
             /*  &sta[i].DeadSta,       */
                 &sta[i].CodaTerm,        
                 &sta[i].AltCoda,        
                 &sta[i].PreEvent );
             /*  &sta[i].Erefs );       */
         if( ndecoded < 8 )
         {
            logit( "et", "coda_dur: Error decoding station file.\n" );
            logit( "e", "ndecoded: %d\n", ndecoded );
            logit( "e", "Offending line:\n" );
            logit( "e", "%s\n", string );
            return -1;
         }
         if( pickflag == 0 ) continue;
         i++;
      }
      fclose( fp );
      logit( "", "coda_dur: Loaded %d channels from station list file:  %s\n",
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

   logit( "", "\nComputing coda durations for %d channel", Nsta );
   if( Nsta != 1 ) logit( "", "s" );
   logit( "", " total:\n" );

   logit( "", "       SCNL       CodaTerm AltCoda PreEvent\n" );

   for( i = 0; i < Nsta; i++ )
   {
      logit( "", "is:%3d  %-5s %-3s %-2s %-2s  %7.2lf %6.2lf %6.2lf\n",     
            i, Sta[i].sta, Sta[i].chan, Sta[i].net, Sta[i].loc, 
            Sta[i].CodaTerm, Sta[i].AltCoda, Sta[i].PreEvent );
   }
   logit( "", "\n" );
}


    /*********************************************************************
     *                             IsComment()                           *
     *                                                                   *
     *  Accepts: String containing one line from a coda_dur station list *
     *  Returns: 1 if it's a comment line                                *
     *           0 if it's not a comment line                            *
     *********************************************************************/

int IsComment( char string[] )
{
   int i;

   for( i=0; i<(int)strlen(string); i++ )
   {
      char test = string[i];

      if( test!=' ' && test!='\t' && test!='\n' )
      {
         if ( test == '#'  ) return 1;    /* It's a comment line     */
         else                return 0;    /* It's not a comment line */
      }
   }
   return 1;                   /* It contains only whitespace */
}
