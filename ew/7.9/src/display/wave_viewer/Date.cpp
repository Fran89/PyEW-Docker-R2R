// date.cpp : Implements an aggregate of map glyphs

/* DavidK 19990707 added a Y2K fix to the Date18 Function */
/* DavidK 19990708 changed the value of CDate.Time() from Gregorian seconds (1600)
                   to Julian seconds (1970) ***************/
/* DavidK 19990708 added Date20 Function for showing 4 digit year */
/* DavidK 19990709 fixed bug in CDate::CDate(year,month,day,hour,minute,second)
                   function that was a result of a bug in mktime() with DaylightST
                   that caused 01/01/1970 00:00:00 to crash mktime(), even though
                   the TZ variable was set to GMT with no DST.  (Machine wide,
                   DST was on, but should've been overruled by the local TZ
                   environment variable according to the Win32 documentation.
                   Due to a poor previous bug fix (by me, yesterday), the CDate 
                   function was creating a Julian seconds value of -1 for 1/1/70.  
                   That value was then being applied to all CDate values by the 
                   Carl code that tried to adapt Julian seconds to the 
                   internal CDate format, which was supposed to be Julian, but was 
                   Gregorian.  The net result was that all times were off by 1
                   second because they were adjusted via the 1/1/70 time of (-1).
                   Man, do I ever whine too much!  So, the fix was to the CDate() 
                   constructor mentioned above.  I put wrapper code around the
                   mktime() call to make sure that mktime() was not called for
                   the sensitive time area (1/1/1970 00:00:00 - 00:59:59), instead
                   the function calculates time based upon minutes * 60 + seconds.
*****************************************************************************/
/* DavidK 19990713 The tmTime struct used for calculating date/time in
                   CDate::CDate(year,month,day,hour,minute,second)
                   was starting with a non-zero value for tm_isdst 
                   (the daylightsavingstime indicator), and thus mktime() 
                   thought it should take DST into account, and all times
                   calc'd by CDate::CDate(year,month,day,hour,minute,second)
                   were an hour off. UGH!!!  We fixed the problem by memset()ing
                   the tmTime struct to 0 before using it.
*****************************************************************************/

#include "stdafx.h"
#include <windows.h>
#include "date.h"
#include <sys/timeb.h>
#include <time.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

static int mo[] = {  0,  31,  59,  90, 120, 151, 181, 212, 243, 273, 304, 334,
					 0,  31,  60,  91, 121, 152, 182, 213, 244, 274, 305, 335};
static char *cmo[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
					  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"	};

time_t __cdecl _mkgmtime (struct tm *tb);


// CDate : CDate constructor (initialize to current time)
CDate::CDate()
{
	time_t tst;
	time(&tst);
	struct tm *loc = localtime(&tst);
	TRACE("LOC %d %d %d %d %d %d\n", loc->tm_year, loc->tm_mon, loc->tm_mday, loc->tm_hour,
		loc->tm_min, loc->tm_sec);
	CDate tNow(loc->tm_year+1900, loc->tm_mon+1, loc->tm_mday, loc->tm_hour, loc->tm_min,
		(double)loc->tm_sec);
	m_nYear = tNow.Year();
	m_nMonth = tNow.Month();
	m_nDay = tNow.Day();
	m_nHour = tNow.Hour();
	m_nMinute = tNow.Minute();
	m_dSeconds = tNow.Seconds();
	m_dTime = tNow.Time();
}


/* Y2K FIX */
// CDate(time) : Construct date object from julian seconds
CDate::CDate(double time)
{
	struct tm tmTime;
  struct tm * ptmGMT;
  long tTime;
  

  tTime=(int) time;
  ptmGMT=gmtime(&tTime);
  memcpy(&tmTime,ptmGMT,sizeof(struct tm));
  
	m_nYear = tmTime.tm_year+1900;
	m_nMonth = tmTime.tm_mon+1;
	m_nDay = tmTime.tm_mday;
	m_nHour = tmTime.tm_hour;
	m_nMinute = tmTime.tm_min;
	m_dSeconds = tmTime.tm_sec+(time -((int) time));  //include the partial sec
	m_dTime = time;
}



/* Experienced Y2K Dec:30th problem with this code.  Obsoleted
   19990708  DavidK
****************************/
/*
// CDate(time) : Construct date object from julian seconds
CDate::CDate(double time)
{
	double secs = time;
	long jul = (long)(time / 86400.0);
	secs -= jul * 86400.0;
	long yr = 1600;
	long n;

	n = jul / 146097L;		// 400 year rule
	yr += n * 400L;
	jul -= n * 146097L;
	
	n = jul / 36524L;		// 100 year rule
	yr += n * 100L;
	jul -= n * 36524L;
	
	n = jul / 1461L;		// 4 year rule
	yr += n * 4L;
	jul -= n * 1461L;
	
	n = jul / 365L;			// Regular years
	yr += n;
	jul -= n * 365L;
	
	int leap = 0;
	if(yr % 4   == 0)	leap = 12;
	if(yr % 100 == 0)	leap = 0;
	if(yr % 400 == 0)	leap = 12;
	int mon = 0;
	for(int i = 0; i<12; i++)
	{
		if(mo[mon+leap] <= jul)
			mon++;
	}
	int day = (int)(jul - mo[mon+leap-1] + 1);
	int hr = (int)(secs / 3600.0);
	secs -= hr * 3600.0;
	int mn = (int)(secs / 60.0);
	secs -= mn * 60.0;
	m_nYear = (UINT)yr;
	m_nMonth = mon;
	m_nDay = day;
	m_nHour = hr;
	m_nMinute = mn;
	m_dSeconds = secs;
	m_dTime = time;
}
*/

// CDate(yr, mo, da, hr, mn, secs) : Define date object from gregorian date/time.
CDate::CDate(UINT year, UINT month, UINT day, UINT hour, UINT minute, double second)
{

  struct tm tmTime;
	m_nYear = year;
	m_nMonth = month;
	m_nDay = day;
	m_nHour = hour;
	m_nMinute = minute;
	m_dSeconds = second;

  putenv( "TZ=GMTGMT" );

  /* 19990713 DavidK  the tmTime struct was starting with a non-zero
     value for isdst, and thus mktime thought it should take DST into
     effect.  UGH!!!  So, we overcome the original programmers 
     stupidity (or atleast part of it), by adding a memset(0) call
     for the tmTime structure, so that isdst and other fields will
     start out at 0.  (I was the original programmer if you haven't
     figured it out yet.  I'm sure it was pretty obvious though!) 
  *************************************************************/
  memset(&tmTime,0,sizeof(struct tm));

	tmTime.tm_year= year - 1900;
	tmTime.tm_mon = month - 1;
	tmTime.tm_mday = day;
	tmTime.tm_hour = hour;
	tmTime.tm_min = minute;
  tmTime.tm_isdst = 0;
	tmTime.tm_sec= (int) second;  //include the partial sec
  if(year < 1971)
  {
    if(year == 1970 && month ==1 && day ==1 && hour==0)
    {
      if(minute >= 0 && minute <= 60)
      {
        m_dTime=minute * 60 +second;
      }
      else // some irrational minute value, let mktime() handle it
      {
        m_dTime = mktime(&tmTime);
      }
    }
    else // 1970, but not in the critical zone in the first hour of the year
    {
      m_dTime = mktime(&tmTime);
    }
  }
  else // atleast 1971
  {
    m_dTime = mktime(&tmTime);
  }
}


// CDate(yr, mo, da, hr, mn, secs) : Define date object from gregorian date/time.
/*CDate::CDate(UINT year, UINT month, UINT day, UINT hour, UINT minute, double second)
{
	long jul = 0;
	long yr = year - 1600;
	long n;
	
	n = yr / 400;			// Four hundred year rule
	jul += n * 146097L;
	yr -= n * 400L;
	
	n = yr / 100L;			// One hundred year rule
	jul += n * 36524L;
	yr -= n * 100L;
	
	n = yr / 4L;			// Four year rule
	jul += n * 1461L;
	yr -= n * 4L;
	
	jul += yr * 365L;		// Regular years
	
	int leap = 0;
	if(year % 4   == 0)	leap = 12;
	if(year % 100 == 0) leap = 0;
	if(year % 400 == 0) leap = 12;
	jul += mo[month + leap - 1] + day - 1;
	long jmin = 1440L * jul + 60L * hour + minute;
	m_dTime = 60.0 * jmin + second;
	m_nYear = year;
	m_nMonth = month;
	m_nDay = day;
	m_nHour = hour;
	m_nMinute = minute;
	m_dSeconds = second;
}
*/
//
// Date18() : Calcualate 18 char date in the form 88Jan23 1234 12.21
//		from the julian seconds.  Remember to leave space for the
//		string termination (NUL).
CString CDate::Date18() {
	char c18[80];
	int hrmn = 100 * m_nHour + m_nMinute;
/*************************************************************************
               BEGIN Y2K BUG FIX
*************************************************************************/

  /* Y2K bug fix.  Changing printed year from Date-1900 to Date % 100, so that
     the date come 2000 is not shown as 100, but instead 0.  This is
     somewhat cheasy for a fix, but give the predicted doom of wave_viewer,
     and the time required to make and test this fix, it has been elected.
     The better fix, would be to write a 4 year date.  7/7/99 Davidk
  */

/*	sprintf(c18, "%2d%3s%2d %4d %6.2f",
		m_nYear-1900, cmo[m_nMonth-1], m_nDay, hrmn, m_dSeconds);
*/
  	sprintf(c18, "%02d%3s%2d %04d %6.2f",
		m_nYear%100, cmo[m_nMonth-1], m_nDay, hrmn, m_dSeconds);
/*************************************************************************
               END Y2K BUG FIX
*************************************************************************/

	return CString(c18);
}

CString CDate::Date20() {
	char c20[80];
	int hrmn = 100 * m_nHour + m_nMinute;
  	sprintf(c20, "%4d %3s%2d %04d %04.2f",
		m_nYear, cmo[m_nMonth-1], m_nDay, hrmn, m_dSeconds);

	return CString(c20);
}




// ~CDate : CDate destructor
CDate::~CDate()
{
}
