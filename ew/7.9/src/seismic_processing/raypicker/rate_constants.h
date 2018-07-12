/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: rate_constants.h 4453 2011-07-27 21:10:40Z stefan $
 *
 *    Revision history:
 *     $Log: rate_constants.h,v $
 *     Revision 1.1  2008/05/29 18:46:46  mark
 *     Initial checkin (move from \resp; also changes for station list auto-update)
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:50  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.3  2004/11/01 02:03:27  cjbryan
 *     removed all amplitude determination code from the picker and changed all error
 *     codes to positive numbers
 *     CVS ----------------------------------------------------------------------
 *
 *     Revision 1.2  2004/06/10 20:22:35  cjbryan
 *     re-engineered array initialization
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:27  michelle
 *     New Hydra Import
 *
 *
 *
 */
 
/*
 * This file declares functions and data struct used to manage and
 * maintain values which are constant to specific sample rates.
 * This allows the constants to be shared and reused among
 * all channels that have the same nominal rate.
 * 
 * Many variable names are from the original FORTRAN.
 * 
 */

#ifndef RATE_CONSTANTS_H
#define RATE_CONSTANTS_H

typedef struct _RATE_CONSTANTS_DATA
{
    double     sample_rate;
    double     sample_interval;
    double     lag1sec;          /* Buland's d1  */
    double     lag2sec;          /* Buland's d2  */
    double     lag10sec;         /* Buland's d10 */
    double     lag30sec;         /* Buland's d30 */

    /* nset() */
    int        nsettm; /* samples before trigger declaration allowed (series initialization time) */
    int        nlnktm; /* max samples that asymmetric signal may persist without rejection ('link' time) */
    int        nsmotm; /* limit on how far to attempt arrival time refinement (points) */
    double     maxph;  /* max width of an amplitude phase */

    double     avemn;   /* signal-to-noise minimum for 1st half-cycle */
   
    long       navest;  /* pre-filter averaging start (sample value index) */
    long       navetm;  /* pre-filter averaging end (sample value index) */
    double     aveln;   /* inverse of pre-filter averaging length (sample value count) */
 
    double     iirrmp;  /* pre-filter length in sample points */

} RATE_CONSTANTS_DATA;


/* Function prototypes */
int InitRateConstants(int maxRateConstants);
void FreeRateConstants();
int GetRateConstants(const double sampleRate, RATE_CONSTANTS_DATA ** p_constants, 
                     int maxRateConstants, int debug);

#endif /* RATE_CONSTANTS_H */
