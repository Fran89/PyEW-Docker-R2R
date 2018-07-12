/* $Id: main.h 3145 2007-11-06 17:04:22Z paulf $ */
/*-----------------------------------------------------------------------
    Copyright (c) 2000-2007 - DAQ Systems, LLC. - All rights reserved.
-------------------------------------------------------------------------

	GRF to Earthworm main module include...

-----------------------------------------------------------------------*/
#if !defined _MAIN_H_INCLUDED_
#define _MAIN_H_INCLUDED_

/* Earthworm includes... */
#include <time_ew.h>
#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>

#include "platform.h"
#include "sock.h"
#include "grf.h"
#include "version.h"

/* Constants ----------------------------------------------------------*/
#define DFL_PORT		 		GRF_REGISTERED_PORT
#define DFL_TIMEOUT				GRF_DFL_TIMEOUT

#define DFL_HEARTBEAT			(UST_SECOND * 10)
#define DFL_RING				"WAVE_RING"
#define DFL_MODULE_NAME			"MOD_GRF2EW"
#define DFL_INST_ID				"INST_WILDCARD"

#define MAX_RING_NAME_LEN		32

/* Types --------------------------------------------------------------*/
typedef struct TAG_MAIN_ARGS {
	int argc;
	char **argv;
	CHAR input_spec[MAX_PATH_LEN + 1];
	BOOL debug;
	BOOL correct_rate;
	BOOL trace_buf2;
	UINT8 min_quality;
	USTIME read_timeout;
	USTIME heartbeat;
	struct {
		CHAR name[MAX_RING_NAME_LEN + 1];
		CHAR module[MAX_RING_NAME_LEN + 1];
		CHAR inst_id[MAX_RING_NAME_LEN + 1];
		MSG_LOGO logo;
		MSG_LOGO logo_hb;
		SHM_INFO shm;
	} ring;
	FILE *file;
	ENDPOINT endpoint;
	SOCKET socket;
} MAIN_ARGS;

/* Macros -------------------------------------------------------------*/

/* Prototypes ---------------------------------------------------------*/
VOID MainSetDefaults(MAIN_ARGS * args);
BOOL QuitRequested(MAIN_ARGS * args);
VOID RequestQuit(BOOL request);
VOID HexDump(UINT32 loglevel, VOID *ptr, size_t length);

#endif
