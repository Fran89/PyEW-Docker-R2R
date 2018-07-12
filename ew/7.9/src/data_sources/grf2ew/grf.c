/* $Id: grf.c 5714 2013-08-05 19:18:08Z paulf $ */
/*-----------------------------------------------------------------------
    Copyright (c) 2000-2007 - DAQ Systems, LLC. - All rights reserved.
-------------------------------------------------------------------------

	GRF functions...

-----------------------------------------------------------------------*/

#define GRF_C	/* Causes serialization templates to be declared */

#include "main.h"
#include "sock.h"
#include "grf.h"
#include "mem.h"

/* Module constants ---------------------------------------------------*/
#define CRC_INITIAL         0
#define CRC_GOOD            0

/* Module types -------------------------------------------------------*/

/* Module globals -----------------------------------------------------*/
static BOOL initialized = FALSE;
static MUTEX mutex;
static UINT32 unit_number = 0;

static const CHAR *qualities[GRF_TIME_MAX + 2] = {
	"Unknown",
	"Bad",
	"Poor",
	"Good",
	"Great",
	"Invalid quality"
};
static const CHAR *data_types[GRF_DATA_MAX + 2] = {
	"32 bit signed integer",
	"24 bit packed signed integer",
	"CM8 compression",
	"Invalid data type"
};
static const CHAR *packet_types[GRF_TYPE_MAX + 2] = {
	"Undefined",
	"Data",
	"Connection Request",
	"Connection Acknowledge",
	"Connection Negative Acknowledge",
	"Command",
	"Command Acknowledge",
	"Command Negative Acknowledge",
	"Information",
	"Disconnect Request",
	"Invalid packet type"
};
static const UINT32 powers_of_two[32] = {
	0x00000001, 0x00000002, 0x00000004, 0x00000008,
	0x00000010, 0x00000020, 0x00000040, 0x00000080,
	0x00000100, 0x00000200, 0x00000400, 0x00000800,
	0x00001000, 0x00002000, 0x00004000, 0x00008000,
	0x00010000, 0x00020000, 0x00040000, 0x00080000,
	0x00100000, 0x00200000, 0x00400000, 0x00800000,
	0x01000000, 0x02000000, 0x04000000, 0x08000000,
	0x10000000, 0x20000000, 0x40000000, 0x80000000
};

/* 
CRC table parameters:
    Width      = 16 bits
    Polynomial = 0x1021 (x^16+x^12+x^5+1, standard CCITT)
    Reflection = no
*/
static const UINT16 crc_table[256] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
	0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
	0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
	0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
	0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
	0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
	0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
	0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
	0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
	0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
	0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
	0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
	0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
	0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
	0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
	0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
	0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
	0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

/* Module macros ------------------------------------------------------*/

/* Module prototypes --------------------------------------------------*/
static VOID Initialize(void);
static UINT16 ComputeLength(GRF_PACKET * packet);
static INT32 ReadSocket(SOCKET socket, UINT8 *ptr, size_t *length, USTIME timeout);
static UINT32 PowerOfTwo(UINT32 exponent);

/*---------------------------------------------------------------------*/
static VOID Initialize(void)
{
	if (!initialized) {
		MUTEX_INIT(&mutex);
		initialized = TRUE;
	}

	return;
}

/*---------------------------------------------------------------------*/
CHAR *GRFInfoPacket(GRF_PACKET * packet, CHAR *format, ...)
{
	va_list marker;

	ASSERT(packet != NULL);
	ASSERT(format != NULL);

	memcpy(packet->hdr.signature, GRF_SIGNATURE, 3);
	packet->hdr.version = GRF_VERSION;
	packet->hdr.length = 0;
	packet->hdr.sequence = 0;
	packet->hdr.unit = GRFGetUnitNumber();
	packet->hdr.type = GRF_TYPE_INFO;

	va_start(marker, format);
	vsnprintf(packet->serial.info.message, GRF_MAX_MESSAGE_LEN, format, marker);
	va_end(marker);

	return packet->serial.info.message;
}

/*---------------------------------------------------------------------*/
BOOL GRFCompareName(GRF_NAME * name1, GRF_NAME * name2)
{

	ASSERT(name1 != NULL);
	ASSERT(name2 != NULL);

	if (name1->network[0] == '*' || name2->network[0] == '*' || strcmp(name1->network, name2->network) == 0) {
		if (name1->station[0] == '*' || name2->station[0] == '*' || strcmp(name1->station, name2->station) == 0) {
			if (name1->component[0] == '*' || name2->component[0] == '*' || strcmp(name1->component, name2->component) == 0) {
				return TRUE;
			}
		}
	}

	return FALSE;
}

/*---------------------------------------------------------------------*/
const CHAR *GRFDescribeType(UINT8 type)
{
	if (!initialized)
		Initialize();

	if (type > GRF_TYPE_MAX)
		type = GRF_TYPE_MAX + 1;

	return packet_types[type];
}

/*---------------------------------------------------------------------*/
const CHAR *GRFDescribeQuality(UINT8 quality)
{
	if (!initialized)
		Initialize();

	if (quality > GRF_TIME_MAX)
		quality = GRF_TIME_MAX + 1;

	return qualities[quality];
}

/*---------------------------------------------------------------------*/
const CHAR *GRFDescribeDataType(UINT8 data_type)
{
	if (!initialized)
		Initialize();

	if (data_type > GRF_DATA_MAX)
		data_type = GRF_DATA_MAX + 1;

	return data_types[data_type];
}

/*---------------------------------------------------------------------*/
VOID GRFSetUnitNumber(UINT32 unit)
{
	if (!initialized)
		Initialize();

	MUTEX_LOCK(&mutex);
	unit_number = unit;
	MUTEX_UNLOCK(&mutex);

	return;
}

/*---------------------------------------------------------------------*/
UINT32 GRFGetUnitNumber(void)
{
	UINT32 unit;

	if (!initialized)
		Initialize();

	MUTEX_LOCK(&mutex);
	unit = unit_number;
	MUTEX_UNLOCK(&mutex);

	return unit;
}

/*---------------------------------------------------------------------*/
BOOL GRFWriteFile(FILE *file, GRF_PACKET * packet)
{
	ASSERT(packet != NULL);

	if (!initialized)
		Initialize();

	if (file == NULL)
		return TRUE;

	packet->hdr.length = ComputeLength(packet);
	if (packet->hdr.length > GRF_MAX_PACKET_LEN) {
		logit("et", "grf2ew: ERROR: GRFWriteFile: Packet length exceeds maximum: %u\n", packet->hdr.length);
		return FALSE;
	}

	SerializePacket(packet);

	if (fwrite(packet->serial.bytes, (size_t)packet->hdr.length, 1, file) != 1) {
		logit("et", "grf2ew: ERROR: GRFWriteFile: fwrite failed: %s\n", strerror(errno));
		return FALSE;
	}

	return TRUE;
}

/*---------------------------------------------------------------------*/
BOOL GRFReadFile(FILE *file, GRF_PACKET * packet)
{
	UINT16 length;

	ASSERT(file != NULL);

	/* Read the common header */
	if (fread(packet->serial.bytes, GRF_HDR_SIZE, 1, file) != 1) {
		if (feof(file)) {
			packet->hdr.type = GRF_TYPE_NONE;
			packet->hdr.length = 0;
			return TRUE;
		}
		else {
			logit("et", "grf2ew: ERROR: GRFReadFile: fread failed: %s\n", strerror(errno));
			return FALSE;
		}
	}

	/* Check signature, version, and length... */
	if (memcmp(packet->serial.bytes, GRF_SIGNATURE, 3) != 0) {
		logit("et", "grf2ew: ERROR: GRFReadFile: Unrecognized packet signature: '%.3s'\n", packet->serial.bytes);
		return FALSE;
	}
	if (packet->serial.bytes[3] < GRF_VERSION) {
		logit("et", "grf2ew: ERROR: GRFReadFile: GRF version mismatch: %u != %u\n", packet->serial.bytes[3], GRF_VERSION);
		return FALSE;
	}
	length = ntohs(*((UINT16 *)&packet->serial.bytes[4]));
	if (length < GRF_HDR_SIZE) {
		logit("et", "grf2ew: ERROR: GRFReadFile: Packet length error: too short: %u < %u\n", length, GRF_HDR_SIZE);
		return FALSE;
	}
	if (length > GRF_MAX_PACKET_LEN) {
		logit("et", "grf2ew: ERROR: GRFReadFile: Packet length error: too long: %u > %u\n", length, GRF_MAX_PACKET_LEN);
		return FALSE;
	}

	/* Read the rest of the packet... */
	if (fread(&packet->serial.bytes[GRF_HDR_SIZE], length - GRF_HDR_SIZE, 1, file) != 1) {
		if (feof(file)) {
			logit("et", "grf2ew: WARNING: GRFReadFile: Premature end-of-file encountered!\n");
			packet->hdr.type = GRF_TYPE_NONE;
			packet->hdr.length = 0;
			return TRUE;
		}
		else {
			logit("et", "grf2ew: ERROR: GRFReadFile: fread failed: %s\n", strerror(errno));
			return FALSE;
		}
	}

	return DeserializePacket(packet);
}

/*---------------------------------------------------------------------*/
BOOL GRFWriteSocket(SOCKET socket, GRF_PACKET * packet, USTIME timeout)
{
	int maxfd, result, this_write;
	fd_set write_set;
	struct timeval tv;
	size_t remaining, so_far;
	USTIMER timer;
	USTIME elapsed;

	ASSERT(socket != INVALID_SOCKET);
	ASSERT(packet != NULL);

	if (!initialized)
		Initialize();

	packet->hdr.length = ComputeLength(packet);
	if (packet->hdr.length > GRF_MAX_PACKET_LEN) {
		logit("et", "grf2ew: ERROR: GRFWriteSocket: Packet length exceeds maximum: %u\n", packet->hdr.length);
		return FALSE;
	}

	SerializePacket(packet);

	remaining = packet->hdr.length;
	so_far = 0;

	StartUSTimer(&timer, timeout);
	while (remaining > 0) {

		/* See if the socket is writable... */
		FD_ZERO(&write_set);
		FD_SET(socket, &write_set);
		maxfd = socket + 1;
		elapsed = USTimerElapsed(&timer);
		if (timeout > elapsed)
			timeout -= elapsed;
		else
			timeout = 0;
		tv.tv_sec = (long)(timeout / UST_SECOND);
		tv.tv_usec = (long)(timeout % UST_SECOND);
		if ((result = select(maxfd, NULL, &write_set, NULL, &tv)) < 0) {
			if (errno == EINTR)
				continue;
			else {
				logit("et", "grf2ew: ERROR: GRFWriteSocket: select failed: %s\n", SOCK_ERRSTRING(SOCK_ERRNO()));
				return FALSE;
			}
		}

		if (result > 0) {
			/* Write to socket */
			if ((this_write = send(socket, (char *)packet->serial.bytes + so_far, packet->hdr.length, 0)) < 0) {
			/*if ((this_write = send(socket, (char *)packet->serial.bytes + so_far, packet->hdr.length, 0)) < 0) {*/
#if 1
				if (SOCK_ERRNO() != ECONNRESET && SOCK_ERRNO() != ECONNABORTED && SOCK_ERRNO() != EPIPE) {
					logit("et", "grf2ew: ERROR: GRFWriteSocket: send failed: %s\n", SOCK_ERRSTRING(SOCK_ERRNO()));
				}
#else
				logit("et", "grf2ew: ERROR: GRFWriteSocket: send failed: %s\n", SOCK_ERRSTRING(SOCK_ERRNO()));
#endif
				return FALSE;
			}
			remaining -= this_write;
			so_far += this_write;
		}
		else {
			logit("et", "grf2ew: ERROR: GRFWriteSocket: write timeout!\n");
			return FALSE;
		}
	}

	return TRUE;
}

/*---------------------------------------------------------------------*/
BOOL GRFReadSocket(SOCKET socket, GRF_PACKET * packet, USTIME timeout)
{
	INT32 result;
	UINT16 length;
	size_t read_length;

	ASSERT(packet != NULL);

	/* Read the common header */
	read_length = GRF_HDR_SIZE;
	result = ReadSocket(socket, packet->serial.bytes, &read_length, timeout);
	if (result < 0)
		return FALSE;
	else if (result == 0) {
		/* Timed out */
		packet->hdr.type = GRF_TYPE_NONE;
		packet->hdr.length = 0;
		return FALSE;
	}
	else if (result == 2) {
		/* End of file (server disconnected) */
		packet->hdr.type = GRF_TYPE_NONE;
		packet->hdr.length = 0;
		return FALSE;
	}

	/* Check signature, version, and length... */
	if (memcmp(packet->serial.bytes, GRF_SIGNATURE, 3) != 0) {
		logit("et", "grf2ew: ERROR: GRFReadSocket: Unrecognized packet signature: '%.3s'\n", packet->serial.bytes);
		return FALSE;
	}
	if (packet->serial.bytes[3] < GRF_VERSION) {
		logit("et", "grf2ew: ERROR: GRFReadSocket: GRF version mismatch: %u != %u\n", packet->serial.bytes[3], GRF_VERSION);
		return FALSE;
	}
	length = ntohs(*((UINT16 *)&packet->serial.bytes[4]));
	if (length < GRF_HDR_SIZE) {
		logit("et", "grf2ew: ERROR: GRFReadSocket: Packet length error: too short: %u < %u\n", length, GRF_HDR_SIZE);
		return FALSE;
	}
	if (length > GRF_MAX_PACKET_LEN) {
		logit("et", "grf2ew: ERROR: GRFReadSocket: Packet length error: too long: %u > %u\n", length, GRF_MAX_PACKET_LEN);
		return FALSE;
	}

	/* Read the rest of the packet... */
	read_length = (size_t)(length - GRF_HDR_SIZE);
	result = ReadSocket(socket, &packet->serial.bytes[GRF_HDR_SIZE], &read_length, timeout);
	if (result < 0)
		return FALSE;
	else if (result == 0) {
		/* Timed out */
		packet->hdr.type = GRF_TYPE_NONE;
		packet->hdr.length = 0;
		return FALSE;
	}
	else if (result == 2) {
		/* End of file (server disconnected) */
		packet->hdr.type = GRF_TYPE_NONE;
		packet->hdr.length = 0;
		return FALSE;
	}

	return DeserializePacket(packet);
}

/*---------------------------------------------------------------------*/
BOOL GRFDecodeSamples(GRF_PACKET * packet, INT32 *samples, UINT16 n)
{
	UINT8 *ptr, sign = 0;
	UINT16 i, n_codes;
	UINT32 magnitude = 0;

	ASSERT(packet != NULL);
	ASSERT(samples != NULL);

	if (packet->payload_hdr.data.n < n)
		n = packet->payload_hdr.data.n;

	if (packet->payload_hdr.data.type == GRF_DATA_INT32) {
		for (i = 0; i < n; i++)
			samples[i] = ntohl(packet->serial.data.samples.ints[i]);
	}
	else if (packet->payload_hdr.data.type == GRF_DATA_INT24) {
		ptr = (UINT8 *)packet->serial.data.samples.ints;
		for (i = 0; i < n; i++) {
			samples[i] = ((INT32)*ptr++ & 0x000000FF) << 16;
			samples[i] |= ((INT32)*ptr++ & 0x000000FF) << 8;
			samples[i] |= ((INT32)*ptr++ & 0x000000FF);
			if (samples[i] & 0x00800000)
				samples[i] |= 0xFF000000;
		}
	}
	else if (packet->payload_hdr.data.type == GRF_DATA_CM8) {
		ptr = packet->serial.data.samples.bytes;
		if (ComputeCRC16(ptr, packet->hdr.length - (GRF_HDR_SIZE + GRF_DATA_HDR_SIZE)) != CRC_GOOD) {
			logit("et", "grf2ew: ERROR: GRFDecodeSamples: CM8 data CRC failure!\n");
			return FALSE;
		}
		i = 0;
		n_codes = 0;
		while (i < n) {
			/* Build up sign and magnitude from codes */
			if (n_codes == 0) {
				sign = (*ptr) & (UINT8)PowerOfTwo(6);
				magnitude = (*ptr) & (UINT8)(PowerOfTwo(6) - 1);
			}
			else
				magnitude = (magnitude << 7) | ((*ptr) & (UINT8)(PowerOfTwo(7) - 1));

			/* Clear control bit indicates end of sample */
			if (((*ptr) & (UINT8)PowerOfTwo(7)) == 0) {
				samples[i++] = (sign ? -(INT32)magnitude : (INT32)magnitude);
				n_codes = 0;
			}
			else {
				n_codes++;
				if (n_codes > 5) {
					logit("et", "grf2ew: ERROR: GRFDecodeSamples: CM8 decompression error!\n");
					return FALSE;
				}
			}
			ptr++;
		}
		/* Integrate twice... */
		for (i = 1; i < n; i++)
			samples[i] += samples[i - 1];
		for (i = 1; i < n; i++)
			samples[i] += samples[i - 1];
	}
	else {
		logit("et", "grf2ew: ERROR: GRFDecodeSamples: unsupported data type: %u!\n", packet->payload_hdr.data.type);
		return FALSE;
	}

	return TRUE;
}

/*---------------------------------------------------------------------*/
BOOL GRFEncodeSamples(GRF_PACKET * packet, INT32 *samples, UINT16 n)
{
	UINT8 *ptr, sign, control;
	UINT16 i, j, shift, crc;
	UINT32 magnitude, mask;
	INT32 xi, xim1, xim2;

	ASSERT(packet != NULL);
	ASSERT(samples != NULL);

	if (packet->payload_hdr.data.type == GRF_DATA_INT32) {
		for (i = 0; i < n; i++)
			packet->serial.data.samples.ints[i] = htonl(samples[i]);
		packet->payload_hdr.data.n = n;
		packet->hdr.length = GRF_HDR_SIZE + GRF_DATA_HDR_SIZE + (n * 4);
	}
	else if (packet->payload_hdr.data.type == GRF_DATA_INT24) {
		ptr = packet->serial.data.samples.bytes;
		for (i = 0; i < n; i++) {
			*ptr++ = (samples[i] & 0x00FF0000) >> 16;
			*ptr++ = (samples[i] & 0x0000FF00) >> 8;
			*ptr++ = samples[i] & 0x000000FF;
		}
		packet->payload_hdr.data.n = n;
		packet->hdr.length = GRF_HDR_SIZE + GRF_DATA_HDR_SIZE + (n * 3);
	}
	else if (packet->payload_hdr.data.type == GRF_DATA_CM8) {
		ptr = packet->serial.data.samples.bytes;
		packet->hdr.length = GRF_HDR_SIZE + GRF_DATA_HDR_SIZE;
		packet->payload_hdr.data.n = 0;
		xim1 = xim2 = 0;
		for (j = 0; j < n; j++) {
			/* Second difference... */
			xi = samples[j] - (2 * xim1) + xim2;
			xim2 = xim1;
			xim1 = samples[j];

			/* Convert to sign and magnitude... */
			if (xi < 0) {
				/* Save sign in bit 6 */
				sign = (UINT8)PowerOfTwo(6);
				magnitude = (UINT32)(-xi);
			}
			else {
				sign = 0;
				magnitude = (UINT32)xi;
			}
			/* Compute starting point and number of codes */
			i = 6;
			shift = 0;
			control = 0;
			while (magnitude >= PowerOfTwo(i) && i < 32) {
				i += 7;
				shift += 7;
				control = PowerOfTwo(7);
			}
			/* Encode most significant 6 data bits with control bit (bit 7) set if
			   more to come and saved sign bit (bit 6) */
			mask = PowerOfTwo(6) - 1;
			packet->serial.bytes[packet->hdr.length++] = ((UINT8)((magnitude >> shift) & mask)) | sign | control;
			/* Encode remaining 7 bit segments with set control bit (bit 7) */
			mask = PowerOfTwo(7) - 1;
			control = PowerOfTwo(7);
			while (shift > 0) {
				shift -= 7;
				/* If this is the last segment, clear the control bit */
				if (shift == 0)
					control = 0;
				packet->serial.bytes[packet->hdr.length++] = ((UINT8)((magnitude >> shift) & mask)) | control;
			}
			packet->payload_hdr.data.n++;

			/* Is this packet getting full? */
			if (packet->hdr.length > GRF_MAX_CM8_LEN - 5 - 2)
				break;
		}
	}
	else {
		logit("et", "grf2ew: ERROR: EncodeSamples: unsupported data type: %u!", packet->payload_hdr.data.type);
		return FALSE;
	}

	if (packet->payload_hdr.data.type == GRF_DATA_CM8) {
		/* Compute and attach CRC to end of data... */
		ptr = packet->serial.data.samples.bytes;
		crc = ComputeCRC16(ptr, packet->hdr.length - (GRF_HDR_SIZE + GRF_DATA_HDR_SIZE));
		packet->serial.bytes[packet->hdr.length++] = (UINT8)((crc & 0xFF00) >> 8);
		packet->serial.bytes[packet->hdr.length++] = (UINT8)(crc & 0x00FF);
	}

	return TRUE;
}

/*--------------------------------------------------------------------- */
BOOL GRFConnect(SOCKET fd, ENDPOINT * endpoint, UINT32 attributes, USTIME timeout, GRF_PACKET * packet)
{
	CHAR string[32];

	ASSERT(endpoint != NULL);
	ASSERT(packet != NULL);

	/* Fill in the headers... */
	memcpy(packet->hdr.signature, GRF_SIGNATURE, 3);
	packet->hdr.version = GRF_VERSION;
	packet->hdr.length = 0;
	packet->hdr.sequence = 0;
	packet->hdr.unit = 0;
	packet->hdr.type = GRF_TYPE_CON_REQ;

	packet->payload_hdr.connect.process_id = (UINT32)getpid();
	packet->payload_hdr.connect.attributes = attributes;
	packet->payload_hdr.connect.time_out = timeout;

	packet->serial.connect.message[0] = '\0';

	/* Send the connection request */
	if (!GRFWriteSocket(fd, packet, GRF_DFL_TIMEOUT))
		return FALSE;

	/* Read the response */
	if (!GRFReadSocket(fd, packet, GRF_DFL_TIMEOUT))
		return FALSE;

	if (packet->hdr.type == GRF_TYPE_CON_ACK) {
		logit("ot", "grf2ew: Connected to unit %u (%s) at %s\n",
			packet->hdr.unit, packet->serial.connect.message, FormatEndpoint(endpoint, string));
	}
	else if (packet->hdr.type == GRF_TYPE_CON_NAK) {
		logit("ot", "grf2ew: Error connecting to unit %u at %s: %s\n",
			packet->hdr.unit, FormatEndpoint(endpoint, string), packet->serial.connect.message);
		return FALSE;
	}
	else {
		logit("ot", "grf2ew: Error connecting to %s: no response from server!\n", FormatEndpoint(endpoint, string));
		return FALSE;
	}

	return TRUE;
}

/*--------------------------------------------------------------------- */
BOOL GRFDisconnect(SOCKET fd, GRF_PACKET * packet)
{
	ASSERT(packet != NULL);

	/* Fill in the headers... */
	memcpy(packet->hdr.signature, GRF_SIGNATURE, 3);
	packet->hdr.version = GRF_VERSION;
	packet->hdr.length = 0;
	packet->hdr.sequence = 0;
	packet->hdr.unit = 0;
	packet->hdr.type = GRF_TYPE_DISCON;

	packet->payload_hdr.connect.process_id = (UINT32)getpid();
	packet->payload_hdr.connect.attributes = 0;
	packet->payload_hdr.connect.time_out = 0;

	packet->serial.connect.message[0] = '\0';

	/* Send the disconnect request */
	if (!GRFWriteSocket(fd, packet, GRF_DFL_TIMEOUT))
		return FALSE;

	return TRUE;
}

/*---------------------------------------------------------------------*/
BOOL DeserializePacket(GRF_PACKET * packet)
{
	size_t actual;

	actual = Deserialize(&packet->hdr, template_grf_hdr, packet->serial.bytes);

	if (packet->hdr.type == GRF_TYPE_INFO)
		return actual;

	if (packet->hdr.type == GRF_TYPE_DATA)
		Deserialize(&packet->payload_hdr.data, template_grf_data_hdr, &packet->serial.bytes[actual]);
	else if (packet->hdr.type == GRF_TYPE_CON_REQ ||
		packet->hdr.type == GRF_TYPE_CON_ACK || packet->hdr.type == GRF_TYPE_CON_NAK || packet->hdr.type == GRF_TYPE_DISCON)
		Deserialize(&packet->payload_hdr.connect, template_grf_connect_hdr, &packet->serial.bytes[actual]);
	else if (packet->hdr.type == GRF_TYPE_CMD || packet->hdr.type == GRF_TYPE_CMD_ACK || packet->hdr.type == GRF_TYPE_CMD_NAK)
		Deserialize(&packet->payload_hdr.command, template_grf_command_hdr, &packet->serial.bytes[actual]);
	else {
		logit("et", "grf2ew: ERROR: DeserializePacket: Unrecognized packet type: %u\n", packet->hdr.type);
		return FALSE;
	}

	return TRUE;
}

/*---------------------------------------------------------------------*/
UINT16 SerializePacket(GRF_PACKET * packet)
{
	size_t actual;

	actual = Serialize(&packet->hdr, template_grf_hdr, packet->serial.bytes);

	if (packet->hdr.type == GRF_TYPE_INFO)
		return actual;

	if (packet->hdr.type == GRF_TYPE_DATA)
		actual += Serialize(&packet->payload_hdr.data, template_grf_data_hdr, &packet->serial.bytes[actual]);
	else if (packet->hdr.type == GRF_TYPE_CON_REQ ||
		packet->hdr.type == GRF_TYPE_CON_ACK || packet->hdr.type == GRF_TYPE_CON_NAK || packet->hdr.type == GRF_TYPE_DISCON)
		actual += Serialize(&packet->payload_hdr.connect, template_grf_connect_hdr, &packet->serial.bytes[actual]);
	else if (packet->hdr.type == GRF_TYPE_CMD || packet->hdr.type == GRF_TYPE_CMD_ACK || packet->hdr.type == GRF_TYPE_CMD_NAK)
		actual += Serialize(&packet->payload_hdr.command, template_grf_command_hdr, &packet->serial.bytes[actual]);
	else
		logit("et", "grf2ew: ERROR: GRF: SerializePacket: Unrecognized packet type: %u\n", packet->hdr.type);

	return actual;
}

/*---------------------------------------------------------------------*/
static UINT16 ComputeLength(GRF_PACKET * packet)
{
	UINT16 length = 0;

	if (packet->hdr.type == GRF_TYPE_DATA)
		length = packet->hdr.length;
	else if (packet->hdr.type == GRF_TYPE_CON_REQ ||
		packet->hdr.type == GRF_TYPE_CON_ACK || packet->hdr.type == GRF_TYPE_CON_NAK || packet->hdr.type == GRF_TYPE_DISCON)
		length = GRF_HDR_SIZE + GRF_CONNECT_HDR_SIZE + strlen(packet->serial.connect.message) + 1;
	else if (packet->hdr.type == GRF_TYPE_CMD || packet->hdr.type == GRF_TYPE_CMD_ACK || packet->hdr.type == GRF_TYPE_CMD_NAK)
		length = GRF_HDR_SIZE + GRF_COMMAND_HDR_SIZE + strlen(packet->serial.command.message) + 1;
	else if (packet->hdr.type == GRF_TYPE_INFO)
		length = GRF_HDR_SIZE + strlen(packet->serial.info.message) + 1;
	else
		logit("et", "grf2ew: ERROR: GRF: ComputeLength: Unrecognized packet type: %u\n", packet->hdr.type);

	return length;
}

/*---------------------------------------------------------------------*/
static INT32 ReadSocket(SOCKET socket, UINT8 *ptr, size_t *length, USTIME timeout)
{
	int maxfd, result, this_read;
	fd_set read_set;
	struct timeval tv;
	size_t remaining;
	USTIMER timer;
	USTIME elapsed;

	ASSERT(ptr != NULL);
	ASSERT(length != NULL);

	remaining = *length;
	*length = 0;

	StartUSTimer(&timer, timeout);
	while (remaining > 0) {

		/* See if there's something to read... */
		FD_ZERO(&read_set);
		FD_SET(socket, &read_set);
		maxfd = socket + 1;
		elapsed = USTimerElapsed(&timer);
		if (timeout > elapsed)
			timeout -= elapsed;
		else
			timeout = 0;
		tv.tv_sec = (long)(timeout / UST_SECOND);
		tv.tv_usec = (long)(timeout % UST_SECOND);
		if ((result = select(maxfd, &read_set, NULL, NULL, &tv)) < 0) {
			if (errno == EINTR)
				continue;
			else {
				logit("et", "grf2ew: ERROR: ReadSocket: select failed: %s\n", SOCK_ERRSTRING(SOCK_ERRNO()));
				return -1;
			}
		}

		if (result > 0) {
			if ((this_read = recv(socket, ptr + (*length), remaining, 0)) <= 0) {
				if (this_read == 0)
					return 2;
				logit("et", "grf2ew: ERROR: ReadSocket: recv failed: %s\n", SOCK_ERRSTRING(SOCK_ERRNO()));
				return -1;
			}
			(*length) += this_read;
			remaining -= this_read;
		}
		else {
			/* Timed out */
			return 0;
		}
	}

	return 1;
}

/*---------------------------------------------------------------------*/
UINT16 ComputeCRC16(VOID *ptr, size_t length)
{
	UINT8 *byte;
	UINT16 crc;

	byte = (UINT8 *)ptr;
	crc = CRC_INITIAL;

	while (length--)
		crc = (crc << 8) ^ crc_table[(crc >> 8) ^ *byte++];

	return crc;
}

/*---------------------------------------------------------------------*/
static UINT32 PowerOfTwo(UINT32 exponent)
{
	if (exponent < 32)
		return powers_of_two[exponent];

	return 0;
}
