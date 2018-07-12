/*! \file
 *
 * \brief Nanometrics Protocol Tool
 *
 * Author:
 * 	Matteo Quintiliani
 * 	Istituto Nazionale di Geofisica e Vulcanologia - Italy
 *	quintiliani@ingv.it
 *
 * $Id: nmxptool_chanseq.h 4165 2011-01-24 15:19:28Z quintiliani $
 *
 */

#ifndef NMXPTOOL_CHANSEQ_H
#define NMXPTOOL_CHANSEQ_H 1

#include <nmxp.h>

typedef struct {
    int significant;
    double last_time;
    time_t last_time_call_raw_stream;
    int32_t x_1;
    double after_start_time;
    NMXP_RAW_STREAM_DATA raw_stream_buffer;
} NMXPTOOL_CHAN_SEQ;

void nmxptool_chanseq_init(NMXPTOOL_CHAN_SEQ **pchan_list_seq, int number, double default_after_start_time, int32_t max_tolerable_latency, int32_t timeoutrecv);
void nmxptool_chanseq_free(NMXPTOOL_CHAN_SEQ **pchan_list_seq, int number);
int  nmxptool_chanseq_check_and_log_gap(double time1, double time2, const double gap_tollerance, const char *station, const char *channel, const char *network);
int nmxptool_chanseq_gap(NMXPTOOL_CHAN_SEQ *chan_list_seq_item, NMXP_DATA_PROCESS *pd);

void nmxptool_chanseq_save_states(NMXP_CHAN_LIST_NET *chan_list, NMXPTOOL_CHAN_SEQ *chan_list_seq, char *statefile);
void nmxptool_chanseq_load_states(NMXP_CHAN_LIST_NET *chan_list, NMXPTOOL_CHAN_SEQ *chan_list_seq, char *statefile, int32_t stc);

#endif

