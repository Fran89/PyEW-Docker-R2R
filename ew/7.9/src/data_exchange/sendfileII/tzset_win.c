/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: tzset_win.c,v 1.1 2002/12/20 02:41:38 lombard Exp $
 *
 *    Revision history:
 *     $Log: tzset_win.c,v $
 *     Revision 1.1  2002/12/20 02:41:38  lombard
 *     Initial revision
 *
 *
 *
 */

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include "sendfileII.h"


/*************************************************************************
 *                             tzset_win.c                               *
 *                                                                       *
 *       Set the time zone environmental variable (TZ).                  *
 *       This is required for the gmtime() function to work              *
 *       correctly.                                                      *
 *************************************************************************/

void TzSet( char *tz )
{
    static char envstring[40];

    strcpy( envstring, "TZ=" );
    strcat( envstring, tz );

    if ( _putenv( envstring ) == -1 )
    {
	log( "et", "Error: Can't set environmental variable TZ. Exiting.\n" );
	exit( -1 );
    }
    _tzset();
    return;
}
