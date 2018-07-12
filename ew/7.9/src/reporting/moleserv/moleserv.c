/* Version history
 * 2014.06.04 - 0.0.11: added weight to pick and polarity
 * 2014.06.02 - 0.0.10: SQL updates for pick
 * 2014.05.29 - 0.0.09: fix of arrival output net and station reversed
 * 2013.10.10 - 0.0.08: Added version to QML output and fixed some bugs
 * 2013.10.09 - 0.0.07: Added some debugging functionalities
 * 2013.08.13 - 0.0.06: Added standalone functionality.
 * 2013.05.28 - 0.0.05:	Replaced streams in the server response to XML requests
 *						by fixed length buffer.
 * 2013.05.26 - 0.0.04:	Included Matteo's revision.
 *						Fixed error with quake versions.
 *						Added "includeallmagnitudes" to the xml request options
 * 2013.05.25 - 0.0.03:	Included waveform server to add tank export
 * 2013.05.14 - 0.0.02:	Modified mongoose to latest version: 3.7
 * 2013.05.12 - 0.0.01:	First version - Base test with 32bit
 */
/* Defines */
#define VERSION_STR 	"0.0.11 2014-06-04"		/* Version of this module */
#define MODNAME			"MoleServ"					/* Name of this module */
#define NWEBOPT 		100							/* Max. # of web options */
#define NCOMMAND		9							/* Min. # of commands */

/* Standard includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Earthworm includes */
#include <earthworm.h>
#include <kom.h>
#include <transport.h>

/* Local includes */
#include "iriswebreq.h"
#include "mongoose.h"
#include "websrequest.h"

/* Functions in this file */
int addwebopt( char* opt, char* val );
void freewebopt( void );
void config( char* configfile );
void lookup( void );
void status( unsigned char type, short ierr, char *note );
//static void *web_callback( enum mg_event event, struct mg_connection *conn );
static int web_callback( struct mg_connection *conn );
static void websocket_ready( struct mg_connection *conn );
static int websocket_data( struct mg_connection *conn );


/* Globals */
static SHM_INFO InRegion; /* Shared memory region for output */
static pid_t MyPid; /* Process id is sent with heartbeat */
static char* webopt[NWEBOPT]; /* Webserver options */
static int nwebopt = 0; /* Number of webserver options */
static int LogSwitch; /* 0 if no logfile should be written */
int Debug = 0; /* 0=no debug msgs, non-zero=debug */
static long HeartbeatInt; /* seconds between heartbeats */
static unsigned char MoleDB[17]; /* Name of the mole database */
static unsigned char MoleUser[17]; /* User name of the moledb */
static unsigned char MolePass[17]; /* Password */
static unsigned char ODBCName[17]; /* Name of the ODBC setting */
static unsigned char XMLUri[] = "mole"; /* Uri use for xml requests */
static unsigned char evtParamPubID[256]; /* Public ID for event parameters */
static size_t XMLbuflen = 10000; /* Buffer length for XML queries */
static WSPARAMS wsparams; /* Waveserver configurations */

static int UseWebSocket = 0;
static int StandAlone = 0;

/* Things to lookup from earthworm.d */
static long InRingKey; /* key of transport ring for input */
static unsigned char InstId; /* local installation id */
static unsigned char MyModId; /* Module Id for this program */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;

/* Main Program
 * - reads configurations
 * - looks up important information like keep-alives and msg types
 * - configures and starts webserver
 * - enters infinite loop with keep-alives until terminated
 ***********************************************************/
int main( int argc, char **argv )
{
	int i; /* Generic counter */
	char* opt;
	time_t timeNow; /* current time */
	time_t timeLastBeat; /* time last heartbeat was sent */
	struct mg_context *webserver; /* Webserver */
	struct mg_callbacks callbacks; /* Webserver callback structure */

	/* Check input arguments */
	if( argc < 2 )
	{
		fprintf( stderr, "Version: %s\n", VERSION_STR );
		fprintf( stderr, "Usage: %s <configfile> [OPTIONS]\n"
				"OPTIONS:\n"
				"\t-s\t: Standalone mode.\n", MODNAME );

		return EW_FAILURE;
	}
	/* Check for additional arguments */
	if( argc == 3 )
	{
		if( strcmp( argv[2], "-s" ) == 0 )
		{
			StandAlone = 1;
		}
		else
		{
			fprintf( stderr, "Unknown option: %s\n", argv[2] );
			return EW_FAILURE;
		}
	}


	/* Initialize name of log-file & open it */
	logit_init( argv[1], 0, 256, 1 );



	/* Set default web server configuration */
	if( !addwebopt( "enable_directory_listing", "no" ) ||
			!addwebopt( "listening_ports", "127.0.0.1:8080" ) ||
			!addwebopt( "extra_mime_types", ".tnk=application/octet-stream" ) )
	{
		freewebopt( );
		logit( "e",
				"%s: Unable to reserve memory for default webserver options.\n",
				MODNAME );
		return EW_FAILURE;
	}

	/* Set default parameters */
	evtParamPubID[0] = '\0';
	strcpy( evtParamPubID, "MoleServ" );
	wsparams.nwaveservers = 0;


	/* Read configuration file */
	config( argv[1] );


	/* Log web configurations */
	if( Debug )
	{
		i = 0;
		logit( "o", "%s: Webserver Options:\n", MODNAME );
		while( ( opt = webopt[ i ] ) != NULL )
		{
			logit( "o", "%3d - %s: %s\n", i / 2, webopt[i], webopt[i + 1] );
			i += 2;
		}
	}

	/* Create callback structure for server */
	memset( &callbacks, 0, sizeof( callbacks ) );
	callbacks.begin_request = web_callback;
	if( UseWebSocket )
	{
		callbacks.websocket_ready = websocket_ready;
		callbacks.websocket_data = websocket_data;
	}

	/* Start webserver */
	if( Debug ) logit( "o", "%s: Starting webserver...\n", MODNAME );
	webserver = mg_start( &callbacks, NULL, ( const char** ) webopt );
	if( webserver == NULL )
	{
		logit( "e", "%s: Unable to start web server\n", MODNAME );
		return EW_FAILURE;
	}
	if( Debug ) logit( "o", "%s: Webserver initiated on address - %s\n",
			MODNAME, mg_get_option( webserver, "listening_ports" ) );

	/* Start ODBC environment */
	if( Debug ) logit( "o", "%s: Starting ODBC environment...\n", MODNAME );
	if( !setODBCenv( ) )
	{
		logit( "et", "%s: Unable to set ODBC environment.\n", MODNAME );
		exit( -1 );
	}
	if( Debug ) logit( "o", "%s: ODBC environment initiated.\n", MODNAME );


	/* Not standalone mode */
	if( StandAlone == 0 )
	{
		/* Lookup info from earthworm */
		lookup( );

		/* Set logit to LogSwitch read from configfile */
		logit_init( argv[1], 0, 256, LogSwitch );
		logit( "", "%s: Read command file <%s>\n", MODNAME, argv[1] );


		/* Get our own process ID for restart purposes */
		if( ( MyPid = getpid( ) ) == -1 )
		{
			logit( "e", "%s: Call to getpid failed. Exiting.\n", MODNAME );
			exit( -1 );
		}


		/* Attach to shared memory rings */
		tport_attach( &InRegion, InRingKey );
		logit( "", "%s: Attached to public memory region: %ld\n",
				MODNAME, InRingKey );

		/* Force a heartbeat to be issued in first pass thru main loop */
		timeLastBeat = time( &timeNow ) - HeartbeatInt - 1;

		/*-------------------- setup done; start main loop -------------------*/
		if( Debug ) logit( "o", "%s: Starting main loop...\n", MODNAME );
		while( tport_getflag( &InRegion ) != TERMINATE &&
				tport_getflag( &InRegion ) != MyPid )
		{
			/* send heartbeat */
			if( HeartbeatInt &&
					time( &timeNow ) - timeLastBeat >= HeartbeatInt )
			{
				timeLastBeat = timeNow;
				status( TypeHeartBeat, 0, "" );
			}

			/* Wait for next time */
			sleep_ew( 1000 ); /* milliseconds */
		}

		/* Clear ODBC environment */
		clearODBCenv( );

		/* Stop webserver */
		mg_stop( webserver );

		/* detach from shared memory */
		tport_detach( &InRegion );

		/* Free web options array */
		freewebopt( );

		/* write a termination msg to log file */
		logit( "t", "%s: Termination requested; exiting!\n", MODNAME );
		fflush( stdout );
	}
	else
	{
		/* Standalone mode... Just an infinite loop */
		while( 1 )
		{
			//TODO: Maybe add some command line commands and output
			sleep_ew( 1000 );
		}
	}

	/* All done! */
	return 0;
}

/* Add new web options */
int addwebopt( char* opt, char* val )
{
	if( ( webopt[nwebopt] = ( char* ) malloc( sizeof( char ) *
			( strlen( opt ) + 1 ) ) ) != NULL )
		strcpy( webopt[nwebopt++], opt );
	else
		return -1;

	if( ( webopt[nwebopt] = ( char* ) malloc( sizeof( char ) *
			( strlen( val ) + 1 ) ) ) != NULL )
		strcpy( webopt[nwebopt++], val );
	else
		return -1;

	webopt[nwebopt] = NULL;
	return 1;
}

/* Free web options */
void freewebopt( void )
{
	int i;
	for( i = 0; i < nwebopt; i++ ) free( webopt[i] );
}

/* Read configuration file */
void config( char* configfile )
{
	char init[NCOMMAND]; /* init flags, one byte for each required command */
	int nmiss; /* number of required commands that were missed   */
	char *com;
	char *str, *val;
	int nfiles;
	int success;
	char processor[15];
	int i;
	char cp[80];



	/* Set to zero one init flag for each required command */
	for( i = 0; i < NCOMMAND; i++ ) init[i] = 0;

	/* Open the main configuration file */
	nfiles = k_open( configfile );
	if( nfiles == 0 )
	{
		logit( "e",
				"%s: Error opening command file <%s>.\n", MODNAME, configfile );
		exit( -1 );
	}

	/* Process all command files */
	while( nfiles > 0 ) /* While there are command files open */
	{
		while( k_rd( ) ) /* Read next line from active file  */
		{
			com = k_str( ); /* Get the first token from line */

			/* Ignore blank lines & comments */
			if( !com || com[0] == '#' ) continue;

			/* Open a nested configuration file */
			if( com[0] == '@' )
			{
				success = nfiles + 1;
				nfiles = k_open( &com[1] );
				if( nfiles != success )
				{
					logit( "e",
							"%s: Error opening command file <%s>; exiting!\n",
							MODNAME, &com[1] );
					exit( -1 );
				}
				continue;
			}

			/* Process anything else as a command */
			if( k_its( "LogFile" ) )
			{
				LogSwitch = k_int( );
				if( LogSwitch < 0 || LogSwitch > 2 )
				{
					logit( "e", "%s: Invalid <LogFile> value %d; "
							"must = 0, 1 or 2; exiting!\n",
							MODNAME, LogSwitch );
					exit( -1 );
				}
				init[0] = 1;
			}

			else if( k_its( "Debug" ) )
			{
				Debug = k_int( );
				init[1] = 1;
			}

			else if( k_its( "MyModuleId" ) )
			{
				if( str = k_str( ) )
				{
					if( GetModId( str, &MyModId ) != 0 )
					{
						logit( "e",
								"%s: Invalid module name <%s> "
								"in <MyModuleId> command; exiting!\n",
								MODNAME, str );
						exit( -1 );
					}
				}
				init[2] = 1;
			}

			else if( k_its( "InRing" ) )
			{
				if( str = k_str( ) )
				{
					if( ( InRingKey = GetKey( str ) ) == -1 )
					{
						logit( "e",
								"%s: Invalid ring name <%s> "
								"in <InRing> command; exiting!\n",
								MODNAME, str );
						exit( -1 );
					}
				}
				init[3] = 1;
			}

			else if( k_its( "HeartbeatInt" ) )
			{
				HeartbeatInt = k_long( );
				init[4] = 1;
			}

			else if( k_its( "MoleDB" ) )
			{
				strcpy( MoleDB, k_str( ) );
				init[5] = 1;
			}

			else if( k_its( "MoleUser" ) )
			{
				strcpy( MoleUser, k_str( ) );
				init[6] = 1;
			}

			else if( k_its( "MolePass" ) )
			{
				strcpy( MolePass, k_str( ) );
				init[7] = 1;
			}

			else if( k_its( "ODBCName" ) )
			{
				strcpy( ODBCName, k_str( ) );
				init[8] = 1;
			}


			else if( k_its( "XMLUri" ) )
			{
				strcpy( XMLUri, k_str( ) );
			}

			else if( k_its( "evtParamPubID" ) )
			{
				strcpy( evtParamPubID, k_str( ) );
			}

			else if( k_its( "XMLbuflen" ) )
			{
				XMLbuflen = ( size_t ) k_int( );
			}

			else if( k_its( "WebServerOpt" ) )
			{
				str = k_str( );
				val = k_str( );
				if( addwebopt( str, val ) == -1 )
				{
					logit( "e", "%s: Invalid web option (%s)\n", MODNAME, str );
					exit( -1 );
				}
			}

			else if( k_its( "WaveServer" ) )
			{
				if( wsparams.nwaveservers < MAX_WAVESERVERS )
				{
					strcpy( cp, k_str( ) );
					strcpy( wsparams.waveservers[wsparams.nwaveservers].wsIP,
							strtok( cp, ":" ) );
					strcpy( wsparams.waveservers[wsparams.nwaveservers].port,
							strtok( NULL, ":" ) );
					wsparams.nwaveservers++;
				}
				else
				{
					logit( "e", "%s: Excessive number of waveservers. "
							"Exiting.\n", MODNAME );
					exit( -1 );
				}
			}

			else if( k_its( "WSTimeout" ) )
			{
				wsparams.wstimeout = k_int( ) * 1000;
			}

			else if( k_its( "MaxSamples" ) )
			{
				wsparams.MaxSamples = k_int( );
			}

			else
			{
				logit( "e", "%s: <%s> Unknown command in <%s>.\n",
						MODNAME, com, configfile );
				continue;
			}

			/* See if there were any errors processing the command */
			if( k_err( ) )
			{
				logit( "e", "%s: Bad <%s> command in <%s>; exiting!\n",
						MODNAME, com, configfile );
				exit( -1 );
			}
		}
		nfiles = k_close( );
	}

	/* After all files are closed, check init flags for missed commands */
	nmiss = 0;
	for( i = 0; i < NCOMMAND; i++ ) if( !init[i] ) nmiss++;
	if( nmiss )
	{
		logit( "e", "%s: ERROR, no ", MODNAME );
		if( !init[0] ) logit( "e", "<LogFile> " );
		if( !init[1] ) logit( "e", "<Debug> " );
		if( !init[2] ) logit( "e", "<MyModuleId> " );
		if( !init[3] && StandAlone == 0 ) logit( "e", "<InRing> " );
		if( !init[4] && StandAlone == 0 ) logit( "e", "<HeartbeatInt> " );
		if( !init[5] ) logit( "e", "<MoleDB> " );
		if( !init[6] ) logit( "e", "<MoleUser> " );
		if( !init[7] ) logit( "e", "<MolePass> " );
		if( !init[8] ) logit( "e", "<ODBCName> " );
		logit( "e", "command(s) in <%s>; exiting!\n", configfile );
		exit( -1 );
	}
	return;
}

/* Lookup information from EW that is required */
void lookup( void )
{
	if( GetLocalInst( &InstId ) != 0 )
	{
		logit( "e",
				"%s: error getting local installation id; exiting!\n",
				MODNAME );
		exit( -1 );
	}

	/* Look up message types of interest */
	if( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 )
	{
		logit( "e",
				"%s: Invalid message type <TYPE_HEARTBEAT>; exiting!\n",
				MODNAME );
		exit( -1 );
	}
	if( GetType( "TYPE_ERROR", &TypeError ) != 0 )
	{
		logit( "e",
				"%s: Invalid message type <TYPE_ERROR>; exiting!\n", MODNAME );
		exit( -1 );
	}
	return;
}

/* Status messages */
void status( unsigned char type, short ierr, char *note )
{
	MSG_LOGO logo;
	char msg[256];
	long size;
	time_t t;

	/* Build the message */
	logo.instid = InstId;
	logo.mod = MyModId;
	logo.type = type;

	time( &t );

	if( type == TypeHeartBeat )
	{
		sprintf( msg, "%ld %ld\n", ( long ) t, ( long ) MyPid );
	}
	else if( type == TypeError )
	{
		sprintf( msg, "%ld %hd %s\n", ( long ) t, ierr, note );
		logit( "et", "%s: %s\n", MODNAME, note );
	}

	size = strlen( msg ); /* don't include the null byte in the message */

	/* Write the message to shared memory */
	if( tport_putmsg( &InRegion, &logo, size, msg ) != PUT_OK )
	{
		if( type == TypeHeartBeat )
		{
			logit( "et", "%s:  Error sending heartbeat.\n", MODNAME );
		}
		else if( type == TypeError )
		{
			logit( "et", "%s:  Error sending error:%d.\n", MODNAME, ierr );
		}
	}
	return;
}







/* Callback functions for webserver */
//static void *web_callback( enum mg_event event, struct mg_connection *conn )

static int web_callback( struct mg_connection *conn )
{
	struct mg_request_info* conninfo; /* Connection info */
	char* buffer; /* Buffer for xml or waveforms */
	size_t bsize = 0; /* Buffer size */
	FILE* stream; /* Data stream pointer */
	int res; /* Results evaluation */
	char querystr[2049]; /* Copy of query string */
	char tfilename[257]; /* Temporary file name */
	int tlink;
	char* sqlquery = NULL;


	/* Get information on the request */
	conninfo = mg_get_request_info( conn );


	/* Check if this is a valid request */
	if( conninfo->query_string != NULL )
	{
		/* So there is a query string. Ĉheck if its known URi */
		if( strcmp( conninfo->uri + 1, XMLUri ) == 0 )
		{
			/* Reserve buffer memory */
			buffer = ( char* ) malloc( sizeof( char ) * XMLbuflen );
			if( buffer == NULL )
			{
				logit( "e", "%s: Unable to reserve buffer for XML query\n",
						MODNAME );
				return 0;
			}

			/* Print XML data to stream */
			strcpy( querystr, conninfo->query_string );
			
			if( Debug )
				sqlquery = ( char* ) malloc( MAX_SQLQUERY_LEN );
				
			res = molereq( buffer, ( int ) XMLbuflen - 1, querystr, MoleDB,
					evtParamPubID, ODBCName, MoleUser, MolePass, sqlquery );
			if( Debug )
			{
				logit( "o", "Query generated %d bytes of data\n", res );
				
				if( res <= 0 )
				{
					/* HTTP Header */
					mg_printf( conn, "HTTP/1.1 200 OK\r\n"
						"Content-Type: text/xml\r\n"
						"\r\n" );

					/* Write data to client */
					mg_printf( conn, "<xml><error>"
							"<Description>Error processing SQL query</Description>"
							"<Data>%*.s</Data>"
							"</error></xml>", MAX_SQLQUERY_LEN, sqlquery );
				}
				
				free( sqlquery );
			}

			/* Check if buffer was enough for request */
			if( res < XMLbuflen )
			{
				/* HTTP Header */
				mg_printf( conn, "HTTP/1.1 200 OK\r\n"
						"Content-Type: text/xml\r\n"
						"\r\n" );

				/* Write data to client */
				mg_printf( conn, "%s", buffer );

				/* Free buffer */
				free( buffer );

				/* All done! */
				return 1;
			}
			else
			{
				/* HTTP Header */
				mg_printf( conn, "HTTP/1.1 200 OK\r\n"
						"Content-Type: text/plain\r\n"
						"\r\n" );

				/* Error message */
				mg_printf( conn, "Response exceeds buffer limit.\n", buffer );
				free( buffer );
				return 0;
			}
				
		}


			/* Waveform file export */
		else if( strcmp( conninfo->uri + 1, "dataselect" ) == 0 )
		{
			/* Waveform request - File mode
			 *******************************/

			//"sta=GNAR&cha=HHZ&net=NM&loc=--&start=2013-05-25T12:26:50.000&end=2013-05-25T12:27:30.000";

			/* Check if there are any waveservers available */
			if( wsparams.nwaveservers < 1 )
				return 0;

			/* Reserve memory for buffer */
			buffer = ( char* ) malloc( ( size_t ) ( wsparams.MaxSamples * 4 ) );
			if( buffer == NULL )
			{
				if( Debug )
					logit( "e", "%s: Unable to reserve buffer for waveserver "
						"call.\n", MODNAME );
				return 0;
			}
			bsize = wsparams.MaxSamples * 4;

			/* Parse request and get waveserver data */
			strcpy( querystr, conninfo->query_string );
			res = websrequest( querystr, buffer, &bsize,
					tfilename, 256, &wsparams );
			if( res != WAVES_OK )
			{
				if( Debug )
					logit( "e", "%s: Error (%d) serving request <%s>.\n",
						MODNAME, res, querystr );
				free( buffer );
				return 0;
			}

			/* Send HTTP header */
			mg_printf( conn,
					"HTTP/1.1\r\n"
					"Content-Type: application/octet-stream\r\n"
					"Content-Disposition: attachment; filename=%s\r\n"
					"Content-length: %d\r\n"
					"\r\n", tfilename, ( int ) bsize );

			/* Send binary data */
			mg_write( conn, buffer, bsize );

			/* Free buffer */
			free( buffer );

			return 1;
		}
	}


	/* Let the webserver handle this one */
	return 0;
}




/******************************************************************************
 * Websocket Requests                                                         *
 *                                                                            *
 * MoleServ supports websocket interaction with a web client. The websocket   *
 * protocol recognizes messages in JSON format, with the following structure: *
 *                                                                            *
 * reqID : " " - A arbitrary alphanumeric request ID                          *
 * query : " " - A query similar to the web queries                           *
 *   or                                                                       *
 * query : {                                                                  *
 *             type: "waveform" or "event"                                    *
 ******************************************************************************/

/* Callbacks for websocket */

/* Server ready */
static void websocket_ready( struct mg_connection *conn )
{
	unsigned char buf[40];

	if( Debug )
		logit( "o",
			"%s: Received websocket initialization request\n", MODNAME );
	buf[0] = 0x81;
	buf[1] = snprintf(
			( char * ) buf + 2, sizeof( buf ) - 2, "%s", "MoleServer ready" );
	mg_write( conn, buf, 2 + buf[1] );
}

static int websocket_data( struct mg_connection *conn )
{
	char buf[1024];
	int len = 0;
	int n;
	n = mg_read( conn, buf + len, sizeof( buf ) - len );
	printf( "Read %d characters\n<%s>\n", n, buf );
	return 1;
}























