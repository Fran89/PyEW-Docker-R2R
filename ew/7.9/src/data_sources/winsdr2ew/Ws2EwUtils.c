/*********************************************************************
*									Ws2EwUtils.c					 *
*																	 *
*********************************************************************/

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include <stdarg.h>
#include "Ws2Ew.h"

struct sockaddr_in saddr;   // Socket address structure

#ifdef WIN32
COORD coordSize;
COORD coordDest = { 0, 0 };
CHAR_INFO charInfo[128];
COORD displaySize;
CONSOLE_SCREEN_BUFFER_INFO csbiInfo; 
HANDLE hStdout = 0;
#endif

extern MSG_LOGO logo;
extern QUEUE tbQ, msgQ;
extern mutex_t tbMx, msgMx, lsMx, rsMx;
extern char messages[ MAX_MESSAGES ][ MESSAGE_LEN ];
extern int numberMessages;

extern char reportString[ MAX_RPT_STR_LEN ];
extern int reportStringLen;

extern UserInfo userInfo[ MAX_CONNECT_USERS ];
extern int activeUsers;
extern mutex_t userMx;

void ParseFirstChanName( UserInfo *pUI )
{
	char *ptr, *to = pUI->who;
	int max = 16;
	
	if( ( ptr = strstr( pUI->settings, "Names: ") ) == NULL )  {
		strcpy( to, "???");
		return;
	}
	ptr += 7;
	while( *ptr && max-- )  {
		if( *ptr == '|' || *ptr == ' ')
			break;
		*to++ = *ptr++;
	}	
	*to = 0;
}

#ifdef WIN32
time_t MakeLTime(int year, int month, int day, int hour, int min, int sec)
{
	struct tm t;
	long tmptz = _timezone;
	int tmpdl = _daylight;
	time_t ret;		

	_timezone = 0;
	_daylight = 0;
	t.tm_wday = t.tm_yday = 0;
	t.tm_isdst = -1;
	t.tm_sec = sec;
	t.tm_min = min;
	t.tm_hour = hour;
	t.tm_mday = day;
	t.tm_mon = month-1;
	t.tm_year = year - 1900;	
	ret = mktime(&t);
	_daylight = tmpdl;
	_timezone = tmptz;
	return(ret);
}
#else
time_t MakeLTime( int year, int mon, int day, int hour, int min, int sec )
{
	struct tm in;

	in.tm_year = year - 1900;
	in.tm_mon = mon-1;
	in.tm_mday = day;
	in.tm_hour = hour;
	in.tm_min = min;
	in.tm_sec = sec;
	in.tm_wday = 0;
	in.tm_isdst = -1;
	in.tm_yday = 0;
	
	return timegm( &in );
}
#endif

/* returns the time as a LONG based on a SYSTEMTIME input structure  */
double CalcPacketTime( SYSTEMTIME *st, int filterDelayMs )
{
	double dTm;
	time_t t = MakeLTime( st->wYear, st->wMonth, st->wDay, st->wHour, st->wMinute, st->wSecond );
	dTm = (double)t + ( (double)st->wMilliseconds / 1000.0 );
	if( filterDelayMs )
		dTm -= ( (double)filterDelayMs * 0.001 );
	return dTm;
}

void ReportError( int errNum, char *errmsg, ... )
{
	int lineLen;
	time_t time_now;			// The current time
	static MSG_LOGO	logo;		// Logo of error messages
	static int first = TRUE;	// TRUE the first time this function is called
	char tmp[256], outmsg[256];	// To hold the complete message
	static time_t time_prev;	// When Heartbeat() was last called
	va_list args;

	va_start( args, errmsg );
	vsprintf( tmp, errmsg, args );
	va_end( args );
	
	/* Initialize the error message logo */
	if ( first )  {
		GetLocalInst( &logo.instid );
		logo.mod = ModuleId;
		GetType( "TYPE_ERROR", &logo.type );
		first = FALSE;
	}

	/* Encode the output message and send it */
	time( &time_now );
	sprintf( outmsg, "%d %d %s", (int)time_now, errNum, tmp );
	lineLen = strlen( outmsg );
	tport_putmsg( &OutRegion, &logo, lineLen, outmsg );
}

void Heartbeat( time_t now )
{
	long msgLen;				// Length of the heartbeat message
	char msg[40];				// To hold the heartbeat message
	static int first = TRUE;	// 1 the first time Heartbeat() is called
	static time_t time_prev;	// When Heartbeat() was last called
	static MSG_LOGO	logo;		// Logo of heartbeat messages

	/* Initialize the heartbeat variables */
	if ( first )  {
		GetLocalInst( &logo.instid );
		logo.mod = ModuleId;
		GetType( "TYPE_HEARTBEAT", &logo.type );
		time_prev = 0;  // force heartbeat first time thru
		first = FALSE;
	}

	/* Is it time to beat the heart? */
	if ( (now - time_prev) < HeartbeatInt )
		return;

	/* It's time to beat the heart */
	sprintf( msg, "%d %d\n", (int)now, MyPid );
	msgLen = strlen( msg );

	if ( tport_putmsg( &OutRegion, &logo, msgLen, msg ) != PUT_OK )
		LogWrite( "", "Error sending heartbeat!\n" );

	time_prev = now;
}

BYTE CalcCRC( BYTE *cp, int cnt )
{
	BYTE crc = 0;
	while(cnt--)
		crc ^= *cp++;
	return(crc);
}

int InitSocket()
{
	struct hostent *hp;
	int addr;

	SocketSysInit();   /* This exits on failure */

	/* Set the socket address structure */
	memset(&saddr, 0, sizeof(struct sockaddr_in));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons( (unsigned short)Port );

	if( ( addr = inet_addr( Host ) ) != INADDR_NONE )
		saddr.sin_addr.s_addr = addr;
	else  {
		if( ( hp = gethostbyname( Host ) ) == NULL)  {
			LogWrite( "e", "Invalid Host Address <%s>\n", Host );
			return FALSE;
		}
	  	memcpy((void *) &saddr.sin_addr, (void*)hp->h_addr, hp->h_length);
	}
	return TRUE;
}

void LogWrite( char *type, char *pszFormat, ...)
{
	char buff[ 1024 ], outStr[ 1024 ], timeStr[ 64 ];
	va_list args;
	struct tm *nt;
	time_t ltime;
	
	time( &ltime );
	nt = localtime( &ltime );
	sprintf( timeStr, "%02d/%02d/%02d %02d:%02d:%02d", nt->tm_mon+1,
		nt->tm_mday, nt->tm_year % 100, nt->tm_hour, nt->tm_min, nt->tm_sec );
	
	va_start( args, pszFormat );
	vsprintf( outStr, pszFormat, args );
	va_end( args );
	
	AddFmtMessage( outStr );
	
	sprintf( buff, "%s %s", timeStr, outStr );
	
	logit( type, "%s", buff );
}

void LogWriteThread( char *pszFormat, ...)
{
	va_list args;
	char buff[256];

	va_start( args, pszFormat );
	vsprintf( buff, pszFormat, args );
	va_end( args );
	
	RequestSpecificMutex( &msgMx );
	enqueue( &msgQ, buff, strlen(buff)+1, logo );
   	ReleaseSpecificMutex( &msgMx );
}

void GmtTimeStr( char *to, double tm, int dec )
{
	int  hsec, ms, whole;
	struct tm *gmt;
	time_t ltime = (time_t)tm;
	char decStr[32];
		
	gmt = gmtime( &ltime );
	
	sprintf( to, "%02d/%02d/%04d %02d:%02d:%02d", gmt->tm_mon+1,
		gmt->tm_mday, gmt->tm_year + 1900, gmt->tm_hour, gmt->tm_min, gmt->tm_sec );
	
	decStr[0] = 0;
	if ( dec == 2 )  {				  	// Two decimal places
		hsec = (int)(100. * (tm - floor(tm)));
		sprintf( decStr, ".%02d", hsec );
	}
	else if ( dec == 3 )  {				// Three decimal places
		double d = tm - floor(tm);
		d *= 1000.0;
	  	d += 0.1;
		sprintf( decStr, ".%03d", (int)d );
	}
	strcat( to, decStr );
}

#ifdef WIN32
void SetCurPos( int x, int y )
{
	COORD coord;

	if( ConsoleDisplay )  {
		coord.X = x;
		coord.Y = y;
		SetConsoleCursorPosition( hStdout, coord );
	}
	return;
}

void InitCon( void )
{
	WORD color;
	COORD coord = {0,0};
	int cnt;
	char tmStr[64];
	DWORD numWritten;
	
	if( ConsoleDisplay )  {
		/* Get the console handle */
		hStdout = GetStdHandle( STD_OUTPUT_HANDLE );
		GetConsoleScreenBufferInfo( hStdout, &csbiInfo );
		displaySize.X = csbiInfo.dwSize.X;
		displaySize.Y = csbiInfo.dwSize.Y;
	
		/* Set foreground and background colors */
		color = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
		FillConsoleOutputAttribute( hStdout, color, 2000, coord, &numWritten );
		SetConsoleTextAttribute( hStdout, color );
		FillConsoleOutputCharacter( hStdout, ' ', displaySize.X * displaySize.Y, coord, &numWritten );
		SetCurPos(0,0);
	}
}
	
void ClearScreen()
{
	if( ConsoleDisplay )  {
		COORD coord = {0,0};
		DWORD numWritten;
		int lines = numberMessages + activeUsers + 6;
		FillConsoleOutputCharacter( hStdout, ' ', displaySize.X * lines, coord, &numWritten );
		SetCurPos(0,0);
	}
}
#else		// dummy functions for linux build

void ClearScreen( void )
{
}
void InitCon( void )
{
}

#endif

void AddFmtStr( char *pszFormat, ... )
{
	va_list ap;
	char line[256];
	int len;
	
	va_start( ap, pszFormat );
	vsprintf( line, pszFormat, ap );
	va_end( ap );
	
	len = strlen( line );
	if( ( reportStringLen + len ) >= MAX_RPT_STR_LEN )
		return;
	strcat( reportString, line );
	reportStringLen += len;
}

void DisplayReport()
{
	MakeReportString();
	ClearScreen();
	printf( "%s", reportString );
}

void AddFmtMessage( char *pszFormat, ... )
{
	va_list ap;
	char buff[2048], out[2048], tmStr[64];
	struct tm *nt;
	time_t ltime;
	int i;
	
	time( &ltime );
	nt = localtime( &ltime );
	sprintf( tmStr, "%02d/%02d/%02d %02d:%02d:%02d", nt->tm_mon+1,
		nt->tm_mday, nt->tm_year % 100, nt->tm_hour, nt->tm_min, nt->tm_sec );
	
	va_start( ap, pszFormat );
	vsprintf( out, pszFormat, ap );
	va_end( ap );
	
	sprintf(buff, "%s %s", tmStr, out );
	
	if( strlen( buff ) >= (MESSAGE_LEN-1) )
		buff[ MESSAGE_LEN-1] = 0;

	if( numberMessages )  {
		for( i = numberMessages; i != 0; i-- )
			strcpy(messages[i], messages[i-1] );
	}
	strcpy( messages[0], buff );
	if( numberMessages < (MAX_MESSAGES-1) )
		++numberMessages;
}

void MakeDiffTimeStr( char *to, time_t tm )
{
	int sec, min, hour, days;
	
	*to = 0;
	if( tm < 0 )
		return;
		
	days = (int)(tm / 86400L);
	tm %= 86400L;
	hour = (int)(tm / 3600L); 
	tm %= 3600L;
	min = (int)(tm / 60L);
	sec = (int)(tm % 60L);
	if(days)
		sprintf(to, "%d %02d:%02d:%02d", days, hour, min, sec);
	else
		sprintf(to, "%02d:%02d:%02d", hour, min, sec);
}

void MakeReportString()
{
	int i, cnt = 0;
	char tmStr[64];
	UserInfo *pUI;
		
	reportString[0] = 0;
	reportStringLen = 0;
		
	if( !ConsoleDisplay )
		AddFmtStr( "-----------------------------------\n" );
	AddFmtStr( "           WinSDR to Earthworm Status\n\n");
	
	AddFmtStr( "Active Users: %d\n", activeUsers );
		
	RequestSpecificMutex(&userMx);
		
	if( ListenMode )  {
		for( i = 0; i != MAX_CONNECT_USERS; i++ )  {
			pUI = &userInfo[ i ];	
			if( !pUI->inUse )
				continue;
			MakeUserStr( pUI );
			if( ++cnt == activeUsers )
				break;
		}
	}
	else if( activeUsers )  {
		pUI = &userInfo[ 0 ];	
		MakeUserStr( pUI );
	}
	
	ReleaseSpecificMutex(&userMx);
	
	if( numberMessages )  {
		AddFmtStr("\nMessages:\n");
		for( i = 0; i != numberMessages; i++ )
			AddFmtStr( messages[i] );
	}
}

void MakeUserStr( UserInfo *pUI )
{
	char tmStr[ 64 ];
	time_t diff = pUI->updateTime - pUI->connectTime;
	ConnectInfo *pCI;
	int count = 0;
	
	strcpy( tmStr, "-----");
	if( pUI->updateTime && pUI->connectTime && ( diff > 0 ) )
		MakeDiffTimeStr( tmStr, diff );
	pCI = pUI->pCI;
	if( pCI )
		count = pCI->connectCount;
	AddFmtStr( "    User: %s Packets: %u ConnectTime: %s ConnectCount: %d\n", 
		pUI->who, pUI->totalRecvPackets, tmStr, count );
}

void ClearUserData()
{
	int i, cnt = 0;
	UserInfo *pUI;
	
	for( i = 0; i != MAX_CONNECT_USERS; i++ )  {
		pUI = &userInfo[ i ];	
		if( !pUI->inUse )
			continue;
		pUI->totalRecvPackets = 0;
		if( ++cnt == activeUsers )
			break;
	}
}
