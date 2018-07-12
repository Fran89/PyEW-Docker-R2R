// date.cpp : General date routines (double representation)
//		The basic idea is to convert between external (human)
//		date representations during I/O, but storing date 
//		internally as a double. Date calculations then just
//		become simple arithmetic.
//
// Modification history...
// 19980616 cej Fixed unitialized m_dSeconds member problem in gregorian constructor
// 19991211 cej Added Date20() for Y2K compliance
// 19991211 cej Fixed a strange bug that caused days for non leap years to be off by 1
// 19991211 cej Allowed base years to be set in a #define statement
// 20000318 cej Made Date18 y2k compliant
// 20020216 cej Removed MFC dependancy
// 
//

#include <windows.h>
#include <stdio.h>
#include "date.h"
#include <sys/timeb.h>
#include <time.h>
#include "str.h"

static int mo[] = {  0,  31,  59,  90, 120, 151, 181, 212, 243, 273, 304, 334,
					 0,  31,  60,  91, 121, 152, 182, 213, 244, 274, 305, 335};
static char *cmo[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
					  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"	};

static int base = 1970;	// Base year  // DK CLEANUP Gut this homemade/homecalculated date class
                                      // Replace with struct tm or C++ equiv

// CDate : CDate constructor (initialize to current time)
CDate::CDate()
{
	time_t tst;
	time(&tst);
  gmtime_ew(&tst, &stmTime);
  dFracSecs = (float)0.0
}

// CDate(time) : Construct date object from julian seconds
CDate::CDate(double time)
{
	time_t tst;
	tst = (time_t)time;;
  gmtime_ew(&tst, &stmTime);
  this.stmTime.
  dFracSecs = time - tst;

}

// CDate(yr, mo, da, hr, mn, secs) : Define date object from gregorian date/time.
CDate::CDate(UINT year, UINT month, UINT day, UINT hour, UINT minute, double second)
{
	long jul = 0;
	int leap;

	// Calculate days from base year
	for(UINT yr=base; yr<year; yr++) {
		jul += 365;
		leap = 0;
		if(yr % 4   == 0)	leap = 1;
		if(yr % 100 == 0)	leap = 0;
		if(yr % 400 == 0)	leap = 1;
		jul += leap;
	}

	leap = 0;
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

//
// Date18() : Calcualate 18 char date in the form 88Jan23 1234 12.21
//		from the julian seconds.  Remember to leave space for the
//		string termination (NUL).
CStr CDate::Date18() {
	int y2k;
	char c18[80];
	int hrmn = 100 * m_nHour + m_nMinute;
	if(m_nYear < 2000)
		y2k = m_nYear - 1900;
	else
		y2k = m_nYear - 2000;
	sprintf(c18, "%02d%3s%02d %04d%6.2f",
		y2k, cmo[m_nMonth-1], m_nDay, hrmn, m_dSeconds);
	return CStr(c18);
}

//
// Date20() : Calcualate 20 char date in the form 1988Jan23 1234 12.21
//		from the julian seconds.  Remember to leave space for the
//		string termination (NUL).
CStr CDate::Date20() {
	char c20[80];
	int hrmn = 100 * m_nHour + m_nMinute;
	sprintf(c20, "%04d%3s%02d %04d %6.2f",
		m_nYear, cmo[m_nMonth-1], m_nDay, hrmn, m_dSeconds);
	return CStr(c20);
}

// ~CDate : CDate destructor
CDate::~CDate()
{
}
