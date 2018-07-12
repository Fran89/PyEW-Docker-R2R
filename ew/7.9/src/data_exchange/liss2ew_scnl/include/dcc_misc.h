/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: dcc_misc.h 2192 2006-05-25 15:32:13Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/05/25 15:32:13  paulf
 *     first checkin from Hydra
 *
 *     Revision 1.1  2005/06/30 20:39:55  mark
 *     Initial checkin
 *
 *     Revision 1.1  2005/04/21 16:55:26  mark
 *     Initial checkin
 *
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
