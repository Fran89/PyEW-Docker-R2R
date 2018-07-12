
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: initvar.c,v 1.1 2000/02/14 19:06:49 lucky Exp $
 *
 *    Revision history:
 *     $Log: initvar.c,v $
 *
 *     Revision 1.2  2013/05/07  nromeu
 *     Reset of variables and parameters from proxies computation 
 * 
 *     Revision 1.1  2000/02/14 19:06:49  lucky
 *     Initial revision
 *
 *
 */

#include <earthworm.h>
#include <transport.h>
#include "pick_ew.h"


   /*******************************************************************
    *                            InitVar()                            *
    *                                                                 *
    *       Initialize STATION variables to 0 for one station.        *
    *******************************************************************/

void InitVar( STATION *Sta )
{
   int i;
   PICK *Pick = &Sta->Pick;         /* Pointer to pick structure */
   CODA *Coda = &Sta->Coda;         /* Pointer to coda structure */
   PROXIES *Proxies = &Sta->Proxies;/* Pointer to proxies structure */

   Sta->cocrit     = 0.; /* Threshold at which to terminate coda */
   Sta->crtinc     = 0.; /* Increment added to ecrit at each zero crossing */
   Sta->eabs       = 0.; /* Running mean absolute value (aav) of rdat */
   Sta->ecrit      = 0.; /* Criterion level to determine if event is over */
   Sta->elta       = 0.; /* Long-term average of edat */
   Sta->old_elta   = 0.; /* Long-term average of edat at ONSET */
   Sta->enddata    = 0L; /* Sample at end of previous message */
   Sta->endtime    = 0.; /* Time at end of previous message */
   Sta->eref       = 0.; /* STA/LTA reference level */
   Sta->esta       = 0.; /* Short-term average of edat */
   Sta->evlen      = 0;  /* Event length in samp */
   Sta->first      = 1;  /* No messages with this channel have been detected */
   Sta->isml       = 0;  /* Small zero-crossing counter */
   Sta->k          = 0;  /* Index to array of windows to push onto stack */
   Sta->m          = 0;  /* 0 if no event; otherwise, zero-crossing counter */
   Sta->ndrt       = 0;  /* Coda length index within window */
   Sta->next       = 0;  /* Counter of zero crossings early in P-phase */
   Sta->ns_restart = 0;  /* Restart sample count */
   Sta->nzero      = 0;  /* Big zero-crossing counter */
   Sta->old_sample = 0;  /* Old value of integer data */
   Sta->rdat       = 0.; /* Filtered data value */
   Sta->rbig       = 0.; /* Threshold for big zero crossings */
   Sta->rlast      = 0.; /* Size of last big zero crossing */
   Sta->rold       = 0.; /* Previous value of filtered data */
   Sta->rsrdat     = 0.; /* Running sum of rdat in coda calculation */
   Sta->tmax       = 0.; /* Instantaneous maximum in current half cycle */
   Sta->xdot       = 0 ; /* First difference at pick time */
   Sta->xfrz       = 0.; /* Used in first motion calculation */
   /***/
   Sta->acc        = 0.; /* Filtered acceleration to compute proxies. 
							Only used with accelerometers */
   Sta->old_acc    = 0.; /* Previous value of filtered acceleration. 
							Only used with accelerometers */
   Sta->vel        = 0.; /* Filtered velocity to compute proxies */
   Sta->old_vel    = 0.; /* Previous value of filtered velocity */
   Sta->disp_offset			= 0.; /* Filtered displacement to compute proxies */
   Sta->old_disp_offset		= 0.; /* Previous value of filtered displacement */
   Sta->disp       = 0.; /* Filtered displacement to compute proxies also filtered to erase the offset from the integration */
   Sta->old_disp   = 0.; /* Previous value of filtered displacement also filtered to erase the offset from the integration */
   Sta->pow_disp   = 0.; /* Power of displacement */
   Sta->num        = 0.; /* Numerator used to calculate the proxy tauC */
   Sta->den        = 0.; /* Denominator used to calculate the proxy tauC */
   Sta->accum_eabs = 0.; /* Accumulation of eabs to compute the best tau0 */
   Sta->ave_eabs   = 0.; /* Average of eabs from the actual window */
   Sta->old_ave_eabs = 0.; /* Average of eabs from the last window */
   Sta->numeabs    = 0;  /* Proxy Tau0 index within window */

   for ( i = 0; i < 10; i++ )
      Sta->sarray[i] = 0;         /* First 10 points of first motion */

/* Pick variables
   **************/
   Pick->time = 0.;         /* Pick time */

   for ( i = 0; i < 3; i++ )
      Pick->xpk[i] = 0.;    /* Absolute value of extrema after ipic */

   Pick->FirstMotion = '?'; /* First motion  ?=Not determined  U=Up  D=Down */
                            /* u=Questionably up  d=Questionably down */
   Pick->weight = 0;        /* Pick weight (0-3) */
   Pick->status = 0;

/* Coda variables
   **************/
   for ( i = 0; i < 6; i++ )
      Coda->aav[i] = 0;     /* Average absolute value of preferred windows */

   Coda->len_sec = 0;       /* Coda length in seconds */
   Coda->len_out = 0;       /* Coda length in seconds (possibly * -1) */
   Coda->len_win = 0;       /* Coda length in number of windows */
   Coda->status  = 0;

/* Proxies variables
   **************/
   Proxies->tau0 = 0;       /* Window of time to calculate the proxies, in seconds */
   Proxies->tauc = 0;       /* Main period of the signal, in seconds */
   Proxies->pd = 0;         /* Maximum from displacement, in [cm] */
   Proxies->status  = 0;

}
