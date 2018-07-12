/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: tzset_sol.c,v 1.1 2002/12/20 02:41:38 lombard Exp $
 *
 *    Revision history:
 *     $Log: tzset_sol.c,v $
 *     Revision 1.1  2002/12/20 02:41:38  lombard
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

#include "sendfileII.h"

void TzSet( char *tz )
{
    return;
}
