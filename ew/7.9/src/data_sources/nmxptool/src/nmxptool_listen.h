/*! \file
 *
 * \brief Nanometrics Protocol Tool
 *
 * Author:
 * 	Matteo Quintiliani
 * 	Istituto Nazionale di Geofisica e Vulcanologia - Italy
 *	quintiliani@ingv.it
 *
 * $Id $
 *
 */

#ifndef NMXPTOOL_LISTEN_H
#define NMXPTOOL_LISTEN_H 1

#include <nmxp.h>

int nmxptool_occ_inc(int inc);
void *nmxptool_p_man_sockfd(void *arg);
void *nmxptool_listen(void *arg);
int nmxptool_listen_print_seq_no(NMXP_DATA_PROCESS *pd);

#endif

