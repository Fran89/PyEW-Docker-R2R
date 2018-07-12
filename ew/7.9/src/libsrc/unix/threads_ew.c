#define _XOPEN_SOURCE 500

              /***********************************************
               *                threads_ew.c                 *
               *                UNIX version                 *
               *                                             *
               *  This file contains functions StartThread,  *
               *  WaitThread, and KillThread                 *
               ***********************************************/

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#ifdef _USE_PTHREADS
#include <unistd.h>
#include <pthread.h>
#include <sys/param.h>
#else /* Solaris threads */
# include <thread.h>
# define pthread_t thread_t
# define pthread_kill thr_kill
# define pthread_exit thr_exit
# define pthread_self thr_self
#endif

#ifdef _USE_POSIX_SIGNALS
void SignalHandle(int, siginfo_t *, void *);
#else
void SignalHandle( int );
#endif

#ifdef _USE_PTHREADS
int setStackSize(char *function_text, pthread_attr_t *pattr, int stack_size);
#endif

   /********************************************************************
    *                           StartThread                            *
    *                                                                  *
    * Arguments:                                                       *
    *     fun:        Name of thread function. Must take (void *)      *
    *                 as an argument and return void                   *
    *     stack_size: Stack size of new thread in bytes                *
    *                 In OS2, if zero the stack size is set to 8192    *
    *                 In SOLARIS, this argument is ignored             *
    *     thread_id:  Thread identification number returned to         *
    *                 calling program.                                 *
    * Returns:                                                         *
    *    -1 if error                                                   *
    *     0 if ok                                                      *
    ********************************************************************/

int StartThread( void *fun(void *), unsigned stack_size, unsigned *thread_id )
{
   int error;                     /* Function return code */
   pthread_t tid;                 /* SendMsg thread id */
#ifdef _USE_PTHREADS
   pthread_attr_t attr;
   int cclev;
#else
   size_t stackSize = 0;
#endif

/* Set up a signal-handling function to be inherited by threads
   ************************************************************/
#ifdef _USE_POSIX_SIGNALS
    struct sigaction act;
    act.sa_flags = SA_SIGINFO; sigemptyset(&act.sa_mask);
    act.sa_sigaction = SignalHandle;
    sigaction(SIGUSR1, &act, (struct sigaction *)NULL);
#else
   sigset( SIGUSR1, &SignalHandle );
#endif 
 
/* Start the thread
   ****************/

#ifdef _USE_PTHREADS
   /* initialize attribute to defaults (stack_size = implementation dependent,
	detach state = PTHREAD_CREATE_JOINABLE, scope = PTHREAD_SCOPE_PROCESS,
	concurrency = adjusted by kernel) */
   if ( (error = pthread_attr_init(&attr)) != 0 ) {
	fprintf(stderr,"StartThread: pthread_attr_init error: %s\n",strerror(error));
	return (-1);
   }
   /* set stack size within allowable limits */
   if (setStackSize("StartThread", &attr, stack_size) != 0)
   {
	return (-1);
   }
   /* set automatic detachment of thread upon exit */
   if ( (error = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0 ) {
	fprintf(stderr,"StartThread: pthread_attr_setdetachstate error: %s\n",strerror(error));
	return (-1);
   }
   if ( (error = pthread_create( &tid, &attr, fun, (void *)NULL )) != 0 ) {
      fprintf(stderr,"StartThread: pthread_create error: %s\n",strerror(error));
      return( -1 );
   }
   /* Concurrency is an X/Open extension to POSIX.1c and may not be implemented by
      all vendors.  Concurrency levels are a hint, and need not be obeyed.  Only
      PTHREAD_SCOPE_PROCESS threads are affected.  By default, the number of kernel
      threads is dynamically adjusted as a process executes.  If the existing
      concurrency level is zero, that means the system is using its default
      concurrency scheme, and we leave it alone.  Otherwise, we bump it up by one  */
   if ( (cclev = pthread_getconcurrency()) > 0 )
	if ( (error = pthread_setconcurrency(++cclev)) != 0 ) {
	    fprintf(stderr,"StartThread: pthread_setconcurrency error: %s\n",strerror(error));
	    return (-1);
	}
#else
   /* Note: THR_DETACHED is required for thr_exit to work. That is,
      a detached thread can truly kill itself without lingering in
      some afterlife, waiting for some other thread to pick up it's exit
      status before it can truly cease to be...*/
   error = thr_create( (void *)0, stackSize, fun, (void *)0,
                    THR_DETACHED|THR_NEW_LWP, &tid );
   if ( error != 0 ) {
      fprintf(stderr,"StartThread: thr_create error: %s\n",strerror(error));
      return( -1 );
   }
#endif



   *thread_id = (unsigned)tid;
   return( 0 );
}


   /********************************************************************
    *                        StartThreadWithArg                        *
    *                                                                  *
    * Arguments:                                                       *
    *     fun:        Name of thread function. Must take (void *)      *
    *                 as an argument and return void.                  *
    *	  arg:	      an unsigned long (void*), passed to the thread   *
    *     stack_size: Stack size of new thread in bytes                *
    *                 In OS2, if zero the stack size is set to 8192    *
    *                 In SOLARIS, this argument is ignored             *
    *     thread_id:  Thread identification number returned to         *
    *                 calling program.                                 *
    * Returns:                                                         *
    *    -1 if error                                                   *
    *     0 if ok                                                      *
    ********************************************************************/

int StartThreadWithArg( void *fun(void *),void* arg, unsigned stack_size, 
			unsigned *thread_id )
{
   int error;                    /* Function return code */
   pthread_t tid;                /* SendMsg thread id */
#ifdef _USE_PTHREADS
   pthread_attr_t attr;
   int cclev;
#else
   size_t stackSize = 0;
#endif

/* Set up a signal-handling function to be inherited by threads
   ************************************************************/
#ifdef _USE_POSIX_SIGNALS
   struct sigaction act;
   act.sa_flags = SA_SIGINFO; sigemptyset(&act.sa_mask);
   act.sa_sigaction = SignalHandle;
   sigaction(SIGUSR1, &act, (struct sigaction *)NULL);
#else
   sigset( SIGUSR1, &SignalHandle );
#endif
 
/* Start the thread
   ****************/
#ifdef _USE_PTHREADS
   if ( (error = pthread_attr_init(&attr)) != 0 ) {
	fprintf(stderr,"StartThreadWithArg: pthread_attr_init error: %s\n",strerror(error));
	return (-1);
   }
   /* set stack size within allowable limits */
   if (setStackSize("StartThread", &attr, stack_size) != 0)
   {
	return (-1);
   }
   /* set automatic detachment of thread upon exit */
   if ( (error = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0 ) {
	fprintf(stderr,"StartThreadWithArg: pthread_attr_setdetachstate error: %s\n",strerror(error));
	return (-1);
   }
   if ( (error = pthread_create( &tid, &attr, fun, (void *)arg )) != 0 ) {
      fprintf(stderr,"StartThreadWithArg: pthread_create error: %s\n",strerror(error));
      return( -1 );
   }
   /* Concurrency is an X/Open extension to POSIX.1c and may not be implemented by
      all vendors.  Concurrency levels are a hint, and need not be obeyed.  Only
      PTHREAD_SCOPE_PROCESS threads are affected.  By default, the number of kernel
      threads is dynamically adjusted as a process executes.  If the existing
      concurrency level is zero, that means the system is using its default
      concurrency scheme, and we leave it alone.  Otherwise, we bump it up by one  */
   if ( (cclev = pthread_getconcurrency()) > 0 )
	if ( (error = pthread_setconcurrency(++cclev)) != 0 ) {
	    fprintf(stderr,"StartThreadWithArg: pthread_setconcurrency error: %s\n",strerror(error));
	    return (-1);
	}
#else
   /* Note: THR_DETACHED is required for thr_exit to work. That is,
      a detached thread can truly kill itself without lingering in
      some afterlife, waiting for some other thread to pick up it's exit
      status before it can truly cease to be...*/
   error = thr_create( (void *)0, stackSize, fun, (void *)arg,
                    THR_DETACHED|THR_NEW_LWP, &tid );
   if ( error != 0 ) {
      fprintf(stderr,"StartThreadWithArg: thr_create error: %s\n",strerror(error));
      return( -1 );
   }
#endif

   *thread_id = (unsigned)tid;
   return( 0 );
}


  /*************************************************************
   *                          WaitThread                       *
   *                    Wait for thread to die.                *
   *                                                           *
   *             This is a dummy function in Unix.             *
   *                                                           *
   * Argument:                                                 *
   *    thread_id = Pointer to thread id                       *
   *************************************************************/

void WaitThread( unsigned *thread_id )
{
}


   /*************************************************************
    *                        KillThread                         *
    *                Force a thread to exit now.                *
    *                                                           *
    * Argument:                                                 *
    *    tid = id of thread to kill                             *
    *                                                           *
    * Returns:                                                  *
    *     0 if ok                                               *
    *     non-zero value indicates an error                     *
    *************************************************************/

int KillThread( unsigned int tid )
{
   return( pthread_kill( (pthread_t) tid, SIGUSR1 ) );
}



   /***************************************************************
    *                         KillSelfThread                      *
    *     For a thread exit without affecting other threads       *
    *                                                             *
    *      Thread must have been created with the THR_DETACHED    *
    *      bit set; else a zombie lingers, waiting for someone    * 
    *      to pick up it's exit status                            *
    ***************************************************************/

int KillSelfThread(void)
{
   pthread_exit((void *)NULL );
   return 0;                     /* well, not really */
}



   /*************************************************************
    *                        SignalHandle                       *
    *         Decide what to do when a signal is caught         *  
    *  Added for use with the KillThread function so that       *
    *  killed threads will exit gracefully                      *
    *************************************************************/

#ifdef _USE_POSIX_SIGNALS
void SignalHandle(int sig, siginfo_t *sip, void *up)
#else
void SignalHandle( int sig )
#endif
{
   int status = 0;
 
   switch (sig)
   {
   case SIGUSR1:
        /*printf( "thread:%d caught SIGUSR1; calling pthread_exit()\n",
                 (int) pthread_self() );*/ /*DEBUG*/
        pthread_exit( &status );
   }
}

#ifdef _USE_PTHREADS

int setStackSize(char *function_text, pthread_attr_t *pattr, int stack_size)
{
   int error;                     /* Function return code */
   /* set stack size within allowable limits */
   if ( stack_size ) {
        size_t cur_stack_size;
        /* if we could not get the current stack size */
        if ((error = pthread_attr_getstacksize(pattr, &cur_stack_size)) != 0)
        {
               fprintf(stderr,"%s: pthread_attr_getstacksize error: %s\n", function_text, strerror(error));
               return (-1);
        }

	/*
        fprintf(stdout, "DEBUG:  stack size is %d", cur_stack_size);
	*/

        /* the current stack size is less than the new stack size */
        if (cur_stack_size < stack_size)
        {

	/*
           fprintf(stdout, ", new stack size will be %d", stack_size);
	*/

           if ( (error = (pthread_attr_setstacksize(pattr, stack_size)) != 0) ) {
               fprintf(stdout,"%s: pthread_attr_setstacksize (%d) error: %s\n", function_text, stack_size, strerror(error));
               return (-1);
           }
        }

	/*
        fprintf(stdout, "\n");
	*/
   }
   return (0);
}

#endif
