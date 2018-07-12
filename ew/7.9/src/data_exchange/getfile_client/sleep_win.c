/********************************************************************
 *                 sleep_ew.c    for   Windows NT                   *
 ********************************************************************/

#include <windows.h>
#include "getfile_cl.h"

void sleep_ew( unsigned milliseconds )
{
    Sleep( (DWORD) milliseconds );
    return;
}
