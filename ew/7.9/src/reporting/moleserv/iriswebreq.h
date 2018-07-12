/*
Configuration of public IDs

All items in QuakeML have public IDs. It is necessary to associate the IDs
from EW messages to IDs in QuakeML. This document specifies how to do that with
the moleserver.

<eventParameters>
ID is unique, fully customizable from moleserver parameters

<event>
Hypocenter ID from the hypocenters summary table.
publicID = <qkseq>
Preferred origin and preferred magnitudes assume the origin with the highest
version number

<origin>
ID for each origin. Assuming that each event may have several version, the
event ID is constructed from the arc message id and version as:
<qkseq>_<version>

<magnitude>
ID for each magnitude. Magnitudes can be computed with different modules. MDs
are computed from the hypo module whereas ML comes from the localmag module.
As such, the magnitude ID must come from the module ID that generated it as:
<qkseq>_<version>_<mag_fk_module>

<arrival> and <pick>
The arrivals and picks will share the same id, corresponding to the pick seq.
number.
*/

/* Defines
 **********/
#define MAX_SQLQUERY_LEN		2049
#define EVNT_REQ				1
#define PICK_REQ				2
#define SQL_DBNAME_LEN			27
#define SQL_TIMEDATE_LEN		27
#define SQL_FLOAT_LEN			33
#define SQL_MAGTYPE_LEN			17
#define SQL_NUM_LEN				17
#define SQL_ORDER_LEN			17
#define SQL_BOOLEAN_LEN			9
#define SQL_EVENTID_LEN			33

/* Names of properties of the Mole DB
 *************************************/
#define	EVT_TABLE		"ew_hypocenters_summary"	// Events table
#define	EVT_OT			"ot_dt"						// Origin time
#define	EVT_LAT			"lat"						// Latitude
#define	EVT_LON			"lon"						// Longitude
#define	EVT_MAG			"mag"						// Magnitude
#define	EVT_MAGTYPE		"mag_type"					// Mag. type
#define	EVT_ID			"qkseq"						// Event ID
#define EVT_FID			"fk_sqkseq"					// Another event ID
#define	EVT_VER			"version"					// Event version
#define	EVT_LUPD		"modified"					// Last update

#define	PHA_TABLE		"ew_arc_phase"				// Phases table
#define	PHA_PT			"Pat_dt"					// P time
#define	PHA_PUSEC		"Pat_usec"					// P microseconds
#define	PHA_ST			"Sat_dt"					// S time
#define	PHA_SUSEC		"Sat_usec"					// S microseconds

#define	PIK_TABLE		"ew_pick_scnl"				// Picks table
#define	PIK_DT			"tpick_dt"					// Pick time
#define PIK_USEC		"tpick_usec"				// Pick microseconds
#define	PIK_ID			"fk_spkseq"					// Pick ID
#define PIK_SCNL		"fk_scnl"					// Station

#define LNK_TABLE		"ew_link"					// Links table

#define	STA_TABLE		"ew_scnl"					// Stations table
#define	STA_ID			"id"						// Staiton ID
#define	STA_STA			"sta"						// Station
#define STA_CHA			"cha"						// Channel
#define	STA_NET			"net"						// Network
#define	STA_LOC			"loc"						// Location

#define	MAG_TABLE		"ew_magnitude_summary"		// Magnitudes table

/* Enumerations
 ***************/
typedef enum
{
	IRISQ_OK,
	IRISQ_QUERY_TOO_LONG,
	IRISQ_OUT_OF_MEMORY,
	IRISQ_INVALID_PARAM
} PARSE_STATUS;

typedef enum
{
	IRISQ_TRUE,
	IRISQ_FALSE
} IRISQ_BOL;

typedef enum
{
	IRISQ_EVENTS,
	IRISQ_MAGNITUDES,
	IRISQ_PHASES,
	IRISQ_PICKS
} IRISQ_QTYPE;

/* Types
 ********/
typedef struct _MOLEREQ
{
	char		query[MAX_SQLQUERY_LEN];		/* SQL query string */
	int			includeallmagnitudes;
	int			includearrivals;
} MOLEREQ;

typedef struct _IRISWEBREQ
{
	char		starttime[SQL_TIMEDATE_LEN + 1];
	char		endtime[SQL_TIMEDATE_LEN + 1];
	char		minlatitude[SQL_FLOAT_LEN + 1];
	char		maxlatitude[SQL_FLOAT_LEN + 1];
	char		minlongitude[SQL_FLOAT_LEN + 1];
	char		maxlongitude[SQL_FLOAT_LEN + 1];
	char		mindepth[SQL_FLOAT_LEN + 1];
	char		maxdepth[SQL_FLOAT_LEN + 1];
	char		minmag[SQL_FLOAT_LEN + 1];
	char		maxmag[SQL_FLOAT_LEN + 1];
	char		magtype[SQL_MAGTYPE_LEN + 1];
	char		limit[SQL_NUM_LEN + 1];
	char		offset[SQL_NUM_LEN + 1];
	char		orderby[SQL_ORDER_LEN + 1];
	char		updateafter[SQL_TIMEDATE_LEN + 1];
	IRISQ_BOL	includeallorigins;
	IRISQ_BOL	includeallmagnitudes;
	IRISQ_BOL	includearrivals;
	char		eventid[SQL_EVENTID_LEN + 1];
} IRISWEBREQ;

/* Functions
 ************/

/* Initialize ODBC environment */
int setODBCenv( void );

/* Terminate ODBC environment */
void clearODBCenv( void );

/* Convert web request in QuakeML */
int molereq( char* buffer, int nbuffer, char* webrequest, char* dbname, char* evtParamPubID,
		char* datasource, char* username, char* password, char* sqlquery );
//int molereq( FILE* dest, char* webrequest, char* dbname, char* evtParamPubID,
//		char* datasource, char* username, char* password );

















