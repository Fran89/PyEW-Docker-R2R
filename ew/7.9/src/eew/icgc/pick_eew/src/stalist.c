
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: stalist.c,v 1.2 2003/03/24 22:19:46 lombard Exp $
 *
 *    Revision history:
 *     $Log: stalist.c,v $
 *
 *     Revision 1.3  2013/05/07  nromeu
 *     Increased string length to MAX_LEN_STRING_STALIST, now set to 512
 *     New parameters for Early Warning Pick_ew: MaxCodaLen, ProxiesFilt, 
 *     Sensibility, Tau0Min, Tau0Inc,Tau0Max
 *
 *     Revision 1.2  2003/03/24 22:19:46  lombard
 *     Nsta erroneously set to sta; should be set to i to get correct
 *     number of picked stations.
 *
 *     Revision 1.1  2000/02/14 19:06:49  lucky
 *     Initial revision
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <earthworm.h>
#include <transport.h>
#include "pick_ew.h"

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
   int     i;
   int     nsta;
   STATION *sta;
   FILE    *fp;


/* Open the station list file
   **************************/
   if ( ( fp = fopen( Gparm->StaFile, "r") ) == NULL )
   {
      logit( "et", "pick_ew: Error opening station list file <%s>.\n",
             Gparm->StaFile );
      return -1;
   }

/* Count channels in the station file.
   Ignore comment lines and lines consisting of all whitespace.
   ***********************************************************/
   nsta = 0;
   while ( fgets( string, MAX_LEN_STRING_STALIST, fp ) != NULL )
      if ( !IsComment( string ) ) nsta++;

   rewind( fp );

/* Allocate the station list
   *************************/
   sta = (STATION *) calloc( nsta, sizeof(STATION) );
   if ( sta == NULL )
   {
      logit( "et", "pick_ew: Cannot allocate the station array\n" );
      return -1;
   }

/* Initialize internal variables in station list
   *********************************************/
   for ( i = 0; i < nsta; i++ )
      InitVar( &sta[i] );

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

      ndecoded = sscanf( string,
              "%d%d%s%s%s%d%d%d%ld%d%d%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%*ld%d%lf%lE%lf%lf%lf%lf%lf",
              &pickflag,
              &pin,
               sta[i].sta,
               sta[i].chan,
               sta[i].net,
              &sta[i].Parm.Itr1,
              &sta[i].Parm.MinSmallZC,
              &sta[i].Parm.MinBigZC,
              &sta[i].Parm.MinPeakSize,
              &sta[i].Parm.MaxMint,
              &sta[i].Parm.MinCodaLen,
              &sta[i].Parm.RawDataFilt,
              &sta[i].Parm.CharFuncFilt,
              &sta[i].Parm.StaFilt,
              &sta[i].Parm.LtaFilt,
              &sta[i].Parm.EventThresh,
              &sta[i].Parm.RmavFilt,
              &sta[i].Parm.DeadSta,
              &sta[i].Parm.CodaTerm,
              &sta[i].Parm.AltCoda,
              &sta[i].Parm.PreEvent,
              &sta[i].Parm.Erefs,
              &sta[i].Parm.MaxCodaLen,
              &sta[i].Parm.ProxiesFilt,
              &sta[i].Parm.Sensibility,
              &sta[i].Parm.Tau0Min,		
              &sta[i].Parm.Tau0Inc,		
              &sta[i].Parm.Tau0Max,
			  &sta[i].Parm.PowDispFilt,
			  &sta[i].Parm.snrThreshold3s
			  );

      if ( ndecoded < 28 )
      {
         logit( "et", "pick_ew: Error decoding station file.\n" );
         logit( "e", "ndecoded: %d\n", ndecoded );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", string );
         return -1;
      }
      if(	 strlen(sta[i].chan)!= 3 
		  || ( strlen(sta[i].chan)== 3 && sta[i].chan[1]!='H' /*&& sta[i].chan[1]!='N' && sta[i].chan[1]!='G'*/ )
		  )
	  {
         logit( "et", "pick_ew: Error decoding station file.\n" );
         logit( "e", "Channel not supported: must have three characters and \n" );
         logit( "e", "the instrument code must be 'H' for seismometers.\n" );
         logit( "e", "accelerometers ('N' or 'G') not yet supported.\n" );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", string );
         return -1; 
      }
      if ( sta[i].Parm.RawDataFilt >= 1. || sta[i].Parm.ProxiesFilt >= 1. )
      {
         logit( "et", "pick_ew: Error decoding station file.\n" );
         logit( "e", "<RawDataFilt> and <ProxiesFilt> can not be greater than 1 \n" );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", string );
         return -1;
      }
      if ( sta[i].Parm.MaxCodaLen < sta[i].Parm.MinCodaLen )
      {
         logit( "et", "pick_ew: Error decoding station file.\n" );
         logit( "e", "Inappropiated coda length interval\n" );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", string );
         return -1;
      }	  
      if ( sta[i].Parm.Sensibility <= 0 )
      {
         logit( "et", "pick_ew: Error decoding station file.\n" );
         logit( "e", "<Sensibility> not accepted: %g\n", sta[i].Parm.Sensibility );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", string );
         return -1;
      }
      if ( sta[i].Parm.Tau0Max < sta[i].Parm.Tau0Min )
      {
         logit( "et", "pick_ew: Error decoding station file.\n" );
         logit( "e", "Inappropiated Tau0 interval\n" );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", string );
         return -1;
      }	  
      if ( sta[i].Parm.snrThreshold3s <= 0 )
      {
         logit( "et", "pick_ew: Error decoding station file.\n" );
         logit( "e", "<snrThreshold3s> not accepted: %g\n", sta[i].Parm.snrThreshold3s );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", string );
         return -1;
      }

	  if ( pickflag == 0 ) continue;
      i++;
   }
   fclose( fp );
   *Sta  = sta;
   *Nsta = i;
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

   logit( "", "\nStation List:\n" );
   for ( i = 0; i < Nsta; i++ )
   {
      logit( "", "%-5s",     Sta[i].sta );
      logit( "", " %-3s",    Sta[i].chan );
      logit( "", " %-2s",    Sta[i].net );
      logit( "", "  %1d",    Sta[i].Parm.Itr1 );
      logit( "", "  %2d",    Sta[i].Parm.MinSmallZC );
      logit( "", "  %1d",    Sta[i].Parm.MinBigZC );
      logit( "", "  %2ld",   Sta[i].Parm.MinPeakSize );
      logit( "", "  %3d",    Sta[i].Parm.MaxMint );
      logit( "", "  %3d",    Sta[i].Parm.MinCodaLen );
      logit( "", "  %5.3lf", Sta[i].Parm.RawDataFilt );
      logit( "", "  %3.1lf", Sta[i].Parm.CharFuncFilt );
      logit( "", "  %3.1lf", Sta[i].Parm.StaFilt );
      logit( "", "  %4.2lf", Sta[i].Parm.LtaFilt );
      logit( "", "  %3.1lf", Sta[i].Parm.EventThresh );
      logit( "", "  %5.3lf", Sta[i].Parm.RmavFilt );
      logit( "", "  %4.0lf", Sta[i].Parm.DeadSta );
      logit( "", "  %5.2lf", Sta[i].Parm.CodaTerm );
      logit( "", "  %3.1lf", Sta[i].Parm.AltCoda );
      logit( "", "  %3.1lf", Sta[i].Parm.PreEvent );
      logit( "", "  %7.1lf", Sta[i].Parm.Erefs );

      logit( "", "  %3d",    Sta[i].Parm.MaxCodaLen );
      logit( "", "  %7.4lf", Sta[i].Parm.ProxiesFilt );
      logit( "", "  %e",     Sta[i].Parm.Sensibility );
      logit( "", "  %3.1lf", Sta[i].Parm.Tau0Min );
      logit( "", "  %3.1lf", Sta[i].Parm.Tau0Inc );
      logit( "", "  %3.1lf", Sta[i].Parm.Tau0Max );
      logit( "", "  %5.3lf", Sta[i].Parm.PowDispFilt );
      logit( "", "  %3.1lf", Sta[i].Parm.snrThreshold3s );


      logit( "", "\n" );
   }
   logit( "", "\n" );
}


    /*********************************************************************
     *                             IsComment()                           *
     *                                                                   *
     *  Accepts: String containing one line from a pick_ew station list  *
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
