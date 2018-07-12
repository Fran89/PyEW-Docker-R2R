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
#include "sendfile_srv.h"


FILE *fopen_excl( const char *fname, const char *mode )
{
    return _fsopen( fname, mode, _SH_DENYRW );
}
