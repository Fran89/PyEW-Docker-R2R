#include <afxwin.h>			// MFC core and standard components
#define NO_VBX_SUPPORT		// to prevent conflict with DDEML
#include <afxext.h> 		// MFC extensions (including VB)
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <conio.h>
#include <ctype.h>
#include <time.h>

#define BYTE unsigned char
#define WORD unsigned short

#define BAUD		38400

int OpenPort(char *, int, HANDLE *);
void ClosePort();

HANDLE hPort = (HANDLE)-1;
BYTE inbuff[8192], *pInBuff, sampleData[8192], *pSample;
int totalSamples, inCount, inSync, inState, saveNum;
int sampleCnt, checkSps, doneFlag;

int SampleRate = 50, NumberChannels = 4;

long sampleBuffer[200 * 8 * 2 ];		// holds one second worth of data
int sampleNumber;
SYSTEMTIME bufferTime;

int FixSample( BYTE chanId, BYTE *pData, long *to )
{
	long sample;
	BYTE *lp = (BYTE *)&sample;
	
	/* Check the sample ID and end byte */
	if( chanId != pData[0] )
		return 0;
	if( ( pData[4] & 0xf8 ) != 0xf8 )
		return 0;
	
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
	
	if( sampleNumber < totalSamples )  {
		*to = sample;
		++sampleNumber;
	}
	return 1;	
}

/* Process new sample data */
void NewSampleData()
{
	BYTE *pData;
	
	if( sampleData[0] == 0x81 )  {		// if time data skip to first sample
		pData = &sampleData[9];
		GetSystemTime( &bufferTime );
		sampleNumber = 0;
	}
	else
		pData = sampleData;
	if( !FixSample( 0x82, pData, &sampleBuffer[sampleNumber] ) )
		return;
	pData += 5;
	if( !FixSample( 0x83, pData, &sampleBuffer[sampleNumber] ) )
		return;
	pData += 5;
	if( !FixSample( 0x84, pData, &sampleBuffer[sampleNumber] ) )
		return;
		
	if( sampleNumber >= totalSamples )  {
		printf("New Data %d %d %d\n", sampleBuffer[0], sampleBuffer[1], sampleBuffer[2] );
	}
}

void GetAdVersion()
{
	BYTE *ptr, buff[6] = { 0x81, 0, 0, 0, 0, 0 };
	DWORD sent;
	COMSTAT ComStat;
	DWORD dwErrFlags, num, len;
	
	WriteFile( hPort, buff, 6, &sent, NULL );

	while( 1 )  {
		ClearCommError(hPort, &dwErrFlags, &ComStat);
		if(len = ComStat.cbInQue)  {
			if(len > 8192)
				len = 8192;
			ReadFile(hPort, inbuff, (DWORD)len, &num, NULL);
			ptr = inbuff;
			while( num-- )  {
				if( *ptr > 128 )
					printf("%d\n", *ptr++ );
				else
					printf("%c\n", *ptr++ );
			}
		}
	}
}

/* Send command to the A/D board to set the baud rate */
void SetSpsRate( int sps )
{
	char *ptr, buff[6] = { 0x84, 0, 0, 0, 0, 0 };
	DWORD sent;
	
	sps = 200 / sps;
	buff[1] = buff[2] = buff[3] = buff[4] = sps;
	WriteFile( hPort, buff, 6, &sent, NULL );
}

/* Process the incomming data */
/* 
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
			ClearCommError(hPort, &dwErrFlags, &ComStat);
			if( ! ( rdlen = ComStat.cbInQue ) )  {
				_sleep( 10 );
				continue;
			}	
			if(rdlen > 8192)
				rdlen = 8192;
		
			ReadFile(hPort, inbuff, rdlen, &rdnum, NULL);
			inCount = rdlen;
			pInBuff = inbuff;
		}	
	
		while( inCount )  {
			--inCount;
			data = *pInBuff++;
			printf("%02x\n", data );
			if( !inSync )  {
				if( !inState )  {
					if( data != 0x81 )  {
						continue;
					}
					inState = 1;
					saveNum = 8;
					pSample = sampleData;
					*pSample++ = data;
					continue;
				}
				if( inState == 1 || inState == 3 )  {
					*pSample++ = data;
					--saveNum;
					if( !saveNum )  {
						++inState;
						saveNum = 5;
					}
					continue;
				}
				if( inState == 2 )  {
					if( data != 0x82 )  {
						inState = 0;
						printf("0x82 sync error\n");
						continue;
					}
					*pSample++ = data;
					--saveNum;
					inState = 3;
					saveNum = 4;
					continue;
				}
				if( inState == 4 )  {
					if( data != 0x83 )  {
						inState = 0;
						printf("0x83 sync error\n");
						continue;
					}
					*pSample++ = data;
					--saveNum;
					inState = 5;
					saveNum = 4;
					continue;
				}
				if( inState == 5 )  {
					*pSample++ = data;
					--saveNum;
					if( !saveNum )  {
						++inState;
						saveNum = 4;
					}
					continue;
				}
				if( inState == 6 )  {
					if( data != 0x84 )  {
						inState = 0;
						printf("0x84 sync error\n");
						continue;
					}
					*pSample++ = data;
					--saveNum;
					inState = 7;
					saveNum = 4;
					continue;
				}
				if( inState == 7 )  {
					*pSample++ = data;
					--saveNum;
					if( !saveNum )  {
						inSync = 1;
						inState = 8;
						NewSampleData();
						pSample = sampleData;
						sampleCnt = 1;
					}
					continue;
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
							if( sampleCnt != SampleRate )  {
								printf("Sample rate error\n");
								SetSpsRate( SampleRate );
								inSync = inState = inCount = 0;
								continue;
							}
							checkSps = 0;
						}
						saveNum = 24;
						sampleCnt = 1;
					}
					else  {
						inSync = inState = 0;
						continue;
					}
					NewSampleData();
					pSample = sampleData;
				}
				*pSample++ = data;
				--saveNum;
			}
		}
	}
	return 0;
}
*/

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
			ClearCommError(hPort, &dwErrFlags, &ComStat);
			if( ! ( rdlen = ComStat.cbInQue ) )  {
				_sleep( 10 );
				continue;
			}	
			if(rdlen > 8192)
				rdlen = 8192;
		
			ReadFile(hPort, inbuff, rdlen, &rdnum, NULL);
			inCount = rdlen;
			pInBuff = inbuff;
		}	
	
		while( inCount )  {
			--inCount;
			data = *pInBuff++;
//			printf("%02x state=%d\n", data & 0xff, inState );
 			if( !inSync )  {
				if( !inState )  {
					if( data != 0x81 )  {
						continue;
					}
					inState = 1;
					saveNum = 8;
					pSample = sampleData;
					*pSample++ = data;
					continue;
				}
				if( inState == 1 || inState == 3 || inState == 5 || inState == 7 )  {
					*pSample++ = data;
					--saveNum;
					if( !saveNum )  {
						++inState;
						saveNum = 3;
					}
					continue;
				}
				if( inState == 2 )  {
					if( data != 0x82 )  {
						inState = 0;
						printf("0x82 sync error\n");
						continue;
					}
					*pSample++ = data;
					--saveNum;
					inState = 3;
					saveNum = 3;
					continue;
				}
				if( inState == 4 )  {
					if( data != 0x83 )  {
						inState = 0;
						printf("0x83 sync error\n");
						continue;
					}
					*pSample++ = data;
					--saveNum;
					inState = 5;
					saveNum = 3;
					continue;
				}
				if( inState == 6 )  {
					if( data != 0x84 )  {
						inState = 0;
						printf("0x84 sync error\n");
						continue;
					}
					*pSample++ = data;
					inState = 7;
					saveNum = 3;
					continue;
				}
				if( inState == 8 )  {
					if( data != 0x85 )  {
						inState = 0;
						printf("0x85 sync error\n");
						continue;
					}
					*pSample++ = data;
					inState = 9;
					saveNum = 3;
					continue;
				}
				if( inState == 9 )  {
					*pSample++ = data;
					--saveNum;
					if( !saveNum )  {
						++inState;
						printf("insync\n");
						inSync = 1;
						inState = 10;
						NewSampleData();
						pSample = sampleData;
						sampleCnt = 1;
						saveNum = 0;
					}
					continue;
				}
			}
			else  {
				if( !saveNum )  {
					if( data == 0x82 )  {
//						printf("data=82\n");
						saveNum = 16;
						++sampleCnt;
					}
					else if( data == 0x81 )  {
//						printf("data=81\n");
						if( checkSps )  {
							if( sampleCnt != SampleRate )  {
								printf("Sample rate error\n");
								SetSpsRate( SampleRate );
								inSync = inState = inCount = 0;
								continue;
							}
							checkSps = 0;
						}
						saveNum = 25;
						sampleCnt = 1;
					}
					else  {
						printf("sync error\n");
						inSync = inState = 0;
						continue;
					}
					NewSampleData();
					pSample = sampleData;
				}
				*pSample++ = data;
				--saveNum;
			}
		}
	}
	return 0;
}

void main(int argc, char *argv[])
{
	int len, cnt, sts;
	FILE *fp;
	char *ptr;
			
	if(!OpenPort("COM5", BAUD, &hPort))  {
		printf("Open port 2 error\n");
		exit(1);
	}
	
	totalSamples = SampleRate * NumberChannels;
	SetSpsRate( SampleRate );
		
	ProcessData();
	
	ClosePort();
}

/* Open Comm Port */
int OpenPort( char *portStr, int speed, HANDLE *port )
{
	int ret;
	DCB dcb;
	COMMTIMEOUTS CommTimeOuts;
	HANDLE hPort;
		  
	hPort = CreateFile(portStr, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if(hPort == (HANDLE)-1)  {
		printf("Comm Port Open Error.\n");
		return(FALSE);
	}
	
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
