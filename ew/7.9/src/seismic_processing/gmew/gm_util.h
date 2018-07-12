/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: gm_util.h 4217 2011-05-16 19:55:21Z scott $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2006/03/15 14:21:54  paulf
 *     SCNL version of gmew v0.2.0
 *
 *     Revision 1.2  2001/07/18 19:18:35  lucky
 *     *** empty log message ***
 *
 *     Revision 1.1  2001/03/30 19:14:25  lombard
 *     Initial revision
 *
 *
 *
 */
/*
 * gm_util.h: Header for gm_util.c, a collection of utility functions for
 *  the gma module.
 *
 */

#ifndef GM_UTIL_H
#define GM_UTIL_H

#include <rw_strongmotionII.h>
#include "gm.h"

#define GM_MAXTXT         150

/* Function prototypes */
double getStaDist( char *, char *, char *, char *, double *, double *, EVENT *, 
                   GMPARAMS *);
double peakSearchEnd(STA *, GMPARAMS *);
double peakSearchStart(STA *, GMPARAMS *);
double traceEndTime(STA *, GMPARAMS *);
double traceStartTime(STA *, GMPARAMS *);
int CompareSCNLPARs( const void *s1, const void *s2 );
int CompareStaDist( const void *s1, const void *s2 );
int addCompToEvent( char *, char *, char *, char *, EVENT *, GMPARAMS *, STA **, 
                    COMP3 **);
int initBufs( long );
int getGMFromTrace( EVENT *, GMPARAMS *);
int makeGM(STA *, COMP3 *, GMPARAMS *, EVENT *, DATABUF *);
int procArc( char *, EVENT *, MSG_LOGO, MSG_LOGO);
void EstPhaseArrivals(STA *, EVENT *, int);
void cleanTrace( DATABUF *);
void getPeakGM(DATABUF *, COMP3 *, STA *, GMPARAMS *, EVENT *,  SM_INFO *);
void prepTrace(DATABUF *, STA *, COMP3 *, GMPARAMS *);
void endEvent(EVENT *, GMPARAMS *);

int addSR2list( double );
void checkSRlist();

#endif
