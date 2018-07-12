/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: irige.h 2 2000-02-14 16:16:56Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 16:00:43  lucky
 *     Initial revision
 *
 *
 */


/*
 * irige.h
 */

/* This block of error detection/return code added by Lynn Dietz 10/93  */
#define TIME_OK        1
#define TIME_WAIT      0
#define TIME_NOSYNC   -1
#define TIME_FLAT     -2
#define TIME_NOISE    -3
#define TIME_UNKNOWN -99
/*     eSync =  1  TIME_OK when there are no problems decoding time     *
 *           =  0  TIME_WAIT before irige gets its first synch with code*
 *           = -1  TIME_NOSYNC when irige loses synch; finds double-    *
 *                 Mark, but has trouble decoding time; glitchey stretch*
 *                 of otherwise ok time code or low freq (<5 hz?) noise *
 *           = -2  TIME_FLAT when time code signal goes flat; count     *
 *                 since last pulse edge exceeds test value, NoTime     *
 *           = -3  TIME_NOISE when time code has too few or too many    *
 *                 Marks (wide pulses);  signal is from some other      *
 *                 wiggly source                                        *
 *           = -99 TIME_UNKNOWN when the IRIGE function has not yet     *
 *                 been called                                          *
 *     For all buffers with negative eSync values, the time returned by *
 *     irige is calculated from scan # and sampling rate at the last    *
 *     successful decode.  It will eventually start to drift away from  *
 *     the actual time.                                                 */


struct TIME_BUFF {
        int     tmstatus;       /* Time status (0,1,2)                  */
        long    scan;           /* Scan count                           */
        double  t;              /* Time at scan count                   */
        double  tdecode;        /* Time of last decode                  */
        double  a, b;           /* t = a + b * scan                     */
};
