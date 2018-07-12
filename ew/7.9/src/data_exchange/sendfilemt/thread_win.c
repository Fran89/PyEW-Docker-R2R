
#include <windows.h>
#include <process.h>
#include "sendfilemt.h"



   /**************************************************************
    *                ThreadAlive - Windows Version               *
    *          Checks whether specified thread is alive.         *
    *                                                            *
    *  Returns THREAD_ALIVE if a thread with id "tid" is alive.  *
    *  Returns THREAD_DEAD otherwise.                            *
    **************************************************************/

int ThreadAlive( unsigned int tid )
{
   DWORD exitCode = 0;

/* If thread's exit code is STILL_ACTIVE,
   then the thread is alive.
   *************************************/
   if( GetExitCodeThread((HANDLE)tid, &exitCode) )
   {
      int alive = (exitCode == STILL_ACTIVE) ? 1 : 0;
      return alive;
   }
   return 0;
}

