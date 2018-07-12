/*
 *   rename_win.c
 */
#include <windows.h>
#include <string.h>
#include "getfile_cl.h"

/**********************************************************************
 *  rename_ew( )  Moves a file                   Windows version      *
 *  path1 = file to be moved.  path2 = destination directory          *
 *  Returns -1 if an error occurred; 0 otherwise                      *
 **********************************************************************/

int rename_ew( char *path1, char *path2 )
{
    char path3[MAXPATHSTR*2];

    if( strlen(path1)+strlen(path2)+1 >= sizeof(path3) ) {
		log( "et", "path buffer overflow in rename_ew\n" );
		return -1;
    }
    strcpy( path3, path2 );
    strcat( path3, "\\" );
    strcat( path3, path1 );
    if ( MoveFileEx( path1, path3, MOVEFILE_REPLACE_EXISTING ) == 0 )
		return -1;

    return 0;
}

