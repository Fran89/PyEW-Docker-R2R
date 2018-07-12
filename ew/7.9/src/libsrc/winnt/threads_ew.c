
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: threads_ew.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2004/10/19 22:42:22  mark
 *     Removed thread priority functions (not ready for prime time yet...)
 *
 *     Revision 1.2  2004/10/07 21:32:15  mark
 *     Added thread priority functions
 *
 *     Revision 1.1  2000/02/14 18:53:30  lucky
 *     Initial revision
 *
 *
 */

              /********************************************
              *               threads_ew.c                *
              *            Windows NT version             *
              * This file contains functions StartThread, *
              * WaitThread, and KillThread                *
              *********************************************/

#include <windows.h>
#include <process.h>


   /********************************************************************
    *                           StartThread                            *
    *                                                                  *
    *  Arguments:                                                      *
    *     fun:        Name of thread function. Must take (void *)      *
    *                 as an argument and return void                   *
    *     stack_size: Stack size of new thread in bytes                *
    *                 If 0, stack size is set to 8192.                 *
    *                 In OS2, 4096 or 8192 is recommended.             *
    *                 In SOLARIS, this argument is ignored             *
    *                 In Windows NT, if stack_size=0, use the stack    *
    *                 size of the calling thread.                      *
    *     thread_id:  Thread identification number returned to         *
    *                 calling program.                                 *
    *                                                                  *
    *  The function <fun> is not passed any arguments.                 *
    *                                                                  *
    *  Returns:                                                        *
    *    -1 if error                                                   *
    *     0 if ok                                                      *
    ********************************************************************/

int StartThread( void fun(void *), unsigned stack_size, unsigned *thread_id )
{
   uintptr_t tid;

   tid = _beginthread( fun, stack_size, NULL );

   if ( tid == -1 )                /* Couldn't create thread */
      return -1;

   *thread_id = (unsigned)tid;     /* Return the thread id */
   return 0;
}

   /********************************************************************
    *                       StartThreadWithArg                         *
    *                                                                  *
    *  Arguments:                                                      *
    *     fun:        Name of thread function. Must take (void *)      *
    *                 as an argument and return void                   *
    *     arg:        an unsigned long (void*) passed to the thread.   *
    *     stack_size: Stack size of new thread in bytes                *
    *                 If 0, stack size is set to 8192.                 *
    *                 In OS2, 4096 or 8192 is recommended.             *
    *                 In SOLARIS, this argument is ignored             *
    *                 In Windows NT, if stack_size=0, use the stack    *
    *                 size of the calling thread.                      *
    *     thread_id:  Thread identification number returned to         *
    *                 calling program.                                 *
    *                                                                  *
    *  Returns:                                                        *
    *    -1 if error                                                   *
    *     0 if ok                                                      *
    ********************************************************************/

int StartThreadWithArg( void fun(void *), void* arg, unsigned stack_size, 
                        unsigned *thread_id )
{
   uintptr_t tid;

   tid = _beginthread( fun, stack_size, arg );

   if ( tid == -1 )                /* Couldn't create thread */
      return -1;

   *thread_id = (unsigned)tid;     /* Return the thread id */
   return 0;
}


   /*************************************************************
    *                        KillSelfThread                     *
    *             For a thread exit without affecting           *
    *                        other threads                      *
    *************************************************************/

int KillSelfThread( void )
{
    _endthread();
    return 0;
}


  /*************************************************************
   *                          WaitThread                       *
   *                   Wait for thread to die.                 *
   *                                                           *
   *  This function is used in coaxtoring.c                    *
   *                                                           *
   *    thread_id = Pointer to thread id                       *
   *                                                           *
   *  Returns:                                                 *
   *    -1 if error                                            *
   *     0 if ok                                               *
   *************************************************************/

int WaitThread( unsigned *thread_id )
{
   if ( WaitForSingleObject( (HANDLE)(*thread_id), INFINITE )
                                             == WAIT_FAILED )
      return -1;
   return 0;
}


   /************************************************************
    *                         KillThread                       *
    *                Force a thread to exit now!               *
    *                                                          *
    *  Windows NT documentation gives a strong warning against *
    *  using TerminateThread(), since no stack cleanup, etc,   *
    *  is done.                                                *
    *                                                          *
    * Argument:                                                *
    *    tid = id of thread to kill                            *
    *                                                          *
    * Returns:                                                 *
    *     0 if ok                                              *
    *     non-zero value indicates an error                    *
    ************************************************************/

int KillThread( unsigned int tid )
{
    const DWORD exit_code = 0;

    if ( TerminateThread( (HANDLE)tid, exit_code ) == 0 )
       return -1;

    return 0;
}

