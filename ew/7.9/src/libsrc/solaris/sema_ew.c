
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sema_ew.c 5027 2012-09-19 11:58:33Z quintiliani $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2004/04/12 22:30:03  dietz
 *     included stdlib.h
 *
 *     Revision 1.2  2002/02/18 18:38:11  davidk
 *     initialized the dummy pointer to NULL on all of the various CreateMutex type calls
 *     that require a dummy pointer param.  The dummy pointer was previously uninitialized.
 *     It doesn't seem to have any affect since all documentation says the dummy pointer
 *     is ignored, but purify complained, so I fixed it.
 *
 *     Revision 1.1  2000/02/14 18:46:17  lucky
 *     Initial revision
 *
 *
 */

/*
 * sema_ew.c  Solaris version
 *
 * This source file contains system-dependent functions for
 * handling event semaphores and mutexes
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <synch.h>

static sema_t   semaphore;  /* Event Semaphore                        */
static mutex_t  mutex;      /* Mutual Exclusion semaphore lock        */


/************************ CreateSemaphore_ew ************************
    Create a semaphore which is posted when some event is complete.
*********************************************************************/
void CreateSemaphore_ew( void )
{
   static unsigned int count = 0;
   void *dummy = NULL;
   int   rc;

   rc = sema_init( &semaphore, count, USYNC_THREAD, dummy );
   if ( rc != 0 )
   {
      fprintf( stderr,
              "CreateSemaphore_ew: Error from sema_init: %d; Exiting.\n", rc );
      exit( -1 );
   }
   return;
}

/**************************** PostSemaphore ****************************
             Let another thread know that a message is ready.
************************************************************************/
void PostSemaphore( void )
{
   int rc;

   rc = sema_post( &semaphore );
   if ( rc != 0 )
      fprintf( stderr,
              "PostSemaphore: Error from sema_post: %d\n", rc );
   return;
}

/**************************** WaitSemPost ***************************
      Wait for the event semaphore to be posted by another thread.
*********************************************************************/
void WaitSemPost( void )
{
   int rc;

   rc = sema_wait( &semaphore );
   if ( rc != 0 )
   {
      fprintf( stderr,
              "WaitSemPost: Error from sema_wait: %d; Exiting.\n", rc );
      exit( -1 );
   }
   return;
}

/**************************** DestroySemaphore *************************
                         Kill the event semaphore.
************************************************************************/
void DestroySemaphore( void )
{
   sema_destroy( &semaphore );
   return;
}


/************************ CreateSpecificSemaphore_ew ****************
    Create a semaphore which is posted when some event is complete.
*********************************************************************/
sema_t *CreateSpecificSemaphore_ew( unsigned int count )
{
   void *dummy = NULL;
   int   rc;

   sema_t *ms = NULL;

   /* Allocate ms */
   ms = (sema_t *) malloc(sizeof(sema_t));
   if(ms == NULL) {
     fprintf( stderr,
	 "CreateSpecificSemaphore_ew: Error allocating sema_t; Exiting.\n");
     exit( -1 );
   }

   rc = sema_init( ms, count, USYNC_THREAD, dummy );
   if ( rc != 0 )
   {
      fprintf( stderr,
              "CreateSpecificSemaphore_ew: Error from sema_init: %d; Exiting.\n", rc );
      exit( -1 );
   }

   return ms;
}

/**************************** PostSpecificSemaphore_ew ********************
             Let another thread know that a message is ready.
************************************************************************/
void PostSpecificSemaphore_ew( sema_t *ms )
{
   int rc;

   rc = sema_post( ms );
   if ( rc != 0 )
      fprintf( stderr,
              "PostSpecificSemaphore_ew: Error from sema_post: %d\n", rc );
   return;
}

/**************************** WaitSpecificSemaphore_ew *******************
      Wait for the event semaphore to be posted by another thread.
*********************************************************************/
void WaitSpecificSemaphore_ew( sema_t *ms )
{
   int rc;

   rc = sema_wait( ms );
   if ( rc != 0 )
   {
      fprintf( stderr,
              "WaitSpecificSemaphore_ew: Error from sema_wait: %d; Exiting.\n", rc );
      exit( -1 );
   }
   return;
}

/**************************** DestroySpecificSemaphore_ew *****************
                         Kill the event semaphore.
************************************************************************/
void DestroySpecificSemaphore_ew( sema_t *ms )
{
  if(ms) {

    sema_destroy( ms );

    /* Deallocate ms */
    free(ms);
  }
}


/************************** CreateMutex_ew *************************
     Set up mutex semaphore to arbitrate the use of some variable
        by different threads.
     Mutex is created "private" and "unowned"
********************************************************************/
void CreateMutex_ew( void )
{
   void *dummy = NULL;
   int   rc;

   rc = mutex_init( &mutex, USYNC_THREAD, dummy );
   if ( rc != 0 )
   {
      fprintf( stderr,
              "CreateMutex_ew: Error from mutex_init: %d; Exiting.\n",
               rc );
      exit( -1 );
   }
   return;
}


/*************************** RequestMutex **************************
                       Grab the mutex semaphore
********************************************************************/
void RequestMutex( void )
{
   int   rc;

   rc = mutex_lock( &mutex );
   if ( rc != 0 )
   {
      fprintf( stderr,
              "RequestMutex: Error from mutex_lock: %d; Exiting.\n",
               rc );
      exit( -1 );
   }
   return;
}


/************************ ReleaseMutex_ew **************************
                    Release the mutex semaphore
********************************************************************/
void ReleaseMutex_ew( void )
{
   int   rc;

   rc = mutex_unlock( &mutex );
   if ( rc != 0 )
   {
      fprintf( stderr,
              "ReleaseMutex_ew: Error from mutex_unlock: %d; Exiting.\n",
               rc );
      exit( -1 );
   }
   return;
}


/***************************** CloseMutex **************************
                        Destroy mutex semaphore
********************************************************************/
void CloseMutex( void )
{
   int   rc;

   rc = mutex_destroy( &mutex );
   if ( rc != 0 )
   {
      fprintf( stderr,
              "CloseMutex: Error from mutex_destroy: %d; Exiting.\n",
               rc );
      exit( -1 );
   }
   return;
}

/************************ CreateSpecificMutex **********************
        As Above, but allows many to be created. Story: The original
        routines were written presuming that only one mutex would
        be used. These routines have become imbedded in much code.
        The ...Specific... versions are created for wave_server2.
        Alex 1/19/97
********************************************************************/
void CreateSpecificMutex( mutex_t* mp )
{
   void *dummy = NULL;
   int   rc;

   rc = mutex_init( mp, USYNC_THREAD, dummy );
   if ( rc != 0 )
   {
      fprintf( stderr,
              "CreateSpecificMutex_ew: Error from mutex_init: %d; Exiting.\n",
               rc );
      exit( -1 );
   }
   return;
}


/********************* RequestSpecificMutex ************************
                     Grab the mutex semaphore
********************************************************************/
void RequestSpecificMutex( mutex_t* mp )
{
   int   rc;

   rc = mutex_lock( mp );
   if ( rc != 0 )
   {
      fprintf( stderr,
              "RequestSpecificMutex: Error from mutex_lock: %d; Exiting.\n",
               rc );
      exit( -1 );
   }
   return;
}


/******************** ReleaseSpecificMutex *************************
                  Release the mutex semaphore
********************************************************************/
void ReleaseSpecificMutex( mutex_t* mp )
{
   int   rc;

   rc = mutex_unlock( mp );
   if ( rc != 0 )
   {
      fprintf( stderr,
              "ReleaseSpecificMutex: Error from mutex_unlock: %d; Exiting.\n",
               rc );
      exit( -1 );
   }
   return;
}


/********************** CloseSpecificMutex *************************
                      Destroy mutex semaphore
********************************************************************/
void CloseSpecificMutex( mutex_t* mp )
{
   int   rc;

   rc = mutex_destroy( mp );
   if ( rc != 0 )
   {
      fprintf( stderr,
              "CloseSpecificMutex: Error from mutex_destroy: %d; Exiting.\n",
               rc );
      exit( -1 );
   }
   return;
}

