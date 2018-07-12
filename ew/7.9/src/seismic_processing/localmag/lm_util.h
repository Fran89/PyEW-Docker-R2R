/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: lm_util.h 2094 2006-03-10 13:03:28Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2006/03/10 13:03:28  paulf
 *     upgraded to SCNL version 2.1.0, tested at Utah
 *
 *     Revision 1.5  2002/03/17 18:32:49  lombard
 *     Changed the way trace times and search times are set. Unchanged is the
 *       trace start: a specified number of seconds before the first P arrival
 *       from the layered velocity model.
 *       Trace end is now a specified number of seconds after the Sg arrival
 *       computed using a specified Sg speed instead of the layered model.
 *       Taper times are added to each end of the trace to get the 10% taper
 *       length as before.
 *       Search start is now a fixed number of seconds before the selected
 *       search start phase - either first P or first S arrival from the
 *       layered model.
 *       Search end is now a specified number of seconds after the Sg arrival
 *       computed using a specified Sg speed.
 *       The Sg arrival is determined using hypocentral distance, so event dept
 *     must be passed to traceEndTime() and searchEndTime().
 *       To determine how long localmag should wait for traces to arrive at
 *       the wave_server before processing a realtime event, a new function
 *       setMaxDelay computes the tarce end time for a hypothetical station
 *       at maxDist from an event on the surface. The optional extraWait is
 *       added to this time; this function is called during startup if
 *       localmag is running as an earthworm module connected to transport.
 *     Changed the way channels are selected for use in the station magnitude:
 *       before, both the E and N channels had to have peak amps before they
 *       could be used, although single channels were included in the Mag message.
 *       Now, any channel with an amplitude is counted in the station average.
 *
 *     Revision 1.4  2002/01/24 19:34:09  lombard
 *     Added 5 percent cosine taper in time domain to both ends
 *     of trace data. This is to eliminate `wrap-around' spikes from
 *     the pre-event noise-check window.
 *
 *     Revision 1.3  2001/01/15 03:55:55  lombard
 *     bug fixes, change of main loop, addition of stacker thread;
 *     moved fft_prep, transfer and sing to libsrc/util.
 *
 *     Revision 1.2  2000/12/25 22:14:39  lombard
 *     bug fixes and development
 *
 *     Revision 1.1  2000/12/19 18:31:25  lombard
 *     Initial revision
 *
 *
 *
 */

/*
 * lm_util.h: Header for lm_util.c, a collection of utility functions for
 *  the local_mag module.
 */

#ifndef LM_UTIL_H
#define LM_UTIL_H

#include "lm.h"
#include "lm_config.h"

/* Largest expect line from a hyp2000 archive message; see MAX_BYTES_PER_EQ
   in <earthworm.h> */
#define MAX_ARC_LINE 225

#define LM_MAXTXT         150

/* Function prototypes */
double getStaDist( char *, char *, char *, char *, double *, double *, EVENT *, 
                   LMPARAMS *);
double peakSearchEnd(STA *, LMPARAMS *, EVENT *);
double peakSearchStart(STA *, LMPARAMS *);
void setTaperTime(STA *, LMPARAMS *, EVENT *);
double traceEndTime(STA *, LMPARAMS *, EVENT *);
double traceStartTime(STA *, LMPARAMS *);
int CompareSCNLPARs( const void *s1, const void *s2 );
int CompareStaDist( const void *s1, const void *s2 );
int addCompToEvent( char *, char *, char *, char *, EVENT *, LMPARAMS *, STA **, 
                    COMP3 **);
int getAmpFromSource( EVENT *, LMPARAMS *);
int getAmpFromTrace( EVENT *, LMPARAMS *);
int getEvent( EVENT *, LMPARAMS * );
int getWAResp( WA_PARAMS *);
int initBufs( long, int );
int initLogA0(LMPARAMS *);
int makeWA(STA *, COMP3 *, LMPARAMS *, EVENT *);
int procArc( char *, EVENT *);
int writeMag(EVENT *, LMPARAMS *);
void EstPhaseArrivals(STA *, EVENT *, int);
void cleanTrace( void );
void endEvent(EVENT *, LMPARAMS *);
void getMagFromAmp(EVENT *, LMPARAMS *);
void getPeakAmp(DATABUF *, COMP3 *, STA *, LMPARAMS *, EVENT *);
void prepTrace(DATABUF *, STA *, COMP3 *, LMPARAMS *, EVENT *, int);
void saveWATrace( DATABUF *, STA *, COMP3 *, EVENT *, LMPARAMS *);
void setMaxDelay( LMPARAMS *, EVENT *);

#endif
