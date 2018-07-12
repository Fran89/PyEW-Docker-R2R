
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: scan.c,v 1.2 2002/03/05 17:18:53 dietz Exp $
 *
 *    Revision history:
 *
 *     Revision 1.4  2013/05/07  nromeu
 *     Reset of variables from proxies computation 
 * 
 *     Revision 1.3  2007/01/10 17:45:01 Núria Romeu
 *		Permetre reinicialitzar LTA després de la detecció d'un event
 *
 *     $Log: scan.c,v $
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

/* Function prototypes
   *******************/
void Sample( long, STATION *, double );


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
   PROXIES *Proxies = &Sta->Proxies;  /* Pointer to proxies variables */

   TRACE_HEADER *WaveHead = (TRACE_HEADER *) WaveBuf;
   long         *WaveLong = (long *) (WaveBuf + sizeof(TRACE_HEADER));

/* Set pick and coda calculations to inactive mode
   ***********************************************/
   Pick->status = Coda->status = Proxies->status = 0;

/* Loop through all samples in the message
   ***************************************/
   while ( ++(*sample_index) < WaveHead->nsamp )
   {
      long   old_sample;                  /* Previous sample */
      long   new_sample;                  /* Current sample */
      double old_eref;                    /* Old value of eref */

      new_sample = WaveLong[*sample_index];
      old_sample = Sta->old_sample;
      old_eref   = Sta->eref;

/* Update Sta.rold, Sta.rdat, Sta.old_sample, Sta.esta,
   Sta.elta, Sta.eref, and Sta.eabs using the current sample
   *********************************************************/
      Sample( new_sample, Sta, WaveHead->samprate );

/* Station is assumed dead when (eabs > DeadSta)
   *********************************************/
      if ( Sta->eabs > Parm->DeadSta ) continue;

/* Has the short-term average abruptly increased
   with respect to the long-term average?
   *********************************************/
      if ( Sta->esta > Sta->eref )
      {
         int wi;                              /* Window index */

		 Sta->old_elta = Sta->elta;	/***/ // Revision 1.3 2007/01/10 Núria Romeu


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

         Proxies->tau0 = 0;				/* Window of time to calculate the proxies, in seconds */
         Proxies->tauc = 0;				/* Main period of the signal, in seconds */
         Proxies->pd = 0;				/* Maximum from displacement, in [cm] */
		 //
         Proxies->pow_disp_noise  = Sta->pow_disp;	/* Noise power of displacement, computation until trigger onset  */
         //
		 Proxies->pow_disp_signal = 0;  /* Signal power of displacement, computation since trigger onset for each tau0 */
         Proxies->snr3s = 0;			/* SNR, signal to noise ration which decides if proxies are of enough good */


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


/* Compute variables for proxies
   ****************************************/
   //      Sta->acc        = 0.; /* Filtered acceleration to compute proxies. 
			//					Only used with accelerometers */
   //      Sta->old_acc    = 0.; /* Previous value of filtered acceleration. 
			//					Only used with accelerometers */
   //      Sta->vel        = 0.; /* Filtered velocity to compute proxies */
   //      Sta->old_vel    = 0.; /* Previous value of filtered velocity */
	  //   Sta->disp_offset			= 0.; /* Filtered displacement to compute proxies */
	  //   Sta->old_disp_offset		= 0.; /* Previous value of filtered displacement */
   //      Sta->disp       = 0.; /* Filtered displacement to compute proxies also filtered to erase the offset from the integration */
   //      Sta->old_disp   = 0.; /* Previous value of filtered displacement also filtered to erase the offset from the integration */
		 //Sta->pow_disp   = 0.; /* Power of displacement */
         Sta->num        = 0.; /* Numerator used to calculate the proxy tauC */
         Sta->den        = 0.; /* Denominator used to calculate the proxy tauC */
         Sta->accum_eabs = 0.; /* Accumulation of eabs to compute the best tau0 */
         Sta->ave_eabs   = 0.; /* Average of eabs from the actual window */
         Sta->old_ave_eabs = 0.; /* Average of eabs from the last window */
         Sta->numeabs    = 0;  /* Proxy Tau0 index within window */


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

         Pick->status = Coda->status = Proxies->status = 1;   /* Picks/codas/proxies are now active */
         return 1;
      }
   }
   return 0;                          /* Message ended; event not found */
}
