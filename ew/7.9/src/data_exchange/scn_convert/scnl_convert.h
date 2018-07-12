/*
 *   This file is managed using Concurrent Versions System (CVS).
 *
 *    $Id: scnl_convert.h 2549 2006-12-28 23:27:53Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/12/28 23:27:53  lombard
 *     Added version number, printed on startup.
 *     Revised scnl2scn to provide complete mapping from SCNL back to SCN, using
 *     configuration command similar to scn2scnl.
 *
 *
 */


#ifndef SCNL_CONVERT_H
#define SCNL_CONVERT_H

#include <earthworm.h>
#include "scnl2scn.h"

/* Windows provides a non-ANSI version of snprintf,
   name _snprintf.  The Windows version has a
   different return value than ANSI snprintf.
   ***********************************************/
#ifdef _WINNT
#define snprintf _snprintf
#endif

/* How many S2S structures to allocate at once: */
#define INCR_SCNL 10

typedef struct 
{
    char *s;
    char *c;
    char *n;
} SCN;
typedef struct 
{
    char *s;
    char *c;
    char *n;
    char *l;
} SCNL;

typedef struct
{
    SCN scn;
    SCNL scnl;
    int ns, nc, nn, nl;
} S2S;

int s2s_com( void );
void sort_scnl(void);
int scnl2scn( S2S* );
void GetConfig( char *config_file, GPARM *Gparm );
void LogConfig( GPARM *Gparm, char *version );
int to_trace_scn( char *msg );
int to_pick2k( char *pick_scnl, char *pick2k, unsigned char newMsgType );
int to_coda2k( char *coda_scnl, char *coda2k, unsigned char newMsgType );


#endif
