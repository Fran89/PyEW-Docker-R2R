/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: gm_ws.h 474 2001-03-30 19:14:25Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2001/03/30 19:14:25  lombard
 *     Initial revision
 *
 *
 *
 */
/*
 * gm_ws.h
 */

#ifndef GM_WS_H
#define GM_WS_H

#include "gm.h"
#include "gm.h"

/* The default wave_servers list in ${EW_PARAMS} */
#define DEF_SERVER "servers"

#ifndef INADDR_NONE
#define INADDR_NONE     0xffffffff
#endif

int Add2ServerList( char *, GMPARAMS * );
int readServerFile( GMPARAMS * );
int getGMFromWS( EVENT *, GMPARAMS *, DATABUF *, SM_INFO *);
int initWsBuf( long );

#endif
