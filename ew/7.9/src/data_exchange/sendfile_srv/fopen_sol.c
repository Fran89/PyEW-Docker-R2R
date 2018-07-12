/****************************************************************
 *                          fopen_excl()                        *
 *                        Solaris version                       *
 *                                                              *
 *  Solaris doesn't have an exclusive open. So it is up to the  *
 *  applications feeding sendfileII to make sure the file is    *
 *  complete before it is placed in sendfileII's OutDir.        *
 ****************************************************************/

#include <stdio.h>
#include "sendfile_srv.h"


FILE *fopen_excl( const char *fname, const char *mode )
{
    return fopen( fname, mode );
}
