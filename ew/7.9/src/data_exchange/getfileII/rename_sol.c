/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: rename_sol.c 4543 2011-08-10 14:54:23Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2004/07/28 17:59:41  dietz
 *     added more string-length checking to avoid array overflows
 *
 *     Revision 1.1  2002/12/20 02:39:11  lombard
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
#include "getfileII.h"

/**************************************************************************
 *  rename_ew( )  Moves a file                   Solaris version          *
 *  path1 = file to be moved.  path2 = destination directory              *
 **************************************************************************/

int rename_ew( char *path1, char *path2 )
{
    int  rc;
    char path3[MAXPATHSTR*2];

    if( strlen(path1)+strlen(path2)+1 >= sizeof(path3) )
    {
      my_log( "et", "path buffer overflow in rename_ew\n" );
      return( -1 );
    }
    strcpy( path3, path2 );
    strcat( path3, "/" );
    strcat( path3, path1 );
    rc = rename( path1, path3 );
    if ( rc == -1 ) perror( "rename" );
    return rc;
}

