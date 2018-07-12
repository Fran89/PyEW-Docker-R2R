/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: rename_win.c 1157 2002-12-20 02:41:38Z lombard $
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
 *   rename_win.c
 */
#include <windows.h>
#include <string.h>
#include "makehbfile.h"


/**********************************************************************
 *  rename_ew( )  Moves a file                   Windows version      *
 *  path1 = file to be moved.  path2 = destination directory          *
 *  Returns -1 if an error occurred; 0 otherwise                      *
 **********************************************************************/

int rename_ew( char *path1, char *path2 )
{
    char path3[80];

    strcpy( path3, path2 );
    strcat( path3, "\\" );
    strcat( path3, path1 );
    if ( MoveFileEx( path1, path3, MOVEFILE_REPLACE_EXISTING ) == 0 )
	return -1;

    return 0;
}

