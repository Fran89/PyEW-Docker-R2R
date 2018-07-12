/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: earthworm_complex_funcs.h 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.7  2007/03/28 14:13:39  paulf
 *     minor MACOSX #ifdefs added
 *
 *     Revision 1.6  2004/10/19 22:41:18  mark
 *     Removed thread priority functions (not ready for prime time yet...)
 *
 *     Revision 1.5  2004/10/07 21:30:28  mark
 *     Added thread priority functions
 *
 *     Revision 1.4  2003/11/25 20:37:56  dietz
 *     *** empty log message ***
 *
 *     Revision 1.3  2003/11/25 00:42:59  dietz
 *     Fixed problem with StartThread prototypes. Need to use the thr_ret definition because
 *     Solaris and Windows have different function returns!
 *
 *     Revision 1.2  2003/11/11 18:06:16  mark
 *     Fixed compile bug for Visual C++ (need parameter names for StartThread functions)
 *
 *     Revision 1.1  2001/04/06 21:03:30  davidk
 *     Initial revision
 *
 *
 ************************************************************/

#ifndef EARTHWORM_COMPLEX_FUNCS_H
#define EARTHWORM_COMPLEX_FUNCS_H

/* This file contains prototypes for earthworm libsrc
   functions that require special type definitions, such as
   (semaphores, threads, mutexes, sockets, etc.).

   If you have functions that only use primitive types and you
   do not need any extra header files for them to compile, then
   you can put them into earthworm_simple_funcs.h.

   Note, please try to keep functions from the same object
   together in one section of one file.  So all of the sema_ew.c
   stuff should go together.  Thank You!
   Davidk 2001/04/06
*************************************************************/

/* System-dependent stuff goes here
   ********************************/
#include <platform.h>

void CreateSemaphore_ew( void );            /* sema_ew.c    system-dependent */
void PostSemaphore   ( void );              /* sema_ew.c    system-dependent */
void WaitSemPost     ( void );              /* sema_ew.c    system-dependent */
void DestroySemaphore( void );              /* sema_ew.c    system-dependent */

/* Use SpecificSemaphore Uppercase APIs names in order to develop cross-platform application:
 *   - DECLARE_SPECIFIC_SEMAPHORE_EW(semname)
 *   - CREATE_SPECIFIC_SEMAPHORE_EW(semname, count)
 *   - POST_SPECIFIC_SEMAPHORE_EW(semname)
 *   - WAIT_SPECIFIC_SEMAPHORE_EW(semname)
 *   - DESTROY_SPECIFIC_SEMAPHORE_EW(semname)
 */
#ifdef _WINNT
#define DECLARE_SPECIFIC_SEMAPHORE_EW(semname) sema_t semname
#else
#define DECLARE_SPECIFIC_SEMAPHORE_EW(semname) sema_t *semname
#endif

#ifdef _MACOSX
#define CREATE_SPECIFIC_SEMAPHORE_EW(semname, count) semname = CreateSpecificNamedSemaphore_ew(get_semaphore_unique_name("semname"), count)
#define DESTROY_SPECIFIC_SEMAPHORE_EW(semname) DestroySpecificNamedSemaphore_ew(get_semaphore_unique_name("semname"), semname)
#else
#ifdef _WINNT
#define CREATE_SPECIFIC_SEMAPHORE_EW(semname, count) CreateSpecificSemaphore_ew(&semname, count)
#else
#define CREATE_SPECIFIC_SEMAPHORE_EW(semname, count) semname = CreateSpecificSemaphore_ew(count)
#endif
#define DESTROY_SPECIFIC_SEMAPHORE_EW(semname) DestroySpecificSemaphore_ew(semname)
#endif

#ifdef _WINNT
#define WAIT_SPECIFIC_SEMAPHORE_EW(semname) WaitSpecificSemaphore_ew(&semname)
#define POST_SPECIFIC_SEMAPHORE_EW(semname) PostSpecificSemaphore_ew(&semname)
#else
#define WAIT_SPECIFIC_SEMAPHORE_EW(semname) WaitSpecificSemaphore_ew(semname)
#define POST_SPECIFIC_SEMAPHORE_EW(semname) PostSpecificSemaphore_ew(semname)
#endif

#ifdef _MACOSX
char *get_semaphore_unique_name(char *semname);
sema_t *CreateSpecificNamedSemaphore_ew( char *sem_name,
                      unsigned int count ); /* sema_ew.c    system-dependent */
#else
#ifdef _WINNT
void CreateSpecificSemaphore_ew(HANDLE *ms, unsigned int count );
                                            /* sema_ew.c    system-dependent */
#else
sema_t *CreateSpecificSemaphore_ew( unsigned int count ); /* sema_ew.c    system-dependent */
#endif
#endif

void PostSpecificSemaphore_ew  ( sema_t * );              /* sema_ew.c    system-dependent */
void WaitSpecificSemaphore_ew  ( sema_t * );              /* sema_ew.c    system-dependent */

#ifdef _MACOSX
void DestroySpecificNamedSemaphore_ew( char *sem_name,  sema_t *ms );
#else
void DestroySpecificSemaphore_ew  ( sema_t * );           /* sema_ew.c    system-dependent */
#endif

void CreateMutex_ew  ( void );              /* sema_ew.c    system-dependent */
void RequestMutex( void );                  /* sema_ew.c    system-dependent */
void ReleaseMutex_ew( void );               /* sema_ew.c    system-dependent */
void CloseMutex( void );                    /* sema_ew.c    system-dependent */
void CreateSpecificMutex( mutex_t * );
void CloseSpecificMutex( mutex_t * );
void RequestSpecificMutex( mutex_t * );
void ReleaseSpecificMutex( mutex_t * );

                                            /* sendmail.c   system-dependent */
void SocketSysInit( void   );               /* socket_ew.c  system-dependent */
void SocketClose  ( SOCKET    );            /* socket_ew.c  system-dependent */
void SocketPerror ( char * );               /* socket_ew.c  system-dependent */
int sendall( SOCKET, const char *, long, int ); /* socket_ew.c  system-dependent */

int  WaitThread( unsigned * );              /* threads_ew.c system-dependent */
int  KillThread( unsigned int );            /* threads_ew.c system-dependent */
int  KillSelfThread( void );                /* threads_ew.c system-dependent */
int  StartThread( thr_ret fun(void *), unsigned stack_size, unsigned *thread_id );
int  StartThreadWithArg( thr_ret fun(void *), void *arg, unsigned stack_size, unsigned *thread_id );

#endif /* EARTHWORM_COMPLEX_FUNCS_H */
