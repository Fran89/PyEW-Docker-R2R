/* PsnAdUtils.c */

#include "PsnAdSend.h"

/* Global variables */
extern SHM_INFO OutRegion;			// Info structure for output region
extern HANDLE hBoard;				// Handle to the DLL/ADC board
extern HANDLE hStdout;				// The console file handle

extern unsigned samples;			// Number of samples received from the ADC board

#ifdef WIN32
extern COORD displaySize;
extern CONSOLE_SCREEN_BUFFER_INFO csbiInfo; 
#endif

/* Messages to display are placed in the array */
extern char messages[ MAX_MESSAGES+1 ][ MESSAGE_LEN ];
extern int numberMessages;

extern char reportString[ MAX_RPT_STR_LEN ];
extern int reportStringLen;

/* Save time adjustment information to a file. Called when a ADC_SAVE_TIME_INFO message is sent by the DLL */
void SaveTimeInfo( TimeInfo *info )
{
	FILE *fp;
	char fname[ 256 ];
	
	sprintf( fname, "%s_%u.dat", TimeFileName, ModuleId );
	if( ! (fp = fopen( fname, "w") ) )
		return;
	fprintf(fp, "%d %d %d\n", (int)info->addDropFlag, (int)info->addDropCount, (int)info->pulseWidth );
	fclose( fp );	
}

/* Read the time adjustment information from a file. This information is then sent to the DLL.*/
void ReadTimeInfo( TimeInfo *info )
{
	FILE *fp;
	char fname[ 256 ], buff[ 128 ];
	int cnt, flag, addDrop, width;
	
	memset( info, 0, sizeof(TimeInfo) );

	sprintf( fname, "%s_%u.dat", TimeFileName, ModuleId );
	if( !(fp = fopen( fname, "r") ) )
		return;
	fgets( buff, 127, fp );
	fclose( fp );
	cnt = sscanf( buff, "%d %d %d", &flag, &addDrop, &width );
	if( cnt != 3 )
		return;
	info->addDropFlag = flag;
	info->addDropCount = addDrop;
	info->pulseWidth = width;
}

/* returns the time as a LONG based on a SYSTEMTIME input structure  */
double CalcPacketTime( SYSTEMTIME *st )
{
	double dTm;
	time_t t = MakeLTime( st->wYear, st->wMonth, st->wDay, st->wHour, st->wMinute, st->wSecond );
	return (double)t + ( (double)st->wMilliseconds / 1000.0 );
}

void GmtTimeStr( char *to, double tm, int dec )
{
	int  hsec;
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
	
	sprintf(buff, "%s %s\n", tmStr, out );
	
	if( strlen( buff ) >= (MESSAGE_LEN-1) )
		buff[ MESSAGE_LEN-1] = 0;

	if( ConsoleDisplay )  {
		if( numberMessages )  {
			for( i = numberMessages; i != 0; i-- )
				strcpy(messages[i], messages[i-1] );
		}
		strcpy( messages[0], buff );
	}
	else  {
		if( numberMessages >= MAX_MESSAGES )  {
			for( i = 0; i != numberMessages; i++ )
				strcpy(messages[i], messages[i+1] );
			strcpy( messages[numberMessages], buff );
		}
		else
			strcpy( messages[numberMessages], buff );
	}
	
	if( numberMessages < MAX_MESSAGES )
		++numberMessages;
}

void LogWrite(char *type, char *pszFormat, ...)
{
	va_list ap;
	char buff[2048], timestr[32];
	struct tm *nt;
	time_t ltime;
	
	time( &ltime );
	nt = localtime( &ltime );
	sprintf( timestr, "%02d/%02d/%02d %02d:%02d:%02d", nt->tm_mon+1,
		nt->tm_mday, nt->tm_year % 100, nt->tm_hour, nt->tm_min, nt->tm_sec );
	
	va_start( ap, pszFormat );
	vsprintf( buff, pszFormat, ap );
	va_end( ap );
	
	logit( type, "%s %s", timestr, buff );
}

#ifdef WIN32
void SetCurPos( int x, int y )
{
	COORD coord;

	if( !ConsoleDisplay )
		return;
	coord.X = x;
	coord.Y = y;
	SetConsoleCursorPosition( hStdout, coord );
	return;
}

void InitCon( void )
{
	WORD color;
	COORD coord = {0,0};
	int cnt;
	char tmStr[64];
	DWORD numWritten;
	
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
	
void ClearScreen()
{
	COORD coord = {0,0};
	DWORD numWritten;
	int lines = numberMessages + 9;
	FillConsoleOutputCharacter( hStdout, ' ', displaySize.X * lines, coord, &numWritten );
	SetCurPos(0,0);
}
#else		// dummy functions for linux build
void ClearScreen( void )
{
	printf( "%c[2J", 0x1b );
	printf( "%c[H", 0x1b );
}
void InitCon( void )
{
}
#endif

/* Returns TRUE if the Time Reference Type = GPS */
int IsGpsRef()
{
	switch( TimeRefType )  {
		case TIME_REF_GARMIN:
		case TIME_REF_MOT_NMEA:
		case TIME_REF_MOT_BIN:
		case TIME_REF_SKG:
		case TIME_REF_4800:
		case TIME_REF_9600:
			return TRUE;
		default:
			return FALSE;
	}
	return FALSE;
}

#ifndef WIN32
void strupr( char *str )
{
	while( *str )  {
		*str = toupper( *str );
		++str;
	}
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
	return _mkgmtime( &in );
}
#endif

char *GetErrorString( DWORD errNo )
{
	static char errMsg[ 64 ];

	strcpy( errMsg, "???");
	switch( errNo )  {
		case E_OPEN_PORT_ERROR:		strcpy( errMsg, "Error Opening Comm Port" ); return errMsg;
		case E_NO_CONFIG_INFO:		strcpy( errMsg, "No Configure Information" ); return errMsg;
		case E_THREAD_START_ERROR: 	strcpy( errMsg, "Thread Start Error" ); return errMsg;
		case E_NO_HANDLES: 			strcpy( errMsg, "Too Many Used Handles" ); return errMsg;
		case E_BAD_HANDLE: 			strcpy( errMsg, "Bad Handle ID" ); return errMsg;
		case E_NO_ADC_CONTROL: 		strcpy( errMsg, "Internal Error" ); return errMsg;
		case E_CONFIG_ERROR: 		strcpy( errMsg, "Configuration Error" ); return errMsg;
		default: return errMsg;
	}
	return errMsg;
}

void GetRefName( int type, char *to )
{
	*to = 0;
	switch( type )  {
		case TIME_REF_NOT_LOCKED:	strcpy( to, "Not Locked" );
									break;
		case TIME_REF_WAS_LOCKED: 	strcpy( to, "Was Locked" );
									break;
		case TIME_REF_LOCKED: 		strcpy( to, "Locked" );
									break;
		default:		            strcpy( to, "???" );
									break;
	}
}
