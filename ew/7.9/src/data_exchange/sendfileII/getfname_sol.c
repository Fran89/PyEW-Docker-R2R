/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: getfname_sol.c,v 1.1 2002/12/20 02:41:38 lombard Exp $
 *
 *    Revision history:
 *     $Log: getfname_sol.c,v $
 *     Revision 1.1  2002/12/20 02:41:38  lombard
 *     Initial revision
 *
 *
 *
 */


/***************************************************************
 *                        getfname_sol.c                       *
 *                                                             *
 *  Function to get the name of a file in the current          *
 *  directory.                                                 *
 *                                                             *
 *  Returns 0 if all ok                                        *
 *          1 if no files were found                           *
 ***************************************************************/

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "sendfileII.h"

int GetFileName( char fname[] )
{
    DIR         *dp;
    struct stat ss;

    dp = opendir( "." );
    if ( dp == NULL ) return 2;

    do
    {
	struct dirent *dentp = readdir( dp );
	if ( dentp == NULL )
	{
	    closedir( dp );
	    return 1;
	}
	strcpy( fname, dentp->d_name );

	if ( stat( fname, &ss ) == -1 )
	{
	    closedir( dp );
	    return 1;
	}
    } while ( !S_ISREG(ss.st_mode) );

    closedir( dp );
    return 0;
}
