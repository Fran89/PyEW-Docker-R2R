/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: fopen_sol.c 5379 2013-02-20 19:27:08Z paulf $
 *
 *    Revision history:
 *     $Log: fopen_sol.c,v $
 *     Revision 1.1  2002/12/20 02:41:38  lombard
 *     Initial revision
 *
 *
 *
 */

/* fopen_sol.c */

/****************************************************************
 *                          fopen_excl()                        *
 *                        Solaris version                       *
 *                                                              *
 *  Solaris doesn't have an exclusive open. So it is up to the  *
 *  applications feeding sendfileII to make sure the file is    *
 *  complete before it is placed in sendfileII's OutDir.        *
 ****************************************************************/

#include <stdio.h>
#include "sendfileII.h"


FILE *fopen_excl( const char *fname, const char *mode )
{
    return fopen( fname, mode );
}
