/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: naqsser.h 3459 2008-12-02 02:26:03Z paulf $
 *
 *    Revision history:
 *     Revision 1.1  2003/02/10 22:35:07  whitmore
 *     Initial revision (copied from naqs2ew)
 *
 */

/*   naqsser.h    */
 
#ifndef _NAQSSER_H
#define _NAQSSER_H

#include "naqschassis.h"

#define MAX_SCN_DUMP     32   /* Total max of stations allowed          */
#define MAX_TG_DATA    1024   /* Number of station allowed in TGDB      */
#define MAX_SERTGTWC_SIZE 512 /* max length of serial tide gage msg     */
#define SAMPLE_LENGTH    10   /* Serial data sample length in bytes     */
#define SAMPLE_LENGTH2   13   /* NOS Serial data sample length in bytes */
#define SAMPLE_LENGTH3   39   /* WCATWC Serial data sample length in bytes */
#define MAX_SAMPS        16   /* Max SERIAL channels per incoming msg   */
#define STATION_LEN      6    /* max string-length of station code      */ 
#define CHAN_LEN         8    /* max string-length of channel code      */
#define NETWORK_LEN      8    /* max string-length of network code      */ 
#define ABBREV_LEN       8    /* max string-length of tide station abbr.*/ 
#define HEADER0_LEN     64    /* max string-length of header value      */ 
#define HEADER1_LEN     32    /* max string-length of header value      */ 
#define HEADER2_LEN     16    /* max string-length of header value      */ 
#define HEADER3_LEN    128    /* max string-length of header value      */ 

#define SER_NOTAVAILABLE 0    /* SER not served by current NaqsServer   */
#define SER_AVAILABLE    1    /* SER is served by current NaqsServer    */

#define ABS(X) (((X) >= 0) ? (X) : -(X))
 
/* Structure for tracking requested channels
 *******************************************/
typedef struct _SER_INFO {
   char               sta[STATION_LEN];
   char               chan[CHAN_LEN];
   char               net[NETWORK_LEN];
   char               stnAbbrev[ABBREV_LEN];
   int                pinno;
   int                delay;
   int                sendbuffer;
   int                flag;
   int                first;      /* 0 until packet processed      */
   int                format;     /* 0=NOS output, 1=WCATWC output */
   double             texp;       /* expected time of next packet  */
   double             dtexp;      /* expected time between samples */
   double             doffset;    /* Amount to add (in meters) to bring data to reference level */
   double             dgain;      /* site gain (mult. by this factor first */
   NMX_CHANNEL_INFO   info;
} SER_INFO;
 
#ifndef __TGFILE_HEADER_TYPE
#define __TGFILE_HEADER_TYPE

/* Structure for holding data file header info
 *********************************************/
typedef struct _TGFILE_HEADER {
   char    szSiteAbbrev[ABBREV_LEN];       /* Site Name Abbreviation */
   char    szSiteName[HEADER0_LEN];        /* Site Name */
   char    szPlatformID[HEADER2_LEN];      /* Site PlatformID */
   char    szWMOCode[HEADER2_LEN];         /* WMO ID with which data is sent */
   char    szGOESID[HEADER2_LEN];          /* GOES channel */
   char    szSiteOperator[HEADER2_LEN];    /* Site Operator Acronym */
   char    szSiteOAbbrev[ABBREV_LEN];      /* Site Operator Abbreviation */
   char    szTransmissionType[HEADER2_LEN];/* Data Transmission */
   char    szSiteLat[HEADER2_LEN];         /* Site Latitude */
   char    szSiteLon[HEADER2_LEN];         /* Site Longitude */
   char    szSensorType[HEADER1_LEN];      /* Sensor type */
   char    szWaterLevelUnits[HEADER2_LEN]; /* Water level units (eg cm..) */
   char    szTimeZone[ABBREV_LEN];         /* Time zone of data */
   char    szSampleRate[ABBREV_LEN];       /* Data sample rate */
   char    szSampleRateUnits[HEADER2_LEN]; /* Sample Rate Units */
   char    szReferenceDatum[HEADER2_LEN];  /* Sea level reference datum */
   char    szRecordingAgency[HEADER2_LEN]; /* Recording agency acronym */
   char    szOcean[HEADER2_LEN];           /* Ocean in which gage resides */
   char    szProcessingInfo[HEADER1_LEN];  /* Extra processing info or "raw" */
   char    szComments[HEADER3_LEN];        /* General comments */
   char    szDataHeader[HEADER3_LEN];      /* Data header line */
} TGFILE_HEADER;
#endif

/* Function Prototypes 
 *********************/
/* serchannels.c */
int       SelectSerChannels( NMX_CHANNEL_INFO *chinf, int nch, 
                             SER_INFO *req, int nreq ); 
SER_INFO *FindSerChannel( SER_INFO *list, int nlist, int chankey );

#endif
