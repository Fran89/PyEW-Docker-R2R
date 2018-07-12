/*
 *   This file is managed using Concurrent Versions System (CVS).
 *
 *    $Id: scn_convert.h 2549 2006-12-28 23:27:53Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2006/12/28 23:27:53  lombard
 *     Added version number, printed on startup.
 *     Revised scnl2scn to provide complete mapping from SCNL back to SCN, using
 *     configuration command similar to scn2scnl.
 *
 *     Revision 1.3  2004/10/19 23:21:01  dietz
 *     added prototype for LogConfig
 *
 *     Revision 1.2  2004/10/19 21:54:04  lombard
 *     Changes to support rules for renaming specific and wild-carded SCNs to
 *     configured SCNLs.
 *
 *     Revision 1.1  2004/05/18 22:57:30  kohler
 *     Initial entry into CVS.
 *
 *     Revision 1.1.1.1  2004/05/18 16:33:18  kohler
 *     initial import into CVS
 *
 */


#ifndef SCN_CONVERT_H
#define SCN_CONVERT_H

#include <earthworm.h>

/* Windows provides a non-ANSI version of snprintf,
   name _snprintf.  The Windows version has a
   different return value than ANSI snprintf.
   ***********************************************/
#ifdef _WINNT
#define snprintf _snprintf
#endif

typedef struct
{
   char MyModName[MAX_MOD_STR];  /* Module name */
   char InRing[MAX_RING_STR];    /* Name of ring containing input messages */
   char OutRing[MAX_RING_STR];   /* Name of ring containing output messages */
   long InKey;                   /* Key to input ring */
   long OutKey;                  /* Key to output ring */
   int  HeartbeatInt;            /* Heartbeat interval in seconds */
} GPARM;

/* How many S2S structures to allocate at once: */
#define INCR_SCN 10

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
    int ns, nc, nn;
} S2S;

int s2s_com( void );
void sort_scn(void);
int scn2scnl( S2S* );
void GetConfig( char *config_file, GPARM *Gparm );
void LogConfig( GPARM *Gparm, char *version );
int to_trace_scnl( char *msg );
int to_pick_scnl( char *pick2k, char *pick_scnl, int outLen, 
		  unsigned char newMsgType );
int to_coda_scnl( char *coda2k, char *coda_scnl, int outLen, 
		  unsigned char newMsgType );


#endif
