/* $Id: grf.h 5714 2013-08-05 19:18:08Z paulf $ */
/*-----------------------------------------------------------------------

	Generic Recording Format include.

    Copyright (c) 2000-2007 - DAQ Systems, LLC. - All rights reserved.

-----------------------------------------------------------------------*/

#if !defined _GRF_H_INCLUDED_
#define _GRF_H_INCLUDED_

#include "platform.h"
#include "serialize.h"
#include "ustime.h"

/* Constants ----------------------------------------------------------*/
#define GRF_VERSION			1
#define GRF_SIGNATURE		"GRF"

/* GRF registered port number... */
#define GRF_REGISTERED_PORT 3757

/* Limits... */
#define GRF_MAX_INT32S		234
#define GRF_MAX_INT24S		313
#define GRF_MAX_CM8S		450
#define GRF_MAX_CM8_LEN		1024
#define GRF_MAX_MESSAGE_LEN 1000
#define GRF_MAX_NET_LEN		7
#define GRF_MAX_STN_LEN		15
#define GRF_MAX_COMP_LEN	7
#define GRF_MAX_PACKET_LEN  2048

/* GRF packet types... */
#define GRF_TYPE_NONE		0				/* Undefined */
#define GRF_TYPE_DATA		1				/* Waveform data packet */
#define GRF_TYPE_CON_REQ	2				/* Connection request */
#define GRF_TYPE_CON_ACK	3				/* Connection acknowledge */
#define GRF_TYPE_CON_NAK	4				/* Connection negative acknowledge */
#define GRF_TYPE_CMD		5				/* Command packet */
#define GRF_TYPE_CMD_ACK	6				/* Positive command response packet */
#define GRF_TYPE_CMD_NAK	7				/* Negative command response packet */
#define GRF_TYPE_INFO		8				/* Informational message */
#define GRF_TYPE_DISCON		9				/* Disconnect */
#define GRF_TYPE_ACK		10				/* General packet acknowledgement */
#define GRF_TYPE_MAX		10

/* GRF connection attributes... */
#define GRF_ATTRIB_DATA		0x00000001		/* Data access */
#define GRF_ATTRIB_CMD		0x00000002		/* Command access */

#define GRF_DFL_TIMEOUT		(UST_SECOND * 30)

/* GRF connection codes... */
#define GRF_CON_NULL		0				/* No meaning */
#define GRF_CON_DENIED		1				/* Permission denied */
#define GRF_CON_TOO_MANY	2				/* Too many connections */
#define GRF_CON_MAX			2

/* GRF command response codes... */
#define GRF_CMD_NULL		0				/* No meaning */
#define GRF_CMD_DENIED		1				/* Permission denied */
#define GRF_CMD_UNRECOG		2				/* Command unrecognized */
#define GRF_CMD_ERROR		3				/* Error occured executing command */
#define GRF_CMD_MAX			3

/* GRF timing qualities... */
#define GRF_TIME_UNKNOWN	0				/* No idea of quality, should never happen! */
#define GRF_TIME_BAD		1				/* GPS has never locked */
#define GRF_TIME_POOR		2				/* No pps, last known correction */
#define GRF_TIME_GOOD		3				/* Lock and pps, correct to within sampling period */
#define GRF_TIME_MS			4				/* Correct to +/- 0.5 ms */
#define GRF_TIME_MAX		4

/* GRF data types... */
#define GRF_DATA_INT32		0				/* Signed 32 bit integers */
#define GRF_DATA_INT24		1				/* Packed signed 24 bit integers */
#define GRF_DATA_CM8		2				/* CM8 encoding */
#define GRF_DATA_MAX		2

/* Types --------------------------------------------------------------*/
typedef struct TAG_GRF_HDR {				/* This is the common header */
	UINT8 signature[3];						/* Always "GRF" */
	UINT8 version;							/* GRF version number, 1 */
	UINT16 length;							/* Packet length in bytes */
	UINT16 sequence;						/* Packet sequence number */
	UINT32 unit;							/* Unit serial number */
	UINT8 type;								/* See GRF_TYPE_? constants */
} GRF_HDR;

#if defined GRF_C
TEMPLATE template_grf_hdr[] = {
	{offsetof(GRF_HDR, signature), sizeof(UINT8) * 3, FALSE},
	{offsetof(GRF_HDR, version), sizeof(UINT8), FALSE},
	{offsetof(GRF_HDR, length), sizeof(UINT16), TRUE},
	{offsetof(GRF_HDR, sequence), sizeof(UINT16), TRUE},
	{offsetof(GRF_HDR, unit), sizeof(UINT32), TRUE},
	{offsetof(GRF_HDR, type), sizeof(UINT8), FALSE},
	{VOID_SIZE, VOID_SIZE, FALSE}
};
#else
extern TEMPLATE template_grf_hdr[];
#endif
#define GRF_HDR_SIZE (3 + sizeof(UINT8) + sizeof(UINT16) + sizeof(UINT16) + \
	sizeof(UINT32) + sizeof(UINT8))

/*---------------------------------------------------------------------*/
typedef struct TAG_GRF_CONNECT {
	UINT32 process_id;						/* Client process identifier */
	UINT32 attributes;						/* Connection attributes */
	USTIME time_out;						/* Connection time_out */
	CHAR *message;							/* Null terminated message text */
} GRF_CONNECT;

#if defined GRF_C
TEMPLATE template_grf_connect_hdr[] = {
	{offsetof(GRF_CONNECT, process_id), sizeof(UINT32), TRUE},
	{offsetof(GRF_CONNECT, attributes), sizeof(UINT32), TRUE},
	{offsetof(GRF_CONNECT, time_out), sizeof(USTIME), TRUE},
	{VOID_SIZE, VOID_SIZE, FALSE}
};
#else
extern TEMPLATE template_grf_connect_hdr[];
#endif
#define GRF_CONNECT_HDR_SIZE (sizeof(UINT32) + sizeof(UINT32) + sizeof(USTIME))

/*---------------------------------------------------------------------*/
typedef struct TAG_GRF_COMMAND {
	UINT16 code;							/* Response code */
	CHAR *message;							/* Null terminated command text */
} GRF_COMMAND;

#if defined GRF_C
TEMPLATE template_grf_command_hdr[] = {
	{offsetof(GRF_COMMAND, code), sizeof(UINT16), TRUE},
	{VOID_SIZE, VOID_SIZE, FALSE}
};
#else
extern TEMPLATE template_grf_command_hdr[];
#endif
#define GRF_COMMAND_HDR_SIZE (sizeof(UINT16))

/*---------------------------------------------------------------------*/
typedef struct TAG_GRF_NAME {
	CHAR network[GRF_MAX_NET_LEN + 1];		/* Network name */
	CHAR station[GRF_MAX_STN_LEN + 1];		/* Station name */
	CHAR component[GRF_MAX_COMP_LEN + 1];	/* Component ID */
} GRF_NAME;

#if defined GRF_C
TEMPLATE template_grf_name[] = {
	{offsetof(GRF_NAME, network), GRF_MAX_NET_LEN + 1, FALSE},
	{offsetof(GRF_NAME, station), GRF_MAX_STN_LEN + 1, FALSE},
	{offsetof(GRF_NAME, component), GRF_MAX_COMP_LEN + 1, FALSE},
	{VOID_SIZE, VOID_SIZE, FALSE}
};
#else
extern TEMPLATE template_grf_name[];
#endif
#define GRF_NAME_SIZE (GRF_MAX_NET_LEN + 1 + GRF_MAX_STN_LEN + 1 + GRF_MAX_COMP_LEN + 1)

/*---------------------------------------------------------------------*/
typedef struct TAG_GRF_TIMING {
	UINT8 time_quality;						/* Time quality indicator, see GRF_TIME_? constants */
	UINT8 rate_quality;						/* Rate quality... */
	USTIME time;							/* Initial sample time */
	USTIME time_correction;					/* Add to time for corrected time */
	REAL64 rate;							/* Sampling rate, samples/second */
	REAL64 rate_correction;					/* Add to rate for corrected rate */
} GRF_TIMING;

#if defined GRF_C
TEMPLATE template_grf_timing[] = {
	{offsetof(GRF_TIMING, time_quality), sizeof(UINT8), FALSE},
	{offsetof(GRF_TIMING, rate_quality), sizeof(UINT8), FALSE},
	{offsetof(GRF_TIMING, time), sizeof(USTIME), TRUE},
	{offsetof(GRF_TIMING, time_correction), sizeof(USTIME), TRUE},
	{offsetof(GRF_TIMING, rate), sizeof(REAL64), TRUE},
	{offsetof(GRF_TIMING, rate_correction), sizeof(REAL64), TRUE},
	{VOID_SIZE, VOID_SIZE, FALSE}
};
#else
extern TEMPLATE template_grf_timing[];
#endif
#define GRF_TIMING_SIZE (sizeof(UINT8) + sizeof(UINT8) + sizeof(USTIME) + sizeof(USTIME) + \
	sizeof(REAL64) + sizeof(REAL64))

/*---------------------------------------------------------------------*/
typedef struct TAG_GRF_DATA_HDR {
	UINT16 channel;							/* Channel number (1 based) */
	GRF_NAME name;							/* Name of data source */
	GRF_TIMING timing;						/* Timing information */
	INT32 counts_per_volt;					/* Conversion to volts */
	UINT8 type;								/* Data type, see GRF_DATA_? constants */
	UINT16 n;								/* Number of samples */
} GRF_DATA_HDR;

#if defined GRF_C
TEMPLATE template_grf_data_hdr[] = {
	{offsetof(GRF_DATA_HDR, channel), sizeof(UINT16), TRUE},
	{offsetof(GRF_DATA_HDR, name.network), GRF_MAX_NET_LEN + 1, FALSE},
	{offsetof(GRF_DATA_HDR, name.station), GRF_MAX_STN_LEN + 1, FALSE},
	{offsetof(GRF_DATA_HDR, name.component), GRF_MAX_COMP_LEN + 1, FALSE},
	{offsetof(GRF_DATA_HDR, timing.time_quality), sizeof(UINT8), FALSE},
	{offsetof(GRF_DATA_HDR, timing.rate_quality), sizeof(UINT8), FALSE},
	{offsetof(GRF_DATA_HDR, timing.time), sizeof(USTIME), TRUE},
	{offsetof(GRF_DATA_HDR, timing.time_correction), sizeof(USTIME), TRUE},
	{offsetof(GRF_DATA_HDR, timing.rate), sizeof(REAL64), TRUE},
	{offsetof(GRF_DATA_HDR, timing.rate_correction), sizeof(REAL64), TRUE},
	{offsetof(GRF_DATA_HDR, counts_per_volt), sizeof(INT32), TRUE},
	{offsetof(GRF_DATA_HDR, type), sizeof(UINT8), FALSE},
	{offsetof(GRF_DATA_HDR, n), sizeof(UINT16), TRUE},
	{VOID_SIZE, VOID_SIZE, FALSE}
};
#else
extern TEMPLATE template_grf_data_hdr[];
#endif
#define GRF_DATA_HDR_SIZE (sizeof(UINT16) + GRF_NAME_SIZE + GRF_TIMING_SIZE + \
	sizeof(UINT32) + sizeof(UINT8) + sizeof(UINT16))

/*---------------------------------------------------------------------*/
typedef struct TAG_GRF_PACKET {
	/* Headers in host order... */
	GRF_HDR hdr;							/* Common header */
	union {									/* Union of all payload header types */
		GRF_DATA_HDR data;					/* Data packet */
		GRF_CONNECT connect;				/* Connection packet */
		GRF_COMMAND command;				/* Command packet */
	} payload_hdr;

	/* Serialized packet storage... */
	union {
		UINT8 bytes[GRF_MAX_PACKET_LEN];	/* Raw bytes... */
		struct {
			UINT8 headers[GRF_HDR_SIZE + GRF_DATA_HDR_SIZE];
			union {							/* Sample data... */
				UINT8 bytes[GRF_MAX_PACKET_LEN - (GRF_HDR_SIZE + GRF_DATA_HDR_SIZE)];
				INT32 ints[GRF_MAX_INT32S];
			} samples;
		} data;
		struct {
			UINT8 headers[GRF_HDR_SIZE + GRF_CONNECT_HDR_SIZE];
			CHAR message[GRF_MAX_MESSAGE_LEN + 1];
		} connect;
		struct {
			UINT8 headers[GRF_HDR_SIZE + GRF_COMMAND_HDR_SIZE];
			CHAR message[GRF_MAX_MESSAGE_LEN + 1];
		} command;
		struct {
			UINT8 header[GRF_HDR_SIZE];
			CHAR message[GRF_MAX_MESSAGE_LEN + 1];
		} info;
	} serial;
} GRF_PACKET;

/* Sequence checking list record */
typedef struct TAG_GRF_UNIT {
	struct TAG_GRF_UNIT *next;
	BOOL started;
	UINT32 unit_id;
	UINT16 channel;
	UINT16 sequence;
	UINT32 count;
	UINT32 seq_errors;
	UINT32 time_tears;
	USTIME expect_time;

    FILE *fileptr;
    CHAR filename[MAX_PATH_LEN + 1];
    USTIME start_time;
    USTIME length;
} GRF_UNIT;

/* Macros -------------------------------------------------------------*/

/* Sequence number comparison macros... */
#define GRF_SEQ_LT(x, y)   ((INT16)((x) - (y)) < 0)
#define GRF_SEQ_LTEQ(x, y) ((INT16)((x) - (y)) <= 0)
#define GRF_SEQ_GT(x, y)   ((INT16)((x) - (y)) > 0)
#define GRF_SEQ_GTEQ(x, y) ((INT16)((x) - (y)) >= 0)

/* Prototypes ---------------------------------------------------------*/
VOID GRFSetUnitNumber(UINT32 unit);
UINT32 GRFGetUnitNumber(void);
const CHAR *GRFDescribeType(UINT8 type);
const CHAR *GRFDescribeQuality(UINT8 quality);
const CHAR *GRFDescribeDataType(UINT8 data_type);
CHAR *GRFInfoPacket(GRF_PACKET * packet, CHAR *format, ...);
BOOL GRFCompareName(GRF_NAME * name1, GRF_NAME * name2);

BOOL GRFReadFile(FILE *file, GRF_PACKET * packet);
BOOL GRFWriteFile(FILE *file, GRF_PACKET * packet);

BOOL GRFReadSocket(SOCKET socket, GRF_PACKET * packet, USTIME timeout);
BOOL GRFWriteSocket(SOCKET socket, GRF_PACKET * packet, USTIME timeout);

BOOL GRFDecodeSamples(GRF_PACKET * packet, INT32 *samples, UINT16 n);
BOOL GRFEncodeSamples(GRF_PACKET * packet, INT32 *samples, UINT16 n);

BOOL GRFConnect(SOCKET fd, ENDPOINT * endpoint, UINT32 attributes, USTIME timeout, GRF_PACKET * packet);
BOOL GRFDisconnect(SOCKET fd, GRF_PACKET * packet);

UINT16 ComputeCRC16(VOID *ptr, size_t length);
UINT16 UpdateCRC16(UINT16 crc, UINT8 byte);
UINT16 SerializePacket(GRF_PACKET * packet);
BOOL DeserializePacket(GRF_PACKET * packet);

#endif
