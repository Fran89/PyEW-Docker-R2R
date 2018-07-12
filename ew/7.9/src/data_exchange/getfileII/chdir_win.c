/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: chdir_win.c 1157 2002-12-20 02:41:38Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2002/12/20 02:39:11  lombard
 *     Initial revision
 *
 *
 *
 */

/************************************************************
 *            chdir_win.c  Windows NT version               *
 ************************************************************/

#include <windows.h>
#include "getfileII.h"


/*******************************************************
 *  chdir_ew( )  changes current working directory     *
 *                                                     *
 *  Returns 0 if all went well                         *
 *          -1 if an error occurred                    *
 *******************************************************/

int chdir_ew( char *path )
{
    BOOL success;

    success = SetCurrentDirectory( path );

    if ( success )
	return 0;
    else
	return -1;
}

