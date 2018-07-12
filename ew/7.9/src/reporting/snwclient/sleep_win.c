
/********************************************************************
 *                 sleep_ew.c    for   Windows NT                   *
 ********************************************************************/

#include <windows.h>

void sleep_ew( unsigned milliseconds )
{
   Sleep( (DWORD) milliseconds );
   return;
}
