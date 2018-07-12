/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: tzset_sol.c 1157 2002-12-20 02:41:38Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2002/12/20 02:39:11  lombard
 *     Initial revision
 *
 *
 *
 */


/*************************************************************************
 *                             tzset_sol.c                               *
 *                                                                       *
 *       Under Windows, it's necessary to set TZ for the gmtime()        *
 *       function to work correctly.  Solaris gets TZ from the           *
 *       global environment, so this function is a dummy.                *
 *************************************************************************/

#include "getfileII.h"

void TzSet( char *tz )
{
    return;
}
