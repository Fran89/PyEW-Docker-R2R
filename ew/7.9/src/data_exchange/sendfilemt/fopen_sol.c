/*
 *    Revision history:
 *     Revision 1.1  2002/12/20 02:41:38  lombard
 *     Initial revision
 *
 */

/* fopen_sol.c */

/****************************************************************
 *                          fopen_excl()                        *
 *                        Solaris version                       *
 *                                                              *
 *  Solaris doesn't have an exclusive open. So it is up to the  *
 *  applications feeding sendfilemt to make sure the file is    *
 *  complete before it is placed in sendfilemt's OutDir.        *
 ****************************************************************/

#include <stdio.h>
#include "sendfilemt.h"


FILE *fopen_excl( const char *fname, const char *mode )
{
    return fopen( fname, mode );
}
