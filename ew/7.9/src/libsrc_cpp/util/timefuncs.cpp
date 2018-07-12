//---------------------------------------------------------------------------
#if defined(_Windows) || defined(_WINNT)
#include <sys\timeb.h>  // ftime
#include <windows.h> // Sleep()
#else
#include <stdlib.h> // fprintf -- DELETE THIS LINE AFTER UNIX CODE TESTED
#include <errno.h>  // errno
#endif

#include <stdio.h>
#include <time.h>

#include "timefuncs.h"
#include <worm_exceptions.h>

//---------------------------------------------------------------------------

#pragma package(smart_init)

const char TTimeFuncs::DOW[7][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

const char TTimeFuncs::MOY[12][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

const short TTimeFuncs::MONTH_STARTS[2][12] =
{
   { 1, 32, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 }
  ,{ 1, 32, 61, 92, 122, 153, 183, 214, 245, 275, 306, 336 }
};

//---------------------------------------------------------------------------
HIGHRES_TIME TTimeFuncs::GetHighResTime()
{
   HIGHRES_TIME r_time;
#if defined(_WINNT) || defined(_Windows)
   timeb _tb;
   ftime(&_tb);
   r_time = (double)_tb.time + (double)_tb.millitm / 1000.0;
#else
static bool _needdebugwarning = true;
if ( _needdebugwarning )
{
   fprintf( stdout, "TTimeFuncs::GetHighResTime() NOT YET TESTED ON UNIX" );
   _needdebugwarning = false;
}
   struct timespec _ts;

   if( clock_gettime( CLOCK_REALTIME, &_ts ) == 0 )
   {
      r_time = (double) _ts.tv_sec + (double)_ts.tv_nsec*0.000000001;
   }
#endif
   return r_time;
}
//---------------------------------------------------------------------------
char* TTimeFuncs::DateString( char* r_buffer
                            , WORM_TIME_FORMAT p_format
                            , WORM_TIME_LOCALE p_locale /* = WORM_GMT_TIME */
                            , double p_time             /* = -1.0, means 'now' */
                            )
{
   time_t   _lowtime;
   int      _millis = 0;

   // Only use high-resolution time for those formats that require it
   switch ( p_format )
   {
     case WORM_TIMEFMT_8:
     case WORM_TIMEFMT_14:
     case WORM_TIMEFMT_16:
     case WORM_TIMEFMT_UTC21:
     case WORM_TIMEFMT_19:
     case WORM_TIMEFMT_26:
          if ( p_time == -1.0 )
          {
             _lowtime = time(NULL);
          }
          else
          {
             _lowtime = (int)p_time;
          }
          break;
     case WORM_TIMEFMT_18:
     case WORM_TIMEFMT_23:
          if ( p_time == -1.0 )
          {
             // add .005 to prep for rounding to milliseconds
             double _hrtime = GetHighResTime() + 0.005;
             // get the standard time type for structure conversion
             _lowtime = (time_t)_hrtime;  // strip off decimal portion
             _millis  = (int)((_hrtime - _lowtime) * 100.0 );
          }
          else
          {
             _lowtime = (time_t)p_time;  // strip off decimal portion
             _millis  = (int)((p_time - _lowtime) * 100.0 );
          }
          break;
   }

   struct tm* _tmstruct;

   switch( p_locale )
   {
     case WORM_GMT_TIME:
          _tmstruct = gmtime(&_lowtime);
          break;
     case WORM_LOCAL_TIME:
          _tmstruct = localtime(&_lowtime);
          break;
   }

   switch( p_format )
   {
     case WORM_TIMEFMT_18:  // YYYYMMDDhhmmss.sss
          sprintf( r_buffer
                 , "%4d%02d%02d%02d%02d%02d.%03d"
                 ,  _tmstruct->tm_year + 1900
                 ,  _tmstruct->tm_mon  + 1
                 ,  _tmstruct->tm_mday
                 ,  _tmstruct->tm_hour
                 ,  _tmstruct->tm_min
                 ,  _tmstruct->tm_sec
                 , _millis
                 );
          break;
     case WORM_TIMEFMT_UTC21:  // YYYYMMDD_UTC_hh:mm:ss
          sprintf( r_buffer
                 , "%4d%02d%02d_UTC_%02d:%02d:%02d"
                 ,  _tmstruct->tm_year + 1900
                 ,  _tmstruct->tm_mon  + 1
                 ,  _tmstruct->tm_mday
                 ,  _tmstruct->tm_hour
                 ,  _tmstruct->tm_min
                 ,  _tmstruct->tm_sec
                 );
          break;
     case WORM_TIMEFMT_8:  // YYYYMMDD
          sprintf( r_buffer
                 , "%4d%02d%02d"
                 ,  _tmstruct->tm_year + 1900
                 ,  _tmstruct->tm_mon  + 1
                 ,  _tmstruct->tm_mday
                 );
          break;
     case WORM_TIMEFMT_14:  // YYYYMMDDhhmmss
          sprintf( r_buffer
                 , "%4d%02d%02d%02d%02d%02d"
                 ,  _tmstruct->tm_year + 1900
                 ,  _tmstruct->tm_mon  + 1
                 ,  _tmstruct->tm_mday
                 ,  _tmstruct->tm_hour
                 ,  _tmstruct->tm_min
                 ,  _tmstruct->tm_sec
                 );
          break;
     case WORM_TIMEFMT_16:  // YYYY/MM/DD hh:mm
          sprintf( r_buffer
                 , "%4d/%02d/%02d %02d:%02d"
                 ,  _tmstruct->tm_year + 1900
                 ,  _tmstruct->tm_mon  + 1
                 ,  _tmstruct->tm_mday
                 ,  _tmstruct->tm_hour
                 ,  _tmstruct->tm_min
                 );
          break;
     case WORM_TIMEFMT_19:  // YYYY/MM/DD hh:mm:ss
          sprintf( r_buffer
                 , "%4d/%02d/%02d %02d:%02d:%02d"
                 ,  _tmstruct->tm_year + 1900
                 ,  _tmstruct->tm_mon  + 1
                 ,  _tmstruct->tm_mday
                 ,  _tmstruct->tm_hour
                 ,  _tmstruct->tm_min
                 ,  _tmstruct->tm_sec
                 );
          break;
     case WORM_TIMEFMT_23:  // YYYY/MM/DD hh:mm:ss.sss
          sprintf( r_buffer
                 , "%4d/%02d/%02d %02d:%02d:%02d.%03d"
                 ,  _tmstruct->tm_year + 1900
                 ,  _tmstruct->tm_mon  + 1
                 ,  _tmstruct->tm_mday
                 ,  _tmstruct->tm_hour
                 ,  _tmstruct->tm_min
                 ,  _tmstruct->tm_sec
                 , _millis
                 );
          break;

     case WORM_TIMEFMT_26:
          sprintf( r_buffer
                 , "%s %s %02d %02d:%02d:%02d %04d\n"
                 , DOW[_tmstruct->tm_wday]
                 , MOY[_tmstruct->tm_mon]
                 ,  _tmstruct->tm_mday
                 ,  _tmstruct->tm_hour
                 ,  _tmstruct->tm_min
                 ,  _tmstruct->tm_sec
                 ,  _tmstruct->tm_year + 1900
                 );
          break;
   }

   return r_buffer;
}
//---------------------------------------------------------------------------
double TTimeFuncs::TimeToDouble( const char * p_buffer )
{
   double r_dbl;

   if ( p_buffer == NULL )
   {
      throw worm_exception("TTimeFuncs::TimeToDouble(): string parameter was NULL");
   }

   // expecting "YYYYMMDDHHMMSS.sss"
   //            0123456789012345678
   if ( strlen(p_buffer) != 18 )
   {
      throw worm_exception("TTimeFuncs::TimeToDouble(): string parameter invalid length");
   }

   char _timestr[18];
   char _work[5];
   char * _stringptr;
   struct tm time_check;


   strcpy( _timestr, p_buffer );

   _stringptr = _timestr;

   // year
   strncpy( _work, _stringptr, 4 );
   _work[4] = '\0';
   time_check.tm_year = atoi(_work) - 1900;

   // month
   _stringptr += 4;
   strncpy( _work, _stringptr, 2 );
   _work[2] = '\0';
   time_check.tm_mon  = atoi(_work) - 1;

   // day
   _stringptr += 2;
   strncpy( _work, _stringptr, 2 );
   time_check.tm_mday = atoi(_work);

   // hour
   _stringptr += 2;
   strncpy( _work, _stringptr, 2 );
   time_check.tm_hour = atoi(_work);

   // minute
   _stringptr += 2;
   strncpy( _work, _stringptr, 2 );
   time_check.tm_min = atoi(_work);

   // second
   _stringptr += 2;
   strncpy( _work, _stringptr, 2 );
   time_check.tm_sec = atoi(_work);

   time_check.tm_isdst = -1;


   // millisecond
   _stringptr += 3;
   strncpy( _work, _stringptr, 3 );


   r_dbl = (double)mktime(&time_check) + ((double)atoi(_work) / 1000.0);

   return r_dbl;
}
//---------------------------------------------------------------------------
void TTimeFuncs::MSecSleep( unsigned int p_msec )
{
#if defined(_WINNT) || defined(_Windows)
   Sleep(p_msec);
#else
   struct timespec sleeptime;
   struct timespec timeleft;
   int err;

   sleeptime.tv_sec = (time_t) p_msec / 1000;
   sleeptime.tv_nsec = (long) (1000000 * (p_msec % 1000));

   while( nanosleep(&sleeptime, &timeleft) != 0 )
   {
      if ( (err = errno) == EINTR)
      {
        /*printf( "sleep_ew: interrupted by signal;" );*//*DEBUG*/ 
        /*printf( " %d msec left\n",
	        (int) (timeleft.tv_sec*1000 + timeleft.tv_nsec/1000000) );*//*DEBUG*/
        sleeptime = timeleft;
      }
      else
      {
         fprintf(stderr, "sleep_ew: error from nanosleep: %s\n",
                 strerror(err));
         fprintf(stderr,"\ttime requested = %.3f sec, time left = %.3f sec\n",
                 sleeptime.tv_sec + sleeptime.tv_nsec*1.e-9,
                 timeleft.tv_sec + timeleft.tv_nsec*1.e-9);
       }
   }
#endif
}
//---------------------------------------------------------------------------
bool TTimeFuncs::IsLeapyear( unsigned int p_year )
{
   if ( p_year % 4 != 0 )
   {
      return false;
   }

   if ( p_year % 400 == 0 )
   {
      return true;
   }

   if ( p_year % 100 == 0 )
   {
      return false;
   }

   return true;
}
//---------------------------------------------------------------------------
bool TTimeFuncs::YearJulianToMonDay( short   p_year
                                   , short   p_julday
                                   , short * r_monthnum
                                   , short * r_monthday
                                   )
{
   if (   p_julday < 1
       || r_monthnum == NULL
       || r_monthday == NULL
      )
   {
      return false;
   }

   // get MONTH_STARTS[x][] index
   int _yt =  ( IsLeapyear(p_year) ? 1 : 0 );
   int _mi = 0;

   for ( int _m = 0 ; _m < 12 ; _m++ )
   {
      if ( MONTH_STARTS[_yt][_m] <= p_julday )
      {
         _mi = _m;
         continue;
      }
      break;
   }

   *r_monthday = p_julday - MONTH_STARTS[_yt][_mi] + 1;

   *r_monthnum = _mi + 1;

   return true;
}
//---------------------------------------------------------------------------
bool TTimeFuncs::CrackDate( const char * p_buffer    // "YYYYMMDD...."
                          , short      * r_year      // 1900....
                          , short      * r_monthnum  // 1 - 12
                          , short      * r_monthday  // 1 - 31
                          , short      * r_year2     // 0....
                          , short      * r_julday    // 1 - 366
                          , bool       * r_isleap    // is it a leap year
                          )
{
   if (   p_buffer == NULL || strlen(p_buffer) < 8
       || r_year     == NULL
       || r_monthnum == NULL
       || r_monthday == NULL
      )
   {
      return false;
   }

   bool _isleap;

   char _wrk[9];

   //  YYYYMMDD....
   //  012345678

   strncpy( _wrk, p_buffer, 8 );
   _wrk[8] = '\0';

   *r_monthday = atoi( _wrk + 6 );


   _wrk[6] = '\0';

   *r_monthnum = atoi( _wrk + 4 );


   _wrk[4] = '\0';

   *r_year = atoi( _wrk );

   if ( r_year2 != NULL )
   {
      *r_year2 = atoi( _wrk + 2 );
   }

   if ( r_julday != NULL )
   {
      _isleap = IsLeapyear( *r_year );

      if ( r_isleap != NULL )
      {
         *r_isleap = _isleap;
      }

      *r_julday = TTimeFuncs::MONTH_STARTS[(_isleap ? 1 : 0)][(*r_monthnum) - 1]
                + *r_monthday
                - 1
              ;
   }

   return true;
}
//---------------------------------------------------------------------------
bool TTimeFuncs::CrackTime( const char * p_buffer // "HHMMSS.sss" or "........HHMMSS.sss"
                          , short      * r_hour
                          , short      * r_min
                          , short      * r_sec
                          , short      * r_msec
                          , double     * r_fpsec
                          )
{
   if ( p_buffer == NULL || r_hour == NULL || r_min == NULL )
   {
      return false;
   }

   if ( strlen(p_buffer) != 10 && strlen(p_buffer) != 18 )
   {
      return false;
   }

   const char  * _pointer = ( strlen(p_buffer) == 10 ? p_buffer : (p_buffer + 8) );

   char   _wrk[4];

   if ( _pointer[6] != '.' )
   {
      return false;
   }

   _wrk[0] = _pointer[0];
   _wrk[1] = _pointer[1];
   _wrk[2] = '\0';
   *r_hour = atoi(_wrk);


   _wrk[0] = _pointer[2];
   _wrk[1] = _pointer[3];
   _wrk[2] = '\0';
   *r_min = atoi(_wrk);

   if ( r_sec != NULL )
   {
      int _sec
        , _msec
        ;

      _wrk[0] = _pointer[4];
      _wrk[1] = _pointer[5];
      _wrk[2] = '\0';
      _sec = atoi(_wrk);

      if ( r_sec != 0 )
      {
         *r_sec = _sec;
      }

      _wrk[0] = _pointer[7];
      _wrk[1] = _pointer[8];
      _wrk[2] = _pointer[9];
      _wrk[3] = '\0';
      _msec = atoi(_wrk);

      if ( r_msec != 0 )
      {
         *r_msec = _msec;
      }

      if ( r_fpsec != NULL && r_msec != 0 )
      {
         *r_msec = (double)_sec + ((double)_msec) / 1000.0;
      }
   }

   return true;
}
//---------------------------------------------------------------------------

