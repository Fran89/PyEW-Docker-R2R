/********************************************
*              worm_threads.c               *
*            Windows NT version             *
*                                           *
* This file contains functions StartThread, *
* WaitThread, and KillThread                *
*********************************************/
//---------------------------------------------------------------------------
#include "threadableobject.h"

//---------------------------------------------------------------------------
#ifdef _SOLARIS
#include <signal.h>
#endif

#pragma package(smart_init)

TMutex * ThreadableObject::ThatMutex = NULL;
ThreadableObject * ThreadableObject::That = NULL;

//---------------------------------------------------------------------------
#ifdef _SOLARIS
/*
** SignalHandler() -- Solaris-specific signal handler
**
**  Added for use with the KillThread function so that
**  killed threads will exit gracefully
*/
static void ThreadableSigHandler( int p_sig )
{
   void * status;

   switch (p_sig)
   {
      case SIGUSR1:
           // DEBUG
           // printf( "thread %d caught SIGUSR1; calling thr_exit()\n"
           //       , (int)thr_self()
           //       );
           thr_exit( status );
   }
}
#endif

//---------------------------------------------------------------------------
// StartThreadableObject() -- this is a helper function, used only in this file,
//                            that has the signature type required by the
//                            _beginthread() function.
//
//                            It is important that sufficient time between
//                            calls to this function be allowed to prevent
//                            conflicts with the class-scope variable
//                            'That' (accessed with ::GetThat).
//                            Generally, sleeping 1/4 - 1/2 second after
//                            calls to StartThread()
//                            or StartThreadWithArg() will suffice.
//
static void StartObjectThread( void * p_argument )
{
   if ( ThreadableObject::GetThat() != NULL )
   {
      ThreadableObject::GetThat()->StartThreadFunc(p_argument);
   }
}

//---------------------------------------------------------------------------
WORM_STATUS_CODE ThreadableObject::StartThreadWithArg( TO_STACK_SIZE   p_stack_size
                                                     , TO_THREAD_ID  * thread_id
                                                     , void *          p_parameters
#ifdef _SOLARIS
                                                     , bool            p_isdaemon = false
#endif
                                                     )
{
#if defined(_WINNT) || defined(_Windows)
   long tid = -1;

   // Use mutex to keep global "That" from being clobbered.
   ThatMutex->RequestLock();
   That = this;
   tid = _beginthread( StartObjectThread, p_stack_size, p_parameters );
   ThatMutex->ReleaseLock();

   if ( tid == -1 )
   {
      // Couldn't create thread
      return WORM_STAT_FAILURE;
   }

#elif defined(_SOLARIS)

   int rc;              // Function return code
   TO_THREAD_ID tid;     // working thread id

   // Set up a signal-handling function to be inherited by threads
   // used by the KillThread() method
   sigset( SIGUSR1, &ThreadableSigHandler );

   // Start the thread
   //
   // Note: THR_DETACHED is required for thr_exit to work. That is,
   //   a detached thread can truly kill itself without lingering in
   //   some afterlife, waiting for some other thread to pick up it's exit
   //   status before it can truly cease to be...
   //
   // Use mutex to keep global "That" from being clobbered.
   ThatMutex->RequestLock();
   That = this;
   rc = thr_create( (void *)0
                  , p_stack_size
                  , StartObjectThread
                  , p_parameters
                  , THR_DETACHED|THR_NEW_LWP|(p_isdaemon ? THR_DAEMON : 0)
                  , &tid
                  );
   ThatMutex->ReleaseLock();

   if ( rc != 0 )
   {
      // Couldn't create thread
      return WORM_STAT_FAILURE;
   }
#endif

   *thread_id = (TO_THREAD_ID)tid;     /* Return the thread id */

   return WORM_STAT_SUCCESS;
}
//---------------------------------------------------------------------------
void ThreadableObject::KillSelfThread( void )
{
#if defined(_WINNT) || defined(_Windows)
    _endthread();
#elif defined(_SOLARIS)
   thr_exit( (void *)NULL );
#endif
}
//---------------------------------------------------------------------------
int ThreadableObject::WaitForThread( TO_THREAD_ID * thread_id )
{
#if defined(_WINNT) || defined(_Windows)
   if ( WaitForSingleObject((HANDLE)(*thread_id), INFINITE ) == WAIT_FAILED )
   {
      return WORM_STAT_FAILURE;
   }
#elif defined(_SOLARIS)
   // this is unimplemented within Solaris
#endif
   return WORM_STAT_SUCCESS;
}
//---------------------------------------------------------------------------
int ThreadableObject::KillThread( TO_THREAD_ID tid )
{
#if defined(_WINNT) || defined(_Windows)
    const DWORD exit_code = 0;

    if ( TerminateThread( (HANDLE)tid, exit_code ) == 0 )
    {
       return WORM_STAT_FAILURE;
    }
    return WORM_STAT_SUCCESS;
#elif defined(_SOLARIS)
   return( thr_kill( (thread_t) tid, SIGUSR1 ) );
#endif
}
//---------------------------------------------------------------------------
