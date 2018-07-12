/*! \file
 *
 * \brief Nanometrics Protocol Tool
 *
 * Author:
 * 	Matteo Quintiliani
 * 	Istituto Nazionale di Geofisica e Vulcanologia - Italy
 *	quintiliani@ingv.it
 *
 * $Id: nmxptool.c 6803 2016-09-09 06:06:39Z et $
 *
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <sys/stat.h>
#include <unistd.h>

#include <nmxp.h>
#include "nmxptool_getoptlong.h"
#include "nmxptool_chanseq.h"
#include "nmxptool_sigcondition.h"
#include <nmxptool_listen.h>

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#else
#warning Requests of channels could not be efficient because they do not use a separate thread. 
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#ifdef HAVE_EARTHWORMOBJS
#include "nmxptool_ew.h"
#endif

#ifdef HAVE_LIBMSEED
#include <libmseed.h>
#endif

#ifdef HAVE_SEEDLINK
#include "seedlink_plugin.h"
#endif

#define TIMES_FLOW_EXIT 100

int if_dap_condition_only_one_time = 0;

#define DAP_CONDITION(params_struct) (params_struct.start_time != 0.0 || params_struct.delay > 0)

#define EXIT_CONDITION_PRELIM (!nmxptool_sigcondition_read()  &&  !ew_check_flag_terminate  &&  !if_dap_condition_only_one_time)
#ifdef HAVE_LIBMSEED
#define EXIT_CONDITION ( EXIT_CONDITION_PRELIM  &&  data_seed.err_general==0 )
#else
#define EXIT_CONDITION ( EXIT_CONDITION_PRELIM )
#endif
		    
#define CURRENT_LOCATION ( (params.location)? params.location : DEFAULT_NULL_LOCATION )
#define LOCCODE_OR_CURRENT_LOCATION ( (location_code[0] != 0)? location_code : CURRENT_LOCATION )

#define CURRENT_NETWORK ( (params.network)? params.network : DEFAULT_NETWORK )
#define NETCODE_OR_CURRENT_NETWORK ( (network_code[0] != 0)? network_code : CURRENT_NETWORK )

static void ShutdownHandler(int sig);
static void nmxptool_AlarmHandler(int sig);
static void CloseConnectionHandler(int sig);

int nmxptool_exitcondition_on_open_socket();

void flushing_raw_data_stream();

void *nmxptool_print_info_raw_stream(void *arg);
int nmxptool_print_seq_no(NMXP_DATA_PROCESS *pd);
void nmxptool_str_time_to_filename(char *str_time);

#ifdef HAVE_LIBMSEED
int nmxptool_write_miniseed(NMXP_DATA_PROCESS *pd);
int nmxptool_log_miniseed(const char *s);
int nmxptool_logerr_miniseed(const char *s);
#endif

#ifdef HAVE_SEEDLINK
int nmxptool_send_raw_depoch(NMXP_DATA_PROCESS *pd);
#endif

#ifdef HAVE_LIBMSEED
#ifdef HAVE_SEEDLINK
void nmxptool_msr_send_mseed_handler (char *record, int reclen, void *handlerdata);
int nmxptool_msr_send_mseed(NMXP_DATA_PROCESS *pd);
#endif
#endif

#ifdef HAVE_PTHREAD_H
pthread_mutex_t mutex_sendAddTimeSeriesChannel = PTHREAD_MUTEX_INITIALIZER;
pthread_t thread_request_channels;
pthread_attr_t attr_request_channels;
void *status_thread;
void *p_nmxp_sendAddTimeSeriesChannel(void *arg);
#endif

#ifdef HAVE_PTHREAD_H
pthread_t thread_socket_listen;
pthread_attr_t attr_socket_listen;
void *status_thread_socket_listen;
int already_listen = 0;
#endif


/* Global variable for main program and handling terminitation program */
NMXPTOOL_PARAMS params={0};


int naqssock = 0;
int flag_force_close_connection = 0;
FILE *outfile = NULL;
NMXP_CHAN_LIST *channelList = NULL;
NMXP_CHAN_LIST_NET *channelList_subset = NULL;
NMXP_CHAN_LIST_NET *channelList_subset_waste = NULL;
NMXPTOOL_CHAN_SEQ *channelList_Seq = NULL;
NMXP_META_CHAN_LIST *meta_channelList = NULL;
int n_func_pd = 0;
int (*p_func_pd[NMXP_MAX_FUNC_PD]) (NMXP_DATA_PROCESS *);

time_t lasttime_pds_receiveddata;
time_t timeout_pds_receiveddata = (NMXP_HIGHEST_TIMEOUT * 2);

#ifdef HAVE_LIBMSEED
/* Mini-SEED variables */
NMXP_DATA_SEED data_seed;
MSRecord *msr_list_chan[MAX_N_CHAN];
#endif

int ew_check_flag_terminate = 0;

int main (int argc, char **argv) {
    int32_t connection_time;
    int request_SOCKET_OK;
#ifdef HAVE_LIBMSEED
    int i_chan =0;
#endif
    int cur_chan = 0;
    int to_cur_chan = 0;
    int request_chan;
    int exitpdscondition;
    int exitdapcondition;
    time_t timeout_for_channel;

    int time_to_sleep = 0;

    char str_start_time[200] = "";
    char str_end_time[200] = "";

    NMXP_MSG_SERVER type;
    char buffer[NMXP_MAX_LENGTH_DATA_BUFFER]={0};
    int32_t length;
    int ret;
    int main_ret = 0;

    int pd_null_count = 0;
    int timeoutrecv_warning = 300; /* 5 minutes */

    int times_flow = 0;

    int recv_errno = 0; 
#ifdef HAVE_EARTHWORMOBJS
    char *recv_errno_str;
#endif
  
    char filename[500] = "";
    char station_code[20] = "", channel_code[20] = "", network_code[20] = "", location_code[20] = "";

    char cur_after_start_time_str[1024];
    double cur_after_start_time = DEFAULT_BUFFERED_TIME;
    int skip_current_packet = 0;

    double default_start_time = 0.0;
    char start_time_str[30], end_time_str[30], default_start_time_str[30];

    NMXP_DATA_PROCESS *pd = NULL;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_init(&mutex_sendAddTimeSeriesChannel, NULL);
#endif

#ifndef HAVE_WINDOWS_H
    /* Signal handling, use POSIX calls with standardized semantics */
    struct sigaction sa;

    sa.sa_handler = nmxptool_AlarmHandler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, NULL);

    sa.sa_handler = CloseConnectionHandler;
    sigaction(SIGUSR1, &sa, NULL);

    sa.sa_handler = ShutdownHandler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL); 
    sigaction(SIGTERM, &sa, NULL);

    sa.sa_handler = SIG_IGN;
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGPIPE, &sa, NULL); 
#else
    /* Signal handling, use function signal() */

    /*
    signal(SIGALRM, nmxptool_AlarmHandler);
    */

    signal(SIGINT, ShutdownHandler);
    /*
    signal(SIGQUIT, ShutdownHandler);
    */
    signal(SIGTERM, ShutdownHandler);

    /*
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    */
#endif

    nmxptool_sigcondition_init();

    /* Default is normal output */
    nmxp_log(NMXP_LOG_SET, NMXP_LOG_D_NULL);

    /* Initialize params from argument values */
    if(nmxptool_getopt_long(argc, argv, &params) != 0) {
	return 1;
    }

    if(params.ew_configuration_file) {

#ifdef HAVE_EARTHWORMOBJS

	nmxp_log_init(nmxptool_ew_logit_msg, nmxptool_ew_logit_err);

	nmxptool_ew_configure(argv, &params);

	/* Check consistency of params */
	if(nmxptool_check_params(&params) != 0) {
	    return 1;
	}

#else
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY, "Earthworm feature has not been enabled in compilation!\n");
	return 1;
#endif

    } else {

	nmxp_log_init(nmxp_log_stdout, nmxp_log_stderr);

	/* Check consistency of params */
	if(nmxptool_check_params(&params) != 0) {
	    return 1;
	}

	/* List available channels on server */
	if(params.flag_listchannels) {

	    meta_channelList = nmxp_getMetaChannelList(params.hostname, params.portnumberdap, NMXP_DATA_TIMESERIES, params.flag_request_channelinfo, params.datas_username, params.datas_password, &channelList, nmxptool_exitcondition_on_open_socket);

	    /* nmxp_meta_chan_print(meta_channelList); */
	    nmxp_meta_chan_print_with_match(meta_channelList, params.channels);

	    return 1;

	} else if(params.flag_listchannelsnaqs) {

	    channelList = nmxp_getAvailableChannelList(params.hostname, params.portnumberpds, NMXP_DATA_TIMESERIES, nmxptool_exitcondition_on_open_socket);

	    /* nmxp_chan_print_channelList(channelList); */
	    nmxp_chan_print_channelList_with_match(channelList, params.channels, 1);

	    return 1;

	}
    }

    nmxp_log(NMXP_LOG_SET, params.verbose_level);
    if(params.verbose_level != DEFAULT_VERBOSE_LEVEL) {
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_ANY, "verbose_level %d\n", params.verbose_level);
    }

#ifdef HAVE_LIBMSEED
    data_seed.err_general = 0;
    if(params.type_writeseed) {
	ms_loginit((void*)&nmxptool_log_miniseed, NULL, (void*)&nmxptool_logerr_miniseed, "error: ");
	/* Init mini-SEED variables */
	nmxp_data_seed_init(&data_seed, params.outdirseed,
		CURRENT_NETWORK,
		(params.type_writeseed == TYPE_WRITESEED_BUD)? NMXP_TYPE_WRITESEED_BUD : NMXP_TYPE_WRITESEED_SDS);
    }
#endif

    nmxptool_log_params(&params);

    if(params.stc == -1) {

#ifndef HAVE_WINDOWS_H
	if(params.listen_port != DEFAULT_LISTEN_PORT) {
	    p_func_pd[n_func_pd++] = nmxptool_listen_print_seq_no;
	}
#endif

	if(params.flag_logdata) {
	    p_func_pd[n_func_pd++] = nmxptool_print_seq_no;
	}

#ifdef HAVE_LIBMSEED
	/* Write Mini-SEED record */
	if(params.type_writeseed) {
	    p_func_pd[n_func_pd++] = nmxptool_write_miniseed;
	}
#endif

#ifdef HAVE_SEEDLINK
	/* Send data to SeedLink Server */
	if(params.flag_slink) {
	    p_func_pd[n_func_pd++] = nmxptool_send_raw_depoch;
	}
#endif

#ifdef HAVE_LIBMSEED
#ifdef HAVE_SEEDLINK
	/* Send data to SeedLink Server */
	if(params.flag_slinkms) {
	    p_func_pd[n_func_pd++] = nmxptool_msr_send_mseed;
	}
#endif
#endif

#ifdef HAVE_EARTHWORMOBJS
	if(params.ew_configuration_file) {
	    p_func_pd[n_func_pd++] = nmxptool_ew_nmx2ew;
	}
#endif

    }

#ifdef HAVE_EARTHWORMOBJS
    if(params.ew_configuration_file) {
	nmxptool_ew_attach();
    }
#endif


    /* Exit only on request */
    while(EXIT_CONDITION) {

    NMXP_MEM_PRINT_PTR(0, 1);

    /* Get list of available channels and get a subset list of params.channels */
    if( DAP_CONDITION(params) ) {
	if_dap_condition_only_one_time = 1;
	/* From DataServer */
	if(!nmxp_getMetaChannelList(params.hostname, params.portnumberdap, NMXP_DATA_TIMESERIES,
		    params.flag_request_channelinfo, params.datas_username, params.datas_password, &channelList, nmxptool_exitcondition_on_open_socket)) {
	    return -1;
	}
    } else {
	/* From NaqsServer */
	channelList = nmxp_getAvailableChannelList(params.hostname, params.portnumberpds, NMXP_DATA_TIMESERIES, nmxptool_exitcondition_on_open_socket);
    }

    if(!channelList) {
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CHANNEL, "Channel list has not been received!\n");
	return 1;
    }

    channelList_subset = nmxp_chan_subset(channelList, NMXP_DATA_TIMESERIES, params.channels, CURRENT_NETWORK, CURRENT_LOCATION);
    
    /* Free the complete channel list */
    if(channelList) {
	NMXP_MEM_FREE(channelList);
	channelList = NULL;
    }

    /* Check if some channel already exists */
    if(channelList_subset->number <= 0) {
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CHANNEL, "Channels not found!\n");
	return 1;
    } else {
	nmxp_chan_print_netchannelList(channelList_subset);

	nmxptool_chanseq_init(&channelList_Seq, channelList_subset->number, DEFAULT_BUFFERED_TIME, params.max_tolerable_latency, params.timeoutrecv);

#ifdef HAVE_LIBMSEED
	if(params.type_writeseed  ||  params.flag_slinkms) {
	    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_EXTRA, "Init mini-SEED record list.\n");

	    /* Init mini-SEED record list */
	    for(i_chan = 0; i_chan < channelList_subset->number; i_chan++) {

		nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_EXTRA,
			"Init mini-SEED record for %s\n", NMXP_LOG_STR(channelList_subset->channel[i_chan].name));

		msr_list_chan[i_chan] = msr_init(NULL);

		/* Separate station_code and channel_code */
		if(nmxp_chan_cpy_sta_chan(channelList_subset->channel[i_chan].name, station_code, channel_code, network_code, location_code)) {

		    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_EXTRA, "%s.%s.%s\n",
			    NMXP_LOG_STR(NETCODE_OR_CURRENT_NETWORK), NMXP_LOG_STR(station_code), NMXP_LOG_STR(channel_code));
		    strncpy(msr_list_chan[i_chan]->network, NETCODE_OR_CURRENT_NETWORK, 11);
		    strncpy(msr_list_chan[i_chan]->station, station_code, 11);
		    strncpy(msr_list_chan[i_chan]->channel, channel_code, 11);
		    if(location_code[0] != 0) {
		      if(strcmp(location_code, DEFAULT_NULL_LOCATION) != 0) {
			strncpy(msr_list_chan[i_chan]->location, location_code, 11);
		      }
		    }

		    msr_list_chan[i_chan]->reclen   = params.reclen;     /* Byte record length */
		    msr_list_chan[i_chan]->encoding = params.encoding;  /* Steim 1 compression by default */

		    /* Reset some values */
		    msr_list_chan[i_chan]->sequence_number = 0;
		    msr_list_chan[i_chan]->datasamples = NULL;
		    msr_list_chan[i_chan]->numsamples = 0;

		} else {
		    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CHANNEL,
			    "Channels %s error in format!\n", NMXP_LOG_STR(channelList_subset->channel[i_chan].name));
		    return 1;
		}

	    }
	}
#endif

    }

    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CONNFLOW, "Begin communication.\n");

    times_flow = 0;
    recv_errno = 0;

    while(times_flow < 2  &&  recv_errno == 0 && !nmxptool_sigcondition_read()
#ifdef HAVE_LIBMSEED
	    &&  data_seed.err_general==0
#endif
	    ) {

	if(params.statefile) {
	    nmxptool_chanseq_load_states(channelList_subset, channelList_Seq, params.statefile, params.stc);
	}

	if(times_flow == 0) {
	    if(params.statefile) {
		params.interval = DEFAULT_INTERVAL_INFINITE;
	    }
	} else if(times_flow == 1) {
	    params.start_time = 0.0;
	    params.end_time = 0.0;
	    params.interval = DEFAULT_INTERVAL_NO_VALUE;

	}

    /* Condition for starting DAP or PDS */
    if( DAP_CONDITION(params) ||
	    (times_flow == 0  &&  params.statefile && params.max_data_to_retrieve > 0 && params.interval == DEFAULT_INTERVAL_INFINITE) ) {

	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CONNFLOW, "Begin DAP Flow.\n");

	if(params.interval > 0  ||  params.interval == DEFAULT_INTERVAL_INFINITE) {
	    if(params.interval > 0) {
		params.end_time = params.start_time + params.interval;
	    } else {
		params.end_time = nmxp_data_gmtime_now();
	    }
	} else if(params.delay > 0) {
	    params.start_time = ((double) (time(NULL) - params.delay - params.span_data) / 10.0) * 10.0;
	    params.end_time = params.start_time + params.span_data;
	}


	/* ************************************************************** */
	/* Start subscription protocol "DATA ACCESS PROTOCOL" version 1.0 */
	/* ************************************************************** */

	/* DAP Step 1: Open a socket */
	if( (naqssock = nmxp_openSocket(params.hostname, params.portnumberdap, nmxptool_exitcondition_on_open_socket)) == NMXP_SOCKET_ERROR) {
	    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "Error opening socket!\n");
	    return 1;
	}

	/* DAP Step 2: Read connection time */
	if(nmxp_readConnectionTime(naqssock, &connection_time) != NMXP_SOCKET_OK) {
	    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "Error reading connection time from server!\n");
	    return 1;
	}

	/* DAP Step 3: Send a ConnectRequest */
	if(nmxp_sendConnectRequest(naqssock, params.datas_username, params.datas_password, connection_time) != NMXP_SOCKET_OK) {
	    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "Error sending connect request!\n");
	    return 1;
	}

	/* DAP Step 4: Wait for a Ready message */
	if(nmxp_waitReady(naqssock) != NMXP_SOCKET_OK) {
	    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "Error waiting Ready message!\n");
	    return 1;
	}

	exitdapcondition = 1;

	default_start_time = (params.start_time > 0.0)? params.start_time : nmxp_data_gmtime_now() - params.max_data_to_retrieve;

	while(exitdapcondition  &&  !nmxptool_sigcondition_read()
#ifdef HAVE_LIBMSEED
		&&  data_seed.err_general==0
#endif
	     ) {

	    /* Start loop for sending requests */
	    request_chan=0;
	    request_SOCKET_OK = NMXP_SOCKET_OK;

	    /* For each channel */
	    while(request_SOCKET_OK == NMXP_SOCKET_OK  &&  request_chan < channelList_subset->number  &&  exitdapcondition && !nmxptool_sigcondition_read()
#ifdef HAVE_LIBMSEED
		    &&  data_seed.err_general==0
#endif
		    ) {

		if(params.statefile) {
		    if(channelList_Seq[request_chan].after_start_time > 0) {
			params.start_time = channelList_Seq[request_chan].after_start_time;
			if(params.end_time - params.start_time > params.max_data_to_retrieve) {
			    nmxp_data_to_str(start_time_str, params.start_time);
			    nmxp_data_to_str(default_start_time_str, params.end_time - params.max_data_to_retrieve);
			    nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_ANY, "%s start_time changed from %s to %s\n",
				    NMXP_LOG_STR(channelList_subset->channel[request_chan].name),
				    NMXP_LOG_STR(start_time_str),
				    NMXP_LOG_STR(default_start_time_str));
			    params.start_time = params.end_time - params.max_data_to_retrieve;
			}
		    } else {
			params.start_time = default_start_time;
		    }
		    channelList_Seq[request_chan].last_time = params.start_time;
		    channelList_Seq[request_chan].significant = 1;

		}

		nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_EXTRA, "nmxp_sendDataRequest %d %s (%d)\n",
			channelList_subset->channel[request_chan].key,
			NMXP_LOG_STR(channelList_subset->channel[request_chan].name),
			request_chan);

		nmxp_data_to_str(start_time_str, params.start_time);
		nmxp_data_to_str(end_time_str, params.end_time);
		nmxp_data_to_str(default_start_time_str, default_start_time);
		nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_EXTRA, "%s start_time = %s - end_time = %s - (default_start_time = %s)\n",
			NMXP_LOG_STR(channelList_subset->channel[request_chan].name),
			NMXP_LOG_STR(start_time_str),
			NMXP_LOG_STR(end_time_str),
			NMXP_LOG_STR(default_start_time_str));

		/* DAP Step 5: Send Data Request */
		request_SOCKET_OK = nmxp_sendDataRequest(naqssock, channelList_subset->channel[request_chan].key, (int32_t) params.start_time, (int32_t) (params.end_time + 1.0));

		if(request_SOCKET_OK == NMXP_SOCKET_OK) {

		    nmxp_data_to_str(str_start_time, params.start_time);
		    nmxp_data_to_str(str_end_time, params.end_time);
		    nmxptool_str_time_to_filename(str_start_time);
		    nmxptool_str_time_to_filename(str_end_time);
/*Possible bug params not inited*/
		    if(params.flag_writefile) {
			/* Open output file */
			if(nmxp_chan_cpy_sta_chan(channelList_subset->channel[request_chan].name, station_code, channel_code, network_code, location_code)) {
			    sprintf(filename, "%s.%s.%s_%s_%s.nmx",
				    NETCODE_OR_CURRENT_NETWORK,
				    station_code,
				    channel_code,
				    str_start_time,
				    str_end_time);
			} else {
			    sprintf(filename, "%s_%s_%s.nmx",
				    channelList_subset->channel[request_chan].name,
				    str_start_time,
				    str_end_time);
			}

			outfile = fopen(filename, "w");
			if(!outfile) {
			    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_EXTRA, "Can not open file %s!\n",
				    NMXP_LOG_STR(filename));
			}
		    }

		    if(params.flag_writefile  &&  outfile) {
			/* Compute SNCL line */

			/* Separate station_code_old_way and channel_code_old_way */
			if(nmxp_chan_cpy_sta_chan(channelList_subset->channel[request_chan].name, station_code, channel_code, network_code, location_code)) {
			    /* Write SNCL line */
			    fprintf(outfile, "%s.%s.%s.%s\n",
				    station_code,
				    NETCODE_OR_CURRENT_NETWORK,
				    channel_code,
				    (params.location)? params.location : "");
			}

		    }

		    /* DAP Step 6: Receive Data until receiving a Ready message */
		    ret = nmxp_receiveMessage(naqssock, &type, buffer, &length, 0, &recv_errno, NMXP_MAX_LENGTH_DATA_BUFFER);

		    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_EXTRA, "ret = %d, type = %d, length = %d, recv_errno = %d\n",
			    ret, type, length, recv_errno);

		    while(ret == NMXP_SOCKET_OK   &&    type != NMXP_MSG_READY  && !nmxptool_sigcondition_read()
#ifdef HAVE_LIBMSEED
			    &&  data_seed.err_general==0
#endif
			 ) {
			/* Process a packet and return value in NMXP_DATA_PROCESS structure */ /*STEFANO*/
                        
			if (pd != NULL) {
			    if (pd->pDataPtr != NULL) {
				NMXP_MEM_FREE(pd->pDataPtr);
			    }
			    NMXP_MEM_FREE(pd);
			}

			pd = nmxp_processCompressedData(buffer, length, channelList_subset, NETCODE_OR_CURRENT_NETWORK, LOCCODE_OR_CURRENT_LOCATION);

			/* Force value for timing_quality if declared in the command-line */
			if(pd && params.timing_quality != -1) {
			    pd->timing_quality = params.timing_quality;
			}

                        /* set the data quality indicator */			
			if (pd) {
                          pd->quality_indicator = params.quality_indicator;
                        }
			                                                                          

			nmxp_data_trim(pd, params.start_time, params.end_time, 0);

			/* To prevent to manage a packet with zero sample after nmxp_data_trim() */
			if(pd->nSamp > 0) {

			/* Log contents of last packet */
			if(params.flag_logdata) {
			    nmxp_data_log(pd, params.flag_logsample);
			}

			/* Set cur_chan */
			cur_chan = nmxp_chan_lookupKeyIndex(pd->key, channelList_subset);

			/* It is not the channel I have requested or error from nmxp_chan_lookupKeyIndex() */
			if(request_chan != cur_chan  &&  cur_chan != -1) {
			    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY, "request_chan != cur_chan  %d != %d! (%d, %s) (%d, %s.%s.%s)\n",
				    request_chan, cur_chan,
				    channelList_subset->channel[request_chan].key,
				    NMXP_LOG_STR(channelList_subset->channel[request_chan].name),
				    pd->key, NMXP_LOG_STR(pd->network), NMXP_LOG_STR(pd->station), NMXP_LOG_STR(pd->channel));
			} else {

			/* Management of gaps */
			nmxptool_chanseq_gap(&(channelList_Seq[cur_chan]), pd);

#ifdef HAVE_LIBMSEED
			/* Write Mini-SEED record */
			if(params.type_writeseed) {
			    nmxptool_write_miniseed(pd);
			}
#endif

#ifdef HAVE_SEEDLINK
			/* Send data to SeedLink Server */
			if(params.flag_slink) {
			    nmxptool_send_raw_depoch(pd);
			}
#endif

#ifdef HAVE_LIBMSEED
#ifdef HAVE_SEEDLINK
			/* Send data to SeedLink Server */
			if(params.flag_slinkms) {
			    nmxptool_msr_send_mseed(pd);
			}
#endif
#endif

#ifdef HAVE_EARTHWORMOBJS
			if(params.ew_configuration_file) {
			    nmxptool_ew_nmx2ew(pd);
			}
#endif

			if(params.flag_writefile  &&  outfile) {
			    /* Write buffer to the output file */
			    if(outfile && length > 0) {
				int32_t length_int = length;
				nmxp_data_swap_4b((int32_t *) &length_int);
				fwrite(&length_int, sizeof(length_int), 1, outfile);
				fwrite(buffer, length, 1, outfile);
			    }
			}

			/* Store x_1 */
			channelList_Seq[cur_chan].x_1 = pd->pDataPtr[pd->nSamp-1];

			}

			} else {
			    /* TODO: nSamp <= 0 */
			}


			/* Receive Data */
			ret = nmxp_receiveMessage(naqssock, &type, buffer, &length, 0, &recv_errno, NMXP_MAX_LENGTH_DATA_BUFFER);
			/* nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CONNFLOW, "ret = %d, type = %d\n", ret, type); */
		    }

		    if(params.flag_writefile  &&  outfile) {
			/* Close output file */
			fclose(outfile);
			outfile = NULL;
		    }

		} else {
		    /* TODO: error message */
		}
		request_chan++;

#ifdef HAVE_EARTHWORMOBJS
		if(params.ew_configuration_file) {

		    /* Check if we are being asked to terminate */
		    if( (ew_check_flag_terminate = nmxptool_ew_check_flag_terminate()) ) {
			logit ("t", "nmxptool terminating on request\n");
			nmxptool_ew_send_error(NMXPTOOL_EW_ERR_TERMREQ, NULL, params.hostname);
			exitdapcondition = 0;
			times_flow = TIMES_FLOW_EXIT;
		    }

		    /* Check if we need to send heartbeat message */
		    nmxptool_ew_send_heartbeat_if_needed();

		}
#endif

	    }
	    /* DAP Step 7: Repeat steps 5 and 6 for each data request */

	    if(params.delay > 0) {
		time_to_sleep = (params.end_time - params.start_time) - (time(NULL) - (params.start_time + params.delay + params.span_data));
		/* TODO if time_to_sleep exceds DAP protocol time-out, split sleep() and send alive packet */
		if(time_to_sleep >= 0) {
		    while(time_to_sleep>0 && !nmxptool_sigcondition_read()) {
			nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_EXTRA, "time to sleep %d sec.\n", time_to_sleep);
			if(time_to_sleep >= NMXP_DAP_TIMEOUT_KEEPALIVE) {
			    nmxp_sleep(NMXP_DAP_TIMEOUT_KEEPALIVE);
			    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_EXTRA, "nmxp_sendRequestPending\n");
			    nmxp_sendRequestPending(naqssock);
			    if(nmxp_waitReady(naqssock) != NMXP_SOCKET_OK) {
				nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "Error waiting Ready message!\n");
				return 1;
			    }
			} else {
			    nmxp_sleep(time_to_sleep);
			}
			time_to_sleep -= NMXP_DAP_TIMEOUT_KEEPALIVE;
		    }
		} else {
		    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "time to sleep %d sec.\n", time_to_sleep);
		    nmxp_sleep(3);
		}
		params.start_time = params.end_time;
		params.end_time = params.start_time + params.span_data;
	    } else {
		exitdapcondition = 0;
	    }

	} /* END while(exitdapcondition) */

#ifdef HAVE_LIBMSEED
	if(params.type_writeseed) {
	    if(*msr_list_chan) {
		for(i_chan = 0; i_chan < channelList_subset->number; i_chan++) {
		    if(msr_list_chan[i_chan]) {
			/* Flush remaining samples */
			nmxp_data_msr_pack(NULL, &data_seed, msr_list_chan[i_chan]);
		    }
		}
	    }
	    nmxp_data_seed_fclose_all(&data_seed);
	}
#endif

	/* DAP Step 8: Send a Terminate message (optional) */
	nmxp_sendTerminateSubscription(naqssock, NMXP_SHUTDOWN_NORMAL, "Bye!");

	/* DAP Step 9: Close the socket */
	nmxp_closeSocket(naqssock);
	naqssock = 0;

	/* ************************************************************ */
	/* End subscription protocol "DATA ACCESS PROTOCOL" version 1.0 */
	/* ************************************************************ */

	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CONNFLOW, "End DAP Flow.\n");

    } else {

	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CONNFLOW, "Begin PDS Flow.\n");

	/* ************************************************************* */
	/* Start subscription protocol "PRIVATE DATA STREAM" version 1.4 */
	/* ************************************************************* */

	/* PDS Step 1: Open a socket */
	naqssock = nmxp_openSocket(params.hostname, params.portnumberpds, nmxptool_exitcondition_on_open_socket);

	if(naqssock == NMXP_SOCKET_ERROR) {
	    return 1;
	}

	/* PDS Step 2: Send a Connect */
	if(nmxp_sendConnect(naqssock) != NMXP_SOCKET_OK) {
	    printf("Error on sendConnect()\n");
	    return 1;
	}

	/* PDS Step 3: Receive ChannelList */
	if(nmxp_receiveChannelList(naqssock, &channelList) != NMXP_SOCKET_OK) {
	    printf("Error on receiveChannelList()\n");
	    return 1;
	}
	/* Get a subset of channel from arguments, in respect to the step 3 of PDS */
	channelList_subset_waste = nmxp_chan_subset(channelList, NMXP_DATA_TIMESERIES, params.channels, CURRENT_NETWORK, CURRENT_LOCATION);

	/* Free the complete channel list */
	if(channelList) {
	    NMXP_MEM_FREE(channelList);
	    channelList = NULL;
	}

	/* TODO check if channelList_subset_waste is equal to channelList_subset and free */
	if(channelList_subset_waste) {
	    NMXP_MEM_FREE(channelList_subset_waste);
	    channelList_subset_waste = NULL;
	}

	/* PDS Step 4: Send a Request Pending (optional) */

	/* PDS Step 5: Send AddChannels */
	/* Request Data */

	/* Better using a Thread */
#ifndef HAVE_PTHREAD_H
	nmxp_sendAddTimeSeriesChannel(naqssock, channelList_subset, params.stc, params.rate,
		(params.flag_buffered)? NMXP_BUFFER_YES : NMXP_BUFFER_NO, params.n_channel, params.usec, 1);
#else
	pthread_attr_init(&attr_request_channels);
	pthread_attr_setdetachstate(&attr_request_channels, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_request_channels, &attr_request_channels, p_nmxp_sendAddTimeSeriesChannel, (void *)NULL);
	pthread_attr_destroy(&attr_request_channels);
#endif

#ifndef HAVE_WINDOWS_H
#ifdef HAVE_PTHREAD_H
	if(!already_listen  &&  params.listen_port != DEFAULT_LISTEN_PORT) {
	    already_listen = 1;
	    pthread_attr_init(&attr_socket_listen);
	    pthread_attr_setdetachstate(&attr_socket_listen, PTHREAD_CREATE_DETACHED);
	    pthread_create(&thread_socket_listen, &attr_socket_listen, nmxptool_listen, (void *) &params.listen_port);
	    pthread_attr_destroy(&attr_socket_listen);
	}
#endif
#endif

	/* PDS Step 6: Repeat until finished: receive and handle packets */

	/* TODO*/
	exitpdscondition = 1;
	flag_force_close_connection = 0;

	skip_current_packet = 0;

	time(&lasttime_pds_receiveddata);

	/* begin  main PDS loop */

	while(exitpdscondition && !nmxptool_sigcondition_read() && !flag_force_close_connection
#ifdef HAVE_LIBMSEED
		&&  data_seed.err_general==0
#endif
	     ) {
	    
	    /* added 2010-07-26, RR */
            if (pd != NULL) {
              if (pd->pDataPtr != NULL) {
                NMXP_MEM_FREE(pd->pDataPtr);
              }
              NMXP_MEM_FREE(pd);
            } 
	    /* Process Compressed or Decompressed Data */
	    pd = nmxp_receiveData(naqssock, channelList_subset, NETCODE_OR_CURRENT_NETWORK, LOCCODE_OR_CURRENT_LOCATION, params.timeoutrecv, &recv_errno);

	    nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_EXTRA, "Received %s packet.\n", (pd)? "not null" : "null");

	    /* Get time when receive some data */
	    if(pd) {
		time(&lasttime_pds_receiveddata);
	    }

	    if ( (time(NULL) - lasttime_pds_receiveddata) >= timeout_pds_receiveddata ) {
		nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_ANY, "No data received within %d seconds. Force to close connection and open again.\n", timeout_pds_receiveddata);
		flag_force_close_connection = 1;
		time(&lasttime_pds_receiveddata);
	    }

	    /* Force value for timing_quality if declared in the command-line */
	    if(pd && params.timing_quality != -1) {
		pd->timing_quality = params.timing_quality;
	    }
            
            /* set the data quality indicator */
            if (pd) {
              pd->quality_indicator = params.quality_indicator;
            }
                                                                                                            
	    if(!pd) {
		pd_null_count++;
		if((pd_null_count * params.timeoutrecv) >= timeoutrecv_warning) {
		    nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_ANY, "Received %d times a null packet. (%d sec.)\n",
			    pd_null_count, pd_null_count * params.timeoutrecv);
		    pd_null_count = 0;
		}
	    } else {
		pd_null_count = 0;
	    }

	    if(recv_errno == 0) {
		exitpdscondition = 1;
	    } else {
#ifdef HAVE_WINDOWS_H
		if(recv_errno == WSAEWOULDBLOCK  ||  recv_errno == WSAETIMEDOUT)
#else
		if(recv_errno == EWOULDBLOCK)
#endif
		{
		    exitpdscondition = 1;
		} else {
		    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "Error receiving data. pd=%p recv_errno=%d\n",
			    pd, recv_errno);

#ifdef HAVE_EARTHWORMOBJS
		    if(params.ew_configuration_file) {
                        recv_errno_str=nmxp_strerror(recv_errno);
			nmxptool_ew_send_error(NMXPTOOL_EW_ERR_RECVDATA, recv_errno_str, params.hostname);
			NMXP_MEM_FREE(recv_errno_str);
		    }
#endif
		    exitpdscondition = 0;
		}
	    }

	    if(pd) {
		/* Set cur_chan */
		cur_chan = nmxp_chan_lookupKeyIndex(pd->key, channelList_subset);
		if(cur_chan == -1) {
		    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY, "Key %d not found in channelList_subset!\n",
			    pd->key);
		}
	    }

	    /* Log contents of last packet */
	    if(params.flag_logdata) {
		nmxp_data_log(pd, params.flag_logsample);
	    }

	    skip_current_packet = 0;
	    if(pd &&
		    (params.statefile  ||  params.buffered_time) &&
		    ( params.timeoutrecv <= 0 )
	      )	{
		if(params.statefile && channelList_Seq[cur_chan].after_start_time > 0.0) {
		    cur_after_start_time = channelList_Seq[cur_chan].after_start_time;
		} else if(params.buffered_time) {
		    cur_after_start_time = params.buffered_time;
		} else {
		    cur_after_start_time = DEFAULT_BUFFERED_TIME;
		}
		nmxp_data_to_str(cur_after_start_time_str, cur_after_start_time);
		nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_PACKETMAN, "cur_chan %d, cur_after_start_time %f, cur_after_start_time_str %s\n",
			cur_chan, cur_after_start_time, NMXP_LOG_STR(cur_after_start_time_str));
		if(pd->time + ((double) pd->nSamp / (double) pd->sampRate) >= cur_after_start_time) {
		    if(pd->time < cur_after_start_time) {
			int first_nsample_to_remove = (cur_after_start_time - pd->time) * (double) pd->sampRate;
			/* Remove the first sample in order avoiding overlap  */
			first_nsample_to_remove++;
			if(pd->nSamp > first_nsample_to_remove) {
			    pd->nSamp -= first_nsample_to_remove;
			    pd->time = cur_after_start_time;
			    /*Here you are!!!! sposta il puntatore, la free come fa?*/
			    //pd->pDataPtr += first_nsample_to_remove;
			    //pd->x0 = pd->pDataPtr[0];
			    pd->x0 = pd->pDataPtr[first_nsample_to_remove];
			} else {
			    skip_current_packet = 1;
			}
		    }
		} else {
		    skip_current_packet = 1;
		}
	    }

	    if(!skip_current_packet) {

		/* Manage Raw Stream */
		if(params.stc == -1) {

		    /* cur_char is computed only for pd != NULL */
		    if(pd) {
			nmxp_raw_stream_manage(&(channelList_Seq[cur_chan].raw_stream_buffer), pd, p_func_pd, n_func_pd);
			channelList_Seq[cur_chan].last_time_call_raw_stream = nmxp_data_gmtime_now();
		    }

		    /* Check timeout for other channels */
		    if(params.timeoutrecv > 0) {
			to_cur_chan = 0;
			while(to_cur_chan < channelList_subset->number) {
			    timeout_for_channel = nmxp_data_gmtime_now() - channelList_Seq[to_cur_chan].last_time_call_raw_stream;
			    if(channelList_Seq[to_cur_chan].last_time_call_raw_stream != 0
				    && timeout_for_channel >= params.timeoutrecv) {
				nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_DOD, "Timeout for channel %s (%d sec.)\n",
					NMXP_LOG_STR(channelList_subset->channel[to_cur_chan].name), timeout_for_channel);
				nmxp_raw_stream_manage(&(channelList_Seq[to_cur_chan].raw_stream_buffer), NULL, p_func_pd, n_func_pd);
				channelList_Seq[to_cur_chan].last_time_call_raw_stream = nmxp_data_gmtime_now();
			    }
			    to_cur_chan++;
			}
		    }

		} else {

		    if(pd) {
			/* Management of gaps */
			nmxptool_chanseq_gap(&(channelList_Seq[cur_chan]), pd);

#ifdef HAVE_LIBMSEED
			/* Write Mini-SEED record */
			if(params.type_writeseed) {
			    nmxptool_write_miniseed(pd);
			}
#endif

#ifdef HAVE_SEEDLINK
			/* Send data to SeedLink Server */
			if(params.flag_slink) {
			    nmxptool_send_raw_depoch(pd);
			}
#endif

#ifdef HAVE_LIBMSEED
#ifdef HAVE_SEEDLINK
			/* Send data to SeedLink Server */
			if(params.flag_slinkms) {
			    nmxptool_msr_send_mseed(pd);
			}
#endif
#endif

#ifdef HAVE_EARTHWORMOBJS
			if(params.ew_configuration_file) {
			    nmxptool_ew_nmx2ew(pd);
			}
#endif

		    }
		}
	    } /* End skip_current_packet condition */

	    if(pd) {
		/* Store x_1 */
		if(pd->nSamp > 0) {
		    channelList_Seq[cur_chan].x_1 = pd->pDataPtr[pd->nSamp-1];
		}
	    }

#ifdef HAVE_EARTHWORMOBJS
	    if(params.ew_configuration_file) {

		/* Check if we are being asked to terminate */
		if( (ew_check_flag_terminate = nmxptool_ew_check_flag_terminate()) ) {
		    logit ("t", "nmxptool terminating on request\n");
		    nmxptool_ew_send_error(NMXPTOOL_EW_ERR_TERMREQ, NULL, params.hostname);
		    exitpdscondition = 0;
		    times_flow = TIMES_FLOW_EXIT;
		}

		/* Check if we need to send heartbeat message */
		nmxptool_ew_send_heartbeat_if_needed();

	    }
#endif

	    /* Better using a Thread */
#ifndef HAVE_PTHREAD_H
	    nmxp_sendAddTimeSeriesChannel(naqssock, channelList_subset, params.stc, params.rate,
		    (params.flag_buffered)? NMXP_BUFFER_YES : NMXP_BUFFER_NO, params.n_channel, params.usec, 0);
#endif

	    if (pd != NULL) {
		if (pd->pDataPtr != NULL) {
		    NMXP_MEM_FREE(pd->pDataPtr);
		}
		NMXP_MEM_FREE(pd);
	    }
            
	} /* End main PDS loop */

#ifdef HAVE_PTHREAD_H
	// pthread_join(thread_request_channels, &status_thread);
#endif

#ifdef HAVE_PTHREAD_H
	// Waiting for p_nmxp_sendAddTimeSeriesChannel()
	pthread_mutex_lock (&mutex_sendAddTimeSeriesChannel);
	pthread_mutex_unlock (&mutex_sendAddTimeSeriesChannel);
#endif

	/* Flush raw data stream for each channel */
	flushing_raw_data_stream();

#ifdef HAVE_LIBMSEED
	if(params.type_writeseed) {
	    if(*msr_list_chan) {
		for(i_chan = 0; i_chan < channelList_subset->number; i_chan++) {
		    if(msr_list_chan[i_chan]) {
			/* Flush remaining samples */
			nmxp_data_msr_pack(NULL, &data_seed, msr_list_chan[i_chan]);
		    }
		}
	    }
	    nmxp_data_seed_fclose_all(&data_seed);
	}
#endif

	/* PDS Step 7: Send Terminate Subscription */
	nmxp_sendTerminateSubscription(naqssock, NMXP_SHUTDOWN_NORMAL, "Good Bye!");

	/* PDS Step 8: Close the socket */
	nmxp_closeSocket(naqssock);
	naqssock = 0;

	/* *********************************************************** */
	/* End subscription protocol "PRIVATE DATA STREAM" version 1.4 */
	/* *********************************************************** */

	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CONNFLOW, "End PDS Flow.\n");

    }

    if(times_flow != TIMES_FLOW_EXIT
	    &&  params.interval == DEFAULT_INTERVAL_INFINITE) {
	times_flow++;
    } else {
	times_flow = TIMES_FLOW_EXIT;
    }

    if(params.statefile) {
	nmxptool_chanseq_save_states(channelList_subset, channelList_Seq, params.statefile);
    }

    } /* End times_flow loop */

    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CONNFLOW, "End communication.\n");

#ifdef HAVE_LIBMSEED
	if(params.type_writeseed  ||  params.flag_slinkms) {
	    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_EXTRA, "Free mini-SEED record list.\n");
	    if(*msr_list_chan) {
		for(i_chan = 0; i_chan < channelList_subset->number; i_chan++) {
		    if(msr_list_chan[i_chan]) {
			msr_free(&(msr_list_chan[i_chan])); 
		    }
		}
	    }
	}
#endif

    if(channelList_Seq  &&  channelList_subset) {
	nmxptool_chanseq_free(&channelList_Seq, channelList_subset->number);
    }

    /* This has to be the last */
    if(channelList_subset) {
	NMXP_MEM_FREE(channelList_subset);
	channelList_subset = NULL;
    }

    /* Same condition of while 'Exit only on request' */
    if(EXIT_CONDITION) {
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CONNFLOW, "Sleep %d seconds before re-connect.\n", params.networkdelay);
	nmxp_sleep(params.networkdelay);
    }

    } /* End 'exit only on request' loop */

#ifdef HAVE_EARTHWORMOBJS
    if(params.ew_configuration_file) {
	nmxptool_ew_detach();
    }
#endif

    if(params.channels) {
	NMXP_MEM_FREE(params.channels);
	params.channels = NULL;
    }

    NMXP_MEM_PRINT_PTR(1, 1);

    main_ret = nmxptool_sigcondition_read();
    nmxptool_sigocondition_destroy();

    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CONNFLOW, "return code %d\n", main_ret);

#ifdef HAVE_PTHREAD_H
    pthread_mutex_destroy(&mutex_sendAddTimeSeriesChannel);
#endif

    return main_ret;
} /* End MAIN */


int nmxptool_exitcondition_on_open_socket() {
    int ret = nmxptool_sigcondition_read();
#ifdef HAVE_EARTHWORMOBJS
    if(!ret) {
	if(params.ew_configuration_file) {

	    /* Check if we are being asked to terminate */
	    if( (ret  = nmxptool_ew_check_flag_terminate()) ) {

		logit ("t", "nmxptool terminating on request\n");
		nmxptool_ew_send_error(NMXPTOOL_EW_ERR_TERMREQ, NULL, params.hostname);

		nmxptool_sigcondition_write(15);
	    }

	    /* Check if we need to send heartbeat message */
	    nmxptool_ew_send_heartbeat_if_needed();

	}
    }
#endif
    return ret;
}



void flushing_raw_data_stream() {
    int to_cur_chan;

    if(channelList_subset == NULL  || channelList_Seq == NULL) {
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY, " flushing_raw_data_stream() channel lists are NULL.\n");
	return ;
    }

    /* Flush raw data stream for each channel */
    if(params.stc == -1) {
	to_cur_chan = 0;
	while(to_cur_chan < channelList_subset->number) {

#ifdef HAVE_EARTHWORMOBJS
	    if(params.ew_configuration_file) {

		/* Check if we are being asked to terminate */
		if( (ew_check_flag_terminate = nmxptool_ew_check_flag_terminate()) ) {
		    logit ("t", "nmxptool terminating on request\n");
		    nmxptool_ew_send_error(NMXPTOOL_EW_ERR_TERMREQ, NULL, params.hostname);
		}

		/* Check if we need to send heartbeat message */
		nmxptool_ew_send_heartbeat_if_needed();

	    }
#endif

	    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_RAWSTREAM, "Flushing data for channel %s\n",
		    NMXP_LOG_STR(channelList_subset->channel[to_cur_chan].name));
	    nmxp_raw_stream_manage(&(channelList_Seq[to_cur_chan].raw_stream_buffer), NULL, p_func_pd, n_func_pd);
	    to_cur_chan++;
	}
    }
}


void *nmxptool_print_params(void *arg) {
    /* nmxptool_log_params(&params); */
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
    char *hostname: %s\n\
    int portnumberdap: %d\n\
    int portnumberpds: %d\n\
",
    NMXP_LOG_STR(params.hostname),
    params.portnumberdap,
    params.portnumberpds
);

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
    char *channels: %s\n\
",
    NMXP_LOG_STR(params.channels)
);

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
    char *network: %s\n\
    char *location: %s\n\
    double start_time: %f\n\
    double end_time: %f\n\
    int32_t interval: %d\n\
",
    NMXP_LOG_STR(params.network),
    NMXP_LOG_STR(params.location),
    params.start_time,
    params.end_time,
    params.interval
);


    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
    char *datas_username: %s\n\
    char *datas_password: %s\n\
",
    NMXP_LOG_STR(params.datas_username),
    NMXP_LOG_STR(params.datas_password)
);

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
    int32_t stc: %d\n\
    int32_t rate: %d\n\
    char *plugin_slink: %s\n\
    int32_t delay: %d\n\
    int32_t max_tolerable_latency: %d\n\
    int32_t timeoutrecv: %d\n\
    int32_t verbose_level: %d\n\
",
    params.stc,
    params.rate,
    NMXP_LOG_STR(params.plugin_slink),
    params.delay,
    params.max_tolerable_latency,
    params.timeoutrecv,
    params.verbose_level
);

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
    char *ew_configuration_file: %s\n\
    char *statefile: %s\n\
    int32_t max_data_to_retrieve: %d\n\
",
    NMXP_LOG_STR(params.ew_configuration_file),
    NMXP_LOG_STR(params.statefile),
    params.max_data_to_retrieve
);

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
    double buffered_time: %f\n\
    char type_writeseed: %d\n\
    int timing_quality: %d\n\
    int flag_listchannels: %d\n\
    int flag_listchannelsnaqs: %d\n\
    int flag_request_channelinfo: %d\n\
    int flag_writefile: %d\n\
    int flag_slink: %d\n\
    int flag_slinkms: %d\n\
    int flag_buffered: %d\n\
    int flag_logdata: %d\n\
    int flag_logsample: %d\n\
",
    params.buffered_time,
    params.type_writeseed,
    params.timing_quality,
    params.flag_listchannels,
    params.flag_listchannelsnaqs,
    params.flag_request_channelinfo,
    params.flag_writefile,
    params.flag_slink,
    params.flag_slinkms,
    params.flag_buffered,
    params.flag_logdata,
    params.flag_logsample
    );
    return NULL;
}

void *nmxptool_print_info_raw_stream(void *arg) {
    int chan_index;
    char last_time_str[30];
    char last_time_call_raw_stream_str[30];
    char after_start_time_str[30];
    char raw_stream_buffer_last_sample_time_str[30];

    if(channelList_subset) {

	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
Channel      Ind S      SeqNo        x-1  nIt    lat     LastSampleTime            LastTime            LastTimeCallRaw        AfterStartTime     MaxIt   MTL   TO\
\n");

	chan_index = 0;
	while(channelList_subset && chan_index < channelList_subset->number) {

	    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "%-12s ", channelList_subset->channel[chan_index].name);

	    nmxp_data_to_str(last_time_str, channelList_Seq[chan_index].last_time);
	    nmxp_data_to_str(last_time_call_raw_stream_str, channelList_Seq[chan_index].last_time_call_raw_stream);
	    nmxp_data_to_str(after_start_time_str, channelList_Seq[chan_index].after_start_time);
	    nmxp_data_to_str(raw_stream_buffer_last_sample_time_str, channelList_Seq[chan_index].raw_stream_buffer.last_sample_time);

	    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY,
		    "%3d ",
		    chan_index);

	    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY,
		    "%d ",
		    channelList_Seq[chan_index].significant);

	    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY,
		    "%12d ",
		    channelList_Seq[chan_index].raw_stream_buffer.last_seq_no_sent);

	    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY,
		    "%8d ",
		    channelList_Seq[chan_index].x_1
		    );

	    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY,
		    "%4d ",
		    channelList_Seq[chan_index].raw_stream_buffer.n_pdlist
		    );

	    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY,
		    "%6.1f ",
		    channelList_Seq[chan_index].raw_stream_buffer.last_latency
		    );

	    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "%s ", NMXP_LOG_STR(raw_stream_buffer_last_sample_time_str));

	    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "%s ", NMXP_LOG_STR(last_time_str));
	    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "%s ", NMXP_LOG_STR(last_time_call_raw_stream_str));
	    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "%s ", NMXP_LOG_STR(after_start_time_str));

	    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY,
		    "%5d ",
		    channelList_Seq[chan_index].raw_stream_buffer.max_pdlist_items);

	    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY,
		    "%5.2f ",
		    channelList_Seq[chan_index].raw_stream_buffer.max_tolerable_latency);

	    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY,
		    "%3d ",
		    channelList_Seq[chan_index].raw_stream_buffer.timeoutrecv);

	    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\n");

	    chan_index++;
	}
    } else {
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY, "Channel list is NULL!\n");
    }

    return NULL;
}


/* Set sigcondition to received signal value  */
static void ShutdownHandler(int sig) {

    NMXP_MEM_PRINT_PTR(0, 1);

    /* Safe Thread Synchronization */
    nmxptool_sigcondition_write(sig);

    /* If nmxptool is not receiving data then unblock recv() */
    if(naqssock > 0) {
	nmxp_setsockopt_RCVTIMEO(naqssock, 1);
    }

    nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_ANY, "%s interrupted by signal %d!\n", NMXP_LOG_STR(PACKAGE_NAME), sig);

} /* End of ShutdownHandler() */


/* Signal handler routine, print info about Raw Stream data buffer */
static void nmxptool_AlarmHandler(int sig) {
    /* TODO Safe Thread Synchronization */

    nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_ANY, "%s received signal %d!\n", NMXP_LOG_STR(PACKAGE_NAME), sig);

    NMXP_MEM_PRINT_PTR(0, 1);

    nmxptool_print_info_raw_stream(NULL);
}

/* Force closing connection  */
static void CloseConnectionHandler(int sig) {
    /* TODO Safe Thread Synchronization */

    NMXP_MEM_PRINT_PTR(0, 1);

    flag_force_close_connection = 1;

    /* If nmxptool is not receiving data then unblock recv() */
    if(naqssock > 0) {
	nmxp_setsockopt_RCVTIMEO(naqssock, 1);
    }

    nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_ANY, "%s received signal %d! Force to close connection and open again.\n", NMXP_LOG_STR(PACKAGE_NAME), sig);

} /* End of CloseConnectionHandler() */




#ifdef HAVE_LIBMSEED
int nmxptool_write_miniseed(NMXP_DATA_PROCESS *pd) {
    int cur_chan;

    int ret = 0;
    if( (cur_chan = nmxp_chan_lookupKeyIndex(pd->key, channelList_subset)) != -1) {

	ret = nmxp_data_msr_pack(pd, &data_seed, msr_list_chan[cur_chan]);

    } else {
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY, "Key %d not found in channelList_subset!\n", pd->key);
    }
    return ret;
}
#endif

#ifdef HAVE_LIBMSEED
#ifdef HAVE_SEEDLINK

void nmxptool_msr_send_mseed_handler (char *record, int reclen, void *handlerdata) {
    int ret = 0;
    NMXP_DATA_PROCESS *pd = handlerdata;
    ret = send_mseed(pd->station, record, reclen);
    if ( ret <= 0 ) {
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_PACKETMAN,
		"send_mseed() for %s.%s.%s\n", pd->network, pd->station, pd->channel);
    }
}

int nmxptool_msr_send_mseed(NMXP_DATA_PROCESS *pd) {
    int ret = 0;
    int cur_chan;

    MSRecord *msr = NULL;
    int64_t psamples;
    int precords;
    flag verbose = 0;

    if( (cur_chan = nmxp_chan_lookupKeyIndex(pd->key, channelList_subset)) != -1) {

	msr = msr_list_chan[cur_chan];

	if(pd) {
	    if(pd->nSamp > 0) {

		/* Populate MSRecord values */

		msr->starttime = MS_EPOCH2HPTIME(pd->time);
		msr->samprate = pd->sampRate;

		/* SEED utilizes the Big Endian word order as its standard.
		 * In 2003, the FDSN adopted the format rule that Steim1 and
		 * Steim2 data records are to be written with the big-endian
		 * encoding only. */
		msr->byteorder = 1;         /* big endian byte order */

		msr->sequence_number = pd->seq_no % 1000000;

		msr->sampletype = 'i';      /* declare type to be 32-bit integers */

		msr->numsamples = pd->nSamp;
		msr->datasamples = NMXP_MEM_MALLOC (sizeof(int) * (msr->numsamples)); 
                msr->dataquality = pd->quality_indicator;
		        
		memcpy(msr->datasamples, pd->pDataPtr, sizeof(int) * pd->nSamp); /* pointer to 32-bit integer data samples */

		/* msr_print(msr, 2); */

		precords = msr_pack (msr, &nmxptool_msr_send_mseed_handler, pd, &psamples, 1, verbose);
                NMXP_MEM_FREE(msr->datasamples);
		if ( precords == -1 ) {
		    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_PACKETMAN,
			    "Cannot pack records %s.%s.%s\n", pd->network, pd->station, pd->channel);
		} else {
		    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN,
			    "Packed %d samples into %d records for %s.%s.%s\n",
			    psamples, precords, pd->network, pd->station, pd->channel);
		}
	    }
	}
    } else {
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY, "Key %d not found in channelList_subset!\n", pd->key);
    }

    return ret;
}
#endif
#endif

int nmxptool_print_seq_no(NMXP_DATA_PROCESS *pd) {
    int ret = 0;
    char str_time[200];
    nmxp_data_to_str(str_time, pd->time);

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "Process %s.%s.%s.%s %2d %d %d %s %dpts lat. %.1fs\n",
	    NMXP_LOG_STR(pd->network),
	    NMXP_LOG_STR(pd->station),
	    NMXP_LOG_STR(pd->channel),
	    NMXP_LOG_STR(pd->location),
	    pd->packet_type,
	    pd->seq_no,
	    pd->oldest_seq_no,
	    NMXP_LOG_STR(str_time),
	    pd->nSamp,
	    nmxp_data_latency(pd)
	    );

    return ret;
}


#ifdef HAVE_SEEDLINK
int nmxptool_send_raw_depoch(NMXP_DATA_PROCESS *pd) {
    /* TODO Set values */
    const int usec_correction = 0;

    return send_raw_depoch(pd->station, pd->channel, pd->time, usec_correction, pd->timing_quality,
	    pd->pDataPtr, pd->nSamp);
}
#endif



void nmxptool_str_time_to_filename(char *str_time) {
    int i;
    for(i=0; i<strlen(str_time); i++) {
	if(   (str_time[i] >= 'A'  &&  str_time[i] <= 'Z')
		|| (str_time[i] >= 'a'  &&  str_time[i] <= 'z')
		|| (str_time[i] >= '0'  &&  str_time[i] <= '9')
		|| (str_time[i] == '_')
	  ) {
	    /* Do nothing */
	} else {
	    str_time[i] = '.';
	}
    }
}

#ifdef HAVE_LIBMSEED
int nmxptool_log_miniseed(const char *s) {
    return nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "%s", s);
}

int nmxptool_logerr_miniseed(const char *s) {
    return nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY, "%s", s);
}
#endif


#ifdef HAVE_PTHREAD_H
void *p_nmxp_sendAddTimeSeriesChannel(void *arg) {
    int i = 0;
    int times_channel = 0;
    double estimated_time = 0.0;

    pthread_mutex_lock (&mutex_sendAddTimeSeriesChannel);

    if(params.n_channel == 0) {
	times_channel = 1;
    } else {
	times_channel = (channelList_subset->number / params.n_channel);
	times_channel += (((channelList_subset->number % params.n_channel) == 0)? 0 : 1);
    }

    /* Check if requests could be satisfied within NMXP_MAX_MSCHAN_MSEC */
    estimated_time = (double) channelList_subset->number * ( ((double) params.usec / 1000000.0) / (double) params.n_channel);
    if(estimated_time > ((double) NMXP_MAX_MSCHAN_MSEC / 1000.0)) {
	params.usec = ( (double) NMXP_MAX_MSCHAN_MSEC * 1000.0 ) * ( (double) params.n_channel / (double) channelList_subset->number);
	estimated_time = (double) channelList_subset->number * ( ((double) params.usec / 1000000.0) / (double) params.n_channel);
	nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_ANY, "New value for mschan is %d/%d!\n", params.usec / 1000, params.n_channel);
    }

    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CONNFLOW, "Begin requests of channels!\n");
    while(times_channel > 0  &&  !nmxptool_sigcondition_read()) {
	nmxp_sendAddTimeSeriesChannel(naqssock, channelList_subset, params.stc, params.rate,
		(params.flag_buffered)? NMXP_BUFFER_YES : NMXP_BUFFER_NO, params.n_channel, params.usec, (i==0)? 1 : 0);
	times_channel--;
	i++;
	if(times_channel > 0) {
	    nmxp_usleep(params.usec);
	}
    }
    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CONNFLOW, "End requests of channels!\n");

    pthread_mutex_unlock (&mutex_sendAddTimeSeriesChannel);

    pthread_exit(NULL);
}
#endif


