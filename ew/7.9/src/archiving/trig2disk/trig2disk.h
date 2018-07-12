
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: trig2disk.h 3584 2009-04-28 17:17:00Z luetgert $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.7  2009/04/28 17:17:00  luetgert
 *     upped MAX_WAVESERVERS to 20.
 *     .CVS: ----------------------------------------------------------------------
 *
 *     Revision 1.6  2005/11/28 16:54:51  friberg
 *     upped MAX_STATIONS to 1024 for AVO
 *
 *     Revision 1.5  2005/11/02 18:04:27  luetgert
 *     Increased array dimensions to accommodate more stations being saved to disk.
 *     .
 *
 *     Revision 1.4  2004/05/31 17:55:07  lombard
 *     Modified for location code.
 *
 *     Revision 1.3  2001/04/12 04:08:22  lombard
 *     define SCN string members with macros, included from trace_buf.h
 *     Added multiple inclusion exclusion
 *     removed pin number from PSCN struct, renamed to SCN.
 *     Added function prototype for CatPsuedoTrig().
 *
 *     Revision 1.2  2000/08/03 19:31:43  lucky
 *     Fixed a bug which caused periodic crashes on NT and Solaris. It was caused
 *     by a non-null-terminated string. Also, cleaned up many things, like:
 *       *  Fixed lint problems;
 *       *  Defined things as static where appropriate;
 *       *  Kill the menu before rebuilding it each time through the loop
 *
 *     Revision 1.1  2000/02/14 19:48:44  lucky
 *     Initial revision
 *
 *
 */

#ifndef TRIG2DISK_H
#define TRIG2DISK_H

/*
 * trig2disk.h : Include file for trig2disk.c
 */

#include <trace_buf.h>

typedef struct
{
    char    sta[TRACE2_STA_LEN];
    char    chan[TRACE2_CHAN_LEN];
    char    net[TRACE2_NET_LEN];
    char    loc[TRACE2_LOC_LEN];
} SCNL;

#define SAMPRATE 100.0

#define MAX_STR          255
#define MAXLINE         1000
#define MAX_NAME_LEN      50
#define MAXCOLS          100
#define MAXTXT           150
#define MAX_WAVESERVERS   20
#define MAX_ADRLEN        20
#define MAX_STATIONS     1024


/* Function prototypes */
int CatPsuedoTrig (char *, SCNL *, int, int);


#endif
