
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: mutils.h 2476 2006-10-23 21:22:54Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2006/10/23 21:22:54  paulf
 *     made mutils more linux friendly
 *
 *     Revision 1.1  2000/02/14 17:17:36  lucky
 *     Initial revision
 *
 *
 */

/* FILE: mutils.h                               (D. Tottingham  03/24/91)

This is an include file of defines, data structure definitions and
external declarations that are common in the mutils module.

*/

#ifndef _MUTILS_
#define _MUTILS_

/************************************************************************
 *                             INCLUDES                                 *
 ************************************************************************/
#include <sys/timeb.h>

#include "mconst.h"


/************************************************************************
 *                              DEFINES                                 *
 ************************************************************************/
#define INVALID_DRIVE           -1
#define UNEXPECTED_ERROR        0
#define TEL_INVALID_SRATE       153
#define TEL_EMPTY_BUFFER        154
#define TEL_NEED_FOR_DEC_BUF    155


/************************************************************************
 *                      STRUCTURE DEFINITIONS                           *
 ************************************************************************/
typedef struct {                 /* used by er_abort */
   unsigned id;
   char * text;
   FLAG memory_dump;
   FLAG to_logfile;
} ER_MESSAGE;


/**************************************************************************
 *                       EXTERNAL DECLARATIONS                            *
 * These functions can be called from all modules that include this file. *
 **************************************************************************/
double u_convert_time( struct timeb );
double u_timestamp ( void );
void er_abort( unsigned );

#endif
