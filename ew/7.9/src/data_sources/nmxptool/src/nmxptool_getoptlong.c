/*! \file
 *
 * \brief Nanometrics Protocol Tool
 *
 * Author:
 * 	Matteo Quintiliani
 * 	Istituto Nazionale di Geofisica e Vulcanologia - Italy
 *	quintiliani@ingv.it
 *
 * $Id: nmxptool_getoptlong.c 6803 2016-09-09 06:06:39Z et $
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <errno.h>
#include <unistd.h>

#include "config.h"
#include "nmxp.h"

#include "nmxptool_getoptlong.h"

const NMXPTOOL_PARAMS NMXPTOOL_PARAMS_DEFAULT =
{
    NULL,
    DEFAULT_PORT_DAP,
    DEFAULT_PORT_PDS,
    NULL,
    NULL,
    NULL,
    0.0,
    0.0,
    DEFAULT_INTERVAL_NO_VALUE,
    NULL,
    NULL,
    DEFAULT_STC,
    DEFAULT_RATE,
    NULL,
    DEFAULT_DELAY,
    DEFAULT_SPANINTERVAL,
    DEFAULT_MAX_TOLERABLE_LATENCY,
    DEFAULT_TIMEOUTRECV,
    DEFAULT_VERBOSE_LEVEL,
    NULL,
    NULL,
    NULL,
    DEFAULT_TYPE_WRITESEED,
    DEFAULT_BUFFERED_TIME,
    DEFAULT_N_CHANNEL,
    DEFAULT_USEC,
    DEFAULT_MAX_TIME_TO_RETRIEVE,
    DEFAULT_NETWORKDELAY,
    DEFAULT_LISTEN_PORT,
    DEFAULT_TIMING_QUALITY,
    DEFAULT_QUALITY_INDICATOR,
    DEFAULT_ENCODING,
    DEFAULT_RECLEN_MINISEED,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
};


void nmxptool_author_support() {
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
Matteo Quintiliani - Istituto Nazionale di Geofisica e Vulcanologia - Italy\n\
Mail bug reports and suggestions to <%s>.\n",
	    NMXP_LOG_STR(PACKAGE_BUGREPORT)
	    );
}


#define PDS_VERSION "1.4"
#define DAP_VERSION "1.0"

void nmxptool_version() {
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
%s %s, tool for Nanometrics Protocols\n\
         Private Data Stream %s, Data Access Protocol %s\n",
	NMXP_LOG_STR(PACKAGE_NAME), NMXP_LOG_STR(PACKAGE_VERSION),
	NMXP_LOG_STR(PDS_VERSION), NMXP_LOG_STR(DAP_VERSION)
	/*
	nmxp_log_version()
	*/
	    );

    nmxptool_supports();
}


void nmxptool_supports() {
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
         Enabled features: libmseed ");
#ifdef HAVE_LIBMSEED
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "YES");
#else
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "NO");
#endif

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, ", SeedLink ");
#ifdef HAVE_SEEDLINK
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "YES");
#else
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "NO");
#endif

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, ", Earthworm ");
#ifdef HAVE_EARTHWORMOBJS
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "YES");
#else
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "NO");
#endif
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, ".\n");

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
         Using pthread: ");
#ifdef HAVE_PTHREAD_H
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "YES");
#else
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "NO");
#endif

#ifdef NMXP_MEM_DEBUG
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, ". Enabled memdebug");
#endif

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, ".\n");
}


void nmxptool_usage(struct option long_options[])
{
    nmxptool_version();

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
\n\
Usage: %s -H hostname  [  -C channellist ]  -l | -L \n\
             Print list of the available Time Series channels\n\
             on DataServer and NaqsServer respectively.\n\
\n",
NMXP_LOG_STR(PACKAGE_NAME));

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
       %s -H hostname -C channellist [...]\n\
             Receive data in near real-time from NaqsServer by PDS %s\n\
\n\
       %s -H hostname -F statefile [-A SECs] [...]\n\
             Receive data from NaqsServer and, in case, retrieve previous\n\
             data from DataServer up to SECs seconds before.\n\
\n",
NMXP_LOG_STR(PACKAGE_NAME),
NMXP_LOG_STR(PDS_VERSION),
NMXP_LOG_STR(PACKAGE_NAME));

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
       %s -H hostname -C channellist -s DATE -e DATE [...]\n\
       %s -H hostname -C channellist -s DATE -t TIME [...]\n\
             Receive a temporal interval of data from DataServer by DAP %s\n\
\n",
NMXP_LOG_STR(PACKAGE_NAME),
NMXP_LOG_STR(PACKAGE_NAME),
NMXP_LOG_STR(DAP_VERSION)
);

#ifdef HAVE_EARTHWORMOBJS
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
       %s nmxptool.d\n\
             Launched as Earthworm module to redirect data into the EW-Rings.\n\
             Refer to nmxptool_cmd.html into the Earthworm documentation.\n\
\n", NMXP_LOG_STR(PACKAGE_NAME));
#endif

#ifdef HAVE_SEEDLINK
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
       %s <option ... option> [ -k  |  -K ]\n\
             Launched as SeedLink plug-in to feed the SL-Server.\n\
\n", NMXP_LOG_STR(PACKAGE_NAME));
#endif

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
       %s --help | -h\n\
             Print this help.\n\
\n", NMXP_LOG_STR(PACKAGE_NAME));

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
Main arguments:\n\
  -H, --hostname=HOST     NaqsServer/DataServer hostname or IP address.\n\
  -C, --channels=LIST     List of NET.STA.CHAN.LOC separated by comma.\n\
                          NET  is optional and used only for output.\n\
                          STA  can be '*', it stands for all stations.\n\
                          CHAN can contain '?', it stands for any character.\n\
                          LOC  is optional and used only for output.\n\
                          Network and location code will be assigned from the\n\
                          first pattern that include station and channel.\n\
                          DO NOT USE with -F.\n\
                                Example: N1.AAA.HH?.01,N2.*.HH?,MMM.BH?.03\n\
                          Second pattern includes the first. Unless AAA, all\n\
                          stations with HH channels will have network to N2.\n\
                          Stations N2.*.HH? will have default location defined by -n.\n\
                          Station MMM will have default network defined by -N.\n\
  -F, --statefile=FILE    List of channel patterns, as in -C. One for each line.\n\
                          Load/Save time of the last sample of each channel\n\
                          into a file with the same name, same directory,\n\
                          appending the suffix '%s'.\n\
                          It enables the option -b in order to allow data continuity\n\
                          when short disconnections occur or between program restarts.\n\
                          Related to -L, -A and -f. DO NOT USE with -C.\n",
			  NMXP_STR_STATE_EXT
);

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
  -A, --maxdataretr=SECs  Max amount of data of the past to retrieve from the\n\
                          DataServer when program restarts (default %d) [%d..%d].\n\
                          0 to disable connection to DataServer.\n\
                          If this option is equal to zero and -F is used,\n\
                          only data buffered by NaqsServer will be retrieved.\n\
                          Rather than using -A, it is preferable, inside the section\n\
                          Datastream of the file Naqs.ini, setting DataBufferLength \n\
                          to a high value. -A allows to retrieve much more\n\
                          data of the past when the program restarts but it\n\
                          considerably slows down the execution.\n\
                          It is extremely harmful when you have many channels,\n\
                          in this case you might consider to subdivide the\n\
                          channels into different nmxptool instances.\n\
                          Related to -F.\n\
\n",
	    DEFAULT_MAX_TIME_TO_RETRIEVE,
	    DEFAULT_MAX_TIME_TO_RETRIEVE_MINIMUM,
	    DEFAULT_MAX_TIME_TO_RETRIEVE_MAXIMUM
);

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
PDS arguments for NaqsServer:\n\
  -P, --portpds=PORT      NaqsServer port number (default %d).\n\
  -S, --stc=SECs          Short-Term-Completion (default %d).\n\
                          -1 is for Raw Stream, no Short-Term-Completion.\n\
                             Packets contain compressed data. Related to -M, -T.\n\
                             It enables --rate=-1.\n\
                           0 decompressed packets are received in chronological\n\
                             order without waiting for missing packets.\n\
                          [1..300] decompressed packets are received in\n\
                             chronological order but waiting for missing packets\n\
                             at most SECs seconds.\n\
  -R, --rate=Hz           Receive data with specified sample rate (default %d).\n\
                          -1 for original sample rate and compressed data.\n\
                           0 for original sample rate and decompressed data.\n\
                          >0 for specified sample rate and decompressed data.\n\
  -b, --buffered          Request also recent packets into the past.\n\
  -B, --buffdate=DATE     Request also recent packets into the past\n\
                          but consider only samples after DATE.\n",
	    DEFAULT_PORT_PDS,
	    DEFAULT_STC,
	    DEFAULT_RATE);

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
  -f, --mschan=mSECs/nC   mSECs are milliseconds to wait before the next request,\n\
                          nC is the number of channels to request at a time.\n\
                          Delaying and requesting few channels at a time make\n\
                          data buffering on NaqsServer side more efficient.\n\
                          Determined empiric values are default %d/%d.\n\
                          Condition: TotalNumberOfChannels * (mSECs/nC) < %d sec. \n\
                          Related to -F and -b. 0/0 for disabling.\
\n",
DEFAULT_USEC / 1000, DEFAULT_N_CHANNEL, NMXP_MAX_MSCHAN_MSEC / 1000);

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
  -L, --listchannelsnaqs  List of the available Time Series channels on NaqsServer.\n\
                          The output list of channels can be redirected to a file\n\
                          which can be used as an input statefile for the option -F.\n\
  -M, --maxlatency=SECs   Max tolerable latency (default %d) [%d..%d].\n\
                          Enable NaqsServer to send out retransmission requests\n\
                          for missed packets. Inside the section NetworkInterface\n\
                          of the file Naqs.ini set RetxRequest to Enabled.\n\
                          If RetxRequest is not enabled then -M is ineffective.\n\
  -T, --timeoutrecv=SECs  Time-out for flushing queued packets of each channel.\n\
                          It sets --mschan=0/0 (default %d, no time-out) [%d..%d].\n\
                          -T is useful for retrieving Data On Demand with minimum delay.\n\
                          -M, -T are usable only with Raw Stream, -S=-1.\n\
                          In general, -M and -T are not used together.\n\
                          Receiving signals interrupts the time out.\n\
\n\
",
	    DEFAULT_MAX_TOLERABLE_LATENCY,
	    DEFAULT_MAX_TOLERABLE_LATENCY_MINIMUM,
	    DEFAULT_MAX_TOLERABLE_LATENCY_MAXIMUM,
	    DEFAULT_TIMEOUTRECV,
	    DEFAULT_TIMEOUTRECV_MINIMUM,
	    DEFAULT_TIMEOUTRECV_MAXIMUM
	  );

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
DAP arguments for DataServer:\n\
  -D, --portdap=PORT      DataServer port number (default %d).\n\
  -s, --start_time=DATE   Start time in date format.\n\
  -e, --end_time=DATE     End time in date format.\n\
                          DATE can be in formats:\n\
                              <date>,<time> | <date>\n\
                          where:\n\
                              <date> = yyyy/mm/dd | yyy.jjj\n\
                              <time> = hh:mm:ss | hh:mm:ss.dddd | hh:mm\n\
  -t, --interval=TIME     Time interval from start_time (greater than zero).\n\
                          TIME is in seconds, otherwise append 'm' for minutes\n\
                          'h' for hours or 'd' for days. [1 sec .. %d days]\n\
                          DO NOT USE with -e.\n\
  -d, --delay=TIME        Receive continuosly data with a fixed delay [%d sec .. %d days].\n\
  -a, --span_data=TIME    How much data is requested every time when receiving\n\
                          with a fixed delay [%d sec .. %d days]. (default %ds).\n\
                          Condition: span_data < delay - %ds. Related to -d.\n\
  -u, --username=USER     DataServer username.\n\
  -p, --password=PASS     DataServer password.\n\
  -l, --listchannels      List of the available Time Series channels on DataServer.\n\
  -i, --channelinfo       Print channelinfo (network name) when using -l.\n\
\n\
",
DEFAULT_PORT_DAP,
(DEFAULT_INTERVAL_MAXIMUM / 86400),
DEFAULT_DELAY_MINIMUM,
(DEFAULT_DELAY_MAXIMUM / 86400),
DEFAULT_SPANINTERVAL_MINIMUM,
(DEFAULT_SPANINTERVAL_MAXIMUM / 86400),
DEFAULT_SPANINTERVAL,
NMXP_DAP_TIMEOUT_KEEPALIVE
    );

#ifdef HAVE_LIBMSEED
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
Mini-SEED arguments:\n");

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
  -m, --writeseed=[TYPE]  Pack received data in Mini-SEED records and\n\
                          store them within a SDS or BUD structure.\n\
                          TYPE can be '%c' or '%c' (-m%c or -m%c)\n\
                          Packets are appended to existing files. Related to -o.\n",
			  TYPE_WRITESEED_SDS, TYPE_WRITESEED_BUD,
			  TYPE_WRITESEED_SDS, TYPE_WRITESEED_BUD);
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
  -o, --outdirseed=DIR    Output directory for SDS or BUD structure.\n\
                          Related to -m (default is current directory).\n");
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
  -q, --quality_indicator=IND\n\
                          Set the quality indicator to the given character.\n\
                          Seed 2.4 supports D (indeterminate), R (raw data),\n\
                          Q (quality controlled data) and M (data center modified).\n\
                          (Default is '%c').\n", DEFAULT_QUALITY_INDICATOR);
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
  -x, --encoding=ENC      Set the mini-SEED encoding. Either 'steim1' or 'steim2'.\n\
                          (Default is 'steim1').\n");
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
  -r, --reclen            Specify the desired mini-SEED record length in bytes\n\
                          which must be expressible as 2 raised to the power of X\n\
                          where X is between (and including) 8 to 20.\n\
                          (Default is %d).\n", DEFAULT_RECLEN_MINISEED);
#endif


#ifdef HAVE_SEEDLINK
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
SeedLink arguments:\n");

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
  -k, --slink=PLUGINID    Send received data to SeedLink as a plug-in.\n\
                          Data are sent by send_raw_depoch().\n\
                          This option, inside the file seedlink.ini, must be\n\
                          the last without adding value for PLUGINID!\n\
                          PLUGINID is set by SeisComP daemon.\n\
                          Not usable together with -K.\n");

#ifdef HAVE_LIBMSEED
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
  -K, --slinkms=PLUGINID  Send received data to SeedLink as a plug-in.\n\
                          This option is similar to previous -k,\n\
                          using -K the program converts data into mini-SEED\n\
                          records and sends them by the function send_mseed()\n\
                          instead of send_raw_depoch().\n\
                          Not usable together with -k.\n");
#endif

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
  -Q, --timing_quality=TQ This value is used for the functions send_raw*().\n\
                          TQ is %d or in [%d..%d] (default %d).\n",
			  DEFAULT_TIMING_QUALITY, DEFAULT_TIMING_QUALITY_MINIMUM, DEFAULT_TIMING_QUALITY_MAXIMUM,
			  DEFAULT_TIMING_QUALITY);

#endif

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
Other arguments:\n\
  -N, --network=NET       Default output Network code. (default '%s').\n\
  -n, --location=LOC      Default output Location code. (default is NULL).\n\
                          Location NULL is printed out as '%s'.\n\
  -v, --verbose=LEVEL     Be verbose. LEVEL is a bitmap:\n\
                          %d Channel State, %d Channel, %d Raw Stream,\n\
                          %d CRC32, %d Connection flow,\n\
                          %d Packet Management, %d Extra, %d Date,\n\
                          %d Gap, %d DOD, %d All messages.\n\
  -g, --logdata           Print info about packet data.\n\
  -G, --logsample         Print sample values of packets. Includes -g.\n\
",
	    NMXP_LOG_STR(DEFAULT_NETWORK),
	    NMXP_LOG_STR(DEFAULT_NULL_LOCATION),
	    NMXP_LOG_D_CHANSTATE,
	    NMXP_LOG_D_CHANNEL,
	    NMXP_LOG_D_RAWSTREAM,
	    NMXP_LOG_D_CRC,
	    NMXP_LOG_D_CONNFLOW,
	    NMXP_LOG_D_PACKETMAN,
	    NMXP_LOG_D_EXTRA,
	    NMXP_LOG_D_DATE,
	    NMXP_LOG_D_GAP,
	    NMXP_LOG_D_DOD,
	    NMXP_LOG_D_ANY
		);

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
  -w, --writefile         Dump received packets to a file.\n");

#ifndef HAVE_WINDOWS_H
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
  -E, --testport=PORT     Accept 'telnet' connection on PORT\n\
                          for data testing and diagnostic purposes.\n");
#endif

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
  -V, --version           Print tool version.\n\
  -h, --help              Print this help.\n\
\n");

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\
Signal handling:\n\
   INT QUIT TERM          Sending these signals to %s causes it\n\
                          to immediately attempt to gracefully terminate.\n\
                          It may take several seconds to complete exiting.\n\
   ALRM                   Print current info about Raw Stream buffer.\n\
   USR1                   Force to close a connection and open again\n\
                          without quitting the program. Only for connection\n\
                          in near real-time to NaqsServer.\n\
   HUP PIPE               Ignored. (SIG_IGN)\n\
\n", NMXP_LOG_STR(PACKAGE_NAME));

    nmxptool_author_support();

    /*
    if(long_options) {
	int i=0;
	while(long_options[i].name) {
	    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_EXTRA, "%s %d %d %d %c\n",
	    NMXP_LOG_STR(long_options[i].name), long_options[i].has_arg,
	    (long_options[i].flag)? *(long_options[i].flag) : 0, long_options[i].val, long_options[i].val);
	    i++;
	}
    }
    */
}


#define MAXSIZE_LINE_CHAN_STATE 2048
#define MAXSIZECHANNELSTRINGARGUMENT 8000
#define MAXSIZE_CHANNEL_STRING 64

char *get_channel_list_argument_from_state_file(const char *filename) {
    char *ret_channel_string = NULL;
    char line[MAXSIZE_LINE_CHAN_STATE];
    char str_chan[MAXSIZE_CHANNEL_STRING];
    int k, c;
    FILE *fstatefile = NULL;
    int i;
    int flag_insert = 1;

    fstatefile = fopen(filename, "r");

    /* Read only channel names from state file */
    if(fstatefile) {
	ret_channel_string = (char *) NMXP_MEM_MALLOC(MAXSIZECHANNELSTRINGARGUMENT);
	ret_channel_string[0] = 0;
	while(fgets(line, MAXSIZE_LINE_CHAN_STATE, fstatefile) != NULL) {
	    k = 0;
	    c = 0;

	    /* eat any white spaces and tabs at the beginning */
	    while(line[k] != 0
		    &&  (line[k] == ' ' ||  line[k] == '\t')
		    &&  k < MAXSIZE_CHANNEL_STRING) {
		k++;
	    }

	    /* copy any character up to the end of the line or # character */
	    while(line[k] != 0
		    &&  line[k] != '#'
		    &&  line[k] != 10
		    &&  line[k] != 13
		    &&  k < MAXSIZE_CHANNEL_STRING) {
		str_chan[c] = line[k];
		k++;
		c++;
	    }
	    str_chan[c] = 0;

	    /* remove possible white spaces and tabs from the end of the str_chan */
	    i = strlen(str_chan);
	    while(i > 0) {
		    if(str_chan[i] == ' ' || str_chan[i] == '\t') {
			str_chan[i] = 0;
		    }
		    i--;
	    }

	    /* check if str_chan is blank, before insert into the channel list */
	    flag_insert = 0;
	    i = 0;
	    while(i < strlen(str_chan)  &&  !flag_insert) {
		if(str_chan[i] != ' ') {
		    flag_insert = 1;
		}
		i++;
	    }

	    if(flag_insert) {
		if(ret_channel_string[0] == 0) {
		    strncpy(ret_channel_string, str_chan, MAXSIZECHANNELSTRINGARGUMENT);
		} else {
		    strncat(ret_channel_string, ",", MAXSIZECHANNELSTRINGARGUMENT - strlen(ret_channel_string));
		    strncat(ret_channel_string, str_chan, MAXSIZECHANNELSTRINGARGUMENT - strlen(ret_channel_string));
		}
	    } else {
		nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CHANSTATE, "Discarded '%s' from station file %s:\n%s",
			NMXP_LOG_STR(str_chan), NMXP_LOG_STR(filename), NMXP_LOG_STR(line));
	    }
	}
	fclose(fstatefile);
    }
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_CHANSTATE, "Channel list: %s\n",
	    NMXP_LOG_STR(ret_channel_string));
    return ret_channel_string;
}

int nmxptool_read_time(char *str_input, int32_t *pvalue) {
    char str_value[100];
    int len_int;
    int32_t value;
    int j;
    char unit = 'X';
    int ret_errors = 0;

    value = 0;
    strncpy(str_value, str_input, 100);
    len_int = strlen(str_value);
    if(len_int <= 0) {
	/* ERROR */
	ret_errors++;
    } else {
	j=0;
	while(j < len_int  && str_value[j] >= '0' && str_value[j] <= '9') {
	    j++;
	}
	if(j < len_int) {
	    if(j == len_int-1) {
		unit = str_value[j];
		str_value[j] = 0;
		if(unit == 'm' || unit == 'h' || unit == 'd') {
		    value = atoi(str_value);
		    switch(unit) {
			case 'm' :
			    value *= 60;
			    break;
			case 'h' :
			    value *= ( 60 * 60 );
			    break;
			case 'd' :
			    value *= ( 60 * 60 * 24 );
			    break;
		    }
		} else {
		    ret_errors++;
		    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY,
			    "Syntax of time '%s' is not correct!\n", NMXP_LOG_STR(str_value));
		}
	    } else {
		ret_errors++;
		nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY,
			"Syntax of time '%s' is not correct!\n", NMXP_LOG_STR(str_value));
	    }
	} else {
	    /* All numbers, then seconds */
	    value = atoi(str_value);
	}

    }

    *pvalue = value;
    return ret_errors;
}

int nmxptool_getopt_long(int argc, char **argv, NMXPTOOL_PARAMS *params)
{
    int ret_errors = 0;

    NMXP_TM_T tmp_tmt;
    int i;
    char one_time_option[255];
    int c;

#ifdef HAVE_LIBMSEED
    int flag_reclen_pow = 0;
    int reclen_pow = DEFAULT_RECLEN_MINIMUM;
#endif

    /*
    int len_int, j;
    char unit = 'X';
    char str_interval[100];
    */

    char *sep = NULL;

    struct option long_options[] =
    {
	/* These options set a flag. */
	/* It is not safe use reference to params in this way */
	/* {"verbose",        no_argument,       &(params->flag_verbose), 1}, */
	/* {"quiet",          no_argument,       &(params->flag_verbose), 0}, */
	/* These options don't set a flag.
	 *                   We distinguish them by their indices. */
	{"hostname",     required_argument, NULL, 'H'},
	{"portpds",      required_argument, NULL, 'P'},
	{"portdap",      required_argument, NULL, 'D'},
	{"channels",     required_argument, NULL, 'C'},
	{"network",      required_argument, NULL, 'N'},
	{"location",     required_argument, NULL, 'n'},
	{"stc",          required_argument, NULL, 'S'},
	{"rate",         required_argument, NULL, 'R'},
	{"start_time",   required_argument, NULL, 's'},
	{"end_time",     required_argument, NULL, 'e'},
	{"interval",     required_argument, NULL, 't'},
	{"delay",        required_argument, NULL, 'd'},
	{"span_data",    required_argument, NULL, 'a'},
	{"username",     required_argument, NULL, 'u'},
	{"password",     required_argument, NULL, 'p'},
	{"maxlatency",   required_argument, NULL, 'M'},
	{"timeoutrecv",  required_argument, NULL, 'T'},
	{"verbose",      required_argument, NULL, 'v'},
	{"bufferedt",    required_argument, NULL, 'B'},
	{"maxdataretr",  required_argument, NULL, 'A'},
	/* Following are flags */
	{"logdata",      no_argument,       NULL, 'g'},
	{"logsample",    no_argument,       NULL, 'G'},
	{"buffered",     no_argument,       NULL, 'b'},
	{"listchannels", no_argument,       NULL, 'l'},
	{"listchannelsnaqs", no_argument,   NULL, 'L'},
	{"channelinfo",  no_argument,       NULL, 'i'},
#ifdef HAVE_LIBMSEED
	{"writeseed",    required_argument, NULL, 'm'},
	{"outdirseed",   required_argument, NULL, 'o'},
	{"quality_indicator", required_argument, NULL, 'q'},
	{"encoding",     required_argument, NULL, 'x'},
	{"reclen",       required_argument, NULL, 'r'},
#endif
	{"writefile",    no_argument,       NULL, 'w'},
#ifdef HAVE_SEEDLINK
	{"slink",        required_argument, NULL, 'k'},
#endif
#ifdef HAVE_LIBMSEED
#ifdef HAVE_SEEDLINK
	{"slinkms",      required_argument, NULL, 'K'},
#endif
#endif
#ifdef HAVE_SEEDLINK
	{"timing_quality", required_argument, NULL, 'Q'},
#endif
#ifndef HAVE_WINDOWS_H
	{"socketport",   required_argument, NULL, 'E'},
#endif
	{"statefile",    required_argument, NULL, 'F'},
	{"mschan",       required_argument, NULL, 'f'},
	{"help",         no_argument,       NULL, 'h'},
	{"version",      no_argument,       NULL, 'V'},
	{0, 0, 0, 0}
    };

    char optstr[300] = "H:P:D:C:N:n:S:R:s:e:t:d:a:u:p:M:T:v:B:A:F:f:gGblLiwhV";

    int option_index = 0;


#ifdef HAVE_LIBMSEED
    strcat(optstr, "m:");
    strcat(optstr, "o:");
    strcat(optstr, "q:");
    strcat(optstr, "x:");
    strcat(optstr, "r:");
#endif


#ifdef HAVE_SEEDLINK
    strcat(optstr, "Q:");
    strcat(optstr, "k:");
#endif

#ifdef HAVE_LIBMSEED
#ifdef HAVE_SEEDLINK
    strcat(optstr, "K:");
#endif
#endif

#ifndef HAVE_WINDOWS_H
    strcat(optstr, "E:");
#endif

    /* getopt_long stores the option index here. */
    /* init array for checking one time option */
    for(i=0; i<255; i++) {
	one_time_option[i] = 0;
    }


    /* init params */
    memcpy(params, &NMXPTOOL_PARAMS_DEFAULT, sizeof(NMXPTOOL_PARAMS_DEFAULT));

    /* Check number of command line arguments for earthworm */
    if (argc == 2)
    {
	int l = strlen(argv[1]);
	if(l >= 3) {
	    if(argv[1][0] != '-') {
		if(argv[1][l-2] == '.'  &&  argv[1][l-1] == 'd') {
		    params->ew_configuration_file = argv[1];
		    return 0;
		}
	    }
	}
    }

    while ( (c = getopt_long (argc, argv, optstr, long_options, &option_index)) != -1) {

	/* BE CAREFUL if use synonym options !!! */
	one_time_option[c]++;

	if(one_time_option[c] > 1) {
	    ret_errors++;
	    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "Replicated option -%c (value %s)\n", c, NMXP_LOG_STR(optarg));
	} else {
	    switch (c)
	    {
		case 0:
		    /* If this option set a flag, do nothing else now. */
		    if (long_options[option_index].flag != 0)
			break;
		    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "option %s",
			    NMXP_LOG_STR(long_options[option_index].name));
		    if (optarg) {
			nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, " with arg %s", NMXP_LOG_STR(optarg));
		    }
		    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\n");
		    break;

		case 'H':
		    params->hostname = optarg;
		    break;

		case 'P':
		    params->portnumberpds = atoi(optarg);
		    break;

		case 'D':
		    params->portnumberdap = atoi(optarg);
		    break;

		case 'C':
		    if(params->channels) {
			nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY,
				"Channels have been already defined by State File (option -F)!\n");
			ret_errors++;
		    } else {
			params->channels = NMXP_MEM_STRDUP(optarg);
		    }
		    break;

		case 'N':
		    params->network = optarg;
		    break;

		case 'n':
		    params->location = optarg;
		    break;

		case 'S':
		    params->stc = atoi(optarg);
		    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_ANY, "Short-Term-Completion %d.\n", params->stc);
		    if(params->stc >= 0) {
			params->rate = 0; // original sample rate
		    }
		    break;

		case 'R':
		    params->rate = atoi(optarg);
		    break;

		case 's':
		    if(nmxp_data_parse_date(optarg, &tmp_tmt) == -1) {
			/* MESSAGE ERROR */
			ret_errors++;
		    } else {
			params->start_time = nmxp_data_tm_to_time(&tmp_tmt);
		    }
		    break;

		case 'e':
		    if(nmxp_data_parse_date(optarg, &tmp_tmt) == -1) {
			/* MESSAGE ERROR */
			ret_errors++;
		    } else {
			params->end_time = nmxp_data_tm_to_time(&tmp_tmt);
		    }
		    break;

		case 't':
		    ret_errors += nmxptool_read_time(optarg, &(params->interval) );
		    break;

		case 'd':
		    ret_errors += nmxptool_read_time(optarg, &(params->delay) );
		    break;

		case 'a':
		    ret_errors += nmxptool_read_time(optarg, &(params->span_data) );
		    break;

		case 'u':
		    params->datas_username = optarg;
		    break;

		case 'p':
		    params->datas_password = optarg;
		    break;

		case 'M':
		    params->max_tolerable_latency = atoi(optarg);
		    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_ANY, "Max_tolerable_latency %d\n",
			    params->max_tolerable_latency);
		    break;

		case 'T':
		    params->timeoutrecv = atoi(optarg);
		    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_ANY, "Time-out receiving %d\n",
			    params->timeoutrecv);
		    break;

		case 'v':
		    params->verbose_level = atoi(optarg);
		    break;

		case 'B':
		    params->flag_buffered = 1;
		    if(nmxp_data_parse_date(optarg, &tmp_tmt) == -1) {
			/* MESSAGE ERROR */
			ret_errors++;
		    } else {
			params->buffered_time = nmxp_data_tm_to_time(&tmp_tmt);
		    }
		    break;

		case 'A':
		    if(optarg) {
			params->max_data_to_retrieve = atoi(optarg);
		    }
		    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_ANY, "Max_time_to_retrieve %d\n", params->max_data_to_retrieve);
		    break;

#ifdef HAVE_SEEDLINK
		case 'k':
		    params->flag_slink = 1;
		    if(params->flag_slinkms) {
			nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY,
				"Options -k and -K are declared in the same command line!\n");
			ret_errors++;
		    } else {
			params->plugin_slink = optarg;
			if(params->plugin_slink) {
			    nmxp_log_set_prefix(params->plugin_slink);
			}
		    }
		    break;
#endif

#ifdef HAVE_LIBMSEED
#ifdef HAVE_SEEDLINK
		case 'K':
		    params->flag_slinkms = 1;
		    if(params->flag_slink) {
			nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY,
				"Options -k and -K are declared in the same command line!\n");
			ret_errors++;
		    } else {
			params->plugin_slink = optarg;
			if(params->plugin_slink) {
			    nmxp_log_set_prefix(params->plugin_slink);
			}
		    }
		    break;
#endif
#endif

#ifdef HAVE_SEEDLINK
		case 'Q':
		    params->timing_quality = atoi(optarg);
		    break;
#endif

#ifndef HAVE_WINDOWS_H
		case 'E':
		    params->listen_port = atoi(optarg);
		    break;
#endif

		case 'F':
		    params->statefile = optarg;
		    if(params->channels == NULL) {
			params->channels = get_channel_list_argument_from_state_file(params->statefile);
			if(params->channels) {
			    /* Do nothing */
			} else {
			    ret_errors++;
			    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY,
				    "State file %s not found or unable to read!\n", NMXP_LOG_STR(params->statefile));
			}
		    } else {
			ret_errors++;
			nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY,
				"Channels have been already defined by option -C!\n");
		    }
		    break;

		case 'f':
		    sep = strstr(optarg, "/");
		    if(sep) {
			sep[0] = 0;
			sep++;
			params->usec = atoi(optarg) * 1000;
			params->n_channel = atoi(sep);
			nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_ANY,
				"Channels %d usec %d!\n", params->n_channel, params->usec);
		    } else {
			ret_errors++;
			nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY,
				"Syntax error in option -%c %s!\n", c, NMXP_LOG_STR(optarg));
		    }
		    break;

		case 'g':
		    params->flag_logdata = 1;
		    break;

		case 'G':
		    params->flag_logdata = 1;
		    params->flag_logsample = 1;
		    break;

		case 'b':
		    params->flag_buffered = 1;
		    break;

		case 'l':
		    params->flag_listchannels = 1;
		    break;

		case 'L':
		    params->flag_listchannelsnaqs = 1;
		    break;

		case 'i':
		    params->flag_request_channelinfo = 1;
		    break;

#ifdef HAVE_LIBMSEED
		case 'm':
		    if(optarg && strlen(optarg) == 1) {
			params->type_writeseed = optarg[0];
			if(params->type_writeseed != TYPE_WRITESEED_SDS
				&& params->type_writeseed != TYPE_WRITESEED_BUD) {
			    ret_errors++;
			    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY,
				    "Syntax error in option -%c %s!\n", c, NMXP_LOG_STR(optarg));
			}
		    } else {
			ret_errors++;
			nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY,
				"Syntax error in option -%c %s!\n", c, NMXP_LOG_STR(optarg));
		    }
		    break;

		case 'o':
		    params->outdirseed = optarg;
		    if(params->outdirseed) {
			if(nmxp_data_dir_exists(params->outdirseed)) {
			    params->outdirseed =  nmxp_data_dir_abspath(params->outdirseed);
			} else {
			    ret_errors++;
			    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY,
				    "Mini-SEED output directory %s does not exist!\n", NMXP_LOG_STR(params->outdirseed));
			}
		    }
		    break;

                case 'q':
                    switch(optarg[0]) {
                      case 'D':
                      case 'R':
                      case 'Q':
                      case 'M':
                        params->quality_indicator = optarg[0];
                        break;
                      default:
                        ret_errors++;
                        nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY,
                               "Quality indicator %s is invalid! Must be D, R, Q or M!\n", NMXP_LOG_STR(optarg));
                        break;
                    }
		    break;

		case 'x':
		    if (strcmp(optarg,"steim1") == 0) {
			params->encoding = DE_STEIM1;
		    } else if (strcmp(optarg,"steim2") == 0) {
			params->encoding = DE_STEIM2;
		    } else {
			ret_errors++;
			nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY,
				"Encoding %s is invalid! Must either be steim1 or steim2!\n",NMXP_LOG_STR(optarg));
		    }
		    break;

		case 'r':
		    params->reclen = atoi(optarg);
		    if(params->reclen >= DEFAULT_RECLEN_MINIMUM  &&  params->reclen <= DEFAULT_RECLEN_MAXIMUM) {
			flag_reclen_pow = 0;
			while(!flag_reclen_pow  &&  reclen_pow <= DEFAULT_RECLEN_MAXIMUM) {
			    if(params->reclen == reclen_pow) {
				flag_reclen_pow = 1;
			    }
			    reclen_pow *= 2;
			}
			if(!flag_reclen_pow) {
			    ret_errors++;
			    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY,
				    "reclen must expressible as 2 raised to the power of X where X is between (and including) 8 to 20.\n");
			}
		    } else {
			ret_errors++;
			nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY,
				"reclen must expressible as 2 raised to the power of X where X is between (and including) 8 to 20.\n");
		    }
		    break;
#endif

		case 'w':
		    params->flag_writefile = 1;
		    break;

		case 'h':
		    nmxptool_usage(long_options);
		    exit (1);
		    break;

		case 'V':
		    nmxptool_version();
		    nmxptool_author_support();
		    exit (1);
		    break;

		case '?':
		    /* getopt_long already printed an error message. */
		    ret_errors++;
		    break;

		default:
		    nmxptool_usage(long_options);
		    exit (1);
	    }
	}
    }

    /* Print any remaining command line arguments (not options). */
    if (optind < argc)
    {
	ret_errors += optind;

	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "non-option ARGV-elements: ");
	while (optind < argc) {
	    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "%s ",
		    NMXP_LOG_STR(argv[optind]));
	    optind++;
	}
	putchar ('\n');
    }

    if(params->statefile) {
	params->flag_buffered = 1;
	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY,
		"Enable buffer for requesting also recent packets into the past.\n");
    }

    return ret_errors;
}


void nmxptool_log_params(NMXPTOOL_PARAMS *params) {
    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_EXTRA, "\
    char *hostname: %s\n\
    int portnumberdap: %d\n\
    int portnumberpds: %d\n\
",
    NMXP_LOG_STR(params->hostname),
    params->portnumberdap,
    params->portnumberpds
);

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_EXTRA, "\
    char *channels: %s\n\
",
    NMXP_LOG_STR(params->channels)
);

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_EXTRA, "\
    char *network: %s\n\
    char *location: %s\n\
    double start_time: %f\n\
    double end_time: %f\n\
    int32_t interval: %d\n\
",
    NMXP_LOG_STR(params->network),
    NMXP_LOG_STR(params->location),
    params->start_time,
    params->end_time,
    params->interval
);


    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_EXTRA, "\
    char *datas_username: %s\n\
    char *datas_password: %s\n\
",
    NMXP_LOG_STR(params->datas_username),
    NMXP_LOG_STR(params->datas_password)
);

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_EXTRA, "\
    int32_t stc: %d\n\
    int32_t rate: %d\n\
    char *plugin_slink: %s\n\
    int32_t delay: %d\n\
    int32_t max_tolerable_latency: %d\n\
    int32_t timeoutrecv: %d\n\
    int32_t verbose_level: %d\n\
",
    params->stc,
    params->rate,
    NMXP_LOG_STR(params->plugin_slink),
    params->delay,
    params->max_tolerable_latency,
    params->timeoutrecv,
    params->verbose_level
);

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_EXTRA, "\
    char *ew_configuration_file: %s\n\
    char *statefile: %s\n\
    int32_t max_data_to_retrieve: %d\n\
",
    NMXP_LOG_STR(params->ew_configuration_file),
    NMXP_LOG_STR(params->statefile),
    params->max_data_to_retrieve
);

    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_EXTRA, "\
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
    params->buffered_time,
    params->type_writeseed,
    params->timing_quality,
    params->flag_listchannels,
    params->flag_listchannelsnaqs,
    params->flag_request_channelinfo,
    params->flag_writefile,
    params->flag_slink,
    params->flag_slinkms,
    params->flag_buffered,
    params->flag_logdata,
    params->flag_logsample
    );
}



int nmxptool_check_params(NMXPTOOL_PARAMS *params) {
    int ret = 0;

    if(params->ew_configuration_file != NULL) {
	/* Do nothing */
    } else if(params->hostname == NULL) {
	ret = -1;
	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "<hostname> is required!\n");
    } else if(params->flag_listchannels) {
	if(params->flag_listchannelsnaqs) {
	    ret = -1;
	    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "-l and -L can not be used together!\n");
	}
    } else if(params->flag_listchannelsnaqs) {
	/* Do nothing */
    } else if(params->hostname == NULL) {
	ret = -1;
	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "<hostname> is required!\n");
    } else if(params->channels == NULL) {
	ret = -1;
	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "Channel list is required!\n");
    } else if(params->start_time == 0.0 &&  params->end_time != 0.0) {
	ret = -1;
	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "<end_time> is required when declaring <start_time>!\n");
    } else if(params->start_time != 0.0  &&  params->end_time != 0.0  && params->interval != DEFAULT_INTERVAL_NO_VALUE) {
	ret = -1;
	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "<start_time> has to be used with either <end_time> or <interval>!\n");
    } else if(params->interval != DEFAULT_INTERVAL_NO_VALUE && params->interval <= 0) {
	ret = -1;
	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "<interval> has to be greater than zero!\n");
    } else if(params->interval > DEFAULT_INTERVAL_MAXIMUM) {
	ret = -1;
	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "<interval> has to be less than %d seconds (%d days)!\n", DEFAULT_INTERVAL_MAXIMUM, DEFAULT_INTERVAL_MAXIMUM / 86400);
    } else if(params->start_time != 0.0   &&   params->end_time != 0.0
	    && params->start_time >= params->end_time) {
	ret = -1;
	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "<start_time> is less than <end_time>!\n");
    } else if(params->stc < DEFAULT_STC_MINIMUM   ||   params->stc > DEFAULT_STC_MAXIMUM) {
	ret = -1;
	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "<stc> has to be in the interval [%d..%d] secs.\n",
		DEFAULT_STC_MINIMUM, DEFAULT_STC_MAXIMUM);
    } else if(params->stc == -1   &&   params->rate != DEFAULT_RATE) {
	ret = -1;
	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "<rate> has to be equal to -1 when <stc> is equal to -1 (Raw Stream).\n");
    } else if(params->delay > 0 && params->start_time != 0.0   &&   params->end_time != 0.0) {
	ret = -1;
	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "<delay> can not be used with options <start_time> and <end_time>.\n");
    } else if( params->delay != DEFAULT_DELAY &&
	    (params->delay < DEFAULT_DELAY_MINIMUM  || params->delay > DEFAULT_DELAY_MAXIMUM) ) {
	ret = -1;
	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "<delay> has to be in the interval [%d..%d] secs.\n",
		DEFAULT_DELAY_MINIMUM, DEFAULT_DELAY_MAXIMUM);
    } else if(params->span_data < DEFAULT_SPANINTERVAL_MINIMUM || params->span_data > DEFAULT_SPANINTERVAL_MAXIMUM) {
	ret = -1;
	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "<span_data> has to be in the interval [%d..%d] secs.\n",
		DEFAULT_SPANINTERVAL_MINIMUM, DEFAULT_SPANINTERVAL_MAXIMUM);
    } else if(params->delay != DEFAULT_DELAY &&
	    (params->span_data >= params->delay - NMXP_DAP_TIMEOUT_KEEPALIVE)) {
	ret = -1;
	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "<span_data> has to be less than delay - %ds. Current value should be less than %ds.\n",
		NMXP_DAP_TIMEOUT_KEEPALIVE, params->delay - NMXP_DAP_TIMEOUT_KEEPALIVE);
    } else if(params->verbose_level < DEFAULT_VERBOSE_LEVEL_MINIMUM  ||  params->verbose_level > DEFAULT_VERBOSE_LEVEL_MAXIMUM) {
	ret = -1;
	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "<verbose_level> has to be in the interval [%d..%d].\n",
		DEFAULT_VERBOSE_LEVEL_MINIMUM, DEFAULT_VERBOSE_LEVEL_MAXIMUM);
    } else if(params->rate < DEFAULT_RATE_MINIMUM  ||  params->rate > DEFAULT_RATE_MAXIMUM) {
	ret = -1;
	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "<rate> has to be in the interval [%d..%d].\n",
		DEFAULT_RATE_MINIMUM, DEFAULT_RATE_MAXIMUM);
    } else if(params->rate != -1 && params->start_time != 0.0   &&   params->end_time != 0.0) {
	ret = -1;
	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "<rate> can not be used with options <start_time> and <end_time>.\n");
    } else if(params->flag_buffered != 0 && params->start_time != 0.0   &&   params->end_time != 0.0) {
	ret = -1;
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY, "<buffered> can not be used with options <start_time> and <end_time>.\n");
    } else if( (params->max_data_to_retrieve < DEFAULT_MAX_TIME_TO_RETRIEVE_MINIMUM  ||
		params->max_data_to_retrieve > DEFAULT_MAX_TIME_TO_RETRIEVE_MAXIMUM)) {
	ret = -1;
	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "<maxdataretr> has to be within [%d..%d].\n",
		DEFAULT_MAX_TIME_TO_RETRIEVE_MINIMUM,
		DEFAULT_MAX_TIME_TO_RETRIEVE_MAXIMUM);

#ifdef HAVE_LIBMSEED
    } else if(params->outdirseed) {
	if(!nmxp_data_dir_exists(params->outdirseed)) {
	    /* ERROR */
	    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_EXTRA, "Output directory %s does not exist!\n", params->outdirseed);
	    ret = -1;
	} else {    
	}
    } else if(params->type_writeseed  && !params->outdirseed) {
	params->outdirseed =  nmxp_data_gnu_getcwd();
	nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_ANY, "Set output dir to %s.\n",
		params->outdirseed);
#endif

    } else if( params->stc == -1
	    && (params->max_tolerable_latency < DEFAULT_MAX_TOLERABLE_LATENCY_MINIMUM  ||
		params->max_tolerable_latency > DEFAULT_MAX_TOLERABLE_LATENCY_MAXIMUM)) {
	ret = -1;
	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "<maxlatency> has to be within [%d..%d].\n",
		DEFAULT_MAX_TOLERABLE_LATENCY_MINIMUM,
		DEFAULT_MAX_TOLERABLE_LATENCY_MAXIMUM);
    } else if( params->stc == -1
	    && (params->timeoutrecv < DEFAULT_TIMEOUTRECV_MINIMUM  ||
		params->timeoutrecv > DEFAULT_TIMEOUTRECV_MAXIMUM)) {
	if(params->timeoutrecv != 0) {
	    ret = -1;
	    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "<timeoutrecv> has to be within [%d..%d] or equal to zero for not time-out.\n",
		    DEFAULT_TIMEOUTRECV_MINIMUM,
		    DEFAULT_TIMEOUTRECV_MAXIMUM);
	}

    } else if(
	    (params->networkdelay < DEFAULT_NETWORKDELAY_MINIMUM  ||
	     params->networkdelay > DEFAULT_NETWORKDELAY_MAXIMUM)) {
	ret = -1;
	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "<networkdelay> has to be within [%d..%d].\n",
		DEFAULT_NETWORKDELAY_MINIMUM,
		DEFAULT_NETWORKDELAY_MAXIMUM);

	/* Follow warning messages only */
    } else if( params->stc != -1 && params->max_tolerable_latency > 0 ){
	nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_ANY, "<maxlatency> ignored since not defined --stc=-1.\n");
    } else if(params->stc != -1 && params->timeoutrecv > 0) {
	params->timeoutrecv = 0;
	nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_ANY, "<timeoutrecv> ignored since not defined --stc=-1.\n");
    }

#ifdef HAVE_SEEDLINK
    if( params->timing_quality != DEFAULT_TIMING_QUALITY &&
	    ( params->timing_quality < DEFAULT_TIMING_QUALITY_MINIMUM ||  params->timing_quality > DEFAULT_TIMING_QUALITY_MAXIMUM) ) {
	ret = -1;
	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "<timing_quality> could be %d or within [%d .. %d].\n",
		DEFAULT_TIMING_QUALITY, DEFAULT_TIMING_QUALITY_MINIMUM, DEFAULT_TIMING_QUALITY_MAXIMUM);
    }
#endif

    if(params->usec == 0  &&  params->n_channel == 0) {
	/* Do nothing */
    } else if(params->usec < DEFAULT_USEC_MINIMUM  ||  params->usec > DEFAULT_USEC_MAXIMUM) {
	ret = -1;
	nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_ANY, "ms in <mschan> has to be within [%d..%d] or equal to zero if 0/0.\n",
		DEFAULT_USEC_MINIMUM/1000, DEFAULT_USEC_MAXIMUM/1000);
    } else if(params->n_channel < DEFAULT_N_CHANNEL_MINIMUM  ||  params->n_channel > DEFAULT_N_CHANNEL_MAXIMUM) {
	ret = -1;
	nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_ANY, "nC in <mschan> has to be within [%d..%d] or equal to zero if 0/0.\n",
		DEFAULT_N_CHANNEL_MINIMUM, DEFAULT_N_CHANNEL_MAXIMUM);
    }

    if(params->timeoutrecv > 0 &&
	    (params->usec != 0  ||  params->n_channel != 0)) {
	params->usec = 0;
	params->n_channel=0;
	nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_ANY, "<mschan> can not be used with <timeoutrecv>. <mschan> set to 0/0.\n");
    }
    
    /*
    if( params->stc == -1 ) {
	nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_ANY, "<maxlatency> is equal to %d sec.\n", params->max_tolerable_latency);
    }
    */

    return ret;
}
