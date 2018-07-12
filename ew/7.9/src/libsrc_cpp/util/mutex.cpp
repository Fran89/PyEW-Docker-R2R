//---------------------------------------------------------------------------
#include "mutex.h"
#include <string.h>
#include <logger.h>

#include <stdio.h>

//---------------------------------------------------------------------------

#pragma package(smart_init)
//=============================================================================
TMutex::TMutex( const MUTEX_NAME p_name )
{
   if ( p_name == NULL )
   {
      throw worm_exception("TMutex(): no mutex name supplied");
   }
   strcpy( Name, p_name );

#if defined(_WINNT) || defined(_Windows) //-------------------------------------------

   if ( (MutexHandle = CreateMutex( NULL, FALSE, Name )) == NULL )
   {
      worm_exception _expt( "TMutex::TMutex(): Error creating mutex handle <" );
                     _expt += Name;
                     _expt += ">";
      throw _expt;
   }

#elif defined(_SOLARIS) //-------------------------------------------

/*
   // Some of the code needed if this was to be a truly
   // inter-process mutex

   // kludge together an id from the character values of the Name
   //
   //
   key_t _memkey = 2091; // fako base value, hopefully above system ids

   for ( int _c = 0, _csz = strlen(p_name) ; _c < _csz ; _c++ )
   {
      _memkey += ( (2 ^ _c) * p_name[_c] );
   }

   if ( (ShMemoryId = shmget( _memkey, sizeof(mutex_t), 0 )) == -1 )
   {
      // memory region does not yet exist, this is the owner
   }
*/

   void * _dummy   = NULL;
   int    _retcode;

   if ( (_retcode = mutex_init( &MutexHandle, USYNC_THREAD, _dummy )) != 0 )
   {
      worm_exception _expt( "TMutex(): Error " );
                     _expt += (int)_retcode;
                     _expt += " returned from mutex_init()";
      throw _expt;
   }

#else            //-------------------------------------------
#error TMutex::TMutex(): Not yet implemented for this O/S
#endif           //-------------------------------------------
}
//=============================================================================
TMutex::~TMutex()
{
#if defined(_WINNT) || defined(_Windows) //-------------------------------------------
   CloseHandle( MutexHandle );
#elif defined(_SOLARIS) //-------------------------------------------
   int   _retcode;
   if ( (_retcode = mutex_destroy( &MutexHandle )) != 0 )
   {
      fprintf( stderr,
             , "~TMutex(): Error from mutex_destroy: %d\n",
             , _retcode
             );
   }
#else            //-------------------------------------------
#error TMutex::~TMutex(): Not yet implemented for this O/S
#endif           //-------------------------------------------
}
//=============================================================================
void TMutex::RequestLock()
{
#if defined(_WINNT) || defined(_Windows) //-------------------------------------------
   if ( WaitForSingleObject( MutexHandle, INFINITE ) == WAIT_FAILED ) // waits forever
   {
      worm_exception _expt( "TMutex()::RequestLock(" );
                     _expt += Name;
                     _expt += "): WaitForSingleObject() failed";
      throw _expt;
   }
#elif defined(_SOLARIS) //-------------------------------------------
   int   _retcode;

   if ( (_retcode = mutex_lock( &MutexHandle )) != 0 )
   {
      worm_exception _expt( "TMutex()::RequestLock() Error " );
                     _expt += (int)_retcode;
                     _expt += " returned from mutex_lock()";
      throw _expt;
   }
#else            //-------------------------------------------
#error TMutex::RequestLock(): Not yet implemented for this O/S
#endif           //-------------------------------------------
}
//=============================================================================
void TMutex::ReleaseLock()
{
#if defined(_WINNT) || defined(_Windows) //-------------------------------------------
   if ( ReleaseMutex( MutexHandle ) == 0 )
   {
      worm_exception _expt( "TMutex()::ReleaseLock() Error releasing mutex <" );
                     _expt += Name;
                     _expt += ">";
      throw _expt;
   }
#elif defined(_SOLARIS) //-------------------------------------------
   int   _retcode;

   if ( (_retcode = mutex_unlock( &MutexHandle )) != 0 )
   {
      worm_exception _expt( "TMutex()::ReleaseLock() Error " );
                     _expt += (int)_retcode;
                     _expt += " returned from mutex_unlock()";
      throw _expt;
   }
#else            //-------------------------------------------
#error TMutex::ReleaseLock(): Not yet implemented for this O/S
#endif           //-------------------------------------------
}
//=============================================================================

