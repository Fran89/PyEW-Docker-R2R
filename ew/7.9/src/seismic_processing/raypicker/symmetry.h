/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: symmetry.h 4475 2011-08-04 15:17:09Z kevin $
 *
 *    Revision history:
 *     $Log: symmetry.h,v $
 *     Revision 1.1  2008/05/29 18:46:46  mark
 *     Initial checkin (move from \resp; also changes for station list auto-update)
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:50  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.3  2005/06/20 21:37:07  cjbryan
 *     cleanup
 *
 *     Revision 1.2  2005/03/29 23:58:57  cjbryan
 *     revised to use macros.h SIGN
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:27  michelle
 *     New Hydra Import
 *
 *
 *
 */
/*
 * Declaration of functions and struct used to track symmetry status.
 * Symmetry is only tracked once for a channel -- from the original
 * (raw) broadband or short-period series.
 * 
 * @author Dale Hanych, Genesis Business Group (dbh)
 * @version 1.0 August 2003, dbh
 */

#ifndef SYMMETRY_H
#define SYMMETRY_H

#define SYMMETRY_CYCLES    3 /* number of peaks to average for asymmetry test [lcyc]  */


#define SYMM_SYMMETRIC     1
#define SYMM_PARTSYMMETRIC 2 /* only defined for SymmetryCheck() return code handling */
#define SYMM_ASYMMETRIC    3

typedef  char  SYMMETRY_TRACK_TYPE; /* Symmetry tracking only needs a type that can 
                                     * contain the SYMM_xxx values, above. */

typedef struct _SYMMETRY_CHECK_DATA
{
    int                  ncycln;                     /* number of sample points in 4.5 seconds */
    int                  icnt;                       /* counter for current ncycle span check  */
    double               rawmean;                    /* mean of the raw data over time (a1)    */    
    double               last;                       /* last demeaned data point (alst)        */
    double               cmx;                        /* last data value                        */
    double               omn;                        /* min ___                                */
    double               omx;                        /* max ___                                */
    double               omni;
    double               omxi;
    SYMMETRY_TRACK_TYPE  ir3;                        /* current symmetry state                 */
    SYMMETRY_TRACK_TYPE  lr3;                        /* last symmetry state                    */
    int                  nasmP;                      /* counter into symmetryP                 */
    int                  nasmM;                      /* counter into symmetryM                 */
    double               symmetryP[SYMMETRY_CYCLES]; /* working area for channel symmetry      */
    double               symmetryM[SYMMETRY_CYCLES]; /* working area for channel symmetry      */
} SYMMETRY_CHECK_DATA;

/* function prototypes */
int InitSymmetryData(const double firstRaw, const double rawmean, 
                     const double rectmean, const double snmin, 
                     const double sampleRate, SYMMETRY_CHECK_DATA *symData);
int SymmetryCheck(const double c2, const double btn, const double xmt, 
                  const double snmin, const long kr3Index,  
                  SYMMETRY_TRACK_TYPE *kr3, SYMMETRY_CHECK_DATA *symData);

#endif /* SYMMETRY_H */
