
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: pick_ra.c 5746 2013-08-07 16:30:31Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.5  2007/12/16 19:18:43  paulf
 *     fixed an improper use of long for 4 byte sample data, some OS have long as 8bytes.
 *
 *     Revision 1.4  2007/02/26 13:59:08  paulf
 *     no coda changes (option)
 *
 *     Revision 1.3  2002/03/05 17:17:03  dietz
 *     minor debug logging changes.
 *
 *     Revision 1.2  2000/07/19 21:12:24  kohler
 *     Now calculates coda lengths correctly for non-100hz data.
 *
 *     Revision 1.1  2000/02/14 19:06:49  lucky
 *     Initial revision
 *
 *
 */

    /******************************************************************
     *                            pick_ra.c                           *
     *                                                                *
     *              Contains PickRA(), a function to pick             *
     *                    one demultiplexed message                   *
     ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <earthworm.h>
#include <trace_buf.h>
#include <transport.h>
#include "pick_ew.h"
#include "sample.h"

/* Function prototypes
   *******************/
void   ReportPick( PICK *, CODA *, STATION *, GPARM *, EWH * );
void   ReportCoda( CODA *, GPARM *, EWH * );
int    ScanForEvent( STATION *, GPARM *, char *, int * );
int    EventActive( STATION *, char *, GPARM *, EWH *, int * );
double Sign( double, double );


 /***********************************************************************
  *                              PickRA()                               *
  *                   Pick one demultiplexed message                    *
  *                                                                     *
  *  Arguments:                                                         *
  *     Sta              Pointer to station being processed             *
  *     WaveBuf          Pointer to the message buffer                  *
  *                                                                     *
  *  Picks are held until it is determined that the coda length is at   *
  *  least three seconds.  Each coda is reported as a separate message  *
  *  after it's corresponding pick is reported, even if the coda        *
  *  calculation is finished before the pick is ready to report.        *
  *  Codas are released from 3 to 144 seconds after the pick time.      *
  ***********************************************************************/

/* pick and coda "status" attribute value explained:
	0 - no event active
	1 - active calculation being performed
	2 - complete  but not reported yet
	once reported, status is set back to 0

	NOTE: These should really be made #DEFINE values
*/
void PickRA( STATION *Sta, char *WaveBuf, GPARM *Gparm, EWH *Ewh )
{
   int  event_found;               /* 1 if an event was found */
   int  event_active;              /* 1 if an event is active */
   int  sample_index = -1;         /* Sample index */
   PICK *Pick = &Sta->Pick;        /* Pointer to pick variables */
   CODA *Coda = &Sta->Coda;        /* Pointer to coda variables */

/* A pick is active; continue it's calculation
   *******************************************/
   if ( Gparm->Debug ) logit( "e", "\n%s.%s.%s  Pick->status: %d  Coda->status: %d\n",
          Sta->sta, Sta->chan, Sta->net, Pick->status, Coda->status );

   if ( (Pick->status > 0) || (Coda->status > 0) )
   {
      if ( Gparm->Debug ) logit( "e", "Still in active mode.\n" );

      event_active = EventActive( Sta, WaveBuf, Gparm, Ewh, &sample_index );

      if ( event_active == 1 )           /* Event active at end of message */
         return;

      if ( (event_active == -1) && Gparm->Debug )
         logit( "e", "Coda too short. Event aborted.\n" );

      if ( (event_active == -2) && Gparm->Debug )
         logit( "e", "No recent zero crossings. Event aborted.\n" );

      if ( (event_active == -3) && Gparm->Debug )
         logit( "e", "Noise pick. Event aborted.\n" );

      if ( (event_active == 0) && Gparm->Debug )
         logit( "e", "Event over. Picks/codas reported.\n" );

      if ( Gparm->Debug )
         logit( "e", "Leaving active mode.\n" );

      /* Next, go into search mode */
   }

/* Search mode
   ***********/
   while ( 1 )
   {
      if ( Gparm->Debug ) logit( "e", "Search mode.\n" );

      /* this next call looks for threshold crossing only */
      event_found = ScanForEvent( Sta, Gparm, WaveBuf, &sample_index );

      if ( event_found )
      {
         if ( Gparm->Debug ) logit( "e", "Event found.\n" );
      }
      else
      {
         if ( Gparm->Debug ) logit( "e", "Event not found.\n" );
         return;
      }

      /* EventActive() tries to see if the event is valid and still active */
      event_active = EventActive( Sta, WaveBuf, Gparm, Ewh, &sample_index );

      if ( event_active == 1 )           /* Event active at end of message */
         return;

      if ( (event_active == -1) && Gparm->Debug )
         logit( "e", "Coda too short. Event aborted.\n" );

      if ( (event_active == -2) && Gparm->Debug )
         logit( "e", "No recent zero crossings. Event aborted.\n" );

      if ( (event_active == -3) && Gparm->Debug )
         logit( "e", "Noise pick. Event aborted.\n" );

      if ( (event_active == 0) && Gparm->Debug )
         logit( "e", "Event over. Picks/codas reported.\n" );
   } /* end of search mode while loop */
}


     /*****************************************************
      *                   EventActive()                   *
      *                                                   *
      *  Returns 1 if pick is active; otherwise 0.        *
      *****************************************************/

/* Returns negative values if problems:
	-1 if coda too short and event aborted
	-2 if no recent zero crossings; aborted
	-3 noise spike; aborted
	
	aborted means status set to 0 for both Coda and Pick
*/

int EventActive( STATION *Sta, char *WaveBuf, GPARM *Gparm, EWH *Ewh,
                 int *sample_index )
{
   PICK *Pick = &Sta->Pick;        /* Pointer to pick variables */
   CODA *Coda = &Sta->Coda;        /* Pointer to coda variables */
   PARM *Parm = &Sta->Parm;        /* Pointer to config parameters */

   TRACE_HEADER *WaveHead = (TRACE_HEADER *) WaveBuf;
   int         *WaveLong = (int *) (WaveBuf + sizeof(TRACE_HEADER));

/* An event (pick and/or coda) is active.
   See if it should be declared over.
   *************************************/
   while ( ++(*sample_index) < WaveHead->nsamp )
   {
      int new_sample;         /* Current sample */

      new_sample = WaveLong[*sample_index];

/* Update Sta.rold, Sta.rdat, Sta.old_sample, Sta.esta,
   Sta.elta, Sta.eref, and Sta.eabs using the current sample
   *********************************************************/
      Sample( new_sample, Sta );

     /********************************************************
      *                 BEGIN CODA CALCULATION               *
      ********************************************************/

/* A coda is active.  Measure coda length and amplitudes.
   Warning. The coda length calculation is correct only
   if the sampling rate is a whole number.
   *****************************************************/
      if ( Coda->status == 1 )
      {
         int lwindow = (int)(WaveHead->samprate + 0.5);        /* Coda window length in samples */

         if ( Coda->len_win != 72 ) lwindow *= 2;

         Sta->rsrdat += fabs( Sta->rdat );

/* Save windows specified in the pwin array
   ****************************************/
         if ( ++Sta->ndrt >= lwindow )
         {
            double ave_abs_val;
            const int pwin[] = {1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 16, 20, 24,
                                28, 32, 36, 40, 44, 48, 56, 64, 70, 73};

            if ( Coda->len_win++ == pwin[Sta->k] )
            {
               int i;                              /* Window index */
               for ( i = 5; i > 0; i-- )
                  Coda->aav[i] = Coda->aav[i-1];
               Sta->k++;
            }

/* Compute coda length in seconds
   ******************************/
            Coda->len_sec = (2 * Coda->len_win) - 1;
            if ( Coda->len_sec == 145 ) Coda->len_sec = 144;

/* Flush pick from buffer if coda is long enough
   *********************************************/
            if ( (Pick->status == 2) && (Coda->len_sec >= Parm->MinCodaLen) )
            {
               if ( Gparm->Debug )
                  logit( "et", "Coda->len_sec: %d\n", Coda->len_sec );

               ReportPick( Pick, Coda, Sta, Gparm, Ewh );
               Pick->status = 0;
	       if (Gparm->NoCoda || (Gparm->NoCodaHorizontal && Sta->chan[2] !='Z')) 
 	       {
                  Coda->status = 0;	/* artificially terminate coda calculation */
                  if ( Gparm->Debug )
                     logit( "et", "Debug: NoCodaHorizontal or NoCoda set for %s, Coda status set to 0, pick flushed\n", Coda->chan);
	       }
            }

/* Compute and save average absolute value
   ***************************************/
            ave_abs_val = Sta->rsrdat / (double)lwindow;
            Coda->aav[0] = (int) (ave_abs_val + .5);

/* Initialize counter and coda amp sum
   ***********************************/
            Sta->ndrt   = 0;
            Sta->rsrdat = 0.0;

/* See if the coda calculation is over.  The coda is flagged 2.
   It won't be reported until after the pick is reported.
   ************************************************************/
            if ( (Coda->len_win == 73) || (ave_abs_val < Sta->cocrit) )
            {
               if ( Coda->len_sec < Parm->MinCodaLen ) {
                  return -1;
               }

               Coda->len_out *= Coda->len_sec;
               Coda->status = 2;          /* Coda complete but not reported */
            }
         }
      }

/* The pick has been reported.
   The coda is complete but hasn't been reported
   *********************************************/
      if ( Coda->status == 2 )
      {
         if ( Pick->status == 0 )
         {
            ReportCoda( Coda, Gparm, Ewh );
            Coda->status = 0;
         }
         if ( Pick->status == 2 )
         {
            ReportPick( Pick, Coda, Sta, Gparm, Ewh );
            ReportCoda( Coda, Gparm, Ewh );
            Pick->status = Coda->status = 0;
         }
      }

     /******************************************************
      *                 END CODA CALCULATION               *
      *                BEGIN PICK CALCULATION              *
      ******************************************************/

/* A pick is active
   ****************/
      if ( Pick->status == 1 )
      {
         int    i;              /* Peak index */
         int    k;              /* Index into sarray */
         int    noise;          /* 1 if ievent is noise */
         int    itrm;           /* Number of small counts allowed before */
                                /*   pick is declared over */
         double xon;            /* Used in pick weight calculation */
         double xpc;            /* ditto */
         double xp0,xp1,xp2;    /* ditto */

/* Save first 10 points after pick for first motion determination
   **************************************************************/
         if ( ++Sta->evlen < 10 )
            Sta->sarray[Sta->evlen] = new_sample;

/* Store current data if it is a new extreme value
   ***********************************************/
         if ( Sta->next < 3 )
         {
            double adata;
            adata = fabs( Sta->rdat );
            if ( adata > Sta->tmax ) Sta->tmax = adata;
         }

/* Test for large zero crossing.  Large zero-crossing
   amplitude must exceed ebig and must represent a
   crossing of opposite polarity to previous crossing.
   **************************************************/
/*       printf( "pick_ew: rdat, rbig, rlast: %.2lf %.2lf %.2lf\n",
                 Sta->rdat, Sta->rbig, Sta->rlast ); */

         if ( fabs( Sta->rdat ) >= Sta->rbig )
         {
            if ( Sign( Sta->rdat, Sta->rlast ) != Sta->rdat )
            {
               Sta->nzero++;
               Sta->rlast = Sta->rdat;
/*             printf( "nzero: %d\n", Sta->nzero ); */
            }
         }

/* Increment zero crossing interval counter.  Terminate
   pick if no zero crossings have occurred recently.
   ****************************************************/
         if ( ++Sta->mint > Parm->MaxMint )
            return -2;

/* Test for small zero crossing
   ****************************/
         if ( Sign( Sta->rdat, Sta->rold ) == Sta->rdat )
            continue;

/* Small zero crossing found.
   Reset zero crossing interval counter.
   ************************************/
         Sta->mint = 0;

/* Update ecrit and determine whether at this crossing esta is still
   above ecrit.  If not, increment isml, the number of successive
   small crossings.  If esta is greater than ecrit, reset isml to 0.
   *****************************************************************/
         Sta->ecrit += Sta->crtinc;
         Sta->isml++;
         if ( Sta->esta > Sta->ecrit ) Sta->isml = 0;

/* Store extrema of preceeding half cycle
   **************************************/
         if ( Sta->next < 3 )
         {
            Pick->xpk[Sta->next++] = Sta->tmax;

            if ( Sta->next == 1 )
            {
               double vt3;
               vt3 = Sta->tmax / 3.;
               Sta->rbig = ( vt3 > Sta->rbig ) ? vt3 : Sta->rbig;
            }

            Sta->tmax = 0.;
         }

/* Compute itrm, the number of small zero crossings
   crossings to be allowed before declaring the
   pick over.  This will start out quite small
   and increase during the event to a maximum of 50.
   ************************************************/
         itrm = Parm->Itr1 + (Sta->m / Parm->Itr1);

         if ( Sta->m > 150 ) itrm = 50;

/*  See if the pick is over
    ***********************/
         if ( (++Sta->m != Parm->MinSmallZC) && (Sta->isml < itrm) )
            continue;                    /* It's not over */

/*  See if the pick was a noise pick.
    If so, terminate pick.
    ********************************/
         if ( Gparm->Debug )
            logit( "e", "xpk: %.0lf %.0lf %.0lf  m: %d  nzero: %d\n", 
                   Pick->xpk[0], Pick->xpk[1], Pick->xpk[2], 
                   Sta->m, Sta->nzero );
         noise = 1;

         for ( i = 0; i < 3; i++ )
            if ( Pick->xpk[i] >= (double) Parm->MinPeakSize )
            {
               if ( (Sta->m == Parm->MinSmallZC) &&
                    (Sta->nzero >= Parm->MinBigZC) )
                  noise = 0;
               break;
            }

         if ( noise ) return -3;

/* A valid pick was found.
   Determine the first motion.
   ***************************/
         Pick->FirstMotion = ' ';            /* First motion unknown */

         for ( k = 0; 1; k++ )
         {
            if ( Sta->xdot <= 0 )
            {
               if ( (Sta->sarray[k+1] > Sta->sarray[k]) || (k == 8) )
               {
                  if ( k == 0 ) break;
                  Pick->FirstMotion = 'D';   /* First motion is down */
                  break;
               }
            }
            else
            {
               if ( (Sta->sarray[k+1] < Sta->sarray[k]) || (k == 8) )
               {
                  if ( k == 0 ) break;
                  Pick->FirstMotion = 'U';   /* First motion is up */
                  break;
               }
            }
         }

/* Pick weight calculation
   ***********************/
         xpc = ( Pick->xpk[0] > fabs( (double)Sta->sarray[0] ) ) ?
               Pick->xpk[0] : Pick->xpk[1];
         xon = fabs( (double)Sta->xdot / Sta->xfrz );
         xp0 = Pick->xpk[0] / Sta->xfrz;
         xp1 = Pick->xpk[1] / Sta->xfrz;
         xp2 = Pick->xpk[2] / Sta->xfrz;

         Pick->weight = 3;

         if ( (xp0 > 2.) && (xon > .5) && (xpc > 25.) )
            Pick->weight = 2;

         if ( (xp0 > 3.) && ((xp1 > 3.) || (xp2 > 3.)) && (xon > .5)
         && (xpc > 100.) )
            Pick->weight = 1;

         if ( (xp0 > 4.) && ((xp1 > 6.) || (xp2 > 6.)) && (xon > .5)
         && (xpc > 200.) )
            Pick->weight = 0;

         Pick->status = 2;               /* Pick calculated but not reported */

/* Report pick and coda
   ********************/
         if ( Coda->status == 2 )
         {
            ReportPick( Pick, Coda, Sta, Gparm, Ewh );
            ReportCoda( Coda, Gparm, Ewh );
            Pick->status = Coda->status = 0;
         }
      }

     /******************************************************
      *                 END PICK CALCULATION               *
      ******************************************************/

/* If the pick is over and coda measurement
   is done, scan for a new event.
   ****************************************/
      if ( (Pick->status == 0) && (Coda->status == 0) )
         return 0;
   }
   return 1;       /* Event is still active */
}
