/*
 *   rename_sol.c
 */
#include <stdio.h>
#include <string.h>
#include "getfile_cl.h"

/**************************************************************************
 *  rename_ew( )  Moves a file                   Solaris version          *
 *  path1 = file to be moved.  path2 = destination directory              *
 **************************************************************************/

int rename_ew( char *path1, char *path2 )
{
    int  rc;
    char path3[MAXPATHSTR*2];

    if( strlen(path1)+strlen(path2)+1 >= sizeof(path3) ) {
		log( "et", "path buffer overflow in rename_ew\n" );
		return( -1 );
    }
    strcpy( path3, path2 );
    strcat( path3, "/" );
    strcat( path3, path1 );
    rc = rename( path1, path3 );
    if ( rc == -1 ) perror( "rename" );
    return rc;
}

