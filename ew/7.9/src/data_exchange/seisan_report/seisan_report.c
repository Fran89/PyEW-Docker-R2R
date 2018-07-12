/*
 * seisan_report.c
 *
 * Watches for TYPE_HYP2000ARC type messages saying that the sausage has located an event.
 * Extracts the parameters needed to write a seisan s-file but waits long enough that
 * trig2disk will have written a wavefile.  This allows the wavefile  name to be included
 * in the s-file when it is finally written out.
 *
 * Also watches for TYPE_MAGNITUDE messages from localmag.  Parameters from these will be
 * added to the appropriate s-file - which should not yet have been written at that point.
 *
 * If requested can write a TYPE_EVENT_ALARM message to ring for alarm modules to pick up.
 * This only happens upon receipt of a TYPE_MAGNITUDE message and so if locmag isn't
 * running no alarms will be sent.
 *
 * Originally based on menlo_report.c.
 * Written by David Scott (BGS)
 * Magnitude and wavefile name added Richard Luckett (BGS) 2007/6
 *
 */

/* VERSION number introduced by Paul F EW 7.7 Sept 4, 2013 */

/* 0.0.10 2014.02.26 - put in a LOCALMAG amplitude multiplier as a #define in nordic.c for amps from localmag */
/* 0.0.11 2015.06.13 - more logging for debugging purposes */
/* 0.0.12 2016.05.31 - Added cast to 'sprintf' param (for 64 bit); minor mods to squelch warnings. */

#define VERSION "0.0.12 2016.05.31"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WINNT
#include <io.h>
#else
#include <dirent.h>
#endif
#include <time.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <rw_mag.h>
#include <read_arc.h>
#include <chron3.h>
#include <fleng.h>

#include <nordic.h>

#define MAX_EVENTS             5       /* Number of events for which details are remembered */
#define FILE_NAME_LEN        150       /* Number of chars in name of sfiles */
#define SFILE_LINE_LEN        80       /* Line length for seisan S file */
#define MAX_PHASE            MAX_NOR_PHASES       /* Maximum number of phases for an event */
#define QID_LEN               11       /* Length of qid in ARC message */
#define AGENCY_LEN             4       /* Length of agency in sfile */
#define MESSAGE_LEN          150       /* Length of messages put on ring */
#define HUGE_TIME      4294967295      /* Time bigger than any possible */

/* Structures to hold the details from an event
 **********************************************/
struct sta_mag
{
	char sta[TRACE_STA_LEN];
	char chan[TRACE_CHAN_LEN];
	double amp;
	int used; /* Flag raised when channel has been written to sfile */
};

struct event_details
{
	char qid[QID_LEN];
	time_t detect_time;
	//struct Hsum hyp;
	double local_mag;
	//struct Hpck pha[MAX_PHASE];
	//int nphase;
	//struct sta_mag stamag[MAX_PHASE];
	//int nstamag;
	/* New fields */
	HypoArc arc; // RSL, 2013.10
	MAG_INFO mag; // RSL, 2013.10
};

/* Functions in this source file
 *******************************/
void seisan_report_hinvarc( char *msg );
int seisan_report_magnitude( char *msg, int msgLen );
void seisan_report_sfile( struct event_details* event );
int seisan_report_wfile( struct event_details* ep, char** frefs ); // RSL 2013.10
void seisan_report_alarm( struct event_details* event );
void seisan_report_config( char *configfile );
int seisan_report_lookup( void );
void seisan_report_status( unsigned char type, short ierr, char *note );

/* Things to read from configuration file
 ****************************************/
#define NUM_COMMANDS 11                   /* Number of required commands in config file */
static char RingName[MAX_RING_STR]; /* Name of transport ring for i/o             */
static char MyModName[MAX_MOD_STR]; /* Speak as this module name/id               */
static int LogSwitch; /* 0 if no logfile should be written          */
static int Debug; /* If not 0 write diagnostic messages to log  */
static time_t HeartBeatInterval; /* Seconds between heartbeats                 */
static long WriteDelay; /* Seconds to wait between detecting an event and saving it */
static char SfileDir[MAX_DIR_LEN]; /* Directory to write sfiles to               */
static char WavDir[MAX_DIR_LEN]; /* Directory where wavefiles are              */
static char Agency[AGENCY_LEN]; /* 3 letter code for agency                   */
static int AlarmMsg; /* If 0 don't put alarm message on ring       */
static int UseMSEEDArchive = 0; /* RSL 2011.03: 0 do not use; 1 BUD; 2 SCP*/

#define   MAXLOGO   2
MSG_LOGO GetLogo[MAXLOGO]; /* Array for requesting module,type,instid */
short nLogo;

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long RingKey; /* Key of transport ring for i/o      */
static unsigned char InstId; /* Local installation id              */
static unsigned char MyModId; /* Module Id for this program         */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypeHyp2000Arc;
static unsigned char TypeMagnitude;
static unsigned char TypeAlarm; /* Type added by RL - read by event_alarm */

/* Error messages used by seisan_report
 *************************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
static char Text[150]; /* string for log/error messages          */

static SHM_INFO Region; /* shared memory region to use for i/o    */
pid_t MyPid; /* Hold our process ID to be sent with heartbeats */

struct event_details event[MAX_EVENTS]; /* array of remembered events */
int event_index; /* current position in array  */

/* Main program starts here
 **************************/
int main( int argc, char **argv )
{
	static char eqmsg[MAX_BYTES_PER_EQ]; /* array to hold event message    */
	time_t timeNow; /* current time                   */
	time_t timeLastBeat; /* time last heartbeat was sent   */
	long recsize; /* size of retrieved message      */
	MSG_LOGO reclogo; /* logo of retrieved message      */
	int res, i;

	/* Check command line arguments
	 ******************************/
	if( argc != 2 )
	{
		fprintf( stderr, "Usage: seisan_report <configfile>\n" );
		fprintf( stderr, "Version: seisan_report %s\n", VERSION );
		exit( EW_FAILURE );
	}

	/* Initialize name of log-file & open it
	 ***************************************/
	logit_init( argv[1], 0, 256, 1 );

	/* Read the configuration file(s)
	 ********************************/
	seisan_report_config( argv[1] );

	/* Set logit to LogSwitch read from configfile.
	 *********************************************/
	logit_init( argv[1], 0, 256, LogSwitch );
	logit( "t", "seisan_report: Read command file <%s>\n", argv[1] );
	logit( "t", "seisan_report: Version %s\n", VERSION );

	/* Look up important info from earthworm.h tables
	 ************************************************/
	if( seisan_report_lookup( ) != EW_SUCCESS )
	{
		logit( "e", "seisan_report: Call to seisan_report_lookup failed \n" );
		exit( EW_FAILURE );
	}

	/* Get our process ID
	 ********************/
	if( ( MyPid = getpid( ) ) == -1 )
	{
		logit( "e", "seisan_report: Call to getpid failed. Exiting.\n" );
		exit( EW_FAILURE );
	}

	/* Initialise array of events
	 ***************************/
	for( i = 0; i < MAX_EVENTS; i++ )
	{
		event[i].detect_time = 0;//( time_t ) HUGE_TIME;
		strcpy( event[i].qid, "" );
	}

	/* Attach to Input/Output shared memory ring
	 *******************************************/
	tport_attach( &Region, RingKey );
	logit( "", "seisan_report: Attached to public memory region %s: %d\n", RingName, RingKey );

	/* Force a heartbeat to be issued in first pass thru main loop
	 *************************************************************/
	timeLastBeat = time( &timeNow ) - HeartBeatInterval - 1;

	/* Flush the incomming transport ring
	 *************************************/
	while( tport_getmsg( &Region, GetLogo, nLogo, &reclogo, &recsize, eqmsg, sizeof (eqmsg ) - 1 ) != GET_NONE );


	/*----------------- setup done; start main loop ----------------------*/

	while( tport_getflag( &Region ) != TERMINATE && tport_getflag( &Region ) != MyPid )
	{
		/* Send seisan_report's heartbeat */
		if( time( &timeNow ) - timeLastBeat >= HeartBeatInterval )
		{
			timeLastBeat = timeNow;
			seisan_report_status( TypeHeartBeat, 0, "" );

			/* Ceck to see if any events are old enough to write */
			for( i = 0; i < MAX_EVENTS; i++ )
				if( event[i].detect_time != 0 && 
						( timeNow - event[i].detect_time ) > WriteDelay )
				{
					seisan_report_sfile( &event[i] );
					event[i].detect_time = 0;
					strcpy( event[i].qid, "" );
				}
		}

		/* Process new hypoinverse archive and magnitude msgs */
		do
		{
			/* Get the next message from shared memory */
			res = tport_getmsg( &Region, GetLogo, nLogo, &reclogo, &recsize, eqmsg, sizeof (eqmsg ) - 1 );

			/* Check return code; report errors if necessary */
			if( res != GET_OK )
			{
				if( res == GET_NONE )
				{
					break;
				}
				else if( res == GET_TOOBIG )
				{
					sprintf( Text, "Retrieved msg[%ld] (i%u m%u t%u) too big for eqmsg[%ld]",
							recsize, reclogo.instid, reclogo.mod, reclogo.type, (long)(sizeof (eqmsg ) - 1) );
					seisan_report_status( TypeError, ERR_TOOBIG, Text );
					continue;
				}
				else if( res == GET_MISS )
				{
					sprintf( Text, "Missed msg(s)  i%u m%u t%u  %s.", reclogo.instid, reclogo.mod, reclogo.type, RingName );
					seisan_report_status( TypeError, ERR_MISSMSG, Text );
				}
				else if( res == GET_NOTRACK )
				{
					sprintf( Text, "Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
							reclogo.instid, reclogo.mod, reclogo.type );
					seisan_report_status( TypeError, ERR_NOTRACK, Text );
				}
			}

			/* Process new message (res==GET_OK,GET_MISS,GET_NOTRACK) */
			eqmsg[recsize] = '\0';

			if( reclogo.type == TypeHyp2000Arc )
			{
				if( Debug )
					logit( "t", "seisan_report: received TypeHyp2000Arc message\n" );
				seisan_report_hinvarc( eqmsg );
			}
			else if( reclogo.type == TypeMagnitude )
			{
				if( Debug )
					logit( "t", "seisan_report: received TypeMagnitude message\n" );
				i = seisan_report_magnitude( eqmsg, recsize + 1 );

				if( AlarmMsg && i >= 0 )
					seisan_report_alarm( &event[i] );
			}

		}
		while( res != GET_NONE ); /* End of message-processing-loop */

		sleep_ew( 1000 ); /* No more messages; wait for new ones to arrive */
	}

	/*-----------------------------end of main loop-------------------------------*/

	/* Termination has been requested
	 ********************************/
	tport_detach( &Region );
	logit( "t", "seisan_report: Termination requested; exiting!\n" );
	exit( EW_SUCCESS );
}

/**************************************************************************
 *	seisan_report_hinvarc
 *
 *	Input:	*arcmsg   - pointer to message taken off ring by main loop.
 *
 *	Process a Hypoinverse archive message and populate structure to
 *      hold the parameters in memory.
 *
 **************************************************************************/
void seisan_report_hinvarc( char *arcmsg )
{
	/* New version, converts message to arc structure
	 * All information is available directly from that structure */
	/* Parse arc message */
	event[event_index].arc.sum.qid = -1;
	event[event_index].arc.num_phases = 0;
	if( parse_arc( arcmsg, &event[event_index].arc ) != 0 ||
			event[event_index].arc.sum.qid == -1 )
	{
		logit( "e", "seisan_report: Error parsing event\n" );
		if( event[event_index].arc.num_phases > 0 ) free_phases( &event[event_index].arc );
		return;
	}
	logit( "t", "parsed event %ld\n", event[event_index].qid );
	
	/* Put QID in structure separately */
	sprintf( event[event_index].qid, "%ld", event[event_index].arc.sum.qid );

	/* Add current time to structure to show when event was detected */
	event[event_index].detect_time = time( NULL );

	/* Clear magnitude fields until magtype message comes along */
	event[event_index].local_mag = 0;
	//event[event_index].nstamag = 0;

	if( Debug )
		logit( "t", "seisan_report: Read %d phases for event %ld\n", 
				event[event_index].arc.num_phases, event[event_index].arc.sum.qid );

	/* Add to array of remembered events */
	if( ++event_index == MAX_EVENTS )
		event_index = 0;

	return;
}

/**************************************************************************
 *	seisan_report_magnitude
 *
 *	Input:	*arcmsg   - pointer to message taken off ring by main loop.
 *               msgLen   - length of the message
 *
 *      Return:  i - index on event array of event this magnitude belongs to
 *                   returns -1 on error
 *
 *	Process a magnitude message and add parameters to event structure.
 *
 **************************************************************************/
int seisan_report_magnitude( char *msg, int msgLen )
{
	MAG_INFO LocMag;
	//MAG_CHAN_INFO *pStaMag;
	int i;//, j;
	

	/* Allocate array of structures for channel info */
	LocMag.pMagAux = calloc( MAX_PHASE, sizeof (MAG_CHAN_INFO ) );
	LocMag.size_aux = MAX_PHASE * sizeof (MAG_CHAN_INFO );

	/* Read params from message using functions from rw_mag.c */
	if( rd_mag( msg, msgLen, &LocMag ) )
	{
		logit( "et", "seisan_report: problem parsing TYPE_MAGNITUDE message\n" );
		free( LocMag.pMagAux );
		return -1;
	}
	if( Debug )
		logit( "t", "seisan_report: magnitude found: %3.1fL%s n=%d qid=%s\n", LocMag.mag, Agency, LocMag.nchannels, LocMag.qid );

	/* Find which recent event this magnitude belongs to */
	for( i = 0; i < MAX_EVENTS; i++ )
		if( strcmp( event[i].qid, LocMag.qid ) == 0 )
			break;
	if( i == MAX_EVENTS )
	{
		logit( "et", "seisan_report: no event in memory with qid=%s\n", LocMag.qid );
		free( LocMag.pMagAux );
		return -1;
	}

	/* Add magnitude to event structure */
	event[i].local_mag = LocMag.mag;
	
	/* Copy magnitude structure to event structure */
	if( event[i].mag.pMagAux != NULL )
		free( event[i].mag.pMagAux );
	event[i].mag.pMagAux = calloc( MAX_PHASE, sizeof (MAG_CHAN_INFO ) );
	event[i].mag.size_aux = MAX_PHASE * sizeof (MAG_CHAN_INFO );
	rd_mag( msg, msgLen, &event[i].mag );

        if (LocMag.pMagAux != NULL)
		free( LocMag.pMagAux );
	return i;
}


#define WAV_FILE_COMPARE_STRING_SIZE 16

/**************************************************************************
 *	seisan_report_sfile
 *
 *	Input:	*ep   - pointer to event structure with parameters to write.
 *
 *     Write an s-file to disk.
 *     This is done WriteDelay seconds after arc message is received to
 *     allow wavefile to be written so that its name can be put in sfile.
 *
 **************************************************************************/
void seisan_report_sfile( struct event_details* ep )
{
	FILE *fp_s;
	struct Greg ot;//, at; /* Greg structure for times is in read_arc.h */
	char sfile_name[FILE_NAME_LEN];

	char* wfiles[MAX_PHASE];
	int nwfiles;

	int i;//, j;
	
	Nordic *ptrevt;


	if( Debug )
		logit( "t", "seisan_report: Writing sfile for event %s\n", ep->qid );
	
	/* Generate nordic event */
	if( ep->local_mag != 0 )
	{
		if( Debug )
			logit( "o", "seisan_report: Converting hypocenter and local magnitude to seisan\n" );
		if( arc2nor( &ptrevt, &( ep->arc ), &( ep->mag ), Agency ) < 0 )
		{
			logit( "e", "seisan_report: Error converting event %s to nordic event\n", ep->qid );
			return;
		}
	}
	else
	{
		if( Debug )
			logit( "o", "seisan_report: Converting hypocenter only to seisan\n" );
		if( arc2nor( &ptrevt, &( ep->arc ), NULL, Agency ) < 0 )
		{
			logit( "e", "seisan_report: Error converting event %s to nordic event\n", ep->qid );
			return;
		}
	}
	
	/* Compute file references */
	nwfiles = seisan_report_wfile( ep, wfiles );
	
	/* Add waveform references to seisan event */
	for( i = 0; i < nwfiles; i++ )
	{
		NorAddWaveRef( ptrevt, wfiles[i] );
		
		/* Memory of this waveform refernce */
		free( wfiles[i] );
	}


	/* Put origin time into structure - epoch is 1600 not 1900 */
	ot = *datime( ep->arc.sum.ot, &ot );
	
	/* Open S-file */
	sprintf( sfile_name, "%s/%02d-%02d%02d-%02dL.S%4d%02d", SfileDir, ot.day, ot.hour, ot.minute, ( int ) ot.second, ot.year, ot.month );
	if( ( fp_s = fopen( sfile_name, "w" ) ) == ( FILE * ) NULL )
	{
		logit( "et", "seisan_report: error opening file <%s>\n", sfile_name );
		freeNordic( ptrevt );
		return;
	}
	
	/* Write seisan file */
	fwriteNor( fp_s, ptrevt );
	
	/* Release nordic event */
	freeNordic( ptrevt );

	fclose( fp_s );
	if( Debug )
		logit( "t", "seisan_report: sfile written <%s>\n", sfile_name );

	return;
}

/**************************************************************************
 *	seisan_report_wfile
 *
 *	Input:	*ep   - pointer to event structure with parameters to write.
 *			**fname - char array of file references. Memory will be allocated
 *                    for the references but must be freed elsewhere
 *          *nrefs - number of generated file references
 *  Output: -1 if file was not found or the number of file references.
 *
 *     Tries to find a waveform file or prepares reference to mseed arch.
 *
 **************************************************************************/
int seisan_report_wfile( struct event_details* ep, char** frefs )
{
	//FILE *fp_s;
#ifdef _WINNT
	WIN32_FIND_DATA findData; /* Structure for directory entries */
	HANDLE fileHandle; /* Handle of first file for FindNextFile */
	char findFile[MAX_PATH]; /* Path/filename pattern to look for */
#else
	DIR *pDir;
	struct dirent *pDirEnt;
#endif
	double timeWav = 0; /* Time in seconds since 1600 like read_arc times */
	int i;
	char wavefileRoot1[FILE_NAME_LEN];
	char wavefileRoot2[FILE_NAME_LEN];
	char wavefileName[FILE_NAME_LEN];
	int refDuration;
	struct Greg rt; //Structure for reference time
	struct Greg wt;
	int nrefs;

	/* Reset counter of number of references */
	nrefs = 0;

	/* RSL 2011.03: Skip this part if using reference to mseed archives */
	if( UseMSEEDArchive == 0 )
	{
		//timeWav = HUGE_TIME;
		for( i = 0; i < ep->arc.num_phases; i++ )
			if( ep->arc.phases[i]->Pat < timeWav || i == 0 )
				timeWav = ep->arc.phases[i]->Pat;
		timeWav -= 30;
		wt = *datime( timeWav, &wt );
		sprintf( wavefileRoot1, "%4d-%02d-%02d-%02d%02d-%02dS", wt.year, 
				wt.month, wt.day, wt.hour, wt.minute, ( int ) wt.second );

		/* Sometimes (too few phases?) name based on last phase less 30 seconds */
		timeWav = 0;
		for( i = 0; i < ep->arc.num_phases; i++ )
			if( ep->arc.phases[i]->Pat > timeWav && 
					( ep->arc.phases[i]->Plabel == 'P' || ep->arc.phases[i]->Ponset == 'P' ) )
			{
				timeWav = ep->arc.phases[i]->Pat;
			}
			else
			{
				timeWav = ep->arc.phases[i]->Sat;
			}
		timeWav -= 30;
		wt = *datime( timeWav, &wt );
		sprintf( wavefileRoot2, "%4d-%02d-%02d-%02d%02d-%02dS", wt.year, 
				wt.month, wt.day, wt.hour, wt.minute, ( int ) wt.second );
		strcpy( wavefileName, "" );

		/* Need to read directory to find complete filename - no other way to know number of chans */
#ifdef _WINNT

		/* Windows looks for pattern so could do this differently but make it similar to UNIX */
		sprintf( findFile, "%s\\*", WavDir );
		if( ( fileHandle = FindFirstFile( findFile, &findData ) ) == INVALID_HANDLE_VALUE )
		{
			if( GetLastError( ) == ERROR_FILE_NOT_FOUND )
			{
				logit( "et", "seisan_report: no files found in dir <%s>\n", WavDir );
			}
			else
			{
				logit( "et", "seisan_report: error opening dir <%s>\n", WavDir );
			}
		}
		else
		{
			if( Debug )
				logit( "et", "seisan_report: starting search for WAV files in %s\n", findFile );
			do
			{

				if( strncmp( wavefileRoot1, findData.cFileName, WAV_FILE_COMPARE_STRING_SIZE ) == 0 )
				{
					strcpy( wavefileName, findData.cFileName );
					break;
				}
				if( strncmp( wavefileRoot2, findData.cFileName, WAV_FILE_COMPARE_STRING_SIZE ) == 0 )
				{
					strcpy( wavefileName, findData.cFileName );
					break;
				}
			}
			while( FindNextFile( fileHandle, &findData ) );

			FindClose( fileHandle );
		}

#else

		if( ( pDir = opendir( WavDir ) ) == NULL )
		{
			logit( "et", "seisan_report: error opening dir <%s>\n", WavDir );
			return -1;
		}
		while( ( pDirEnt = readdir( pDir ) ) != NULL )
		{
			if( strncmp( wavefileRoot1, pDirEnt->d_name, WAV_FILE_COMPARE_STRING_SIZE ) == 0 )
			{
				strcpy( wavefileName, pDirEnt->d_name );
				break;
			}
			if( strncmp( wavefileRoot2, pDirEnt->d_name, WAV_FILE_COMPARE_STRING_SIZE ) == 0 )
			{
				strcpy( wavefileName, pDirEnt->d_name );
				break;
			}
		}
		closedir( pDir );

#endif

		if( strcmp( wavefileName, "" ) == 0 )
		{
			logit( "et", "seisan_report: cant find wavefile starting %s or %s\n", wavefileRoot1, wavefileRoot2 );
		}
		else
		{
			if( Debug )
				logit( "", "seisan_report: wavefile found <%s>\n", wavefileName );

			frefs[nrefs] = ( char* ) malloc( sizeof ( char ) * 81 );
			if( frefs[nrefs] == NULL )
			{
				logit( "e", "seisan_report: Unable to allocate memory for station reference in seisan file\n" );
				return -1;
			}
			snprintf( frefs[nrefs], 80, "%s", wavefileName );
			( nrefs )++;
		}
	}
	else
	{
		/* RSL 2011.03 Using mseed archive references... Note that these are not verified in any way */
		double refStart = 0;//HUGE_TIME; /*Set this to the earliest start of a phase*/
		double latestEnd = 0; /*Set this to the latest end of all the phases*/
		for( i = 0; i < ep->arc.num_phases; i++ )
		{
			if( ep->arc.phases[i]->Pat < refStart || i == 0 )
			{
				refStart = ep->arc.phases[i]->Pat;
			}
			if( ( ep->arc.phases[i]->Pat + ep->arc.phases[i]->codalen ) > latestEnd )
			{
				latestEnd = ep->arc.phases[i]->Pat + ep->arc.phases[i]->codalen;
			}
			//printf("DATA: phaseStart: %f codaLen: %d refStart: %f latestEnd: %f\n", ep->pha[i].Pat, ep->pha[i].codalen, refStart, latestEnd);
		}
		refStart -= 30; //30 seconds margin of the start
		refDuration = ( int ) ( latestEnd + 15 - refStart ); //15 seconds margin to coda length
		rt = *datime( refStart, &rt );

		for( i = 0; i < ep->arc.num_phases && i < MAX_PHASE; i++ )
		{
			/* Produce reference line for each station*/
			frefs[nrefs] = ( char* ) malloc( sizeof ( char ) * 81 );
			if( frefs[nrefs] == NULL )
			{
				logit( "e", "seisan_report: Unable to allocate memory for station reference in seisan file\n" );
				return -1;
			}
			snprintf( frefs[nrefs], 80, "%s %-5s %3s %2s %2s %4d %2d%2d %2d%2d %2d %4d",
					( UseMSEEDArchive == 1 ) ? "BUD" : "SCP",
					ep->arc.phases[i]->site, ep->arc.phases[i]->comp, ep->arc.phases[i]->net,
					( strcmp( ep->arc.phases[i]->loc, "--" ) == 0 ) ? "  " : ep->arc.phases[i]->loc,
					rt.year, rt.month, rt.day, rt.hour, rt.minute, ( int ) rt.second,
					( int ) ( refDuration + 0.5 ) );
			( nrefs )++;
		}

	}
	return nrefs;
}

/**************************************************************************
 *	seisan_report_alarm
 *
 *	Input:	*ep   - pointer to event structure with parameters to write.
 *
 *     Put a TYPE_EVENT_ALARM message onto ring for other modules to act on.
 *     Does this for any event with a magnitude.
 *
 **************************************************************************/
void seisan_report_alarm( struct event_details* ep )
{
	struct Greg ot; /* Greg structure for times is in read_arc.h */
	char *grname[36]; /* Flinn-Engdahl region name */
	char alarmText[150];

	/* Get Flinn-Engdahl geographical region name */
	FlEngLookup( ep->arc.sum.lat, ep->arc.sum.lon, grname, NULL );

	/* Put origin time into structure - epoch is 1600 not 1900 */
	ot = *datime( ep->arc.sum.ot, &ot );

	/* Write message */
	sprintf( alarmText, "%-36s %3.1f %+6.2f %+6.2f %d/%02d/%02d %02d:%02d %3d %3.1f", *grname, ep->local_mag,
			ep->arc.sum.lat, ep->arc.sum.lon, ot.year, ot.month, ot.day,
			ot.hour, ot.minute, ep->arc.num_phases, ep->arc.sum.rms );

	seisan_report_status( TypeAlarm, 0, alarmText );

	if( Debug )
		logit( "t", "seisan_report: alarm written to ring:\n\t<%s>\n", alarmText );

	return;
}

/**************************************************************************
 *	seisan_report_config
 *
 *	Input:	*configfile - name of main command file to read.
 *
 *	Processes command file(s) using kom.c functions.
 *
 **************************************************************************/
void seisan_report_config( char *configfile )
{
	char init[NUM_COMMANDS]; /* init flags, one byte for each required command */
	int nmiss; /* number of required commands that were missed   */
	char *com;
	char *str;
	int nfiles;
	int success;
	int i;

	/* Set to zero one init flag for each required command
	 *****************************************************/
	for( i = 0; i < NUM_COMMANDS; i++ )
		init[i] = 0;
	nLogo = 0;

	/* Open the main configuration file
	 **********************************/
	nfiles = k_open( configfile );
	if( nfiles == 0 )
	{
		logit( "e", "seisan_report: Error opening command file <%s>; exiting!\n", configfile );
		exit( -1 );
	}

	/* Process all command files
	 ***************************/
	while( nfiles > 0 ) /* While there are command files open */
	{
		while( k_rd( ) ) /* Read next line from active file  */
		{
			com = k_str( ); /* Get the first token from line */

			/* Ignore blank lines & comments
			 *******************************/
			if( !com ) continue;
			if( com[0] == '#' ) continue;

			/* Open a nested configuration file
			 **********************************/
			if( com[0] == '@' )
			{
				success = nfiles + 1;
				nfiles = k_open( &com[1] );
				if( nfiles != success )
				{
					logit( "e",
							"seisan_report: Error opening command file <%s>; exiting!\n",
							&com[1] );
					exit( -1 );
				}
				continue;
			} /* Process anything else as a command
                 ************************************/

				/*0*/ else if( k_its( "MyModuleId" ) )
			{
				str = k_str( );
				if( str ) strcpy( MyModName, str );
				init[0] = 1;
			}
				/*1*/ else if( k_its( "RingName" ) )
			{
				str = k_str( );
				if( str ) strcpy( RingName, str );
				init[1] = 1;
			}
				/*2*/ else if( k_its( "HeartBeatInterval" ) )
			{
				HeartBeatInterval = ( time_t ) k_long( );
				init[2] = 1;
			}
				/*3*/ else if( k_its( "LogFile" ) )
			{
				LogSwitch = k_int( );
				init[3] = 1;
			}
				/*4*/ else if( k_its( "Debug" ) )
			{
				Debug = k_int( );
				init[4] = 1;
			}
				/*5*/ else if( k_its( "WriteDelay" ) )
			{
				WriteDelay = k_long( );
				init[5] = 1;
			}/* Enter installation & module to get event messages from
             ********************************************************/
				/*6*/ else if( k_its( "GetEventsFrom" ) )
			{
				if( nLogo + 1 >= MAXLOGO )
				{
					logit( "e",
							"seisan_report: Too many <GetEventsFrom> commands in <%s>",
							configfile );
					logit( "e", "; max=%d; exiting!\n", ( int ) MAXLOGO / 2 );
					exit( -1 );
				}
				if( ( str = k_str( ) ) != NULL )
				{
					if( GetInst( str, &GetLogo[nLogo].instid ) != 0 )
					{
						logit( "e",
								"seisan_report: Invalid installation name <%s>", str );
						logit( "e", " in <GetEventsFrom> cmd; exiting!\n" );
						exit( -1 );
					}
					GetLogo[nLogo + 1].instid = GetLogo[nLogo].instid;
				}
				if( ( str = k_str( ) ) != NULL )
				{
					if( GetModId( str, &GetLogo[nLogo].mod ) != 0 )
					{
						logit( "e",
								"seisan_report: Invalid module name <%s>", str );
						logit( "e", " in <GetEventsFrom> cmd; exiting!\n" );
						exit( -1 );
					}
					GetLogo[nLogo + 1].mod = GetLogo[nLogo].mod;
				}
				if( GetType( "TYPE_HYP2000ARC", &GetLogo[nLogo].type ) != 0 )
				{
					logit( "e",
							"seisan_report: Invalid message type <TYPE_HYP2000ARC>" );
					logit( "e", "; exiting!\n" );
					exit( -1 );
				}
				if( GetType( "TYPE_MAGNITUDE", &GetLogo[nLogo + 1].type ) != 0 )
				{
					logit( "e",
							"seisan_report: Invalid message type <TYPE_MAGNITUDE>" );
					logit( "e", "; exiting!\n" );
					exit( -1 );
				}
				nLogo += 2;
				init[6] = 1;
			}/* Enter name of local directory to write files to
             *************************************************/
				/*7*/ else if( k_its( "SfileDir" ) )
			{
				str = k_str( );
				if( str )
				{
					if( strlen( str ) >= MAX_DIR_LEN )
					{
						logit( "e", "seisan_report: SfileDir <%s> too long; ", str );
						exit( -1 );
					}

					strcpy( SfileDir, str );
					init[7] = 1;
				}
			}/* Enter name of directory where wave files are
             *********************************************/
				/*8*/ else if( k_its( "WavDir" ) )
			{
				str = k_str( );
				if( str )
				{
					if( strlen( str ) >= MAX_DIR_LEN )
					{
						logit( "e", "seisan_report: SfileDir <%s> too long; ", str );
						exit( -1 );
					}
					strcpy( WavDir, str );
					init[8] = 1;
				}
			}/* Agency code for locations and magnitudes
             *****************************************/
				/*9*/ else if( k_its( "Agency" ) )
			{
				str = k_str( );
				if( str )
				{
					if( strlen( str ) >= AGENCY_LEN )
					{
						logit( "e", "seisan_report: Agency <%s> too long; ", str );
						exit( -1 );
					}
					strcpy( Agency, str );
					init[9] = 1;
				}
			}
				/*10*/ else if( k_its( "AlarmMsg" ) )
			{
				AlarmMsg = k_int( );
				init[10] = 1;
			}/* RSL 2011.03: Optional commands */
			else if( k_its( "UseMSEEDArchive" ) )
			{
				UseMSEEDArchive = k_int( );
				if( UseMSEEDArchive < 0 || UseMSEEDArchive > 2 )
				{
					logit( "e", "seisan_report: Invalid MSEED archive format %d\n", UseMSEEDArchive );
					exit( -1 );
				}
				if( UseMSEEDArchive != 0 )
				{
					logit( "o", "seisan_report: Using MSEED archive reference format\n" );
				}
			}/* Unknown command     *****************/
			else
			{
				logit( "e", "seisan_report: <%s> Unknown command in <%s>.\n", com, configfile );
				continue;
			}

			/* See if there were any errors processing the command
			 *****************************************************/
			if( k_err( ) )
			{
				logit( "e", "seisan_report: Bad <%s> command in <%s>; exiting!\n", com, configfile );
				exit( -1 );
			}
		}
		nfiles = k_close( );
	}

	/* After all files are closed, check init flags for missed commands
	 ******************************************************************/
	nmiss = 0;
	for( i = 0; i < NUM_COMMANDS; i++ )
		if( !init[i] ) nmiss++;
	if( nmiss )
	{
		logit( "e", "seisan_report: ERROR, no " );
		if( !init[0] ) logit( "e", "<MyModuleId> " );
		if( !init[1] ) logit( "e", "<RingName> " );
		if( !init[2] ) logit( "e", "<HeartBeatInterval> " );
		if( !init[3] ) logit( "e", "<LogFile> " );
		if( !init[4] ) logit( "e", "<Debug> " );
		if( !init[5] ) logit( "e", "<WriteDelay> " );
		if( !init[6] ) logit( "e", "<GetEventsFrom> " );
		if( !init[7] ) logit( "e", "<SfileDir> " );
		if( !init[8] ) logit( "e", "<WavDir> " );
		if( !init[9] ) logit( "e", "<Agency> " );
		if( !init[10] ) logit( "e", "<AlarmMsg> " );
		logit( "e", "command(s) in <%s>; exiting!\n", configfile );
		exit( -1 );
	}
	logit( "t", " Using Sfile Dir: %s\n", SfileDir);
	logit( "t", "Using WavDir Dir: %s\n", WavDir);

	return;
}

/**************************************************************************
 *	seisan_report_lookup
 *
 *	Look up important info from earthworm.h tables.
 *
 **************************************************************************/
int seisan_report_lookup( void )
{
	/* Look up keys to shared memory regions
	 *************************************/
	if( ( RingKey = GetKey( RingName ) ) == -1 )
	{
		logit( "e", "seisan_report:  Invalid ring name <%s>; exiting!\n", RingName );
		return (EW_FAILURE );
	}

	/* Look up installations of interest
	 *********************************/
	if( GetLocalInst( &InstId ) != 0 )
	{
		logit( "e", "seisan_report: error getting local installation id; exiting!\n" );
		return (EW_FAILURE );
	}

	/* Look up modules of interest
	 ***************************/
	if( GetModId( MyModName, &MyModId ) != 0 )
	{
		logit( "e", "seisan_report: Invalid module name <%s>; exiting!\n", MyModName );
		return (EW_FAILURE );
	}

	/* Look up message types of interest
	 *********************************/
	if( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 )
	{
		logit( "e", "seisan_report: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
		return (EW_FAILURE );
	}
	if( GetType( "TYPE_ERROR", &TypeError ) != 0 )
	{
		logit( "e", "seisan_report: Invalid message type <TYPE_ERROR>; exiting!\n" );
		return (EW_FAILURE );
	}
	if( GetType( "TYPE_HYP2000ARC", &TypeHyp2000Arc ) != 0 )
	{
		logit( "e", "seisan_report: Invalid message type <TYPE_HYP2000ARC>; exiting!\n" );
		return (EW_FAILURE );
	}
	if( GetType( "TYPE_MAGNITUDE", &TypeMagnitude ) != 0 )
	{
		logit( "e", "seisan_report: Invalid message type <TYPE_MAGNITUDE>; exiting!\n" );
		return (EW_FAILURE );
	}
	if( GetType( "TYPE_EVENT_ALARM", &TypeAlarm ) != 0 )
	{
		logit( "e", "seisan_report: Invalid message type <TYPE_EVENT_ALARM>; exiting!\n" );
		return (EW_FAILURE );
	}
	return (EW_SUCCESS );
}

/*****************************************************************************
 * seisan_report_status
 *
 *	Input:	type - message type to send.
 *          ierr - error code, only used if type is TypeError.
 *          note - string to put in message.
 *
 *  Builds a message & puts it into shared memory.
 *
 *****************************************************************************/
void seisan_report_status( unsigned char type, short ierr, char *note )
{
	MSG_LOGO logo;
	char msg[256];
	long size;
	time_t t;

	/* Build the message
	 *******************/
	logo.instid = InstId;
	logo.mod = MyModId;
	logo.type = type;

	time( &t );

	if( type == TypeHeartBeat )
	{
		sprintf( msg, "%ld %d\n", (long)t, MyPid );
	}
	else if( type == TypeError )
	{
		sprintf( msg, "%ld %hd %s\n", (long)t, ierr, note );
		logit( "et", "seisan_report: %s\n", note );
	}
	else if( type == TypeAlarm )
	{
		sprintf( msg, "%ld %s\n", (long)t, note );
	}

	size = (long)strlen( msg ); /* don't include the null byte in the message */

	/* Write the message to shared memory
	 ************************************/
	if( tport_putmsg( &Region, &logo, size, msg ) != PUT_OK )
	{
		if( type == TypeHeartBeat )
		{
			logit( "et", "seisan_report:  Error sending heartbeat.\n" );
		}
		else if( type == TypeError )
		{
			logit( "et", "seisan_report:  Error sending error:%d.\n", ierr );
		}
		else if( type == TypeAlarm )
		{
			logit( "et", "seisan_report:  Error sending alarm:%d.\n", ierr );
		}
	}
	return;
}


