/*
 *   chdir_sol.c
 */

#include <unistd.h>
#include "getfile_cl.h"

/*****************************************************************************
 *  chdir_ew( )  changes current working directory; Solaris version          *
 *****************************************************************************/

int chdir_ew( char *path )
{
    return( chdir( path ) );
}

