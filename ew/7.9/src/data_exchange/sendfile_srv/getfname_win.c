/***************************************************************
 *                        getfname_win.c                       *
 *                                                             *
 *  Function to get the name of a file in directory "Path".    *
 *                                                             *
 *  Returns 0 if all ok                                        *
 *          1 if no files were found                           *
 ***************************************************************/

#include <stdio.h>
#include <windows.h>
#include "sendfile_srv.h"


int GetFileName( char fname[] )
{
    extern char Path[80];     /* Directory containing files to be sent */
    char            fileMask[] = "*";
    WIN32_FIND_DATA findData;
    HANDLE          fileHandle;
    FILE            *fp;

    strcpy( fileMask, "*" );

    /* Get the name of the first file.
       The file may be a directory or a partially-written file.
       If so, skip this file and look for others.
    *******************************************************/
    fileHandle = FindFirstFile( fileMask, &findData );
    if ( fileHandle == INVALID_HANDLE_VALUE )         /* No files found */
	return 1;

    fp = fopen( findData.cFileName, "rb" );
    if ( fp != NULL )  {               /* File can be opened */
		fclose( fp );
		strcpy( fname, findData.cFileName );
		FindClose( fileHandle );
		return 0;
    }

    /* First file is a directory or it is otherwise unopenable.
       Find another file, if any.
    *******************************************************/
    while ( FindNextFile( fileHandle, &findData ) ) {
		fp = fopen( findData.cFileName, "rb" );
	
		if ( fp != NULL )  {               /* File can be opened */
			fclose( fp );
			strcpy( fname, findData.cFileName );       /* Found a file */
			FindClose( fileHandle );
			return 0;
		}
    }

    FindClose( fileHandle );          /* No files found */
    return 1;
}

