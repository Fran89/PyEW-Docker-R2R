/* $Id: config.h 3145 2007-11-06 17:04:22Z paulf $ */
/*-----------------------------------------------------------------------
    Copyright (c) 2000-2007 - DAQ Systems, LLC. - All rights reserved.
-------------------------------------------------------------------------

	Configuration...

-----------------------------------------------------------------------*/
#if !defined _CONFIG_H_INCLUDED_
#define _CONFIG_H_INCLUDED_

#include "main.h"

/* Constants ----------------------------------------------------------*/
#define DFL_CONF_FILE 		"grf2ew.d"

/* Types --------------------------------------------------------------*/
typedef struct TAG_CMDLINE_ARGS {
	BOOL daemon;
	BOOL debug;
	BOOL correct_rate;
	CHAR message_format;
	USTIME read_timeout;
	USTIME heartbeat;
	CHAR input_spec[MAX_PATH_LEN + 1];
	CHAR ring[MAX_RING_NAME_LEN + 1];
	CHAR facility[MAX_PATH_LEN + 1];
	CHAR logfile[MAX_PATH_LEN + 1];
	CHAR confile[MAX_PATH_LEN + 1];
	ENDPOINT endpoint;
} CMDLINE_ARGS;

/* Macros -------------------------------------------------------------*/

/* Prototypes ---------------------------------------------------------*/
BOOL Configure(MAIN_ARGS * args);
VOID DumpConfiguration(MAIN_ARGS * args);

#endif
