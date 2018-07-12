/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: mkdir_win.c 1157 2002-12-20 02:41:38Z lombard $
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
 *   mkdir_win.c  Windows version
 */
#include <stdio.h>
#include <windows.h>
#include "makehbfile.h"


/******************************************************************
 *  mkdir_ew( )  Create a new directory                           *
 *  Returns -1 if the directory can't be created; 0 otherwise     *
 ******************************************************************/

int mkdir_ew( char *path )
{
    DWORD lastError;

    if ( CreateDirectory( path, NULL ) ) return 0;   /* Success */

    lastError = GetLastError();                      /* Failure. Get error number. */

    if ( lastError == ERROR_ALREADY_EXISTS ) return 0;

    printf( "CreateDirectory error: %d\n", lastError );
    return -1;
}

