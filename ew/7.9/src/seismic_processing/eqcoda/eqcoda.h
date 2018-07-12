
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: eqcoda.h 1489 2004-05-18 20:21:00Z dietz $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2004/05/18 20:21:00  dietz
 *     Modified to use TYPE_EVENT_SCNL as input and to include location
 *     codes in the TYPE_HYP2000ARC output msg.
 *
 *     Revision 1.3  2000/08/17 20:09:28  dietz
 *     Changed configurable clip param from DigNbit to ClipCount
 *
 *     Revision 1.2  2000/07/21 23:07:44  dietz
 *     Added per-channel parameters (instead of globals).
 *
 *     Revision 1.1  2000/02/14 17:07:37  lucky
 *     Initial revision
 *
 *
 */

/*
 *  eqcoda.h
 */

#include <trace_buf.h>

/* Define some simple "functions":
   *******************************/
#ifndef MAX
#define MAX(a,b) (((a)>(b)) ? (a) : (b))        /* Larger value   */
#endif
#ifndef MIN
#define MIN(a,b) (((a)<(b)) ? (a) : (b))        /* Smaller value  */
#endif
#define ABS(a) ((a) > 0 ? (a) : -(a))           /* Absolute value */


/* Station list parameters
   ***********************/
typedef struct {
   char   sta[TRACE2_STA_LEN];    /* Station name                        */
   char   chan[TRACE2_CHAN_LEN];  /* Component code                      */
   char   net[TRACE2_NET_LEN];    /* Network code                        */
   char   loc[TRACE2_LOC_LEN];    /* Location code                       */
   float  CodaTerm;      /* Coda termination threshold (in counts)       */
   int    UsedDefault;   /* flag=1 if default values were used,=0 if not */
   long   ClipCount;     /* number of counts (0-to-peak) at which this   */
                         /*   channel is clipped; configurable           */
   long   KlipP1;        /* Clip level for first P-amplitude             */ 
   long   KlipP2;        /* Clip level for 2nd & 3rd P-amplitude         */
   long   KlipC;         /* Clip level for coda 2-sec avg abs amplitudes */
} STAPARM;


/* Prototypes for all functions from eqm2_calls.c
 ************************************************/
int  codasub( float, float, long *, long, float *, short *,
	      float *, float *, float *, float *, float * );
int  cntrtime2( float, short *, float * );
int  l1fit  ( float *, float *, short, float *, float *, float *,
	      float *, float * );
int  codmag( float, float, float * );	
void cphs2 ( float, float, char * );
void asnwgt( short, float, float, float, short *, short * );	
void cnvwt2( short * );
void cdxtrp( float, int, char  *, short *, short, float, float,
	     float, float, float, int *, short * );
	

/* Prototypes for all functions from stalist.c
 ************************************************/
int  GetStaList( STAPARM **, int *, char * );
void SetParmStaList( STAPARM *, int );
void LogStaList( STAPARM *, int );
int  CompareSCNLs( const void *, const void * );


