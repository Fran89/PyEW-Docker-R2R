/*********************************************************************
*							SaraUtils.c					         *
*																	 *
*	         Digitizing program for the SADC20 A/D Board             *
*            by SARA di Mariotti Gabriele & C s.n.c. s	             *
*********************************************************************/

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <process.h>
#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include "SaraAdSend.h"

static HANDLE outHandle;			// The console file handle
static COORD coordSize, coordDest = { 0, 0 };
static CHAR_INFO charInfo[128];

extern HANDLE hPort;				// Handle to the comm port
extern BYTE inbuff[8192], *pInBuff, sampleData[8192], *pSample;
extern int inCount, inSync, inState, saveNum, cmdsSent;
extern int sampleCnt, checkSps, doneFlag;
extern time_t checkSetTime;

/* Messages to display are placed in this array */
static char messages[ MAX_MESSAGES ][ MESSAGE_LEN ];

/*  Send command to the A/D board to set the GMT Correction value */
void SetGmtCorrection( char correction )
{
	BYTE buff[6] = { 0x82, 0, 0, 0, 0, 0 };
	DWORD sent;
	
	logWrite("GMT Correction: %d", (BYTE)correction );
	buff[1] = correction;
	WriteFile( hPort, buff, 6, &sent, NULL );
}

/* Send command to the A/D board to set the time */
void SetTime( int hour, int min, int sec )
{
	BYTE buff[6] = { 0x83, 0, 0, 0, 0, 0 };
	DWORD sent;
	
	buff[3] = (BYTE)hour;
	buff[2] = (BYTE)min;
	buff[1] = (BYTE)sec;
	WriteFile( hPort, buff, 6, &sent, NULL );
}

/* Send command to the A/D board to set the baud rate */
void SetSpsRate( int sps )
{
	char buff[6] = { 0x84, 0, 0, 0, 0, 0 };
	DWORD sent;
	
	sps = 200 / sps;
	buff[1] = buff[2] = buff[3] = sps;
	if( AdcBoardType )
		buff[4] = sps;
	WriteFile( hPort, buff, 6, &sent, NULL );
}

/* Send command to the A/D board to set the date */
void SetDate( int year, int mon, int day )
{
	BYTE buff[6] = { 0x87, 0, 0, 0, 0, 0 };
	DWORD sent;
	
	buff[1] = (BYTE)(year % 100);
	buff[2] = (BYTE)mon;
	buff[3] = (BYTE)day;
	WriteFile( hPort, buff, 6, &sent, NULL );
}

void SetDateTime( int wait )
{
	SYSTEMTIME st;
	int sec, max = 2000;
		
	logWrite("Setting Date and Time on ADC Board");
	GetSystemTime( &st );
	if( wait )  {
		sec = st.wSecond;		
		while( max-- )  {
			GetSystemTime( &st );
			if( st.wSecond != sec )
				break;
			_sleep(1);
		}
	}
	SetDate( st.wYear, st.wMonth, st.wDay );
	SetTime( st.wHour, st.wMinute, st.wSecond );
	checkSetTime = SystemTimeToTime( &st );
	cmdsSent = 2;
}

void FlushComm()
{
	DWORD rdlen, rdnum, dwErrFlags;
	COMSTAT ComStat;
	char *to;
	
	ClearCommError( hPort, &dwErrFlags, &ComStat );
	if( ! ( rdlen = ComStat.cbInQue ) )
		return;
	if( ! ( to = (char *)malloc( rdlen ) ) )
		return;
	ReadFile(hPort, to, rdlen, &rdnum, NULL);
	free( to );
}

/* Open Comm Port */
int OpenPort( int portNumber, int speed, HANDLE *port )
{
	int ret;
	DCB dcb;
	COMMTIMEOUTS CommTimeOuts;
	HANDLE hPort;
	char portStr[24];
		  
	logWrite("Opening Port %d at %d Baud", CommPort, PortSpeed );

	if( portNumber >= 10 )
		sprintf( portStr, "\\\\.\\COM%d", portNumber );
	else
		sprintf( portStr, "COM%d", portNumber );
	
	hPort = CreateFile(portStr, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if(hPort == (HANDLE)-1)
		return(FALSE);
	
	SetupComm(hPort, 8192, 8192);
	CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = 0;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	CommTimeOuts.WriteTotalTimeoutConstant = 5000;
	SetCommTimeouts(hPort, &CommTimeOuts);
	
	dcb.DCBlength = sizeof(DCB);
	GetCommState(hPort, &dcb);
	
	dcb.BaudRate = speed;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;
	dcb.fOutxDsrFlow = 0;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fOutxCtsFlow = 0;
	dcb.fRtsControl = RTS_CONTROL_DISABLE;
	dcb.fInX = dcb.fOutX = 0;
	dcb.fBinary = TRUE;
	dcb.fParity = TRUE;
	dcb.fDsrSensitivity = 0;
	ret = SetCommState(hPort, &dcb);
	EscapeCommFunction(hPort, CLRDTR);
	*port = hPort;
	inCount = 0;
	pInBuff = inbuff;
	return(ret);
}

/* Close the Comm Port */
void ClosePort()
{
	if(hPort == (HANDLE)-1)
		return;
	PurgeComm(hPort, PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);
	CloseHandle(hPort);
	hPort = (HANDLE)-1;
}

/* Clear to end of line */
void ClearToEOL( int startCol, int startRow )
{
	SMALL_RECT region;
	int cols = coordSize.X - startCol;

	region.Left = startCol;
	region.Right = startCol+cols;
	region.Top = startRow;
	region.Bottom = startRow;
	WriteConsoleOutput( outHandle, &charInfo[0], coordSize, coordDest, &region);
	SetCurPos( startCol, startRow );
}

/* Set the cursor to a location on the screen */
void SetCurPos( int x, int y )
{
	COORD	  coord;

	coord.X = x;
	coord.Y = y;
	SetConsoleCursorPosition( outHandle, coord );
	return;
}

/* Init screen handling */
void InitCon( void )
{
	time_t current_time;
	CHAR_INFO *to = &charInfo[0];
	WORD color;
	COORD coord;
	DWORD numWritten;
	int cnt;

	memset( messages, 0, sizeof( messages ) );
	
	/* Get the console handle */
	outHandle = GetStdHandle( STD_OUTPUT_HANDLE );

	/* Set foreground and background colors */
	color = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
	
	coordSize.X = coordSize.Y = 80;
	coord.X = coord.Y = 0;
	
	FillConsoleOutputAttribute( outHandle, color, 2000, coord, &numWritten );
	SetConsoleTextAttribute( outHandle, color );

	cnt = 127;
	while(cnt--)  {
		to->Char.AsciiChar = ' ';
		to->Attributes = color;
		++to;
	}
	/* Fill in the labels */
	SetCurPos( 30, 0 );
	printf( "Sara Digitizer" );

	SetCurPos( 0, 2 );
	printf( "Program Start Time (UTC): " );
	time( &current_time );
	SetCurPos( 26, 2 );
	PrintGmtime( (double)current_time, 0 );

	SetCurPos( 0, 3 );
	printf( "TraceBuf Start Time (UTC): " );
	
	SetCurPos( 0, 4 );
	printf( "System Status: " );

	SetCurPos( 0, 5 );
	if( !TimeRefType )
		printf( "Time Reference Status: Using PC Time" );
	else
		printf( "Time Reference Status: ???" );

	SetCurPos( 0, 6 );
	printf( "PC to ADC Time Difference: ???" );

	SetCurPos( 0, 7 );
	printf( "Samples:" );
	
	SetCurPos( 0, 9 );
	printf( "Messages:" );

	SetCurPos( 0, 0 );
}

void MakePacketTime( SYSTEMTIME *st, BYTE *packetTime )
{
	memset( st, 0, sizeof( st ) );
	st->wYear = packetTime[0] + 2000;
	st->wMonth = packetTime[1];
	st->wDay = packetTime[2];
	st->wSecond = packetTime[3];
	st->wMinute = packetTime[4];
	st->wHour = packetTime[5];
}

time_t SystemTimeToTime( SYSTEMTIME *st )
{
	struct tm t;
	
	t.tm_wday = t.tm_yday = 0;
	t.tm_isdst = 0;
	t.tm_hour = st->wHour;
	t.tm_min = st->wMinute;
	t.tm_sec = st->wSecond;
	t.tm_mday = st->wDay;
	t.tm_mon = st->wMonth-1;
	t.tm_year = st->wYear - 1900;
	return MakeTimeFromGMT( &t );
}

/* Found this on the Internet. If takes a tm structure and returns time_t based on
   GMT/UTC time. If you use the standard runtime mktime function the returned time_t is based
   on the local system time and includes any TZ and DST adjustments. */
time_t MakeTimeFromGMT( struct tm *t )
{
	time_t tl, tb;
	struct tm *tg;

	tl = mktime( t );
	if( tl == -1 )  {
		t->tm_hour--;
      	tl = mktime( t );
      	if(tl == -1)
			return -1;
      	tl += 3600;
  	}
  	tg = gmtime( &tl );
  	tg->tm_isdst = 0;
  	tb = mktime( tg );
	if(tb == -1)  {
      tg->tm_hour--;
      tb = mktime( tg );
      if( tb == -1 )
			return -1; /* can't deal with output from gmtime */
      tb += 3600;
	}
	return (tl - (tb - tl));
}

/* Print the time at the current screen location */
void PrintGmtime( double tm, int dec )
{
	int  hsec, ms, whole;
	struct tm *gmt;
	time_t ltime = (time_t)tm;
	
	gmt = gmtime( &ltime );
	
	printf( "%02d/%02d/%04d %02d:%02d:%02d", gmt->tm_mon+1,
		gmt->tm_mday, gmt->tm_year + 1900, gmt->tm_hour, gmt->tm_min, gmt->tm_sec );
	
	if ( dec == 2 )  {				  	// Two decimal places
		hsec = (int)(100. * (tm - floor(tm)));
		printf( ".%02d", hsec );
	}
	else if ( dec == 3 )  {				// Three decimal places
		double d = tm - floor(tm);
		d *= 1000.0;
	  	d += 0.1;
		printf( ".%03d", (int)d );
	}
}

void logWrite(char *pszFormat, ...)
{
	int i;
	char buff[256], viewStr[256], sdate[32], stime[32], 
		*pszArguments = (char*)&pszFormat + sizeof( pszFormat );
	
	vsprintf( buff, pszFormat, pszArguments );
	_strdate( sdate );
	sdate[5] = 0;
	_strtime( stime );
	sprintf( viewStr, "%s %s %s", sdate, stime, buff );
	if( strlen(viewStr) >= (MESSAGE_LEN-1) )
		viewStr[ MESSAGE_LEN-1] = 0;
	
	for( i = MAX_MESSAGES-1; i != 0; i-- )
		strcpy(messages[i], messages[i-1] );
	strcpy( messages[0], viewStr );
	for( i = 0; i != MAX_MESSAGES; i++ )
		ClearToEOL( 0, MESSAGE_START_ROW + i );
	for( i = 0; i != MAX_MESSAGES; i++ )  {
		SetCurPos( 0, MESSAGE_START_ROW + i );
		printf("%s", messages[ i ] );
	}
	logit("", "%s\n", viewStr );
}

void TimeStr( char *to, SYSTEMTIME *t )
{
	sprintf( to, "%02d/%02d/%04d %02d:%02d:%02", t->wMonth, t->wDay, t->wYear, t->wHour, t->wMinute, t->wSecond );
}
