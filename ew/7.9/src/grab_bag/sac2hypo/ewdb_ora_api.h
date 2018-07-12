#ifndef EWDB_ORA_API_H
# define EWDB_ORA_API_H




/**********************************************************
 #########################################################
    INCLUDE Section
 #########################################################
**********************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <rw_strongmotionII.h>

/* Needed for TRACE_REQ structure */
#include <ws_clientII.h>


/**********************************************************
 #########################################################
    EWDB_API_LIB  Comment Section
 #########################################################
**********************************************************/

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW API FORMATTED COMMENT
TYPE LIBRARY

LIBRARY  EWDB_API_LIB

LOCATION THIS_FILE

DESCRIPTION The EWDB_API_LIB is the C API that 
provides access to the Earthworm Database system.
The API provides read and write access to the 
EW DBMS.  It provides a means for application
developers to access the data in the DBMS without
having to learn how to interact with Oracle. 
<br><br>
All access to the Earthworm DBMS must 
be done via this API. This is done in order to 
limit both the code that writes data to the DBMS, and
the amount of code that interacts with the internal structure
of the DBMS.  
<br><br>
Currently, the only API Available is in C, so to interact with
the DBMS programatically you must have a way of calling C 
functions. 
<br><br>
Unless otherwise specified, all time measurements are assumed to
be seconds since 1/1/1970.
<br><br>

*************************************************
************************************************/


/**********************************************************
 #########################################################
    Constant Section
 #########################################################
**********************************************************/

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE DEFINE 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY UTILITY

LANGUAGE C

LOCATION THIS_FILE

CONSTANT_GROUP True/False constants

CONSTANT TRUE
VALUE 1

CONSTANT FALSE
VALUE 0

WARNING!!!!!!!!!!
DO NOT CHANGE THESE VALUES.  THEY ARE COPIED FROM
ewdb_cli_base.h!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

*************************************************
************************************************/
#define TRUE 1
#define FALSE 0


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE DEFINE 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY UTILITY

LANGUAGE C

LOCATION THIS_FILE

CONSTANT_GROUP EWDB Return Codes:

CONSTANT EWDB_RETURN_SUCCESS
VALUE 0
DESCRIPTION Value returned by an EWDB function that
executes successfully. 

CONSTANT EWDB_RETURN_FAILURE
VALUE -1
DESCRIPTION Value returned by an EWDB function that
fails.

CONSTANT EWDB_RETURN_WARNING
VALUE 1
DESCRIPTION Value returned by an EWDB function that experiences
non-fatal problems during execution.  Please see individual function
comments for a description of what this code means for each function.

WARNING!!!!!!!!!!
DO NOT CHANGE THESE VALUES.  THEY ARE COPIED FROM
ewdb_cli_base.h!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

*************************************************
************************************************/
#define EWDB_RETURN_SUCCESS 0
#define EWDB_RETURN_FAILURE -1
#define EWDB_RETURN_WARNING 1


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE DEFINE 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY UTILITY

LANGUAGE C

LOCATION THIS_FILE

CONSTANT_GROUP EWDB Debug Levels

CONSTANT EWDB_DEBUG_DB_BASE_NONE
VALUE 0
DESCRIPTION Log NO EWDB debugging information.

CONSTANT EWDB_DEBUG_DB_BASE_CONNECT_INFO
VALUE 1
DESCRIPTION Log DB connect/disconnect information.

CONSTANT EWDB_DEBUG_DB_BASE_STATEMENT_PARSE_INFO
VALUE 2
DESCRIPTION Log SQL statement parsing information.

CONSTANT EWDB_DEBUG_DB_BASE_FUNCTION_ENTRY_INFO
VALUE 4
DESCRIPTION Log entry/exit of DB Call level functions.

CONSTANT EWDB_DEBUG_DB_API_FUNCTION_ENTRY_INFO
VALUE 8
DESCRIPTION Log entry/exit of EWDB API level functions.

CONSTANT EWDB_DEBUG_DB_BASE_ALL
VALUE -1
DESCRIPTION Log All available debugging information for
the EWDB API.

WARNING!!!!!!!!!!
DO NOT CHANGE THESE VALUES.  THEY ARE COPIED FROM
ewdb_cli_base.h!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

*************************************************
************************************************/
#define EWDB_DEBUG_DB_BASE_NONE                  0
#define EWDB_DEBUG_DB_BASE_CONNECT_INFO          1
#define EWDB_DEBUG_DB_BASE_STATEMENT_PARSE_INFO  2
#define EWDB_DEBUG_DB_BASE_FUNCTION_ENTRY_INFO   4
#define EWDB_DEBUG_DB_API_FUNCTION_ENTRY_INFO    8
#define EWDB_DEBUG_DB_BASE_ALL                  -1


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE DEFINE 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY UTILITY

LANGUAGE C

LOCATION THIS_FILE

CONSTANT_GROUP Criteria Selection Flags

Criteria selection flags, used in determining what criteria to use when
selecting a list of events or other objects.

CONSTANT EWDB_CRITERIA_USE_TIME
VALUE 1
DESCRIPTION Flag used to indicate whether or not a time criteria should
be used in selecting a list of objects.

CONSTANT EWDB_CRITERIA_USE_LAT
VALUE 2
DESCRIPTION Flag used to indicate whether or not
a lattitude criteria should be used.

CONSTANT EWDB_CRITERIA_USE_LON
VALUE 4
DESCRIPTION Flag used to indicate whether or not
a longitude criteria should be used.

CONSTANT EWDB_CRITERIA_USE_DEPTH
VALUE 8
DESCRIPTION Flag used to indicate whether or not
a depth criteria should be used.

CONSTANT EWDB_CRITERIA_USE_SCNL
VALUE 0x10
DESCRIPTION Flag used to indicate whether or not
a channel(SCNL) criteria should be used.

CONSTANT EWDB_CRITERIA_USE_IDEVENT
VALUE 0x20
DESCRIPTION Flag used to indicate whether or not
an Event(using idEvent) criteria should be used.

CONSTANT EWDB_CRITERIA_USE_IDCHAN
VALUE 0x40
DESCRIPTION Flag used to indicate whether or not
a Channel(using idChan) criteria should be used.

*************************************************
************************************************/
#define EWDB_CRITERIA_USE_TIME        1
#define EWDB_CRITERIA_USE_LAT         2
#define EWDB_CRITERIA_USE_LON         4
#define EWDB_CRITERIA_USE_DEPTH       8
#define EWDB_CRITERIA_USE_SCNL     0x10
#define EWDB_CRITERIA_USE_IDEVENT  0x20
#define EWDB_CRITERIA_USE_IDCHAN   0x40


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE DEFINE 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY UTILITY

LANGUAGE C

LOCATION THIS_FILE

CONSTANT_GROUP EWDB Time constants

These define the max/min time values and thus the
allowable time range.  Absolute times are seconds since 1970.

CONSTANT EWDB_MAX_TIME
VALUE 9999999999.9999
DESCRIPTION Maximum time value supported for EWDB times.
Times are seconds since 1970.

CONSTANT EWDB_MIN_TIME
VALUE 0
DESCRIPTION Minimum time value supported for EWDB times.
Times are seconds since 1970.

*************************************************
************************************************/
#define EWDB_MAX_TIME 9999999999.9999
#define EWDB_MIN_TIME 0


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE DEFINE 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE

CONSTANT_GROUP Parametric Misc. Data Constants

CONSTANT EWDB_MAXIMUM_AMPS_PER_CODA
VALUE 6
DESCRIPTION Maximum number of coda average amplitude values per coda.
6 is derived from the maximum number of coda average amplitude windows
tracked by Earthworm(Pick_EW).

*************************************************
************************************************/
#define EWDB_MAXIMUM_AMPS_PER_CODA 6

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE DEFINE 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE

CONSTANT_GROUP Event Types

ala Quake, Blast, Mudslide?

CONSTANT EWDB_EVENT_TYPE_UNKNOWN
VALUE 0
DESCRIPTION Flag indicating that the type of a DB Event is unknown.

CONSTANT EWDB_EVENT_TYPE_QUAKE
VALUE 2
DESCRIPTION Flag indicating that a DB Event is a Quake

CONSTANT EWDB_EVENT_TYPE_COINCIDENCE
VALUE 3
DESCRIPTION Flag indicating that a DB Event is a Coincidence Trigger

*************************************************
************************************************/
#define EWDB_EVENT_TYPE_COINCIDENCE 3
#define EWDB_EVENT_TYPE_QUAKE 2
#define EWDB_EVENT_TYPE_UNKNOWN 0

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE DEFINE 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE

CONSTANT_GROUP Event Update Types


CONSTANT EWDB_UPDATE_EVENT_NONE
VALUE 0
DESCRIPTION Perform no update.

CONSTANT EWDB_UPDATE_EVENT_EVENTTYPE
VALUE 1
DESCRIPTION Update the EventType of the Event.

CONSTANT EWDB_UPDATE_EVENT_DUBIOCITY
VALUE 2
DESCRIPTION Update the Dubiocity of the Event.

CONSTANT EWDB_UPDATE_EVENT_ARCHIVED
VALUE 4
DESCRIPTION Update the "Archived" status of the Event.

*************************************************
************************************************/
#define EWDB_UPDATE_EVENT_NONE 0
#define EWDB_UPDATE_EVENT_EVENTTYPE 1
#define EWDB_UPDATE_EVENT_DUBIOCITY 2
#define EWDB_UPDATE_EVENT_ARCHIVED  4




/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE DEFINE 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE

CONSTANT_GROUP Event Update Types


CONSTANT EWDB_UNASSOCIATED_DATA_NONE
VALUE 0
DESCRIPTION Perform no update.

CONSTANT EWDB_UNASSOCIATED_DATA_PICKS
VALUE 1
DESCRIPTION Update the EventType of the Event.

CONSTANT EWDB_UNASSOCIATED_DATA_WAVEFORMS
VALUE 2
DESCRIPTION Update the Dubiocity of the Event.

CONSTANT EWDB_UNASSOCIATED_DATA_PEAKAMPS
VALUE 4
DESCRIPTION Update the "Archived" status of the Event.

CONSTANT EWDB_UNASSOCIATED_DATA_SMMESSAGES
VALUE 8
DESCRIPTION Update the "Archived" status of the Event.

*************************************************
************************************************/
#define EWDB_UNASSOCIATED_DATA_NONE       0
#define EWDB_UNASSOCIATED_DATA_PICKS      1
#define EWDB_UNASSOCIATED_DATA_WAVEFORMS  2
#define EWDB_UNASSOCIATED_DATA_PEAKAMPS   4
#define EWDB_UNASSOCIATED_DATA_SMMESSAGES 8


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW API FORMATTED COMMENT
TYPE LIBRARY

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM_API

LOCATION THIS_FILE

DESCRIPTION This is the Waveform portion
of the EWDB_API_LIB library.  It provides access to
binary waveform data in the Earthworm DB.

*************************************************
************************************************/

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE DEFINE 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM_API

LANGUAGE C

LOCATION THIS_FILE

CONSTANT_GROUP Waveform Data Formats

CONSTANT EWDB_WAVEFORM_FORMAT_UNDEFINED
VALUE 0
DESCRIPTION Indicates that the data format of a waveform
snippet is unknown.

CONSTANT EWDB_WAVEFORM_FORMAT_EW_TRACE_BUF
VALUE 1
DESCRIPTION Indicates that a waveform snippet is of type Earthworm
Trace Buf.  Please see Earthworm documentation of TRACE_BUF messages
for more info.

*************************************************
************************************************/
#define EWDB_WAVEFORM_FORMAT_UNDEFINED 0
#define EWDB_WAVEFORM_FORMAT_EW_TRACE_BUF 1


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE DEFINE 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY COOKED_INFRASTRUCTURE_API

LANGUAGE C

LOCATION THIS_FILE

CONSTANT_GROUP Poles and Zeroes Constants


CONSTANT EWDB_MAX_POLES_OR_ZEROES
VALUE 100
DESCRIPTION Maximum number of poles or zeroes
allowed for a transfer function

CONSTANT EWDB_PZTYPE_POLE
VALUE 1
DESCRIPTION Flag indicating that a Pole/Zero is a Pole.

CONSTANT EWDB_PZTYPE_ZERO
VALUE 0
DESCRIPTION Flag indicating that a Pole/Zero is a Zero.

*************************************************
************************************************/
#define EWDB_MAX_POLES_OR_ZEROES 100
#define EWDB_PZTYPE_POLE 1
#define EWDB_PZTYPE_ZERO 0


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE DEFINE 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY STRONG_MOTION_API

LANGUAGE C

LOCATION THIS_FILE

CONSTANT_GROUP Strong Motion Message Search Codes

CONSTANT EWDB_SM_SEARCH_UNDEFINED
VALUE 0
DESCRIPTION Code for no type of search defined.  
This is not supported by any functions, and 
exists to prevent 0(the default value) from 
being used as a meaningful code.

CONSTANT EWDB_SM_SEARCH_FOR_ALL_SMMESSAGES
VALUE 1
DESCRIPTION Code that indicates that all strong
motion messages should be retrieved, regardless of
whether they are associated with an event or not.

CONSTANT EWDB_SM_SEARCH_FOR_ALL_UNASSOCIATED_MESSAGES
VALUE 2
DESCRIPTION Code that indicates that only strong
motion messages that are not already associated 
with an event, should be retrieved.

*************************************************
************************************************/
#define EWDB_SM_SEARCH_UNDEFINED                      0
#define EWDB_SM_SEARCH_FOR_ALL_SMMESSAGES             1
#define EWDB_SM_SEARCH_FOR_ALL_UNASSOCIATED_MESSAGES  2


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE DEFINE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

CONSTANT_GROUP Utility constants for the alarm system

CONSTANT EWDB_ALARMS_MAX_RULES_PER_RECIPIENT
VALUE 20
DESCRIPTION Maximum number of rules that a recipient 
can have.

CONSTANT EWDB_ALARMS_MAX_RECIPIENT_DELIVERIES
VALUE 20
DESCRIPTION Maximum number of deliveries that a recipient 
can have.

CONSTANT EWDB_ALARMS_MAX_FORMAT_LEN
VALUE 4000
DESCRIPTION Largest (in bytes) format string.

*************************************************
************************************************/
#define 	EWDB_ALARMS_MAX_RULES_PER_RECIPIENT    	 20
#define 	EWDB_ALARMS_MAX_RECIPIENT_DELIVERIES     20
#define 	EWDB_ALARMS_MAX_FORMAT_LEN 		   	 	4000


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE DEFINE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

CONSTANT_GROUP Delivery Mechanisms

CONSTANT EWDB_ALARMS_DELIVERY_NUM_MECHANISMS
VALUE 5
DESCRIPTION Number of defined delivery mechanisms.

CONSTANT EWDB_ALARMS_DELIVERY_IND_EMAIL
VALUE 0
DESCRIPTION Index of the email delivery mechanism.

CONSTANT EWDB_ALARMS_DELIVERY_IND_PAGER
VALUE 1
DESCRIPTION Index of the pager delivery mechanism.

CONSTANT EWDB_ALARMS_DELIVERY_IND_PHONE
VALUE 2
DESCRIPTION Index of the phone delivery mechanism.

CONSTANT EWDB_ALARMS_DELIVERY_IND_QDDS
VALUE 3
DESCRIPTION Index of the qdds delivery mechanism.

CONSTANT EWDB_ALARMS_DELIVERY_IND_CUSTOM
VALUE 4
DESCRIPTION Index of the custom delivery mechanism.

*************************************************
************************************************/
#define	    EWDB_ALARMS_DELIVERY_NUM_MECHANISMS 5
#define     EWDB_ALARMS_DELIVERY_IND_EMAIL      0
#define     EWDB_ALARMS_DELIVERY_IND_PAGER  		1
#define     EWDB_ALARMS_DELIVERY_IND_PHONE      2
#define     EWDB_ALARMS_DELIVERY_IND_QDDS       3
#define     EWDB_ALARMS_DELIVERY_IND_CUSTOM     4



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE DEFINE 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY RAW_INFRASTRUCTURE_API

LANGUAGE C

LOCATION THIS_FILE

CONSTANT_GROUP Raw Infrastructure Misc. Data Constants

CONSTANT EWDB_RAW_INFRA_NAME_LEN
VALUE 40
DESCRIPTION Standard name length for raw infrastructure applications,
such as DeviceName, SlotName, and SlotTypeName.

*************************************************
************************************************/
#define EWDB_RAW_INFRA_NAME_LEN 40


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE DEFINE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM_REQUEST_API

LANGUAGE C

LOCATION THIS_FILE

CONSTANT_GROUP Attempt Param Flags

Attempt Parameters and Scheduling of Snippet-Retrieval Attempts

The concierge system resolves around the concept of having a
background process periodically attempt to retrieve trace from
wave_servers.  There exists within the concierge, a scheduling
system, that schedules requests for retrieval.  The requests
are scheduled based upon a scheduling algorithm, as well as
feedback from the process that is trying to retrieve the data.
These "attempt params" flags, allow the retrieval process
to provide feedback to the scheduling logic that runs within
the database.
There are currently 3 flags:
Flag 1:  Do not update.
This flag says, "I made a retrieval attempt, and for some
reason, I don't want to record that retrieval attempt, 
so just pretend (for scheduling purposes) that there was 
no attempt."  For the most part, this flag will only be used
when a SPECIAL attempt was made, that was not prompted by
the scheduler.  Maybe the retrieval process decided on its
own to attempt to retrieve data, and it doesn't want to interfere
with the official attempt schedule.

Flag 2:  Update and reschedule.
This flag says, "I made an attempt, record that I attempted,
and schedule the next attempt, based upon the scheduling logic."
This would be the standard flag to use when an attempt failed.
It says that you tried, but had no luck, and so it should be 
scheduled for another try.

Flag 3:  Update only the next attempt time.
This flag says, "I made an attempt, but for some reason, I don't
want to record that attempt, and I want to attempt again at
the time I specify."  This flag could be used if you made an attempt
and got a partial response, and you want to attempt again in the
near future, instead of waiting for whenever the next 
scheduling-algorithm based attempt.

CONSTANT EWDB_WAVEFORM_REQUEST_DO_NOT_MODIFY_ATTEMPT_PARAMS
VALUE 0
DESCRIPTION Do not modify attempt params at all

CONSTANT EWDB_WAVEFORM_REQUEST_RECORD_ATTEMPT_AND_UPDATE_PARAMS
VALUE 1
DESCRIPTION Record the current attempt (adjust the number of
attempts remaining), and schedule the next attempt based upon
the scheduling algorithm.

CONSTANT EWDB_WAVEFORM_REQUEST_UPDATE_NEXT_ATTEMPT
VALUE 2
DESCRIPTION Do not record the current attempt.  Schedule the
next attempt at the time given by tNow(calculated by the API
call), plus tAttemptInterval seconds, as specified in the 
SnippetRequestStruct by the application.


*************************************************
************************************************/
#define EWDB_WAVEFORM_REQUEST_DO_NOT_MODIFY_ATTEMPT_PARAMS 0
#define EWDB_WAVEFORM_REQUEST_RECORD_ATTEMPT_AND_UPDATE_PARAMS 1
#define EWDB_WAVEFORM_REQUEST_UPDATE_NEXT_ATTEMPT 2

/**********************************************************
 #########################################################
    Typedef & Struct Section
 #########################################################
**********************************************************/


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY UTILITY

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDBid
TYPE_DEFINITION int
DESCRIPTION The C format of a EWDB Database ID.

NOTE Last seen, EWDBids were capable of being larger than an integer,
and this could create problems for the storing of EWDBids in C.  There
is a move to change the type from int to either 64-bit int or string.
DavidK 05/01/00  This should not cause problems with operations on
existing EW5 databases, but in the existing format, it does prevent
existing nodes from coexisting in a tightly coupled manner.

*************************************************
************************************************/
typedef int EWDBid;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY COOKED_INFRASTRUCTURE_API

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDB_StationStruct
TYPE_DEFINITION struct _EWDB_StationStruct
DESCRIPTION Structure that provides information on seismic station components.
Specifically it describes the name, location, and orientation of the 
sensor component for a channel.  
ComponentStruct would have been a better name than StationStruct
since there may be multiple components at a Site/Station, and
each component may have a different location/orientation.

MEMBER idChan
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION The Database ID of the Channel.

MEMBER idComp
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION The Database ID of the Component.

MEMBER Sta
MEMBER_TYPE char[10]
MEMBER_DESCRIPTION The Station code of the SCNL 
for the Component.

MEMBER Comp
MEMBER_TYPE char[10]
MEMBER_DESCRIPTION The Channel/Component code of the SCNL for the
Component.

MEMBER Net
MEMBER_TYPE char[10]
MEMBER_DESCRIPTION The Network code of the SCNL for the Component.

MEMBER Loc
MEMBER_TYPE char[10]
MEMBER_DESCRIPTION The Location code of the SCNL for the Component.

MEMBER Lat
MEMBER_TYPE float
MEMBER_DESCRIPTION The lattitude of the Component.  Expressed in
Degrees.

MEMBER Lon
MEMBER_TYPE float
MEMBER_DESCRIPTION The longitude of the Component.  Expressed in
Degrees.

MEMBER Elev
MEMBER_TYPE float
MEMBER_DESCRIPTION The elevation of the Component above sea level.
Expressed in meters.

MEMBER Azm
MEMBER_TYPE float
MEMBER_DESCRIPTION The horizontal orientation of the Component.
Expressed in degrees clockwise of North.

MEMBER Dip
MEMBER_TYPE float
MEMBER_DESCRIPTION The vertical orientation of the Component.
Expressed in degrees below horizontal.

*************************************************
************************************************/
typedef struct _EWDB_StationStruct
{
  EWDBid  idChan;
  EWDBid  idComp;
  char    Sta[10];
  char    Comp[10];
  char    Net[10];
  char    Loc[10];
  float   Lat; 
  float   Lon; 
  float   Elev;
  float   Azm; 
  float   Dip; 
} EWDB_StationStruct;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDB_ArrivalStruct
TYPE_DEFINITION struct _EWDB_ArrivalStruct
DESCRIPTION Arrival structure describing information about a phase,
possibly with information relating the phase to an Origin.

MEMBER idPick
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID of the Pick.

MEMBER idOriginPick
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID of the OriginPick, the record relating
the Pick(phase) to an Origin.

MEMBER idOrigin
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID of the Origin with which this phase is
associated.

MEMBER idChan
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID of the channel that this pick came
from.

MEMBER pStation
MEMBER_TYPE EWDB_StationStruct
MEMBER_DESCRIPTION Station/Component information for the channel
associated with this phase.

NOTE The fields szCalcPhase - tResPick are only legitimate if a phase
is associated with an Origin.  If idOrigin is not set then these feels
should be ignored.

MEMBER szCalcPhase
MEMBER_TYPE char[6]
MEMBER_DESCRIPTION The calculated type of the Phase: P, S, PKP(sp?),
and others up to 5 chars.  This values is assigned by the phase
associator or locator that created the Origin that this phase is
associated with.  This is not the phase type given by the picker.

MEMBER tCalcPhase
MEMBER_TYPE double
MEMBER_DESCRIPTION The onset time of the phase, according to the
associator/locator.

MEMBER dWeight
MEMBER_TYPE float
MEMBER_DESCRIPTION The weight of the phase pick in calculating the
Origin.

MEMBER dWeight
MEMBER_TYPE float
MEMBER_DESCRIPTION The weight of the phase pick in calculating the
Origin.

MEMBER dDist
MEMBER_TYPE float
MEMBER_DESCRIPTION The epicentral distance to the station where the phase
was observed.

MEMBER dAzm
MEMBER_TYPE float
MEMBER_DESCRIPTION Azimuthal direction from the station where the phase
was observed to the epicenter.  Expressed as degrees east of North.

MEMBER dTakeoff
MEMBER_TYPE float
MEMBER_DESCRIPTION Takeoff angle of the Phase from the hypocenter.

MEMBER tResPick
MEMBER_TYPE float
MEMBER_DESCRIPTION Pick residual in seconds.

MEMBER szExtSource
MEMBER_TYPE char[16]
MEMBER_DESCRIPTION Reserved

MEMBER szExternalPickID
MEMBER_TYPE char[16]
MEMBER_DESCRIPTION Reserved

MEMBER szExtPickTabName
MEMBER_TYPE char[16]
MEMBER_DESCRIPTION Reserved

MEMBER szObsPhase
MEMBER_TYPE char[6]
MEMBER_DESCRIPTION The type of the Phase:  P, S, PKP(sp?), and others
up to 5 chars.  This values is assigned by the picker(the entity that
observed the phase arrival).

MEMBER tObsPhase
MEMBER_TYPE double
MEMBER_DESCRIPTION The onset time of the phase, according to the
picker.

MEMBER cMotion
MEMBER_TYPE char
MEMBER_DESCRIPTION The initial motion of the
phase: U (up), D (down).  

MEMBER cOnset
MEMBER_TYPE char
MEMBER_DESCRIPTION The onset type of the phase:  CLEANUP??? what are
the possible onset values?  Berkeley has several in their
documentation.

MEMBER dSigma
MEMBER_TYPE double
MEMBER_DESCRIPTION Potential Phase error.  Expressed in seconds.
CLEANUP detailed meaning?

*************************************************
************************************************/
typedef struct _EWDB_ArrivalStruct
{    
	EWDBid  idPick;
	EWDBid  idOriginPick;
	EWDBid  idOrigin;
	EWDBid  idChan;
  EWDB_StationStruct * pStation;
  char    szCalcPhase[6]; /* name of phase */
  double  tCalcPhase;  /* time of phase */
  float   dWeight;   /* weight of pick in Origin calc */
  float   dDist;     /* distance(km) from epicenter */
  float   dAzm;      /* Azimuth(deg from N.) to epicenter */
  float   dTakeoff;  /* Takeoff angle of incoming waves */
  double  tResPick;  /* Pick residual in seconds */
  char    szExtSource[16];  /* Source that produced this pick */
  char    szExternalPickID[16];  /* Source's Pick ID */
  char    szExtPickTabName[16];  /* Source's Pick Table */
  char    szObsPhase[6]; /* name of phase */
  double  tObsPhase;  /* time of phase */
  char    cMotion; /* first motion U,D,X */
  char    cOnset;  /* how steep the first wave was? */
  double  dSigma;  /* quality of the pick sigma interval(secs) */
} EWDB_ArrivalStruct;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDB_CodaAmplitudeStruct
TYPE_DEFINITION struct _EWDB_CodaAmplitudeStruct
DESCRIPTION Information for average amplitude measurments of a
coda in support of a coda termination measurement/calculation.

MEMBER tOn
MEMBER_TYPE double[EWDB_MAXIMUM_AMPS_PER_CODA]
MEMBER_DESCRIPTION Array of window starting times for coda average
amplitude measurements.  Expressed as seconds since 1970.

MEMBER tOff
MEMBER_TYPE double[EWDB_MAXIMUM_AMPS_PER_CODA]
MEMBER_DESCRIPTION Array of window ending times for coda average
amplitude measurements.  Expressed as seconds since 1970.

MEMBER iAvgAmp
MEMBER_TYPE int[EWDB_MAXIMUM_AMPS_PER_CODA]
MEMBER_DESCRIPTION Array of average amplitude values for coda average
amplitude measurements.  Expressed as digital counts.  tOn[x] and
tOff[x] describe a time window during a coda at a component. AvgAmp[x]
is the average wave amplitude during that window.  A series of timed
average amplitude measurements can be combined with a decay algorithm
to estimate the coda termination time.

*************************************************
************************************************/
typedef struct _EWDB_CodaAmplitudeStruct
{
  /* AmpCoda part */
  double  tOn[EWDB_MAXIMUM_AMPS_PER_CODA];
  double  tOff[EWDB_MAXIMUM_AMPS_PER_CODA];
  int     iAvgAmp[EWDB_MAXIMUM_AMPS_PER_CODA];
}  EWDB_CodaAmplitudeStruct;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE

TYPEDEF EWDB_PeakAmpStruct
TYPE_DEFINITION struct _EWDB_PeakAmpStruct
DESCRIPTION Information for the Peak Amplitude measurement
associated with a station magnitude calculation.

MEMBER idPeakAmp
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION DB ID of the Peak Amplitude record for this amplitude
structure.

MEMBER idChan
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID of the channel that this pick came
from.

MEMBER szExtSource
MEMBER_TYPE char[16]
MEMBER_DESCRIPTION Source where this amplitude was produced

MEMBER szExternalAmpID
MEMBER_TYPE char[16]
MEMBER_DESCRIPTION ID of the amplitude at the source

MEMBER dAmp1
MEMBER_TYPE float
MEMBER_DESCRIPTION 1st Amplitude measurement(millimeters).

MEMBER dAmpPeriod1
MEMBER_TYPE float
MEMBER_DESCRIPTION Time period of the first measurement.

MEMBER tAmp1
MEMBER_TYPE double
MEMBER_DESCRIPTION Starting time of the first amplitude measurement.  
Expressed as seconds since 1970.

MEMBER dAmp2
MEMBER_TYPE float
MEMBER_DESCRIPTION 2nd Amplitude measurement(millimeters).

MEMBER dAmpPeriod2
MEMBER_TYPE float
MEMBER_DESCRIPTION Time period of the second measurement.

MEMBER tAmp2
MEMBER_TYPE double
MEMBER_DESCRIPTION Starting time of the second amplitude measurement.  
Expressed as seconds since 1970.

MEMBER iAmpType
MEMBER_TYPE int
MEMBER_DESCRIPTION Type of amplitude.  


*************************************************
************************************************/
typedef struct _EWDB_PeakAmpStruct
{
  /* PeakAmp part */
  EWDBid   idPeakAmp;  /* same as idDatum */
  EWDBid   idChan;

  char    szExtSource[16];  		/* Source that produced this amp */
  char    szExternalAmpID[16];  	/* Source's amplitude ID */
 
  float   dAmp1;
  float   dAmpPeriod1;
  double  tAmp1;
  float   dAmp2;
  float   dAmpPeriod2;
  double  tAmp2;
  int     iAmpType;
} EWDB_PeakAmpStruct;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDB_CodaDurationStruct
TYPE_DEFINITION struct _EWDB_CodaDurationStruct
DESCRIPTION Information for the Coda Duration measurement
associated with a station magnitude calculation.

MEMBER idChan
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID of the channel that this pick came
from.

MEMBER idTCoda
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION DB ID of the coda termination record.

MEMBER tCodaTermObs
MEMBER_TYPE double
MEMBER_DESCRIPTION Time when the coda associated with this station
duration magnitude, was observed to have ended.  Time is expressed as
seconds since 1970.  *NOTE*: In general, this value should not be used.  
It is provided as a reference in case tCodaTermXtp contains an extrapolated
value.  The value of tCodaTermXtp should always be a better value.  When
recalculating a coda termination time, the results should be written to
tCodaTermXtp.)

MEMBER tCodaTermXtp
MEMBER_TYPE double
MEMBER_DESCRIPTION Time when the coda associated with this station
duration magnitude ended.  This will be the better of either an
observed coda termination, or one calculated based on average amplitude
values and a decay rate.  Time is expressed as seconds since 1970.

Coda Duration

MEMBER tCodaDurObs
MEMBER_TYPE double
MEMBER_DESCRIPTION Length of the coda (in seconds) from the time of the
P-pick to the time of the OBSERVED coda termination.  This value should
not be used for the coda estimate, use tCodaDurXtp instead.  
See the tCodaTermObs comment.

MEMBER tCodaDurXtp
MEMBER_TYPE double
MEMBER_DESCRIPTION Length of the coda (in seconds) from the time of the
P-pick to the time of the coda termination.  This value is based upon
tCodaTermXtp and P-pick times.

MEMBER idCodaDur
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION DB ID of the coda duration record calculated by
associated a Phase pick with a coda termination.

MEMBER idPick
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION DB ID of the Phase arrival(pick) that is associated
with a Coda termination from the same channel, to produce the coda
duration(s) that this station magniutde is calculated from.

MEMBER pCodaAmp
MEMBER_TYPE EWDB_CodaAmplitudeStruct *
MEMBER_DESCRIPTION Pointer to a structure of Coda Average
Amplitude Values, used to calculate the coda termination
time.

*************************************************
************************************************/
typedef struct _EWDB_CodaDurationStruct
{

  EWDBid   idChan;

  /* TCoda part */
  EWDBid   idTCoda;
  double   tCodaTermObs;  /* IGNORE */
  double   tCodaTermXtp; 

  double   tCodaDurObs;   /* IGNORE */
  double   tCodaDurXtp;

  /* CodaDur part */
  EWDBid  idCodaDur;  /* same as idDatum */
  EWDBid  idPick;

  /* Coda Amplitude Information for extrapolated codas */
  EWDB_CodaAmplitudeStruct * pCodaAmp;

}  EWDB_CodaDurationStruct;



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDB_StationMagUnion
TYPE_DEFINITION union _EWDB_StationMagUnion
DESCRIPTION Structure of Station Magnitude information.

MEMBER PeakAmp
MEMBER_TYPE EWDB_PeakAmpStruct
MEMBER_DESCRIPTION Measurement information for an Amplitude
measurement.

MEMBER CodaDur
MEMBER_TYPE EWDB_CodaDurationStruct
MEMBER_DESCRIPTION Measurement information for a Coda
Duration measurement.

*************************************************
************************************************/
typedef union _EWDB_StationMagUnion
{
  EWDB_PeakAmpStruct PeakAmp;
  EWDB_CodaDurationStruct CodaDur;
}  EWDB_StationMagUnion;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDB_StationMagStruct
TYPE_DEFINITION struct _EWDB_StationMagStruct
DESCRIPTION Structure of Station Magnitude information.

MEMBER pStationInfo
MEMBER_TYPE EWDB_StationStruct *
MEMBER_DESCRIPTION Station information for the station for which
the magnitude was calculated.

MEMBER idChan
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID of the channel that this pick came
from.

MEMBER pStationInfo
MEMBER_TYPE EWDB_StationStruct
MEMBER_DESCRIPTION Station/Component information for the channel
associated with this phase.

MEMBER idMagLink
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION DB ID of the station magnitude.

MEMBER idMagnitude
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION DB ID of the summary magnitude.

MEMBER iMagType
MEMBER_TYPE int
MEMBER_DESCRIPTION Type of magnitude.  See 
rw_mag.h for a list of Magnitude types.

MEMBER dMag
MEMBER_TYPE float
MEMBER_DESCRIPTION Station magnitude.

MEMBER dWeight
MEMBER_TYPE float
MEMBER_DESCRIPTION Weight of the Station Magnitude in calculating the
summary magnitude this Station Magnitude is associated with.

MEMBER idDatum
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION DB ID of the measurement that the station magnitude
is calculated from.  The measurement could be a Peak Amplitude or Coda
Duration.

MEMBER StaMagUnion
MEMBER_TYPE EWDB_StationMagUnion
MEMBER_DESCRIPTION Information about the measurement upon which the 
magnitude is based.  This could be either Peak Amplitude information
for an Amplitude based magnitude, or Coda Duration information for
a Duration based magnitude.

NOTE: When both an observed(dMeasurementObs) and a
calculated(dMeasurementCalc) measurement exist the method for
determining which one the magnitude is derived from is undefined.

*************************************************
************************************************/
typedef struct _EWDB_StationMagStruct
{
  EWDBid  idChan;
  EWDB_StationStruct * pStationInfo;

  /* MagLink part */
  EWDBid  idMagLink;
  EWDBid  idMagnitude;
  int     iMagType;
  float   dMag;
  float   dWeight;
  EWDBid  idDatum;

  /* Datum Info */
  EWDB_StationMagUnion StaMagUnion;

} EWDB_StationMagStruct;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDB_EventStruct
TYPE_DEFINITION struct _EWDB_EventStruct
DESCRIPTION Structure containing information about a Database Event.

MEMBER idEvent
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID for the event

MEMBER iEventType
MEMBER_TYPE int
MEMBER_DESCRIPTION Human assigned event-type (quake, avalanche, 
volcano, nuclear detination).  Currently the only types supported
are Quake and Unknonwn.  See EWDB_EVENT_TYPE_QUAKE for more 
information.

MEMBER iDubiocity
MEMBER_TYPE int
MEMBER_DESCRIPTION Describes the perceived validity of the Event.  The
Dubiocity range is currently defined as TRUE/FALSE with TRUE indicating
a dubious event.

MEMBER bArchived
MEMBER_TYPE int
MEMBER_DESCRIPTION Indicates whether or not the Event has already been
archived from the database to disk.

MEMBER szComment
MEMBER_TYPE char[512]
MEMBER_DESCRIPTION Description of and comments about the Event.

MEMBER szSource
MEMBER_TYPE char[256]
MEMBER_DESCRIPTION Identifier of the source/author that declared the
Event.  Only one Source is listed here.  If more than one Source
contributed to the declaration of the Event, then the manner by which
this Source is chosen as THE Source is undefined.  This could be the
Logo of the Earthworm module that generated the Event, or the or the
login name of an analyst that reviewed it.

MEMBER szSourceName
MEMBER_TYPE char[256]
MEMBER_DESCRIPTION Name of the source/author that declared the Event.
The SourceName is a human readable version of the Source.  The
SourceName could be the name of the network that generated the event,
or the name of an Analyst that reviewed it.

MEMBER szSourceEventID
MEMBER_TYPE char[256]
MEMBER_DESCRIPTION Event identifer assigned to this event by the
Source.  This would be the EventID given by and Earthworm Binder Module
to an automatic event that was generated by an Earthworm.

*************************************************
************************************************/
typedef struct _EWDB_EventStruct
{
    EWDBid  idEvent; 
    int     iEventType; 
    int     iDubiocity;
    int     bArchived;
    char    szComment[512]; 
    char    szSource[256];  
    char    szSourceName[256];  
    char    szSourceEventID[256];
} EWDB_EventStruct;



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDB_MagStruct
TYPE_DEFINITION struct _EWDB_MagStruct
DESCRIPTION Summary magnitude struct.

MEMBER idMag
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID for the Magnitude.

MEMBER iMagType
MEMBER_TYPE int
MEMBER_DESCRIPTION Magnitude type: Duration, Local, BodyAmp, Surface, etc.
This is an enumerated value that identifies a magnitude type.  The possible
enumerated values are defined in rw_mag.h.

MEMBER dMagAvg
MEMBER_TYPE float
MEMBER_DESCRIPTION The summary magnitude.  For most magnitude types, this
is computed as a weighted average of a bunch of station magnitudes.

MEMBER iNumMags
MEMBER_TYPE int
MEMBER_DESCRIPTION Number of station magnitudes used to compute the
dMagAvg.  For a Moment magnitude, this maybe the number of reporting
stations that were used in calculating the moment.

MEMBER dMagErr
MEMBER_TYPE float
MEMBER_DESCRIPTION The potential error of the summary magnitude.
CLEANUP!  Detailed units, meaning?  I believe this is the Median
Absolute Deviation of the station mags.

MEMBER idEvent
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database identifier of the Event with which this
Magnitude is associated.

MEMBER bBindToEvent
MEMBER_TYPE int
MEMBER_DESCRIPTION Boolean flag indicating whether or not this
Magnitude should be bound to an Event when it is inserted into the DB.
If yes, it is associated with the Event identified by idEvent upon
insertion into the DB.

MEMBER bSetPreferred
MEMBER_TYPE int
MEMBER_DESCRIPTION Boolean flag indicating whether or not this
Magnitude should be set as the Preferred Magnitude for the Event
identified by idEvent.  This flag is ignored if bBindToEvent is FALSE.

MEMBER idOrigin
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Optional Database identifier of the Origin 
that this Magnitude was calculated from.

MEMBER szSource
MEMBER_TYPE char[256]
MEMBER_DESCRIPTION Identifier of the source/author that declared the
Origin.  This could be the Logo of the Earthworm module that generated
the Origin, or the login name of an analyst that reviewed it.  CLEANUP
Should this be the human readable version of the source name??  It
should be consistent with Origin and Mechanism.

MEMBER szComment
MEMBER_TYPE char *
MEMBER_DESCRIPTION Optional comment associated with the magnitude.
This is provided primarily as an option for later development.  Most
functions that handle the EWDB_MagStruct, do not know/care about
comments.

*************************************************
************************************************/
typedef struct _EWDB_MagStruct
{
  EWDBid  idMag;          /* database ID for this magnitude */
	int     iMagType;       /* enumerated magnitude type.  See rw_mag.h for the 
                             list of possible values */
	float	  dMagAvg;			  /* summary magnitude value */
	int		  iNumMags;		    /* number of stations used to compute magAvg */
	float	  dMagErr;
	EWDBid  idEvent;		    /* database ID of the associated event */
	int		  bBindToEvent;	  /* should magnitude be bound to this event */
	int		  bSetPreferred;  /* is this the preferred magnitude */
  EWDBid  idOrigin;       /* database ID of the Origin upon which
                             this magnitude is based */
	char	  szSource[256];  /* Author of this magnitude */
  char *  szComment;      /* Optional comment for this magnitude.
                             PROBABLY NOT SUPPORTED */
} EWDB_MagStruct;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDB_OriginStruct
TYPE_DEFINITION struct _EWDB_OriginStruct
DESCRIPTION Structure containing information about an Event Origin.
Includes error information.

MEMBER idOrigin
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database identifier of the Origin.

MEMBER idEvent
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database identifier of the Event with which this
origin is associated.

MEMBER BindToEvent
MEMBER_TYPE int
MEMBER_DESCRIPTION Boolean flag indicating whether or not this Origin
should be bound to an Event when it is inserted into the DB.  If yes,
it is associated with the Event identified by idEvent upon insertion
into the DB.

MEMBER SetPreferred
MEMBER_TYPE int
MEMBER_DESCRIPTION Boolean flag indicating whether or not this Origin
should be set as the Preferred Origin for the Event identified by
idEvent.  This flag is ignored if BindToEvent is FALSE.

MEMBER tOrigin
MEMBER_TYPE double
MEMBER_DESCRIPTION Origin time in seconds since 1970.

MEMBER dLat
MEMBER_TYPE float
MEMBER_DESCRIPTION Lattitude of the Origin.  Expressed in degrees.
(North=positive)

MEMBER dLon
MEMBER_TYPE float
MEMBER_DESCRIPTION Longitude of the Origin.  Expressed in degrees.
(East=positive)

MEMBER dDepth
MEMBER_TYPE float
MEMBER_DESCRIPTION Depth of the Origin.
Expressed in kilometers below surface.

MEMBER dLatStart
MEMBER_TYPE float
MEMBER_DESCRIPTION Lattitude of the starting location.  

MEMBER dLonStart
MEMBER_TYPE float
MEMBER_DESCRIPTION Longitude of the starting location. 

MEMBER dDepthStart
MEMBER_TYPE float
MEMBER_DESCRIPTION Depth of the starting location .

MEMBER dErrLat
MEMBER_TYPE float
MEMBER_DESCRIPTION Potential lattitude error of the Origin.  Expressed
in degrees.

MEMBER dErrLon
MEMBER_TYPE float
MEMBER_DESCRIPTION Potential longitude error of the Origin.  Expressed
in degrees.

MEMBER dErZ
MEMBER_TYPE float
MEMBER_DESCRIPTION Potential vertical(depth) error of the Origin.
Expressed in km.

MEMBER ExternalTableName
MEMBER_TYPE char[256]
MEMBER_DESCRIPTION Reserved

MEMBER xidExternal
MEMBER_TYPE char[256]
MEMBER_DESCRIPTION Reserved

MEMBER sRealSource
MEMBER_TYPE char[51]
MEMBER_DESCRIPTION Identifier of the source/author that declared the
Origin.  This could be the Logo of the Earthworm module that generated
the Origin, or the login name of an analyst that reviewed it.

MEMBER sSource
MEMBER_TYPE char[101]
MEMBER_DESCRIPTION Human readable name of the source/author of the origin.

MEMBER Comment
MEMBER_TYPE char[4000]
MEMBER_DESCRIPTION Comment about the Origin. Currently unused.

MEMBER iGap
MEMBER_TYPE int
MEMBER_DESCRIPTION Largest gap between stations used to compute the
origin.  The gap is the largest arc between stations, based on a
epicentral circle.  Expressed in degrees.

MEMBER dMin
MEMBER_TYPE float
MEMBER_DESCRIPTION Epicentral distance to nearest station used in
origin computation.

MEMBER dRms
MEMBER_TYPE float
MEMBER_DESCRIPTION RMS misfit for the origin.  What does RMS mean?
CLEANUP dk 05/01/00.

MEMBER iAssocRD
MEMBER_TYPE int
MEMBER_DESCRIPTION Number of readings associated with the Origin.

MEMBER iAssocPh
MEMBER_TYPE int
MEMBER_DESCRIPTION Number of phases associated with the Origin.

MEMBER iUsedRd
MEMBER_TYPE int
MEMBER_DESCRIPTION Number of readings used to calculate the Origin.

MEMBER iUsedPh
MEMBER_TYPE int
MEMBER_DESCRIPTION Number of phases used to calculate the Origin.

MEMBER iE0Azm
MEMBER_TYPE int
MEMBER_DESCRIPTION Azimuth of the smallest principal error of the error
elipse.  Expressed as Degrees east of North.

MEMBER iE1Azm
MEMBER_TYPE int
MEMBER_DESCRIPTION Azimuth of the intermediate principal error of the
error elipse.  Expressed as Degrees east of North.

MEMBER iE2Azm
MEMBER_TYPE int
MEMBER_DESCRIPTION Azimuth of the largest principal error of the error
elipse.  Expressed as Degrees east of North.

MEMBER iE0Dip
MEMBER_TYPE int
MEMBER_DESCRIPTION Dip of the smallest principal error of the error
elipse.  Expressed as Degrees below horizontal.

MEMBER iE1Dip
MEMBER_TYPE int
MEMBER_DESCRIPTION Dip of the intermediate principal error of the error
elipse.  Expressed as Degrees below horizontal.

MEMBER iE2Dip
MEMBER_TYPE int
MEMBER_DESCRIPTION Dip of the largest principal error of the error
elipse.  Expressed as Degrees below horizontal.

MEMBER dE0
MEMBER_TYPE float
MEMBER_DESCRIPTION Magnitude of smallest principle error of the error
elipse.

MEMBER dE1
MEMBER_TYPE float
MEMBER_DESCRIPTION Magnitude of intermediate principle error of the
error elipse.

MEMBER dE2
MEMBER_TYPE float
MEMBER_DESCRIPTION Magnitude of largest principle error of the error
elipse.

MEMBER dMCI
MEMBER_TYPE double
MEMBER_DESCRIPTION Confidence interval.  CLEANUP???  Help? Units,
detailed meaning?

MEMBER iFixedDepth
MEMBER_TYPE int
MEMBER_DESCRIPTION Flag indicating whether a fixed depth model was used
to calculate the Origin.

MEMBER iVersionNum
MEMBER_TYPE int
MEMBER_DESCRIPTION Integer > 0 indicating the version number of the current
origin.  The idea, is that external system will produce multiple revisions
of event solutions as time goes on.  Each new origin will have an increasing
version number assigned to it.  That way when you process origin data to
produce other summary data (such as magnitudes) you can associate it with
the correct origin version.

MEMBER szSourceEventID
MEMBER_TYPE char[256]
MEMBER_DESCRIPTION Event identifer assigned to this origin's event by the
Source.  This would be the EventID given by and Earthworm Binder Module
to an automatic event that was generated by an Earthworm.

*************************************************
************************************************/
typedef struct _EWDB_OriginStruct
{

	EWDBid    idOrigin;		/* database ID of the associated event */
	EWDBid    idEvent;		/* database ID of the associated event */
	int		  BindToEvent;	/* should origin be bound to this event */
	int		  SetPreferred;	/* is this the preferred origin */
	double	  tOrigin;		/* origin time */
	float	  dLat;
	float	  dLon;
	float	  dDepth;
	float	  dLatStart;
	float	  dLonStart;
	float	  dDepthStart;
	float	  dErrLat;
	float	  dErrLon;
	float	  dErZ;
	char	  ExternalTableName[256];
	char	  xidExternal[256];
	char	  sSource[101];		/* Human readable Source string */
	char	  sRealSource[51]; /* sSource in the Source table */
	char	  Comment[4000];
	int		  iGap;
	float	  dDmin;
	float	  dRms;
	int		  iAssocRd;
	int		  iAssocPh;
	int		  iUsedRd;
	int		  iUsedPh;
	int		  iE0Azm;
	int		  iE1Azm;
	int		  iE2Azm;
	int		  iE0Dip;
	int		  iE1Dip;
	int		  iE2Dip;
	float	  dE0;
	float	  dE1;
	float	  dE2;
	double	  dMCI;
	int		  iFixedDepth;
  int     iVersionNum;
  char    szSourceEventID[256];
} EWDB_OriginStruct;




/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDB_EventListStruct
TYPE_DEFINITION struct _EWDB_EventListStruct DESCRIPTION Structure to
provide summary information for one of a list of Events.

MEMBER Event
MEMBER_TYPE EWDB_EventStruct
MEMBER_DESCRIPTION Event summary information for the current event.

MEMBER dOT
MEMBER_TYPE double
MEMBER_DESCRIPTION Time of the Event's preferred origin of in seconds
since 1970.

MEMBER dLat
MEMBER_TYPE float
MEMBER_DESCRIPTION Latitude of the preferred Origin 
of the Event. Expressed in degrees.  (North=positive)

MEMBER dLon
MEMBER_TYPE float
MEMBER_DESCRIPTION Longitude of the preferred Origin 
of the Event. Expressed in degrees.  (East=positive)

MEMBER dDepth
MEMBER_TYPE float
MEMBER_DESCRIPTION Depth of the preferred Origin 
of the Event. Expressed in kilometers below surface.

MEMBER dPrefMag
MEMBER_TYPE float
MEMBER_DESCRIPTION preferred Magnitude of the Event.

NOTE There was overlap between EWDB_EventListStruct and
EWDB_EventStruct, so the overlapping fields from
EWDB_EventListStruct were removed and EWDB_EventStruct 
was incorporated.

*************************************************
************************************************/
typedef struct _EWDB_EventListStruct
{
    EWDB_EventStruct Event;
    EWDBid  idOrigin;             
    double  dOT;             
    float   dLat;           
    float   dLon;          
    float   dDepth;       
    float   dPrefMag;  
} EWDB_EventListStruct;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY EXTERNAL_API

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDB_External_StationStruct
TYPE_DEFINITION struct _EWDB_External_StationStruct DESCRIPTION
Structure for inserting station information into a side "Cheater Table"
area of the DB.

MEMBER Station
MEMBER_TYPE EWDB_StationStruct
MEMBER_DESCRIPTION Station information.

MEMBER StationID
MEMBER_TYPE int
MEMBER_DESCRIPTION The DB StationID of the current station.

MEMBER Description
MEMBER_TYPE char[100]
MEMBER_DESCRIPTION A text description of the station, usually
the long name of the station.


*************************************************
************************************************/
typedef struct _EWDB_External_StationStruct
{
  EWDB_StationStruct Station;
  int     StationID;
  char    Description[100];   /* station description (for nsn) */
} EWDB_External_StationStruct;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF

LIBRARY  EWDB_API_LIB

SUB_LIBRARY EXTERNAL_API

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDB_External_UH_InfraInfo
TYPE_DEFINITION struct _EWDB_External_UH_InfraInfo 
DESCRIPTION
Structure for inserting additional infrastructure information 
into a side table. This information is needed by the Urban
Hazards team. 

MEMBER idUHInfo
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION DB id of this record.

MEMBER idChanT
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION DB id of the channel to which this auxiliary
data belongs.

MEMBER dFullscale
MEMBER_TYPE double
MEMBER_DESCRIPTION fullscale.

MEMBER dSensitivity
MEMBER_TYPE double
MEMBER_DESCRIPTION sensitivity.

MEMBER dNaturalFrequency
MEMBER_TYPE double
MEMBER_DESCRIPTION natural frequency.

MEMBER dDamping
MEMBER_TYPE double
MEMBER_DESCRIPTION damping.

MEMBER dAzm
MEMBER_TYPE double
MEMBER_DESCRIPTION The horizontal orientation of the Component.
Expressed in degrees clockwise of North.

MEMBER dDip
MEMBER_TYPE double
MEMBER_DESCRIPTION The vertical orientation of the Component.
Expressed in degrees below horizontal.

MEMBER iGain
MEMBER_TYPE int
MEMBER_DESCRIPTION gain.

MEMBER iSensorType
MEMBER_TYPE int
MEMBER_DESCRIPTION numeric sensor type.

*************************************************
************************************************/
typedef struct _EWDB_External_UH_InfraInfo
{

	EWDBid			idUHInfo;
	EWDBid			idChanT;
	double 			dFullscale;
	double 			dSensitivity;
	double 			dNaturalFrequency;
	double			dDamping;
	double			dAzm;
	double			dDip;
	int				iGain;
	int				iSensorType;

} EWDB_External_UH_InfraInfo;




/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM_API

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDB_WaveformStruct
TYPE_DEFINITION struct _EWDB_WaveformStruct
DESCRIPTION Structure containing waveform information and a raw
waveform snippet.

MEMBER idWaveform
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION DB ID of the Waveform snippet.

MEMBER idChan
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID of the channel that this pick came
from.

MEMBER tStart
MEMBER_TYPE double
MEMBER_DESCRIPTION Beginning time of the waveform snippet.  Expressed
as seconds since 1970.

MEMBER tEnd
MEMBER_TYPE double
MEMBER_DESCRIPTION Ending time of the waveform snippet.  Expressed as
seconds since 1970.

MEMBER iDataFormat
MEMBER_TYPE int
MEMBER_DESCRIPTION Format of the actual waveform snippet data.  See
EWDB_WAVEFORM_FORMAT_UNDEFINED.

MEMBER iByteLen
MEMBER_TYPE int
MEMBER_DESCRIPTION Length of the actual waveform snippet data in
bytes.

MEMBER binSnippet
MEMBER_TYPE char *
MEMBER_DESCRIPTION Actual waveform snippet.  Its length is specified by
iByteLen and its format is specified by iDataFormat.  This is just a 
pointer.  Space for the waveform snippet must be dynamically allocated.

NOTE There should probably also be an iStorageMethod value that
indicates how the data is stored (in the DB, out of the DB as a file,
at an ftp site, etc.), but it has yet to be implemented.

*************************************************
************************************************/
typedef struct _EWDB_WaveformStruct
{
  EWDBid idWaveform;
  EWDBid idChan;
  double tStart;
  double tEnd;
  int    iDataFormat;
  int    iByteLen;
  char * binSnippet;
} EWDB_WaveformStruct;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM_API

LANGUAGE C

LOCATION THIS_FILE

TYPEDEF EWDB_SnippetStruct
TYPE_DEFINITION TRACE_REQ
DESCRIPTION Uh. I don't know.  We are using EWDB_SnippetStruct for a
Snippet data entry struct or something, and we are copying it from the
TRACE_REQ definition in the Wave_Server Client II routines
<ws_clientII.h>  Help!

*************************************************
************************************************/
typedef TRACE_REQ EWDB_SnippetStruct;  



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY COOKED_INFRASTRUCTURE_API

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDB_PZNum
TYPE_DEFINITION struct _EWDB_PZNum
DESCRIPTION Describes a Pole/Zero as a complex number.

MEMBER dReal
MEMBER_TYPE double
MEMBER_DESCRIPTION Real portion of this number.

MEMBER dImag
MEMBER_TYPE double
MEMBER_DESCRIPTION Imaginary portion of this number.

*************************************************
************************************************/
typedef struct _EWDB_PZNum
{
  double      dReal;
  double      dImag;
} EWDB_PZNum;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY COOKED_INFRASTRUCTURE_API

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDB_TransformFunctionStruct
TYPE_DEFINITION struct _EWDB_TransformFunctionStruct
DESCRIPTION Structure for describing Poles and Zeros based
transform/transfer functions.

MEMBER idCookedTF
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION DB ID of the transfer function.

MEMBER szCookedTFDesc
MEMBER_TYPE char[50]
MEMBER_DESCRIPTION Function name or description.

MEMBER iNumPoles
MEMBER_TYPE int
MEMBER_DESCRIPTION Number of Poles in the function.

MEMBER iNumZeroes
MEMBER_TYPE int
MEMBER_DESCRIPTION Number of Zeroes in the function.

MEMBER Poles
MEMBER_TYPE EWDB_PZNum [EWDB_MAX_POLES_OR_ZEROES]
MEMBER_DESCRIPTION Poles of the function.

MEMBER Zeroes
MEMBER_TYPE EWDB_PZNum [EWDB_MAX_POLES_OR_ZEROES]
MEMBER_DESCRIPTION Zeroes of the function.

*************************************************
************************************************/
typedef struct _EWDB_TransformFunctionStruct
{
  EWDBid      idCookedTF;
  char        szCookedTFDesc[50];
  int         iNumPoles;
  int         iNumZeroes;
  EWDB_PZNum  Poles[EWDB_MAX_POLES_OR_ZEROES];
  EWDB_PZNum  Zeroes[EWDB_MAX_POLES_OR_ZEROES];
} EWDB_TransformFunctionStruct;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY COOKED_INFRASTRUCTURE_API

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDB_ChanTCTFStruct
TYPE_DEFINITION struct _EWDB_ChanTCTFStruct
DESCRIPTION Structure for storing the Poles/Zeroes transfer function
for a channel/time range, including gain and samplerate.

MEMBER idChanT
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION DB ID of the channel validity range(ChanT) for which
the associated transform function is valid.

MEMBER dGain
MEMBER_TYPE double
MEMBER_DESCRIPTION Scalar gain of the channel for the ChanT.

MEMBER dSampRate
MEMBER_TYPE double
MEMBER_DESCRIPTION Digital sample rate of the waveforms exiting the
channel for the ChanT.

MEMBER tfsFunc
MEMBER_TYPE EWDB_TransformFunctionStruct
MEMBER_DESCRIPTION Poles and Zeroes transform function for the channel
time interval.

*************************************************
************************************************/
typedef struct _EWDB_ChanTCTFStruct
{
  EWDBid    idChanT;
  double    dGain;
  double    dSampRate;
  EWDB_TransformFunctionStruct tfsFunc;
} EWDB_ChanTCTFStruct;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM_REQUEST_API

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDB_SnippetRequestStruct
TYPE_DEFINITION struct _EWDB_SnippetRequestStruct
DESCRIPTION Structure for storing parameters for a waveform request.
This structure is used to store (in the database) a request for
waveform data.  Requests are presumably later gathered by a concierge
type program that recovers as many desired waveforms as possible.  This
structure describes a time interval of interest for which it is
interested in data from a particular channel.  It defines the channel
whose data it is interested in, the time interval for which it wants
data, how many attempts should be made to retrieve the data, how often
each attempt should be made, and what DB idEvent if any that the data
should be associated with when retrieved.

MEMBER idSnipReq
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION DB ID of the waveform request(SnipReq) that this
structure's data is associated with.

MEMBER idChan
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION DB ID of the sesimic data channel for which the
request wants data.

MEMBER tStart
MEMBER_TYPE double
MEMBER_DESCRIPTION Starting time of the desired time interval.

MEMBER tEnd
MEMBER_TYPE double
MEMBER_DESCRIPTION Ending time of the desired time interval.

MEMBER idEvent
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION DB ID of the Event with which the desired data
should be associated.

MEMBER iNumAttempts
MEMBER_TYPE int
MEMBER_DESCRIPTION Number of attempts that should be made to retrieve
the desired data.

MEMBER tNextAttempt
MEMBER_TYPE time_t
MEMBER_DESCRIPTION This field has a dual purpose.  When retrieving a
list of Snippet Requests from the DB, this field will contain the
time that the current snippet request is scheduled for a retrieval
attempt.  When used in the reverse manner (updating a snippet request),
the "data retriever"/scheduler should put the "time till next attempt"
in this field, meaning the time that the scheduler wants the request
to be attempted minus the current time.  (The deltaT).   The number
is done as a delta to ensure that updated requests are not scheduled 
for the past.

MEMBER idExistingWaveform
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION DB ID of a Waveform in the DB that contains partial
data for the original request.

MEMBER tInitialRequest
MEMBER_TYPE time_t
MEMBER_DESCRIPTION Time of the initial snippet request.  That is the
time that it is created in the DB.  Seconds since 1970.

MEMBER iRequestGroup
MEMBER_TYPE int
MEMBER_DESCRIPTION Request Group of the snippet request.  Requests can
be placed in a Request Group so that different requests can be handled
differently, or can be handled by separate data retrievers(like 
ora_trace_fetch).

MEMBER iNumAlreadyAtmptd
MEMBER_TYPE int
MEMBER_DESCRIPTION Number of times that a data retriever has already
attempted to retrieve the subject of the request.  It is up to the
retriever to update this number.  This number field is used for 
informational purposes.  It is up to the data retriever to delete 
the snippet request when it feels it has been attempted enough
times.  Theoretically, a request could be attempted 100 times, even
if iNumAttempts was only set to 5, since it is up to the retriever
to delete the request.

MEMBER iRetCode
MEMBER_TYPE int
MEMBER_DESCRIPTION This is the return code that the retriever gives
to the request after it attempts to process it.  There are currently
no generally agreed upon return codes.

MEMBER iLockTime
MEMBER_TYPE int
MEMBER_DESCRIPTION The Lock value associated with the snippet request.
In order to provide a mechanism for concurrent data retrievers, 
snippet requests must be locked before being processed.  iLockTime
is the key to the Lock for the snippet request.  After the request
has been processed, the key must be used to access the locked
snippet request, so that the request can be updated.

MEMBER szNote
MEMBER_TYPE char[100]
MEMBER_DESCRIPTION This is a note that the retriever may include
with the snippet request.  It is free to put whatever it likes
in this field, such as its name, strategy params, a log, etc.

MEMBER ComponentInfo
MEMBER_TYPE EWDB_StationStruct
MEMBER_DESCRIPTION Station/Component information for the channel
associated with this snippet request.

*************************************************
************************************************/
typedef struct _EWDB_SnippetRequestStruct
{
  EWDBid idSnipReq;
  EWDBid idChan;
  double tStart;
  double tEnd;
  EWDBid idEvent;
  int    iNumAttempts;
  time_t tNextAttempt;
  EWDBid idExistingWaveform;
  time_t tInitialRequest;
  int    iRequestGroup;
  int    iNumAlreadyAtmptd;
  int    iRetCode;
  int    iLockTime;
  char   szNote[100];
  EWDB_StationStruct ComponentInfo;
} EWDB_SnippetRequestStruct;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY UTILITY

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDB_CriteriaStruct
TYPE_DEFINITION struct _EWDB_CriteriaStruct
DESCRIPTION Structure for specifying criteria for object searches.
Criteria can be used to retrieve a list of certian objects, like Events
or Stations.

MEMBER Criteria
MEMBER_TYPE int
MEMBER_DESCRIPTION Set of flags specifying which criteria to use.  See
EWDB_CRITERIA_USE_TIME for a list of the possible criteria flags.  The
flags are not mutually exclusive.  The values of the remaning members
will be ignored if their matching criteria flags are not set.

MEMBER MinTime
MEMBER_TYPE int
MEMBER_DESCRIPTION The minimum acceptable time for a list value.
Specified as seconds since 1970.  If time were a criteria for selecting
a list of events, then all events in the returned list would have to
have occurred after MinTime.

MEMBER MaxTime
MEMBER_TYPE int
MEMBER_DESCRIPTION The maximum acceptable time for a list value.
Comparable to MinTime.  Expressed as seconds since 1970.

MEMBER MinLon
MEMBER_TYPE float
MEMBER_DESCRIPTION The minimum acceptable longitude for a list value.
Same principal as MinTime.  Expressed as degrees. (West=negative)

MEMBER MaxLon
MEMBER_TYPE float
MEMBER_DESCRIPTION The maximum acceptable longitude for a list value.
Same principal as MinTime.  Expressed as degrees. (West=negative)

MEMBER MinLat
MEMBER_TYPE float
MEMBER_DESCRIPTION The minimum acceptable lattitude for a list value.
Same principal as MinTime.  Expressed as degrees. (South=negative)

MEMBER MaxLat
MEMBER_TYPE float
MEMBER_DESCRIPTION The maximum acceptable lattitude for a list value.
Same principal as MinTime.  Expressed as degrees. (South=negative)

MEMBER MinDepth
MEMBER_TYPE float
MEMBER_DESCRIPTION The minimum acceptable depth for a list value.  Same
principal as MinTime.  Expressed as kilometers(km) below surface.

MEMBER MaxDepth
MEMBER_TYPE float
MEMBER_DESCRIPTION The maximum acceptable depth for a list value.  Same
principal as MinTime.  Expressed as kilometers(km) below surface.

*************************************************
************************************************/
typedef struct _EWDB_CriteriaStruct
{
  int   Criteria;
  int   MinTime;
  int   MaxTime;
  float MinLon;
  float MaxLon;
  float MinLat;
  float MaxLat;
  float MinDepth;
  float MaxDepth;
} EWDB_CriteriaStruct;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY STRONG_MOTION_API

LANGUAGE C

LOCATION THIS_FILE

TYPEDEF EWDB_SMChanAllStruct
TYPE_DEFINITION struct _EWDB_SMChanAllStruct
DESCRIPTION This struct contains information for 
a single strong motion message(SMMessage), including
any associated data, such as idEvent, Channel data.
It is used to retrieve SMMessages from the DB.

MEMBER idSMMessage
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION The DB ID of the SMMessage.

MEMBER idSMMessage
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION The DB ID of the Event with which
the message is associated.

MEMBER Station
MEMBER_TYPE EWDB_StationStruct
MEMBER_DESCRIPTION A structure containing information
about the channel from which the message was generated.

MEMBER SMChan
MEMBER_TYPE SM_INFO
MEMBER_DESCRIPTION The strong motion message itself,
in SM_INFO form.

*************************************************
************************************************/
typedef struct _EWDB_SMChanAllStruct
{
  EWDBid idSMMessage;
  EWDBid idEvent;
  EWDB_StationStruct Station;
  SM_INFO SMChan;
} EWDB_SMChanAllStruct;


/*** ALARMS ***/
/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF

LIBRARY  EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

TYPEDEF EWDB_AlarmsRecipientStruct
TYPE_DEFINITION struct _EWDB_AlarmsRecipientStruct
DESCRIPTION Information about a recipient of alarms.

MEMBER idRecipient
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID of this recipient.

MEMBER dPriority
MEMBER_TYPE int
MEMBER_DESCRIPTION Priority number of this recipient.

MEMBER sDescription
MEMBER_TYPE char[EWDB_MAXIMUM_AMPS_PER_CODA]
MEMBER_DESCRIPTION Name label for this recipient

*************************************************
************************************************/
typedef struct _EWDB_AlarmsRecipientStruct
{
	EWDBid 	idRecipient;
	int 	dPriority;
	char	sDescription[1024];
} EWDB_AlarmsRecipientStruct;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF

LIBRARY  EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

TYPEDEF EWDB_EmailDeliveryStruct
TYPE_DEFINITION struct _EWDB_EmailDeliveryStruct
DESCRIPTION Information about an email delivery. 

MEMBER idDelivery
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID of this delivery..

MEMBER sAddress
MEMBER_TYPE char[256]
MEMBER_DESCRIPTION Email address to deliver to.

MEMBER sMailServer
MEMBER_TYPE char[256]
MEMBER_DESCRIPTION Mail server to use for delivery.

*************************************************
************************************************/
typedef struct _EWDB_EmailDeliveryStruct
{
	EWDBid	idDelivery;
	char	sAddress[256];	
	char	sMailServer[256];	
} EWDB_EmailDeliveryStruct;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF

LIBRARY  EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

TYPEDEF EWDB_PagerDeliveryStruct
TYPE_DEFINITION struct _EWDB_PagerDeliveryStruct
DESCRIPTION Information about a pager delivery. 

MEMBER idDelivery
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID of this delivery..

MEMBER sNumber
MEMBER_TYPE char[256]
MEMBER_DESCRIPTION Pager number to send to.

MEMBER sPagerCompany
MEMBER_TYPE char[256]
MEMBER_DESCRIPTION Pager company to use.

*************************************************
************************************************/
typedef struct _EWDB_PagerDeliveryStruct
{
	EWDBid	idDelivery;
	char	sNumber[256];	
	char	sPagerCompany[256];	
} EWDB_PagerDeliveryStruct;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF

LIBRARY  EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

TYPEDEF EWDB_PhoneDeliveryStruct
TYPE_DEFINITION struct _EWDB_PhoneDeliveryStruct
DESCRIPTION Information about a phone delivery. 

MEMBER idDelivery
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID of this delivery..

MEMBER sPhoneNumber
MEMBER_TYPE char[256]
MEMBER_DESCRIPTION Telephone number to dial.

*************************************************
************************************************/
typedef struct _EWDB_PhoneDeliveryStruct
{
	EWDBid	idDelivery;
	char	sPhoneNumber[256];	
} EWDB_PhoneDeliveryStruct;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF

LIBRARY  EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

TYPEDEF EWDB_QddsDeliveryStruct
TYPE_DEFINITION struct _EWDB_QddsDeliveryStruct
DESCRIPTION Information about a qdds delivery. 

MEMBER idDelivery
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID of this delivery..

MEMBER sQddsDirectory
MEMBER_TYPE char[256]
MEMBER_DESCRIPTION Full path to the directory
where qdds files should be written.

*************************************************
************************************************/
typedef struct _EWDB_QddsDeliveryStruct
{
	EWDBid	idDelivery;
	char	sQddsDirectory[256];	
} EWDB_QddsDeliveryStruct;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF

LIBRARY  EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

TYPEDEF EWDB_CustomDeliveryStruct
TYPE_DEFINITION struct _EWDB_CustomDeliveryStruct
DESCRIPTION Information about a custom delivery. 

MEMBER idDelivery
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID of this delivery..

MEMBER sDescription
MEMBER_TYPE char[256]
MEMBER_DESCRIPTION Name label for this custom delivery.

*************************************************
************************************************/
typedef struct _EWDB_CustomDeliveryStruct
{
	EWDBid	idDelivery;
	char	sDescription[256];
} EWDB_CustomDeliveryStruct;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF

LIBRARY  EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

TYPEDEF EWDB_AlarmDeliveryUnionStruct
TYPE_DEFINITION struct _EWDB_AlarmDeliveryUnionStruct
DESCRIPTION Structure simulating a union of different
delivery mechanism types, switched based on the value
of DelMethodInd.

MEMBER DelMethodInd
MEMBER_TYPE int
MEMBER_DESCRIPTION Index of the delivery method
corresponding to the constants defined above.

MEMBER idRecipientDelivery
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database id of the recipient.

MEMBER email
MEMBER_TYPE EWDB_EmailDeliveryStruct
MEMBER_DESCRIPTION Information about an email delivery.

MEMBER pager
MEMBER_TYPE EWDB_PagerDeliveryStruct
MEMBER_DESCRIPTION Information about a pager delivery.

MEMBER phone
MEMBER_TYPE EWDB_PhoneDeliveryStruct
MEMBER_DESCRIPTION Information about a phone delivery.

MEMBER qdds
MEMBER_TYPE EWDB_QddsDeliveryStruct
MEMBER_DESCRIPTION Information about a qdds delivery.

MEMBER custom
MEMBER_TYPE EWDB_CustomDeliveryStruct
MEMBER_DESCRIPTION Information about a custom delivery.

*************************************************
************************************************/
typedef struct _EWDB_AlarmDeliveryUnionStruct
{
	int							DelMethodInd;
	EWDBid						idRecipientDelivery;
	EWDB_EmailDeliveryStruct	email;
	EWDB_PagerDeliveryStruct	pager;
	EWDB_PhoneDeliveryStruct	phone;
	EWDB_QddsDeliveryStruct		qdds;
	EWDB_CustomDeliveryStruct	custom;
} EWDB_AlarmDeliveryUnionStruct;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF

LIBRARY  EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

TYPEDEF EWDB_AlarmsCritProgramStruct
TYPE_DEFINITION struct _EWDB_AlarmsCritProgramStruct
DESCRIPTION Information about a criteria program 

MEMBER idCritProgram
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID of this program.

MEMBER sProgName
MEMBER_TYPE char[256]
MEMBER_DESCRIPTION Full path to the criteria program executable

MEMBER sProgDir
MEMBER_TYPE char[256]
MEMBER_DESCRIPTION Full path to the working directory.

MEMBER sProgDescription
MEMBER_TYPE char[256]
MEMBER_DESCRIPTION Label for this program.

*************************************************
************************************************/
typedef struct _EWDB_AlarmsCritProgramStruct
{
	EWDBid		idCritProgram;
	char		sProgName[256];
	char		sProgDir[256];
	char		sProgDescription[256];
} EWDB_AlarmsCritProgramStruct;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF

LIBRARY  EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

TYPEDEF EWDB_AlarmsFormatStruct
TYPE_DEFINITION struct _EWDB_AlarmsFormatStruct
DESCRIPTION Information about a format.

MEMBER idFormat
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID of this format.

MEMBER sDescription
MEMBER_TYPE char[256]
MEMBER_DESCRIPTION Label for this format.

MEMBER sFmtInsert
MEMBER_TYPE char[EWDB_ALARMS_MAX_FORMAT_LEN]
MEMBER_DESCRIPTION Insertion format.

MEMBER sFmtDelete
MEMBER_TYPE char[EWDB_ALARMS_MAX_FORMAT_LEN]
MEMBER_DESCRIPTION Deletion format.

*************************************************
************************************************/
typedef struct _EWDB_AlarmsFormatStruct
{
	EWDBid		idFormat;
	char		sDescription[256];
	char		sFmtInsert[EWDB_ALARMS_MAX_FORMAT_LEN];
	char		sFmtDelete[EWDB_ALARMS_MAX_FORMAT_LEN];
} EWDB_AlarmsFormatStruct;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF

LIBRARY  EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

TYPEDEF EWDB_AlarmsRuleStruct
TYPE_DEFINITION struct _EWDB_AlarmsRuleStruct
DESCRIPTION Information about a single rule.

MEMBER idRule
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID of this rule.

MEMBER dMag
MEMBER_TYPE double
MEMBER_DESCRIPTION Threshhold magnitude for this rule.

MEMBER Auto
MEMBER_TYPE int
MEMBER_DESCRIPTION Automatic or reviewed criterion.

MEMBER idRecipientDelivery
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID of the delivery to be used
if this rule is satisfied.

MEMBER DeliveryIndex
MEMBER_TYPE ind
MEMBER_DESCRIPTION Index into the Delivery array of the AlarmList
structure defined in the APPS section. 

MEMBER Format
MEMBER_TYPE EWDB_AlarmsFormatStruct
MEMBER_DESCRIPTION Information about the format to use.

MEMBER bCritInUse
MEMBER_TYPE int
MEMBER_DESCRIPTION Should the criteria program be evaluated?

MEMBER CritProg
MEMBER_TYPE EWDB_AlarmsCritProgramStruct
MEMBER_DESCRIPTION Information about the criteria program to use.

*************************************************
************************************************/
typedef struct _EWDB_AlarmsRuleStruct
{
	EWDBid							idRule;
	double							dMag;
	int								Auto;	
	EWDBid							idRecipientDelivery;
	int								DeliveryIndex; 
	EWDB_AlarmsFormatStruct			Format;
	int								bCritInUse;	
	EWDB_AlarmsCritProgramStruct	CritProg;
} EWDB_AlarmsRuleStruct;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF

LIBRARY  EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

TYPEDEF EWDB_AlarmsRecipientStructSummary
TYPE_DEFINITION struct _EWDB_AlarmsRecipientStructSummary
DESCRIPTION Summary information about how an alarm should be delivered to 
a recipient.

MEMBER idRule
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID of the rule which raised the alarm.

MEMBER sTableName
MEMBER_TYPE char[64]
MEMBER_DESCRIPTION Name of the delivery table (email, pager, etc.) to use.

MEMBER idDelivery
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID of the delivery to use.

MEMBER idRecipientDelivery
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID of the recipient's delivery to use.

*************************************************
************************************************/
typedef struct _EWDB_AlarmsRecipientStructSummary
{
    EWDBid			idRule;
	char			sTableName[64];
	EWDBid			idDelivery;
	EWDBid			idRecipientDelivery;
} EWDB_AlarmsRecipientStructSummary;


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF

LIBRARY  EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

TYPEDEF EWDB_AlarmAuditStruct
TYPE_DEFINITION struct _EWDB_AlarmAuditStruct
DESCRIPTION Information about an alarms audit entry.

MEMBER idAudit
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID of this audit.

MEMBER idEvent
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID of the event which triggered this alarm.

MEMBER bAuto
MEMBER_TYPE int
MEMBER_DESCRIPTION Automatic or reviewed criterion.

MEMBER Recipient
MEMBER_TYPE EWDB_AlarmsRecipientStruct
MEMBER_DESCRIPTION Information about the recipient who receieved this alarm.

MEMBER Format
MEMBER_TYPE EWDB_AlarmsFormatStruct
MEMBER_DESCRIPTION Information about the format used to send.

MEMBER idDelivery
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID of the delivery mechanism used to send.

MEMBER tAlarmDeclared
MEMBER_TYPE double
MEMBER_DESCRIPTION Time when the alarm was declared.

MEMBER tAlarmExecuted
MEMBER_TYPE double
MEMBER_DESCRIPTION Time when the alarm was executed.

MEMBER DelMethodInd
MEMBER_TYPE int
MEMBER_DESCRIPTION  Index of the delivery method
corresponding to the constants defined above.

MEMBER sInvocationString
MEMBER_TYPE char[256]
MEMBER_DESCRIPTION  Text inserted into the alarm message by the calling program.

*************************************************
************************************************/
typedef struct _EWDB_AlarmAuditStruct
{
	EWDBid								idAudit;
	EWDBid								idEvent;
	int									bAuto;
    EWDB_AlarmsRecipientStruct  		Recipient;
    EWDB_AlarmsFormatStruct	 			Format;
    EWDBid	    						idDelivery;
	double								tAlarmDeclared;
	double								tAlarmExecuted;
	int									DelMethodInd;
	char								sInvocationString[256];
} EWDB_AlarmAuditStruct;


/*** RAW INFRASTRUCTURE ***/
/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY RAW_INFRASTRUCTURE_API

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDB_FunctionBindStruct
TYPE_DEFINITION struct _EWDB_FunctionBindStruct
DESCRIPTION Structure for storing parameters for a function bind.  This
structure is used to store (in the database raw infrastructure schema)
a binding between a device and a response function.  This structure's
contents dictate that a certain device has a certain piece of
functionality (presumably related to signal response) for a time period
between tOn and tOff.

MEMBER idFunctionBind
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION DB ID of the function bind mechanism.

MEMBER idDevice
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION DB ID of the device to which a given functionality
is bound.

MEMBER tOn
MEMBER_TYPE double
MEMBER_DESCRIPTION Start of the time interval for which the
functionality exists for the given device.

MEMBER tOff
MEMBER_TYPE double
MEMBER_DESCRIPTION End of the time interval for which the functionality
exists for the given device.

MEMBER idFunction
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION DB ID of the Function bound to the device.

MEMBER szFunctionName
MEMBER_TYPE char[]
MEMBER_DESCRIPTION Name of the function type of the function identified
by idFunction.

MEMBER bOverridable
MEMBER_TYPE int
MEMBER_DESCRIPTION Flag indicating whether the function is an
overridable default for the device.  Devices can be derived from other
devices, and the derived devices inherit the functional properties of
the inheritees.  This flag indicates if this is a property of the
inheritee that should be overriden by the derived device.

MEMBER pFunction
MEMBER_TYPE void *
MEMBER_DESCRIPTION Pointer to the actual Function that is being bound
by this FunctionBind.

*************************************************
************************************************/
typedef struct _EWDB_FunctionBindStruct
{
  EWDBid idFunctionBind;
  EWDBid idDevice;
  EWDBid idFunction;
  char   szFunctionName[EWDB_RAW_INFRA_NAME_LEN];
  double tOff;
  double tOn;
  int    bOverridable;
  void * pFunction;
} EWDB_FunctionBindStruct;




/**********************************************************
 #########################################################
    FUNCTION PROTOTYPES Section
 #########################################################
**********************************************************/

/**********************************************************
 #########################################################
    UTILITY 
 #########################################################
**********************************************************/

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW API FORMATTED COMMENT
TYPE LIBRARY

LIBRARY  EWDB_API_LIB

SUB_LIBRARY UTILITY

LOCATION THIS_FILE

DESCRIPTION This is a portion of the EWDB_API_LIB
that contains constants and functions that are
used by the other portions of the EWDB_API_LIB
library.  It contains basic constants, and also
functions that deal with the setup and teardown
of the API, not with retrieval or insertion of
scientific data.

*************************************************
************************************************/


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY UTILITY

LANGUAGE C

LOCATION THIS_FILE

STABILITY MATURE

FUNCTION ewdb_api_Init

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION The API environment was successfully initialized.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION The API environment failed to initialize.  Please
see stderr or a logfile for details of the failure.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION The API environment was already initialized.  If the
client needs to reinitialize the API environment with different
parameters, they must first call ewdb_api_Shutdown to uninitialize
the existing environment.

PARAMETER 1
PARAM_NAME DBuser
PARAM_TYPE char *
PARAM_DESCRIPTION The database user that the program should run as.

PARAMETER 2
PARAM_NAME DBpassword
PARAM_TYPE char *
PARAM_DESCRIPTION The password for user DBuser.

PARAMETER 3
PARAM_NAME DBservice
PARAM_TYPE char *
PARAM_DESCRIPTION The network serviceID of the database to connect to.
In Oracle environments, the serviceID is usually defined in either a
tnsnames.ora file or by an Oracle Names server.

DESCRIPTION Initialization function for the API.  This function should
be called prior to any other API calls.  It initializes the API
environment and validates the database connection parameters (DBuser,
DBpassword, DBservice).

*************************************************
************************************************/
int ewdb_api_Init(char * DBuser, char * DBpassword, char * DBservice);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY UTILITY

LANGUAGE C

LOCATION THIS_FILE

STABILITY MATURE

FUNCTION ewdb_api_Shutdown

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION The API environment was successfully
de-initialized.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION The API environment failed to de-initialize.  Please
see stderr or a logfile for details of the failure.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION The API environment had not been initialized using
ewdb_api_Init.

DESCRIPTION Shutdown the API environment and free any related memory
and handles.

*************************************************
************************************************/
int ewdb_api_Shutdown(void);



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY UTILITY

LANGUAGE C

LOCATION THIS_FILE

STABILITY MATURE

FUNCTION ewdb_api_Set_Debug

RETURN_TYPE void

PARAMETER 1
PARAM_NAME IN_iDebug
PARAM_TYPE int
PARAM_DESCRIPTION Flag indicating the level of debug
information that the caller wishes the API to log.
See EWDB_DEBUG_DB_BASE_NONE for information on the 
various debug level constants.  Constants can be 
bitwise OR'd together to form the proper debug level.

DESCRIPTION Set the debug level for the API (and
supporting) functions.

*************************************************
************************************************/
void ewdb_api_Set_Debug(int IN_iDebug);


/**********************************************************
 #########################################################
    PARAMETRIC SCHEMA
 #########################################################
**********************************************************/


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW API FORMATTED COMMENT
TYPE LIBRARY

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LOCATION THIS_FILE

DESCRIPTION This is a portion of the EWDB_API_LIB
that deals with weak-motion parametric event data.
It provides access to parametric data, such as
event data, origin's, magnitudes, and mechanisms
as well as their supporting parametric data.

*************************************************
************************************************/

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY MATURE

FUNCTION ewdb_api_CreateArrival

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME pArrival
PARAM_TYPE EWDB_ArrivalStruct *
PARAM_DESCRIPTION Pointer to a EWDB_ArrivalStruct filled by the caller.

DESCRIPTION Function creates an arrival from the caller supplied
information in pArrival.  The function assumes that the caller has
supplied all information for an arrival including origin-phase
information.  The pArrival->idOrigin must be a valid existing origin DB
ID.  Upon successful completion, the function writes the DB ID of the
newly created phase pick(Pick) and the newly created OriginPick to
pArrival->idPick and pArrival->idOriginPick respectively.



*************************************************
************************************************/
int ewdb_api_CreateArrival(EWDB_ArrivalStruct *pArrival);




/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION ewdb_api_CreatePick

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME pArrival
PARAM_TYPE EWDB_ArrivalStruct *
PARAM_DESCRIPTION Pointer to a EWDB_ArrivalStruct filled by the caller.

DESCRIPTION Function creates an unassociated pick from the caller supplied
information in pArrival.  This function creates a Pick in the DB
but it does not associate the Pick with an Origin.  To associate Picks with 
an Origin, either call ewdb_api_CreateArrival() to create the entire arrival
at once, or call ewdb_api_CreateOriginPick() after creating a naked pick..  

The function assumes that the 
caller has supplied phase type(szObsPhase), phase arrival time (tObsPhase),
identification of the channel for which the phase was detected(idChan), 
phase first motion(cMotion), phase onset(cOnset), and pick error(dSigma).
All other members of the EWDB_ArrivalStruct are ignored by the function.

Upon successful completion, the function writes the DB ID of the
newly created phase pick(Pick) to pArrival->idPick.

*************************************************
************************************************/
int ewdb_api_CreatePick(EWDB_ArrivalStruct *pArrival);  



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION ewdb_api_CreateOriginPick

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME pArrival
PARAM_TYPE EWDB_ArrivalStruct *
PARAM_DESCRIPTION Pointer to a EWDB_ArrivalStruct filled by the caller.

DESCRIPTION Function creates an association between a pick and an origin.  
The information must be supplied by the caller in pArrival. 

Upon successful completion, the function writes the DB ID of the
newly created OriginPick to pArrival->idOriginPick

*************************************************
************************************************/
int ewdb_api_CreateOriginPick (EWDB_ArrivalStruct *pArrival, EWDBid idOrigin);  


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY MATURE


FUNCTION ewdb_api_CreateEvent

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME pEvent
PARAM_TYPE EWDB_EventStruct *
PARAM_DESCRIPTION Pointer to an EWDB_EventStruct filled by the caller.

DESCRIPTION Function creates an event in the DB using the information
provided by the caller in pEvent.  Upon successful completion, the
function writes the DB ID of the newly created event to pEvent->idEvent.
<br>
If the caller specifies a Source and a SourceEventID, and there is 
an existing Event with that same Source/SourceEventID, then the 
idEvent of the existing event will be returned, instead of 
creating a new event.  The function will return success in this case
even though it has not actually created a new Event.  If a matching
Event already exists, then the Comment passed by the caller is ignored.

*************************************************
************************************************/
int ewdb_api_CreateEvent(EWDB_EventStruct *pEvent);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY MODIFIED


FUNCTION ewdb_api_CreateMagnitude

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME pMag
PARAM_TYPE EWDB_MagStruct *
PARAM_DESCRIPTION Pointer to an EWDB_MagStruct filled by the caller.

DESCRIPTION Function creates a magnitude in the DB using the
information provided by the caller in pMag.  If pMag->bBindToEvent is
set to TRUE, then the function will attempt to associate the new
magnitude with an existing event(pMag->idEvent).  If both
pMag->bBindToEvent and pMag->bSetPreferred are TRUE, then the function
will attempt to set the magnitude as the preferred one for the Event.
Upon successful completion, the function writes the DB ID of the newly
created magnitude to pMag->idMag.  <br>
The call will fail under any of the following circumstances:
If bBindToEvent is TRUE, and idEvent is not a valid existing idEvent 
in the DB <br>
If idOrigin is set to a non-zero value, and it does not match 
an existing idOrigin in the DB<br>
szSource is not a valid source/author string.

*************************************************
************************************************/
int ewdb_api_CreateMagnitude(EWDB_MagStruct *pMag);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY MATURE


FUNCTION ewdb_api_CreateOrigin

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME pOrigin
PARAM_TYPE EWDB_OriginStruct *
PARAM_DESCRIPTION Pointer to an EWDB_OriginStruct filled by the caller.

DESCRIPTION Function creates an origin in the DB using the information
provided by the caller in pOrigin.  If pOrigin->BindToEvent is set to
TRUE, then the function will attempt to associate the new origin with
an existing event(pOrigin->idEvent).  If both pOrigin->BindToEvent and
pOrigin->SetPreferred are TRUE, then the function will attempt to set
the origin as the preferred one for the Event.  Upon successful
completion, the function writes the DB ID of the newly created origin
to pOrigin->idOrigin.

*************************************************
************************************************/
int ewdb_api_CreateOrigin(EWDB_OriginStruct *pOrigin);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY MODIFIED

FUNCTION ewdb_api_DeleteEvent

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME IN_idEvent
PARAM_TYPE int
PARAM_DESCRIPTION DB id of the event to delete.

PARAMETER 2
PARAM_NAME IN_bDeleteTraceOnly
PARAM_TYPE int
PARAM_DESCRIPTION If set to TRUE, delete only trace, otherwise delete both
trace and parametric data for this event.

DESCRIPTION Deletes trace and/or parametric data for an event from the
database. Note that this means that the deleted data is permanently
gone.

*************************************************
************************************************/
int ewdb_api_DeleteEvent(int IN_idEvent, int IN_bDeleteTraceOnly);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMmENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY MATURE


FUNCTION ewdb_api_GetArrivals

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Partial success.  A list of arrivals was retrieved,
but the caller's buffer was not large enough to accomadate all of the
arrivals found.  See pNumArrivalsFound for the number of arrivals found
and pNumArrivalsRetrieved for the number of arrivals placed in the
caller's buffer.

PARAMETER 1
PARAM_NAME idOrigin
PARAM_TYPE EWDBid
PARAM_DESCRIPTION This function retrieves a list of arrivals associated
with an origin.  idOrigin is the database ID of the origin for which
the caller wants a list of arrivals.

PARAMETER 2
PARAM_NAME pArrivals
PARAM_TYPE EWDB_ArrivalStruct *
PARAM_DESCRIPTION Buffer allocated by the caller, where the function
will write the list of arrivals.

PARAMETER 3
PARAM_NAME pNumArrivalsFound
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of found arrivals.

PARAMETER 4
PARAM_NAME pNumArrivalsRetrieved
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of arrivals placed in the callers buffer(pArrivals).

PARAMETER 5
PARAM_NAME BufferLen
PARAM_TYPE int
PARAM_DESCRIPTION Size of the pArrivals buffer as a multiple of
EWDB_ArrivalStruct. (example: 15 structs)

DESCRIPTION The function retrieves a list of arrivals that are
associated with a given origin.  See EWDB_ArrivalStruct for a
description of the information retrieved for each associated arrival.

*************************************************
************************************************/
int ewdb_api_GetArrivals(EWDBid idOrigin, EWDB_ArrivalStruct * pArrivals,
                         int * pNumArrivalsFound, int * pNumArrivalsRetrieved,
                         int BufferLen);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY MATURE

FUNCTION ewdb_api_GetEventInfo

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME idEvent
PARAM_TYPE EWDBid
PARAM_DESCRIPTION The DB identifier of the Event for which the caller
wants information.

PARAMETER 2
PARAM_NAME pEvent
PARAM_TYPE EWDB_EventStruct *
PARAM_DESCRIPTION Pointer to a EWDB_EventStruct allocated by the
caller, where the function will write the information about the Event.

DESCRIPTION Function retrieves information about the Event identified
by idEvent.  See EWDB_EventStruct for a description of the information
provided.

*************************************************
************************************************/
int ewdb_api_GetEventInfo(EWDBid idEvent, EWDB_EventStruct * pEvent);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY MATURE


FUNCTION ewdb_api_GetEventList

DESCRIPTION Function retrieves summary information for a list of
events, based on selection criteria provided by the caller.  See
EWDB_EventListStruct for a description of the information returned for
each event in the list.

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION A list of events was successfully retrieved.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION The function experienced a fatal error while
retrieving a list of events.

RETURN_VALUE -x < -1
RETURN_DESCRIPTION (-x) is returned where x > 1 when the size of the
caller's buffer is insufficient to hold all of the events that matched
the caller's criteria.  x is the size of the buffer required to
retrieve all of the events that were found.  The function places as
many events as possible into the caller's existing buffer.  This is not
a failure condition, just a warning that the caller did not receive all
of the requested data.

PARAMETER 1
PARAM_NAME pBuffer
PARAM_TYPE EWDB_EventListStruct *
PARAM_DESCRIPTION Buffer allocated by the caller, in which the function
will place the list of events matching the caller's criteria.

PARAMETER 2
PARAM_NAME len
PARAM_TYPE int
PARAM_DESCRIPTION Length of pBuffer as a multiple of
EWDB_EventListStruct.

PARAMETER 3
PARAM_NAME StartTime
PARAM_TYPE int
PARAM_DESCRIPTION Minimum(oldest) time acceptable for an event.
Expressed as seconds since 1970(time_t).

PARAMETER 4
PARAM_NAME EndTime
PARAM_TYPE int
PARAM_DESCRIPTION Maximum(youngest) time acceptable for an event.
Expressed as seconds since 1970(time_t).

PARAMETER 5
PARAM_NAME pCriteria
PARAM_TYPE EWDB_EventListStruct *
PARAM_DESCRIPTION Pointer to the EWDB_EventListStruct
that contains part of the selection criteria for the
list, including: minimum lattitude, minimum longitude,
minimum depth, minimum magnitude, and source
limitations.  Source criteria is:             <br>
         "*"  events from all sources             <br>
         "**" events from only human sources      <br>
         ""   (blank string)all automatic sources <br>
Any other string will be matched against the source identifier, such as
the logo of the earthworm module that created a solution, or the
dblogin of a human reviewer.

PARAMETER 6
PARAM_NAME pMaxEvent
PARAM_TYPE EWDB_EventListStruct *
PARAM_DESCRIPTION Pointer to the EWDB_EventListStruct that contains
part of the selection criteria for the list, including: maximum
lattitude, maximum longitude, maximum depth, and maximum magnitude.

PARAMETER 7
PARAM_NAME pNumEvents
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function writes the
number of events it put in pBuffer.

DESCRIPTION Returns the list of all events in the database, in a given
time range, that meet the criteria specified in pCriteria.  For each Event
found to meet the given criteria, an EWDB_EventListStruct is returned.
This is the primary function used for searching for a subset of the events
in the DB.

*************************************************
************************************************/
int ewdb_api_GetEventList(EWDB_EventListStruct * pBuffer, int len,
                          unsigned int StartTime, unsigned int EndTime,
                          EWDB_EventListStruct * pCriteria, 
                          EWDB_EventListStruct * pMaxEvent, int * pNumEvents);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY MODIFIED


FUNCTION ewdb_api_GetMagnitude

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME idMag
PARAM_TYPE EWDBid
PARAM_DESCRIPTION The DB identifier of the Magnitude for which the
caller wants information.

PARAMETER 2
PARAM_NAME pMag
PARAM_TYPE EWDB_MagStruct *
PARAM_DESCRIPTION Pointer to an EWDB_MagStruct allocated by the caller,
where the function will write the information about the Magnitude.

DESCRIPTION Function retrieves information about the Magnitude
identified by idMag.  See EWDB_MagStruct for a description of the
information provided.

*************************************************
************************************************/
int ewdb_api_GetMagnitude(EWDBid idMag, EWDB_MagStruct * pMag);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY MATURE


FUNCTION ewdb_api_GetOrigin

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME idOrigin
PARAM_TYPE EWDBid
PARAM_DESCRIPTION The DB identifier of the Origin for which the caller
wants information.

PARAMETER 2
PARAM_NAME pOrigin
PARAM_TYPE EWDB_OriginStruct *
PARAM_DESCRIPTION Pointer to a EWDB_OriginStruct allocated by the
caller, where the function will write the information about the
Origin.

DESCRIPTION Function retrieves information about the Origin identified
by idOrigin.  See EWDB_OriginStruct for a description of the
information provided.

*************************************************
************************************************/
int ewdb_api_GetOrigin(EWDBid idOrigin, EWDB_OriginStruct * pOrigin);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY MATURE


FUNCTION ewdb_api_GetPreferredSummaryInfo

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME idEvent
PARAM_TYPE EWDBid
PARAM_DESCRIPTION The DB identifier for the Event for which the caller
wants summary info.

PARAMETER 2
PARAM_NAME pidOrigin
PARAM_TYPE EWDBid *
PARAM_DESCRIPTION Pointer to a EWDBid where the function will write the
DB ID of the preferred origin for the Event.  0 will be written as the
ID if there is no preferred origin for the given Event.

PARAMETER 3
PARAM_NAME pidMagnitude
PARAM_TYPE EWDBid *
PARAM_DESCRIPTION Pointer to a EWDBid where the function will write the
DB ID of the preferred magnitude for the Event.  0 will be written as
the ID if there is no preferred magnitude for the given event.

PARAMETER 4
PARAM_NAME pidMech
PARAM_TYPE EWDBid *
PARAM_DESCRIPTION Pointer to a EWDBid where the function will write the
DB ID of the preferred mechanism for the Event.  0 will be written as
the ID if there is no preferred mechanism for the given event.  (Note:
Mechanism are currently only supported in a trivial manner)

DESCRIPTION Function retrieves the DB IDs for the preferred summary
information(origin, magnitude, and mechanism) for the Event identified
by idEvent.

*************************************************
************************************************/
int ewdb_api_GetPreferredSummaryInfo(EWDBid idEvent, EWDBid *pidOrigin,
                                     EWDBid *pidMagnitude, EWDBid *pidMech);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW


FUNCTION ewdb_api_GetStaMags

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Partial success.  A list of station magnitudes was
retrieved, but the caller's buffer was not large enough to accomadate
all of the station magnitudes found.  See pNumStaMagsFound for the
number of station magnitudes found and pNumStaMagsRetrieved for the
number of station magnitudes placed in the caller's buffer.

PARAMETER 1
PARAM_NAME idMagnitude
PARAM_TYPE EWDBid
PARAM_DESCRIPTION This function retrieves a list of station magnitudes
associated with an summary magnitude.  idMagnitude is the database ID
of the magnitude for which the caller wants a list of station
magnitudes.

PARAMETER 2
PARAM_NAME pStaMags
PARAM_TYPE EWDB_StationMagStruct *
PARAM_DESCRIPTION Buffer allocated by the caller, where the function
will write the list of station magnitudes.

PARAMETER 3
PARAM_NAME pNumStaMagsFound
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of station magnitudes found.

PARAMETER 4
PARAM_NAME pNumStaMagsRetrieved
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of station magnitudes placed in the caller's
buffer(pStaMags).

PARAMETER 5
PARAM_NAME BufferLen
PARAM_TYPE int
PARAM_DESCRIPTION Size of the pStaMags buffer as a multiple of
EWDB_StationMagStruct. (example: 15 structs)

DESCRIPTION The function retrieves a list of station magnitudes that
are associated with a given summary magnitude.  See
EWDB_StationMagStruct for a description of the information
retrieved for each associated station magnitude.

*************************************************
************************************************/
int ewdb_api_GetStaMags(EWDBid idMagnitude, 
                        EWDB_StationMagStruct * pStaMags,
                        int * pNumStaMagsFound, int * pNumStaMagsRetrieved,
                        int BufferLen);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW


FUNCTION ewdb_api_UpdateComment

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME szFieldTypeName
PARAM_TYPE char *
PARAM_DESCRIPTION Name of the table to updated

PARAMETER 2
PARAM_NAME idCore
PARAM_TYPE EWDBid
PARAM_DESCRIPTION id of the record to update

PARAMETER 3
PARAM_NAME szUpdatedComment
PARAM_TYPE char *
PARAM_DESCRIPTION Text of the updated comment. 

DESCRIPTION  Updates a comment for any record in the DB.  Enter the
tablename and the record ID for the table that the comment belongs to,
and the function will update the comment for that record.  This
requires that the caller know DB layout, which is a bad thing.  What we
need is a lookup table for the comment types, and we can perform a
conversion from the lookup table to the actual DB table type within
the DB.  

*************************************************
************************************************/
int ewdb_api_UpdateComment (char * szFieldTypeName, EWDBid idCore,
                            char * szUpdatedComment);



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW


FUNCTION ewdb_api_GetOriginsForEvent

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Origin records found successfully, but one
or more records were unable to be retrieved.  Most likely 
indicates that the caller's buffer is not large enough to hold
all of the records found.

PARAMETER 1
PARAM_NAME IN_idEvent
PARAM_TYPE EWDBid
PARAM_DESCRIPTION Event for which the caller wishes to 
retrieve origins.

PARAMETER 2
PARAM_NAME pOrigins
PARAM_TYPE EWDB_OriginStruct *
PARAM_DESCRIPTION Pointer to a buffer (allocated by the caller)
where the function will put information for the Origins found 
for the given Event.  

PARAMETER 3
PARAM_NAME pNumOriginsFound
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer (caller allocated) where
the function will write the number of origins found for the given
idEvent.  Note:  this is not the number actually retrieved.

PARAMETER 4
PARAM_NAME pNumOriginsRetrieved
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer (caller allocated) where
the function will write the number of origins retrieved into the
caller's buffer for the given idEvent

PARAMETER 5
PARAM_NAME IN_BufferRecLen
PARAM_TYPE int 
PARAM_DESCRIPTION The size of the pOrigins buffer allocated
by the caller.  (In terms of number of EWDB_OriginStruct records)

DESCRIPTION  This function retrieves information for each Origin that
is associated with the event(idEvent) given by the caller.  

NOTE  Then entire EWDB_OriginStruct is not filled!  Only the following
fields are filled by this function:  idOrigin, tOrigin, dLat, dLon, 
dDepth, iAssocPh, iUsedPh, iFixedDepth, sSource, and sRealSource.

*************************************************
************************************************/
int ewdb_api_GetOriginsForEvent(EWDBid IN_idEvent, EWDB_OriginStruct * pOrigins,
                                int * pNumOriginsFound, 
                                int * pNumOriginsRetrieved,
                                int IN_BufferRecLen);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY  PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION ewdb_api_InsertCodaWithMag 


SOURCE_LOCATION src/oracle/schema-working/src/parametric/ewdb_api_InsertcodaWithMag.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME pCodaDur
PARAM_TYPE EWDB_StationMagStruct *
PARAM_DESCRIPTION Pointer to a EWDB_StationMagStruct filled by the caller.

DESCRIPTION Function creates a coda duration based station magnitude
from the caller supplied information in pCodaDur.  The function assumes
that the caller has supplied all station magnitude information
including summary magnitude data in the EWDB_StationMagStruct.  The
caller need not fill in PeakAmp related information as it is
inappropriate.  The pCodaDur->idMagnitude and 
pCodaDur->StaMagUnion.CodaDur.idPick values
must be valid existing Magnitude and Pick DB IDs respectively.  The
pCodaDur->idChan value must also be filled in with the DB ID of a valid
existing channel.  Upon successful completion, the function writes the
DB IDs of the newly created coda duration, coda termination, and
station magnitude to pCodaDur->idCodaDur, pCodaDur->idTCoda, and
pCodaDur->idMagLink respectively.
This function performs a full insertion of a coda,
complete with how that coda (duration) contributed to an overall 
duration magnitude.  

NOTE You should use this function instead of using ewdb_internal_CreateTCoda(),
ewdb_internal_CreateCodaDur, and ewdb_internal_CreateStaMag(), unless you are 
VERY SURE that you know what you are doing, and know exactly how 
to use those functions.

*************************************************
************************************************/
int ewdb_api_InsertCodaWithMag(EWDB_StationMagStruct *pCodaDur);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY  PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION ewdb_api_InsertPeakAmpWithMag 


SOURCE_LOCATION src/oracle/schema-working/src/parametric/ewdb_api_InsertPeakAmpWithMag.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME pPeakAmp
PARAM_TYPE EWDB_StationMagStruct *
PARAM_DESCRIPTION Pointer to a EWDB_PhaseAmpStruct filled by the caller.

DESCRIPTION Function creates a peak amplitude based station magnitude
from the caller supplied information in pPeakAmp.  The function assumes
that the caller has supplied all station magnitude information
including summary magnitude data in the EWDB_StationMagStruct.  The
caller need not fill in CodaDur related information as it is
inappropriate. 
This function performs a full insertion of an amplitude,
complete with how that amplitude contributed to an overall 
magnitude.  

NOTE You should use this function instead of using ewdb_internal_CreatePeakAmp(),
and ewdb_internal_CreateStaMag(), unless you are VERY SURE that you know what
you are doing, and know exactly how to use those functions.

*************************************************
************************************************/
int ewdb_api_InsertPeakAmpWithMag(EWDB_StationMagStruct *pPeakAmp);  

/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_api_GetMagsForOrigin

STABILITY NEW

SOURCE_LOCATION THIS_FILE

RETURN_TYPE int

RETURN_VALUE Default_Return_Value
RETURN_DESCRIPTION Description of the default return value

PARAMETER 1
PARAM_NAME IN_idOrigin
PARAM_TYPE EWDBid
PARAM_DESCRIPTION Optional description of (IN_idOrigin)

PARAMETER 2
PARAM_NAME pMags
PARAM_TYPE EWDB_MagStruct *
PARAM_DESCRIPTION Optional description of (pMags)

PARAMETER 3
PARAM_NAME pNumMagsFound
PARAM_TYPE int *
PARAM_DESCRIPTION Optional description of (pNumMagsFound)

PARAMETER 4
PARAM_NAME pNumMagsRetrieved
PARAM_TYPE int *
PARAM_DESCRIPTION Optional description of (pNumMagsRetrieved)

PARAMETER 5
PARAM_NAME IN_BufferRecLen
PARAM_TYPE int
PARAM_DESCRIPTION Optional description of (IN_BufferRecLen)

DESCRIPTION Optionally, write a description of what the function does
and how it behaves.

NOTE Optionally, write a note about
the function here.

*************************************************
************************************************/
/* DK CLEANUP */
int ewdb_api_GetMagsForOrigin(EWDBid IN_idOrigin, EWDB_MagStruct * pMags,
                                int * pNumMagsFound, 
                                int * pNumMagsRetrieved,
                                int IN_BufferRecLen);



/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY UNKNOWN

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_api_DeleteDataBeforeTime

STABILITY NEW

SOURCE_LOCATION THIS_FILE

RETURN_TYPE int

RETURN_VALUE Default_Return_Value
RETURN_DESCRIPTION Description of the default return value

PARAMETER 1
PARAM_NAME IN_tTime
PARAM_TYPE int
PARAM_DESCRIPTION Optional description of (IN_tTime)

PARAMETER 2
PARAM_NAME IN_iDatatypes
PARAM_TYPE int
PARAM_DESCRIPTION Optional description of (IN_iDatatypes)

PARAMETER 3
PARAM_NAME bUnassociatedOnly
PARAM_TYPE int
PARAM_DESCRIPTION Optional description of (bUnassociatedOnly)

DESCRIPTION Optionally, write a description of what the function does
and how it behaves.

NOTE Optionally, write a note about
the function here.


*************************************************
************************************************/
int ewdb_api_DeleteDataBeforeTime(int IN_tTime, int IN_iDatatypes, 
                                  int bUnassociatedOnly);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY UNKNOWN

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_api_UpdateEvent

STABILITY NEW

SOURCE_LOCATION THIS_FILE

RETURN_TYPE int

RETURN_VALUE Default_Return_Value
RETURN_DESCRIPTION Description of the default return value

PARAMETER 1
PARAM_NAME pEvent
PARAM_TYPE EWDB_EventStruct *
PARAM_DESCRIPTION Optional description of (pEvent)

PARAMETER 2
PARAM_NAME iUpdateInfoType
PARAM_TYPE int
PARAM_DESCRIPTION Optional description of (iUpdateInfoType)

DESCRIPTION Optionally, write a description of what the function does
and how it behaves.

NOTE Optionally, write a note about
the function here.


*************************************************
************************************************/
int ewdb_api_UpdateEvent(EWDB_EventStruct * pEvent, int iUpdateInfoType);

/**********************************************************
 #########################################################
    EXTERNAL SCHEMA 
 #########################################################
**********************************************************/

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW API FORMATTED COMMENT
TYPE LIBRARY

LIBRARY  EWDB_API_LIB

SUB_LIBRARY EXTERNAL_API

LOCATION THIS_FILE

DESCRIPTION This is a portion of the EWDB_API_LIB
that contains references to external systems and
data.  This includes things like a cheater table
to go from Earthworm SCN to EWDB idChan.

*************************************************
************************************************/


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY EXTERNAL_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY MATURE


FUNCTION ewdb_api_CreateOrAlterExternalStation

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME pStation
PARAM_TYPE EWDB_External_StationStruct *
PARAM_DESCRIPTION Pointer to an EWDB_External_StationStruct filled by the caller.

DESCRIPTION Function creates or updates a component and associated
channel in the DB using an external station table and a a cheater table
to go from external station to chan.  Upon successful completion it
writes the DB ID of the new External Station to pStation->StationID.
This function ignores the pStation->idComp parameter.

NOTE   If station with the scnl is already in the table,
its STATIONID will be set in the pStation struct, AND all
non-zero, and non-empty character values will be updated.
<br><br>
If this is a new record, all values from the pStation struct
will be stored in the table.


*************************************************
************************************************/
int ewdb_api_CreateOrAlterExternalStation(EWDB_External_StationStruct *pStation);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY EXTERNAL_API

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_api_GetIdChanFromStationExternal

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME pidChan
PARAM_TYPE EWDBid *
PARAM_DESCRIPTION Pointer to EWDBid where the function will write
the idChan for this external station.

PARAMETER 2
PARAM_NAME IN_StationID
PARAM_TYPE int
PARAM_DESCRIPTION Database StationID of the external station.

DESCRIPTION Given the DB StationID of the external station(component), 
the function will return find and return the idChan for it.

NOTE This function uses a "cheater table" mechanism to convert
from a StationID to an idChan.  This "cheater table" has theoretically
been setup beforehand by the DB operator, to map the names on the data
channels coming in, to idChans.  This is a task that must be done by
the operator, because it usually requires a priori knowledge about
what names refer to what channels(since more than one channel may
have the same name).

*************************************************
************************************************/
int ewdb_api_GetIdChanFromStationExternal(EWDBid * pidChan, int IN_StationID);



/**********************************************************
 #########################################################
    WAVEFORM SCHEMA
 #########################################################
**********************************************************/

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW API FORMATTED COMMENT
TYPE LIBRARY

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM_API

LOCATION THIS_FILE

DESCRIPTION This is the Waveform portion
of the EWDB_API_LIB library.  It provides access to
binary waveform data in the Earthworm DB.

*************************************************
************************************************/


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM_API

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_api_GetWaveformDesc

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME pWaveformDesc
PARAM_TYPE EWDB_WaveformStruct
PARAM_DESCRIPTION Structure containing information about the waveform 
descriptor.  The function will read pWaveformDesc->idWaveform, and 
attempt to retrieve information about that descriptor.  Upon successfull 
completion, the function will write the retrieved information into the
pWaveformDesc struct.  The structure pointed to by pWaveformDesc must
be allocated by the client.

DESCRIPTION Retrieve information about the waveform descriptor pointed
to by pWaveformDesc->idWaveform from the database.

*************************************************
************************************************/
int ewdb_api_GetWaveformDesc(EWDB_WaveformStruct * pWaveformDesc);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM_API

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_api_GetWaveformListByEvent

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME IN_idEvent
PARAM_TYPE EWDBid
PARAM_DESCRIPTION Database ID of the Event whose waveform
snippets the caller is interested in.

PARAMETER 2
PARAM_NAME pWaveformBuffer
PARAM_TYPE EWDB_WaveformStruct
PARAM_DESCRIPTION Pointer to a  caller allocated buffer of waveform structs
which will be filled by this function.

PARAMETER 3
PARAM_NAME pStationBuffer
PARAM_TYPE EWDB_WaveformStruct
PARAM_DESCRIPTION Pointer to a  caller allocated buffer of station structs
which will be filled by this function.  NOTE:  This buffer is only filled
if bIncludeStationInfo is set to TRUE.  This param can be set to NULL
if bIncludeStationInfo is FALSE.

PARAMETER 4
PARAM_NAME bIncludeStationInfo
PARAM_TYPE int
PARAM_DESCRIPTION Flag(TRUE/FALSE) indicating whether the function should
retrieve station/component information for each waveform found.

PARAMETER 5
PARAM_NAME pWaveformDescsFound
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of waveforms found.

PARAMETER 6
PARAM_NAME pWaveformDescsRetrieved
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of waveform descriptors placed in the caller's 
buffer(pWaveformBuffer).

PARAMETER 7
PARAM_NAME BufferRecLen
PARAM_TYPE int
PARAM_DESCRIPTION Size of the pWaveformBuffer buffer as a multiple of
EWDB_WaveformStruct. (example: 15 structs)  The pStationBuffer is also
assumed to be of atleast this size if bIncludeStationInfo is set to TRUE.


DESCRIPTION The function retrieves a list of waveforms that are
associated with a given event.  See EWDB_WaveformStruct for a
description of the information retrieved for each associated waveform.
If bIncludeStationInfo is set to TRUE, then the function also retrieves
station/component information for each waveform and writes it into 
the pStationBuffer buffer, in matching order with the waveforms.

NOTE This function only retrieves the waveform DESCRIPTORS, it does not
actually retrieve the binary waveform.

*************************************************
************************************************/
int ewdb_api_GetWaveformListByEvent(EWDBid idEvent, 
                                    EWDB_WaveformStruct * pWaveformBuffer,
                                    EWDB_StationStruct * pStationBuffer,
                                    int bIncludeStationInfo,
                                    int * pWaveformDescsFound,
                                    int * pWaveformDescsRetrieved,
                                    int BufferRecLen);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY MATURE


FUNCTION ewdb_api_GetWaveformSnippet

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION The Actual Length of the snippet retrieved
was 0 bytes. (empty snippet)

PARAMETER 1
PARAM_NAME IN_idWaveform
PARAM_TYPE EWDBid
PARAM_DESCRIPTION Database ID of the waveform descriptor whose
snippet the caller is interested in.

PARAMETER 2
PARAM_NAME IN_pWaveform
PARAM_TYPE char *
PARAM_DESCRIPTION Buffer allocated (prior to calling this function)
by the caller to hold the retrieved snippet.  The function writes the
retrieved snippet to the buffer.

PARAMETER 3
PARAM_NAME IN_WaveformLength
PARAM_TYPE int
PARAM_DESCRIPTION Length of the IN_pWaveform buffer.

DESCRIPTION Retrieves the snippet associated with IN_idWaveform and
returns it in the IN_pWaveform buffer.

*************************************************
************************************************/
int ewdb_api_GetWaveformSnippet(EWDBid IN_idWaveform, char * IN_pWaveform,
                                 int IN_WaveformLength);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY MATURE


FUNCTION ewdb_api_CreateWaveform

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME pWaveform
PARAM_TYPE EWDB_WaveformStruct *
PARAM_DESCRIPTION Pointer to an EWDB_WaveformStruct filled by the caller
with both waveform descriptor information and the actual binary waveform.

PARAMETER 2
PARAM_NAME idEvent
PARAM_TYPE EWDBid
PARAM_DESCRIPTION DB ID of an existing Event in the database that the
caller wishes to associate with the new Waveform.

DESCRIPTION  Function inserts a waveform into the database, including
both descriptor information and the actual binary snippet.  Optionally
(if idEvent is not 0), the function will associate the new Waveform
with the existing DB Event identified by idEvent.

*************************************************
************************************************/
int ewdb_api_CreateWaveform(EWDB_WaveformStruct * pWaveform, EWDBid idEvent);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW


FUNCTION ewdb_api_UpdateWaveform

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME pWaveform
PARAM_TYPE EWDB_WaveformStruct *
PARAM_DESCRIPTION Pointer to an EWDB_WaveformStruct filled by the caller
with both waveform descriptor information and the actual binary waveform.

DESCRIPTION  Function updates an existing waveform in the database, including
both descriptor information and the actual binary snippet.

*************************************************
************************************************/
int ewdb_api_UpdateWaveform(EWDB_WaveformStruct * pWaveform);


/**********************************************************
 #########################################################
    COOKED INFRASTRUCTURE
 #########################################################
**********************************************************/

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW API FORMATTED COMMENT
TYPE LIBRARY

LIBRARY  EWDB_API_LIB

SUB_LIBRARY COOKED_INFRASTRUCTURE_API

LOCATION THIS_FILE

DESCRIPTION This is the Cooked Infrastructure portion
of the EWDB_API_LIB library.  It provides access to
station/component name, location and orientation,
and response information for each channel in the
Earthworm DB.

*************************************************
************************************************/


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY COOKED_INFRASTRUCTURE_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY EXPERIMENTAL

FUNCTION ewdb_api_AssociateChanWithComp 

SOURCE_LOCATION src/oracle/schema-working/src/infra/ewdb_api_AssociateChanWithComp.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME idComp
PARAM_TYPE EWDBid
PARAM_DESCRIPTION The DB identifier of the component(Comp) record the
caller wants to associate with a channel.

PARAMETER 2
PARAM_NAME idChan
PARAM_TYPE EWDBid
PARAM_DESCRIPTION The DB identifier of the channel(Chan) record the
caller wants to associate with a component.

PARAMETER 3
PARAM_NAME tStart
PARAM_TYPE double
PARAM_DESCRIPTION The start of the time interval for which the caller
wants to associate the component and channel.  Expressed as seconds
since 1970.

PARAMETER 4
PARAM_NAME tEnd
PARAM_TYPE double
PARAM_DESCRIPTION The end of the time interval for which the caller
wants to associate the component and channel.  Expressed as seconds
since 1970.

PARAMETER 5
PARAM_NAME pidCompT
PARAM_TYPE EWDBid *
PARAM_DESCRIPTION A pointer to a EWDBid where the function will write
information if the call fails.

PARAMETER 6
PARAM_NAME pidChanT
PARAM_TYPE EWDBid *
PARAM_DESCRIPTION A pointer to a EWDBid where the function will write
information if the call fails.

DESCRIPTION Function creates an association between a channel and a
component for a specified time interval.  The function can fail if a
relationship exists between the specified component and an alternative
channel, or between the specified channel and an alternative component,
during an overlapping time period.
<br><br>
<b>NOTE THIS FUNCTION IS UNSUPPORTED.<br>
Please contact someone in
the Earthworm DB development group if you need to set specific
component/channel time based relationships.  <br>
ewdb_internal_SetChanParams()
is also available(and not recommended for use).</b>
<br><br>
The relationship between a component and a channel is complex to
implement.  I believe the issues that cause the pidCompT and pidChanT
variables to be written to, have been cleared up, and they are no
longer applicable;  however, I am not sure of this, so please be sure
to pass valid pointers until better documentation can be made
available.  davidk 2000/05/04. <br> 
Davidk 07/27/2001
<br><br>

NOTE The following is believed to be out
of date, but is the best information available at this time: 
<br><br>
A channel can be associated with at most one component at a given time.
If the channel that the caller is trying to bind a component to is
already bound to another component during an overlapping time interval,
then the call will fail, and the idComp of the first component found
that has an overlapping association with that channel will be
returned.  Currently, if there are any existing chan relationships
during the time period described by tStart,tEnd, for the given idChan,
then the idChanT of the first ChanT relationship record will be
returned in pidChanT along with an error condition. <br>
Davidk 07/27/2001


*************************************************
************************************************/
int ewdb_api_AssociateChanWithComp(EWDBid idComp, EWDBid idChan, 
                                   double tStart, double tEnd,
                                   EWDBid * pidCompT, EWDBid * pidChanT);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY COOKED_INFRASTRUCTURE_API

LANGUAGE C

LOCATION THIS_FILE

STATUS EXPERIMENTAL

FUNCTION ewdb_api_CreateChannel 

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME pidChan
PARAM_TYPE EWDBid *
PARAM_DESCRIPTION A pointer to a EWDBid where the
function will write The DB identifier of the Channel
it created.

PARAMETER 2
PARAM_NAME szComment
PARAM_TYPE char *
PARAM_DESCRIPTION Optional comment regarding the component.  It may be
blank, and should be a maximum of 4k characters.  It may not be NULL!

DESCRIPTION Function creates a channel in the DB. A channel identifies
the entire path of a seismic signal from ground motion to completed
waveform.  The function writes the DB ID of the newly created channel
to pidChan.  Because most of a channel's properties are variable over
time, this function does not take much as input, as it only creates a
channel with its fixed attribute(s).  This function should be used to
generate a new idChan that can be used in conjunction with other
functions for creating components and setting time based attributes of
components and channels.

NOTE <b>THIS FUNCTION IS UNSUPPORTED!</b><br>  A channel(Chan) represents a unique 
data path from ground motion to finished waveform.  It is associated with 
a Component sensor, and all of the signal altering devices that the 
sensor's signal passes through before becoming a finished set of waveforms.

*************************************************
************************************************/
int ewdb_api_CreateChannel(EWDBid * pidChan, char * szComment);



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY COOKED_INFRASTRUCTURE_API

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_api_CreateCompTForSCNLT

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION  Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success

PARAMETER 1
PARAM_NAME pidChan
PARAM_TYPE EWDBid *
PARAM_DESCRIPTION Pointer to an EWDBid where the function will
write the idChan of the newly created channel, associated with
the SCNL.

PARAMETER 2
PARAM_NAME szSta
PARAM_TYPE char *
PARAM_DESCRIPTION The Station Code of the SCNL.

PARAMETER 3
PARAM_NAME szComp
PARAM_TYPE char *
PARAM_DESCRIPTION The Component(Channel) Code of the SCNL.

PARAMETER 4
PARAM_NAME szNet
PARAM_TYPE char *
PARAM_DESCRIPTION The Network Code of the SCNL.

PARAMETER 5
PARAM_NAME szLoc
PARAM_TYPE char *
PARAM_DESCRIPTION The Location Code of the SCNL.

PARAMETER 6
PARAM_NAME tStart
PARAM_TYPE double
PARAM_DESCRIPTION The start of the validity time range for the given SCNL.
Seconds since 1970.

PARAMETER 7
PARAM_NAME tEnd
PARAM_TYPE double
PARAM_DESCRIPTION The end of the validity time range for the given SCNL.
Seconds since 1970.

DESCRIPTION This function is misnamed.  It actually creates an association
between an SCNL and a channel for a given time period.  It is called 
CreateCompTForSCNLT() because it creates a component time interval(CompT)
record as part of the process of creating a component and a channel, and
associating the two together for a given time period.  The function returns
the idChan of the newly created channel with which the SCNL has been
associated for the supplied time interval.  The DB ID of the component(Comp)
and the component time interval(CompT) are not returned.  The caller can
retrieve the idComp and idCompT by calling ewdb_api_GetComponentInfo() with
the idChan of the newly created channel, and tStart or tEnd.  The caller can
change the location/orientation of the component by calling 
ewdb_api_SetCompParams() after calling ewdb_api_CreateCompTForSCNLT().

*************************************************
************************************************/
int ewdb_api_CreateCompTForSCNLT(EWDBid * pidChan, char * szSta, 
                                 char * szComp, char * szNet, char * szLoc, 
                                 double tStart, double tEnd);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY COOKED_INFRASTRUCTURE_API

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_api_CreateComponent

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME pidComp
PARAM_TYPE EWDBid *
PARAM_DESCRIPTION A pointer to a EWDBid where the function will write
The DB identifier of the Component it created.

PARAMETER 2
PARAM_NAME sSta
PARAM_TYPE char *
PARAM_DESCRIPTION SEEDlike Station code for the component.  It may not
be blank or NULL, and should be a maximum of 7 characters.

PARAMETER 3
PARAM_NAME sComp
PARAM_TYPE char *
PARAM_DESCRIPTION SEEDlike Component code for the component.  The field
may not be blank or NULL and should be a maximum of 9 characters.

PARAMETER 4
PARAM_NAME sNet
PARAM_TYPE char *
PARAM_DESCRIPTION SEEDlike Network code for the component.  It may not
be blank or NULL and should be a maximum of 9 characters.

PARAMETER 5
PARAM_NAME sLoc
PARAM_TYPE char *
PARAM_DESCRIPTION Optional SEEDlike Location code for the component. It
may not be blank or NULL and should be a maximum of 9 characters.

PARAMETER 6
PARAM_NAME szComment
PARAM_TYPE char *
PARAM_DESCRIPTION Optional comment regarding the component.  May be
blank, and should be a maximum of 4k characters.  It may not be NULL!

DESCRIPTION Function creates a component sensor in the DB with an SCN
and optionally L.  It associates the component with a site identified
by (Sta,Net).  It returns the DB ID of the newly created component.  If
a component already exists with the specified SCNL, then a new
component is not created and the idComp of the existing component is
returned.

NOTE A component(Comp) represents a seismic sensor in the field.  It is
represented by SCN and optionaly L, and has location
properties(Lat,Lon, and Elev) and orientation properties(Azimuth and
Dip).  
<br><br>
If a component with matching SCNL already exists, then the call will
succeed and return the idComp of the existing component.  A new
component will not be created!
<br><br>
The comment is currently ignored.

*************************************************
************************************************/
int ewdb_api_CreateComponent(EWDBid *pidComp, char * sSta, char * sComp,
                             char * sNet, char * sLoc, char * szComment);



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY COOKED_INFRASTRUCTURE_API

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_api_CreateTransformFunction 

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME pidCookedTF
PARAM_TYPE EWDBid *
PARAM_DESCRIPTION Pointer to an EWDBid where the function will write the
DB ID of the newly created poles and zeroes based cooked transform
function(CookedTF).

PARAMETER 2
PARAM_NAME pCookedTF
PARAM_TYPE EWDB_TransformFunctionStruct *
PARAM_DESCRIPTION Pointer to a caller filled
EWDB_TransformFunctionStruct that contains the transform function and
associated information that the caller wants to insert into the DB.

DESCRIPTION Function inserts a caller provided poles/zeroes based
transform function into the DB.  The transform Function is used to
transform recorded waveforms back into ground motion.  The function
writes the DB id of the newly created transform function to
pidCookedTF.

NOTE The transform function can be associated with multipe channel time
intervals using EWDB_SetTransformFuncForChanT().

*************************************************
************************************************/
int ewdb_api_CreateTransformFunction(EWDBid * pidCookedTF,
                                     EWDB_TransformFunctionStruct * pCookedTF);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY COOKED_INFRASTRUCTURE_API

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_api_GetComponentInfo

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME idChan
PARAM_TYPE EWDBid
PARAM_DESCRIPTION The DB identifier of the channel for which the caller
wants information.

PARAMETER 2
PARAM_NAME tParamsTime
PARAM_TYPE int
PARAM_DESCRIPTION Point in time for which the caller wants component
parameter information.  A time is required because some parameters are
variable over time.

PARAMETER 3
PARAM_NAME pStation
PARAM_TYPE EWDB_StationStruct *
PARAM_DESCRIPTION A pointer to a EWDB_StationStruct (allocated by the caller)
where the function will write the component parameter information for 
the given channel and time.

DESCRIPTION Function retrieves a EWDB_StationStruct full of information
about the component associated with a channel at a give time.

*************************************************
************************************************/
int ewdb_api_GetComponentInfo(EWDBid idChan, int tParamsTime,
                              EWDB_StationStruct * pStation);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY COOKED_INFRASTRUCTURE_API

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_api_GetStationList

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Returned when the caller passes an invalid set of
criteria.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Partial success.  A list of stations was retrieved,
but the caller's buffer was not large enough to accomadate all of the
stations found.  See pNumStationsFound for the number of stations found
and pNumStationsRetrieved for the number of stations placed in the
caller's buffer.

PARAMETER 1
PARAM_NAME MinLat
PARAM_TYPE double
PARAM_DESCRIPTION Minimum lattitude coordinate of stations
to be retrieved. Lattitude is expressed in degrees.

PARAMETER 2
PARAM_NAME MaxLat
PARAM_TYPE double
PARAM_DESCRIPTION Maximum lattitude coordinate of stations to be
retrieved.  Lattitude is expressed in degrees.

PARAMETER 3
PARAM_NAME MinLon
PARAM_TYPE double
PARAM_DESCRIPTION Minimum longitude coordinate of stations to be
retrieved.  Longitude is expressed in degrees.

PARAMETER 4
PARAM_NAME MaxLon
PARAM_TYPE double
PARAM_DESCRIPTION Maximum longitude coordinate of stations to be
retrieved.  Longitude is expressed in degrees.

PARAMETER 5
PARAM_NAME ReqTime
PARAM_TYPE double
PARAM_DESCRIPTION Time of interest.  The list of active components
changes over time.  Reqtime identifies the historical point in time for
which the caller wants a station list.  Time is expressed in seconds
since 1970.

PARAMETER 6
PARAM_NAME pBuffer
PARAM_TYPE EWDB_StationStruct *
PARAM_DESCRIPTION Buffer allocated by the caller, in which the function
will place information for each of the stations matching the caller's 
criteria.

PARAMETER 7
PARAM_NAME pNumStationsFound
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of stations found to match the calller's criteria.

PARAMETER 8
PARAM_NAME pNumStationsRetrieved
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of arrivals placed in the callers buffer(pBuffer).

PARAMETER 9
PARAM_NAME BufferLen
PARAM_TYPE int
PARAM_DESCRIPTION Size of the pBuffer buffer as a multiple of
EWDB_StationStruct. (example: 15 structs)

DESCRIPTION This function retrieves a list of component/channels given
a lat/lon box and time of interest.  Component attributes for the given
time are included.  (MinLat,MinLon) and (MaxLat,MaxLon)
define two vertices of a box.  The function retrieves the list of
stations within the box.  

NOTE Be careful of the effects of negative Lat/Lon on Min/Max!!
<br><br>
Use ewdb_api_GetStationListWithoutLocation() to retrieve all 
stations available in the database at a given time.  It does not
impose a lat/lon box.

*************************************************
************************************************/
int ewdb_api_GetStationList(double MinLat, double MaxLat, double MinLon,
                            double MaxLon, double ReqTime, 
                            EWDB_StationStruct * pBuffer,
                            int * pNumStationsFound, 
                            int * pNumStationsRetrieved,
                            int BufferLen);



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY COOKED_INFRASTRUCTURE_API

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_api_GetStationListWithoutLocation

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Returned when the caller passes an invalid set of
criteria.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Partial success.  A list of stations was retrieved,
but the caller's buffer was not large enough to accomadate all of the
stations found.  See pNumStationsFound for the number of stations found
and pNumStationsRetrieved for the number of stations placed in the
caller's buffer.

PARAMETER 1
PARAM_NAME ReqTime
PARAM_TYPE double
PARAM_DESCRIPTION Time of interest.  The list of active components
changes over time.  Reqtime identifies the historical point in time for
which the caller wants a station list.  Time is expressed in seconds
since 1970.

PARAMETER 2
PARAM_NAME pBuffer
PARAM_TYPE EWDB_StationStruct *
PARAM_DESCRIPTION Buffer allocated by the caller, in which the function
will place information for each of the stations matching the caller's 
criteria.

PARAMETER 3
PARAM_NAME pNumStationsFound
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of stations found to match the calller's criteria.

PARAMETER 4
PARAM_NAME pNumStationsRetrieved
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of arrivals placed in the callers buffer(pBuffer).

PARAMETER 5
PARAM_NAME BufferLen
PARAM_TYPE int
PARAM_DESCRIPTION Size of the pBuffer buffer as a multiple of
EWDB_StationStruct. (example: 15 structs)

DESCRIPTION This function retrieves a list of component/channels given
a time of interest.  This function is the same as ewdb_api_GetStationList(),
except that it does not impose a Lat/Lon box.

*************************************************
************************************************/
int ewdb_api_GetStationListWithoutLocation(double ReqTime, 
                                           EWDB_StationStruct * pBuffer,
                                           int * pNumStationsFound, 
                                           int * pNumStationsRetrieved,
                                           int BufferLen);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY COOKED_INFRASTRUCTURE_API

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_api_GetidChanT 

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME idChan
PARAM_TYPE EWDBid
PARAM_DESCRIPTION The DB identifier of the channel for which the caller
wants a channel time interval(ChanT) identifier.

PARAMETER 2
PARAM_NAME tTime
PARAM_TYPE double
PARAM_DESCRIPTION Point in time for which the caller wants a time
interval identifier for the channel.

PARAMETER 3
PARAM_NAME pidChanT
PARAM_TYPE EWDBid *
PARAM_DESCRIPTION A pointer to a EWDBid where the function will write
The DB identifier of the requested channel time interval(ChanT).

DESCRIPTION Function retrieves the identifier of the channel time
interval associated with the given channel and time.

*************************************************
************************************************/
int ewdb_api_GetidChanT(EWDBid idChan, double tTime, EWDBid * pidChanT);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY COOKED_INFRASTRUCTURE_API

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_api_GetidChansFromSCNLT

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Partial success.  A list of idChans was retrieved,
but the caller's buffer was not large enough to accomadate all of the
idChans found.  See pNumChansFound for the number of idChans found
and pNumChansRetrieved for the number of chans placed in the
caller's buffer.

PARAMETER 1
PARAM_NAME pBuffer
PARAM_TYPE EWDBid *
PARAM_DESCRIPTION Buffer allocated by the caller, where the function
will write the list idChans that match the given SCNL and Time range.

PARAMETER 2
PARAM_NAME IN_szSta
PARAM_TYPE char *
PARAM_DESCRIPTION SEEDlike Station code for the SNCL.  It may not
be blank or NULL, and should be a maximum of 7 characters.  
<br><br>
"*" may be used as a wildcard that will match all Station codes.

PARAMETER 3
PARAM_NAME IN_szComp
PARAM_TYPE char *
PARAM_DESCRIPTION SEEDlike Component code for the SCNL.  The field
may not be blank or NULL and should be a maximum of 9 characters.
<br><br>
"*" may be used as a wildcard that will match all Component codes.

PARAMETER 4
PARAM_NAME IN_szNet
PARAM_TYPE char *
PARAM_DESCRIPTION SEEDlike Network code for the SCNL.  It may not
be blank or NULL and should be a maximum of 9 characters.
<br><br>
"*" may be used as a wildcard that will match all Network codes.

PARAMETER 5
PARAM_NAME IN_szLoc
PARAM_TYPE char *
PARAM_DESCRIPTION Optional SEEDlike Location code for the component. It
may not be NULL (Use a blank string "" to indicate a NULL location code.
It should be a maximum of 9 characters.
<br><br>
"*" may be used as a wildcard that will match all Location codes.

PARAMETER 6
PARAM_NAME IN_tOff
PARAM_TYPE double *
PARAM_DESCRIPTION End time of the time range of interest.  
Expressed as seconds since 1970.

PARAMETER 7
PARAM_NAME IN_tOn
PARAM_TYPE double *
PARAM_DESCRIPTION Start time of the time range of interest.  
Expressed as seconds since 1970.

PARAMETER 8
PARAM_NAME pNumChansFound
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of idChans found to match the SCNL and time range.

PARAMETER 9
PARAM_NAME pNumChansRetrieved
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of idChans placed in the callers buffer(pBuffer).

PARAMETER 10
PARAM_NAME BufferLen
PARAM_TYPE int
PARAM_DESCRIPTION Size of the pBuffer buffer as a multiple of
EWDBid. (example: 15 EWDBids)


DESCRIPTION The function retrieves a list of idChans that are
valid for the given SCNL and time range.  Wildcards ("*") may
be used for any of the S, C, N, and L codes.  The function will
return all idChans that are associated with the components that
have codes matching the given SCNL, during a time period that
overlaps with (IN_tOn - IN_tOff).

*************************************************
************************************************/
int ewdb_api_GetidChansFromSCNLT(EWDBid * pBuffer,  
                                 char * IN_szSta, char * IN_szComp, 
                                 char * IN_szNet, char * IN_szLoc,
                                 double IN_tOff, double IN_tOn, 
                                 int * pNumChansFound, 
                                 int * pNumChansRetrieved,
                                 int BufferLen);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY COOKED_INFRASTRUCTURE_API

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_api_SetCompParams

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME pStation
PARAM_TYPE EWDB_StationStruct *
PARAM_DESCRIPTION Pointer to a EWDB_StationStruct that contains the
parameters that the caller wishes to set for the component for the
given time interval.

PARAMETER 2
PARAM_NAME tOn
PARAM_TYPE double
PARAM_DESCRIPTION The start of the time interval for which the caller
wants to the component parameters to be valid.  Expressed as seconds
since 1970.

PARAMETER 3
PARAM_NAME tOff
PARAM_TYPE double
PARAM_DESCRIPTION The end of the time interval for which the caller
wants to the component parameters to be valid.  Expressed as seconds
since 1970.

PARAMETER 4
PARAM_NAME pidCompT
PARAM_TYPE EWDBid *
PARAM_DESCRIPTION A pointer to a EWDBid where the function will write
the DB ID of the updated or newly created Component Time Interval.

PARAMETER 5
PARAM_NAME szComment
PARAM_TYPE char *
PARAM_DESCRIPTION Optional comment regarding the Component Time
Interval.  It may be blank, and should be a maximum of 4k characters.
It may not be NULL!

DESCRIPTION Function sets the variable parameters of a component for a
given time interval.  The caller specifies the component by filling in
the SCNL codes in pStation.  Variable parameters include (lat,lon), elevation,
and the orientation of the component.  It will write the DB ID of the
newly created/updated Component Time Interval record to to pidCompT.
  
NOTE pStation->idComp is ignored by this function.  The component must
be specified using the SCNL codes, not by a DB idComp.

*************************************************
************************************************/
int ewdb_api_SetCompParams(EWDB_StationStruct * pStation, 
                           double tOn, double tOff,
                           EWDBid * pidCompT, char * szComment);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY COOKED_INFRASTRUCTURE_API

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_api_SetTransformFuncForChanT 

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME idChanT
PARAM_TYPE EWDBid
PARAM_DESCRIPTION The DB identifier of the channel time interval(ChanT)
with which the caller wants to associate a Poles and Zeroes transform
function(CookedTF).

PARAMETER 2
PARAM_NAME dGain
PARAM_TYPE double
PARAM_DESCRIPTION The signal gain for the channel time interval.
Expressed as a double floating point multiplicative scalar.

PARAMETER 3
PARAM_NAME idCookedTF
PARAM_TYPE EWDBid
PARAM_DESCRIPTION The DB identifier of the existing poles and zeroes
based cooked transfer function, that the caller wants to associate with
a given channel time interval(ChanT).

PARAMETER 4
PARAM_NAME dSampRate
PARAM_TYPE double
PARAM_DESCRIPTION Digital sample rate of the data spouted out of the
channel.  Expressed as samples per second.

DESCRIPTION Function associates an existing transform function with a
Channel for a given time period.  Records the gain and sample-rate that
are specific to the channel.

NOTE To create a transform function for a channel, you must call two 
API functions.  You must first call ewdb_api_CreateTransformFunction() to
create the transfer function.  Then you must call 
ewdb_api_SetTransformFuncForChanT() (using the idCookedTF that you got back 
from the create function call) for each "channel time interval" with 
which you wish to associate the transfer function.

*************************************************
************************************************/
int ewdb_api_SetTransformFuncForChanT(EWDBid idChanT, double dGain,
                                      EWDBid idCookedTF, double dSampRate);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY COOKED_INFRASTRUCTURE_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW

FUNCTION ewdb_api_GetTransformFunctionForChan

SOURCE_LOCATION src/oracle/schema-working/src/infra/ewdb_api_GetTransformFunctionForChan.c

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME idChan
PARAM_TYPE EWDBid
PARAM_DESCRIPTION The DB ID of the channel for
which the caller wants to retrieve a cooked
transform function.

PARAMETER 2
PARAM_NAME tTime
PARAM_TYPE time_t
PARAM_DESCRIPTION Point in time for which the caller wants
a cooked transform function for the channel.
A time is required because the transform function varies
over time as device properties are changed or the
channel is altered.

PARAMETER 3
PARAM_NAME pChanCTF
PARAM_TYPE EWDB_ChanTCTFStruct *
PARAM_DESCRIPTION Pointer to a caller allocated
EWDB_ChanTCTFStruct where the function will write
the cooked transform function and associated "channel
specific response information" requested by the caller.

DESCRIPTION Function retrieves the poles/zeroes based
transform function, gain and sample rate for a given
channel and time.
The transform Function is used to transform recorded
waveforms back into ground motion.

*************************************************
************************************************/
int ewdb_api_GetTransformFunctionForChan(EWDBid idChan, time_t tTime,
                                         EWDB_ChanTCTFStruct * pChanCTF);


/**********************************************************
 #########################################################
    WAVEFORM REQUEST 
 #########################################################
**********************************************************/

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW API FORMATTED COMMENT
TYPE LIBRARY

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM_REQUEST_API

LOCATION THIS_FILE

DESCRIPTION This is the Waveform Request portion of the
EWDB API Library.  The WAVEFORM_REQUEST_API library 
contains the C functions that provide an interface for 
accessing the portion of the Earthworm DBMS that deals
with making waveform requests.  This code deals with
making requests for particular waveform snippets that
may be available from one or more sources (currently
EW wave servers.(wave_serverV)  It does not deal with
actual binary waveforms.  That is handled by the 
WAVEFORM_API portion of the EWDB_API_LIB library.

*************************************************
************************************************/


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM_REQUEST_API

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_api_DeleteSnippetRequest

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME IN_idSnipReq
PARAM_TYPE EWDBid
PARAM_DESCRIPTION DB ID of an existing SnippetRequest which the
caller wishes to delete.

PARAMETER 2
PARAM_NAME IN_iLockTime
PARAM_TYPE int
PARAM_DESCRIPTION The lock value of the given request.  This
should be the time the Request was locked.  This value can 
be obtained via the EWDB_SnippetRequestStruct that was used
in the call to lock the request.  If the request is not locked,
then this value is ignored.


DESCRIPTION The Function deletes the snippet request and any
supporting information from the database.

*************************************************
************************************************/
int ewdb_api_DeleteSnippetRequest (EWDBid IN_idSnipReq, int IN_iLockTime);




/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM_REQUEST_API

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_api_GetListOfOldSnipReqs

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Partial success.  A list of snippet requests was retrieved,
but the caller's buffer was not large enough to accomadate all of the
requests found.  See pNumItemsFound for the number of arrivals found
and pNumItemsRetrieved for the number of requests placed in the
caller's buffer.


PARAMETER 1
PARAM_NAME pBuffer
PARAM_TYPE EWDB_SnippetRequestStruct *
PARAM_DESCRIPTION Buffer allocated by the caller, where the function
will write the list of snippet requests that are active as of tThreshold.

PARAMETER 2
PARAM_NAME iLockTimeThreshold
PARAM_TYPE int
PARAM_DESCRIPTION Threshold time for determining whether snippet requests
are old or not.  If the lock time of the request is earlier than 
iLockTimeThreshold, then the request is considered old. 
(Expressed as seconds since 1970)

 
PARAMETER 3
PARAM_NAME pNumItemsFound
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of snippet reuqests found.

PARAMETER 4
PARAM_NAME pNumItemsRetrieved
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of snippet requests placed in the caller's buffer(pBuffer).

PARAMETER 5
PARAM_NAME BufferLen
PARAM_TYPE int
PARAM_DESCRIPTION Size of the pBuffer buffer as a multiple of
EWDB_SnippetRequestStruct. (example: 15 structs)

DESCRIPTION THIS FUNCTION IS NOT FOR GENERAL CONSUMPTION.  The function 
retrieves a list of snippet requests with OLD locks.  It is for use
by programs auditing the operation of the concierge system and should
not be used in the mainstream processing of data.

NOTE  THIS FUNCTION IS NOT FOR GENERAL CONSUMPTION.  It is for auditing
an problem detection. The function retrieves only the SCNL portion of the
EWDB_SnippetRequestStruct.ComponentInfo.


*************************************************
************************************************/
int ewdb_api_GetListOfOldSnipReqs(EWDB_SnippetRequestStruct * pBuffer,
                                             int iLockTimeThreshold,
                                             int * pNumItemsFound, 
                                             int * pNumItemsRetrieved,
                                             int BufferLen);

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM_REQUEST_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW


FUNCTION ewdb_api_GetSnippetRequestList

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Partial success.  A list of snippet requests was retrieved,
but the caller's buffer was not large enough to accomadate all of the
requests found.  See pNumItemsFound for the number of arrivals found
and pNumItemsRetrieved for the number of requests placed in the
caller's buffer.

PARAMETER 1
PARAM_NAME pBuffer
PARAM_TYPE EWDB_SnippetRequestStruct *
PARAM_DESCRIPTION Buffer allocated by the caller, where the function
will write the list of snippet requests that are active as of tThreshold.
If you want to retrieve a list of request, then you MUST initialize 
pBuffer[0].idSnipReq to 0.  Otherwise, the function will only attempt to 
retrieve the one SnippetRequest identified by pBuffer[0].idSnipReq 
regardless of the other parameters.

PARAMETER 2
PARAM_NAME tThreshold
PARAM_TYPE time_t
PARAM_DESCRIPTION Threshold time for determining whether snippet requests
are active or not.  If the next scheduled attempt for retrieving a 
snippet requests is earlier than tThreshold, then the request is 
considered active.  Only active requests are returned, in increasing order 
of next scheduled attempt.  (Expressed as seconds since 1970)

PARAMETER 3
PARAM_NAME iRequestGroup
PARAM_TYPE int
PARAM_DESCRIPTION Request group for which the caller is interested
in snippet requests.  Set this to -1 to get all Snippet Requests regardless
of Request Group.  See EWDB_SnippetRequestStruct for a description of
RequestGroup.

PARAMETER 4
PARAM_NAME pNumItemsFound
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of snippet reuqests found.

PARAMETER 5
PARAM_NAME pNumItemsRetrieved
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of snippet requests placed in the caller's buffer(pBuffer).

PARAMETER 6
PARAM_NAME BufferLen
PARAM_TYPE int
PARAM_DESCRIPTION Size of the pBuffer buffer as a multiple of
EWDB_SnippetRequestStruct. (example: 15 structs)

DESCRIPTION The function retrieves the list of active snippet 
requests (as of time tThreshold) from the DB.  A Snippet Request
is deemed active if its next scheduled retrieval attempt is 
prior to tThreshold.  Requests are returned in order of 
increasing time of next retrieval attempt, so the most overdue
ones should be at the top of the list.  Note that only the snippets
for the given RequestGroup are returned, unless the RequestGroup
is set to a special value.
<p>
The function retrieves only the SCNL portion of the
EWDB_SnippetRequestStruct.ComponentInfo.
<p>
The function can be used to retrieve a single SnippetRequest by
idSnipReq.  See the (PARAMETER 1 pBuffer) documentation for 
more info.

NOTE  THIS CALL LOCKS THE SnippetRequests THAT IT RETRIEVES.
You MUST UNLOCK these requests using ewdb_api_UpdateSnippetRequest()
or ewdb_api_DeleteSnippetRequest() after you have finished with them.
*************************************************
************************************************/
int ewdb_api_GetSnippetRequestList(EWDB_SnippetRequestStruct * pBuffer,
                                   time_t tThreshold,
                                   int iRequestGroup,
                                   int * pNumItemsFound, 
                                   int * pNumItemsRetrieved,
                                   int BufferLen);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM_REQUEST_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW


FUNCTION ewdb_api_UpdateSnippetRequest

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME IN_pSnipReq
PARAM_TYPE EWDB_SnippetRequestStruct *
PARAM_DESCRIPTION Pointer to a EWDB_SnippetRequestStruct, where the
caller puts the parameters of the SnippetRequest that they wish to
update.

PARAMETER 2
PARAM_NAME IN_bModifyAttemptParams
PARAM_TYPE int
PARAM_DESCRIPTION TRUE/FALSE  flag indicating whether the attempt should
be recorded.  If true, the DB will increment the counter that tracks
how many times the request has been attempted. (VERY SIMPLE)

PARAMETER 3
PARAM_NAME IN_bModifyResultParams
PARAM_TYPE int
PARAM_DESCRIPTION TRUE/FALSE flag indicating whether or not any new
snippet data was found, and thus whether the snippetrequest should 
be updated (new tStart or tEnd, idExistingWaveform).

DESCRIPTION The function updates a snippet request.  This function
acts as a recording mechanism for the request processor.  It allows
the processor to record that it tried to retrieve a snippet, and it
either got part or none of the desired snippet.  <br>
The function always updates the tNextAttempt time in the database 
with the value of IN_pSnipReq->tNextAttempt.<br>
If IN_bModifyAttemptParams is true, then the DB updates the counter
that track the number of attempts so far. <br>
If IN_bModifyResultParams is true, then the DB will update tStart,
tEnd, and idExistingWaveform if appropriate.

NOTE  Set IN_pSnipReq->idExistingWaveform to 0 if a partial snippet was 
not retrieved.  If a partial snippet was retrieved, first call 
ewdb_api_CreateWaveform() to store the snippet in the database, 
then call this function using the idWaveform that you got back 
from the create call.

*************************************************
************************************************/
int ewdb_api_UpdateSnippetRequest(EWDB_SnippetRequestStruct * IN_pSnipReq,  
                                  int IN_bModifyAttemptParams,
                                  int IN_bModifyResultParams);



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM_REQUEST_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW


FUNCTION ewdb_api_UpdateSnippetRequestControlParams

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME IN_pSnipReq
PARAM_TYPE EWDB_SnippetRequestStruct *
PARAM_DESCRIPTION Pointer to a EWDB_SnippetRequestStruct, where the
caller puts the parameters of the SnippetRequest that they wish to
update.

DESCRIPTION The function updates the control parameters for a snippet request.  
At this time, the control params are:<br>
 iNumAttempts(the number of times the request will attempt to be fulfilled), and<br>
 iRequestGroup(the request group in which the request has been categorized).<br>
 
NOTE IN_pSnipReq->iLockTime must match the LockTime for the request (and the request
must be locked).  (You must lock the request before you can modify the control params
and you must remember to unlock it afterwards.)

*************************************************
************************************************/
int ewdb_api_UpdateSnippetRequestControlParams(EWDB_SnippetRequestStruct * IN_pSnipReq);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM_REQUEST_API

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_api_ProcessSnippetReqs

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME szSta
PARAM_TYPE char *
PARAM_DESCRIPTION SEEDlike Station code of the channels for which
the caller wishes to create snippet requests.  <br>
This param CANNOT be NULL or blank.
Use the wildcard string "*" to match all possible Station codes.

PARAMETER 2
PARAM_NAME szComp
PARAM_TYPE char *
PARAM_DESCRIPTION SEEDlike Component code of the channels for which
the caller wishes to create snippet requests.  <br>
This param CANNOT be NULL or blank.
Use the wildcard string "*" to match all possible Component codes.

PARAMETER 3
PARAM_NAME szNet
PARAM_TYPE char *
PARAM_DESCRIPTION SEEDlike Network code of the channels for which
the caller wishes to create snippet requests.  <br>
This param CANNOT be NULL or blank.
Use the wildcard string "*" to match all possible Network codes.

PARAMETER 4
PARAM_NAME szLoc
PARAM_TYPE char *
PARAM_DESCRIPTION SEEDlike Location code of the channels for which
the caller wishes to create snippet requests.  <br>
This param CANNOT be NULL.  It may be blank("").
Use the wildcard string "*" to match all possible Location codes.

PARAMETER 5
PARAM_NAME tStart
PARAM_TYPE double
PARAM_DESCRIPTION The start of the time interval for which
the caller wants a snippet for the given SCNL.
Expressed as seconds since 1970.

PARAMETER 6
PARAM_NAME tEnd
PARAM_TYPE double
PARAM_DESCRIPTION The end of the time interval for which
the caller wants a snippet for the given SCNL.
Expressed as seconds since 1970.

PARAMETER 7
PARAM_NAME idEvent
PARAM_TYPE EWDBid
PARAM_DESCRIPTION The EWDB event identifer that the
any resulting snippets should be associated with.

PARAMETER 8
PARAM_NAME iNumAttempts
PARAM_TYPE int
PARAM_DESCRIPTION The number of times that the caller 
wishes to have the request attempted.  It is up to the
data retriever to enforce/interpret this number.

PARAMETER 9
PARAM_NAME iRequestGroup
PARAM_TYPE int
PARAM_DESCRIPTION The Request Group that the caller wishes
the request associated with.  Requests may be divided into 
groups for more efficient retrieval.  This param may be later
modified/ignored by the data retriever.

DESCRIPTION The Function creates a Snippet Request record in the DB, for
the given time range, for each channel that matches the given SCNL codes.
The record describes snippet data that the caller wants stored in the DB.
(This call does not retrieve snippet data, it only records the request.)
A snippet is essentially a piece of data from a given channel for a given
time interval.  If idEvent is specified as non 0, then any snippets
recovered as a result of the requests will be associated
the given idEvent.

NOTE  This function does not retrieve any waveform data!
It only creates requests for waveform data.  The requests
must be handled by another program(concierge).

*************************************************
************************************************/
int ewdb_api_ProcessSnippetReqs(char * szSta,char * szComp, 
                                char * szNet, char * szLoc,
                                double tStart, double tEnd, EWDBid idEvent,
                                int iNumAttempts, int iRequestGroup);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY WAVEFORM_REQUEST_API

LANGUAGE C

LOCATION THIS_FILE

STABILITY NEW


FUNCTION ewdb_api_GetNextScheduledSnippetRetrievalAttempt

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION There were no pending snippet requests in the DB.

PARAMETER 1
PARAM_NAME ptNextAttempt
PARAM_TYPE time_t *
PARAM_DESCRIPTION Pointer to a time_t variable supplied by the caller
where the function will write the next scheduled attempt time of all
the pending snippet requests in the DB.  (assuming that the function
executes successfully and returns EWDB_RETURN_SUCCESS)

DESCRIPTION The function checks the snippet requests in the database
to see when the next scheduled attempt is among all of the pending
requests.  The requests are schedule by the DB to be attempted at
intervals.  The function looks up the first scheduled attempt time.

*************************************************
************************************************/
int ewdb_api_GetNextScheduledSnippetRetrievalAttempt(time_t * ptNextAttempt);


/**********************************************************
 #########################################################
    STRONG MOTION 
 #########################################################
**********************************************************/

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY STRONG_MOTION_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_PutSMMessage

SOURCE_LOCATION ewdb_api_PutSMMessage.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Warning.  The message was not inserted 
because the DB had no record of the channel or else the
Lat/Lon of the component from which the channel came.  The
channel check is done based on the SCNL from the message.
See the logfile for the contents of the message, and for more
info on why the message was not inserted.


PARAMETER 1
PARAM_NAME pMessage
PARAM_TYPE SM_INFO *
PARAM_DESCRIPTION  A pointer to a RW_STRONGMOTIONII message
(in SM_INFO form), that the caller wishes to insert into the
EW DBMS.  

PARAMETER 2
PARAM_NAME idEvent
PARAM_TYPE EWDBid 
PARAM_DESCRIPTION DB Identifier of the event(that already exists 
in the DB)  that this strong motion message should be associated 
with.  The caller should set idEvent to 0 if they do not want 
this message associated with an Event.  

DESCRIPTION This function stores a strong motion message in the
DB and returns the idSMMessage of the new record.  If idEvent is 
a positive number, then the function will attempt to bind the new 
SMMessage to the Event identified by idEvent.  

NOTE Be sure to properly initialize idEvent.  If a seemingly valid 
idEvent is passed, and no such Event exists, the function will fail, 
even though it can create the SMMessage without problems.
If you do not have an idEvent, but have an author and an author's
EventID, then you can use ewdb_api_CreateEvent() to get an idEvent
from an author and author's EventID.

*************************************************
************************************************/
int ewdb_api_PutSMMessage(SM_INFO * pMessage, EWDBid idEvent);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY STRONG_MOTION_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_DeleteSMMessage

SOURCE_LOCATION ewdb_api_DeleteSMMessage.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.


PARAMETER 1
PARAM_NAME idSMMessage
PARAM_TYPE EWDBid 
PARAM_DESCRIPTION The DB id of the message that the caller 
wishes to delete.

DESCRIPTION This function dissassociates a strong motion message
with any Events, and then deletes the message.

*************************************************
************************************************/
int ewdb_api_DeleteSMMessage(EWDBid idSMMessage);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY STRONG_MOTION_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_GetSMData

SOURCE_LOCATION ewdb_api_GetSMData.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Partial Success.  The function successfully
executed, but either 1)the caller's buffer wasn't large enough 
to retrieve all of the messages, and/or 2) there was a problem
retrieving one or more of the messages.  If the value written
to pNumRecordsFound at the completion of the function is greater
than the iBufferRecLen parameter passed to the function, then
more records were found than the caller's buffer could hold.
If the value written to pNumRecordsRetrieved is less than 
the iBufferRecLen parameter passed to the function, then the
function failed to retrieve atleast one of the messages that
it found.


PARAMETER 1
PARAM_NAME pcsCriteria
PARAM_TYPE EWDB_CriteriaStruct * 
PARAM_DESCRIPTION  The criteria struct for specifying criteria
types and values for the query to retrieve messages.  
pcsCriteria->Criteria, contains the flags which indicate
which criteria to use in searching for messages.  NOTE: The 
Depth criteria is not supported by this function!  See 
EWDB_CriteriaStruct for more detail on criteria.

PARAMETER 2
PARAM_NAME szSta
PARAM_TYPE char * 
PARAM_DESCRIPTION The Station name criteria for retrieving 
messages.  This param is only used for criteria if 
pcsCriteria->Criteria includes the EWDB_CRITERIA_USE_SCNL
flag.  '*' may be used as a wildcard.  This param must 
always be a valid pointer, the function may fail if it
is set to NULL.

PARAMETER 3
PARAM_NAME szComp
PARAM_TYPE char * 
PARAM_DESCRIPTION The Component name criteria for retrieving 
messages.  This param is only used for criteria if 
pcsCriteria->Criteria includes the EWDB_CRITERIA_USE_SCNL
flag.  '*' may be used as a wildcard.  This param must 
always be a valid pointer, the function may fail if it
is set to NULL.

PARAMETER 4
PARAM_NAME szNet
PARAM_TYPE char * 
PARAM_DESCRIPTION The Network name criteria for retrieving 
messages.  This param is only used for criteria if 
pcsCriteria->Criteria includes the EWDB_CRITERIA_USE_SCNL
flag.  '*' may be used as a wildcard.  This param must 
always be a valid pointer, the function may fail if it
is set to NULL.

PARAMETER 5
PARAM_NAME szLoc
PARAM_TYPE char * 
PARAM_DESCRIPTION The Location name criteria for retrieving 
messages.  This param is only used for criteria if 
pcsCriteria->Criteria includes the EWDB_CRITERIA_USE_SCNL
flag.  '*' may be used as a wildcard.  This param must 
always be a valid pointer, the function may fail if it
is set to NULL.

PARAMETER 6
PARAM_NAME idEvent
PARAM_TYPE EWDBid 
PARAM_DESCRIPTION The Event criteria for retrieving 
messages.  This param is only used for criteria if 
pcsCriteria->Criteria includes the EWDB_CRITERIA_USE_IDEVENT
flag.  This param must be set to a valid(existing) idEvent,
if pcsCriteria->Criteria includes the EWDB_CRITERIA_USE_IDEVENT 
flag, or the function will fail.

PARAMETER 7
PARAM_NAME iEventAssocFlag
PARAM_TYPE int 
PARAM_DESCRIPTION  A flag indicating how the function should 
retrieve strong motion messages with respect to event association.
If pcsCriteria->Criteria includes the EWDB_CRITERIA_USE_IDEVENT flag,
then this parameter is ignored;  otherwise, if this parameter is set
to EWDB_SM_SEARCH_FOR_ALL_SMMESSAGES, then all messages meeting the
criteria will be retrieved, otherwise if this param is set to 
EWDB_SM_SEARCH_FOR_ALL_UNASSOCIATED_MESSAGES, then only messages
(meeting the criteria) that are NOT associated with an event, will
be retrieved.

PARAMETER 8
PARAM_NAME pSMI
PARAM_TYPE SM_INFO * 
PARAM_DESCRIPTION  A buffer allocated by the caller, where the
function will place the retrieved messages.  

PARAMETER 9
PARAM_NAME BufferRecLen
PARAM_TYPE int 
PARAM_DESCRIPTION  The length of the buffer(pBuffer) in terms of
SM_INFO records.

PARAMETER 10
PARAM_NAME pNumRecordsFound
PARAM_TYPE int * 
PARAM_DESCRIPTION  A pointer to an int where the function will
write the number of messages found to meet the given criteria.
NOTE:  The number found is not the number retrieved and written
to the caller's buffer.  That is NumRecordsRetrieved.  If the
number of messages found is greater than the caller's buffer will
hold, then the function will return a warning.

PARAMETER 11
PARAM_NAME pNumRecordsRetrieved
PARAM_TYPE int * 
PARAM_DESCRIPTION  A pointer to an int where the function will
write the number of messages actualy retrieved by the function.
This number will be less than or equal to the number found.

DESCRIPTION This function retrieves strong motion messages 
from the DB that meet the criteria given by the caller.  

*************************************************
************************************************/
int ewdb_api_GetSMData(EWDB_CriteriaStruct * pcsCriteria,
                       char * szSta, char * szComp,
                       char * szNet, char * szLoc,
                       EWDBid idEvent, int iEventAssocFlag,
                       SM_INFO * pSMI, int BufferRecLen,
                       int * pNumRecordsFound, int * pNumRecordsRetrieved);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE FUNCTION_PROTOTYPE

LIBRARY  EWDB_API_LIB

SUB_LIBRARY STRONG_MOTION_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_GetSMDataWithChannelInfo

SOURCE_LOCATION ewdb_api_GetSMDataWithChannelInfo.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Partial Success.  See ewdb_api_GetSMData().


PARAMETER 1
PARAM_NAME pcsCriteria
PARAM_TYPE EWDB_CriteriaStruct * 

PARAMETER 2
PARAM_NAME szSta
PARAM_TYPE char * 

PARAMETER 3
PARAM_NAME szComp
PARAM_TYPE char * 

PARAMETER 4
PARAM_NAME szNet
PARAM_TYPE char * 

PARAMETER 5
PARAM_NAME szLoc
PARAM_TYPE char * 

PARAMETER 6
PARAM_NAME idEvent
PARAM_TYPE EWDBid 

PARAMETER 7
PARAM_NAME iEventAssocFlag
PARAM_TYPE int 

PARAMETER 8
PARAM_NAME pSMCAS
PARAM_TYPE EWDB_SMChanAllStruct * 
PARAM_DESCRIPTION  A buffer allocated by the caller, where the
function will place the retrieved messages.  

PARAMETER 9
PARAM_NAME BufferRecLen
PARAM_TYPE int 

PARAMETER 10
PARAM_NAME pNumRecordsFound
PARAM_TYPE int * 

PARAMETER 11
PARAM_NAME pNumRecordsRetrieved
PARAM_TYPE int * 

DESCRIPTION This function retrieves strong motion messages and
associated info from the DB, for all messages that meet 
the criteria given by the caller.  This function retrieves the 
strong motion messages and Channel and Event association 
information for each message.

NOTE This function is the same as ewdb_api_GetSMData(), except where
noted above.  Please see ewdb_api_GetSMData() for better documentation
on each of the parameters.

*************************************************
************************************************/
int ewdb_api_GetSMDataWithChannelInfo(EWDB_CriteriaStruct * pcsCriteria,
                                      char * szSta, char * szComp,
                                      char * szNet, char * szLoc,
                                      EWDBid idEvent, int iEventAssocFlag,
                                      EWDB_SMChanAllStruct * pSMCAS, 
                                      int BufferRecLen,
                                      int * pNumRecordsFound, 
                                      int * pNumRecordsRetrieved);



/**********************************************************
 #########################################################
    ALARM SCHEMA
 #########################################################
**********************************************************/



/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_CreateAlarmAudit

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_CreateAlarmAudit.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

PARAMETER 1
PARAM_NAME pAudit
PARAM_TYPE EWDB_AlarmAuditStruct * 
PARAM_DESCRIPTION Pointed to the structure containing information
about the audit entry to be created.

DESCRIPTION Creates or updates an alarm audit entry. If idAudit
value in the structure is set, it updates the values for the
audit. Otherwise, it creates a new audit entry.

*************************************************
************************************************/
int ewdb_api_CreateAlarmAudit(EWDB_AlarmAuditStruct *pAudit);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_CreateAlarmsCriteria

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_CreateAlarmsCriteria.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

PARAMETER 1
PARAM_NAME pCrit
PARAM_TYPE EWDB_AlarmsCritProgramStruct * 
PARAM_DESCRIPTION Pointer to the structure containing the information
about a criteria program to insert into the database.

DESCRIPTION Creates or updates a criteria program entry in the database. 
If idCritProgram value in the structure is set, it updates the values 
for the program. Otherwise, it creates a new program entry.

*************************************************
************************************************/
int ewdb_api_CreateAlarmsCriteria(EWDB_AlarmsCritProgramStruct *pCrit);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_CreateAlarmsFormat

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_CreateAlarmsFormat.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

PARAMETER 1
PARAM_NAME pForm
PARAM_TYPE EWDB_AlarmsFormatStruct * 
PARAM_DESCRIPTION Pointer to the structure containing the information
about a format to insert into the database.

DESCRIPTION Creates or updates a format entry in the database. 
If idFormat value in the structure is set, it updates the values 
for the format. Otherwise, it creates a new format entry.

*************************************************
************************************************/
int ewdb_api_CreateAlarmsFormat(EWDB_AlarmsFormatStruct *pForm);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_CreateAlarmsRule

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_CreateAlarmsRule.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

PARAMETER 1
PARAM_NAME pRule
PARAM_TYPE EWDB_AlarmsRuleStruct * 
PARAM_DESCRIPTION Pointer to the structure containing the information
about the rule to insert into the database.

PARAMETER 2
PARAM_NAME pDel
PARAM_TYPE EWDB_AlarmDeliveryUnionStruct * 
PARAM_DESCRIPTION Pointer to the structure containing the information
about the delivery mechanism to associate with this rule.

DESCRIPTION Creates or updates a rule entry in the database. 
If idRule value in the pRule structure is set, it updates the values 
for the current rule. Otherwise, it creates a new rule entry.

*************************************************
************************************************/
int ewdb_api_CreateAlarmsRule(EWDB_AlarmsRuleStruct *pRule, 
                              EWDB_AlarmDeliveryUnionStruct *pDel);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_CreateAlarmsRecipient

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_CreateAlarmsRecipient.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

PARAMETER 1
PARAM_NAME pRecipient
PARAM_TYPE EWDB_AlarmsRecipientStruct * 
PARAM_DESCRIPTION Pointer to the structure containing the information
about a recipient to insert into the database.

DESCRIPTION Creates or updates a recipient entry in the database. 
If idRecipient value in the structure is set, it updates the values 
for the current recipient. Otherwise, it creates a new recipient entry.

*************************************************
************************************************/
int ewdb_api_CreateAlarmsRecipient(EWDB_AlarmsRecipientStruct *pRecipient);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_CreateCustomDelivery

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_CreateCustomDelivery.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

PARAMETER 1
PARAM_NAME *pidRecipientDelivery
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to the database ID of the recipient delivery for 
this delivery mechanism returned by this function.

PARAMETER 2
PARAM_NAME idRecipient
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the recipient for whom this delivery is created.

PARAMETER 3
PARAM_NAME *pCustom
PARAM_TYPE EWDB_CustomDeliveryStruct 
PARAM_DESCRIPTION Information about the delivery to insert.

PARAMETER 4
PARAM_NAME isAudit
PARAM_TYPE int 
PARAM_DESCRIPTION If 1, the values should be inserted into the alarms audit
tables, otherwise they should go to the actual alarm tables.

DESCRIPTION If the current delivery is already in the database, it updates
its values. Otherwise, it creates a new delivery and returns the database
ID of the recipient delivery value (pidRecipientDelivery).

*************************************************
************************************************/
int ewdb_api_CreateCustomDelivery(int *pidRecipientDelivery,
                                  int idRecipient, 
                                  EWDB_CustomDeliveryStruct *pCustom, 
                                  int isAudit);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_CreateEmailDelivery

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_CreateEmailDelivery.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

PARAMETER 1
PARAM_NAME *pidRecipientDelivery
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to the database ID of the recipient delivery for 
this delivery mechanism returned by this function.

PARAMETER 2
PARAM_NAME idRecipient
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the recipient for whom this delivery is created.

PARAMETER 3
PARAM_NAME *pEmail
PARAM_TYPE EWDB_EmailDeliveryStruct 
PARAM_DESCRIPTION Information about the delivery to insert.

PARAMETER 4
PARAM_NAME isAudit
PARAM_TYPE int 
PARAM_DESCRIPTION If 1, the values should be inserted into the alarms audit
tables, otherwise they should go to the actual alarm tables.

DESCRIPTION If the current delivery is already in the database, it updates
its values. Otherwise, it creates a new delivery and returns the database
ID of the recipient delivery value (pidRecipientDelivery).

*************************************************
************************************************/
int ewdb_api_CreateEmailDelivery(int *pidRecipientDelivery,
                                 int idRecipient, 
                                 EWDB_EmailDeliveryStruct *pEmail, 
                                 int isAudit);

/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_CreatePagerDelivery

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_CreatePagerDelivery.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

PARAMETER 1
PARAM_NAME *pidRecipientDelivery
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to the database ID of the recipient delivery for 
this delivery mechanism returned by this function.

PARAMETER 2
PARAM_NAME idRecipient
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the recipient for whom this delivery is created.

PARAMETER 3
PARAM_NAME *pPager
PARAM_TYPE EWDB_PagerDeliveryStruct 
PARAM_DESCRIPTION Information about the delivery to insert.

PARAMETER 4
PARAM_NAME isAudit
PARAM_TYPE int 
PARAM_DESCRIPTION If 1, the values should be inserted into the alarms audit
tables, otherwise they should go to the actual alarm tables.

DESCRIPTION If the current delivery is already in the database, it updates
its values. Otherwise, it creates a new delivery and returns the database
ID of the recipient delivery value (pidRecipientDelivery).

*************************************************
************************************************/
int ewdb_api_CreatePagerDelivery(int *pidRecipientDelivery,
                                 int idRecipient, 
                                 EWDB_PagerDeliveryStruct *pPager, 
                                 int isAudit);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_CreatePhoneDelivery

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_CreatePhoneDelivery.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

PARAMETER 1
PARAM_NAME *pidRecipientDelivery
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to the database ID of the recipient delivery for 
this delivery mechanism returned by this function.

PARAMETER 2
PARAM_NAME idRecipient
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the recipient for whom this delivery is created.

PARAMETER 3
PARAM_NAME *pPhone
PARAM_TYPE EWDB_PhoneDeliveryStruct 
PARAM_DESCRIPTION Information about the delivery to insert.

PARAMETER 4
PARAM_NAME isAudit
PARAM_TYPE int 
PARAM_DESCRIPTION If 1, the values should be inserted into the alarms audit
tables, otherwise they should go to the actual alarm tables.

DESCRIPTION If the current delivery is already in the database, it updates
its values. Otherwise, it creates a new delivery and returns the database
ID of the recipient delivery value (pidRecipientDelivery).

*************************************************
************************************************/
int ewdb_api_CreatePhoneDelivery(int *pidRecipientDelivery,
                                 int idRecipient, 
                                 EWDB_PhoneDeliveryStruct *pPhone, 
                                 int isAudit);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_CreateQddsDelivery

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_CreateQddsDelivery.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

PARAMETER 1
PARAM_NAME *pidRecipientDelivery
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to the database ID of the recipient delivery for 
this delivery mechanism returned by this function.

PARAMETER 2
PARAM_NAME idRecipient
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the recipient for whom this delivery is created.

PARAMETER 3
PARAM_NAME *pQdds
PARAM_TYPE EWDB_QddsDeliveryStruct 
PARAM_DESCRIPTION Information about the delivery to insert.

PARAMETER 4
PARAM_NAME isAudit
PARAM_TYPE int 
PARAM_DESCRIPTION If 1, the values should be inserted into the alarms audit
tables, otherwise they should go to the actual alarm tables.

DESCRIPTION If the current delivery is already in the database, it updates
its values. Otherwise, it creates a new delivery and returns the database
ID of the recipient delivery value (pidRecipientDelivery).

*************************************************
************************************************/
int ewdb_api_CreateQddsDelivery(int *pidRecipientDelivery,
                                int idRecipient, 
                                EWDB_QddsDeliveryStruct *pQdds, int isAudit);



/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_DeleteAlarmsFormat

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_DeleteAlarmsFormat.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

PARAMETER 1
PARAM_NAME idFormat
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the format entry to delete.

DESCRIPTION Permanently deletes the format entry from the database.

*************************************************
************************************************/
int ewdb_api_DeleteAlarmsFormat(int idFormat);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_DeleteAlarmsRule

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_DeleteAlarmsRule.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

PARAMETER 1
PARAM_NAME idRule
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the rule entry to delete.

DESCRIPTION Permanently deletes the rule entry from the database.

*************************************************
************************************************/
int ewdb_api_DeleteAlarmsRule(int idRule);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_DeleteAlarmsRecipient

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_DeleteAlarmsRecipient.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

PARAMETER 1
PARAM_NAME idRule
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the recipient entry to delete.

DESCRIPTION Permanently deletes the recipient entry from the database.

*************************************************
************************************************/
int ewdb_api_DeleteAlarmsRecipient(int idRecipient);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_DeleteCritProgram

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_DeleteCritProgram.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

PARAMETER 1
PARAM_NAME idRule
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the criteria program entry to delete.

DESCRIPTION Permanently deletes the criteria program entry from the database.

*************************************************
************************************************/
int ewdb_api_DeleteCritProgram(int idCritProgram);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_DeleteCustomDelivery

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_DeleteCustomDelivery.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

PARAMETER 1
PARAM_NAME idDelivery
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the delivery to deleted.

DESCRIPTION Permanently deletes the delivery entry from the database.

*************************************************
************************************************/
int ewdb_api_DeleteCustomDelivery(int idDelivery);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_DeleteEmailDelivery

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_DeleteEmailDelivery.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

PARAMETER 1
PARAM_NAME idDelivery
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the delivery to deleted.

DESCRIPTION Permanently deletes the delivery entry from the database.

*************************************************
************************************************/
int ewdb_api_DeleteEmailDelivery(int idDelivery);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_DeletePagerDelivery

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_DeletePagerDelivery.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

PARAMETER 1
PARAM_NAME idDelivery
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the delivery to deleted.

DESCRIPTION Permanently deletes the delivery entry from the database.

*************************************************
************************************************/
int ewdb_api_DeletePagerDelivery(int idDelivery);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_DeletePhoneDelivery

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_DeletePhoneDelivery.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

PARAMETER 1
PARAM_NAME idDelivery
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the delivery to deleted.

DESCRIPTION Permanently deletes the delivery entry from the database.

*************************************************
************************************************/
int ewdb_api_DeletePhoneDelivery(int idDelivery);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_DeleteQddsDelivery

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_DeleteQddsDelivery.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

PARAMETER 1
PARAM_NAME idDelivery
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the delivery to deleted.

DESCRIPTION Permanently deletes the delivery entry from the database.

*************************************************
************************************************/
int ewdb_api_DeleteQddsDelivery(int idDelivery);



/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_GetAlarmsAudit

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_GetAlarmsAudit.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Successful return, but there were more entries found
in the database than could be written into the supplied buffer.

PARAMETER 1
PARAM_NAME idEvent
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the event for which alarm audits
are being retrieved.

PARAMETER 2
PARAM_NAME *pAudit
PARAM_TYPE EWDB_AlarmAuditStruct 
PARAM_DESCRIPTION Preallocated array of audit structures to be filled.

PARAMETER 3
PARAM_NAME *NumFound
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of items found in the database..

PARAMETER 4
PARAM_NAME *NumRetrieved
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of items retrieved from the database..

PARAMETER 5
PARAM_NAME BufferLen
PARAM_TYPE int 
PARAM_DESCRIPTION Length of the input buffer - number of preallocated
structures to be filled.

DESCRIPTION For a given idEvent, retrieves and writes to the pAudit
buffer the information about the alarms that were sent for this event.
It will write up to the BufferLen records, then return the number of 
found and retrieved items so that the calling routine can re-allocate the 
buffer and call again.

*************************************************
************************************************/
int ewdb_api_GetAlarmsAudit(int idEvent, EWDB_AlarmAuditStruct *pAudit,
                            int *NumFound, int *NumRetrieved, int BufferLen);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_GetAlarmsCriteriaList

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_GetAlarmsCriteriaList.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Successful return, but there were more entries found
in the database than could be written into the supplied buffer.

PARAMETER 1
PARAM_NAME idCritProgram
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the criteria program to retrieve. If present,
the function will retrieve the information for that criteria program only. 
Otherwise, all programs are retrieved.

PARAMETER 2
PARAM_NAME *pCrit
PARAM_TYPE EWDB_AlarmAuditStruct 
PARAM_DESCRIPTION Preallocated array of criteria program structures to be filled.

PARAMETER 3
PARAM_NAME *NumFound
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of items found in the database..

PARAMETER 4
PARAM_NAME *NumRetrieved
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of items retrieved from the database..

PARAMETER 5
PARAM_NAME BufferLen
PARAM_TYPE int 
PARAM_DESCRIPTION Length of the input buffer - number of preallocated
structures to be filled.

DESCRIPTION If idCritProgram is greater than 0, the function returns information
about that criteria program only. Otherwise it retrieves and writes to the 
pCrit buffer the information about all criteria programs. 
It will write up to the BufferLen records, then return the number of 
found and retrieved items so that the calling routine can re-allocate the 
buffer and call again.

*************************************************
************************************************/
int ewdb_api_GetAlarmsCriteriaList(int idCritProgram,
                                   EWDB_AlarmsCritProgramStruct *pCrit, 
                                   int *NumFound, int *NumRetrieved, 
                                   int BufferLen);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_GetAlarmsFormats

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_GetAlarmsFormats.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Successful return, but there were more entries found
in the database than could be written into the supplied buffer.

PARAMETER 1
PARAM_NAME idFormat
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the format to retrieve. If present,
the function will retrieve the information for that format only. 
Otherwise, all formats are retrieved.

PARAMETER 2
PARAM_NAME *pFormat
PARAM_TYPE EWDB_AlarmsFormatStruct 
PARAM_DESCRIPTION Preallocated array of format structures to be filled.

PARAMETER 3
PARAM_NAME *NumFound
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of items found in the database..

PARAMETER 4
PARAM_NAME *NumRetrieved
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of items retrieved from the database..

PARAMETER 5
PARAM_NAME BufferLen
PARAM_TYPE int 
PARAM_DESCRIPTION Length of the input buffer - number of preallocated
structures to be filled.

DESCRIPTION If idFormat is greater than 0, the function returns information
about that format only. Otherwise it retrieves and writes to the 
pFormat buffer the information about all formats. 
It will write up to the BufferLen records, then return the number of 
found and retrieved items so that the calling routine can re-allocate the 
buffer and call again.

*************************************************
************************************************/
int ewdb_api_GetAlarmsFormats(int idFormat, EWDB_AlarmsFormatStruct *pFormat, 
                              int *NumFound, int *NumRetrieved, int BufferLen);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_GetAlarmsRules

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_GetAlarmsRules.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Successful return, but there were more entries found
in the database than could be written into the supplied buffer.

PARAMETER 1
PARAM_NAME idRule
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the rule to retrieve. If present,
the function will retrieve the information for that rule only. 
Otherwise, all rules are retrieved.

PARAMETER 2
PARAM_NAME *pRule
PARAM_TYPE EWDB_AlarmsRuleStruct 
PARAM_DESCRIPTION Preallocated array of rule structures to be filled.

PARAMETER 3
PARAM_NAME *NumFound
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of items found in the database..

PARAMETER 4
PARAM_NAME *NumRetrieved
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of items retrieved from the database..

PARAMETER 5
PARAM_NAME BufferLen
PARAM_TYPE int 
PARAM_DESCRIPTION Length of the input buffer - number of preallocated
structures to be filled.

DESCRIPTION If idRule is greater than 0, the function returns information
about that rule only. Otherwise it retrieves and writes to the 
pRule buffer the information about all rules. 
It will write up to the BufferLen records, then return the number of 
found and retrieved items so that the calling routine can re-allocate the 
buffer and call again.

*************************************************
************************************************/
int ewdb_api_GetAlarmsRules(int idRule, EWDB_AlarmsRuleStruct *pRule,
                            int *NumFound, int *NumRetrieved, int BufferLen);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_GetAlarmsRecipientList

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_GetAlarmsRecipientList.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Successful return, but there were more entries found
in the database than could be written into the supplied buffer.

PARAMETER 1
PARAM_NAME idRecipient
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the recipient to retrieve. If present,
the function will retrieve the information for that recipient only. 
Otherwise, all recipients are retrieved.

PARAMETER 2
PARAM_NAME *pRecipients
PARAM_TYPE EWDB_AlarmsRecipientStruct 
PARAM_DESCRIPTION Preallocated array of recipient structures to be filled.

PARAMETER 3
PARAM_NAME *NumFound
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of items found in the database..

PARAMETER 4
PARAM_NAME *NumRetrieved
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of items retrieved from the database..

PARAMETER 5
PARAM_NAME BufferLen
PARAM_TYPE int 
PARAM_DESCRIPTION Length of the input buffer - number of preallocated
structures to be filled.

DESCRIPTION If idRecipient is greater than 0, the function returns information
about that recipient only. Otherwise it retrieves and writes to the 
pRecipient buffer the information about all recipients. 
It will write up to the BufferLen records, then return the number of 
found and retrieved items so that the calling routine can re-allocate the 
buffer and call again.

*************************************************
************************************************/
int ewdb_api_GetAlarmsRecipientList(int idRecipient, 
                                    EWDB_AlarmsRecipientStruct *pRecipients, 
                                    int *NumFound, int *NumRetrieved, 
                                    int BufferLen);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_GetAlarmsRecipientSummary

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_GetAlarmsRecipientSummary.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Successful return, but there were more entries found
in the database than could be written into the supplied buffer.

PARAMETER 1
PARAM_NAME idRecipient
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the recipient to retrieve. If present,
the function will retrieve the summary information for that recipient only. 
Otherwise, all recipients are retrieved.

PARAMETER 2
PARAM_NAME *pSummary
PARAM_TYPE EWDB_AlarmsRecipientStructSummary 
PARAM_DESCRIPTION Preallocated array of summary structures to be filled.

PARAMETER 3
PARAM_NAME *NumFound
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of items found in the database..

PARAMETER 4
PARAM_NAME *NumRetrieved
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of items retrieved from the database..

PARAMETER 5
PARAM_NAME BufferLen
PARAM_TYPE int 
PARAM_DESCRIPTION Length of the input buffer - number of preallocated
structures to be filled.

DESCRIPTION If idRecipient is greater than 0, the function returns summary
information about that recipient only. Otherwise it retrieves and writes to the 
pSummary buffer the summary information about all recipients. 
It will write up to the BufferLen records, then return the number of 
found and retrieved items so that the calling routine can re-allocate the 
buffer and call again.

*************************************************
************************************************/
int ewdb_api_GetAlarmsRecipientSummary(int idRecipient, 
                                       EWDB_AlarmsRecipientStructSummary *pSummary, 
                                       int *NumFound, int *NumRetrieved, 
                                       int BufferLen);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_GetCustomDeliveries

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_GetCustomDeliveries.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Successful return, but there were more entries found
in the database than could be written into the supplied buffer.

PARAMETER 1
PARAM_NAME isAudit
PARAM_TYPE int 
PARAM_DESCRIPTION If 1, audit delivery entries will be retrieved. Otherwise,
function returns actual alarm delivery entries.

PARAMETER 2
PARAM_NAME idDelivery
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the delivery to retrieve. If present,
the function will retrieve the information for that delivery only. Otherwise,
all deliveries of this type are returned.

PARAMETER 3
PARAM_NAME *pDelivery
PARAM_TYPE EWDB_CustomDeliveryStruct 
PARAM_DESCRIPTION Preallocated array of delivery structures to be filled.

PARAMETER 4
PARAM_NAME *NumFound
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of items found in the database..

PARAMETER 5
PARAM_NAME *NumRetrieved
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of items retrieved from the database..

PARAMETER 6
PARAM_NAME BufferLen
PARAM_TYPE int 
PARAM_DESCRIPTION Length of the input buffer - number of preallocated
structures to be filled.

DESCRIPTION If idDelivery is greater than 0, the function returns delivery
information about that delivery only. Otherwise it retrieves and writes to the 
pDelivery buffer the delivery information about all deliveries. 
It will write up to the BufferLen records, then return the number of 
found and retrieved items so that the calling routine can re-allocate the 
buffer and call again.
If isAudit is set, the retrieval is done from the audit tables, instead of 
from the actual alarm tables.

*************************************************
************************************************/
int ewdb_api_GetCustomDeliveries(int isAudit, int idDelivery, 
                                 EWDB_CustomDeliveryStruct *pDelivery, 
                                 int *NumFound, int *NumRetrieved, 
                                 int BufferLen);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_GetEmailDeliveries

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_GetEmailDeliveries.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Successful return, but there were more entries found
in the database than could be written into the supplied buffer.

PARAMETER 1
PARAM_NAME isAudit
PARAM_TYPE int 
PARAM_DESCRIPTION If 1, audit delivery entries will be retrieved. Otherwise,
function returns actual alarm delivery entries.

PARAMETER 2
PARAM_NAME idDelivery
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the delivery to retrieve. If present,
the function will retrieve the information for that delivery only. Otherwise,
all deliveries of this type are returned.

PARAMETER 3
PARAM_NAME *pDelivery
PARAM_TYPE EWDB_EmailDeliveryStruct 
PARAM_DESCRIPTION Preallocated array of delivery structures to be filled.

PARAMETER 4
PARAM_NAME *NumFound
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of items found in the database..

PARAMETER 5
PARAM_NAME *NumRetrieved
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of items retrieved from the database..

PARAMETER 6
PARAM_NAME BufferLen
PARAM_TYPE int 
PARAM_DESCRIPTION Length of the input buffer - number of preallocated
structures to be filled.

DESCRIPTION If idDelivery is greater than 0, the function returns delivery
information about that delivery only. Otherwise it retrieves and writes to the 
pDelivery buffer the delivery information about all deliveries. 
It will write up to the BufferLen records, then return the number of 
found and retrieved items so that the calling routine can re-allocate the 
buffer and call again.
If isAudit is set, the retrieval is done from the audit tables, instead of 
from the actual alarm tables.

*************************************************
************************************************/
int ewdb_api_GetEmailDeliveries(int isAudit, int idDelivery, 
                                EWDB_EmailDeliveryStruct *pDelivery, 
                                int *NumFound, int *NumRetrieved, int BufferLen);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_GetPagerDeliveries

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_GetPagerDeliveries.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Successful return, but there were more entries found
in the database than could be written into the supplied buffer.

PARAMETER 1
PARAM_NAME isAudit
PARAM_TYPE int 
PARAM_DESCRIPTION If 1, audit delivery entries will be retrieved. Otherwise,
function returns actual alarm delivery entries.

PARAMETER 2
PARAM_NAME idDelivery
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the delivery to retrieve. If present,
the function will retrieve the information for that delivery only. Otherwise,
all deliveries of this type are returned.

PARAMETER 3
PARAM_NAME *pDelivery
PARAM_TYPE EWDB_PagerDeliveryStruct 
PARAM_DESCRIPTION Preallocated array of delivery structures to be filled.

PARAMETER 4
PARAM_NAME *NumFound
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of items found in the database..

PARAMETER 5
PARAM_NAME *NumRetrieved
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of items retrieved from the database..

PARAMETER 6
PARAM_NAME BufferLen
PARAM_TYPE int 
PARAM_DESCRIPTION Length of the input buffer - number of preallocated
structures to be filled.

DESCRIPTION If idDelivery is greater than 0, the function returns delivery
information about that delivery only. Otherwise it retrieves and writes to the 
pDelivery buffer the delivery information about all deliveries. 
It will write up to the BufferLen records, then return the number of 
found and retrieved items so that the calling routine can re-allocate the 
buffer and call again.
If isAudit is set, the retrieval is done from the audit tables, instead of 
from the actual alarm tables.

*************************************************
************************************************/
int ewdb_api_GetPagerDeliveries(int isAudit, int idDelivery, 
                                EWDB_PagerDeliveryStruct *pDelivery, 
                                int *NumFound, int *NumRetrieved, int BufferLen);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_GetPhoneDeliveries

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_GetPhoneDeliveries.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Successful return, but there were more entries found
in the database than could be written into the supplied buffer.

PARAMETER 1
PARAM_NAME isAudit
PARAM_TYPE int 
PARAM_DESCRIPTION If 1, audit delivery entries will be retrieved. Otherwise,
function returns actual alarm delivery entries.

PARAMETER 2
PARAM_NAME idDelivery
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the delivery to retrieve. If present,
the function will retrieve the information for that delivery only. Otherwise,
all deliveries of this type are returned.

PARAMETER 3
PARAM_NAME *pDelivery
PARAM_TYPE EWDB_PhoneDeliveryStruct 
PARAM_DESCRIPTION Preallocated array of delivery structures to be filled.

PARAMETER 4
PARAM_NAME *NumFound
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of items found in the database..

PARAMETER 5
PARAM_NAME *NumRetrieved
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of items retrieved from the database..

PARAMETER 6
PARAM_NAME BufferLen
PARAM_TYPE int 
PARAM_DESCRIPTION Length of the input buffer - number of preallocated
structures to be filled.

DESCRIPTION If idDelivery is greater than 0, the function returns delivery
information about that delivery only. Otherwise it retrieves and writes to the 
pDelivery buffer the delivery information about all deliveries. 
It will write up to the BufferLen records, then return the number of 
found and retrieved items so that the calling routine can re-allocate the 
buffer and call again.
If isAudit is set, the retrieval is done from the audit tables, instead of 
from the actual alarm tables.

*************************************************
************************************************/
int ewdb_api_GetPhoneDeliveries(int isAudit, int idDelivery, 
                                EWDB_PhoneDeliveryStruct *pDelivery, 
                                int *NumFound, int *NumRetrieved, int BufferLen);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_GetQddsDeliveries

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_GetQddsDeliveries.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Successful return, but there were more entries found
in the database than could be written into the supplied buffer.

PARAMETER 1
PARAM_NAME isAudit
PARAM_TYPE int 
PARAM_DESCRIPTION If 1, audit delivery entries will be retrieved. Otherwise,
function returns actual alarm delivery entries.

PARAMETER 2
PARAM_NAME idDelivery
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the delivery to retrieve. If present,
the function will retrieve the information for that delivery only. Otherwise,
all deliveries of this type are returned.

PARAMETER 3
PARAM_NAME *pDelivery
PARAM_TYPE EWDB_QddsDeliveryStruct 
PARAM_DESCRIPTION Preallocated array of delivery structures to be filled.

PARAMETER 4
PARAM_NAME *NumFound
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of items found in the database..

PARAMETER 5
PARAM_NAME *NumRetrieved
PARAM_TYPE int 
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of items retrieved from the database..

PARAMETER 6
PARAM_NAME BufferLen
PARAM_TYPE int 
PARAM_DESCRIPTION Length of the input buffer - number of preallocated
structures to be filled.

DESCRIPTION If idDelivery is greater than 0, the function returns delivery
information about that delivery only. Otherwise it retrieves and writes to the 
pDelivery buffer the delivery information about all deliveries. 
It will write up to the BufferLen records, then return the number of 
found and retrieved items so that the calling routine can re-allocate the 
buffer and call again.
If isAudit is set, the retrieval is done from the audit tables, instead of 
from the actual alarm tables.

*************************************************
************************************************/
int ewdb_api_GetQddsDeliveries(int isAudit, int idDelivery, 
                               EWDB_QddsDeliveryStruct *pDelivery, 
                               int *NumFound, int *NumRetrieved, int BufferLen);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY ALARMS_API

LANGUAGE C

LOCATION THIS_FILE

FUNCTION ewdb_api_GetAndIncrementCubeVersion

STABILITY NEW

SOURCE_LOCATION alarms/ewdb_api_GetAndIncrementCubeVersion.c

RETURN_TYPE int 

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Failure.  See logfile for details.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Successful return, but there were more entries found
in the database than could be written into the supplied buffer.

PARAMETER 1
PARAM_NAME idEvent
PARAM_TYPE int 
PARAM_DESCRIPTION Database ID of the event for which CUBE
version number is retrieved.

PARAMETER 2
PARAM_NAME VersionNumber
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to the integer value representing the
CUBE version number being returned.

DESCRIPTION Returns the current CUBE version number for the 
event and increments the number by one.

*************************************************
************************************************/
int ewdb_api_GetAndIncrementCubeVersion (int idEvent, int *VersionNumber);



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDB_CoincEventStruct
TYPE_DEFINITION struct _EWDB_CoincEventStruct
DESCRIPTION Coincidence Trigger event structure.

MEMBER idCoincidence
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID for the Coincidence.

MEMBER tCoincTrigger
MEMBER_TYPE double
MEMBER_DESCRIPTION Time of the event trigger

MEMBER idEvent
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database identifier of the Event with which this
Coincidence is associated.

MEMBER bBindToEvent
MEMBER_TYPE int
MEMBER_DESCRIPTION Boolean flag indicating whether or not this
Magnitude should be bound to an Event when it is inserted into the DB.
If yes, it is associated with the Event identified by idEvent upon
insertion into the DB.


MEMBER szSource
MEMBER_TYPE char[256]
MEMBER_DESCRIPTION Identifier of the source/author that declared the
Coincidence.  This could be the Logo of the Earthworm module that generated
the Coincidence, or the login name of an analyst that reviewed it.  

MEMBER szComment
MEMBER_TYPE char *
MEMBER_DESCRIPTION Optional comment associated with the magnitude.

*************************************************
************************************************/
typedef struct _EWDB_CoincEventStruct
{
	EWDBid  idCoincidence;
	EWDBid  idEvent;
	double	tCoincidence;
	int		bBindToEvent;
	char	szSource[256]; 
	char	szHumanReadable[256]; 
	char 	*szComment;
} EWDB_CoincEventStruct;



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDB_TriggerStruct
TYPE_DEFINITION struct _EWDB_TriggerStruct
DESCRIPTION Single Channel Triggerstructure.

MEMBER idTrigger
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID for the Coincidence.

MEMBER tTrigger
MEMBER_TYPE double
MEMBER_DESCRIPTION Time of the trigger

MEMBER idCoincidence
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database identifier of the Coincidence Event with 
which this trigger is associated.

MEMBER idChan
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database identifier of the channel with which this
trigger is associated.

*************************************************
************************************************/
typedef struct _EWDB_TriggerStruct
{
	EWDBid  idTrigger;
	EWDBid  idCoincidence;
	EWDBid  idChan;
	double	tTrigger;
	double	dDist;
} EWDB_TriggerStruct;



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDB_MergeStruct
TYPE_DEFINITION struct _EWDB_MergeStruct
DESCRIPTION Event Merge info structure.

MEMBER idMerge
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID for the Merge.

MEMBER idEvent
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database identifier of the Event which is to be merged. 

MEMBER idPh
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database identifier of the Phenomenon which is to 
be merged with the event.

MEMBER szSource
MEMBER_TYPE char[256]
MEMBER_DESCRIPTION Identifier of the source/author that declared the
Merge.  This could be the Logo of the Earthworm module that generated
the Merge, or the login name of an analyst that reviewed it.  

MEMBER szHumanReadable
MEMBER_TYPE char[256]
MEMBER_DESCRIPTION Huamn readable string identifying the person or
program that declared the Merge.

MEMBER szComment
MEMBER_TYPE char *
MEMBER_DESCRIPTION Optional comment associated with the magnitude.

*************************************************
************************************************/
typedef struct _EWDB_MergeStruct
{
	EWDBid  idMerge;
	EWDBid  idPh; 			/* phenomenon */
	EWDBid  idEvent;
	char	szSource[256]; 
	char	szHumanReadable[256]; 
	char 	*szComment;
} EWDB_MergeStruct;



/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDB_PhenomenonStruct
TYPE_DEFINITION struct _EWDB_PhenomenonStruct
DESCRIPTION Phenomenon structure

MEMBER idPh
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID for the phenomenon.

MEMBER idEvent
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database identifier of the Event which first 
gave rise to this phenomenon. 

MEMBER szSource
MEMBER_TYPE char[256]
MEMBER_DESCRIPTION Identifier of the source/author that declared the
Merge.  This could be the Logo of the Earthworm module that generated
the Merge, or the login name of an analyst that reviewed it.  

MEMBER szHumanReadable
MEMBER_TYPE char[256]
MEMBER_DESCRIPTION Huamn readable string identifying the person or
program that declared the Merge.

MEMBER szComment
MEMBER_TYPE char *
MEMBER_DESCRIPTION Optional comment associated with the merge.

*************************************************
************************************************/
typedef struct _EWDB_PhenomenonStruct
{
	EWDBid  idPh;
	EWDBid  idPrefEvent; 		/* preferred solution of this phenomenon */
	char	szSource[256]; 
	char	szHumanReadable[256]; 
	char 	*szComment;
} EWDB_PhenomenonStruct;


/* Basic Merge Info -- retrieved from a handy view? */
typedef struct _MergeInfo
{
    EWDBid  idEvent;
    EWDBid  idPrefEvent; /* event giving rise to this phenomenon */
    EWDBid  idPh;
    EWDBid  idMerge;
    double  tOrigin;
    float   dLat;
    float   dLon;
    float   dDepth;
    float   dPrefMag;
	char	szSource[256]; 
	char	szHumanReadable[256]; 
} EWDB_MergeList;


/* Newest stuff -- to be commented */
int ewdb_api_UH_CreateExternalInfraRecord (EWDB_External_UH_InfraInfo *pUHInfo);
int     ewdb_api_UH_GetExternalInfraRecord (int idChanT,
        EWDB_External_UH_InfraInfo *pUHInfo, int BufferLen,
        int *NumFound, int *NumRetrieved);
int ewdb_api_GetNumStationsForEvent(EWDBid idEvent, int *pNumStations); 
int ewdb_api_CreateCoincidence (EWDB_CoincEventStruct *pCoinc);
int ewdb_api_CreateTrigger(EWDB_TriggerStruct *pTrigger);
int ewdb_api_GetCoincidence(EWDBid idEvent, int EvtType, 
				EWDB_CoincEventStruct *pCoinc,
				int NumAlloc, double StartTime, double EndTime, 
				int *pNumFound, int *pNumRetr);
int ewdb_api_GetTriggerList (EWDBid idCoincidence, EWDB_TriggerStruct *pTrigs,
                            int NumAlloc, double StartTime, double EndTime,
                            int *pNumFound, int *pNumRetr);
int ewdb_api_CreateMerge (EWDB_MergeStruct *pMerge);
int ewdb_api_DeleteMerge (EWDBid idPh, EWDBid idEvent);
int ewdb_api_DeletePhenomenon (EWDBid idPh);
int ewdb_api_CreatePhenomenon (EWDB_PhenomenonStruct *pPhenomenon);
int ewdb_api_GetMergeList (EWDB_MergeList *pMerge, int BufferLen, EWDBid idPh,
                          EWDB_MergeList *pMin, EWDB_MergeList *pMax,
                            int *NumFound, int *NumRetr);
int ewdb_api_BindToEvent (EWDBid idEvent, int CoreTable, EWDBid idCore, EWDBid *Out_idBind);
int ewdb_api_SetPrefer (EWDBid idEvent, int CoreTable, EWDBid idCore, EWDBid *Out_idPrefer);


#define CORE_TABLE_ORIGIN		4
#define CORE_TABLE_MAGNITUDE 	5
#define CORE_TABLE_MECHFM 		6
#define CORE_TABLE_WAVEFORMDESC	27
#define CORE_TABLE_COINCIDENCE	50



/***************************** NEAREST TOWN SECTION *****************************/

/************************************************
************ SPECIAL FORMATTED COMMENT **********
EW5 API FORMATTED COMMENT
TYPE TYPEDEF 

LIBRARY  EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE


TYPEDEF EWDB_PlaceStruct
TYPE_DEFINITION struct _EWDB_PlaceStruct
DESCRIPTION Towns and Places structure

MEMBER idPlace
MEMBER_TYPE EWDBid
MEMBER_DESCRIPTION Database ID for the place.

MEMBER szState
MEMBER_TYPE char[3]
MEMBER_DESCRIPTION two-letter State code 

MEMBER szPlaceName
MEMBER_TYPE char[70]
MEMBER_DESCRIPTION  Name of the place

MEMBER szPlaceType
MEMBER_TYPE char[20]
MEMBER_DESCRIPTION  Type of the place

MEMBER szCountry
MEMBER_TYPE char[30]
MEMBER_DESCRIPTION  Country of the place

MEMBER szCounty
MEMBER_TYPE char[30]
MEMBER_DESCRIPTION  County of the place

MEMBER dLat
MEMBER_TYPE double
MEMBER_DESCRIPTION  Latitude

MEMBER dLon
MEMBER_TYPE double
MEMBER_DESCRIPTION  Longitude

MEMBER dElev
MEMBER_TYPE double
MEMBER_DESCRIPTION  Elevation

MEMBER iPopulation
MEMBER_TYPE int
MEMBER_DESCRIPTION  Population

MEMBER iPlaceMajorType
MEMBER_TYPE int
MEMBER_DESCRIPTION  Major Type (City, Dam, etc...)

MEMBER iPlaceMinorType
MEMBER_TYPE int
MEMBER_DESCRIPTION  Minor Type (large city, small city...)


*************************************************
************************************************/
typedef struct _EWDB_PlaceStruct
{
	int    idPlace;
	char   szState[3];
	char   szPlaceName[70];
	char   szPlaceType[20];
	char   szCounty[30];
	char   szCountry[30];
	double dLat;
	double dLon;
	double dElev;
	int    iPopulation;
	int    iPlaceMajorType;
	int    iPlaceMinorType;

	/* These are not in the DB, but are filled upon retrieval and
		comparisson with the origin 
	 */
	double dDist;
	double dAzm;
}  EWDB_PlaceStruct;


#define	EWDB_PLACE_TYPE_GENERIC_CITY	1

#define EWDB_CITY_TYPE_UNKNOWN 		0
#define EWDB_CITY_TYPE_CDP     		5
#define EWDB_CITY_TYPE_CITY    		4
#define EWDB_CITY_TYPE_LARGE_CITY 	3
#define EWDB_CITY_TYPE_METROPOLIS 	2
#define EWDB_CITY_TYPE_MEGALOOPOLIS 1


int ewdb_api_CreatePlace (EWDB_PlaceStruct *pPlace);
int ewdb_api_GetPlaceList (EWDB_PlaceStruct *pBuffer, int BufferLen,
		EWDB_PlaceStruct *pMinPlace, EWDB_PlaceStruct *pMaxPlace, 
		int *pNumRet, int *pNumFound);




/***************************** STATE SCHEMA SECTION *****************************/


/*
 * This is work in progress
 */
typedef struct _EWDB_DataChangeStruct
{
	EWDBid		idDataChange;
	EWDBid		idDCType;
	EWDBid		idCore;
	EWDBid		tiCore;
	int			iChangeType;
	int			iChangeSubType;
}  EWDB_DataChangeStruct;


typedef struct _EWDB_ProcessReqStruct
{
	EWDBid		idProcessReq;
	int			iDatumType;
	char   		sDescription[40];
	EWDBid		idDCType;
	EWDBid		idTaskType;
	int			iRetCode;
}  EWDB_ProcessReqStruct;


typedef struct _EWDB_StatePRStruct
{
	EWDBid		idStatePR;
	EWDBid		idState;
	EWDBid		idProcessReq;
	int			iStateRuleSet;
}  EWDB_StatePRStruct;


typedef struct _EWDB_StateTaskStruct
{
	EWDBid		idRunTask;
	EWDBid		idState;
	EWDBid		idTaskType;
	int			tiCore;
	char   		sDescription[40];
	char   		sProgName[40];
}  EWDB_StateTaskStruct;



int ewdb_api_DeleteDataChange(int);
int ewdb_api_GetDataChangeList(EWDB_DataChangeStruct *, int *, int *, int);
int ewdb_api_GetProcessReqs (EWDBid, EWDB_ProcessReqStruct *, int *, int *, int);
int ewdb_api_GetStatePR (EWDBid, EWDBid, EWDB_StatePRStruct *, int *, int *, int);
int ewdb_api_GetTasksForState (EWDBid, EWDB_StateTaskStruct *, int *, int *, int);

/* to comment */
int ewdb_api_CreatePeakAmp (EWDB_PeakAmpStruct *);
int ewdb_api_CreateStaMag (EWDB_StationMagStruct *);



/* AMPLITUDE_TYPE and amplitude types are defined in include/earthworm_defs.h */

/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_api_GetAmpsByTime

STABILITY NEW

SOURCE_LOCATION THIS_FILE

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Partial success.  A list of amplitudes was retrieved,
but the caller's buffer was not large enough to accomadate all of the
amplitudes found.  See pNumAmpsFound for the number of arrivals found
and pNumAmpsRetrieved for the number of amplitudes placed in the
caller's buffer.


PARAMETER 1
PARAM_NAME tStart
PARAM_TYPE time_t
PARAM_DESCRIPTION Start time of criteria window.  (Secs since 1970)

PARAMETER 2
PARAM_NAME tEnd
PARAM_TYPE time_t
PARAM_DESCRIPTION End time of criteria window.  (Secs since 1970)

PARAMETER 3
PARAM_NAME iAmpType
PARAM_TYPE AMPLITUDE_TYPE
PARAM_DESCRIPTION Type of amplitude the caller is searching for.
"Amplitude Type" constants are defined in <earthworm_defs.h>.

PARAMETER 4
PARAM_NAME pAmpBuffer
PARAM_TYPE EWDB_PeakAmpStruct *
PARAM_DESCRIPTION Buffer allocated by the caller, where the function
will write the list of amplitudes retrieved.

PARAMETER 5
PARAM_NAME pStationBuffer
PARAM_TYPE EWDB_StationStruct *
PARAM_DESCRIPTION Buffer allocated by the caller, where the function
will write the list of station/channel info associated with
the amplitudes retrieved.  If pStationBuffer is set to NULL by the 
caller, then the function will not attempt to retrieve Station information
for the retrieved amplitudes.  If pStationBuffer is NOT NULL, then 
the function will fill out an EWDB_StationStruct for the channel 
associated with each amplitude.

PARAMETER 6
PARAM_NAME pNumAmpsFound
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of amplitudes found.

PARAMETER 7
PARAM_NAME pNumAmpsRetrieved
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of amplitudes retrieved and placed in the caller's buffer(pAmpBuffer).

PARAMETER 8
PARAM_NAME iBufferLen
PARAM_TYPE int
PARAM_DESCRIPTION Size of the pAmpBuffer buffer in as a multiple of
EWDB_PeakAmpStruct. (example: 15 structs)  If pStationBuffer is
not NULL, then it must also contain space for this many 
EWDB_StationStruct records.

DESCRIPTION Function retrieves a list of amplitudes of a given type
for a given time window.  Use tStart and tEnd to specify the time 
window for which you are interested in amplitudes.  If you want station
information for the channel associated with each amplitude, allocate 
space for pStationBuffer, otherwise set pStationBuffer to NULL, and
station information will not be retrieved.


*************************************************
************************************************/
int ewdb_api_GetAmpsByTime(time_t tStart, time_t tEnd, AMPLITUDE_TYPE iAmpType,
                           EWDB_PeakAmpStruct * pAmpBuffer,
                           EWDB_StationStruct * pStationBuffer,
                           int * pNumAmpsFound, int * pNumAmpsRetrieved,
                           int iBufferLen);




/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_api_GetAmpsByEvent

STABILITY NEW

SOURCE_LOCATION THIS_FILE

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Partial success.  A list of amplitudes was retrieved,
but the caller's buffer was not large enough to accomadate all of the
amplitudes found.  See pNumAmpsFound for the number of arrivals found
and pNumAmpsRetrieved for the number of amplitudes placed in the
caller's buffer.

PARAMETER 1
PARAM_NAME idEvent
PARAM_TYPE EWDBid
PARAM_DESCRIPTION DB identifier of the Event for which the caller
wants a list of associated amplitudes.

PARAMETER 2
PARAM_NAME iAmpType
PARAM_TYPE AMPLITUDE_TYPE
PARAM_DESCRIPTION Type of amplitude the caller is searching for.
"Amplitude Type" constants are defined in <earthworm_defs.h>.

PARAMETER 3
PARAM_NAME pAmpBuffer
PARAM_TYPE EWDB_PeakAmpStruct *
PARAM_DESCRIPTION Buffer allocated by the caller, where the function
will write the list of amplitudes retrieved.

PARAMETER 4
PARAM_NAME pStationBuffer
PARAM_TYPE EWDB_StationStruct *
PARAM_DESCRIPTION Buffer allocated by the caller, where the function
will write the list of station/channel info associated with
the amplitudes retrieved.  If pStationBuffer is set to NULL by the 
caller, then the function will not attempt to retrieve Station information
for the retrieved amplitudes.  If pStationBuffer is NOT NULL, then 
the function will fill out an EWDB_StationStruct for the channel 
associated with each amplitude.

PARAMETER 5
PARAM_NAME pNumAmpsFound
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of amplitudes found.

PARAMETER 6
PARAM_NAME pNumAmpsRetrieved
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of amplitudes retrieved and placed in the caller's buffer(pAmpBuffer).

PARAMETER 7
PARAM_NAME iBufferLen
PARAM_TYPE int
PARAM_DESCRIPTION Size of the pAmpBuffer buffer in as a multiple of
EWDB_PeakAmpStruct. (example: 15 structs)  If pStationBuffer is
not NULL, then it must also contain space for this many 
EWDB_StationStruct records.

DESCRIPTION Function retrieves a list of amplitudes of a given type
that are associated with a given DB Event.  Use idEvent to define the
DB Event for which you want a list of associated amplitudes.  If you want 
station information for the channel associated with each amplitude, 
allocate space for pStationBuffer, otherwise set pStationBuffer to NULL, 
and station information will not be retrieved.

NOTE This function only retrieves amplitudes that are directly associated
with an Event.  It will not retrieve amplitudes that have been associated
with an Event via a Magnitude.


*************************************************
************************************************/
int ewdb_api_GetAmpsByEvent(EWDBid idEvent, AMPLITUDE_TYPE iAmpType,
                            EWDB_PeakAmpStruct * pAmpBuffer,
                            EWDB_StationStruct * pStationBuffer,
                            int * pNumAmpsFound, int * pNumAmpsRetrieved,
                            int iBufferLen);


/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_api_GetAmpsByOrigin

STABILITY NEW

SOURCE_LOCATION THIS_FILE

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

RETURN_VALUE EWDB_RETURN_WARNING
RETURN_DESCRIPTION Partial success.  A list of amplitudes was retrieved,
but the caller's buffer was not large enough to accomadate all of the
amplitudes found.  See pNumAmpsFound for the number of arrivals found
and pNumAmpsRetrieved for the number of amplitudes placed in the
caller's buffer.

PARAMETER 1
PARAM_NAME idOrigin
PARAM_TYPE EWDBid
PARAM_DESCRIPTION DB identifier of the Origin for which the caller
wants a list of associated amplitudes.

PARAMETER 2
PARAM_NAME iAmpType
PARAM_TYPE AMPLITUDE_TYPE
PARAM_DESCRIPTION Type of amplitude the caller is searching for.
"Amplitude Type" constants are defined in <earthworm_defs.h>.

PARAMETER 3
PARAM_NAME pAmpBuffer
PARAM_TYPE EWDB_PeakAmpStruct *
PARAM_DESCRIPTION Buffer allocated by the caller, where the function
will write the list of amplitudes retrieved.

PARAMETER 4
PARAM_NAME pStationBuffer
PARAM_TYPE EWDB_StationStruct *
PARAM_DESCRIPTION Buffer allocated by the caller, where the function
will write the list of station/channel info associated with
the amplitudes retrieved.  If pStationBuffer is set to NULL by the 
caller, then the function will not attempt to retrieve Station information
for the retrieved amplitudes.  If pStationBuffer is NOT NULL, then 
the function will fill out an EWDB_StationStruct for the channel 
associated with each amplitude.

PARAMETER 5
PARAM_NAME pNumAmpsFound
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of amplitudes found.

PARAMETER 6
PARAM_NAME pNumAmpsRetrieved
PARAM_TYPE int *
PARAM_DESCRIPTION Pointer to an integer where the function will write
the number of amplitudes retrieved and placed in the caller's buffer(pAmpBuffer).

PARAMETER 7
PARAM_NAME iBufferLen
PARAM_TYPE int
PARAM_DESCRIPTION Size of the pAmpBuffer buffer in as a multiple of
EWDB_PeakAmpStruct. (example: 15 structs)  If pStationBuffer is
not NULL, then it must also contain space for this many 
EWDB_StationStruct records.

DESCRIPTION Function retrieves a list of amplitudes of a given type
that are associated with a given DB Origin.  Use idOrigin to define the
DB Origin for which you want a list of associated amplitudes.  If you want 
station information for the channel associated with each amplitude, 
allocate space for pStationBuffer, otherwise set pStationBuffer to NULL, 
and station information will not be retrieved.

NOTE This function only retrieves amplitudes that are directly associated
with an Origin, or the Event with which that Origin is associated.  
It will not retrieve amplitudes that have been associated
with an Origin via a Magnitude.


*************************************************
************************************************/
int ewdb_api_GetAmpsByOrigin(EWDBid idOrigin, AMPLITUDE_TYPE iAmpType,
                             EWDB_PeakAmpStruct * pAmpBuffer,
                             EWDB_StationStruct * pStationBuffer,
                             int * pNumAmpsFound, int * pNumAmpsRetrieved,
                             int iBufferLen);



/************************************************
************ SPECIAL FORMATTED COMMENT **********
TYPE FUNCTION_PROTOTYPE

LIBRARY EWDB_API_LIB

SUB_LIBRARY PARAMETRIC_API

LANGUAGE C

LOCATION THIS_FILE


FUNCTION ewdb_api_GetEventForOrigin

STABILITY NEW

SOURCE_LOCATION THIS_FILE

RETURN_TYPE int

RETURN_VALUE EWDB_RETURN_FAILURE
RETURN_DESCRIPTION Fatal error.  See logfile for details.

RETURN_VALUE EWDB_RETURN_SUCCESS
RETURN_DESCRIPTION Success.

PARAMETER 1
PARAM_NAME idOrigin
PARAM_TYPE EWDBid
PARAM_DESCRIPTION The DB identifier of the Origin for
which the caller wants to obtain the DB Event.

PARAMETER 2
PARAM_NAME idEvent
PARAM_TYPE EWDBid *
PARAM_DESCRIPTION Pointer to an EWDBid value, where the 
function will write the DB ID of the Event associated 
with the given Origin.

DESCRIPTION Function returns the DB ID of the Event associated
with the given Origin.  If "zero" or "more than one" Events
are associated with the given origin, then an error is returned.


*************************************************
************************************************/
int ewdb_api_GetEventForOrigin(EWDBid idOrigin, EWDBid * idEvent);


typedef struct _EWDB_ChannelStruct
{
  EWDB_StationStruct Comp;
  EWDBid             idSite;
  EWDBid             idSiteT;
  EWDBid             idCompT;
  double             tOn;
  double             tOff;
  EWDB_ChanTCTFStruct * pChanProps;

} EWDB_ChannelStruct;

int ewdb_api_GetSelectedStations(EWDB_ChannelStruct * pBuffer, int iBufferLen,
                                 EWDB_ChannelStruct * pCSCriteria, 
                                 EWDB_ChannelStruct * pCSMaxCriteria, 
                                 int iCriteria,
                                 int * pNumStationsFound, 
                                 int * pNumStationsRetrieved);

int ewdb_api_GetSelectedChannels(EWDB_ChannelStruct * pBuffer, int iBufferLen,
                                 EWDB_ChannelStruct * pCSCriteria, 
                                 EWDB_ChannelStruct * pCSMaxCriteria, 
                                 int iCriteria,
                                 int * pNumStationsFound, 
                                 int * pNumStationsRetrieved);

int ewdb_api_GetSelectedSites(EWDB_ChannelStruct * pBuffer, int iBufferLen,
                                 EWDB_ChannelStruct * pCSCriteria, 
                                 EWDB_ChannelStruct * pCSMaxCriteria, 
                                 int iCriteria,
                                 int * pNumStationsFound, 
                                 int * pNumStationsRetrieved);

int ewdb_api_GetSelectedStations_w_Response(EWDB_ChannelStruct * pBuffer, int iBufferLen,
                                            EWDB_ChannelStruct * pCSCriteria, 
                                            EWDB_ChannelStruct * pCSMaxCriteria, 
                                            int iCriteria,
                                            int * pNumStationsFound, 
                                            int * pNumStationsRetrieved);



#endif   /* undef EWDB_ORA_API_H */
