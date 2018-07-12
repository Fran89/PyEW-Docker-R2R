
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sample.c,v 1.1 2000/02/14 19:06:49 lucky Exp $
 *
 *    Revision history:
 *     $Log: sample.c,v $
 *
 *     Revision 1.2  2013/05/07  nromeu
 *     Computation for proxies: velocity and displacement by means of
 *     trapezoidal method 
 *
 *     Revision 1.1  2000/02/14 19:06:49  lucky
 *     Initial revision
 *
 *
 */

#include <math.h>
#include <earthworm.h>
#include <transport.h>
#include "pick_ew.h"


  /******************************************************************
   *                             Sample()                           *
   *                    Process one digital sample.                 *
   *                                                                *
   *  Arguments:                                                    *
   *    LongSample  One waveform data sample                        *
   *    Sta         Station list                                    *
   *    SampRate    Sample rate of the channel                      *
   *                                                                *
   *  The constant SmallDouble is used to avoid underflow in the    *
   *  calculation of rdat.                                          *
   *                                                                *
   *  Modifies: rold, rdat, old_sample, esta, elta, eref, eabs      *
   ******************************************************************/

void Sample( long LongSample, STATION *Sta, double SampRate )
{
   PARM *Parm = &Sta->Parm;
   static double rdif;                    /* First difference */
   static double edat;                    /* Characteristic function */
   const  double small_double = 1.0e-10;

/* Store present value of filtered data */
   Sta->rold = Sta->rdat;

/* Compute new value of filtered data */
   Sta->rdat = (Sta->rdat * Parm->RawDataFilt) +
               (double) (LongSample - Sta->old_sample) + small_double;

/* Compute 1'st difference of filtered data */
   rdif = Sta->rdat - Sta->rold;


/* Computation for proxies */ 
	if( strlen(Sta->chan)>=3 && SampRate > 0)
	{
		// filtered velocity
		Sta->old_vel	= Sta->vel;
		Sta->vel		= (Sta->vel * Parm->ProxiesFilt) + (LongSample - Sta->old_sample);

		// integration to compute displacement
		Sta->old_disp_offset = Sta->disp_offset;
		Sta->disp_offset     += (Sta->vel + Sta->old_vel) * 0.5 / SampRate;

		// filtered displacement to erase the offset from the integration
		Sta->old_disp	= Sta->disp;
		Sta->disp		= (Sta->disp * Parm->ProxiesFilt) + (Sta->disp_offset - Sta->old_disp_offset);

		// power of the displacement
	    Sta->pow_disp   =	(Parm->PowDispFilt * Sta->pow_disp) 
							+ (( 1.0 - Parm->PowDispFilt) * (Sta->disp)*(Sta->disp));
	}

/* Store integer data value */
   Sta->old_sample = LongSample;

/* Compute characteristic function */
   edat = (Sta->rdat * Sta->rdat) + (Parm->CharFuncFilt * rdif * rdif);

/* Compute esta, the short-term average of edat */
   Sta->esta += Parm->StaFilt * (edat - Sta->esta);

/* Compute elta, the long-term average of edat */
   Sta->elta += Parm->LtaFilt * (edat - Sta->elta);

/* Compute eref, the reference level for event checking */
   Sta->eref = Sta->elta * Parm->EventThresh;

/* Compute eabs, the running mean absolute value of rdat */
   Sta->eabs = (Parm->RmavFilt * Sta->eabs) +
                (( 1.0 - Parm->RmavFilt ) * fabs( Sta->rdat ));
}