/*******************************
  adsendxs.h
 *******************************/
#define ERRMSGSIZE 80
#define STASIZE 6
#define COMPSIZE 4
#define NETSIZE 3
#define LOCSIZE 3
#define DAQCHANSIZE 16
#define DATATYPESIZE 3
#define PORTNAMESIZE 10


typedef struct
{
   char sta[STASIZE];
   char comp[COMPSIZE];
   char net[NETSIZE];
   char loc[LOCSIZE];
   int  pin;
   char daqChan[DAQCHANSIZE];
}
SCNL;

#define MAXCHAN  32          // Maximum number of channels to digitize
#define DLE 0x10
#define ETX 0x03
#define MAXGPSPACKETSIZE 100

// GetGpsTime() return values
#define CRITICAL_ERROR              -1
#define GPS_TIME_UNAVAILABLE         0
#define UNLOCKED_GPS_TIME_AVAILABLE  1
#define LOCKED_GPS_TIME_AVAILABLE    2

// antennaOpen values
#define FALSE    0
#define TRUE     1
#define UNKNOWN -1

// Statmgr error codes
#define NI_ERROR      1
#define GPS_ERROR     2
#define NOPULSE_ERROR 3
