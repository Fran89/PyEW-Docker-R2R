
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: waveman2disk.h 1629 2004-07-16 20:47:13Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2004/07/16 20:47:13  lombard
 *     Modified to provide minimal support for SEED location codes.
 *     waveman2disk can now query both SCN (old) and SCNL (new) wave_serverV
 *
 *     Revision 1.3  2003/01/06 23:26:25  bogaert
 *     Increased MAX_WAVESERVER from 10 to 1000.
 *
 *     Revision 1.2  2001/04/12 04:26:55  lombard
 *     cleanup.
 *
 *     Revision 1.1  2000/02/14 20:02:23  lucky
 *     Initial revision
 *
 *
 */

#ifndef WAVEMAN2DISK_H
#define WAVEMAN2DISK_H

/*
 * waveman2disk.h : Include file for waveman2disk.c
 */
#include <trace_buf.h>

typedef struct _scn
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
#define MAX_WAVESERVERS   1000
#define MAX_ADRLEN        20
#define MAX_STATIONS     256


int CatPsuedoTrig (char *, SCNL *, int, int);

#endif
