
#include <signal.h>
#include <pthread.h>
#include "sendfilemt.h"


   /**************************************************************
    *                ThreadAlive - Solaris Version               *
    *        Checks whether a specified thread is alive.         *
    *                                                            *
    *  Returns THREAD_ALIVE if a thread with id "tid" is alive.  *
    *  Returns THREAD_DEAD otherwise.                            *
    *                                                            *
    *  Beware:  If a thread dies and a new thread is created,    *
    *  the operating system might assign the old thread id to    *
    *  the newly created thread.  So, if threads are destroyed   *
    *  and created, ThreadAlive doesn't guarantee the original   *
    *  thread is alive.                                          *
    **************************************************************/

int ThreadAlive( unsigned int tid )
{
   return (pthread_kill((pthread_t) tid, 0) == 0) ? THREAD_ALIVE : THREAD_DEAD;
}

