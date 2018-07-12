/* $Id: ustime.c 5776 2013-08-09 17:24:38Z paulf $ */
/*-----------------------------------------------------------------------
    Copyright (c) 2000-2007 - DAQ Systems, LLC. - All rights reserved.
-------------------------------------------------------------------------

    Generic, thread-safe, microsecond resolution, Gregorian calendar and
    timer routines.
	
-----------------------------------------------------------------------*/
//#define UNIT_TEST                         /* Define and compile to test */

#include "ustime.h"

#if defined WIN32
#   include <sys/types.h>					/* For ftime() */
#   include <sys/timeb.h>
#else
#   include <sys/time.h>					/* For gettimeofday() */
#endif

/* Module constants ---------------------------------------------------*/
#define EPOCH_JDN			2440588			/* 1 January 1970 */
#define MAX_FORMAT			128

/* Module types -------------------------------------------------------*/

/* Module globals -----------------------------------------------------*/
static MUTEX mutex;
static BOOL mutex_initialized = FALSE;
static INT64 EndOfMonth[2][15] = {
	{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365, 396, 424},
	{0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366, 397, 425},
};
static UST_DFL_DATE_FMT date_format = UST_ORDINAL;
static BOOL fmt_initialized = FALSE;

/* Module macros ------------------------------------------------------*/
/* floor(x/y), where x, y>0 are integers, using integer arithmetic */
#define QFLOOR(x, y) ((x) > 0 ? (x) / (y) : -(((y) - 1 - (x)) / (y)))

/* Module prototypes --------------------------------------------------*/
static INT32 YearDOYToJdn(INT32 year, INT32 doy);
static VOID JdnToYearDOY(INT32 jdn, INT32 *year, INT32 *doy);

/*---------------------------------------------------------------------*/
/* Time routines... */
/*---------------------------------------------------------------------*/
USTIME SystemUSTime(void)
{
#if !defined WIN32
	struct timeval tv;
	struct timezone tz;

	tz.tz_minuteswest = 0;
	tz.tz_dsttime = 0;

	if (!mutex_initialized) {
		MUTEX_INIT(&mutex);
		mutex_initialized = TRUE;
	}

	MUTEX_LOCK(&mutex);
	gettimeofday(&tv, &tz);
	MUTEX_UNLOCK(&mutex);

	return ((USTIME)tv.tv_sec * 1000000) + (USTIME)tv.tv_usec;
#else
	struct _timeb tp;

	if (!mutex_initialized) {
		MUTEX_INIT(&mutex);
		mutex_initialized = TRUE;
	}

	MUTEX_LOCK(&mutex);
	_ftime(&tp);
	MUTEX_UNLOCK(&mutex);

	return ((USTIME)tp.time * 1000000) + ((USTIME)tp.millitm * 1000);
#endif
}

/*---------------------------------------------------------------------*/
USTIME EncodeUSTime(INT32 year, INT32 doy, INT32 hour, INT32 minute, REAL64 second)
{

	ASSERT(year != 0);
	ASSERT(doy >= 1 && doy <= 366);

	return (UST_DAY * (YearDOYToJdn(year, doy) - EPOCH_JDN)) +
		(UST_HOUR * hour) + (UST_MINUTE * minute) + (USTIME)((REAL64)UST_SECOND * second);
}

/*---------------------------------------------------------------------*/
VOID DecodeUSTime(USTIME ustime, INT32 *year, INT32 *doy, INT32 *hour, INT32 *minute, REAL64 *second)
{
	INT64 d;

	d = (INT32)QFLOOR(ustime, UST_DAY);
	JdnToYearDOY((INT32)(d + EPOCH_JDN), year, doy);
	ustime -= UST_DAY * d;

	*hour = (INT32)(ustime / UST_HOUR);
	ustime -= UST_HOUR * (*hour);

	*minute = (INT32)(ustime / UST_MINUTE);
	ustime -= UST_MINUTE * (*minute);

	*second = (REAL64)ustime / (REAL64)UST_SECOND;

	return;
}

/*---------------------------------------------------------------------*/
INT32 DayOfYear(INT32 year, INT32 month, INT32 day)
{
	INT32 leap;

	ASSERT(month >= 1 && month <= 12);

	leap = (IsLeapYear(year) ? 1 : 0);

	return (INT32)EndOfMonth[leap][month - 1] + day;
}

/*---------------------------------------------------------------------*/
VOID MonthDay(INT32 *month, INT32 *day, INT32 year, INT32 doy)
{
	INT32 i, leap;

	ASSERT(doy >= 0 && doy <= 366);

	leap = (IsLeapYear(year) ? 1 : 0);

	i = 1;
	while (doy > EndOfMonth[leap][i])
		i++;

	*month = i;
	*day = doy - (INT32)EndOfMonth[leap][i - 1];

	return;
}

/*---------------------------------------------------------------------*/
BOOL IsLeapYear(INT32 year)
{
	BOOL leap;

	if (year < 0)
		year += 1;

	leap = (year % 4 == 0);
	leap = leap && (year % 100 != 0 || year % 400 == 0);

	return leap;
}

/*---------------------------------------------------------------------*/
VOID USTSetDateRepresentation(CHAR *format)
{

	if (strncasecmp(format, "Ordinal", strlen(format)) == 0) 
		date_format = UST_ORDINAL;
	else if (strncasecmp(format, "Calendar", strlen(format)) == 0) 
		date_format = UST_CALENDAR;

	return;
}

/*---------------------------------------------------------------------*/
static VOID GetDFLDateFormat(void)
{
	CHAR *ptr;

    if((ptr = getenv("DATE_FORMAT")) != NULL) {
		if (strncasecmp(ptr, "Ordinal", strlen(ptr)) == 0) 
			date_format = UST_ORDINAL;
		else if (strncasecmp(ptr, "Calendar", strlen(ptr)) == 0) 
			date_format = UST_CALENDAR;
    }

	fmt_initialized = TRUE;

	return;
}

/*---------------------------------------------------------------------*/
CHAR *FormatUSTime(CHAR *string, size_t n, UST_TIME_FMT format, USTIME time)
{
	INT32 year, doy, month, day, hour, minute;
	REAL64 second;
	size_t length;

	if (!fmt_initialized) 
		GetDFLDateFormat();

	if (time == VOID_USTIME) {
		snprintf(string, n, "Undefined");
		return string;
	}

	DecodeUSTime(time, &year, &doy, &hour, &minute, &second);
	MonthDay(&month, &day, year, doy);

	length = 0;

	if (format == UST_DATE) {
		if (date_format == UST_ORDINAL)
			length = snprintf(string, n, "%04d-%03d", year, doy);
		else if (date_format == UST_CALENDAR)
			length = snprintf(string, n, "%04d-%02d-%02d", year, month, day);
	} else if (format == UST_DATE_TIME || format == UST_DATE_TIME_MSEC || format == UST_DATE_TIME_USEC) {
		if (date_format == UST_ORDINAL)
			length = snprintf(string, n, "%04d-%03dT", year, doy);
		else if (date_format == UST_CALENDAR)
			length = snprintf(string, n, "%04d-%02d-%02dT", year, month, day);
	}

	if (format == UST_TIME || format == UST_DATE_TIME) 
		length = snprintf(string + length, n, "%02d:%02d:%02dZ", hour, minute, (INT32)second);
	else if (format == UST_TIME_MSEC || format == UST_DATE_TIME_MSEC) 
		length = snprintf(string + length, n, "%02d:%02d:%06.3lfZ", hour, minute, second);
	else if (format == UST_TIME_USEC || format == UST_DATE_TIME_USEC) 
		length = snprintf(string + length, n, "%02d:%02d:%09.6lfZ", hour, minute, second);

	return string;
}

/*---------------------------------------------------------------------*/
/* Sleep routine... */
/*---------------------------------------------------------------------*/
VOID USSleep(INT64 interval)
{
#if !defined WIN32
	int result;
	struct timespec requested, remainder;

	requested.tv_sec = (time_t)(interval / UST_SECOND);
	requested.tv_nsec = (long)((interval % UST_SECOND) * 1000);

	if ((result = nanosleep(&requested, &remainder)) != 0) {
		/* Were we interrupted by a signal? */
		if (errno != EINTR) {
			fprintf(stderr, "ERROR: nanosleep(%lu.%09lu, %lu.%09lu)) returned %d\n",
				requested.tv_sec, requested.tv_nsec, 
				remainder.tv_sec, remainder.tv_nsec, result);
		}
	}
#else
	Sleep((UINT32)(interval / 1000));
#endif
	return;
}

/*---------------------------------------------------------------------*/
/* Timer routines... */
/*---------------------------------------------------------------------*/
VOID StartUSTimer(USTIMER *timer, INT64 interval)
{
	ASSERT(timer != NULL);

	timer->start = SystemUSTime();
	timer->expire = timer->start + (USTIME)interval;

	return;
}

/*---------------------------------------------------------------------*/
VOID SetUSTimer(USTIMER *timer, INT64 interval)
{
	ASSERT(timer != NULL);

	timer->expire = SystemUSTime() + (USTIME)interval;

	return;
}

/*---------------------------------------------------------------------*/
VOID ResetUSTimer(USTIMER *timer, INT64 interval)
{
	ASSERT(timer != NULL);

	timer->expire += (USTIME)interval;

	return;
}

/*---------------------------------------------------------------------*/
VOID USTimerWait(USTIMER *timer)
{
	INT64 interval;

	ASSERT(timer != NULL);

	interval = timer->expire - SystemUSTime();

	if (interval > 0)
		USSleep(interval);

	return;
}

/*---------------------------------------------------------------------*/
INT64 USTimerElapsed(USTIMER *timer)
{
	ASSERT(timer != NULL);

	return SystemUSTime() - timer->start;
}

/*---------------------------------------------------------------------*/
BOOL USTimerIsExpired(USTIMER *timer)
{
	ASSERT(timer != NULL);

	if (timer->expire > SystemUSTime())
		return FALSE;

	return TRUE;
}

/*---------------------------------------------------------------------*/
CHAR *FormatUSTimerInterval(CHAR *string, size_t n, INT64 interval, UST_INTERVAL_FMT format)
{
	BOOL started;
	CHAR tstring[MAX_FORMAT + 1], tstring1[MAX_FORMAT + 1], *p;
	UINT64 temp;
	size_t length = 0;

	ASSERT(string != NULL);

	string[0] = '\0';
	started = FALSE;

	if (interval >= UST_YEAR) {
		temp = interval / UST_YEAR;
		interval -= temp * UST_YEAR;
		if (format == UST_VERBOSE)
			length = snprintf(tstring, MAX_FORMAT, "%u %s", (UINT32)temp, (temp == 1 ? "year" : "years"));
		else if (format == UST_TERSE)
			length = snprintf(tstring, MAX_FORMAT, "%u-", (UINT32)temp);
		if (strlen(string) + length >= n) {
			string[0] = '\0';
			return NULL;
		}
		strcat(string, tstring);
		started = TRUE;
	}

	if (interval >= UST_DAY) {
		temp = interval / UST_DAY;
		interval -= temp * UST_DAY;
		if (format == UST_VERBOSE && temp > 0) {
			if (started)
				length = snprintf(tstring, MAX_FORMAT, ", %u %s", (UINT32)temp, (temp == 1 ? "day" : "days"));
			else 
				length = snprintf(tstring, MAX_FORMAT, "%u %s", (UINT32)temp, (temp == 1 ? "day" : "days"));
		} else if (format == UST_TERSE)
			length = snprintf(tstring, MAX_FORMAT, "%03uT", (UINT32)temp);
		if (strlen(string) + length >= n) {
			string[0] = '\0';
			return NULL;
		}
		strcat(string, tstring);
		started = TRUE;
	}

	if (format == UST_TERSE)
		started = TRUE;
	if (interval >= UST_HOUR) {
		temp = interval / UST_HOUR;
		interval -= temp * UST_HOUR;
		if (format == UST_VERBOSE && temp > 0) {
			if (started)
				length = snprintf(tstring, MAX_FORMAT, ", %u %s", (UINT32)temp, (temp == 1 ? "hour" : "hours"));
			else 
				length = snprintf(tstring, MAX_FORMAT, "%u %s", (UINT32)temp, (temp == 1 ? "hour" : "hours"));
		} else if (format == UST_TERSE)
			length = snprintf(tstring, MAX_FORMAT, "%02u:", (UINT32)temp);
		if (strlen(string) + length >= n) {
			string[0] = '\0';
			return NULL;
		}
		strcat(string, tstring);
		started = TRUE;
	}

	if (interval >= UST_MINUTE) {
		temp = interval / UST_MINUTE;
		interval -= temp * UST_MINUTE;
		if (format == UST_VERBOSE && temp > 0) {
			if (started)
				length = snprintf(tstring, MAX_FORMAT, ", %u %s", (UINT32)temp, (temp == 1 ? "minute" : "minutes"));
			else 
				length = snprintf(tstring, MAX_FORMAT, "%u %s", (UINT32)temp, (temp == 1 ? "minute" : "minutes"));
		} else if (format == UST_TERSE)
			length = snprintf(tstring, MAX_FORMAT, "%02u:", (UINT32)temp);
		if (strlen(string) + length >= n) {
			string[0] = '\0';
			return NULL;
		}
		strcat(string, tstring);
		started = TRUE;
	}

	temp = interval / UST_SECOND;
	interval -= temp * UST_SECOND;

	if (format == UST_VERBOSE && temp > 0) {
		if (started)
			length = snprintf(tstring, MAX_FORMAT, ", %u.%06u", (UINT32)temp, (UINT32)interval);
		else 
			length = snprintf(tstring, MAX_FORMAT, "%u.%06u", (UINT32)temp, (UINT32)interval);
	} else if (format == UST_TERSE)
		length = snprintf(tstring, MAX_FORMAT, "%02u.%06u", (UINT32)temp, (UINT32)interval);

	p = &tstring[strlen(tstring) - 1];
	while (*p == '0') {
		*p-- = '\0';
		length--;
	}
	if (*p == '.') {
		*p-- = '\0';
		length--;
	}

	if (format == UST_VERBOSE && temp > 0) {
		length += snprintf(tstring1, MAX_FORMAT, "%s %s", tstring, (temp == 1 ? "second" : "seconds"));
		strcpy(tstring, tstring1);
	}

	if (strlen(string) + length >= n) {
		string[0] = '\0';
		return NULL;
	}
	if (temp > 0)
		strcat(string, tstring);

	return string;
}

/*---------------------------------------------------------------------*/
INT64 ParseUSTimerInterval(CHAR *string)
{
	CHAR delimiters[] = "//,:;-T ";
	CHAR *token, copy[128];
	INT32 count, ivalue, state;
	REAL64 fvalue;
	INT64 interval = 0;

	if (string == NULL)
		return interval;

	/* Parses strings of the form: "[YY]YY-DDDTHH:MM:SS.SSS" */

	strncpy(copy, string, 127);
	copy[127] = '\0';

	count = 0;
	ivalue = 0;
	fvalue = 0.0;
	token = strtok(copy, delimiters);

	state = 0;
	while (token != NULL) {
		if (count % 2 == 0) {
			ivalue = atoi(token);
			fvalue = atof(token);
			state = 1;
		}
		else {
			switch (*token) {
			  case 'y':
			  case 'Y':
				interval += (INT64)ivalue *UST_YEAR;

				break;
			  case 'd':
			  case 'D':
				interval += (INT64)ivalue *UST_DAY;

				break;
			  case 'h':
			  case 'H':
				interval += (INT64)ivalue *UST_HOUR;

				break;
			  case 'm':
			  case 'M':
				interval += (INT64)ivalue *UST_MINUTE;

				break;
			  default:
				interval += (INT64)(fvalue * (REAL64)UST_SECOND);
				break;
			}
			state = 2;
		}

		count++;
		token = strtok(NULL, delimiters);
	}

	if (state == 1)
		interval += (INT64)(fvalue * (REAL64)UST_SECOND);

	return interval;
}

/*---------------------------------------------------------------------*/
USTIME ParseUSTimeDOY(CHAR *string)
{
	CHAR delimiters[] = "//,:;-Tt ";
	CHAR *token, copy[64];
	INT32 count;
	INT32 year = 1970, doy = 0, hour = 0, minute = 0;
	REAL64 second = 0.0;
	USTIME time = 0;

	if (string == NULL)
		return time;

	/* Parses strings of the form: "[YY]YY-DDDTHH:MM:SS.SSS" */

	strncpy(copy, string, 63);
	copy[63] = '\0';

	count = 0;
	token = strtok(copy, delimiters);

	while (token != NULL) {
		if (count == 0) {
			year = atoi(token);
			if (year < 70)
				year += 1900;
			else if (year < 100)
				year += 2000;
		}
		else if (count == 1) {
			doy = atoi(token);
			if (doy < 0 || doy > 366)
				return time;
		}
		else if (count == 2) {
			hour = atoi(token);
			if (hour < 0 || hour > 23)
				return time;
		}
		else if (count == 3) {
			minute = atoi(token);
			if (minute < 0 || minute > 59)
				return time;
		}
		else if (count == 4) {
			second = atof(token);
			if (second < 0.0 || second >= 61.0)
				return time;
		}

		count++;
		token = strtok(NULL, delimiters);
	}

	time = EncodeUSTime(year, doy, hour, minute, second);

	return time;
}

/*---------------------------------------------------------------------*/
USTIME ParseUSTimeMD(CHAR *string)
{
	CHAR delimiters[] = "//,:;-Tt ";
	CHAR *token, copy[64];
	INT32 count;
	INT32 year = 1970, month = 1, day = 1, hour = 0, minute = 0;
	REAL64 second = 0.0;
	USTIME time = 0;

	if (string == NULL)
		return time;

	/* Parses strings of the form: "[YY]YY-MM-DDTHH:MM:SS.SSS" */

	strncpy(copy, string, 63);
	copy[63] = '\0';

	count = 0;
	token = strtok(copy, delimiters);

	while (token != NULL) {
		if (count == 0) {
			year = atoi(token);
			if (year < 70)
				year += 1900;
			else if (year < 100)
				year += 2000;
		}
		else if (count == 1) {
			month = atoi(token);
			if (month < 1 || month > 12)
				return time;
		}
		else if (count == 2) {
			day = atoi(token);
			if (day < 1 || day > 32)
				return time;
		}
		else if (count == 3) {
			hour = atoi(token);
			if (hour < 0 || hour > 23)
				return time;
		}
		else if (count == 4) {
			minute = atoi(token);
			if (minute < 0 || minute > 59)
				return time;
		}
		else if (count == 5) {
			second = atof(token);
			if (second < 0.0 || second >= 61.0)
				return time;
		}

		count++;
		token = strtok(NULL, delimiters);
	}

	time = EncodeUSTime(year, DayOfYear(year, month, day), hour, minute, second);

	return time;
}

/*---------------------------------------------------------------------*/
/* Module helpers, not externally visible... */
/*---------------------------------------------------------------------*/
static INT32 YearDOYToJdn(INT32 year, INT32 doy)
{
	INT32 jdn, month, day;

	ASSERT(doy >= 0 && doy <= 366);

	MonthDay(&month, &day, year, doy);

	if (year < 0)
		year += 1;

	/* Move Jan. & Feb. to end of previous year */
	if (month <= 2) {
		year -= 1;
		month += 12;
	}

	/* 1461 = (4*365+1) */
	jdn = (INT32)(QFLOOR(1461 * (year + 4712), 4) +
		EndOfMonth[0][month - 1] + day + -QFLOOR(year, 100) + QFLOOR(year, 400) + 2);

	return jdn;
}

/*---------------------------------------------------------------------*/
static VOID JdnToYearDOY(INT32 jdn, INT32 *year, INT32 *doy)
{
	INT32 y, d, temp;

/* Find position within cycles that are nd days long */
#define CYCLE(n, nd) { \
    temp = QFLOOR(d - 1, nd); \
    y += temp * n; \
    d -= temp * nd; \
}

/* The same, with bound on cycle number */
#define LCYCLE(n, nd, l) { \
    temp = QFLOOR(d - 1, nd); \
    if (temp > l) \
        temp = l; \
    y += temp * n; \
    d -= temp * nd; \
}

	y = -4799;
	d = jdn + 31739;						/* JDN -31739 = 31 Dec 48 *01 B.C. */
	CYCLE(400, 146097);						/* Four-century cycle */
	LCYCLE(100, 36524, 3);					/* 100-year cycle */
	CYCLE(4, 1461);							/* Four-year cycle */
	LCYCLE(1, 365, 3);						/* Yearly cycle */
	if (y <= 0)
		y -= 1;

	*year = y;
	*doy = d;

	return;
}

/*---------------------------------------------------------------------*/
#if defined UNIT_TEST
VOID CatchSignal(int sig);
BOOL SetSignalHandler(void);

static BOOL quit = FALSE;

int main(int argc, char *argv[])
{
	CHAR string[MAX_FORMAT + 1];
	INT32 year, doy, month, day, hour, minute;
	REAL64 second;
	UINT64 interval;
	USTIME ustime;
	USTIMER timer;

	if (!SetSignalHandler())
		exit(1);

	if (argc > 1 ) {
		printf("Parsing: %s\n", argv[1]);
		interval = ParseUSTimerInterval(argv[1]);
		printf("%s\n", FormatUSTimerInterval(string, MAX_FORMAT, interval, UST_TERSE));
		printf("%s\n", FormatUSTimerInterval(string, MAX_FORMAT, interval, UST_VERBOSE));
	}

	if (argc > 2 ) {
		printf("Parsing: %s\n", argv[2]);
		ustime = ParseUSTimeDOY(argv[2]);
		printf("%s\n", FormatUSTime(string, MAX_FORMAT, UST_DATE_TIME_USEC, ustime));
	}

	if (argc > 3 ) {
		printf("Parsing: %s\n", argv[3]);
		ustime = ParseUSTimeMD(argv[3]);
		printf("%s\n", FormatUSTime(string, MAX_FORMAT, UST_DATE_TIME_USEC, ustime));
	}

	printf("Epoch: must be 1970:001:00:00:00.000000\n");
	ustime = 0;
#if defined WIN32
	printf("%I64X, %I64d\n", ustime, ustime);
#else
	printf("%LX, %Ld\n", ustime, ustime);
#endif
	printf("%s\n", FormatUSTime(string, MAX_FORMAT, UST_DATE_TIME_USEC, ustime));

	printf("Undefined or void time value:\n");
	//ustime = 9223372036854775807;
	ustime = 0x7FFFFFFFFFFFFFFF;
#if defined WIN32
	printf("%I64X, %I64d\n", ustime, ustime);
#else
	printf("%LX, %Ld\n", ustime, ustime);
#endif
	printf("%s\n", FormatUSTime(string, MAX_FORMAT, UST_DATE_TIME_USEC, ustime));

	printf("Latest representable time:\n");
	//ustime = 9223372036854775806;
	ustime = 0x7FFFFFFFFFFFFFFE;
#if defined WIN32
	printf("%I64X, %I64d\n", ustime, ustime);
#else
	printf("%LX, %Ld\n", ustime, ustime);
#endif
	printf("%s\n", FormatUSTime(string, MAX_FORMAT, UST_DATE_TIME_USEC, ustime));

	ustime = SystemUSTime();
	printf("Current time in all formats:\n");
	printf("%s\n", FormatUSTime(string, MAX_FORMAT, UST_DATE, ustime));
	printf("%s\n", FormatUSTime(string, MAX_FORMAT, UST_TIME, ustime));
	printf("%s\n", FormatUSTime(string, MAX_FORMAT, UST_TIME_MSEC, ustime));
	printf("%s\n", FormatUSTime(string, MAX_FORMAT, UST_TIME_USEC, ustime));
	printf("%s\n", FormatUSTime(string, MAX_FORMAT, UST_DATE_TIME, ustime));
	printf("%s\n", FormatUSTime(string, MAX_FORMAT, UST_DATE_TIME_MSEC, ustime));
	printf("%s\n", FormatUSTime(string, MAX_FORMAT, UST_DATE_TIME_USEC, ustime));

	DecodeUSTime(ustime, &year, &doy, &hour, &minute, &second);
	printf("Decode: %d %d %d %d %lf\n", year, doy, hour, minute, second);
	MonthDay(&month, &day, year, doy);
	printf("month/day: %d %d\n", month, day);

	printf("doy: %d\n", DayOfYear(year, month, day));

	ustime = EncodeUSTime(year, doy, hour, minute, second);
	printf("Encode: %s\n", FormatUSTime(string, MAX_FORMAT, UST_DATE_TIME_USEC, ustime));

	printf("Check leap years, 1900 is not, 2000 is\n");
	for (year = 1896; year <= 1904; year++)
		printf("%04d %s\n", year, (IsLeapYear(year) ? "<- leap year" : ""));
	for (year = 1996; year <= 2004; year++)
		printf("%04d %s\n", year, (IsLeapYear(year) ? "<- leap year" : ""));

	year = 1900;
	ustime = EncodeUSTime(year, DayOfYear(year, 2, 29), 0, 0, 0.0);
	printf("Encode Feb 29 1900: %s\n", FormatUSTime(string, MAX_FORMAT, UST_DATE_TIME, ustime));
	year = 2000;
	ustime = EncodeUSTime(year, DayOfYear(year, 2, 29), 0, 0, 0.0);
	printf("Encode Feb 29 2000: %s\n", FormatUSTime(string, MAX_FORMAT, UST_DATE_TIME, ustime));

	year = -1;
	ustime = EncodeUSTime(year, DayOfYear(year, 12, 31), 23, 59, 59.999999);
	printf("No year 0000: %s + 1 usec = ", FormatUSTime(string, MAX_FORMAT, UST_DATE_TIME_USEC, ustime));
	ustime += 1;
	printf("%s\n", FormatUSTime(string, MAX_FORMAT, UST_DATE_TIME_USEC, ustime));

	interval = UST_SECOND * 10;
	printf("Starting a timer with %u second interval...\n", (UINT32)(interval / UST_SECOND));
	StartUSTimer(&timer, interval);
	while (!quit) {
		if (USTimerIsExpired(&timer)) {
			printf("\rTimer expired at %s, resetting\n", FormatUSTimerInterval(string, 256, USTimerElapsed(&timer),
					UST_VERBOSE));
			ResetUSTimer(&timer, interval);
		}
		printf("\r%s", FormatUSTimerInterval(string, 256, USTimerElapsed(&timer), UST_TERSE));
		fflush(stdout);
		USSleep(UST_MSECOND * 50);
	}

#if defined WIN32
	printf("Raw: %I64d\n", USTimerElapsed(&timer));
#else
	printf("Raw: %Ld\n", USTimerElapsed(&timer));
#endif
	printf("Terse: %s\n", FormatUSTimerInterval(string, 256, USTimerElapsed(&timer), UST_TERSE));
	printf("Verbose: %s\n", FormatUSTimerInterval(string, 256, USTimerElapsed(&timer), UST_VERBOSE));

	interval = UST_SECOND * 10;
	printf("Starting a timer with %u second interval...\n", (UINT32)(interval / UST_SECOND));
	StartUSTimer(&timer, interval);
	printf("Waiting for time to expire...\n");
	USTimerWait(&timer);
	printf("Timer has expired...\n");

	exit(0);
}

/*--------------------------------------------------------------------- */
VOID CatchSignal(int sig)
{
	switch (sig) {
	  case SIGTERM:
		fprintf(stderr, "\nCaught SIGTERM! \n");
		quit = TRUE;
		break;
	  case SIGINT:
		fprintf(stderr, "\nCaught SIGINT! \n");
		quit = TRUE;
		break;
	  default:
		fprintf(stderr, "\rCaught unexpected signal %d, ignored! \n", sig);
		break;
	}

	signal(sig, CatchSignal);
}

/*--------------------------------------------------------------------- */
BOOL SetSignalHandler(void)
{
	if (signal(SIGTERM, CatchSignal) == SIG_ERR) {
		perror("signal(SIGTERM)");
		return FALSE;
	}
	if (signal(SIGINT, CatchSignal) == SIG_ERR) {
		perror("signal(SIGINT)");
		return FALSE;
	}

	return TRUE;
}

#endif
