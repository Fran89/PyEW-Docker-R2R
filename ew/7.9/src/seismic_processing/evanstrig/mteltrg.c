
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: mteltrg.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2004/05/21 22:32:23  dietz
 *     added location code; inputs TYPE_TRACEBUF2, outputs TYPE_LPTRIG_SCNL
 *
 *     Revision 1.1  2000/02/14 17:17:36  lucky
 *     Initial revision
 *
 *
 */

 /*************************************************************************
  *                            FILE: mteltrg.c                            *
  *                                                                       *
  *  These C functions implement the teleseismic detection algorithm      *
  *  developed by Evans and Allen (1983).  This algorithm can be used to  *
  *  detect any event containing "impulsive" energy at about 1 Hz but is  *
  *  intended primarily for finding weak teleseisms within time series    *
  *  containing local earthquakes and noise.  It is currently limited to  *
  *  input data at 100 sps but can be modified for other sample rates     *
  *  by appropriate selection of a decimation factor and FIR filter       *
  *  coefficients.                                                        *
  *************************************************************************/


 /*************************************************************************
  *                   FUNCTIONS INCLUDED IN THIS FILE                     *
  *************************************************************************/
/*
tel_energy_valid_on_channel ()  check for energy on given channel
tel_get_trigsetting ()          return pointer to trigger settings in use
tel_initialize_channel ()       initialize trigger variables for given channel
tel_initialize_params ()        initialize the trigger parameter defaults
tel_set_aset ()                 set aset
tel_set_bset ()                 set bset
tel_set_cmlta ()                set cmlta
tel_set_cmsta ()                set cmsta
tel_set_cmstav ()               set cmstav
tel_set_cset ()                 set cset
tel_set_ctlta ()                set ctlta
tel_set_ctsta ()                set ctsta
tel_set_ctstav ()               set ctstav
tel_set_dccoef ()               set dccoef
tel_set_eset ()                 set eset
tel_set_mminen ()               set mminen
tel_set_mndur1 ()               set mndur1
tel_set_mndur2 ()               set mndur2
tel_set_mth ()                  set mth
tel_set_sset ()                 set sset
tel_set_tminen ()               set tminen
tel_set_tth1 ()                 set tth1
tel_set_tth2 ()                 set tth2
tel_triggered_on_channel ()     look for a trigger on the given channel
                                  a multiplexed waveform message.
tel_triggered_on_tracebuf ()    look for a trigger in a single-channel 
                                  tracebuf message.       
 
EXTERNAL FUNCTIONS CALLED:
er_abort ()                     display an error message then quit
u_timestamp ()                  get timestamp
*/


 /************************************************************************
  *                           INCLUDE FILES                              *
  ************************************************************************/

#include <math.h>
#include <stdio.h>
#include <earthworm.h>             /* for logit() */
#include "mconst.h"
#include "mteltrg.h"
#include "mutils.h"


 /************************************************************************
  *                            GLOBAL DATA                               *
  ************************************************************************/

static TEL_TRIGSETTING trigsetting;
static TEL_TRIGSETTING *t = &trigsetting;

extern int Debug_enabled;      /* Set to 1 for extra printing */
extern double Drate;           /* Required input sample rate */
extern double Decim;           /* Decimation factor (whole number) */
extern double Ctlta;           /* seconds (1/e time) */
extern double Ctsta;           /* seconds (1/e time) */
extern double Ctstav;          /* seconds (1/e time) */
extern double Tth1;            /* db ((tsta-tstav)/(tlta-tminen)) */
extern double Tth2;            /* db ((tsta-tstav)/(tlta-tminen)) */
extern long   Tminen;          /* digitizer counts */
extern double Mndur1;          /* seconds */
extern double Mndur2;          /* seconds */
extern double Cmlta;           /* seconds (1/e time) */
extern double Cmsta;           /* seconds (1/e time) */
extern double Cmstav;          /* seconds (1/e time) */
extern double Mth;             /* db ((msta-mstav)/(mlta-mminen)) */
extern long   Mminen;          /* digitizer counts */
extern double Aset;            /* Microseism caution window - sec */
extern double Bset;            /* Pre-event quiet - sec */
extern double Cset;            /* Post-event + pre-event quiet - sec */
extern double Eset;            /* Microseism caution start delay - sec */
extern double Sset;            /* Settling period - sec */
extern double DCcoef;          /* 1/e time for DC (hipass) filter - sec */
extern int    Dflt_len;        /* Decimation filter length */
extern long   *Dflt;           /* Decimation filter coefficients */
extern int    Lflt_len;    
extern long   *Lflt;
extern double Quiet;           /* Used to avoid floating-pt underflow */


 /************************************************************************
  *                               MACROS                                 *
  ************************************************************************/

/* One-pole low-pass recursive filter */
#define ONE_POLE(old_val, new_pt, coeff) ((old_val) + ((coeff) * (((float) (new_pt)) - (old_val))) + (Quiet))

/* Threshold tester */
#define OVER(thresh, lta, sta, stav, min_en) (((sta) - (stav)) >= (((lta) + (min_en)) * (thresh)))

/* Absolute value generator for long integers */
#define LG_ABS(i)  ( ((i) < 0L) ? (-i) : (i) )


/*=======================================================================*
 *                              db_to_ratio()                            *
 *=======================================================================*/

static double db_to_ratio( double decibels )
{
   return pow(10.0, (decibels / 20.));
}


/*=======================================================================*
 *                               fir_filter()                            *
 *                                                                       *
 * Integer implementation of odd-length linear-phase                     *
 * finite-impulse response (FIR) filter.                                 *
 *                                                                       *
 * Requires an odd number of 32-bit-integer data in "buffer"             *
 * and an array of filter coefficients in "filter_coeff".                *
 * Since filter is linear phase, the coefficients are symmetric          *
 * and only the lower half (+1) of them are given.                       *
 *                                                                       *
 * Indexing the arrays is a bit complex since "buffer" is cyclic and     *
 * full length, but "filter_coeff" is non-cyclic and half length.        *
 *=======================================================================*/

static long fir_filter(
   int32_t *buffer,        /* input-data buffer (cyclic)           */
   int buf_len,            /* length of input-data buffer          */
   int buf_index,          /* index of oldest point in buffer      */
   long filter_coeff[] )   /* filter coefficients                  */
{
   int i, j, k;            /* dummy indicies                       */
   int flt_half_len;       /* half-width of buffer                 */
   int flt_half_index;     /* index of middle point of buffer      */
   long sum;               /* filter output (not right shifted)    */

   flt_half_len = (buf_len - 1) / 2;
   flt_half_index = (buf_index + 1 + flt_half_len) % buf_len;

   sum = filter_coeff[flt_half_len] * buffer[flt_half_index];
   for ( i = 0 ; i < flt_half_len ; i++ )
   {
      j = ((buf_index + 1) + i) % buf_len;
      k = (buf_index - i + buf_len) % buf_len;
      sum += filter_coeff[i] * (buffer[j] + buffer[k]);
   }
   return sum;
}


/*=======================================================================*
 *                             one_pole_coeff()                          *
 *=======================================================================*/

static double one_pole_coeff( double seconds )
{
   return (1.0 - exp(-(Decim / Drate) / seconds));
}


  /*===================================================================*
   *                       teleseismic_trigger()                       *
   *                                                                   *
   *            Guts of the teleseismic trigger algorithm              *
   *                                                                   *
   *  FIR filters are used to decimate the data and separate it into   *
   *  trigger and masking bands.  Both sets of coefficients are        *
   *  symmetric (they are linear-phase FIR's), so only half+1 are      *
   *  needed. These values have been multiplied by 2^15 so filtering   *
   *  can be done in integer for speed.                                *
   *===================================================================*/

static long teleseismic_trigger(
   TEL_CHANNEL *qch,
   long current_buffer,       /* current position in input buffer    */
   long index )               /* index of earliest trigger in buffer */
{
   double  var;            /* temporary double                       */
   long    dec_datum;      /* decimation-pass filter output          */
   long    lp_datum;       /* low-pass (trigger-band) filter output  */
   long    hp_datum;       /* high-pass (masking-band) filter output */
   int     i_dirac;        /* points at center of qch->lp_buf        */

/* Decimation filter
   *****************/
   dec_datum = fir_filter( qch->dec_buf, (int) Dflt_len, qch->i_dec_buf,
                           Dflt );

/* Compensate for scaling of filter coefficients
   *********************************************/
   qch->lp_buf[qch->i_lp_buf] = dec_datum >> 15;

/* Low-pass filter
   ***************/
   lp_datum = fir_filter( qch->lp_buf, (int) Lflt_len, qch->i_lp_buf, Lflt );

/* Compensate for scaling of filter coefficients
   *********************************************/
   lp_datum = lp_datum >> 15;

/* High-pass filter
   ****************/
   i_dirac = (qch->i_lp_buf + ((Lflt_len - 1) / 2) + 1) % Lflt_len;
   hp_datum = qch->lp_buf[i_dirac] - lp_datum;

/* Remove "DC" fraction of trigger band
   ************************************/
   qch->info.dc = (float) ONE_POLE( qch->info.dc, lp_datum, t->dccoef );
   lp_datum = (long)((double)lp_datum - (double)qch->info.dc);

/* Estimate power in each band
   ***************************/
   lp_datum = LG_ABS( lp_datum );
   hp_datum = LG_ABS( hp_datum );

/* Update LTA, STA, and STAV
   *************************/
   qch->info.tlta = (float) ONE_POLE( qch->info.tlta, lp_datum, t->ctlta );
   qch->info.tsta = (float) ONE_POLE( qch->info.tsta, lp_datum, t->ctsta );
   var = fabs ((double)(qch->info.tsta - qch->info.tlta));
   qch->info.tstav = (float) ONE_POLE( qch->info.tstav, var, t->ctstav );

   qch->info.mlta = (float) ONE_POLE( qch->info.mlta, hp_datum, t->cmlta );
   qch->info.msta = (float) ONE_POLE( qch->info.msta, hp_datum, t->cmsta );
   var = fabs( (double)(qch->info.msta - qch->info.mlta) );
   qch->info.mstav = (float) ONE_POLE( qch->info.mstav, var, t->cmstav );

/* Continue if sclk has expired (allowing averages to settle)
   **********************************************************/
   if ( qch->info.sclk > 0L )
      --qch->info.sclk;
   else
   {

/* LOW-PASS Logic :: Above low threshold (tth1)?
   *********************************************/
      if ( OVER( t->tth1, qch->info.tlta, qch->info.tsta,
                 qch->info.tstav, t->tminen ) )
      {
         qch->info.durflg = TRUE;

         if ( qch->info.first && ! (qch->info.bclk > 0L &&
              qch->info.dur >= t->mndur1) )
         {

            qch->info.first = FALSE;
            qch->info.astat = (qch->info.aclk > 0L) ? TRUE : FALSE;

/* Start post-event quiet and duration meter
   *****************************************/
            qch->info.bclk = t->bset;
            qch->info.dur = 0L;

/* Delay effect of microseism (A) clock
   ************************************/
            if ( qch->info.eclk > 0L )
               qch->info.eclk = t->eset;

            if ( Debug_enabled )
            {
               logit( "et", "Lower threshold crossed:  " );
               logit( "et", "Pin #%d, ", qch->pin );
               logit( "et", "A clock = %.2f seconds\n", qch->info.aclk / Drate );
            }
         }

/* Above upper threshold (tth2)?
   *****************************/
         if ( OVER( t->tth2, qch->info.tlta, qch->info.tsta,
                    qch->info.tstav, t->tminen ) )
         {

/* Fails for rapid multiple crossings?
   ***********************************/
            if ( Debug_enabled && (!qch->info.bigev) )
            {
               logit( "et", "Upper threshold crossed:  " );
               logit( "et", "Pin #%d\n", qch->pin );
            }
            qch->info.bigev = TRUE;
         }
      }
      else
      {
/* Below lower threshold (tth1)
   ****************************/
         if (Debug_enabled && (qch->info.durflg))
         {
            logit( "et", "LF duration on pin #%d was ", qch->pin );
            logit( "et", "%.2f seconds.\n", (qch->info.dur / (Drate / Decim)) );
         }
         qch->info.first = TRUE;
         qch->info.durflg = FALSE;
         qch->info.bigev = FALSE;
      }

/* HIGH-PASS Masking Logic
   ***********************/
      if ( OVER( t->mth, qch->info.mlta, qch->info.msta, qch->info.mstav,
                 t->mminen ) )
      {

         if ( Debug_enabled && (qch->info.cclk == 0L) )
            logit( "et", "HF trigger starts:  Pin #%d\n", qch->pin );
         qch->info.cclk = t->cset;
      }

/* Clock logic
   ***********/
      if ( qch->info.durflg )
         qch->info.dur++;

      if ( qch->info.aclk > 0L )
         qch->info.aclk--;

      if ( qch->info.cclk > 0L )
      {
         qch->info.cclk--;

         if ( Debug_enabled && (qch->info.cclk == 0L) )
            logit( "et", "HF trigger ends:  Pin #%d\n", qch->pin );
      }

      if (qch->info.bclk > 0L)
      {
         qch->info.bclk--;

/* Normal-event trigger
   ********************/
         if ( qch->info.bclk == 0L && qch->info.dur >= t->mndur1 &&
              !qch->info.astat     && qch->info.cclk == 0L )
         {
            if ( index == -1 )
            {
               index = current_buffer;
               qch->info.event_type = 'N';
            }

            if ( Debug_enabled )
               logit( "et", "==>SMALL EVENT TRIGGER:  Pin #%d\n", qch->pin );
         }
      }

/* Big-event trigger
   *****************/
      if (qch->info.bigev && qch->info.dur >= t->mndur2)
      {
         qch->info.bigev = FALSE;
         if (index == -1)
            index = current_buffer;
         qch->info.event_type = 'B'; /* has precedence over 'N'*/

         /* Kill off any pending normal trigger and reduce
            short-cycling on big-event trigger */
         qch->info.dur = -(qch->info.bclk);

         if ( Debug_enabled )
            logit( "et", "==>BIG EVENT TRIGGER:  Pin #%d\n", qch->pin );
      }

      if ( qch->info.eclk > 0L )
      {
         qch->info.eclk--;
         if (qch->info.eclk == 0L)
            qch->info.aclk = t->aset;
      }
   }
   return index;
}


/*=======================================================================*
 *                                to_clock()                             *
 *                                                                       *
 *  Warning: Round-off errors may occur on non-PC systems.               *
 *=======================================================================*/

static long to_clock( double seconds )
{
   return (long)(seconds * Drate / Decim);
}


/*=======================================================================*
 *                      tel_energy_valid_on_channel()                    *
 *                                                                       *
 * Check for energy on given channel.                                    *
 *                                                                       *
 * This routine is called while data are being "recorded" (saved to disk)*
 * for a teleseismic event to determine whether to stop saving it.       *
 *                                                                       *
 * At the moment, the data are saved simply for some constant block of   *
 * time (which depends on whether it is a "normal" or "big" event).      *
 *                                                                       *
 * where: qch     is a pointer to channel trigger variables              *
 *        buffer  is a pointer to raw data (for one channel only)        *
 *        npts    is the number of data points in the buffer             *
 *                                                                       *
 * returns: TRUE  if data is still seismically significant               *
 *          FALSE if data is other than significant                      * 
 *=======================================================================*/

FLAG tel_energy_valid_on_channel(
   TEL_CHANNEL * qch,
   unsigned * buffer,
   unsigned npts )
{
   if ( qch->info.event_type == 'B' )
      return TRUE;      /* record for MaxEventTime for big events */
   return FALSE;        /* record for MinEventTime for normal events */
}


/*=======================================================================*
 *                          tel_get_trigsetting                          *
 * Return pointer to trigger settings in use.                            *
 *=======================================================================*/

TEL_TRIGSETTING *tel_get_trigsetting( void )
{
   return t;
}


/*=======================================================================*
 *                        tel_initialize_channel                         *
 * Initialize trigger variables for given channel.                       *
 *=======================================================================*/

void tel_initialize_channel( TEL_CHANNEL *qch )
{
   int i;

   /* Trigger band variables */
   qch->info.tlta = 0.0;
   qch->info.tsta = 0.0;
   qch->info.tstav = 0.0;
   qch->info.dur = 0L;
   qch->info.durflg = FALSE;
   qch->info.bigev = FALSE;

   /* Masking band variables */
   qch->info.mlta = 0.0;
   qch->info.msta = 0.0;
   qch->info.mstav = 0.0;

   /* Clocks */
   qch->info.astat = FALSE;
   qch->info.aclk = 0L;
   qch->info.bclk = 0L;
   qch->info.cclk = 0L;
   qch->info.eclk = 0L;
   qch->info.sclk = t->sset;

   /* Other */
   qch->info.dc = 0.0;
   qch->info.first = TRUE;

   for ( i = 0; i < Dflt_len; i++ )
      qch->dec_buf[i] = 0;

   qch->i_dec_buf = 0;
   qch->need_for_dec_buf = (short)Dflt_len;  /* start with qch->dec_buf full */

   for ( i = 0; i < Lflt_len; i++ )
      qch->lp_buf[i] = 0L;

   qch->i_lp_buf = 0;
   qch->info.event_type = ' ';
}


/*=======================================================================*
 *                         tel_initialize_params                         *
 *          Initialize the trigger parameters to their defaults.         *
 *=======================================================================*/

void tel_initialize_params( void )
{
   /* Initialize teleseismic parameters */
   tel_set_ctlta( Ctlta );
   tel_set_ctsta( Ctsta );
   tel_set_ctstav( Ctstav );
   tel_set_tth1( Tth1 );
   tel_set_tth2( Tth2 );
   tel_set_tminen( Tminen );
   tel_set_mndur1( Mndur1 );
   tel_set_mndur2( Mndur2 );
   tel_set_dccoef( DCcoef );
   tel_set_cmlta( Cmlta );
   tel_set_cmsta( Cmsta );
   tel_set_cmstav( Cmstav );
   tel_set_mth( Mth );
   tel_set_mminen( Mminen );
   tel_set_aset( Aset );
   tel_set_bset( Bset );
   tel_set_cset( Cset );
   tel_set_eset( Eset );
   tel_set_sset( Sset );
}


/*=======================================================================*
 *                             tel_set_aset                              *
 * Set aset.                                                             *
 *=======================================================================*/

void tel_set_aset( double seconds )
{
   t->aset = to_clock( seconds );
   t->beginttime = u_timestamp();
}


/*=======================================================================*
 *                             tel_set_bset                              *
 * Set bset.                                                             *
 *=======================================================================*/

void tel_set_bset( double seconds )
{
   t->bset = to_clock( seconds );
   t->beginttime = u_timestamp();
}


/*=======================================================================*
 *                             tel_set_cmlta                             *
 * Set cmlta.                                                            *
 *=======================================================================*/

void tel_set_cmlta( double seconds )
{
   t->cmlta = (float) one_pole_coeff( seconds );
   t->beginttime = u_timestamp();
}


/*=======================================================================*
 *                             tel_set_cmsta                             *
 * Set cmsta.                                                            *
 *=======================================================================*/

void tel_set_cmsta( double seconds )
{
   t->cmsta = (float) one_pole_coeff( seconds );
   t->beginttime = u_timestamp();
}


/*=======================================================================*
 *                            tel_set_cmstav                             *
 * Set cmstav.                                                           *
 *=======================================================================*/

void tel_set_cmstav( double seconds )
{
   t->cmstav = (float) one_pole_coeff( seconds );
   t->beginttime = u_timestamp();
}


/*=======================================================================*
 *                             tel_set_cset                              *
 * Set cset.                                                             *
 *=======================================================================*/

void tel_set_cset( double f )
{
   t->cset = to_clock( f );
   t->beginttime = u_timestamp();
}


/*=======================================================================*
 *                             tel_set_ctlta                             *
 * Set ctlta.                                                            *
 *=======================================================================*/

void tel_set_ctlta( double seconds )
{
   t->ctlta = (float) one_pole_coeff( seconds );
   t->beginttime = u_timestamp();
}


/*=======================================================================*
 *                             tel_set_ctsta                             *
 * Set ctsta.                                                            *
 *=======================================================================*/

void tel_set_ctsta( double seconds )
{
   t->ctsta = (float) one_pole_coeff( seconds );
   t->beginttime = u_timestamp();
}


/*=======================================================================*
 *                             tel_set_ctstav                            *
 * Set ctstav.                                                           *
 *=======================================================================*/

void tel_set_ctstav( double seconds )
{
   t->ctstav = (float) one_pole_coeff( seconds );
   t->beginttime = u_timestamp();
}


/*=======================================================================*
 *                            tel_set_dccoef                             *
 * Set dccoef.                                                           *
 *=======================================================================*/

void tel_set_dccoef( double seconds )
{
   t->dccoef = (float) one_pole_coeff( seconds );
   t->beginttime = u_timestamp();
}


/*=======================================================================*
 *                             tel_set_eset                              *
 * Set eset.                                                             *
 *=======================================================================*/

void tel_set_eset( double f )
{
   t->eset = to_clock( f );
   t->beginttime = u_timestamp();
}


/*=======================================================================*
 *                            tel_set_mminen                             *
 * Set mminen.                                                           *
 *=======================================================================*/

void tel_set_mminen( long l )
{
   t->mminen = (float) l;      /* digitizer counts */
   t->beginttime = u_timestamp();
}


/*=======================================================================*
 *                            tel_set_mndur1                             *
 * Set mndur1.                                                           *
 *=======================================================================*/

void tel_set_mndur1( double seconds )
{
   t->mndur1 = to_clock( seconds );
   t->beginttime = u_timestamp();
}


/*=======================================================================*
 *                            tel_set_mndur2                             *
 * Set mndur2.                                                           *
 *=======================================================================*/

void tel_set_mndur2( double seconds )
{
   t->mndur2 = to_clock( seconds );
   t->beginttime = u_timestamp();
}


/*=======================================================================*
 *                             tel_set_mth                               *
 * Set mth.                                                              *
 *=======================================================================*/

void tel_set_mth( double db )
{
   t->mth = (float) db_to_ratio( db );
   t->beginttime = u_timestamp();
}


/*=======================================================================*
 *                             tel_set_sset                              *
 * Set sset.                                                             *
 *=======================================================================*/

void tel_set_sset( double seconds )
{
   t->sset = to_clock(seconds);
   t->beginttime = u_timestamp();
}


/*=======================================================================*
 *                            tel_set_tminen                             *
 * Set tminen.                                                           *
 *=======================================================================*/

void tel_set_tminen( long l )
{
   t->tminen = (float)l;      /* digitizer counts */
   t->beginttime = u_timestamp();
}


/*=======================================================================*
 *                              tel_set_tth1                             *
 * Set tth1.                                                             *
 *=======================================================================*/

void tel_set_tth1( double db )
{
   t->tth1 = (float) db_to_ratio( db );
   t->beginttime = u_timestamp();
}


/*=======================================================================*
 *                              tel_set_tth2                             *
 * Set tth2.                                                             *
 *=======================================================================*/

void tel_set_tth2( double db )
{
   t->tth2 = (float) db_to_ratio( db );
   t->beginttime = u_timestamp();
}


 /*=======================================================================*
  *                       tel_triggered_on_channel                        *
  *                                                                       *
  *               Look for a trigger on the given channel.                *
  *                                                                       *
  * This routine will be called to determine whether there is teleseismic *
  * activity (a trigger) on a given channel anywhere in the current       *
  * buffer.                                                               *
  *                                                                       *
  * where:   qch     is a pointer to channel trigger variables            *
  *          buffer  is a pointer to the multiplexed raw data             *
  *          nscan   is the number of scans in the buffer                 *
  *          chan    is the channel number to be processed                *
  *          nchan   is the number of channels in the buffer              *
  *                                                                       *
  * returns: -1      if there is no activity, or                          *
  *          index of the earliest triggering data point in buffer,       *
  *                  if the data contain a teleseism (or false alarm)     *
  *=======================================================================*/

long tel_triggered_on_channel( TEL_CHANNEL *qch, short *buffer, int nscan,
                               int chan, int nchan )
{
   long  index = -1L;                  /* return value         */
   short *buffer_ptr;                  /* input data           */
   int   remaining_in_buffer;          /* data remaining to use */

/* Do some tests
   *************/
   if ( nscan < 1 )
      er_abort( TEL_EMPTY_BUFFER );
   else if ( qch->need_for_dec_buf < 1 )
      er_abort( TEL_NEED_FOR_DEC_BUF );

/* Move through buffer
   *******************/
   buffer_ptr = &buffer[chan];
   for ( remaining_in_buffer = nscan; remaining_in_buffer > 0;
         remaining_in_buffer-- )
   {
      /* Convert another buffer point */
      qch->dec_buf[qch->i_dec_buf] = (int32_t)(*buffer_ptr);

/* If dec_buf is ready, run trigger algorithm on this point
   ********************************************************/
      if ((--qch->need_for_dec_buf) == 0)
      {
         index = teleseismic_trigger( qch, (long)(nscan - remaining_in_buffer),
                                      index );
         qch->need_for_dec_buf = (short) Decim;
         qch->i_lp_buf = (++qch->i_lp_buf) % Lflt_len;
      }

/* Update pointers
   ***************/
      qch->i_dec_buf = (++qch->i_dec_buf) % Dflt_len;
      buffer_ptr += nchan;
   }
   return index;
}

 /*=======================================================================*
  *                       tel_triggered_on_tracebuf                       *
  *                                                                       *
  * This routine will be called to determine whether there is teleseismic *
  * activity (a trigger)  anywhere in the current single-channel buffer.  *
  *                                                                       *
  * where:   qch     is a pointer to channel trigger variables            *
  *          buffer  is a pointer to the demultiplexed raw data           *
  *          nsample is the number of data samples in the buffer          *
  *                                                                       *
  * returns: -1      if there is no activity, or                          *
  *          index of the earliest triggering data point in buffer,       *
  *                  if the data contain a teleseism (or false alarm)     *
  *=======================================================================*/

long tel_triggered_on_tracebuf( TEL_CHANNEL *qch, int32_t *buffer, int nsample )
{
   long  index = -1L;                  /* return value          */
   int32_t *buffer_ptr;                /* input data            */
   int   remaining_in_buffer;          /* data remaining to use */

/* Do some tests
   *************/
   if ( nsample < 1 )
      er_abort( TEL_EMPTY_BUFFER );
   else if ( qch->need_for_dec_buf < 1 )
      er_abort( TEL_NEED_FOR_DEC_BUF );

/* Move through buffer
   *******************/
   buffer_ptr = buffer;
   for ( remaining_in_buffer = nsample; remaining_in_buffer > 0;
         remaining_in_buffer-- )
   {
      /* Grab another data point */
      qch->dec_buf[qch->i_dec_buf] = *buffer_ptr;

   /* If dec_buf is ready, run trigger algorithm on this point
      ********************************************************/
      if ((--qch->need_for_dec_buf) == 0)
      {
         index = teleseismic_trigger( qch, 
                                     (long)(nsample - remaining_in_buffer),
                                      index );
         qch->need_for_dec_buf = (short) Decim;
         qch->i_lp_buf = (++qch->i_lp_buf) % Lflt_len;
      }

   /* Update pointers
      ***************/
      qch->i_dec_buf = (++qch->i_dec_buf) % Dflt_len;
      buffer_ptr++;
   }
   return index;
}
