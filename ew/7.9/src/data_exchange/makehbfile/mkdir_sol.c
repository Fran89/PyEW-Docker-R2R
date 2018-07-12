/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: mkdir_sol.c 1157 2002-12-20 02:41:38Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2002/12/20 02:40:33  lombard
 *     Initial revision
 *
 *
 *
 */

/*
 *   mkdir_sol.c  Solaris version
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "makehbfile.h"


/******************************************************************
 *  mkdir_ew( )  Create a new directory                           *
 *  permissions are "777"                                         *
 ******************************************************************/

int mkdir_ew( char *path )
{
    mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;
    int    rc   = mkdir( path, mode );

    return ((rc == -1) && (errno != EEXIST)) ? -1 : 0;
}

