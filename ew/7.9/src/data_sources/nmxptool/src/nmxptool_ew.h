/*! \file
 *
 * \brief Earthworm support for Nanometrics Protocol Tool
 *
 * Author:
 * 	Matteo Quintiliani
 * 	Istituto Nazionale di Geofisica e Vulcanologia - Italy
 *	quintiliani@ingv.it
 *
 * $Id $
 *
 */

#ifndef NMXPTOOL_EW_H
#define NMXPTOOL_EW_H 1

#include "nmxp.h"

/* Earthworm includes */
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <trace_buf.h>

/* TODO is number of different kinds of error */
#define NMXPTOOL_EW_ERR_MAXVALUE 3

#define NMXPTOOL_EW_ERR_NULL		0
#define NMXPTOOL_EW_ERR_RECVDATA	1
#define NMXPTOOL_EW_ERR_TERMREQ		2

#define NMXPTOOL_EW_MAXSZE_MSG 1024

#define _LOGITMT 1


typedef struct {
    unsigned int error;
    char message[NMXPTOOL_EW_MAXSZE_MSG];
} NMXPTOOL_EW_ERR_MSG;

void nmxptool_ew_attach();
void nmxptool_ew_detach();

int nmxptool_ew_pd2ewring (NMXP_DATA_PROCESS *pd, SHM_INFO *pregionOut, MSG_LOGO *pwaveLogo);

int nmxptool_ew_nmx2ew(NMXP_DATA_PROCESS *pd);

void nmxptool_ew_configure (char ** argvec, NMXPTOOL_PARAMS *params);

int nmxptool_ew_proc_configfile (char * configfile, NMXPTOOL_PARAMS *params);

void nmxptool_ew_report_status ( MSG_LOGO *pLogo, short code, char * message );

int nmxptool_ew_check_flag_terminate();
void nmxptool_ew_send_heartbeat_if_needed();
void nmxptool_ew_send_error(unsigned int ierr, char *message, const char *hostname);

int nmxptool_ew_logit_msg ( char *msg );
int nmxptool_ew_logit_err (  char *msg );

#endif

