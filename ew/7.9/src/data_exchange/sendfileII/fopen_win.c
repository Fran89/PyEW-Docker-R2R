/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: fopen_win.c 5379 2013-02-20 19:27:08Z paulf $
 *
 *    Revision history:
 *     $Log: fopen_win.c,v $
 *     Revision 1.1  2002/12/20 02:41:38  lombard
 *     Initial revision
 *
 *
 *
 */

/* fopen_win.c */

/****************************************************************
 *                          fopen_excl()                        *
 *                        Windows version                       *
 *                                                              *
 *  Open a disk file exclusively.                               *
 *  Returns a file pointer if the file was opened exclusively.  *
 *  Returns NULL if the file can't be opened.                   *
 ****************************************************************/

#include <stdio.h>
#include <share.h>
#include "sendfileII.h"


FILE *fopen_excl( const char *fname, const char *mode )
{
    return _fsopen( fname, mode, _SH_DENYRW );
}
