/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *   
 *    $Id: samtac2ew_main.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.5  2009/01/22 16:47:20  tim
 *     *** empty log message ***
 *
 *     Revision 1.4  2009/01/22 15:59:55  tim
 *     get rid of Network from config file
 *
 *     Revision 1.3  2009/01/21 16:17:28  tim
 *     cleaned up and adjusted data collection for minimal latency
 *
 *     Revision 1.2  2009/01/15 22:09:51  tim
 *     Clean up
 *
 *     Revision 1.1  2009/01/15 17:33:22  tim
 *     Dies gracefully when startstop tells it to, and rename main.tmp.c to samtac2ew_main.c
 *
 *     Revision 1.37  2009/01/14 22:06:11  tim
 *     Works for other sample rates now
 *
 *     Revision 1.36  2009/01/13 17:12:53  tim
 *     Clean up source
 *
 *     Revision 1.35  2009/01/13 15:41:27  tim
 *     Removed more k2 references
 *
 *     Revision 1.34  2009/01/12 20:52:32  tim
 *     Removing K2 references
 *
 *     Revision 1.33  2009/01/08 15:05:31  tim
 *     Integrated the buffer and tested.  works
 *
 *     Revision 1.32  2009/01/06 22:16:17  tim
 *     Adding buffer and dealing with new firmware
 *
 *     Revision 1.31  2008/11/10 16:31:16  tim
 *     Improved error handling and cleaned up
 *
 *     Revision 1.30  2008/10/29 21:12:54  tim
 *     error handling
 *
 *     Revision 1.29  2008/10/29 15:55:47  tim
 *     included transport.h, convert.h, time_ew.h
 *     added global vars for Logo, typetrace, SOH, myPid
 *     added support to convert the time they gave me to epochs
 *     Added SCNL support
 *     Added sequence tracking support
 *     Moved heartbeat to above socket opening
 *     updated checksum to match the version of SAMTAC data they have, not what is written in the doc or in their simulator
 *     Added SOH support
 *
 *     Revision 1.28  2008/10/24 02:39:33  tim
 *     Cleaned up code
 *
 *     Revision 1.27  2008/10/23 21:05:24  tim
 *     *** empty log message ***
 *
 *     Revision 1.26  2008/10/23 21:00:03  tim
 *     Updating to use SCNL, and ewtrace
 *
 *     Revision 1.25  2008/10/22 21:29:10  tim
 *     Read data from the simulator, log header info, and start transforming payload to be put into EW
 *
 *     Revision 1.24  2008/10/22 03:13:40  tim
 *     added code from q2ew, now need to get all the variables and such I need, and convert their names to ones I am using.
 *
 *     Revision 1.23  2008/10/21 22:52:51  tim
 *     *** empty log message ***
 *
 *     Revision 1.22  2008/10/21 21:11:00  tim
 *     Adding tport_Attach before heartbeat starts
 *
 *     Revision 1.21  2008/10/21 21:09:43  tim
 *     dealing with segfault in heartbeat function
 *
 *     Revision 1.20  2008/10/21 19:31:59  tim
 *     *** empty log message ***
 *
 *     Revision 1.19  2008/10/21 19:30:48  tim
 *     add debugging
 *
 *     Revision 1.18  2008/10/21 19:28:14  tim
 *     debugging for heartbeat
 *
 *     Revision 1.17  2008/10/21 19:14:58  tim
 *     init logit
 *
 *     Revision 1.16  2008/10/21 19:13:50  tim
 *     getting to compile and link
 *
 *     Revision 1.15  2008/10/21 14:19:45  tim
 *     terminat.h
 *
 *     Revision 1.14  2008/10/21 14:17:41  tim
 *     k2ewerrs.h
 *
 *     Revision 1.13  2008/10/21 14:14:07  tim
 *     define rc, dependancies
 *
 *     Revision 1.12  2008/10/21 14:11:56  tim
 *     included earthworm.h
 *
 *     Revision 1.11  2008/10/21 02:04:15  tim
 *     added code using k2comif.c
 *
 *     Revision 1.10  2008/10/17 20:24:37  tim
 *     allowed for VC 2008 not supporting C99 standard, moved a variable decl up to top of main function
 *
 *     Revision 1.9  2008/10/17 20:17:45  tim
 *     Cleaning up unused variables in glbvars.h
 *
 *     Revision 1.8  2008/10/17 20:03:12  tim
 *     declare signal handler function at top of file
 *
 *     Revision 1.7  2008/10/17 19:22:52  tim
 *     no longer including main.h
 *
 *     Revision 1.6  2008/10/17 18:55:41  tim
 *     editing the RCS banner
 *
 *
 */


#define MAIN
#include "heartbt.h"
#include <time.h>
#include <signal.h>
#include "glbvars.h"
#include "getconfig.h"
#include <earthworm.h>
#include "samtac_comif.h"
#include "samtac2ew_errs.h"
#include "terminat.h"
#include <trace_buf.h>
#include "scnl_map.h"
#include "convert.h"
#include "transport.h"
#include <time_ew.h>
#include "samtac2ew_buffer.h"

/***************************************************
 * define signal handling function
 ***************************************************/

void initiate_termination(int signum);

/***************************************************
 * define checksum computing function
 ***************************************************/

int compute_checksum(char *buffer, int total_length);

/***************************************************
 * define time interval function
 ***************************************************/

int time_itvl(time_t *last_time, double itvl);

/***************************************************
 * define globals
 ***************************************************/
#define SAMTAC2EW_VERSION_STR "0.02"
//#define PACKET_MAX_SIZE  13519      // 17(header) + 500(max hz) * 9(max channels) * 3(bytes per samples) + 2(footer)
//#define PACKET_MAX_SIZE  1819 		//Defined in glbvars.h
#define SAMTAC_IBFSIZ (PACKET_MAX_SIZE)

unsigned char g_samtac_buff[PACKET_MAX_SIZE]; /* received data buffer */
unsigned char samtac_initial_buffer[PACKET_MAX_SIZE * 3]; /* received data buffer */

static int samtac2ew_signal_val=-1;    /* initialize value from signal function */
char g_progname_str[]="SAMTAC2EW";     /* global program name string */
int g_terminate_flg = 0;          /* = 1 to initiate program termination */
s_gen_io gen_io;                       /* general IO params structure */
int gcfg_heartbeat_itvl = 30;          /* heartbeat interval (secs) */
int gcfg_commtimeout_itvl = 1000;      /* communication timeout (millisecs) */
unsigned char g_heart_ltype = 0;  /* logo type for heartbeat msgs */
MSG_LOGO g_heartbeat_logo;        /* Transport logo for heartbeat messages */
SHM_INFO g_tport_region;          /* transport region struct for Earthworm */
MSG_LOGO g_error_logo;            /* Transport logo for error messages */
int gcfg_debug;
char gcfg_ring_name[MAXNAMELEN+2];   /* name of ring buffer */
long gcfg_ring_key;             /* key to ring buffer samtac2ew dumps data */
int gcfg_logfile_flgval;        /* output to logfile enable flag */
char gcfg_module_name[MAXNAMELEN+2]; /* module name for samtac2ew */
unsigned char gcfg_module_idnum;     /* module id for samtac2ew */
MSG_LOGO DataLogo;               /* EW logo tag  for data */
MSG_LOGO SOHLogo;                /* EW logo tag  for TYPE_GCFSOH_PACKET */
unsigned char TypeTrace;         /* Trace EW type for logo */
unsigned char TypeTrace2;        /* Trace2  EW type for logo */
//unsigned char TypeHB;            /* HB=HeartBeat EW type for logo */
unsigned char TypeErr;           /* Error EW type for logo */
unsigned char TypeSAMTACSOH;        /* EW TYPE_GCFSOH_PACKET type for logo */
pid_t MyPid;
unsigned int gcfg_deviceID;		//DeviceID(serial) as read from config
char device_id[2];
int last_found_packetstart;
double SOH_itvl = 300;		//5 minutes is default(300 seconds)


int
main(int argc, char **argv)
{
/***************************************************
 * VC 2008 does not support C99 standard, so 
 * moving all variable declarations up here
 ***************************************************/
	time_t last_hb_time;
	time_t last_soh_time;
	struct tm dt = { 0,};
	static char msg_txt[180];
	//int local_timeout_ms = gcfg_commtimeout_itvl; //From k2misc.c
	int dcnt, rc;
	char *ew_trace_buf;			/* earthworm trace buffer pointer */
	long ew_trace_len;
	//TracePacket trace_buffer;	/* Tracebuffer to be used below */
	int i;	/* iterator */
	//unsigned int convert_to_int;
	SCN *scnl;				/* an scnl struct pointer  returned by getSCN() */
	char systemID[6];		/* max value for system id will be numeric 0xffff, 
							need to itoa to max length of ascii is 6(65535 plus null byte) */
	char streamID[2];
	char SOH_data[256];		/*State of health data buffer*/
	int SOH_data_location = 0;	/* used when writing all data to SOH buffer */
	int last_seq_num = 0;
	int first_run = 1;
	queue *packet_buffer = QueueCreate(PACKET_MAX_SIZE * 3);
	char past_SOH = '\0';
	
/***************************************************
 * setting up
 ***************************************************/
	//Read Config
	MyPid = getpid();
	
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <configfile>\n", argv[0]);
		exit (EW_FAILURE);
	}
	if (get_config(argv[1]) == -1)  /* process configuration file */
	{
		fprintf(stderr, "samtac2ew: error processing config file <%s>\n", argv[1]);
		return 1;    /* if error then exit program (error reported in function) */
	}

	//Start logging
	logit_init(argv[1],(short)gcfg_module_idnum, 1024, gcfg_logfile_flgval);
	logit("et","SAMTAC-to-Earthworm Module, Version %s\n",SAMTAC2EW_VERSION_STR);
	logit("et","with SCNL and TRACEBUF2 messages\n");
	logit("et","Processed configuration file <%s>\n",argv[1]);
	
	/* attach to Earthworm shared memory ring buffer */
	/*  (fn will display error message and abort if error) */
	tport_attach(&g_tport_region,gcfg_ring_key);
	if (gcfg_debug > 2)
	{
		logit("et","Attached to ring key:  %ld\n",gcfg_ring_key);
	}
	
	//Capture Signals
	signal(SIGINT, initiate_termination);
	signal(SIGTERM, initiate_termination);
	
	last_soh_time = 0;
	
	//Start Heartbeat
	last_hb_time = 0;
	if (gcfg_debug > 2)
	{
		logit("et", "samtac2ew: starting heart beat\n");
	}

	if(beat_heart(&last_hb_time) == 0) {
		//TODO: we did not beat the heart the first time.
		logit("et", "samtac2ew I did not beat my heart the first time I tried\n");
	} else {
		//TODO: heart beat just fine
		if (gcfg_debug > 2)
		{
			logit("et", "samtac2ew I beat my heart the first time I tried\n");
		}
	}

	//open tcp socket of serial port

	/* Initialize IO to the SAMTAC: sockets or serial comms */
	if ( (rc = samtac_init_io( &gen_io, gcfg_commtimeout_itvl )) != SAMTAC2R_NO_ERROR)
	{      /* comms initialization failed */
		sprintf (msg_txt, "Unable to initialize IO port: %s",argv[1]);
		if (gcfg_debug > 3) 
		{
			logit("et", "samtac2ew: %s\n", msg_txt);
		}
		samtac2ew_enter_exitmsg(SAMTACTERM_SAMTAC_STARTUP, msg_txt);
		samtac2ew_exit(0);                  /* exit program with code value */
	}
	
	
	//TODO 	acquisition
	while (!g_terminate_flg)
	{
		if(beat_heart(&last_hb_time)) { if (gcfg_debug > 2) { logit("et", "samtac2ew_main: last_hb_time is:%d\n", last_hb_time); }; }; 
		//read n bytes from open descriptor
				//g_handle is used by k2c_ser_nt.c
				//sfd is used by k2c_tcp.c
				//looks like I may want to use samtac_recv_buff() to pull in data strea

		//if ( (dcnt = samtac_recv_buff(g_samtac_buff, SAMTAC_IBFSIZ,
        //                         local_timeout_ms, 0)) > 0)
		//{
		//device_id[0]=0x00;
		//device_id[1]=0x15;
		dcnt = Queue_packetStart(packet_buffer, device_id);
		//printf("size: %d\n", dcnt);
		QueueExportBuffer(packet_buffer, (char *)g_samtac_buff, dcnt);
			//Data was received 
			//g_samtac_buff[dcnt] = '\0';           /* NULL terminate buffer */

			if (gcfg_debug > 2)
			{
				logit("et", "received:  %d bytes '%x'\n", dcnt, g_samtac_buff);
				logit("e", "STX:  %02X\n", g_samtac_buff[0]);
				logit("e", "Sequence #:  %d\n", g_samtac_buff[1]);
				logit("e", "Block Size:  %02X%02X\n", g_samtac_buff[2], g_samtac_buff[3]);
				logit("e", "Header Size:  %02X\n", g_samtac_buff[4]);
				logit("e", "Status:  %02X\n", g_samtac_buff[5]);
				logit("e", "Device #:  %02X%02X\n", g_samtac_buff[6], g_samtac_buff[7]);
				logit("e", "Channels:  %d\n", g_samtac_buff[8]);
				logit("e", "Sample Rate:  %02X%02X\n", g_samtac_buff[9], g_samtac_buff[10]);
				logit("e", "Year:  %d\n", g_samtac_buff[11]);
				logit("e", "Month:  %d\n", g_samtac_buff[12]);
				logit("e", "Day:  %d\n", g_samtac_buff[13]);
				logit("e", "Hour:  %d\n", g_samtac_buff[14]);
				logit("e", "Minute:  %d\n", g_samtac_buff[15]);
				logit("e", "Second:  %d\n", g_samtac_buff[16]);
				logit("e", "checksum:  %02X\n", g_samtac_buff[dcnt-2]);
				logit("e", "ETX:  %02x\n", g_samtac_buff[dcnt-1]);
			}

			if(!first_run) {	//Don't do this on the first run
				//if sequence #s aren't sequential throw error
				if(last_seq_num == 255) {
					if (g_samtac_buff[1] != 0) {
						//throw ERROR
						sprintf (msg_txt, "packet sequence numbers out of sequence. Prev: %d, Current: %d", 
								last_seq_num, g_samtac_buff[1]);
						samtac2ew_enter_exitmsg(SAMTACTERM_SAMTAC_COMMERR, msg_txt);
						samtac2ew_throw_error();
						//logit("et", "samtac2ew: packet sequence numbers out of sequence. Prev: %d, Current: %d\n", 
						//		last_seq_num, g_samtac_buff[1]);
					}
				} else {
					if (last_seq_num + 1 != g_samtac_buff[1]) {
						//throw ERROR
						sprintf (msg_txt, "packet sequence numbers out of sequence. Prev: %d, Current: %d", 
								last_seq_num, g_samtac_buff[1]);
						samtac2ew_enter_exitmsg(SAMTACTERM_SAMTAC_COMMERR, msg_txt);
						samtac2ew_throw_error();
						//logit("et", "samtac2ew: packet sequence numbers out of sequence. Prev: %d, Current: %d\n", 
						//		last_seq_num, g_samtac_buff[1]);
					}
				}
			}

			//Compute checksum and compare with passed checksum
			if(compute_checksum((char *)g_samtac_buff, dcnt) != (int) g_samtac_buff[dcnt-2])
			{
				//Checksums did not match
				//throw error
				sprintf (msg_txt, "Error reading SAMTAC packet, checksums did not match");
				samtac2ew_enter_exitmsg(SAMTACTERM_SAMTAC_COMMERR, msg_txt);
				samtac2ew_throw_error();
				//logit("et", "samtac2ew_main: error reading SAMTAC packet, checksums did not match\n");
				if (gcfg_debug > 2) 
				{
					logit("e", "computed checksum:  %02X\n", (char)compute_checksum((char *)g_samtac_buff, dcnt) & 0xff);
				}
			} else {
				//Checksums did match
				if (gcfg_debug > 3)
				{
					logit("e", "computed checksum:  %02X\n", (char)compute_checksum((char *)g_samtac_buff, dcnt) & 0xff);
				}
			}
		/*
		}
		else
		{
			//if (gcfg_debug>2) { logit("et", "samtac2ew_main: received:  %d bytes  after timeout of %d ms'\n", dcnt, local_timeout_ms); }
			sprintf (msg_txt, "Received:  %d bytes  after timeout of %d ms'\n", dcnt, local_timeout_ms);
			//logit("et", "samtac2ew: %s\n", msg_txt);
			samtac2ew_enter_exitmsg(SAMTACTERM_SAMTAC_COMMERR, msg_txt);
			samtac2ew_exit(0);                  /* exit program with code value *//*

			//exit(dcnt);
		}
		*/
		// create systemID
		sprintf(systemID, "%d", (device_id[0] << 8) + device_id[1]);
		//SOH
		
		//TODO FIXME if something has changed in SOH or if it has been 5 minutes or more since last SOH
		if (time_itvl(&last_soh_time, SOH_itvl) || (past_SOH & 0xff) != g_samtac_buff[5]) {
			if (gcfg_debug > 3) { printf("past_SOH: %x, current_SOH: %x\n", past_SOH, g_samtac_buff[5]);}
			if (getFirstSCN(systemID, &scnl)) {
				char soh_msg[2048];
				int soh_len;
				SOH_data_location = 0;
				past_SOH = g_samtac_buff[5];
				//Deal with possible statuses
				//report SOH(state of health)
				SOH_data_location += sprintf(SOH_data, "DeviceID: %s\n", systemID);
				if(g_samtac_buff[5] & 0x01) { /* Detecting Earthquake */ 
					SOH_data_location += sprintf(&SOH_data[SOH_data_location], "Detecting Earthquake: 1\n" );
				} else { /* Stand-by */ 
					SOH_data_location += sprintf(&SOH_data[SOH_data_location], "Detecting Earthquake: 0\n" );
				}
				if(g_samtac_buff[5] & 0x08) { /* Alarm for the Capacity of the Record Media */ 
					SOH_data_location += sprintf(&SOH_data[SOH_data_location], "Media Capacity Alarm: 1\n" );
					//error that the media capacity is alarming!
					sprintf (msg_txt, "Media Capacity Alarm");
					samtac2ew_enter_exitmsg(SAMTACSTAT_LOW_DISK, msg_txt);
					samtac2ew_throw_error();
				} else { /* Capacity is normal */ 
					SOH_data_location += sprintf(&SOH_data[SOH_data_location], "Media Capacity Alarm: 0\n" );
				}
				if(g_samtac_buff[5] & 0x20) { /* Alarm for the Voltage of Power Supply */ 
					SOH_data_location += sprintf(&SOH_data[SOH_data_location], "Power Supply Voltage Alarm: 1\n" );
					//error that the power supply voltage is alarming!
					sprintf (msg_txt, "Power Supply Voltage Alarm");
					samtac2ew_enter_exitmsg(SAMTACSTAT_VOLTAGE_ALARM, msg_txt);
					samtac2ew_throw_error();
				} else { /* Voltage is normal */ 
					SOH_data_location += sprintf(&SOH_data[SOH_data_location], "Power Supply Voltage Alarm: 0\n" );
				}
				if(g_samtac_buff[5] & 0x80) { /* GPS receive trouble */ 
					SOH_data_location += sprintf(&SOH_data[SOH_data_location], "GPS Alarm: 1\n" );
					//error that the GPS signal is alarming!
					sprintf (msg_txt, "GPS Signal Alarm");
					samtac2ew_enter_exitmsg(SAMTACSTAT_GPSLOCK, msg_txt);
					samtac2ew_throw_error();
				} else { /* GPS receive normal */ 
					SOH_data_location += sprintf(&SOH_data[SOH_data_location], "GPS Alarm: 0\n" );
				}
				
				if (gcfg_debug > 2)
				{
					//printf("location:%d\n", SOH_data_location);

					logit("et", "InjectSOH: SOH processing for System_id %s as %s-%s\n", 
							systemID, scnl->net, scnl->site);
				}
				
				dt.tm_year = g_samtac_buff[11] + 100;
				dt.tm_mon = g_samtac_buff[12] - 1;
				dt.tm_mday = g_samtac_buff[13];
				dt.tm_hour = g_samtac_buff[14];
				dt.tm_min = g_samtac_buff[15];
				dt.tm_sec = g_samtac_buff[16];
	//			logit("et", "mktime results: %d\n", timegm_ew( &dt ));
	//			logit("e", "Year:  %d\n", dt.tm_year);
	//			logit("e", "Month:  %d\n", dt.tm_mon);
	//			logit("e", "Day:  %d\n", dt.tm_mday);
	//			logit("e", "Hour:  %d\n", dt.tm_hour);
	//			logit("e", "Minute:  %d\n", dt.tm_min);
	//			logit("e", "Second:  %d\n", dt.tm_sec);
				
				sprintf(soh_msg, "%s-%s %ld\n%s", scnl->net, scnl->site, (long)timegm_ew( &dt )
							/*gepoch2uepoch(&(hdr.epoch))*/, SOH_data);
				soh_len = (int)strlen(soh_msg);
				if ( tport_putmsg(&g_tport_region, &SOHLogo, (long) soh_len, soh_msg) != PUT_OK) {
					logit("et", "samtac2ew: Fatal Error sending SOH via tport_putmsg()\n");
					g_terminate_flg = TRUE;
					//err_exit = GCF2EW_DEATH_EW_PUTMSG;
					continue;
				 } 
				if (gcfg_debug>2) {
					logit("et", "InjectSOH: SOH sent for System_id %s as %s-%s\n%s\n", 
							systemID, scnl->net, scnl->site, soh_msg);

				}
			} else {
				logit("et", "InjectSOH: SCN not found for System_id %s\n", systemID);
			}
		}
			
		//Check to make sure number of channels is not greater than 9(as defined in the specs)
		//Step through all channels and put them into earthworm
		for(i=1;i<=g_samtac_buff[8] && i<=9; i++) {
			//Create SCNL
			
			/* see if this is an SCN we actually want */
			//define stream_id
			sprintf(streamID, "%d", i);
			if (getSCN(systemID, streamID, &scnl) != 1) {
				/* we don't want this packet */
				/* log it if in verbose mode */
				if(gcfg_debug>2) {
					logit("et", "Discarding packet\n");
						//gcfhdr_print(&hdr, stdout);
				}
				continue;
			}
			if(gcfg_debug>2) {
				logit("et", "Processing packet\n");
				//gcfhdr_print(&hdr, stdout);
				// if (UseTraceBuf2)  {
					// logit("et", " converting to SCNL %s.%s.%s.%s \n", scnl->site, scnl->chan, scnl->net, scnl->loc);
				// } else {
					// logit("et", " converting to SCN %s.%s.%s \n",  scnl->site, scnl->chan, scnl->net);
				// }
			}
			if ((ew_trace_buf = convert_samtac_to_ewtrace((char *)g_samtac_buff, scnl, &ew_trace_len, i)) == NULL) {
				/* an error has occurred in the conversion */
				logit("et", "samtac2ew: Error converting gcf trace to ew packet in convert_gcf_to_ewtrace()\n");
			} else {
			    /* transport it off to EW */
				//If I decide to use this, I need to define TSLastGCFData
				//time(&TSLastGCFData); /* set the time since the last GCF data packet sent to Earthworm */
				if ( tport_putmsg(&g_tport_region, &DataLogo, ew_trace_len, ew_trace_buf) != PUT_OK) {
					logit("et", "samtac2ew: Fatal Error sending trace via tport_putmsg()\n");
					g_terminate_flg = TRUE;
					//err_exit = GCF2EW_DEATH_EW_PUTMSG;
				}
			}
		}
		//set last_seq_num to current sequence number
		last_seq_num = g_samtac_buff[1];
		if(first_run) { first_run = 0; };
		
		/* EARTHWORM see if we are being told to stop */
		if ( tport_getflag( &g_tport_region ) == TERMINATE || tport_getflag( &g_tport_region ) == MyPid)
		{
			sprintf (msg_txt, "Earthworm TERMINATE request");
			samtac2ew_enter_exitmsg(SAMTACTERM_EW_TERM, msg_txt);
			samtac2ew_exit(0);
	    }

	}
	//TODO: no longer reading
	//TODO: die()
	if(samtac2ew_signal_val == SIGINT){
		sprintf (msg_txt, "Recieved SIGINT");
		samtac2ew_enter_exitmsg(SAMTACTERM_SIG_TRAP, msg_txt);
	}
	samtac2ew_exit(0);
}


/***************************************************
 *
 * initiate_termination:  function to handle "signal" events
 *         signum - ID value of signal that caused event
 *
 ***************************************************/

void 
initiate_termination(int signum)
{
  signal(signum,initiate_termination);     /* call fn to reset signal handling */
  samtac2ew_signal_val = signum;            /* save signal ID value */
  g_terminate_flg = 1;                 /* set flag to terminate program */
}

/***************************************************
 *
 * compute_checksum:  function to compute checksum on SAMTAC packets
 *         buffer: address of buffer containing SAMTAC packet
 *         total_length: size of SAMTAC packet contained in buffer
 *    returns checksum
 *
 ***************************************************/


int
compute_checksum(char *buffer, int total_length) {
	unsigned int checksum = 0;
	int i;
	for (i=1; i < (total_length - 2); i++) {
		checksum += buffer[i];
		checksum = checksum & 0xff;
	}
	return checksum;
}

/***************************************************
 *
 * time_itvl:  function to compute if a time has passed the time interval
 * 		is passed the last time and the interval
 *	returns 1 if the time is outside the interval, 0 if inside
 *
 ***************************************************/


int
time_itvl(time_t *last_time, double itvl)
{
	static time_t timevar = 0;
	
	/* get current system time */
	time(&timevar);

	// if it is time, beat the heart
	if (difftime(timevar,*last_time) >= itvl) 
	{
		*last_time = timevar;       /* save time for last true assessment */
		return 1;
	}
	return 0;
}
