/* $Id: ustime.h 5714 2013-08-05 19:18:08Z paulf $ */
/*-----------------------------------------------------------------------

    Generic, thread-safe, microsecond resolution, ISO 8601 compliant
	Gregorian calendar and timer routines.

-----------------------------------------------------------------------*/
#if !defined _USTIME_H_INCLUDED_
#define _USTIME_H_INCLUDED_

#include "platform.h"

/* Constants ----------------------------------------------------------*/
#if !defined UST_USECOND
#   define UST_USECOND 	((INT64)1)
#   define UST_MSECOND 	(UST_USECOND * (INT64)1000)
#   define UST_SECOND  	(UST_MSECOND * (INT64)1000)
#   define UST_MINUTE  	(UST_SECOND * (INT64)60)
#   define UST_HOUR    	(UST_MINUTE * (INT64)60)
#   define UST_DAY     	(UST_HOUR * (INT64)24)
#   define UST_YEAR    	(UST_DAY * (INT64)365)
#   define UST_CENTURY 	(UST_YEAR * (INT64)100)
#endif

/* Undefined or "void" time value */
#define VOID_USTIME 	0x7FFFFFFFFFFFFFFFLL

/* Types --------------------------------------------------------------*/
typedef INT64 USTIME;
typedef struct _USTIMER {
	USTIME start;
	USTIME expire;
} USTIMER;

/* ISO 8601 extended date and time formats, date format defaults to ordinal */
typedef enum {						
	UST_DATE = 0,						/* yyyy-ddd */
	UST_TIME = 1,						/* hh:mm:ss */	
	UST_TIME_MSEC = 2,					/* hh:mm:ss.sss */
	UST_TIME_USEC = 3,					/* hh:mm:ss.ssssss */
	UST_DATE_TIME = 4,					/* yyyy-dddThh:mm:ss */
	UST_DATE_TIME_MSEC = 5,				/* yyyy-dddThh:mm:ss.sss */
	UST_DATE_TIME_USEC = 6				/* yyyy-dddThh:mm:ss.ssssss */	
} UST_TIME_FMT;

/* Used with USTSetDFLDateFormat() */
typedef enum {
	UST_ORDINAL = 0,					/* yyyy-ddd, three digit ordinal day-of-year */
	UST_CALENDAR = 1,					/* yyyy-mm-dd, two digit month and day-of-month */
} UST_DFL_DATE_FMT;

typedef enum {
	UST_TERSE,
	UST_VERBOSE
} UST_INTERVAL_FMT;

/* Prototypes ---------------------------------------------------------*/

/* Return system time as USTIME */
USTIME SystemUSTime(void);

/* Convert between USTIME and year, day-of-year, hour, minute, and second */
USTIME EncodeUSTime(INT32 year, INT32 doy, INT32 hour, INT32 minute, REAL64 second);
VOID DecodeUSTime(USTIME ustime, INT32 *year, INT32 *doy, INT32 *hour, INT32 *minute, REAL64 *second);

/* Convert between day-of-year and month/day */
INT32 DayOfYear(INT32 year, INT32 month, INT32 day);
VOID MonthDay(INT32 *month, INT32 *day, INT32 year, INT32 doy);
BOOL IsLeapYear(INT32 year);

/* Set the default date representation to "ordinal" or "calendar" */
VOID USTSetDateRepresentation(CHAR *format);
/* Format USTIME as ASCII string in various ISO 8601 formats */
CHAR *FormatUSTime(CHAR *string, size_t n, UST_TIME_FMT format, USTIME time);

/* Parse string representation '[YY]YY-DOY-HHTMM:SS.SSSSSS' into USTime */
USTIME ParseUSTimeDOY(CHAR *string);

/* Parse string representation '[YY]YY-MM-DDTHH:MM:SS.SSSSSS' into USTime */
USTIME ParseUSTimeMD(CHAR *string);

/* Microsecond sleep */
VOID USSleep(INT64 interval);

/* Start a timer and set interval */
VOID StartUSTimer(USTIMER *timer, INT64 interval);

/* Set, or reset, a timer interval */
VOID SetUSTimer(USTIMER *timer, INT64 interval);
VOID ResetUSTimer(USTIMER *timer, INT64 interval);

/* Wait for a timer to expired */
VOID USTimerWait(USTIMER *timer);

/* Returns elapsed time since timer start */
INT64 USTimerElapsed(USTIMER *timer);

/* Has a timer expired? */
BOOL USTimerIsExpired(USTIMER *timer);

/* Format timer intervals as ASCII string */
CHAR *FormatUSTimerInterval(CHAR *string, size_t n, INT64 interval, UST_INTERVAL_FMT format);

/* Parse string representations of timer intervals into INT64 USTime values */
INT64 ParseUSTimerInterval(CHAR *string);

#endif
