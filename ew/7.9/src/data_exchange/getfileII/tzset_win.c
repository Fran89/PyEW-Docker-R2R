/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: tzset_win.c 5092 2012-10-25 05:37:20Z cochrane $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2002/12/20 02:39:11  lombard
 *     Initial revision
 *
 *
 *
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "getfileII.h"


/*********************************************************************
 *                           tzset_win.c                             *
 *                                                                   *
 *     Set the time zone environmental variable (TZ).                *
 *     This is required for the gmtime() function to work            *
 *     correctly.                                                    *
 *********************************************************************/

void TzSet( char *tz )
{
    static char envstring[40];

    strcpy( envstring, "TZ=" );
    strcat( envstring, tz );

    if ( _putenv( envstring ) == -1 )
    {
	my_log( "et", "Error: Can't set environmental variable TZ. Exiting.\n" );
	exit( -1 );
    }
    _tzset();
    return;
}
