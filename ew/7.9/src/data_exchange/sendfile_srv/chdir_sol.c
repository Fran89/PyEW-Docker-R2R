/************************************************************
 *            chdir_win.c  Solaris version                  *
 *                                                          *
 ************************************************************/

#include <unistd.h>
#include "sendfile_srv.h"

/****************************************************************************
 *  chdir_ew( )  changes current working directory; Solaris version         *
 ****************************************************************************/
int chdir_ew( char *path )
{
    return( chdir( path ) );
}

