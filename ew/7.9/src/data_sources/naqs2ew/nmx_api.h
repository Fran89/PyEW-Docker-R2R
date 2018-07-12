/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: nmx_api.h 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.5  2003/10/07 17:59:53  dietz
 *     added ConnectReponse message
 *
 *     Revision 1.4  2003/02/11 01:07:00  dietz
 *     Added functions to read transparent serial packets
 *
 *     Revision 1.3  2003/02/10 22:03:09  dietz
 *     Added definitions for serial packet requests
 *
 *     Revision 1.2  2002/11/06 19:32:31  dietz
 *     Added new field to NMX_CHANNEL_INFO struct
 *
 *     Revision 1.1  2002/10/15 23:52:58  dietz
 *     Initial revision
 *
 *     Revision 1.2  2002/03/15 23:10:09  dietz
 *     *** empty log message ***
 *
 *     Revision 1.1  2001/06/20 22:35:07  dietz
 *     Initial revision
 *
 *
 *
 */

/*
 * nmx_api.h
 *
 * Lynn Dietz  22 January 1999
 *
 * Definitions and prototypes useful for a client of
 * Nanometrics server software such as NaqsServer.
 * Written to specifications described in: 
 *
 *    "Nanometrics Software Reference Manual",
 *    "Naqs Online Data Streams" chapter (last revised 24 Nov 1998).
 *
 * Added more definitions, structures, functions needed for
 * clients of Nanometrics DataServer, as described in:
 *    "Nanometrics Data Access Protocol" (last revised 27 June 2001).
 */

#ifndef _NMX_API_H
#define _NMX_API_H

#include "nmxp_packet.h"
#include "crc32.h"

/**********************************************************************
 *                            BYTE ORDER                              *
 *    The Nanometrics server software may run on Intel platforms,     *
 *    but the byte order of the socket stream between NMX server      *
 *    software and its clients is in network byte order.              *
 *                        Swap accordingly!                           *
 **********************************************************************/


/* Define a NMX message
 * ---------------------
 * 
 * Each NMX message is composed of a fixed length header (12 bytes),
 * followed by variable length content.
 *
 *   Header:   4 byte int   signature = 0x7ABCDE0F
 *             4 byte int   message type
 *             4 byte int   message content length = N
 *
 *   Content:  N byte string with internal format dependent on
 *             message type.
 */

typedef struct _NMXHDR
{
  int signature;   /* set to magic value to verify the start of a header  */ 
  int msgtype;     /* type of message to follow (defines internal format) */
  int msglen;      /* length (bytes) of message content to follow         */
} NMXHDR;

#define NMXHDR_LEN         12
#define NMXHDR_SIGNATURE   0x7ABCDE0F

/* NMX Server message types
 **************************/
#define NMXMSG_COMPRESSED_DATA       1  /* data, SOH, transparent serial */
#define NMXMSG_DECOMPRESSED_DATA     4  /* decompressed data */
#define NMXMSG_TRIGGER               5
#define NMXMSG_EVENT                 6
#define NMXMSG_CHANNEL_LIST        150
#define NMXMSG_ERROR               190
#define NMXMSG_TERMINATE           200
#define NMXMSG_CONNECT_RESPONSE    207
#define NMXMSG_READY               208
#define NMXMSG_PRECIS_LIST         253
#define NMXMSG_CHANNEL_HEADER      256
#define NMXMSG_DATA_SIZE           257
#define NMXMSG_NAQS_TRIGGER        259
#define NMXMSG_NAQS_EVENT          260

/* NMX Client message types
 **************************/
#define NMXMSG_CONNECT             100
#define NMXMSG_REQUEST_PENDING     110
#define NMXMSG_ADD_TIMESERIES      120
#define NMXMSG_ADD_SOH             121
#define NMXMSG_ADD_TRIGGER         122
#define NMXMSG_ADD_EVENTS          123
#define NMXMSG_ADD_SERIAL          124
#define NMXMSG_REMOVE_TIMESERIES   130
#define NMXMSG_REMOVE_SOH          131
#define NMXMSG_REMOVE_TRIGGER      132
#define NMXMSG_REMOVE_EVENTS       133
#define NMXMSG_REMOVE_SERIAL       134
#define NMXMSG_PRECISLIST_REQUEST  203
#define NMXMSG_CANCEL_REQUEST      205
#define NMXMSG_CONNECT_REQUEST     206
#define NMXMSG_CHANNELLIST_REQUEST 209
#define NMXMSG_CHANNELINFO_REQUEST 226
#define NMXMSG_DATA_REQUEST        227
#define NMXMSG_DATASIZE_REQUEST    229
#define NMXMSG_TRIGGER_REQUEST     231
#define NMXMSG_EVENT_REQUEST       232

/* Other definitions 
 *******************/
#define NMX_FAILURE                 -1   /* function return for failure */
#define NMX_SUCCESS                  0   /* function return for success */

#define NMX_SUBTYPE_TIMESERIES       1   /* time series data subtype */
#define NMX_SUBTYPE_SOH              2   /* state-of-health data subtype */
#define NMX_SUBTYPE_TRANSSERIAL      6   /* transparent serial subtype */

#define NAQS_REQUEST_TIMEOUT        30   /* once a connection is made to
                                          * NaqsServer, a client has this 
                                          * many seconds to send the first 
                                          * NMXMSG_ADD* msg, otherwise  
                                          * the connection times out */

#define DS_REQUEST_TIMEOUT          20   /* once a connection is made to
                                          * DataServer, a client has this 
                                          * many seconds to send the first 
                                          * NMXMSG_*REQUEST msg, otherwise  
                                          * the connection times out */


/* Structure filled from a NMXMSG_CHANNEL_LIST message
 ******************************************************/
typedef struct _NMX_CHANNEL_INFO
{
   int   chankey;     /* equals ( (ID<<16) | (type<<8) | channel ) where      */
                      /*   ID is the full instrument serial number            */
                      /*   type is the data subtype (1=time series, 2=SOH)    */
                      /*   channel is the data channel # (0 to 5)             */
   char  stachan[12]; /* null-terminated channel name string (e.g. STN01.BHZ) */
   short instrid;     /* derived from chankey */
   int   subtype;     /* derived from chankey */
   int   channel;     /* derived from chankey */
   NMXPINSTRUMENT inst; /* derived from instrid */
} NMX_CHANNEL_INFO;

/* Structure filled from a NMXMSG_PRECIS_LIST message
 ****************************************************/
typedef struct _NMX_PRECIS_INFO
{
   int   chankey;     /* equals ( (ID<<16) | (type<<8) | channel ) where      */
                      /*   ID is the full instrument serial number            */
                      /*   type is the data subtype (1=time series, 2=SOH)    */
                      /*   channel is the data channel # (0 to 5)             */
   char  stachan[12]; /* null-terminated channel name string (e.g. STN01.BHZ) */
   short instrid;     /* derived from chankey */
   int   subtype;     /* derived from chankey */
   int   channel;     /* derived from chankey */
   NMXPINSTRUMENT inst; /* derived from instrid */
   int32_t  tstart;   /* start time of data available, sec since 1970/1/1     */
   int32_t  tend;     /* end time of data available, sec since 1970/1/1       */
} NMX_PRECIS_INFO;

/* Structure filled from a NMXMSG_COMPRESSED_DATA or 
                         a NMXMSG_DECOMPRESSED_DATA message
 ***********************************************************/
typedef struct _NMX_DECOMP_DATA
{
   int    chankey;    /* same as NMX_CHANNEL_INFO structure            */
   double starttime;  /* time of 1st sample (seconds since 1 Jan 1970) */
   int    nsamp;      /* number of samples in this packet = N          */
   int    isamprate;  /* sample rate (samples per second)              */
   int    maxdatalen; /* maximum number of bytes in data               */
   int   *data;       /* N samples as 32-bit integers                  */ 
} NMX_DECOMP_DATA;

/* Structure filled from a NMXMSG_SERIAL_DATA message
 ****************************************************/
typedef struct _NMX_SERIAL_DATA
{
   NMXPSERIALHDR hdr; /* transparent serial packet header              */
   double starttime;  /* time of 1st data byte (secs since 1 Jan 1970) */
   int    chankey;    /* same as NMX_CHANNEL_INFO structure            */
   int    nbyte;      /* number of serial bytes in this packet = N     */
   int    maxdatalen; /* maximum number of bytes in data               */
   char  *data;       /* N bytes of serial data                        */ 
} NMX_SERIAL_DATA;

/* Structure filled from a NMXMSG_TERMINATE message
 ***************************************************/
#define NMX_NORMAL_SHUTDOWN     1   /* normal shutdown                */
#define NMX_ERROR_SHUTDOWN      2   /* shutdown due to error          */
#define NMX_TIMEOUT_SHUTDOWN    3   /* shutdown due to server timeout */

typedef struct _NMX_TERMINATE
{
   int    reason;     /* reason for termination: 1 = normal shutdown   */
                      /*                         2 = error shutdown    */
                      /*                         3 = timeout shutdown  */
   char  *note;       /* char string giving explanation of termination */
} NMX_TERMINATE;

/* Structure of info needed to build NMXMSG_CONNECT_REQUEST message
 ******************************************************************/
#define NMX_DS_LEN  12
typedef struct _NMX_CONNECT_REQUEST
{
   int32_t   DAPversion;     /* Data Access Protocol version           */
   char   user[NMX_DS_LEN];  /* DataServer user name (null terminated) */
   char   pswd[NMX_DS_LEN];  /* DataServer password  (null terminated) */
   int32_t   tconnect;       /* connection time returned by DataServer */
} NMX_CONNECT_REQUEST;

/* Structure filled from a NMXMSG_CONNECT_RESPONSE message
 *********************************************************/
typedef struct _NMX_CONNECT_RESPONSE
{
   int32_t   DAPversion;     /* Data Access Protocol version           */
} NMX_CONNECT_RESPONSE;

/* API for building and reading NMX messages
 *******************************************/
int  nmx_checklen( char **buf, int *buflen, int reqlen ); 
void nmx_unloadhdr( NMXHDR *nhd, char *chd ); 

/* These functions are used in Earthworm NMX clients: */
int nmx_bld_connect( char **buf, int *buflen, int *totlen ); 
int nmx_bld_connectrequest( char **buf, int *buflen, int *totlen,
                            NMX_CONNECT_REQUEST *conreq );
int nmx_bld_request_pending( char **buf, int *buflen, int *totlen ); 
int nmx_bld_add_timeseries( char **buf, int *buflen, int *totlen, int *keys,
                            int nkeys, int delay, int format, int sendbuffer ); 
int nmx_bld_add_soh( char **buf, int *buflen, int *totlen, int *keys,
                     int nkeys, int delay, int sendbuffer ); 
int nmx_bld_add_serial( char **buf, int *buflen, int *totlen, int *keys,
                        int nkeys, int delay, int sendbuffer );
int nmx_bld_channellist_request( char **buf, int *buflen, int *totlen ); 
int nmx_bld_precislist_request( char **buf, int *buflen, int *totlen,
                                int instrid, int dtype, int chan ); 
int nmx_bld_terminate( char **buf, int *buflen, int *totlen, 
                       NMX_TERMINATE *term ); 
int nmx_rd_connect_response( NMXHDR *nhd, char *msg,  
                             NMX_CONNECT_RESPONSE *naqsbuf ); 
int nmx_rd_decompress_data( NMXHDR *nhd, char *msg,  
                            NMX_DECOMP_DATA *naqsbuf ); 
int nmx_rd_channel_list( NMXHDR *nhd, char *msg,  
                         NMX_CHANNEL_INFO **naqschan, int *nchan );
int nmx_rd_precis_list( NMXHDR *nhd, char *msg,  
                        NMX_PRECIS_INFO **naqschan, int *nchan );
int nmx_rd_terminate( NMXHDR *nhd, char *msg, NMX_TERMINATE *term );
int nmx_rd_compressed_data( NMXHDR *nhd, char *msg,  
                            NMX_DECOMP_DATA *naqsbuf );
int nmx_rd_serial_data( NMXHDR *nhd, char *msg, NMX_SERIAL_DATA *serial );

/* The following functions aren't used by current Earthworm NMX clients   
   & aren't be written just yet:                                         
int nmx_bld_add_trigger( char **buf, int *buflen, int *totlen ); 
int nmx_bld_add_events( char **buf, int *buflen, int *totlen ); 
int nmx_bld_remove_timeseries( char **buf, int *buflen, int *totlen ); 
int nmx_bld_remove_soh( char **buf, int *buflen, int *totlen ); 
int nmx_bld_remove_serial( char **buf, int *buflen, int *totlen ); 
int nmx_bld_remove_trigger( char **buf, int *buflen, int *totlen ); 
int nmx_bld_remove_events( char **buf, int *buflen, int *totlen ); 
int nmx_rd_trigger( NMXHDR *nhd, char *msg, NMX_TRIGGER *naqstrg );
int nmx_rd_event( NMXHDR *nhd, char *msg, NMX_EVENT *naqsevent );
*/

/* Socket_ew funtions to simplify socket communications (nmxsrv_socket.c) 
 ************************************************************************/
int nmxsrv_connect( char *ipadr, int port );
int nmxsrv_sendto( char *msg, int msglen );
int nmxsrv_recvconnecttime( int32_t *tconnect );
int nmxsrv_recvmsg( NMXHDR *nhd, char **buf, int *buflen );
int nmxsrv_disconnect( void );

/* Externals useful when dealing with raw Nanometrics packets
 ************************************************************/
extern char *NMXP_Instrument[];    /* declared in nmxp_packet.c */
extern int   Num_NMXP_Instrument;  /* declared in nmxp_packet.c */
extern short NMXP_SampleRate[];    /* declared in nmxp_packet.c */
extern int   Num_NMXP_SampleRate;  /* declared in nmxp_packet.c */

#endif
