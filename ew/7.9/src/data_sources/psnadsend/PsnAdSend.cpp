/***********************************************************************
*					    PsnAdSend.cpp Version 2.x 	                   *
*																	   *
*  Digitizing program for the PSN-ADC-SERIAL or PSN-ADC-USB A/D Board  *
*
************************************************************************/

#include "PsnAdSend.h"

#ifndef WIN32
#include <sys/time.h>
#endif

/* Global variables */
SHM_INFO OutRegion;				// Info structure for demuxed data output region
SHM_INFO MuxRegion;				// Info structure for multiplexed data output region
HANDLE hBoard = 0;				// Handle to the DLL/ADC board
HANDLE hStdout = 0;				// The console file handle
int MyPid;				 		// process id, sent with heartbeat

/* Configuration information sent to the DLL and ADC board */
AdcBoardConfig2 config;

BYTE newDllData[ MAX_CHANNELS * MAX_SPS_RATE * sizeof( LONG ) ];	// multiplexed data here
BYTE newAdcData[ MAX_CHANNELS * MAX_SPS_RATE * sizeof( LONG ) ];	// de-muxed data here
StatusInfo statusInfo;
DataHeader dataHdr;
int adcBoardType = BOARD_UNKNOWN;
unsigned dataSize;			// Size of dataBuf in number of samples
LONG *dataBuf;				// Pointer to received A/D data

unsigned samples = 0;		// Number of samples received from the ADC board
double dTraceTime; 			// Trace buffer time

#ifdef WIN32
COORD displaySize;			// display control stuff
CONSOLE_SCREEN_BUFFER_INFO csbiInfo; 
#endif

TRACE2_HEADER *traceHead2; // Where the trace header is stored

/* Messages to display on the terminal are placed in this array */
char messages[ MAX_MESSAGES+1 ][ MESSAGE_LEN ];
int numberMessages;

/* MakeReportString output buffer */
char reportString[ MAX_RPT_STR_LEN ];
int reportStringLen;

static time_t startTime;

void AtExitHandler();
int needsShutdown = FALSE;

int timeRefStatus;			// current reference time status	
time_t startNotLockTime;	// used to send error message to StatMgr if no GPS lock after 30 min.

int checkSysTime;			// Used to check the system time at startup
int timeSetWait;			// Also used to check the system time at startup

int main( int argc, char *argv[] )
{
	int i, c, num,			// Loop stuff
		traceBufSize;	  	// Size of the trace buffer, in bytes
	LONG *lTraceDat = 0;	// Where the data points are stored in the trace msg
	short *sTraceDat = 0;	// Where the data points are stored in the trace msg
	char *traceBuf;			// Where the trace message is assembled
	unsigned char InstId;	// Installation id placed in the trace header
	MSG_LOGO logo;		 	// Logo of message to put out
	int rc, sts;	  		// Function return code
	short *sTracePtr; 		// Short pointer to more data to tracebuff
	LONG d, *lTracePtr;		// long pointer to more data to tracebuff
	int displayCount = 0;	// when to refresh display
	SCNL *pSCNL;			// Current pointer to SCNL
	int timeout;			// No data timeout counter
	int sendHeartCnt;		// Used to send out heartbeats
	
#ifdef WIN32	
	int dspFlag;
#endif								
	
	/* Get command line arguments */
	if ( argc < 2 )  {
		printf( "Usage: PsnAdSend <config file>\n" );
		return -1;
	}

	time( &startTime );
	dTraceTime = 0.0;
	
	/* Clear out the message buffer */
	memset( messages, 0, sizeof( messages ) );
	numberMessages = 0;
	
	timeRefStatus = TIME_REF_UNKNOWN;
	startNotLockTime = 0;
		
	/* Initialize name of log-file & open it */
	logit_init( argv[1], 0, 256, 1 );

	/* Read configuration parameters */
	if ( GetConfig( argv[1] ) < 0 )  {
		LogWrite( "e", "Error reading configuration file. Exiting.\n" );
		return -1;
	}

	signal( SIGINT, CntlHandler );
	
	/* Set up the logo of outgoing waveform messages */
	if ( GetLocalInst( &InstId ) < 0 )  {
		LogWrite( "e", "Error getting the local installation ID. Exiting.\n" );
		return -1;
	}
	LogWrite( "", "PsnAdSend Version %s\n", VERSION_STRING );
	LogWrite( "", "Local InstId:	 %u\n", InstId );

	/* Log the configuration file */
	LogConfig();

	/* Get our Process ID for restart purposes */
	MyPid = getpid();
	if( MyPid == -1 )  {
		LogWrite("e", "Cannot get PID. Exiting.\n" );
		return -1;
	}

	logo.instid = InstId;
	logo.mod = ModuleId;
	GetType( "TYPE_TRACEBUF2", &logo.type );

	/* Allocate some array space */
	dataSize = ChanRate * TotalChans;
	dataBuf = (LONG *)malloc( dataSize * sizeof( LONG ) );
	if ( dataBuf == NULL )  {
		LogWrite( "", "Cannot allocate the A/D buffer\n" );
		return -1;
	}
	
	if( AdcDataSize == 4 )
		traceBufSize = sizeof(TRACE2_HEADER) + (ChanRate * sizeof(LONG));
	else
		traceBufSize = sizeof(TRACE2_HEADER) + (ChanRate * sizeof(short));
	traceBuf = (char *) malloc( traceBufSize );
	if ( traceBuf == NULL )  {
		LogWrite( "", "Cannot allocate the trace buffer\n" );
		return -1;
	}
	
	traceHead2 = (TRACE2_HEADER *) &traceBuf[0];
	if( AdcDataSize == 4 )
		lTraceDat = (LONG *)&traceBuf[sizeof(TRACE2_HEADER)];
	else
		sTraceDat = (short *)&traceBuf[sizeof(TRACE2_HEADER)];
		
	/* Attach to existing transport rings */
	tport_attach( &OutRegion, OutKey );
	LogWrite( "", "Attached to demultiplexed data transport ring: %d\n", OutKey );
	if( MuxKey )  {
		tport_attach( &MuxRegion, MuxKey );
		LogWrite( "", "Attached to multiplexed data transport ring: %d\n", MuxKey );
	}
	
	/* Initialize the console display, Windows Only */
	if( ConsoleDisplay )
		InitCon();

	/* Initialize the ADC board */
	if( !InitADC() )  {
		LogWrite( "", "ADC Init board error\n" );
		return -1;
	}
	
	if( RefreshTime )
		DisplayReport();
	
	Heartbeat();					// send first heatbeat
	
	timeout = NoDataTimeout * 20;	// calc timeout in seconds based on sleep_ms = 50ms
	sendHeartCnt = 20;				// about every second if sleep_ew = 50ms		
	needsShutdown = TRUE;
	
	checkSysTime = timeSetWait = 0;
	if( IsGpsRef() && StartTimeError )
		checkSysTime = StartTimeError;	
	
	atexit( AtExitHandler );
	
	/************************* The main program loop	*********************/
	while( TRUE )  {
		if( ( sts = tport_getflag( &OutRegion ) ) == TERMINATE )  {
			LogWrite("", "PsnAdSend Got TERMINATE Message\n");
			break;
		}
		if( sts == MyPid )  {
			LogWrite("", "PsnAdSend Got RESTART Message\n");
			break;
		}

		/* Get one second worth of data */
		rc = GetData( dataBuf );
		if( rc == -1 )  {		// check for error	
			LogWrite("e", "GetData() returned -1!\n");
			ReportError( 9, "PsnAdSend Terminated do to getdata() error!");
			break;
		}
		if( rc == 0 )  {		// no new data so do heartbeat and timeout checking
			if( --sendHeartCnt <= 0 )  {
				Heartbeat(); 				/* Beat the heart */
				sendHeartCnt = 20;
			}	
			--timeout;
			if( !timeout )  {
				LogWrite("e", "No Data Timeout!\n");
				ReportError( 2, "No Data Timeout!");
				if( ExitOnTimeout )
					break;
				if( !Restart() )
					break;
				timeout = NoDataTimeout * 20;	// calc timeout in seconds based on sleep_ms = 50ms
			}
			sleep_ew( 50 );
			continue;
		}
		
		// GetData() must have returned 1 so we have more ADC data to send to the ring
		
		timeout = NoDataTimeout * 20;	// reset timeout for next time
		
		samples += dataSize;
		
		DemuxShiftData( dataBuf );		// also applies dcOffset
		
		dTraceTime = CalcPacketTime( &dataHdr.packetTime );
		if( checkSysTime && ( dataHdr.timeRefStatus == TIME_REF_LOCKED ) )
			CheckSetSysTime( dTraceTime );
		
	  	/* Loop through the trace messages to be sent out */
		for ( i = 0; i < TotalChans; i++ )  {
			pSCNL = &ChanList[ i ];
			
			FilterData( pSCNL );		// filter data if needed
			
			/* Calc the trace time based on the packet time and the filter delay for the channel */
			if( pSCNL->filterDelay )
				dTraceTime -= ( (double)pSCNL->filterDelay * 0.001 );
			
			MakeTraceHeader( pSCNL, dTraceTime, &dataHdr );
			
			/* If data size == 32 bits just move the data to the output buffer and */
			/* if data size == 16 bits we need to convert the integer data to short data */
			if( AdcDataSize == 4 )
				memcpy( lTraceDat, pSCNL->dataBuff, ChanRate * sizeof( LONG ) );
			else  {
				sTracePtr = sTraceDat;
				lTracePtr = pSCNL->dataBuff;	
				num = ChanRate;
				while( num-- )  {
					d = *lTracePtr++;
					if( d > 32767 )
						d = 32767;
					else if( d < -32768 )
						d = -32768;
					*sTracePtr++ = (short)d;
				}
			}
			
			/* If it's ok to send the data send it to the ring */
			if( OkToSendData( pSCNL, &dataHdr ) )  {
			  	rc = tport_putmsg( &OutRegion, &logo, traceBufSize, traceBuf );
			  	if ( rc == PUT_TOOBIG )
					LogWrite( "", "Trace message for channel %d too big\n", i );
		  		else if ( rc == PUT_NOTRACK )
			  		LogWrite( "", "Tracking error while sending channel %d\n", i );
			}
		}

#ifdef WIN32	
		/* If enabled check the con input for characters */
		if( CheckStdin )  {
			char ch;
			dspFlag = 0;
			while( kbhit() )  {
				ch = getch();
// 				if( ch == 'e' )	exit(0); 	// used to test restartMe feature
				if( ch == 'c' || ch == 'C' )  {
					memset( messages, 0, sizeof( messages ) );
					numberMessages = 0;
					samples = 0;
				}
				dspFlag = TRUE;
			}
			if( dspFlag )  {
				DisplayReport();
				displayCount = 0;
			}
		}
#endif

		/* If enabled display the current report */
		if( RefreshTime && ( ++displayCount >= RefreshTime ) )  {
			DisplayReport();
			displayCount = 0;
		}
	}

	LogWrite( "", "PsnAdSend terminating.\n" );
	
	Shutdown();
	
	return 0;
}

/* Checks to see if more data or messages are available from the PSNAdBoard DLL/Lib
   Returns: 1 = more data ADC, 0 = no data ADC data and -1 = error */
int GetData( LONG *adcData )
{
	int timeout, boardType, count;
	DWORD type, dataLen, sts, errNo;
	char errMsg[ 128 ];
	LONG *lPtr;
	short *sPtr;
	
	sts = PSNGetBoardData( hBoard, &type, newDllData, newAdcData, &dataLen );
	if( sts == ADC_BOARD_ERROR )
		return -1;		
	if( sts == ADC_NO_DATA )
		return 0;
			
	/* On the first data packet and board type is unknown, get the board type 
	   and call FirstPacketCheck() */
	if( adcBoardType == BOARD_UNKNOWN && type == ADC_AD_DATA )  {
		if( !PSNGetBoardInfo( hBoard, ADC_GET_BOARD_TYPE, &boardType ) || 
				( boardType == BOARD_UNKNOWN ) )  {
			LogWrite( "e", "ADC Board Type Error\n" );
			ReportError( 1, "Config Error: ADC Board Type Error");
			return -1;
		}
		adcBoardType = boardType;
		if( ( boardType == BOARD_VM || boardType == BOARD_SDR24 ) && AdcDataSize == 2 )  {
			LogWrite( "e", "Configuration Error. Must use AdcDataSize of 4 for this ADC board.\n" );
			ReportError( 1, "Config Error: Must use AdcDataSize of 4 for this ADC board");
			return -1;
		}
		
		if( ! FirstPacketCheck( errMsg ) )  {
			LogWrite( "e", errMsg );
			ReportError( 1, errMsg );
			return -1;
		}
	}
		
	/* If we have a data packet return 1 so the data can be sent to the wave ring */
	if( type == ADC_AD_DATA )  {
		memcpy( &dataHdr, newDllData, sizeof( dataHdr ) );
		/* Normalize the data to 32-bits, VoltsMeter and SDR24 boards are all ready 32 bits so 
		   just do a memcpy to the output data buffer. */
		if( adcBoardType == BOARD_VM || adcBoardType == BOARD_SDR24 )
			memcpy( adcData, newAdcData, dataLen );			
		else  {				// Convert 16-bit ADC data to 32 bits
			sPtr = (short *)newAdcData;
			lPtr = adcData;
			count = dataSize;
			while( count-- )
				*lPtr++ = *sPtr++;
		}
		CheckLockStatus( dataHdr.timeRefStatus );
		if( MuxKey != 0 )
			SendMuxedData( &dataHdr, newAdcData, dataLen );
			
		return 1;		// tell the caller we have good data
	}
			
	/* All other messages from the DLL/lib are handled here... */
		
	/* Check for config error message from DLL */
	if( type == ADC_MSG && strstr( (char *)newDllData, "Configuration Error =" ) )  {
		AddFmtMessage( (char *)newDllData );
		LogWrite( "", "DLL/ADC Msg: %s\n", newDllData );
		return -1;
	}	
			
	if( type == ADC_MSG || type == ADC_ERROR || type == ADC_AD_MSG )  {
		if( MuxKey != 0 )
			SendLogMessage( (char *)newDllData );
		CheckForErrors( (char *)newDllData );
		if( LogMessages && !FilterLogMsg( (char *)newDllData ) )  {
			AddFmtMessage( (char *)newDllData );
			LogWrite( "", "DLL/ADC Msg: %s\n", newDllData );
		}
	}
	else if( type == ADC_SAVE_TIME_INFO )
		SaveTimeInfo( (TimeInfo *)newDllData );
	else if( type == ADC_STATUS )
		memcpy( &statusInfo, newDllData, sizeof( statusInfo ) );
	
	return 0;		// We have no new ADC data to send so return 0
}

/* Make the trace buffer header */
void MakeTraceHeader( SCNL *pSCNL, double tm, DataHeader *hdr ) 
{
	traceHead2->nsamp = ChanRate;				// Number of samples in message
	traceHead2->samprate = ChanRate;			// Sample rate; nominal
	traceHead2->version[0] = TRACE2_VERSION0;   // Header version number
	traceHead2->version[1] = TRACE2_VERSION1;   // Header version number
		
  	traceHead2->quality[0] = 0;					// One bit per condition
  	traceHead2->quality[1] = 0;					// One bit per condition
  	if( hdr->timeRefStatus != TIME_REF_LOCKED )
		traceHead2->quality[0] |= TIME_TAG_QUESTIONABLE;
		
	if( AdcDataSize == 4 )
	  	strcpy( traceHead2->datatype, "i4" );	// Data format code
	else
	  	strcpy( traceHead2->datatype, "i2" );	// Data format code
  	strcpy( traceHead2->sta,  pSCNL->sta ); 	// Site name
  	strcpy( traceHead2->net,  pSCNL->net ); 	// Network name
  	strcpy( traceHead2->chan, pSCNL->comp );	// Component/channel code
    strcpy( traceHead2->loc,  pSCNL->loc );  	// Location code
  	traceHead2->pinno = pSCNL->chanNum;			// Pin number
	traceHead2->starttime = tm;					// block start time
  	traceHead2->endtime	= traceHead2->starttime + (double)(ChanRate - 1) / (double)ChanRate;
}

/* Init the ADC board */
int InitADC()
{
	DWORD version, sts, lastErrNo;
	TimeInfo timeInfo;
	AdcConfig adcConfig;
	int i, major, minor, ok = 0, numChan;		
	
	/* Get a handle to the board */
	hBoard = PSNOpenBoard();
	if( !hBoard )  {
		PSNGetBoardInfo( 0, ADC_GET_LAST_ERR_NUM, &lastErrNo );
		LogWrite( "e", "PSNOpenBoard(); Last Error = %s\n", GetErrorString( lastErrNo ) );
		return FALSE;
	}
		
	/* Get and display the DLL/ Linux Library version */
	if( !PSNGetBoardInfo( hBoard, ADC_GET_DLL_VERSION, &version ) )  {
		PSNGetBoardInfo( hBoard, ADC_GET_LAST_ERR_NUM, &lastErrNo );
		LogWrite( "e", "Get DLL version error; Last Error = %s\n", GetErrorString( lastErrNo ) );
		PSNCloseBoard( hBoard );
		return FALSE;
	}
	major = version / 10; minor = version % 10;
	LogWrite( "", "PSNADBoard DLL Version %d.%1d\n", major, minor );
	
	if( major > DLL_VERSION_MAJOR )
		ok = TRUE;
	else if( major == DLL_VERSION_MAJOR && minor >= DLL_VERSION_MINOR )
		ok = TRUE;	
	if( !ok )  {
		LogWrite( "e", "PSNADBoard DLL Version Error, Must be %d.%d or higher\n", 
			DLL_VERSION_MAJOR, DLL_VERSION_MINOR  );
		ReportError( 1, "ADC Board Start Error");
		PSNCloseBoard( hBoard );
		return FALSE;
	}
	
	ReadTimeInfo( &timeInfo );
	
	memset( &adcConfig, 0, sizeof( adcConfig ) );
	memset( &config, 0, sizeof( config ) );

	// first set the ADC gain for each channel - Only used by the new SDR24 board
	adcConfig.referenceVolts = 2.5; 	// not used
	numChan = AdcChans;
	if( AdcChans > MAX_SDR24_CHANNELS )
		numChan = MAX_SDR24_CHANNELS;
		
	for( i = 0; i != numChan; i++ )
		adcConfig.adcGains[i] = ChanList[i].adcGain;  
	
	sts = PSNSendBoardCommand( hBoard, ADC_CMD_SET_GAIN_REF, &adcConfig ); 
	if( !sts )  {
		LogWrite( "e", "PSNSendBoardCommand Error setting gain and reference information\n" );
		PSNGetBoardInfo( hBoard, ADC_GET_LAST_ERR_NUM, &lastErrNo );
		LogWrite( "e", "Last Error = %s\n", GetErrorString( lastErrNo ) );
		ReportError( 1, "ADC Board Start Error");
		PSNCloseBoard( hBoard );
		return FALSE;
	}

#ifdef WIN32
	/* Build the configuration structure to be sent to the DLL */
	if( !TcpMode )  {
		config.commPort = atoi( CommPortHostStr );
		config.commPortTcpHost[0] = 0;
	}
#else
	if( TcpMode )
		config.commPort = 0;			// this will tell the DLL/Library to open a TCP connection to the ADC board
	else
		config.commPort = 1;			// this will tell the DLL to open a normal Comm Port using the CommPortHostStr string
#endif
	strcpy( config.commPortTcpHost, CommPortHostStr );
	
	config.commSpeed = PortSpeed;
	config.tcpPort = TcpPort;
	config.numberChannels = AdcChans;
	config.sampleRate = ChanRate;
	config.timeRefType = TimeRefType;
	config.addDropTimer = timeInfo.addDropCount;
	config.addDropMode = timeInfo.addDropFlag;
	config.pulseWidth = timeInfo.pulseWidth;
	
	config.highToLowPPS = HighToLowPPS;
	config.noPPSLedStatus = NoPPSLedStatus;
	config.timeOffset = TimeOffset;
	if( UpdateSysClock )
		config.setPCTime = UpdateSysClock;
	else if( SystemTimeError )
		config.checkPCTime = 1;
	sts = PSNConfigBoard( hBoard, &config, NULL );	
	if( !sts )  {
		PSNGetBoardInfo( hBoard, ADC_GET_LAST_ERR_NUM, &lastErrNo );
		LogWrite( "e", "Config ADC Board Error; Last Error = %s\n", GetErrorString( lastErrNo ) );
		ReportError( 1, "ADC Board Config Error");
		PSNCloseBoard( hBoard );
		return FALSE;
	}
	
	/* Now try and start the data collection */
	sts = PSNStartStopCollect( hBoard, TRUE );
	if( !sts )  {
		PSNGetBoardInfo( hBoard, ADC_GET_LAST_ERR_NUM, &lastErrNo );
		LogWrite( "e", "PSNStartStopCollect Error; Last Error = %s\n", GetErrorString( lastErrNo ) );
		ReportError( 1, "ADC Board Start Error");
		PSNCloseBoard( hBoard );
		return FALSE;
	}
	else if( Debug )
		PSNSendBoardCommand( hBoard, ADC_CMD_DEBUG_REF, (void *)Debug ); 
	LogWrite( "", "Good ADC Board Init\n" );

	return TRUE;	
}

/* Shutdown the ADC board and DLL */
void StopADC()
{
	if( hBoard )
		PSNCloseBoard( hBoard );
	hBoard = 0;
}

/* Report an error to the statmgr */
void ReportError( int errNum, char *errmsg )
{
	int lineLen;
	time_t time_now;			// The current time
	static MSG_LOGO	logo;		// Logo of error messages
	static int first = TRUE;	// TRUE the first time this function is called
	char outmsg[128];			// To hold the complete message

	/* Initialize the error message logo */
	if ( first )  {
		GetLocalInst( &logo.instid );
		logo.mod = ModuleId;
		GetType( "TYPE_ERROR", &logo.type );
		first = FALSE;
	}

	/* Encode the output message and send it */
	time( &time_now );
	sprintf( outmsg, "%d %d ", (int)time_now, errNum );
	strcat( outmsg, errmsg );
	lineLen = strlen( outmsg );
	tport_putmsg( &OutRegion, &logo, lineLen, outmsg );
	return;
}

/* Send a heart beat to statmgr */
void Heartbeat( void )
{
	LONG  msgLen;			  	// Length of the heartbeat message
	char  msg[64];			 	// To hold the heartbeat message
	static int first = TRUE;	// 1 the first time Heartbeat() is called
	static time_t time_prev;	// When Heartbeat() was last called
	time_t	time_now;			// The current time
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
	time( &time_now );
	if ( (time_now - time_prev) < HeartbeatInt )
		return;

	/* It's time to beat the heart */
	sprintf( msg, "%d %d\n", (int)time_now, MyPid );
	msgLen = strlen( msg );

	if ( tport_putmsg( &OutRegion, &logo, msgLen, msg ) != PUT_OK )
		LogWrite( "", "Error sending heartbeat\n" );

	time_prev = time_now;
}

/* Handle the CNTL-C event */
void CntlHandler( int sig )
{
	LogWrite("", "CntlHandler Sig=%d\n", sig );
	
	/* if control-c clean up and exit program if ControlCExit = 1 */
	if( sig == 2 )  {
		if( ControlCExit )  {
			if( needsShutdown )
				Shutdown();
			sleep_ew(100);
			LogWrite( "", "PsnAdSend terminating do to Cntl-C\n" );
			exit( 0 );
		}
		else
			signal(sig, SIG_IGN);
	}
}

/* Make the report string */
void MakeReportString()
{
	int i;
	char tmStr[64], tmStr1[64];
	static int firstLog = 1;
	
	reportString[0] = 0;
	reportStringLen = 0;
		
	if( !ConsoleDisplay )
		AddFmtStr( "-----------------------------------\n" );
		
	if( ConsoleDisplay || firstLog )  {
		if( adcBoardType == BOARD_VM )
			AddFmtStr( "VolksMeter Digitizer\n\n" );
		else if ( adcBoardType == BOARD_SDR24 )
			AddFmtStr( "Webtronics 24-Bit Digitizer - PsnAdSend Version: %s\n\n", VERSION_STRING );
		else
			AddFmtStr( "Webtronics 16-Bit Digitizer - PsnAdSend Version: %s\n\n", VERSION_STRING );
		firstLog = 0;
	}

	GmtTimeStr( tmStr, (double)startTime, 0 );
	AddFmtStr( "Program Start Time (UTC): %s\n", tmStr );
	
	if( dTraceTime )  {
		GmtTimeStr( tmStr1, dTraceTime, 3 );
		AddFmtStr( "Last Packet Time (UTC): %s\n", tmStr1 );
	}	
	else
		AddFmtStr( "Last Packet Time (UTC): ???\n" );
	
	AddFmtStr( "Channels: %d   Samples Received: %u\n", TotalChans, samples );
		
	if( TimeRefType == TIME_REF_PCTIME )  {
		AddFmtStr("Time Reference: Computer Time\n");
	}
	else  {
		if( dataHdr.timeRefStatus == TIME_REF_NOT_LOCKED )
			AddFmtStr("Time Reference: Not Locked\n");
		else if( dataHdr.timeRefStatus == TIME_REF_WAS_LOCKED )
			AddFmtStr("Time Reference: Was Locked\n");
		else 	  
			AddFmtStr("Time Reference: Locked\n");
	}
		
	if( numberMessages )  {
		if( ConsoleDisplay )
			AddFmtStr("\nMessages:\n");
		else
			AddFmtStr("Messages:\n");
		for( i = 0; i != numberMessages; i++ )
			AddFmtStr( messages[i] );
	}
}

/* Makes a new display status string and display it on the screen */
void DisplayReport()
{
	MakeReportString();
	ClearScreen();
	printf( "%s", reportString );
}

/* Called once then the first packet from the ADC board is receiver */
int FirstPacketCheck( char *errMsg )
{
	int c, maxBits = 16, maxChannels = 8, setGain = 0;
	DWORD numChannels;
	SCNL *pSCNL;
		
	errMsg[ 0 ] = 0;
		
	/* Get the number of ADC channels on the board */
	if( !PSNGetBoardInfo( hBoard, ADC_GET_NUM_CHANNELS, &numChannels ) )  {
		sprintf(errMsg, "Error getting number of channels from DLL/LIB\n");
		return 0;
	}
		
	/* Make sure the user has not selected more channels then what's on the ADC board */
	if( adcBoardType == BOARD_VM )  {
		maxChannels = numChannels;
		maxBits = 24;
	}
	else if( adcBoardType == BOARD_SDR24 )  {
		maxChannels = numChannels;
		maxBits = 24;
		setGain = TRUE;
	}
	if( AdcChans > maxChannels )  {
		sprintf( errMsg, "Too many channels defined in config file. AdcChans = %d Max = %d\n", 
			AdcChans, maxChannels );
		return 0;
	}
		
	/* Make sure the user has not selected more ADC bits to use then there are on the board */
	for( c = 0; c != TotalChans; c++ )  {
		pSCNL = &ChanList[ c ];
		if( pSCNL->bitsToUse > maxBits )  {
			sprintf( errMsg, "Chan %d Config Error; ADC Bits set higher then %d\n", c+1, maxBits );
			return 0;
		}
		if( !setGain && pSCNL->adcGain != 1 )  {
			sprintf( errMsg, "Chan %d Config Error; Gain must be 1 for this ADC Board\n", c+1 );
			return 0;
		}
		pSCNL->shiftNumber = maxBits - pSCNL->bitsToUse;
	}
	return 1;
}

/* This function demuxes the data data, shifts out any unneeded bits and applies
   DC Offset to the data */
void DemuxShiftData( LONG *adcData )
{
	int data, inv, shift, c, cnt, offset;
	LONG *to, *from, *pBuff;
	SCNL *pSCNL;
	
	/* First demux the incomming data and move it to the individual SCNL data buffer(s) */
	for( c = 0; c != AdcChans; c++ )  {
		to = ChanList[ c ].dataBuff;
		from = &adcData[ c ];
		cnt = ChanRate;
		while( cnt-- )  {
			*to++ = *from;
			from += AdcChans;
		}
	}
	
	/* Now make a copy of the raw ADC data to the additional channel data buffer */
	if( AddChans )  {
		for( c = AdcChans; c != TotalChans; c++ )  {
			pSCNL = &ChanList[ c ];
			memcpy( pSCNL->dataBuff, ChanList[ pSCNL->useChannel ].dataBuff, ChanRate * sizeof( LONG ) ); 		
		}
	}
			
	for( c = 0; c != TotalChans; c++ )  {
		pSCNL = &ChanList[ c ];
		shift = pSCNL->shiftNumber;
		inv = pSCNL->invertChannel;
		offset = pSCNL->dcOffset;
		if( !offset && !inv && !shift )		// if nothing to do skip channel
			continue;
		
		/* Now shift, apply offset and invert the data as needed */
		pBuff = pSCNL->dataBuff;
		cnt = ChanRate;
		while( cnt-- )  {
			data = *pBuff;
			if( shift )  {
				if( data < 0 )  {
					data = -data;
					data >>= shift;
					data = -data;
				}
				else
					data >>= shift;
			}
			data += offset;
			if( inv )
				data = -data;
			*pBuff++ = data;
		}
	}
}

/* Apply any filters to the data */
void FilterData( SCNL *pSCNL )
{
	if( pSCNL->invFilter )
		pSCNL->invFilter->FilterData( pSCNL->dataBuff, ChanRate );
	if( pSCNL->lpFilter )
		pSCNL->lpFilter->FilterData( pSCNL->dataBuff, ChanRate );
	if( pSCNL->hpFilter )
		pSCNL->hpFilter->FilterData( pSCNL->dataBuff, ChanRate );
}

/* Checks to see if it's ok to send the ADC data to the data ring. Currently 
   it checks to see if the GPS time status is not locked. */
int OkToSendData( SCNL *pSCNL, DataHeader *hdr )
{
	if( !pSCNL->sendToRing )
		return FALSE;
	if( !NoSendBadTime || !IsGpsRef() )
		return TRUE;	
	if( hdr->timeRefStatus == TIME_REF_NOT_LOCKED )  
		return FALSE;
	return TRUE;
}

/* Handle the At Exit call */
void AtExitHandler()
{
	if( needsShutdown )
		Shutdown();
}

/* Shutdown the PsnAdSend Module */
void Shutdown()
{
	SCNL *pSCNL;			// Current pointer to SCNL
	int c;
	
	StopADC();
	free( dataBuf );
	for( c = 0; c != TotalChans; c++ )  {
		pSCNL = &ChanList[ c ];
		if( pSCNL->lpFilter )
			delete pSCNL->lpFilter;
		if( pSCNL->hpFilter )
			delete pSCNL->hpFilter;
		if( pSCNL->invFilter )
			delete pSCNL->invFilter;
		if( pSCNL->dataBuff )
			free( pSCNL->dataBuff );
	}
	if( MuxKey != 0 )
		SendMuxedData( 0, 0, 0 );		// frees data buffer
	needsShutdown = FALSE;
}	

/* Restart the A/D board */
int Restart()
{
	StopADC();
	sleep_ew( 500 );
	if( !InitADC() )  {		/* Initialize the ADC board */
		LogWrite( "", "Restart ADC Init board error\n" );
		return FALSE;
	}
	if( RefreshTime )
		DisplayReport();
	LogWrite( "", "Restart ADC Board\n" );
	timeRefStatus = TIME_REF_UNKNOWN;
	startNotLockTime = 0;
	return TRUE;	
}

/* Report any error messages from the PsnAdBoard library to the StatMgr module */
void CheckForErrors( char *msg )
{
	char *ptr, errMsg[ 256 ];
	int tdiff, i;
		
	// first test the system time to the A/D board time if enabled
	if( SystemTimeError && strstr( msg, "Time difference between A/D Board and Host Computer") )  {
		if( ( ptr = strchr(msg, '=' ) ) == 0 )  {
			LogWrite( "", "CheckForErrors() did not find = in Time Difference Message\n" );
			return;
		}
		++ptr;
		tdiff = (int)( ( fabs( atof( ptr ) ) + 0.5 ) );
		if( tdiff >= SystemTimeError )  {
			sprintf( errMsg, "High time difference between A/D board and host computer detected. Error = %d seconds", tdiff );
			ReportError( 3, errMsg );
		}
	}
	
	// now test for high 1pps error 	
	if( PPSTimeError && IsGpsRef() && ( ( ptr = strstr( msg, "1PPSDif:") ) != 0 ) )  {
		if( *ptr == '?' )
			return;
		if( timeRefStatus == TIME_REF_WAS_LOCKED || timeRefStatus == TIME_REF_LOCKED )  {
			ptr += 8;
			i = atoi( ptr );
			if( abs( i ) >= PPSTimeError )  {
				sprintf( errMsg, "High 1PPS error detected. Error = %d milliseconds", i );
				ReportError( 4, errMsg );
			}
		}
	}
}

/* Send an error message if the time status changes */
void CheckLockStatus( int newStatus )
{
	char str1[ 32 ], str2[ 32 ], str[ 256 ];
	time_t now, diff;
	
	time( &now );
	if( newStatus == TIME_REF_NOT_LOCKED )  {
		if( !startNotLockTime )
			 startNotLockTime = now;
		else if( NoLockTimeError && ( now - startNotLockTime ) > NoLockTimeError )  {
			sprintf( str, "Time Reference Error - No Time Lock after %d minutes", NoLockTimeError / 60 );
			LogWrite( "", "%s\n", str );
			ReportError( 10, str );
			startNotLockTime = now;
		}
	}
		
	if( timeRefStatus == TIME_REF_UNKNOWN )  {
		if( newStatus == TIME_REF_NOT_LOCKED )
			return;
		if( newStatus == TIME_REF_LOCKED )  {
			timeRefStatus = newStatus;
			startNotLockTime = 0;
			return;
		}
	}
	if(	timeRefStatus != newStatus )  {
		GetRefName( timeRefStatus, str1 );
		GetRefName( newStatus, str2 );
		LogWrite( "", "Time Reference Status Change was '%s' now '%s'\n", str1, str2 );
		if( ( newStatus == TIME_REF_LOCKED ) && ( timeRefStatus != TIME_REF_LOCKED ) )  {
			startNotLockTime = 0;
			ReportError( 8, "" );
		}
		else if( ( timeRefStatus == TIME_REF_LOCKED ) && ( newStatus == TIME_REF_WAS_LOCKED ) )
			ReportError( 5, "" );
		else if( ( timeRefStatus == TIME_REF_WAS_LOCKED ) && ( newStatus == TIME_REF_NOT_LOCKED ) )
			ReportError( 6, "" );
		else if( ( timeRefStatus == TIME_REF_LOCKED ) && ( newStatus == TIME_REF_NOT_LOCKED ) )
			ReportError( 7, "" );
		timeRefStatus = newStatus;
	}
}

/* Used to filter out some DLL/Library messages from going to the log file */
int FilterLogMsg( char *msg )
{
	if( SystemTimeError &&  FilterSysTimeDiff &&  
			strstr( msg, "Time difference between A/D Board and Host Computer") )
		return 1;		// return filter message
	
	if( IsGpsRef() && FilterGPSMessages )  {
		if( strstr( msg, "GPSRef: Sts:1 Lck:1") || strstr( msg, "GPSRef: Sts:2 Lck:1") )
			return 1;						// return filter message
		return 0;							// do not filter message
	}
	
	return 0;			// do not filter message
}

/* Sends multiplexed A/D data to the raw data mux ring */
void SendMuxedData( DataHeader *hdr, BYTE *data, int dataLen )
{
	static BYTE *pBuff = 0;		// Holds the message
	static int first = TRUE;	// 1 the first time function is called
	static int dlen = 0;		// current data length
	static MSG_LOGO	logo;		// Logo of messages
	static MuxHdr muxHdr;		// pre header for packet sent to ring
		
	if( hdr == 0 && data == 0 && pBuff != 0 )  {		// called at exit to free buffer
		free( pBuff );
		pBuff = 0;
		return;
	}
		
	if( first )  {
		first = 0;
		GetLocalInst( &logo.instid );
		logo.mod = ModuleId;
		GetType( "TYPE_ADBUF", &logo.type );
		pBuff = (BYTE *)malloc( dataLen + sizeof( DataHeader ) + sizeof( MuxHdr ) );
		dlen = dataLen;
		muxHdr.boardType = (BYTE)adcBoardType;
		muxHdr.numChannels = (BYTE)AdcChans;
		muxHdr.sampleRate = (WORD)ChanRate;
		muxHdr.msgType = 'D';
	}
		
	if( ( dlen != dataLen ) && pBuff )  {
		LogWrite( "", "Data length change in SendMuxedData()\n");
		free( pBuff );
		pBuff = (BYTE *)malloc( dataLen + sizeof( DataHeader ) + sizeof( MuxHdr ) );
		dlen = dataLen;
	}
		
	if( pBuff == 0 )  {
		LogWrite("e", "No Data Buffer to send muxed data\n");
		return;
	}
	
	memcpy( pBuff, &muxHdr, sizeof( MuxHdr ) );
	memcpy( &pBuff[ sizeof( MuxHdr ) ], hdr, sizeof( DataHeader ) );
	memcpy( &pBuff[ sizeof( DataHeader ) + sizeof( MuxHdr ) ], data, dataLen );
	if ( tport_putmsg( &MuxRegion, &logo, dataLen + sizeof( DataHeader ) + sizeof( MuxHdr ), 
			(char *)pBuff ) != PUT_OK )
		LogWrite( "", "Error sending mux data\n" );
}

/* Sends log messages to the raw data mux ring */
void SendLogMessage( char *msg )
{
	static int first = TRUE;	// 1 the first time function is called
	static MSG_LOGO	logo;		// Logo of messages
	static MuxHdr muxHdr;		// pre header for packet sent to ring
	char outBuff[ 1024 ];
	int len = strlen( msg ) + 1;
			
	if( first )  {
		first = 0;
		GetLocalInst( &logo.instid );
		logo.mod = ModuleId;
		GetType( "TYPE_ADBUF", &logo.type );
		muxHdr.boardType = (BYTE)adcBoardType;
		muxHdr.numChannels = (BYTE)AdcChans;
		muxHdr.sampleRate = (WORD)ChanRate;
		muxHdr.msgType = 'L';
	}
		
	memcpy( outBuff, &muxHdr, sizeof( MuxHdr ) );
	memcpy( &outBuff[ sizeof( MuxHdr )], msg, len );
	if ( tport_putmsg( &MuxRegion, &logo, len + sizeof( MuxHdr ), outBuff ) != PUT_OK )
		LogWrite( "", "Error sending mux data\n" );
}

#ifdef WIN32
void CheckSetSysTime( double dTime )
{
	checkSysTime = 0;
}
void SetComputerTime( double now )
{
}
#else
void CheckSetSysTime( double dTime )
{		
	time_t sys;
	double diff;
	 	
	if( timeSetWait )  {
		--timeSetWait;
		if( !timeSetWait )  {		// timer has expired so we can now set the system time
			SetComputerTime( dTime );
			checkSysTime = 0;		// stop testing
		}
		return;
	}
	
	// timer not set to check the system time and the A/D board time
	time( &sys );
	diff = dTime - (double)sys;
	if( fabs( diff ) > checkSysTime )  {
		LogWrite( "", "Setting Computer Timer - Error = %g sec\n", diff );
		PSNSendBoardCommand( hBoard, ADC_CMD_FORCE_TIME_TEST, 0 ); 
		timeSetWait = 5;		// need to wait until time changes on the A/D board
	}
	else
		checkSysTime = 0;		// stop testing time is ok
}

void SetComputerTime( double now )
{
	struct timeval t_set;
	double iSec, fract;
		
	LogWrite( "", "Setting Computer Time to %.3f\n", now );
	fract = modf( now, &iSec );
	t_set.tv_sec = (time_t)iSec;
	t_set.tv_usec = (time_t)( fract * 1000000 );
	if( settimeofday( &t_set, 0 ) )
		LogWrite( "", "settimeofday error\n" );
	else
		startTime = (time_t)iSec;
}
#endif
