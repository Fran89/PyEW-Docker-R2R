/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sleep_win.c,v 1.1 2002/12/20 02:41:38 lombard Exp $
 *
 *    Revision history:
 *     $Log: sleep_win.c,v $
 *     Revision 1.1  2002/12/20 02:41:38  lombard
 *     Initial revision
 *
 *
 *
 */

/********************************************************************
 *                 sleep_ew.c    for   Windows NT                   *
 ********************************************************************/

#include <windows.h>
#include "sendfileII.h"

void sleep_ew( unsigned milliseconds )
{
    Sleep( (DWORD) milliseconds );
    return;
}
