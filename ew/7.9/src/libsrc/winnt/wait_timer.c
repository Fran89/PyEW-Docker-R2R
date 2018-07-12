
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: wait_timer.c 10 2000-02-14 18:56:41Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 18:53:30  lucky
 *     Initial revision
 *
 *
 */

     /********************************************************************
      *                wait_timer.c   for   Windows NT                   *
      *                                                                  *
      ********************************************************************/

#include <earthworm.h>


         /**********************************************************
          *                    init_wait_timer()                   *
          *                Create a new timer object               *
          *                                                        *
          *  Returns -1 if an error is detected. errorCode is set. *
          **********************************************************/

int init_wait_timer( timer_t *timerHandle, DWORD *errorCode )
{
   LPSECURITY_ATTRIBUTES securityAttributes = NULL;  // Default security attributes
   BOOL                  manualReset = FALSE;        // This is a synchronization timer
   LPCTSTR               timerName = NULL;           // The timer is unnamed

   *timerHandle = CreateWaitableTimer( securityAttributes, manualReset, timerName );
   if ( timerHandle == NULL )
   {
      *errorCode = GetLastError();
      return -1;
   }
   return 0;
}


          /***********************************************************
           *                    start_wait_timer()                   *
           *  Start the timer.                                       *
           *  lPeriod is the repeat interval in milliseconds.        *
           *                                                         *
           *  Returns -1 if an error is detected. errorCode is set.  *
           ***********************************************************/

int start_wait_timer( timer_t timerHandle, LONG lPeriod, DWORD *errorCode )
{
   LARGE_INTEGER     dueTime;                   // 100 nanosecond intervals
   PTIMERAPCROUTINE  fnComplRoutine = NULL;     // No completion routine
   LPVOID            argToComplRoutine = NULL;  // No data passed to completion routine
   BOOL              fResume = FALSE;           // No power conservation mode
   BOOL              returnValue;

   dueTime.QuadPart = (LONGLONG)-1;             // Start immediately

   returnValue = SetWaitableTimer( timerHandle, &dueTime, lPeriod,
                                   fnComplRoutine, argToComplRoutine, fResume );
   if ( returnValue == 0 )
   {
      *errorCode = GetLastError();
      return -1;
   }
   return 0;
}


         /**********************************************************
          *                      wait_timer()                      *
          *             Wait for the timer to complete             *
          *                                                        *
          *  Returns -1 if an error is detected. errorCode is set. *
          *  Returns -2 if the wait timed out                      *
          **********************************************************/

int wait_timer( timer_t timerHandle, DWORD *errorCode )
{
   DWORD milliseconds = 5000;         // Time out in five seconds
   DWORD returnValue;

   returnValue = WaitForSingleObject( timerHandle, milliseconds );

   if ( returnValue == WAIT_FAILED )
   {
      *errorCode = GetLastError();
      return -1;
   }

   if ( returnValue == WAIT_TIMEOUT )
      return -2;

   return 0;
}
