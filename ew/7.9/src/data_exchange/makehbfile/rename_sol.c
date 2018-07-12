/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: rename_sol.c 1157 2002-12-20 02:41:38Z lombard $
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
 *   rename_sol.c
 */
#include <stdio.h>
#include <string.h>
#include "makehbfile.h"

/**************************************************************************
 *  rename_ew( )  Moves a file                   Solaris version          *
 *  path1 = file to be moved.  path2 = destination directory              *
 **************************************************************************/

int rename_ew( char *path1, char *path2 )
{
    int  rc;
    char path3[80];

    strcpy( path3, path2 );
    strcat( path3, "/" );
    strcat( path3, path1 );
    rc = rename( path1, path3 );
    if ( rc == -1 ) perror( "rename" );
    return rc;
}

