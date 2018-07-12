/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: irige.c 1185 2003-02-04 00:15:18Z alex $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2003/02/04 00:15:18  alex
 *     fixed search tag 'factor'
 *
 *     Revision 1.2  2002/07/17 23:24:16  alex
 *     tweaks to run up to 500 sps. Alex
 *
 *     Revision 1.1  2000/02/14 16:00:43  lucky
 *     Initial revision
 *
 *
 */

/*
 * irige.c : Real-time irige time code decoder.
 * Original code written by Carl Johnson  
 */

/* ?/97 LDD: changed to skip the SCAN_FREQ case since it uses the (bad)
   assumption that it's being handed one second's worth of data at a time.
/* 11/4/94 lynn: added SCAN_FREQ case and new return value TIME_WAIT.
   Make sure that the signal consists of 10 pulses per buffer (assumes
   that each buffer is one second of data) before synching. */
/* 7/22/94 lynn: added the "fabs" to res=fabs(tcalc-tmark) in the
   LOCK_CHECK case of irige_code to make it notice large negative
   residuals */
/* 4/6/94 lynn: change nCode from int to unsigned long. If it reaches
   its limit and wraps, time will be unsynched for < 1 minute */
/* 11/26/93 alex: Inserted Year as calling parameter */
/* 11/11/93 alex: moved definition of error constants to irige.h */
/* 11/5/93  Defined error return values; added lots of comments!   LDD */
/* 11/2/93  changed to notice problems in time code signal; return error
   value accordingly (1 if everything's ok, -1 for no synch,
   -2 for no signal, -3 for some wiggly signal other than irige) LDD */
/* 10/28/93 made irige return an integer "error" value based on whether
   it was synched (1) or not (-1)   LDD */


#include <math.h>
#include "irige.h"
#include "chron.h"
#define Q(x)            Code[(nCode+(x))%100]
#define SCAN_INIT       0
#define SCAN_BIAS       1
#define SCAN_FREQ       2
#define SCAN_SYNC       3
#define SCAN_UP         4
#define SCAN_DOWN       5
#define SCAN_HIGH       6
#define SCAN_LOW        7
#define TIME_NONE       0
#define TIME_FREE       1
#define TIME_LOCK       2
#define LOCK_FREE       0
#define LOCK_FIRST      1
#define LOCK_CHECK      2
#define COUNT           500
#define MAXSYN          100
#define SYNCUT          20

/* Synchronization parameters */
static int Init = 1;
static short State = SCAN_INIT;
static long Check;              /* Time check, next expected scan       */
static long Count;              /* Count down for bias calculation      */
static long Bias;               /* Signal bias                          */
static short Npulse;            /* Count the # of pulses in one buffer  */
static long Last;               /* Last sample of previous buffer       */
static double Sum;              /* Summation accumulator for bias       */
static long Up;                 /* Scan of last rising edge             */
static long Down;               /* Scan of last falling edge            */
static short Zero;              /* Count length of 0 code (20ms)        */
static short One;               /* Count length of 1 code (50ms)        */
static short Mark;              /* Count length of position mark (80ms) */
static short Syn[MAXSYN];       /* Synchronization buffer               */
static char cDate[40];          /* date in character string             */
static double resMax;           /* Largest residual encountered         */
static double resTol = 1.0;     /* If resMax>resTol, time is unsynched  */
static unsigned long nCode;     /* Length of time code buffer           */

/* Time code translation paramters */
static short Lock = LOCK_FREE;
static long Sold;               /* Scan at last decode                  */
static double Told;             /* Time at last decode                  */
static double Tau = 10.0;       /* Relaxation time constant             */
static double aFac,
              bFac;             /* xnew = aFac*xnew + bFac*xold         */
static int Code[100];           /* Time code buffer                     */

void irige_code(struct TIME_BUFF *, long, long, int);

/* This block of error detection/return code added by Lynn Dietz 10/93  */
static int eSync;               /* error value returned by irige        */
static int NoTime;              /* Test length for no time code (500ms) */
static int Flat;                /* # Samples since last pulse edge      */

/*     eSync =  1  TIME_OK when there are no problems decoding time      *
 *           =  0  TIME_WAIT before irige gets its first synch with code *
 *           = -1  TIME_NOSYNC when irige loses synch; finds double-     *
 *                 Mark, but has trouble decoding time; glitchey stretch *
 *                 of otherwise ok time code or low freq (<5 hz?) noise  *
 *           = -2  TIME_FLAT when time code signal goes flat; count      *
 *                 since last pulse edge exceeds test value, NoTime      *
 *           = -3  TIME_NOISE when time code has too few or too many     *
 *                 Marks (wide pulses);  signal is from some other       *
 *                 wiggly source                                         *
 *     For all buffers with negative eSync values, the time returned by  *
 *     irige is calculated from scan # and sampling rate at the last     *
 *     successful decode.  It will eventually start to drift away from   *
 *     the actual time.                                                  */

/***************************************************************************
   function irige_init initializes some algorithm control variables
 **************************************************************************/
void irige_init(struct TIME_BUFF *tbuf)
{
   State  = SCAN_BIAS;
   Count  = COUNT;
   Sum    = 0.0;
   Npulse = 0;
   Init   = 0;
   nCode  = 0;
   Lock   = LOCK_FREE;
   bFac   = pow(10.0, 10.0 * log10(0.5) / Tau);
   aFac   = (1.0 - bFac);
   eSync  = TIME_WAIT;       /* LDD */
}

/***************************************************************************
   function Irige decodes the time code; it requires at least 23 seconds of
    code to set the necessary parameters and then get a lock on time
 **************************************************************************/

int Irige(struct  TIME_BUFF *tbuf, // structure storing time-decoding info
          long    scan,            // cum scan count of first sample in s
          int     nchan,           // number of channels in s
          int     nscan,           // number of samples/channel in s
          short  *s,               // array containing muxed? digitized data
          int     tchan,           // Which channel in s contains time code
          int     yearIn)          // the year, supplied by our caller
{
   double    b;
   int       i;
   int       j;
   int       samp;
   int       delta;
   long      mark;

// First time around, initialize control variables
   if (Init)
      irige_init(tbuf);

// If the scan number of this buffer isn't what we expected, start over
   if (scan != Check)
   {
      //printf("IRIGE : Break : Time code rest at scan %d. (%d)\n",
      //      scan, Check);
      irige_init(tbuf);
   }

//------------Start Main Loop; do for each sample in this buffer---------------

   for (i = 0; i < nscan; i++)
   {
      samp = s[i * nchan + tchan];
      delta = samp - Last;
      switch (State)
      {

// Calculate bias (average) from first COUNT samples
      case SCAN_BIAS:
         if (Count > 0)
         {
            Count--;
            Sum += samp;
         }
         else
         {
            Bias = (long)(Sum / COUNT);
            Count = nscan;
            State = SCAN_SYNC;  /*skip SCAN_FREQ due to bad assumptions!! */
         }
         break;

// Make sure the signal is of the correct frequency (assumes each buffer
// holds one second of time code, therefore it expects to see 10 pulses):
      case SCAN_FREQ:
         if (Count > 0)
         {
            Count--;
            if (Last < Bias && samp > Bias)
               Npulse++;
         }
         else
         {
            if (Npulse != 10)
            {
               irige_init(tbuf);
               break;
            }
            State = SCAN_SYNC;
         }
         break;

//-------------------------------STORY-----------------------------------------
// The next 3 cases deal with setting parameters needed to decode IRIGE.
// The time code should consist of 10 pulses every second; the front edges of
// these pulses are evenly spaced in time (0.1 sec), but the widths can
// be one of 3 values: 0.02 sec for a binary Zero, 0.05 sec for a binary One,
// and 0.08 sec for a Mark (position identifier). Ten seconds (100 pulses)
// of data are needed to read the seconds, minute, hour and day. IRIGE
// doesn't tell you what year it is.
//
// The program now finds the # of samples (j) between pulse "front/rising edges"
// and updates a counter, Syn[j].  When the counter for any value of j reaches
// SYNCUT, the program calculates various parameters, such as digitization
// rate (sec/sample), pulse widths, etc., needed to decode time.
// ----------------------------------------------------------------------------

// First find the "rising edge" of the first pulse; initialize counter array
      case SCAN_SYNC:
		  if (Last < Bias && samp > Bias)
         {
            for (j = 0; j < MAXSYN; j++)
               Syn[j] = 0;
            State = SCAN_UP;
            Up = scan + i;
            break;
         }
         break;

// Now look for the "back/falling edge" of the pulse
      case SCAN_UP:
 		 if (samp >= Bias)
            break;
         State = SCAN_DOWN;
         Down = scan + i;
         break;

// Look for the next rising edge; update counter
      case SCAN_DOWN:
         if (samp < Bias)
            break;
         j = scan + i - Up;     //find # samples since previous rising edge

         if (j > 0 && j < MAXSYN)
         {
            Syn[j]++;           //update counter

// If we've collected enough data; calc the digitization rate, b
            if (Syn[j] >= SYNCUT)
            {

               b = 0.1
                  * (Syn[j - 1] + Syn[j] + Syn[j + 1])
                  / ((j - 1) * Syn[j - 1] + (j) * Syn[j]
                     + (j + 1) * Syn[j + 1]);
               // Set count lengths (# samples) for pulse widths
               //Zero = (short)(250 * b);
               //One  = (short)(550 * b);
               //Mark = (short)(850 * b);
               //NoTime = (int)(5000 * b);       /*LDD */
			   Zero = (short)(.02 / b);
               One  = (short)(.05 / b);
               Mark = (short)(.08 / b);
               NoTime = (int)(.5000 / b);       /*AB: factor fix*/
               State = SCAN_HIGH;
               Flat = 0;        /*LDD */
               Up = scan + i;
               break;
            }
         }
         State = SCAN_UP;
         Up = scan + i;
         break;

//----------------------------------------------------------------------------
// Once all the parameters are set, irige stays in the next 2 cases where it
// finds pulse edges, calculates widths, and calls irige_code to decode time.
// It also checks to make sure it's still getting a signal.   
//----------------------------------------------------------------------------

// Look for next falling edge
      case SCAN_HIGH:
         if (samp >= Bias)
         {
            if (++Flat >= NoTime)
               eSync = TIME_FLAT;       /*LDD */
            break;
         }
         State = SCAN_LOW;
         Flat = 0;              /*LDD */
         Down = scan + i;
         break;

// Look for next rising edge
      case SCAN_LOW:
         if (samp < Bias)
         {
            if (++Flat >= NoTime)
               eSync = TIME_FLAT;       /*LDD */
            break;
         }
         j = Down - Up;
         mark = 2;                 // set mark based on pulse width.
         if (j < Mark) mark = 1;   // NOTE: no test for wider-
         if (j < One ) mark = 0;   //       than-expected pulses 

         irige_code(tbuf, Up, mark, yearIn); //update proxy; try to decode

         State = SCAN_HIGH;
         Flat = 0;              /*LDD */
         Up = scan + i;
         break;

      }
      Last = samp;
   }
//-------------------------------End Main Loop---------------------------------
// Looked at all samples in this buffer; reset scan # and time of first sample;
   tbuf->scan = scan;
   tbuf->t = tbuf->a + tbuf->b * scan;
   Check = scan + nscan;

// Go back to calling function, returning error value as you go;
   return (eSync);              /* LDD 10/28/93 */
}

/*****************************************************************************
 function irige_code builds and analyzes a time-code proxy, Code.
 Code is an array of 100 integers representing the 100 pulses required to
 read IRIGE.  The values of Code (0, 1 or 2=Mark) indicate the pulse width.
 Code is overwritten every 100 pulses, which should be every 10 seconds if
 it's actually working on IRIGE.
*****************************************************************************/
void irige_code(struct TIME_BUFF *tbuf,
                long   scan,
                long   mark,
                int    year)
{
   long      julmin();
   struct Greg g;
   double    a,
             b;
   double    tmark;      // time at double-Mark read from Code
   double    tcalc;      // time at double-Mark calculated from previous  
                         //    dig.rate, etc
   double    res;
   int       kday;
   int       khour;
   int       kmin;
   int       ksec;
   int       nmark;
   int       i;

   Code[nCode % 100] = mark;         // update Code with current pulse width

   if (nCode < 100)                  // if Code isn't totally initialized,
      goto pau;                      // go get next pulse

   if (nCode % 100 == 0)
   {                                 // test added by LDD to count the
      nmark = 0;                     // # of Marks (wide pulses) in the
      for (i = 0; i < 100; ++i)
      {                              // most recent version of Code
         if (Code[i] == 2)
            ++nmark;                 // (clean IRIGE should have 11)
      }
      if (nmark < 9 || nmark > 13)   // if there are too few or
         eSync = TIME_NOISE;         // way too many, set eSync.
   }

   if (mark != 2)                    // if this pulse isn't a Mark,
      goto pau;                      // get next one

   if (Code[(nCode - 1) % 100] != 2) // if the previous pulse wasn't a Mark,
      goto pau;                      // get next one

// Here at second position Mark, check for proper placement
// of other Marks before trying to decode time      LDD
   for (i = 9; i < 100; i = i + 10)
   {
      if (Q(i) != 2)
      {
         eSync = TIME_NOSYNC;
         Lock = LOCK_FREE;
         goto pau;
      }
   }

// Check for proper number of Marks before decoding    LDD
   nmark = 0;
   for (i = 0; i < 100; ++i)
   {
      if (Code[i] == 2)
         ++nmark;
   }
   if (nmark != 11)
   {
      eSync = TIME_NOSYNC;
      Lock = LOCK_FREE;
      goto pau;
   }

// Code looks ok; here at second position mark, reset reference time
   ksec  = 10 * Q(6) + 20 * Q(7) + 40 * Q(8);
   kmin  = Q(10) + 2 * Q(11) + 4 * Q(12) + 8 * Q(13)
           + 10 * Q(15) + 20 * Q(16) + 40 * Q(17);
   khour = Q(20) + 2 * Q(21) + 4 * Q(22) + 8 * Q(23)
           + 10 * Q(25) + 20 * Q(26);
   kday  = Q(30) + 2 * Q(31) + 4 * Q(32) + 8 * Q(33)
           + 10 * Q(35) + 20 * Q(36) + 40 * Q(37)
           + 80 * Q(38) + 100 * Q(40) + 200 * Q(41);
   g.year = year;          //was hard-wired. Now from calling sequence

   g.month = 1;
   g.day = 1;
   g.hour = khour;
   g.minute = kmin;
   tmark = 60.0 * julmin(&g) + 86400.0 * (kday - 1) + ksec + 10.0;
   tcalc = tbuf->a + tbuf->b * scan;
   //date18(tmark, cDate);

//----------------------------------------------------------------------------
// tmark is the time (seconds since 0000 January 1, 1600) at the first sample 
//   of the second consecutive Mark.  Because we decoded time from the 10 secs
//   of data before the double-Mark, not after, we need to correct the decoded 
//   time by adding 10.0 sec.
// tcalc is the time calculated from previously determined "fudge factor" (a),
//   digitization rate (b), and current scan #.
//----------------------------------------------------------------------------

//-----------------------start switch--------------------------------------
// Based on if we're synched or not, follow one of the 3 cases:
   switch (Lock)
   {

// Previously un-synched. Reset values after first successful decode
   case LOCK_FREE:
      //printf("FREE : %.24s\n", cDate);
      Sold = scan;         // store scan # of this decode
      Told = tmark;        // store time of this decode
      Lock = LOCK_FIRST;
      break;

// Second successful decode. Find dig. rate (b) and "fudge factor" (a)
   case LOCK_FIRST:
      //printf("FIRST: %.24s\n", cDate);
      b = (tmark - Told) / (scan - Sold);  // dig. rate
      a = tmark - b * scan;                // fudge factor

      tbuf->a = a;
      tbuf->b = b;
      Sold    = scan;            // store scan and time of this decode
      Told    = tmark;
      resMax  = 0.0;             // max residual in current locked interval
      Lock    = LOCK_CHECK;
      eSync   = TIME_OK;         /* LDD 10/28/93 */
      break;

// Third and subsequent decodes since synchronization.  Verify synch.
   case LOCK_CHECK:
      b = (tmark - Told) / (scan - Sold);    // calc a & b from last
      a = tmark - b * scan;                  // two decodes

      res = fabs(tcalc - tmark);
      if (res > resMax) resMax = res;
      //printf("CHECK: %.24s (%.2f) %.2f\n", cDate, res, resMax);
      // if the residual is greater than the allowed amount; synch is broken.
      if (res > resTol)
      {
         //printf("*****  Tolerance exceeded, resetting\n");
         Lock = LOCK_FREE;
         eSync = TIME_NOSYNC;   /* LDD 10/28/93 */
         break;
      }
      // otherwise, we're still synched; update all parameters
      tbuf->tdecode = tcalc;
      tbuf->a = 0.9 * tbuf->a + 0.1 * a;
      tbuf->b = 0.9 * tbuf->b + 0.1 * b;
      Sold = scan;
      Told = tmark;
      break;

   }
//----------------------end switch------------------------------------------

// update pulse counter; go back to calling function
 pau:
   /* if(!(nCode%100))
      //printf("\n");
      //printf("%d", mark); */
   nCode++;
}
