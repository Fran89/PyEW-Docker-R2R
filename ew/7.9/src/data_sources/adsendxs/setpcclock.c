#include <windows.h>
#include <time.h>
#include <time_ew.h>
#include <earthworm.h>


/**************************************************************
  SetPcClock

  Set the PC clock to time from the GPS receiver.
  The callback routine routine waits GetGpsDelay msec after the
  1-PPS pulse before requesting GPS time, so we correct here.

  tgps = Time from the GPS clock.
 **************************************************************/

void SetPcClock( time_t tgps )
{
   extern int GetGpsDelay;     // Wait this many msec before requesting GPS time
   SYSTEMTIME st;
   struct tm  gmt;

   gmtime_ew( &tgps, &gmt );
   st.wYear   = gmt.tm_year + 1900;
   st.wMonth  = gmt.tm_mon + 1;
   st.wDay    = gmt.tm_mday;
   st.wHour   = gmt.tm_hour;
   st.wMinute = gmt.tm_min;
   st.wSecond = gmt.tm_sec;
   st.wMilliseconds = GetGpsDelay;
   SetSystemTime( &st );
   return;
}
