
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: scan.c 6233 2015-02-17 19:55:38Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2007/12/16 19:18:43  paulf
 *     fixed an improper use of long for 4 byte sample data, some OS have long as 8bytes.
 *
 *     Revision 1.2  2002/03/05 17:18:53  dietz
 *     minor debug logging changes
 *
 *     Revision 1.1  2000/02/14 19:06:49  lucky
 *     Initial revision
 *
 *
 */

         /**********************************************
          *                   scan.c                   *
          **********************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <chron3.h>
#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include "pick_ew.h"
#include "sample.h"


     /*****************************************************
      *                   ScanForEvent()                  *
      *                                                   *
      *              Search for a pick event.             *
      *                                                   *
      *  Returns 1 if event found; otherwise 0.           *
      *****************************************************/

int ScanForEvent( STATION *Sta, GPARM *Gparm, char *WaveBuf, int *sample_index )
{
   PICK *Pick = &Sta->Pick;        /* Pointer to pick variables */
   CODA *Coda = &Sta->Coda;        /* Pointer to coda variables */
   PARM *Parm = &Sta->Parm;        /* Pointer to config parameters */

   TRACE_HEADER *WaveHead = (TRACE_HEADER *) WaveBuf;
   int         *WaveLong = (int *) (WaveBuf + sizeof(TRACE_HEADER));

/* Set pick and coda calculations to inactive mode
   ***********************************************/
   Pick->status = Coda->status = 0;

/* Loop through all samples in the message
   ***************************************/
   while ( ++(*sample_index) < WaveHead->nsamp )
   {
      int   old_sample;                  /* Previous sample */
      int   new_sample;                  /* Current sample */
      double old_eref;                    /* Old value of eref */

      new_sample = WaveLong[*sample_index];
      old_sample = Sta->old_sample;
      old_eref   = Sta->eref;

/* Update Sta.rold, Sta.rdat, Sta.old_sample, Sta.esta,
   Sta.elta, Sta.eref, and Sta.eabs using the current sample
   *********************************************************/
      Sample( new_sample, Sta );

/* Station is assumed dead when (eabs > DeadSta) - ignore DeadSta if value is 0.0 or less
   *********************************************/
      if ( Parm->DeadSta > 0.0 && Sta->eabs > Parm->DeadSta ) continue;

/* Has the short-term average abruptly increased
   with respect to the long-term average?
   *********************************************/
      if ( Sta->esta > Sta->eref )
      {
         int wi;                              /* Window index */

/* Initialize pick variables
   *************************/
         Pick->time = WaveHead->starttime + 11676096000. +
                      (double)*sample_index / WaveHead->samprate;
         if ( Gparm->Debug ) {
            char datestr[20];
            date17( Pick->time, datestr );
            logit( "e", "Pick time: %.3lf  %s\n", Pick->time, datestr );
         }

         Coda->len_win = 0;             /* Coda length in windows */
         Coda->len_sec = 0;             /* Coda length in seconds */

         for ( wi = 0; wi < 6; wi++ )
               Coda->aav[wi] = 0;

         Sta->crtinc    = Sta->eref / Parm->Erefs;
         Sta->ecrit     = old_eref;
         Sta->evlen     = 0;
         Sta->isml      = 0;
         Sta->k         = 0;
         Sta->m         = 1;
         Sta->mint      = 0;
         Sta->ndrt      = 0;
         Sta->next      = 0;
         Sta->nzero     = 0;
         Sta->rlast     = Sta->rdat;
         Sta->rsrdat    = 0.;
         Sta->sarray[0] = new_sample;
         Sta->tmax      = fabs( Sta->rdat );
         Sta->xfrz      = 1.6 * Sta->eabs;

/* Compute threshold for big zero crossings
   ****************************************/
         Sta->xdot = new_sample - old_sample;
         Sta->rbig = ( (Sta->xdot < 0) ? -Sta->xdot : Sta->xdot ) / 3.;
         Sta->rbig = (Sta->eabs > Sta->rbig) ? Sta->eabs : Sta->rbig;

/* Compute cocrit and the sign of
   Coda->len_out for big and small events
   **************************************/
         if ( Sta->eabs > (Parm->AltCoda * Parm->CodaTerm) )  /* Big */
         {
            Sta->cocrit = Parm->PreEvent * Sta->eabs;
            Coda->len_out = -1;
         }
         else                                                 /* Small */
         {
            Sta->cocrit  = Parm->CodaTerm;
            Coda->len_out = 1;
         }

         Pick->status = Coda->status = 1;   /* Picks/codas are now active */
         return 1;
      }
   }
   return 0;                          /* Message ended; event not found */
}
