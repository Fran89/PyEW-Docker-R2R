/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: lm_ws.h 345 2000-12-19 18:31:25Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/12/19 18:31:25  lombard
 *     Initial revision
 *
 *
 *
 */

/*
 * lm_ws.h
 */

#ifndef LM_WS_H
#define LM_WS_H

#include "lm.h"
#include "lm_config.h"

/* The default wave_servers list in ${EW_PARAMS} */
#define DEF_SERVER "servers"

#ifndef INADDR_NONE
#define INADDR_NONE     0xffffffff
#endif

int Add2ServerList( char *, LMPARAMS * );
int readServerFile( LMPARAMS * );
int getAmpFromWS( EVENT *, LMPARAMS *, DATABUF *);
int initWsBuf( long );

#endif
