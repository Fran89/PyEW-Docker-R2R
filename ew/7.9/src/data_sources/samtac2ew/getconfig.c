/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: getconfig.c 3552 2009-01-22 16:47:20Z tim $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.14  2009/01/22 16:47:20  tim
 *     *** empty log message ***
 *
 *     Revision 1.13  2009/01/22 15:59:55  tim
 *     get rid of Network from config file
 *
 *     Revision 1.12  2009/01/21 16:17:28  tim
 *     cleaned up and adjusted data collection for minimal latency
 *
 *     Revision 1.11  2009/01/13 15:41:26  tim
 *     Removed more k2 references
 *
 *     Revision 1.10  2008/11/10 16:30:44  tim
 *     Cleaned up includes
 *
 *     Revision 1.9  2008/10/29 21:12:13  tim
 *     setup error logo
 *
 *     Revision 1.8  2008/10/29 15:39:12  tim
 *     Added support for Logos and Packet types
 *
 *     Revision 1.7  2008/10/23 21:00:03  tim
 *     Updating to use SCNL, and ewtrace
 *
 *     Revision 1.6  2008/10/21 20:01:50  tim
 *     *** empty log message ***
 *
 *     Revision 1.5  2008/10/21 15:28:55  tim
 *     get rid of unneeded config lines
 *
 *     Revision 1.4  2008/10/21 14:33:02  tim
 *     including them both now.... this is a pain
 *
 *     Revision 1.3  2008/10/21 14:31:59  tim
 *     change from externs.h to glbvars.h
 *
 *     Revision 1.2  2008/10/17 19:22:52  tim
 *     no longer including main.h
 *
 *     Revision 1.1  2008/10/15 20:53:52  tim
 *     *** empty log message ***
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <earthworm.h>       /* Earthworm main include file */
#include <kom.h>             /* command parcer functions (Earthworm util) */
#include "glbvars.h"         /* Global variables header */
#include "scnl_map.h"

void setuplogo(MSG_LOGO *);

     /***************************************************************
      *                          get_config()                       *
      *         Processes command file using kom.c functions.       *
      *           Returns -1 if any errors are encountered.         *
      ***************************************************************/

#define NCOMMAND 8             /*  Number of commands in the config file */
#define NREQ_CMDS 7            /*  Number of required commands in config file */
#define LOCATION_ID_LENGTH 3   /* Length of location name, plus terminal null */
static int parse_chnames_str(char *chnames_str);   /* function defined below */
static int parse_locnames_str(char *locnames_str); /* function defined below */
static int parse_invpol_str(char *invpol_str);     /* function defined below */

                        /* string of "ChannelNames" remapping names: */


int get_config( char *configfile )
{
	char     init[NCOMMAND];     /* Flags, one for each command */
	int      nmiss;              /* Number of commands that were missed */
	int      nfiles;
	int      i;
	char     *sptr,*sptr2;
	int      len;

	/* Set to zero one init flag for each required command
	***************************************************/
	for ( i = 0; i < NCOMMAND; i++ )
		init[i] = 0;

	/* Initialize the IO mode to `none' */
	gen_io.mode = IO_NUM_MODES;

	/* Open the main configuration file
	********************************/
	nfiles = k_open( configfile );
	if ( nfiles == 0 )
	{
		fprintf(stderr, "%s: Error opening configuration file <%s>\n",
			g_progname_str, configfile );
		return -1;
	}

	/* Process all nested configuration files
	**************************************/
	while ( nfiles > 0 )          /* While there are config files open */
	{
		while ( k_rd() )           /* Read next line from active file  */
		{
			int  success;
			char *com;
			char *str;

			com = k_str();          /* Get the first token from line */

			if ( !com ) continue;             /* Ignore blank lines */
			if ( com[0] == '#' ) continue;    /* Ignore comments */

			/* Open another configuration file
			*******************************/
			if ( com[0] == '@' )
			{
				success = nfiles + 1;
				nfiles  = k_open( &com[1] );
				if ( nfiles != success )
				{
					fprintf(stderr, "%s: Error opening command file <%s>.\n",
						g_progname_str, &com[1] );
					return -1;
				}
				continue;
			}

			/* Read configuration parameters
			*****************************/
			if ( k_its( "ComPort" ) )
			{
				if ( gen_io.mode != IO_NUM_MODES)
				{
					fprintf(stderr, "%s: Too many comm modes used; second is %s\n",
						g_progname_str, "ComPort");
					return -1;
				}
				gen_io.mode = IO_COM_NT;
				if((gen_io.com_io.commsel=k_int()) < 1 ||
					gen_io.com_io.commsel > 255)
				{      /* COM port value out of range; display error message */
					fprintf(stderr,"%s: ComPort value (%d) out of range\n",
						g_progname_str,gen_io.com_io.commsel);
					return -1;          /* return error code */
				}
				init[0] = 1;
			}

			else if ( k_its( "TcpAddr" ) )
			{
				if ( gen_io.mode != IO_NUM_MODES)
				{
					fprintf(stderr, "%s: Too many comm modes used; second is %s\n",
						g_progname_str, "TcpAddr");
					return -1;
				}
				gen_io.mode = IO_TCP;
				if ( (str = k_str()) != 0 )
				{
					if (strlen(str) > KC_MAX_ADDR_LEN - 1)
					{
						fprintf(stderr, "%s: TcpAddr too long; max is %d\n",
							g_progname_str, KC_MAX_ADDR_LEN - 1);
						return -1;
					}
					strcpy(gen_io.tcp_io.samtac_address, str);
					init[0] = 1;
				}
				/* Null string; error picked up later */
			}

			else if ( k_its( "TtyName" ) )
			{
				if ( gen_io.mode != IO_NUM_MODES)
				{
					fprintf(stderr, "%s: Too many comm modes used; second is %s\n",
						g_progname_str, "TtyName");
					return -1;
				}
				gen_io.mode = IO_TTY_UN;
				if ( (str = k_str()) != 0 )
				{
					if (strlen(str) >= KC_MAX_TTY_LEN - 1)
					{
						fprintf(stderr, "%s: TtyName too long; max is %d\n",
							g_progname_str, KC_MAX_TTY_LEN - 1);
						return -1;
					}
					strcpy(gen_io.tty_io.ttyname, str);
					init[0] = 1;
				}
				/* Null string; error picked up later */
			}

			else if ( k_its( "Speed" ) )
			{
				if (gen_io.mode != IO_COM_NT && gen_io.mode != IO_TTY_UN)
				{
					fprintf(stderr, "%s: %s specifed but serial mode comms not set\n",
						g_progname_str, "Speed");
					return -1;
				}
				if (gen_io.mode == IO_COM_NT)
				{
					if((gen_io.com_io.speed=k_long()) < 150L ||
						gen_io.com_io.speed > 10000000L)
					{      /* baud rate value out of range; display error message */
						fprintf(stderr,"%s: Speed value (%ld) out of range\n",
							g_progname_str, gen_io.com_io.speed);
						return -1;          /* return error code */
					}
					init[1] = 1;
				}
				else
				{   /* mode == IO_TTY_UN */
					if((gen_io.tty_io.speed=k_long()) < 150L ||
						gen_io.tty_io.speed > 10000000L)
					{      /* baud rate value out of range; display error message */
						fprintf(stderr,"%s: Speed value (%ld) out of range\n",
							g_progname_str, gen_io.tty_io.speed);
						return -1;          /* return error code */
					}
					init[1] = 1;
				}
			}

			else if ( k_its( "TcpPort" ) )
			{
				if (gen_io.mode != IO_TCP)
				{
					fprintf(stderr, "%s: %s specifed but TCP mode comms not set\n",
						g_progname_str, "TcpPort");
					return -1;
				}
				if((gen_io.tcp_io.samtac_port=k_long()) < 1024L ||
					gen_io.tcp_io.samtac_port > 100000L)
				{      /* tcp portvalue out of range; display error message */
					fprintf(stderr,"%s: TcpPort value (%ld) out of range\n",
						g_progname_str, gen_io.tcp_io.samtac_port);
					return -1;          /* return error code */
				}
				init[1] = 1;
			}
			else if ( k_its( "DeviceID" ) )
			{
				if((gcfg_deviceID=k_int()) > 65535){	//More than two bytes
					//Error
					fprintf(stderr,"%s: DeviceID value (%d) out of range\n",
						g_progname_str, gcfg_deviceID);
					return -1;          /* return error code */
				} else {
					device_id[0]= ((gcfg_deviceID >> 8) & 0xff);
					device_id[1]= (gcfg_deviceID & 0xff);
					//fprintf(stderr,"deviceID: %d, %x %x\n", gcfg_deviceID, device_id[0], device_id[1]);
				}
				init[6] = 1;
			}

			else if ( k_its( "ModuleId" ) )
			{
				if ( (str = k_str()) != 0 )
				{
					/* copy module name; make sure NULL terminated */
					strncpy(gcfg_module_name,str,sizeof(gcfg_module_name)-1);
					gcfg_module_name[sizeof(gcfg_module_name)-1] = '\0';
					if ( GetModId(str, &gcfg_module_idnum) == -1 )
					{
						fprintf( stderr, "%s: Invalid ModuleId <%s>. \n",
							g_progname_str, str );
						fprintf( stderr,
							"%s: Please Register ModuleId <%s> in earthworm.d!\n",
							g_progname_str, str );
						return -1;
					}
				}
				init[2] = 1;
			}

			else if ( k_its( "RingName" ) )
			{
				if ( (str = k_str()) != NULL )
				{
					/* copy ring name; make sure NULL terminated */
					strncpy(gcfg_ring_name,str,sizeof(gcfg_ring_name)-1);
					gcfg_ring_name[sizeof(gcfg_ring_name)-1] = '\0';
					if ( (gcfg_ring_key = GetKey(str)) == -1 )
					{
						fprintf( stderr, "%s: Invalid RingName <%s>. \n",
							g_progname_str, str );
						return -1;
					}
				}
				init[3] = 1;
			}

			else if ( k_its( "HeartbeatInt" ) )
			{
				gcfg_heartbeat_itvl = k_int();
				init[4] = 1;
			}

			else if ( k_its( "LogFile" ) )
			{
				gcfg_logfile_flgval = k_int();
				init[5] = 1;
			}

			/* Optional Commands */

			else if (k_its( "CommTimeout" ) )
			{
				gcfg_commtimeout_itvl = k_int();
			}
			
			else if (k_its( "SOH_int" ) )
			{
				SOH_itvl = (double)k_int();
			}


			else if (k_its( "Debug" ) )
			{
				gcfg_debug = k_int();
			}
			//INFOSCNL

			else if ( k_its( "InfoSCNL") ) 
			{
				char *Strm, *Sys, *S,*C,*N, *L;
//				UseTraceBuf2 = 1;
				Sys=k_str();
				Strm=k_str();
				S=k_str();
				C=k_str();
				N=k_str();
				L=k_str();
				if (insertSCNL(Sys, Strm, S,C,N,L) == -1) 
				{
					fprintf( stderr, "SAMTAC2EW: Invalid InfoSCNL entry <%s %s %s %s %s>.\n", Sys, Strm, S,C,N);
					fprintf( stderr, "Follow the SEED header definitions!\n");
				}
			}

			else
			{
				/* An unknown parameter was encountered */
				fprintf( stderr, "%s: <%s> unknown parameter in <%s>\n",
					g_progname_str,com, configfile );
				return -1;
			}

			/* See if there were any errors processing the command
			***************************************************/
			if ( k_err() )
			{
				fprintf( stderr, "%s: Bad <%s> command in <%s>.\n",
					g_progname_str, com, configfile );
				return -1;
			}
		}
		nfiles = k_close();
	}

	/* After all files are closed, check flags for missed required commands
	***********************************************************/
	nmiss = 0;
	for ( i = 0; i < NREQ_CMDS; i++ )
		if ( !init[i] )
			nmiss++;

	if ( nmiss > 0 )
	{
		fprintf( stderr,"%s: ERROR, no ", g_progname_str );
		if ( !init[0] ) fprintf(stderr, "<ComPort>, <TcpAddr> or <TtyName> " );
		if ( !init[1] ) fprintf(stderr, "<Speed> or <TcpPort>" );
		if ( !init[2] ) fprintf(stderr, "<ModuleId> " );
		if ( !init[3] ) fprintf(stderr, "<RingName> " );
		if ( !init[4] ) fprintf(stderr, "<HeartbeatInt> " );
		if ( !init[5] ) fprintf(stderr, "<LogFile> " );
		if ( !init[6] ) fprintf(stderr, "<DeviceID> " );
		//if ( !init[6] ) fprintf(stderr, "<LCFlag> " );
		fprintf(stderr, "command(s) in <%s>.\n", configfile );
		return -1;
	}
	if ( GetType( "TYPE_HEARTBEAT", &g_heart_ltype ) != 0 ) {
		logit("et", "samtac2ew: Invalid message type <TYPE_HEARTBEAT>\n");
		return( -1 );
	}
	if ( GetType( "TYPE_TRACEBUF", &TypeTrace ) != 0 ) {
		logit("et", "samtac2ew: Invalid message type <TYPE_TRACEBUF>; exiting!\n");
		return(-1);
	}
	if ( GetType( "TYPE_TRACEBUF2", &TypeTrace2 ) != 0 ) {
		logit("et", "samtac2ew: Message type <TYPE_TRACEBUF2> not found in earthworm_global.d; exiting!\n");
		return(-1);
	}
	if ( GetType( "TYPE_ERROR", &TypeErr ) != 0 ) {
		logit("et", "samtac2ew: Invalid message type <TYPE_ERROR>\n");
		return( -1 );
	}
	if ( GetType( "TYPE_SAMTACSOH_PACKET", &TypeSAMTACSOH ) != 0 ) {
		logit("et", "samtac2ew: Message type <TYPE_SAMTACSOH_PACKET> not found in earthworm.d, please add it if you will use InjectSOH.\n");
		return( -1 );
	} else {
		SOHLogo.type=TypeSAMTACSOH;
		setuplogo(&SOHLogo);
	}

	/* build the datalogo */
	setuplogo(&DataLogo);
	DataLogo.type=TypeTrace2;
	
	/* build heartbeat logo */
	setuplogo(&g_heartbeat_logo);
	g_heartbeat_logo.type=g_heart_ltype;

	setuplogo(&g_error_logo);
	g_error_logo.type=TypeErr;

	return 0;
}
