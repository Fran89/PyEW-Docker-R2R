/*! \file
 *
 * \brief Nanometrics Protocol Tool
 *
 * Author:
 * 	Matteo Quintiliani
 * 	Istituto Nazionale di Geofisica e Vulcanologia - Italy
 *	quintiliani@ingv.it
 *
 * $Id: nmxptool_chanseq.c 4965 2012-07-22 07:12:34Z quintiliani $
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <nmxp.h>
#include "nmxptool_chanseq.h"
#include "nmxptool_getoptlong.h"

void nmxptool_chanseq_init(NMXPTOOL_CHAN_SEQ **pchan_list_seq, int number, double default_after_start_time, int32_t max_tolerable_latency, int32_t timeoutrecv) {
    int i_chan;
    NMXPTOOL_CHAN_SEQ *chan_list_seq;

    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CHANNEL, "Init chan_list_seq.\n");

    /* init chan_list_seq */
    chan_list_seq = (NMXPTOOL_CHAN_SEQ *) NMXP_MEM_MALLOC(sizeof(NMXPTOOL_CHAN_SEQ) * number);
    for(i_chan = 0; i_chan < number; i_chan++) {
	chan_list_seq[i_chan].significant = 0;
	chan_list_seq[i_chan].last_time = 0.0;
	chan_list_seq[i_chan].last_time_call_raw_stream = 0;
	chan_list_seq[i_chan].x_1 = 0;
	chan_list_seq[i_chan].after_start_time = default_after_start_time;
	nmxp_raw_stream_init(&(chan_list_seq[i_chan].raw_stream_buffer), max_tolerable_latency, timeoutrecv);
    }

    *pchan_list_seq = chan_list_seq;
}


void nmxptool_chanseq_free(NMXPTOOL_CHAN_SEQ **pchan_list_seq, int number) {
    int i_chan;
    NMXPTOOL_CHAN_SEQ *chan_list_seq = NULL;

    chan_list_seq = *pchan_list_seq;
    if(chan_list_seq) {

	for(i_chan = 0; i_chan < number; i_chan++) {
	    nmxp_raw_stream_free(&(chan_list_seq[i_chan].raw_stream_buffer));
	}

	NMXP_MEM_FREE(chan_list_seq);
	*pchan_list_seq = NULL;
    }
}


int nmxptool_chanseq_check_and_log_gap(double time1, double time2, const double gap_tollerance, const char *station, const char *channel, const char *network) {
    char str_time1[200];
    char str_time2[200];
    int ret = 0;
    double gap = time1 - time2 ;
    nmxp_data_to_str(str_time1, time1);
    nmxp_data_to_str(str_time2, time2);
    if(gap > gap_tollerance) {
	nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_ANY, "Gap %.2f sec. for %s.%s.%s from %s to %s!\n",
		gap, NMXP_LOG_STR(network), NMXP_LOG_STR(station), NMXP_LOG_STR(channel), NMXP_LOG_STR(str_time2), NMXP_LOG_STR(str_time1));
	ret = 1;
    } else if (gap < -gap_tollerance) {
	nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_ANY, "Overlap %.2f sec. for %s.%s.%s from %s to %s!\n",
		gap, NMXP_LOG_STR(network), NMXP_LOG_STR(station), NMXP_LOG_STR(channel), NMXP_LOG_STR(str_time1), NMXP_LOG_STR(str_time2));
	ret = -1;
    }
    return ret;
}


#define GAP_TOLLERANCE 0.001
int nmxptool_chanseq_gap(NMXPTOOL_CHAN_SEQ *chan_list_seq_item, NMXP_DATA_PROCESS *pd) {
    int ret = 0;
    char str_pd_time[200] = "";
    if(!chan_list_seq_item->significant && pd->nSamp > 0) {
	chan_list_seq_item->significant = 1;
    } else {
	if(chan_list_seq_item->significant && pd->nSamp > 0) {
	    ret = nmxptool_chanseq_check_and_log_gap(pd->time, chan_list_seq_item->last_time, GAP_TOLLERANCE, pd->station, pd->channel, pd->network);
	    if(ret != 0) {
		chan_list_seq_item->x_1 = 0;
		nmxp_data_to_str(str_pd_time, pd->time);
		nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_EXTRA, "%s.%s x0 set to zero at %s!\n",
			NMXP_LOG_STR(pd->station), NMXP_LOG_STR(pd->channel), NMXP_LOG_STR(str_pd_time));
	    }
	}
    }
    if(chan_list_seq_item->significant && pd->nSamp > 0) {
	chan_list_seq_item->last_time = pd->time + ((double) pd->nSamp / (double) pd->sampRate);
    }
    return ret;
}


#define MAX_LEN_FILENAME 4096
void nmxptool_chanseq_save_states(NMXP_CHAN_LIST_NET *chan_list, NMXPTOOL_CHAN_SEQ *chan_list_seq, char *statefile) {
    int to_cur_chan;
    char last_time_str[30];
    char raw_last_sample_time_str[30];
    char state_line_str[1000];
    FILE *fstatefile = NULL;
    char statefilefilename[MAX_LEN_FILENAME] = "";

    if(chan_list == NULL  ||  chan_list_seq == NULL) {
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CHANSTATE, "nmxptool_chanseq_save_states() channel lists are NULL!\n");
	return;
    }

    if(statefile) {
	strncpy(statefilefilename, statefile, MAX_LEN_FILENAME);
	strncat(statefilefilename, NMXP_STR_STATE_EXT, MAX_LEN_FILENAME - strlen(statefilefilename));
	fstatefile = fopen(statefilefilename, "w");
	if(fstatefile == NULL) {
	    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CHANSTATE, "Unable to write channel states into %s!\n",
		    NMXP_LOG_STR(statefilefilename));
	} else {
	    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CHANSTATE, "Writing channel states into %s!\n",
		    NMXP_LOG_STR(statefilefilename));
	}

	/* Save state for each channel */
	to_cur_chan = 0;
	while(to_cur_chan < chan_list->number) {
	    nmxp_data_to_str(last_time_str, chan_list_seq[to_cur_chan].last_time);
	    nmxp_data_to_str(raw_last_sample_time_str, chan_list_seq[to_cur_chan].raw_stream_buffer.last_sample_time);
	    sprintf(state_line_str, "%10d %s %s %s",
		    chan_list->channel[to_cur_chan].key,
		    chan_list->channel[to_cur_chan].name,
		    last_time_str,
		    raw_last_sample_time_str
		   );
	    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CHANSTATE, "%s\n", NMXP_LOG_STR(state_line_str));
	    if(fstatefile) {
		fprintf(fstatefile, "%s\n", state_line_str);
		if( (chan_list_seq[to_cur_chan].last_time != 0) || (chan_list_seq[to_cur_chan].raw_stream_buffer.last_sample_time != -1.0) ) {
		    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CHANSTATE, "%s %d %d %f %f\n",
			    NMXP_LOG_STR(state_line_str), to_cur_chan, chan_list->channel[to_cur_chan].key,
			    chan_list_seq[to_cur_chan].last_time, chan_list_seq[to_cur_chan].raw_stream_buffer.last_sample_time);
		} else {
		    /* Do nothing */
		}
	    }
	    to_cur_chan++;
	}
	if(fstatefile) {
	    fclose(fstatefile);
	}
    }
}


void nmxptool_chanseq_load_states(NMXP_CHAN_LIST_NET *chan_list, NMXPTOOL_CHAN_SEQ *chan_list_seq, char *statefile, int32_t stc) {
    FILE *fstatefile = NULL;
    FILE *fstatefileINPUT = NULL;
#define MAXSIZE_LINE 2048
    char line[MAXSIZE_LINE];
    char s_chan[128];
    char s_noraw_time_s[128];
    char s_rawtime_s[128];
    double s_noraw_time_f_calc, s_rawtime_f_calc;
    int cur_chan;
    int n_scanf;
    int32_t key_chan;
    NMXP_TM_T tmp_tmt;
    char statefilefilename[MAX_LEN_FILENAME] = "";

    if(statefile) {
	strncpy(statefilefilename, statefile, MAX_LEN_FILENAME);
	strncat(statefilefilename, NMXP_STR_STATE_EXT, MAX_LEN_FILENAME - strlen(statefile));
	fstatefile = fopen(statefilefilename, "r");
	if(fstatefile == NULL) {
	    fstatefileINPUT = fopen(statefile, "r");
	    if(fstatefileINPUT == NULL) {
		nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CHANSTATE, "Unable to read channel states from %s!\n",
			NMXP_LOG_STR(statefile));
	    } else {
		fstatefile = fopen(statefilefilename, "w");
		if(fstatefile == NULL) {
		    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CHANSTATE, "Unable to write channel states into %s!\n",
			    NMXP_LOG_STR(statefilefilename));
		} else {
		    /*
		    while(fgets(line, MAXSIZE_LINE, fstatefileINPUT) != NULL) {
			fputs(line, fstatefile);
		    }
		    */
		    fclose(fstatefile);
		}
		fclose(fstatefileINPUT);
	    }
	} else {
	    fclose(fstatefile);
	}
    }

    if(statefile) {
	strncpy(statefilefilename, statefile, MAX_LEN_FILENAME);
	strncat(statefilefilename, NMXP_STR_STATE_EXT, MAX_LEN_FILENAME - strlen(statefile));
	fstatefile = fopen(statefilefilename, "r");
	if(fstatefile == NULL) {
	    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CHANSTATE, "Unable to read channel states from %s!\n",
		    NMXP_LOG_STR(statefilefilename));
	} else {
	    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CHANSTATE, "Loading channel states from %s!\n",
		    NMXP_LOG_STR(statefilefilename));
	    while(fgets(line, MAXSIZE_LINE, fstatefile) != NULL) {
		s_chan[0] = 0;
		s_noraw_time_s[0] = 0;
		s_rawtime_s[0] = 0;
		n_scanf = sscanf(line, "%d %s %s %s", &key_chan, s_chan, s_noraw_time_s, s_rawtime_s); 

		s_noraw_time_f_calc = DEFAULT_BUFFERED_TIME;
		s_rawtime_f_calc = DEFAULT_BUFFERED_TIME;
		if(n_scanf == 4) {
		    if(nmxp_data_parse_date(s_noraw_time_s, &tmp_tmt) == -1) {
			nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CHANSTATE, "Parsing time '%s'\n", NMXP_LOG_STR(s_noraw_time_s)); 
		    } else {
			s_noraw_time_f_calc = nmxp_data_tm_to_time(&tmp_tmt);
		    }
		    if(nmxp_data_parse_date(s_rawtime_s, &tmp_tmt) == -1) {
			nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CHANSTATE, "Parsing time '%s'\n", NMXP_LOG_STR(s_rawtime_s)); 
		    } else {
			s_rawtime_f_calc = nmxp_data_tm_to_time(&tmp_tmt);
		    }
		}

		nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CHANSTATE, "%d %12d %-14s %16.4f %s %16.4f %s\n",
			n_scanf, key_chan, s_chan,
			s_noraw_time_f_calc, NMXP_LOG_STR(s_noraw_time_s),
			s_rawtime_f_calc, NMXP_LOG_STR(s_rawtime_s)); 

		cur_chan = 0;
		while(cur_chan < chan_list->number  &&  strcasecmp(s_chan, chan_list->channel[cur_chan].name) != 0) {
		    cur_chan++;
		}
		if(cur_chan < chan_list->number) {
		    if( s_rawtime_f_calc != DEFAULT_BUFFERED_TIME  && s_rawtime_f_calc != 0.0 ) {
			chan_list_seq[cur_chan].last_time                          = s_rawtime_f_calc;
			chan_list_seq[cur_chan].raw_stream_buffer.last_sample_time = s_rawtime_f_calc;
		    } else if( s_noraw_time_f_calc != DEFAULT_BUFFERED_TIME ) {
			chan_list_seq[cur_chan].last_time                          = s_noraw_time_f_calc;
			chan_list_seq[cur_chan].raw_stream_buffer.last_sample_time = s_noraw_time_f_calc;
		    } else {
			nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CHANSTATE, "For channel %s there is not valid start_time.\n",
				NMXP_LOG_STR(s_chan)); 
		    }
		    if(stc == -1) {
			chan_list_seq[cur_chan].after_start_time                   = s_rawtime_f_calc;
			nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CHANSTATE, "For channel %s (%d %d) starting from %s. %f.\n",
				NMXP_LOG_STR(s_chan), cur_chan, chan_list->channel[cur_chan].key,
				NMXP_LOG_STR(s_rawtime_s), s_rawtime_f_calc); 
		    } else {
			chan_list_seq[cur_chan].after_start_time                   = s_noraw_time_f_calc;
			nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CHANSTATE, "For channel %s (%d %d) starting from %s. %f.\n",
				NMXP_LOG_STR(s_chan), cur_chan, chan_list->channel[cur_chan].key,
				NMXP_LOG_STR(s_noraw_time_s), s_noraw_time_f_calc); 
		    }
		} else {
		    if(line[strlen(line)-1] == 10  || line[strlen(line)-1] == 13) {
			line[strlen(line)-1] = 0;
		    }
		    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CHANSTATE, "Channel %s not found! (%d %s)\n",
			    NMXP_LOG_STR(s_chan), strlen(line), NMXP_LOG_STR(line)); 
		}
	    }
	    fclose(fstatefile);
	}
    }
    errno = 0;
}

