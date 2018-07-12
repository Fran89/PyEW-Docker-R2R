/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: dcc_misc.h 23 2000-03-05 21:49:40Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/03/05 21:47:33  lombard
 *     Initial revision
 *
 *
 *
 */

/* Protos for misclib */

#include <stationidx.h>

#define ISMAGOPEN(a) ((a)>=0)

void _bomb2(int, char *, ...);
void _bombdesc(char *, char *, int);

#include <seed/itemlist.h>

/* Handle protos for compatibility routines */

#ifndef HAVE_STRERROR
char *strerror(int errnum);
#endif

#include <dcc_misc_proto.h>
