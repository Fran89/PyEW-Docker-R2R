/*********************************************************************
*							SaraAdSend.c					         *
*																	 *
*	         Digitizing program for the SADC20 A/D Board             *
*            by SARA di Mariotti Gabriele & C s.n.c. s	             *
*********************************************************************/

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <process.h>
#include "earthworm.h"
#include "transport.h"
#include "trace_buf.h"
#include "SaraAdSend.h"

#define CHECK_PC_TIME_SEC		10
#define TIME_ERR_COUNT			5

/* Global variables */
SHM_INFO OutRegion;			// Info structure for output region
pid_t MyPid;				// process id, sent with heartbeat
HANDLE hPort = 0;			// Handle to the comm port

int sampleNumber;			// current number of samples	
SYSTEMTIME bufferTime;		// time of first sample in the buffer

BYTE inbuff[8192], *pInBuff, sampleData[8192], *pSample;
int inCount, inSync, inState, saveNum;
int sampleCnt, checkSps, doneFlag;

int traceBufSize;	  		// Size of the trace buffer, in bytes
long *dataBuf;				// Pointer to received A/D data
long *lTraceDat;			// Where the data points are stored in the trace msg
short *sTraceDat;			// Where the data points are stored in the trace msg
char *traceBuf;				// Where the trace message is assembled
unsigned samples = 0;		// Number of samples received from the ADC board
unsigned char InstId;		// Installation id placed in the trace header
MSG_LOGO logo;		 		// Logo of message to put out
TRACE2_HEADER *traceHead; 	// Where the trace header is stored
double Tbuf; 				// Time read-write structure
long *lTracePtr;	 	    // Pointer into traceBuf for demuxing
short *sTracePtr;	 		// Pointer into traceBuf for demuxing
long *lDataPtr;	  			// Long Pointer 

unsigned dataSize;			// Size of dataBuf in number of samples

int checkTimeCount;			// check time every 60 seconds
int checkTimeFlag;			// Used to check the time after setting the time
time_t checkSetTime;		// Time to test too

double lastTimeErr;
double lastTimeErrDsp;
double timeErrAvg;
int timeErrCount;
int lastTimeErrSet;
int cmdsSent;

int recordChannels;

void ProcessSara16( BYTE data )
{
	
	if( !inSync )  {
		if( !inState )  {
			if( data != 0x81 )  {
				return;
			}
			inState = 1;
			saveNum = 8;
			pSample = sampleData;
			*pSample++ = data;
			return;
		}
		if( inState == 1 || inState == 3 || inState == 5 || inState == 7 )  {
			*pSample++ = data;
			--saveNum;
			if( !saveNum )  {
				++inState;
				saveNum = 3;
			}
			return;
		}
		if( inState == 2 )  {
			if( data != 0x82 )  {
				inState = 0;
				return;
			}
			*pSample++ = data;
			--saveNum;
			inState = 3;
			saveNum = 3;
			return;
		}
		if( inState == 4 )  {
			if( data != 0x83 )  {
				inState = 0;
				return;
			}
			*pSample++ = data;
			--saveNum;
			inState = 5;
			saveNum = 3;
			return;
		}
		if( inState == 6 )  {
			if( data != 0x84 )  {
				inState = 0;
				return;
			}
			*pSample++ = data;
			inState = 7;
			saveNum = 3;
			return;
		}
		if( inState == 8 )  {
			if( data != 0x85 )  {
				inState = 0;
				return;
			}
			*pSample++ = data;
			inState = 9;
			saveNum = 3;
			return;
		}
		if( inState == 9 )  {
			*pSample++ = data;
			--saveNum;
			if( !saveNum )  {
				++inState;
				inSync = 1;
				inState = 10;
				NewSampleData16();
				pSample = sampleData;
				sampleCnt = 1;
			}
			return;
		}
	}
	else  {
		if( !saveNum )  {
			if( data == 0x82 )  {
				saveNum = 16;
				++sampleCnt;
			}
			else if( data == 0x81 )  {
				if( checkSps )  {
					if( sampleCnt != ChanRate )  {
						logWrite("Sample Rate Error");
						SetSpsRate( ChanRate );
						inSync = inState = inCount = 0;
						FlushComm();
						return;
					}
					checkSps = 0;
				}
				saveNum = 25;
				sampleCnt = 1;
			}
			else  {
				if( cmdsSent && data == 0xf8 )  {
					--cmdsSent;
					return;
				}
				logWrite("Comm Sync Error - Data = %x", data );
				inSync = inState = 0;
				return;
			}
			NewSampleData16();
			pSample = sampleData;
		}
		*pSample++ = data;
		--saveNum;
	}		
}		
		
void ProcessSara24( BYTE data )
{
	if( !inSync )  {
		if( !inState )  {
			if( data != 0x81 )
				return;
			inState = 1;
			saveNum = 8;
			pSample = sampleData;
			*pSample++ = data;
			return;
		}
		if( inState == 1 || inState == 3 )  {
			*pSample++ = data;
			--saveNum;
			if( !saveNum )  {
				++inState;
				saveNum = 5;
			}
			return;
		}
		if( inState == 2 )  {
			if( data != 0x82 )  {
				inState = 0;
				return;
			}
			*pSample++ = data;
			--saveNum;
			inState = 3;
			saveNum = 4;
			return;
		}
		if( inState == 4 )  {
			if( data != 0x83 )  {
				inState = 0;
				return;
			}
			*pSample++ = data;
			--saveNum;
			inState = 5;
			saveNum = 4;
			return;
		}
		if( inState == 5 )  {
			*pSample++ = data;
			--saveNum;
			if( !saveNum )  {
				++inState;
				saveNum = 4;
			}
			return;
		}
		if( inState == 6 )  {
			if( data != 0x84 )  {
				inState = 0;
				return;
			}
			*pSample++ = data;
			--saveNum;
			inState = 7;
			saveNum = 4;
			return;
		}
		if( inState == 7 )  {
			*pSample++ = data;
			--saveNum;
			if( !saveNum )  {
				inSync = 1;
				inState = 8;
				NewSampleData24();
				pSample = sampleData;
				sampleCnt = 1;
			}
			return;
		}
	}
	else  {
		if( !saveNum )  {
			if( data == 0x82 )  {
				saveNum = 15;
				++sampleCnt;
			}
			else if( data == 0x81 )  {
				if( checkSps )  {
					if( sampleCnt != ChanRate )  {
						logWrite("Sample Rate Error");
						SetSpsRate( ChanRate );
						inSync = inState = inCount = 0;
						FlushComm();
						return;
					}
					checkSps = 0;
				}
				saveNum = 24;
				sampleCnt = 1;
			}
			else  {
				if( cmdsSent && data == 0xf8 )  {
					--cmdsSent;
					return;
				}
				inSync = inState = 0;
				logWrite("Comm Sync Error - Data = %x", data );
				return;
			}
			NewSampleData24();
			pSample = sampleData;
		}
		*pSample++ = data;
		--saveNum;
	}
}

/* Process the incomming data */
int ProcessData()
{
	DWORD rdlen, rdnum, dwErrFlags;
	COMSTAT ComStat;
	BYTE data;
	
	inSync = inState = doneFlag = 0;
	pSample = sampleData;
	checkSps = TRUE;
			
	while( !doneFlag )  {
		if( !inCount )  {
			if( tport_getflag( &OutRegion ) == TERMINATE  || tport_getflag( &OutRegion ) == MyPid )
				return 0;
			ClearCommError( hPort, &dwErrFlags, &ComStat );
			if( ! ( rdlen = ComStat.cbInQue ) )  {
				_sleep( 20 );
				continue;
			}	
			if(rdlen > 8192)
				rdlen = 8192;
		
			ReadFile(hPort, inbuff, rdlen, &rdnum, NULL);
			inCount = rdlen;
			pInBuff = inbuff;
		}	
	
		while( inCount )  {
			if( !AdcBoardType )
				ProcessSara24( *pInBuff++ );
			else
				ProcessSara16( *pInBuff++ );
			--inCount;
		}
	}
	return 0;
}

void SendDataToRing()
{
	int i, j, rc, ms, sec;
	double last, diff;
	time_t tm;
		
	samples += (unsigned)(ChanRate * Nchan); 	/* Update the samples count */
	
	tm = SystemTimeToTime( &bufferTime );
	
	SetCurPos( 9, 7 );
	for ( i = 0; i < 10; i++ )
		putchar( ' ' );
	SetCurPos( 9, 7 );
	printf( "%u", samples );
		
	Tbuf = (double)tm + 11676096000.0; 
	Tbuf += (double)bufferTime.wMilliseconds / 1000.0;
	
	SetCurPos( 27, 3 );
	PrintGmtime( Tbuf - 11676096000.0, 3 );
		
	SetCurPos( 15, 4 );
	printf( "Sending Data..." );
		
	if( lastTimeErrSet && ( lastTimeErr != lastTimeErrDsp ) )  {
		SetCurPos( 35, 6 );
		for ( i = 0; i < 10; i++ )
			putchar( ' ' );
		SetCurPos( 27, 6 );
		if( lastTimeErr < 0.0 )  {
			last = fabs( lastTimeErr );
			printf("-%d.%03d Seconds", (int)(last / 1000.0), (int)last % 1000 );
		}
		else
			printf("%d.%03d Seconds", (int)(lastTimeErr / 1000.0), (int)lastTimeErr % 1000 );
		lastTimeErrDsp = lastTimeErr;
	}	
	
  	/* Position the screen cursor for error messages */
	SetCurPos( 0, 0 );
	  
  	/* Loop through the trace messages to be sent out */
	for ( i = 0; i < Nchan; i++ )  {
  		/* Do not send messages for unused channels */
	  	if ( strlen( ChanList[i].sta  ) == 0 && strlen( ChanList[i].net  ) == 0 &&
			strlen( ChanList[i].comp ) == 0 )
		  continue;
		  
	  	/* Fill the trace buffer header */
		traceHead->nsamp = ChanRate;				// Number of samples in message
		traceHead->samprate = ChanRate;			  	// Sample rate; nominal
		traceHead->version[0] = TRACE2_VERSION0;     // Header version number
		traceHead->version[1] = TRACE2_VERSION1;     // Header version number
	  	traceHead->quality[0] = '\0';				// One bit per condition
	  	traceHead->quality[1] = '\0';				// One bit per condition
		  
		if( AdcDataSize == 4 )
		  	strcpy( traceHead->datatype, "i4" );		// Data format code
		else
		  	strcpy( traceHead->datatype, "i2" );		// Data format code
	  	strcpy( traceHead->sta,  ChanList[i].sta ); // Site name
	  	strcpy( traceHead->net,  ChanList[i].net ); // Network name
	  	strcpy( traceHead->chan, ChanList[i].comp ); // Component/channel code
        strcpy( traceHead->loc,  ChanList[i].loc );  // Location code
	  	traceHead->pinno = i+1;						// Pin number
		  
	  	/* Set the trace start and end times. Times are in seconds since midnight 1/1/1970 */
	  	traceHead->starttime = Tbuf - 11676096000.;
	  	traceHead->endtime	= traceHead->starttime + (double)(ChanRate - 1) / (double)ChanRate;
		  
	  	/* Set error bits in buffer header */
		if( !TimeRefType )
			traceHead->quality[0] |= TIME_TAG_QUESTIONABLE;
		  
	  	/* Transfer samples from dataBuf to traceBuf */
	  	lDataPtr  = &dataBuf[i];
		if( AdcDataSize == 4 )  {
		  	lTracePtr = &lTraceDat[0];
		  	for ( j = 0; j < ChanRate; j++ )  {
				*lTracePtr++ = *lDataPtr;
			  	lDataPtr += Nchan;
		  	}
		}		  
		else  {
		  	sTracePtr = &sTraceDat[0];
		  	for ( j = 0; j < ChanRate; j++ )  {
				*sTracePtr++ = (short)*lDataPtr;
			  lDataPtr += Nchan;
		  	}
		}		  
	  	rc = tport_putmsg( &OutRegion, &logo, traceBufSize, traceBuf );
	  	if ( rc == PUT_TOOBIG )
			logWrite( "Trace message for channel %d too big", i );
	  	else if ( rc == PUT_NOTRACK )
		  	logWrite( "Tracking error while sending channel %d", i );
	}
	SetCurPos( 0, 0 );
}

int FixSample24( BYTE chanId, BYTE *pData, long *to )
{
	long sample;
	BYTE *lp = (BYTE *)&sample;
	
	/* Check the sample ID and end byte */
	if( chanId != pData[0] )  {
		logWrite("A/D Channel ID Error.");
		return 0;
	}
	if( ( pData[4] & 0xf8 ) != 0xf8 )  {
		logWrite("A/D Channel End Error.");
		return 0;
	}

	/* Add the eighth bit from the last byte of the sample data */
	if( pData[4] & 0x01 )
		pData[1] |= 0x80;
	if( pData[4] & 0x02 )
		pData[2] |= 0x80;
	if( pData[4] & 0x04 )
		pData[3] |= 0x80;
	
	/* Move the data to a long integer */
	lp[0] = pData[1]; lp[1] = pData[2]; lp[2] = pData[3];
	
	/* sign extend the 24 bit data */
	if( lp[2] & 0x80 )
		lp[3] = 0xff;
	else
		lp[3] = 0;		
	
	if( sampleNumber < dataSize )  {
		*to = sample;
		++sampleNumber;
	}
	return 1;	
}

int FixSample16( BYTE chanId, BYTE *pData, long *to )
{
	long sample;
	BYTE *lp = (BYTE *)&sample;
	
	/* Check the sample ID and end byte */
	if( chanId != pData[0] )  {
		logWrite("A/D Channel ID Error.");
		return 0;
	}
	if( ( pData[3] & 0xf0 ) != 0xf0 )  {
		logWrite("A/D Channel End Error.");
		return 0;
	}

	/* Add the eighth bit from the last byte of the sample data */
	if( pData[3] & 0x01 )
		pData[1] |= 0x80;
	
	if( pData[3] & 0x02 )
		pData[2] |= 0x80;
	
	/* Move the data to a long integer */
	lp[0] = pData[1]; 
	lp[1] = pData[2]; 
	
	/* sign extend the 24 bit data */
	if( lp[1] & 0x80 )  {
		lp[2] = 0xff;
		lp[3] = 0xff;
	}
	else  {
		lp[2] = 0;		
		lp[3] = 0;		
	}
		
	if( sampleNumber < dataSize )  {
		*to = sample;
		++sampleNumber;
	}
	return 1;	
}

BOOL GoodPacketTime( BYTE *packetTime )
{
	SYSTEMTIME st;
	int diff;
	
	MakePacketTime( &st, packetTime );
	
	diff = SystemTimeToTime( &st ) - ( checkSetTime + 1 );

	if( abs( diff ) > 2 )  {
		logWrite("Start Time Diff Error: %d Seconds", diff );
		return 0;				// return error
	}
	return 1;					// return ok
}

void ResyncAdcTime( double diff )
{
	logWrite("Reset ADC Time - Error = %g milliseconds", diff );
	SetDateTime( 1 );
	checkTimeCount = 0;
	lastTimeErr = timeErrAvg = 0.0;
	timeErrCount = lastTimeErrSet = 0;
}

void CheckPcTime()
{
	SYSTEMTIME now;
	ULARGE_INTEGER st, ad;
	FILETIME ft;
	double diff;
	char astr[16], sstr[16];
		
	GetSystemTime( &now );
	SystemTimeToFileTime( &now, &ft);
	st.HighPart = ft.dwHighDateTime;
	st.LowPart = ft.dwLowDateTime;

	SystemTimeToFileTime( &bufferTime, &ft);
	ad.HighPart = ft.dwHighDateTime;
	ad.LowPart = ft.dwLowDateTime;
	
	diff = ( (double)st.QuadPart - (double)ad.QuadPart ) / 10000.0;
	
	timeErrAvg += diff;
	if( ++timeErrCount < TIME_ERR_COUNT )
		return;

	lastTimeErr = timeErrAvg / (double)timeErrCount;
	lastTimeErrSet = TRUE;
	timeErrCount = 0;
	timeErrAvg = 0.0;
	if( !TimeRefType && ( fabs( lastTimeErr ) > (double)TimeSyncError ) )  {	// if using PC
		logWrite( "Time Diff = %g", diff );
		ResyncAdcTime( diff );
	}
}

/* Process new sample data */
void NewSampleData24()
{
	BYTE *pData;
	
	if( sampleData[0] == 0x81 )  {		// if time data skip to first sample
		if( checkTimeFlag )  {
			if( !GoodPacketTime( &sampleData[1] ) )  {
				logWrite("Resetting Adc Time");
				SetDateTime( TRUE );			// true = wait for top of second
			}
			else
				checkTimeFlag = 0;
			return;								// wait for time to be OK before processing data
		}
		MakePacketTime( &bufferTime, &sampleData[1] );
		pData = &sampleData[9];
		if( ++checkTimeCount > CHECK_PC_TIME_SEC )  {	// check the time every minute
			checkTimeCount = 0;
			CheckPcTime();
		}
		sampleNumber = 0;
		Heartbeat();
	}
	else
		pData = sampleData;
	if( !FixSample24( 0x82, pData, &dataBuf[sampleNumber] ) )
		return;
	pData += 5;
	if( !FixSample24( 0x83, pData, &dataBuf[sampleNumber] ) )
		return;
	pData += 5;
	if( !FixSample24( 0x84, pData, &dataBuf[sampleNumber] ) )
		return;
		
	if( sampleNumber >= dataSize )
		SendDataToRing();
}

/* Process new sample data */
void NewSampleData16()
{
	BYTE *pData;
	
	if( sampleData[0] == 0x81 )  {		// if time data skip to first sample
		if( checkTimeFlag )  {
			if( !GoodPacketTime( &sampleData[1] ) )  {
				logWrite("Resetting Adc Time");
				SetDateTime( TRUE );			// true = wait for top of second
			}
			else
				checkTimeFlag = 0;
			return;								// wait for time to be OK before processing data
		}
		MakePacketTime( &bufferTime, &sampleData[1] );
		pData = &sampleData[9];
		if( ++checkTimeCount > CHECK_PC_TIME_SEC )  {	// check the time every minute
			checkTimeCount = 0;
			CheckPcTime();
		}
		sampleNumber = 0;
		Heartbeat();
	}
	else
		pData = sampleData;
	
	if( !FixSample16( 0x82, pData, &dataBuf[sampleNumber] ) )
		return;
	pData += 4;
	if( !FixSample16( 0x83, pData, &dataBuf[sampleNumber] ) )
		return;
	pData += 4;
	if( !FixSample16( 0x84, pData, &dataBuf[sampleNumber] ) )
		return;
	pData += 4;
	if( !FixSample16( 0x85, pData, &dataBuf[sampleNumber] ) )
		return;
		
	if( sampleNumber >= dataSize )
		SendDataToRing();
}

BOOL InitDAQ()
{
	if(!OpenPort( CommPort, PortSpeed, &hPort ) )  {
		logit( "e", "Error opening Comm Port.\n" );
		return 0;
	}
	
	SetSpsRate( ChanRate );		// send twice to make sure board gets the command
	_sleep( 100 );
	SetSpsRate( ChanRate );
	_sleep( 100 );
	SetGmtCorrection( (char)GmtCorrection  );
	_sleep( 100 );
	if( !TimeRefType )  {
		SetDateTime( TRUE );			// true = wait for top of second
		_sleep( 100 );
	}
	FlushComm();
	checkTimeFlag = TRUE;
	checkTimeCount = 0;
	timeErrAvg = 0.0;
	timeErrCount = 0;
	lastTimeErr = 0.0;	
	lastTimeErrSet = 0;
	cmdsSent = 0;
	return TRUE;	
}

void StopDAQ()
{
	ClosePort();
}

void ReportError( int errNum, char *errmsg )
{
	int lineLen;
	time_t time_now;			// The current time
	static MSG_LOGO	logo;		// Logo of error messages
	static int first = TRUE;	// TRUE the first time this function is called
	char outmsg[100];			// To hold the complete message
	static time_t time_prev;	// When Heartbeat() was last called

	/* Initialize the error message logo */
	if ( first )  {
		GetLocalInst( &logo.instid );
		logo.mod = ModuleId;
		GetType( "TYPE_ERROR", &logo.type );
		first = FALSE;
	}

	/* Encode the output message and send it */
	time( &time_now );
	sprintf( outmsg, "%d %d ", (long)time_now, errNum );
	strcat( outmsg, errmsg );
	lineLen = strlen( outmsg );
	tport_putmsg( &OutRegion, &logo, lineLen, outmsg );
}

void Heartbeat( void )
{
	long msgLen;			  	// Length of the heartbeat message
	char msg[40];			 	// To hold the heartbeat message
	static int first = TRUE;	// 1 the first time Heartbeat() is called
	static time_t time_prev;	// When Heartbeat() was last called
	time_t time_now;			// The current time
	static MSG_LOGO	logo;		// Logo of heartbeat messages

	if( !HeartbeatInt )
		return;
		
	/* Initialize the heartbeat variables */
	if ( first )  {
		GetLocalInst( &logo.instid );
		logo.mod = ModuleId;
		GetType( "TYPE_HEARTBEAT", &logo.type );
		time_prev = 0;  // force heartbeat first time thru
		first = FALSE;
	}

	/* Is it time to beat the heart? */
	time( &time_now );
	if ( (time_now - time_prev) < HeartbeatInt )
		return;

	/* It's time to beat the heart */
	sprintf( msg, "%d %d\n", (long)time_now, MyPid );
	msgLen = strlen( msg );

	if ( tport_putmsg( &OutRegion, &logo, msgLen, msg ) != PUT_OK )
		logWrite( "Error Sending Heartbeat" );

	time_prev = time_now;
}

int main( int argc, char *argv[] )
{
	/* Get command line arguments */
	if ( argc < 2 )  {
		printf( "Usage: SaraAdSend <config file>\n" );
		return -1;
	}

	/* Initialize name of log-file & open it */
	logit_init( argv[1], 0, 256, 1 );

	/* Read configuration parameters */
	if ( GetConfig( argv[1] ) < 0 )  {
		logit( "e", "Error reading configuration file. Exiting.\n" );
		return -1;
	}

	/* Set up the logo of outgoing waveform messages */
	if ( GetLocalInst( &InstId ) < 0 )  {
		logit( "e", "Error getting the local installation id. Exiting.\n" );
		return -1;
	}
	logit( "", "Local InstId:	 %u\n", InstId );

	/* Log the configuration file */
	LogConfig();

	/* Get our Process ID for restart purposes */
	MyPid = _getpid();
	if( MyPid == -1 )  {
		logit("e", "Cannot get PID. Exiting.\n" );
		return -1;
	}

	logo.instid = InstId;
	logo.mod = ModuleId;
	GetType( "TYPE_TRACEBUF2", &logo.type );

	if( !AdcBoardType )
		recordChannels = 3;
	else
		recordChannels = 4;
	
	/* Allocate some array space */
	dataSize = (unsigned)(ChanRate * recordChannels);
	dataBuf = (long *) calloc( dataSize, sizeof(long) );
	if ( dataBuf == NULL )  {
		logit( "", "Cannot allocate the A/D buffer\n" );
		return -1;
	}
	if( AdcDataSize == 4 )
		traceBufSize = sizeof(TRACE2_HEADER) + (ChanRate * sizeof(long));
	else
		traceBufSize = sizeof(TRACE2_HEADER) + (ChanRate * sizeof(short));
	traceBuf = (char *) malloc( traceBufSize );
	if ( traceBuf == NULL )  {
		logit( "", "Cannot allocate the trace buffer\n" );
		return -1;
	}
	
	traceHead = (TRACE2_HEADER *) &traceBuf[0];
	if( AdcDataSize == 4 )
		lTraceDat  = (long *) &traceBuf[sizeof(TRACE2_HEADER)];
	else
		sTraceDat  = (short *) &traceBuf[sizeof(TRACE2_HEADER)];

	/* Attach to existing transport ring and send first heartbeat */
	tport_attach( &OutRegion, OutKey );
	logit( "", "Attached to transport ring: %d\n", OutKey );
	if( HeartbeatInt )
		Heartbeat();

	/* Initialize the console display */
	InitCon();

	/* Initialize the ADC board */
	if( !InitDAQ() )  {
		logit( "", "ADC Init board error\n" );
		return -1;
	}
	
	/* Process Incomming Data */
	ProcessData();
	
	/* Clean up and exit program */
	StopDAQ();
	free( dataBuf );
	free( ChanList );
	logit( "", "SaraAdSend terminating.\n" );
	return 0;
}

