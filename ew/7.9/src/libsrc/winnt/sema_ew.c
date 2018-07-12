
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sema_ew.c 5685 2013-07-29 10:32:57Z quintiliani $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 18:53:30  lucky
 *     Initial revision
 *
 *
 */

           /**************************************************************
            *                sema_ew.c  Windows NT version               *
            *                                                            *
            *  This file contains system-dependent functions for         *
            *  handling semaphores and mutexes.                          *
            **************************************************************/

#include <windows.h>
#include <stdlib.h>
#include <earthworm.h>

HANDLE mutSem;            /* Mutex semaphore handle */
HANDLE semahandle;        /* Handle of event semaphore */


/************************* CreateMutex_ew **************************
 *  Set up mutex semaphore to arbitrate the use of some variable   *
 *  by different threads.  Since the mutex is unnamed, it can be   *
 *  used by only one process.                                      *
 *******************************************************************/

void CreateMutex_ew( void )
{
   mutSem = CreateMutex( 0, FALSE, 0 );
   if ( mutSem == 0 )
      logit( "et", "Error creating the mutex semaphore.\n" );
   return;
}


/************************** RequestMutex ***************************
 *  Wait until the mutex is available.  Then, grab it.             *
 *******************************************************************/

void RequestMutex( void )
{
   WaitForSingleObject( mutSem, INFINITE );      /* Wait forever */
   return;
}


/************************ ReleaseMutex_ew **************************
 *                  Release the muxtex semaphore                   *
 *******************************************************************/

void ReleaseMutex_ew( void )
{
   if ( ReleaseMutex( mutSem ) == 0 )
      logit( "et", "Error releasing the mutex semaphore.\n" );
   return;
}


/*************************** CloseMutex ****************************
 *              We are done with the mutex semaphore.              *
 *******************************************************************/

void CloseMutex( void )
{
   CloseHandle( mutSem );
   return;
}


/********************** CreateSpecificMutex ************************
 *  Set up mutex semaphore to arbitrate the use of resources.      *
 *******************************************************************/

void CreateSpecificMutex( HANDLE* mp )
{
   *mp = CreateMutex( 0, FALSE, 0 );
   if ( *mp == 0 )
      logit( "et", "Error creating specific mutex semaphore.\n" );
   return;
}


/********************** RequestSpecificMutex ***********************
 *                   Request the mutex semaphore                   *
 *******************************************************************/

void RequestSpecificMutex( HANDLE* mp )
{
   WaitForSingleObject( *mp, INFINITE );
   return;
}


/********************** ReleaseSpecificMutex ***********************
 *                   Release the mutex semaphore                   *
 *******************************************************************/

void ReleaseSpecificMutex( HANDLE* mp )
{
   if ( ReleaseMutex( *mp ) == 0 )
      logit( "et", "Error releasing specific mutex semaphore.\n" );
   return;
}


/*********************** CloseSpecificMutex ************************
 *              We are done with the mutex semaphore.              *
 *******************************************************************/

void CloseSpecificMutex( HANDLE* mp )
{
   CloseHandle( *mp );
   return;
}


/************************* CreateSemaphore_ew **********************
 *                         Create a semaphore                      *
 *  The semaphore is not posted when it is created.                *
 *******************************************************************/

void CreateSemaphore_ew( void )
{
   const LONG InitialCount = 0;                /* Initially unset */
   const LONG MaxCount     = 1000000000;       /* One billion */

   semahandle = CreateSemaphore( NULL, InitialCount, MaxCount, NULL );
   if ( semahandle == NULL )
   {
      logit( "et", "CreateSemaphore() error: %d  Exiting.\n",
             GetLastError() );
      exit( -1 );
   }
   return;
}


/*************************** PostSemaphore *************************
 *                        Post the semaphore.                      *
 *  The semaphore counter is incremented.                          *
 *******************************************************************/

void PostSemaphore( void )
{
   if ( ReleaseSemaphore( semahandle, 1, NULL ) == FALSE )
      logit( "et", "ReleaseSemaphore() error: %d\n", GetLastError() );
   return;
}


/************************** WaitSemPost *************************
 *            Wait for the semaphore to be signaled.            *
 ****************************************************************/

void WaitSemPost( void )
{
   if ( WaitForSingleObject( semahandle, INFINITE ) == WAIT_FAILED )
      logit( "et", "WaitForSingleObject() error: %d\n", GetLastError() );
   return;
}


/************************* DestroySemaphore *********************
 *                        Kill the semaphore.                   *
 ****************************************************************/

void DestroySemaphore( void )
{
   CloseHandle( semahandle );
   return;
}

/************************* CreateSpecificSemaphore_ew **************
 *                         Create a semaphore                      *
 *  The semaphore is not posted when it is created.                *
 *******************************************************************/

void CreateSpecificSemaphore_ew(HANDLE *ms, unsigned int count )
{
   LONG InitialCount = count;                  /* Initially set */
   const LONG MaxCount     = 1000000000;       /* One billion */

   *ms = CreateSemaphore( NULL, count, MaxCount, NULL );
   if ( *ms == NULL )
   {
      logit( "et", "CreateSpecificSemaphore_ew() error: %d  Exiting.\n",
             GetLastError() );
      exit( -1 );
   }

   return ;
}


/*************************** PostSpecificSemaphore_ew *****************
 *                        Post the semaphore.                      *
 *  The semaphore counter is incremented.                          *
 *******************************************************************/

void PostSpecificSemaphore_ew( HANDLE *ms )
{
   if ( ReleaseSemaphore( *ms, 1, NULL ) == FALSE )
      logit( "et", "PostSpecificSemaphore_ew() error: %d\n", GetLastError() );
   return;
}


/************************** WaitSemSpecificPost *****************
 *            Wait for the semaphore to be signaled.            *
 ****************************************************************/

void WaitSpecificSemaphore_ew( HANDLE *ms )
{
   if ( WaitForSingleObject( *ms, INFINITE ) == WAIT_FAILED )
      logit( "et", "WaitSpecificSemaphore_ew() error: %d\n", GetLastError() );
   return;
}


/************************* DestroySpecificSemaphore_ew *************
 *                        Kill the semaphore.                   *
 ****************************************************************/

void DestroySpecificSemaphore_ew( HANDLE *ms )
{
   CloseHandle( *ms );
   return;
}

