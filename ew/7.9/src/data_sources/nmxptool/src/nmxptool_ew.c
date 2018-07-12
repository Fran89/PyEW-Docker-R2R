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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "config.h"
#include "nmxp.h"
#include "nmxptool_getoptlong.h"

#ifdef HAVE_EARTHWORMOBJS
/* Earthworm includes */
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <trace_buf.h>

#include "nmxptool_ew.h"

NMXPTOOL_EW_ERR_MSG nmxptool_ew_err_msg[NMXPTOOL_EW_ERR_MAXVALUE] = {
    { NMXPTOOL_EW_ERR_NULL, ""} ,
    { NMXPTOOL_EW_ERR_RECVDATA, "Error receiving data." },
    { NMXPTOOL_EW_ERR_TERMREQ, "Terminating on request." }
};


#define MAXMESSAGELEN   160     /* Maximum length of a status or error  */
/*   message.                           */
#define MAXRINGNAMELEN  28      /* Maximum length of a ring name.       */
/* Should be defined by kom.h           */
#define MAXMODNAMELEN   30      /* Maximum length of a module name      */
/* Should be defined by kom.h           */
#define MAXADDRLEN      80      /* Length of NaqsServer hostname/address  */

SHM_INFO      regionOut;         /* Shared memory region                 */
pid_t         myPid;             /* Process ID                           */

char  ringName[MAXRINGNAMELEN];  /* Name of destination ring for data    */
long  ringKey;                   /* Key to output shared memory region   */
char  myModName[MAXMODNAMELEN];  /* Name of module instance              */
int   forcetracebuf = 0;         /* Switch to force TRACEBUF             */

unsigned char myModId;           /* ID of this module                    */
unsigned char myInstId;          /* Installation running this module     */
unsigned char typeError;         /* Error message type                   */
unsigned char typeHeartbeat;     /* Heartbeat message type               */
unsigned char typeWaveform;      /* Waveform message type TRACEBUF       */
unsigned char typeWaveform2 = 0; /* Waveform message type TRACEBUF2      */

MSG_LOGO      hrtLogo;           /* Heartbeat message logo               */
MSG_LOGO      waveLogo;          /* Waveform message logo                */
MSG_LOGO      errLogo;           /* Error message logo                   */

int           heartbeatInt;      /* Heartbeat interval (seconds)         */
int           logSwitch;         /* 1 -> write log, 0 -> no log          */
				/* 2 -> write module log but not stderr/stdout */

time_t timeNow;
time_t timeLastBeat = 0;

void nmxptool_ew_attach() {
    /* Attach to Output transport ring */
    tport_attach (&regionOut, ringKey);
    logit ("t", "%s version %s\n",
	    NMXP_LOG_STR(PACKAGE_NAME), NMXP_LOG_STR(PACKAGE_VERSION));
}

void nmxptool_ew_detach() {
    tport_detach(&regionOut);
    logit("t","%s terminated\n", NMXP_LOG_STR(PACKAGE_NAME));
}


int nmxptool_ew_pd2ewring (NMXP_DATA_PROCESS *pd, SHM_INFO *pregionOut, MSG_LOGO *pwaveLogo) {
    TracePacket tbuf;
    int tracebuf2 = 0;   /* TRACEBUF2 => 0: none, 1: available, 2: populated */
    int len;
    int32_t *samples;
    int i;

    /* TRACE_HEADER and TRACE2_HEADER are the same size */
    memset (&tbuf, 0, sizeof(TRACE_HEADER));

    /* Create a TRACEBUF2 message if supported */
#ifdef TRACE2_STA_LEN
    tracebuf2 = 1;

    if ( ! forcetracebuf ) {
	tbuf.trh2.pinno = 0;
	tbuf.trh2.nsamp = pd->nSamp;

	tbuf.trh2.starttime = pd->time;
	tbuf.trh2.samprate = pd->sampRate;
	if(pd->sampRate > 0) {
	    tbuf.trh2.endtime = (tbuf.trh2.starttime +
		(((double) tbuf.trh2.nsamp - 1.0) / (double) tbuf.trh2.samprate));
	} else {
	    /* Avoiding to divide by zero, try to invalidate the data in a safe way */
	    tbuf.trh2.endtime = tbuf.trh2.starttime;
	}

	strncpy(tbuf.trh2.net, pd->network, TRACE2_NET_LEN);
	strncpy(tbuf.trh2.sta, pd->station, TRACE2_STA_LEN);
	strncpy(tbuf.trh2.chan, pd->channel, TRACE2_CHAN_LEN);

	strncpy(tbuf.trh2.loc, (pd->location[0]==0)?  LOC_NULL_STRING : ( strcmp(pd->location, DEFAULT_NULL_LOCATION)==0? LOC_NULL_STRING : pd->location ), 2);

	tbuf.trh2.version[0] = TRACE2_VERSION0;
	tbuf.trh2.version[1] = TRACE2_VERSION1;

	/* The decoding always produces 32-bit integers in host byte order */
#ifdef _INTEL
	strcpy(tbuf.trh2.datatype, "i4");
#endif
#ifdef _SPARC
	strcpy(tbuf.trh2.datatype, "s4");
#endif

	tbuf.trh2.quality[0] = 100; /* TODO */
	tbuf.trh2.quality[1] = 0;

	tracebuf2 = 2;
    }
#endif

    if ( tracebuf2 != 2 ) {
	/* Create a TRACEBUF message otherwise */
	tbuf.trh.pinno = 0;
	tbuf.trh.nsamp = pd->nSamp;

	tbuf.trh.starttime = pd->time;
	tbuf.trh.samprate = pd->sampRate;
	if(pd->sampRate > 0) {
	    tbuf.trh.endtime = (tbuf.trh.starttime +
		(((double) tbuf.trh.nsamp - 1.0) / (double) tbuf.trh.samprate));
	} else {
	    /* Avoiding to divide by zero, try to invalidate the data in a safe way */
	    tbuf.trh.endtime = tbuf.trh.starttime;
	}

	strncpy(tbuf.trh.net, pd->network, TRACE_NET_LEN);
	strncpy(tbuf.trh.sta, pd->station, TRACE_STA_LEN);
	strncpy(tbuf.trh.chan, pd->channel, TRACE_CHAN_LEN);

	/* The decoding always produces 32-bit integers in host byte order */
#ifdef _INTEL
	strcpy(tbuf.trh.datatype, "i4");
#endif
#ifdef _SPARC
	strcpy(tbuf.trh.datatype, "s4");
#endif

	tbuf.trh.quality[0] = 100; /* TODO */
	tbuf.trh.quality[1] = 0;
    }


    /* TODO : all of the samples
       should always fit into a single TracePacket if MAX_TRACEBUF_SIZ
       remains defined in Trace_buf.h as 4096 or greater
       17 * 59 = 1003 samples = 4012 bytes
       4012 + 64 = 4076 < 4096
     */

    samples = (int32_t *) ((char *)&tbuf + sizeof(TRACE_HEADER));
    for(i=0; i < pd->nSamp; i++) {
	samples[i] = pd->pDataPtr[i];
    }

    len = (pd->nSamp * sizeof(int32_t)) + sizeof(TRACE_HEADER);

    /* Set the approriate TRACE type in the logo */
    if ( tracebuf2 == 2 ) {
	if ( typeWaveform2 == 0 ) {
	    logit("et", "%s: Error - created TRACE2_HEADER but TYPE_TRACEBUF2 is unknown\n", NMXP_LOG_STR(PACKAGE_NAME));
	    return EW_FAILURE;
	} else {
	    pwaveLogo->type = typeWaveform2;
	}
    } else {
	pwaveLogo->type = typeWaveform;
    }

    if ( tport_putmsg( pregionOut, pwaveLogo, len, (char*)&tbuf ) != PUT_OK ) {
	logit("et", "%s: Error sending message via transport.\n", NMXP_LOG_STR(PACKAGE_NAME));
	return EW_FAILURE;
    }

    return EW_SUCCESS;
}				/* End of nmxptool_ew_pd2ewring() */


int nmxptool_ew_nmx2ew(NMXP_DATA_PROCESS *pd) {
    int ret;
    ret = nmxptool_ew_pd2ewring (pd, &regionOut, &waveLogo);
    return ret;
}



/***************************************************************************
 * nmxptool_ew_configure():
 * Process configuration parameters.
 *
 ***************************************************************************/
void nmxptool_ew_configure (char ** argvec, NMXPTOOL_PARAMS *params) {

    /* Initialize name of log-file & open it */
    logit_init (argvec[1], 0, 512, 1);

    /* Read module config file */
    if ( nmxptool_ew_proc_configfile (argvec[1], params) == EW_FAILURE ) {
	logit("et", "%s: configure() failed \n", argvec[0]);
	exit (EW_FAILURE);
    }

    /* Read node configuration info */
    if ( GetLocalInst( &myInstId) != 0 ) {
	logit("et", "%s: Error getting myInstId.\n", PACKAGE_NAME );
	exit (EW_FAILURE);
    }

    /* Lookup the ring key */
    if ((ringKey = GetKey (ringName) ) == -1) {
	logit("et",
		"%s:  Invalid ring name <%s>; exitting!\n", PACKAGE_NAME, ringName);
	exit (EW_FAILURE);
    }

    /* Look up message types of interest */
    if (GetType ("TYPE_HEARTBEAT", &typeHeartbeat) != 0) {
	logit("et", 
		"%s: Invalid message type <TYPE_HEARTBEAT>; exitting!\n", PACKAGE_NAME);
	exit (EW_FAILURE);
    }
    if (GetType ("TYPE_ERROR", &typeError) != 0) {
	logit("et", 
		"%s: Invalid message type <TYPE_ERROR>; exitting!\n", PACKAGE_NAME);
	exit (EW_FAILURE);
    }

    if (GetType ("TYPE_TRACEBUF", &typeWaveform) != 0) {
	logit("et", 
		"%s: Invalid message type <TYPE_TRACEBUF>; exitting!\n", PACKAGE_NAME);
	exit (EW_FAILURE);
    }

    /* No GetType error checking as this type will not exist in all versions */
    GetType ("TYPE_TRACEBUF2", &typeWaveform2);

    /* Set up logos for outgoing messages */
    hrtLogo.instid = myInstId;
    hrtLogo.mod    = myModId;
    hrtLogo.type   = typeHeartbeat;

    errLogo.instid = myInstId;
    errLogo.mod    = myModId;
    errLogo.type   = typeError;

    waveLogo.instid = myInstId;
    waveLogo.mod    = myModId;
    waveLogo.type   = 0;  /* This gets set to the appropriate type later */

    /* Get my process ID so I can let statmgr restart me */
    myPid = getpid();

    logit ("et" , "%s(%s): Read command file <%s>\n",
	    NMXP_LOG_STR(argvec[0]), NMXP_LOG_STR(myModName), NMXP_LOG_STR(argvec[1]));

    /* Reinitialize the logging level */
    logit_init (argvec[1], 0, 512, logSwitch);

}				/* End of nmxptool_ew_configure() */


/***************************************************************************
 * nmxptool_ew_proc_configfile():
 * Process the module configuration parameters.
 *
 ***************************************************************************/
int nmxptool_ew_proc_configfile (char * configfile, NMXPTOOL_PARAMS *params) {
    char    		*com;
    char    		*str;
    int      		nfiles;
    int      		success;
    int                 flag_MaxTolerableLatency = 0;
    int                 flag_ShortTermCompletion = 0;

    char *sep = NULL;

    /* Some important initial values or defaults */
    ringName[0]   = '\0';
    myModName[0] = '\0';
    heartbeatInt = -1;
    logSwitch    = -1;

    /* Open the main configuration file */
    nfiles = k_open (configfile);
    if (nfiles == 0) {
	logit("et",
		"%s: Error opening command file <%s>; exiting!\n", PACKAGE_NAME,
		configfile);
	return EW_FAILURE;
    }

    /* Process all command files */
    while (nfiles > 0) {   /* While there are command files open */
	while (k_rd ()) {       /* Read next line from active file  */
	    com = k_str ();         /* Get the first token from line */

	    /* Ignore blank lines & comments */
	    if (!com)
		continue;
	    if (com[0] == '#')
		continue;

	    /* Open a nested configuration file */
	    if (com[0] == '@') {
		success = nfiles + 1;
		nfiles  = k_open (&com[1]);
		if (nfiles != success) {
		    logit("et", 
			    "%s: Error opening command file <%s>; exiting!\n", PACKAGE_NAME,
			    &com[1]);
		    return EW_FAILURE;
		}
		continue;
	    }

	    /* Process anything else as a command */
	    if (k_its ("MyModuleId")) {
		if ( (str = k_str ()) ) {
		    if (strlen(str) >= MAXMODNAMELEN) {
			logit("et", "MyModId too long; max is %d\n", MAXMODNAMELEN -1);
			return EW_FAILURE;
		    }

		    strncpy (myModName, str, MAXMODNAMELEN);

		    /* Lookup module ID */
		    if ( GetModId( myModName, &myModId) != 0 ) {
			logit("et", "%s: Error getting myModId.\n", PACKAGE_NAME );
			exit (EW_FAILURE);
		    }
		}
	    }

	    else if (k_its ("RingName")) {
		if ( (str = k_str ()) ) {
		    if (strlen(str) >= MAXRINGNAMELEN) {
			logit("et", "OutRing name too long; max is %d\n", 
				MAXRINGNAMELEN - 1);
			return EW_FAILURE;
		    }

		    strncpy (ringName, str, MAXRINGNAMELEN);
		}
	    }

	    else if (k_its ("HeartBeatInterval")) {
		heartbeatInt = k_long ();
	    }

	    else if (k_its ("LogFile")) {
		logSwitch = k_int();
	    }

	    else if (k_its ("Verbosity")) {
		params->verbose_level = k_int();
	    }

	    else if (k_its ("NmxpHost")) {
		if ( (str = k_str ()) ) {
		    if (strlen(str) >= MAXADDRLEN) {
			logit("et", "nmxphost too long; max is %d characters\n",
				MAXADDRLEN);
			return EW_FAILURE;
		    }
		    params->hostname = NMXP_MEM_STRDUP(str);
		}
	    }

	    else if ( k_its ("NmxpPortPDS")) {
		params->portnumberpds = k_int();
	    }

	    else if ( k_its ("NmxpPortDAP")) {
		params->portnumberdap = k_int();
	    }

	    else if (k_its ("UserDAP")) {
		if ( (str = k_str ()) ) {
		    params->datas_username = NMXP_MEM_STRDUP(str);
		}
	    }

	    else if (k_its ("PassDAP")) {
		if ( (str = k_str ()) ) {
		    params->datas_password = NMXP_MEM_STRDUP(str);
		}
	    }

	    else if (k_its ("ForceTraceBuf1")) {
		forcetracebuf = k_int();
	    }

	    else if (k_its ("TimeoutRecv")) {
		params->timeoutrecv = k_int();
	    }

	    else if (k_its ("MaxTolerableLatency")) {
		params->max_tolerable_latency = k_int();
		flag_MaxTolerableLatency = 1;
		if(flag_ShortTermCompletion) {
		    logit("et", "You can use either MaxTolerableLatency or ShortTermCompletion\n");
		    return EW_FAILURE;
		}
	    }

	    else if (k_its ("ShortTermCompletion")) {
		params->stc = k_int();
		params->rate = 0; // original sample rate
		flag_ShortTermCompletion = 1;
		if(flag_MaxTolerableLatency) {
		    logit("et", "You can use either MaxTolerableLatency or ShortTermCompletion\n");
		    return EW_FAILURE;
		}
	    }

	    else if (k_its ("MaxDataToRetrieve")) {
		params->max_data_to_retrieve = k_int();
	    }

	    else if (k_its ("DefaultNetworkCode")) {
		if ( (str = k_str ()) ) {
		    if(params->network) {
			logit("et", "DefaultNetworkCode has been replicated!\n");
			return EW_FAILURE;
		    } else {
			params->network = NMXP_MEM_STRDUP(str);
		    }
		}
	    }

	    else if (k_its ("DefaultLocationCode")) {
		if ( (str = k_str ()) ) {
		    if(params->location) {
			logit("et", "DefaultLocationCode has been replicated!\n");
			return EW_FAILURE;
		    } else {
			params->location = NMXP_MEM_STRDUP(str);
		    }
		}
	    }

	    else if (k_its ("Channel")) {
		if ( (str = k_str ()) ) {
		    if(params->statefile) {
			nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY,
				"Channels have been already defined inside channel state file!\n");
			return EW_FAILURE;
		    }
		    if(!params->channels) {
#define MAXSIZECHANNELSTRING 8000
			params->channels = (char *) NMXP_MEM_MALLOC(MAXSIZECHANNELSTRING);
			strncpy(params->channels, str, MAXSIZECHANNELSTRING);
		    } else {
			strncat(params->channels, ",", MAXSIZECHANNELSTRING - strlen(params->channels));
			strncat(params->channels, str, MAXSIZECHANNELSTRING - strlen(params->channels));
		    }
		}
	    }

	    else if (k_its ("mschan")) {
		if ( (str = k_str ()) ) {
		    sep = strstr(str, "/");
		    if(sep) {
			sep[0] = 0;
			sep++;
			params->usec = atoi(str) * 1000;
			params->n_channel = atoi(sep);
			nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_ANY,
				"Channels %d usec %d!\n", params->n_channel, params->usec);
		    } else {
			nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY,
				"Syntax error in parameter 'mschan' %s!\n", NMXP_LOG_STR(str));
			return EW_FAILURE;
		    }
		}
	    }

	    else if (k_its ("ChannelFile")) {
		if ( (str = k_str ()) ) {
		    params->statefile = (char *) NMXP_MEM_MALLOC(512 * sizeof(char));
		    strncpy(params->statefile, str, 512);
		    if(params->channels == NULL) {
			params->channels = get_channel_list_argument_from_state_file(params->statefile);
			if(params->channels) {
			    /* Do nothing */
			} else {
			    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY,
				    "State file %s not found or unable to read!\n", NMXP_LOG_STR(params->statefile));
			    return EW_FAILURE;
			}
		    } else {
			nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY,
				"Channels have been already defined inside main configuration file!\n");
			return EW_FAILURE;
		    }
		}
	    }


	    /* Unknown command */ 
	    else {
		logit("et", "%s: <%s> Unknown command in <%s>.\n", PACKAGE_NAME,
			com, configfile);
		continue;
	    }

	    /* See if there were any errors processing the command */
	    if (k_err ()) {
		logit("et", 
			"%s: Bad command in <%s>; exiting!\n\t%s\n", PACKAGE_NAME,
			configfile, k_com());
		return EW_FAILURE;
	    }

	} /** while k_rd() **/

	nfiles = k_close ();

    } /** while nfiles **/

    /* Check for required parameters */
    if ( myModName[0] == '\0' ) {
	logit("et", "%s: No MyModId parameter found in %s\n", PACKAGE_NAME,
		configfile);
	return EW_FAILURE;
    }
    if ( ringName[0] == '\0' ) {
	logit("et", "%s: No OutRing parameter found in %s\n", PACKAGE_NAME,
		configfile);
	return EW_FAILURE;
    }
    if ( heartbeatInt == -1 ) {
	logit("et", "%s: No HeartBeatInterval parameter found in %s\n", PACKAGE_NAME,
		configfile);
	return EW_FAILURE;
    }
    if ( logSwitch == -1 ) {
	logit("et", "%s: No LogFile parameter found in %s\n", PACKAGE_NAME,
		configfile);
	return EW_FAILURE;
    }

    if(params->statefile) {
	params->flag_buffered = 1;
	logit("et", "Enable buffer for requesting also recent packets into the past.\n");
    }

    return EW_SUCCESS;
}				/* End of nmxptool_ew_proc_configfile() */

/***************************************************************************
 * nmxptoole_ew_report_status():
 * Send error and hearbeat messages to transport ring.
 *
 ***************************************************************************/
void nmxptool_ew_report_status( MSG_LOGO * pLogo, short code, char * message ) {
    char          outMsg[MAXMESSAGELEN];  /* The outgoing message.        */
    time_t        msgTime;        /* Time of the message.                 */

    /*  Get the time of the message                                       */
    time( &msgTime );

    /* Build & process the message based on the type */
    if ( pLogo->type == typeHeartbeat ) {
	/* sprintf( outMsg, "%ld %ld\n\0", (long) msgTime, (long) myPid ); */
	sprintf( outMsg, "%ld %ld\n%c", (long) msgTime, (long) myPid, 0 );

	/* Write the message to the output region */
	if ( tport_putmsg( &regionOut, &hrtLogo, (long) strlen( outMsg ),
		    outMsg ) != PUT_OK ) {
	    /* Log an error message */
	    logit( "et", "%s: Failed to send a heartbeat message (%d).\n",
		    NMXP_LOG_STR(PACKAGE_NAME),
		    code );
	}
    } else {
	if ( message ) {
	    /* sprintf( outMsg, "%ld %hd %s\n\0", (long) msgTime, code, NMXP_LOG_STR(message) ); */
	    sprintf( outMsg, "%ld %hd %s\n%c", (long) msgTime, code, NMXP_LOG_STR(message), 0 );
	    logit("t","Error:%d (%s)\n", code, NMXP_LOG_STR(message) );
	} else {
	    /* sprintf( outMsg, "%ld %hd\n\0", (long) msgTime, code ); */
	    sprintf( outMsg, "%ld %hd\n%c", (long) msgTime, code, 0 );
	    logit("t","Error:%d (No description)\n", code );
	}

	/* Write the message to the output region  */
	if ( tport_putmsg( &regionOut, &errLogo, (long) strlen( outMsg ),
		    outMsg ) != PUT_OK ) {
	    /*     Log an error message                                    */
	    logit( "et", "%s: Failed to send an error message (%d).\n",
		    NMXP_LOG_STR(PACKAGE_NAME),
		    code );
	}

    }
}				/* End of nmxptool_ew_report_status() */


int nmxptool_ew_check_flag_terminate() {
    /* Check if we are being asked to terminate */
    return (tport_getflag (&regionOut) == TERMINATE || tport_getflag (&regionOut) == myPid );
}

void nmxptool_ew_send_heartbeat_if_needed() {
    /* Check if we need to send heartbeat message */
    if ( time( &timeNow ) - timeLastBeat >= heartbeatInt )
    {
	timeLastBeat = timeNow;
	nmxptool_ew_report_status ( &hrtLogo, 0, "" ); 
    }
}

void nmxptool_ew_send_error(unsigned int ierr, char *message, const char *hostname) {
    char complete_message[NMXPTOOL_EW_MAXSZE_MSG];
    int i;

    i=0;
    while(i < NMXPTOOL_EW_ERR_MAXVALUE && nmxptool_ew_err_msg[i].error != ierr) {
	i++;
    }
    if(i < NMXPTOOL_EW_ERR_MAXVALUE) {
	if(message) {
	    snprintf(complete_message, NMXPTOOL_EW_MAXSZE_MSG, "m%u - %s - %s %s.", myModId, hostname, nmxptool_ew_err_msg[i].message, message);
	} else {
	    snprintf(complete_message, NMXPTOOL_EW_MAXSZE_MSG, "m%u - %s - %s", myModId, hostname, nmxptool_ew_err_msg[i].message);
	}
	nmxptool_ew_report_status ( &errLogo, nmxptool_ew_err_msg[i].error, complete_message); 
    } else {
	nmxptool_ew_report_status ( &errLogo, 0, "Unknown error"); 
    }

}

/***************************************************************************
 * nmxptool_ew_logit_msg() and nmxptool_ew_logit_err():
 * 
 * Hooks for Earthworm logging facility.
 ***************************************************************************/
int nmxptool_ew_logit_msg (char *msg) {
  logit ("o",  NMXP_LOG_STR(msg));
  return 0;
}

int nmxptool_ew_logit_err (char *msg) {
  logit ("e",  NMXP_LOG_STR(msg));
  return 0;
}


#endif

