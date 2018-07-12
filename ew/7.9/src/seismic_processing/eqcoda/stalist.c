
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: stalist.c 5852 2013-08-16 12:44:47Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2007/01/10 01:22:48  dietz
 *     Changed to ignore lines with pickflag=0
 *
 *     Revision 1.5  2006/09/20 23:19:42  dietz
 *     Fixed bug in setting UsedDefault flag (referring to clipping value)
 *
 *     Revision 1.4  2006/09/20 18:34:29  dietz
 *     Modified to read per-channel parameters from more than one "StaFile"
 *
 *     Revision 1.3  2004/05/18 20:21:00  dietz
 *     Modified to use TYPE_EVENT_SCNL as input and to include location
 *     codes in the TYPE_HYP2000ARC output msg.
 *
 *     Revision 1.2  2000/08/17 20:09:28  dietz
 *     Changed configurable clip param from DigNbit to ClipCount
 *
 *     Revision 1.1  2000/07/21 23:09:31  dietz
 *     Initial revision
 *
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <earthworm.h>
#include <transport.h>
#include "eqcoda.h"

/* External Variables, from eqcoda.c
 ***********************************/
extern long   DefaultClipCount; /* default clip count (0-to-peak)      */
extern double FracKlipC;        /* Clip: coda 2-sec avg abs amplitudes */
extern double FracKlipP1;       /* Clip: first P-amplitude             */
extern double FracKlipP2;       /* Clip: 2nd & 3rd P-amplitude         */

/* Function prototype
   ******************/
int  IsComment( char [] );
int  PickFlag( char [] );

/* Maximum length for reading a line from the station list file */
#define MAX_LEN_STRING_STALIST 512

  /***************************************************************
   *                         GetStaList()                        *
   *                                                             *
   *                     Read the station list                   *
   *                                                             *
   *  Returns -1 if an error is encountered.                     *
   ***************************************************************/

int GetStaList( STAPARM **Sta, int *Nsta, char *stafile )
{
   char    string[MAX_LEN_STRING_STALIST];
   int     i;
   int     nstanew;
   STAPARM *sta;
   STAPARM *tmp;
   FILE    *fp;

/* Open the station list file
   **************************/
   if ( ( fp = fopen( stafile, "r") ) == NULL )
   {
      logit( "et", "eqcoda: Error opening station list <%s>.\n",
             stafile );
      return -1;
   }

/* Count channels in the station file.
   Ignore comment lines and lines consisting of all whitespace.
   ***********************************************************/
   nstanew = 0;
   while ( fgets( string, MAX_LEN_STRING_STALIST-1, fp ) != NULL ) 
   {
      if ( IsComment( string )    ) continue;
      if ( PickFlag( string ) > 0 ) nstanew++;
   }

   rewind( fp );

/* Re-allocate the station list
   ****************************/
   tmp = (STAPARM *) realloc( *Sta, sizeof(STAPARM)*(*Nsta+nstanew) );
   if ( tmp == NULL )
   {
      logit( "et", "eqcoda: Cannot re-allocate the station array\n" );
      return -1;
   }
   *Sta = tmp;
   sta  = *Sta + *Nsta;  /* point to first empty slot */

/* Read stations from the station list file into the station array. 
   This file includes mostly picking parameters used by pick_ew,
   so we ignore most of them.  Some pick files may not have the
   last column, which was added specifically for eqcoda to establish
   the per-channel clipping level (counts 0-to-peak).  If this field 
   is missing, a default of DefaultClipCount will be used.
   *****************************************************************/
   i = 0;
   while ( fgets( string, MAX_LEN_STRING_STALIST-1, fp ) != NULL )
   {
      int ndecoded;
      int pickflag;

      if ( IsComment( string ) ) continue;
      pickflag = PickFlag( string );
      if ( pickflag == -1 )
      {
         logit( "et", "eqcoda: Error reading pickflag from station file.\n" );
         logit( "e", "%s\n", string );
         return -1;
      }
      if ( pickflag == 0 ) continue;

      ndecoded = sscanf( string,
              "%*d%*d%s%s%s%s%*d%*d%*d%*d%*d%*d%*f%*f%*f%*f%*f%*f%*f%f%*f%*f%*f%ld",
               sta[i].sta,
               sta[i].chan,
               sta[i].net,
               sta[i].loc,
              &sta[i].CodaTerm,
              &sta[i].ClipCount );

      if ( ndecoded == 6 )
      {
         sta[i].UsedDefault = 0;
      }
      else if ( ndecoded == 5 )
      {
         logit( "e", "eqcoda: No ClipCount parameter for %s %s %s %s; "
                      "default (%ld) will be used.\n", 
                      sta[i].sta, sta[i].chan, sta[i].net, sta[i].loc,
                      DefaultClipCount );
         sta[i].ClipCount   = DefaultClipCount;
         sta[i].UsedDefault = 1;
      }
      else if ( ndecoded < 5 )
      {
         logit( "e", "eqcoda: Error decoding station file;" );
         logit( "e", " uses 6 fields (#3,4,5,6,20,24), decoded %d.\n", 
                 ndecoded );
         logit( "e", "Offending line:\n%s\n", string );
         return -1;
      }
      i++;
   }
   fclose( fp );
   SetParmStaList( sta, nstanew );
   *Nsta += nstanew;
   return 0;
}


 /***********************************************************************
  *                           SetParmStaList()                          *
  *                                                                     *
  * Given values read from station list, fill out remaining parameters  *
  ***********************************************************************/

void SetParmStaList( STAPARM *sta, int nsta )
{
   int i;

   for ( i = 0; i < nsta; i++ )
   {
      sta[i].KlipP1 = (long)(FracKlipP1 * (double)sta[i].ClipCount + 0.5);
      sta[i].KlipP2 = (long)(FracKlipP2 * (double)sta[i].ClipCount + 0.5);
      sta[i].KlipC  = (long)(FracKlipC  * (double)sta[i].ClipCount + 0.5);
   }
}


 /***********************************************************************
  *                             LogStaList()                            *
  *                                                                     *
  *                         Log the station list                        *
  ***********************************************************************/
 
void LogStaList( STAPARM *sta, int nsta )
{
   int i;
   char dflt;
 
   logit( "", " sta   cmp nt lc  CodaTerm   ClipCount       KlipP1     KlipP2      KlipC\n" );
   for ( i = 0; i < nsta; i++ )
   {
      if( !sta[i].UsedDefault ) dflt = ' ';
      else                      dflt = '*';
      logit( "", " %-5s %-3s %-2s %-2s  %8.2f  %10ld %c %10ld %10ld %10ld\n",
             sta[i].sta, sta[i].chan, sta[i].net, sta[i].loc, sta[i].CodaTerm,
             sta[i].ClipCount, dflt, sta[i].KlipP1, sta[i].KlipP2, 
             sta[i].KlipC );
   }
   logit( "", "NOTE: * right of ClipCount means default value was used.\n\n" );
}   


    /*********************************************************************
     *                             IsComment()                           *
     *                                                                   *
     *  Accepts: String containing one line from a eqcoda station list   *
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


    /*********************************************************************
     *                            PickFlag()                             *
     *                                                                   *
     *  Accepts: String containing one line from a eqcoda station list   *
     *  Returns: 1 if its pick flag is non-zero                          *
     *           0 if its pick flag is zero                              *
     *          -1 if there's an error reading the pick flag             *
     *********************************************************************/

int PickFlag( char string[] )
{
   int ndecoded;
   int pickflag;

   ndecoded = sscanf( string, "%d", &pickflag );
   if ( ndecoded < 1  ) return -1;   /* error reading pickflag */
   if ( pickflag == 0 ) return  0;
   return 1;
}

     /*************************************************************
      *                       CompareSCNLs()                      *
      *                                                           *
      *  This function is passed to qsort() and bsearch().        *
      *  We use qsort() to sort the station list by SCNL numbers, *
      *  and we use bsearch to look up an SCNL in the list.       *
      *************************************************************/
 
int CompareSCNLs( const void *s1, const void *s2 )
{
   int rc;
   STAPARM *t1 = (STAPARM *) s1;
   STAPARM *t2 = (STAPARM *) s2;
 
   rc = strcmp( t1->sta, t2->sta );
   if ( rc != 0 ) return rc;
   rc = strcmp( t1->chan, t2->chan );
   if ( rc != 0 ) return rc;
   rc = strcmp( t1->net,  t2->net );
   if ( rc != 0 ) return rc;
   rc = strcmp( t1->loc,  t2->loc );
   return rc;
}
