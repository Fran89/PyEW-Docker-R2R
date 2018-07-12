

      /*********************************************************************
       *                              makeTrace.c                          *
       *                                                                   *
       *                 for trace data load testing                       *
       *********************************************************************/

/*                                     Story

   This is derived from adsend. It's purpose is to execute a test ordered by
   Ray Buland: To have one machine producing trace_buf's, and broadcasting
   them onto a dedicated wire. To have two machines listening: one running
   WaveServer, the other an export. To then crank up the rate of trace_buf's 
   until something breaks. Thus I was directed to do. Alex 11/12/02
*/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include <ctype.h>
#include <conio.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <string.h>
#include <process.h>    /* for _getpid() */
#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include "makeTrace.h"

/* Functions prototypes
   ********************/
void CombGuideChk( int *, int, int * );
int  GetArgs( int, char **, char ** );
int  GetConfig( char * );
int  GetDAQ( short *, unsigned long * );
int  GuideChk( short *, int, int, int, double *, double *, double, double );
void InitCon( void );
void InitDAQ( int );
void irige_init( struct TIME_BUFF * );
int  Irige( struct TIME_BUFF *, long, int, int, short *, int, int );
void LogAndReportError( int, char * );
void LogConfig( void );
void LogSta( int, SCN * );
void PrintGmtime( double, int );
void LogitGmtime( double, int );
int  ReadSta( char *, unsigned char, int, SCN * );
void ReportError( int, char * );
void SetCurPos( int, int );
void StopDAQ( void );
int  NoSend( int );
int  NoTimeSynch( int );
int  inc( SCN* );

/* Global configuration parameters
   *******************************/
extern unsigned char ModuleId;   // Data source id placed in the trace header
extern int    OnboardChans;      // The number of channels on the DAQ board
extern int    NumMuxBoards;      // Number of mux cards
extern double ChanRate;          // Rate in samples per second per channel
extern long    ChanMsgSize;			// Message size in samples per channel
extern int    NumTimeCodeChan;   // How many time-code channels there are
extern int    *TimeCodeChan;     // The chans where we find the IRIGE signals
extern long   OutKey;            // Key to ring where traces will live
extern int    Year;              // The current year
extern int    NumGuide;          // Number of guide channels
extern int    *GuideChan;        // The chans where we find the reference voltage
extern int    SendBadTime;       // Send data even if no IRIGE signal is present
extern int    UpdateSysClock;    // Update PC clock with good IRIGE time
extern int    YearFromPC;        // Take year from PC instead of "Year"

extern int    Nchan;             // Number of channels to send
extern int		Rate;
extern int    IrigeIsLocalTime;  // Set to 1 if IRIGE represents local time; 0 if GMT time.
extern int    ErrNoLockTime;     // If no guide lock for ErrNoLockTime sec, report an error
extern int    Debug=0;        

/* Global variables
   ****************/
SHM_INFO      OutRegion;         // Info structure for output region
pid_t         MyPid;             // process id, sent with heartbeat
static SCN    ChanList[MAX_CHAN];         // Array to fill with SCN values


int main( int argc, char *argv[] )
{
	double	startTime;			// Header start time
	double  tnow, twait, tfirst;
	double  tnextSend;
	double  packetDt;
	double  PtsSent=0;
	double  PktsSent=0;
	double  tShowNext=0;
	unsigned long nPacket=0;			// number of packets sent
   int       i, j,             // Loop indexes
             traceBufSize;     // Size of the trace buffer, in bytes
   int      *halfBuf,          // Pointer to half buffer of A/D data
            *traceDat;         // Where the data points are stored in the trace msg
   char     *traceBuf;         // Where the trace message is assembled
   unsigned  halfBufSize,      // Size of the half buffer in number of samples
             scan = 0;         // Scan number of first scan in a message
   unsigned char InstId;       // Installation id placed in the trace header
   MSG_LOGO  logo;             // Logo of message to put out
   TRACE_HEADER    *traceHead; // Where the trace header is stored
   SCN			seedName;
   int           rc;                     // Function return code
   int         *tracePtr;              // Pointer into traceBuf for demuxing
   int         *halfPtr;               // Pointer into halfBuf for demuxing

/* Get command line arguments
   **************************/
   if ( argc < 2 )
   {
      printf( " missing command file name\n");
      return -1;
   }

/* Initialize name of log-file & open it
   *************************************/
   logit_init( argv[1], 0, 256, 1 );

/* Read configuration parameters
   *****************************/
   if ( GetConfig( argv[1] ) < 0 )
   {
      printf( "makeTrace: Error reading configuration file. Exiting.\n" );
      return -1;
   }

/* Get our Process ID for restart purposes
   ***************************************/
   MyPid = _getpid();
   if( MyPid == -1 )
   {
      logit("e", "makeTrace: Cannot get PID. Exiting.\n" );
      return -1;
   }
   logit(""," MakeTrace version 1.0 \n");

/* Set up the logo of outgoing waveform messages
   *********************************************/
   if ( GetLocalInst( &InstId ) < 0 )
   {
      printf( "MakeTrace: Error getting the local installation id. Exiting.\n" );
      return -1;
   }
   if( Debug ==1) logit( "", "Local InstId:         %8u\n", InstId );

   logo.instid = InstId;
   logo.mod    = ModuleId;
   GetType( "TYPE_TRACEBUF", &logo.type );

/* Allocate some array space
   *************************/
   halfBufSize = (int)(ChanMsgSize * Nchan);
   if( Debug ==1) logit( "t", "Half buffer size: %u samples\n", halfBufSize );

   halfBuf = (int *) calloc( halfBufSize, sizeof(int) );
   if ( halfBuf == NULL )
   {
      logit( "", "Cannot allocate the A/D buffer\n" );
      return -1;
   }
   traceBufSize = sizeof(TRACE_HEADER) + (ChanMsgSize * sizeof(int));
   if ( traceBufSize > MAX_TRACEBUF_SIZ)
   {
	   logit("e","Trace Buffer too large: %d\n", traceBufSize);
	   logit("e","Max legal size: %d\n",MAX_TRACEBUF_SIZ);
	   return -1;
   }
   if( Debug ==1) logit( "e", "Trace buffer size: %d bytes\n", traceBufSize );
   if( Debug ==1) logit( "e", "Trace buffer header size: %d bytes\n", sizeof(TRACE_HEADER) );
   traceBuf = (char *) malloc( traceBufSize );
   if ( traceBuf == NULL )
   {
      logit( "", "Cannot allocate the trace buffer\n" );
      return -1;
   }
   traceHead = (TRACE_HEADER *) &traceBuf[0];
   traceDat  = (int *) &traceBuf[sizeof(TRACE_HEADER)];


   /* make up a station name list
   ******************************/
   if (Nchan > MAX_CHAN)
   {
	   logit("e"," %d channels requested. %d is max\n",Nchan,MAX_CHAN);
	   return -1;
   }

   strcpy(seedName.sta,"AAAAA");
   strcpy(seedName.comp, "AAA");
   strcpy(seedName.net, "AA");
   logit("","Nchan: %d\n",Nchan);
   logit("","ChanMsgSize: %d\n",ChanMsgSize);  
   logit("","ChanRate: %f\n",ChanRate);

	for (i=0; i<Nchan; i++)
	{

		strcpy( ChanList[i].sta, seedName.sta);
		strcpy( ChanList[i].comp, seedName.comp);
		strcpy( ChanList[i].net, seedName.net);
		ChanList[i].pin = i;
		if(Debug ==1)logit("","%8d %s %s %s\n",
			ChanList [i].pin,ChanList[i].sta, ChanList[i].comp, ChanList[i].net);
		if( inc(&seedName)<0)
		{
			logit("e","Name inc failed\n");
			return -1;
		}
	}

	/* Initialize the console display
	******************************/
	InitCon();
	SetCurPos(4,6); printf("Nchan: %d",Nchan);
	SetCurPos(4,7); printf("MsgSize: %d",ChanMsgSize);
	SetCurPos(4,8); printf("Rate: %f",ChanRate);

/* Attach to existing transport ring and send first heartbeat
   **********************************************************/
   tport_attach( &OutRegion, OutKey );
   logit( "t", "Attached to transport ring: %d\n", OutKey );

	/* get initial time for the header time stamps
	**********************************************/
	hrtime_ew(&startTime);
	if( Debug ==1) logit("","startTime: %lf\n",startTime);

	/* calculate inter-packet time
	******************************/
	packetDt = 1.0 / ( Nchan * (ChanRate/ChanMsgSize) );
	if( Debug ==1) logit("","packetDt: %f\n",packetDt);
	SetCurPos(40,6); printf("Inter packet time: %f sec",packetDt);

/************************* The main program loop   *********************/

  while ( tport_getflag( &OutRegion ) != TERMINATE  &&
	   tport_getflag( &OutRegion ) != MyPid         )
   {
	   /* Update the scan count
	   *********************/
	   scan += (unsigned)ChanMsgSize;
	   
	   SetCurPos( 10, 10 );
	   printf( "%d", scan );
	   
	  /* Position the screen cursor for error messages
	  *********************************************/
      SetCurPos( 4, 22 );
	  
	  /* Loop through the trace messages to be sent out
	  **********************************************/
      for ( i = 0; i < Nchan; i++ )
      {
		  /* Fill the trace buffer header
		  ****************************/
		  traceHead->nsamp      = ChanMsgSize;         // Number of samples in message
		  traceHead->samprate   = ChanRate;            // Sample rate; nominal
		  traceHead->quality[0] = '\0';                // One bit per condition
		  traceHead->quality[1] = '\0';                // One bit per condition
		  
		  strcpy( traceHead->datatype, "i4" );         // Data format code
		  strcpy( traceHead->sta,  ChanList[i].sta );  // Site name
		  strcpy( traceHead->net,  ChanList[i].net );  // Network name
		  strcpy( traceHead->chan, ChanList[i].comp ); // Component/channel code
		  traceHead->pinno = ChanList[i].pin;          // Pin number
		  
		  /* Set the trace start and end times.
		  Times are in seconds since midnight 1/1/1970
		  ********************************************/
		  traceHead->starttime = startTime;
		  traceHead->endtime   = traceHead->starttime + (ChanMsgSize - 1)/ChanRate;
		  startTime = startTime + (double)ChanMsgSize/ChanRate;
		  
		  /* Transfer ChanMsgSize samples from halfBuf to traceBuf
		  *****************************************************/
		  tracePtr = &traceDat[0];
		  halfPtr  = &halfBuf[i];
		  for ( j = 0; j < ChanMsgSize; j++ )
		  {
			  traceDat[j] = j%10;
		  }
		  
		  /* sleep a bit if necessary
		  ***************************/
		  /* Mystery: I had weird results (sometimes) with the following:
		  hrtime_ew( &tnow ); 
		  Doing it in-line, as below, works. weird, no? */
		  {
			  struct _timeb t;
			  
			  _ftime( &t );
			  SetCurPos( 4, 22 );
			  //logit("","t.time: %ld  t.millitm: %u\n",t.time, t.millitm);
			  tnow = (double)t.time + (double)t.millitm*0.001;
		  }
		  SetCurPos( 4, 22 );
		  if(Debug ==1) logit("","\nnPacket: %d\n",nPacket);
		  if(Debug ==1) logit(""," tnow:     %lf\n", tnow);
		  
		  if(nPacket == 0 ) tfirst = tnow;	// mark the time of the first packet

		  tnextSend = nPacket*packetDt + tfirst;
		  SetCurPos( 4, 22 );
		  if(Debug ==1) logit("","tnextSend: %lf\n",tnextSend);
		  twait = tnextSend - tnow;
		  if (twait > 0 )
		  {
			  sleep_ew((unsigned)(twait*1000. +.5));
			  SetCurPos( 4, 22 );
			  if(Debug ==1) logit("e","sleeping: %d\n", (unsigned)(twait*1000. +.5));
				  else
				  printf("sleeping: %d\n", (unsigned)(twait*1000. +.5));
		  }

		  /* Send the message
		  *******************/
		  rc = tport_putmsg( &OutRegion, &logo, traceBufSize, traceBuf );
		  if ( rc == PUT_TOOBIG )
		  {
			  logit("e","Trace message for channel %d too big\n", i );
			  return -1;
		  }
		  if ( rc == PUT_NOTRACK )
		  {
			  logit("e","Tracking error while sending channel %d\n", i );
			  return -1;
		  }
		  nPacket++;	// count outgoing packets

		  /* keep statistics on what' been sent
		  *************************************/
		  PtsSent = PtsSent + ChanMsgSize;
		  PktsSent++;

		  /* Show data rates
		  ******************/
		  if (tnow >= tShowNext )
		  {
			  SetCurPos( 18, 12 );
			  printf( "%6.lf", nPacket*ChanMsgSize/(tnow-tfirst) );
			  SetCurPos( 18, 14 );
			  printf( "%6.1lf", nPacket/(tnow-tfirst) );
			  tShowNext = tnow + 2.0;
		  }
      }
      SetCurPos( 0, 0 );
   }

/* Clean up and exit program
   *************************/
   free( halfBuf );
   free( ChanList );

   logit( "t", "makeTrace terminating.\n" );
   return 0;
}


int inc( SCN* name)
{
	
	name->sta[4] = name->sta[4]+1;
	if (name->sta[4] == '[')
	{
		name->sta[4] = 'A'; 
		name->sta[3]++; 
	}
		if (name->sta[3] == '[')
	{
		name->sta[3] = 'A'; 
		name->sta[2]++; 
	}
		if (name->sta[2] == '[')
	{
		name->sta[2] = 'A'; 
		name->sta[1]++; 
	}
		if (name->sta[1] == '[')
	{
		name->sta[1] = 'A'; 
		name->sta[0]++; 
	}
		if (name->sta[0] == '[')
	{
		name->sta[0] = 'A'; 
		name->comp[2]++;		// overflow into component name
	}
		if (name->comp[2] == '[')
	{
		name->comp[2] = 'A'; 
		name->comp[1]++; 
	}
		if (name->comp[1] == '[')
	{
		name->comp[1] = 'A'; 
		name->comp[0]++; 
	}
		if (name->comp[0] == '[')
	{
		name->comp[0] = 'A'; 
		name->net[1]++;		// overflow into network name
	}
		if (name->net[1] == '[')
	{
		name->net[1] = 'A'; 
		name->net[0]++; 
	}
		if (name->net[0] == '[')
	{
		logit("e","Cannot generate this many names\n");
		return -1; 
	}

	return 0;
}
