/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: symmetry.c 4453 2011-07-27 21:10:40Z stefan $
 *
 *    Revision history:
 *     $Log: symmetry.c,v $
 *     Revision 1.1  2008/05/29 18:46:46  mark
 *     Initial checkin (move from \resp; also changes for station list auto-update)
 *
 *     Revision 1.3  2006/07/13 15:14:39  cjbryan
 *     temporarily disable symmetry check by setting all values in g_Symmetry array to symmetric
 *
 *     Revision 1.2  2006/06/28 14:24:53  labcvs
 *     test of symmetryCheck; require return code of SYMM_SYMMETRIC
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:50  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.4  2005/06/20 21:37:24  cjbryan
 *     cleanup
 *
 *     Revision 1.3  2005/03/29 23:58:57  cjbryan
 *     revised to use macros.h SIGN
 *
 *     Revision 1.2  2004/11/01 02:03:27  cjbryan
 *     removed all amplitude determination code from the picker and changed all error
 *     codes to positive numbers
 *     CVS ----------------------------------------------------------------------
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:27  michelle
 *     New Hydra Import
 *
 *
 *
 */
/*
 * Implementation of functions used to track symmetry status.
 * 
 * see symmetry.h for comments.
 * 
 * @author Ray Buland, original FORTRAN
 * @author Dale Hanych, Genesis Business Group (dbh)
 * @version 1.0 August 2003, dbh
 */

/* system includes */
#include <math.h>                    /*  fabs(), floor()  */

/* earthworm/hydra includes */
#include <earthworm.h>
#include <macros.h>                  /*  MAX(), MIN(), SIGN()  */

/* raypicker includes */
#include "symmetry.h"
#include "ray_trigger.h"             /* CYCLN  */
#include "returncodes.h"
#include "raypicker.h"

#define STOL 1.5

/* #define DEBUG_LOG_SYMMETRY  */ /* define this to write detailed symmetry debug tracking to log */

/************************************************************************
 *                      InitSymmetryData()                              *
 *                                                                      *
 * Initializes a SYMMETRY_CHECK_DATA struct to prepare for the start of *
 * a time series.                                                       *
 *                                                                      *
 * @return EW_SUCCESS                                                   *
 *                                                                      *
 * firstRaw = rawdata[0]                                                *
 * rawmean = inital raw mean                                            *
 * rectmean = initial rectified raw mean                                *
 ************************************************************************/
int InitSymmetryData(const double firstRaw, const double rawmean,
                     const double rectmean, const double snmin, 
                     const double sampleRate, SYMMETRY_CHECK_DATA *symData)
{
    int i;

    symData->ncycln      = (long)floor(sampleRate * CYCLN + 0.5);
   
    symData->icnt        = 0;

    symData->rawmean     = rawmean;
   
    symData->last        = firstRaw - rawmean;   /* alst=rng(nptr).nbuf(1)-a1 */
   
    symData->cmx         = SIGN(snmin, symData->last);
   
    /*
     * dbh: As well as I can tell multiplying by this fixed value
     *      is required to prevent one or the other of the polarity
     *      tracking values -- omx or omn -- from 
     */
    symData->omx         = rectmean * (double)SYMMETRY_CYCLES;
   
    symData->omn         = symData->omx;
   
    symData->omxi        = (1.0 / MAX(symData->omx , snmin));
   
    symData->omni        = symData->omxi;
   
    symData->ir3         = SYMM_SYMMETRIC;
   
    symData->lr3         = SYMM_SYMMETRIC;

    symData->nasmM       = 0;
    symData->nasmP       = 0;
   
    for (i = 0; i < SYMMETRY_CYCLES; i++)
    {
      symData->symmetryM[i] = -rectmean;
      symData->symmetryP[i] =  rectmean;
    }
   
#ifdef DEBUG_LOG_SYMMETRY
    logit("", "InitSymmetryData(): ncyc %d  rwmn %f  remn %f  lst %f  cmx %f  omx/omn %f  omxi/omni %f  M %f  P %f\n", 
	
            symData->ncycln, symData->rawmean, rectmean, symData->last, symData->cmx, symData->omx, 
            symData->omxi, symData->symmetryM[0], symData->symmetryP[0]);
#endif

    return EW_SUCCESS;
}

/************************************************************************
 *                       SymmetryCheck()                                *
 *                                                                      *
 * Checks the next data point for symmetry                              *
 *                                                                      *
 * Have to pass in the entire symmetry array, because an entire span    *
 * can be set to asymmetric under some circumstances.                   *
 *                                                                      *                               *
 * @return SYMM_SYMMETRIC                                               *  
 *         SYMM_PARTSYMMETRIC                                           *
 *         SYMM_ASYMMETRIC                                              *
 *                                                                      *
 * c2 - current point diff from simulated mean,  c2(kl)                 *
 * btn - filtered value,   btn(kl)                                      *
 * xmt - xmt1, xmt (mid-level) from highest freq filter                 *
 * snmin - min signal to noise                                          *
 * kr3Index - current point in kr3[]                                    *
 *                                                                      *
 * Run filter 1: FORTRAN do 9 kl=1,nodat                                *
 * Separate the positive and negative signals and average them          *
 *                                                                      *
 ************************************************************************/
int SymmetryCheck(const double c2, const double btn, 
                  const double xmt, const double snmin, 
                  const long kr3Index, SYMMETRY_TRACK_TYPE *kr3, 
                  SYMMETRY_CHECK_DATA *symData)
{
    int     j;
    double  testv;


    /* if(c2(kl)*last.gt.0.) go to 10 
     *  if(c2(kl).eq.0.) go to 10 */

    /* Current adjusted point is zero or of the same polarity as the last point 
     * i.e., not a zero crossing  */
    if ((c2 * symData->last) > 0.0 || c2 == 0)
    {
      /* FORTAN LINE 10 */
      
      symData->icnt++;
      
      /* Number of points checked has met or exceeded 
       * the number in the symmetry check span.
       * if(icnt.lt.ncycln) go to 14 */
      if (symData->icnt == symData->ncycln)
      { 
		  
          /* capture symmetric state in last symmetric state */
          symData->lr3 = symData->ir3;
         
          /* mark current point as asymmetric
           * (i.e., no polarity change within the span)
           */
          symData->ir3 = SYMM_ASYMMETRIC;
         
          /* End of cycle, prepare to count length of next cycle */
          symData->icnt = 0;      
         
          /* Mark the entire preceeding span as asymmetric
           * (because no polarity change was detected within the span) */
          if (symData->lr3 == SYMM_SYMMETRIC) /* if(lr3.gt.1) go to 14 */
          {            
              for (j = 1; j < symData->ncycln; j++) /* do 120 j=2,ncycln */
              {
                  /* Beginning of array (which does not wrap), leave loop */
                  if (kr3Index < j) /* go to 14 */
                      break;

                  /* FORTRAN LINE 120 */
                  kr3[kr3Index - j] = SYMM_ASYMMETRIC;
              }
          }
      }
      
      /* FORTRAN LINE 14 */
      if (symData->cmx > 0.0) /* if(cmx.gt.0.) go to 84 */
      {
          /* trace running above zero */
          /* FORTRAN LINE 84 */
          symData->cmx = MAX(symData->cmx, c2); /* cmx = amax1(cmx, c2(kl) */
          testv = symData->cmx * symData->omni;
      }
      else
      {
          /* trace running below zero */
          symData->cmx = MIN(symData->cmx, c2); /* cmx = amin1(cmx, c2(kl) */
          testv = -1.0 * symData->cmx * symData->omxi;
      }
               
      /* symmetry state persists and filtered value is above threshold */
      if ((symData->ir3 == symData->lr3) && (STOL < testv) && (xmt < btn))
      {
          /* Increment current state up to SYMM_ASYMMETRIC */

#ifdef DEBUG_LOG_SYMMETRY         
          logit("", "symmetry ('m%s') ++  cmx: %9.4f  omn: %8.4f  omx: %9.4f  omni %7.4f  omxi %7.4f  xmt: %.5f btn %.4f test %f\n", 
			  (0.0 < symData->cmx ? "ax" : "in"), symData->cmx, symData->omn, symData->omx, 
					symData->omni, symData->omxi, xmt, btn, testv);
#endif

          /* ir3 = min0(ir3+1,3) */
          symData->ir3++;
         
          if (symData->ir3 == SYMM_ASYMMETRIC)
          {
#ifdef DEBUG_LOG_SYMMETRY         
              logit("", "State has gone asymmetric\n");
#endif
          }
          else if (SYMM_ASYMMETRIC < symData->ir3)
          {
#ifdef DEBUG_LOG_SYMMETRY         
              logit("", "State remains asymmetric\n");
#endif
              symData->ir3 = SYMM_ASYMMETRIC;
          }
      }  /* go to 83 */
    } /* if c2 == 0 || c2 * alst > 0) */

    /* Current adjusted point has changed polarity relative to the last point,
     * i.e., found a zero crossing */
    else
    { 
      if (symData->cmx > 0.0) /* if(cmx.gt.0.) go to 85 */
      {
          /* FORTRAN LINE 85 */
 
          /* Increment symmetric positive cycle counter 
           * This is an array index, must be normalized to zero-based */
          if ((++symData->nasmP) == SYMMETRY_CYCLES)
              symData->nasmP = 0;
         
#ifdef DEBUG_LOG_SYMMETRY         
          logit("", "omx %f  -=  asmp[%d] = %f  -  %f\n", symData->omx, 
                      symData->nasmP, symData->symmetryP[symData->nasmP], symData->cmx);
#endif
                
          symData->omx -= (symData->symmetryP[symData->nasmP] - symData->cmx); 
		  
          symData->symmetryP[symData->nasmP] = symData->cmx;
         
          symData->omxi = 1.0 / MAX(symData->omx , snmin);
         
               
#ifdef DEBUG_LOG_SYMMETRY         
          /* print *,'omx',kl,ir3,cmx,omn,omx,xmt1,cmx*omni/stol */
          logit("", "symmetry omx ir3: %d  cmx: %9.4f  omn: %8.4f  omx: %9.4f  omni %7.4f  omxi %7.4f  xmt: %.5f  cmx*omni/stol: %f\n", 
                     symData->ir3, symData->cmx, symData->omn, symData->omx, symData->omni, 
                     symData->omxi, xmt,  symData->cmx * symData->omni / STOL);
#endif
        
          if (STOL < (symData->omx * symData->omni)   
                && xmt < btn /* and filtered value remains above threshold */)
          {
              /* FORTRAN LINE 11 */
#ifdef DEBUG_LOG_SYMMETRY
              logit("", "Incrementing symmetry state (omx) [%f]\n", symData->omx * symData->omni);
#endif

              /* This cycle symmetry state same as last */
              if(symData->ir3 == symData->lr3)
              {
               
                  /* increment current symmetry state (up to asymmetric) */
                  /* ir3=min0(ir3+1,3) */
                  symData->ir3++;
                  if (symData->ir3 == SYMM_ASYMMETRIC)
                  {
#ifdef DEBUG_LOG_SYMMETRY         
                       logit("", "State has gone asymmetric\n");
#endif
                  }
                  else if (SYMM_ASYMMETRIC < symData->ir3)
                  {
#ifdef DEBUG_LOG_SYMMETRY         
                      logit("", "State remains asymmetric\n");
#endif
                      symData->ir3 = SYMM_ASYMMETRIC;
                  }
              }
          }
          else
          {
              if (symData->cmx / MAX( -1.0 * symData->symmetryM[symData->nasmM], snmin) <= STOL)
                  symData->ir3 = SYMM_SYMMETRIC;
          } /* go to 12 */ 
      } /* last adjusted value positive and polarity change occurred */
      else
      {
          /* Increment symmetric negative cycle counter 
           * This is an array index, must be normalized to zero-based */
         
          if ((++symData->nasmM) == SYMMETRY_CYCLES)
              symData->nasmM = 0;
         
#ifdef DEBUG_LOG_SYMMETRY         
          logit("", "omn %f  +=  asmn[%d] = %f  -  %f\n", symData->omn, 
                       symData->nasmM, symData->symmetryM[symData->nasmM], symData->cmx);
#endif
          
          symData->omn += symData->symmetryM[symData->nasmM] - symData->cmx;
         
          symData->symmetryM[symData->nasmM] = symData->cmx;
         
          symData->omni = 1.0 / MAX(symData->omn, snmin);
         
#ifdef DEBUG_LOG_SYMMETRY
          /* print *,'omn',kl,ir3,cmx,omn,omx,xmt1,-cmx*omxi/stol */
          logit("", "symmetry omn ir3: %d  cmx: %9.4f  omn: %8.4f  omx: %9.4f  omni %7.4f  omxi %7.4f  xmt: %.5f -cmx*omxi/stol: %f\n", 
                        symData->ir3, symData->cmx, symData->omn, symData->omx, symData->omni, 
                        symData->omxi, xmt, -symData->cmx * symData->omxi / STOL);
#endif

          if (STOL < (symData->omn * symData->omxi)
				&& xmt < btn  /* and filtered value remains above threshold */)
          {
              /* FORTRAN LINE 11 */
            
              /* This cycle symmetry state same as last */
              if (symData->ir3 == symData->lr3)
              {
                  /* increment current symmetry state (up to 3) */
#ifdef DEBUG_LOG_SYMMETRY
                  logit("", "Incrementing symmetry state (omn) [%f]\n", symData->omn * symData->omxi);
#endif
               
                  /* r3=min0(ir3+1,3) */
                  symData->ir3++;
                  if (symData->ir3 == SYMM_ASYMMETRIC)
                  {
#ifdef DEBUG_LOG_SYMMETRY
                      logit("", "State has gone asymmetric\n");
#endif
                  }
                  else if (SYMM_ASYMMETRIC < symData->ir3)
                  { 
#ifdef DEBUG_LOG_SYMMETRY
                      logit("", "State remains asymmetric\n");
#endif
                      symData->ir3 = SYMM_ASYMMETRIC;
                  }
              }
          }
          else
          {
              if ((-1.0 * symData->cmx) / MAX( symData->symmetryP[symData->nasmP], snmin) <= STOL)
                  symData->ir3 = SYMM_SYMMETRIC;
          } /* go to 12 */
      } /* last adjusted value negative and polarity change occurred */
      
      /* FORTRAN LINE 12 */
      
      /* capture current symmetry state as last */
      symData->lr3  = symData->ir3;
      
      /* Re-initialize symmetry cycle length counter */
      symData->icnt = 0;
      
      /* capture current adjusted value */
      symData->cmx  = c2;
      
    } /* polarity change detected */
   
    /* capture current adjusted point as last point (FORTRAN LINE 83)  */
    symData->last = c2;
   
    /* temp - for now always assume signal is symmetric */
    symData->ir3 =  SYMM_SYMMETRIC;
	
	/* FORTRAN LINE 9 */
    kr3[kr3Index] = symData->ir3;

   return symData->ir3;


}
   

