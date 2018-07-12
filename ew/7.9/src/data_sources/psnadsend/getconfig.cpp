/* getconfig.cpp */

#include "PsnAdSend.h"

/* The configuration file parameters */
char CommPortHostStr[256];	// Comm port to use or TCP/IP Server
int TcpPort;				// Port number if TCP/IP Connection
int TcpMode;				// If 1 CommPortHostStr = TCP/IP Host 
int TotalChans;        		// Number of channels in ChanList (AdcChans + AddChans)
int AdcChans;				// Number of real ADC channels from the "Chan" scnl line
int AddChans;				// Number of additional or derived channels from the "AddChan" scnl line
unsigned char ModuleId;		// Module id of this program
int	ChanRate;			  	// Rate in samples per second per channel
LONG OutKey;				// Key to ring where traces will live
LONG MuxKey;				// Key to ring where raw mux data will live
int HeartbeatInt;		 	// Heartbeat interval in seconds
int UpdateSysClock;	  		// Use a good IRIGE time to update the PC clock
SCNL ChanList[ MAX_CHANNELS]; // Array to fill with SCNL values
int NoDataTimeout;			// Take action if no data sent for this many seconds
int TimeRefType;			// Time reference type. See .d file for types
LONG PortSpeed;				// Comm port baud rate
int HighToLowPPS;			// 1PPS input signal direction
int	NoPPSLedStatus;			// Stops blinking of the LED
int	TimeOffset;				// Time reference offset in milliseonds
int LogMessages;			// Log messages from the DLL and ADC board
int AdcDataSize;			// Data size sent to the trace buffer. Either 2 or 4 Bytes
int ConsoleDisplay;			// Windows Only, if 1 use console functions like SetPos()
							// to display process info.
int ControlCExit;			// if 0 program ignores control-c, if 1 exit program							
int RefreshTime;			// Console refresh time in seconds. ConsoleDisplay must be 1.
int CheckStdin;				// If 1, check for user input
int NoSendBadTime;			// If 1, don't send data packets to the ring if time not locked to GPS time
int ExitOnTimeout;			// If 1, Exit on data timeout. 
int Debug;					// If 1, display/log debug information
int SystemTimeError;		// Time test between A/D board and System time. 0 = no time test
int PPSTimeError;			// Report error if 1PPS diff exceeds this amount in milliseconds
int NoLockTimeError;		// Report error if time reference not locked exceeds this amount in minutes
int FilterSysTimeDiff;		// if 1 do not log System to A/D board time difference messages
int FilterGPSMessages;		// if 1 do not log GPSRef messages
int StartTimeError;			// Set system time using GPS at startup if error exceeds this amount in seconds

char TimeFileName[ 256 ];	// Root name and where to save the time information

#define NCOMMAND	15		// Number of mandatory commands in the config file

#define TIME_REF_PCTIME				0		// Use PC Time
#define TIME_REF_GARMIN				1		// Garmin 16/18
#define TIME_REF_MOT_NMEA			2		// Motorola NMEA
#define TIME_REF_MOT_BIN			3		// Motorola Binary
#define TIME_REF_WWV				4		// WWV Mode
#define TIME_REF_WWVB				5		// WWVB Mode
#define TIME_REF_SKG				6		// Sure Electronics SKG GPS Board

/* Used to convert the config string to TIME_REF_xxx type */
TimeRefInfo timeRefInfo[] = { 
	{ TIME_REF_PCTIME, 		"PC" }, 
	{ TIME_REF_GARMIN, 		"GARMIN" },
	{ TIME_REF_MOT_NMEA, 	"MOT_NMEA" },
	{ TIME_REF_MOT_BIN, 	"MOT_BIN" },
	{ TIME_REF_WWV, 		"WWV" },
	{ TIME_REF_WWVB, 		"WWVB" },
	{ TIME_REF_SKG, 		"SKG" },
	{ TIME_REF_4800, 		"OEM_4800" },
	{ TIME_REF_9600, 		"OEM_9600" },
	{ 0, 0 }				// end mark 
};

int GetChanFilter()
{
	char *s, type;
	int ch, poles;
	double freq, sensorQ, hpFreq, hpQ;
	SCNL *pSCNL;
		
	if( (ch = k_int() ) <= 0 || ch > MAX_CHANNELS )  {
		LogWrite("e", "Parse Filter Channel Error Must be 1 to %d\n", MAX_CHANNELS );
		return 0;
	}
	pSCNL = &ChanList[ ch-1 ];		// get the pointer to the channel
		
	if( ( s = k_str() ) == NULL )  {
		LogWrite("e", "Parse Filter Type Error\n" );
		return 0;
	} 
	
	type = toupper( s[0] );
	
	if( type != 'L' && type != 'H' && type != 'I' )  {
		LogWrite("e", "Parse Filter Type Not 'L'owpass, 'H'ighpass or 'I'nverse Error Type=%c Channel=%d\n", 
			type, ch );
		return 0;
	}
	
	/* Get low-pass and high-pass info */
	if( type == 'L' || type == 'H' )  {
		if( ( s = k_str() ) == NULL )  {				// Get filter cut-off frequence
			LogWrite("e", "Parse Filter No Cutoff Freq Error Type=%c Channel=%d\n", type, ch );
			return 0;
		}	
		freq = atof( s );
		if( ( poles = k_int() ) <= 1 || poles > 64 )  {	// Get filter poles
			LogWrite("e", "Parse Filter Poles Error Channel=%d\n", ch );
			return 0;
		}	
		
		/* Now try and create the filter based on type */
		if( type == 'L' )  {	
			pSCNL->lpFilter = new CFilter;
			pSCNL->lpFilter->Setup( type, freq, poles, ChanRate );
			if( pSCNL->lpFilter->FilterError() )  {
				LogWrite("e", "Parse Lowpass Filter Setup Error Channel=%d Freq=%g Poles=%d\n", ch, freq, poles );
				delete pSCNL->lpFilter;
				pSCNL->lpFilter = NULL;
				return 0;
			}
		}
		else if( type == 'H' )  {	
			pSCNL->hpFilter = new CFilter;
			pSCNL->hpFilter->Setup( type, freq, poles, ChanRate );
			if( pSCNL->hpFilter->FilterError() )  {
				LogWrite("e", "Parse Filter Setup Error Channel=%d Freq=%g Poles=%d\n", ch, freq, poles );
				delete pSCNL->hpFilter;
				pSCNL->hpFilter = NULL;
				return 0;
			}
		}
	}
	else  {			// filter type must be inverse or period extending
		if( ( s = k_str() ) == NULL )  {			// Get sensor frequence
			LogWrite("e", "Parse Inverse Filter No Sensor Freq Error Channel=%d\n", ch );
			return 0;
		}	
		freq = atof( s );
		if( ( s = k_str() ) == NULL )  {			// Get sensor Q
			LogWrite("e", "Parse Inverse Filter No Sensor Q Error Channel=%d\n", ch );
			return 0;
		}	
		sensorQ = atof( s );
		if( ( s = k_str() ) == NULL )  {			// Get high-pass filter freq
			LogWrite("e", "Parse Inverse Filter No HP Filter Freq Error Channel=%d\n", ch );
			return 0;
		}	
		hpFreq = atof( s );
		if( ( s = k_str() ) == NULL )  {			// Get high-pass filter Q
			LogWrite("e", "Parse Inverse Filter No HP Q Error Channel=%d\n", ch );
			return 0;
		}	
		hpQ = atof( s );

		/* Now create the filter */
		pSCNL->invFilter = new CPeriodExtend;
		pSCNL->invFilter->Setup( freq, sensorQ, hpFreq, hpQ, ChanRate );
	}
	return 1;
}

int GetChanScnl( int useChannel )
{
	char *s, yn;
	int len, i;
	SCNL *pSCNL = &ChanList[ TotalChans ];
		
	pSCNL->useChannel = useChannel;		// -1 = real ADC channel or index to real channel
	
	// Set some defaults
	pSCNL->bitsToUse = 16;
	pSCNL->adcGain = 1;

	if( (s = k_str() ) == NULL )  {
		LogWrite("e", "Parse Station Error\n" );
		return 0;
	}
	len = strlen( s );
	if( !len || ( len > ( (int)sizeof( pSCNL->sta ) - 1 ) ) )  {
		LogWrite("e", "Parse SCNL Station Length Error Max = %d\n", sizeof( pSCNL->sta ) - 1 );
		return 0;
	}
	strcpy( pSCNL->sta,  s );
		
	if( (s = k_str() ) == NULL )  {
		LogWrite("e", "Parse Comp Error\n" );
		return 0;
	}
	len = strlen( s );
	if( !len || ( len > ( (int)sizeof( pSCNL->comp ) - 1 ) ) )  {
		LogWrite("e", "Parse SCNL Comp Length Error Max = %d\n", sizeof( pSCNL->comp ) - 1 );
		return 0;
	}
	strcpy( pSCNL->comp,  s );
		
	if( (s = k_str() ) == NULL )  {
		LogWrite("e", "Parse SCNL Network Error\n" );
		return 0;
	}
	len = strlen( s );
	if( !len || ( len > ( (int)sizeof( pSCNL->net ) - 1 ) ) )  {
		LogWrite("e", "Parse SCNL Network Length Error Max = %d\n", sizeof( pSCNL->net ) - 1 );
		return 0;
	}
	strcpy( pSCNL->net,  s );
	
	if( (s = k_str() ) == NULL )  {
		LogWrite("e", "Parse SCNL Location Error\n" );
		return 0;
	}
	len = strlen( s );
	if( !len || ( len > ( (int)sizeof( pSCNL->loc ) - 1 ) ) )  {
		LogWrite("e", "Parse SCNL Location Length Error Max = %d\n", sizeof( pSCNL->loc ) - 1 );
		return 0;
	}
	strcpy( pSCNL->loc,  s );
	
	if( ( i = k_int() ) == 0 )  {
		LogWrite("e", "Parse SCNL Error No ADC Bits to use\n" );
		return 0;
	}
	else  {
		if( i < 8 || i > 24 )
			LogWrite("e", "Parse SCNL ADC Bits range error, using default of 16\n" );
		else
			pSCNL->bitsToUse = i;
	}
			
	if( useChannel == -1 )  {
		if( ( i = k_int() ) == 0 )  {
			LogWrite("e", "Parse SCNL No ADC Gain error\n" );
			return 0;
		}
		else  {
			if( ( i != 1 ) && ( i != 2 ) && ( i != 4 ) && ( i != 8 ) && ( i != 16 ) && ( i != 32 ) && ( i != 64 ) )
				LogWrite("e", "Parse SCNL Error ADC Gain not 1,2,4,8,16,32, or 64, using default of 1\n" );
			else	
				pSCNL->adcGain = i;
		}		
	}
			
	if( (s = k_str() ) == NULL )  {
		LogWrite("e", "Parse SCNL Filter Delay Error\n" );
		return 0;
	}
	pSCNL->filterDelay = atoi( s );		
		
	if( ( s = k_str() ) == NULL )  {
		LogWrite("e", "Parse SCNL No Invert Y/N Error\n" );
		return 0;
	}
	yn = toupper( s[0] );
	if( yn == 'Y' || yn == '1' )
		pSCNL->invertChannel = TRUE;
				
	if( ( s = k_str() ) == NULL )  {
		LogWrite("e", "Parse SCNL No SendToRing Y/N Error\n" );
		return 0;
	}
	
	yn = toupper( s[0] );
	if( yn == 'Y' || yn == '1' )
		pSCNL->sendToRing = TRUE;
				
	if( ( s = k_str() ) == NULL )  {
		LogWrite("e", "Parse SCNL No DC Offset Entry Error\n" );
		return 0;
	}
	else  {
		pSCNL->dcOffset = atoi( s );
	}

	if( ! ( pSCNL->dataBuff = (LONG *)malloc( ChanRate * sizeof( int ) ) ) )  {
		LogWrite( "e", "data malloc error\n");
		return 0;
	}
	
	pSCNL->chanNum = TotalChans + 1;
	
	if( useChannel == -1 )  {
		if( ++AdcChans > MAX_ADC_CHANNELS )  {
			LogWrite( "e", "Too many ADC channels defined in config file, max = %d\n", MAX_ADC_CHANNELS );
			return 0;
		}
		if( ( ChanRate > 250 ) && ( AdcChans > MAX_500SPS_CHANNELS ) )  {
			LogWrite( "e", "Too many channels defined in config file for 500SPS, max = %d\n", 
				MAX_500SPS_CHANNELS );
			return 0;
		}	
	}
	else
		++AddChans;
	
	if( TestForDupChan( pSCNL, TotalChans ) )  {
		LogWrite( "e", "Duplicate channel found in config file, SCNL=%s,%s,%s,%s\n", pSCNL->sta,
			pSCNL->comp, pSCNL->net, pSCNL->loc );
		return 0;
	}
	
	TotalChans = AdcChans + AddChans;	
	if( TotalChans > MAX_CHANNELS )  {
		LogWrite( "e", "Too many total channels defined in config file, max = %d\n", MAX_CHANNELS );
		return 0;
	}
				
	return TRUE;
}
	
int GetAddChanScnl()
{
	int ch;
	
	if( ( ch = k_int() ) == 0 )  {
		LogWrite("e", "Parse Additional Channel SCLN Channel Error\n" );
		return 0;
	}
	
	if( ch <= 0 || ch > AdcChans )  {
		LogWrite("e", "Parse Additional Channel SCLN Channel Error, Invalid Channel Number\n" );
		return 0;
	}
	
	/* Call the above code to complete the parsing */
	return GetChanScnl( ch - 1 );
}

int GetTimeRefType( char *str )
{
	strupr( str );
	
	TimeRefInfo *pInfo = timeRefInfo;
	while( 1 )  {
		if( !pInfo->name )
			break;
		if( !strcmp( pInfo->name, str ) )  {
			TimeRefType = pInfo->type;
			return 1;
		}			
		++pInfo;
	}	
	return 0;
}

int GetConfig( char *configfile )
{
	const int ncommand = NCOMMAND;
	char  init[NCOMMAND];	  /* Flags, one for each command */
	int	nmiss;				  /* Number of commands that were missed */
	int	nfiles, i, cherr = 0;
	char *pLastCom = 0;
		
	memset( ChanList, 0, sizeof( ChanList ) );
	AdcChans = AddChans = TotalChans = 0;
	
	// set non mandatory defaults
	ConsoleDisplay = 0;
	ControlCExit = 0;
	RefreshTime = 5;
	CheckStdin = 1;
	NoSendBadTime = 0;
	TcpMode = TcpPort = 0;
	ExitOnTimeout = TRUE;
	SystemTimeError = 0;
	StartTimeError = 0;
	PPSTimeError = 0;
	NoLockTimeError = DEF_NOT_LOCK_TIME * 60;			// default is 30 min.
	
	FilterSysTimeDiff = FilterGPSMessages = 0;
	OutKey = 0;
	MuxKey = 0;
								
	/* Set to zero one init flag for each required command */
	memset( init, 0, sizeof( int ) );
	
	/* Open the main configuration file */
	nfiles = k_open( configfile );
	if ( nfiles == 0 )  {
		LogWrite("", "Error opening configuration file <%s>\n", configfile );
		return -1;
	}

	/* Process all nested configuration files */
	while ( nfiles > 0 )  {			 	/* While there are config files open */
		while ( k_rd() )  {			  	/* Read next line from active file  */
			int  success;
			char *com;
			char *str;

			com = k_str();			 /* Get the first token from line */
			pLastCom = com;
			if ( !com ) continue;				 /* Ignore blank lines */
			if ( com[0] == '#' ) continue;	 /* Ignore comments */

			/* Open another configuration file */
			if ( com[0] == '@' )  {
				success = nfiles + 1;
				nfiles  = k_open( &com[1] );
				if ( nfiles != success )  {
					LogWrite("e", "Error opening command file <%s>.\n", &com[1] );
					return -1;
				}
				continue;
			}

			/* Read configuration parameters */
			else if ( k_its( "ModuleId" ) )  {
				if ( ( str = k_str() ) != NULL )  {
					if ( GetModId(str, &ModuleId) == -1 )  {
						LogWrite("e", "Invalid ModuleId <%s>. Exiting.\n", str );
						return -1;
					}
				}
				init[0] = 1;
			}
		 	else if( k_its( "TimeFileName" ) && ( str = k_str() ) )  {
				strcpy( TimeFileName, str );
				init[1] = 1;
		 	}
			else if ( k_its( "ChanRate" ) )  {
				ChanRate = k_int();
				init[2] = 1;
			}
			else if ( k_its( "TimeRefType" ) )  {
				str = k_str();
				if( GetTimeRefType( str ) )
					init[3] = 1;
				else  {
					LogWrite("e", "Invalid Time Reference Type <%s>. Exiting.\n", str );
					return -1;
				}
			}
			else if ( k_its( "OutRing" ) )  {
				if ( ( str = k_str() ) != NULL )  {
					if ( (OutKey = GetKey(str)) == -1 )  {
						LogWrite("e", "Invalid OutRing <%s>. Exiting.\n", str );
						return -1;
					}
				}
				init[4] = 1;
			}
			else if ( k_its( "HeartbeatInt" ) )  {
				HeartbeatInt = k_int();
				init[5] = 1;
			}
			else if ( k_its( "PortSpeed" ) )  {
				PortSpeed = k_int();
				init[6] = 1;
			}
			else if ( k_its( "UpdateSysClock" ) )  {
				UpdateSysClock = k_int();
				init[7] = 1;
			}
			else if ( k_its( "CommPortTcpHost" ) )  {
				str = k_str();
				strcpy( CommPortHostStr, str );
				init[8] = 1;
			}
		 	else if ( k_its( "NoDataTimeout" ))  {
				NoDataTimeout = k_int();
				init[9] = 1;
			}
		 	else if ( k_its( "HighToLowPPS" ))  {
				HighToLowPPS = k_int();
				init[10] = 1;
			}
		 	else if ( k_its( "NoPPSLedStatus" ))  {
				NoPPSLedStatus = k_int();
				init[11] = 1;
			}
			else if ( k_its( "TimeOffset" ))  {
				TimeOffset = k_int();
				init[12] = 1;
			}
			else if ( k_its( "LogMessages" ))  {
				LogMessages = k_int();
				init[13] = 1;
			}
			else if ( k_its( "AdcDataSize" ))  {
				AdcDataSize = k_int();
				init[14] = 1;
			}
			else if ( k_its( "TcpMode" ))  {
				TcpMode = k_int();
			}
			else if ( k_its( "TcpPort" ))  {
				TcpPort = k_int();
			}
			else if ( k_its( "ConsoleDisplay" ))  {
				ConsoleDisplay = k_int();
			}
			else if ( k_its( "ControlCExit" ))  {
				ControlCExit = k_int();
			}
			else if ( k_its( "ExitOnTimeout" ))  {
				ExitOnTimeout = k_int();
			}
			else if ( k_its( "RefreshTime" ))  {
				RefreshTime = k_int();
			}
			else if ( k_its( "CheckStdin" ))  {
				CheckStdin = k_int();
			}
			else if ( k_its( "NoSendBadTime" ))  {
				NoSendBadTime = k_int();
			}
			else if ( k_its( "Debug" ))  {
				Debug = k_int();
			}
			else if ( k_its( "SystemTimeError" ))  {
				SystemTimeError = k_int();
				if( SystemTimeError < 0 )
					SystemTimeError = 0;
			}
			else if ( k_its( "PPSTimeError" ))  {
				PPSTimeError = k_int();
				if( PPSTimeError < 0 )
					PPSTimeError = 0;
			}
			else if ( k_its( "NoLockTimeError" ))  {
				NoLockTimeError = k_int() * 60;
				if( NoLockTimeError < 0 )
					NoLockTimeError = DEF_NOT_LOCK_TIME * 60;
			}
			else if ( k_its( "FilterSysTimeDiff" ))  {
				FilterSysTimeDiff = k_int();
			}
			else if ( k_its( "FilterGPSMessages" ))  {
				FilterGPSMessages = k_int();
			}
			else if ( k_its( "StartTimeError" ))  {
				StartTimeError = k_int();
				if( StartTimeError < 0 )
					StartTimeError = 0;
			}
			else if ( k_its( "MuxDataRing" ) )  {
				if ( ( str = k_str() ) != NULL )  {
					if( str[0] == '0' )
						MuxKey = 0;
					else if( ( MuxKey = GetKey( str ) ) == -1 )  {
						LogWrite("e", "Invalid MuxDataRing <%s>. Exiting.\n", str );
						return -1;
					}
				}
			}
			else if ( k_its( "Nchan" ) )  {
				LogWrite("e", "Config file error, Nchan no longer used, please update file <%s>. Exiting.\n", configfile );
				return -1;
			}
			/* Get the channel list */
			else if ( k_its( "Chan" ) )  { 		// Get Scnl value for each real ADC channel
				if( !GetChanScnl( -1 ) )  {		// -1 = real channel
					cherr = TRUE;
					break;
				}
			}
			else if ( k_its( "AddChan" ) )  { 	// Get Scnl value for additional derived channel
				if( !GetAddChanScnl() )  {		// -1 = real channel
					cherr = TRUE;
					break;
				}
			}
			else if ( k_its( "Filter" ) )  { // Get Scnl value for each channel
				if( !GetChanFilter() )  {
					cherr = TRUE;
					break;
				}
			}
			/* An unknown parameter was encountered */
			else  {
				LogWrite("", "<%s> unknown parameter in <%s>\n", com, configfile );
				return -1;
			}
			/* See if there were any errors processing the command */
			if ( k_err() )  {
				LogWrite("", "Bad <%s> command in <%s>.\n", com, configfile );
				return -1;
			}
		}
		nfiles = k_close();
	}

	if( cherr )  {
		if( pLastCom )
			LogWrite("", "Bad Parse Channel at <%s> in <%s>.\n", pLastCom, configfile );
		return -1;	
	}
	
	/* After all files are closed, check flags for missed commands */
	nmiss = 0;
	for ( i = 0; i < ncommand; i++ )
		if ( !init[i] )
			nmiss++;
	if ( nmiss > 0 )  {
		logit("", "ERROR, no " );
		if ( !init[0]  ) logit("", "<ModuleId> " );
		if ( !init[1]  ) logit("", "<TimeFileName> " );
		if ( !init[2]  ) logit("", "<ChanRate> " );
		if ( !init[3]  ) logit("", "<TimeRefType> " );
		if ( !init[4]  ) logit("", "<OutRing> " );
		if ( !init[5]  ) logit("", "<HeartbeatInt> " );
		if ( !init[6]  ) logit("", "<PortSpeed> " );
		if ( !init[7]  ) logit("", "<UpdateSysClock> " );
		if ( !init[8]  ) logit("", "<CommPortTcpHost> " );
		if ( !init[9]  ) logit("", "<NoDataTimeout> " );
		if ( !init[10] ) logit("", "<HighToLowPPS> " );
		if ( !init[11] ) logit("", "<NoPPSLedStatus> " );
		if ( !init[12] ) logit("", "<TimeOffset> " );
		if ( !init[13] ) logit("", "<LogMessages> " );
		if ( !init[14] ) logit("", "<AdcDataSize> " );
		logit("", "command(s) in <%s>.\n", configfile );
		return -1;
	}
	if( TcpMode && TcpPort == 0 )  {
		logit("", "ERROR, no TCP/IP Port Number" );
		return -1;
	}
	
	// turn off system time test if TimeRefType = computer time
	if( TimeRefType == TIME_REF_PCTIME && SystemTimeError )
		SystemTimeError = 0;
	
	// turn off startup time test if TimeRefType = computer time
	if( TimeRefType == TIME_REF_PCTIME && StartTimeError )
		StartTimeError = 0;
	
	return 0;
}

void LogConfig( void )
{
	SCNL *pSCNL;
	double freq, sensorQ, hpFreq, hpQ;
	int i, poles, dspFilters = 0, dspInvFilters = 0;
	char invYN, sendYN;
		
	logit( "", "ModuleId:          %u\n", ModuleId );
	if( TcpMode )  {
		logit( "", "TCP/IP Host:       %s\n", CommPortHostStr );
		logit( "", "TCP/IP Port:       %d\n", TcpPort );
	}
	else  {
		logit( "", "Comm Port:         %s\n", CommPortHostStr );
		logit( "", "Port Speed:        %d\n", PortSpeed );
	}
	logit( "", "TcpMode:           %d\n", TcpMode );
	logit( "", "ChanRate:          %d sps\n", ChanRate );
	logit( "", "TimeFileName:      %s\n", TimeFileName );
	logit( "", "TimeRefType:       %d\n", TimeRefType );
	logit( "", "OutKey:            %d\n", OutKey );
	logit( "", "MuxKey:            %d\n", MuxKey );
	logit( "", "HeartbeatInt:      %d sec\n", HeartbeatInt );
	logit( "", "UpdateSysClock:    %d\n", UpdateSysClock );
	logit( "", "NoDataTimeout:     %d sec\n", NoDataTimeout );
	logit( "", "HighToLowPPS:      %d\n", HighToLowPPS );
	logit( "", "NoPPSLedStatus:    %d\n", NoPPSLedStatus );
	logit( "", "TimeOffset:        %d ms\n", TimeOffset );
	logit( "", "LogMessages:       %d\n", LogMessages );
	logit( "", "AdcDataSize:       %d bytes\n", AdcDataSize );
	logit( "", "ConsoleDisplay:    %d\n", ConsoleDisplay );
	logit( "", "ControlCExit:      %d\n", ControlCExit );
	logit( "", "RefreshTime:       %d sec\n", RefreshTime );
	logit( "", "CheckStdin:        %d\n", CheckStdin );
	logit( "", "NoSendBadTime:     %d\n", NoSendBadTime );
	logit( "", "ExitOnTimeout:     %d\n", ExitOnTimeout );
	logit( "", "SystemTimeError:   %d sec\n", SystemTimeError );
	logit( "", "PPSTimeError:      %d ms\n", PPSTimeError );
	logit( "", "NoLockTimeError:   %d min\n", NoLockTimeError / 60 );
	logit( "", "FilterSysTimeDiff: %d\n", FilterSysTimeDiff );
	logit( "", "FilterGPSMessages: %d\n", FilterGPSMessages );
	logit( "", "StartTimeError:    %d sec\n", StartTimeError );
	
	
	logit( "", "Debug:             %d\n", Debug );
	logit( "", "ADC Channels:      %d\n", AdcChans );
	if( AddChans )
		logit( "", "Total Channels:  %d\n", TotalChans );

	/* Log the channel list */
	logit( "", "\nADC Channels ----\n" );
	logit( "", "Ch#  Sta   Comp Net Loc Bits Gain FltrDly Inv Send Offset\n" );
	logit( "", "--   ----- ---- --- --- ---- ---- ------- --- ---- ------\n" );
	for( i = 0; i < AdcChans; i++ )  {
		pSCNL = &ChanList[i];
		if( pSCNL->invertChannel )
			invYN = 'Y';
		else
			invYN = 'N';
		if( pSCNL->sendToRing )
			sendYN = 'Y';
		else
			sendYN = 'N';
		logit( "", "%2d   %-5s %-3s  %-2s  %-2s   %-2d   %-2d    %-2d     %c   %c    %d\n", 
			pSCNL->chanNum, pSCNL->sta, pSCNL->comp, pSCNL->net, pSCNL->loc, pSCNL->bitsToUse, 
			pSCNL->adcGain,	pSCNL->filterDelay, invYN, sendYN, pSCNL->dcOffset );
		
		if( pSCNL->lpFilter || pSCNL->hpFilter )
			dspFilters = TRUE;
		if( pSCNL->invFilter )
			dspInvFilters = TRUE;
	}
	
	if( AddChans )  {
		/* Log the addtional channel list */
		logit( "", "\nAdditional Channels ----\n" );
		logit( "", "Ch# AdcCh Sta   Comp Net Loc Bits FltrDly Inv Send Offset\n" );
		logit( "", "--  ----- ----- ---- --- --- ---- ------- --- ---- ------\n" );
		for ( i = AdcChans; i < TotalChans; i++ )  {
			pSCNL = &ChanList[i];
			if( pSCNL->invertChannel )
				invYN = 'Y';
			else
				invYN = 'N';
			if( pSCNL->sendToRing )
				sendYN = 'Y';
			else
				sendYN = 'N';
			logit( "", "%2d   %2d   %-5s %-3s  %-2s  %-2s   %-2d   %-2d     %c   %c    %d\n", 
				pSCNL->chanNum, pSCNL->useChannel+1, pSCNL->sta, 
				pSCNL->comp, pSCNL->net, pSCNL->loc, pSCNL->bitsToUse, pSCNL->filterDelay, 
					invYN, sendYN, pSCNL->dcOffset );
			
			if( pSCNL->lpFilter || pSCNL->hpFilter )
				dspFilters = TRUE;
			if( pSCNL->invFilter )
				dspInvFilters = TRUE;
		}
	}
		
	if( dspFilters )  {
		logit( "", "\nLow-Pass and High-Pass Filters\n" );
		logit( "", "Ch# Type  Freq  Poles\n" );
		logit( "", "--- ----  ----  -----\n" );
		for ( i = 0; i < TotalChans; i++ )  {
			pSCNL = &ChanList[i];
			if( pSCNL->lpFilter )  {
				pSCNL->lpFilter->GetParams( &freq, &poles );
				logit( "", "%2d   LP   %-4.1f    %d\n", i+1, freq, poles ); 
			}
			if( pSCNL->hpFilter )  {
				pSCNL->hpFilter->GetParams( &freq, &poles );
				logit( "", "%2d   HP   %-4.1f    %d\n", i+1, freq, poles ); 
			}
		}
	}
	
	if( dspInvFilters )  {
		logit( "", "\nInverse Filters\n" );
		logit( "", "Ch# SensorFreq SensorQ HP_Freq HP_Q\n" );
		logit( "", "--- ---------- ------- ------- ------\n" );
		for ( i = 0; i < TotalChans; i++ )  {
			pSCNL = &ChanList[i];
			if( pSCNL->invFilter )  {
				pSCNL->invFilter->GetParams( &freq, &sensorQ, &hpFreq, &hpQ );
				logit( "", "%2d     %-3.3f    %-3.3f   %-3.3f  %-3.3f\n", 
					i+1, freq, sensorQ, hpFreq, hpQ ); 
			}
		}
	}
	
	logit("", "\n-------------------------------------------------------\n");
}

/* Returns true if duplicate channel found in channel list */
int TestForDupChan( SCNL *pNewSCNL, int channels )
{
	int i;
	SCNL *pSCNL;
	
	if( !channels )
		return 0;
	--channels;
	for( i = 0; i != channels; i++ )  {
		pSCNL = &ChanList[i];
		if( !strcmp( pSCNL->sta, pNewSCNL->sta ) && !strcmp( pSCNL->comp, pNewSCNL->comp ) && 
				!strcmp( pSCNL->net, pNewSCNL->net ) && !strcmp( pSCNL->loc, pNewSCNL->loc ) )  {
			return 1;
		}
	}
	return 0;
}
