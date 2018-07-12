/* $Id: main.c 6803 2016-09-09 06:06:39Z et $ */
/*-----------------------------------------------------------------------

	GRF to Earthworm data source module...

    Copyright (c) 2000-2007 - DAQ Systems, LLC. - All rights reserved.

-----------------------------------------------------------------------*/

#include "main.h"
#include "mem.h"
#include "config.h"

/* Module constants ---------------------------------------------------*/

/* Module types -------------------------------------------------------*/

/* Module globals -----------------------------------------------------*/
static BOOL quit = FALSE;
static BOOL attached = FALSE;
static TracePacket tbuf;

static CHAR *data_types[] = {
	"INT32",
	"INT24",
	"CM8"
};

/* Module macros ------------------------------------------------------*/

/* Module prototypes --------------------------------------------------*/
static BOOL GRFToEarthworm(MAIN_ARGS * args);
static VOID SendHeartBeat(MAIN_ARGS * args);
static BOOL ReadPacket(MAIN_ARGS * args, GRF_PACKET * packet);
static BOOL WritePacket(MAIN_ARGS * args, GRF_PACKET * packet);

static BOOL OpenInput(MAIN_ARGS * args);
static VOID CloseInput(MAIN_ARGS * args);
static BOOL OpenOutput(MAIN_ARGS * args);
static VOID CloseOutput(MAIN_ARGS * args);

static BOOL SetSignalHandlers(void);
static VOID CatchSignal(int sig);

/*---------------------------------------------------------------------*/
VOID MainSetDefaults(MAIN_ARGS * args)
{
	CHAR *ptr;

	ASSERT(args != NULL);

	args->input_spec[0] = '\0';
	args->debug = FALSE;
	args->correct_rate = FALSE;
	args->trace_buf2 = TRUE;
	args->min_quality = 2;
	args->read_timeout = DFL_TIMEOUT;
	args->heartbeat = DFL_HEARTBEAT;
	strcpy(args->ring.name, DFL_RING);
	strcpy(args->ring.module, DFL_MODULE_NAME);

	ptr = getenv("EW_INSTALLATION");
	if (ptr != NULL) {
		if (ptr[0] != '\0') {
			strcpy(args->ring.inst_id, ptr);
		}
	} else {
		strcpy(args->ring.inst_id, DFL_INST_ID);
	}

	args->file = NULL;
	memset(&args->endpoint, 0, sizeof(ENDPOINT));
	args->socket = INVALID_SOCKET;

	return;
}

/*---------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	MAIN_ARGS args;

	if (!SetSignalHandlers())
		abort();

	if (!SocketsInit())
		abort();

	args.argc = argc;
	args.argv = argv;
	if (!Configure(&args))
		exit(1);

	if (isdigit((int)args.ring.inst_id[0])) {
		args.ring.logo.instid = (UINT8)atoi(args.ring.inst_id);
	} else if (GetInst(args.ring.inst_id, &args.ring.logo.instid) != 0) {
		fprintf(stderr, "grf2ew: ERROR: %s is an invalid module ID\n", args.ring.module);
		return FALSE;
	}
	args.ring.logo_hb.instid = args.ring.logo.instid;

	if (GetModId(args.ring.module, &args.ring.logo.mod) != 0) {
		fprintf(stderr, "grf2ew: ERROR: %s is an invalid module ID\n", args.ring.module);
		return FALSE;
	}
	args.ring.logo_hb.mod = args.ring.logo.mod;

	logit_init("grf2ew", args.ring.logo.mod, 1024, 1);
	logit("ot", "grf2ew: version %s (%s %s) startup, pid:%d\n", VERSION, __DATE__, __TIME__, getpid());

	DumpConfiguration(&args);
	logit("ot", "grf2ew: GRF data source: %s\n", args.input_spec);

	while (!QuitRequested(&args)) {
		if (OpenInput(&args))
			break;
		USSleep(UST_SECOND * 5);
		CloseInput(&args);
	}

	if (!OpenOutput(&args)) {
		CloseInput(&args);
		exit(1);
	}

	if (!GRFToEarthworm(&args)) {
		CloseInput(&args);
		CloseOutput(&args);
		exit(1);
	}

	CloseInput(&args);
	CloseOutput(&args);

	return 0;
}

/*--------------------------------------------------------------------- */
static BOOL GRFToEarthworm(MAIN_ARGS * args)
{
	BOOL ok;
	GRF_PACKET *packet;
	UINT16 sequence;
	USTIMER heartbeat;
	GRF_UNIT *list = NULL, *unit;

	if (!AllocateMemory((VOID **)&packet, GRF_MAX_PACKET_LEN)) {
		logit("et", "grf2ew: ERROR: GRFLog: AllocateMemory failed!\n");
		return FALSE;
	}

	StartUSTimer(&heartbeat, 0);

	ok = TRUE;
	sequence = 0;
	while (!QuitRequested(args)) {
		/* Periodically send heartbeat */
		if (USTimerIsExpired(&heartbeat)) {
			SendHeartBeat(args);
			StartUSTimer(&heartbeat, args->heartbeat);
		}
		/* Read a packet... */
		if (ReadPacket(args, packet)) {
			if (packet->hdr.length == 0)
				continue;

			/* Check sequence numbers */
			for (unit = list; unit != NULL; unit = unit->next) {
				if (packet->hdr.unit == unit->unit_id)
					break;
			}
			if (unit == NULL) {
				if (!AllocateMemory((VOID **)&unit, sizeof(GRF_UNIT))) {
					logit("et", "grf2ew: ERROR: Unable to allocate memory!\n");
					break;
				}
				unit->unit_id = packet->hdr.unit;
				unit->sequence = packet->hdr.sequence;
				unit->next = list;
				list = unit;
			}
			if (packet->hdr.sequence != unit->sequence) {
				logit("et", "grf2ew: Sequence break on unit %u, got:%u, expected: %u\n",
					unit->unit_id, packet->hdr.sequence, unit->sequence);
			}
			unit->sequence = packet->hdr.sequence + 1;

			if (!WritePacket(args, packet)) {
				ok = FALSE;
				break;
			}
		}
		/* If we're reading a socket... */
		else if (args->file == NULL) {
			while (!QuitRequested(args)) {
				CloseInput(args);
				/* Try to reconnect */
				logit("ot", "grf2ew: Attempting to reconnect...\n");
				if (OpenInput(args))
					break;
				USSleep(UST_SECOND * 5);
			}
		}
		else {
			break;
		}
	}

	ReleaseMemory(packet);

	/* Release sequence check list */
	while (list != NULL) {
		unit = list;
		list = list->next;
		ReleaseMemory(unit);
	}

	return ok;
}

/*--------------------------------------------------------------------- */
static VOID SendHeartBeat(MAIN_ARGS * args)
{
	INT32 status, length;
	static CHAR message[32];

	ASSERT(args != NULL);

	sprintf(message, "%lld %d\n", (long long)(SystemUSTime() / UST_SECOND), getpid());
	length = strlen(message);

	RequestMutex();
	status = tport_putmsg(&args->ring.shm, &args->ring.logo_hb, length, message);
	ReleaseMutex_ew();

	if (status != PUT_OK) {
		logit("et", "grf2ew: TYPE_HEARTBEAT tport_putmsg error (ignored)\n");
	}

	return;
}

/*--------------------------------------------------------------------- */
static BOOL ReadPacket(MAIN_ARGS * args, GRF_PACKET * packet)
{

	ASSERT(args != NULL);
	ASSERT(packet != NULL);

	/* Read from file? */
	if (args->file != NULL) {
		if (!GRFReadFile(args->file, packet))
			return FALSE;
		/* End-of-file? */
		if (packet->hdr.type == GRF_TYPE_NONE && packet->hdr.length == 0) {
			RequestQuit(TRUE);
			return FALSE;
		}
	}
	else {
		if (!GRFReadSocket(args->socket, packet, args->read_timeout))
			return FALSE;
	}

	return TRUE;
}

/*--------------------------------------------------------------------- */
static BOOL WritePacket(MAIN_ARGS * args, GRF_PACKET * packet)
{
	CHAR string[32];
	USTIME time;
	REAL64 rate;
	INT32 status, length;
	UINT8 *ptr;

	ASSERT(args != NULL);
	ASSERT(packet != NULL);

	if (packet->hdr.type != GRF_TYPE_DATA)
		return TRUE;

	if (packet->payload_hdr.data.timing.time_quality < args->min_quality)
		return TRUE;

	time = packet->payload_hdr.data.timing.time + packet->payload_hdr.data.timing.time_correction;
	if (args->correct_rate) 
		rate = packet->payload_hdr.data.timing.rate + packet->payload_hdr.data.timing.rate_correction;
	else
		rate = packet->payload_hdr.data.timing.rate;

	if (args->debug) {
		logit("ot", "grf2ew: TRACEBUF%s %s:%s:%s %05u %s (%s) %.3lf %u %s\n",
			(args->trace_buf2 ? "2" : ""),
			packet->payload_hdr.data.name.network,
			packet->payload_hdr.data.name.station,
			packet->payload_hdr.data.name.component,
			packet->hdr.sequence,
			FormatUSTime(string, 32, UST_DATE_TIME_MSEC, time),
			GRFDescribeQuality(packet->payload_hdr.data.timing.time_quality),
			rate, 
			packet->payload_hdr.data.n, 
			data_types[packet->payload_hdr.data.type]
		);
	}

	/* Build a tracebuf message... */
	memset(&tbuf, 0, MAX_TRACEBUF_SIZ);
	if (args->trace_buf2) {
		tbuf.trh2.pinno = (INT32)packet->payload_hdr.data.channel;
		tbuf.trh2.nsamp = (INT32)packet->payload_hdr.data.n;
		tbuf.trh2.starttime = (REAL64)time * 0.000001;
		tbuf.trh2.endtime = tbuf.trh2.starttime + ((REAL64)(tbuf.trh2.nsamp - 1) / rate);
		tbuf.trh2.samprate = rate;
		strncpy(tbuf.trh2.sta, packet->payload_hdr.data.name.station, TRACE2_STA_LEN);
		strncpy(tbuf.trh2.net, packet->payload_hdr.data.name.network, TRACE2_NET_LEN);
		strncpy(tbuf.trh2.chan, packet->payload_hdr.data.name.component, TRACE2_CHAN_LEN);
		strcpy(tbuf.trh2.loc, LOC_NULL_STRING);
#if defined BIG_ENDIAN_HOST
		strcpy(tbuf.trh2.datatype, "s4");
#else
		strcpy(tbuf.trh2.datatype, "i4");
#endif
	} else {
		tbuf.trh.pinno = (INT32)packet->payload_hdr.data.channel;
		tbuf.trh.nsamp = (INT32)packet->payload_hdr.data.n;
		tbuf.trh.starttime = (REAL64)time * 0.000001;
		tbuf.trh.endtime = tbuf.trh.starttime + ((REAL64)(tbuf.trh.nsamp - 1) / rate);
		tbuf.trh.samprate = rate;
		strncpy(tbuf.trh.sta, packet->payload_hdr.data.name.station, TRACE_STA_LEN);
		strncpy(tbuf.trh.net, packet->payload_hdr.data.name.network, TRACE_NET_LEN);
		strncpy(tbuf.trh.chan, packet->payload_hdr.data.name.component, TRACE_CHAN_LEN);
#if defined BIG_ENDIAN_HOST
		strcpy(tbuf.trh.datatype, "s4");
#else
		strcpy(tbuf.trh.datatype, "i4");
#endif
	}
	ptr = (UINT8 *) (tbuf.msg + sizeof(TRACE_HEADER));
	length = sizeof(TRACE_HEADER) + (packet->payload_hdr.data.n * sizeof(INT32));

	if (!GRFDecodeSamples(packet, (INT32 *)ptr, packet->payload_hdr.data.n))
		return FALSE;

	RequestMutex();
	status = tport_putmsg(&args->ring.shm, &args->ring.logo, length, tbuf.msg);
	ReleaseMutex_ew();
	if (status != PUT_OK) {
		logit("et", "grf2ew: TYPE_TRACEBUF tport_putmsg error (ignored)\n");
	}

	return TRUE;
}

/*--------------------------------------------------------------------- */
static BOOL OpenOutput(MAIN_ARGS * args)
{
	INT32 key;

	ASSERT(args != NULL);

	CreateMutex_ew();

	if ((key = GetKey(args->ring.name)) < 0) {
		logit("et", "grf2ew: ERROR: Can't get %s key!\n", args->ring.name);
		return FALSE;
	}

	if (args->trace_buf2) {
		if (GetType("TYPE_TRACEBUF2", &args->ring.logo.type) != 0) {
			logit("et", "grf2ew: ERROR: TRACEBUF is an invalid message type!\n");
			return FALSE;
		}
	} else {
		if (GetType("TYPE_TRACEBUF", &args->ring.logo.type) != 0) {
			logit("et", "grf2ew: ERROR: TRACEBUF is an invalid message type!\n");
			return FALSE;
		}
	}
	if (GetType("TYPE_HEARTBEAT", &args->ring.logo_hb.type) != 0) {
		logit("et", "grf2ew: ERROR: TRACEBUF is an invalid message type!\n");
		return FALSE;
	}

	tport_attach(&args->ring.shm, key);
	attached = TRUE;
	logit("ot", "grf2ew: Attached to %s memory region %ld\n", args->ring.name, key);

	return TRUE;
}

/*--------------------------------------------------------------------- */
static VOID CloseOutput(MAIN_ARGS * args)
{
	ASSERT(args != NULL);

	if (attached) {
		tport_detach(&args->ring.shm);
		attached = FALSE;
	}

	return;
}

/*--------------------------------------------------------------------- */
static BOOL OpenInput(MAIN_ARGS * args)
{
	CHAR string[128];
	BOOL result;
	GRF_PACKET *packet;

	ASSERT(args != NULL);

	/* Is it a file or a socket */
	if (args->endpoint.sin_addr.s_addr != 0) {
		if ((args->socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
			logit("et", "grf2ew: ERROR: OpenInput: socket: %s\n", SOCK_ERRSTRING(SOCK_ERRNO()));
			return FALSE;
		}
		args->endpoint.sin_family = AF_INET;
		if (connect(args->socket, (struct sockaddr *)&args->endpoint, sizeof(ENDPOINT)) != 0) {
			logit("et", "grf2ew: ERROR: OpenInput: connect(%s): %s\n",
				FormatEndpoint(&args->endpoint, string), SOCK_ERRSTRING(SOCK_ERRNO()));
			SOCK_CLOSE(args->socket);
			args->socket = INVALID_SOCKET;
			return FALSE;
		}
		if (!AllocateMemory((VOID **)&packet, GRF_MAX_PACKET_LEN)) {
			logit("et", "grf2ew: ERROR: GRFLog: AllocateMemory failed!\n");
			SOCK_CLOSE(args->socket);
			args->socket = INVALID_SOCKET;
			return FALSE;
		}
		if ((result = GRFConnect(args->socket, &args->endpoint, GRF_ATTRIB_DATA, args->read_timeout, packet)) == FALSE) {
			SOCK_CLOSE(args->socket);
			args->socket = INVALID_SOCKET;
		}
		ReleaseMemory(packet);
		return result;
	}
	else {
		if ((args->file = fopen(args->input_spec, "rb")) == NULL) {
			logit("et", "grf2ew: ERROR: OpenInput: fopen(%s) failed: %s\n", args->input_spec, strerror(errno));
			return FALSE;
		}
	}

	return TRUE;
}

/*--------------------------------------------------------------------- */
static VOID CloseInput(MAIN_ARGS * args)
{
	static GRF_PACKET packet;

	ASSERT(args != NULL);

	if (args->file != NULL)
		fclose(args->file);
	else if (args->socket != INVALID_SOCKET) {
		GRFDisconnect(args->socket, &packet);
		SOCK_CLOSE(args->socket);
		args->socket = INVALID_SOCKET;
	}

	return;
}

/*--------------------------------------------------------------------- */
BOOL QuitRequested(MAIN_ARGS * args)
{
	ASSERT(args != NULL);

	/* Check Earthworm termination flag */
	if (attached) {
		if (tport_getflag(&args->ring.shm) == TERMINATE) {
			logit("ot", "grf2ew: Earthworm is shutting down, exiting\n");
			quit = TRUE;
		}
	}

	return quit;
}

/*--------------------------------------------------------------------- */
VOID RequestQuit(BOOL request)
{
	quit = request;
	return;
}

/*--------------------------------------------------------------------- */
static BOOL SetSignalHandlers(void)
{
	if (signal(SIGTERM, CatchSignal) == SIG_ERR) {
		perror("signal(SIGTERM)");
		return FALSE;
	}
	if (signal(SIGINT, CatchSignal) == SIG_ERR) {
		perror("signal(SIGINT)");
		return FALSE;
	}
#if !defined WIN32
	/* Ignore the SIGPIPE signal on UNIX */
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		perror("signal(SIGPIPE)");
		return FALSE;
	}
#endif

	return TRUE;
}

/*--------------------------------------------------------------------- */
static VOID CatchSignal(int sig)
{
	switch (sig) {
	  case SIGTERM:
		fprintf(stderr, "\rQuitting on SIGTERM!    \n");
		quit = TRUE;
		break;
	  case SIGINT:
		fprintf(stderr, "\rQuitting on SIGINT!     \n");
		quit = TRUE;
		break;
	  default:
		fprintf(stderr, "\rCaught unexpected signal %d, ignored!     \n", sig);
		break;
	}

	signal(sig, CatchSignal);
}
