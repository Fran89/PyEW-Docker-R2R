/*********************************************************************
*									Ws2Ew.c						     *
* 																	 *
*********************************************************************/

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include <signal.h>
#include "./mseed/psnmseed.h"
#include "Ws2Ew.h"

/* Global variables */
SHM_INFO OutRegion;			// Info structure for output region
int MyPid;					// process id, sent with heartbeat

char traceBuffData[ 16384 ];
BYTE inQueData[ 4096 ];

SOCKET listenSock = SOCKET_ERROR;		// Listener Socket
int exitListenThread = 0, exitListenAck = 0;
unsigned  tidListen, tidReceive;
time_t listenRestartTime;

QUEUE tbQ, msgQ;
mutex_t tbMx, msgMx, lsMx, rsMx, userMx;

MSG_LOGO logo;		 				// Logo of message to put out
unsigned int sampleCount = 0;		// Scan number of samples processed
int noDataCounter = 0;

char ackOutBuff[ 128 ];

/* Messages to display onthe screen are placed in this array */
char messages[ MAX_MESSAGES ][ MESSAGE_LEN ];
int numberMessages = 0;

char reportString[ MAX_RPT_STR_LEN ];
int reportStringLen;

unsigned char InstId;	// Installation id placed in the trace header

UserInfo userInfo[ MAX_CONNECT_USERS ];
int activeUsers = 0;

int curTimeRef = 0;
double lastUpdateTime = 0.0;

time_t start_time;
int displayCount = 0;

int exitWs2Ew = 0;

ConnectInfo connectInfo[ MAX_CONNECT_USERS ];

void ProcessLoop()
{
	int chan, len, rc, cnt, dataLen, *iPtr, d, dataSize, sts;
	BYTE *dataPtr;
	double tm;
	short *sPtr;
	DataHeader *pHdr;
	TRACE2_HEADER *traceHead = (TRACE2_HEADER *)&traceBuffData[0];
	void *traceData = &traceBuffData[ sizeof(TRACE2_HEADER) ];
	SCNL *pChan;
		
	if( RefreshTime )
		DisplayReport();
	
	/************************* The main program loop	*********************/
	while ( 1 )  {
		sts = tport_getflag( &OutRegion );
		if( sts == TERMINATE || sts == MyPid || exitWs2Ew )
			break;
			
		if( ! ( len = GetInQueue( inQueData ) ) )  {
			CheckStatus();
			sleep_ew( 100 );
			continue;
		}
		pHdr = (DataHeader *)inQueData;
		
		if( ( chan = FindChannel( pHdr ) ) == -1 )  {
			if( Debug )
				LogWrite("", "Chan [%s.%s.%s.%s] Not Found SCNL List\n", pHdr->station, 
					pHdr->channel, pHdr->network, pHdr->location );	
			continue;
		}
		pChan = &ChanList[ chan ];
				
		if ( !pChan->send )  {			// check to see if we should send channel to the ring 
			if( Debug )
				LogWrite("", "Chan [%s.%s.%s.%s] Send Disabled\n", pHdr->station, 
					pHdr->channel, pHdr->network, pHdr->location );	
			continue;
		}
		
		if( Debug )
			LogWrite( "", "Chan [%s.%s.%s.%s] Seq#=%d\n", pHdr->station, 
					pHdr->channel, pHdr->network, pHdr->location, pHdr->sequenceNumber );	
		
		if( pChan->sequenceNumber == 999999 && pHdr->sequenceNumber == 1 )
			pChan->sequenceNumber = pHdr->sequenceNumber; 
		else if( pChan->sequenceNumber != -1 && ( ( pChan->sequenceNumber + 1 ) != pHdr->sequenceNumber ) )  {
			LogWrite( "", "%s.%s.%s.%s Seq Error Last=%d New=%d Diff=%d\n", pHdr->station, 
				pHdr->channel, pHdr->network, pHdr->location, pChan->sequenceNumber, pHdr->sequenceNumber, 
				pHdr->sequenceNumber - pChan->sequenceNumber );	
		}
		pChan->sequenceNumber = pHdr->sequenceNumber; 
		
		noDataCounter = 0;
		
		sampleCount += (unsigned)( pHdr->numSamples );		/* Update the sample count */

		tm = CalcPacketTime( &pHdr->startTime, pChan->filterDelay );
		if( tm > lastUpdateTime )
			lastUpdateTime = tm;
	
		curTimeRef = pHdr->timeRefStatus;
		
	  	/* Fill the trace buffer header */
		traceHead->nsamp = pHdr->numSamples;		// Number of samples in message
		traceHead->samprate = pHdr->sampleRate;	  	// Sample rate; nominal
		traceHead->version[0] = TRACE2_VERSION0;     // Header version number
		traceHead->version[1] = TRACE2_VERSION1;     // Header version number
	  	traceHead->quality[0] = '\0';				// One bit per condition
	  	traceHead->quality[1] = '\0';				// One bit per condition

	  	strcpy( traceHead->sta,  pChan->sta );  	// Site name
	  	strcpy( traceHead->net,  pChan->net );  	// Network name
	  	strcpy( traceHead->chan, pChan->comp ); 	// Component/channel code
	  	strcpy( traceHead->loc,  pChan->loc );  	// Location code
	  	traceHead->pinno = chan + 1;				// Pin number
			
	  	/* Set the trace start and end times. Times are in seconds since midnight 1/1/1970 */
	  	traceHead->starttime = (double)tm;
	  	traceHead->endtime	= traceHead->starttime + (double)(pHdr->numSamples - 1) / pHdr->sampleRate;

	  	/* Set error bits in buffer header */
	  	if( pHdr->timeRefStatus != TIME_REF_LOCKED )
			traceHead->quality[0] |= TIME_TAG_QUESTIONABLE;

		dataSize = 4;
		if( !AdcDataSize )  {	// check for auto data size based on board type
			if( pHdr->boardType == 1 || pHdr->boardType == 2 || pHdr->boardType ==  4 )
				dataSize = 2;
		}
		else if( AdcDataSize == 2 )
			dataSize = 2;
			
		if( dataSize == 4 )  {
		  	strcpy( traceHead->datatype, "i4" );	// Data format code = long
			dataLen = pHdr->numSamples * sizeof( int );
			memcpy( traceData, &inQueData[ sizeof( DataHeader ) ], dataLen );
		}
		else  {
		  	strcpy( traceHead->datatype, "i2" );	// Data format code = short
			dataLen = pHdr->numSamples * sizeof( short );
		  	sPtr = (short *)traceData;
			iPtr = (int *)&inQueData[ sizeof( DataHeader ) ];
			cnt = pHdr->numSamples;
			while( cnt-- )  {
				d = *iPtr++;
				if( d > 32767 )
					d = 32767;
				else if( d < -32768 )
					d = -32768;
				*sPtr++ = (short)d;
			}
		}
		
	  	rc = tport_putmsg( &OutRegion, &logo, dataLen + sizeof(TRACE2_HEADER), traceBuffData );
	  	if ( rc == PUT_TOOBIG )
			LogWrite( "", "Trace message for channel %d too big\n", chan );
	  	else if ( rc == PUT_NOTRACK )
		  	LogWrite( "", "Tracking error while sending channel %d\n", chan );
		else if( Debug )
			LogWrite( "", "Chan [%s.%s.%s.%s] Sent to ring\n", pHdr->station, 
				pHdr->channel, pHdr->network, pHdr->location );	
	}
	LogWrite("", "Ws2Ew Got Terminate Message\n");
}

int QueueRecvData( UserInfo *pUI, MSRecord *msr )
{
	int len, month, day;
	char *ptr;
	DataHeader *pHdr;
	char outData[ 4096 ];

	++pUI->recvPackets;
	++pUI->totalRecvPackets;
	pUI->updateTime = time( 0 );
	pUI->noDataReport = 0;
	
	pHdr = (DataHeader *)&outData;
	memset( pHdr, 0, sizeof( DataHeader ) );
			
	pHdr->sequenceNumber = (ULONG)msr->sequence_number;
	strncpy( pHdr->station, msr->fsdh.station, 5 );
	if( ptr = strchr( pHdr->station, ' ' ) )
		*ptr = 0;
	strncpy( pHdr->channel, msr->fsdh.channel, 3 );
	strncpy( pHdr->location, msr->fsdh.location, 2 );
	strncpy( pHdr->network, msr->fsdh.network, 2 );
	if( msr->timeQual == 2 )
		pHdr->timeRefStatus = TIME_REF_LOCKED;
	else if( msr->timeQual == 1 )
		pHdr->timeRefStatus = TIME_REF_WAS_LOCKED;
	else
		pHdr->timeRefStatus = TIME_REF_NOT_LOCKED;;
		
	pHdr->numSamples = msr->numsamples;
	pHdr->sampleRate = msr->samprate;
	pHdr->boardType = pUI->boardType;
		
	ms_doy2md( msr->fsdh.start_time.year, msr->fsdh.start_time.day,  &month, &day );
	pHdr->startTime.wYear = (WORD)msr->fsdh.start_time.year;
	pHdr->startTime.wMonth = (WORD)month;
	pHdr->startTime.wDay = (WORD)day;
	pHdr->startTime.wHour = (WORD)msr->fsdh.start_time.hour;
	pHdr->startTime.wMinute = (WORD)msr->fsdh.start_time.min;
	pHdr->startTime.wSecond = (WORD)msr->fsdh.start_time.sec;
	pHdr->startTime.wMilliseconds = (WORD)msr->fsdh.start_time.fract / 10;
			
	len = msr->numsamples * sizeof( int );
	memcpy( &outData[ sizeof( DataHeader ) ], msr->datasamples, len );
	RequestSpecificMutex( &tbMx );
	enqueue( &tbQ, outData, len + sizeof( DataHeader ), logo );
	ReleaseSpecificMutex( &tbMx );

	if( SendAck && ( pUI->recvPackets >= SendAck ) )  {
		pUI->recvPackets = 0;
		len = strlen( ackOutBuff ) + 1;
		if( send_ew( pUI->sock, ackOutBuff, len, 0, 10000 ) != len )  {
			LogWriteThread( "Error Sending to %s; Exiting", pUI->who );
			ReportError( 3, "Error Sending Ack to user %s", pUI->who );
			return FALSE;
		}
	}
	
	return TRUE;
}

int FindChannel( DataHeader *pHdr )
{
	int i;
	
	for ( i = 0; i != Nchans; i++ )  {
		if(  !strncmp( pHdr->station, ChanList[i].sta, 5 ) && 
		     !strncmp( pHdr->channel, ChanList[i].comp, 3 ) &&
		     !strncmp( pHdr->location, ChanList[i].loc, 2 ) &&
		     !strncmp( pHdr->network, ChanList[i].net, 2 ) )  {
			return i;
		}
	}
	
	return -1;
}

thr_ret ReceiveLoop( void *p )
{
	int ret, total, need;
	char *ptr;
	MSRecord msRecord, *msr = &msRecord;
	UserInfo *pUI = ( UserInfo *)p;
	ConnectInfo *pCI;
			
	pUI->updateTime = time( 0 );
	pUI->noDataReport = 0;
	pUI->recvPackets = 0;
	pUI->totalRecvPackets = 0;
	
	if( send_ew( pUI->sock, "SEND MINISEED", 14, 0, 10000 ) != 14 )  {
		LogWriteThread( "Error Sending Command to User; Exiting" );
		ReportError( 3, "Error Sending Data to User");
		CloseUserSock( pUI );
		KillReceiveThread( pUI );
		goto end;
	}

	ret = recv_ew( pUI->sock, pUI->inData, 4095, 0, SocketTimeout );
	if( ret <= 0 )  {
		LogWriteThread( "Bad Response from WinSDR; Exiting" );
		ReportError( 4, "Bad Response from User");
		CloseUserSock( pUI );
		KillReceiveThread( pUI );
		goto end;
	}
	pUI->inData[ ret ] = 0;
	
	GetWinSDRInfo( pUI );
	
	pUI->updateTime = time( 0 );
	
	LogWriteThread("Connected to WinSDR at %s:%d", pUI->who, Port );
	LogWriteThread("Settings: %s", pUI->inData );
	strncpy( pUI->settings, pUI->inData, 255 );
	ParseFirstChanName( pUI );
	
	pCI = FindOrAddUser( pUI, TRUE );
	if( !pCI )  {
		LogWriteThread("Can't Add User %s, Too many connections", pUI->who );
		ReportError( 5, "Too Many Connections");
		CloseUserSock( pUI );
		KillReceiveThread( pUI );
		goto end;
	}
	pCI->pUI = pUI;
	pUI->pCI = pCI;
	++pCI->connectCount;
	
	pUI->connecting = FALSE;
	pUI->restart = pUI->connected = TRUE;

	memset( &msRecord, 0, sizeof( msRecord ) );

	total = 0;
	need = 512;
	while( !pUI->exitThread )  {
		ret = recv_ew( pUI->sock, &pUI->inData[ total ], need, 0, 250 );
		if( tport_getflag( &OutRegion ) == TERMINATE || tport_getflag( &OutRegion ) == MyPid )  {
			LogWriteThread( "Received terminate message" );
			break;
		}
		if( ret < 1 )  {
			if( socketGetError_ew() == WOULDBLOCK_EW )
				continue;
			LogWriteThread("TCP/IP Connection Lost with %s", pUI->who );
			ReportError( 6, "TCP/IP Connection Lost with User %s", pUI->who );
			break;
		}
		total += ret;
		if( total > 512 )  {
			LogWriteThread("Total > 512 Error Total=%d", total );
			need = 512;
			total = 0;
			continue;
		}	
		if( total < 512 )  {
			need = 512 - total;
			continue;
		}
		msr_reset( msr );
		ret = msr_unpack( pUI->inData, total, &msr, 1, 1 );
		if( ret == MS_NOERROR )  {
			if( !QueueRecvData( pUI, msr ) )  {
				LogWriteThread("QueuRecvData Error");
				ReportError( 7, "QueuRecvData Error");
				break;
			}
		}
		else
			LogWriteThread("MS Unpack Error Ret=%d Total=%d", ret, total );
		need = 512;
		total = 0;
	}
	
	LogWriteThread( "Exit ReceiveLoop" );
	
	CloseUserSock( pUI );
	KillReceiveThread( pUI );

end:
#ifdef WIN32
	return;
#else
	return 0;
#endif
}

UserInfo *GetUserInfo()
{
	UserInfo *pUI;
	int i;
		
	if( !ListenMode )  {
		pUI = &userInfo[ 0 ];	
		memset( pUI, 0, sizeof( UserInfo ) );
		pUI->inUse = TRUE;
		pUI->sock = SOCKET_ERROR;
		pUI->pCI = 0;
		strcpy( pUI->who, Host );
		return pUI;
	}
	
	RequestSpecificMutex(&userMx);
		
	for( i = 0; i != MAX_CONNECT_USERS; i++ )  {
		pUI = &userInfo[ i ];	
		if( pUI->inUse )
			continue;
		memset( pUI, 0, sizeof( UserInfo ) );
		pUI->inUse = TRUE;
		pUI->index = i;
		pUI->sock = SOCKET_ERROR;
		pUI->pCI = 0;
		++activeUsers;
		ReleaseSpecificMutex(&userMx);
		return pUI;
	}
	
	ReleaseSpecificMutex(&userMx);
	
	return 0;
}

void RemoveUserInfo( UserInfo *pUI )
{
	if( Debug )
		LogWriteThread( "RemoveUser at %d", pUI->index );
	
	if( !ListenMode )
		return;
	
	RequestSpecificMutex(&userMx);
	
	pUI->inUse = FALSE;
	pUI->updateTime = 0; 
	pUI->restart = pUI->connected = pUI->connecting = FALSE;
	
	if( pUI->pCI )  {
		ConnectInfo *pCI = (ConnectInfo *)pUI->pCI;	
		pCI->pUI = 0;
	}	
	pUI->pCI = 0;
	
	if( activeUsers )
		--activeUsers;
	
	ReleaseSpecificMutex(&userMx);
}

void GetWinSDRInfo( UserInfo *pUI )
{
	char *ptr;
	if( !AdcDataSize )  {
		if( ( ptr = strstr( pUI->inData, "BrdType:" ) ) )  {
			ptr += 9;
			pUI->boardType = atoi( ptr );
		}
	}
}

thr_ret ListenThread( void *parm )
{
	int err, retval, timeout, addrSize = sizeof(struct sockaddr_in);
	struct timeval wait;
	struct sockaddr_in localAddr, peer;
	fd_set rfds, wfds;
	UserInfo *pUI;
			
	LogWriteThread( "Ws2Ew Listen Thread Start" );
	
	listenRestartTime = time( 0 );
		
	if( ( listenSock = socket_ew( AF_INET, SOCK_STREAM, 0)) == -1 )  {
		LogWriteThread( "Error Opening Socket. Exiting" );
		ReportError( 7, "Socket Error");
		KillListenThread();
		return;
	}
	
	memset( &localAddr, 0, sizeof(localAddr) );
	
	localAddr.sin_port = htons( Port );
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		
	if( ( err = bind_ew(listenSock, (struct sockaddr *)&localAddr, sizeof(localAddr))) == SOCKET_ERROR )  {
		LogWriteThread( "TCP/IP Socket Bind Failed" );
		ReportError( 7, "Socket Error");
		KillListenThread();
		goto end;
	}
		
	if( listen_ew( listenSock, 1 ) == SOCKET_ERROR)  {
		ReportError( 7, "Socket Error");
		LogWriteThread( "TCP/IP Couldn't Get Listen Socket" );
		KillListenThread();
		goto end;
	}
	
	sampleCount = 0;
	
	listenRestartTime = time( 0 );
		
	while( !exitListenThread )  {
		FD_ZERO( &rfds );
		FD_ZERO( &wfds );
		FD_SET( listenSock, &rfds );
		FD_SET( listenSock, &wfds );
		if( (retval = select_ew( listenSock+1, &rfds, &wfds, NULL, 2000 ) ) == SOCKET_ERROR )  {
			LogWriteThread( "TCP/IP select Failed" );
			ReportError( 7, "Socket Error");
			break;
		}
		
		if( tport_getflag( &OutRegion ) == TERMINATE || tport_getflag( &OutRegion ) == MyPid )  {
			LogWriteThread( "Received terminate message" );
			break;
		}
			
		if( !retval )  {
			listenRestartTime = time( 0 );
			continue;
		}
		
		if( ! ( pUI = GetUserInfo() ) )  {
			LogWriteThread( "Too Many TCP/IP Users" );
			ReportError( 5, "Too Many TCP/IP Users");
			continue;
		}
		LogWriteThread( "New User at %d", pUI->index );
		
		if(( pUI->sock = accept_ew( listenSock, (struct sockaddr *)&peer, &addrSize, 2000 ) ) == SOCKET_ERROR )  {
			LogWriteThread( "TCP/IP Accept Failed" );
			RemoveUserInfo( pUI );
			ReportError( 7, "Socket Error");
			break;
		}
		
		pUI->connecting = pUI->restart = TRUE;
		pUI->updateTime = pUI->connectTime = time( 0 );
		
		if( StartThreadWithArg( ReceiveLoop, pUI, (unsigned)THREAD_STACK, &pUI->tidReceive ) == -1 )  {
			LogWriteThread( "Error starting receive thread. Exiting." );
			ReportError( 7, "Thread Start Error");
			RemoveUserInfo( pUI );
			break;
      	}
	}
	
	LogWriteThread( "Ws2Ew Listen Thread Done." );
	
	exitListenAck = TRUE;
	
	KillListenThread();

end:
#ifdef WIN32
	return;
#else
	return 0;
#endif
}

thr_ret ReceiveThread( void *parm )
{
	UserInfo *pUI = GetUserInfo();
	if( !pUI )  {
		LogWriteThread("ReceiveThread GetUserInfo Error" );
		ReportError( 7, "Get User Error");
		goto end;
	}
	
	pUI->connecting = TRUE;

	if( ( pUI->sock = socket_ew( AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR )  {
		LogWriteThread("Error Opening Socket. Exiting" );
		ReportError( 7, "Socket Error");
		pUI->updateTime = pUI->connectTime = 0;
		KillReceiveThread( pUI );
		goto end;
	}
	if( connect_ew( pUI->sock, (struct sockaddr*) &saddr, sizeof(saddr), SocketTimeout ) == SOCKET_ERROR )  {
		LogWriteThread("Error Connecting to %s; Exiting", pUI->who );
		ReportError( 7, "Socket Error");
		pUI->updateTime = pUI->connectTime = 0;
		KillReceiveThread( pUI );
		goto end;
	}
	pUI->updateTime = pUI->connectTime = time( 0 );
	if( !ListenMode )
		activeUsers = 1;
	ReceiveLoop( pUI );

end:

#ifdef WIN32
	return;
#else
	return 0;
#endif
}

void CloseUserSock( UserInfo *pUI )
{
	RequestSpecificMutex(&rsMx);
	
	if( pUI->sock != SOCKET_ERROR )
		closesocket_ew( pUI->sock, SOCKET_CLOSE_IMMEDIATELY_EW );
	pUI->sock = SOCKET_ERROR;
	
   	ReleaseSpecificMutex( &rsMx );

	pUI->connected = pUI->connecting = 0;
	
}

void KillReceiveThread( UserInfo *pUI )
{
	RemoveUserInfo( pUI );
	pUI->exitAck = TRUE;
	if( !ListenMode )
		activeUsers = 0;
	KillSelfThread();
}

void KillListenThread()
{
	RequestSpecificMutex(&lsMx);
	
	if( listenSock != SOCKET_ERROR )
		closesocket_ew(listenSock, SOCKET_CLOSE_IMMEDIATELY_EW);
	listenSock = SOCKET_ERROR;
	
   	ReleaseSpecificMutex( &lsMx );
	
	listenRestartTime = time( 0 );
	KillSelfThread();
}

int GetInQueue( BYTE *data )
{
	MSG_LOGO recLogo;
	long dataLen, ret;

	RequestSpecificMutex(&tbMx);
	ret = dequeue(&tbQ, (char *)data, &dataLen, &recLogo );
    ReleaseSpecificMutex(&tbMx);
	if( ret < 0 )
		return 0;
	return dataLen;
}

void CheckConnectStatus( UserInfo *pUI, time_t now )
{
	time_t diff = now - pUI->updateTime;
	if( diff >= NoDataWaitTime )  {
		LogWrite( "", "%s No Data Timeout\n", pUI->who );
		ReportError( 2, "%s No Data Timeout", pUI->who );
		pUI->exitThread = TRUE;
	}
}

void CheckStatus()
{
	static int test = 0;
	int max, i, diff = 0, cnt, dspFlag = 0;
	MSG_LOGO recLogo;
	long msgLen, ret;
	char newMsg[256], ch;
	UserInfo *pUI;
	ConnectInfo *pCI;
	time_t now;
		
	RequestSpecificMutex(&msgMx);
	ret = dequeue(&msgQ, newMsg, &msgLen, &recLogo );
    ReleaseSpecificMutex(&msgMx);
	if( ret >= 0 && msgLen )
		LogWrite( "", "%s\n", newMsg );

	if( ++test < 10 )
		return;
	test = 0;

	now = time( 0 );
	
	Heartbeat( now );		/* Beat the heart once per second */
	
	if( ListenMode )  {
		diff = now - listenRestartTime;
		if( diff >= RestartWaitTime )  {
			LogWrite( "", "Restarting Listen Thread\n");
			if( StartThread( ListenThread, (unsigned)THREAD_STACK, &tidListen ) == -1 )  {
				LogWrite( "", "Error starting receive thread. Exiting.\n" );
				ReportError( 7, "Thread Start Error");
				ExitWs2Ew( -1 );
			}
		}
		else {
			for( i = 0; i != MAX_CONNECT_USERS; i++ )  {
				pUI = &userInfo[ i ];
				if( pUI->inUse )
					CheckConnectStatus( pUI, now );
			}
			for( i = 0; i != MAX_CONNECT_USERS; i++ )  {
				pCI = &connectInfo[ i ];
				if( pCI->who[0] && pCI->pUI )
					CheckUsers( pCI, now );
			}
		}
	}
 	else  {
		pUI = &userInfo[0];
		if( !pUI->connected && pUI->restart )  {
			diff = now - pUI->updateTime;
			if( diff >= RestartWaitTime )  {
				pUI->restart = noDataCounter = 0;
				LogWrite( "", "Restarting Receive Thread\n");
				if( StartThread( ReceiveThread, (unsigned)THREAD_STACK, &pUI->tidReceive ) == -1 )  {
					LogWrite( "", "Error starting receive thread. Exiting.\n" );
					ReportError( 7, "Thread Start Error");
					ExitWs2Ew( -1 );
				}
			}
		}
		else if( ++noDataCounter >= NoDataWaitTime )  {
			noDataCounter = 0;
			pUI->connected = 0;
			pUI->exitThread = TRUE;
			max = 100;
			while( max-- )  {
				if( pUI->exitAck )
					break;
				sleep_ew( 50 );
			}
			pUI->restart = TRUE;
			pUI->updateTime = now;
		}
	}

#ifdef WIN32
	if( CheckStdin )  {
		dspFlag = 0;
		while( kbhit() )  {
			ch = getch();
			if( ch == 'c' || ch == 'C' )  {
				if( ConsoleDisplay )
					ClearScreen();
				memset( messages, 0, sizeof( messages ) );
				numberMessages = 0;
				ClearUserData();
			}
			dspFlag = TRUE;
		}
		if( dspFlag )  {
			DisplayReport();
			displayCount = 0;
		}
	}
#endif
		
	if( RefreshTime && ( ++displayCount >= RefreshTime ) )  {
		displayCount = 0;
		DisplayReport();
	}
}

void ms_log( int level, char *pszFormat, ... )
{
	int retval;
	char buff[ 1024 ];
	va_list varlist;
	va_start( varlist, pszFormat );
	vsprintf( buff, pszFormat, varlist );
	va_end (varlist);
	LogWriteThread("(%d) %s", level,  buff );
}

void ExitWs2Ew( int exitcode )
{
	int i, max = 100;

	exitWs2Ew = TRUE;
	
	if( ListenMode )  {
		exitListenThread = TRUE;
		while( max-- )  {
			if( exitListenAck )
				break;
			sleep_ew( 50 );
		}
	}
	
	if( activeUsers )  {
		for( i = 0; i != MAX_CONNECT_USERS; i++ )
			userInfo[ i ].exitThread = TRUE;
		max = 100;
		while( max-- )  {
			if( !activeUsers )
				break;
			sleep_ew( 50 );
		}
	}

	/* Clean up and exit program */
	tport_detach( &OutRegion );
	
	LogWrite( "", "Ws2Ew Exiting\n" );
	
	exit( exitcode );
}

void CntlHandler( int sig )
{
	/* if control-c clean up and exit program if ControlCExit = 1 */
	if( sig == 2 )  {
		if( ControlCExit )  {
			LogWrite( "", "Exiting Program...\n");
			DisplayReport();
			ExitWs2Ew( -1 );
		}
		else
			signal(sig, SIG_IGN);
	}
}

int InitWs2Ew( char *configFile )
{
	int i;
	
	start_time = time( NULL );
	
	memset( messages, 0, sizeof( messages ) );
	numberMessages = 0;
	memset( connectInfo, 0, sizeof( connectInfo ) );
		
	signal( SIGINT, CntlHandler );
	
	/* Initialize name of log-file & open it */
	logit_init( configFile, 0, 256, 1 );

	/* Read configuration parameters */
	if ( GetConfig( configFile ) < 0 )  {
		printf("Ws2Ew: Error reading configuration file. Exiting.\n");
		logit( "e", "Ws2Ew: Error reading configuration file. Exiting.\n");
		return 0;
	}

	/* Initialize the console display */
	if( ConsoleDisplay )
		InitCon();

	/* Set up the logo of outgoing waveform messages */
	if ( GetLocalInst( &InstId ) < 0 )  {
		LogWrite( "e", "Ws2Ew: Error getting the local installation id. Exiting.\n" );
		return 0;
	}
	LogWrite( "", "Local InstId:	 %u\n", InstId );

	/* Log the configuration file */
	LogConfig();

	/* Get our Process ID for restart purposes */
	MyPid = getpid();
	if( MyPid == -1 )  {
		LogWrite("e", "Ws2Ew: Cannot get PID. Exiting.\n" );
		return 0;
	}
	
	memset( ackOutBuff, 0, sizeof( ackOutBuff ) );
	strcpy( ackOutBuff, "@@ACK\r\n" );
	
	logo.instid = InstId;
	logo.mod = ModuleId;
	GetType( "TYPE_TRACEBUF2", &logo.type );

	/* Attach to existing transport ring and send first heartbeat */
	tport_attach( &OutRegion, OutKey );
	LogWrite( "", "Attached to transport ring: %d\n", OutKey );
	
	Heartbeat( start_time );

	/* Init tcp stuff */
	if( !InitSocket() )  {
		tport_detach( &OutRegion );
		LogWrite( "", "InitSocket Error\n" );
		return 0;
	}

	/* Init queue stuff. One for the EW ring receive data thread and the other for messages */
  	initqueue( &tbQ, 10, 4096 );
  	initqueue( &msgQ, 10, 256 );
	
	CreateSpecificMutex( &tbMx );
	CreateSpecificMutex( &msgMx );		// message queue Mx
	CreateSpecificMutex( &lsMx );		// listen sock Mx
	CreateSpecificMutex( &rsMx );		// receive sock Mx
	CreateSpecificMutex( &userMx );		// user add/remove Mx
	
	exitListenAck = 0;
	sampleCount = 0;
	
	activeUsers = 0;
	memset( userInfo, 0, sizeof( userInfo ) );
	for( i = 0; i != MAX_CONNECT_USERS; i++ )
		userInfo[i].sock = SOCKET_ERROR;
	
	for( i = 0; i != MAX_CHAN_LIST ; i++ )
		ChanList[i].sequenceNumber = -1;
	return TRUE;
}

ConnectInfo *FindOrAddUser( UserInfo *pUI, int addOk )
{
	ConnectInfo *pCI;
	int i;
	
	for( i = 0; i != MAX_CONNECT_USERS; i++ )  {
		pCI = &connectInfo[ i ];
		if( !strcmp( pUI->who, pCI->who ) )  {
			return pCI;
		}
	}
		
	if( !addOk )
		return NULL;
		
	for( i = 0; i != MAX_CONNECT_USERS; i++ )  {
		pCI = &connectInfo[ i ];
		if( !pCI->who[0] )  {
			strcpy( pCI->who, pUI->who );
			pCI->connectCount = 0;
			return pCI;
		}
	}
	return NULL;
}

int main( int argc, char *argv[] )
{
	UserInfo *pUI;
	int max;
		
	/* Get command line arguments */
	if ( argc < 2 )  {
		printf( "Usage: Ws2Ew <config file>\n" );
		return -1;
	}

	if( !InitWs2Ew( argv[1] ) )
		return -1;
	
	if( ListenMode )  {
		LogWrite( "", "Starting Listen Thread\n");
		if( StartThread( ListenThread, (unsigned)THREAD_STACK, &tidListen ) == -1 )  {
			LogWrite( "e", "Error starting receive thread. Exiting.\n" );
			ReportError( 7, "Thread Start Error");
			tport_detach( &OutRegion );
      		return -1;
      	}
	}
	else  {
		LogWrite( "", "Starting Receive Thread\n");
		if( StartThread( ReceiveThread, (unsigned)THREAD_STACK, &tidReceive ) == -1 )  {
			LogWrite( "e", "Error starting receive thread. Exiting.\n" );
			ReportError( 7, "Thread Start Error");
			tport_detach( &OutRegion );
      		return -1;
      	}
	}

	ProcessLoop();
	
	ExitWs2Ew( 0 );
}

void CheckUsers( ConnectInfo *pCI, time_t now )
{
	time_t diff;
	
	if( !pCI->pUI )  {
		LogWrite( "", "CheckUser %s No pUI ptr\n", pCI->pUI->who );
		return;
	}
	
	diff = now - pCI->pUI->updateTime;
	if( diff >= NoDataWaitTime && !pCI->pUI->noDataReport )  {
		ReportError( 2, "%s No Data Timeout", pCI->pUI->who );
		pCI->pUI->noDataReport = TRUE;
	}
}
