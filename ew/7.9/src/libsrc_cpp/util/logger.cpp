//---------------------------------------------------------------------------
#include "logger.h"
#include <ios>      // ios_base
#include <time.h>   // _tzset()
#include <string.h>   // strcpy(), strcat()
#include <string>
#include <stdarg.h>   // va_list, et.al.
#include <stdio.h>    // error reporting
#include <globalutils.h>
#include <worm_environ.h>

//---------------------------------------------------------------------------
#pragma package(smart_init)

TMutex * TLogger::AccessLock = NULL;
std::fstream TLogger::OutStream;
char TLogger::PreviousDate[WORM_TIMESTR_LENGTH+1] = { "" };
bool TLogger::TruncOnOpen = false;
int TLogger::MaxTooLongLength = 0;

char TLogger::_buff[16384];
char TLogger::_args[16384];

//---------------------------------------------------------------------------
WORM_STATUS_CODE TLogger::OpenFile()
{
   WORM_STATUS_CODE r_status = WORM_STAT_SUCCESS;

   char _timbuf[WORM_TIMESTR_LENGTH];

   // request time in format YYYYMMDD
   TTimeFuncs::DateString(_timbuf, WORM_TIMEFMT_8, WORM_LOCAL_TIME );

   if (   strncmp(_timbuf, PreviousDate, strlen(PreviousDate)) != 0
       || (! OutStream.is_open())
      )
   {
      // date has changed, open a new file
      strcpy( PreviousDate, _timbuf );

      if ( OutStream.is_open() )
      {
         OutStream.close();
      }

      char _logname[256];

      bool _nodirectory = false;

      if ( TGlobalUtils::GetEnvironmentValue(WORM_LOG_DIR) == NULL )
      {
         if ( TGlobalUtils::GetEnvironmentValue(EW_LOG_DIR) == NULL )
         {
            _nodirectory = true;
            strcpy( _logname, "" );
         }
         else
         {
            strcpy( _logname, TGlobalUtils::GetEnvironmentValue(EW_LOG_DIR) );
         }
      }
      else
      {
         strcpy( _logname, TGlobalUtils::GetEnvironmentValue(WORM_LOG_DIR) );
      }
      strcat( _logname, TGlobalUtils::GetProgramName() );
      strcat( _logname, "_" );
      strcat( _logname, _timbuf );
      strcat( _logname, ".log" );

      try
      {
         if ( TruncOnOpen )
         {
            OutStream.open( _logname, std::ios_base::out|std::ios_base::trunc );
         }
         else
         {
            OutStream.open( _logname, std::ios_base::out|std::ios_base::app );
         }

         if ( OutStream.fail() )
         {
            throw std::ios_base::failure( "Failed opening Log file" );
         }
         else if ( ! OutStream.is_open() )
         {
            throw std::ios_base::failure( "OutStream Log file" );
         }

#if defined(_WINNT) || defined(_Windows)
         if ( TGlobalUtils::GetEnvironmentValue(WORM_TIME_ZONE) == NULL )
         {
            OutStream << "==================================================" << std::endl;
            OutStream << "WARNING: The environment variable " WORM_TIME_ZONE << std::endl;
            OutStream << "         (time zone) is not set." << std::endl;
            OutStream << "         UTC times in log messages may be bogus." << std::endl;
            OutStream << "==================================================" << std::endl;
         }
         else
         {
            OutStream << "Notice: Using environment variable " WORM_TIME_ZONE;
            OutStream << "=" << TGlobalUtils::GetEnvironmentValue(WORM_TIME_ZONE);
            OutStream << " for time zone." << std::endl;
            _tzset();
         }
#endif

         if ( _nodirectory )
         {
            Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                 , "Neither environment variable <%s> nor <%s> defined, writing logs to execution directory\n"
                 , WORM_LOG_DIR
                 , EW_LOG_DIR
                 );
         }
      }
      catch( std::ios_base::failure ex )
      {
         OutStream.close();
         fprintf( stderr
                , "TLogger::OpenFile() File: %s\nError: %s"
                , _logname
                , ex.what()
                );
         r_status = WORM_STAT_FAILURE;
      }

   }
   return r_status;
}
//---------------------------------------------------------------------------
void TLogger::Close()
{
   if ( OutStream.is_open() )
   {
      OutStream.close();
   }
   if ( AccessLock != NULL )
   {
      delete( AccessLock );
      AccessLock = NULL;
   }
}
//---------------------------------------------------------------------------
void TLogger::HandleLog(  WORM_LOG_FLAGS p_flags, char * p_buffer )
{
   if ( p_flags & WORM_LOG_TOSTDOUT )
   {
      fprintf( stdout, "%s", p_buffer );
   }

   if ( p_flags & WORM_LOG_TOSTDERR )
   {
      fprintf( stderr, "%s", p_buffer );
   }


   if ( p_flags & WORM_LOG_TOFILE && TGlobalUtils::WriteLogFile() )
   {
//fprintf( stdout, "DEBUG Logit() V\n" );

      if ( OpenFile() == WORM_STAT_SUCCESS )
      {
//fprintf( stdout, "DEBUG Logit() W -1\n" );
//fprintf( stdout, "DEBUG Logit() W:  >%s<\n", p_buffer );
//fprintf( stdout, "DEBUG Logit() W  1\n" );

         if ( ! OutStream.is_open() )
         {
            fprintf( stderr
                   , "TLogger::Logit() OutStream is not open\n"
                   );
         }
         else
         {
//fprintf( stdout, "DEBUG Logit() X -1\n" );
            try
            {
                OutStream << p_buffer;
            }
            catch( ... ) //            catch( ios_base::failure _ioex )
            {
//fprintf( stdout, "TLogger::Logit() OutStream << resulted in exception\n" );
            }
//fprintf( stdout, "DEBUG Logit() X\n" );
            OutStream.flush();
         }
//fprintf( stdout, "DEBUG Logit() Y\n" );
      }
      else
      {
         fprintf( stderr
                , "TLogger::Logit() FAILED OPENING LOG FILE FOR MESSAGE\n%s\n"
                , p_buffer
                );
      }
   }
}
//---------------------------------------------------------------------------
void TLogger::Logit( WORM_LOG_FLAGS p_flags, const char* p_format, ... )
{
   int r_count = 0;

   if ( AccessLock == NULL )
   {
      if ( (AccessLock = new TMutex("logmutex")) == NULL )
      {
         fprintf( stderr, "TLogger::Logit(): Failed creating log file mutex\n" );
         return;
      }
   }

   AccessLock->RequestLock();


   strcpy( _buff, "" );

   if ( p_flags & WORM_LOG_TIMESTAMP )
   {
      TTimeFuncs::DateString(_buff, WORM_TIMEFMT_UTC21);
      strcat( _buff, " " );
   }

   /* TODO : Doesn't really make sense to add program filename to error file */

   if ( p_flags & WORM_LOG_NAMESTAMP )
   {
      strcat( _buff, TGlobalUtils::GetProgramName() );
      strcat( _buff, " " );
   }

   if ( p_flags & WORM_LOG_PIDSTAMP )
   {
      char _pids[10];
      sprintf( _pids, " %d", TGlobalUtils::GetPID() );
      strcat( _buff, _pids );
      strcat( _buff, " " );
   }

   strcpy( _args, "" );
   va_list _argptr;
   va_start(_argptr, p_format);
   r_count = vsprintf(_args, p_format, _argptr);
   va_end(_argptr);


   if ( 0 < r_count )
   {
      strcat( _buff, _args );
   }

   HandleLog( p_flags, _buff );

   AccessLock->ReleaseLock();

//fprintf( stdout, "DEBUG Logit() Z\n" );
}
//---------------------------------------------------------------------------
int TLogger::Logit( char * p_charflags, char * p_format, ... )
{
   int r_count = 0;

   if ( AccessLock == NULL )
   {
      if ( (AccessLock = new TMutex("logmutex")) == NULL )
      {
         fprintf( stderr, "TLogger::Logit(): Failed creating log file mutex\n" );
         return -1;
      }
   }

   AccessLock->RequestLock();


   strcpy( _buff, "" );

   WORM_LOG_FLAGS _flags = 0;


   // Check flag argument

   char * fl = p_charflags;
   while ( (*fl) != '\0' )
   {
      switch( (*fl) )
      {
        case 'o':
             _flags += WORM_LOG_TOSTDOUT;
             break;
        case 'e':
             _flags += WORM_LOG_TOSTDERR;
             break;
        case 't':
             _flags += WORM_LOG_TIMESTAMP;
             break;
        case 'd':
             _flags += WORM_LOG_PIDSTAMP;
             break;
      }
      fl++;
   }


   if ( _flags & WORM_LOG_TIMESTAMP )
   {
      TTimeFuncs::DateString(_buff, WORM_TIMEFMT_UTC21);
      strcat( _buff, " " );
   }

   /* TODO : Doesn't really make sense to add program filename to error file */

   if ( _flags & WORM_LOG_NAMESTAMP )
   {
      strcat( _buff, TGlobalUtils::GetProgramName() );
      strcat( _buff, " " );
   }

   if ( _flags & WORM_LOG_PIDSTAMP )
   {
      char _pids[10];
      sprintf( _pids, " %d", TGlobalUtils::GetPID() );
      strcat( _buff, _pids );
      strcat( _buff, " " );
   }

   strcpy( _args, "" );
   va_list _argptr;
   va_start(_argptr, p_format);
   r_count = vsprintf(_args, p_format, _argptr);
   va_end(_argptr);


   if ( 0 < r_count )
   {
      strcat( _buff, _args );
   }

   HandleLog( _flags, _buff );

   AccessLock->ReleaseLock();

   return r_count;
}
//---------------------------------------------------------------------------
void TLogger::HTML_Logit( char *, char *, ... )
{
   throw worm_exception("TLogger::HTML_Logit() Not yet implemented");
}
//---------------------------------------------------------------------------
