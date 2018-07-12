/******************************************************************************
 * Parser for event requests using the format from IRIS Event Webservice      *
 * Details on the format can be found here: http://www.iris.edu/ws/event      *
 *                                                                            *
 * This file contains a set of functions to parse a GET request using the     *
 * webservice format in a series of SQL queries that can be used to access    *
 * a Mole database (http://hdl.handle.net/2122/8002).                         *
 *                                                                            *
 * Main functionality is given by the function:                               *
 *                                                                            *
 *    int irisreq2molereq( char* irisreq, MOLEREQ** molereq, int* nreq  )     *
 *                                                                            *
 * where parameters are:                                                      *
 *                                                                            *
 * char* irisreq		: a request in the IRIS event format                  *
 * MOLEREQ** molereq	: an array with the mole db requests                  *
 * int* nreq			: inputs the maximum number of acceptable requests    *
 *						  and outputs the number of produced mole requests    *
 *                                                                            *
 ******************************************************************************/

/* Defines
 **********/
#define		reqDelimiter	"&"		/* Query separator */
#define		QML_LINETERM	""

/* Standard includes
 ********************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Windows include for SQL
 ************************/
#ifdef WIN32
#include <windows.h>
#define strtok_r strtok_s
#endif

/* ODBC includes
 ****************/
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

/* Local includes
 ****************/
#include "iriswebreq.h"

/* Earthworm includes */
#include <earthworm.h>



/* Functions in this file
 *************************/
PARSE_STATUS irisreq2molereq( char* irisreq, MOLEREQ* molereq, char* dbname );
PARSE_STATUS parsewebreq( char* irisreq, IRISWEBREQ* req );
void fillQuery( char* target, char* param, int* isFirst );
void initRequest( IRISWEBREQ* req );
int magreq( char* buffer, int nbuffer, SQLHDBC conn, char* dbname,
		SQLUINTEGER qkseq, SQLUINTEGER fk_sqkseq );
int phasereq( char* buffer, int nbuffer, SQLHDBC conn, char* dbname,
		SQLUINTEGER qkseq, SQLUINTEGER fk_sqkseq );
size_t getspace( int a, int b );
void getOriginId( char* dest, int ndest, SQLUINTEGER ewsqkid,
		SQLUINTEGER version );
void getMagnitudeId( char* dest, int ndest, SQLUINTEGER magmodID,
		char* originID );

/* Global Variables
 *******************/
extern int		Debug;	/* this comes from config file */
static SQLHENV		odbcEnv;		/* ODBC Environment handle */



PARSE_STATUS irisreq2molereq( char* irisreq, MOLEREQ* molereq, char* dbname )
{
	IRISWEBREQ 		request;		/* Request structure to be populated */
	PARSE_STATUS	out = IRISQ_OK;	/* Default output status */
	int				isFirst = 1;
	char			tempstr[256];
	char			parpre[256];		/* Preceeds query params */


	/* Populate request structure */
	initRequest( &request );
	if( ( out = parsewebreq( irisreq, &request ) ) != IRISQ_OK )
		return out;

	/* Create events query */
	if( request.includeallorigins  == IRISQ_TRUE )
	{
		if( snprintf( molereq->query, MAX_SQLQUERY_LEN,
				"SELECT * FROM %s.%s WHERE", dbname, EVT_TABLE )
				> MAX_SQLQUERY_LEN )
			return IRISQ_QUERY_TOO_LONG;
		sprintf( parpre, "%s.%s", dbname, EVT_TABLE );
	}
	else
	{
		if( snprintf( molereq->query, MAX_SQLQUERY_LEN,
				"SELECT yt.* FROM %s.%s yt "
				"INNER JOIN( "
				"SELECT %s, MAX(%s) %s "
				"FROM %s.%s "
				"GROUP BY %s "
				") ss ON yt.%s = ss.%s AND yt.%s = ss.%s WHERE",
				dbname, EVT_TABLE,
				EVT_ID, EVT_VER, EVT_VER,
				dbname, EVT_TABLE,
				EVT_ID,
				EVT_ID, EVT_ID, EVT_VER, EVT_VER )
				> MAX_SQLQUERY_LEN )
			return IRISQ_QUERY_TOO_LONG;
		strcpy( parpre, "yt" );
	}


	/* Basic query parameters
	 * These parameters are used to query the events table directly
	 ***************************************************************/

	/* Start time */
	if( request.starttime[0] != '\0' )
	{
		snprintf( tempstr, 256, " %s.%s>=CAST('%s' AS DATETIME)",
				parpre, EVT_OT, request.starttime );
		fillQuery( molereq->query, tempstr, &isFirst );
	}

	/* End time */
	if( request.endtime[0] != '\0' )
	{
		snprintf( tempstr, 256, " %s.%s<=CAST('%s' AS DATETIME)",
				parpre, EVT_OT, request.endtime );
		fillQuery( molereq->query, tempstr, &isFirst );
	}

	/* Minimum latitude */
	if( request.minlatitude[0] != '\0' )
	{
		snprintf( tempstr, 256, " %s.%s>=%s",
				parpre, EVT_LAT, request.minlatitude );
		fillQuery( molereq->query, tempstr, &isFirst );
	}

	/* Maximum latitude */
	if( request.maxlatitude[0] != '\0' )
	{
		snprintf( tempstr, 256, " %s.%s<=%s",
				parpre, EVT_LAT, request.maxlatitude );
		fillQuery( molereq->query, tempstr, &isFirst );
	}

	/* Minimum longitude */
	if( request.minlongitude[0] != '\0' )
	{
		snprintf( tempstr, 256, " %s.%s>=%s",
				parpre, EVT_LON, request.minlongitude );
		fillQuery( molereq->query, tempstr, &isFirst );
	}

	/* Maximum longitude */
	if( request.maxlongitude[0] != '\0' )
	{
		snprintf( tempstr, 256, " %s.%s<=%s",
				parpre, EVT_LON, request.maxlongitude );
		fillQuery( molereq->query, tempstr, &isFirst );
	}

	/* Minimum magnitude */
	if( request.minmag[0] != '\0' )
	{
		snprintf( tempstr, 256, " %s.%s>=%s",
				parpre, EVT_MAG, request.minmag );
		fillQuery( molereq->query, tempstr, &isFirst );
	}

	/* Maximum magnitude */
	if( request.maxmag[0] != '\0' )
	{
		snprintf( tempstr, 256, " %s.%s<=%s",
				parpre, EVT_MAG, request.maxmag );
		fillQuery( molereq->query, tempstr, &isFirst );
	}

	/* Magnitude Type */
	if( request.magtype[0] != '\0' )
	{
		snprintf( tempstr, 256, " %s.%s='%s'",
				parpre, EVT_MAGTYPE, request.magtype );
		fillQuery( molereq->query, tempstr, &isFirst );
	}

	/* Event ID */
	if( request.eventid[0] != '\0' )
	{
		snprintf( tempstr, 256, " %s.%s='%s'",
				parpre, EVT_ID, request.eventid );
		fillQuery( molereq->query, tempstr, &isFirst );
	}

	/* Update after */
	if( request.updateafter[0] != '\0' )
	{
		snprintf( tempstr, 256, " %s.%s>CAST('%s' AS DATETIME)",
				parpre, EVT_LUPD, request.updateafter );
		fillQuery( molereq->query, tempstr, &isFirst );
	}

	/* Query limitation parameters
	 * These parameters are placed at the end of the query to limit results
	 ***********************************************************************/


	if( request.offset[0] != '\0' )
	{
		isFirst = 1;
		snprintf( tempstr, 256, " OFFSET %s", request.offset );
		fillQuery( molereq->query, tempstr, &isFirst );
	}

	/* Order the presented results */
	isFirst = 1;
	snprintf( tempstr, 256, " ORDER BY %s.%s, %s.%s DESC",
			parpre, EVT_ID, parpre, EVT_VER );
	fillQuery( molereq->query, tempstr, &isFirst );

	/* Optional ordering arguments */
	if( request.orderby[0] != '\0' )
	{
		/* Translate orderby to mole columns */
		if( strcmp( request.orderby, "time" ) == 0 )
			snprintf( tempstr, 256, ", %s.%s DESC",
					parpre, EVT_OT, request.orderby );
		else if( strcmp( request.orderby, "time-asc" ) == 0 )
			snprintf( tempstr, 256, ", %s.%s ASC",
					parpre, EVT_OT, request.orderby );
		else if( strcmp( request.orderby, "magnitude" ) == 0 )
			snprintf( tempstr, 256, ", %s.%s DESC",
					parpre, EVT_MAG, request.orderby );
		else if( strcmp( request.orderby, "magnitude-asc" ) == 0 )
			snprintf( tempstr, 256, ", %s.%s ASC",
					parpre, EVT_MAG, request.orderby );
		else
			tempstr[0] = '\0';
		isFirst = 1;
		fillQuery( molereq->query, tempstr, &isFirst );
	}

	/* Limit the number of results */
	if( request.limit[0] != '\0' )
	{
		isFirst = 1;
		snprintf( tempstr, 256, " LIMIT %s", request.limit );
		fillQuery( molereq->query, tempstr, &isFirst );
	}


	/* Will assume that secondary queries are performed based on ewsqkid
	 ********************************************************************/
	molereq->includeallmagnitudes =
			( request.includeallmagnitudes == IRISQ_TRUE );

	molereq->includearrivals =
		( request.includearrivals == IRISQ_TRUE );



	/* Done */
	return out;
}





/******************************************************************************
 * Breaks a web request in tokens to populate a structure                     *
 ******************************************************************************/
PARSE_STATUS parsewebreq( char* irisreq, IRISWEBREQ* req )
{
	char*	treq;		/* Temporary request string */
	char*	state;		/* For thread safety while parsing string */
	char*	token;		/* To check request segments */
	char*	eqsign;		/* Pointer to equal sign */
	int		ncmp;		/* Number of characters to compare for parameter name */
	int i;
	char* c;
	PARSE_STATUS out = IRISQ_OK;	/* Default output status */

	/* Check that output is not NULL */
	if( req == NULL )
		return IRISQ_OUT_OF_MEMORY;

	/* Reserve memory for temporary request string */
	treq = ( char* ) malloc( (strlen( irisreq )+1) * sizeof( char ) );
	if( treq == NULL )
		return IRISQ_OUT_OF_MEMORY;
	strcpy( treq, irisreq );


	/* Go through every field */
	for( token = strtok_r( treq, reqDelimiter, &state );
			token != NULL;
			token = strtok_r( NULL, reqDelimiter, &state ) )
	{

		/* Find equal sign */
		eqsign = strchr( token, '=' );

		/* If = is not found... its an invalid parameter */
		if( eqsign == NULL && out == IRISQ_OK )
		{
			out = IRISQ_INVALID_PARAM;
			continue;
		}

		/* Number of characters to use for the comparison */
		ncmp = eqsign - token;


		/* Start time */
		if( strncmp( token, "start", ncmp ) == 0 ||
				strncmp( token, "starttime", ncmp ) == 0 )
		{
			strncpy( req->starttime, eqsign + 1, SQL_TIMEDATE_LEN );
		}

		/* End time */
		else if( strncmp( token, "end", ncmp ) == 0 ||
				strncmp( token, "endtime", ncmp ) == 0 )
		{
			strncpy( req->endtime, eqsign + 1, SQL_TIMEDATE_LEN );
		}

		/* Minimum latitude */
		else if( strncmp( token, "minlat", ncmp ) == 0 ||
				strncmp( token, "minlatitude", ncmp ) == 0 )
		{
			strncpy( req->minlatitude, eqsign + 1, SQL_FLOAT_LEN );
		}

		/* Maximum latitude */
		else if( strncmp( token, "maxlat", ncmp ) == 0 ||
				strncmp( token, "maxlatitude", ncmp ) == 0 )
		{
			strncpy( req->maxlatitude, eqsign + 1, SQL_FLOAT_LEN );
		}

		/* Minimum longitude */
		else if( strncmp( token, "minlon", ncmp ) == 0 ||
				strncmp( token, "minlongitude", ncmp ) == 0 )
		{
			strncpy( req->minlongitude, eqsign + 1, SQL_FLOAT_LEN );
		}

		/* Maximum longitude */
		else if( strncmp( token, "maxlon", ncmp ) == 0 ||
				strncmp( token, "maxlongitude", ncmp ) == 0 )
		{
			strncpy( req->maxlongitude, eqsign + 1, SQL_FLOAT_LEN );
		}

		/* Minimum depth */
		else if( strncmp( token, "mindepth", ncmp ) == 0 )
		{
			strncpy( req->mindepth, eqsign + 1, SQL_FLOAT_LEN );
		}

		/* Maximum depth */
		else if( strncmp( token, "maxdepth", ncmp ) == 0 )
		{
			strncpy( req->maxdepth, eqsign + 1, SQL_FLOAT_LEN );
		}

		/* Minimum magnitude */
		else if( strncmp( token, "minmag", ncmp ) == 0 ||
				strncmp( token, "minmagnitude", ncmp ) == 0 )
		{
			strncpy( req->minmag, eqsign + 1, SQL_FLOAT_LEN );
		}

		/* Maximum magnitude */
		else if( strncmp( token, "maxmag", ncmp ) == 0 ||
				strncmp( token, "maxmagnitude", ncmp ) == 0 )
		{
			strncpy( req->maxmag, eqsign + 1, SQL_FLOAT_LEN );
		}

		/* Magnitude type */
		else if( strncmp( token, "magtype", ncmp ) == 0 ||
				strncmp( token, "magnitudetype", ncmp ) == 0 )
		{
			strncpy( req->magtype, eqsign + 1, SQL_MAGTYPE_LEN );
		}

		/* TODO: Catalog */

		/* TODO: Contributor */

		/* Limit */
		else if( strncmp( token, "limit", ncmp ) == 0 )
		{
			strncpy( req->limit, eqsign + 1, SQL_NUM_LEN );
		}

		/* Offset */
		else if( strncmp( token, "offset", ncmp ) == 0 )
		{
			strncpy( req->offset, eqsign + 1, SQL_NUM_LEN );
		}

		/* Order of the events */
		else if( strncmp( token, "orderby", ncmp ) == 0 )
		{
			strncpy( req->orderby, eqsign + 1, SQL_ORDER_LEN );
		}

		/* Limit to last update */
		else if( strncmp( token, "updateafter", ncmp ) == 0 )
		{
			strncpy( req->updateafter, eqsign + 1, SQL_TIMEDATE_LEN );
		}

		/* Include all origins */
		else if( strncmp( token, "includeallorigins", ncmp ) == 0 )
		{
			if( strcmp( eqsign + 1, "yes" ) == 0 ||
					strcmp( eqsign + 1, "true" ) == 0 )
				req->includeallorigins = IRISQ_TRUE;
		}

		/* Include all magnitudes */
		else if( strncmp( token, "includeallmagnitudes", ncmp ) == 0 )
		{
			if( strcmp( eqsign + 1, "yes" ) == 0 ||
					strcmp( eqsign + 1, "true" ) == 0 )
				req->includeallmagnitudes = IRISQ_TRUE;
		}

		/* Include all arrivals (and corresponding picks) */
		else if( strncmp( token, "includearrivals", ncmp ) == 0 )
		{
			if( strcmp( eqsign + 1, "yes" ) == 0 ||
					strcmp( eqsign + 1, "true" ) == 0 )
				req->includearrivals = IRISQ_TRUE;
		}

		/* Event ID - Only one event */
		else if( strncmp( token, "eventid", ncmp ) == 0 )
		{
			strncpy( req->eventid, eqsign + 1, SQL_EVENTID_LEN );
		}

	}

	/* Release memory */
	free( treq );


	/* Done */
	return out;
}




/******************************************************************************
 * Fill in parameter in query                                                 *
 ******************************************************************************/
void fillQuery( char* target, char* param, int* isFirst )
{
	if( *isFirst == 0 )
		strncat( target, " AND", MAX_SQLQUERY_LEN - strlen( target ) - 1 );
	else
		*isFirst = 0;
	strncat( target, param, MAX_SQLQUERY_LEN - strlen( target ) - 1 );
}







/******************************************************************************
 * Error messages - Returns an error message based on an error code           *
 ******************************************************************************/
void irisreq_errormsg( PARSE_STATUS error, char* msg, int msglen )
{
	switch( error )
	{
		case IRISQ_OK:
			snprintf( msg, msglen, "Parse OK" );
			break;
		case IRISQ_QUERY_TOO_LONG:
			snprintf( msg, msglen, "Query too long for available memory (%d)",
					MAX_SQLQUERY_LEN );
			break;
		case IRISQ_OUT_OF_MEMORY:
			snprintf( msg, msglen, "Unable to reserve memory for parsing" );
			break;
		case IRISQ_INVALID_PARAM:
			snprintf( msg, msglen, "Detected an invalid query parameter" );
			break;
		default:
			snprintf( msg, msglen, "Unknown error" );
	}
}

/******************************************************************************
 * Initialize a request structure                                             *
 ******************************************************************************/
void initRequest( IRISWEBREQ* req )
{
	// TODO: Use cycles to REALLY initialize all the strings
	req->starttime[0] = '\0';
	req->endtime[0] = '\0';
	req->minlatitude[0] = '\0';
	req->maxlatitude[0] = '\0';
	req->minlongitude[0] = '\0';
	req->maxlongitude[0] = '\0';
	req->mindepth[0] = '\0';
	req->maxdepth[0] = '\0';
	req->minmag[0] = '\0';
	req->maxmag[0] = '\0';
	req->magtype[0] = '\0';
	req->limit[0] = '\0';
	req->offset[0] = '\0';
	req->orderby[0] = '\0';
	req->updateafter[0] = '\0';
	req->includeallorigins = IRISQ_FALSE;
	req->includeallmagnitudes = IRISQ_FALSE;
	req->includearrivals = IRISQ_FALSE;
	req->eventid[0] = '\0';
}










/******************************************************************************
 * Initialize ODBC environment. Common for all connections to be made         *
 ******************************************************************************/
int setODBCenv( void )
{
	long 	ret;					/* For returns */

	/* Allocate environment handle */
	ret = SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, &odbcEnv );
	if( ( ret != SQL_SUCCESS ) && ( ret != SQL_SUCCESS_WITH_INFO ) )
		return -1;

	/* Set version */
	ret = SQLSetEnvAttr( odbcEnv, SQL_ATTR_ODBC_VERSION,
			( void* ) SQL_OV_ODBC3, 0 );
	if( ( ret != SQL_SUCCESS ) && ( ret != SQL_SUCCESS_WITH_INFO ) )
	{
		SQLFreeHandle( SQL_HANDLE_ENV, odbcEnv );
		return -1;
	}

	return 1;
}

/******************************************************************************
 * Free ODBC environment.                                                     *
 ******************************************************************************/
void clearODBCenv( void )
{
	SQLFreeHandle( SQL_HANDLE_ENV, odbcEnv );
}





int molereq( char* buffer, int nbuffer, char* webrequest, char* dbname, char* evtParamPubID,
		char* datasource, char* username, char* password, char* sqlquery )
{
	SQLHDBC 		conn;			/* Handle for SQL connection */
	SQLHSTMT		stmt;			/* Handle for SQL statements */
	long ret;						/* Return from ODBC functions */
#ifdef __x86_64__
	SQLLEN			nrows;
#else
	SQLINTEGER		nrows;			/* Number of returned rows */
#endif
	int				i;				/* Generic counter */
	long			indicator;		/* Data indicator for SQL */
	PARSE_STATUS 	pstatus;		/* Status indicator - parsing the webreq */
	char			errmsg[257];	/* Buffer for messages */
	MOLEREQ			molequery;		/* Buffer for mole queries */
	char			tempstr[257];	/* Temporary string */

	/* Variables for reading the quake data
	 ***************************************/
	char				originID[257];
	char				magnitudeID[257];
	SQLUINTEGER			fk_sqkseq, ewsqkid, lastewsqkid = 0, last_fk_sqkseq = 0;
	SQLUINTEGER			version;
	TIMESTAMP_STRUCT	ot_dt;
	SQLUINTEGER			otusec;
	SQLDOUBLE			lat;
	SQLDOUBLE			lon;
	SQLDOUBLE			depth;
	SQLUINTEGER			magmodule;
	SQLDOUBLE			mag;
	SQLCHAR				magtype[33];
	long				magindicator;
	int					bufoffset = 0;
/* from msdn.microsoft.com dropping this stuff in to read returned SQL errors - Stefan */
	SQLCHAR       SqlState[6] = "", SQLStmt[100] = "", Msg[SQL_MAX_MESSAGE_LENGTH] = "";
	SQLINTEGER    NativeError = 0;
	SQLSMALLINT   j = 0, k=0, MsgLen = 0;
	SQLRETURN     rc1, rc2;
	SQLHSTMT      hstmt = "";

	/* Parse the webrequest into multiple SQL queries */
	if( (pstatus = irisreq2molereq( webrequest, &molequery, dbname ) ) != IRISQ_OK )
	{
		if( Debug )
		{
			irisreq_errormsg( pstatus, errmsg, 256 );
			fprintf( stderr, "%s\n", errmsg );
			fflush( stderr );
		}
		return 0;
	}
	if( Debug ) printf("Query:\n%s\n\n", molequery.query );

	// For debugging
	if( sqlquery != NULL ) strncpy( sqlquery, molequery.query, MAX_SQLQUERY_LEN );


	/* Allocate connection handle */
	ret = SQLAllocHandle( SQL_HANDLE_DBC, odbcEnv, &conn );
	if( ( ret != SQL_SUCCESS ) && ( ret != SQL_SUCCESS_WITH_INFO ) )
		return 0;


	/* Set timeout */
	SQLSetConnectAttr( conn, SQL_LOGIN_TIMEOUT, ( SQLPOINTER* ) 5, 0 );


	/* Connect to data source */
	ret = SQLConnect( 
		conn, 
		( SQLCHAR* ) datasource, 
		SQL_NTS,
		( SQLCHAR* ) username, 
		SQL_NTS, 
		( SQLCHAR* ) password, 
		SQL_NTS );
	if( ( ret != SQL_SUCCESS ) && ( ret != SQL_SUCCESS_WITH_INFO ) )
	{
	 // Get the status records.
	   j = 1;
	   while ((rc2 = SQLGetDiagRec(SQL_HANDLE_DBC, conn, j, SqlState, &NativeError,
				Msg, sizeof(Msg), &MsgLen)) != SQL_NO_DATA) {
		  printf("%s",datasource); printf("%s",username); printf("%s", NativeError);
		  k=0;
		  while(k < sizeof(Msg)) {
			  if (Msg[k] != '\0')
				printf("%c",Msg[k]);
			  k++;
		  }
		  printf("\nSqlState=");
		  k=0;
		  while(k < sizeof(SqlState)) {
			  if (SqlState[k] != '\0')
				  printf("%c",SqlState[k]);
			  k++;
		  }

		  j++;
	   }
		SQLFreeHandle( SQL_HANDLE_DBC, conn );
		return 0;
	}

	/* Allocate statement handle */
	ret = SQLAllocHandle( SQL_HANDLE_STMT, conn, &stmt );
	if( ( ret != SQL_SUCCESS ) && ( ret != SQL_SUCCESS_WITH_INFO ) )
	{
		SQLDisconnect( conn );
		SQLFreeHandle( SQL_HANDLE_DBC, conn );
		return -1;
	}

	/* Make first SQL request for main table of events */
	ret = SQLExecDirect( stmt, molequery.query, SQL_NTS );
	if( ( ret != SQL_SUCCESS ) && ( ret != SQL_SUCCESS_WITH_INFO ) )
	{
		SQLFreeHandle( SQL_HANDLE_STMT, stmt );
		SQLDisconnect( conn );
		SQLFreeHandle( SQL_HANDLE_DBC, conn );
		return 0;
	}


	/* Get number of returned rows
	 ******************************/
	ret = SQLRowCount( stmt, &nrows );
	if( Debug ) printf( "Retrieved %d rows.\n", ( int ) nrows );
	if( ( ( int ) nrows ) < 1 )
	{
		SQLFreeHandle( SQL_HANDLE_STMT, stmt );
		SQLDisconnect( conn );
		SQLFreeHandle( SQL_HANDLE_DBC, conn );
		return 0;
	}

	/* Start printing out QuakeML */
	bufoffset = snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
			"<q:quakeml xmlns:q=\"http://quakeml.org/xmlns/quakeml/1.2\""
			" xmlns=\"http://quakeml.org/xmlns/bed/1.2\">" );

	bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
			"<eventParameters publicID=\"%s\">%s",
			evtParamPubID, QML_LINETERM );


	/* Bind data */
	//TODO: Column numbers are static. They should be dynamic!
	//TODO: Check if bindings are OK!
	ret = SQLBindCol( stmt, 2, SQL_C_ULONG, &fk_sqkseq,
			( SQLINTEGER ) sizeof( SQLUINTEGER ), &indicator );
	ret = SQLBindCol( stmt, 3, SQL_C_ULONG, &ewsqkid,
			( SQLINTEGER ) sizeof( SQLUINTEGER ), &indicator );
	ret = SQLBindCol( stmt, 4, SQL_C_ULONG, &version,
			( SQLINTEGER ) sizeof( SQLUINTEGER ), &indicator );
	ret = SQLBindCol( stmt, 7, SQL_C_TYPE_TIMESTAMP, &ot_dt,
			( SQLINTEGER ) sizeof( TIMESTAMP_STRUCT ), &indicator );
	ret = SQLBindCol( stmt, 8, SQL_C_ULONG, &otusec,
			( SQLINTEGER ) sizeof( SQLUINTEGER ), &indicator );
	ret = SQLBindCol( stmt, 9, SQL_C_DOUBLE, &lat,
			( SQLINTEGER ) sizeof( SQLDOUBLE ), &indicator );
	ret = SQLBindCol( stmt, 10, SQL_C_DOUBLE, &lon,
			( SQLINTEGER ) sizeof( SQLDOUBLE ), &indicator );
	ret = SQLBindCol( stmt, 11, SQL_C_DOUBLE, &depth,
			( SQLINTEGER ) sizeof( SQLDOUBLE ), &indicator );
	ret = SQLBindCol( stmt, 20, SQL_C_ULONG, &magmodule,
			( SQLINTEGER ) sizeof( SQLUINTEGER ), &indicator );
	ret = SQLBindCol( stmt, 21, SQL_C_DOUBLE, &mag,
			( SQLINTEGER ) sizeof( SQLDOUBLE ), &magindicator );
	ret = SQLBindCol( stmt, 24, SQL_C_CHAR, &magtype,
			( SQLINTEGER ) sizeof( magtype ), &magindicator );


	/* Cycle to fetch data on each row
	 **********************************/
	while(1)
	{
		/* Get data from a row
		 **********************/
		ret = SQLFetch( stmt );
		if( ret == SQL_NO_DATA )
			break;

		/* Check if this is a new event */
		if( ewsqkid != lastewsqkid )
		{
			/* This is a new event. Finish stuff of last */
			if( lastewsqkid != 0 )
			{
				/* Print magnitudes of the event */
				if( molequery.includeallmagnitudes )
					bufoffset += magreq( buffer + bufoffset,
							getspace( nbuffer, bufoffset ),
							conn, dbname, lastewsqkid, last_fk_sqkseq );

				/* Print phases of the event */
				if( molequery.includearrivals )
					bufoffset += phasereq( buffer + bufoffset,
							getspace( nbuffer, bufoffset ),
							conn, dbname,
							lastewsqkid, last_fk_sqkseq );

				/* Terminate event */
				bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
						"</event>%s", QML_LINETERM );
			}


			/* The first origin is the preferred! */

			/* Event start */
			bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
					"<event publicID=\"%d\">%s",
					( int ) ewsqkid, QML_LINETERM );

			/* type: TODO: This version only supports earthquakes */
			bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
					"<type>earthquake</type>%s", QML_LINETERM );

			/* TODO: Description */

			/* preferredOriginID */
			getOriginId( originID, 256, ewsqkid, version );
			bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
					"<preferredOriginID>%s</preferredOriginID>%s",
					originID, QML_LINETERM );

			/* preferredMagnitudeID */
			if( magindicator != -1 )
			{
				getMagnitudeId( magnitudeID, 256, magmodule, originID );
				bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
					"<preferredMagnitudeID>%s</preferredMagnitudeID>%s",
							magnitudeID, QML_LINETERM );
			}
		}

		/* Print this origin */
		getOriginId( originID, 256, ewsqkid, version );
		bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
				"<origin publicID=\"%s\">%s",
				originID, QML_LINETERM );

		/* Time */
		bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
				"<time><value>%4d-%02d-%02dT"
				"%02d:%02d:%02d.%06d</value></time>%s",
				ot_dt.year, ot_dt.month, ot_dt.day, ot_dt.hour, ot_dt.minute,
				ot_dt.second, ( int ) otusec, QML_LINETERM );

		/* Location */
		bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
				"<latitude>%.3f</latitude>%s"
				"<longitude>%.3f</longitude>%s<depth>%.1f</depth>%s",
				lat, QML_LINETERM, lon, QML_LINETERM, depth * 1000,
				QML_LINETERM );

		/* creationInfo */
		bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
				"<creationInfo>"
				"<version>%d</version>"
				"</creationInfo>%s",
				( int ) version, QML_LINETERM );

		/* Terminate Origin */
		bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
				"</origin>%s", QML_LINETERM );



		/* Print this Magnitude */
		if( !molequery.includeallmagnitudes && magindicator != -1 )
		{
			getMagnitudeId( magnitudeID, 256, magmodule, originID );
			bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
					"<magnitude publicID=\"%s\">%s",
					magnitudeID, QML_LINETERM );
			bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
					"<originID>%s</originID>%s",
					originID, QML_LINETERM );
			bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
					"<type>%s</type>%s", magtype, QML_LINETERM );
			bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
					"<mag><value>%.1f</value></mag>%s",
					mag, QML_LINETERM );
			bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
					"</magnitude>%s", QML_LINETERM );
		}


		/* Update last quake id */
		lastewsqkid = ewsqkid;
		last_fk_sqkseq = fk_sqkseq;
	}

	/* Print magnitudes of the last event */
	if( molequery.includeallmagnitudes )
		bufoffset += magreq( buffer + bufoffset, getspace( nbuffer, bufoffset ),
				conn, dbname, lastewsqkid, fk_sqkseq );

	/* Print phases of the last event */
	if( molequery.includearrivals )
		bufoffset += phasereq( buffer + bufoffset, getspace( nbuffer, bufoffset ),
				conn, dbname, lastewsqkid, fk_sqkseq );

	/* Terminate last event */
	bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
			"</event>%s", QML_LINETERM );

	/* Terminate event parameters */
	bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
			"</eventParameters>%s", QML_LINETERM );

	/* Terminate quakeML */
	bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
			"</q:quakeml>%s", QML_LINETERM );




	/* Free resourcess */
	SQLFreeHandle( SQL_HANDLE_STMT, stmt );
	SQLDisconnect( conn );
	SQLFreeHandle( SQL_HANDLE_DBC, conn );


	return bufoffset;
}






/******************************************************************************
 * Query and print data on magnitudes for a given origin                      *
 ******************************************************************************/
int magreq( char* buffer, int nbuffer, SQLHDBC conn, char* dbname,
		SQLUINTEGER qkseq, SQLUINTEGER fk_sqkseq )
{
	SQLHSTMT		stmt;			/* Handle for SQL statements */
	long			ret;			/* Return indicator */
	char			query[1025];		/* SQL phases query */
#ifdef __x86_64__
	SQLLEN			nrows;
#else
	SQLINTEGER		nrows;			/* Number of returned rows */
#endif
	char			originID[257];
	char			magnitudeID[257];
	long			indicator;		/* Data indicator for SQL */
	int				bufoffset = 0;

	/* Variables bound to query results */
	SQLUINTEGER		magmodule, version;
	SQLDOUBLE			mag;
	SQLCHAR				magtype[33];
	long				magindicator;

	/* Allocate statement handle */
	ret = SQLAllocHandle( SQL_HANDLE_STMT, conn, &stmt );
	if( ( ret != SQL_SUCCESS ) && ( ret != SQL_SUCCESS_WITH_INFO ) )
	{
		SQLDisconnect( conn );
		SQLFreeHandle( SQL_HANDLE_DBC, conn );
		return 0;
	}


	/* Create SQL Query */
	snprintf( query, 1024,
			"SELECT * FROM %s.%s "
			"WHERE %s=%d",
			dbname, MAG_TABLE,
			EVT_FID, ( int ) fk_sqkseq );


	if( Debug ) printf( "Query:\n%s\n", query );

	ret = SQLExecDirect( stmt, query, SQL_NTS );
	if( ( ret != SQL_SUCCESS ) && ( ret != SQL_SUCCESS_WITH_INFO ) )
	{
		SQLFreeHandle( SQL_HANDLE_STMT, stmt );
		return 0;
	}

	/* Get number of returned rows
	 ******************************/
	ret = SQLRowCount( stmt, &nrows );
	if( Debug ) printf( "Retrieved %d rows.\n", ( int ) nrows );
	if( ( ( int ) nrows ) < 1 )
	{
		SQLFreeHandle( SQL_HANDLE_STMT, stmt );
		return 0;
	}

	/* Bind columns */
	ret = SQLBindCol( stmt, 2, SQL_C_ULONG, &magmodule,
			( SQLINTEGER ) sizeof( SQLUINTEGER ), &indicator );
	ret = SQLBindCol( stmt, 4, SQL_C_ULONG, &version,
			( SQLINTEGER ) sizeof( SQLUINTEGER ), &indicator );
	ret = SQLBindCol( stmt, 5, SQL_C_DOUBLE, &mag,
			( SQLINTEGER ) sizeof( SQLDOUBLE ), &magindicator );
	ret = SQLBindCol( stmt, 15, SQL_C_CHAR, &magtype,
			( SQLINTEGER ) sizeof( magtype ), &magindicator );

	/* Print data */
	while(1)
	{
		/* Get data from a row
		 **********************/
		ret = SQLFetch( stmt );
		if( ret == SQL_NO_DATA )
			break;

		/* Get Origin Id */
		getOriginId( originID, 256, qkseq, version );

		/* Retrieve magnitude Id */
		getMagnitudeId( magnitudeID, 256, magmodule, originID );


		/* Start printing magnitude */
		bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
				"<magnitude publicID=\"%s\">%s",
				magnitudeID, QML_LINETERM );

		/* Corresponding origin */
		bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
				"<originID>%s</originID>%s",
				originID, QML_LINETERM );

		/* Magnitude type */
		bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
				"<type>%s</type>%s", magtype, QML_LINETERM );

		/* Magnitude value */
		bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
				"<mag><value>%.1f</value></mag>%s",
				mag, QML_LINETERM );

		/* Terminate magnitude */
		bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
				"</magnitude>%s", QML_LINETERM );
	}

	return bufoffset;
}







/******************************************************************************
 * Query and print data on phases for a given origin                          *
 ******************************************************************************/
int phasereq( char* buffer, int nbuffer, SQLHDBC conn, char* dbname,
		SQLUINTEGER qkseq, SQLUINTEGER fk_sqkseq )
{
	SQLHSTMT		stmt;			/* Handle for SQL statements */
	long			ret;			/* Return indicator */
	char			query[2049];		/* SQL phases query */
	int				bufoffset = 0;
#ifdef __x86_64__
	SQLLEN			nrows;
#else
	SQLINTEGER		nrows;			/* Number of returned rows */
#endif

	/* Variables bound to query results */
	SQLUINTEGER			pickid, pick_wt;
	SQLCHAR				Ponset[33], Sonset[33], Plabel[33], Slabel[33];
	SQLCHAR				sta[9], cha[9], net[9], loc[9],pick_fm[2];
	long				isP, isS;
	TIMESTAMP_STRUCT	tpick;
	SQLUINTEGER			tpick_usec;
	SQLDOUBLE			Pres;
	TIMESTAMP_STRUCT	Sat_dt;
	SQLUINTEGER			Sat_usec;
	SQLDOUBLE			Sres;
	SQLDOUBLE			azm, dist;
	long				indicator;

	/* Allocate statement handle */
	ret = SQLAllocHandle( SQL_HANDLE_STMT, conn, &stmt );
	if( ( ret != SQL_SUCCESS ) && ( ret != SQL_SUCCESS_WITH_INFO ) )
	{
		SQLDisconnect( conn );
		SQLFreeHandle( SQL_HANDLE_DBC, conn );
		return 0;
	}

	/* Make SQL request for table of phases
	 * This is a messy query because it combines data from
	 * the Link, Picks and Phases tables to be able to assoaciate
	 * each phase with a pick or at least the nearest pick
	 *************************************************************/
	snprintf( query, 2048,
			"SELECT ss.%s, %s.%s.* FROM "
			"%s.%s INNER JOIN( "
			"SELECT %s.%s.* "
			"FROM %s.%s INNER JOIN %s.%s "
			"ON %s.%s.%s = %s.%s.%s "
			"WHERE %s.%s.%s = %d) ss "
			"ON %s.%s.%s = %d "
			"AND ( "
			"ROUND( UNIX_TIMESTAMP(ss.%s)+ss.%s/1000000, 2 ) = "
			"ROUND( UNIX_TIMESTAMP(%s.%s.%s)+%s.%s.%s/1000000, 2 ) "
			"OR "
			"ROUND( UNIX_TIMESTAMP(ss.%s)+ss.%s/1000000, 2 ) = "
			"ROUND( UNIX_TIMESTAMP(%s.%s.%s)+%s.%s.%s/1000000, 2 ) "
			") "
			"AND ss.%s = %s.%s.%s",
			PIK_ID, dbname, PHA_TABLE,
			dbname, PHA_TABLE,
			dbname, PIK_TABLE,
			dbname, LNK_TABLE, dbname, PIK_TABLE,
			dbname, LNK_TABLE, PIK_ID, dbname, PIK_TABLE, PIK_ID,
			dbname, LNK_TABLE, EVT_FID, ( int ) fk_sqkseq,
			dbname, PHA_TABLE, EVT_FID, ( int ) fk_sqkseq,
			PIK_DT, PIK_USEC,
			dbname, PHA_TABLE, PHA_PT, dbname, PHA_TABLE, PHA_PUSEC,
			PIK_DT, PIK_USEC,
			dbname, PHA_TABLE, PHA_ST, dbname, PHA_TABLE, PHA_SUSEC,
			PIK_SCNL, dbname, PHA_TABLE, PIK_SCNL );


	if( Debug ) printf( "Query:\n%s\n", query );

	ret = SQLExecDirect( stmt, query, SQL_NTS );
	if( ( ret != SQL_SUCCESS ) && ( ret != SQL_SUCCESS_WITH_INFO ) )
	{
		SQLFreeHandle( SQL_HANDLE_STMT, stmt );
		return 0;
	}

	/* Get number of returned rows
	 ******************************/
	ret = SQLRowCount( stmt, &nrows );
	if( Debug ) printf( "Retrieved %d rows.\n", ( int ) nrows );
	if( ( ( int ) nrows ) < 1 )
	{
		SQLFreeHandle( SQL_HANDLE_STMT, stmt );
		return 0;
	}

	/* Bind columns */
	ret = SQLBindCol( stmt, 1, SQL_C_ULONG, &pickid,
			( SQLINTEGER ) sizeof( SQLUINTEGER ), &indicator );
	ret = SQLBindCol( stmt, 8, SQL_C_CHAR, &Ponset,
			( SQLINTEGER ) sizeof( Ponset ), &isP );
	ret = SQLBindCol( stmt, 9, SQL_C_CHAR, &Sonset,
			( SQLINTEGER ) sizeof( Sonset ), &isS );
	ret = SQLBindCol( stmt, 6, SQL_C_CHAR, &Plabel,
			( SQLINTEGER ) sizeof( Ponset ), &isP );
	ret = SQLBindCol( stmt, 7, SQL_C_CHAR, &Slabel,
			( SQLINTEGER ) sizeof( Sonset ), &isS );
	ret = SQLBindCol( stmt, 24, SQL_C_DOUBLE, &azm,
			( SQLINTEGER ) sizeof( SQLDOUBLE ), &indicator );
	ret = SQLBindCol( stmt, 26, SQL_C_DOUBLE, &dist,
			( SQLINTEGER ) sizeof( SQLDOUBLE ), &indicator );

	/* Cycle to fetch data on each row
	 **********************************/
	while(1)
	{
		/* Get data from a row
		 **********************/
		ret = SQLFetch( stmt );
		if( ret == SQL_NO_DATA )
			break;

		/* Print arrival data */
		bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
				"<arrival publicID=\"%d\">%s",
				( int ) pickid, QML_LINETERM );
		if( Ponset[0] == 'P' )
		{
			/* Phase */
			bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
					"<phase>P%s</phase>%s", Plabel, QML_LINETERM );
		}
		else
		{
			/* Phase */
			bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
					"<phase>S%s</phase>%s", Slabel, QML_LINETERM );
		}

		/* Azimuth */
		bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
				"<azimuth>%.1f</azimuth>%s", azm, QML_LINETERM );
		/* Distance */
		bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
				"<distance>%.2f</distance>%s", dist, QML_LINETERM );
		/* Pick ID */
		bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
				"<pickID>%d</pickID>",
				( int ) pickid, QML_LINETERM );

		/* Terminate arrival */
		bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
				"</arrival>%s", QML_LINETERM );
	}


	/* Query for picks
	 * This combines the links and picks table to find the picks
	 * associated with an event. Note that it may include rejected
	 * picks that are not associated with phases
	 *************************************************************/

	/* Re-initiate the stmt */
	SQLFreeHandle( SQL_HANDLE_STMT, stmt );
	ret = SQLAllocHandle( SQL_HANDLE_STMT, conn, &stmt );
	if( ( ret != SQL_SUCCESS ) && ( ret != SQL_SUCCESS_WITH_INFO ) )
	{
		SQLDisconnect( conn );
		SQLFreeHandle( SQL_HANDLE_DBC, conn );
		return bufoffset;
	}

	/* Prepare query */
	snprintf( query, 2048,
			"SELECT %s.%s.%s, %s.%s.%s, %s.%s.%s, %s.%s.%s, %s.%s.%s, %s.%s.* "
			"FROM %s.%s "
			"INNER JOIN %s.%s "
			"ON %s.%s.%s = %s.%s.%s "
			"INNER JOIN %s.%s "
			"ON %s.%s.%s = %s.%s.%s "
			"WHERE %s.%s.fk_sqkseq = %d",
			dbname, STA_TABLE, STA_STA, dbname, STA_TABLE, STA_CHA,
			dbname, STA_TABLE, STA_NET, dbname, STA_TABLE, STA_LOC,
			dbname, LNK_TABLE, PIK_ID,
			dbname, PIK_TABLE,
			dbname, LNK_TABLE,
			dbname, PIK_TABLE,
			dbname, LNK_TABLE, PIK_ID, dbname, PIK_TABLE, PIK_ID,
			dbname, STA_TABLE,
			dbname, PIK_TABLE, PIK_SCNL, dbname, STA_TABLE, STA_ID,
			dbname, LNK_TABLE, ( int ) fk_sqkseq );

	if( Debug ) printf( "Query:\n%s\n", query );

	ret = SQLExecDirect( stmt, query, SQL_NTS );
	if( ( ret != SQL_SUCCESS ) && ( ret != SQL_SUCCESS_WITH_INFO ) )
	{
		SQLFreeHandle( SQL_HANDLE_STMT, stmt );
		if( Debug ) fprintf( stderr, "Error with pick query\n" );
		fflush( stderr );
		return bufoffset;
	}

	/* Get number of returned rows
	 ******************************/
	ret = SQLRowCount( stmt, &nrows );
	if( Debug ) printf( "Retrieved %d rows.\n", ( int ) nrows );
	if( ( ( int ) nrows ) < 1 )
	{
		SQLFreeHandle( SQL_HANDLE_STMT, stmt );
		return bufoffset;
	}

	/* Bind columns */
	ret = SQLBindCol( stmt, 1, SQL_C_CHAR, &sta,
			( SQLINTEGER ) sizeof( sta ), &indicator );
	ret = SQLBindCol( stmt, 2, SQL_C_CHAR, &cha,
			( SQLINTEGER ) sizeof( cha ), &indicator );
	ret = SQLBindCol( stmt, 3, SQL_C_CHAR, &net,
			( SQLINTEGER ) sizeof( net ), &indicator );
	ret = SQLBindCol( stmt, 4, SQL_C_CHAR, &loc,
			( SQLINTEGER ) sizeof( loc ), &indicator );
	ret = SQLBindCol( stmt, 5, SQL_C_ULONG, &pickid,
			( SQLINTEGER ) sizeof( SQLUINTEGER ), &indicator );
	ret = SQLBindCol( stmt, 10, SQL_C_CHAR, &pick_fm,
			( SQLINTEGER ) sizeof( pick_fm ), &indicator );
	ret = SQLBindCol( stmt, 11, SQL_C_ULONG, &pick_wt,
			( SQLINTEGER ) sizeof( SQLUINTEGER ), &indicator );
	ret = SQLBindCol( stmt, 12, SQL_C_TYPE_TIMESTAMP, &tpick,
			( SQLINTEGER ) sizeof( TIMESTAMP_STRUCT ), &indicator );
	ret = SQLBindCol( stmt, 13, SQL_C_ULONG, &tpick_usec,
			( SQLINTEGER ) sizeof( SQLUINTEGER ), &indicator );

	/* Print quakeML */
	while(1)
	{
		/* Get data from a row
		 **********************/
		ret = SQLFetch( stmt );
		if( ret == SQL_NO_DATA )
			break;

		/* Print arrival data */
		bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
				"<pick publicID=\"%d\">%s",
				( int ) pickid, QML_LINETERM );
                if (pick_fm[0] == 'U') 
		    bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
				"<polarity>positive</polarity>%s", QML_LINETERM );
                if (pick_fm[0] == 'D') 
		    bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
				"<polarity>negative</polarity>%s", QML_LINETERM );

		/* Time */
		bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
				"<time><value>%4d-%02d-%02dT"
				"%02d:%02d:%02d.%06d</value><uncertainty>%d</uncertainty></time>%s",
				tpick.year, tpick.month, tpick.day, tpick.hour, tpick.minute,
				tpick.second, ( int ) tpick_usec, pick_wt, QML_LINETERM );

		/* SCNL */
		bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
				"<waveformID networkCode=\"%s\" stationCode=\"%s\" "
				"channelCode=\"%s\" locationCode=\"%s\"/>%s",
				net, sta, cha, loc, QML_LINETERM );
		/* Terminate pick */
		bufoffset += snprintf( buffer + bufoffset, getspace( nbuffer, bufoffset ),
				"</pick>%s", QML_LINETERM );
	}

	/* Free statement handle */
	SQLFreeHandle( SQL_HANDLE_STMT, stmt );

	return bufoffset;
}



size_t getspace( int a, int b )
{
	int c = a - b;
	if( c > 0 ) return ( size_t ) c;
	return 0;
}








/******************************************************************************
 * Generate ids                                                               *
 ******************************************************************************/
void getOriginId( char* dest, int ndest, SQLUINTEGER ewsqkid,
		SQLUINTEGER version )
{
	snprintf( dest, ndest, "%d_%d",
			( int ) ewsqkid, ( int ) version );
}

void getMagnitudeId( char* dest, int ndest, SQLUINTEGER magmodID,
		char* originID )
{
	snprintf( dest, ndest, "%s_%d",
			originID, ( int ) magmodID );
}




