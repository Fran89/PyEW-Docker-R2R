
#define		reqDelimiter		"&"		/* Query separator */
#define		MIN_PARAM_COUNT		5		/* Required number of parameters */
#define		MAX_DURATION		240		/* Max. duration of request */

/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ws_clientII.h>
#include <chron3.h>
#include "websrequest.h"

/* Earthworm includes */
#include <earthworm.h>

#ifdef WIN32
#define strtok_r strtok_s
#endif


/* Functions in this file */
double parseTime( char* t );
WAVE_STATUS getWStrbf( char* buffer, size_t* buflen,
		double starttime, double endtime, char* sta, char* cha,
		char* net, char* loc, WSPARAMS* params );

static int		Debug = 0;

/*
 * Handles a waveform request
 */

WAVE_STATUS websrequest( char* request, char* buffer, size_t* nbuffer,
		char* fname, int nfname, WSPARAMS* params )
{
	char*	treq;
	char*	state;		/* For thread safety while parsing string */
	char*	token;		/* To check request segments */
	char*	eqsign;		/* Pointer to equal sign */
	int		ncmp;		/* Number of characters to compare for parameter name */
	int		nparams = 0;/* Counter for minimum number of request params */
	double 	starttime = -1.0;
	double	endtime = -1.0;
	char	sta[13];
	char	cha[13];
	char	net[13];
	char	loc[13];
	WAVE_STATUS	ret;


	/* Parse web request
	 ********************/

	/* Reserve memory for temporary request string */
	treq = ( char* ) malloc( ((strlen( request ))+1) * sizeof( char ) );
	if( treq == NULL )
		return WAVES_MEM_ALLOC_ERR;
	strcpy( treq, request );


	/* Go through every field */
	for( token = strtok_r( treq, reqDelimiter, &state );
			token != NULL;
			token = strtok_r( NULL, reqDelimiter, &state ) )
	{
		/* Find equal sign */
		eqsign = strchr( token, '=' );

		/* If = is not found... its an invalid parameter */
		if( eqsign == NULL )
		{
			if( Debug )
				printf( "Invalid wave request\n" );
			free( treq );
			return WAVES_INVAL_PARAM;
		}

		/* Number of characters to use for the comparison */
		ncmp = eqsign - token;

		/* Start time */
		if( strncmp( token, "start", 5 ) == 0 ||
				strncmp( token, "starttime", 9 ) == 0 )
		{
			starttime = parseTime( eqsign + 1 );
			if( starttime == 0 )
				continue;
			nparams++;
		}

		/* End time */
		else if( strncmp( token, "end", 3 ) == 0 ||
				strncmp( token, "endtime", 7 ) == 0 )
		{
			endtime = parseTime( eqsign + 1 );
			if( endtime == 0 )
				continue;
		}

		/* Station */
		else if( strncmp( token, "sta", 3 ) == 0 ||
				strncmp( token, "station", 7 ) == 0 )
		{
			strncpy( sta, eqsign + 1, 12 );
			nparams++;
		}

		/* Channel */
		else if( strncmp( token, "cha", 3 ) == 0 ||
				strncmp( token, "channel", 7 ) == 0 )
		{
			strncpy( cha, eqsign + 1, 12 );
			nparams++;
		}

		/* Network */
		else if( strncmp( token, "net", 3 ) == 0 ||
				strncmp( token, "network", 7 ) == 0 )
		{
			strncpy( net, eqsign + 1, 12 );
			nparams++;
		}

		/* Location */
		else if( strncmp( token, "loc", 3 ) == 0 ||
				strncmp( token, "location", 8 ) == 0 )
		{
			strncpy( loc, eqsign + 1, 12 );
			nparams++;
		}
	}
	if( Debug )
	{
		printf( "Finished parsing request... %d params\n", nparams );
		printf( "%s.%s.%s.%s: %f\n", sta, cha, net, loc, starttime ) ;
	}

	/* Release memory */
	free( treq );

	/* Check number of parameters */
	if( nparams < MIN_PARAM_COUNT )
		return WAVES_INVAL_REQ;

	/* Check endtime */
	if( endtime == -1 )
		endtime = starttime + MAX_DURATION;




	/* Get waveform data from source
	 ********************************/
	ret = getWStrbf( buffer, nbuffer, starttime, endtime, sta, cha,
			net, loc, params );
	if( ret != WAVES_OK ) return ret;

	/* Create filename for this data */
	snprintf( fname, nfname, "%s.%s.%s.%s.%.3f_%.3f.tnk",
			sta, cha, net, loc, starttime, endtime );



	/* All done!
	 ***********/
	return WAVES_OK;
}







/* Parse a time string into a double epoch time */
double parseTime( char* t )
{
	char	carldate[19]; 		/* Store a datetime in the epochsec17 format */
	char*	pos = t;
	int		i;
	double	out;

	carldate[0] = '\0';
	i = strlen( t );
	if( i < 4 )
	{
		if( Debug )
			printf( "Invalid date (%s)\n", t );
		return 0.0;
	}

	/* Year */
	if( i >= 4 )
		strncat( carldate, t, 4 );

	/* Month */
	if( i >= 7 )
		strncat( carldate, t + 5, 2 );

	/* Day */
	if( i >= 10 )
		strncat( carldate, t + 8, 2 );

	/* Hour */
	if( i >= 13 )
		strncat( carldate, t + 11, 2 );

	/* Minute */
	if( i >= 16 )
		strncat( carldate, t + 14, 2 );

	/* Second */
	if( i >= 19 )
		strncat( carldate, t + 17, 2 );

	/* Separating dot */
	if( i >= 20 )
		strncat( carldate, t + 19, 1 );

	/* Decimal seconds */
	if( i>= 21 )
		strncat( carldate, t + 20, 3 );

	if( ( i = strlen( carldate ) ) < 18 )
	{
		for( ; i < 18; i ++ )
			carldate[i] = '0';
		carldate[18] = '\0';
		carldate[14] = '.';
	}

	/* Convert using epochsec17 */
	if( epochsec18( &out, carldate ) != 0 )
	{
		if( Debug )
			printf( "Error parsing date (%s) from (%s).\n",
					carldate, t );
	}

	return out;
}








/****************************************************************************************
 * getWStrbf: Retrieve a set of samples from the waveserver and store the raw tracebuf2 *
 *            in a buffer                                                               *
 ****************************************************************************************/
WAVE_STATUS getWStrbf( char* buffer, size_t* buflen,
		double starttime, double endtime, char* sta, char* cha,
		char* net, char* loc, WSPARAMS* params )
{
	WS_MENU_QUEUE_REC 	menu_queue;
	TRACE_REQ 			trace_req;
	int 				wsResponse;
	int					atLeastOne;
	int					i;
	char				WSErrorMsg[80];

	/* Initialize menu queue
	 ***********************/
	menu_queue.head = NULL;
	menu_queue.tail = NULL;

	atLeastOne = 0;

	/* Make menu request
    *******************/
	for( i = 0; i < params->nwaveservers; i++ )
	{
		wsResponse = wsAppendMenu(
				params->waveservers[i].wsIP, params->waveservers[i].port,
				&menu_queue, params->wstimeout );
		if( wsResponse != WS_ERR_NONE )
		{
			continue;
		}
		else
		{
			atLeastOne++;
		}
	}
	if( atLeastOne == 0 )
	{
		return WAVES_WS_UNAVAIL;
	}

	/* Make request structure
	************************/
	strcpy( trace_req.sta, sta );
	strcpy( trace_req.chan, cha );
	strcpy( trace_req.net, net );
	strcpy( trace_req.loc, loc );
	trace_req.reqStarttime = starttime;
	trace_req.reqEndtime = endtime;
	trace_req.partial = 1;
	trace_req.pBuf = buffer;
	trace_req.bufLen = *buflen;
	trace_req.timeout = params->wstimeout;
	trace_req.fill = 0;


	/* Pull data from ws
	 *******************/
	wsResponse = wsGetTraceBinL( &trace_req, &menu_queue, params->wstimeout );
	if( wsResponse != WS_ERR_NONE )
	{
		if( Debug )
		{
			switch( wsResponse )
			{
				case WS_ERR_NO_CONNECTION:
					fprintf( stderr,
							"Waveserver reading error: No connection\n" );
					break;
				case WS_ERR_SOCKET:
					fprintf( stderr,
							"Waveserver reading error: "
							"Could not create socket\n" );
					break;
				case WS_ERR_BROKEN_CONNECTION:
					fprintf( stderr,
							"Waveserver reading error: "
							"Connection broke\n" );
					break;
				case WS_ERR_TIMEOUT:
					fprintf( stderr,
							"Waveserver reading error: Connection timeout\n" );
					break;
				case WS_ERR_MEMORY:
					fprintf( stderr,
							"Waveserver reading error: Out of memory\n" );
					break;
				case WS_ERR_INPUT:
					fprintf( stderr,
							"Waveserver reading error: "
							"Bad or missing input parameters\n" );
					break;
				case WS_ERR_PARSE:
					fprintf( stderr,
							"Waveserver reading error: "
							"Menu parser fail.\n" );
					break;
			}
			fflush( stderr );
		}
		return WAVES_WS_ERR;
	}

	/* Update output number of bytes
	 *********************************/
	*buflen = ( int ) trace_req.actLen;


	/* Terminate connection
     **********************/
    wsKillMenu( &menu_queue );

    return WAVES_OK;
}














