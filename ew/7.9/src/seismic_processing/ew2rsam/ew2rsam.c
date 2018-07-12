
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ew2rsam.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2007/02/26 13:44:40  paulf
 *     fixed heartbeat sprintf() to cast time_t as long
 *
 *     Revision 1.5  2002/06/05 15:21:07  patton
 *     Made logit changes.
 *
 *     Revision 1.4  2001/05/09 20:09:22  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPid.
 *
 *     Revision 1.3  2000/08/08 18:38:20  lucky
 *     Lint cleanup
 *
 *     Revision 1.2  2000/07/24 20:38:44  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.1  2000/02/14 17:18:52  lucky
 *     Initial revision
 *
 *
 */

/*
 * ew2rsam.c:  
 *
 *    Initial version by Lucky Vidmar Thu May  6 10:07:21 MDT 1999
 *
 *    This module reads trace messages from the InRing and computes
 *  rsam values over time periods defined in the configuration file.
 *  The rsam values are written out to the OutRing as a tracebuf
 *  message (TYPE_TRACEBUF), containing one value, with endtime 
 *  and starttime both equal to the time of the last sample in the
 *  rsam computation period; the sampling rate is the inverse of 
 *  the rsam averaging period.
 *    The channel name in the S-C-N of the newly produced message  
 *  is modified to reflect the Rsam nature of the message. The 
 *  original channel code is appended the string _Rx, where x is 
 *  the index of the rsam computation period (0 - 4). 
 *  If the newly generated channel name exceedes the maximum 
 *  length of the chan string, an error is reported and that message
 *  is NOT written out to the OutRing.
 *      
 *  The rsam computation algorithm was provided by Tom Murray
 *  (tlmurray@usgs.gov)
 *      
 */

#ifdef _OS2
#define INCL_DOSMEMMGR
#define INCL_DOSSEMAPHORES
#include <os2.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <earthworm.h>
#include <kom.h>
#include <swap.h>
#include <transport.h>
#include <trace_buf.h>
#include <mem_circ_queue.h>



#define EW2RSAM_VERSION  "1.0.6 2015.05.12 - SCNL capable"

static  SHM_INFO  InRegion;      /* shared memory region to use for i/o    */
static  SHM_INFO  OutRegion;     /* shared memory region to use for i/o    */


#define   NUM_COMMANDS 	9 	/* how many required commands in the config file */

#define   MAXLOGO   5
MSG_LOGO  GetLogo[MAXLOGO];       /* array for requesting module,type,instid */
short     nLogo;
 

#define SCN_INCREMENT   10	 /* how many more are allocated each time we run out */
#define STATION_LEN      6   /* max string-length of station code     */
#define CHAN_LEN         8   /* max string-length of component code   */
#define NETWORK_LEN      8   /* max string-length of network code     */
#define LOCATION_LEN	 4	 /* max string-length of location code    */
#define DUMMY_LOC	 "=="

/* The message queue
 *******************/
int		queue_size = 500;   	/* How many msgs can we queue */
QUEUE 	MsgQueue;				/* from queue.h */

/* Thread stuff */
#define THREAD_STACK 8192
static unsigned tidProcessor;    /* Processor thread id */
static unsigned tidStacker;      /* Thread moving messages from InRing */
                                 /* to MsgQueue */
int MessageStackerStatus = 0;      /* 0=> Stacker thread ok. <0 => dead */
int ProcessorStatus = 0;           /* 0=> Processor thread ok. <0 => dead */



/* Combination of the SCN codes and logos 
 *****************************************/
typedef struct scn_struct {

	char		sta[STATION_LEN];         /* Site name */
	char		chan[CHAN_LEN];           /* Component/channel code */
	char		net[NETWORK_LEN];         /* Network name */
	char		loc[LOCATION_LEN];
} SCNstruct;

SCNstruct 	*IncludeSCN = NULL; 		/* which SCNs to get */
int     	num_IncludeSCN = 0;			/* how many do we have */
int     	max_IncludeSCN = 0;				/* how many are allocated so far */

SCNstruct 	*ExcludeSCN = NULL; 		/* which SCNs NOT to get */
int     	num_ExcludeSCN = 0;			/* how many do we have */
int     	max_ExcludeSCN = 0;				/* how many are allocated so far */


/* RSAM book-keeping constants and structures */

#define	DC_ARRAY_ENTRIES	20
#define	DC_TIME_DIFF		3.0
#define	DC_STARTUP_ENTRIES	4
#define	DC_OFFSET_INVALID	-1.00
#define	TIME_INVALID        -1.00
#define	MAX_TIME_PERIODS	5
#define	MAX_CHAN_LEN		5 	

typedef struct time_period_struct
{
	double	time_period; 	/* time period in seconds */
	double	rsam_value;		/* currently kept rsam total */
	int		rsam_nsamp;		/* samples in the current total */
	double	rsam_starttime;	/* when did we start counting ? */
	char	outchan[TRACE_CHAN_LEN];	/* outgoing channel label */
	char	outloc[LOCATION_LEN];		/* TB2 location code */
} Tstruct;

/* there will be one for each requested channel */
/* with the maximum read from the configuration file */

typedef struct rsam_values_struct
{

	/* START DC offset section */

	double	DC_offset;			/* DC offset value */

	double 	DC_cur_val;			/* Current DC total */
	int		DC_cur_nsamp;		/* Number of samples in current total */
	double	DC_starttime;		/* When did we start counting ? */

	double	DC_array[DC_ARRAY_ENTRIES];	/* Averages for previous time slices */
	int		DC_cur_pos;			/* index of the current position in the array */
	int		DC_start_pos;		/* index of the starting position in the array */

	int		DC_startup;			/* TRUE if we are still starting out */
	/* END DC offset section */

	/* START time period section */
	Tstruct		TP[MAX_TIME_PERIODS];

} RSAM_val;


typedef struct rsam_struct
{

	char	sta[7];
	char	chan[9];
	char	net[9];
	char	loc[3];
	RSAM_val *values;

} RSAM;


int     	num_periods = 0;				/* how many rsam time periods do we keep */
double		RsamPeriod[MAX_TIME_PERIODS];
int     	MaxSCN = 0;				 	/* max number of SCNs to track */
        
RSAM		*Rsam;						/* Rsam struct, one for each SCN to track */
int			num_rsam;					/* How many have been allocated */

char 		useTB2in = 0, useTB2out = 0;/* Process TRACEBUF2 messages instead of TRACEBUF */


/* Things to read or derive from configuration file
 **************************************************/
static char    InRingName[MAX_RING_STR];      /* name of transport ring for i/o    */
static char    OutRingName[MAX_RING_STR];     /* name of transport ring for i/o    */
static char    MyModName[MAX_MOD_STR];       /* this module's given name          */
static char    MyProgName[256];     /* this module's program name        */
static int     LogSwitch;           /* 0 if no logfile should be written */
static long    HeartBeatInterval;   /* seconds between heartbeats        */
static int     Debug = 0;           /* Debug flag, 0-no debug, 
												   1-basic debug,
												   2-super debug */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          InKey;         /* key of transport ring for i/o     */
static long          OutKey;        /* key of transport ring for i/o     */
static unsigned char InstId;        /* local installation id             */
static unsigned char MyModId;       /* Module Id for this program        */
static unsigned char TypeHeartBeat; 
static unsigned char TypeWaveformIn, TypeWaveformOut;
static unsigned char TypeError;
static unsigned char InstWild;
static unsigned char ModWild;

/* Error messages used by ew2rsam 
 *********************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_QUEUE         4   /* trouble with the MsgQueue operation */

static char  errText[256];    /* string for log/error messages          */

pid_t MyPid;	/** Hold our process ID to be sent with heartbeats **/


/* Functions in this source file 
 *******************************/
static	int  	ew2rsam_config (char *);
static	int  	ew2rsam_lookup (void);
static	void  	ew2rsam_status (unsigned char, short, char *);
static 	int 	matchSCNL (char *, char *, char *, char *, SCNstruct *, int, SCNstruct *, int, int *);
thr_ret			MessageStacker (void *);
thr_ret			Processor (void *);





int main (int argc, char **argv)
{
	time_t			timeNow;		   /* current time                  */ 
	time_t			timeLastBeat;	   /* time last heartbeat was sent  */
	long			recsize;		   /* size of retrieved message     */
	MSG_LOGO		reclogo;		   /* logo of retrieved message     */
	char			*flushmsg;
	int				i;


	/* Check command line arguments 
	 ******************************/
	if (argc != 2)
	{
		fprintf (stderr, "Usage: ew2rsam <configfile>\n");
		fprintf (stderr, "Version: %s\n", EW2RSAM_VERSION);
		return EW_FAILURE;
	}

	/* Check that either SPARC or INTEL were defined during compilation */
	/* We want to make sure before we get too far */

#if defined _SPARC
	;
#elif defined _INTEL
	;
#else
	logit ("e", "ew2rsam: Must have either SPARC or INTEL defined during compilation; exitting!");
	return EW_FAILURE;
#endif
	

	/* To be used in loging functions
	 ********************************/
	if (get_prog_name (argv[0], MyProgName) != EW_SUCCESS)
	{
		fprintf (stderr, "ew2rsam: Call to get_prog_name failed.\n");
		return EW_FAILURE;
	}

	/* Initialize name of log-file & open it 
	 ***************************************/
	logit_init (argv[1], 0, 256, 1);	

	/* Read the configuration file(s)
	 ********************************/
	if (ew2rsam_config(argv[1]) != EW_SUCCESS)
	{
		fprintf (stderr, "ew2rsam: Call to ew2rsam_config failed \n");
		return EW_FAILURE;
	}
	logit ("" , "%s(%s): Read command file <%s>\n", 
						MyProgName, MyModName, argv[1]);

	/* Look up important info from earthworm.h tables
	 ************************************************/
	if (ew2rsam_lookup() != EW_SUCCESS)
	{
		logit ("et", "%s(%s): Call to ew2rsam_lookup failed \n",
						MyProgName, MyModName);
		return EW_FAILURE;
	}


	/* Reinitialize logit to desired logging level
	 *********************************************/
	logit_init (argv[1], 0, 256, LogSwitch);

	/* Get our process ID
	 **********************/
	if ((MyPid = getpid ()) == -1)
	{
		logit ("e", "%s(%s): Call to getpid failed. Exitting.\n",
										MyProgName, MyModName);
		return (EW_FAILURE);
	}



	/* Attach to Input shared memory ring 
	 *******************************************/
	tport_attach (&InRegion, InKey);
	logit ("", "%s(%s): Attached to public memory region %s: %d\n", 
	          MyProgName, MyModName, InRingName, InKey);

	/* Attach to Output shared memory ring 
	 *******************************************/
	tport_attach (&OutRegion, OutKey);
	logit ("", "%s(%s): Attached to public memory region %s: %d\n", 
	          MyProgName, MyModName, OutRingName, OutKey);

	/* Force a heartbeat to be issued in first pass thru main loop
	 *************************************************************/
	timeLastBeat = time (&timeNow) - HeartBeatInterval - 1;

	/* Flush the incomming transport ring 
	 *************************************/
	if ((flushmsg = (char *) malloc (MAX_BYTES_PER_EQ)) ==  NULL)
	{
		logit ("e", "ew2rsam: can't allocate flushmsg; exitting.\n");
		return EW_FAILURE;
	}

	while (tport_getmsg (&InRegion, GetLogo, nLogo, &reclogo,
			&recsize, flushmsg, (MAX_BYTES_PER_EQ - 1)) != GET_NONE)

        ;


	/* Initialize the Rsam structure */
	num_rsam = 0;
	
	if ((Rsam = (RSAM *) malloc (MaxSCN * sizeof (RSAM))) == NULL)
	{
		logit ("e", "ew2rsam: Could not malloc Rsam; exitting!\n");
		return EW_FAILURE;
	}

	for (i = 0; i < MaxSCN; i++)
	{
		Rsam->values = NULL;
	}


	/* Create MsgQueue mutex */
	CreateMutex_ew();


	/* Allocate the message Queue
	 ********************************/
	initqueue (&MsgQueue, queue_size, MAX_BYTES_PER_EQ);

	/* Start message stacking thread which will read 
	 * messages from the InRing and put them into the Queue 
	 *******************************************************/
	if (StartThread (MessageStacker, (unsigned) THREAD_STACK, &tidStacker) == -1)
	{
		logit( "e", 
			"ew2rsam: Error starting MessageStacker thread.  Exitting.\n");
		tport_detach (&InRegion);
		tport_detach (&OutRegion);
		return EW_FAILURE;
	}

	MessageStackerStatus = 0; /*assume the best*/


	/* Start decimator thread which will read messages from
	 * the Queue, process them and write them to the OutRing
	 *******************************************************/
	if (StartThread (Processor, (unsigned) THREAD_STACK, &tidProcessor) == -1)
	{
		logit( "e", 
			"ew2rsam: Error starting Processor thread.  Exitting.\n");
		tport_detach (&InRegion);
		tport_detach (&OutRegion);
		return EW_FAILURE;
	}

	ProcessorStatus = 0; /*assume the best*/

/*--------------------- setup done; start main loop -------------------------*/

	while (tport_getflag (&InRegion) != TERMINATE  &&
               tport_getflag (&InRegion) != MyPid )
	{

		/* send ew2rsam' heartbeat
		***************************/
		if (time (&timeNow) - timeLastBeat  >=  HeartBeatInterval) 
		{
			timeLastBeat = timeNow;
			ew2rsam_status (TypeHeartBeat, 0, ""); 
		}

		/* Check on our threads */
		if (MessageStackerStatus < 0)
		{
			logit ("et", 
				"ew2rsam: MessageStacker thread died. Exitting\n");
			return EW_FAILURE;
		}

		if (ProcessorStatus < 0)
		{
			logit ("et", 
				"ew2rsam: Processor thread died. Exitting\n");
			return EW_FAILURE;
		}

		sleep_ew (1000);

	} /* wait until TERMINATE is raised  */  

	/* Termination has been requested
	 ********************************/
	tport_detach (&InRegion);
	tport_detach (&OutRegion);
	logit ("t", "ew2rsam: Termination requested; exitting!\n" );
	return EW_SUCCESS;

}

/******************************************************************************
 *  ew2rsam_config() processes command file(s) using kom.c functions;        *
 *                    exits if any errors are encountered.                    *
 ******************************************************************************/
static int ew2rsam_config (char *configfile)
{
	char     		init[NUM_COMMANDS];     
						/* init flags, one byte for each required command */
	int      		nmiss;
						/* number of required commands that were missed   */
	char    		*com;
	char    		*str = NULL;
	int      		nfiles;
	int      		success;
	int      		i;


	/* Set to zero one init flag for each required command 
	*****************************************************/   
	for (i = 0; i < NUM_COMMANDS; i++)
		init[i] = 0;

	nLogo = 0;
	num_periods = 0;
	num_IncludeSCN = 0;
	num_ExcludeSCN = 0;

	/* Open the main configuration file 
	**********************************/
	nfiles = k_open (configfile); 
	if (nfiles == 0) 
	{
		logit ("e",
			"ew2rsam: Error opening command file <%s>; exitting!\n", 
															configfile);
		return EW_FAILURE;
	}

	/* Process all command files
	***************************/
	while (nfiles > 0)   /* While there are command files open */
	{
		while (k_rd ())        /* Read next line from active file  */
		{  
			com = k_str ();         /* Get the first token from line */

			/* Ignore blank lines & comments
			*******************************/
			if (!com)
				continue;
			if (com[0] == '#')
				continue;

			/* Open a nested configuration file 
			**********************************/
			if (com[0] == '@') 
			{
				success = nfiles + 1;
				nfiles  = k_open (&com[1]);
				if (nfiles != success) 
				{
					logit ("e", 
				  	  "ew2rsam: Error opening command file <%s>; exitting!\n", &com[1]);
					return EW_FAILURE;
				}
				continue;
			}

			/* Process anything else as a command 
			************************************/
	/*0*/ 	if (k_its ("MyModId")) 
			{
				if ((str = k_str ()) != NULL)
				{
					strcpy (MyModName, str);
					init[0] = 1;
				}
			}
	/*1*/	else if (k_its ("InRing")) 
			{
				if ((str = k_str ()) != NULL)
				{
					strcpy (InRingName, str);
					init[1] = 1;
				}
			}
	/*2*/	else if (k_its ("OutRing")) 
			{
				if ((str = k_str ()) != NULL)
				{
					strcpy (OutRingName, str);
					init[2] = 1;
				}
			}
	/*3*/	else if (k_its ("HeartBeatInterval")) 
			{
				HeartBeatInterval = k_long ();
				init[3] = 1;
			}
	/*4*/	else if (k_its ("LogFile"))
			{
				LogSwitch = k_int();
				init[4] = 1;
			}

			/* Enter installation & module types to get
			 *******************************************/
	/*5*/	else if (k_its ("GetWavesFrom")) 
			{
				char *logoType = useTB2in ? "TYPE_TRACEBUF2" : "TYPE_TRACEBUF";
				if (nLogo >= MAXLOGO) 
				{
					logit ("e", "ew2rsam: Too many <GetMsgLogo> commands in <%s>; "
										"; max=%d; exiting!\n", configfile, (int) MAXLOGO);
					return EW_FAILURE;
				}
				if ((str = k_str()) != NULL)
				{
					if (GetInst (str, &GetLogo[nLogo].instid) != 0) 
					{
						logit ("e", "ew2rsam: Invalid installation name <%s> in "
											"<GetWavesFrom> cmd; exiting!\n", str);
						return EW_FAILURE;
					}
                }
				if ((str = k_str()) != NULL)
				{
					if (GetModId (str, &GetLogo[nLogo].mod) != 0) 
					{
						logit ("e", "ew2rsam: Invalid module name <%s> in <GetWavesFrom> "
										"cmd; exiting!\n", str);
						return EW_FAILURE;
					}
				}
				/* We'll always fetch trace messages */
				
				if (GetType (logoType, &GetLogo[nLogo].type) != 0) 
				{
					logit ("e", "ew2rsam: Invalid msgtype <%s> in <GetWavesFrom> "
										"cmd; exiting!\n", str);
					return EW_FAILURE;
				}
				nLogo++;
				init[5] = 1;
			}


			/* Enter SCNs to be included 
			*******************************/
	/*6*/	else if (k_its ("IncludeSCN")) 
			{
				if (num_IncludeSCN >= max_IncludeSCN) 
				{
					/* Need to allocate more */
					max_IncludeSCN = max_IncludeSCN + SCN_INCREMENT;
					if ((IncludeSCN = (SCNstruct *) realloc (IncludeSCN, 
							(max_IncludeSCN * sizeof (SCNstruct)))) == NULL)
					{
						logit ("e", "ew2rsam: Call to realloc failed; exitting!\n");
						return EW_FAILURE;
					}
				}

				str = k_str ();
				if ( (str != NULL) && (strlen (str) < (size_t) TRACE_STA_LEN) )
                	strcpy (IncludeSCN[num_IncludeSCN].sta, str);

				str = k_str ();
				if ( (str != NULL) && (strlen (str) < (size_t) TRACE_CHAN_LEN) )
                	strcpy (IncludeSCN[num_IncludeSCN].chan, str);

				str = k_str ();
				if ( (str != NULL) && (strlen (str) < (size_t) TRACE_NET_LEN) )
                	strcpy (IncludeSCN[num_IncludeSCN].net, str);
                	strcpy (IncludeSCN[num_IncludeSCN].loc, DUMMY_LOC);

				num_IncludeSCN = num_IncludeSCN + 1;
				init[6] = 1;

			} 
	/*6*/	else if (k_its ("IncludeSCNL")) 
			{
				if (num_IncludeSCN >= max_IncludeSCN) 
				{
					/* Need to allocate more */
					max_IncludeSCN = max_IncludeSCN + SCN_INCREMENT;
					if ((IncludeSCN = (SCNstruct *) realloc (IncludeSCN, 
							(max_IncludeSCN * sizeof (SCNstruct)))) == NULL)
					{
						logit ("e", "ew2rsam: Call to realloc failed; exitting!\n");
						return EW_FAILURE;
					}
				}

				str = k_str ();
				if ( (str != NULL) && (strlen (str) < (size_t) TRACE2_STA_LEN) )
                	strcpy (IncludeSCN[num_IncludeSCN].sta, str);

				str = k_str ();
				if ( (str != NULL) && (strlen (str) < (size_t) TRACE2_CHAN_LEN) )
                	strcpy (IncludeSCN[num_IncludeSCN].chan, str);

				str = k_str ();
				if ( (str != NULL) && (strlen (str) < (size_t) TRACE2_NET_LEN) )
                	strcpy (IncludeSCN[num_IncludeSCN].net, str);

				str = k_str ();
				if ( (str != NULL) && (strlen (str) < (size_t) TRACE2_LOC_LEN) )
                	strcpy (IncludeSCN[num_IncludeSCN].loc, str);

				num_IncludeSCN = num_IncludeSCN + 1;
				init[6] = 1;

			} 
			/* Enter SCNs to be excluded 
			*******************************/
	/*NR*/	else if (k_its ("ExcludeSCN")) 
			{
				if (num_ExcludeSCN >= max_ExcludeSCN) 
				{
					/* Need to allocate more */
					max_ExcludeSCN = max_ExcludeSCN + SCN_INCREMENT;
					if ((ExcludeSCN = (SCNstruct *) realloc (ExcludeSCN, 
							(max_ExcludeSCN * sizeof (SCNstruct)))) == NULL)
					{
						logit ("e", "ew2rsam: Call to realloc failed; exitting!\n");
						return EW_FAILURE;
					}
				}

				str = k_str ();
				if ( (str != NULL) && (strlen (str) < (size_t) TRACE_STA_LEN) )
                	strcpy (ExcludeSCN[num_ExcludeSCN].sta, str);

				str = k_str ();
				if ( (str != NULL) && (strlen (str) < (size_t) TRACE_CHAN_LEN) )
                	strcpy (ExcludeSCN[num_ExcludeSCN].chan, str);

				str = k_str ();
				if ( (str != NULL) && (strlen (str) < (size_t) TRACE_NET_LEN) )
                	strcpy (ExcludeSCN[num_ExcludeSCN].net, str);
                	strcpy (ExcludeSCN[num_ExcludeSCN].loc, DUMMY_LOC); /* a dummy location code */

				num_ExcludeSCN = num_ExcludeSCN + 1;

			}
	/*NR*/	else if (k_its ("ExcludeSCNL")) 
			{
				if (num_ExcludeSCN >= max_ExcludeSCN) 
				{
					/* Need to allocate more */
					max_ExcludeSCN = max_ExcludeSCN + SCN_INCREMENT;
					if ((ExcludeSCN = (SCNstruct *) realloc (ExcludeSCN, 
							(max_ExcludeSCN * sizeof (SCNstruct)))) == NULL)
					{
						logit ("e", "ew2rsam: Call to realloc failed; exitting!\n");
						return EW_FAILURE;
					}
				}

				str = k_str ();
				if ( (str != NULL) && (strlen (str) < (size_t) TRACE2_STA_LEN) )
                	strcpy (ExcludeSCN[num_ExcludeSCN].sta, str);

				str = k_str ();
				if ( (str != NULL) && (strlen (str) < (size_t) TRACE2_CHAN_LEN) )
                	strcpy (ExcludeSCN[num_ExcludeSCN].chan, str);

				str = k_str ();
				if ( (str != NULL) && (strlen (str) < (size_t) TRACE2_NET_LEN) )
                	strcpy (ExcludeSCN[num_ExcludeSCN].net, str);

				str = k_str ();
				if ( (str != NULL) && (strlen (str) < (size_t) TRACE2_LOC_LEN) )
                	strcpy (ExcludeSCN[num_ExcludeSCN].loc, str);

				num_ExcludeSCN = num_ExcludeSCN + 1;

			}
	/*7*/	else if (k_its ("MaxSCN")) 
			{
				MaxSCN = k_int ();
				init[7] = 1;
			}
	/*8*/	else if (k_its ("RsamPeriod")) 
			{
				if (num_periods >= MAX_TIME_PERIODS)
				{
					logit ("e", 
						"ew2rsam: Maximum number of <RsamPeriod> commands exceeded; exitting!\n");
									
					return EW_FAILURE;
				}
				
				RsamPeriod[num_periods] = k_val ();
				num_periods = num_periods + 1;
				
				init[8] = 1;
			}
	/*NR*/	else if (k_its ("Debug"))
			{
				Debug = k_int();
			}

	/*NR*/	else if ( k_its ("ReadTRACEBUF2") )
			{
				int j;
				
				for ( j=0; j<nLogo; j++ )
					if (GetType ("TYPE_TRACEBUF2", &GetLogo[nLogo].type) != 0) 
					{
						logit ("e", "ew2rsam: Invalid msgtype <%s> in <GetWavesFrom> "
											"cmd; exiting!\n", str);
						return EW_FAILURE;
					}
				useTB2in = 1;
				logit( "", "ew2rsam: Reading TRACEBUF2 messages\n" );
			}
			
	/*NR*/	else if ( k_its ("QueueSize") )
			{
				queue_size = k_int();
				logit( "", "ew2rsam: Changing queue size to %d messages\n", queue_size );
			}

	/*NR*/	else if ( k_its ("WriteTRACEBUF2") )
			{
				useTB2out = 1;
				logit( "", "ew2rsam: Writing TRACEBUF2 messages\n" );
			}
			
			/* Unknown command
			*****************/ 
			else 
			{
				logit ("e", "ew2rsam: <%s> Unknown command in <%s>.\n", 
								com, configfile);
				continue;
			}

			/* See if there were any errors processing the command 
			*****************************************************/
			if (k_err ()) 
			{
				logit ("e", 
					"ew2rsam: Bad <%s> command in <%s>; exitting!\n",
						com, configfile);
				return EW_FAILURE;
			}

		} /** while k_rd() **/

		nfiles = k_close ();

	} /** while nfiles **/

	/* After all files are closed, check init flags for missed commands
	******************************************************************/
	nmiss = 0;
	for (i = 0; i < NUM_COMMANDS; i++)  
		if (!init[i]) 
			nmiss++;

	if (nmiss) 
	{
		logit ("e", "ew2rsam: ERROR, no ");
		if (!init[0])  logit ("e", "<MyModId> "        );
		if (!init[1])  logit ("e", "<InRing> "          );
		if (!init[2])  logit ("e", "<OutRing> " );
		if (!init[3])  logit ("e", "<HeartBeatInterval> "     );
		if (!init[4])  logit ("e", "<LogFile> "     );
		if (!init[5])  logit ("e", "<GetWavesFrom> "     );
		if (!init[6])  logit ("e", "<IncludeSCN> "     );
		if (!init[7])  logit ("e", "<MaxSCN> "     );
		if (!init[8])  logit ("e", "<RsamPeriod> "     );

		logit ("e", "command(s) in <%s>; exitting!\n", configfile);
		return EW_FAILURE;
	}

	return EW_SUCCESS;
}

/******************************************************************************
 *  ew2rsam_lookup( )   Look up important info from earthworm.h tables       *
 ******************************************************************************/
static int ew2rsam_lookup( void )
{
	char *typeNameIn = useTB2in ? "TYPE_TRACEBUF2" : "TYPE_TRACEBUF";
	char *typeNameOut = useTB2out ? "TYPE_TRACEBUF2" : "TYPE_TRACEBUF";
	
	/* Look up keys to shared memory regions
	*************************************/
	if ((InKey = GetKey (InRingName)) == -1) 
	{
		logit ("et",
				"ew2rsam:  Invalid ring name <%s>; exitting!\n", InRingName);
		return EW_FAILURE;
	}

	if ((OutKey = GetKey (OutRingName) ) == -1) 
	{
		logit ("et",
			"ew2rsam:  Invalid ring name <%s>; exitting!\n", OutRingName);
		return EW_FAILURE;
	}

	/* Look up installations of interest
	*********************************/
	if (GetLocalInst (&InstId) != 0) 
	{
		logit ("et", 
			"ew2rsam: error getting local installation id; exitting!\n");
		return EW_FAILURE;
	}


	if (GetInst ("INST_WILDCARD", &InstWild ) != 0) 
	{ 
		logit ("et", 
			"ew2rsam: error getting wildcard installation id; exitting!\n");
		return EW_FAILURE;
	}

	/* Look up modules of interest
	******************************/
	if (GetModId (MyModName, &MyModId) != 0) 
	{
		logit ("et", 
			"ew2rsam: Invalid module name <%s>; exitting!\n", MyModName);
		return EW_FAILURE;
	}

	if (GetModId ("MOD_WILDCARD", &ModWild) != 0) 
	{
		logit ("et", 
			"ew2rsam: Invalid module name <MOD_WILDCARD>; exitting!\n");
		return EW_FAILURE;
	}

	/* Look up message types of interest
	*********************************/
	if (GetType ("TYPE_HEARTBEAT", &TypeHeartBeat) != 0) 
	{
		logit ("et", 
			"ew2rsam: Invalid message type <TYPE_HEARTBEAT>; exitting!\n");
		return EW_FAILURE;
	}

	if (GetType ("TYPE_ERROR", &TypeError) != 0) 
	{
		logit ("et", 
			"ew2rsam: Invalid message type <TYPE_ERROR>; exitting!\n");
		return EW_FAILURE;
	}

	if (GetType (typeNameIn, &TypeWaveformIn) != 0) 
	{
		logit ("et", 
			"ew2rsam: Invalid message type <%s>; exitting!\n", typeNameIn);
		return EW_FAILURE;
	}

	if (GetType (typeNameOut, &TypeWaveformOut) != 0) 
	{
		logit ("et", 
			"ew2rsam: Invalid message type <%s>; exitting!\n", typeNameOut);
		return EW_FAILURE;
	}

	return EW_SUCCESS;

} 

/******************************************************************************
 * ew2rsam_status() builds a heartbeat or error message & puts it into       *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
static void ew2rsam_status( unsigned char type, short ierr, char *note )
{
   MSG_LOGO    logo;
   char        msg[256];
   long        size;
   time_t        t;
 
	/* Build the message 
	*******************/ 
	logo.instid = InstId;
	logo.mod    = MyModId;
	logo.type   = type;

	time (&t);

	if (type == TypeHeartBeat)
	{
		sprintf (msg, "%ld %ld\n", (long) t, (long) MyPid);
	}
	else if (type == TypeError)
	{
		sprintf (msg, "%ld %hd %s\n", (long) t, ierr, note);
		logit ("et", "%s(%s): %s\n", MyProgName, MyModName, note);
	}

	size = (long)strlen (msg);   /* don't include the null byte in the message */

	/* Write the message to shared memory
	************************************/
	if (tport_putmsg (&OutRegion, &logo, size, msg) != PUT_OK)
	{
		if (type == TypeHeartBeat) 
		{
			logit ("et","%s(%s):  Error sending heartbeat.\n",
											MyProgName, MyModName);
		}
		else if (type == TypeError) 
		{
			logit ("et","%s(%s):  Error sending error:%d.\n", 
										MyProgName, MyModName, ierr);
		}
	}

}



/******************************************************************************
 * matchSCNL () - set retind to the index of the SCNL matching sta.cha.net.loc*
 *               found in the SCNlist array. Otherwise, set retind to -1      *
 *                                                                            *
 ******************************************************************************/
static int matchSCNL (char *sta, char *chan, char *net, char *loc,
					SCNstruct *Include, int numInclude, 
					SCNstruct *Exclude, int numExclude, int *retind)
{
	int i;
	int j;

	int staWild = 0;
	int netWild = 0;
	int chanWild = 0;
        int locWild = 0;

	int staMatch = 0;
	int netMatch = 0;
	int chanMatch = 0;
	int locMatch = 0;

	if (Debug>1) {
		if (loc != NULL)
			logit ("et",  "ew2rsam: checking SCNL against list %s.%s.%s.%s\n", sta, chan, net, loc);
		else
			logit ("et",  "ew2rsam: checking SCN against list %s.%s.%s\n", sta, chan, net);
	}


	if ((sta == NULL) || (chan == NULL) || (net == NULL) || 
								(Include == NULL) || (numInclude < 0))
	{
		logit ("et",  "ew2rsam: invalid parameters to check_scn_logo\n");
		return (EW_FAILURE);
	}

	*retind = -1;

	/* Is the SCN or SCNL if L is set to something real in the include list */
	for (i = 0; i < numInclude; i++)
	{
		staWild = 0;
		netWild = 0;
		chanWild = 0;
		locWild = 0;

		staMatch = 0;
		netMatch = 0;
		chanMatch = 0;
		locMatch = 0;

		/* Any wild cards ?*/
		if (strcmp (Include[i].sta, "*") == 0)
			staWild = 1;
		if (strcmp (Include[i].chan, "*") == 0)
			chanWild = 1;
		if (strcmp (Include[i].net, "*") == 0)
			netWild = 1;
		if (loc != NULL && strcmp (Include[i].loc, "*") == 0)
			locWild = 1;

		/* try to match explicitly */
		if (strcmp (sta, Include[i].sta) == 0)
			staMatch = 1;
		if (strcmp (chan, Include[i].chan) == 0)
			chanMatch = 1;
		if (strcmp (net, Include[i].net) == 0)
			netMatch = 1;
		if (loc != NULL && strcmp (loc, Include[i].loc) == 0)
			locMatch = 1;

		if ((staWild == 1) ||  (staMatch == 1))  
			staMatch = 1;
		if ((netWild == 1) ||  (netMatch == 1)) 
			netMatch = 1;
		if ((chanWild == 1) || (chanMatch == 1))
			chanMatch = 1;
		if ((locWild == 1) || (locMatch == 1))
			locMatch = 1;

		/* If all 3 components match, set SCN was found in the 
 		 * Include list - now we have to check Exclude list */
		if ( (loc == NULL && (staMatch + netMatch + chanMatch) == 3) || 
		     ( loc != NULL && (staMatch + netMatch + chanMatch + locMatch) == 4) )
		{

			/* Is the SCN (and possibly L) in the exclude list */
			for (j = 0; j < numExclude; j++)
			{
				staWild = 0;
				netWild = 0;
				chanWild = 0;
				locWild = 0;

				staMatch = 0;
				netMatch = 0;
				chanMatch = 0;
				locMatch = 0;

				/* Any wild cards ? */
				if (strcmp (Exclude[j].sta, "*") == 0)
					staWild = 1;
				if (strcmp (Exclude[j].chan, "*") == 0)
					chanWild = 1;
				if (strcmp (Exclude[j].net, "*") == 0)
					netWild = 1;
				if (loc != NULL && strcmp (Exclude[j].loc, "*") == 0)
					locWild = 1;

				/* try to match explicitly */
				if (strcmp (sta, Exclude[j].sta) == 0)
					staMatch = 1;
				if (strcmp (chan, Exclude[j].chan) == 0)
					chanMatch = 1;
				if (strcmp (net, Exclude[j].net) == 0)
					netMatch = 1;
				if (loc != NULL && strcmp (loc, Exclude[j].loc) == 0)
					netMatch = 1;

				if ((staWild == 1) ||  (staMatch == 1))  
					staMatch = 1;
				if ((netWild == 1) ||  (netMatch == 1)) 
					netMatch = 1;
				if ((chanWild == 1) || (chanMatch == 1))
					chanMatch = 1;
				if ((locWild == 1) || (locMatch == 1))
					locMatch = 1;

				/* If all 3 (or 4 if loc used) components match, set SCN was found in the 
 				 * Exclude list, we don't want it -   return -1 */
				if ( (loc == NULL && (staMatch + netMatch + chanMatch) == 3) || 
			             (loc != NULL && (staMatch + netMatch + chanMatch + locMatch) == 4) )
				{
					*retind = -1;
					return EW_SUCCESS;
				}

			} /* loop over exclude list */

			/* If we made it this far - SCN was not
			 * in the Exclude list - return i */

			*retind = i;
			return EW_SUCCESS;
			
		} /* if SCN was found in the include list */

	}
		
	return EW_SUCCESS;
}

/********************** Message Stacking Thread *******************
 *           Move messages from transport to memory queue         *
 ******************************************************************/
thr_ret		MessageStacker (void *dummy)
{
	char         	*msg;           /* "raw" retrieved message */
	int          	res;
	long         	recsize;        /* size of retrieved message */
	MSG_LOGO     	reclogo;        /* logo of retrieved message */
	TRACE_HEADER	*WaveHeader; 	
	TRACE2_HEADER	*Wave2Header;
	int          	ret;

	/* Allocate space for input/output messages
	 *******************************************/
	if ((msg = (char *) malloc (MAX_BYTES_PER_EQ)) == (char *) NULL)
	{
		logit ("e", "ew2rsam: error allocating msg; exitting!\n");
		goto error;
	}

	WaveHeader = (TRACE_HEADER *) msg;
	Wave2Header = (TRACE2_HEADER *) msg;


	/* Tell the main thread we're ok
	 ********************************/
	MessageStackerStatus = 0;

	/* Start service loop, picking up trigger messages
	 **************************************************/
	while (1)
	{
		/* Get a message from transport ring
		 ************************************/
		res = tport_getmsg (&InRegion, GetLogo, nLogo, &reclogo, 
								&recsize, msg, MAX_BYTES_PER_EQ-1);


		if (res == GET_NONE) 
		{
			sleep_ew(100); 
			continue;
		} /*wait if no messages for us */

		/* Check return code; report errors
		***********************************/
		if (res != GET_OK)
		{
			if (res == GET_TOOBIG)
			{
				sprintf (errText, "msg[%ld] i%d m%d t%d too long for target",
							recsize, (int) reclogo.instid,
							(int) reclogo.mod, (int)reclogo.type);
				ew2rsam_status (TypeError, ERR_TOOBIG, errText);
				continue;
			}
			else if (res == GET_MISS)
			{
				sprintf (errText, "missed msg(s) i%d m%d t%d in %s",
						(int) reclogo.instid, (int) reclogo.mod, 
						(int)reclogo.type, InRingName);
				ew2rsam_status (TypeError, ERR_MISSMSG, errText);
			}
			else if (res == GET_NOTRACK)
			{
				sprintf (errText, "no tracking for logo i%d m%d t%d in %s",
						(int) reclogo.instid, (int) reclogo.mod, 
						(int)reclogo.type, InRingName);
				ew2rsam_status (TypeError, ERR_NOTRACK, errText);
			}
		}
		if (Debug>1) {
			if (useTB2in==0)
				logit ("et",  "ew2rsam: checking SCN against list %s.%s.%s\n", 
					WaveHeader->sta, WaveHeader->chan, WaveHeader->net);
			else
				logit ("et",  "ew2rsam: checking SCNL against list %s.%s.%s.%s\n", 
					Wave2Header->sta, Wave2Header->chan, Wave2Header->net, Wave2Header->loc);
		}

		/* If necessary, swap bytes in the wave message
		 **********************************************/
		if (useTB2in==0)  {
			if (WaveMsgMakeLocal (WaveHeader) < 0)
			{
				logit ("et", "%s(%s): problems making waveform native.\n",
									MyProgName, MyModName);
				continue;
			}
		} else {
			if (WaveMsg2MakeLocal (Wave2Header) < 0)
			{
				logit ("et", "%s(%s): problems making waveform native.\n",
									MyProgName, MyModName);
				continue;
			}
		}

		/* Check to see if msg's SCN code is desired
		 *********************************************/
		if (matchSCNL (WaveHeader->sta, WaveHeader->chan, WaveHeader->net, 
					useTB2in ? Wave2Header->loc : NULL,
					IncludeSCN, num_IncludeSCN, ExcludeSCN,
					num_ExcludeSCN, &res) != EW_SUCCESS)
		{
			logit ("et", "%s(%s): Call to matchSCNL failed\n",
										MyProgName, MyModName);
			goto error;
		}

		/* If the message matches one of desired SCNs, queue it 
		 **********************************************************/
		if (res >= 0)
		{
			if (Debug>1)
			{
				if (useTB2in==0)
					logit ("et",  "ew2rsam: MATCHED SCN against list %s.%s.%s\n", 
						WaveHeader->sta, WaveHeader->chan, WaveHeader->net);
				else
					logit ("et",  "ew2rsam: MATCHED SCNL against list %s.%s.%s.%s\n", 
						Wave2Header->sta, Wave2Header->chan, Wave2Header->net, Wave2Header->loc);
			}

			/* Queue retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK)
			*********************************************************/
			RequestMutex ();
			/* put it into the queue */
			ret = enqueue (&MsgQueue, msg, recsize, reclogo); 
			ReleaseMutex_ew ();

			if (ret != 0)
			{
				if (ret == -2)  /* Serious: quit */
				{
					sprintf (errText, "internal queue error. Terminating.");
					ew2rsam_status (TypeError, ERR_QUEUE, errText);
					goto error;
				}
				if (ret == -1)
				{
					sprintf (errText, 
						"queue cannot allocate memory. Lost message.");
					ew2rsam_status (TypeError, ERR_QUEUE, errText);
					continue;
				}
				if (ret == -3) 
				{
					sprintf (errText, "Queue full. Message lost.");
					ew2rsam_status (TypeError, ERR_QUEUE, errText);
					continue;
				}
			} /* problem from enqueue */

		} /* if we have a desired message */
		else if (Debug > 2)
			logit( "", "Message didn't match\n" );

	} /* while (1) */

	/* we're quitting
	 *****************/
	error:
	MessageStackerStatus = -1; /* file a complaint to the main thread */
	KillSelfThread (); /* main thread will restart us */
	return THR_NULL_RET; /* should never be reached */
}



/********************** Message Processing Thread ****************
 ******************************************************************/
thr_ret		Processor (void *dummy)
{


	int				ret;
	int				tmp = 0;
	int				i, j, index, found;
	int				sec_into_min, last_min;
	double 			        packet_time_int,temp_time_period;
	double			tmpf;
	int				gotMsg;
    MSG_LOGO        reclogo;           /* logo of retrieved message */

	TRACE_HEADER	*WaveHeader;	   /* ptr to Wave data buffer       */
	TRACE2_HEADER	*Wave2Header;	   /* ptr to Wave data buffer       */
	int32_t			*WaveLong;		   /* if wave data is int32s         */
	short			*WaveShort;		   /* if wave data is shorts        */
	char 			*WaveBuf;          /* string to hold wave message   */
	long 			WaveBufLen;        /* length of WaveBuf             */

	TRACE_HEADER	*RsamHeader;	   /* ptr to Wave data buffer       */
	TRACE2_HEADER	*Rsam2Header;	   /* ptr to Wave data buffer       */
	int32_t			*RsamLong;		   /* if wave data is longs         */
	char 			*RsamBuf;          /* string to hold wave message   */
	long 			RsamBufLen;        /* length of WaveBuf             */

	char			scn[256];


	/* Allocate the waveform buffer
     *******************************/
	WaveBufLen = (MAX_TRACEBUF_SIZ * sizeof (int32_t)) + sizeof (TRACE_HEADER);
	WaveBuf = (char *) malloc ((size_t) WaveBufLen);

	if (WaveBuf == NULL)
	{
		logit ("et", "ew2rsam: Cannot allocate waveform buffer\n");
		ProcessorStatus = -1;
		KillSelfThread();
	}


	/* Point to header and data portions of waveform message
	 *****************************************************/
	WaveHeader  = (TRACE_HEADER *) WaveBuf;
	Wave2Header = (TRACE2_HEADER *)WaveBuf;
	WaveLong  = (int32_t *) (WaveBuf + sizeof (TRACE_HEADER));
	WaveShort = (short *) (WaveBuf + sizeof (TRACE_HEADER));



	/* Now, do the same for the Rsam trace message 
     ********************************************8***/
	/* We only need room for one sample */
	RsamBufLen = sizeof (int32_t) + sizeof (TRACE_HEADER);
	RsamBuf = (char *) malloc ((size_t) RsamBufLen);

	if (RsamBuf == NULL)
	{
		logit ("et", "ew2rsam: Cannot allocate Rsam waveform buffer\n");
		ProcessorStatus = -1;
		KillSelfThread();
	}


	/* Point to header and data portions of waveform message
	 *****************************************************/
	RsamHeader  = (TRACE_HEADER *) RsamBuf;
	Rsam2Header = (TRACE2_HEADER *)RsamBuf;
	RsamLong  = (int32_t *) (RsamBuf + sizeof (TRACE_HEADER));

	while (1)
	{
		gotMsg = FALSE;
		while (gotMsg == FALSE)
		{
		
			RequestMutex ();
			ret = dequeue (&MsgQueue, WaveBuf, &WaveBufLen, &reclogo);
			ReleaseMutex_ew ();

			if (ret < 0)
			{
				/* empty queue */
				sleep_ew (1000);
			}
				else
				gotMsg = TRUE;
		} /* while no messages are dequeued */

		/* Just a little typing help */
		if ( useTB2in )
			sprintf (scn, "%s.%s.%s.%s", 
				Wave2Header->sta, Wave2Header->chan, Wave2Header->net, Wave2Header->loc);
		else
			sprintf (scn, "%s.%s.%s", 
				WaveHeader->sta, WaveHeader->chan, WaveHeader->net);

		/* try to find this SCN in the RSAM list */
		i = 0; 
		index = -1; 
		found = FALSE; 
		while ((i < num_rsam) && (found == FALSE))
		{
			if ( (useTB2in && (strcmp (Rsam[i].sta, Wave2Header->sta) == 0) &&
			    		(strcmp (Rsam[i].chan, Wave2Header->chan) == 0) &&
			    		(strcmp (Rsam[i].net, Wave2Header->net) == 0)) ||
			     (!useTB2in && (strcmp (Rsam[i].sta, WaveHeader->sta) == 0) &&
			    		(strcmp (Rsam[i].chan, WaveHeader->chan) == 0) &&
			    		(strcmp (Rsam[i].net, WaveHeader->net) == 0)) )
			{
				index = i;
				found = TRUE;
			}

			i = i + 1;
		}

		/* if index not found, allocate a new struct */
		if (index == -1)
		{

			if (Debug > 2)
				logit ("e", "Got new SCN %s\n", scn);

			if (num_rsam >= MaxSCN)
			{
				logit ("e", "ew2rsam: Maximum number of SCNs reached; %s will be ignored.\n", scn);
			}
			else if ((strlen (WaveHeader->chan)) > MAX_CHAN_LEN)
			{
				logit ("e", "ew2rsam: Channel name too long; %s will be ignored.\n", scn);
			}
			else
			{
				/* allocate and initialize a new structure */

				/* SCN */
				if ( useTB2in ) {
					strcpy (Rsam[num_rsam].sta, Wave2Header->sta);
					strcpy (Rsam[num_rsam].chan, Wave2Header->chan);
					strcpy (Rsam[num_rsam].net, Wave2Header->net);
					strcpy (Rsam[num_rsam].loc, Wave2Header->loc);
				} else {
					strcpy (Rsam[num_rsam].sta, WaveHeader->sta);
					strcpy (Rsam[num_rsam].chan, WaveHeader->chan);
					strcpy (Rsam[num_rsam].net, WaveHeader->net);
				}

				if ((Rsam[num_rsam].values = (RSAM_val *) 
							malloc (sizeof (RSAM_val))) == NULL)
				{
					logit ("e", 
						"ew2rsam: Cannot malloc Rsam->values; exitting!\n");
#ifdef _WINNT
					return THR_NULL_RET;
#else
                                        return EW_FAILURE;
#endif
				}

                                /* cheryl - minute syncing */
				last_min = 0;
				sec_into_min = -1;
                                temp_time_period = -1;

				/* DC offset stuff */
				Rsam[num_rsam].values->DC_offset = DC_OFFSET_INVALID;
				Rsam[num_rsam].values->DC_cur_val = 0.0;
				Rsam[num_rsam].values->DC_cur_nsamp = 0;
				Rsam[num_rsam].values->DC_starttime = TIME_INVALID;

				Rsam[num_rsam].values->DC_cur_pos = 0;
				Rsam[num_rsam].values->DC_start_pos = 0;
				Rsam[num_rsam].values->DC_startup = TRUE;


				/* Rsam time interval stuff */
				for (i = 0; i < num_periods; i++)
				{

					Rsam[num_rsam].values->TP[i].time_period = RsamPeriod[i];
					Rsam[num_rsam].values->TP[i].rsam_value = 0.0;
					Rsam[num_rsam].values->TP[i].rsam_nsamp = 0;
					Rsam[num_rsam].values->TP[i].rsam_starttime = TIME_INVALID;

					if ( useTB2out ) {
						/* Build the outgoing location name */
						sprintf (Rsam[num_rsam].values->TP[i].outloc, "R%d", i);
					} else {
						/* Build the outgoing channel name */
						sprintf (Rsam[num_rsam].values->TP[i].outchan, "%s_R%d", 
									Rsam[num_rsam].chan, i);
					}
				}

				num_rsam = num_rsam + 1;
			}

		}

		if (index != -1)
		{
			if (Debug > 2)
				logit ("e", "%s: found at index %d\n", scn, index);

			/* RSAM PROCESSING HERE */

                        /* cheryl */
                        if (Debug > 2)
				logit ("e", "packet start time = %d, packet end time = %d\n", (int)WaveHeader->starttime,(int)WaveHeader->endtime);
                 

                        /* cheryl*/
			packet_time_int = WaveHeader->endtime - WaveHeader->starttime;


			/* Add samples to the DC offset computation */
			if (Rsam[index].values->DC_cur_nsamp == 0)
			{
				Rsam[index].values->DC_starttime = WaveHeader->starttime;
			}

			for (i = 0; i < WaveHeader->nsamp; i++)
			{
				if ((strcmp (WaveHeader->datatype, "i2") == 0) ||
						(strcmp (WaveHeader->datatype, "s2") == 0))
				{
					tmp = (int) WaveShort[i];
				}
				else if ((strcmp (WaveHeader->datatype, "i4") == 0) ||
						(strcmp (WaveHeader->datatype, "s4") == 0))
				{
					tmp = (int) WaveLong[i];
				}

				Rsam[index].values->DC_cur_val = 
							Rsam[index].values->DC_cur_val + tmp;

				Rsam[index].values->DC_cur_nsamp = 
							Rsam[index].values->DC_cur_nsamp + 1;

			}

			/*
			 * If time difference between endtime and starttime 
			 * is bigger than DC_TIME_DIFF, compute the next 
			 * entry in the DC_array, reset counters, and compute
			 * an updated DC_offset value.
			 */
			if ((WaveHeader->endtime - 
					Rsam[index].values->DC_starttime) > DC_TIME_DIFF)
			{


				j = Rsam[index].values->DC_cur_pos;
				Rsam[index].values->DC_array[j] = 
						Rsam[index].values->DC_cur_val /
						Rsam[index].values->DC_cur_nsamp;
  
				Rsam[index].values->DC_cur_val = 0.0;
				Rsam[index].values->DC_cur_nsamp = 0;

				if (Debug > 2)
					logit ("e", "%s: DC offset portion at index %d = %f\n", scn, j, Rsam[index].values->DC_array[j]);

				/* 
				 * Do we have enough to compute a DC offset? 
				 * This only matters if this is one of the first packets of 
				 * this SCN, i.e. if startup is TRUE 
				 */
				if (Rsam[index].values->DC_startup == TRUE)
				{
					if (Debug > 2)
						logit ("e", "%s: WAIT: still in startup phase \n", scn);
					/*
					 * Next time around we will have enough to compute 
					 * a valid DC offset 
					 */
					if (Rsam[index].values->DC_cur_pos >= DC_STARTUP_ENTRIES)
					{
						Rsam[index].values->DC_startup = FALSE;
					}
				}
				else
				{

					j = Rsam[index].values->DC_start_pos;
					tmpf = 0.0;
					tmp = 0;
					while ((j < DC_ARRAY_ENTRIES) && 
						(j != Rsam[index].values->DC_cur_pos))
					{
						if (Debug > 2)
							logit ("e", "%s: Adding index %d = %f\n", scn, j, Rsam[index].values->DC_array[j]);

						tmpf = tmpf + Rsam[index].values->DC_array[j];
						tmp = tmp + 1;
						j = j + 1;
					}

					/* Wrap around the array, if we reach the right end */
					if ((j == DC_ARRAY_ENTRIES) && 
							(j != Rsam[index].values->DC_cur_pos))
					{

						j = 0;
						while (j != Rsam[index].values->DC_cur_pos)
						{

							if (Debug > 2)
								logit ("e", "%s: Adding index %d = %f\n", scn, j, Rsam[index].values->DC_array[j]);


							tmpf = tmpf + Rsam[index].values->DC_array[j];
							tmp = tmp + 1;
							j = j + 1;
						}
					}

					/* add the current portion */
					j = Rsam[index].values->DC_cur_pos;
					tmpf = tmpf + Rsam[index].values->DC_array[j];
					tmp = tmp + 1;

					if (Debug > 2)
						logit ("e", "%s: Adding index %d = %f\n", scn, j, Rsam[index].values->DC_array[j]);

						
					Rsam[index].values->DC_offset = tmpf / tmp;

					if (Debug > 1)
						logit ("e", "%s: New offset (%d-%d) ==> %f\n", scn,
							Rsam[index].values->DC_start_pos,
							Rsam[index].values->DC_cur_pos,
							Rsam[index].values->DC_offset);


				}


				/* increment position pointers */
				if ((Rsam[index].values->DC_cur_pos =  
							Rsam[index].values->DC_cur_pos + 1) >= DC_ARRAY_ENTRIES)
					Rsam[index].values->DC_cur_pos = 0;

				/* Increment start_position only if we have filled the array */
				if (Rsam[index].values->DC_cur_pos == Rsam[index].values->DC_start_pos)
				{
					if ((Rsam[index].values->DC_start_pos =  
							Rsam[index].values->DC_start_pos + 1) >= DC_ARRAY_ENTRIES)
						Rsam[index].values->DC_start_pos = 0;
				}

			} /* End of DC offset computation */
			
			/*cheryl*/
			last_min = (int)((int)WaveHeader->starttime/60);
 			sec_into_min = (int)WaveHeader->starttime - last_min * 60;
                        if (Debug > 2)
			  logit ("e", "last_min = %d, sec_into_min = %d, packet_time_int = %f \n", last_min, sec_into_min, packet_time_int);
			

			/* If a valid DC offset exists, continue */
			if (Rsam[index].values->DC_offset != DC_OFFSET_INVALID)	
			{

				if (Debug > 2)
					logit ("e", "%s: Got a valid offset %f - continuing\n", scn, Rsam[index].values->DC_offset);


				for (j = 0; j < num_periods; j++)
				{
					/* Set start time for a new rsam period */
					if (Rsam[index].values->TP[j].rsam_starttime == TIME_INVALID)
					{
						Rsam[index].values->TP[j].rsam_starttime = 
													WaveHeader->starttime;
					}

				}



				for (i = 0; i < WaveHeader->nsamp; i++)
				{

					if ((strcmp (WaveHeader->datatype, "i2") == 0) ||
							(strcmp (WaveHeader->datatype, "s2") == 0))
					{
						tmp = (int) WaveShort[i];
					}
					else if ((strcmp (WaveHeader->datatype, "i4") == 0) ||
							(strcmp (WaveHeader->datatype, "s4") == 0))
					{
						tmp = (int) WaveLong[i];
					}

					tmpf = fabs ((double) tmp - Rsam[index].values->DC_offset);

					for (j = 0; j < num_periods; j++)
					{
						Rsam[index].values->TP[j].rsam_value =
							Rsam[index].values->TP[j].rsam_value + tmpf;
						Rsam[index].values->TP[j].rsam_nsamp =
							Rsam[index].values->TP[j].rsam_nsamp + 1;
					}

				}

				if (Debug > 1)
					logit ("e", "%s: %f %d\n", scn, Rsam[index].values->TP[0].rsam_value,
						Rsam[index].values->TP[0].rsam_nsamp);



				/*
				 * Check elapsed time - if over time_period 
				 * prepare a new message a write it out
				 */
					
				for (j = 0; j < num_periods; j++)
				{
					if (Debug > 2)
						logit ("e", "%s: Period %f - %f-%f\n", scn,
							Rsam[index].values->TP[j].time_period,
							Rsam[index].values->TP[j].rsam_starttime, 
							WaveHeader->endtime);

			/* cheryl */
					if (Debug > 2)
                                                 logit ("e", "RSAM running total: %f, Number of samples: %d\n", Rsam[index].values->TP[j].rsam_value,Rsam[index].values->TP[j].rsam_nsamp);


                        /* cheryl added time syncing condition to the */
                        /*  following if statement.  If the the data  */
                        /*  packet is at a new minute mark but less   */
                        /*  than 60 seconds has elapsed, the buffers  */
                        /*  are written out and the averaging sums    */
                        /*  reinitialized. This has only been tested  */
                        /*  for 1 second data packets.                */
                 temp_time_period = Rsam[index].values->TP[j].time_period; 

					if  ( (((temp_time_period * 
              (WaveHeader->endtime/temp_time_period -
            (int)(WaveHeader->endtime/temp_time_period)))
            < packet_time_int) && (temp_time_period >= 60.0)) ||
 (((int)WaveHeader->endtime - 
				(int)Rsam[index].values->TP[j].rsam_starttime) >=
			        (Rsam[index].values->TP[j].time_period)) )  
 
					{

					/* Fill the header of the message */

						strcpy (RsamHeader->sta, WaveHeader->sta);
						if ( useTB2out ) {
							if ( useTB2in )
								strcpy (Rsam2Header->chan, Wave2Header->chan);
							else {
								strncpy (Rsam2Header->chan, WaveHeader->chan, TRACE2_CHAN_LEN-1 );
								Rsam2Header->chan[TRACE2_CHAN_LEN-1] = 0;
							}
							strcpy (Rsam2Header->loc, Rsam[index].values->TP[j].outloc);
						} else {
							strcpy (RsamHeader->chan, Rsam[index].values->TP[j].outchan);
						}
						strcpy (RsamHeader->net, WaveHeader->net);

						RsamHeader->starttime = WaveHeader->endtime;
						RsamHeader->endtime = WaveHeader->endtime;
						RsamHeader->nsamp = 1;
						RsamHeader->samprate = ((double) 1) / Rsam[index].values->TP[j].time_period;

#if defined _SPARC
						strcpy (RsamHeader->datatype, "s4");
							
#elif defined _INTEL
						strcpy (RsamHeader->datatype, "i4");
#else
	logit ("e", "ew2rsam: Must have either SPARC or INTEL defined during compilation; exitting!");
	ProcessorStatus = -1;
	KillSelfThread();
#endif

						RsamLong[0] = (int32_t)
								(Rsam[index].values->TP[j].rsam_value /
								(double) Rsam[index].values->TP[j].rsam_nsamp);
							
						if (Debug > 0)
							logit ("e", "%s: RSAM-%f (%f/%d) = %ld\n", 
								scn, 
								Rsam[index].values->TP[j].time_period, 
								Rsam[index].values->TP[j].rsam_value, 
								Rsam[index].values->TP[j].rsam_nsamp, 
								(long)RsamLong[0]);
	

						/* Reset all book-keeping counters */
						Rsam[index].values->TP[j].rsam_value = 0.0;
						Rsam[index].values->TP[j].rsam_nsamp = 0;
						Rsam[index].values->TP[j].rsam_starttime = TIME_INVALID;
	
	
						/* Write it OUT */
						reclogo.instid = InstId;
						reclogo.mod = MyModId;
						reclogo.type = TypeWaveformOut;
						
						if (tport_putmsg (&OutRegion, &reclogo, 
										RsamBufLen, RsamBuf) != PUT_OK)
						{
							logit ("et", "ew2rsam:  Error sending message: %d-%d-%d.\n",
									reclogo.instid, reclogo.mod, reclogo.type);
							ProcessorStatus = -1;
							KillSelfThread();
						} 
						                      
					} /* is it time to write this rsam value out? */
				} /* loop over time periods */
			} /* If we have a valid DC offset */

		} /* If we have a valid index */

		ProcessorStatus = 0;

	} /* while 1*/

	return THR_NULL_RET; /* should never be reached, unless someone inserts a break into the above loop! */
}
