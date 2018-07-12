
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: interpolate.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 17:17:36  lucky
 *     Initial revision
 *
 *
 */

#include <earthworm.h>       /* needed for 'int32_t' type on Windows */
#include <trace_buf.h>

   /*************************************************************
    *                       Interpolate()                       *
    *                                                           *
    *  Given the value of the last sample in the previous msg   *
    *  & the number of sample intervals since the previous msg, *
    *  linearly interpolate the proper number of samples and    *
    *  insert them at the beginning of the tracebuf message     *
    *************************************************************/

void Interpolate( char *WaveBuf, int GapSize, int32_t LastValue )
{
/* WaveBuf   is a pointer to the current tracebuf message 
 * GapSize   is the number of sample intervals since the previous message   
 * LastValue is the value of the last data sample of the previous message 
 */
   int      i;
   int      j = 0;
   int      nInterp = GapSize - 1;
   TRACE_HEADER *WaveHead  = (TRACE_HEADER *) WaveBuf;
   int32_t      *WaveLong  = (int32_t *) (WaveBuf + sizeof(TRACE_HEADER));
   double   SampleInterval = 1. / WaveHead->samprate;
   double   delta = (double)(WaveLong[0] - LastValue) / GapSize;

/* Shift all data points to make room for interpolated values */
   for ( i = WaveHead->nsamp - 1; i >= 0; i-- )
      WaveLong[i + nInterp] = WaveLong[i];

/* Insert the interpolated samples at the start of the message */
   for ( i = 0; i < nInterp; i++ )
      WaveLong[i] = (int32_t) (LastValue + (++j * delta) + 0.5);

   WaveHead->nsamp     +=  nInterp;
   WaveHead->starttime -= (nInterp*SampleInterval);

   return;
}
