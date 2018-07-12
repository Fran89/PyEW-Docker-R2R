/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: guidechk.c 2 2000-02-14 16:16:56Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 16:00:43  lucky
 *     Initial revision
 *
 *
 */

/***************************************************************************
   guidechk.c     a triangle wave detector

   This function assumes triangle waves as its input.  The function
   returns one of these "status" values in gStat:
     gStat = GUIDE_OK    Everything is in synch.
           = GUIDE_FLAT  no triangle wave; flat trace; dead mux
           = GUIDE_NOISY Signal does not look like a triangle wave.
/****************************************************************************/

#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <trace_buf.h>           /* Contains definition of MAX_TRACEBUF_SIZ */
#include "guidechk.h"

#define IABS(x) ((x)<0? -(x): (x))

/* External parameters
   *******************/
extern int TimeForLock;        // Guides are assumed locked on after this many secs

/* ThreePointMedian() is called by GuideChk
   ****************************************/
short ThreePointMedian( short x, short y, short z )
{
   if ( x <= y && x >= z ) return x;
   if ( x >= y && x <= z ) return x;
   if ( y <= x && y >= z ) return y;
   if ( y >= x && y <= z ) return y;
   return z;
}


/***************************************************************************
   function GuideChk is the triangle-wave detector

   Guide channel is declared dead if the mean first-difference is less
   than MinGuideSignal.  Guide channel is declared noisy if the standard
   deviation of first-difference values is greater than MaxGuideNoise.
 ***************************************************************************/

int GuideChk( short  *s,               /* Multiplexed data buffer */
              int    nchan,            /* Number of channels in buffer */
              int    nscan,            /* Number of samples per channel in buffer */
              int    gChan,            /* Channel number of guide channel */
              double *GuideSignal,     /* Mean first-difference */
              double *GuideNoise,      /* Standard deviation of first-difference */
              double MinGuideSignal,   /* Death threshold */
              double MaxGuideNoise )   /* Noise threshold */
{
   static short guideBuf[MAX_TRACEBUF_SIZ]; /* Work buffer */
   int    i;
   int    nFirstDif = nscan - 1;       /* Number of first-differences */
   int    nFiltered = nscan - 3;       /* Number of filtered first-differences */
   double x  = 0.0;                    /* Sum of samples */
   double xx = 0.0;                    /* Sum of squared samples */
   double signal;
   double noise;

/* Grab the guide channel samples and stuff them into guideBuf
   ***********************************************************/
   for ( i = 0; i < nscan; i++ )
      guideBuf[i] = s[i * nchan + gChan];

/* Calculate the absolute value of the 1'st differences
   between samples in the time series.
   ****************************************************/
   for ( i = 0; i < nFirstDif; i++ )
      guideBuf[i] = IABS( guideBuf[i+1] - guideBuf[i] );

/* Apply 3-point, running-median filter.  This will get rid of
   noise points that are created when the slope changes sign.
   Calculate the sum and sum of squares of these values.
   ***********************************************************/
   for ( i = 0; i < nFiltered; i++ )
   {
      short tpm = ThreePointMedian( guideBuf[i], guideBuf[i+1], guideBuf[i+2] );
      double dtpm;

/*    if ( gChan == 0 )
      {
      logit( "", " %3d", tpm );
      if ( i%15 == 14 ) logit( "", "\n" );
      }  */

      dtpm = (double)tpm;
      x  += dtpm;
      xx += (dtpm * dtpm);
   }
/* if ( gChan == 0 )
      logit( "", "\n" ); */

/* Calculate mean and standard deviation
   *************************************/
   signal = x / nFiltered;
   noise  = (xx - signal * signal * nFiltered) / (nFiltered - 1);
   noise  = (noise > 0.) ? sqrt(noise) : 0.0;

   *GuideSignal = signal;
   *GuideNoise  = noise;

   if ( signal < MinGuideSignal )
      return GUIDE_FLAT;

   if ( noise > MaxGuideNoise )
      return GUIDE_NOISY;

   return GUIDE_OK;
}



     /****************************************************************
      *                         CombGuideChk()                       *
      *                                                              *
      *  Returns BAD if any individual guide status is not ok        *
      *  Returns OK if all guide statuses are ok                     *
      *  Returns LOCKED_ON if all guide statuses have been ok for    *
      *     at least TimeForLock seconds.                            *
      ****************************************************************/

void CombGuideChk( int *GuideStat, int nguide, int *CombGuideStat )
{
   int           i,
                 gstat = OK;                 // Current status (OK or BAD)
   static int    gstatPrev = BAD;            // LOCKED_ON, OK or BAD
   static time_t then;
   time_t        now;

/* See if the current status is OK or BAD
   **************************************/
   for ( i = 0; i < nguide; i++ )
      if ( GuideStat[i] != GUIDE_OK )
         gstat = BAD;

/* The status is still BAD
   ***********************/
   if ( gstatPrev == BAD && gstat == BAD )
   {
      *CombGuideStat = BAD;
      return;
   }

/* The status went from BAD to OK.
   Record the current time.
   ******************************/
   if ( gstatPrev == BAD && gstat == OK )
   {
      time( &then );
      gstatPrev = OK;
      *CombGuideStat = OK;
      return;
   }

/* The status went from OK to BAD.
   ******************************/
   if ( gstatPrev == OK && gstat == BAD )
   {
      gstatPrev = BAD;
      *CombGuideStat = BAD;
      return;
   }

/* The status is still OK.  If the system has been OK
   for long enough, flag the system as locked on.
   **************************************************/
   if ( gstatPrev == OK && gstat == OK )
   {
      time( &now );
      if ( (now - then) < TimeForLock )
         *CombGuideStat = OK;
      else
      {
         gstatPrev = LOCKED_ON;
         *CombGuideStat = LOCKED_ON;
      }
      return;
   }

/* The guides are still locked on
   ******************************/
   if ( gstatPrev == LOCKED_ON && gstat == OK )
   {
      *CombGuideStat = LOCKED_ON;
      return;
   }

/* The status went from LOCKED_ON to BAD.
   A channel rotation may have occurred.
   Restart the DAQ system.
   *************************************/
   if ( gstatPrev == LOCKED_ON && gstat == BAD )
   {
      gstatPrev = BAD;
      *CombGuideStat = RESTART_DAQ;
      return;
   }
}
